/* 
 * File:   config.c
 * Author: lfr
 *
 * Created on May 24, 2012, 3:27 PM
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <string.h>

configuration_t config;

int readConfig(char *resource) {
    char cfgpath[PATH_MAX];
    config_t cfg;
    const char *str;

    strncpy(config.resource, resource, RESOURCE_MAX);

    (void) strcat(cfgpath, "/etc/syncedfs.d/");
    (void) strcat(cfgpath, resource);
    (void) strcat(cfgpath, ".conf");

    config_init(&cfg);

    // Read the file. If there is an error, report it and exit
    if (!config_read_file(&cfg, cfgpath)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    int ret = 0;
    if (config_lookup_string(&cfg, "rootdir", &str)) {
        (void) strcpy(config.rootdir, str);
    } else {
        fprintf(stderr, "No 'rootdir' setting in configuration file.\n");
        ret = -2;
    }
    if (config_lookup_string(&cfg, "mountdir", &str)) {
        (void) strcpy(config.mountdir, str);
    } else {
        fprintf(stderr, "No 'rootdir' setting in configuration file.\n");
        ret = -2;
    }
    if (config_lookup_string(&cfg, "logdir", &str)) {
        (void) strcpy(config.logdir, str);
    } else {
        fprintf(stderr, "No 'rootdir' setting in configuration file.\n");
        ret = -2;
    }
    
    return ret;
}
