/* 
 * File:   snapshot.c
 * Author: lfr
 *
 * Created on June 9, 2012, 11:12 PM
 */

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "../syncedfs-common/logging_functions.h"
#include "snapshot.h"
#include "config.h"

// returns -1 if there was an error, or child was not terminated normally
// if child was terminated normally, returns its exit status

static int mexec(const char *path, char *const argv[]) {
    int childpid;
    int status;

    switch (childpid = fork()) {
        case -1:
            return -1;
            break;
        case 0: // child
            execv(path, argv);
            errnoMsg(LOG_ERR, "Could not execute program %s", path);
            _exit(127);
            break;
        default: // parent
            while (waitpid(childpid, &status, 0) == -1) {
                // waitpid could be interrupted by a signal, in that case try it
                // again
                if (errno != EINTR)
                    return -1;
            }
            break;
    }

    // this is not reliable (btrfs theoretically may return 127)
    if (WIFEXITED(status) && WEXITSTATUS(status) != 127)
        return WEXITSTATUS(status);
    else
        return -1;
}

int createSnapshot(char *source, char *dest, int readonly) {
    int ret;
    
    char *argv[7];
    int i = 0;
    argv[i++] = strrchr(config.btfsbin, '/');
    argv[i++] = "subvolume";
    argv[i++] = "snapshot";
    if (readonly)
        argv[i++] = "-r";
    argv[i++] = source;
    argv[i++] = dest;
    argv[i] = NULL;

    ret = mexec(config.btfsbin, argv);
    if (ret != 0) {
        if (ret == -1)  // execution failed
            errMsg(LOG_ERR, "Error while executing btrfs binary.");
        else    // command successfully executed, but return non zero value
            errMsg(LOG_ERR, "btrfs failed to create a snapshot.");
        
        return -1;
    }

    return 0;
}

int deleteSnapshot(char *path) {
    int ret;
    
    char *argv[5];
    argv[0] = strrchr(config.btfsbin, '/');
    argv[1] = "subvolume";
    argv[2] = "delete";
    argv[3] = path;
    argv[4] = NULL;

    ret = mexec(config.btfsbin, argv);
    if (ret != 0) {
        if (ret == -1)  // execution failed
            errMsg(LOG_ERR, "Error while executing btrfs binary.");
        else    // command successfully executed, but return non zero value
            errMsg(LOG_ERR, "btrfs failed to delete a snapshot.");
        
        return -1;
    }

    return 0;
}
