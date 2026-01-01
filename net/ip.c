// net/ip.c
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "arp.h"
#include "ethernet.h"
#include "../drivers/network.h"

static uint16_t ip_id_counter = 0;

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

void ip_init(void) {
    ip_id_counter = 1;
}

uint16_t ip_checksum(const uint8_t* data, uint16_t length) {
    uint32_t sum = 0;
    const uint16_t* ptr = (const uint16_t*)data;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

void ip_send(uint32_t dest_ip, uint8_t protocol, const uint8_t* data, uint16_t length) {
    uint8_t packet[1500];
    ip_header_t* header = (ip_header_t*)packet;
    
    uint16_t total_length = IP_HEADER_MIN_LEN + length;
    if (total_length > 1500) {
        return;
    }
    
    header->version_ihl = 0x45;
    header->tos = 0;
    header->total_length = htons(total_length);
    header->identification = htons(ip_id_counter++);
    header->flags_fragment = htons(0x4000);
    header->ttl = IP_DEFAULT_TTL;
    header->protocol = protocol;
    header->checksum = 0;
    header->src_ip = htonl(network_get_ip());
    header->dest_ip = htonl(dest_ip);
    
    header->checksum = htons(ip_checksum((uint8_t*)header, IP_HEADER_MIN_LEN));
    
    memcpy(packet + IP_HEADER_MIN_LEN, data, length);
    
    uint8_t dest_mac[6];
    
    uint32_t local_ip = network_get_ip();
    uint32_t gateway = (local_ip & 0xFFFFFF00) | 0x01;
    uint32_t subnet_mask = 0xFFFFFF00;
    
    uint32_t target_ip;
    if ((dest_ip & subnet_mask) == (local_ip & subnet_mask)) {
        target_ip = dest_ip;
    } else {
        target_ip = gateway;
    }
    
    if (!arp_resolve(target_ip, dest_mac)) {
        arp_send_request(target_ip);
        
        for (int retry = 0; retry < 10; retry++) {
            for (volatile int i = 0; i < 1000000; i++);
            
            uint8_t rx_buffer[1518];
            int rx_len = network_receive_packet(rx_buffer, sizeof(rx_buffer));
            if (rx_len > 0) {
                eth_receive_frame(rx_buffer, rx_len);
            }
            
            if (arp_resolve(target_ip, dest_mac)) {
                break;
            }
        }
        
        if (!arp_resolve(target_ip, dest_mac)) {
            return;
        }
    }
    
    eth_send_frame(dest_mac, ETH_TYPE_IP, packet, total_length);
}

void ip_receive(const uint8_t* data, uint16_t length) {
    if (length < IP_HEADER_MIN_LEN) {
        return;
    }
    
    ip_header_t* header = (ip_header_t*)data;
    
    uint8_t version = (header->version_ihl >> 4) & 0x0F;
    if (version != IP_VERSION) {
        return;
    }
    
    uint8_t ihl = (header->version_ihl & 0x0F) * 4;
    if (ihl < IP_HEADER_MIN_LEN || ihl > length) {
        return;
    }
    
    uint16_t total_length = ntohs(header->total_length);
    if (total_length > length) {
        return;
    }
    
    uint32_t dest_ip = ntohl(header->dest_ip);
    uint32_t local_ip = network_get_ip();
    
    if (dest_ip != local_ip && dest_ip != 0xFFFFFFFF) {
        return;
    }
    
    const uint8_t* payload = data + ihl;
    uint16_t payload_length = total_length - ihl;
    
    uint32_t src_ip = ntohl(header->src_ip);
    
    switch (header->protocol) {
        case IP_PROTO_ICMP:
            icmp_receive(src_ip, payload, payload_length);
            break;
            
        case IP_PROTO_UDP:
            udp_receive(src_ip, payload, payload_length);
            break;
            
        default:
            break;
    }
}