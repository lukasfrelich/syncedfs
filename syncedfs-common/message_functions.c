/* 
 * File:   message_functions.c
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#include "message_functions.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include <stdlib.h>
#include <limits.h>
#include "string.h"
#include "lib/error_functions.h"
#include <arpa/inet.h>

//------------------------------------------------------------------------------
// File system
//------------------------------------------------------------------------------
// TODO: move to path_functions

char *getAbsolutePath(char *relpath) {
    static char absolutepath[PATH_MAX];
    //(void) strcpy(absolutepath, config.rootdir);
    (void) strcat(absolutepath, relpath);

    return absolutepath;
}

//------------------------------------------------------------------------------
// Packing messages
//------------------------------------------------------------------------------

int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length) {

    uint32_t msglen;
    static uint32_t totallen = 0; // size of buffer
    uint8_t *buf;
    static uint32_t *bufbegin = NULL; // buffer used across all calls
    
    switch (msgtype) {
        case SyncInitializationType:
            msglen = (uint32_t) sync_initialization__get_packed_size
                    ((SyncInitialization *) message);
            break;
        case FileChunkType:
            msglen = (uint32_t) file_chunk__get_packed_size
                    ((FileChunk *) message);
            break;
        case FileOperationType:
            msglen = (uint32_t) file_operation__get_packed_size
                    ((FileOperation *) message);
            break;
        default:
            errMsg("Unsupported message type.");
            return -1;
    }

    // we might need to increase size of the buffer
    if (msglen + sizeof (uint32_t) > totallen) {
        if (bufbegin != NULL)
            free(bufbegin);
        
        bufbegin = malloc(msglen + sizeof (uint32_t));
        if (bufbegin == NULL) {
            errMsg("Failed to allocate memory for packing a message.");
            return -1;
        }
    }
    totallen = msglen + sizeof (uint32_t);
    *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);

    int ret;
    switch (msgtype) {
        case SyncInitializationType:
            ret = sync_initialization__pack((SyncInitialization *) message, buf);
            break;
        case FileChunkType:
            ret = file_chunk__pack((FileChunk *) message, buf);
            break;
        case FileOperationType:
            ret = file_operation__pack((FileOperation *) message, buf);
            break;
    }

    if (ret != msglen) {
        errMsg("Pack operation failed.");
        return -1;
    }

    *buffer = (uint8_t *) bufbegin;
    *length = totallen;

    return 0;
}

/*
void freePackedMessage(uint8_t *buffer) {
    free(buffer);
}
 */

//------------------------------------------------------------------------------
// Unpacking messages
//------------------------------------------------------------------------------

void *getMessageFromSocket(int cfd, enum messagetype msgtype,
        long long *bytesread) {

    size_t s;
    uint8_t *buf;
    uint32_t msglen;
    void *message;

    // TODO: make more robust (could be interrupted by a signal)
    s = recv(cfd, &msglen, sizeof (uint32_t), MSG_WAITALL);
    if (s != sizeof (uint32_t))
        return NULL;

    msglen = ntohl(msglen);

    buf = malloc(msglen);
    if (buf == NULL)
        return NULL;

    s = recv(cfd, buf, msglen, MSG_WAITALL);
    if (s != msglen)
        return NULL;

    switch (msgtype) {
        case SyncInitializationType:
            message = sync_initialization__unpack(NULL, msglen, buf);
            break;
        case FileChunkType:
            message = file_chunk__unpack(NULL, msglen, buf);
            break;
        case FileOperationType:
            break;
    }

    free(buf);
    *bytesread = *bytesread + msglen + sizeof (uint32_t);
    return message;
}