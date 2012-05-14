/* 
 * File:   params.h
 * Author: lfr
 *
 * Created on April 23, 2012, 11:10 AM
 */

#ifndef PARAMS_H
#define	PARAMS_H

// latest FUSE API
#define FUSE_USE_VERSION 26

#define _FILE_OFFSET_BITS 64

// to get pread and pwrite
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <fuse.h>
struct sfs_state {
    FILE *logfile;
    FILE *writelog;
    char *rootdir;
};
#define LOG_ENTRY_HEADER sizeof(char) + sizeof(size_t) + sizeof(int)
#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

#endif	/* PARAMS_H */

