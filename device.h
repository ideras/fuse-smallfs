#ifndef _DEVICE_H_
#define _DEVICE_H_

#define SECTOR_SIZE		        512

#ifdef __cplusplus
extern "C" {
#endif

int device_open(const char *path);
void device_close();
int device_read_sector(unsigned char buffer[], int sector);
int device_write_sector(unsigned char buffer[], int sector);
void device_flush();

#ifdef __cplusplus
}
#endif

#endif
