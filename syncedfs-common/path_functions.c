/* 
 * File:   paths.c
 * Author: lukes
 *
 * Created on May 28, 2012, 8:57 PM
 */

#include <string.h>
#include <sys/stat.h>
#include "path_functions.h"

void getAbsolutePath(char *fpath, const char *rootdir, const char *path) {
    int rdirlen;
    strcpy(fpath, rootdir);
    rdirlen = strlen(rootdir);

    // if relative path does not begin with '/'
    if (path != NULL && *path != '/') {
        fpath[rdirlen] = '/';
        strncpy(fpath + rdirlen + 1, path, PATH_MAX - rdirlen - 1);
    } else {
        strncpy(fpath + rdirlen, path, PATH_MAX - rdirlen);
    }
    fpath[PATH_MAX - 1] = '\0'; // fpath might not have been terminated
}

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

int fileExists(char *path) {
    struct stat st;
 
    if (stat(path, &st) == -1)
        return 0;
    else
        return 1;
}