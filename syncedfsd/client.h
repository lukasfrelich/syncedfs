/* 
 * File:   client.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef CLIENT_H
#define	CLIENT_H

#include "config.h"
#include "lib/uthash.h"
#include "protobuf/syncedfs.pb-c.h"

typedef struct fileop {
    char filename[PATH_MAX]; // key
    int order; // files must be transfered in order
    GenericOperation **operations;
    int capacity;
    int nelem;
    UT_hash_handle hh;
} fileop_t;

void synchronize(void);

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------
void processLog(char *logpath);
void printLog(void);
void addOperation(char *relpath, GenericOperation *genop);
int optimizeOperations(fileop_t *fileop);

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
int handleGenericOperation(int fd, GenericOperation *genop);
int handleWrite(int fd, WriteOperation *writeop);

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------
int sortByOrder(fileop_t *a, fileop_t *b);

#endif	/* CLIENT_H */

