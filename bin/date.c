#include "commands.h"
#include "../drivers/screen.h"

void cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("Mon Jan  1 00:00:00 UTC 2025\n");
}