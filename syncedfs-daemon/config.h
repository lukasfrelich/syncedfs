/* 
 * File:   config.h
 * Author: lfr
 *
 * Created on May 2, 2012, 10:07 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#define RESOURCE_MAX 32
#define MAX_WRITE_LEN 4096
#define MESSAGE_MAX 524288

#define OFFSET_LEN 8
#define LENGTH_LEN 4
#define VECTOR_INIT_CAPACITY 8

#include <netdb.h>
#include <limits.h>

typedef struct configuration_t {
    char resource[RESOURCE_MAX];
    char rootdir[PATH_MAX];
    char snapshot[PATH_MAX];
    char logdir[PATH_MAX];
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    char ident[RESOURCE_MAX + 10];
    char btfsbin[PATH_MAX];
} configuration_t;

extern configuration_t config;


int readConfig(char *resource);

#endif	/* CONFIG_H */

