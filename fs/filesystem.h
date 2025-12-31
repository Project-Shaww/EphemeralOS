// fs/filesystem.h
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../kernel/kernel.h"

#define FS_MAGIC 0x45504853
#define FS_VERSION 2

#define FS_SUPERBLOCK_SECTOR 100
#define FS_INODE_TABLE_SECTOR 101
#define FS_BLOCK_BITMAP_SECTOR 151
#define FS_DATA_START_SECTOR 201

#define FS_MAX_INODES 200
#define FS_MAX_BLOCKS 512
#define FS_BLOCK_SIZE 512
#define FS_MAX_FILENAME 28
#define FS_MAX_FILE_SIZE 16384
#define FS_INODE_DIRECT_BLOCKS 12
#define FS_MAX_DIR_ENTRIES 15

#define INODE_TYPE_FREE 0
#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR 2

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t free_inodes;
    uint32_t free_blocks;
    uint32_t root_inode;
    uint32_t installed;
    char hostname[64];
    char username[64];
    uint8_t password_hash[32];
} fs_superblock_t;

typedef struct {
    uint8_t type;
    uint32_t size;
    uint32_t blocks[FS_INODE_DIRECT_BLOCKS];
    uint32_t parent_inode;
} fs_inode_t;

typedef struct {
    char name[FS_MAX_FILENAME];
    uint32_t inode;
    uint8_t in_use;
} fs_dir_entry_t;

typedef struct {
    fs_dir_entry_t entries[FS_MAX_DIR_ENTRIES];
} fs_dir_block_t;

void fs_init(void);
int fs_check_installed(void);
void fs_install(const char* hostname, const char* username, const char* password);
void fs_format(void);

void fs_get_hostname(char* buffer);
void fs_get_username(char* buffer);
int fs_verify_password(const char* password);

int fs_create_file(const char* path, const uint8_t* data, uint32_t size);
int fs_read_file(const char* path, uint8_t* buffer, uint32_t max_size);
int fs_delete_file(const char* path);

int fs_create_dir(const char* path);
int fs_list_dir(const char* path, char* buffer, int max_size);
int fs_change_dir(const char* path, char* current_dir);
int fs_dir_exists(const char* path);

#endif
