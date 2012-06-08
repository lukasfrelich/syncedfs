/* 
 * File:   log.h
 * Author: lfr
 *
 * Created on April 23, 2012, 3:12 PM
 */

#ifndef LOG_H
#define	LOG_H

#include <unistd.h>
#include <fcntl.h>
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"

int openOpLog(void);
int switchLog(void);
void logGeneric(const char *relpath, GenericOperation genop);

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
void logCreate(const char *relpath, mode_t mode);
void logMknod(const char *relpath, mode_t mode, dev_t dev);
void logMkdir(const char *relpath, mode_t mode);
void logSymlink(const char *relpath, const char *target);
void logLink(const char *relpath, const char *target);
void logWrite(const char *relpath, size_t size, off_t offset);
void logUnlink(const char *relpath);
void logRmdir(const char *relpath);
void logTruncate(const char *relpath, off_t newsize);
void logChmod(const char *relpath, mode_t mode);
void logChown(const char *relpath, uid_t uid, gid_t gid);
void logRename(const char *relpath, const char *newpath);

#endif	/* LOG_H */
