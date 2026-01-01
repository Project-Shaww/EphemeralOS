// drivers/pci.h
#ifndef PCI_H
#define PCI_H

#include "../kernel/kernel.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_CLASS_CODE 0x0B
#define PCI_SUBCLASS 0x0A
#define PCI_PROG_IF 0x09
#define PCI_HEADER_TYPE 0x0E
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C

// Network controller class
#define PCI_CLASS_NETWORK 0x02

// Vendor IDs
#define PCI_VENDOR_INTEL 0x8086
#define PCI_VENDOR_REALTEK 0x10EC

// Device IDs
#define PCI_DEVICE_INTEL_E1000 0x100E
#define PCI_DEVICE_INTEL_E1000E 0x10D3
#define PCI_DEVICE_RTL8139 0x8139

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t class_code;
    uint8_t subclass;
    uint32_t bar0;
    uint32_t bar1;
    uint8_t irq;
} pci_device_t;

void pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev);
int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* dev);

#endif