#include "installer.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../fs/filesystem.h"

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

void installer_run(void) {
    char hostname[64];
    char username[64];
    char password[64];
    char password_confirm[64];

    screen_set_color(0x0B, 0x00);
    screen_print("===================================\n");
    screen_print("   EphemeralOS Installation Wizard\n");
    screen_print("===================================\n\n");
    screen_set_color(0x0F, 0x00);

    screen_print("Welcome! Let's set up your system.\n\n");

    screen_print("Enter device hostname: ");
    keyboard_getline(hostname, sizeof(hostname));

    screen_print("Enter username: ");
    keyboard_getline(username, sizeof(username));

    while (1) {
        screen_print("Enter password: ");
        keyboard_getline_password(password, sizeof(password));

        screen_print("Confirm password: ");
        keyboard_getline_password(password_confirm, sizeof(password_confirm));

        if (strcmp(password, password_confirm) == 0) {
            break;
        }

        screen_set_color(0x0C, 0x00);
        screen_print("Passwords do not match. Please try again.\n\n");
        screen_set_color(0x0F, 0x00);
    }

    screen_print("\n");
    screen_set_color(0x0A, 0x00);
    screen_print("Installing EphemeralOS...\n");
    screen_set_color(0x0F, 0x00);

    screen_print("- Creating file system...\n");
    for (volatile int i = 0; i < 10000000; i++);

    screen_print("- Setting up /bin directory...\n");
    for (volatile int i = 0; i < 10000000; i++);

    screen_print("- Configuring system...\n");
    for (volatile int i = 0; i < 10000000; i++);

    fs_install(hostname, username, password);

    screen_print("- Writing configuration to disk...\n");
    for (volatile int i = 0; i < 10000000; i++);

    screen_set_color(0x0A, 0x00);
    screen_print("\nInstallation successful!\n");
    screen_set_color(0x0F, 0x00);
}