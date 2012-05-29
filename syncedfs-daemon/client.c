/* 
 * File:   client.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#include "config.h"
#include "client.h"
#include "common.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/uthash.h"
#include "../syncedfs-common/lib/tlpi_hdr.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

fileop_t *files = NULL; // Hash map

void synchronize(void) {
    // force syncedfs to switch to a new log file

    // new sync or resume?

    //test("/test.txt", 0, 4096);
    processLog("/home/lfr/syncedfs/primary/r0.log");
    //printLog();
    transfer(config.host, config.port);
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
    if (log == NULL) {
        errExit("%s", logpath);
    }

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

void optimizeOperations(fileop_t *fileop) {
    // TODO: Interval tree algorithm
    // Detect deletions
    // etc.
    return;
}

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------

void transfer(char *host, char *port) {
    int sfd;            // server socket
    fileop_t *fileop;   // contains all operations for a file
    GenericOperation *genop;    // current operation
    int fd;             // descriptor for file we will process
    
    dyndata_t ddata = {0};// used for dynamically allocated data (to lower number
                        // of malloc calls)
    //int nbytes;         // how many bytes in ddata are used
    GenericOperation **opstart; //first operation to be send in the message
    int nops;           // how many GenericOperations are in the message

    ddata.buf = malloc(MESSAGE_MAX * sizeof (uint8_t));
    if (ddata.buf == NULL)
        errExit("malloc");
    ddata.size = MESSAGE_MAX;

    sfd = inetConnect(host, port, SOCK_STREAM);
    if (sfd == -1)
        fatal("unable to connect to the server");

    // initiate sync (sync-id, resource, number of files)
    initiateSync(sfd, HASH_COUNT(files));

    // iterate over files in correct order
    HASH_SORT(files, sortByOrder);
    for (fileop = files; fileop != NULL; fileop = (fileop_t*) (fileop->hh.next)) {
        nops = 0;
        ddata.size = 0;
        ddata.offset = 0;
        opstart = fileop->operations;

        optimizeOperations(fileop);

        // TODO: what about deleted files
        fd = open(getAbsolutePath(fileop->filename), O_RDONLY);
        if (fd == -1)
            errExit("Couldn't open source file.\n");

        for (int i = 0; i < fileop->nelem; i++) {
            // Fetch next operation
            genop = *(fileop->operations + i);

            // load data for the operation
            if (cHandleGenericOperation(fd, genop, &ddata) != 0)
                fatal("Couldn't load required data.\n");
            nops++;
            
            if (ddata.size >= MESSAGE_MAX) {
                // if we have just added last operation, set last_chunk flag
                if (i == fileop->nelem - 1)
                    transferChunk(sfd, fileop, opstart, nops, 1);
                else
                    transferChunk(sfd, fileop, opstart, nops, 0);
                
                // reset counters
                opstart = fileop->operations + i;
                nops = 0;
                ddata.offset = 0;
                ddata.size = 0;
            }
        }
        // Transfer last chunk (if there is still some data left)
        if (nops > 0) {
            transferChunk(sfd, fileop, opstart, nops, 1);
        }

        if (close(fd) == -1)
            perror("close file");
    }
}

void initiateSync(int sfd, int numfiles) {
    SyncInitialization syncinit = SYNC_INITIALIZATION__INIT;
    uint8_t *buf;
    uint32_t msglen;

    syncinit.number_files = numfiles;
    syncinit.sync_id = getSyncId();
    syncinit.resource = config.resource;

    packMessage(SyncInitializationType, &syncinit, &buf, &msglen);

    if (send(sfd, buf, msglen, MSG_NOSIGNAL) == -1) {
        perror("send init");
    }

    freePackedMessage(buf);

    // wait for response
    // TODO
}

void transferChunk(int sfd, fileop_t *fileop, GenericOperation **opstart,
        int nops, int lastchunk) {
    FileChunk fchunk = FILE_CHUNK__INIT;
    uint8_t *buf;
    uint32_t msglen;

    fchunk.relative_path = fileop->filename;
    fchunk.last_chunk = lastchunk;
    fchunk.n_ops = nops;
    fchunk.ops = opstart;

    packMessage(FileChunkType, &fchunk, &buf, &msglen);

    if (send(sfd, buf, msglen, MSG_NOSIGNAL) == -1) {
        perror("send chunk");
    }
    // wait for ACK?

    freePackedMessage(buf);
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

int cHandleGenericOperation(int fd, GenericOperation *genop, dyndata_t *dyndata) {
    int ret;

    switch (genop->type) {
        case GENERIC_OPERATION__OPERATION_TYPE__MKNOD:
        case GENERIC_OPERATION__OPERATION_TYPE__MKDIR:
        case GENERIC_OPERATION__OPERATION_TYPE__SYMLINK:
        case GENERIC_OPERATION__OPERATION_TYPE__UNLINK:
        case GENERIC_OPERATION__OPERATION_TYPE__RMDIR:
        case GENERIC_OPERATION__OPERATION_TYPE__RENAME:
        case GENERIC_OPERATION__OPERATION_TYPE__LINK:
        case GENERIC_OPERATION__OPERATION_TYPE__CHMOD:
        case GENERIC_OPERATION__OPERATION_TYPE__CHOWN:
        case GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE:
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__WRITE:
            ret = cHandleWrite(fd, genop->write_op, dyndata);
            break;
    }

    return ret;
}

int cHandleWrite(int fd, WriteOperation *writeop, dyndata_t *dyndata) {
    // realloc, if necessary
    if (writeop->size > dyndata->size - dyndata->offset) {
        int nsize = dyndata->size + writeop->size;
        
        dyndata->buf = realloc(dyndata->buf, nsize);
        if (dyndata->buf == NULL) {
            perror("realloc");
            return -1;
        }
        dyndata->size = nsize;
    }
    
    if (pread(fd, dyndata->buf + dyndata->offset, writeop->size,
            writeop->offset) != writeop->size) {
        perror("pread");
        return -2;
    }

    ProtobufCBinaryData data = {writeop->size, dyndata->buf + dyndata->offset};
    writeop->has_data = 1;
    writeop->data = data;
    
    dyndata->offset += writeop->size;
    return 0;
}

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------

int sortByOrder(fileop_t *a, fileop_t *b) {
    if (a->order == b->order)
        return 0;
    return (a->order < b->order) ? -1 : 1;
}
