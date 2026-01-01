// drivers/pci.c
#include "pci.h"

static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pci_init(void) {
    // PCI ya está inicializado por el BIOS
}

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC) |
        0x80000000
    );
    
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)(
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC) |
        0x80000000
    );
    
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t config = pci_read_config(bus, device, function, 0);
                uint16_t vendor = config & 0xFFFF;
                uint16_t dev_id = (config >> 16) & 0xFFFF;
                
                if (vendor == 0xFFFF) {
                    continue;
                }
                
                if (vendor == vendor_id && dev_id == device_id) {
                    dev->vendor_id = vendor;
                    dev->device_id = dev_id;
                    dev->bus = bus;
                    dev->device = device;
                    dev->function = function;
                    
                    uint32_t class_info = pci_read_config(bus, device, function, 0x08);
                    dev->class_code = (class_info >> 24) & 0xFF;
                    dev->subclass = (class_info >> 16) & 0xFF;
                    
                    dev->bar0 = pci_read_config(bus, device, function, PCI_BAR0);
                    dev->bar1 = pci_read_config(bus, device, function, PCI_BAR1);
                    
                    uint32_t irq_info = pci_read_config(bus, device, function, PCI_INTERRUPT_LINE);
                    dev->irq = irq_info & 0xFF;
                    
                    return 1;
                }
            }
        }
    }
    return 0;
}

int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* dev) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t config = pci_read_config(bus, device, function, 0);
                uint16_t vendor = config & 0xFFFF;
                
                if (vendor == 0xFFFF) {
                    continue;
                }
                
                uint32_t class_info = pci_read_config(bus, device, function, 0x08);
                uint8_t dev_class = (class_info >> 24) & 0xFF;
                uint8_t dev_subclass = (class_info >> 16) & 0xFF;
                
                if (dev_class == class_code && dev_subclass == subclass) {
                    dev->vendor_id = vendor;
                    dev->device_id = (config >> 16) & 0xFFFF;
                    dev->bus = bus;
                    dev->device = device;
                    dev->function = function;
                    dev->class_code = dev_class;
                    dev->subclass = dev_subclass;
                    
                    dev->bar0 = pci_read_config(bus, device, function, PCI_BAR0);
                    dev->bar1 = pci_read_config(bus, device, function, PCI_BAR1);
                    
                    uint32_t irq_info = pci_read_config(bus, device, function, PCI_INTERRUPT_LINE);
                    dev->irq = irq_info & 0xFF;
                    
                    return 1;
                }
            }
        }
    }
    return 0;
}