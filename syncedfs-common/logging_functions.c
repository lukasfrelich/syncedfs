/* 
 * File:   logging_functions.c
 * Author: lfr
 *
 * Created on June 6, 2012, 2:38 PM
 * Based on error_functions.c from TLPI
 */

#define ERROR_MAX 500
#define SYSLOG_NAMES

#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ename.c.inc"          // defines ename and MAX_ENAME
#include "logging_functions.h"

static void terminate(void) {
    char *s;

    // Dump core if EF_DUMPCORE environment variable is defined and
    // is a nonempty string
    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else
        exit(EXIT_FAILURE);
}

static inline char *getSeverityName(int severity) {
    static char severitynames[9][10] = {"EMERGENCY", "ALERT", "CRITICAL",
        "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG", "UNKNOWN"};

    if (severity >= 0 && severity <= 7) {
        return severitynames[severity];
    } else {
        return severitynames[8];     // UNKNOWN
    }
}

// assumes, that openlog function has already been called

static void outputError(int useerrno, int err, int severity,
        const char *format, va_list ap) {

    char buf[ERROR_MAX], userMsg[ERROR_MAX], errText[ERROR_MAX];

    vsnprintf(userMsg, ERROR_MAX, format, ap);

    if (useerrno) {
        if (err > 0 && err <= MAX_ENAME) {
            snprintf(errText, ERROR_MAX, " [%s %s]", ename[err], strerror(err));
            snprintf(buf, ERROR_MAX, "%s: %s %s\n", getSeverityName(severity),
                    errText, userMsg);
        }
    } else {
        snprintf(buf, ERROR_MAX, "%s: %s\n", getSeverityName(severity),
                userMsg);
    }

    syslog(severity, "%s", buf);
}

void errnoMsg(int severity, const char *format, ...) {
    int savederrno;
    va_list argList;

    savederrno = errno;

    va_start(argList, format);
    outputError(1, errno, severity, format, argList);
    va_end(argList);

    errno = savederrno;
}

void errnoExit(int severity, const char *format, ...) {
    va_list argList;

    va_start(argList, format);
    outputError(1, errno, severity, format, argList);
    va_end(argList);

    terminate();
}

void errMsg(int severity, const char *format, ...) {
    int savederrno;
    va_list argList;

    savederrno = errno;

    va_start(argList, format);
    outputError(0, 0, severity, format, argList);
    va_end(argList);

    errno = savederrno;
}

void errExit(int severity, const char *format, ...) {
    va_list argList;

    va_start(argList, format);
    outputError(0, 0, severity, format, argList);
    va_end(argList);

    terminate();
}

void stderrExit(const char *format, ...) {
    va_list argList;

    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    terminate();
}