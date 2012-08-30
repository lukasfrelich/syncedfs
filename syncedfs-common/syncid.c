/* 
 * File:   syncid.c
 * Author: lfr
 *
 * Created on June 14, 2012, 11:39 AM
 */

#include "logging_functions.h"
#include "syncid.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int writeSyncId(char *id, char *idpath) {
    int fd;

    fd = open(idpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        errnoMsg(LOG_ERR, "Could not open sync-id file %s", idpath);
        return -1;
    }

    if (ftruncate(fd, 0) == -1) {
        errnoMsg(LOG_ERR, "Could not truncate sync-id file '%s'", idpath);
        return -1;
    }

    if (write(fd, id, strlen(id)) != strlen(id)) {
        errnoMsg(LOG_ERR, "Could not write to sync-id file '%s'", idpath);
        return -1;
    }

    if (close(fd) == -1) {
        errnoMsg(LOG_ERR, "Could not close sync-id file '%s'", idpath);
        return -1;
    }

    return 0;
}

int readSyncId(char *id, char *idpath, int maxlength) {
    FILE *idfile;

    idfile = fopen(idpath, "r");
    if (idfile == NULL) {
        errnoMsg(LOG_ERR, "Error opening sync-id file: %s", idpath);
        return -1;
    }

    if (fgets(id, maxlength, idfile) == NULL) {
        errnoMsg(LOG_ERR, "Error reading sync-id file: %s", idpath);
        return -1;
    }

    if (fclose(idfile) == EOF) {
        errnoMsg(LOG_ERR, "Error closing sync-id file: %s", idpath);
        return -1;
    }
    return 0;
}