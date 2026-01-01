#include "commands.h"
#include "../drivers/screen.h"
#include "../fs/filesystem.h"
#include "../kernel/kernel.h"
#include "../shell/shell.h"

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