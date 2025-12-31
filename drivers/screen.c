#include "screen.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t color = 0x0F;

static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

    asm volatile("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    asm volatile("outb %0, %1" : : "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));

    asm volatile("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    asm volatile("outb %0, %1" : : "a"((uint8_t)((pos >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
}

static void scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (color << 8) | ' ';
    }

    cursor_y = VGA_HEIGHT - 1;
}

void screen_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    color = 0x0F;
    update_cursor();
}

void screen_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = (color << 8) | ' ';
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void screen_set_color(uint8_t fg, uint8_t bg) {
    color = (bg << 4) | (fg & 0x0F);
}

void screen_newline(void) {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }
    update_cursor();
}

void screen_backspace(void) {
    if (cursor_x > 0) {
        cursor_x--;
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
        update_cursor();
    }
}

void screen_print_char(char c) {
    if (c == '\n') {
        screen_newline();
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        update_cursor();
        return;
    }

    if (c == '\b') {
        screen_backspace();
        return;
    }

    VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        screen_newline();
    }

    update_cursor();
}

void screen_print(const char* str) {
    while (*str) {
        screen_print_char(*str++);
    }
}
