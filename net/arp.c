// net/arp.c
#include "arp.h"
#include "ethernet.h"
#include "../drivers/network.h"
#include "../drivers/screen.h"

static arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];
static uint32_t arp_timer = 0;

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

void arp_init(void) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = 0;
    }
    arp_timer = 0;
}

void arp_add_entry(uint32_t ip, const uint8_t* mac) {
    int oldest_index = 0;
    uint32_t oldest_time = 0xFFFFFFFF;
    
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            arp_cache[i].timestamp = arp_timer;
            return;
        }
        
        if (!arp_cache[i].valid) {
            oldest_index = i;
            break;
        }
        
        if (arp_cache[i].timestamp < oldest_time) {
            oldest_time = arp_cache[i].timestamp;
            oldest_index = i;
        }
    }
    
    arp_cache[oldest_index].ip = ip;
    memcpy(arp_cache[oldest_index].mac, mac, 6);
    arp_cache[oldest_index].timestamp = arp_timer;
    arp_cache[oldest_index].valid = 1;
}

int arp_resolve(uint32_t ip, uint8_t* mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac, arp_cache[i].mac, 6);
            return 1;
        }
    }
    return 0;
}

void arp_send_request(uint32_t target_ip) {
    arp_packet_t arp;
    uint8_t local_mac[6];
    uint32_t local_ip = network_get_ip();
    
    eth_get_mac(local_mac);
    
    arp.htype = htons(ARP_HTYPE_ETHERNET);
    arp.ptype = htons(ARP_PTYPE_IPV4);
    arp.hlen = 6;
    arp.plen = 4;
    arp.oper = htons(ARP_OPER_REQUEST);
    
    memcpy(arp.sha, local_mac, 6);
    arp.spa = htonl(local_ip);
    
    memset(arp.tha, 0, 6);
    arp.tpa = htonl(target_ip);
    
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    eth_send_frame(broadcast, ETH_TYPE_ARP, (uint8_t*)&arp, sizeof(arp_packet_t));
}

void arp_receive(const uint8_t* data, uint16_t length) {
    if (length < sizeof(arp_packet_t)) {
        return;
    }
    
    arp_packet_t* arp = (arp_packet_t*)data;
    
    if (ntohs(arp->htype) != ARP_HTYPE_ETHERNET || 
        ntohs(arp->ptype) != ARP_PTYPE_IPV4) {
        return;
    }
    
    uint32_t sender_ip = ntohl(arp->spa);
    uint32_t target_ip = ntohl(arp->tpa);
    uint32_t local_ip = network_get_ip();
    
    arp_add_entry(sender_ip, arp->sha);
    
    uint16_t operation = ntohs(arp->oper);
    
    if (operation == ARP_OPER_REQUEST && target_ip == local_ip) {
        arp_packet_t reply;
        uint8_t local_mac[6];
        eth_get_mac(local_mac);
        
        reply.htype = htons(ARP_HTYPE_ETHERNET);
        reply.ptype = htons(ARP_PTYPE_IPV4);
        reply.hlen = 6;
        reply.plen = 4;
        reply.oper = htons(ARP_OPER_REPLY);
        
        memcpy(reply.sha, local_mac, 6);
        reply.spa = htonl(local_ip);
        
        memcpy(reply.tha, arp->sha, 6);
        reply.tpa = arp->spa;
        
        eth_send_frame(arp->sha, ETH_TYPE_ARP, (uint8_t*)&reply, sizeof(arp_packet_t));
    }
}