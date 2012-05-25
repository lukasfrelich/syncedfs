/* 
 * File:   config.h
 * Author: lfr
 *
 * Created on April 23, 2012, 11:10 AM
 */

#ifndef PARAMS_H
#define	PARAMS_H

// latest FUSE API
#define FUSE_USE_VERSION 26
#define RESOURCE_MAX 32

#include <limits.h>

typedef struct configuration_t {
    char resource[RESOURCE_MAX];
    char rootdir[PATH_MAX];
    char mountdir[PATH_MAX];
    char logdir[PATH_MAX];
} configuration_t;

typedef struct log_t {
    int fd;
    int switchpending;
    int writepending;
} log_t;

extern configuration_t config;

int readConfig(char *resource);

/*struct sfs_state {
    int logfile;
    int errlog;
    char *rootdir;
};*/
//#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

#endif	/* PARAMS_H */

