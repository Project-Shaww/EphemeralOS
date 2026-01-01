// net/udp.h
#ifndef UDP_H
#define UDP_H

#include "../kernel/kernel.h"

#define UDP_HEADER_LEN 8

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef void (*udp_callback_t)(uint32_t src_ip, uint16_t src_port, const uint8_t* data, uint16_t length);

void udp_init(void);
void udp_receive(uint32_t src_ip, const uint8_t* data, uint16_t length);
void udp_send(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, const uint8_t* data, uint16_t length);
void udp_register_handler(uint16_t port, udp_callback_t callback);

#endif