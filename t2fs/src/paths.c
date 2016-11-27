/* For logging.h */
#define _POSIX_C_SOURCE 200112L
#include "paths.h"
#include "t2fs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

char* leftmost(char *pathname) {
    assert(is_absolute_path(pathname) && PWD_BUFFER_SIZE > 1);
    char *ans = malloc(PWD_BUFFER_SIZE * sizeof(*ans));
    ans[0] = '\0';
    if (strlen(pathname) == 1) {
        ans = "/";
    } else {
        size_t i = 1;
        while (pathname[i] != '/' && pathname[i] != '\0' && i < PWD_BUFFER_SIZE)
            i++;
        if (i >= PWD_BUFFER_SIZE) {
            printf("Pathname too long - buffer would overflow, aborting.\n");
        } else if (pathname[i] == '\0') {
            strncpy(ans, pathname + 1, strlen(pathname));
        } else {
            strncpy(ans, pathname + 1, i - 1);
            ans[i - 1] = '\0';
        }
    }
    
    return ans;
}


