#include "commands.h"
#include "../drivers/screen.h"
#include "../shell/shell.h"

void cmd_whoami(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print(shell_get_username());
    screen_print("\n");
}