/* 
 * File:   paths.c
 * Author: lukes
 *
 * Created on May 28, 2012, 8:57 PM
 */

#include <string.h>

#include "paths.h"
#include <limits.h>

char *buildPath(char *dest, char *parent, char* child) {
    int plen;
    plen = strlen(parent);

    if (parent != NULL && parent[plen - 1] == '/') {
        strncpy(dest, parent, plen - 1);
    } else {
        strncpy(dest, parent, plen);
    }
    
    dest[plen] = '/';
    plen++;
    
    if (child != NULL && *child == '/') {
        strncpy(dest + plen, child + 1, PATH_MAX - plen - 2);
    } else {
        strncpy(dest + plen, child, PATH_MAX - plen - 2);
    }
    
    return dest;
}
