/* 
 * File:   server.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#define ERRMSG_MAX 500

#include "config.h"
#include "server.h"
#include "snapshot.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "../syncedfs-common/syncid.h"
#include "../syncedfs-common/logging_functions.h"
#include "../syncedfs-common/lib/create_pid_file.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/tlpi_hdr.h"

void startServer(void) {
    int pidfd;
    char pidpath[PATH_MAX];
    int lfd, cfd;
    socklen_t addrlen;
    struct sockaddr claddr;

    lfd = inetListen(config.port, 0, &addrlen);
    if (lfd == -1) {
        errMsg(LOG_ERR, "Could not listen on port %s.", config.port);
        return;
    }

    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/server/%s.pid",
            config.resource);
    pidfd = createPidFile(config.ident, pidpath, 0);
    if (pidfd == -1) {
        errMsg(LOG_ERR, "Could not create pid file %s, Exiting server.", pidpath);
        return;
    }

    errMsg(LOG_INFO, "Server booted.");

    //setup signal handler to stop handling requests
    for (;;) {
        cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);

        if (cfd == -1) {
            errnoMsg(LOG_ERR, "Error accepting client.");
            continue;
        }
        handleClient(cfd, &claddr, &addrlen);
    }

    // delete pid file
    if (deletePidFile(pidfd, pidpath) == -1)
        errExit(LOG_ERR, "Deleting PID file '%s'", pidpath);
}

int handleClient(int cfd, struct sockaddr *claddr, socklen_t *addrlen) {
    long long tbytes = 0; // total number of transfered bytes
    char addrstr[IS_ADDR_STR_LEN];

    errMsg(LOG_INFO, "Connection from %s", inetAddressStr(claddr, *addrlen,
            addrstr, IS_ADDR_STR_LEN));

    // sync-init, return value is number of files
    int32_t nfiles;
    nfiles = syncInit(cfd, &tbytes);
    if (nfiles == -1)
        return -1;
    
    errMsg(LOG_INFO, "Number of files to synchronize: %d", nfiles);
    // outer loop: files, inner loop: chunks
    for (int i = 0; i < nfiles; i++) {
        FileChunk *chunk;
        int fd = -1;

        // we are guaranteed to get at least one chunk for each file
        chunk = (FileChunk *) recvMessage(cfd, FileChunkType, &tbytes);
        if (chunk == NULL) {
            errMsg(LOG_ERR, "Could not read message from client.");
            return -1;
        }

        for (;;) {
            for (int k = 0; k < chunk->n_ops; k++) {
                // TODO
                // in case of a failure inform client to do an rsync based sync
                handleGenericOperation(&fd, chunk->relative_path, chunk->ops[k]);
            }

            if (chunk->last_chunk == 1) {
                file_chunk__free_unpacked(chunk, NULL);
                break;
            } else {
                file_chunk__free_unpacked(chunk, NULL);
                chunk = (FileChunk *) recvMessage(cfd, FileChunkType, &tbytes);
                if (chunk == NULL) {
                    errMsg(LOG_ERR, "Could not read message from client.");
                    return -1;
                }
            }
        }

        if (fd != -1) {
            if (close(fd) == -1)
                errMsg(LOG_WARNING, "Could not close file.");
        }

    }

    errMsg(LOG_INFO, "Received bytes: %lld", tbytes);

    // transfer was successful, we can delete snapshot (created in syncInit())
    if (deleteSnapshot(config.snapshot) == -1) {
        errMsg(LOG_ERR, "Could not delete snapshot %s", config.snapshot);
        close(cfd);
        return -1;
    }

    // rename resource.syncid to resource.syncid.last
    char lastsyncidpath[PATH_MAX];
    char syncidpath[PATH_MAX];
    snprintf(lastsyncidpath, PATH_MAX, "%s/%s.syncid.last", config.logdir,
            config.resource);
    snprintf(syncidpath, PATH_MAX, "%s/%s.syncid", config.logdir,
            config.resource);

    if (rename(syncidpath, lastsyncidpath) == -1) {
        errnoMsg(LOG_CRIT, "Could not rename sync-id file to sync-id.last.");
        close(cfd);
        return -1;
    }

    // and finally send confirmation to client
    SyncFinish conf = SYNC_FINISH__INIT;
    conf.has_transferred_bytes = 1;
    conf.transferred_bytes = (uint64_t) tbytes;
    if (sendMessage(cfd, SyncFinishType, &conf) != 0) {
        close(cfd);
        return -1;
    }

    if (close(cfd) == -1)
        errMsg(LOG_WARNING, "Could not close socket.");
    
    return 0;
}

void printOp(const char *relpath, const char *fpath, GenericOperation *op) {
    static int i = 0;
    printf("relpath: %s fpath: %s", relpath, fpath);

    switch (op->type) {
        case GENERIC_OPERATION__TYPE__CREATE:
            printf("Operation %d, type: create, mode: %d\n",
                    i, (int) op->create_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__MKNOD:
            printf("Operation %d, type: mknod, mode: %d, dev: %ld\n",
                    i, (int) op->mknod_op->mode,
                    (long int) op->mknod_op->dev);
            break;
        case GENERIC_OPERATION__TYPE__MKDIR:
            printf("Operation %d, type: mkdir, mode: %d\n",
                    i, (int) op->mkdir_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__SYMLINK:
            printf("Operation %d, type: symlink, newpath: %s\n",
                    i, op->symlink_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__LINK:
            printf("Operation %d, type: link, newpath: %s\n",
                    i, op->link_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__WRITE:
            printf("Operation %d, type: write, offset: %ld, size: %d\n",
                    i, (long int) op->write_op->offset,
                    (int) op->write_op->size);
            break;
        case GENERIC_OPERATION__TYPE__UNLINK:
            printf("Operation %d, type: unlink\n", i);
            break;
        case GENERIC_OPERATION__TYPE__RMDIR:
            printf("Operation %d, type: rmdir\n", i);
            break;
        case GENERIC_OPERATION__TYPE__TRUNCATE:
            printf("Operation %d, type: truncate, newsize: %d\n",
                    i, (int) op->truncate_op->newsize);
            break;
        case GENERIC_OPERATION__TYPE__CHMOD:
            printf("Operation %d, type: chmod, mode: %d\n",
                    i, (int) op->chmod_op->mode);
            break;
        case GENERIC_OPERATION__TYPE__CHOWN:
            printf("Operation %d, type: chown, uid: %d, gid: %d\n",
                    i, (int) op->chown_op->uid,
                    (int) op->chown_op->gid);
            break;
        case GENERIC_OPERATION__TYPE__RENAME:
            printf("Operation %d, type: rename, newpath: %s\n",
                    i, op->rename_op->newpath);
            break;
        case GENERIC_OPERATION__TYPE__SETXATTR:
        case GENERIC_OPERATION__TYPE__REMOVEXATTR:
            break;
    }

    i++;
}

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------

int syncInit(int cfd, long long *tbytes) {
    char errmsg[ERRMSG_MAX] = {0};
    SyncInit *syncinit;
    SyncInitResponse response = SYNC_INIT_RESPONSE__INIT;

    syncinit = (SyncInit *) recvMessage(cfd, SyncInitType, tbytes);
    if (syncinit == NULL) {
        errMsg(LOG_ERR, "Could not read initialization message from client.");
        return -1;
    }

    // initial checks
    // check resource (must match)
    if (strcmp(syncinit->resource, config.resource) != 0) {
        snprintf(errmsg, ERRMSG_MAX, "Received resource %s does not match."
                "local resource %s", syncinit->resource, config.resource);
        response.continue_ = 0;
        response.error_message = errmsg;
        sendMessage(cfd, SyncInitResponseType, &response);
        return -1;
    }

    // check sync-id
    char lastsyncid[SYNCID_MAX];
    char lastsyncidpath[PATH_MAX];
    char syncid[SYNCID_MAX];
    char syncidpath[PATH_MAX];

    // if we receive syncid, we already marked as successfully done
    // only sending confirmation has failed last time
    snprintf(lastsyncidpath, PATH_MAX, "%s/%s.syncid.last", config.logdir,
            config.resource);
    if (fileExists(lastsyncidpath) &&
            (readSyncId(lastsyncid, lastsyncidpath, SYNCID_MAX) == 0)) {
        if (lastsyncid == syncinit->sync_id) {
            // send the confirmation, don't sync
            response.continue_ = 0;
            response.has_already_synced = 1;
            response.already_synced = 1;
            sendMessage(cfd, SyncInitResponseType, &response);
            return 0; // no files to be synchronized
        }
    }

    int errflag = 0;
    // if we can read syncid file, last sync did not finish properly
    // revert snapshot
    snprintf(syncidpath, PATH_MAX, "%s/%s.syncid", config.logdir,
            config.resource);
    if (fileExists(syncidpath) &&
            (readSyncId(syncid, syncidpath, SYNCID_MAX) == 0)) {
        if (syncid == syncinit->sync_id) {
            // TODO: here we should support resume
        }

        // revert last snapshot, check for its existence is necessary, but
        // is done nevertheless
        if (fileExists(config.snapshot) == 1) {
            // do as one block, this should not fail, if yes, we are in trouble
            errMsg(LOG_INFO, "Reverting snapshot.");
            int ret = 0;

            ret = deleteSnapshot(config.rootdir);
            ret |= createSnapshot(config.snapshot, config.rootdir, 0);
            ret |= deleteSnapshot(config.snapshot);

            if (ret != 0) {
                errflag = 1;
                snprintf(errmsg, ERRMSG_MAX, "Reverting to last snapshot"
                        "has failed.");
            }
        }
    }

    // create snapshot
    // snapshot might exists (if last time program crash before sync-id was
    // written or before it was deleted), but in this case we don't need to
    // revert it
    if (!errflag && fileExists(config.snapshot)) {
        errMsg(LOG_INFO, "Snapshot %s unexpectedly exists, deleting.",
                config.snapshot);
        if (deleteSnapshot(config.snapshot) == -1) {
            errMsg(LOG_ERR, "Could not delete snapshot %s", config.snapshot);
            errflag = 1;
        }
    }
    if (!errflag &&
            (createSnapshot(config.rootdir, config.snapshot, 1) != 0)) {
        errMsg(LOG_ERR, "Could not create snapshot %s", config.snapshot);
        errflag = 1;
    }

    // write syncid
    if (!errflag && (writeSyncId(syncinit->sync_id, syncidpath) != 0)) {
        errMsg(LOG_ERR, "Could not write sync-id. Stopping.");
        errflag = 1;
    }

    // send response
    int ret;
    if (errflag) {
        response.continue_ = 0;
        snprintf(errmsg, ERRMSG_MAX, "Error in snapshot operation.");
        response.error_message = errmsg;
        ret = -1;
    } else {
        response.continue_ = 1;
        ret = syncinit->number_files;
    }
    sync_init__free_unpacked(syncinit, NULL);

    if (sendMessage(cfd, SyncInitResponseType, &response) == 0)
        return ret;
    else
        return -1;
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

int handleGenericOperation(int *fd, const char *relpath,
        GenericOperation *genop) {

    int ret;
    char fpath[PATH_MAX];

    // optimistic approach: we are writing the changes right in root directory
    // in case of a failure, we revert back to a snapshot
    getAbsolutePath(fpath, config.rootdir, relpath);
    //printOp(relpath, fpath, genop);

    switch (genop->type) {
        case GENERIC_OPERATION__TYPE__CREATE:
            ret = handleCreate(fpath, fd, genop->create_op);
            break;
        case GENERIC_OPERATION__TYPE__MKNOD:
            ret = handleMknod(fpath, genop->mknod_op);
            break;
        case GENERIC_OPERATION__TYPE__MKDIR:
            ret = handleMkdir(fpath, genop->mkdir_op);
            break;
        case GENERIC_OPERATION__TYPE__SYMLINK:
            ret = handleSymlink(fpath, genop->symlink_op);
            break;
        case GENERIC_OPERATION__TYPE__LINK:
            ret = handleLink(fpath, genop->link_op);
            break;
        case GENERIC_OPERATION__TYPE__WRITE:
            ret = handleWrite(fpath, fd, genop->write_op);
            break;
        case GENERIC_OPERATION__TYPE__UNLINK:
            ret = handleUnlink(fpath, fd, genop->unlink_op);
            break;
        case GENERIC_OPERATION__TYPE__RMDIR:
            ret = handleRmdir(fpath, genop->rmdir_op);
            break;
        case GENERIC_OPERATION__TYPE__TRUNCATE:
            ret = handleTruncate(fpath, genop->truncate_op);
            break;
        case GENERIC_OPERATION__TYPE__CHMOD:
            ret = handleChmod(fpath, genop->chmod_op);
            break;
        case GENERIC_OPERATION__TYPE__CHOWN:
            ret = handleChown(fpath, genop->chown_op);
            break;
        case GENERIC_OPERATION__TYPE__RENAME:
            ret = handleRename(fpath, genop->rename_op);
            break;
        case GENERIC_OPERATION__TYPE__SETXATTR:
        case GENERIC_OPERATION__TYPE__REMOVEXATTR:
            break;
    }

    return ret;
}

int handleCreate(const char *fpath, int *fd, CreateOperation *createop) {
    *fd = creat(fpath, createop->mode);

    if (*fd == -1) {
        errnoMsg(LOG_ERR, "Could not create file %s", fpath);
        return -1;
    }

    return 0;
}

int handleMknod(const char *fpath, MknodOperation *mknodop) {
    if (S_ISREG(mknodop->mode)) {
        int tfd;
        tfd = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mknodop->mode);
        if (tfd == -1) {
            errnoMsg(LOG_ERR, "Could not create file %s", fpath);
            return -1;
        } else {
            if (close(tfd) == -1) {
                errnoMsg(LOG_ERR, "Could not close file %s", fpath);
                return -1;
            }
        }
    } else {
        if (S_ISFIFO(mknodop->mode)) {
            if (mkfifo(fpath, mknodop->mode) == -1) {
                errnoMsg(LOG_ERR, "Could not create named pipe %s", fpath);
                return -1;
            }
        } else {
            if (mknod(fpath, mknodop->mode, mknodop->dev) == -1) {
                errnoMsg(LOG_ERR, "Could not create node %s", fpath);
                return -1;
            }
        }
    }

    return 0;
}

int handleMkdir(const char *fpath, MkdirOperation *mkdirop) {
    if (mkdir(fpath, mkdirop->mode) == -1) {
        errnoMsg(LOG_ERR, "Could not create directory %s", fpath);
        return -1;
    }

    return 0;
}

int handleSymlink(const char *fpath, SymlinkOperation *symlinkop) {
    //char ftarget[PATH_MAX];

    //getAbsolutePath(ftarget, config.rootdir, symlinkop->target);

    if (symlink(symlinkop->newpath, fpath) == -1) {
        errnoMsg(LOG_ERR, "Could not create symlink %s", fpath);
        return -1;
    }

    return 0;
}

int handleLink(const char *fpath, LinkOperation *linkop) {
    char foldpath[PATH_MAX];

    getAbsolutePath(foldpath, config.rootdir, linkop->oldpath);

    // eexist error is ok
    if (link(foldpath, fpath) == -1) {
        errnoMsg(LOG_ERR, "Could not create link %s", fpath);
        return -1;
    }

    return 0;
}

int handleWrite(const char *path, int *fd, WriteOperation *writeop) {
    // if haven't open the file in previous calls yet, do it now
    if (*fd == -1) {
        *fd = open(path, O_WRONLY);
        if (*fd == -1) {
            errnoMsg(LOG_ERR, "Could not open file %s", path);
            return -1;
        }
    }

    // TODO
    if (writeop->has_data != 1)
        return -1;

    if (pwrite(*fd, writeop->data.data, writeop->size, writeop->offset)
            != writeop->size) {
        errnoMsg(LOG_ERR, "pwrite has failed on file %s", path);
        return -1;
    }

    return 0;
}

int handleUnlink(const char *fpath, int *fd, UnlinkOperation *unlinkop) {
    // close file, if it is open
    if (*fd != -1) {
        if (close(*fd) == -1)
            errnoMsg(LOG_ERR, "Could not close file %s", fpath);
        else
            *fd = -1;
    }
    if (unlink(fpath) == -1) {
        errnoMsg(LOG_ERR, "Could not unlink file %s", fpath);
        return -1;
    }

    return 0;
}

int handleRmdir(const char *fpath, RmdirOperation *rmdirop) {
    if (rmdir(fpath) == -1) {
        errnoMsg(LOG_ERR, "Could not unlink file %s", fpath);
        return -1;
    }

    return 0;
}

int handleTruncate(const char *fpath, TruncateOperation *truncateop) {
    if (truncate(fpath, truncateop->newsize) == -1) {
        errnoMsg(LOG_ERR, "Could not truncate file %s", fpath);
        return -1;
    }

    return 0;
}

int handleChmod(const char *fpath, ChmodOperation *chmodop) {
    if (chmod(fpath, chmodop->mode) == -1) {
        errnoMsg(LOG_ERR, "Could not chmod file %s", fpath);
        return -1;
    }

    return 0;
}

int handleChown(const char *fpath, ChownOperation *chownop) {
    if (chown(fpath, chownop->uid, chownop->gid) == -1) {
        errnoMsg(LOG_ERR, "Could not chown file %s", fpath);
        return -1;
    }

    return 0;
}

int handleRename(const char *fpath, RenameOperation *renameop) {
    char foldpath[PATH_MAX];
    char fnewpath[PATH_MAX];

    getAbsolutePath(foldpath, config.rootdir, renameop->oldpath);
    getAbsolutePath(fnewpath, config.rootdir, renameop->newpath);

    if (rename(foldpath, fnewpath) == -1) {
        errnoMsg(LOG_ERR, "Could not rename file %s", fpath);
        return -1;
    }

    return 0;
}
