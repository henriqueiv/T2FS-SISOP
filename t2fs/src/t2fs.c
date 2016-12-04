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

static int INITIALIZED = 0;
#define INIT()\
do {\
if (!INITIALIZED) {\
init_t2fs();\
INITIALIZED = 1;\
}\
} while(0)

// ------- Definido na especificação -------
#define ID "T2FS"
#define VERSION 0x7E02
#define DISK_NAME "t2fs_disk.dat"

#define OPENED_FILES_LIMIT 20

#define T2FS_RECORD_SIZE 64 // Conforme email do Carissimi 64 ao inves de sizeof que da 44 bytes
#define ROOT_INODE_BLOCK_INDEX 0

typedef int BLOCK_TYPE;
#define BLOCK_TYPE_DATA 0
#define BLOCK_TYPE_INODE 1
static int data_block_sector_offset;
static int inode_sector_offset;
static int sectors_per_block;
static int records_per_sector;
// -----------------------------------------

struct opened_file {
    int inode_number;
    int pointer;
    struct opened_file* next;
};

static struct opened_file* opened_files;

// ------------------------------------

struct t2fs_superbloco* superblock;

// MARK: - Private functions
static int is_asbsolute_path(char* path) {
    return path[0] == '/';
}

void show_superblock_info(struct t2fs_superbloco* superblock) {
    printf("%s", __PRETTY_FUNCTION__);
    printf("ID: %s\n", superblock->id);
    printf("Version: 0x%x\n", superblock->version);
    printf("Superblock Size: %hu\n", superblock->superblockSize);
    printf("Free Blocks Bitmap Size: %hu\n", superblock->freeBlocksBitmapSize);
    printf("Free inode Bitmap Size: %hu\n", superblock->freeInodeBitmapSize);
    printf("inode Area Size: %hu\n", superblock->inodeAreaSize);
    printf("BlockSize: %hu\n", superblock->blockSize);
    printf("Disksize: %u\n", superblock->diskSize);
}

void show_bitmap_info(int handle) {
    printf("%s", __PRETTY_FUNCTION__);
    int bitmap_size = (handle == BITMAP_DADOS) ? superblock->freeBlocksBitmapSize : superblock->freeInodeBitmapSize ;
    int i = 0;
    for(; i < SECTOR_SIZE * bitmap_size; i++) {
        printf("bit %i -> %i\n", i, getBitmap2(handle, i));
    }
}

void show_inode_info(struct t2fs_inode* inode) {
    printf("%s", __PRETTY_FUNCTION__);
    printf("%i\n", inode->dataPtr[0]);
    printf("%i\n", inode->dataPtr[1]);
    printf("%i\n", inode->singleIndPtr);
    printf("%i\n", inode->doubleIndPtr);
}

void show_record_info(struct t2fs_record* record) {
    printf("%s", __PRETTY_FUNCTION__);
    printf("%hhu\n", record->TypeVal);
    printf("%s\n", record->name);
    printf("%d\n", record->blocksFileSize);
    printf("%d\n", record->bytesFileSize);
    printf("%i\n", record->inodeNumber);
}

void show_sector_info(char sector_data[SECTOR_SIZE], const char* format) {
    int index = 0;
    for (; index < SECTOR_SIZE; index++) {
        printf(format, sector_data[index]);
    }
}

void add_to_opened_list(int inode_number) {
    if (opened_files == NULL) {
        opened_files = (struct opened_file*) malloc(sizeof(struct opened_file));
    }
    
    struct opened_file *aux = opened_files;
    while(aux != NULL) {
        aux = aux->next;
    }
    struct opened_file* new = malloc(sizeof(struct opened_file));
    new->inode_number = inode_number;
    new->next = NULL;
    new->pointer = 0;
    aux = new;
}

void remove_from_opened_list(int inode_number) {
    struct opened_file *aux = opened_files;
    struct opened_file *prev;
    while(aux != NULL) {
        if (aux->inode_number == inode_number) {
            free(aux);
            aux = aux->next;
            
            break;
        }
        
        prev = aux;
        aux = aux->next;
    }
    
    
    
}

int inodes_per_sector = (SECTOR_SIZE/sizeof(struct t2fs_inode));
struct t2fs_inode* get_inode(int index) {
    struct t2fs_inode* inode = (struct t2fs_inode*) malloc(sizeof(struct t2fs_inode));
    char buffer[SECTOR_SIZE];
    // PDG: ver pra substituir esse 16 por "inodes_per_sector"
    int sector_to_read = inode_sector_offset + index/16; // 16->inodes por setor
    int inode_index_in_sector = index % 16;
    int byte_offset_in_block = inode_index_in_sector * sizeof(struct t2fs_inode);
    
    printf("inode %i fica no setor %i e é o inode de numero %i a %i bytes de offset.\n", index, sector_to_read, inode_index_in_sector, byte_offset_in_block);
    
    if(read_sector(sector_to_read, buffer) != 0) {
        printf("ERRO LENDO SETOR: %i\n", sector_to_read);
        exit(ERROR);
    }
    memcpy(inode->dataPtr,       buffer+byte_offset_in_block,       8); // dataPtr[2]
    memcpy(&inode->singleIndPtr, buffer+byte_offset_in_block +  8,  4); // singleIndPtr
    memcpy(&inode->doubleIndPtr, buffer+byte_offset_in_block + 12,  4); // doubleIndPtr
    
    return inode;
}

struct t2fs_record* read_block(int index, BLOCK_TYPE block_type) {
    struct t2fs_record *record = (struct t2fs_record*) malloc(T2FS_RECORD_SIZE);
    char buffer[SECTOR_SIZE];
    
    int sector_offset = (block_type == BLOCK_TYPE_DATA) ? data_block_sector_offset : inode_sector_offset;
    int sector_per_block_offset = (index * sectors_per_block);
    int sector = sector_offset + sector_per_block_offset;
    if (read_sector(sector, buffer) != 0) {
        printf("Erro lendo!\n");
        printf("Tipo do bloco: %d(%s)\n", block_type, ((block_type == BLOCK_TYPE_DATA) ? "Dados" : "Inode"));
        printf("Offset: %d \n", sector_offset);
        printf("Setor: %d \n", sector_per_block_offset);
        exit(ERROR);
    }
    
    memcpy(&record->TypeVal,        buffer,       1);
    memcpy(&record->name,           buffer + 1,  31); // PDG: nao eh 32?
    memcpy(&record->blocksFileSize, buffer + 33,  4);
    memcpy(&record->bytesFileSize,  buffer + 37,  4);
    memcpy(&record->inodeNumber,    buffer + 41,  4);
    
    return record;
}

struct t2fs_record* get_record_in_block(char *name, int blockIndex) {
    struct t2fs_record *record = (struct t2fs_record*) malloc(T2FS_RECORD_SIZE);
    char current_sector_buffer[SECTOR_SIZE];
    int current_sector_disk_index = data_block_sector_offset + (blockIndex*superblock->blockSize);
    int hasFound = 0;
    int current_record_index = 0;
    int current_sector_block_index = 0;
    while (current_sector_block_index < superblock->blockSize) {
        int sector_to_be_read = current_sector_disk_index+current_sector_block_index;
        if (read_sector(sector_to_be_read, current_sector_buffer) != 0) {
            printf("Erro lendo setor %i do bloco %i",current_sector_block_index, blockIndex);
            return NULL;
        }
        current_record_index = 0;
        while (current_record_index < records_per_sector) {
            char *name_to_check;
            name_to_check = malloc(32);//32 = limite do nome dentro de um record
            strncpy(name_to_check, current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1, 32);
            printf("Leu o name: %s\n", name_to_check);
            if (strcmp(name, name_to_check) != 0){
                current_record_index++; continue;
            }
            hasFound = 1;
            break;
        }
        if (hasFound)
            break;
        current_sector_block_index++;
    }
    if (!hasFound) { return NULL; }
    memcpy(&record->TypeVal,        current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE),    1);
    memcpy(&record->name,           current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1,  31);
    memcpy(&record->blocksFileSize, current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+33, 4);
    memcpy(&record->bytesFileSize,  current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+37, 4);
    memcpy(&record->inodeNumber,    current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+41, 4);
    return record;
}

static void init_t2fs() {
    char buffer[SECTOR_SIZE];
    
    if (read_sector(0, buffer)) {
        printf("Erro ao ler o superbloco. O arquivo '%s' esta no mesmo caminho do executavel?\n", DISK_NAME);
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
    
    // Populando var globais
    inode_sector_offset = superblock->superblockSize + superblock->freeBlocksBitmapSize + superblock->freeInodeBitmapSize;
    data_block_sector_offset = inode_sector_offset + superblock->inodeAreaSize;
    sectors_per_block = superblock->blockSize/SECTOR_SIZE;
    records_per_sector = SECTOR_SIZE/T2FS_RECORD_SIZE;
    
    // printf("BITMAP iNODE\n");
    // show_bitmap_info(BITMAP_INODE);
    // printf("BITMAP DADOS\n");
    // show_bitmap_info(BITMAP_DADOS);
    // printf("INODE 0\n");
    // struct t2fs_inode* inode = read_inode(1);
    // printf("%s\n", (inode->dataPtr[0]));
    // printf("%s\n", (inode->dataPtr[1]));
    // show_inode_info(0);
    // struct t2fs_inode *inode = get_inode(3);
    // // printf("%i\n", inode->dataPtr[0]);
    // struct t2fs_record *record = read_block(inode->dataPtr[0]);
    
    // inode = get_inode(1);
    
}

int open_entry() {
    return -1;
}

// MARK: libt2fs functions

// Informa a identificação dos desenvolvedores do T2FS.
int identify2 (char *name, int size) {
    printf("240501 - Henrique Valcanaia\n243666 - Pietro Degrazia\n");
    return 0;
}

// Função usada para criar um novo arquivo no disco.
FILE2 create2 (char *filename) {
    INIT();
    FILE2 file = ERROR;
    
    return file;
}

// Função usada para remover (apagar) um arquivo do disco.
int delete2 (char *filename) {
    INIT();
    return ERROR;
}

// Função que abre um arquivo existente no disco.
FILE2 open2 (char *filename) {
    INIT();
    printf("OPENFILE: %s\n", filename);
    struct t2fs_record *record;
    int currentInodeIndex = ROOT_INODE_BLOCK_INDEX;//raiz
    char* component;
    char* path;
    path = strdup(filename);//copia o path
    if (path == NULL || strlen(path) == 0) {
        return ERROR;
    }
    
    while ((component = strsep(&path, "/")) != NULL) {
        if (strcmp(component, "") == 0){
            continue;
        }
        
        printf("Path Component: %s, será procurado no iNode: %i\n", component, currentInodeIndex);
        struct t2fs_inode *inode = get_inode(currentInodeIndex);
        record = get_record_in_block(component, inode->dataPtr[0]);
        
        //parte do path não existia
        if (record == NULL) {
            return ERROR;
        }
        
        if (record->TypeVal == TYPEVAL_REGULAR) {
            break; // há de se fazer algo pra isso, por enquanto foda-se
        }
        
        currentInodeIndex = record->inodeNumber;
    }
    
    if (record == NULL) {
        printf("não achou\n");
        return ERROR;
    }
    
    if (record->TypeVal != TYPEVAL_REGULAR) {
        printf("achou diretorio onde deveria ser arquivo\n");
        return ERROR;
    }
    
    if (strcmp(record->name, basename(filename)) != 0) {
        printf("achou arquivo onde deveria ser diretorio\n");
        return ERROR;
    }
    
    printf("Fim do Open File\n");
    // opened_files[/] = record->inodeNumber;
    // HIV: botar o amigo pra dentro da opened_files
    add_to_opened_list(record->inodeNumber);
    
    return record->inodeNumber;
}

// Função usada para fechar um arquivo.
int close2 (FILE2 handle) {
    INIT();
    return ERROR;
}

// Função usada para realizar a leitura de uma certa quantidade de bytes (size) de um arquivo.
int read2 (FILE2 handle, char *buffer, int size) {
    INIT();
    return ERROR;
}

// Função usada para realizar a escrita de uma certa quantidade de bytes (size) de um arquivo.
int write2 (FILE2 handle, char *buffer, int size) {
    INIT();
    return ERROR;
}

// Função usada para truncar um arquivo. Remove do arquivo todos os bytes a partir da posição atual do contador de posição (current pointer), inclusive, até o seu final.
int truncate2 (FILE2 handle) {
    INIT();
    return ERROR;
}

// Altera o contador de posição (current pointer) do arquivo.
int seek2 (FILE2 handle, DWORD offset) {
    INIT();
    return ERROR;
}

// Função usada para criar um novo diretório.
int mkdir2 (char *pathname) {
    INIT();
    return ERROR;
}

// Função usada para remover (apagar) um diretório do disco.
int rmdir2 (char *pathname) {
    INIT();
    return ERROR;
}

// Função que abre um diretório existente no disco.
DIR2 opendir2 (char *pathname) {
    INIT();
    DIR2 dir = ERROR;
    return dir;
}

// Função usada para ler as entradas de um diretório.
int readdir2 (DIR2 handle, DIRENT2 *dentry) {
    INIT();
    return ERROR;
}

// Função usada para fechar um diretório.
int closedir2 (DIR2 handle) {
    INIT();
    return ERROR;
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




//file open antes de eu mexer
// Função que abre um arquivo existente no disco.
//FILE2 open2 (char *filename) {
//    //    return ERROR;
//
//    INIT();
//
//    char* string;
//    string = strdup(filename);
//    if (string != NULL) {
//        char* tofree;
//        char* token;
//
//        tofree = string;
//        while ((token = strsep(&string, "/")) != NULL) {
//            if (strcmp(token, "") == 0) {
//                // raiz
//                printf("raiz\n");
//                struct t2fs_record* entry = (struct t2fs_record*) malloc(T2FS_RECORD_SIZE);
//                entry = read_block(ROOT_INODE_BLOCK_INDEX, BLOCK_TYPE_INODE);
//            } else {
//                printf("%s\n", token);
//            }
//        }
//
//        free(tofree);
//    }
//
//
//    // olhar todos pq pode querer fechar um arquivo do meio do array
//    int index = 0;
//    struct t2fs_record record;
//    opened_files[index] = record.inodeNumber;
//
//    return record.inodeNumber;
//}
