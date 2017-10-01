pi-camera-raw-lj92
==================
Lossless JPEG compressor for RAW portion of Raspberry Pi Camera RAW format.

Uses a modified version of the lj92 library found [here](https://bitbucket.org/baldand/mlrawviewer/src/master/liblj92/?at=master).

Programs
========

split_jpg
---------

Designed to run on the Raspberry Pi. Splits a Raspberry Pi Camera JPEG+RAW file into the JPEG and the RAW portions, then compresses the RAW portion with Lossless JPEG (1992) format. This algorithm takes a fair bit of time to run on a Pi, so it's suggested to enable threaded lj92 execution on multicore Raspberry Pi systems. Because this program needs to be fast, I've chosen to use #define to set the camera version and compile on the Raspberry Pi.

merge_jpg
---------

Designed to combine a JPEG+RAW file split into JPEG and LJ92-compressed RAW portion back into a JPEG+RAW file. This makes the output compatible with most other programs that manipulate Raspberry Pi Camera RAW files.

Makefile
========

If you are using a Pi Camera V1, run `make camera_v1`

If you are using a Pi Camera V2, run `make camera_v2`

If you only want to make the merge_jpg program, run `make merge_jpg`

merge_jpg is not camera version dependent.

Recommended Compilation Options
===============================

If you are going to be using a multi-core Raspberry Pi, use this:

    gcc lj92.c split_jpg.c -o split_jpg --std=gnu99 -O3 -march=native -funroll-loops -D${camera_version} -DTHREADED_LJ92 -lpthread
**Using -march=native on Raspbian may trigger a compiler bug on the Raspberry Pi 3 (or Pi 2B2); use this instead:**

    gcc lj92.c split_jpg.c -o split_jpg --std=gnu99 -O3 -mcpu=cortex-a53 -funroll-loops -D${camera_version} -DTHREADED_LJ92 -lpthread

If you are using a Raspberry Pi A/B(+)/Zero (W):

    gcc lj92.c split_jpg.c -o split_jpg --std=gnu99 -O3 -march=native -funroll-loops -D${camera_version}

Performance
===========

Only the Lossless JPEG compression is threaded, and is split up per-component.

Raspberry Pi 3, Camera V1 RAW file
----------------------------------

* Single thread, 2 components: ~619ms
* Multithreaded, 2 components: ~338ms (1.83x)
* Single thread, 4 components: ~619ms
* Multithreaded, 4 components: ~241ms (2.57x)

Raspberry Pi Zero, Camera V1 RAW file
-------------------------------------

* Single thread, 2 components: ~1072ms
* Single thread, 4 components: ~1059ms

Compression
===========

Tests were done on an ISO100, ISO400, and ISO800 source.

* 2 components: 70.7%, 73.6%, 84.8%
* 4 components: 70.1%, 73.8%, 84.4%

Performance is slightly worse than Adobe's DNG compressor.

