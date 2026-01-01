#include "commands.h"
#include "../drivers/screen.h"

void cmd_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("Rebooting...\n");
    for (volatile int i = 0; i < 50000000; i++);

    uint8_t temp;
    asm volatile("inb $0x64, %0" : "=a"(temp));
    temp |= 0xFE;
    asm volatile("outb %0, $0x64" : : "a"(temp));

    asm volatile("int $0x19");

    while(1) {
        asm volatile("hlt");
    }
}
