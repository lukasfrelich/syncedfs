/* 
 * File:   sugar.h
 * Author: lfr
 *
 * Created on May 15, 2012, 12:34 PM
 */

#ifndef SUGAR_H
#define	SUGAR_H

enum messagetype {
    SyncInitializationType,
    FileChunkType,
    FileOperationType
};

#include <inttypes.h>

int getPackedMessage(enum messagetype msgtype, void *message, uint8_t **buffer,
        uint32_t *length);
void freePackedMessage(uint8_t *buffer);

#endif	/* SUGAR_H */

