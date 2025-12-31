// drivers/keyboard.c
#include "keyboard.h"
#include "screen.h"

static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static int shift_pressed = 0;

void keyboard_init(void) {
    shift_pressed = 0;
}

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint8_t keyboard_get_scancode(void) {
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            return inb(0x60);
        }
    }
}

char keyboard_getchar(void) {
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            uint8_t scancode = inb(0x60);

            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
                continue;
            }

            if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = 0;
                continue;
            }

            if (scancode & 0x80) {
                continue;
            }

            if (scancode < sizeof(scancode_to_ascii)) {
                char c = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
                if (c) {
                    return c;
                }
            }
        }
    }
}

void keyboard_getline(char* buffer, int max_length) {
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
            screen_print_char(c);
        }
    }
}
