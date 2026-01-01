// net/ntp.c
#include "ntp.h"
#include "udp.h"
#include "ethernet.h"
#include "../drivers/network.h"
#include "../drivers/rtc.h"

#define NTP_TIMEOUT_MS 5000

static uint32_t ntp_server = 0x08080808; // 8.8.8.8 por defecto
static uint32_t synced_timestamp = 0;
static int sync_complete = 0;

static uint32_t htonl(uint32_t n) {
    return ((n & 0xFF) << 24) | 
           ((n & 0xFF00) << 8) | 
           ((n & 0xFF0000) >> 8) | 
           ((n & 0xFF000000) >> 24);
}

static uint32_t ntohl(uint32_t n) {
    return htonl(n);
}

static void ntp_response_handler(uint32_t src_ip, uint16_t src_port, const uint8_t* data, uint16_t length) {
    (void)src_ip;
    (void)src_port;
    
    if (length < sizeof(ntp_packet_t)) {
        return;
    }
    
    ntp_packet_t* packet = (ntp_packet_t*)data;
    
    uint8_t mode = packet->li_vn_mode & 0x07;
    if (mode != 4) {
        return;
    }
    
    uint32_t trans_sec = ntohl(packet->trans_timestamp_sec);
    
    if (trans_sec > NTP_TIMESTAMP_DELTA) {
        synced_timestamp = trans_sec - NTP_TIMESTAMP_DELTA;
        sync_complete = 1;
    }
}

void ntp_init(void) {
    udp_register_handler(NTP_PORT, ntp_response_handler);
}

void ntp_set_server(uint32_t server_ip) {
    ntp_server = server_ip;
}

int ntp_sync_time(uint32_t* timestamp_out) {
    ntp_packet_t packet;
    memset(&packet, 0, sizeof(ntp_packet_t));
    
    packet.li_vn_mode = (NTP_VERSION << 3) | NTP_MODE_CLIENT;
    
    uint32_t local_time = rtc_get_unix_timestamp();
    packet.trans_timestamp_sec = htonl(local_time + NTP_TIMESTAMP_DELTA);
    packet.trans_timestamp_frac = 0;
    
    synced_timestamp = 0;
    sync_complete = 0;
    
    udp_send(ntp_server, NTP_PORT, NTP_PORT, (uint8_t*)&packet, sizeof(ntp_packet_t));
    
    uint32_t timeout = NTP_TIMEOUT_MS;
    while (timeout > 0 && !sync_complete) {
        uint8_t rx_buffer[1518];
        int rx_len = network_receive_packet(rx_buffer, sizeof(rx_buffer));
        
        if (rx_len > 0) {
            eth_receive_frame(rx_buffer, rx_len);
        }
        
        for (volatile int i = 0; i < 10000; i++);
        timeout--;
    }
    
    if (sync_complete && synced_timestamp != 0) {
        *timestamp_out = synced_timestamp;
        return 1;
    }
    
    return 0;
}