/* 
 * File:   paths.c
 * Author: lukes
 *
 * Created on May 28, 2012, 8:57 PM
 */

#include <string.h>

#include "path_functions.h"
#include <limits.h>

char *getCanonincalPath(char *path) {
    int len;

    if (path != NULL) {
        len = strlen(path);
        if (len > 0 && path[len - 1] == '/') {
            path[len - 1] = '\0';
        }
    }

    return path;
}