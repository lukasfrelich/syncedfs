/* 
 * File:   config_functions.h
 * Author: lfr
 *
 * Created on May 29, 2012, 2:55 PM
 */

#ifndef CONFIG_FUNCTIONS_H
#define	CONFIG_FUNCTIONS_H

#include <libconfig.h>

int setConfigString(const config_t* config, const char *path,
        const char *cfgbuf, char *dest, int destmax, int useCanonicalPath);

#endif	/* CONFIG_FUNCTIONS_H */

