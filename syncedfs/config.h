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

//------------------------------------------------------------------------------
// Structures
//------------------------------------------------------------------------------
typedef struct configuration_t {
    char resource[RESOURCE_MAX];
    // all paths are stored without trailing '/'
    char rootdir[PATH_MAX];
    int rootdir_len;
    char mountdir[PATH_MAX];
    char logdir[PATH_MAX];
} configuration_t;


/*typedef struct log_t {
    int fd;
    int switchpending;
    int writepending;
} log_t;*/

extern configuration_t config;
//extern log_t olog;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
int readConfig(char *resource);

#endif	/* PARAMS_H */

