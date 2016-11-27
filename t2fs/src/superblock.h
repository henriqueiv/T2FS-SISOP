//
//  superblock.h
//  t2fs
//
//  Created by Henrique Valcanaia on 27/11/16.
//  Copyright © 2016 SISOP. All rights reserved.
//

#ifndef superblock_h
#define superblock_h

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "t2fs.h"

int read_superblock(char* path, struct t2fs_superbloco* superblock);
void show_superblock_info(struct t2fs_superbloco*);

#endif /* superblock_h */
