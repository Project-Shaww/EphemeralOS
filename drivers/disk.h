#ifndef DISK_H
#define DISK_H

#include "../kernel/kernel.h"

void disk_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer);
void disk_write_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer);

#endif
