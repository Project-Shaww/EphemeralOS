#ifndef COMMANDS_H
#define COMMANDS_H

void cmd_help(int argc, char** argv);
void cmd_clear(int argc, char** argv);
void cmd_ls(int argc, char** argv);
void cmd_cat(int argc, char** argv);
void cmd_echo(int argc, char** argv);
void cmd_touch(int argc, char** argv);
void cmd_rm(int argc, char** argv);
void cmd_mkdir(int argc, char** argv);
void cmd_cd(int argc, char** argv);
void cmd_uname(int argc, char** argv);
void cmd_date(int argc, char** argv);
void cmd_pwd(int argc, char** argv);
void cmd_whoami(int argc, char** argv);
void cmd_shutdown(int argc, char** argv);
void cmd_reboot(int argc, char** argv);
void cmd_ping(int argc, char** argv);

char* shell_get_current_dir(void);
char* shell_get_username(void);

#endif