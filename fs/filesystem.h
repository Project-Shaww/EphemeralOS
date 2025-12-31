#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../kernel/kernel.h"

// Configuración del filesystem
#define FS_MAGIC 0x45504853
#define FS_VERSION 2

// Layout del disco
#define FS_SUPERBLOCK_SECTOR 100
#define FS_INODE_TABLE_SECTOR 101
#define FS_BLOCK_BITMAP_SECTOR 151
#define FS_DATA_START_SECTOR 201

// Límites
#define FS_MAX_INODES 200
#define FS_MAX_BLOCKS 512
#define FS_BLOCK_SIZE 512
#define FS_MAX_FILENAME 28
#define FS_MAX_FILE_SIZE 16384
#define FS_INODE_DIRECT_BLOCKS 12
#define FS_MAX_DIR_ENTRIES 16

// Tipos de inodo
#define INODE_TYPE_FREE 0
#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR 2

// Superblock - Información del filesystem
typedef struct {
    uint32_t magic;              // Identificador mágico
    uint32_t version;            // Versión del FS
    uint32_t total_inodes;       // Total de inodos
    uint32_t total_blocks;       // Total de bloques
    uint32_t free_inodes;        // Inodos libres
    uint32_t free_blocks;        // Bloques libres
    uint32_t root_inode;         // Inodo del directorio raíz
    uint32_t installed;          // Sistema instalado?
    char hostname[64];           // Nombre del host
    char username[64];           // Usuario del sistema
    uint8_t password_hash[32];   // Hash de la contraseña
} fs_superblock_t;

// Inodo - Representa un archivo o directorio
typedef struct {
    uint8_t type;                           // Tipo: libre/archivo/directorio
    uint32_t size;                          // Tamaño en bytes
    uint32_t blocks[FS_INODE_DIRECT_BLOCKS]; // Bloques de datos directos
    uint32_t parent_inode;                  // Inodo del directorio padre
} fs_inode_t;

// Entrada de directorio
typedef struct {
    char name[FS_MAX_FILENAME];  // Nombre del archivo/directorio
    uint32_t inode;              // Número de inodo
    uint8_t in_use;              // Entrada en uso?
} fs_dir_entry_t;

// Bloque de directorio
typedef struct {
    fs_dir_entry_t entries[FS_INODE_DIRECT_BLOCKS];
} fs_dir_block_t;

// Funciones principales
void fs_init(void);
int fs_check_installed(void);
void fs_install(const char* hostname, const char* username, const char* password);
void fs_format(void);

// Autenticación
void fs_get_hostname(char* buffer);
void fs_get_username(char* buffer);
int fs_verify_password(const char* password);

// Operaciones con archivos
int fs_create_file(const char* path, const uint8_t* data, uint32_t size);
int fs_read_file(const char* path, uint8_t* buffer, uint32_t max_size);
int fs_delete_file(const char* path);

// Operaciones con directorios
int fs_create_dir(const char* path);
int fs_list_dir(const char* path, char* buffer, int max_size);
int fs_change_dir(const char* path, char* current_dir);
int fs_dir_exists(const char* path);

#endif