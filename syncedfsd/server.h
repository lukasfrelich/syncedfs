/* 
 * File:   server.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef SERVER_H
#define	SERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include "lib/inet_sockets.h"
#include "lib/read_line.h"
#include "lib/tlpi_hdr.h"

void startServer(void);
void handleClient(int cfd, struct sockaddr *claddr, socklen_t *addrlen);
int createSnapshot(void);
int parseSyncStart(char *header, char *resname, int *numoper);
int parseOperation(char *header, char *operation, long long *datalength,
                   char *filename);
int handleWrite(int cfd, long long datalength, char *filename);

#endif	/* SERVER_H */
