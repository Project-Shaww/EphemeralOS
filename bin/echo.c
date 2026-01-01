#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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