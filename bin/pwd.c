#include "commands.h"
#include "../drivers/screen.h"
#include "../shell/shell.h"

void cmd_pwd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print(shell_get_current_dir());
    screen_print("\n");
}