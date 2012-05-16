/* 
 * File:   client.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#define _BSD_SOURCE

#include "config.h"
#include "client.h"
#include "lib/inet_sockets.h"
#include "lib/uthash.h"
#include "common.h"
#include "lib/tlpi_hdr.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

fileop_t *files = NULL; // Hash map

void sync(void) {
    // force syncedfs to switch to a new log file

    // new sync or resume?

    //test("/test.txt", 0, 4096);
    processLog("/home/lukes/writes.log");
    //printLog();
}

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------
void processLog(char *logpath) {
    FILE *log;
    uint8_t *buf;
    uint32_t msglen;
    FileOperation *fileop;

    log = fopen(logpath, "rb");
    if (log == NULL)
        errMsg("open");

    while (fread(&msglen, sizeof (uint32_t), 1, log) == 1) {
        msglen = ntohl(msglen);

        buf = malloc(msglen);
        if (buf == NULL)
            perror("malloc buf");
        if (fread(buf, msglen, 1, log) != 1)
            perror("read message");

        fileop = file_operation__unpack(NULL, msglen, buf);

        addOperation(fileop->relative_path, fileop->op);

        //file_operation__free_unpacked(fileop, NULL);
        free(buf);
    }
}

void printLog(void) {
    HASH_SORT(files, sortByOrder);
    fileop_t *f;
    GenericOperation *op;

    for (f = files; f != NULL; f = (fileop_t*) (f->hh.next)) {
        printf("%s\n", f->filename);

        for (int i = 0; i < f->nelem; i++) {
            op = *(f->operations + i);
            printf("Operation %d, type: %d, offset: %d, size: %d\n",
                    i, (int) op->type, (int) op->write_op->offset,
                    (int) op->write_op->size);
        }
    }

}

void addOperation(char *relpath, GenericOperation *genop) {
    static int fileorder = 0;
    static int id = 0;
    fileop_t *f;

    genop->id = id++;

    HASH_FIND_STR(files, relpath, f);

    // key not found
    if (f == NULL) {
        f = (fileop_t*) malloc(sizeof (fileop_t));
        if (f == NULL)
            errExit("malloc");

        (void) strcpy(f->filename, relpath);
        f->order = fileorder++;
        f->capacity = VECTOR_INIT_CAPACITY;
        f->operations = malloc(VECTOR_INIT_CAPACITY * sizeof (fileop_t*));
        HASH_ADD_STR(files, filename, f);
    }

    // extend vector of operations if needed
    if (f->nelem == (f->capacity - 1)) {
        f->capacity = f->capacity * 2;
        f->operations = realloc(f->operations,
                f->capacity * sizeof (fileop_t*));
    }

    *(f->operations + f->nelem) = genop;
    f->nelem++;
}

int optimizeOperations(fileop_t *fileop) {
    // TODO: Interval tree algorithm
    // Detect deletions
    // etc.

    // number of messages need to transfer all file operations
    int nbytes = 0;
    int nops = 0;
    int nmessages = 0;
    GenericOperation *op;

    for (int i = 0; i < fileop->nelem; i++) {
        op = *(fileop->operations + i);

        if (op->type == GENERIC_OPERATION__OPERATION_TYPE__WRITE) {
            // TODO: handle big single writes?
            if (nbytes + op->write_op->size > MESSAGE_MAX) {
                nops = 0;
                nbytes = 0;
                nmessages++;
            }
            nbytes += op->write_op->size;
        }
        nops++;
    }

    if (nops > 0)
        nmessages++;

    return nmessages;
}

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------
void transfer(char *host, char *port) {
    int cfd;
    fileop_t *fileop;
    GenericOperation *genop;
    uint8_t *wdata;
    int fd;
    GenericOperation **opstart;
    int nchunks; // how many chunks for one file we have
    int nops; // how many GenericOperations are in the message
    int nbytes; // how many data bytes (coming from write) are in
    // the message

    wdata = malloc(MESSAGE_MAX * sizeof (uint8_t));
    if (wdata == NULL)
        perror("malloc");

    cfd = inetConnect(host, port, SOCK_STREAM);

    // initiate sync (sync-id, resource, number of files)
    initiateSync(cfd, HASH_COUNT(files));

    // iterate over files in correct order
    HASH_SORT(files, sortByOrder);
    for (fileop = files; fileop != NULL; fileop = (fileop_t*) (fileop->hh.next)) {
        nops = 0;
        nbytes = 0;
        opstart = fileop->operations;

        nchunks = optimizeOperations(fileop);

        fd = open(getAbsolutePath(fileop->filename), O_RDONLY);

        for (int i = 0; i < fileop->nelem; i++) {
            // Fetch next operation
            genop = *(fileop->operations + i);

            // If it is a write operation, we must fiddle around with wdata 
            // buffer
            if (genop->type == GENERIC_OPERATION__OPERATION_TYPE__WRITE) {
                // TODO: handle big single writes?
                if (nbytes + genop->write_op->size > MESSAGE_MAX) {
                    // create & transfer message
                    transferChunk(cfd, fileop, opstart, nops, nchunks);

                    // reset counters & pointers
                    opstart = fileop->operations + i;
                    nops = 0;
                    nbytes = 0;
                }
                nbytes += genop->write_op->size;
            }
            nops++;

            // Load data in operation message
            if (genop->type == GENERIC_OPERATION__OPERATION_TYPE__WRITE) {
                ProtobufCBinaryData data =
                        {genop->write_op->size, wdata + nbytes};
                
                if (pread(fd, wdata + nbytes, genop->write_op->size,
                        genop->write_op->offset) != 1)
                    perror("pread");
                
                genop->write_op->data = data;
            }

        }
        // Transfer rest
        if (nops > 0) {
            transferChunk(cfd, fileop, opstart, nops, nchunks);
        }

        if (close(fd) == -1)
            perror("close file");
    }
}

void initiateSync(int cfd, int numfiles) {
    SyncInitialization syncinit = SYNC_INITIALIZATION__INIT;
    uint8_t *buf;
    uint32_t msglen;

    syncinit.number_files = numfiles;
    syncinit.sync_id = getSyncId();
    syncinit.resource = c_resource;

    packMessage(SyncInitializationType, &syncinit, &buf, &msglen);

    if (send(cfd, buf, msglen, MSG_NOSIGNAL) == -1) {
        perror("send init");
    }

    freePackedMessage(buf);
    
    // wait for response
    // TODO
}

void transferChunk(int cfd, fileop_t *fileop, GenericOperation **opstart,
        int nops, int numchunks) {
    FileChunk opchunk = FILE_CHUNK__INIT;
    uint8_t *buf;
    uint32_t msglen;

    opchunk.relative_path = fileop->filename;
    opchunk.number_chunks = numchunks;
    opchunk.n_ops = nops;
    opchunk.ops = opstart;

    packMessage(FileChunkType, &opchunk, &buf, &msglen);

    if (send(cfd, buf, msglen, MSG_NOSIGNAL) == -1) {
        perror("send chunk");
    }
    // wait for ACK?

    freePackedMessage(buf);
}

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------
int sortByOrder(fileop_t *a, fileop_t *b) {
    if (a->order == b->order)
        return 0;
    return (a->order < b->order) ? -1 : 1;
}