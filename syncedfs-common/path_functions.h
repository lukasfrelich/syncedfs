/* 
 * File:   paths.h
 * Author: lukes
 *
 * Created on May 28, 2012, 6:12 PM
 */

#ifndef PATHS_H
#define	PATHS_H

#include <limits.h>

void getAbsolutePath(char *fpath, const char *rootdir, const char *path);
void getRelativePath(char *fpath, const char *rootdir, const char *path);
// removes trailing '/'
char *getCanonincalPath(char *path);

int fileExists(char *path);

//void getFullpath(char fpath[PATH_MAX], config_t *cfg, const char *path);

#endif	/* PATHS_H */

