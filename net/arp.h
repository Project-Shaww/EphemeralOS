// net/arp.h
#ifndef ARP_H
#define ARP_H

#include "../kernel/kernel.h"

#define ARP_HTYPE_ETHERNET 0x0001
#define ARP_PTYPE_IPV4     0x0800

#define ARP_OPER_REQUEST   0x0001
#define ARP_OPER_REPLY     0x0002

#define ARP_CACHE_SIZE 32

typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} __attribute__((packed)) arp_packet_t;

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint32_t timestamp;
    uint8_t valid;
} arp_cache_entry_t;

void arp_init(void);
void arp_receive(const uint8_t* data, uint16_t length);
void arp_send_request(uint32_t target_ip);
int arp_resolve(uint32_t ip, uint8_t* mac);
void arp_add_entry(uint32_t ip, const uint8_t* mac);

#endif