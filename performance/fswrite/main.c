/* 
 * File:   main.c
 * Author: lfr
 *
 * Created on March 29, 2012, 10:19 AM
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "actions.h"


/*
 * First parameter: command
 * 
 */
int main(int argc, char** argv) {
    if (argc != 5) {
        perror("Usage: fswrite action size srcfile destfile");
        return (EXIT_FAILURE);
    }
    
    /*action*/
    if (strcmp(argv[1], "seqwrite") != 0 && strcmp(argv[1], "ranwrite") != 0 &&
            strcmp(argv[1], "append") != 0 ) {
        perror("Unrecognized command");
        return (EXIT_FAILURE);
    }
    
    /*count*/
    long count = atol(argv[2]);
    if (count == 0) {
        perror("Wrong size\n");
    }
    printf("Count: %ld\n", count);
    
    /*files*/
    char srcpath[256];
    strncpy(srcpath, argv[3], 255);
    char dstpath[256];
    strncpy(dstpath, argv[4], 255);
    
    
    if (strcmp(argv[1], "seqwrite") == 0) {
        seqwrite(count, MODE_WRITE, srcpath, dstpath);
    }
    if (strcmp(argv[1], "ranwrite") == 0) {
        ranwrite(count, srcpath, dstpath);
    }
    if (strcmp(argv[1], "append") == 0) {
        seqwrite(count, MODE_APPEND, srcpath, dstpath);
    }
    return (EXIT_SUCCESS);
}

