// bin/nettest.c
#include "commands.h"
#include "../drivers/screen.h"
#include "../drivers/network.h"
#include "../net/ethernet.h"
#include "../kernel/kernel.h"

void cmd_nettest(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    screen_print("Network Hardware Test\n");
    screen_print("=====================\n\n");
    
    if (!network_is_ready()) {
        screen_print("ERROR: Network not initialized\n");
        return;
    }
    
    network_info_t info;
    network_get_info(&info);
    
    screen_print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        char hex[3];
        hex[0] = "0123456789ABCDEF"[info.mac[i] >> 4];
        hex[1] = "0123456789ABCDEF"[info.mac[i] & 0xF];
        hex[2] = '\0';
        screen_print(hex);
        if (i < 5) screen_print(":");
    }
    screen_print("\n");
    
    screen_print("IP Address:  ");
    for (int i = 3; i >= 0; i--) {
        uint8_t octet = (info.ip >> (i * 8)) & 0xFF;
        char num[4];
        int pos = 0;
        if (octet == 0) {
            num[pos++] = '0';
        } else {
            char temp[4];
            int tpos = 0;
            int val = octet;
            while (val > 0) {
                temp[tpos++] = '0' + (val % 10);
                val /= 10;
            }
            while (tpos > 0) {
                num[pos++] = temp[--tpos];
            }
        }
        num[pos] = '\0';
        screen_print(num);
        if (i > 0) screen_print(".");
    }
    screen_print("\n");
    
    screen_print("Link Status: ");
    screen_print(info.link_up ? "UP\n" : "DOWN\n");
    
    screen_print("\nSending test packet...\n");
    
    // Crear un paquete ARP broadcast simple
    uint8_t test_packet[60];
    memset(test_packet, 0, 60);
    
    // Ethernet header
    for (int i = 0; i < 6; i++) test_packet[i] = 0xFF; // Broadcast
    for (int i = 0; i < 6; i++) test_packet[6 + i] = info.mac[i]; // Source
    test_packet[12] = 0x08; // ARP
    test_packet[13] = 0x06;
    
    // ARP request
    test_packet[14] = 0x00; test_packet[15] = 0x01; // Hardware type
    test_packet[16] = 0x08; test_packet[17] = 0x00; // Protocol type
    test_packet[18] = 0x06; // Hardware size
    test_packet[19] = 0x04; // Protocol size
    test_packet[20] = 0x00; test_packet[21] = 0x01; // Operation (request)
    
    network_send_packet(test_packet, 60);
    screen_print("Packet sent!\n");
    
    screen_print("\nWaiting for incoming packets (5 seconds)...\n");
    int packet_count = 0;
    
    for (int i = 0; i < 50; i++) {
        uint8_t rx_buffer[1518];
        int len = network_receive_packet(rx_buffer, sizeof(rx_buffer));
        
        if (len > 0) {
            packet_count++;
            screen_print("Received packet #");
            char num[4];
            num[0] = '0' + packet_count;
            num[1] = '\0';
            screen_print(num);
            screen_print(", length=");
            
            int val = len;
            int pos = 0;
            char lenbuf[8];
            if (val == 0) {
                lenbuf[pos++] = '0';
            } else {
                char temp[8];
                int tpos = 0;
                while (val > 0) {
                    temp[tpos++] = '0' + (val % 10);
                    val /= 10;
                }
                while (tpos > 0) {
                    lenbuf[pos++] = temp[--tpos];
                }
            }
            lenbuf[pos] = '\0';
            screen_print(lenbuf);
            screen_print(" bytes\n");
        }
        
        for (volatile int d = 0; d < 100000; d++);
    }
    
    screen_print("\nTest complete. Received ");
    char num[4];
    num[0] = '0' + packet_count;
    num[1] = '\0';
    screen_print(num);
    screen_print(" packets.\n");
    
    if (packet_count == 0) {
        screen_print("\nWARNING: No packets received!\n");
        screen_print("Possible issues:\n");
        screen_print("- RTL8139 driver problem\n");
        screen_print("- QEMU network configuration\n");
        screen_print("- Hardware initialization failed\n");
    } else {
        screen_print("\nSUCCESS: Network hardware is working!\n");
    }
}