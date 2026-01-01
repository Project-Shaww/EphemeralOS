#include "commands.h"
#include "../drivers/screen.h"

void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("\nEphemeralOS Available Commands:\n");
    screen_print("================================\n");
    screen_print("help      - Show this help message\n");
    screen_print("clear     - Clear the screen\n");
    screen_print("ls        - List files and directories\n");
    screen_print("cd        - Change directory\n");
    screen_print("mkdir     - Create directory\n");
    screen_print("cat       - Display file contents\n");
    screen_print("echo      - Print text or create file\n");
    screen_print("touch     - Create empty file\n");
    screen_print("rm        - Remove file\n");
    screen_print("pwd       - Print working directory\n");
    screen_print("whoami    - Print current user\n");
    screen_print("uname     - Show system information\n");
    screen_print("date      - Show current date/time\n");
    screen_print("shutdown  - Power off the system\n");
    screen_print("reboot    - Restart the system\n\n");
    screen_print("ping      - Test network connectivity\n");
    screen_print("nettest   - Test network hardware\n");
}