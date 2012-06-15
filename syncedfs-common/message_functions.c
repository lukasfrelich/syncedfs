/* 
 * File:   message_functions.c
 * Author: lfr
 *
 * Created on May 16, 2012, 4:44 PM
 */

#include <stdlib.h>
#include <limits.h>
#include "string.h"
#include <arpa/inet.h>
#include "message_functions.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "logging_functions.h"

int packMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length) {

    uint32_t msglen;
    static uint32_t totallen = 0; // size of buffer
    uint8_t *buf;
    static uint32_t *bufbegin = NULL; // buffer used across all calls
    // TODO: how to free it

    switch (msgtype) {
        case SyncInitType:
            msglen = (uint32_t) sync_init__get_packed_size
                    ((SyncInit *) message);
            break;
        case SyncInitResponseType:
            msglen = (uint32_t) sync_init_response__get_packed_size
                    ((SyncInitResponse *) message);
            break;
        case SyncFinishType:
            msglen = (uint32_t) sync_finish__get_packed_size
                    ((SyncFinish *) message);
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
            errMsg(LOG_ERR, "Unsupported message type.");
            return -1;
    }

    // we might need to increase size of the buffer
    if (msglen + sizeof (uint32_t) > totallen) {
        if (bufbegin != NULL)
            free(bufbegin);

        bufbegin = malloc(msglen + sizeof (uint32_t));
        if (bufbegin == NULL) {
            errMsg(LOG_ERR, "Failed to allocate memory for packing a message.");
            return -1;
        }
    }
    totallen = msglen + sizeof (uint32_t);
    *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);

    int ret;
    switch (msgtype) {
        case SyncInitType:
            ret = sync_init__pack((SyncInit *) message, buf);
            break;
        case SyncInitResponseType:
            ret = sync_init_response__pack
                    ((SyncInitResponse *) message, buf);
            break;
        case SyncFinishType:
            ret = sync_finish__pack((SyncFinish *) message, buf);
            break;
        case FileChunkType:
            ret = file_chunk__pack((FileChunk *) message, buf);
            break;
        case FileOperationType:
            ret = file_operation__pack((FileOperation *) message, buf);
            break;
    }

    if (ret != msglen) {
        errMsg(LOG_ERR, "Pack operation failed.");
        return -1;
    }

    *buffer = (uint8_t *) bufbegin;
    *length = totallen;

    return 0;
}

void *recvMessage(int fd, enum messagetype msgtype, long long *bytesread) {
    size_t s;
    uint8_t *buf;
    uint32_t msglen;
    void *message;

    // TODO: make more robust (could be interrupted by a signal)
    s = recv(fd, &msglen, sizeof (uint32_t), MSG_WAITALL);
    if (s != sizeof (uint32_t))
        return NULL;

    msglen = ntohl(msglen);

    buf = malloc(msglen);
    if (buf == NULL)
        return NULL;

    s = recv(fd, buf, msglen, MSG_WAITALL);
    if (s != msglen)
        return NULL;

    switch (msgtype) {
        case SyncInitType:
            message = sync_init__unpack(NULL, msglen, buf);
            break;
        case SyncInitResponseType:
            message = sync_init_response__unpack(NULL, msglen, buf);
            break;
        case SyncFinishType:
            message = sync_finish__unpack(NULL, msglen, buf);
            break;
        case FileChunkType:
            message = file_chunk__unpack(NULL, msglen, buf);
            break;
        case FileOperationType:
            errMsg(LOG_ERR, "Received unsupported message type.");
            return NULL;
            break; // we should never get this message type from a socket
    }

    free(buf);
    if (bytesread != NULL)
        *bytesread = *bytesread + msglen + sizeof (uint32_t);
    
    return message;
}

int sendMessage(int fd, enum messagetype msgtype, void *message) {
    uint8_t *buf;
    uint32_t msglen;
    
    if (packMessage(msgtype, &message, &buf, &msglen) == -1) {
        errMsg(LOG_ERR, "Could not pack a InitializationResponse message.");
        return -1;
    }
    if (send(fd, buf, msglen, MSG_NOSIGNAL) == -1) {
        errnoMsg(LOG_ERR, "Sending file chunk has failed.");
        return -1;
    }
    
    return 0;
}
