// drivers/screen.c
#include "screen.h"

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_foreground = 0x0F;
static uint8_t current_background = 0x00;

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define VIDEO_MEMORY 0xB8000

static void update_cursor(void) {
    int pos = cursor_y * SCREEN_WIDTH + cursor_x;
    
    uint8_t high = (pos >> 8) & 0xFF;
    uint8_t low = pos & 0xFF;
    
    asm volatile("outb %0, $0x3D4" : : "a"((uint8_t)0x0E));
    asm volatile("outb %0, $0x3D5" : : "a"(high));
    
    asm volatile("outb %0, $0x3D4" : : "a"((uint8_t)0x0F));
    asm volatile("outb %0, $0x3D5" : : "a"(low));
}

static void scroll_down(void) {
    uint16_t* video = (uint16_t*)VIDEO_MEMORY;
    
    for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT - 1); i++) {
        video[i] = video[i + SCREEN_WIDTH];
    }
    
    uint16_t space = (current_background << 12) | (current_foreground << 8) | ' ';
    for (int i = SCREEN_WIDTH * (SCREEN_HEIGHT - 1); i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video[i] = space;
    }
    
    cursor_y = SCREEN_HEIGHT - 1;
    cursor_x = 0;
    update_cursor();
}

void screen_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_foreground = 0x0F;
    current_background = 0x00;
    update_cursor();
}

void screen_clear(void) {
    uint16_t* video = (uint16_t*)VIDEO_MEMORY;
    uint16_t blank = (current_background << 12) | (current_foreground << 8) | ' ';
    
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video[i] = blank;
    }
    
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void screen_set_row(int row) {
    if (row >= 0 && row < SCREEN_HEIGHT) {
        cursor_y = row;
        cursor_x = 0;
        update_cursor();
    }
}

void screen_set_color(uint8_t foreground, uint8_t background) {
    current_foreground = foreground;
    current_background = background;
}

void screen_print_char(char c) {
    uint16_t* video = (uint16_t*)VIDEO_MEMORY;
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= SCREEN_HEIGHT) {
            scroll_down();
        }
        update_cursor();
        return;
    }
    
    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
        int pos = cursor_y * SCREEN_WIDTH + cursor_x;
        video[pos] = (current_background << 12) | (current_foreground << 8) | ' ';
        update_cursor();
        return;
    }
    
    int pos = cursor_y * SCREEN_WIDTH + cursor_x;
    video[pos] = (current_background << 12) | (current_foreground << 8) | c;
    
    cursor_x++;
    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= SCREEN_HEIGHT) {
            scroll_down();
        }
    }
    
    update_cursor();
}

void screen_backspace(void) {
    uint16_t* video = (uint16_t*)VIDEO_MEMORY;
    
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = SCREEN_WIDTH - 1;
    }
    
    int pos = cursor_y * SCREEN_WIDTH + cursor_x;
    video[pos] = (current_background << 12) | (current_foreground << 8) | ' ';
    
    update_cursor();
}

void screen_print(const char* str) {
    while (*str) {
        screen_print_char(*str);
        str++;
    }
}
