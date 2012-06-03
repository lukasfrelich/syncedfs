/* 
 * File:   syncedfs.c
 * Author: lfr
 *
 * Created on April 20, 2012, 4:13 PM
 * 
 * Based on Big Brother File System by Joseph J. Pfeiffer, Jr., Ph.D.
 */

#include "config.h"
#include "log.h"
#include "sighandlers.h"
#include "../syncedfs-common/lib/error_functions.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/xattr.h>


// Report errors to logfile and give -errno to caller

static int sfs_error(char *str) {
    int ret = -errno;

    //log_msg("    ERROR %s: %s\n", str, strerror(errno));

    return ret;
}

// Check whether the given user is permitted to perform the given operation on the given 

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, we need to
//  have the rootdir.
// syncedfs specific version for better performance

static inline void sfs_fullpath(char fpath[PATH_MAX], const char *path) {
    strcpy(fpath, config.rootdir);

    // if relative path does not begin with '/'
    if (path != NULL && *path != '/') {
        fpath[config.rootdir_len] = '/';
        strncpy(fpath + config.rootdir_len + 1, path,
                PATH_MAX - config.rootdir_len - 1);
    } else {
        strncpy(fpath + config.rootdir_len, path,
                PATH_MAX - config.rootdir_len);
    }
    fpath[PATH_MAX - 1] = '\0'; // fpath might not have been terminated
}

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = lstat(fpath, statbuf);
    if (retstat != 0)
        retstat = sfs_error("sfs_getattr lstat");

    return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to sfs_readlink()
// sfs_readlink() code by Bernardo F Costa (thanks!)

int sfs_readlink(const char *path, char *link, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = readlink(fpath, link, size - 1);
    if (retstat < 0)
        retstat = sfs_error("sfs_readlink readlink");
    else {
        link[retstat] = '\0';
        retstat = 0;
    }

    return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?

int sfs_mknod(const char *path, mode_t mode, dev_t dev) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    // On Linux this could just be 'mknod(path, mode, rdev)' but this
    // is more portable
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (retstat < 0) {
            retstat = sfs_error("sfs_mknod open");
        } else {
            logMknod(path, mode, dev); // log only successful attempts

            retstat = close(retstat);
            if (retstat < 0)
                retstat = sfs_error("sfs_mknod close");
        }
    } else {
        if (S_ISFIFO(mode)) {
            retstat = mkfifo(fpath, mode);
            if (retstat < 0)
                retstat = sfs_error("sfs_mknod mkfifo");
            else
                logMknod(path, mode, dev); // log only successful attempts
        } else {
            retstat = mknod(fpath, mode, dev);
            if (retstat < 0)
                retstat = sfs_error("sfs_mknod mknod");
            else
                logMknod(path, mode, dev); // log only successful attempts
        }
    }

    return retstat;
}

/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = mkdir(fpath, mode);

    if (retstat != -1) // log only successful attempts
        logMkdir(path, mode);

    if (retstat < 0)
        retstat = sfs_error("sfs_mkdir mkdir");

    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = unlink(fpath);

    if (retstat != -1) // log only successful attempts
        logUnlink(path);

    if (retstat < 0)
        retstat = sfs_error("sfs_unlink unlink");

    return retstat;
}

/** Remove a directory */
int sfs_rmdir(const char *path) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = rmdir(fpath);

    if (retstat != -1) // log only successful attempts
        logRmdir(path);

    if (retstat < 0)
        retstat = sfs_error("sfs_rmdir rmdir");

    return retstat;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.

int sfs_symlink(const char *path, const char *link) {
    int retstat = 0;
    char flink[PATH_MAX];

    sfs_fullpath(flink, link);

    retstat = symlink(path, flink);

    if (retstat != -1) // log only successful attempts
        logSymlink(link, path);

    if (retstat < 0)
        retstat = sfs_error("sfs_symlink symlink");

    return retstat;
}

/** Rename a file */
// both path and newpath are fs-relative

int sfs_rename(const char *path, const char *newpath) {
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];

    sfs_fullpath(fpath, path);
    sfs_fullpath(fnewpath, newpath);

    retstat = rename(fpath, fnewpath);

    if (retstat != -1) // log only successful attempts
        logRename(path, newpath);

    if (retstat < 0)
        retstat = sfs_error("sfs_rename rename");

    return retstat;
}

/** Create a hard link to a file */
int sfs_link(const char *path, const char *newpath) {
    int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];

    sfs_fullpath(fpath, path);
    sfs_fullpath(fnewpath, newpath);

    retstat = link(fpath, fnewpath);

    if (retstat != -1) // log only successful attempts
        logLink(path, newpath);

    if (retstat < 0)
        retstat = sfs_error("sfs_link link");

    return retstat;
}

/** Change the permission bits of a file */
int sfs_chmod(const char *path, mode_t mode) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = chmod(fpath, mode);

    if (retstat != -1) // log only successful attempts
        logChmod(path, mode);

    if (retstat < 0)
        retstat = sfs_error("sfs_chmod chmod");

    return retstat;
}

/** Change the owner and group of a file */
int sfs_chown(const char *path, uid_t uid, gid_t gid) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = chown(fpath, uid, gid);

    if (retstat != -1) // log only successful attempts
        logChown(path, uid, gid);

    if (retstat < 0)
        retstat = sfs_error("sfs_chown chown");

    return retstat;
}

/** Change the size of a file */
int sfs_truncate(const char *path, off_t newsize) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = truncate(fpath, newsize);

    if (retstat != -1) // log only successful attempts
        logTruncate(path, newsize);

    if (retstat < 0)
        sfs_error("sfs_truncate truncate");

    return retstat;
}

/** Change the access and/or modification times of a file */

/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int sfs_utime(const char *path, struct utimbuf *ubuf) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = utime(fpath, ubuf);
    if (retstat < 0)
        retstat = sfs_error("sfs_utime utime");

    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    fd = open(fpath, fi->flags);
    if (fd < 0)
        retstat = sfs_error("sfs_open open");

    fi->fh = fd;

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.

int sfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    int retstat = 0;

    // no need to get fpath on this one, since I work from fi->fh not the path

    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0)
        retstat = sfs_error("sfs_read read");

    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.

int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    int retstat = 0;

    retstat = pwrite(fi->fh, buf, size, offset);

    // log all writes, where number of written bytes was greater than 0
    // since number of written bytes can be less than size, use returned number
    // for logging
    if (retstat > 0)
        logWrite(path, retstat, offset);

    if (retstat < 0)
        retstat = sfs_error("sfs_write pwrite");

    return retstat;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int sfs_statfs(const char *path, struct statvfs *statv) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    // get stats for underlying filesystem
    retstat = statvfs(fpath, statv);
    if (retstat < 0)
        retstat = sfs_error("sfs_statfs statvfs");

    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int sfs_flush(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;

    // no need to get fpath on this one, since I work from fi->fh not the path

    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    retstat = close(fi->fh);

    return retstat;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int sfs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    int retstat = 0;

    if (datasync)
        retstat = fdatasync(fi->fh);
    else
        retstat = fsync(fi->fh);

    if (retstat < 0)
        sfs_error("sfs_fsync fsync");

    return retstat;
}

/** Set extended attributes */
int sfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
    int retstat = 0;
    char fpath[PATH_MAX];

    // TODO: log?
    sfs_fullpath(fpath, path);

    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0)
        retstat = sfs_error("sfs_setxattr lsetxattr");

    return retstat;
}

/** Get extended attributes */
int sfs_getxattr(const char *path, const char *name, char *value, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0)
        retstat = sfs_error("sfs_getxattr lgetxattr");

    return retstat;
}

/** List extended attributes */
int sfs_listxattr(const char *path, char *list, size_t size) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = llistxattr(fpath, list, size);
    if (retstat < 0)
        retstat = sfs_error("sfs_listxattr llistxattr");

    return retstat;
}

/** Remove extended attributes */
int sfs_removexattr(const char *path, const char *name) {
    int retstat = 0;
    char fpath[PATH_MAX];

    // TODO: log?
    sfs_fullpath(fpath, path);

    retstat = lremovexattr(fpath, name);
    if (retstat < 0)
        retstat = sfs_error("sfs_removexattr lrmovexattr");

    return retstat;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi) {
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    dp = opendir(fpath);
    if (dp == NULL)
        retstat = sfs_error("sfs_opendir opendir");

    fi->fh = (intptr_t) dp;


    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info *fi) {
    int retstat = 0;
    DIR *dp;
    struct dirent *de;

    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) fi->fh;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    de = readdir(dp);
    if (de == 0) {
        retstat = sfs_error("sfs_readdir readdir");
        return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do {
        if (filler(buf, de->d_name, NULL, 0) != 0) {
            return -ENOMEM;
        }
    } while ((de = readdir(dp)) != NULL);


    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi) {
    int retstat = 0;

    closedir((DIR *) (uintptr_t) fi->fh);

    return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ???

int sfs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
    int retstat = 0;

    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).

void *sfs_init(struct fuse_conn_info *conn) {
    int fd;
    char pidpath[PATH_MAX];
    char buf[RESOURCE_MAX];

    // write PID into .pid file
    // we can't do it in main(), because fuse clones this process
    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/fs/%s.pid", config.resource);
    fd = open(pidpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (ftruncate(fd, 0) == -1)
        errExit("Could not truncate PID file '%s'", pidpath);

    snprintf(buf, RESOURCE_MAX, "%ld\n", (long) getpid());
    if (write(fd, buf, strlen(buf)) != strlen(buf))
        fatal("Writing to PID file '%s'", pidpath);

    // setup signal handlers
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = handleSIGUSR1;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &act, NULL) == -1)
        fatal("sigaction");

    return NULL;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata) {
    char pidpath[PATH_MAX];

    snprintf(pidpath, PATH_MAX, "/var/run/syncedfs/fs/%s.pid", config.resource);

    if (unlink(pidpath) == -1)
        errExit("Deleting PID file '%s'", pidpath);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int sfs_access(const char *path, int mask) {
    int retstat = 0;
    char fpath[PATH_MAX];

    sfs_fullpath(fpath, path);

    retstat = access(fpath, mask);

    if (retstat < 0)
        retstat = sfs_error("sfs_access access");

    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int retstat = 0;
    char fpath[PATH_MAX];
    int fd;

    sfs_fullpath(fpath, path);

    fd = creat(fpath, mode);

    if (fd != -1) // log only successful attempts
        logCreate(path, mode);

    if (fd < 0)
        retstat = sfs_error("sfs_create creat");

    fi->fh = fd;

    return retstat;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int sfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    int retstat = 0;

    retstat = ftruncate(fi->fh, offset);

    // TODO: check if path is properly populated
    if (retstat != -1) // log only successful attempts as truncate operation
        logTruncate(path, offset);

    if (retstat < 0)
        retstat = sfs_error("sfs_ftruncate ftruncate");

    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
// Since it's currently only called after sfs_create(), and sfs_create()
// opens the file, I ought to be able to just use the fd and ignore
// the path...

int sfs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi) {
    int retstat = 0;

    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
        retstat = sfs_error("sfs_fgetattr fstat");

    return retstat;
}

struct fuse_operations sfs_oper = {
    .getattr = sfs_getattr,
    .readlink = sfs_readlink,
    // no .getdir -- that's deprecated
    .getdir = NULL,
    .mknod = sfs_mknod,
    .mkdir = sfs_mkdir,
    .unlink = sfs_unlink,
    .rmdir = sfs_rmdir,
    .symlink = sfs_symlink,
    .rename = sfs_rename,
    .link = sfs_link,
    .chmod = sfs_chmod,
    .chown = sfs_chown,
    .truncate = sfs_truncate,
    .utime = sfs_utime,
    .open = sfs_open,
    .read = sfs_read,
    .write = sfs_write,
    /** Just a placeholder, don't set */ // huh???
    .statfs = sfs_statfs,
    .flush = sfs_flush,
    .release = sfs_release,
    .fsync = sfs_fsync,
    .setxattr = sfs_setxattr,
    .getxattr = sfs_getxattr,
    .listxattr = sfs_listxattr,
    .removexattr = sfs_removexattr,
    .opendir = sfs_opendir,
    .readdir = sfs_readdir,
    .releasedir = sfs_releasedir,
    .fsyncdir = sfs_fsyncdir,
    .init = sfs_init,
    .destroy = sfs_destroy,
    .access = sfs_access,
    .create = sfs_create,
    .ftruncate = sfs_ftruncate,
    .fgetattr = sfs_fgetattr
};

/*
 * 
 */
int main(int argc, char** argv) {
    int fuse_stat;
    int fargc;
    char** fargv;

    if (argc != 2)
        usageErr("syncedfs resource-name\n");

    if ((getuid() == 0) || (geteuid() == 0))
        fatal("We don't want to run syncedfs as root.");

    // read configuration
    if (readConfig(argv[1]) != 0)
        fatal("Error reading configuration file.");

    /*
        printf("resource: %s\n", config.resource);
        printf("rootdir: %s\n", config.rootdir);
        printf("mountdir: %s\n", config.mountdir);
        printf("logdir: %s\n", config.logdir);
     */

    // open log
    if (openLog() != 0)
        errExit("Could not open log file.");

    // add -o nonempty,use_ino (maybe also allow_other,default_permissions?)
    fargc = 5;
    fargv = malloc((fargc + 1) * sizeof (char *));
    if (fargv == NULL)
        fatal("Cannot allocate memory for fuse arguments.\n");

    fargv[0] = argv[0];
    fargv[1] = config.mountdir; // mount point
    fargv[2] = "-s"; // single thread
    fargv[3] = "-o"; // options
    fargv[4] = "nonempty,use_ino";
    fargv[5] = NULL;

    // argument parsing: so far only add options and pass to fuse
    // TODO: proper processing
    fuse_stat = fuse_main(fargc, fargv, &sfs_oper, NULL);
    return (fuse_stat);
}
