/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on April 26, 2012, 9:54 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../syncedfs-common/lib/inet_sockets.h"
#include "config.h"
#include "client.h"
#include "server.h"
#include "../syncedfs-common/protobuf/syncedfs.pb-c.h"

void usage(void) {
    fprintf(stderr, "usage: syncedfsd server|client [host] port\n");
}

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return (EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "server") == 0) {
        if (argc == 3) {
            parseConfig("", argv[2]);
            startServer();
        } else {
            usage();
        }
    }
    if (strcmp(argv[1], "client") == 0) {
        if (argc == 4) {
            parseConfig(argv[2], argv[3]);
            synchronize();
        } else {
            usage();
        }
    }
    return (EXIT_SUCCESS);
}
