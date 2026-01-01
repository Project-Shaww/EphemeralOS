// net/dns.h
#ifndef DNS_H
#define DNS_H

#include "../kernel/kernel.h"

#define DNS_PORT 53
#define DNS_QUERY_TYPE_A 1
#define DNS_CLASS_IN 1

#define DNS_FLAG_QR 0x8000
#define DNS_FLAG_AA 0x0400
#define DNS_FLAG_TC 0x0200
#define DNS_FLAG_RD 0x0100
#define DNS_FLAG_RA 0x0080

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

void dns_init(void);
int dns_resolve(const char* hostname, uint32_t* ip_out);
void dns_set_server(uint32_t server_ip);

#endif