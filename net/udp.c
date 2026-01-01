// net/udp.c
#include "udp.h"
#include "ip.h"
#include "../drivers/network.h"

#define MAX_UDP_HANDLERS 8

typedef struct {
    uint16_t port;
    udp_callback_t callback;
} udp_handler_t;

static udp_handler_t udp_handlers[MAX_UDP_HANDLERS];

static uint16_t htons(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

static uint16_t ntohs(uint16_t n) {
    return htons(n);
}

static uint32_t htonl(uint32_t n) {
    return ((n & 0xFF) << 24) | 
           ((n & 0xFF00) << 8) | 
           ((n & 0xFF0000) >> 8) | 
           ((n & 0xFF000000) >> 24);
}

static uint32_t ntohl(uint32_t n) {
    return htonl(n);
}

void udp_init(void) {
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        udp_handlers[i].port = 0;
        udp_handlers[i].callback = 0;
    }
}

void udp_register_handler(uint16_t port, udp_callback_t callback) {
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (udp_handlers[i].port == 0) {
            udp_handlers[i].port = port;
            udp_handlers[i].callback = callback;
            return;
        }
    }
}

void udp_send(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, const uint8_t* data, uint16_t length) {
    uint8_t packet[1500];
    udp_header_t* header = (udp_header_t*)packet;
    
    uint16_t total_length = UDP_HEADER_LEN + length;
    if (total_length > 1500) {
        return;
    }
    
    header->src_port = htons(src_port);
    header->dest_port = htons(dest_port);
    header->length = htons(total_length);
    header->checksum = 0;
    
    memcpy(packet + UDP_HEADER_LEN, data, length);
    
    ip_send(dest_ip, IP_PROTO_UDP, packet, total_length);
}

void udp_receive(uint32_t src_ip, const uint8_t* data, uint16_t length) {
    if (length < UDP_HEADER_LEN) {
        return;
    }
    
    udp_header_t* header = (udp_header_t*)data;
    
    uint16_t src_port = ntohs(header->src_port);
    uint16_t dest_port = ntohs(header->dest_port);
    uint16_t udp_length = ntohs(header->length);
    
    if (udp_length > length || udp_length < UDP_HEADER_LEN) {
        return;
    }
    
    const uint8_t* payload = data + UDP_HEADER_LEN;
    uint16_t payload_length = udp_length - UDP_HEADER_LEN;
    
    for (int i = 0; i < MAX_UDP_HANDLERS; i++) {
        if (udp_handlers[i].port == dest_port && udp_handlers[i].callback) {
            udp_handlers[i].callback(src_ip, src_port, payload, payload_length);
            return;
        }
    }
}