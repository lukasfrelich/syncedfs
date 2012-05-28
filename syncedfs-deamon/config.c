/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 2, 2012, 10:16 AM
 */

#include "config.h"
#include <string.h>
#include <unistd.h>
#include "time.h"
#include <pwd.h>
#include "string.h"


/* TODO: this should be called when SIGHUP signal is received */
void parseConfig(char *host, char* port) {
    (void) strcpy(c_host, host);
    (void) strcpy(c_port, port);

    (void) strcpy(c_mountpoint, "/mnt/kvmstorage");
    
    (void) strcat(c_rootdir, getHomeDir());
    if (strcmp(c_host, "") == 0)
        (void) strcat(c_rootdir, "/syncedfs/secondary/physical");
    else
        (void) strcat(c_rootdir, "/syncedfs/primary/physical");
        
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

char *getHomeDir(void) {
    struct passwd *pw;
    static char homedir[40];
    register uid_t uid;

    uid = geteuid();
    pw = getpwuid(uid);
    if (pw) {
        strncpy(homedir, pw->pw_dir, 39);
    }

    return homedir;
}
