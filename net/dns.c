// net/dns.c
#include "dns.h"
#include "udp.h"
#include "ethernet.h"
#include "../drivers/network.h"

#define DNS_TIMEOUT_MS 5000
#define DNS_BUFFER_SIZE 512

static uint32_t dns_server = 0x08080808; // 8.8.8.8 por defecto
static uint32_t resolved_ip = 0;
static int resolution_complete = 0;
static uint16_t dns_transaction_id = 1;

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

static void dns_response_handler(uint32_t src_ip, uint16_t src_port, const uint8_t* data, uint16_t length) {
    (void)src_ip;
    (void)src_port;
    
    if (length < sizeof(dns_header_t)) {
        return;
    }
    
    dns_header_t* header = (dns_header_t*)data;
    uint16_t flags = ntohs(header->flags);
    
    if (!(flags & DNS_FLAG_QR)) {
        return;
    }
    
    uint16_t ancount = ntohs(header->ancount);
    if (ancount == 0) {
        return;
    }
    
    const uint8_t* ptr = data + sizeof(dns_header_t);
    const uint8_t* end = data + length;
    
    while (ptr < end && *ptr != 0) {
        if ((*ptr & 0xC0) == 0xC0) {
            ptr += 2;
            break;
        } else {
            ptr += *ptr + 1;
        }
    }
    
    if (ptr >= end) return;
    if (*ptr == 0) ptr++;
    
    ptr += 4;
    
    for (uint16_t i = 0; i < ancount && ptr < end; i++) {
        while (ptr < end && *ptr != 0) {
            if ((*ptr & 0xC0) == 0xC0) {
                ptr += 2;
                break;
            } else {
                ptr += *ptr + 1;
            }
        }
        
        if (ptr >= end) return;
        if (*ptr == 0) ptr++;
        
        if (ptr + 10 > end) return;
        
        uint16_t type = (ptr[0] << 8) | ptr[1];
        uint16_t rdlength = (ptr[8] << 8) | ptr[9];
        ptr += 10;
        
        if (type == DNS_QUERY_TYPE_A && rdlength == 4) {
            if (ptr + 4 <= end) {
                resolved_ip = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                resolution_complete = 1;
                return;
            }
        }
        
        ptr += rdlength;
    }
}

void dns_init(void) {
    udp_register_handler(DNS_PORT, dns_response_handler);
}

void dns_set_server(uint32_t server_ip) {
    dns_server = server_ip;
}

static int is_ip_address(const char* str) {
    int dots = 0;
    int digits = 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == '.') {
            if (digits == 0 || digits > 3) return 0;
            dots++;
            digits = 0;
        } else if (str[i] >= '0' && str[i] <= '9') {
            digits++;
        } else {
            return 0;
        }
    }
    
    return (dots == 3 && digits > 0 && digits <= 3);
}

static uint32_t parse_ip(const char* str) {
    uint32_t ip = 0;
    int octet = 0;
    int shift = 24;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == '.') {
            ip |= (octet << shift);
            shift -= 8;
            octet = 0;
        } else {
            octet = octet * 10 + (str[i] - '0');
        }
    }
    ip |= (octet << shift);
    
    return ip;
}

int dns_resolve(const char* hostname, uint32_t* ip_out) {
    if (is_ip_address(hostname)) {
        *ip_out = parse_ip(hostname);
        return 1;
    }
    
    uint8_t query[DNS_BUFFER_SIZE];
    dns_header_t* header = (dns_header_t*)query;
    
    header->id = htons(dns_transaction_id++);
    header->flags = htons(DNS_FLAG_RD);
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    
    uint8_t* qname = query + sizeof(dns_header_t);
    const char* label_start = hostname;
    uint8_t* label_len_ptr = qname++;
    uint8_t label_len = 0;
    
    for (int i = 0; hostname[i]; i++) {
        if (hostname[i] == '.') {
            *label_len_ptr = label_len;
            label_len_ptr = qname++;
            label_len = 0;
            label_start = &hostname[i + 1];
        } else {
            *qname++ = hostname[i];
            label_len++;
        }
    }
    *label_len_ptr = label_len;
    *qname++ = 0;
    
    uint16_t* qtype = (uint16_t*)qname;
    *qtype++ = htons(DNS_QUERY_TYPE_A);
    
    uint16_t* qclass = (uint16_t*)qtype;
    *qclass++ = htons(DNS_CLASS_IN);
    
    uint16_t query_length = (uint8_t*)qclass - query;
    
    resolved_ip = 0;
    resolution_complete = 0;
    
    udp_send(dns_server, 12345, DNS_PORT, query, query_length);
    
    uint32_t timeout = DNS_TIMEOUT_MS;
    while (timeout > 0 && !resolution_complete) {
        uint8_t rx_buffer[1518];
        int rx_len = network_receive_packet(rx_buffer, sizeof(rx_buffer));
        
        if (rx_len > 0) {
            eth_receive_frame(rx_buffer, rx_len);
        }
        
        for (volatile int i = 0; i < 10000; i++);
        timeout--;
    }
    
    if (resolution_complete && resolved_ip != 0) {
        *ip_out = resolved_ip;
        return 1;
    }
    
    return 0;
}