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
    uint8_t *buf;
    uint32_t msglen;

    if (cfd == -1) {
        errMsg("accept");
        return;
    }
    printf("Connection from %s\n", inetAddressStr(claddr, *addrlen,
            addrstr, IS_ADDR_STR_LEN));

    // sync-init
    SyncInitialization *syncinit;
    getMessage(cfd, &buf, &msglen);
    syncinit = sync_initialization__unpack(NULL, msglen, buf);

    // check sync-id
    // check resource (must match
    // send response
    

    // outer loop: files, inner loop: chunks
    
/*
    
    char header[MAX_HEADER_LEN];
    char msg[MAX_HEADER_LEN];
    char res[RESNAME_MAX];
    int numoper;





    char oper;
    long long dlength;
    char filename[PATH_MAX];
    for (int i = 0; i < numoper; i++) {
        if (readLine(cfd, header, MAX_HEADER_LEN) <= 0) {
            close(cfd);
            errMsg("read header, operation %d", (i + 1));
            return;
        }
        if (parseOperation(header, &oper, &dlength, filename) != 0) {
            close(cfd);
            errMsg("incorrect header, operation %d", (i + 1));
            return;
        }

        if (strcmp(&oper, "write") == 0) {
            handleWrite(cfd, dlength, filename);
        }
    }

    // TODO: send ack
    snprintf(msg, MAX_HEADER_LEN, "ack\n");
*/

    if (close(cfd) == -1)
        errMsg("close");
}

int createSnapshot(void) {
    // btrfs subvolume snapshot -r machine1 machine1-snapshot
    // fork + exec
    return 0;
}

int getMessage(int cfd, uint8_t **buffer, uint32_t *length) {
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
}


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