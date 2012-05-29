/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 24, 2012, 3:27 PM
 */

#include "config.h"
#include "../syncedfs-common/path_functions.h"
#include "../syncedfs-common/config_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <string.h>

configuration_t config;

int readConfig(char *resource) {
    char cfgpath[PATH_MAX];
    config_t cfg;
    char *str;

    strncpy(config.resource, resource, RESOURCE_MAX);
    
    snprintf(cfgpath, PATH_MAX, "%s%s%s",
            "/etc/syncedfs.d/", resource, ".conf");

    config_init(&cfg);
    if (!config_read_file(&cfg, cfgpath)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -2;
    }

    int ret = 0;
    ret |= setConfigString(&cfg, "rootdir", str, config.rootdir, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "mountdir", str, config.mountdir, PATH_MAX, 1);
    ret |= setConfigString(&cfg, "logdir", str, config.logdir, PATH_MAX, 1);
    config.rootdir_len = strlen(config.rootdir);
    
    config_destroy(&cfg);
    return ret;
}
