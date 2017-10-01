#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lj92.h"

#ifdef CAMERA_V1
#define RAWSIZE 6404096
#define RAW_H 2592
#define RAW_V 1944
#define RAW_PAD 24
#endif
#ifdef CAMERA_V2
#define RAWSIZE 10270208
#define RAW_H 3280
#define RAW_V 2464
#define RAW_PAD 28
#endif
//v1: compile using gcc -O3 -DRAWSIZE=6404096 -DRAW_H=2592 -DRAW_V=1944 -DRAW_PAD=24
//v2: compile using gcc -O3 -DRAWSIZE=10270208 -DRAW_H=3280 -DRAW_V=2464 -DRAW_PAD=28
#ifndef RAWSIZE
#error Please specify -DRAWSIZE=6404096 or -DRAWSIZE=10270208 on gcc compile command.
#endif
#ifndef RAW_H
#error Please specify -DRAW_H=3280 or -DRAW_H=2592 on gcc compile command.
#endif
#ifndef RAW_V
#error Please specify -DRAW_V=2464 or -DRAW_V=1944 on gcc compile command.
#endif
#ifndef RAW_PAD
#error Please specify -DRAW_PAD=28 or -DRAW_PAD=24 on gcc compile command.
#endif

#define RAW_BITS 10
#define RAW_ROW  ((RAW_H*RAW_BITS/8)+RAW_PAD)

#ifdef THREADED_LJ92
#include <pthread.h>
struct LJ92_PARAM
{
	unsigned short *image;
	int width;
	int height;
	int bitdepth;
	int readLength;
	int skipLength;
	unsigned char **output;
	int *outputLength;
} typedef LJ92_PARAM;

void *lj92_thread(void *param)
{
	LJ92_PARAM *td = (LJ92_PARAM*) param;
	return lj92_encode(td->image,td->width,td->height,td->bitdepth,td->readLength,td->skipLength,NULL,0, td->output, td->outputLength);
};
#endif

struct RAWHEADER
{
        char MAGIC[4];
        unsigned int H;
        unsigned int V;
        unsigned int BPP;
        unsigned int PADDING;
	unsigned int COMPONENTS;
	unsigned int ENDPADDING;
	unsigned int dummy2;
} typedef RAWHEADER;
struct RAWPLANE
{
	unsigned int SIZE;
	unsigned int OFFSET;
	unsigned int H;
	unsigned int V;
} typedef RAWPLANE;

const char match[8] = "BRCM";
int main(int argc, char **argv)
{
	unsigned int components = 2;
	unsigned char *img = 0;
	unsigned short *img16 = 0;
	if (argc<2 || argc>=4)
	{
		puts("usage: split_jpg <jpeg_file_with_raw_data> <optional: number of components (2,4)>");
		return -1;
	}
	if (argc==3)
	{
		components = atoi(argv[2]);
		if ((components != 2) && (components != 4))
		{
			fprintf(stderr,"invalid number of components (2,4)");
			return -1;
		}
	}
        FILE *A = fopen(argv[1],"r+b");
	if (A==NULL)
	{
		fprintf(stderr,"can't open input file");
		return -1;
	}
	char fn[256];
	strcpy(fn,argv[1]);
	char *fn_extension = 0;
	if (strstr(fn,"."))
	{
		for (int n=strlen(fn)-1;n>0;n--)
		{
			if (fn[n]=='.')
			{
				fn_extension = fn+n;
				break;
			}
		}
	}
	if (!fn_extension)
	{
		strcat(fn,".ljpg");
	}
	else
	{
		*fn_extension = 0;
		strcat(fn,".ljpg");
	}
        fseek(A,0,SEEK_END);
        long fl=ftell(A);
        if (fl < RAWSIZE)
        {
                fprintf(stderr,"file too short");
                return -2;
        }
        long offset = fl - RAWSIZE;
        fseek(A,offset,SEEK_SET);
        char buf[8];
        fread(buf,1,4,A);
        if (!strncmp(match, buf, 4))
        {
		/* process it */
		fseek(A,(32768-4),SEEK_CUR);
		img = malloc(RAW_ROW*RAW_V*sizeof(unsigned char));
		img16 = malloc(RAW_H*RAW_V*sizeof(unsigned short));
		if (!img || !img16)
		{
			fprintf(stderr,"insufficient memory");
			return -1;
		}
		fread(img,RAW_ROW,RAW_V,A);
		unsigned char *output[4]={0,0,0,0};
		int output_length[4]={0,0,0,0};
		unsigned int j = 0;  // offset into buffer
		for (int row=0; row < RAW_V; row++) {  // iterate over pixel rows
			for (int col = 0; col < RAW_H; col+= 4) {  // iterate over pixel columns
//				refactored: original code was inefficient with memory access.
				unsigned short a, b, c, d, e;
				a=img[j++];
				b=img[j++];
				c=img[j++];
				d=img[j++];
				e=img[j++];
				img16[row*RAW_H+col+0] = (a << 2) | ((e & 0b11000000)>>6);
				img16[row*RAW_H+col+1] = (b << 2) | ((e & 0b00110000)>>4);
				img16[row*RAW_H+col+2] = (c << 2) | ((e & 0b00001100)>>2);
				img16[row*RAW_H+col+3] = (d << 2) | ((e & 0b00000011));

			}
			j+=RAW_PAD;
		}
		free(img);
		int retVal = 0;
		RAWHEADER hdr;
		RAWPLANE raw[4];
#ifdef THREADED_LJ92
		pthread_t threads[4];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		LJ92_PARAM thread_data[4];
		memset(thread_data,0,sizeof(LJ92_PARAM)*4);
#endif
		if (components == 2) //single core, higher compression
		{
#ifdef THREADED_LJ92
			int err;
			for(int i=0; i<2; i++)
			{
				thread_data[i].image       = img16+i*RAW_H;
				thread_data[i].width       = RAW_H;
				thread_data[i].height      = RAW_V/2;
				thread_data[i].bitdepth    = RAW_BITS;
				thread_data[i].readLength  = RAW_H;
				thread_data[i].skipLength  = RAW_H;
				thread_data[i].output      = &output[i];
				thread_data[i].outputLength= &output_length[i];
				err = pthread_create(&threads[i], &attr, &lj92_thread, (void *) &thread_data[i]);
				if (err)
				{
					fprintf(stderr,"BUG: pthread_create fail: %d",err);
					return -3;
				}
			}
			pthread_attr_destroy(&attr);
			retVal = 0;
			for(int i=0; i<2; i++)
			{
				void *tempRetVal;
				err = pthread_join(threads[i], tempRetVal);
				retVal |= (int)tempRetVal;
				if (err)
				{
					fprintf(stderr,"BUG: pthread_join fail: %d",err);
					return -3;
				}
			}
#else
			retVal  = lj92_encode(img16      ,RAW_H,RAW_V/2,RAW_BITS,RAW_H,RAW_H,NULL,0, &output[0], &output_length[0]);
			retVal |= lj92_encode(img16+RAW_H,RAW_H,RAW_V/2,RAW_BITS,RAW_H,RAW_H,NULL,0, &output[1], &output_length[1]);
#endif
			if (retVal!=0)
			{
				fprintf(stderr,"lj92 returned error %08x",retVal);
				return -3;
			}
#ifdef DEBUG
			printf("totalsize=%u (%2.4f%%)\n",output_length[0]+output_length[1],100.0f*(float)(output_length[0]+output_length[1])/((float)(RAW_H*RAW_V*RAW_BITS/8)));
#endif
			raw[0].SIZE = output_length[0];
			raw[0].OFFSET = 0;
			raw[0].H = RAW_H;
			raw[0].V = RAW_V/2;
			raw[1].SIZE = output_length[1];
			raw[1].OFFSET = RAW_H;
			raw[1].H = RAW_H;
			raw[1].V = RAW_V/2;
		}
		else if (components == 4)
		{
#ifdef THREADED_LJ92
			int err;
			for(int i=0; i<4; i++)
			{
				thread_data[i].image       = img16+i*RAW_H/2;
				thread_data[i].width       = RAW_H/2;
				thread_data[i].height      = RAW_V/2;
				thread_data[i].bitdepth    = RAW_BITS;
				thread_data[i].readLength  = RAW_H/2;
				thread_data[i].skipLength  = 3*RAW_H/2;
				thread_data[i].output      = &output[i];
				thread_data[i].outputLength= &output_length[i];
				err = pthread_create(&threads[i], &attr, &lj92_thread, (void *) &thread_data[i]);
				if (err)
				{
					fprintf(stderr,"BUG: pthread_create fail: %d",err);
					return -3;
				}
			}
			pthread_attr_destroy(&attr);
			retVal = 0;
			for(int i=0; i<4; i++)
			{
				void *tempRetVal;
				err = pthread_join(threads[i], tempRetVal);
				retVal |= (int)tempRetVal;
				if (err)
				{
					fprintf(stderr,"BUG: pthread_join fail: %d",err);
					return -3;
				}
			}
#else
			retVal  = lj92_encode(img16          ,RAW_H/2,RAW_V/2,RAW_BITS,RAW_H/2,3*RAW_H/2,NULL,0, &output[0], &output_length[0]);
			retVal |= lj92_encode(img16+RAW_H/2  ,RAW_H/2,RAW_V/2,RAW_BITS,RAW_H/2,3*RAW_H/2,NULL,0, &output[1], &output_length[1]);
			retVal |= lj92_encode(img16+RAW_H    ,RAW_H/2,RAW_V/2,RAW_BITS,RAW_H/2,3*RAW_H/2,NULL,0, &output[2], &output_length[2]);
			retVal |= lj92_encode(img16+3*RAW_H/2,RAW_H/2,RAW_V/2,RAW_BITS,RAW_H/2,3*RAW_H/2,NULL,0, &output[3], &output_length[3]);
#endif
#ifdef DEBUG
			printf("totalsize=%u (%2.4f%%)\n",output_length[0]+output_length[1]+output_length[2]+output_length[3],100.0f*(float)(output_length[0]+output_length[1]+output_length[2]+output_length[3])/((float)(RAW_H*RAW_V*RAW_BITS/8)));
#endif
			if (retVal!=0)
			{
				fprintf(stderr,"lj92 returned error %08x",retVal);
				return -3;
			}
			raw[0].SIZE = output_length[0];
			raw[0].OFFSET = 0;
			raw[0].H = RAW_H/2;
			raw[0].V = RAW_V/2;
			raw[1].SIZE = output_length[1];
			raw[1].OFFSET = RAW_H/2;
			raw[1].H = RAW_H/2;
			raw[1].V = RAW_V/2;
			raw[2].SIZE = output_length[2];
			raw[2].OFFSET = RAW_H;
			raw[2].H = RAW_H/2;
			raw[2].V = RAW_V/2;
			raw[3].SIZE = output_length[3];
			raw[3].OFFSET = 3*RAW_H/2;
			raw[3].H = RAW_H/2;
			raw[3].V = RAW_V/2;
		}
		free(img16);
		hdr.MAGIC[0]='L';
		hdr.MAGIC[1]='J';
		hdr.MAGIC[2]='P';
		hdr.MAGIC[3]='G';
		hdr.H = RAW_H;
		hdr.V = RAW_V;
		hdr.BPP = RAW_BITS;
		hdr.PADDING = RAW_PAD;
		hdr.COMPONENTS = components;
		hdr.ENDPADDING = RAWSIZE-j-32768;
		hdr.dummy2 = 0;
		FILE *O = fopen(fn,"wb");
		fwrite(&hdr,sizeof(hdr),1,O);
		fwrite(&raw[0],sizeof(raw[0]),components,O);
		for (int c=0;c<components;c++)
			fwrite(output[c],output_length[c],1,O);
		fclose(O);
                fseek(A,0,SEEK_SET);
                ftruncate(fileno(A), offset+32768);//keep header elsewhere
#ifdef THREADED_LJ92
		pthread_exit(NULL);
#endif
                fclose(A);
        }
        else
        {
                fprintf(stderr,"raw data signature not found");
                return -2;
        }
        return 0;
}

