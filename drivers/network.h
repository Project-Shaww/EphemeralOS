// drivers/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include "../kernel/kernel.h"

#define ETH_ALEN 6
#define ETH_FRAME_LEN 1518
#define ETH_DATA_LEN 1500

typedef struct {
    uint8_t mac[ETH_ALEN];
    uint32_t ip;
    int link_up;
    int dhcp_configured;
} network_info_t;

// Funciones principales
void network_init(void);
int network_is_ready(void);
void network_get_info(network_info_t* info);
void network_send_packet(const uint8_t* data, uint16_t length);
int network_receive_packet(uint8_t* buffer, uint16_t max_length);

// Funciones de utilidad
void network_set_mac(const uint8_t* mac);
void network_get_mac(uint8_t* mac);
void network_set_ip(uint32_t ip);
uint32_t network_get_ip(void);

#endif