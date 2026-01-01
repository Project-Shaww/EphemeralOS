#include "commands.h"
#include "../drivers/screen.h"

void cmd_uname(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("EphemeralOS\n");
}