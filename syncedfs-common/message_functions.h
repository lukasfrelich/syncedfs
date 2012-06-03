/* 
 * File:   message_functions.h
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#ifndef MESSAGE_FUNCTIONS_H
#define	MESSAGE_FUNCTIONS_H

enum messagetype {
    SyncInitializationType,
    FileChunkType,
    FileOperationType
};

#include <inttypes.h>

//------------------------------------------------------------------------------
// File system
//------------------------------------------------------------------------------
char *getAbsolutePath(char *relpath);

//------------------------------------------------------------------------------
// Packing messages
//------------------------------------------------------------------------------
int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length);
void freePackedMessage(uint8_t *buffer);

//------------------------------------------------------------------------------
// Unpacking messages
//------------------------------------------------------------------------------
void *getMessageFromSocket(int cfd, enum messagetype msgtype,
        long long *bytesread);

#endif	/* MESSAGE_FUNCTIONS_H */
