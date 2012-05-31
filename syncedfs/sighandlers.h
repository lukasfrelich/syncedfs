/* 
 * File:   sighandlers.h
 * Author: lfr
 *
 * Created on May 31, 2012, 1:59 PM
 */

#ifndef SIGHANDLERS_H
#define	SIGHANDLERS_H

#include <signal.h>

void handleSIGUSR1(int sig, siginfo_t *siginfo, void *ucontext);

#endif	/* SIGHANDLERS_H */

