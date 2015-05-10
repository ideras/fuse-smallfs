FUSE Small File System
======================

`SmallFS` in a implementation of a FUSE driver for a simple file system.  This file system has been originally designed to be use as part of the Operating System classes al UNITEC (Universidad Tecnologica Centroamericana http://www.unitec.edu).

SmallFS Description
===================

`SmallFS` is supports only a root directory qith 16 entries, which translate in 16 files available to use. The files can have at most 26 blocks, 512B each, so this mean the files can be at most 13K.
Each entry have 32 bytes, the first 6 bytes is the file name, the rest 26 bytes are used to index blocks.
The free blocks are handled using a map, which is basically an array of 256 bytes, a 0 value mean the block is free, 255 mean the block is used.

