/* 
 * File:   common.c
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#include "common.h"
#include "protobuf/syncedfs.pb-c.h"
#include <stdlib.h>
#include <limits.h>
#include "string.h"
#include <arpa/inet.h>
#include "config.h"

//------------------------------------------------------------------------------
// File system
//------------------------------------------------------------------------------
char *getAbsolutePath(char *relpath) {
    static char absolutepath[PATH_MAX];
    (void) strcpy(absolutepath, c_rootdir);
    (void) strcat(absolutepath, relpath);

    return absolutepath;
}

//------------------------------------------------------------------------------
// Packing messages
//------------------------------------------------------------------------------
int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length) {
    uint32_t msglen;
    uint32_t totallen;
    uint8_t *buf;
    uint32_t *bufbegin;

    switch (msgtype) {
        case SyncInitializationType:
            msglen = (uint32_t) sync_initialization__get_packed_size((SyncInitialization *) message);
            break;
        case FileChunkType:
            msglen = (uint32_t) file_chunk__get_packed_size((FileChunk *) message);
            break;
        case FileOperationType:
            break;
        default:
            return -1; // unsupported message type
    }

    totallen = msglen + sizeof (uint32_t);
    bufbegin = malloc(totallen);
    if (bufbegin == NULL) {
        return -2; // memory allocation failed
    }
    *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);

    switch (msgtype) {
        case SyncInitializationType:
            (void) sync_initialization__pack((SyncInitialization *) message, buf);
            break;
        case FileChunkType:
            (void) file_chunk__pack((FileChunk *) message, buf);
            break;
        case FileOperationType:
            break;
    }

    *buffer = (uint8_t *) bufbegin;
    *length = totallen;

    return 0;
}

void freePackedMessage(uint8_t *buffer) {
    free(buffer);
}

//------------------------------------------------------------------------------
// Unpacking messages
//------------------------------------------------------------------------------
void *getMessageFromSocket(int cfd, enum messagetype msgtype, long long *bytesread) {
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