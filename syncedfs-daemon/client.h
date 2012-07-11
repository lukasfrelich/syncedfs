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
    char relpath[PATH_MAX]; // key
    int order; // files must be transfered in order
    GenericOperation **operations;
    int capacity;
    int nelem;
    int64_t inode;
    int created; // did the file exist already in the last epoch?
    UT_hash_handle hh;
} fileop_t;

typedef struct dyndata {
    uint8_t *buf;
    int offset;
    int size;
} dyndata_t;

int synchronize(void);

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------
int loadLog(int logfd);
void printLog(void);
int addOperation(char *relpath, GenericOperation *genop);
int mergeOperations(fileop_t *dest, fileop_t *src);
int initializeFileop(char *relpath, fileop_t **newentry);
int matchInodefiles(void);
int optimizeOperations(fileop_t *fileop);

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------
int transfer(char *host, char *port);
int initiateSync(int sfd, int numfiles);
int transferFile(int sfd, fileop_t *fileop);
int transferChunk(int sfd, fileop_t *fileop, GenericOperation **opstart,
        int nops, int numchunks);

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
int loadWriteData(char *relpath, WriteOperation *writeop, dyndata_t *dyndata);

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------
int sortByOrder(fileop_t *a, fileop_t *b);
int switchLog(void);
int generateSyncId(char *id, int maxlength);
// used in analyse-log
fileop_t *getFiles(void);
fileop_t *getInodeFiles(void);

#endif	/* CLIENT_H */
