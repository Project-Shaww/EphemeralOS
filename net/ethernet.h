// net/ethernet.h
#ifndef ETHERNET_H
#define ETHERNET_H

#include "../kernel/kernel.h"

#define ETH_ALEN 6
#define ETH_HLEN 14
#define ETH_FRAME_LEN 1518
#define ETH_DATA_LEN 1500

#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

typedef struct {
    uint8_t dest[ETH_ALEN];
    uint8_t src[ETH_ALEN];
    uint16_t type;
} __attribute__((packed)) eth_header_t;

void eth_init(void);
void eth_send_frame(const uint8_t* dest_mac, uint16_t eth_type, const uint8_t* data, uint16_t length);
void eth_receive_frame(const uint8_t* frame, uint16_t length);
void eth_get_mac(uint8_t* mac);

#endif