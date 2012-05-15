/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on April 26, 2012, 9:54 AM
 */
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/inet_sockets.h"
#include "config.h"
#include "client.h"
#include "server.h"
#include "protobuf/syncedfs.pb-c.h"

void usage(void) {
    fprintf(stderr, "usage: syncedfsd server|client [host] port\n");
    abort();
}

/*void log_write(const char *relpath, off_t offset, size_t size) {
    FILE *log;
    log = fopen("/home/lfr/writes.log", "ab");
    if (log == NULL)
        perror("open");
    
    uint32_t msglen;
    uint32_t writelen;
    uint8_t *buf;
    uint32_t *buf_p;
    FileOperation fileop = FILE_OPERATION__INIT;
    GenericOperation genop = GENERIC_OPERATION__INIT;
    WriteOperation writeop = WRITE_OPERATION__INIT;

    writeop.offset = (int64_t) offset;
    writeop.size = (int32_t) size;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__WRITE;
    genop.write_op = &writeop;

    fileop.relative_path = relpath;
    fileop.op = &genop;

    msglen = (uint32_t) file_operation__get_packed_size(&fileop);
    writelen = msglen + sizeof (uint32_t);
    buf_p = malloc(writelen);
    if (buf_p == NULL) {
        return;
    }
    *buf_p = htonl(msglen);
    buf = (uint8_t *) (buf_p + 1);
    file_operation__pack(&fileop, buf);

    if (fwrite(buf_p, writelen, 1, log) != 1)
        return; //error
    fclose(log);
}*/

/*
 * 
 */
int main(int argc, char** argv) {
/*
    if (argc < 3)
        usage();
    
    if (strcmp(argv[1], "server") == 0) {
        if (argc == 3) {
            parseConfig("", argv[2]);
            startServer();
        } else {
            usage();
        }
    }
    if (strcmp(argv[1], "client") == 0) {
        if (argc == 4) {
            sync();
            parseConfig(argv[3], argv[2]);
        } else {
            usage();
        }
    }
    return (EXIT_SUCCESS);
*/
    sync();
}
