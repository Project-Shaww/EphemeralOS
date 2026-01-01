#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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