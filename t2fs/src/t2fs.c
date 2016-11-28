//
//  t2fs.c
//  t2fs
//
//  Created by Henrique Valcanaia on 14/11/16.
//  Copyright © 2016 SISOP. All rights reserved.
//

#include <assert.h>
#include <stdio.h>
#include "../include/t2fs.h"

#define ID "T2FS"
#define VERSION 0x7E02
#define DISK_NAME "t2fs_disk.dat"

static int INITIALIZED = 0;
#define INIT()\
    do {\
        if (!INITIALIZED) {\
            init_t2fs();\
            INITIALIZED = 1;\
        }\
    } while(0)

struct t2fs_superbloco* superblock;


void show_superblock_info(struct t2fs_superbloco* superblock) {
    printf("ID: %s\n", superblock->id);
    printf("Version: 0x%x\n", superblock->version);
    printf("Superblock Size: %hu\n", superblock->superblockSize);
    printf("Free Blocks Bitmap Size: %hu\n", superblock->freeBlocksBitmapSize);
    printf("Free inode Bitmap Size: %hu\n", superblock->freeInodeBitmapSize);
    printf("inode Area Size: %hu\n", superblock->inodeAreaSize);
    printf("BlockSize: %hu\n", superblock->blockSize);
    printf("Disksize: %u\n", superblock->diskSize);
}

static void init_t2fs() {
    char buffer[SECTOR_SIZE];
    
    if (read_sector(0, buffer)) {
        printf("Erro ao ler o superbloco. O arquivo %s esta no mesmo caminho do executavel?\n", DISK_NAME);
        exit(EXIT_FAILURE);
    }
    
    superblock = malloc(sizeof(*superblock));
    
    /* offset 0 bytes */
    strncpy(superblock->id, buffer, 4);
    if (strncmp(superblock->id, ID, 4)) {
        printf("Sistemas de arquivos desconhecido!\nEsperado: %s\nEncontrado: %s\n", ID, superblock->id);
        exit(EXIT_FAILURE);
    }
    
    superblock->version = *((WORD *)(buffer + 4));
    if (superblock->version != 0x7E02) {
        printf("Versão do sistema de arquivos não suportada!\nEsperado: %d\nEncontrado: 0x%hu\n", VERSION, superblock->version);
        exit(EXIT_FAILURE);
    }
    
    superblock->superblockSize = *((WORD *)(buffer + 6));
    superblock->freeBlocksBitmapSize = *((WORD *)(buffer + 8));
    superblock->freeInodeBitmapSize = *((WORD *)(buffer + 10));
    superblock->inodeAreaSize = *((WORD *)(buffer + 12));
    superblock->blockSize = *((DWORD *)(buffer + 14));
    superblock->diskSize = *((DWORD *)(buffer + 16));
    
    show_superblock_info(superblock);
}

// ----------------------------------------

int identify2 (char *name, int size) {
    printf("240501 - Henrique Valcanaia\n243666 - Pietro Degrazia");
    return 0;
}

FILE2 create2 (char *filename) {
    FILE2 file = ERROR;
    INIT();
    return file;
}

int delete2 (char *filename) {
    return ERROR;
}

FILE2 open2 (char *filename) {
    FILE2 file = ERROR;
    return file;
}

int close2 (FILE2 handle) {
    return ERROR;
}

int read2 (FILE2 handle, char *buffer, int size) {
    return ERROR;
}

int write2 (FILE2 handle, char *buffer, int size) {
    return ERROR;
}

int truncate2 (FILE2 handle) {
    return ERROR;
}

int seek2 (FILE2 handle, DWORD offset) {
    return ERROR;
}

int mkdir2 (char *pathname) {
    return ERROR;
}

int rmdir2 (char *pathname) {
    return ERROR;
}

DIR2 opendir2 (char *pathname) {
    DIR2 dir = ERROR;
    return dir;
}

int readdir2 (DIR2 handle, DIRENT2 *dentry) {
    return ERROR;
}

int closedir2 (DIR2 handle) {
    return ERROR;
}

// MARK: - Private functions
static int is_asbsolute_path(char* path) {
    return path[0] == '/';
}

/*
 * Given an absolute pathname, return the record of the file/subdir, if any
 * Else return NULL
 *
 * For now works only in the root directory, so if you pass "/home/teste" it
 * will be interpreted as "/home"
 */
//#define PWD_BUFFER_SIZE 256
//#define RECORD_SIZE sizeof(struct t2fs_record)
//
//static struct t2fs_record* get_record(char* pathname) {
//    assert(pathname && is_absolute_path(pathname) && strlen(pathname) < PWD_BUFFER_SIZE);
//    assert(SECTOR_SIZE % RECORD_SIZE == 0);
//    
//    /* There is no record for the root directory */
//    if (strlen(pathname) == 1)
//        return NULL;
//    struct t2fs_record *records;
//    size_t size = alloc_max_records(&records);
//    size = read_records(records, size, superblock->RootSectorStart, superblock->DataSectorStart);
//    assert(size > 0);
//    
//    // TODO
//    char *want = leftmost(pathname);
//    size_t i = 0;
//    while (i < size && strncmp(records[i].name, want, MAX_FILE_NAME_SIZE))
//        i++;
//    if (i >= size) {
//        free(records);
//        free(want);
//        return NULL;
//    } else {
//        struct t2fs_record *ans = malloc(sizeof(*ans));
//        *ans = records[i];
//        free(records);
//        free(want);
//        return ans;
//    }
//}
//
//
//FILE2 open_file(char *filename, BYTE type) {
//    if (!filename) {
//        printf("Unknown filename!\n");
//        return -1;
//    }
//    
//    if (is_asbsolute_path(filename)) {
//        struct t2fs_record *record = get_record(filename);
//        if (!record) {
//            printf("%s not found in disk\n", filename);
//            return -1;
//        }
//        if (record->TypeVal != type) {
//            printf("%s is not a %s\n", is_asbsolute_path, type == TYPE_DIR ? "dir" : "file");
//            return -1;
//        }
//        printf("%s found in disk, adding to FT w/ fd %d\n", filename, CT);
//        
//        struct file_table *e = malloc(sizeof(*e));
//        e->fd = CT++;
//        e->record = *record;
//        e->offset = 0;
//        HASH_ADD_INT(FT, fd, e);
//        
//        return e->fd;
//    } else {
//        printf("Relative paths not implemented\n");
//        return -1;
//    }
//}
