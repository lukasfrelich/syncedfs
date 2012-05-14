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
#include <stdio.h>

/*void test(const char *relpath, off_t offset, size_t size) {
    FILE *f;
    uint32_t msglen;
    uint32_t writelen;
    uint8_t *buf;
    uint32_t *bufbegin;
    FileOperation fileop = FILE_OPERATION__INIT;
    FileOperation *fileop2;
    GenericOperation genop = GENERIC_OPERATION__INIT;
    WriteOperation writeop = WRITE_OPERATION__INIT;

    f = fopen("/home/lukes/twrite.log", "wb");
    writeop.offset = (int64_t) offset;
    writeop.size = (int32_t) size;

    genop.has_type = 1;
    genop.type = GENERIC_OPERATION__OPERATION_TYPE__WRITE;
    genop.writeop = &writeop;

    fileop.relpath = relpath;
    fileop.op = &genop;

    msglen = (uint32_t) file_operation__get_packed_size(&fileop);
    writelen = msglen + sizeof (uint32_t);
    bufbegin = malloc(writelen);
    if (bufbegin == NULL) {
        return;
    }
 *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);
    file_operation__pack(&fileop, buf);

    fileop2 = file_operation__unpack(NULL, msglen, buf);

    return;

    //if (fwrite(bufbegin, writelen, 1, f) != 1)
    //    return; //error
}*/

fileop_t *files = NULL;

char *getAbsolutePath(char *relpath) {
    static char absolutepath[PATH_MAX];
    (void) strcpy(absolutepath, "/home/lfr/test");
    (void) strcpy(absolutepath, relpath);

    return absolutepath;
}

int sortByOrder(fileop_t *a, fileop_t *b) {
    if (a->order == b->order) return 0;
    return (a->order < b->order) ? -1 : 1;
}

void transfer(char *host, char *port) {
    int cfd;
    fileop_t *fileop;
    GenericOperation *op;
    uint8_t *wdata;
    FILE *f;
    GenericOperation **opstart;
    int nops = 0;       // how many GenericOperations are in the message
    int nbytes = 0;     // how many data bytes (coming from write) are in
                        // the message

    wdata = malloc(MESSAGE_MAX * sizeof (uint8_t));
    if (wdata == NULL)
        perror("malloc");

    cfd = inetConnect(host, port, SOCK_STREAM);

    // iterate over files in correct order
    HASH_SORT(files, sortByOrder);
    for (fileop = files; fileop != NULL; fileop = (fileop_t*) (fileop->hh.next)) {
        opstart = fileop->operations;
        optimizeOperations(fileop);

        f = fopen(getAbsolutePath(fileop->filename), "rb");

        for (int i = 0; i < fileop->nelem; i++) {
            // 1. Fetch next operation
            op = *(fileop->operations + i);

            // 2. If it is a write operation, we must fiddle around with wdata 
            // buffer
            if (op->type == GENERIC_OPERATION__OPERATION_TYPE__WRITE) {
                // TODO: handle big single writes?
                if (nbytes + op->writeop->size > MESSAGE_MAX) {
                    // create & transfer message
                    transferChunk(cfd, fileop, opstart, nops);

                    // reset counters & pointers
                    opstart = fileop->operations + i;
                    nops = 0;
                    nbytes = 0;
                }
                nbytes += op->writeop->size;
            }
            nops++;

            // 3. load data in operation message
            if (op->type == GENERIC_OPERATION__OPERATION_TYPE__WRITE) {
                ProtobufCBinaryData data = {op->writeop->size, wdata + nbytes};
                if (fread(wdata + nbytes, op->writeop->size, 1, f) != 1)
                    perror("read");
                op->writeop->data = data;
            }

        }
        //transfer rest

        fclose(f);
    }
}

void transferChunk(int cfd, fileop_t *fileop, GenericOperation **opstart,
        int nops) {
    OperationsChunk opchunk = OPERATIONS_CHUNK__INIT;
    uint32_t msglen;
    uint32_t writelen;
    uint8_t *buf;
    uint32_t *bufbegin;

    opchunk.n_ops = nops;
    opchunk.ops = opstart;
    opchunk.relpath = fileop->filename;

    msglen = (uint32_t) operations_chunk__get_packed_size(&opchunk);
    writelen = msglen + sizeof (uint32_t);
    bufbegin = malloc(writelen);
    if (bufbegin == NULL) {
        return;
    }
    *bufbegin = htonl(msglen);
    buf = (uint8_t *) (bufbegin + 1);
    operations_chunk__pack(&opchunk, buf);

    send(cfd, buf, writelen);
}

void sync(void) {
    // force syncedfs to switch to a new log file

    //test("/test.txt", 0, 4096);
    readLog("/home/lukes/writes.log");
    //printLog();
}

void readLog(char *logpath) {
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

        addOperation(fileop->relpath, fileop->op);

        //file_operation__free_unpacked(fileop, NULL);
        free(buf);
    }
}

void addOperation(char *relpath, GenericOperation *op) {
    static int fileorder = 0;
    static int id = 0;
    fileop_t *f;

    op->id = id++;

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

    *(f->operations + f->nelem) = op;
    f->nelem++;
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
                    i, (int) op->type, (int) op->writeop->offset, (int) op->writeop->size);
        }

    }

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
            if (nbytes + op->writeop->size > MESSAGE_MAX) {
                nops = 0;
                nbytes = 0;
                nmessages++;
            }
            nbytes += op->writeop->size;
        }
        nops++;
    }
    
    if (nops > 0)
        nmessages++;
    
    return nmessages;
}
