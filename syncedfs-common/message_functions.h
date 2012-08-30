/* 
 * File:   message_functions.h
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#ifndef MESSAGE_FUNCTIONS_H
#define	MESSAGE_FUNCTIONS_H

enum messagetype {
    SyncInitType,
    SyncInitResponseType,
    SyncFinishType,
    FileChunkType,
    FileOperationType
};

#include <inttypes.h>

int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length);

void *recvMessage(int fd, enum messagetype msgtype, long long *bytesread);
int sendMessage(int fd, enum messagetype msgtype, void *message);

#endif	/* MESSAGE_FUNCTIONS_H */
