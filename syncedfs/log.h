/* 
 * File:   log.h
 * Author: lfr
 *
 * Created on April 23, 2012, 3:12 PM
 */

#ifndef LOG_H
#define	LOG_H

#include <unistd.h>

int openLog(void);
void switchLog(void);

//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
void logWrite(const char *relpath, off_t offset, size_t size);


#endif	/* LOG_H */
