/* 
 * File:   log.h
 * Author: lfr
 *
 * Created on April 23, 2012, 3:12 PM
 */

#ifndef LOG_H
#define	LOG_H

/*FILE *log_open(void);
// used only to make the code run
// TODO: get rid of it
void log_msg(const char *format, ...);*/
#include <unistd.h>

int logOpen(void);
void logWrite(const char *relpath, off_t offset, size_t size);

#endif	/* LOG_H */
