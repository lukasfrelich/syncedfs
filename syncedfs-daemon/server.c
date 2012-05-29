/* 
 * File:   server.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#include "config.h"
#include "server.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/tlpi_hdr.h"
#include "common.h"

#include <fcntl.h>

void startServer(void) {
    int lfd, cfd;
    socklen_t addrlen;
    struct sockaddr claddr;

    lfd = inetListen(config.port, 0, &addrlen);
    printf("Server booted.\n");

    //setup signal handler to stop handling requests
    for (;;) {
        cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);

        if (cfd == -1) {
            errMsg("accept");
            continue;
        }
        handleClient(cfd, &claddr, &addrlen);
    }
}

void handleClient(int cfd, struct sockaddr *claddr, socklen_t *addrlen) {
    long long nbytes = 0;
    char addrstr[IS_ADDR_STR_LEN];

    if (cfd == -1) {
        errMsg("accept");
        return;
    }
    printf("Connection from %s\n", inetAddressStr(claddr, *addrlen,
            addrstr, IS_ADDR_STR_LEN));

    // sync-init
    SyncInitialization *syncinit;
    syncinit = (SyncInitialization *)
            getMessageFromSocket(cfd, SyncInitializationType, &nbytes);
    if (syncinit == NULL) {
        errExit("Couldn't read message from client.\n");
    }

    // check sync-id
    // check resource (must match)
    if (strcmp(syncinit->resource, config.resource) != 0) {
        // TODO: set fail flag
        return;
    }

    int32_t nfiles;
    nfiles = syncinit->number_files;

    // TODO: send response


    sync_initialization__free_unpacked(syncinit, NULL);

    // outer loop: files, inner loop: chunks
    for (int i = 0; i < nfiles; i++) {
        FileChunk *chunk;
        int fd;

        // we are guaranteed to get at least one chunk for each file
        chunk = (FileChunk *) getMessageFromSocket(cfd, FileChunkType, &nbytes);
        if (chunk == NULL) {
            errExit("Couldn't read message from client.\n");
        }
        fd = open(getAbsolutePath(chunk->relative_path), O_WRONLY | O_CREAT,
                /*temporary*/ S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            perror("open");
            continue;
        }

        for (;;) {
            for (int k = 0; k < chunk->n_ops; k++) {
                sHandleGenericOperation(fd, chunk->ops[k]);
            }

            if (chunk->last_chunk == 1) {
                file_chunk__free_unpacked(chunk, NULL);
                break;
            } else {
                file_chunk__free_unpacked(chunk, NULL);
                chunk = (FileChunk *) getMessageFromSocket(cfd, FileChunkType,
                        &nbytes);
                if (chunk == NULL) {
                    errExit("Couldn't read message from client.\n");
                }
            }
        }

        close(fd);
    }
    if (close(cfd) == -1)
        errMsg("close");
    printf("Transfered bytes: %lld \n", nbytes);
}

int createSnapshot(void) {
    // btrfs subvolume snapshot -r machine1 machine1-snapshot
    // fork + exec
    return 0;
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

int sHandleGenericOperation(int fd, GenericOperation *genop) {
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
            ret = sHandleWrite(fd, genop->write_op);
            break;
    }

    return ret;
}

int sHandleWrite(int fd, WriteOperation *writeop) {
    if (writeop->has_data != 1)
        return -1;

    if (pwrite(fd, writeop->data.data, writeop->size, writeop->offset)
            != writeop->size)
        return -2;

    return 0;
}
