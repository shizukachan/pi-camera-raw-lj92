pi-camera-raw-lj92
==================
Lossless JPEG compressor for RAW portion of Raspberry Pi Camera RAW format.

Uses a modified version of the lj92 library found [here](https://bitbucket.org/baldand/mlrawviewer/src/master/liblj92/?at=master).

Makefile
========

If you are using a Pi Camera V1, run `make camera_v1`

If you are using a Pi Camera V2, run `make camera_v2`

If you only want to make the merge_jpg program, run `make lj92_merge`

merge_jpg is not camera version dependent.

Programs
========

split_jpg
---------

Designed to run on the Raspberry Pi. Splits a Raspberry Pi Camera JPEG+RAW file into the JPEG and the RAW portions, then compresses the RAW portion with Lossless JPEG (1992) format. This algorithm takes a fair bit of time to run on a Pi, so it's suggested to enable threaded lj92 execution on multicore Raspberry Pi systems.

To split a Raspberry Pi JPEG+RAW into a truncated JPEG and its compressed RAW portion (ljpg), run `split_jpg jpeg_file.jpg`. The original JPEG will be truncated to remove all compressed data. You may specify two-component or four-component compression by passing in `split_jpg jpeg_file.jpg 2` or `split_jpg jpeg_file.jpg 4`, respectively. Two-component compression is the default.

merge_jpg
---------

Designed to combine a JPEG+RAW file split into JPEG and LJ92-compressed RAW portion back into a JPEG+RAW file. This makes the output compatible with most other programs that manipulate Raspberry Pi Camera RAW files.

To merge a truncated JPEG and a ljpg produced by split_jpg, just place the ljpg in the same directory as the truncated JPEG file, then run `merge_jpg split_jpeg_file.jpg` This will recreate the split_jpeg_file.jpg with the same RAW data that was stripped off and compressed by split_jpg.

Design Decisions
================
Because this program needs to be fast, I've chosen to use #define to set the camera version and compile on the Raspberry Pi.

I've chosen to discard non-imaging RAW data in the original RAW. This includes unset padding pixels in the last vertical lines and the last few horizontal lines of RAW data. This reduces filesize without affecting imaging output.

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

Only the Lossless JPEG compression is threaded, and is split up per-component. Therefore, 4-component compression can be threaded better.

For a better explanation of "components", please see [this](https://thndl.com/how-dng-compresses-raw-data-with-lossless-jpeg92.html) article.

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

Compression performance is slightly worse than Adobe's DNG compressor.

