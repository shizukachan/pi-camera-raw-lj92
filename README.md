# pi-camera-raw-lj92
Lossless JPEG compressor for RAW portion of Raspberry Pi Camera RAW format.

Programs:

split_jpg - designed to run on the Raspberry Pi. Splits a Raspberry Pi Camera JPEG+RAW file into the JPEG and the RAW portions, then compresses the RAW portion with Lossless JPEG (1992) format. This algorithm takes a fair bit of time to run on a Pi, so it's suggested to enable threaded lj92 execution on multicore Raspberry Pi systems. Because this program needs to be fast, Raspberry Pi Camera versions are compiled into the program.
merge_jpg - designed to combine a JPEG+RAW file split into JPEG and LJ92-compressed RAW portion back into a JPEG+RAW file. This makes the output compatible with most other programs that manipulate Raspberry Pi Camera RAW files.

Makefile:

make camera_v1 
make camera_v2

Recommended compilation command for split_jpg on a Raspberry Pi 3 (or 2 with BCM2837):
gcc -O3 lj92.c split_jpg.c -o split_jpg --std=gnu99 -mcpu=cortex-a53 -funroll-loops -D${camera_version} -DTHREADED_LJ92

Recommended compilation command for split_jpg on a Raspberry Pi 2:
gcc -O3 lj92.c split_jpg.c -o split_jpg --std=gnu99 -march=native -funroll-loops -D${camera_version} -DTHREADED_LJ92
**Running this on Raspbian may trigger a compiler bug**

Recommended compilation command for split_jpg on a Raspberry Pi A/B(+)/Zero (W):
gcc -O3 lj92.c split_jpg.c -o split_jpg --std=gnu99 -march=native -funroll-loops
If you are doing other things, it may be useful to append 'nice -n 19 ' before the compilation step.
