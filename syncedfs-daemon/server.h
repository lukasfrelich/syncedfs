/* 
 * File:   server.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef SERVER_H
#define	SERVER_H

#include <inttypes.h>
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"

void startServer(void);
void handleClient(int cfd, struct sockaddr *claddr, socklen_t *addrlen);
int createSnapshot(void);
void printOp(const char *relpath, const char *fpath, GenericOperation *op);

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
int handleGenericOperation(int *fd, const char *relpath,
        GenericOperation *genop);
int handleCreate(const char *fpath, int *fd, CreateOperation *createop);
int handleMknod(const char *fpath, MknodOperation *mknodop);
int handleMkdir(const char *fpath, MkdirOperation *mkdirop);
int handleSymlink(const char *fpath, SymlinkOperation *symlinkop);
int handleLink(const char *fpath, LinkOperation *linkop);
int handleWrite(const char *fpath, int *fd, WriteOperation *writeop);
int handleUnlink(const char *fpath, int *fd, UnlinkOperation *unlinkop);
int handleRmdir(const char *fpath, RmdirOperation *rmdirop);
int handleTruncate(const char *fpath, TruncateOperation *truncateop);
int handleChmod(const char *fpath, ChmodOperation *chmodop);
int handleChown(const char *fpath, ChownOperation *chownop);
int handleRename(const char *fpath, RenameOperation *renameop);

#endif	/* SERVER_H */
