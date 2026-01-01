// drivers/network.c
#include "network.h"
#include "pci.h"
#include "screen.h"

// RTL8139 Registers
#define RTL8139_IDR0        0x00
#define RTL8139_MAR0        0x08
#define RTL8139_RBSTART     0x30
#define RTL8139_CMD         0x37
#define RTL8139_CAPR        0x38
#define RTL8139_CBR         0x3A
#define RTL8139_IMR         0x3C
#define RTL8139_ISR         0x3E
#define RTL8139_TCR         0x40
#define RTL8139_RCR         0x44
#define RTL8139_CONFIG1     0x52

// RTL8139 Commands
#define RTL8139_CMD_RESET   0x10
#define RTL8139_CMD_RX_EN   0x08
#define RTL8139_CMD_TX_EN   0x04

// Buffer sizes
#define RX_BUFFER_SIZE (8192 + 16 + 1500)
#define TX_BUFFER_SIZE 2048

static uint32_t io_base = 0;
static uint8_t mac_address[ETH_ALEN];
static uint32_t ip_address = 0;
static int initialized = 0;

static uint8_t rx_buffer[RX_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t tx_buffer[TX_BUFFER_SIZE] __attribute__((aligned(4)));
static uint16_t rx_offset = 0;

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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

static void rtl8139_reset(void) {
    outb(io_base + RTL8139_CMD, RTL8139_CMD_RESET);
    
    while ((inb(io_base + RTL8139_CMD) & RTL8139_CMD_RESET) != 0) {
        for (volatile int i = 0; i < 1000; i++);
    }
}

static void rtl8139_init(uint32_t bar0) {
    io_base = bar0 & ~0x3;
    
    // Reset
    rtl8139_reset();
    
    // Leer MAC
    for (int i = 0; i < ETH_ALEN; i++) {
        mac_address[i] = inb(io_base + RTL8139_IDR0 + i);
    }
    
    // Configurar buffer de recepción
    outl(io_base + RTL8139_RBSTART, (uint32_t)rx_buffer);
    
    // Habilitar todas las interrupciones
    outw(io_base + RTL8139_IMR, 0x0005);
    
    // Configurar recepción: aceptar broadcast, multicast y paquetes físicos
    // WRAP, AB (Accept Broadcast), AM (Accept Multicast), APM (Accept Physical Match), AAP (Accept All Packets)
    outl(io_base + RTL8139_RCR, 0x0000070F);
    
    // Configurar transmisión
    outl(io_base + RTL8139_TCR, 0x03000000);
    
    // Habilitar RX y TX
    outb(io_base + RTL8139_CMD, RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN);
    
    rx_offset = 0;
    initialized = 1;
}

void network_init(void) {
    pci_device_t nic;
    
    if (pci_find_device(PCI_VENDOR_REALTEK, PCI_DEVICE_RTL8139, &nic)) {
        screen_print("[NET] Found RTL8139 NIC\n");
        
        // Habilitar bus mastering e IO space
        uint16_t command = pci_read_config(nic.bus, nic.device, nic.function, PCI_COMMAND) & 0xFFFF;
        command |= 0x05; // IO Space + Bus Master
        pci_write_config(nic.bus, nic.device, nic.function, PCI_COMMAND, command);
        
        rtl8139_init(nic.bar0);
        
        screen_print("[NET] MAC: ");
        for (int i = 0; i < ETH_ALEN; i++) {
            char hex[3];
            uint8_t val = mac_address[i];
            hex[0] = "0123456789ABCDEF"[val >> 4];
            hex[1] = "0123456789ABCDEF"[val & 0xF];
            hex[2] = '\0';
            screen_print(hex);
            if (i < ETH_ALEN - 1) screen_print(":");
        }
        screen_print("\n");
        
        return;
    }
    
    screen_print("[NET] No supported network card found\n");
}

int network_is_ready(void) {
    return initialized;
}

void network_get_info(network_info_t* info) {
    memcpy(info->mac, mac_address, ETH_ALEN);
    info->ip = ip_address;
    info->link_up = initialized;
    info->dhcp_configured = (ip_address != 0);
}

void network_send_packet(const uint8_t* data, uint16_t length) {
    if (!initialized || length > TX_BUFFER_SIZE) {
        return;
    }
    
    // Copiar datos al buffer de TX
    memcpy(tx_buffer, data, length);
    
    // Asegurar tamaño mínimo de 60 bytes
    if (length < 60) {
        memset(tx_buffer + length, 0, 60 - length);
        length = 60;
    }
    
    // Usar el descriptor TX 0
    static int tx_port = 0;
    uint32_t tx_status_port = 0x10 + (tx_port * 4);
    uint32_t tx_addr_port = 0x20 + (tx_port * 4);
    
    // Escribir dirección del buffer
    outl(io_base + tx_addr_port, (uint32_t)tx_buffer);
    
    // Escribir longitud y comenzar transmisión
    outl(io_base + tx_status_port, length & 0x1FFF);
    
    // Esperar a que termine la transmisión
    while (!(inl(io_base + tx_status_port) & 0x8000)) {
        for (volatile int i = 0; i < 100; i++);
    }
    
    tx_port = (tx_port + 1) % 4;
}

int network_receive_packet(uint8_t* buffer, uint16_t max_length) {
    if (!initialized) {
        return -1;
    }
    
    // Verificar si hay datos disponibles
    uint8_t cmd = inb(io_base + RTL8139_CMD);
    if (cmd & 0x01) { // Buffer vacío
        return 0;
    }
    
    // Leer el header del paquete
    uint16_t* header = (uint16_t*)(&rx_buffer[rx_offset]);
    uint16_t status = header[0];
    uint16_t length = header[1];
    
    // Verificar status
    if (!(status & 0x01)) { // Paquete no válido
        // Reset del RX
        outb(io_base + RTL8139_CMD, 0x0C); // Solo TX habilitado
        for (volatile int i = 0; i < 10000; i++);
        rx_offset = 0;
        outl(io_base + RTL8139_RBSTART, (uint32_t)rx_buffer);
        outb(io_base + RTL8139_CMD, RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN);
        return -1;
    }
    
    // Verificar longitud
    if (length < 60 || length > 1518) {
        // Saltar paquete malo
        rx_offset = (rx_offset + length + 4 + 3) & ~3;
        if (rx_offset >= 8192) rx_offset -= 8192;
        outw(io_base + RTL8139_CAPR, rx_offset - 0x10);
        return 0;
    }
    
    // Quitar CRC (4 bytes)
    length -= 4;
    
    // Limitar al tamaño del buffer
    if (length > max_length) {
        length = max_length;
    }
    
    // Copiar datos
    memcpy(buffer, &rx_buffer[rx_offset + 4], length);
    
    // Actualizar offset
    rx_offset = (rx_offset + length + 4 + 3) & ~3;
    if (rx_offset >= 8192) rx_offset -= 8192;
    
    // Actualizar CAPR
    outw(io_base + RTL8139_CAPR, rx_offset - 0x10);
    
    return length;
}

void network_set_mac(const uint8_t* mac) {
    memcpy(mac_address, mac, ETH_ALEN);
}

void network_get_mac(uint8_t* mac) {
    memcpy(mac, mac_address, ETH_ALEN);
}

void network_set_ip(uint32_t ip) {
    ip_address = ip;
}

uint32_t network_get_ip(void) {
    return ip_address;
}