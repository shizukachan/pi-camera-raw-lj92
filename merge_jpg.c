// lj92decode.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lj92.h"

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

int main(int argc, char **argv)
{
	if (argc!=2)
	{
		puts("merge_jpg\nUsage: merge_jpg <truncated jpg file>\nRequires .ljpg files in current dir to reconstruct image file.\nNot all data will be restored - dummy data was discarded.\nOutput: Truncated jpg file with appended RAW data. You can use other utilities to process.");
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
	FILE *A = fopen(fn,"rb");
	FILE *B = fopen(argv[1],"r+b");
	if (B==NULL)
	{
		fprintf(stderr,"Can't open JPEG file: %s, dumping as RAW",argv[1]);
		B = fopen(argv[1],"wb");
	}
	RAWHEADER hdr;
	RAWPLANE raw[4];
	int fread_v = 0;
	if (A==NULL)
		goto bad_file;
	fread_v = fread(&hdr,sizeof(RAWHEADER),1,A);
	if (!fread_v)
		goto bad_file;
	if (!(hdr.MAGIC[0]=='L' && hdr.MAGIC[1]=='J' && hdr.MAGIC[2]=='P' && hdr.MAGIC[3]=='G'))
	{
		fprintf(stderr,"FATAL: Magic failed\n");
		goto bad_file;
	}
	for (int i=0;i<hdr.COMPONENTS;i++)
	{
		fread_v = fread(&raw[i],sizeof(RAWPLANE),1,A);
		if (!fread_v)
			goto bad_file;
	}
	unsigned char *Img10 = 0; //used as component data buffer initially, then later as destination buffer
	unsigned short *Img16 = 0;//intermediate representation buffer
	Img10 = (unsigned char*) malloc(hdr.V*((hdr.H*hdr.BPP/8)+hdr.PADDING)*sizeof(unsigned char));
	if (!Img10)
		goto memalloc_fail;
	//memset(Img10,0,hdr.V*((hdr.H*hdr.BPP/8)+hdr.PADDING));
	Img16 = (unsigned short*) malloc(hdr.V*hdr.H*sizeof(unsigned short));
	if (!Img16)
		goto memalloc_fail;
	memset(Img16,0,hdr.V*hdr.H*sizeof(unsigned short));
	lj92 decoderInstance;
	int a[3];//dummy variables
	for (int i=0;i<hdr.COMPONENTS;i++)
	{
		fread_v = fread(Img10,raw[i].SIZE,1,A);
		if (!fread_v)
			goto bad_file_m;
		lj92_open(&decoderInstance, Img10, raw[i].SIZE, &a[0], &a[1], &a[2]);
		lj92_decode(decoderInstance, Img16+raw[i].OFFSET, raw[i].H, 2*hdr.H-raw[i].H, NULL, 0);
		lj92_close(decoderInstance);
	}
	fclose(A);
	memset(Img10,0,hdr.V*((hdr.H*hdr.BPP/8)+hdr.PADDING));
	unsigned int j=0;
	for (int y=0;y<hdr.V;y++)
	{
		for (int x=0;x<hdr.H;x+=4)
		{
			unsigned char split = (Img16[y*hdr.H+x+0]&0x03)<<6 | (Img16[y*hdr.H+x+1]&0x03)<<4 | (Img16[y*hdr.H+x+2]&0x03)<<2 | (Img16[y*hdr.H+x+3]&0x03);
			Img10[j++] = Img16[y*hdr.H+x+0]>>2;
			Img10[j++] = Img16[y*hdr.H+x+1]>>2;
			Img10[j++] = Img16[y*hdr.H+x+2]>>2;
			Img10[j++] = Img16[y*hdr.H+x+3]>>2;
			Img10[j++] = split;
		}
		j+=hdr.PADDING;
	}
	free(Img16);
	fseek(B,0,SEEK_END);
	fwrite(Img10,hdr.V*((hdr.H*hdr.BPP/8)+hdr.PADDING),1,B);//re-append raw section to jpeg.
	memset(Img10,0,hdr.ENDPADDING);//need to pad the end of the file for compatibility reasons.
	fwrite(Img10,hdr.ENDPADDING,1,B);
	free(Img10);
	fclose(B);
	return 0;
memalloc_fail:
	fprintf(stderr,"out of memory");
	fclose(A);
	return -2;
bad_file_m:
	fprintf(stderr,"ljpg file truncated");
	free(Img16);
	free(Img10);
bad_file:
	fprintf(stderr,"ljpg file not readable");
	if (A!=NULL) fclose(A);
	return -1;
}
