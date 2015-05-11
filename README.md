FUSE Small File System
======================

`SmallFS` in a implementation of a FUSE driver for a small and simple file system.  The driver has been implemented to be use as part of the Operating System classes at UNITEC (Universidad Tecnologica Centroamericana http://www.unitec.edu).  The file system was designed by Michael Black (http://nw08.american.edu/~mblack/) from the American University.

File System Description
-----------------------
`SmallFS` is managed by two sectors at the beginning of the disk.  The Disk Map sits at sector 1, and the Directory sits at sector 2.  Block 3+ are used to store file data.

The Map tells which sectors are available and which sectors are currently used by files.  This makes it easy to find a free sector when writing a file.  Each sector on the disk is represented by one byte in the Map.  A byte entry of 0xFF means that the sector is used.  A byte entry of 0x00 means that the sector is free. 

The Directory lists the names and locations of the files.  There are 16 file entries in the Directory and each entry contains 32 bytes  (32 times 16 = 512, which is the storage capacity of a sector).  The first six bytes of each directory entry is the file name.  The remaining 26 bytes are sector numbers, which tell where the file is on the disk.  If the first byte of the entry is 0x0, then there is no file at that entry.

For example, a file entry of:

```
4B 45 52 4E 45 4C 03 04 05 06 00 00 00 00 00 00 00 00 00 00...
K  E  R  N  E  L
```

means that there is a valid file, with name “KERNEL”, located at sectors 3, 4, 5, 6.  (00 is not a valid sector number but a filler since every entry must be 32 bytes).

If a file name is less than 6 bytes, the remainder of the 6 bytes should be padded out with 00s.

You should note, by the way, that this file system is very restrictive.  Since one byte represents a sector, there can be no more than 256 sectors used on the disk (128kB of storage).  Additionally, since a file can have no more than 26 sectors, file sizes are limited to 13kB.  This is adequate storage for a small class project, but for a modern operating system, this would be grossly inadequate.
