// net/ip.h
#ifndef IP_H
#define IP_H

#include "../kernel/kernel.h"

#define IP_VERSION 4
#define IP_HEADER_MIN_LEN 20
#define IP_DEFAULT_TTL 64

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

void ip_init(void);
void ip_receive(const uint8_t* data, uint16_t length);
void ip_send(uint32_t dest_ip, uint8_t protocol, const uint8_t* data, uint16_t length);
uint16_t ip_checksum(const uint8_t* data, uint16_t length);

#endif