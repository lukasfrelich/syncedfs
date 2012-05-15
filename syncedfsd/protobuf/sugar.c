/* 
 * File:   sugar.c
 * Author: lfr
 *
 * Created on May 15, 2012, 12:34 PM
 */


#include "sugar.h"
#include "syncedfs.pb-c.h"
#include <stdlib.h>
#include <arpa/inet.h>

int getPackedMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length) {
    uint32_t msglen;
    uint32_t totallen;
    uint8_t *buf;
    uint32_t *bufbegin;

    switch (msgtype) {
        case SyncInitializationType:
            break;
        case FileChunkType:
            msglen = (uint32_t) file_chunk__get_packed_size((FileChunk *) message);
            break;
        case FileOperationType:
            break;
        default:
            return -1;  // unsupported message type
    }

    totallen = msglen + sizeof (uint32_t);
    bufbegin = malloc(totallen);
    if (bufbegin == NULL) {
        return -2;      // memory allocation failed
    }
    *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);

    switch (msgtype) {
        case SyncInitializationType:
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
