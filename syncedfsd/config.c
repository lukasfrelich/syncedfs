/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 2, 2012, 10:16 AM
 */

#define _BSD_SOURCE

#include "config.h"
#include <string.h>

/* TODO: this should be called when SIGHUP signal is received */
void parseConfig(char *host, char* port) {
    (void) strcpy(c_host, host);
    (void) strcpy(c_port, port);
    
    (void) strcpy(c_mountpoint, "/mnt/kvmstorage/");
    (void) strcpy(c_rootdir, "/mnt/kvmstorage/machine1");
    (void) strcpy(c_resource, "machine1");
}