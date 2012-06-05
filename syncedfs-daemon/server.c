/* 
 * File:   server.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#include "config.h"
#include "server.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/lib/tlpi_hdr.h"

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
        int fd = -1;

        // we are guaranteed to get at least one chunk for each file
        chunk = (FileChunk *) getMessageFromSocket(cfd, FileChunkType, &nbytes);
        if (chunk == NULL) {
            errExit("Couldn't read message from client.\n");
        }

        for (;;) {
            for (int k = 0; k < chunk->n_ops; k++) {
                handleGenericOperation(&fd, chunk->relative_path, chunk->ops[k]);
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

        if (fd != -1) {
            if (close(fd) == -1)
                errExit("Couldn't close file.\n");
        }

    }
    if (close(cfd) == -1)
        errMsg("close socket");
    printf("Transfered bytes: %lld \n", nbytes);
}

int createSnapshot(void) {
    // btrfs subvolume snapshot -r machine1 machine1-snapshot
    // fork + exec
    return 0;
}

void printOp(const char *relpath, const char *fpath, GenericOperation *op) {
    static int i = 0;
    printf("relpath: %s fpath: %s", relpath, fpath);

    switch (op->type) {
        case GENERIC_OPERATION__OPERATION_TYPE__CREATE:
            printf("Operation %d, type: create, mode: %d\n",
                    i, (int) op->create_op->mode);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__MKNOD:
            printf("Operation %d, type: mknod, mode: %d, dev: %ld\n",
                    i, (int) op->mknod_op->mode,
                    (long int) op->mknod_op->dev);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__MKDIR:
            printf("Operation %d, type: mkdir, mode: %d\n",
                    i, (int) op->mkdir_op->mode);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__SYMLINK:
            printf("Operation %d, type: symlink, target: %s\n",
                    i, op->symlink_op->target);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__LINK:
            printf("Operation %d, type: link, target: %s\n",
                    i, op->link_op->target);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__WRITE:
            printf("Operation %d, type: write, offset: %ld, size: %d\n",
                    i, (long int) op->write_op->offset,
                    (int) op->write_op->size);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__UNLINK:
            printf("Operation %d, type: unlink\n", i);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__RMDIR:
            printf("Operation %d, type: rmdir\n", i);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE:
            printf("Operation %d, type: truncate, newsize: %d\n",
                    i, (int) op->truncate_op->newsize);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__CHMOD:
            printf("Operation %d, type: chmod, mode: %d\n",
                    i, (int) op->chmod_op->mode);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__CHOWN:
            printf("Operation %d, type: chown, uid: %d, gid: %d\n",
                    i, (int) op->chown_op->uid,
                    (int) op->chown_op->gid);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__RENAME:
            printf("Operation %d, type: rename, newpath: %s\n",
                    i, op->rename_op->newpath);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__SETXATTR:
        case GENERIC_OPERATION__OPERATION_TYPE__REMOVEXATTR:
            break;
    }
    
    i++;
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

int handleGenericOperation(int *fd, const char *relpath,
        GenericOperation *genop) {

    int ret;
    char fpath[PATH_MAX];

    getAbsolutePath(fpath, config.rootdir, relpath);
    //printOp(relpath, fpath, genop);

    switch (genop->type) {
        case GENERIC_OPERATION__OPERATION_TYPE__CREATE:
            handleCreate(fpath, fd, genop->create_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__MKNOD:
            handleMknod(fpath, genop->mknod_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__MKDIR:
            handleMkdir(fpath, genop->mkdir_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__SYMLINK:
            handleSymlink(fpath, genop->symlink_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__LINK:
            handleLink(fpath, genop->link_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__WRITE:
            ret = handleWrite(fpath, fd, genop->write_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__UNLINK:
            ret = handleUnlink(fpath, fd, genop->unlink_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__RMDIR:
            ret = handleRmdir(fpath, genop->rmdir_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE:
            ret = handleTruncate(fpath, genop->truncate_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__CHMOD:
            ret = handleChmod(fpath, genop->chmod_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__CHOWN:
            ret = handleChown(fpath, genop->chown_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__RENAME:
            ret = handleRename(fpath, genop->rename_op);
            break;
        case GENERIC_OPERATION__OPERATION_TYPE__SETXATTR:
        case GENERIC_OPERATION__OPERATION_TYPE__REMOVEXATTR:
            break;
    }

    return ret;
}

int handleCreate(const char *fpath, int *fd, CreateOperation *createop) {
    *fd = creat(fpath, createop->mode);

    if (*fd == -1) {
        errMsg("Could not create file %s", fpath);
        return -1;
    }

    return 0;
}

int handleMknod(const char *fpath, MknodOperation *mknodop) {
    if (S_ISREG(mknodop->mode)) {
        int tfd;
        tfd = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mknodop->mode);
        if (tfd == -1) {
            errMsg("Could not create file %s", fpath);
            return -1;
        } else {
            if (close(tfd) == -1) {
                errMsg("Could not close file %s", fpath);
                return -1;
            }
        }
    } else {
        if (S_ISFIFO(mknodop->mode)) {
            if (mkfifo(fpath, mknodop->mode) == -1) {
                errMsg("Could not create named pipe %s", fpath);
                return -1;
            }
        } else {
            if (mknod(fpath, mknodop->mode, mknodop->dev) == -1) {
                errMsg("Could not create node %s", fpath);
                return -1;
            }
        }
    }

    return 0;
}

int handleMkdir(const char *fpath, MkdirOperation *mkdirop) {
    if (mkdir(fpath, mkdirop->mode) == -1) {
        errMsg("Could not create directory %s", fpath);
        return -1;
    }

    return 0;
}

int handleSymlink(const char *fpath, SymlinkOperation *symlinkop) {
    //char ftarget[PATH_MAX];
    
    //getAbsolutePath(ftarget, config.rootdir, symlinkop->target);
    
    if (symlink(symlinkop->target, fpath) == -1) {
        errMsg("Could not create symlink %s", fpath);
        return -1;
    }

    return 0;
}

int handleLink(const char *fpath, LinkOperation *linkop) {
    char ftarget[PATH_MAX];
    
    getAbsolutePath(ftarget, config.rootdir, linkop->target);
    
    if (link(ftarget, fpath) == -1) {
        errMsg("Could not create link %s", fpath);
        return -1;
    }

    return 0;
}

int handleWrite(const char *path, int *fd, WriteOperation *writeop) {
    // if haven't open the file in previous calls yet, do it now
    if (*fd == -1) {
        *fd = open(path, O_WRONLY);
        if (*fd == -1) {
            errMsg("Could not open file %s", path);
            return -1;
        }
    }

    if (writeop->has_data != 1)
        return -1;

    if (pwrite(*fd, writeop->data.data, writeop->size, writeop->offset)
            != writeop->size)
        return -1;

    return 0;
}

int handleUnlink(const char *fpath, int *fd, UnlinkOperation *unlinkop) {
    // close file, if it is open
    if (*fd != -1) {
        if (close(*fd) == -1)
            errMsg("Could not close file %s", fpath);
        else
            *fd = -1;
    }
    if (unlink(fpath) == -1) {
        errMsg("Could not unlink file %s", fpath);
        return -1;
    }

    return 0;
}

int handleRmdir(const char *fpath, RmdirOperation *rmdirop) {
    if (rmdir(fpath) == -1) {
        errMsg("Could not unlink file %s", fpath);
        return -1;
    }

    return 0;
}

int handleTruncate(const char *fpath, TruncateOperation *truncateop) {
    if (truncate(fpath, truncateop->newsize) == -1) {
        errMsg("Could not truncate file %s", fpath);
        return -1;
    }

    return 0;
}

int handleChmod(const char *fpath, ChmodOperation *chmodop) {
    if (chmod(fpath, chmodop->mode) == -1) {
        errMsg("Could not chmod file %s", fpath);
        return -1;
    }

    return 0;
}

int handleChown(const char *fpath, ChownOperation *chownop) {
    if (chown(fpath, chownop->uid, chownop->gid) == -1) {
        errMsg("Could not chown file %s", fpath);
        return -1;
    }

    return 0;
}

int handleRename(const char *fpath, RenameOperation *renameop) {
    char fnewpath[PATH_MAX];
    
    getAbsolutePath(fnewpath, config.rootdir, renameop->newpath);
    
    if (rename(fpath, fnewpath) == -1) {
        errMsg("Could not rename file %s", fpath);
        return -1;
    }

    return 0;
}
