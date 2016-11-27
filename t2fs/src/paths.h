#pragma once

/* Current directory buffer size, limits "char *pathname" length */
#define PWD_BUFFER_SIZE 256

/* Is pathname absolute? Assumes pathname != NULL */
static inline int is_absolute_path(char *pathname) {
  return (pathname[0] == '/' ? 1 : 0);
}

/*
 * Returns the leftmost directory of the pathname. E.g.
 * pathname == "/home/afh"
 * pathname <- leftmos(pathname)
 * pathname == "/home"
 * pathname <- leftmost(pathname)
 * pathname == "/home"
 */
char* leftmost(char *pathname);
