#include "shell.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../fs/filesystem.h"
#include "../bin/commands.h"

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16

static char current_dir[64] = "/";
static char username[64];

static void parse_command(char* cmd, char** args, int* argc) {
    *argc = 0;
    int in_token = 0;

    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t') {
            if (in_token) {
                *cmd = '\0';
                in_token = 0;
            }
        } else {
            if (!in_token) {
                args[(*argc)++] = cmd;
                in_token = 1;
            }
        }
        cmd++;
    }
    args[*argc] = NULL;
}

static void execute_command(char** args, int argc) {
    if (argc == 0) return;

    if (strcmp(args[0], "help") == 0) {
        cmd_help(argc, args);
    } else if (strcmp(args[0], "clear") == 0) {
        cmd_clear(argc, args);
    } else if (strcmp(args[0], "ls") == 0) {
        cmd_ls(argc, args);
    } else if (strcmp(args[0], "cat") == 0) {
        cmd_cat(argc, args);
    } else if (strcmp(args[0], "echo") == 0) {
        cmd_echo(argc, args);
    } else if (strcmp(args[0], "touch") == 0) {
        cmd_touch(argc, args);
    } else if (strcmp(args[0], "rm") == 0) {
        cmd_rm(argc, args);
    } else if (strcmp(args[0], "mkdir") == 0) {
        cmd_mkdir(argc, args);
    } else if (strcmp(args[0], "cd") == 0) {
        cmd_cd(argc, args);
    } else if (strcmp(args[0], "uname") == 0) {
        cmd_uname(argc, args);
    } else if (strcmp(args[0], "date") == 0) {
        cmd_date(argc, args);
    } else if (strcmp(args[0], "pwd") == 0) {
        cmd_pwd(argc, args);
    } else if (strcmp(args[0], "whoami") == 0) {
        cmd_whoami(argc, args);
    } else if (strcmp(args[0], "shutdown") == 0) {
        cmd_shutdown(argc, args);
    } else if (strcmp(args[0], "reboot") == 0) {
        cmd_reboot(argc, args);
    } else {
        screen_print(args[0]);
        screen_print(": command not found\n");
    }
}

void shell_init(void) {
    fs_get_username(username);
    strcpy(current_dir, "/");
}

void shell_run(void) {
    char command[MAX_COMMAND_LENGTH];
    char* args[MAX_ARGS];
    int argc;

    while (1) {
        screen_set_color(0x0A, 0x00);
        screen_print(username);
        screen_set_color(0x0F, 0x00);
        
        if (strcmp(username, "root") == 0) {
            screen_print("# ");
        } else {
            screen_print("$ ");
        }

        keyboard_getline(command, MAX_COMMAND_LENGTH);

        parse_command(command, args, &argc);
        execute_command(args, argc);
    }
}

char* shell_get_current_dir(void) {
    return current_dir;
}

void shell_set_current_dir(const char* dir) {
    strncpy(current_dir, dir, 63);
    current_dir[63] = '\0';
}

char* shell_get_username(void) {
    return username;
}