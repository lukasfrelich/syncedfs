/* 
 * File:   server.h
 * Author: lfr
 *
 * Created on May 2, 2012, 9:53 AM
 */

#ifndef SERVER_H
#define	SERVER_H

#include <inttypes.h>
#include "protobuf/syncedfs.pb-c.h"

void startServer(void);
void handleClient(int cfd, struct sockaddr *claddr, socklen_t *addrlen);
int createSnapshot(void);
int getMessage(int cfd, uint8_t **buffer, uint32_t *length);


//------------------------------------------------------------------------------
// Operation handlers
//------------------------------------------------------------------------------
int handleGenericOperation(int fd, GenericOperation *genop);
int handleWrite(int fd, WriteOperation *writeop);

#endif	/* SERVER_H */
