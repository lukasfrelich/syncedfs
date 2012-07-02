/* 
 * File:   client.c
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#define _GNU_SOURCE

#include "config.h"
#include "client.h"
#include "../syncedfs-common/logging_functions.h"
#include "../syncedfs-common/message_functions.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/syncid.h"
#include "../syncedfs-common/lib/create_pid_file.h"
#include "../syncedfs-common/lib/uthash.h"
#include "snapshot.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

fileop_t *files = NULL; // Hash map
fileop_t *inodefiles = NULL; // unlinked files identified by i-node

int synchronize(void) {
    int pidfd;
    char pidpath[PATH_MAX];
    int logfd;
    char logpath[PATH_MAX];
    int newsync = 0;

    // make sure, that there isn't another client process for this resource
    // already running
    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/client/%s.pid",
            config.resource);
    pidfd = createPidFile(config.ident, pidpath, 0);
    if (pidfd == -1) {
        errMsg(LOG_ERR, "Could not create pid file %s, Exiting sync.", pidpath);
        return -1;
    }

    // if sync log does not exist, switch log
    snprintf(logpath, PATH_MAX, "%s/%s.sync", config.logdir, config.resource);
    errMsg(LOG_INFO, "logpath: %s", logpath);
    logfd = open(logpath, O_RDONLY);
    if (logfd == -1) {
        if (switchLog() != 0) {
            errMsg(LOG_ERR, "Could not switch log file. Stopping.");
            return -1;
        }

        logfd = open(logpath, O_RDONLY);
        if (logfd == -1) {
            errnoMsg(LOG_ERR, "Could not open log file %s", logpath);
            return -1;
        }
        newsync = 1;
    }


    // sync-id
    char id[SYNCID_MAX];
    char idpath[PATH_MAX];
    snprintf(idpath, PATH_MAX, "%s/%s.id", config.logdir, config.resource);

    // second part of the condition covers weird states after crash etc.
    if (newsync == 1 || readSyncId(id, idpath, SYNCID_MAX) != 0) {
        if (generateSyncId(id, SYNCID_MAX) != 0) {
            errMsg(LOG_ERR, "Could not get sync-id. Stopping.");
            return -1;
        }
        if (writeSyncId(id, idpath) != 0) {
            errMsg(LOG_ERR, "Could not write sync-id. Stopping.");
            return -1;
        }
    }

    // create snapshot
    if (newsync || !fileExists(config.snapshot)) {
        if (createSnapshot(config.rootdir, config.snapshot, 1) == -1) {
            errMsg(LOG_ERR, "Could not create snapshot of %s to %s Stopping.",
                    config.rootdir, config.snapshot);
            return -1;
        }
    }

    // load log
    if (loadLog(logfd) != 0) {
        errMsg(LOG_ERR, "Could not load log %s", logpath);
        return -1;
    }

    // transfer changes
    if (transfer(config.host, config.port) == -1) {
        errMsg(LOG_ERR, "Error in transfer. Exiting sync.");

        // in case of failure only delete pid file
        if (deletePidFile(pidfd, pidpath) == -1)
            errExit(LOG_ERR, "Deleting PID file '%s'", pidpath);

        return -1;
    }

    // delete snapshot
    if (deleteSnapshot(config.snapshot) == -1)
        errMsg(LOG_ERR, "Could not delete snapshot %s", config.snapshot);

    // delete syncid
    if (unlink(idpath) == -1)
        errExit(LOG_ERR, "Deleting sync-id file '%s'", idpath);

    // delete sync log
    close(logfd);
    if (unlink(logpath) == -1)
        errExit(LOG_ERR, "Deleting log file '%s'", logpath);

    // delete pid file
    if (deletePidFile(pidfd, pidpath) == -1)
        errExit(LOG_ERR, "Deleting PID file '%s'", pidpath);

    return 0;
}

//------------------------------------------------------------------------------
// Log processing
//------------------------------------------------------------------------------

int loadLog(int logfd) {
    int alocatedmem = 0;
    uint8_t *buf = NULL;
    uint32_t msglen;
    FileOperation *fileop;

    while (read(logfd, &msglen, sizeof (uint32_t)) == sizeof (uint32_t)) {
        msglen = ntohl(msglen);

        if (alocatedmem < msglen) {
            free(buf);

            buf = malloc(msglen);
            if (buf == NULL) {
                errMsg(LOG_ERR, "Could not allocate memory for message.");
                return -1;
            }
            alocatedmem = msglen;
        }
        if (read(logfd, buf, msglen) != msglen) {
            errnoMsg(LOG_ERR, "Could not read message from log.");
            free(buf);
            return -1;
        }

        // memory leak, need to store allocated addresses and free them later
        //file_operation__free_unpacked(fileop, NULL);
        fileop = file_operation__unpack(NULL, msglen, buf);

        if (addOperation(fileop->relative_path, fileop->op) != 0) {
            errMsg(LOG_ERR, "Could add operation to file operations. "
                    "Exiting sync.");
            free(buf);
            return -1;
        }
    }

    free(buf);
    return 0;
}

void printLog(void) {
    HASH_SORT(files, sortByOrder);
    fileop_t *f;
    GenericOperation *op;

    for (f = files; f != NULL; f = (fileop_t*) (f->hh.next)) {
        printf("%s\n", f->filename);

        for (int i = 0; i < f->nelem; i++) {
            op = *(f->operations + i);

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
        }
    }
}

int addOperation(char *relpath, GenericOperation *genop) {
    static int fileorder = 0;
    static int id = 0;
    fileop_t *f;

    // TODO: has ID
    genop->has_id = 1;
    genop->id = id++;
    HASH_FIND_STR(files, relpath, f);

    // handle link
    if (genop->type == GENERIC_OPERATION__TYPE__LINK) {
        // we want to add the operation to newpath list
        // so just get newpath entry and let the generic handler do the rest
        HASH_FIND_STR(files, genop->link_op->newpath, f);
        // falls through to the last section to add current operation
    }

    // handle unlink/rmdir
    // remove all operations and add unlink/rmdir
    // there might be latter a problem: (file) test -> delete test -> create
    // folder test -> delete test, would leave rmdir operation, but we should
    // do in fact an unlink
    if (genop->type == GENERIC_OPERATION__TYPE__UNLINK ||
            genop->type == GENERIC_OPERATION__TYPE__RMDIR) {

        // if this was not the last link (there cannot be links to a directory)
        if (genop->type == GENERIC_OPERATION__TYPE__UNLINK &&
                !genop->unlink_op->last_link) {
            // operations are valid and we must try to merge them with other
            // link of the file, links are identified by inode, but inode
            // number are only known for deleted/unlinked files
            // so if we can't merge it now, we must keep the operations to merge
            // them later
            fileop_t *finode;
            // search in inodefiles hash table
            HASH_FIND_INT(inodefiles, &genop->unlink_op->inode, finode);
            if (finode != NULL) {
                if (mergeOperations(f, finode) != 0)
                    return -1;

                // remove entry from the hash table
                free(finode->operations);
                HASH_DEL(inodefiles, finode);
                free(finode);
            }
        } else {
            // if this is was the last link of a file, we shall get rid of all
            // the operations, with a possible exception for the unlink itself
            // (if the file was created in older epoch)

            // first, if this is an unlink operation, are there any operations
            // for a link of this file, we have deleted? (if there weren't other
            // links, there is for sure nothing, that's especially
            // the case for directories)
            if (genop->type == GENERIC_OPERATION__TYPE__UNLINK) {
                fileop_t *finode;
                // search in inodefiles hash table
                HASH_FIND_INT(inodefiles, &genop->unlink_op->inode, finode);
                if (finode != NULL) {
                    // discard the operations, remove entry from the hash table
                    free(finode->operations);
                    HASH_DEL(inodefiles, finode);
                    free(finode);
                }
            }
            // get rid of all operations
            if (f != NULL) {
                // if the first operation is an unlink or rmdir, we should keep
                // it and don't add current unlink/delete
                if (((GenericOperation *) f->operations)->type ==
                        GENERIC_OPERATION__TYPE__UNLINK ||
                        ((GenericOperation *) f->operations)->type ==
                        GENERIC_OPERATION__TYPE__RMDIR) {
                    // shrink the array
                    int alocsize = VECTOR_INIT_CAPACITY *
                            sizeof (GenericOperation*);
                    f->operations = realloc(f->operations, alocsize);
                    if (f->operations == NULL) {
                        errMsg(LOG_ERR, "Could not allocate memory for "
                                "file operations. Requested memory: %d",
                                alocsize);
                        return -1;
                    }
                    f->capacity = alocsize;
                    f->nelem = 1;
                    f->order = -1;

                    return 0;
                }
                // otherwise simply remove this entry from the hash table and
                // add current operation in the next step
                free(f->operations);
                HASH_DEL(files, f);
                free(f);
            }

            // we only add current unlink/rmdir operation if the file was not
            // created only in this epoch
            if (!f->created) {
                // at this point entry for current file name in the has table
                // has either never existed, or we deleted it, now add it with
                // fileorder = -1
                if (addFileToTable(relpath, f) != 0)
                    return -1;

                *(f->operations) = genop;
                f->nelem = 1;
                f->order = -1;
            }

            return 0;
        }
    }



    //old--------------------------------------------------------------

    // handle rename
    // remove all operations for newpath, add this rename operation to newpath,
    // add all operations from oldpath to newpath
    if (genop->type == GENERIC_OPERATION__TYPE__RENAME) {
        fileop_t *newf;
        HASH_FIND_STR(files, genop->rename_op->newpath, newf);

        // make sure, that newfile exists in the hash table and operations
        // is not allocated
        if (newf == NULL) {
            newf = (fileop_t*) malloc(sizeof (fileop_t));
            if (newf == NULL) {
                errMsg(LOG_ERR, "Could not allocate memory for file operations.");
                return -1;
            }

            (void) strcpy(newf->filename, genop->rename_op->newpath);
            newf->order = fileorder++;

            HASH_ADD_STR(files, filename, newf);
        } else {
            free(newf->operations);
        }

        // allocate memory for newpath ops
        int newcap = (f == NULL) ? VECTOR_INIT_CAPACITY : f->capacity + 1;
        newf->capacity = newcap;
        newf->operations = malloc((newcap) * sizeof (GenericOperation*));
        if (newf->operations == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for file operations.");
            return -1;
        }

        //  copy ops from oldpath to newpath
        if (f != NULL)
            memcpy(newf->operations + 1, f->operations,
                f->nelem * sizeof (GenericOperation*));

        // add rename op and the beginning of newpath ops
        *(newf->operations) = genop;

        // remove oldpath op
        if (f != NULL) {
            free(f->operations);
            HASH_DEL(files, f);
            free(f);
        }

        return 0;
    }

    //old--------------------------------------------------------------

    // handle all other cases
    // key not found
    if (f == NULL) {
        if (addFileToTable(relpath, f) != 0)
            return -1;

        f->order = fileorder++;

        // set created flag
        switch (genop->type) {
            case GENERIC_OPERATION__TYPE__CREATE:
            case GENERIC_OPERATION__TYPE__MKNOD:
            case GENERIC_OPERATION__TYPE__MKDIR:
            case GENERIC_OPERATION__TYPE__SYMLINK:
            case GENERIC_OPERATION__TYPE__LINK:
                //case GENERIC_OPERATION__TYPE__RENAME:
                f->created = 1;
                break;
            default:
                break;
        }
    }

    // extend vector of operations if needed
    //if (f->nelem == (f->capacity - 1)) {
    if (f->nelem == f->capacity) {
        f->capacity = f->capacity * 2;
        f->operations = realloc(f->operations,
                f->capacity * sizeof (GenericOperation*));

        if (f->operations == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for file operations. "
                    "Requested memory: %d", f->capacity *
                    sizeof (GenericOperation*));
            return -1;
        }
    }

    *(f->operations + f->nelem) = genop;
    f->nelem++;

    // if this file was previously deleted, assign new fileorder
    if (f->order == -1)
        f->order = fileorder++;

    return 0;
}

int mergeOperations(fileop_t *dest, fileop_t *src) {
    if (dest == NULL || src == NULL)
        return 0;

    int newcap = dest->nelem + src->nelem;

    dest->operations = realloc(dest->operations,
            newcap * sizeof (GenericOperation*));

    if (dest->operations == NULL) {
        errMsg(LOG_ERR, "Could not allocate memory for file operations. "
                "Requested memory: %d", newcap *
                sizeof (GenericOperation*));
        return -1;
    }

    memcpy(dest->operations + dest->nelem, src->operations,
            src->nelem * sizeof (GenericOperation*));

    dest->capacity = newcap;
    dest->nelem = newcap;

    return 0;
}

int addFileToTable(char *key, fileop_t *newentry) {
    fileop_t *f;

    f = (fileop_t*) malloc(sizeof (fileop_t));
    if (f == NULL) {
        errMsg(LOG_ERR, "Could not allocate memory for file operations.");
        return -1;
    }

    (void) strcpy(f->filename, key);
    f->capacity = VECTOR_INIT_CAPACITY;
    f->created = 0;
    f->operations = malloc(VECTOR_INIT_CAPACITY * sizeof (GenericOperation*));

    if (f->operations == NULL) {
        errMsg(LOG_ERR, "Could not allocate memory for file operations.");
        return -1;
    }

    HASH_ADD_STR(files, filename, f);
    newentry = f;

    return 0;
}

void optimizeOperations(fileop_t *fileop) {
    // TODO
    // deal with files we have in inodefiles hash table
    // sort operations (write should go at the end, create, link etc. at the
    // beginning...)
    // remove duplicates
    return;
}

//------------------------------------------------------------------------------
// Transfer
//------------------------------------------------------------------------------

int transfer(char *host, char *port) {
    int sfd; // server socket
    fileop_t *fileop; // contains all operations for a file

    sfd = inetConnect(host, port, SOCK_STREAM);
    if (sfd == -1) {
        errnoMsg(LOG_ERR, "Unable to connect to the server.");
        return -1;
    }

    // initiate sync (sync-id, resource, number of files)
    errMsg(LOG_INFO, "Number of files: %d", HASH_COUNT(files));
    switch (initiateSync(sfd, HASH_COUNT(files))) {
        case -1: // error
            return -1;
        case -2: // there's nothing to synchronize
            return 0;
        default:
            break;
    }

    // iterate over files in correct order
    HASH_SORT(files, sortByOrder);
    for (fileop = files; fileop != NULL;
            fileop = (fileop_t*) (fileop->hh.next)) {

        if (transferFile(sfd, fileop) == -1) {
            errMsg(LOG_ERR, "Transfer of file %s has failed.",
                    fileop->filename);
            return -1;
        }
    }

    // wait for ACK
    SyncFinish *conf;
    conf = (SyncFinish *) recvMessage(sfd, SyncFinishType, NULL);
    if (conf == NULL) {
        errMsg(LOG_ERR, "Could not get transfer confirmation.");
        return -1;
    }
    return 0;
}

int initiateSync(int sfd, int numfiles) {
    SyncInit syncinit = SYNC_INIT__INIT;
    SyncInitResponse *response;
    char syncid[SYNCID_MAX];
    char idpath[PATH_MAX];

    snprintf(idpath, PATH_MAX, "%s/%s.id", config.logdir, config.resource);
    if (readSyncId(syncid, idpath, SYNCID_MAX) == -1) {
        errMsg(LOG_ERR, "Could not read sync-id %s.");
        return -1;
    }

    syncinit.number_files = numfiles;
    syncinit.sync_id = syncid;
    syncinit.resource = config.resource;

    if (sendMessage(sfd, SyncInitType, &syncinit) != 0) {
        return -1;
    }

    response = (SyncInitResponse *)
            recvMessage(sfd, SyncInitResponseType, NULL);
    if (response == NULL) {
        errMsg(LOG_ERR, "Could not read initialization response from server.");
        return -1;
    }

    if (response->continue_ == 0) {
        if (response->has_already_synced && response->already_synced) {
            return -2; // special case, which has to be handled in transfer()
        } else {
            if (strlen(response->error_message) > 0) {
                errMsg(LOG_ERR, "Could not initiate sync. "
                        "Response from server: %s", response->error_message);
            }
            return -1;
        }
    }

    return 0;
}

int transferFile(int sfd, fileop_t *fileop) {
    GenericOperation *genop; // current operation
    GenericOperation **opstart; //first operation to be send in the message
    int nops = 0; // how many GenericOperations are in the message
    // used for dynamically allocated data (to lower number of malloc calls)
    dyndata_t ddata = {0};

    ddata.buf = malloc(MESSAGE_MAX * sizeof (uint8_t));
    if (ddata.buf == NULL) {
        errMsg(LOG_ERR, "Could not allocate memory for write data.");
        return -1;
    }
    ddata.size = MESSAGE_MAX;
    ddata.offset = 0;
    opstart = fileop->operations;

    optimizeOperations(fileop);

    for (int i = 0; i < fileop->nelem; i++) {
        // Fetch next operation
        genop = *(fileop->operations + i);

        // load data for write operation
        if (genop->type == GENERIC_OPERATION__TYPE__WRITE) {
            // prevent buffer overflow
            if (genop->write_op->size + ddata.offset > ddata.size) {
                if (transferChunk(sfd, fileop, opstart, nops, 0) == -1) {
                    errMsg(LOG_ERR, "Transfer of a chunk has failed.");
                    return -1;
                }

                // reset counters
                opstart = fileop->operations + i; // only + i
                nops = 0;
                ddata.offset = 0;
            }

            if (loadWriteData(fileop->filename, genop->write_op, &ddata) != 0)
                // TODO: when optimizeOperations is done change to LOG_ERR
                // and return -1;
                errMsg(LOG_WARNING, "Could not load required data.");
            // if this was the last operation: close file, which might have been
            // opened
            if (i == fileop->nelem - 1)
                loadWriteData(NULL, NULL, NULL);
        }
        nops++;

        if (ddata.offset >= MESSAGE_MAX) {
            // if we have just added last operation, set last_chunk flag
            if (transferChunk(sfd, fileop, opstart, nops,
                    (i == fileop->nelem - 1) ? 1 : 0 /*last_chunk*/) == -1) {
                errMsg(LOG_ERR, "Transfer of a chunk has failed.");
                return -1;
            }

            // reset counters
            opstart = fileop->operations + i + 1; // i + 1
            nops = 0;
            ddata.offset = 0;
            //ddata.size = 0;
        }
    }
    // Transfer last chunk (if there is still some data left)
    if (nops > 0) {
        if (transferChunk(sfd, fileop, opstart, nops, 1) == -1) {
            errMsg(LOG_ERR, "Transfer of a chunk has failed.");
            return -1;
        }
    }

    free(ddata.buf);

    return 0;
}

int transferChunk(int sfd, fileop_t *fileop, GenericOperation **opstart,
        int nops, int lastchunk) {

    FileChunk fchunk = FILE_CHUNK__INIT;

    fchunk.relative_path = fileop->filename;
    fchunk.last_chunk = lastchunk;
    fchunk.n_ops = nops;
    fchunk.ops = opstart;

    if (sendMessage(sfd, FileChunkType, &fchunk) != 0)
        return -1;

    // wait for ACK?

    //freePackedMessage(buf);
    return 0;
}

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------

int loadWriteData(char *relpath, WriteOperation *writeop, dyndata_t *dyndata) {
    static char storedpath[PATH_MAX];
    char fpath[PATH_MAX];
    static int fd = -1;

    // a mean for closing files
    if (relpath == NULL && fd != -1) {
        if (close(fd) == -1)
            errnoMsg(LOG_WARNING, "Could not close file %s", relpath);
        return 0;
    }

    // if we are loading data the first time from this file, open it
    // last file might remain open
    if (strcmp(relpath, storedpath) != 0) {
        (void) strcpy(storedpath, relpath);

        if (fd != -1) {
            if (close(fd) == -1)
                errnoMsg(LOG_WARNING, "Could not close file %s", relpath);
        }

        getAbsolutePath(fpath, config.snapshot, relpath);
        fd = open(fpath, O_RDONLY);
        if (fd == -1) {
            errnoMsg(LOG_ERR, "Could not open source file %s", relpath);
            return -1;
        }
    }

    // realloc, if necessary (i.e. only if one operation is larger than buffer)
    // other buffer overflow cases are being taken care of in transferFile()
    if (writeop->size > dyndata->size) {
        int nsize = dyndata->size + writeop->size;

        dyndata->buf = realloc(dyndata->buf, nsize);
        if (dyndata->buf == NULL) {
            errMsg(LOG_ERR, "Could not allocate memory for write data.");
            return -1;
        }
        dyndata->size = nsize;
    }

    if (pread(fd, dyndata->buf + dyndata->offset, writeop->size,
            writeop->offset) != writeop->size) {
        errnoMsg(LOG_ERR, "Could not read data from file %s", relpath);
        return -1;
    }

    ProtobufCBinaryData data = {writeop->size, dyndata->buf + dyndata->offset};
    writeop->has_data = 1;
    writeop->data = data;

    dyndata->offset += writeop->size;
    return 0;
}

//------------------------------------------------------------------------------
// Auxiliary functions
//------------------------------------------------------------------------------

int sortByOrder(fileop_t *a, fileop_t *b) {
    if (a->order == b->order)
        return 0;
    return (a->order < b->order) ? -1 : 1;
}

int switchLog(void) {
    FILE *pidfile;
    long pid;
    char pidpath[PATH_MAX]; // syncedfs pid
    char buf[RESOURCE_MAX];
    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/fs/%s.pid", config.resource);

    pidfile = fopen(pidpath, "r");
    if (pidfile == NULL) {
        errnoMsg(LOG_ERR, "Error opening syncedfs PID file: %s", pidpath);
        return -1;
    }

    // read PID
    if (fgets(buf, RESOURCE_MAX, pidfile) == NULL) {
        errnoMsg(LOG_ERR, "Error reading syncedfs PID file: %s", pidpath);
        return -1;
    }

    if (fclose(pidfile) == EOF) {
        errnoMsg(LOG_ERR, "Error closing syncedfs PID file: %s", pidpath);
        return -1;
    }

    char *endptr = NULL;
    errno = 0;
    pid = strtol(buf, &endptr, 10);
    if (errno != 0 || pid == 0) {
        errnoMsg(LOG_ERR, "Error reading syncedfs PID file: %s", pidpath);
        return -1;
    }

    // verify PID
    if (kill(pid, 0) == -1) {
        errnoMsg(LOG_ERR, "Could not verify syncedfs PID.");
        return -1;
    }

    // block SIGUSR1 signal (so a response is not delivered before we wait
    // for it)
    sigset_t origmask, blockmask;

    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &blockmask, &origmask) == -1) {
        errMsg(LOG_ERR, "Could not block SIGUSR1 signal.");
        return -1;
    }

    // send signal
    if (kill(pid, SIGUSR1) == -1) {
        errnoMsg(LOG_ERR, "Could not send SIGUSR1 signal to %ld", pid);
        return -1;
    }

    // wait for response
    int sig;
    siginfo_t si;
    sigset_t allsigs;
    struct timespec tspec = {0};

    sigemptyset(&allsigs);
    sigaddset(&allsigs, SIGUSR1);
    tspec.tv_sec = 10;

    sig = sigtimedwait(&allsigs, &si, &tspec);
    if (sig != SIGUSR1) {
        errnoMsg(LOG_ERR, "sigtimedwait failed.");
        return -1;
    }

    if (si.si_pid != pid) {
        errMsg(LOG_ERR, "Received SIGUSR1 from wrong process");
        return -1;
    }

    // restore signal mask
    if (sigprocmask(SIG_SETMASK, &origmask, NULL) == -1) {
        errMsg(LOG_WARNING, "Could not restore signal mask.");
    }
    return 0;
}

int generateSyncId(char *id, int maxlength) {
    time_t t;
    struct tm *tm;
    char stime[SYNCID_MAX] = {0};
    int rnum;

    if (maxlength > SYNCID_MAX)
        maxlength = SYNCID_MAX;

    t = time(NULL);
    tm = gmtime(&t);
    strftime(stime, SYNCID_MAX, "%y-%m-%d--%H-%M-%S", tm);

    srand(time(NULL));
    rnum = rand();

    snprintf(id, SYNCID_MAX, "%s_%s_%d", config.resource, stime, rnum);
    return 0;
}