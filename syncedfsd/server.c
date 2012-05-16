/* 
 * File:   server.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64

#include "config.h"
#include "server.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include "lib/inet_sockets.h"
#include "lib/tlpi_hdr.h"
#include "common.h"

#include <fcntl.h>

void startServer(void) {
    int lfd, cfd;
    socklen_t addrlen;
    struct sockaddr claddr;

    lfd = inetListen(c_port, 0, &addrlen);

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
            getMessageFromSocket(cfd, SyncInitializationType);

    // check sync-id
    // check resource (must match)
    if (strcmp(syncinit->resource, c_resource) != 0) {
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
        int32_t nchunks;
        int fd;
        
        fd = open(chunk->relative_path, O_WRONLY | O_CREAT);
        if (fd == -1)
            perror("open");

        chunk = (FileChunk *) getMessageFromSocket(cfd, FileChunkType);
        nchunks = chunk->number_chunks;
        for (int j = 0; j < nchunks;
                chunk = (FileChunk *) getMessageFromSocket(cfd, FileChunkType)) {

            for (int k = 0; k < chunk->n_ops; k++) {
                handleGenericOperation(fd, chunk->ops[k]);
            }

            file_chunk__free_unpacked(chunk, NULL);
        }
        
        close(fd);

    }

    if (close(cfd) == -1)
        errMsg("close");
}

int createSnapshot(void) {
    // btrfs subvolume snapshot -r machine1 machine1-snapshot
    // fork + exec
    return 0;
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
int handleGenericOperation(int fd, GenericOperation *genop) {
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
            handleWrite(fd, genop->write_op);
    }

    return 0;
}

int handleWrite(int fd, WriteOperation *writeop) {
    
    return 0;
}


/*int getMessage(int cfd, uint8_t **buffer, uint32_t *length) {
    size_t s;
    uint8_t *buf;
    uint32_t msglen;
    
    // TODO: make more robust (could be interrupted by a signal)
    s = recv(cfd, &msglen, sizeof (uint32_t), MSG_WAITALL);
    if (s != sizeof (uint32_t))
        return -1;
    
    msglen = ntohl(msglen);
    
    buf = malloc(msglen);
    if (buf == NULL)
        return -2;
    
    s = recv(cfd, buf, msglen, MSG_WAITALL);
    if (s != msglen)
        return -1;
    
 *buffer = buf;
 *length = msglen;
    return 0;
}*/


/*
int parseSyncStart(char *header, char *resname, int *numoper) {
    char *p_header;

    // parse "sync-start"
    p_header = strtok(header, ";");
    if ((p_header == NULL) || (strcmp(p_header, "sync-start") != 0))
        return -1;

    // parse resource
    p_header = strtok(NULL, ";");
    if (p_header == NULL)
        return -1;
    else
        (void) strcpy(resname, p_header);

    // parse number of operations
    p_header = strtok(NULL, ";");
    if (p_header == NULL)
        return -1;
    else
 *numoper = atoi(p_header);

    return 0;
}

int parseOperation(char *header, char *operation, long long *datalength,
        char *filename) {
    char *p_header;

    // parse operation
    p_header = strtok(header, ";");
    if (p_header == NULL)
        return -1;
    else
        (void) strncpy(operation, p_header, 1);

    // parse number of operations
    p_header = strtok(NULL, ";");
    if (p_header == NULL)
        return -1;
    else
 *datalength = atoll(p_header);

    // parse filename
    p_header = strtok(NULL, ";");
    if (p_header == NULL)
        return -1;
    else
        (void) strcpy(filename, p_header);

    return 0;
}

int handleWrite(int cfd, long long datalength, char *filename) {
    char path[PATH_MAX];
    char data[MAX_WRITE_LEN];
    char boffset[OFFSET_LEN];
    char blength[LENGTH_LEN];
    long long bread = 0;
    int ffd;
    off_t offset = 0;
    size_t length = 0;

    // construct filepath
    // TODO: do it in a better way (use syscall/library function from TLPI)
    (void) strcat(path, c_rootdir);
    (void) strcat(path, filename);

    // TODO: check if file exists
    ffd = open(path, O_WRONLY);
    if (ffd == -1)
        return -1;

    while (bread != datalength) {
        if (recv(cfd, boffset, OFFSET_LEN, MSG_WAITALL) != OFFSET_LEN)
            return -1;
        offset = *((int64_t *) boffset);
        bread += OFFSET_LEN;

        if (recv(cfd, blength, LENGTH_LEN, MSG_WAITALL) != LENGTH_LEN)
            return -1;
        length = *((int32_t *) blength);
        bread += LENGTH_LEN;

        if (recv(cfd, data, length, MSG_WAITALL) != length)
            return -1;
        bread += length;

        // locking?

        if (lseek(ffd, offset, SEEK_SET) != offset)
            return -1;
        if (write(ffd, data, length) != length)
            return -1;
    }

    return 0;
}
 */