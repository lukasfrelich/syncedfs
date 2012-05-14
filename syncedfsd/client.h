/* 
 * File:   client.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef CLIENT_H
#define	CLIENT_H

#include "config.h"
#include "lib/tlpi_hdr.h"
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

void transfer(char *host, char *port);
void sync(void);
void processOperation(char *line);
void readLog(char *logpath);
void addOperation(char *relpath, GenericOperation *op);
void printLog(void);
int optimizeOperations(fileop_t *fileop);
void transferChunk(int cfd, fileop_t *fileop, GenericOperation **opstart,
        int nops);


#endif	/* CLIENT_H */

