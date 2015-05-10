#include "sfs.h"

#include <fuse.h>
#include <stdio.h>

struct fuse_operations sfs_oper;

int main(int argc, char *argv[]) {
	int i, fuse_stat;

	sfs_oper.getattr = sfs_getattr;
	sfs_oper.readlink = sfs_readlink;
	sfs_oper.getdir = NULL;
	sfs_oper.mknod = sfs_mknod;
	sfs_oper.mkdir = sfs_mkdir;
	sfs_oper.unlink = sfs_unlink;
	sfs_oper.rmdir = sfs_rmdir;
	sfs_oper.symlink = sfs_symlink;
	sfs_oper.rename = sfs_rename;
	sfs_oper.link = sfs_link;
	sfs_oper.chmod = sfs_chmod;
	sfs_oper.chown = sfs_chown;
	sfs_oper.truncate = sfs_truncate;
	sfs_oper.utime = sfs_utime;
	sfs_oper.open = sfs_open;
	sfs_oper.read = sfs_read;
	sfs_oper.write = sfs_write;
	sfs_oper.statfs = sfs_statfs;
	sfs_oper.flush = sfs_flush;
	sfs_oper.release = sfs_release;
	sfs_oper.fsync = sfs_fsync;
	sfs_oper.opendir = sfs_opendir;
	sfs_oper.readdir = sfs_readdir;
	sfs_oper.releasedir = sfs_releasedir;
	sfs_oper.fsyncdir = sfs_fsyncdir;
	sfs_oper.init = sfs_init;

	printf("mounting file system...\n");
	
	for(i = 1; i < argc && (argv[i][0] == '-'); i++) {
		if(i == argc) {
			return (-1);
		}
	}

	//realpath(...) returns the canonicalized absolute pathname
	if (!device_open(realpath(argv[i], NULL)) ) {
	    printf("Cannot open device file %s\n", argv[i]);
	    return 1;
	}

	for(; i < argc; i++) {
		argv[i] = argv[i+1];
	}
	argc--;

	fuse_stat = fuse_main(argc, argv, &sfs_oper, NULL);

    device_close();
    
	printf("fuse_main returned %d\n", fuse_stat);

	return fuse_stat;
}


