#include "commands.h"
#include "../drivers/screen.h"

void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_clear();
}
