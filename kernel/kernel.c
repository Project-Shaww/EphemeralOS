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

static int do_login(void) {
    char username_input[64];
    char password_input[64];
    char stored_username[64];

    fs_get_username(stored_username);

    while (1) {
        screen_set_color(0x0F, 0x00);
        screen_print("login: ");
        keyboard_getline(username_input, sizeof(username_input));

        if (strcmp(username_input, stored_username) != 0) {
            screen_print("Login incorrect\n\n");
            continue;
        }

        screen_print("Password: ");
        keyboard_getline_password(password_input, sizeof(password_input));

        if (fs_verify_password(password_input)) {
            return 1;
        }

        screen_print("Login incorrect\n\n");
    }
}

void kernel_main(void) {
    screen_clear();
    screen_init();

    screen_print("EphemeralOS v1.0\n");
    screen_print("================\n\n");
    screen_print("[DEBUG] Checking if installed...\n");

    if (!fs_check_installed()) {
        screen_print("[DEBUG] Not installed, starting installer...\n");
        installer_run();

        screen_print("\nInstallation complete! Rebooting...\n");
        for (volatile int i = 0; i < 50000000; i++);

        asm volatile("int $0x19");
    }

    screen_print("[DEBUG] System is installed\n");
    screen_print("[DEBUG] Initializing keyboard...\n");
    keyboard_init();
    
    screen_print("[DEBUG] Initializing filesystem...\n");
    fs_init();

    screen_print("[DEBUG] Loading system info...\n");
    char hostname[64];
    fs_get_hostname(hostname);

    screen_print(hostname);
    screen_print(" login: ");
    screen_print("\n\n");

    screen_print("[DEBUG] Waiting for login...\n");
    do_login();

    screen_print("\n");
    char username[64];
    fs_get_username(username);
    
    screen_print("Welcome to ");
    screen_print(hostname);
    screen_print("!\n");
    screen_print("Last login: Mon Jan  1 00:00:00 2025\n\n");

    screen_print("[DEBUG] Starting shell...\n");
    shell_init();
    shell_run();

    while(1) {
        asm volatile("hlt");
    }
}