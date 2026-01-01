// bin/ping.c
#include "commands.h"
#include "../drivers/screen.h"
#include "../drivers/network.h"
#include "../net/icmp.h"
#include "../net/dns.h"
#include "../net/ethernet.h"
#include "../kernel/kernel.h"

static void print_ip(uint32_t ip) {
    char buffer[16];
    char num[4];
    
    buffer[0] = '\0';
    
    for (int i = 3; i >= 0; i--) {
        uint8_t octet = (ip >> (i * 8)) & 0xFF;
        int val = octet;
        int len = 0;
        
        if (val == 0) {
            num[len++] = '0';
        } else {
            char temp[4];
            int tlen = 0;
            while (val > 0) {
                temp[tlen++] = '0' + (val % 10);
                val /= 10;
            }
            while (tlen > 0) {
                num[len++] = temp[--tlen];
            }
        }
        num[len] = '\0';
        
        strcat(buffer, num);
        if (i > 0) strcat(buffer, ".");
    }
    
    screen_print(buffer);
}

void cmd_ping(int argc, char** argv) {
    if (argc < 2) {
        screen_print("Usage: ping <hostname or IP>\n");
        return;
    }
    
    if (!network_is_ready()) {
        screen_print("ping: network not initialized\n");
        return;
    }
    
    network_info_t net_info;
    network_get_info(&net_info);
    
    if (net_info.ip == 0) {
        screen_print("ping: no IP address configured\n");
        return;
    }
    
    screen_print("PING ");
    screen_print(argv[1]);
    screen_print("\n");
    
    uint32_t target_ip;
    if (!dns_resolve(argv[1], &target_ip)) {
        screen_print("ping: cannot resolve ");
        screen_print(argv[1]);
        screen_print("\n");
        return;
    }
    
    screen_print("Pinging ");
    print_ip(target_ip);
    screen_print(" with 64 bytes of data:\n\n");
    
    int count = 4;
    int received = 0;
    uint16_t ping_id = 1234;
    
    for (int i = 0; i < count; i++) {
        uint16_t sequence = i + 1;
        
        icmp_send_echo_request(target_ip, ping_id, sequence);
        
        uint32_t start_time = 0;
        for (volatile int t = 0; t < 1000000; t++) start_time++;
        
        if (icmp_wait_reply(target_ip, sequence, 1000)) {
            screen_print("Reply from ");
            print_ip(target_ip);
            screen_print(": bytes=64 time<1ms TTL=64\n");
            received++;
        } else {
            screen_print("Request timed out.\n");
        }
        
        for (volatile int delay = 0; delay < 10000000; delay++);
    }
    
    screen_print("\nPing statistics for ");
    print_ip(target_ip);
    screen_print(":\n");
    screen_print("    Packets: Sent = ");
    
    char num[16];
    num[0] = '0' + count;
    num[1] = '\0';
    screen_print(num);
    
    screen_print(", Received = ");
    num[0] = '0' + received;
    screen_print(num);
    
    screen_print(", Lost = ");
    num[0] = '0' + (count - received);
    screen_print(num);
    
    screen_print("\n");
}