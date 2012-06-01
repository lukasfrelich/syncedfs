/* 
 * File:   sighandlers.c
 * Author: lfr
 *
 * Created on May 31, 2012, 2:00 PM
 */

#define _GNU_SOURCE

#include <signal.h>
#include <unistd.h>
#include "config.h"
#include "sighandlers.h"
#include "../syncedfs-common/lib/error_functions.h"
#include "log.h"
#include <stdint.h>

extern sig_atomic_t switchpending;
extern sig_atomic_t writepending;
static sig_atomic_t pidpending;

void handleSIGUSR1(int sig, siginfo_t *siginfo, void *ucontext) {
    // we don't have to block SIGUSR1, it is done automatically by sigaction
    // if we can't do the switch right away, store pid for further use
    if (writepending == 1) {
        switchpending = 1;
        pidpending = siginfo->si_pid;
        return;
    }
    
    switchLog();
    switchpending = 0;
    
    // if we were called directly by operating system, we know pid from siginfo
    // if we were called by log operation, we used stored pid
    if (siginfo != NULL)
        kill(siginfo->si_pid, SIGUSR1);
    else
        kill(pidpending, SIGUSR1);
}
