#include "device.h"
#include "sfs.h"

unsigned char map[SECTOR_SIZE];
unsigned char root_dir_block[SECTOR_SIZE];
int root_dir_need_update, root_dir_loaded;
struct dir_entry *root_dir = (struct dir_entry *)root_dir_block;

void dump_hex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

void sfs_ensure_root_dir_loaded()
{
    if (!root_dir_loaded) {
        
        device_read_sector(map, 1);
        device_read_sector(root_dir_block, 2);
        
        root_dir_loaded = 1;
        root_dir_need_update = 0;
        
        printf("Loading root directory ...\n");
        dump_hex(map, 16);
        dump_hex(root_dir_block, 64);
    }
}

struct dir_entry *sfs_lookup_root_dir(const char *d_name) 
{
    int i = 0;

    sfs_ensure_root_dir_loaded();
    while (i<MAX_ROOT_DENTRIES) {
        
        if (root_dir[i].name[0] != '\0') {
            if (strncmp(d_name, root_dir[i].name, MAX_NAME_LEN) == 0)
                return &root_dir[i];
        }
        
        i++;
    }
    
    return NULL;
}

void sfs_get_file_stat(struct dir_entry *dentry, int *size, int *block_count)
{
    int i;
    
    *size = 0;
    *block_count = 0;
    
    for (i = 0; dentry->sectors[i] != 0; i++) {
        (*size) += SECTOR_SIZE;
        (*block_count)++;
    }
}

struct file_info *sfs_load_file(struct dir_entry *dentry)
{
    struct file_info *fi = (struct file_info *)malloc(sizeof(struct file_info));
    unsigned char *ptr_data = fi->f_data;
    
    if (fi == NULL)
        return NULL;
    
    int i = 0, size = 0;
    
    memset(fi, 0, sizeof(struct file_info));
    
    for (; dentry->sectors[i] != 0; i++) {
        int sector = dentry->sectors[i];
        
        device_read_sector(ptr_data, sector);
        size += SECTOR_SIZE;
        ptr_data += SECTOR_SIZE;
    }
    
    fi->f_dentry = dentry;
    fi->f_size = size;
    
    return fi;
}

void sfs_remove_file(struct dir_entry *dentry)
{
    int i;
    
    for (i = 0; dentry->sectors[i] != 0; i++) {
        int sector = dentry->sectors[i];
        
        map[sector] = 0x0;
        
        printf("Releasing sector %d\n", sector);
    }
    
    memset(dentry, 0, sizeof(struct dir_entry));
    
    root_dir_need_update = 1;
}

int sfs_get_free_sector()
{
    int i = 0;
    
    while ( (map[i] == 0xFF) && (i < MAX_MAP_ENTRIES) )
        i++;
    
    return (map[i] == 0xFF)? -1 : i;
}

int sfs_update_file_info(struct file_info *fi)
{
    int i = 0, size = fi->f_size;
    struct dir_entry *dentry = fi->f_dentry;
    unsigned char *ptr_data = fi->f_data;
    
    printf("%s: size=%d\n", __FUNCTION__, size);
    
    while (size > 0) {
        int sector = dentry->sectors[i];
        
        if (sector == 0) {
            //We need a new sector
            sector = sfs_get_free_sector();
            
            if (sector == -1) 
                return 0;
            
            dentry->sectors[i] = sector;
            map[sector] = 0xFF;
        }
        
        printf("Writing device sector %d\n", sector);
        device_write_sector(ptr_data, sector);
        
        size -= SECTOR_SIZE;
        ptr_data += SECTOR_SIZE;
        i++;
    }
    
    return 1;
}

void sfs_get_root_dir_info(int *file_count, int *free_dentries)
{
    int i = 0;

    *file_count = 0;
    *free_dentries = 0;
    
    sfs_ensure_root_dir_loaded();
        
    while (i<MAX_ROOT_DENTRIES) {
        
        if (root_dir[i].name[0] != '\0') {
            (*file_count)++;
        } else {
            (*free_dentries)++;
        }
        
        i++;
    }
    
    printf("File count: %d Free entries: %d\n", *file_count, *free_dentries);
}

int sfs_count_free_blocks()
{
    int i, free_block_count;
    
    free_block_count = 0;
    for (i=0; i<MAX_MAP_ENTRIES; i++)  {
        if (map[i] == 0x0) free_block_count++;
    }
    
    return free_block_count;
}

int sfs_getattr(const char *path, struct stat *statbuf) 
{
    int path_len = strlen(path);
    printf("%s: %s\n", __FUNCTION__, path);
    
    if ( (path_len == 1) && path[0] == '/') {
        statbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_nlink = 1;
        statbuf->st_ino = 0;
        statbuf->st_size = SECTOR_SIZE;
        statbuf->st_blksize = SECTOR_SIZE;
        statbuf->st_blocks = 1;
    } else {
        struct dir_entry *dentry = sfs_lookup_root_dir(&path[1]);
        int size, block_count;
        
        if (dentry == NULL)
            return -ENOENT;
        
        sfs_get_file_stat(dentry, &size, &block_count);
        
        statbuf->st_mode = (S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO) & ~S_IXUSR & ~S_IXGRP & ~S_IXOTH;
        statbuf->st_nlink = 1;
        statbuf->st_ino = (ino_t)(dentry - root_dir);
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;
        statbuf->st_size = size;
        statbuf->st_blksize = SECTOR_SIZE;              /* blocksize for filesystem I/O */
        statbuf->st_blocks = block_count;               /* number of 512B blocks allocated */
    }
    
    return 0;
}

int sfs_readlink(const char *path, char *link, size_t size) {
    return -EPERM;
}

int sfs_mknod(const char *path, mode_t mode, dev_t dev) 
{
    printf("mknod(path=%s, mode=%d)\n", path, mode);
    
    if ( S_ISREG(mode) ) {
        int i = 0;
        struct dir_entry *dentry;
        
        root_dir_need_update = 1;
        printf("Creating file %s\n", &path[1]);
        
        sfs_ensure_root_dir_loaded();
        
        //Look up a empty directory entry
        while (root_dir[i].name[0] != '\0' && i<MAX_ROOT_DENTRIES) i++;
        
        if (root_dir[i].name[0] != '\0')
            return -ENOSPC;
        
        printf("Creating file %d\n", i);
        
        dentry = &root_dir[i];
        strncpy(dentry->name, &path[1], MAX_NAME_LEN);
        
        for (i=0; i<MAX_SECTORS_PER_FILE; i++)
            dentry->sectors[i] = 0;
        
        dump_hex(root_dir_block, 64);
        
        return 0;
    }
    return -EPERM;
}

int sfs_mkdir(const char *path, mode_t mode) {
    printf("**mkdir(path=%s, mode=%d)\n", path, (int)mode);
    return -EPERM;
}

int sfs_unlink(const char *path) 
{
    struct dir_entry *de = sfs_lookup_root_dir(&path[1]);
    
    printf("unlink(path=%s)\n", path);
    
    if (de == NULL)
        return -ENOENT;
    
    sfs_remove_file(de);
    
    return 0;
}

int sfs_rmdir(const char *path) {
    printf("rmdir(path=%s\n)", path);
    return -EPERM;
}

int sfs_symlink(const char *path, const char *link) {
    printf("symlink(path=%s, link=%s)\n", path, link);
    return -EPERM;
}

int sfs_rename(const char *path, const char *newpath) 
{
    struct dir_entry *de = sfs_lookup_root_dir(&path[1]);
    
    printf("rename(path=%s, newPath=%s)\n", path, newpath);
    if (de == NULL)
        return -ENOENT;
    
    root_dir_need_update = 1;
    strncpy(de->name, &newpath[1], 6);
    
    return 0;
}

int sfs_link(const char *path, const char *newpath) {
	printf("link(path=%s, newPath=%s)\n", path, newpath);
    return -EPERM;
}

int sfs_chmod(const char *path, mode_t mode) {
	printf("chmod(path=%s, mode=%d)\n", path, mode);
    return -EPERM;
}

int sfs_chown(const char *path, uid_t uid, gid_t gid) {
	printf("chown(path=%s, uid=%d, gid=%d)\n", path, (int)uid, (int)gid);
    return -EPERM;
}

int sfs_truncate(const char *path, off_t newSize) 
{
	struct dir_entry *de = sfs_lookup_root_dir(&path[1]);
    
    printf("truncate(path=%s, newSize=%d\n", path, (int)newSize);
    
    if (de == NULL)
        return -ENOENT;
    
    if (newSize == 0) {
        int i;
        
        for (i=0; de->sectors[i] != 0; i++) {
            int sector = de->sectors[i];
            de->sectors[i] = 0;
            map[sector] = 0;
            printf("Releasing sector %d\n", sector);
        }
        root_dir_need_update = 1;
    }
    
    return 0;
}

int sfs_utime(const char *path, struct utimbuf *ubuf) {
	printf("utime(path=%s)\n", path);
    return -EPERM;
}

int sfs_open(const char *path, struct fuse_file_info *fileInfo) 
{
    struct dir_entry *de = sfs_lookup_root_dir(&path[1]);
    
    printf("open(path=%s)\n", path);
    if (de == NULL)
        return -ENOENT;
    
    struct file_info *fi = sfs_load_file(de);
    
    if (fi == NULL)
        return -ENOMEM;
    
    fileInfo->fh = (uint64_t)fi;
    fi->need_update = 0;
    
    return 0;
}

int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) 
{
    struct file_info *fi = (struct file_info *)fileInfo->fh;
    int bytes_to_read = size;

    printf("read(path=%s, size=%d, offset=%d, f_size=%d)\n", path, (int)size, (int)offset, fi->f_size);
    
    if (offset >= fi->f_size)
        return 0;
    
    if (offset + size > fi->f_size)
        bytes_to_read = fi->f_size - offset;
    
    memcpy(buf, &fi->f_data[offset], size);
    
    return bytes_to_read;
}

int sfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) 
{
    struct file_info *fi = (struct file_info *)fileInfo->fh;

    printf("write(path=%s, size=%d, offset=%d, f_size=%d)\n", path, (int)size, (int)offset, fi->f_size);
    
    if (offset + size > fi->f_size) {
        if (offset + size > MAX_FILE_SIZE)
            return -EFBIG;
        
        fi->f_size = offset + size;
    }

    fi->need_update = 1;
    memcpy(&fi->f_data[offset], buf, size);
    printf("write(path=%s, size=%d, offset=%d, f_size=%d)\n", path, (int)size, (int)offset, fi->f_size);

    return size;
}

int sfs_statfs(const char *path, struct statvfs *statInfo) 
{
	printf("statfs(path=%s)\n", path);
    int file_count, free_dentries;
    
    sfs_get_root_dir_info(&file_count, &free_dentries);
    
    statInfo->f_bsize = SECTOR_SIZE;    /* filesystem block size */
    statInfo->f_frsize = SECTOR_SIZE;   /* fragment size */
    statInfo->f_blocks = 2880;          /* size of fs in f_frsize units */
    statInfo->f_bfree = sfs_count_free_blocks();    /* # free blocks */
    statInfo->f_bavail = sfs_count_free_blocks();   /* # free blocks for unprivileged users */
    statInfo->f_files = MAX_ROOT_DENTRIES;          /* # inodes */
    statInfo->f_ffree = free_dentries;              /* # free inodes */
    statInfo->f_favail = free_dentries;             /* # free inodes for unprivileged users */
    statInfo->f_fsid = 0;                   /* filesystem ID */
    statInfo->f_flag = 0;                   /* mount flags */
    statInfo->f_namemax = MAX_NAME_LEN;     /* maximum filename length */
    
    return 0;
}

int sfs_flush(const char *path, struct fuse_file_info *fileInfo) 
{
    struct file_info *fi = (struct file_info *)fileInfo->fh;
    
    printf("flush(path=%s)\n", path);
    
    if (fi->need_update) {
        if (!sfs_update_file_info(fi))
            return -ENOSPC;
    }

    return 0;
}

int sfs_release(const char *path, struct fuse_file_info *fileInfo) 
{
    struct file_info *fi = (struct file_info *)fileInfo->fh;
    
    printf("release(path=%s)\n", path);
    
    free(fi);
    fileInfo->fh = 0;
    
    return 0;
}

int sfs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
	printf("fsync(path=%s, datasync=%d\n", path, datasync);
    return -EPERM;
}

int sfs_opendir(const char *path, struct fuse_file_info *fileInfo) 
{
    int path_len = strlen(path);

    printf("%s: %s\n", __FUNCTION__, path);
    
    if (root_dir_loaded)
        return 0;

    if ((path_len != 1) && path[0] != '/')
        return -EPERM;
    
    if (!device_read_sector(root_dir_block, 2))
        return -ENOENT;
    
    root_dir_need_update = 0;
    root_dir_loaded = 1;
    
    dump_hex(root_dir_block, 64);
    
    return 0;    
}

int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
    int i = 0;
    char name[MAX_NAME_LEN+1];

    printf("readdir(path=%s, offset=%d)\n", path, (int)offset);
    
    while (i<MAX_ROOT_DENTRIES) {
        
        if (root_dir[i].name[0] != '\0') {
            strncpy(name, root_dir[i].name, MAX_NAME_LEN);
            name[MAX_NAME_LEN] = '\0';
        
            if( filler(buf, name, NULL, 0) != 0) 
                return -ENOMEM;
        }
        
        i++;
    }
    
    return 0;
}

int sfs_releasedir(const char *path, struct fuse_file_info *fileInfo) {
    int path_len = strlen(path);

    printf("%s: %s\n", __FUNCTION__, path);

    if ((path_len != 1) && path[0] != '/')
        return -EPERM;
    
    if (root_dir_need_update) {
        printf("Writing root directory to disc...\n");

        device_write_sector(map, 1);
        device_write_sector(root_dir_block, 2);
        root_dir_need_update = 0;
    }
    
    return 0;
}

int sfs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo) {
    printf("%s: %s\n", __FUNCTION__, path);
    return -EPERM;
}

void sfs_init(struct fuse_conn_info *conn) {
    printf("%s\n", __FUNCTION__);
    return;
}

