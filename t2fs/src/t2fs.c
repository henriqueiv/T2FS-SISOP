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
    DWORD pointer;
    int occupied;
};

struct opened_file opened_files[OPENED_FILES_LIMIT];
int opened_files_count = 0;

// ------------------------------------

struct t2fs_superbloco* superblock;

// MARK: - Auxiliar Show functions
void show_superblock_info(struct t2fs_superbloco* superblock) {
    printf("%s", __PRETTY_FUNCTION__);
    printf("ID: %s\n", superblock->id);
    printf("Version: 0x%x\n", superblock->version);
    printf("Superblock Size: %hu\n", superblock->superblockSize);
    printf("Free Blocks Bitmap Size: %hu\n", superblock->freeBlocksBitmapSize);
    printf("Free inode Bitmap Size: %hu\n", superblock->freeInodeBitmapSize);
    printf("inode Area Size: %hu\n", superblock->inodeAreaSize);
    printf("BlockSize: %hu\n", superblock->blockSize);
    printf("Disksize: %u\n\n\n\n\n", superblock->diskSize);
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
void show_open_file_info(struct opened_file* file) {
    printf("--------- %s ---------", __PRETTY_FUNCTION__);
    printf("\ninode: %d\n", file->inode_number);
    printf("pointer: %d\n", file->pointer);
}
void show_opened_files_info() {
    int i = 0;
    for (; i < OPENED_FILES_LIMIT; i++) {
        show_open_file_info(&opened_files[i]);
    }
}

// MARK: - Opened Files Functions
int isLastComponent(char *string) {
    if (string == NULL) {return 1;}
    char* tempString;
    char* nextComponent;
    tempString = strdup(string);
    if (tempString == NULL || strlen(tempString) == 0) { return 1; }
    
    if ((nextComponent = strsep(&tempString, "/")) == NULL) {return 1;}
    if (strcmp(nextComponent, "") == 0){ return 1; }
    return 0;
}
int add_to_opened_list(int inode_number) {
    struct opened_file new;
    new.inode_number = inode_number;
    new.occupied = 1;
    new.pointer = 0;
    
    int freePosition = 0;
    for (; freePosition < OPENED_FILES_LIMIT; freePosition++) {
        if (opened_files[freePosition].occupied == 0) {
            opened_files[opened_files_count] = new;
            opened_files_count++;
            return freePosition;
        }
    }
    
    printf("nenhuma posicao livre\n");
    return -1;
}
int is_file_open(int inode_number) {
    int i = 0;
    for (; i < OPENED_FILES_LIMIT; i++) {
        if (opened_files[i].inode_number == inode_number) {
            return 1;
        }
    }
    return 0;
}
int remove_from_opened_list(int inode_number) {
    int i = 0;
    printf("Procurando arquivo aberto para remover ( %i )\n", inode_number);
    for (; i < OPENED_FILES_LIMIT; i++) {
//        printf("Procurado: %d | Comparando: %d\n", inode_number, opened_files[i].inode_number);
        if (opened_files[i].inode_number == inode_number) {
            printf("achou\n");
            opened_files[i].occupied = 0;
            opened_files_count--;
            break;
        }
    }
    
    printf("arquivo não estava aberto\n");
    return 1;
}
int open_file_seek(int inode_number, DWORD offset) {
    int i = 0;
    for (; i < OPENED_FILES_LIMIT; i++) {
        if (opened_files[i].inode_number == inode_number) {
            opened_files[i].pointer = offset;
            return 1;
        }
    }
    return 0;
}
int get_index_of_opened_file(int handle) {
    int index = 0;
    for (; index < OPENED_FILES_LIMIT; index++) {
        if (opened_files[index].inode_number == handle) {
            return index;
        }
    }
    return ERROR;
}

// MARK: - iNode Auxiliar Functions
int inodes_per_sector = (SECTOR_SIZE/sizeof(struct t2fs_inode));
struct t2fs_inode* get_inode(int index) {
//    printf("get_inode( %i )\n", index);
    struct t2fs_inode* inode = (struct t2fs_inode*) malloc(sizeof(struct t2fs_inode));
    char buffer[SECTOR_SIZE];
    // PDG: ver pra substituir esse 16 por "inodes_per_sector"
    int sector_to_read = inode_sector_offset + index/16; // 16->inodes por setor
    int inode_index_in_sector = index % 16;
    int byte_offset_in_block = inode_index_in_sector * sizeof(struct t2fs_inode);
    
//    printf("inode %i fica no setor %i e é o inode de numero %i a %i bytes de offset.\n", index, sector_to_read, inode_index_in_sector, byte_offset_in_block);
    
    if(read_sector(sector_to_read, buffer) != 0) {
        printf("ERRO LENDO SETOR: %i\n", sector_to_read);
        exit(ERROR);
    }
    memcpy(inode->dataPtr,       buffer+byte_offset_in_block,       8); // dataPtr[2]
    memcpy(&inode->singleIndPtr, buffer+byte_offset_in_block +  8,  4); // singleIndPtr
    memcpy(&inode->doubleIndPtr, buffer+byte_offset_in_block + 12,  4); // doubleIndPtr
    
    return inode;
}
int put_inode(int index, struct t2fs_inode* inode) {
    char buffer[SECTOR_SIZE];
    int sector_to_read = inode_sector_offset + index/16; // 16->inodes por setor
    int inode_index_in_sector = index % 16;
    int byte_offset_in_block = inode_index_in_sector * sizeof(struct t2fs_inode);
//    printf("inode %i fica no setor %i e é o inode de numero %i a %i bytes de offset.\n", index, sector_to_read, inode_index_in_sector, byte_offset_in_block);

    if(read_sector(sector_to_read, buffer) != 0) { printf("ERRO LENDO SETOR: %i\n", sector_to_read); return ERROR; }
    
    memcpy(buffer+byte_offset_in_block,      inode->dataPtr,         8); // dataPtr[2]
    memcpy(buffer+byte_offset_in_block +  8, &inode->singleIndPtr,   4); // singleIndPtr
    memcpy(buffer+byte_offset_in_block + 12, &inode->doubleIndPtr,   4); // doubleIndPtr
    
    if(write_sector(sector_to_read, buffer) != 0) { printf("ERRO LENDO SETOR: %i\n", sector_to_read); return ERROR; }
    
    return 0;
}

// MARK: - Block Auxiliar Functions
struct t2fs_record* get_record_in_block_with_offset(int blockIndex, DWORD offset) {
    //    printf("get_record( %i )\n", blockIndex);
    struct t2fs_record *record = malloc(T2FS_RECORD_SIZE);
    char buffer[SECTOR_SIZE];
    
    int sector_to_read = data_block_sector_offset + (blockIndex*superblock->blockSize) + offset/SECTOR_SIZE;
    int byte_offset_in_block = offset - ((offset/SECTOR_SIZE)*SECTOR_SIZE);
    //    int byte_offset_in_block = 0;
    
    if(read_sector(sector_to_read, buffer) != 0) {
        printf("ERRO LENDO SETOR: %i\n", sector_to_read);
        exit(ERROR);
    }
    
    memcpy(&record->TypeVal,        buffer+(byte_offset_in_block),    1);
    memcpy(&record->name,           buffer+(byte_offset_in_block)+1,  31);
    memcpy(&record->blocksFileSize, buffer+(byte_offset_in_block)+33, 4);
    memcpy(&record->bytesFileSize,  buffer+(byte_offset_in_block)+37, 4);
    memcpy(&record->inodeNumber,    buffer+(byte_offset_in_block)+41, 4);
    
    if (record->TypeVal != TYPEVAL_DIRETORIO && record->TypeVal != TYPEVAL_REGULAR) {
        return NULL;
    }
    
    
    return record;
}
struct t2fs_record* get_record_in_block(char *name, int blockIndex) {
    printf("Vai procurar arquivo: '%s' no bloco: %i\n", name, blockIndex);
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
int put_record_in_block(struct t2fs_record *record, int blockIndex) {
    printf("Vai gravar record: '%s' no bloco: %i\n", record->name, blockIndex);
    char current_sector_buffer[SECTOR_SIZE];
    int current_sector_disk_index = data_block_sector_offset + (blockIndex*superblock->blockSize);
    int hasFoundEmptySpot = 0;
    int current_record_index = 0;
    int current_sector_block_index = 0;
    while (current_sector_block_index < superblock->blockSize) {
        int sector_to_be_read = current_sector_disk_index+current_sector_block_index;
        if (read_sector(sector_to_be_read, current_sector_buffer) != 0) {
            printf("Erro lendo setor %i do bloco %i",current_sector_block_index, blockIndex);
            return ERROR;
        }
        current_record_index = 0;
        while (current_record_index < records_per_sector) {
            char *name_to_check;
            name_to_check = malloc(32);//32 = limite do nome dentro de um record
            strncpy(name_to_check, current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1, 32);
            printf("Leu o name: %s\n", name_to_check);
            if (strcmp("", name_to_check) != 0){
                current_record_index++; continue;
            }
            hasFoundEmptySpot = 1;
            break;
        }
        if (hasFoundEmptySpot)
            break;
        
        current_sector_block_index++;
    }
    if (!hasFoundEmptySpot) { printf("Bloco cheio\n"); return ERROR; }
    
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE),    &record->TypeVal,         1);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1,  &record->name,           31);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+33, &record->blocksFileSize,  4);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+37, &record->bytesFileSize,   4);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+41, &record->inodeNumber,     4);
    
    int sector_to_write = current_sector_disk_index + current_sector_block_index;
    if (write_sector(sector_to_write, current_sector_buffer) != 0) {
        printf("Erro escrevendo setor %i do bloco %i",current_sector_block_index, blockIndex);
        return ERROR;
    }
    
    return 0;
}
int delete_record_in_block(struct t2fs_record *record, int blockIndex) {
    printf("Vai deletar record: '%s' no bloco: %i\n", record->name, blockIndex);
    char current_sector_buffer[SECTOR_SIZE];
    int current_sector_disk_index = data_block_sector_offset + (blockIndex*superblock->blockSize);
    int hasFoundTargetRecord = 0;
    int current_record_index = 0;
    int current_sector_block_index = 0;
    while (current_sector_block_index < superblock->blockSize) {
        int sector_to_be_read = current_sector_disk_index+current_sector_block_index;
        if (read_sector(sector_to_be_read, current_sector_buffer) != 0) {
            printf("Erro lendo setor %i do bloco %i",current_sector_block_index, blockIndex);
            return ERROR;
        }
        current_record_index = 0;
        while (current_record_index < records_per_sector) {
            char *name_to_check;
            name_to_check = malloc(32);//32 = limite do nome dentro de um record
            strncpy(name_to_check, current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1, 32);
            printf("Leu o name: %s\n", name_to_check);
            if (strcmp(record->name, name_to_check) != 0){
                current_record_index++; continue;
            }
            hasFoundTargetRecord = 1;
            break;
        }
        if (hasFoundTargetRecord)
            break;
        
        current_sector_block_index++;
    }
    if (!hasFoundTargetRecord) { printf("Erro para deletar. Nao estava nesse bloco\n"); return ERROR; }
    
    DWORD blocksFileSize = 0;
    record->blocksFileSize = blocksFileSize;
    DWORD bytesFileSize = 0;
    record->bytesFileSize = bytesFileSize;
    record->inodeNumber = -1;
    strcpy(record->name, "");
    record->TypeVal = TYPEVAL_INVALIDO;
    
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE),    &record->TypeVal,         1);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+1,  &record->name,           31);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+33, &record->blocksFileSize,  4);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+37, &record->bytesFileSize,   4);
    memcpy( current_sector_buffer+(current_record_index*T2FS_RECORD_SIZE)+41, &record->inodeNumber,     4);
    
    int sector_to_write = current_sector_disk_index + current_sector_block_index;
    if (write_sector(sector_to_write, current_sector_buffer) != 0) {
        printf("Erro escrevendo setor %i do bloco %i",current_sector_block_index, blockIndex);
        return ERROR;
    }
    
    return 0;
}

//int write_to_block(DWORD seek_pointer,int blockIndex, char *buffer, int size){
//    printf("Vai escrever buffer: '%s' no bloco: %i\n", buffer, blockIndex);
//    char current_sector_buffer[SECTOR_SIZE];
//    int current_sector_index_in_block = seek_pointer % SECTOR_SIZE;
//    int current_offset_in_sector = seek_pointer - (current_sector_index_in_block * SECTOR_SIZE);
//    int first_sector_in_block_index = data_block_sector_offset + (blockIndex*superblock->blockSize);
//    
//    while (size > 0 && current_sector_index_in_block < superblock->blockSize) {
//        int sector_to_be_read = first_sector_in_block_index + current_sector_index_in_block;
//        if (read_sector(sector_to_be_read, current_sector_buffer) != 0) {
//            printf("Erro lendo setor %i do bloco %i",sector_to_be_read, blockIndex);
//            return ERROR;
//        }
//// descobrir quanto(size) tenho que escrever de (buffer) para (current_sector_buffer)
//        //posso escrver SECTOR_SIZE - current_offset_in_sector
//        int max_size_to_write_in_sector = SECTOR_SIZE - (seek_pointer - (current_sector_index_in_block*SECTOR_SIZE));
//        if (max_size_to_write_in_sector > size) {max_size_to_write_in_sector = size;}
//        
//        //escreve
//        memcpy(current_sector_buffer+(), buffer, max_size_to_write_in_sector);
//        
//        //diminiui size
//        size -= max_size_to_write_in_sector;
//        
//        current_sector_index_in_block++;
//    }
//    
//    int sector_to_write = current_sector_disk_index + current_sector_block_index;
//    if (write_sector(sector_to_write, current_sector_buffer) != 0) {
//        printf("Erro escrevendo setor %i do bloco %i",current_sector_block_index, blockIndex);
//        return ERROR;
//    }
//    
//    return 0;
//
//}

//até o momento não está sendo usada
struct t2fs_record* read_block(int index, BLOCK_TYPE block_type) {
//    struct t2fs_record *record = (struct t2fs_record*) malloc(T2FS_RECORD_SIZE);
//    char buffer[SECTOR_SIZE];
//    
//    int sector_offset = (block_type == BLOCK_TYPE_DATA) ? data_block_sector_offset : inode_sector_offset;
//    int sector_per_block_offset = (index * sectors_per_block);
//    int sector = sector_offset + sector_per_block_offset;
//    if (read_sector(sector, buffer) != 0) {
//        printf("Erro lendo!\n");
//        printf("Tipo do bloco: %d(%s)\n", block_type, ((block_type == BLOCK_TYPE_DATA) ? "Dados" : "Inode"));
//        printf("Offset: %d \n", sector_offset);
//        printf("Setor: %d \n", sector_per_block_offset);
//        exit(ERROR);
//    }
//    
//    memcpy(&record->TypeVal,        buffer,       1);
//    memcpy(&record->name,           buffer + 1,  31); // PDG: nao eh 32?
//    memcpy(&record->blocksFileSize, buffer + 33,  4);
//    memcpy(&record->bytesFileSize,  buffer + 37,  4);
//    memcpy(&record->inodeNumber,    buffer + 41,  4);
//    
//    return record;
}
//

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
    
    struct opened_file new;
    new.inode_number = -1;
    new.occupied = 0;
    new.pointer = 0;
    
    int i = 0;
    for (; i < OPENED_FILES_LIMIT; i++) {
        opened_files[i] = new;
    }
    
    // Populando var globais
    inode_sector_offset = superblock->superblockSize + superblock->freeBlocksBitmapSize + superblock->freeInodeBitmapSize;
    data_block_sector_offset = inode_sector_offset + superblock->inodeAreaSize;
    sectors_per_block = superblock->blockSize/SECTOR_SIZE;
    records_per_sector = SECTOR_SIZE/T2FS_RECORD_SIZE;
}
int open_entry() {
    return -1;
}

// MARK: libt2fs functions

// FOI // Informa a identificação dos desenvolvedores do T2FS.
int identify2 (char *name, int size) {
    printf("240501 - Henrique Valcanaia\n243666 - Pietro Degrazia\n");
    return 0;
}

// FOI // Função que abre um arquivo existente no disco.
FILE2 open2 (char *filename) {
    INIT();
    
    if (opened_files_count >= 20) {printf("Ja existem 20 arquivos abertos");return ERROR;}
    printf("OPENFILE: %s\n", filename);
    struct t2fs_record *record;
    int currentInodeIndex = ROOT_INODE_BLOCK_INDEX;//raiz
    char* component;
    char* path;
    path = strdup(filename);//copia o path
    if (path == NULL || strlen(path) == 0) { return ERROR; }
    
    while ((component = strsep(&path, "/")) != NULL) {
        if (strcmp(component, "") == 0){ continue; }
        printf("component: %s  ---  path: %s  --- isLast: %i\n",component, path, isLastComponent(path));
        printf("Path Component: %s, será procurado no iNode: %i\n", component, currentInodeIndex);
        struct t2fs_inode *inode = get_inode(currentInodeIndex);
        record = get_record_in_block(component, inode->dataPtr[0]);
        if (record == NULL) {printf("parte do path não existia\n");return ERROR;}
        if (record->TypeVal == TYPEVAL_REGULAR) {break;} // há de se fazer algo pra isso, por enquanto foda-se
        currentInodeIndex = record->inodeNumber;
    }
    if (record == NULL) { printf("não achou\n"); return ERROR; }
    if (record->TypeVal != TYPEVAL_REGULAR) { printf("achou diretorio onde deveria ser arquivo\n"); return ERROR; }
    if (strcmp(record->name, basename(filename)) != 0) { printf("achou arquivo onde deveria ser diretorio\n"); return ERROR; }
    
    printf("Fim do Open File\n");
    add_to_opened_list(record->inodeNumber);
    show_opened_files_info();
    return record->inodeNumber;
}

// FOI // Função usada para criar um novo arquivo no disco.
FILE2 create2 (char *filename) {
    INIT();
    if (opened_files_count >= 20) {printf("Ja existem 20 arquivos abertos");return ERROR;}
    printf("Create File: %s\n", filename);
    struct t2fs_record *record;
    int currentInodeIndex = ROOT_INODE_BLOCK_INDEX;//raiz
    char* component;
    char* path;
    path = strdup(filename);//copia o path
    if (path == NULL || strlen(path) == 0) { return ERROR; }
    while ((component = strsep(&path, "/")) != NULL) {
        if (strcmp(component, "") == 0){
            if (path == NULL) {printf("nao pode criar arquivo sem nome\n");return ERROR;}
            continue;
        }
        printf("component: %s  ---  path: %s  --- isLast: %i\n",component, path, isLastComponent(path));
        printf("Path Component: %s, será procurado no iNode: %i\n", component, currentInodeIndex);
        struct t2fs_inode *inode = get_inode(currentInodeIndex);
        record = get_record_in_block(component, inode->dataPtr[0]);
        if ((record != NULL)&&(path == NULL) ) {printf("achou arquivo com o nome, não pode criar\n");return ERROR;}
        if ((record != NULL)&&(record->TypeVal==TYPEVAL_REGULAR)){printf("achou arquivo onde deveria ser diretorio\n");return ERROR;}
        if (record == NULL && path == NULL) {
            printf("achou lugar pra criar\n");
            break;
        }
        if (record == NULL && path != NULL) {printf("Diretorio no path nao existia\n"); return ERROR;}
        currentInodeIndex = record->inodeNumber;
    }
    printf("Agora vai criar o arquivo '%s' com registro no bloco: %i\n", component, get_inode(currentInodeIndex)->dataPtr[0]);
    
    //achar local pra novo bloco
    int new_block_index = searchBitmap2(BITMAP_DADOS, BIT_FREE);
    if (new_block_index < 0) { printf("Erro procurando no bitmap de dados\n"); return ERROR;}
    if (new_block_index == 0) { printf("Todos blocos ocupados.\n"); return ERROR;}
    if (setBitmap2(BITMAP_DADOS, new_block_index, BIT_TAKEN) != 0) { printf("Erro gravando no bitmap de dados\n"); return ERROR;}
    printf("Novo Bloco - %i\n", new_block_index);
    
    //achar local pra iNODE
    int new_inode_index = searchBitmap2(BITMAP_INODE, BIT_FREE);
    if (new_inode_index < 0) { printf("Erro procurando no bitmap de inodes\n"); return ERROR;}
    if (new_inode_index == 0) { printf("Todos inodes ocupados.\n"); return ERROR;}
    if (setBitmap2(BITMAP_INODE, new_inode_index, BIT_TAKEN) != 0) { printf("Erro gravando no bitmap de inodes\n"); return ERROR;}
    printf("Novo iNODE - %i\n", new_inode_index);
    
    //inserir iNode NA AREA DE INODES (depende do new_block_index)
    struct t2fs_inode* inode = malloc(sizeof(struct t2fs_inode));
    inode->dataPtr[0] = new_block_index;
    inode->dataPtr[1] = -1;
    inode->doubleIndPtr = -1;
    inode->singleIndPtr = -1;
    if (put_inode(new_inode_index, inode) != 0) {printf("Erro gravando inode\n"); return ERROR;}
    printf("Gravou iNODE no seu Setor\n");
    
    //inserir record no bloco (precisa do iNode antes)
    struct t2fs_record *new_record = malloc(T2FS_RECORD_SIZE);
    DWORD blocksFileSize = 16;
    new_record->blocksFileSize = blocksFileSize;
    DWORD bytesFileSize = 16 * SECTOR_SIZE;
    new_record->bytesFileSize = bytesFileSize;
    new_record->inodeNumber = new_inode_index;
    strcpy(new_record->name, component);
    new_record->TypeVal = TYPEVAL_REGULAR;
    if (put_record_in_block(new_record, get_inode(currentInodeIndex)->dataPtr[0]) != 0) {printf("Erro atualizando bloco\n"); return ERROR;}
    printf("Gravou bloco Atualizado no seu Setor\n");
    add_to_opened_list(new_inode_index);
    return 0;
}

// FOI // Função usada para remover (apagar) um arquivo do disco.
int delete2 (char *filename) {
    INIT();
    printf("DELETE FILE: %s\n", filename);
    struct t2fs_record *record;
    int currentInodeIndex = ROOT_INODE_BLOCK_INDEX;//raiz
    char* component;
    char* path;
    path = strdup(filename);//copia o path
    if (path == NULL || strlen(path) == 0) { return ERROR; }
    
    while ((component = strsep(&path, "/")) != NULL) {
        if (strcmp(component, "") == 0){ continue; }
        printf("component: %s  ---  path: %s  --- isLast: %i\n",component, path, isLastComponent(path));
        printf("Path Component: %s, será procurado no iNode: %i\n", component, currentInodeIndex);
        struct t2fs_inode *inode = get_inode(currentInodeIndex);
        record = get_record_in_block(component, inode->dataPtr[0]);
        if (record == NULL) {printf("parte do path não existia\n");return ERROR;}
        if (record->TypeVal == TYPEVAL_REGULAR) {break;} // há de se fazer algo pra isso, por enquanto foda-se
        currentInodeIndex = record->inodeNumber;
    }
    if (record == NULL) { printf("não achou\n"); return ERROR; }
    if (record->TypeVal != TYPEVAL_REGULAR) { printf("achou diretorio onde deveria ser arquivo\n"); return ERROR; }
    if (strcmp(record->name, basename(filename)) != 0) { printf("achou arquivo onde deveria ser diretorio\n"); return ERROR; }
    
    int parent_dir_inode_index = currentInodeIndex; //precisa de update no bloco que aponta
    int child_file_inode_index = record->inodeNumber; //precisa ser deletado
    
    //tudo ok, record deve ser deletado do bloco atual(pai)
    delete_record_in_block(record, get_inode(parent_dir_inode_index)->dataPtr[0]);
    
    struct t2fs_inode *inode = get_inode(child_file_inode_index);
    setBitmap2(BITMAP_DADOS, inode->dataPtr[0], BIT_FREE);
    //precisa remover inode(record->inodeNumber) e setar FREE
    setBitmap2(BITMAP_INODE, child_file_inode_index, BIT_FREE);
    //precisa checar se o arquivo não estava aberto e então deletar ele do array de abertos
    remove_from_opened_list(child_file_inode_index);
    show_opened_files_info();
    return 0;
    
}

// FOI // Função usada para fechar um arquivo.
int close2 (FILE2 handle) {
    INIT();
    return remove_from_opened_list(handle);
}

// Função usada para realizar a leitura de uma certa quantidade de bytes (size) de um arquivo.
int read2 (FILE2 handle, char *buffer, int size) {
    INIT();
    //buscar o inode pelo handle
//    int opened_file_index = get_index_of_opened_file(handle);


    return ERROR;
}

// Função usada para realizar a escrita de uma certa quantidade de bytes (size) de um arquivo.
int write2 (FILE2 handle, char *buffer, int size) {
    INIT();
    //buscar o inode pelo handle
//    int opened_file_index = get_index_of_opened_file(handle);
//    write_to_block(opened_files[opened_file_index].pointer, get_inode(handle)->dataPtr[0],buffer, size);
//    
    return ERROR;
}

// Função usada para truncar um arquivo. Remove do arquivo todos os bytes a partir da posição atual do contador de posição (current pointer), inclusive, até o seu final.
int truncate2 (FILE2 handle) {
    INIT();
    return ERROR;
}

// FOI // Altera o contador de posição (current pointer) do arquivo.
int seek2 (FILE2 handle, DWORD offset) {
    INIT();
    return open_file_seek(handle, offset);
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

// FOI // Função que abre um diretório existente no disco.
DIR2 opendir2 (char *pathname) {
    INIT();
    if (opened_files_count >= 20) {printf("Ja existem 20 arquivos abertos");return ERROR;}
    printf("OPEN DIR: %s\n", pathname);
    struct t2fs_record *record;
    int currentInodeIndex = ROOT_INODE_BLOCK_INDEX;//raiz
    char* component;
    char* path;
    path = strdup(pathname);//copia o path
    if (path == NULL || strlen(path) == 0) { return ERROR; }
    
    while ((component = strsep(&path, "/")) != NULL) {
        if (strcmp(component, "") == 0){ continue; }
        printf("component: %s  ---  path: %s  --- isLast: %i\n",component, path, isLastComponent(path));
        printf("Path Component: %s, será procurado no iNode: %i\n", component, currentInodeIndex);
        struct t2fs_inode *inode = get_inode(currentInodeIndex);
        record = get_record_in_block(component, inode->dataPtr[0]);
        if (record == NULL) {printf("parte do path não existia\n");return ERROR;}
        if (record->TypeVal == TYPEVAL_REGULAR) {break;} // há de se fazer algo pra isso, por enquanto foda-se
        currentInodeIndex = record->inodeNumber;
    }
    if (record == NULL) { printf("não achou\n"); return ERROR; }
    if (record->TypeVal != TYPEVAL_DIRETORIO) { printf("achou arquivo onde deveria ser DIR\n"); return ERROR; }
    if (strcmp(record->name, basename(pathname)) != 0) { printf("achou arquivo onde deveria ser diretorio\n"); return ERROR; }
    
    printf("Fim do Open DIR - achou-> %s \n", record->name);
    add_to_opened_list(record->inodeNumber);
//    show_opened_files_info();
    return record->inodeNumber;
}

// FOI // Função usada para ler as entradas de um diretório.
int readdir2 (DIR2 handle, DIRENT2 *dentry) {
    INIT();
    int opened_dir_index = get_index_of_opened_file(handle);
    struct opened_file *dir = &(opened_files[opened_dir_index]);
    DWORD current_offset = dir->pointer;
    dir->pointer += T2FS_RECORD_SIZE;
    struct t2fs_inode *inode = get_inode(handle);
    struct t2fs_record *record = get_record_in_block_with_offset(inode->dataPtr[0], current_offset);
    if (record == NULL) {
//        printf("RECORD VEIO NULL. POSSIVEL FIM DO DIRETORIO.\n");
        return ERROR;
    }
    
    dentry->fileSize = record->bytesFileSize;
    dentry->fileType = record->TypeVal;
    strncpy(dentry->name, record->name, 32);
    
    return 0;
}


// FOI // Função usada para fechar um diretório.
int closedir2 (DIR2 handle) {
    INIT();
    return remove_from_opened_list(handle);
}
