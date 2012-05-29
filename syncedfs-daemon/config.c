/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 2, 2012, 10:16 AM
 */

#include "config.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/config_functions.h"
#include <string.h>
#include <unistd.h>
#include <libconfig.h>
#include "time.h"
#include <pwd.h>
#include "string.h"

configuration_t config;

/* TODO: this should be called when SIGHUP signal is received */
int readConfig(char *resource) {
    char cfgpath[PATH_MAX];
    config_t cfg;
    const char *str;

    strncpy(config.resource, resource, RESOURCE_MAX);
    
    snprintf(cfgpath, PATH_MAX, "%s%s%s",
            "/etc/syncedfs.d/", resource, ".conf");

    config_init(&cfg);
    // Read the file. If there is an error, report it and exit
    if (!config_read_file(&cfg, cfgpath)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -2;
    }

    int ret = 0;
    ret |= setConfigString(&cfg, "rootdir", str, config.rootdir, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "snapshot", str, config.snapshot, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "logdir", str, config.logdir, PATH_MAX, 1);

    ret |= setConfigString(&cfg, "host", str, config.host, NI_MAXHOST, 0);
    ret |= setConfigString(&cfg, "port", str, config.port, NI_MAXSERV, 0);
    
    config_destroy(&cfg);
    return ret;
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
        (void) strncpy(syncid + s, config.resource, SYNCID_MAX - s);
    }

    return syncid;
}
