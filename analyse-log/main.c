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
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "../syncedfs-common/logging_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/create_pid_file.h"
#include "../syncedfs-common/lib/uthash.h"

typedef struct fileop {
    char filename[PATH_MAX]; // key
    int order; // files must be transfered in order
    GenericOperation **operations;
    int capacity;
    int nelem;
    UT_hash_handle hh;
} fileop_t;

fileop_t *files = NULL; // Hash map

int sortByOrder(fileop_t *a, fileop_t *b) {
    if (a->order == b->order)
        return 0;
    return (a->order < b->order) ? -1 : 1;
}

int addOperation(char *relpath, GenericOperation *genop) {
    static int fileorder = 0;
    static int id = 0;
    fileop_t *f;

    genop->id = id++;

    HASH_FIND_STR(files, relpath, f);

    // key not found
    if (f == NULL) {
        f = (fileop_t*) malloc(sizeof (fileop_t));
        if (f == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for file operations.");
            return -1;
        }

        (void) strcpy(f->filename, relpath);
        f->order = fileorder++;
        f->capacity = VECTOR_INIT_CAPACITY;
        f->operations = malloc(VECTOR_INIT_CAPACITY * sizeof (fileop_t*));

        if (f->operations == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for file operations.");
            return -1;
        }

        HASH_ADD_STR(files, filename, f);
    }

    // extend vector of operations if needed
    if (f->nelem == (f->capacity - 1)) {
        f->capacity = f->capacity * 2;
        f->operations = realloc(f->operations,
                f->capacity * sizeof (fileop_t*));

        if (f->operations == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for file operations. "
                    "Requested memory: %d", f->capacity * sizeof (fileop_t*));
            return -1;
        }
    }

    *(f->operations + f->nelem) = genop;
    f->nelem++;

    return 0;
}

int loadLog(int logfd) {
    int alocatedmem = 0;
    uint8_t *buf = NULL;
    uint32_t msglen;
    FileOperation *fileop;

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

        if (addOperation(fileop->relative_path, fileop->op) != 0) {
            errMsg(LOG_ERR, "Could add operation to file operations. "
                    "Exiting sync.");
            free(buf);
            return -1;
        }
    }

    free(buf);
    return 0;
}

void printLog(void) {
    HASH_SORT(files, sortByOrder);
    fileop_t *f;
    GenericOperation *op;

    for (f = files; f != NULL; f = (fileop_t*) (f->hh.next)) {
        printf("%s\n", f->filename);

        for (int i = 0; i < f->nelem; i++) {
            op = *(f->operations + i);

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
                    printf("Operation %d, type: link, newpath: %s\n",
                            i, op->link_op->newpath);
                    break;
                case GENERIC_OPERATION__TYPE__WRITE:
                    printf("Operation %d, type: write, offset: %ld, size: %d\n",
                            i, (long int) op->write_op->offset,
                            (int) op->write_op->size);
                    break;
                case GENERIC_OPERATION__TYPE__UNLINK:
                    printf("Operation %d, type: unlink\n", i);
                    break;
                case GENERIC_OPERATION__TYPE__RMDIR:
                    printf("Operation %d, type: rmdir\n", i);
                    break;
                case GENERIC_OPERATION__TYPE__TRUNCATE:
                    printf("Operation %d, type: truncate, newsize: %d\n",
                            i, (int) op->truncate_op->newsize);
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
    }
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

    if (loadLog(logfd) != 0) {
        fprintf(stderr, "Could not load log.");
        return -1;
    }
    
    printLog();
}
