// kernel/kernel.c
#include "kernel.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../drivers/disk.h"
#include "../drivers/rtc.h"
#include "../drivers/network.h"
#include "../drivers/pci.h"
#include "../fs/filesystem.h"
#include "../installer/installer.h"
#include "../shell/shell.h"
#include "../net/ethernet.h"
#include "../net/arp.h"
#include "../net/ip.h"
#include "../net/icmp.h"
#include "../net/udp.h"
#include "../net/dns.h"
#include "../net/ntp.h"

static void keyboard_getline_password(char* buffer, int max_length) {
    int index = 0;

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            buffer[index] = '\0';
            screen_print_char('\n');
            return;
        }

        if (c == '\b') {
            if (index > 0) {
                index--;
                screen_backspace();
            }
            continue;
        }

        if (index < max_length - 1 && c >= 32 && c <= 126) {
            buffer[index++] = c;
            screen_print_char('*');
        }
    }
}

static void shutdown_system(void) {
    screen_print("\nShutting down...\n");
    for (volatile int i = 0; i < 50000000; i++);

    asm volatile("mov $0x2000, %ax; mov %ax, %dx; out %ax, %dx");
    asm volatile("mov $0x5307, %ax; mov $0x0001, %bx; mov $0x0003, %cx; int $0x15");

    while(1) {
        asm volatile("hlt");
    }
}

static uint8_t read_keyboard_direct(void) {
    while (1) {
        uint8_t status;
        asm volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            uint8_t scancode;
            asm volatile("inb $0x60, %0" : "=a"(scancode));
            return scancode;
        }
    }
}

static int show_continue_menu(void) {
    int selected = 0;
    int screen_row = 20;
    
    for (volatile int d = 0; d < 10000000; d++);
    
    for (int i = 0; i < 100; i++) {
        uint8_t status;
        asm volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            uint8_t dummy;
            asm volatile("inb $0x60, %0" : "=a"(dummy));
        }
        for (volatile int j = 0; j < 10000; j++);
    }
    
    screen_set_row(screen_row);
    screen_set_color(0x0F, 0x00);
    screen_print("                                          ");
    screen_set_row(screen_row + 1);
    screen_print("  Deseas continuar?                      ");
    screen_set_row(screen_row + 2);
    screen_print("                                          ");
    screen_set_row(screen_row + 3);
    screen_print("                                          ");
    screen_set_row(screen_row + 4);
    screen_print("                                          ");
    screen_set_row(screen_row + 5);
    screen_print("                                          ");
    screen_set_row(screen_row + 6);
    screen_print("                                          ");
    screen_set_row(screen_row + 7);
    screen_print("  Use LEFT/RIGHT arrows, then ENTER     ");
    
    while (1) {
        screen_set_row(screen_row + 4);
        if (selected == 0) {
            screen_set_color(0x00, 0x0F);
            screen_print("  > SI  ");
            screen_set_color(0x0F, 0x00);
        } else {
            screen_set_color(0x0F, 0x00);
            screen_print("    SI  ");
        }
        
        screen_set_row(screen_row + 5);
        if (selected == 1) {
            screen_set_color(0x00, 0x0F);
            screen_print("  > No  ");
            screen_set_color(0x0F, 0x00);
        } else {
            screen_set_color(0x0F, 0x00);
            screen_print("    No  ");
        }
        
        uint8_t scancode = read_keyboard_direct();
        
        if (scancode & 0x80) {
            continue;
        }
        
        if (scancode == 0x4B) {
            selected = 0;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x4D) {
            selected = 1;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x48) {
            selected = 0;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x50) {
            selected = 1;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x1C) {
            while (1) {
                scancode = read_keyboard_direct();
                if (scancode == 0x9C) break;
            }
            return selected;
        }
    }
}

static int do_login(void) {
    char username_input[64];
    char password_input[64];
    char stored_username[64];

    fs_get_username(stored_username);

    screen_set_color(0x0F, 0x00);
    screen_print("login: ");
    keyboard_getline(username_input, sizeof(username_input));

    if (strcmp(username_input, stored_username) != 0) {
        screen_print("Login incorrect\n");
        char hostname[64];
        fs_get_hostname(hostname);
        screen_print(hostname);
        screen_print(" login: ");
        return do_login();
    }

    screen_print("Password: ");
    keyboard_getline_password(password_input, sizeof(password_input));

    if (fs_verify_password(password_input)) {
        return 1;
    }

    screen_print("Login incorrect\n");
    char hostname[64];
    fs_get_hostname(hostname);
    screen_print(hostname);
    screen_print(" login: ");
    return do_login();
}

void kernel_main(void) {
    screen_init();
    screen_clear();

    screen_print("EphemeralOS v1.0\n");
    screen_print("================\n\n");

    // Inicializar RTC
    rtc_init();
    screen_print("[RTC] Real Time Clock initialized\n");

    // Inicializar PCI y Network
    pci_init();
    screen_print("[PCI] PCI bus initialized\n");
    
    network_init();
    
    // Inicializar stack de red
    eth_init();
    arp_init();
    ip_init();
    icmp_init();
    udp_init();
    dns_init();
    ntp_init();
    
    if (network_is_ready()) {
        screen_print("[NET] Network stack initialized\n");
        
        // Configurar IP de QEMU (10.0.2.15)
        network_set_ip(0x0A00020F); // 10.0.2.15
        screen_print("[NET] IP configured: 10.0.2.15\n");
        
        // Configurar DNS de QEMU (10.0.2.3)
        dns_set_server(0x0A000203); // 10.0.2.3
        screen_print("[NET] DNS server: 10.0.2.3\n");
    }
    
    screen_print("\n");

    if (!fs_check_installed()) {
        installer_run();

        screen_print("\n");
        screen_set_color(0x0A, 0x00);
        screen_print("Installation complete!\n");
        screen_set_color(0x0F, 0x00);
        
        for (volatile int i = 0; i < 30000000; i++);
        
        int choice = show_continue_menu();
        
        if (choice == 1) {
            shutdown_system();
        }
        
        screen_clear();
        screen_print("Rebooting...\n");
        for (volatile int i = 0; i < 30000000; i++);
        asm volatile("int $0x19");
    }

    keyboard_init();
    fs_init();

    char hostname[64];
    fs_get_hostname(hostname);

    screen_clear();
    screen_init();
    
    for (volatile int i = 0; i < 1000000; i++);
    
    for (int i = 0; i < 50; i++) {
        uint8_t status;
        asm volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            uint8_t dummy;
            asm volatile("inb $0x60, %0" : "=a"(dummy));
        }
    }
    
    screen_set_color(0x0F, 0x00);
    screen_print(hostname);
    screen_print(" ");
    
    do_login();

    screen_print("\n");
    char username[64];
    fs_get_username(username);
    
    screen_print("Welcome to ");
    screen_print(hostname);
    screen_print("!\n");
    
    // Mostrar fecha/hora real del login
    rtc_time_t time;
    rtc_get_time(&time);
    
    const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    screen_print("Last login: ");
    if (time.weekday >= 1 && time.weekday <= 7) {
        screen_print(weekdays[time.weekday - 1]);
    }
    screen_print(" ");
    if (time.month >= 1 && time.month <= 12) {
        screen_print(months[time.month - 1]);
    }
    screen_print(" ");
    
    char num[3];
    if (time.day < 10) screen_print(" ");
    num[0] = '0' + (time.day / 10);
    num[1] = '0' + (time.day % 10);
    num[2] = '\0';
    if (num[0] != '0') screen_print_char(num[0]);
    screen_print_char(num[1]);
    
    screen_print(" ");
    num[0] = '0' + (time.hour / 10);
    num[1] = '0' + (time.hour % 10);
    screen_print(num);
    screen_print(":");
    num[0] = '0' + (time.minute / 10);
    num[1] = '0' + (time.minute % 10);
    screen_print(num);
    screen_print(":");
    num[0] = '0' + (time.second / 10);
    num[1] = '0' + (time.second % 10);
    screen_print(num);
    screen_print(" ");
    
    // Imprimir año
    char year[5];
    year[0] = '0' + (time.year / 1000);
    year[1] = '0' + ((time.year / 100) % 10);
    year[2] = '0' + ((time.year / 10) % 10);
    year[3] = '0' + (time.year % 10);
    year[4] = '\0';
    screen_print(year);
    screen_print("\n\n");

    shell_init();
    shell_run();

    while(1) {
        asm volatile("hlt");
    }
}