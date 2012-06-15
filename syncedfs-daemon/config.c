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

    // TODO resource should only contain [a-Z] [0-9] or - characters
    strncpy(config.resource, resource, RESOURCE_MAX);

    // read main config
    config_init(&cfg);
    // Read the file. If there is an error, report it and exit
    if (!config_read_file(&cfg, "/etc/syncedfs.conf")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    int ret = 0;
    ret |= setConfigString(&cfg, "btrfs", str, config.btfsbin, PATH_MAX, 1);

    sprintf(config.ident, "syncedfsd %s", config.resource);

    config_destroy(&cfg);

    // read resource config
    snprintf(cfgpath, PATH_MAX, "%s%s%s",
            "/etc/syncedfs.d/", resource, ".conf");

    config_init(&cfg);
    // Read the file. If there is an error, report it and exit
    if (!config_read_file(&cfg, cfgpath)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    ret |= setConfigString(&cfg, "rootdir", str, config.rootdir, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "snapshot", str, config.snapshot, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "logdir", str, config.logdir, PATH_MAX, 1);

    ret |= setConfigString(&cfg, "host", str, config.host, NI_MAXHOST, 0);
    ret |= setConfigString(&cfg, "port", str, config.port, NI_MAXSERV, 0);

    sprintf(config.ident, "syncedfsd %s", config.resource);

    config_destroy(&cfg);
    return ret;
}
