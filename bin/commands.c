// bin/commands.c
#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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
}

void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_clear();
}

void cmd_ls(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    char* current_dir = shell_get_current_dir();
    char buffer[2048];
    
    int result = fs_list_dir(current_dir, buffer, sizeof(buffer));
    
    if (result < 0) {
        screen_print("ls: cannot access '");
        screen_print(current_dir);
        screen_print("': No such file or directory\n");
        return;
    }
    
    if (result == 0) {
        return;
    }
    
    char* line = buffer;
    while (*line) {
        char* end = line;
        while (*end && *end != '\n') end++;
        
        char temp = *end;
        *end = '\0';
        
        int is_dir = 0;
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '/') {
            is_dir = 1;
        }
        
        if (is_dir) {
            screen_set_color(0x0B, 0x00);
        }
        screen_print(line);
        screen_set_color(0x0F, 0x00);
        screen_print("\n");
        
        if (temp == '\n') {
            line = end + 1;
        } else {
            break;
        }
    }
}

void cmd_cd(int argc, char** argv) {
    char* current_dir = shell_get_current_dir();
    
    if (argc < 2) {
        char new_dir[64];
        strcpy(new_dir, "/home/");
        strcat(new_dir, shell_get_username());
        
        if (fs_change_dir(new_dir, new_dir) == 0) {
            shell_set_current_dir(new_dir);
        }
        return;
    }

    char new_dir[256];
    strcpy(new_dir, current_dir);
    
    if (fs_change_dir(argv[1], new_dir) == 0) {
        shell_set_current_dir(new_dir);
    } else {
        screen_print("cd: ");
        screen_print(argv[1]);
        screen_print(": No such file or directory\n");
    }
}

void cmd_mkdir(int argc, char** argv) {
    if (argc < 2) {
        screen_print("Usage: mkdir <directory>\n");
        return;
    }

    char* current_dir = shell_get_current_dir();
    char full_path[256];
    
    if (argv[1][0] == '/') {
        strcpy(full_path, argv[1]);
    } else {
        if (strcmp(current_dir, "/") == 0) {
            strcpy(full_path, "/");
            strcat(full_path, argv[1]);
        } else {
            strcpy(full_path, current_dir);
            strcat(full_path, "/");
            strcat(full_path, argv[1]);
        }
    }
    
    if (fs_create_dir(full_path) != 0) {
        screen_print("mkdir: cannot create directory '");
        screen_print(argv[1]);
        screen_print("': Error\n");
    }
}

void cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        screen_print("Usage: cat <filename>\n");
        return;
    }

    char* current_dir = shell_get_current_dir();
    char full_path[256];
    
    if (argv[1][0] == '/') {
        strcpy(full_path, argv[1]);
    } else {
        if (strcmp(current_dir, "/") == 0) {
            strcpy(full_path, "/");
            strcat(full_path, argv[1]);
        } else {
            strcpy(full_path, current_dir);
            strcat(full_path, "/");
            strcat(full_path, argv[1]);
        }
    }

    uint8_t buffer[FS_MAX_FILE_SIZE];
    int size = fs_read_file(full_path, buffer, FS_MAX_FILE_SIZE);

    if (size < 0) {
        screen_print("cat: ");
        screen_print(argv[1]);
        screen_print(": No such file or directory\n");
        return;
    }

    for (int i = 0; i < size; i++) {
        screen_print_char(buffer[i]);
    }
    if (size > 0 && buffer[size - 1] != '\n') {
        screen_print("\n");
    }
}

void cmd_echo(int argc, char** argv) {
    if (argc < 2) {
        screen_print("\n");
        return;
    }

    if (argc >= 4 && strcmp(argv[argc - 2], ">") == 0) {
        char content[FS_MAX_FILE_SIZE];
        int pos = 0;

        for (int i = 1; i < argc - 2; i++) {
            int len = strlen(argv[i]);
            if (pos + len + 1 < FS_MAX_FILE_SIZE) {
                strcpy(content + pos, argv[i]);
                pos += len;
                if (i < argc - 3) {
                    content[pos++] = ' ';
                }
            }
        }
        content[pos] = '\0';

        char* current_dir = shell_get_current_dir();
        char full_path[256];
        
        if (argv[argc - 1][0] == '/') {
            strcpy(full_path, argv[argc - 1]);
        } else {
            if (strcmp(current_dir, "/") == 0) {
                strcpy(full_path, "/");
                strcat(full_path, argv[argc - 1]);
            } else {
                strcpy(full_path, current_dir);
                strcat(full_path, "/");
                strcat(full_path, argv[argc - 1]);
            }
        }

        if (fs_create_file(full_path, (uint8_t*)content, pos) != 0) {
            screen_print("echo: write error\n");
        }
    } else {
        for (int i = 1; i < argc; i++) {
            screen_print(argv[i]);
            if (i < argc - 1) screen_print(" ");
        }
        screen_print("\n");
    }
}

void cmd_touch(int argc, char** argv) {
    if (argc < 2) {
        screen_print("Usage: touch <filename>\n");
        return;
    }

    char* current_dir = shell_get_current_dir();
    char full_path[256];
    
    if (argv[1][0] == '/') {
        strcpy(full_path, argv[1]);
    } else {
        if (strcmp(current_dir, "/") == 0) {
            strcpy(full_path, "/");
            strcat(full_path, argv[1]);
        } else {
            strcpy(full_path, current_dir);
            strcat(full_path, "/");
            strcat(full_path, argv[1]);
        }
    }

    if (fs_create_file(full_path, (uint8_t*)"", 0) != 0) {
        screen_print("touch: cannot touch '");
        screen_print(argv[1]);
        screen_print("'\n");
    }
}

void cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        screen_print("Usage: rm <filename>\n");
        return;
    }

    char* current_dir = shell_get_current_dir();
    char full_path[256];
    
    if (argv[1][0] == '/') {
        strcpy(full_path, argv[1]);
    } else {
        if (strcmp(current_dir, "/") == 0) {
            strcpy(full_path, "/");
            strcat(full_path, argv[1]);
        } else {
            strcpy(full_path, current_dir);
            strcat(full_path, "/");
            strcat(full_path, argv[1]);
        }
    }

    if (fs_delete_file(full_path) != 0) {
        screen_print("rm: cannot remove '");
        screen_print(argv[1]);
        screen_print("': No such file or directory\n");
    }
}

void cmd_uname(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("EphemeralOS\n");
}

void cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print("Mon Jan  1 00:00:00 UTC 2025\n");
}

void cmd_pwd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print(shell_get_current_dir());
    screen_print("\n");
}

void cmd_whoami(int argc, char** argv) {
    (void)argc;
    (void)argv;
    screen_print(shell_get_username());
    screen_print("\n");
}

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
