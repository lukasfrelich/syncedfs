/* 
 * File:   log.c
 * Author: lfr
 *
 * Created on April 20, 2012, 4:13 PM
 * 
 * Based on Big Brother File System by Joseph J. Pfeiffer, Jr., Ph.D.
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "log.h"
#include "protobuf/syncedfs.pb-c.h"

log_t olog;

/*FILE *log_open() {
    FILE *logfile;
    
    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't open the logfile, we're dead.
    logfile = fopen("/home/lfr/syncedfs.log", "w");
    if (logfile == NULL) {
        perror("logfile");
        exit(EXIT_FAILURE);
    }
    
    // set logfile to line buffering
    setvbuf(logfile, NULL, _IOLBF, 0);

    return logfile;*
    return NULL;
}*/
/*void log_msg(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vfprintf(SFS_DATA->logfile, format, ap);
}*/

int writelog_open() {
    FILE *logfile;

    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't opent he logfile, we're dead.
    logfile = fopen("/home/lfr/syncedfs/primary/r0.log", "ab");
    if (logfile == NULL) {
        perror("logfile");
        exit(EXIT_FAILURE);
    }

    // set logfile to no buffering
    setvbuf(logfile, NULL, _IONBF, 0);

    // TODO
    return 0;
}


void log_write(const char *relpath, off_t offset, size_t size) {
    // TODO: change to use getPackedMessage
    
    uint32_t msglen;
    uint32_t writelen;
    uint8_t *buf;
    uint32_t *bufbegin;
    FileOperation fileop = FILE_OPERATION__INIT;
    GenericOperation genop = GENERIC_OPERATION__INIT;
    WriteOperation writeop = WRITE_OPERATION__INIT;

    writeop.offset = (int64_t) offset;
    writeop.size = (int32_t) size;

    genop.has_type = 1;
    genop.type = GENERIC_OPERATION__OPERATION_TYPE__WRITE;
    genop.write_op = &writeop;

    fileop.relative_path = relpath;
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

    if (write(olog.fd, bufbegin, writelen) != writelen)
        return;
}

