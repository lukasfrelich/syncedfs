/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 2, 2012, 10:16 AM
 */

#include "config.h"
#include <string.h>
#include "time.h"
#include "string.h"

/* TODO: this should be called when SIGHUP signal is received */
void parseConfig(char *host, char* port) {
    (void) strcpy(c_host, host);
    (void) strcpy(c_port, port);

    (void) strcpy(c_mountpoint, "/mnt/kvmstorage");
    if (strcmp(c_host, "") == 0)
        (void) strcpy(c_rootdir, "/home/lukes/syncedfs/secondary/physical");      // server
    else
        (void) strcpy(c_rootdir, "/home/lukes/syncedfs/primary/physical");
    (void) strcpy(c_resource, "r0");
}

char *getSyncId(void) {
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