// shell/shell.c
#include "shell.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../fs/filesystem.h"
#include "../bin/commands.h"
#include "../kernel/kernel.h"

#define MAX_ARGS     20
#define MAX_ARG_LEN  64

static char current_dir[256] = "/";
static char username[64] = "";

typedef struct {
    const char* name;
    void (*func)(int argc, char** argv);
} command_t;

extern void cmd_help(int argc, char** argv);
extern void cmd_clear(int argc, char** argv);
extern void cmd_ls(int argc, char** argv);
extern void cmd_cd(int argc, char** argv);
extern void cmd_mkdir(int argc, char** argv);
extern void cmd_cat(int argc, char** argv);
extern void cmd_echo(int argc, char** argv);
extern void cmd_touch(int argc, char** argv);
extern void cmd_rm(int argc, char** argv);
extern void cmd_pwd(int argc, char** argv);
extern void cmd_whoami(int argc, char** argv);
extern void cmd_uname(int argc, char** argv);
extern void cmd_date(int argc, char** argv);
extern void cmd_shutdown(int argc, char** argv);
extern void cmd_reboot(int argc, char** argv);
extern void cmd_ping(int argc, char** argv);

static const command_t commands[] = {
    {"help",     cmd_help},
    {"clear",    cmd_clear},
    {"ls",       cmd_ls},
    {"cd",       cmd_cd},
    {"mkdir",    cmd_mkdir},
    {"cat",      cmd_cat},
    {"echo",     cmd_echo},
    {"touch",    cmd_touch},
    {"rm",       cmd_rm},
    {"pwd",      cmd_pwd},
    {"whoami",   cmd_whoami},
    {"uname",    cmd_uname},
    {"date",     cmd_date},
    {"shutdown", cmd_shutdown},
    {"reboot",   cmd_reboot},
    {"ping",     cmd_ping},
};

static const int num_commands = sizeof(commands) / sizeof(command_t);

static char argv_buffer[MAX_ARGS][MAX_ARG_LEN];

static void parse_command(const char* input, char** argv, int* argc) {
    *argc = 0;
    int in_quotes = 0;
    int i = 0;
    int buf_pos = 0;

    while (input[i] && *argc < MAX_ARGS) {
        char c = input[i];

        if (c == '"') {
            in_quotes = !in_quotes;
            i++;
            continue;
        }

        if ((c == ' ' || c == '\t') && !in_quotes) {
            if (buf_pos > 0) {
                argv_buffer[*argc][buf_pos] = '\0';
                argv[*argc] = argv_buffer[*argc];
                (*argc)++;
                buf_pos = 0;
            }
            i++;
            continue;
        }

        if (buf_pos < MAX_ARG_LEN - 1) {
            argv_buffer[*argc][buf_pos++] = c;
        }

        i++;
    }

    if (buf_pos > 0 && *argc < MAX_ARGS) {
        argv_buffer[*argc][buf_pos] = '\0';
        argv[*argc] = argv_buffer[*argc];
        (*argc)++;
    }
}

static void get_display_path(char* output) {
    char user_home[128];
    strcpy(user_home, "/home/");
    strcat(user_home, username);
    
    size_t home_len = strlen(user_home);
    size_t current_len = strlen(current_dir);
    
    if (strcmp(current_dir, user_home) == 0) {
        strcpy(output, "~");
        return;
    }
    
    if (current_len > home_len && 
        current_dir[home_len] == '/' &&
        strncmp(current_dir, user_home, home_len) == 0) {
        
        strcpy(output, "~");
        strcat(output, current_dir + home_len);
        return;
    }
    
    strcpy(output, current_dir);
}

static void print_prompt(void) {
    screen_set_color(0x0A, 0x00);
    screen_print(username);

    screen_set_color(0x0F, 0x00);
    screen_print("@");

    char hostname[64];
    fs_get_hostname(hostname);

    screen_set_color(0x0B, 0x00);
    screen_print(hostname);

    screen_set_color(0x0F, 0x00);
    screen_print(":");

    char display_path[256];
    get_display_path(display_path);
    
    screen_set_color(0x0E, 0x00);
    screen_print(display_path);

    screen_set_color(0x0F, 0x00);
    screen_print("$ ");
}

void shell_init(void) {
    fs_get_username(username);
    strcpy(current_dir, "/");

    char user_home[128];
    strcpy(user_home, "/home/");
    strcat(user_home, username);

    if (fs_dir_exists(user_home)) {
        strcpy(current_dir, user_home);
    }
}

void shell_run(void) {
    char command_line[256];

    while (1) {
        print_prompt();

        keyboard_getline(command_line, sizeof(command_line));

        if (strlen(command_line) == 0) {
            continue;
        }

        char* argv[MAX_ARGS];
        int argc = 0;

        parse_command(command_line, argv, &argc);

        if (argc == 0) {
            continue;
        }

        int found = 0;
        for (int i = 0; i < num_commands; i++) {
            if (strcmp(commands[i].name, argv[0]) == 0) {
                commands[i].func(argc, argv);
                found = 1;
                break;
            }
        }

        if (!found) {
            screen_print(argv[0]);
            screen_print(": command not found\n");
        }
    }
}

char* shell_get_current_dir(void) {
    return current_dir;
}

void shell_set_current_dir(const char* dir) {
    strcpy(current_dir, dir);
}

char* shell_get_username(void) {
    return username;
}