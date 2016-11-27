//
//  superblock.c
//  t2fs
//
//  Created by Henrique Valcanaia on 27/11/16.
//  Copyright © 2016 SISOP. All rights reserved.
//

#include "superblock.h"

int read_superblock(char* path, struct t2fs_superbloco* superblock) {
    FILE *fileptr;
    
    fileptr = fopen(path, "rb");
    if (fileptr == NULL) {
        printf("Erro(%d):: %s\n", errno, strerror(errno));
        return -1;
    } else {
        if (superblock == NULL) {
            printf("Erro ao tentar alocar memória para o superbloco!");
        } else {
            fread(superblock, sizeof(struct t2fs_superbloco), 1, fileptr);
        }
    }
    
    fclose(fileptr);
    return 1;
}

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
