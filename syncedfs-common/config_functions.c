/* 
 * File:   config_functions.c
 * Author: lfr
 *
 * Created on May 29, 2012, 2:57 PM
 */

#include "config_functions.h"
#include "../syncedfs-common/path_functions.h"
#include <string.h>

int setConfigString(const config_t* config, const char *path,
        const char *cfgbuf, char *dest, int destmax, int useCanonicalPath) {
    
    if (config_lookup_string(config, path, &cfgbuf)) {
        if (useCanonicalPath)
            (void) strncpy(dest, getCanonincalPath((char *) cfgbuf), destmax);
        else
            (void) strncpy(dest, cfgbuf, destmax);
    } else {
        fprintf(stderr, "No '%s' setting in configuration file.\n", path);
        return -1;
    }

    return 0;
}