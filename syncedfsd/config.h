/* 
 * File:   config.h
 * Author: lfr
 *
 * Created on May 2, 2012, 10:07 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <netdb.h>
#include <limits.h>

#define MAX_HEADER_LEN 4096 /*file name etc.*/
#define RESNAME_MAX 32
#define SYNCID_MAX 64
#define MAX_WRITE_LEN 4096
#define MESSAGE_MAX 524288

#define OFFSET_LEN 8
#define LENGTH_LEN 4
#define VECTOR_INIT_CAPACITY 8

char c_port[NI_MAXSERV];
char c_host[NI_MAXHOST];

char c_resource[RESNAME_MAX];
char c_mountpoint[PATH_MAX];
char c_rootdir[PATH_MAX];

void parseConfig(char *host, char* port);

const char *getSyncId(void);

#endif	/* CONFIG_H */

