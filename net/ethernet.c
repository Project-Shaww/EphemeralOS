// net/ethernet.c
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "../drivers/network.h"

static uint8_t local_mac[ETH_ALEN];

static uint16_t htons(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

static uint16_t ntohs(uint16_t n) {
    return htons(n);
}

void eth_init(void) {
    network_get_mac(local_mac);
}

void eth_send_frame(const uint8_t* dest_mac, uint16_t eth_type, const uint8_t* data, uint16_t length) {
    if (length > ETH_DATA_LEN) {
        return;
    }
    
    uint8_t frame[ETH_FRAME_LEN];
    eth_header_t* header = (eth_header_t*)frame;
    
    memcpy(header->dest, dest_mac, ETH_ALEN);
    memcpy(header->src, local_mac, ETH_ALEN);
    header->type = htons(eth_type);
    
    memcpy(frame + ETH_HLEN, data, length);
    
    uint16_t frame_length = ETH_HLEN + length;
    if (frame_length < 60) {
        memset(frame + frame_length, 0, 60 - frame_length);
        frame_length = 60;
    }
    
    network_send_packet(frame, frame_length);
}

void eth_receive_frame(const uint8_t* frame, uint16_t length) {
    if (length < ETH_HLEN) {
        return;
    }
    
    eth_header_t* header = (eth_header_t*)frame;
    uint16_t eth_type = ntohs(header->type);
    
    const uint8_t* payload = frame + ETH_HLEN;
    uint16_t payload_length = length - ETH_HLEN;
    
    switch (eth_type) {
        case ETH_TYPE_ARP:
            arp_receive(payload, payload_length);
            break;
            
        case ETH_TYPE_IP:
            ip_receive(payload, payload_length);
            break;
            
        default:
            break;
    }
}

void eth_get_mac(uint8_t* mac) {
    memcpy(mac, local_mac, ETH_ALEN);
}