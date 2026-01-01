#include "commands.h"
#include "../drivers/screen.h"

void cmd_shutdown(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("Shutting down...\n");
    for (volatile int i = 0; i < 50000000; i++);

    asm volatile("mov $0x2000, %ax; mov %ax, %dx; out %ax, %dx");
    asm volatile("mov $0x5307, %ax; mov $0x0001, %bx; mov $0x0003, %cx; int $0x15");

    while(1) {
        asm volatile("hlt");
    }
}