/* 
 * File:   client.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef CLIENT_H
#define	CLIENT_H

#include "config.h"
#include "../syncedfs-common/lib/uthash.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"

typedef struct fileop {
    char filename[PATH_MAX]; // key
    int order; // files must be transfered in order
    GenericOperation **operations;
    int capacity;
    int nelem;
    UT_hash_handle hh;
} fileop_t;

typedef struct dyndata {
    uint8_t *buf;
    int offset;
    int size;
} dyndata_t;

void synchronize(void);

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------
void loadLog(int logfd);
void printLog(void);
void addOperation(char *relpath, GenericOperation *genop);
void optimizeOperations(fileop_t *fileop);

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------
void transfer(char *host, char *port);
void initiateSync(int sfd, int numfiles);
void transferChunk(int sfd, fileop_t *fileop, GenericOperation **opstart,
        int nops, int numchunks);

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
int loadWriteData(char *relpath, WriteOperation *writeop, dyndata_t *dyndata);
/*int cHandleGenericOperation(int fd, GenericOperation *genop,
        dyndata_t *dyndata);*/

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------
int sortByOrder(fileop_t *a, fileop_t *b);
int switchLog(void);

#endif	/* CLIENT_H */

