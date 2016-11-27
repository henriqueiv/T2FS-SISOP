//
//  main.c
//  t2fs
//
//  Created by Henrique Valcanaia on 14/11/16.
//  Copyright © 2016 SISOP. All rights reserved.
//

#include <stdio.h>
#include "superblock.h"

int main(int argc, const char * argv[]) {
    
    struct t2fs_superbloco* superblock = (struct t2fs_superbloco*) malloc(sizeof(struct t2fs_superbloco));
    read_superblock("/Users/valcanaia/Documents/UFRGS/2016-2/Cadeiras/INF01142 - Sistemas Operacionais I N/Trabalhos/T2/t2fs_disk.dat", superblock);
    show_superblock_info(superblock);
    
    return 0;
}
