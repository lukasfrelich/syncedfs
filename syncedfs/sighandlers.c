/* 
 * File:   sighandlers.c
 * Author: lfr
 *
 * Created on May 31, 2012, 2:00 PM
 */

#define _GNU_SOURCE

#include <signal.h>
#include <unistd.h>
#include "sighandlers.h"

void handleSIGUSR1(int sig, siginfo_t *siginfo, void *ucontext) {
    // switch log
    sleep(1);
    
    
    // when everything is done, send signal back
    kill(siginfo->si_pid, SIGUSR1);
}
