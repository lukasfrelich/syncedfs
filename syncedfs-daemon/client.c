/* 
 * File:   client.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#define _GNU_SOURCE

#include "config.h"
#include "client.h"
#include "common.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/create_pid_file.h"
#include "../syncedfs-common/lib/uthash.h"
#include "../syncedfs-common/lib/tlpi_hdr.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

fileop_t *files = NULL; // Hash map

void synchronize(void) {
    char pidpath[PATH_MAX];
    char logpath[PATH_MAX];
    int logfd;

    // make sure, that there isn't another client process for this resource
    // already running
    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/client/%s.pid",
            config.resource);
    createPidFile(config.resource, pidpath, 0);

    // new sync or resume?
    snprintf(logpath, PATH_MAX, "%s/%s.sync", config.logdir, config.resource);
    printf("logpath: %s\n", logpath);
    logfd = open(logpath, O_RDWR);
    if (logfd == -1) {
        if (switchLog() != 0) {
            fatal("Could not switch log file.");
        }

        logfd = open(logpath, O_RDWR);
        if (logfd == -1) {
            fatal("Could not open log file %s", logpath);
        }
    }


    //processLog(logfd);
    //printLog();
    //transfer(config.host, config.port);

    close(logfd);
    // delete pid file
    if (unlink(pidpath) == -1)
        errExit("Deleting PID file '%s'", pidpath);

    // delete log
}

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------

void processLog(int logfd) {
    uint8_t *buf;
    uint32_t msglen;
    FileOperation *fileop;

    while (read(logfd, &msglen, sizeof (uint32_t)) == sizeof (uint32_t)) {
        msglen = ntohl(msglen);

        buf = malloc(msglen);
        if (buf == NULL)
            perror("malloc buf");
        if (read(logfd, buf, msglen) != msglen)
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
    int sfd; // server socket
    fileop_t *fileop; // contains all operations for a file
    GenericOperation *genop; // current operation
    int fd; // descriptor for file we will process

    dyndata_t ddata = {0}; // used for dynamically allocated data (to lower
    // number of malloc calls)
    //int nbytes;          // how many bytes in ddata are used
    GenericOperation **opstart; //first operation to be send in the message
    int nops; // how many GenericOperations are in the message

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

int cHandleGenericOperation(int fd, GenericOperation *genop,
        dyndata_t *dyndata) {
    
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

int switchLog(void) {
    FILE *pidfile;
    long pid;
    char pidpath[PATH_MAX]; // syncedfs pid
    char buf[RESOURCE_MAX];
    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/fs/%s.pid", config.resource);

    pidfile = fopen(pidpath, "r");
    if (pidfile == NULL) {
        errMsg("Error opening syncedfs PID file: %s", pidpath);
        return -1;
    }

    // read PID
    if (fgets(buf, RESOURCE_MAX - 1, pidfile) == NULL) {
        errMsg("Error reading syncedfs PID file: %s", pidpath);
        return -1;
    }

    fclose(pidfile);

    char *endptr = NULL;
    errno = 0;
    pid = strtol(buf, &endptr, 10);
    if (errno != 0 || pid == 0) {
        errMsg("Error reading syncedfs PID file: %s", pidpath);
        return -1;
    }

    // verify PID
    if (kill(pid, 0) == -1) {
        errMsg("Could not verify syncedfs PID.");
        return -1;
    }

    // block SIGUSR1 signal (so a response is not delivered before we wait
    // for it)
    sigset_t origmask, blockmask;

    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &blockmask, &origmask) == -1) {
        errMsg("Could not block SIGUSR1 signal.");
        return -1;
    }

    // send signal
    if (kill(pid, SIGUSR1) == -1) {
        errMsg("Could not send SIGUSR1 signal to %ld", pid);
        return -1;
    }

    // wait for response
    int sig;
    siginfo_t si;
    sigset_t allsigs;
    struct timespec tspec;

    sigemptyset(&allsigs);
    sigaddset(&allsigs, SIGUSR1);
    tspec.tv_sec = 10;

    sig = sigtimedwait(&allsigs, &si, &tspec);
    if (sig != SIGUSR1) {
        errMsg("sigtimedwait failed.");
        return -1;
    }

    if (si.si_pid != pid) {
        errMsg("Received SIGUSR1 from wrong process");
        return -1;
    }
    
    // restore signal mask
    if (sigprocmask(SIG_SETMASK, &origmask, NULL) == -1) {
        errMsg("Could not restore signal mask.");
        return -1;
    }
    return 0;
}