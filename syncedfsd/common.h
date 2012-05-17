/* 
 * File:   common.h
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#ifndef COMMON_H
#define	COMMON_H

enum messagetype {
    SyncInitializationType,
    FileChunkType,
    FileOperationType
};

#include <inttypes.h>

char *getAbsolutePath(char *relpath);
int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length);
void freePackedMessage(uint8_t *buffer);
void *getMessageFromSocket(int cfd, enum messagetype msgtype, long long *bytesread);

#endif	/* COMMON_H */

