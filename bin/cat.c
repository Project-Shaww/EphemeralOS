#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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