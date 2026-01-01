// net/icmp.c
#include "icmp.h"
#include "ip.h"
#include "ethernet.h"
#include "../drivers/network.h"
#include "../drivers/screen.h"

#define MAX_PING_STATES 16

static icmp_ping_state_t ping_states[MAX_PING_STATES];
static uint32_t icmp_timer = 0;

static uint16_t htons(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

static uint16_t ntohs(uint16_t n) {
    return htons(n);
}

void icmp_init(void) {
    for (int i = 0; i < MAX_PING_STATES; i++) {
        ping_states[i].received = 0;
    }
    icmp_timer = 0;
}

void icmp_send_echo_request(uint32_t dest_ip, uint16_t id, uint16_t sequence) {
    uint8_t packet[64];
    icmp_header_t* header = (icmp_header_t*)packet;
    
    header->type = ICMP_TYPE_ECHO_REQUEST;
    header->code = ICMP_CODE_ECHO;
    header->checksum = 0;
    header->id = htons(id);
    header->sequence = htons(sequence);
    
    for (int i = 0; i < 56; i++) {
        packet[sizeof(icmp_header_t) + i] = 0x41 + (i % 26);
    }
    
    uint16_t packet_size = sizeof(icmp_header_t) + 56;
    header->checksum = htons(ip_checksum(packet, packet_size));
    
    int slot = -1;
    for (int i = 0; i < MAX_PING_STATES; i++) {
        if (!ping_states[i].received || 
            (icmp_timer - ping_states[i].timestamp) > 5000) {
            slot = i;
            break;
        }
    }
    
    if (slot >= 0) {
        ping_states[slot].ip = dest_ip;
        ping_states[slot].sequence = sequence;
        ping_states[slot].timestamp = icmp_timer;
        ping_states[slot].received = 0;
    }
    
    ip_send(dest_ip, IP_PROTO_ICMP, packet, packet_size);
}

void icmp_receive(uint32_t src_ip, const uint8_t* data, uint16_t length) {
    if (length < sizeof(icmp_header_t)) {
        return;
    }
    
    icmp_header_t* header = (icmp_header_t*)data;
    
    if (header->type == ICMP_TYPE_ECHO_REQUEST) {
        uint8_t reply[1500];
        memcpy(reply, data, length);
        
        icmp_header_t* reply_header = (icmp_header_t*)reply;
        reply_header->type = ICMP_TYPE_ECHO_REPLY;
        reply_header->code = ICMP_CODE_ECHO;
        reply_header->checksum = 0;
        reply_header->checksum = htons(ip_checksum(reply, length));
        
        ip_send(src_ip, IP_PROTO_ICMP, reply, length);
    }
    else if (header->type == ICMP_TYPE_ECHO_REPLY) {
        uint16_t sequence = ntohs(header->sequence);
        
        for (int i = 0; i < MAX_PING_STATES; i++) {
            if (ping_states[i].ip == src_ip && 
                ping_states[i].sequence == sequence &&
                !ping_states[i].received) {
                ping_states[i].received = 1;
                break;
            }
        }
    }
}

int icmp_wait_reply(uint32_t dest_ip, uint16_t sequence, uint32_t timeout_ms) {
    uint32_t start_time = icmp_timer;
    
    while ((icmp_timer - start_time) < timeout_ms) {
        uint8_t rx_buffer[1518];
        int rx_len = network_receive_packet(rx_buffer, sizeof(rx_buffer));
        
        if (rx_len > 0) {
            eth_receive_frame(rx_buffer, rx_len);
        }
        
        for (int i = 0; i < MAX_PING_STATES; i++) {
            if (ping_states[i].ip == dest_ip && 
                ping_states[i].sequence == sequence &&
                ping_states[i].received) {
                return 1;
            }
        }
        
        icmp_timer++;
        for (volatile int i = 0; i < 10000; i++);
    }
    
    return 0;
}