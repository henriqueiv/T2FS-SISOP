//
//  main.c
//  t2fs
//
//  Created by Henrique Valcanaia on 14/11/16.
//  Copyright © 2016 SISOP. All rights reserved.
//

#include <stdio.h>
#include "include/t2fs.h"

int printa_todo_diretorio(int handle) {
    DIRENT2 entry;
    printf("******** DIR *********\n");
    while ( readdir2(handle, &entry) == 0) {
        printf("readdir2-> %s\n", entry.name);
    }
    printf("-------- /DIR --------\n");
    return 0;
}


int main(int argc, const char * argv[]) {
    create2("sub/f");
    create2("sub/g");
    create2("sub/h");
    create2("sub/i");
    create2("sub/j");
//    open2("sub/arq3");
//    delete2("sub/arq3");
    DIRENT2 entry;
    int dirHandle = opendir2("sub");
    printa_todo_diretorio(dirHandle);
    
    return 0;
}
