/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 2, 2012, 10:16 AM
 */

#define _BSD_SOURCE

#include "config.h"
#include <string.h>
#include "time.h"
#include "string.h"

/* TODO: this should be called when SIGHUP signal is received */
void parseConfig(char *host, char* port) {
    (void) strcpy(c_host, host);
    (void) strcpy(c_port, port);

    (void) strcpy(c_mountpoint, "/mnt/kvmstorage/");
    (void) strcpy(c_rootdir, "/mnt/kvmstorage/machine1");
    (void) strcpy(c_resource, "machine1");
}

const char *getSyncId(void) {
    static char syncid[SYNCID_MAX];

    if (strlen(syncid) == 0) {
        time_t t;
        struct tm *tm;
        size_t s;

        t = time(NULL);
        tm = gmtime(&t);
        s = strftime(syncid, SYNCID_MAX, "%D_%T", tm);
        (void) strncpy(syncid + s, c_resource, SYNCID_MAX - s);
    }
    
    return syncid;
}