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
    fprintf(stderr, "%s\n", cfgpath);

    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, cfgpath)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    /* Get the store name. */
    if (config_lookup_string(&cfg, "rootdir", &str))
        printf("Store name: %s\n\n", str);
    else
        fprintf(stderr, "No 'name' setting in configuration file.\n");
    
    return 0;
}
