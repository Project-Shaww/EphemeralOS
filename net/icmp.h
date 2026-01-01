// net/icmp.h
#ifndef ICMP_H
#define ICMP_H

#include "../kernel/kernel.h"

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

#define ICMP_CODE_ECHO 0

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

typedef struct {
    uint32_t ip;
    uint16_t sequence;
    uint32_t timestamp;
    int received;
} icmp_ping_state_t;

void icmp_init(void);
void icmp_receive(uint32_t src_ip, const uint8_t* data, uint16_t length);
void icmp_send_echo_request(uint32_t dest_ip, uint16_t id, uint16_t sequence);
int icmp_wait_reply(uint32_t dest_ip, uint16_t sequence, uint32_t timeout_ms);

#endif