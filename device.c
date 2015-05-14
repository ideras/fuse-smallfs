#include <stdlib.h>
#include <stdio.h>
#include "device.h"

static const char *device_path;
static FILE *f;

int device_open(const char *path) 
{
    device_path = path;
    f = fopen(path, "r+");
	
    return (f != NULL);
}

void device_close()
{
    fflush(f);
    fclose(f);
}

int device_read_sector(unsigned char buffer[], int sector) 
{
    fseek(f, sector*SECTOR_SIZE, SEEK_SET);
	
    return ( fread(buffer, 1, SECTOR_SIZE, f) == SECTOR_SIZE );
}

int device_write_sector(unsigned char buffer[], int sector) 
{
    fseek(f, sector*SECTOR_SIZE, SEEK_SET);
	
    return ( fwrite(buffer, 1, SECTOR_SIZE, f) == SECTOR_SIZE );
}

void device_flush()
{
    fflush(f);
}
