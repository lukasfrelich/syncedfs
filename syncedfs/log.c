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
#include <fcntl.h>
#include <signal.h>
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/lib/error_functions.h"
#include "log.h"
#include "sighandlers.h"

//log_t olog;
static int logfd;
volatile sig_atomic_t switchpending = 0;
volatile sig_atomic_t writepending = 0;

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

int openLog() {
    char logpath[PATH_MAX];
    snprintf(logpath, PATH_MAX, "%s/%s.log", config.logdir, config.resource);
    //printf("%s", logpath);

    // O_SYNC absolutely kills the performance
    logfd = open(logpath, O_WRONLY | O_APPEND | O_CREAT, /*O_SYNC,*/
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (logfd == -1)
        return -1;

    return 0;
}

void switchLog(void) {
    // switch log by renaming current log and opening a new log
    char oldpath[PATH_MAX], newpath[PATH_MAX];
    snprintf(oldpath, PATH_MAX, "%s/%s.log", config.logdir, config.resource);
    snprintf(newpath, PATH_MAX, "%s/%s.sync", config.logdir, config.resource);

    if (close(logfd) == -1)
        errExit("Could not close log file %s.", oldpath);

    if (rename(oldpath, newpath) == -1)
        fatal("Could not rename log file %s to %s", oldpath, newpath);

    if (openLog() != 0)
        errExit("Could not create new log file.");
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

void logWrite(const char *relpath, off_t offset, size_t size) {
    writepending = 1;
    
    // TODO: change to use getPackedMessage?
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

    fileop.relative_path = (char *) relpath;
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

    if (write(logfd, bufbegin, writelen) != writelen)
        return; // TODO: log
    
    writepending = 0;
    if (switchpending == 1)
        handleSIGUSR1(0, NULL, NULL);
}
