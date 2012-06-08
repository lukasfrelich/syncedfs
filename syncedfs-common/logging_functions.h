/* 
 * File:   logging_functions.h
 * Author: lfr
 *
 * Created on June 6, 2012, 2:37 PM
 * Based on error_functions.h from TLPI
 */

#ifndef LOGGING_FUNCTIONS_H
#define	LOGGING_FUNCTIONS_H

#include <syslog.h>

void errnoMsg(int severity, const char *format, ...);
void errnoExit(int severity, const char *format, ...);
void errMsg(int severity, const char *format, ...);
void errExit(int severity, const char *format, ...);

void stderrExit(const char *format, ...);

#endif	/* LOGGING_FUNCTIONS_H */
