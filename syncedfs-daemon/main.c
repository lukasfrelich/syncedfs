/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on April 26, 2012, 9:54 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../syncedfs-common/logging_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "../syncedfs-common/lib/become_daemon.h"
#include "config.h"
#include "client.h"
#include "server.h"

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc < 3)
        stderrExit("usage: syncedfsd resource server|client\n");

    // TODO: this app probably will have to run as root
    // but it can drop root privileges most of the time

    if (readConfig(argv[1]) != 0)
        stderrExit("Error reading configuration file %s\n", argv[1]);

    // TODO: daemonize here
    
    openlog(config.ident, 0, LOG_DAEMON);

    // temp soulution
    if (strcmp(argv[2], "server") == 0) {
        (void) strcpy(config.resource, "r0");
    }
    /*printf("resource: %s\n", config.resource);
    printf("rootdir: %s\n", config.rootdir);
    printf("mountdir: %s\n", config.snapshot);
    printf("logdir: %s\n", config.logdir);

    printf("host: %s\n", config.host);
    printf("port: %s\n", config.port);*/

    if (strcmp(argv[2], "server") == 0) {
        // at this momement only daemonize server
        if (becomeDaemon(0) != 0)
            stderrExit("Could not become a daemon.\n", argv[1]);
        startServer();
    }

    if (strcmp(argv[2], "client") == 0)
        synchronize();

    return (EXIT_SUCCESS);
}
