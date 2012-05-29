/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on April 26, 2012, 9:54 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../syncedfs-common/lib/error_functions.h"
#include "../syncedfs-common/lib/inet_sockets.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"
#include "config.h"
#include "client.h"
#include "server.h"

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc < 2)
        usageErr("syncedfsd server|client [host] port\n");

    if (readConfig(argv[1]) != 0)
        fatal("Error reading configuration file.");

    printf("resource: %s\n", config.resource);
    printf("rootdir: %s\n", config.rootdir);
    printf("mountdir: %s\n", config.snapshot);
    printf("logdir: %s\n", config.logdir);

    printf("host: %s\n", config.host);
    printf("port: %s\n", config.port);
    
    return 0;
    //
    if (strcmp(argv[1], "server") == 0)
        startServer();

    if (strcmp(argv[1], "client") == 0)
        synchronize();

    return (EXIT_SUCCESS);
}
