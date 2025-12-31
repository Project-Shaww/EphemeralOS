#include "filesystem.h"
#include "../drivers/disk.h"
#include "../drivers/screen.h"

static fs_superblock_t superblock;
static fs_inode_t inode_table[FS_MAX_INODES];
static uint8_t block_bitmap[FS_MAX_BLOCKS / 8];
static uint8_t sector_buffer[512];

// ============================================================================
// UTILIDADES INTERNAS
// ============================================================================

static uint32_t simple_hash(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

static int is_block_free(uint32_t block_num) {
    if (block_num >= FS_MAX_BLOCKS) return 0;
    uint32_t byte_index = block_num / 8;
    uint32_t bit_index = block_num % 8;
    return !(block_bitmap[byte_index] & (1 << bit_index));
}

static void mark_block_used(uint32_t block_num) {
    if (block_num >= FS_MAX_BLOCKS) return;
    uint32_t byte_index = block_num / 8;
    uint32_t bit_index = block_num % 8;
    block_bitmap[byte_index] |= (1 << bit_index);
    superblock.free_blocks--;
}

static void mark_block_free(uint32_t block_num) {
    if (block_num >= FS_MAX_BLOCKS) return;
    uint32_t byte_index = block_num / 8;
    uint32_t bit_index = block_num % 8;
    block_bitmap[byte_index] &= ~(1 << bit_index);
    superblock.free_blocks++;
}

static uint32_t allocate_block(void) {
    for (uint32_t i = 0; i < FS_MAX_BLOCKS; i++) {
        if (is_block_free(i)) {
            mark_block_used(i);
            return i;
        }
    }
    return 0xFFFFFFFF;
}

static uint32_t allocate_inode(void) {
    for (uint32_t i = 0; i < FS_MAX_INODES; i++) {
        if (inode_table[i].type == INODE_TYPE_FREE) {
            inode_table[i].type = INODE_TYPE_FILE;
            inode_table[i].size = 0;
            inode_table[i].parent_inode = 0;
            for (int j = 0; j < FS_INODE_DIRECT_BLOCKS; j++) {
                inode_table[i].blocks[j] = 0;
            }
            superblock.free_inodes--;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

static void free_inode(uint32_t inode_num) {
    if (inode_num >= FS_MAX_INODES) return;
    
    for (int i = 0; i < FS_INODE_DIRECT_BLOCKS; i++) {
        if (inode_table[inode_num].blocks[i] != 0) {
            mark_block_free(inode_table[inode_num].blocks[i]);
            inode_table[inode_num].blocks[i] = 0;
        }
    }
    
    inode_table[inode_num].type = INODE_TYPE_FREE;
    inode_table[inode_num].size = 0;
    superblock.free_inodes++;
}

// ============================================================================
// LECTURA/ESCRITURA DE ESTRUCTURAS
// ============================================================================

static void sync_superblock(void) {
    disk_write_sectors(FS_SUPERBLOCK_SECTOR, 1, (uint8_t*)&superblock);
}

static void sync_inode_table(void) {
    uint32_t sectors_needed = (sizeof(inode_table) + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        disk_write_sectors(FS_INODE_TABLE_SECTOR + i, 1, 
                          ((uint8_t*)inode_table) + (i * 512));
    }
}

static void sync_block_bitmap(void) {
    uint32_t sectors_needed = (sizeof(block_bitmap) + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        disk_write_sectors(FS_BLOCK_BITMAP_SECTOR + i, 1, 
                          ((uint8_t*)block_bitmap) + (i * 512));
    }
}

static void load_superblock(void) {
    disk_read_sectors(FS_SUPERBLOCK_SECTOR, 1, (uint8_t*)&superblock);
}

static void load_inode_table(void) {
    uint32_t sectors_needed = (sizeof(inode_table) + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        disk_read_sectors(FS_INODE_TABLE_SECTOR + i, 1, 
                         ((uint8_t*)inode_table) + (i * 512));
    }
}

static void load_block_bitmap(void) {
    uint32_t sectors_needed = (sizeof(block_bitmap) + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        disk_read_sectors(FS_BLOCK_BITMAP_SECTOR + i, 1, 
                         ((uint8_t*)block_bitmap) + (i * 512));
    }
}

static void read_inode_block(uint32_t block_num, uint8_t* buffer) {
    if (block_num == 0 || block_num >= FS_MAX_BLOCKS) {
        memset(buffer, 0, 512);
        return;
    }
    disk_read_sectors(FS_DATA_START_SECTOR + block_num, 1, buffer);
}

static void write_inode_block(uint32_t block_num, uint8_t* buffer) {
    if (block_num == 0 || block_num >= FS_MAX_BLOCKS) {
        return;
    }
    disk_write_sectors(FS_DATA_START_SECTOR + block_num, 1, buffer);
}

// ============================================================================
// FUNCIONES DE RUTAS
// ============================================================================

static void split_path(const char* path, char* parent, char* name) {
    if (path[0] != '/') {
        strcpy(parent, "/");
        strcpy(name, path);
        return;
    }
    
    const char* last_slash = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    if (last_slash == path) {
        strcpy(parent, "/");
        strcpy(name, path + 1);
    } else {
        int parent_len = last_slash - path;
        memcpy(parent, path, parent_len);
        parent[parent_len] = '\0';
        strcpy(name, last_slash + 1);
    }
}

static uint32_t find_inode_by_path(const char* path) {
    if (strcmp(path, "/") == 0) {
        return superblock.root_inode;
    }
    
    char remaining_path[256];
    strcpy(remaining_path, path);
    
    if (remaining_path[0] == '/') {
        memmove(remaining_path, remaining_path + 1, strlen(remaining_path));
    }
    
    uint32_t current_inode = superblock.root_inode;
    char* token = remaining_path;
    
    while (*token) {
        char* next_slash = token;
        while (*next_slash && *next_slash != '/') next_slash++;
        
        char component[FS_MAX_FILENAME];
        int len = next_slash - token;
        memcpy(component, token, len);
        component[len] = '\0';
        
        if (len == 0) break;
        
        if (current_inode >= FS_MAX_INODES) {
            screen_print("[DEBUG] find_inode_by_path: Inode out of bounds!\n");
            return 0xFFFFFFFF;
        }
        
        if (inode_table[current_inode].type != INODE_TYPE_DIR) {
            screen_print("[DEBUG] find_inode_by_path: Not a directory!\n");
            return 0xFFFFFFFF;
        }
        
        if (inode_table[current_inode].blocks[0] == 0 || 
            inode_table[current_inode].blocks[0] >= FS_MAX_BLOCKS) {
            if (inode_table[current_inode].blocks[0] == 0) screen_print("[DEBUG] find_inode_by_path: Block is 0!\n");
            if (inode_table[current_inode].blocks[0] >= FS_MAX_BLOCKS) screen_print("[DEBUG] find_inode_by_path: Block out of bounds!\n");
            return 0xFFFFFFFF;
        }
        
        fs_dir_block_t dir_block;
        read_inode_block(inode_table[current_inode].blocks[0], (uint8_t*)&dir_block);
        
        uint32_t found_inode = 0xFFFFFFFF;
        for (int i = 0; i < FS_MAX_DIR_ENTRIES; i++) {
            if (dir_block.entries[i].in_use && 
                strcmp(dir_block.entries[i].name, component) == 0) {
                found_inode = dir_block.entries[i].inode;
                break;
            }
        }
        
        if (found_inode == 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        
        current_inode = found_inode;
        
        if (*next_slash == '/') {
            token = next_slash + 1;
        } else {
            break;
        }
    }
    
    return current_inode;
}

// ============================================================================
// FUNCIONES PÚBLICAS - INICIALIZACIÓN
// ============================================================================

void fs_init(void) {
    load_superblock();
    load_inode_table();
    load_block_bitmap();
}

int fs_check_installed(void) {
    load_superblock();
    return superblock.magic == FS_MAGIC && 
           superblock.version == FS_VERSION && 
           superblock.installed == 1;
}

void fs_format(void) {
    memset(&superblock, 0, sizeof(fs_superblock_t));
    memset(inode_table, 0, sizeof(inode_table));
    memset(block_bitmap, 0, sizeof(block_bitmap));
    
    superblock.magic = FS_MAGIC;
    superblock.version = FS_VERSION;
    superblock.total_inodes = FS_MAX_INODES;
    superblock.total_blocks = FS_MAX_BLOCKS;
    superblock.free_inodes = FS_MAX_INODES;
    superblock.free_blocks = FS_MAX_BLOCKS;
    superblock.installed = 0;
    superblock.root_inode = 0;
    
    for (uint32_t i = 0; i < FS_MAX_INODES; i++) {
        inode_table[i].type = INODE_TYPE_FREE;
    }
    
    sync_superblock();
    sync_inode_table();
    sync_block_bitmap();
}

void fs_install(const char* hostname, const char* username, const char* password) {
    fs_format();
    
    strcpy(superblock.hostname, hostname);
    strcpy(superblock.username, username);
    
    uint32_t hash = simple_hash(password);
    for (int i = 0; i < 32; i++) {
        superblock.password_hash[i] = (hash >> (i % 32)) & 0xFF;
    }
    
    // Crear el inodo raíz (forzar que sea el inodo 0)
    uint32_t root_inode = 0;
    inode_table[0].type = INODE_TYPE_DIR;
    inode_table[0].size = 0;
    inode_table[0].parent_inode = 0;
    superblock.free_inodes--;
    
    // Asignar bloque para el directorio raíz
    uint32_t root_block = allocate_block();
    inode_table[root_inode].blocks[0] = root_block;
    
    // Inicializar el directorio raíz
    fs_dir_block_t root_dir;
    memset(&root_dir, 0, sizeof(fs_dir_block_t));
    
    strcpy(root_dir.entries[0].name, ".");
    root_dir.entries[0].inode = root_inode;
    root_dir.entries[0].in_use = 1;
    
    strcpy(root_dir.entries[1].name, "..");
    root_dir.entries[1].inode = root_inode;
    root_dir.entries[1].in_use = 1;
    
    write_inode_block(root_block, (uint8_t*)&root_dir);
    
    // Configurar el superblock ANTES de crear subdirectorios
    superblock.root_inode = root_inode;
    superblock.installed = 1;
    
    // Sincronizar ANTES de crear subdirectorios
    sync_superblock();
    sync_inode_table();
    sync_block_bitmap();
    
    // Ahora crear los subdirectorios
    fs_create_dir("/bin");
    fs_create_dir("/home");
    fs_create_dir("/tmp");
    
    char user_home[128];
    strcpy(user_home, "/home/");
    strcat(user_home, username);
    fs_create_dir(user_home);
    
    // Sincronizar de nuevo al final
    sync_superblock();
    sync_inode_table();
    sync_block_bitmap();
}

// ============================================================================
// FUNCIONES PÚBLICAS - AUTENTICACIÓN
// ============================================================================

void fs_get_hostname(char* buffer) {
    strcpy(buffer, superblock.hostname);
}

void fs_get_username(char* buffer) {
    strcpy(buffer, superblock.username);
}

int fs_verify_password(const char* password) {
    uint32_t hash = simple_hash(password);
    uint8_t hash_bytes[32];
    for (int i = 0; i < 32; i++) {
        hash_bytes[i] = (hash >> (i % 32)) & 0xFF;
    }
    return memcmp(hash_bytes, superblock.password_hash, 32) == 0;
}

// ============================================================================
// FUNCIONES PÚBLICAS - DIRECTORIOS
// ============================================================================

int fs_create_dir(const char* path) {
    char parent_path[256];
    char dir_name[FS_MAX_FILENAME];
    split_path(path, parent_path, dir_name);
    
    if (strlen(dir_name) == 0 || strlen(dir_name) >= FS_MAX_FILENAME) {
        return -1;
    }
    
    uint32_t parent_inode = find_inode_by_path(parent_path);
    if (parent_inode == 0xFFFFFFFF || 
        inode_table[parent_inode].type != INODE_TYPE_DIR) {
        return -1;
    }
    
    uint32_t new_inode = allocate_inode();
    if (new_inode == 0xFFFFFFFF) return -1;
    
    inode_table[new_inode].type = INODE_TYPE_DIR;
    inode_table[new_inode].size = 0;
    inode_table[new_inode].parent_inode = parent_inode;
    
    uint32_t new_block = allocate_block();
    if (new_block == 0xFFFFFFFF) {
        free_inode(new_inode);
        return -1;
    }
    
    inode_table[new_inode].blocks[0] = new_block;
    
    fs_dir_block_t new_dir;
    memset(&new_dir, 0, sizeof(fs_dir_block_t));
    
    strcpy(new_dir.entries[0].name, ".");
    new_dir.entries[0].inode = new_inode;
    new_dir.entries[0].in_use = 1;
    
    strcpy(new_dir.entries[1].name, "..");
    new_dir.entries[1].inode = parent_inode;
    new_dir.entries[1].in_use = 1;
    
    write_inode_block(new_block, (uint8_t*)&new_dir);
    
    fs_dir_block_t parent_dir;
    read_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    for (int i = 0; i < FS_MAX_DIR_ENTRIES; i++) {
        if (!parent_dir.entries[i].in_use) {
            strcpy(parent_dir.entries[i].name, dir_name);
            parent_dir.entries[i].inode = new_inode;
            parent_dir.entries[i].in_use = 1;
            break;
        }
    }
    
    write_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    sync_inode_table();
    sync_block_bitmap();
    
    return 0;
}

int fs_dir_exists(const char* path) {
    uint32_t inode = find_inode_by_path(path);
    if (inode == 0xFFFFFFFF) return 0;
    if (inode >= FS_MAX_INODES) return 0;
    return inode_table[inode].type == INODE_TYPE_DIR;
}

int fs_list_dir(const char* path, char* buffer, int max_size) {
    uint32_t inode = find_inode_by_path(path);
    
    if (inode == 0xFFFFFFFF || inode >= FS_MAX_INODES) {
        buffer[0] = '\0';
        return -1;
    }
    
    if (inode_table[inode].type != INODE_TYPE_DIR) {
        buffer[0] = '\0';
        return -1;
    }
    
    if (inode_table[inode].blocks[0] == 0 || 
        inode_table[inode].blocks[0] >= FS_MAX_BLOCKS) {
        if (inode_table[inode].blocks[0] == 0) {
            screen_print("[DEBUG] fs_list_dir: Directory is empty\n");
        }
        buffer[0] = '\0';
        return -1;
    }
    
    fs_dir_block_t dir_block;
    memset(&dir_block, 0, sizeof(fs_dir_block_t));
    read_inode_block(inode_table[inode].blocks[0], (uint8_t*)&dir_block);
    
    int written = 0;
    buffer[0] = '\0';
    
    for (int i = 0; i < FS_MAX_DIR_ENTRIES && written < max_size - 64; i++) {
        if (!dir_block.entries[i].in_use) {
            continue;
        }
        
        if (strcmp(dir_block.entries[i].name, ".") == 0 || 
            strcmp(dir_block.entries[i].name, "..") == 0) {
            continue;
        }
        
        int name_len = 0;
        for (int j = 0; j < FS_MAX_FILENAME && dir_block.entries[i].name[j]; j++) {
            char c = dir_block.entries[i].name[j];
            if (c < 32 || c > 126) {
                name_len = 0;
                break;
            }
            name_len++;
        }
        
        if (name_len == 0) continue;
        
        if (written + name_len + 2 >= max_size) {
            break;
        }
        
        strcpy(buffer + written, dir_block.entries[i].name);
        written += name_len;
        
        uint32_t entry_inode = dir_block.entries[i].inode;
        if (entry_inode < FS_MAX_INODES && 
            inode_table[entry_inode].type == INODE_TYPE_DIR) {
            buffer[written++] = '/';
        }
        
        buffer[written++] = '\n';
    }
    
    buffer[written] = '\0';
    return written;
}

int fs_change_dir(const char* path, char* current_dir) {
    if (strcmp(path, "..") == 0) {
        if (strcmp(current_dir, "/") == 0) {
            return 0;
        }
        
        char* last_slash = current_dir;
        for (char* p = current_dir; *p; p++) {
            if (*p == '/') last_slash = p;
        }
        
        if (last_slash == current_dir) {
            strcpy(current_dir, "/");
        } else {
            *last_slash = '\0';
        }
        return 0;
    }
    
    char new_path[256];
    if (path[0] == '/') {
        strcpy(new_path, path);
    } else {
        if (strcmp(current_dir, "/") == 0) {
            strcpy(new_path, "/");
            strcat(new_path, path);
        } else {
            strcpy(new_path, current_dir);
            strcat(new_path, "/");
            strcat(new_path, path);
        }
    }
    
    if (fs_dir_exists(new_path)) {
        strcpy(current_dir, new_path);
        return 0;
    }
    
    return -1;
}

// ============================================================================
// FUNCIONES PÚBLICAS - ARCHIVOS
// ============================================================================

int fs_create_file(const char* path, const uint8_t* data, uint32_t size) {
    if (size > FS_MAX_FILE_SIZE) return -1;
    
    char parent_path[256];
    char file_name[FS_MAX_FILENAME];
    split_path(path, parent_path, file_name);
    
    if (strlen(file_name) == 0 || strlen(file_name) >= FS_MAX_FILENAME) {
        return -1;
    }

    screen_print("[DEBUG] fs_create_file: Creating file ");
    screen_print(file_name);
    screen_print("\n");
    screen_print("[DEBUG] fs_create_file: Parent path: ");
    screen_print(parent_path);
    screen_print("\n");
    
    uint32_t parent_inode = find_inode_by_path(parent_path);
    if (parent_inode == 0xFFFFFFFF || 
        inode_table[parent_inode].type != INODE_TYPE_DIR) {
        return -1;
    }
    
    uint32_t new_inode = allocate_inode();
    if (new_inode == 0xFFFFFFFF) return -1;
    
    inode_table[new_inode].type = INODE_TYPE_FILE;
    inode_table[new_inode].size = size;
    inode_table[new_inode].parent_inode = parent_inode;
    
    uint32_t blocks_needed = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    if (blocks_needed > FS_INODE_DIRECT_BLOCKS) {
        blocks_needed = FS_INODE_DIRECT_BLOCKS;
    }
    
    for (uint32_t i = 0; i < blocks_needed; i++) {
        uint32_t block = allocate_block();
        if (block == 0xFFFFFFFF) {
            free_inode(new_inode);
            return -1;
        }
        inode_table[new_inode].blocks[i] = block;
        
        memset(sector_buffer, 0, FS_BLOCK_SIZE);
        uint32_t copy_size = size > FS_BLOCK_SIZE ? FS_BLOCK_SIZE : size;
        memcpy(sector_buffer, data + (i * FS_BLOCK_SIZE), copy_size);
        write_inode_block(block, sector_buffer);
        size -= copy_size;
    }
    
    fs_dir_block_t parent_dir;
    read_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    int added = 0;
    for (int i = 0; i < FS_MAX_DIR_ENTRIES; i++) {
        if (!parent_dir.entries[i].in_use) {
            strcpy(parent_dir.entries[i].name, file_name);
            parent_dir.entries[i].inode = new_inode;
            parent_dir.entries[i].in_use = 1;
            added = 1;
            break;
        }
    }
    
    if (!added) {
        free_inode(new_inode);
        return -1;
    }
    
    write_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    sync_inode_table();
    sync_block_bitmap();
    sync_superblock();
    
    screen_print("[DEBUG] fs_create_file: File created successfully!\n");
    return 0;
}

int fs_read_file(const char* path, uint8_t* buffer, uint32_t max_size) {
    uint32_t inode = find_inode_by_path(path);
    if (inode == 0xFFFFFFFF || inode_table[inode].type != INODE_TYPE_FILE) {
        if(inode == 0xFFFFFFFF) screen_print("[DEBUG] fs_read_file: Inode not found!\n");
        if(inode_table[inode].type != INODE_TYPE_FILE) screen_print("[DEBUG] fs_read_file: Not a file!\n");
        screen_print("[DEBUG] fs_read_file: Path: ");
        screen_print(path);
        screen_print("\n");
        return -1;
    }
    
    uint32_t size = inode_table[inode].size;
    if (size > max_size) size = max_size;
    
    uint32_t blocks_needed = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    if (blocks_needed > FS_INODE_DIRECT_BLOCKS) {
        blocks_needed = FS_INODE_DIRECT_BLOCKS;
    }
    
    uint32_t remaining = size;
    for (uint32_t i = 0; i < blocks_needed; i++) {
        read_inode_block(inode_table[inode].blocks[i], sector_buffer);
        uint32_t copy_size = remaining > FS_BLOCK_SIZE ? FS_BLOCK_SIZE : remaining;
        memcpy(buffer + (i * FS_BLOCK_SIZE), sector_buffer, copy_size);
        remaining -= copy_size;
    }
    
    return inode_table[inode].size;
}

int fs_delete_file(const char* path) {
    char parent_path[256];
    char file_name[FS_MAX_FILENAME];
    split_path(path, parent_path, file_name);
    
    uint32_t parent_inode = find_inode_by_path(parent_path);
    if (parent_inode == 0xFFFFFFFF) return -1;
    
    uint32_t file_inode = find_inode_by_path(path);
    if (file_inode == 0xFFFFFFFF || inode_table[file_inode].type != INODE_TYPE_FILE) {
        return -1;
    }
    
    free_inode(file_inode);
    
    fs_dir_block_t parent_dir;
    read_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    for (int i = 0; i < FS_MAX_DIR_ENTRIES; i++) {
        if (parent_dir.entries[i].in_use && 
            parent_dir.entries[i].inode == file_inode) {
            parent_dir.entries[i].in_use = 0;
            memset(parent_dir.entries[i].name, 0, FS_MAX_FILENAME);
            break;
        }
    }
    
    write_inode_block(inode_table[parent_inode].blocks[0], (uint8_t*)&parent_dir);
    
    sync_inode_table();
    sync_block_bitmap();
    
    return 0;
}