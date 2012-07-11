/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on June 11, 2012, 12:00 PM
 */
#define VECTOR_INIT_CAPACITY 8
#define PATH_MAX        4096	/* # chars in a path name including nul */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "../syncedfs-common/logging_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/create_pid_file.h"
#include "../syncedfs-common/lib/uthash.h"
#include "../syncedfs-daemon/optimization.h"
#include "../syncedfs-daemon/client.h"

void writesSize(void) {
    fileop_t *f;
    GenericOperation *genop; // current operation
    long long totalsum = 0;

    // first try to match inodefiles to files
    for (f = getFiles(); f != NULL; f = (fileop_t*) (f->hh.next)) {
        for (int i = 0; i < f->nelem; i++) {
            genop = *(f->operations + i);
            if (genop == NULL) {
                continue;
            }

            if (genop->type == GENERIC_OPERATION__TYPE__WRITE)
                totalsum += genop->write_op->size;
        }
    }
    printf("Total size: %lld\n", totalsum);
}

void optimizedWritesSize(void) {
    fileop_t *f;
    GenericOperation *genop; // current operation
    long long totalsum = 0;

    // first try to match inodefiles to files
    for (f = getFiles(); f != NULL; f = (fileop_t*) (f->hh.next)) {
        optimizeOperations(f);
        for (int i = 0; i < f->nelem; i++) {
            genop = *(f->operations + i);
            if (genop == NULL) {
                continue;
            }

            if (genop->type == GENERIC_OPERATION__TYPE__WRITE)
                totalsum += genop->write_op->size;
        }
    }
    printf("Total size: %lld\n", totalsum);
}

void printOperation(GenericOperation *op, int i) {
    if (op == NULL) {
        printf("Operation %d, type: empty\n", i);
        return;
    }

    switch (op->type) {
        case GENERIC_OPERATION__TYPE__CREATE:
            printf("Operation %d, type: create, mode: %d\n",
                    i, (int) op->create_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__MKNOD:
            printf("Operation %d, type: mknod, mode: %d, dev: %ld\n",
                    i, (int) op->mknod_op->mode,
                    (long int) op->mknod_op->dev);
            break;
        case GENERIC_OPERATION__TYPE__MKDIR:
            printf("Operation %d, type: mkdir, mode: %d\n",
                    i, (int) op->mkdir_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__SYMLINK:
            printf("Operation %d, type: symlink, newpath: %s\n",
                    i, op->symlink_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__LINK:
            printf("Operation %d, type: link, oldpath: %s newpath: %s\n",
                    i, op->link_op->oldpath, op->link_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__WRITE:
            printf("Operation %d, type: write, offset: %ld, size: %d, id: %d\n",
                    i, (long int) op->write_op->offset,
                    (int) op->write_op->size, (int) op->id);
            break;
        case GENERIC_OPERATION__TYPE__UNLINK:
            printf("Operation %d, type: unlink, inode: %ld\n", i, op->unlink_op->inode);
            break;
        case GENERIC_OPERATION__TYPE__RMDIR:
            printf("Operation %d, type: rmdir\n", i);
            break;
        case GENERIC_OPERATION__TYPE__TRUNCATE:
            printf("Operation %d, type: truncate, newsize: %d, id: %d\n",
                    i, (int) op->truncate_op->newsize, (int) op->id);
            break;
        case GENERIC_OPERATION__TYPE__CHMOD:
            printf("Operation %d, type: chmod, mode: %d\n",
                    i, (int) op->chmod_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__CHOWN:
            printf("Operation %d, type: chown, uid: %d, gid: %d\n",
                    i, (int) op->chown_op->uid,
                    (int) op->chown_op->gid);
            break;
        case GENERIC_OPERATION__TYPE__RENAME:
            printf("Operation %d, type: rename, newpath: %s\n",
                    i, op->rename_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__SETXATTR:
        case GENERIC_OPERATION__TYPE__REMOVEXATTR:
            break;
    }
}

void printLog(void) {
    fileop_t *ff = getFiles();
    HASH_SORT(ff, sortByOrder);
    fileop_t *f;
    GenericOperation *op;

    for (f = ff; f != NULL; f = (fileop_t*) (f->hh.next)) {
        printf("%s\n", f->relpath);
        //qsort(f->operations, f->nelem, sizeof (GenericOperation *),cmpOperationType);
        optimizeOperations(f);

        for (int i = 0; i < f->nelem; i++) {
            op = *(f->operations + i);
            printOperation(op, i);
        }
    }
}

int printPureLog(int logfd) {
    int alocatedmem = 0;
    uint8_t *buf = NULL;
    uint32_t msglen;
    FileOperation *fileop;
    int i = 0;

    while (read(logfd, &msglen, sizeof (uint32_t)) == sizeof (uint32_t)) {
        msglen = ntohl(msglen);

        if (alocatedmem < msglen) {
            free(buf);

            buf = malloc(msglen);
            if (buf == NULL) {
                errMsg(LOG_ERR, "Could not allocate memory for message.");
                return -1;
            }
            alocatedmem = msglen;
        }
        if (read(logfd, buf, msglen) != msglen) {
            errnoMsg(LOG_ERR, "Could not read message from log.");
            free(buf);
            return -1;
        }

        // memory leak, need to store allocated addresses and free them later
        //file_operation__free_unpacked(fileop, NULL);
        fileop = file_operation__unpack(NULL, msglen, buf);

        printf("%s: ", fileop->relative_path);
        printOperation(fileop->op, i++);
    }

    free(buf);

    // matchInodefiles
    matchInodefiles();

    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) {
    int logfd;

    if (argc < 2) {
        fprintf(stderr, "usage: analyse-log logfile");
        return (EXIT_FAILURE);
    }

    logfd = open(argv[1], O_RDONLY);
    if (logfd == -1) {
        fprintf(stderr, "Could open log file.");
        return (EXIT_FAILURE);
    }

    if (argc == 3 && strcmp(argv[2], "pure") == 0) {
        if (printPureLog(logfd) != 0) {
            fprintf(stderr, "Could not load log.");
            return -1;
        }
        return 0;
    }
    
    strcpy(config.snapshot, "/media/btrfs/syncedfs/primary/physical");

    if (loadLog(logfd) != 0) {
        fprintf(stderr, "Could not load log.");
        return -1;
    }
    if (argc == 3 && strcmp(argv[2], "writes") == 0) {
        writesSize();
        return 0;
    }

    if (argc == 3 && strcmp(argv[2], "optimized-writes") == 0) {
        optimizedWritesSize();
        return 0;
    }

    printLog();
}
