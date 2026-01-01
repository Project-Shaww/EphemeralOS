#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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
