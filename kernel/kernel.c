// kernel/kernel.c
#include "kernel.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../drivers/disk.h"
#include "../fs/filesystem.h"
#include "../installer/installer.h"
#include "../shell/shell.h"

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

// Helper para leer un scancode del teclado sin usar keyboard_get_scancode()
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
    
    // Vaciar buffer del teclado MUCHO más agresivamente
    for (volatile int d = 0; d < 10000000; d++);  // Delay largo
    
    for (int i = 0; i < 100; i++) {
        uint8_t status;
        asm volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            uint8_t dummy;
            asm volatile("inb $0x60, %0" : "=a"(dummy));
        }
        for (volatile int j = 0; j < 10000; j++);
    }
    
    // Dibujar el menú UNA sola vez al inicio
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
        // Solo actualizar las opciones, no todo
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
        
        // Leer scancode directamente
        uint8_t scancode = read_keyboard_direct();
        
        // Ignorar key releases (bit 7 = 1)
        if (scancode & 0x80) {
            continue;
        }
        
        // CAMBIO: Usar LEFT/RIGHT en vez de UP/DOWN
        if (scancode == 0x4B) {  // Flecha IZQUIERDA
            selected = 0;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x4D) {  // Flecha DERECHA
            selected = 1;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x48) {  // Flecha ARRIBA (también funciona)
            selected = 0;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x50) {  // Flecha ABAJO (también funciona)
            selected = 1;
            for (volatile int i = 0; i < 500000; i++);
        } else if (scancode == 0x1C) {  // Enter
            // Esperar a que se suelte Enter
            while (1) {
                scancode = read_keyboard_direct();
                if (scancode == 0x9C) break;  // Enter release
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

    // Primera vez, solo imprimir "login:"
    screen_set_color(0x0F, 0x00);
    screen_print("login: ");
    keyboard_getline(username_input, sizeof(username_input));

    if (strcmp(username_input, stored_username) != 0) {
        screen_print("Login incorrect\n");
        // En intentos fallidos, imprimir de nuevo el prompt completo
        char hostname[64];
        fs_get_hostname(hostname);
        screen_print(hostname);
        screen_print(" login: ");
        return do_login();  // Recursión
    }

    screen_print("Password: ");
    keyboard_getline_password(password_input, sizeof(password_input));

    if (fs_verify_password(password_input)) {
        return 1;
    }

    screen_print("Login incorrect\n");
    // Si falla, mostrar prompt completo de nuevo
    char hostname[64];
    fs_get_hostname(hostname);
    screen_print(hostname);
    screen_print(" login: ");
    return do_login();  // Recursión
}

void kernel_main(void) {
    screen_init();
    screen_clear();

    screen_print("EphemeralOS v1.0\n");
    screen_print("================\n\n");

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

    // ARREGLO: Obtener hostname ANTES de clear
    char hostname[64];
    fs_get_hostname(hostname);

    // Clear + init + delay para sincronizar todo
    screen_clear();
    screen_init();
    
    // Pequeño delay para que el VGA se estabilice
    for (volatile int i = 0; i < 1000000; i++);
    
    // Vaciar buffer del teclado completamente
    for (int i = 0; i < 50; i++) {
        uint8_t status;
        asm volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            uint8_t dummy;
            asm volatile("inb $0x60, %0" : "=a"(dummy));
        }
    }
    
    // Imprimir el prompt de login UNA SOLA VEZ
    screen_set_color(0x0F, 0x00);
    screen_print(hostname);
    screen_print(" ");
    
    // Llamar a do_login SIN imprimir "login:" aquí
    do_login();

    screen_print("\n");
    char username[64];
    fs_get_username(username);
    
    screen_print("Welcome to ");
    screen_print(hostname);
    screen_print("!\n");
    screen_print("Last login: Mon Jan  1 00:00:00 2025\n\n");

    shell_init();
    shell_run();

    while(1) {
        asm volatile("hlt");
    }
}