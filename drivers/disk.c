#include "disk.h"

static void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void insw(uint16_t port, uint16_t* buffer, uint32_t count) {
    asm volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

static void outsw(uint16_t port, uint16_t* buffer, uint32_t count) {
    asm volatile("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

static void io_wait(void) {
    for (volatile int i = 0; i < 100; i++);
}

static int ata_wait_bsy(void) {
    for (int i = 0; i < 10000000; i++) {
        uint8_t status = inb(0x1F7);
        if (!(status & 0x80)) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static int ata_wait_drq(void) {
    for (int i = 0; i < 10000000; i++) {
        uint8_t status = inb(0x1F7);
        if (status & 0x08) {
            return 1;
        }
        if (status & 0x01) {
            return 0;
        }
        io_wait();
    }
    return 0;
}

void disk_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer) {
    if (!ata_wait_bsy()) {
        for (int i = 0; i < sector_count * 512; i++) {
            buffer[i] = 0;
        }
        return;
    }

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    io_wait();

    for (int i = 0; i < sector_count; i++) {
        if (!ata_wait_bsy()) {
            for (int j = i * 512; j < sector_count * 512; j++) {
                buffer[j] = 0;
            }
            return;
        }
        
        if (!ata_wait_drq()) {
            for (int j = i * 512; j < sector_count * 512; j++) {
                buffer[j] = 0;
            }
            return;
        }
        
        insw(0x1F0, (uint16_t*)(buffer + i * 512), 256);
        io_wait();
    }
}

void disk_write_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer) {
    if (!ata_wait_bsy()) {
        return;
    }

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    io_wait();

    for (int i = 0; i < sector_count; i++) {
        if (!ata_wait_bsy()) {
            return;
        }
        
        if (!ata_wait_drq()) {
            return;
        }
        
        outsw(0x1F0, (uint16_t*)(buffer + i * 512), 256);
        io_wait();
    }

    if (ata_wait_bsy()) {
        outb(0x1F7, 0xE7);
        ata_wait_bsy();
    }
}