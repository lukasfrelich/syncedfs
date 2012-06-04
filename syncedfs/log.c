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
#include "../syncedfs-common/lib/error_functions.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "log.h"
#include "sighandlers.h"

//static FILE *tmplogfd;
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
}

void openTmpLog(void) {
    tmplogfd = fopen("/home/lukes/tmp.log", "a");
    setvbuf(tmplogfd, NULL, _IOLBF, 0);
}

void tmpLog(char *msg) {
    fprintf(tmplogfd, "%s", msg);
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

void logGeneric(const char *relpath, GenericOperation genop) {
    uint32_t msglen;
    uint8_t *buf;
    FileOperation fileop = FILE_OPERATION__INIT;

    fileop.relative_path = (char *) relpath;
    fileop.op = &genop;

    packMessage(FileOperationType, &fileop, &buf, &msglen);

    // critical section
    writepending = 1;
    if (write(logfd, buf, msglen) != msglen)
        errMsg("Could not write message.");
    writepending = 0;
    
    if (switchpending == 1)
        handleSIGUSR1(0, NULL, NULL);
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

void logCreate(const char *relpath, mode_t mode) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    CreateOperation creatop = CREATE_OPERATION__INIT;

    creatop.mode = (uint32_t) mode;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__CREATE;
    genop.create_op = &creatop;

    logGeneric(relpath, genop);
}

void logMknod(const char *relpath, mode_t mode, dev_t dev) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    MknodOperation mknodop = MKNOD_OPERATION__INIT;

    mknodop.mode = (uint32_t) mode;
    mknodop.dev = (uint64_t) dev;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__MKNOD;
    genop.mknod_op = &mknodop;

    logGeneric(relpath, genop);
}

void logMkdir(const char *relpath, mode_t mode) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    MkdirOperation mkdirop = MKDIR_OPERATION__INIT;

    mkdirop.mode = (uint32_t) mode;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__MKDIR;
    genop.mkdir_op = &mkdirop;

    logGeneric(relpath, genop);
}

void logSymlink(const char *relpath, const char *target) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    SymlinkOperation symlinkop = SYMLINK_OPERATION__INIT;

    symlinkop.target = (char *) target;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__SYMLINK;
    genop.symlink_op = &symlinkop;

    logGeneric(relpath, genop);
}

void logLink(const char *relpath, const char *target) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    LinkOperation linkop = LINK_OPERATION__INIT;

    linkop.newpath = (char *) target;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__LINK;
    genop.link_op = &linkop;

    logGeneric(relpath, genop);
}

void logWrite(const char *relpath, size_t size, off_t offset) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    WriteOperation writeop = WRITE_OPERATION__INIT;

    writeop.offset = (int64_t) offset;
    writeop.size = (int32_t) size;
    // we don't store data (to save space on disk)

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__WRITE;
    genop.write_op = &writeop;

    logGeneric(relpath, genop);
}

void logUnlink(const char *relpath) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    UnlinkOperation unlinkop = UNLINK_OPERATION__INIT;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__UNLINK;
    genop.unlink_op = &unlinkop;

    logGeneric(relpath, genop);
}

void logRmdir(const char *relpath) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    RmdirOperation rmdirop = RMDIR_OPERATION__INIT;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__RMDIR;
    genop.rmdir_op = &rmdirop;

    logGeneric(relpath, genop);
}

void logTruncate(const char *relpath, off_t newsize) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    TruncateOperation truncateop = TRUNCATE_OPERATION__INIT;

    truncateop.newsize = (int64_t) newsize;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE;
    genop.truncate_op = &truncateop;

    logGeneric(relpath, genop);
}

void logChmod(const char *relpath, mode_t mode) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    ChmodOperation chmodop = CHMOD_OPERATION__INIT;

    chmodop.mode = (uint32_t) mode;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__CHMOD;
    genop.chmod_op = &chmodop;

    logGeneric(relpath, genop);
}

void logChown(const char *relpath, uid_t uid, gid_t gid) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    ChownOperation chownop = CHOWN_OPERATION__INIT;

    chownop.uid = (uint32_t) uid;
    chownop.gid = (uint32_t) gid;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__CHOWN;
    genop.chown_op = &chownop;

    logGeneric(relpath, genop);
}

void logRename(const char *relpath, const char *newpath) {
    GenericOperation genop = GENERIC_OPERATION__INIT;
    RenameOperation renameop = RENAME_OPERATION__INIT;

    renameop.newpath = (char *) newpath;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__RENAME;
    genop.rename_op = &renameop;

    logGeneric(relpath, genop);
}

/*void logWrite(const char *relpath, size_t size, off_t offset) {
    writepending = 1;

    uint32_t msglen;
    uint8_t *buf;
    FileOperation fileop = FILE_OPERATION__INIT;
    GenericOperation genop = GENERIC_OPERATION__INIT;
    WriteOperation writeop = WRITE_OPERATION__INIT;

    writeop.offset = (int64_t) offset;
    writeop.size = (int32_t) size;

    genop.type = GENERIC_OPERATION__OPERATION_TYPE__WRITE;
    genop.write_op = &writeop;

    fileop.relative_path = (char *) relpath;
    fileop.op = &genop;

    packMessage(FileOperationType, &fileop, &buf, &msglen);

    if (write(logfd, buf, msglen) != msglen)
        errMsg("Could not write message.");

    writepending = 0;
    if (switchpending == 1)
        handleSIGUSR1(0, NULL, NULL);
}*/