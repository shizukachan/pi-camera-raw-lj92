CFLAGS = -g -Wpointer-sign -Wunused-variable -std=gnu99 -Wunused-but-set-variable

all:
	@echo Please specify camera_v1 or camera_v2.

camera_v1: CAMERA_VERSION = CAMERA_V1
camera_v1: lj92_split lj92_merge

camera_v2: CAMERA_VERSION = CAMERA_V2
camera_v2: lj92_split lj92_merge

lj92_split: split_jpg.c lj92.c lj92.h
	gcc ${CFLAGS} -D${CAMERA_VERSION} lj92.c split_jpg.c -o split_jpg

lj92_merge: merge_jpg.c lj92.c lj92.h
	gcc ${CFLAGS} lj92.c merge_jpg.c -o merge_jpg

clean:
	rm split_jpg merge_jpg
