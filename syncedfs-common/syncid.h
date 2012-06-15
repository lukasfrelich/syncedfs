/* 
 * File:   syncid.h
 * Author: lfr
 *
 * Created on June 14, 2012, 11:42 AM
 */

#ifndef SYNCID_H
#define	SYNCID_H

#define SYNCID_MAX 64

int writeSyncId(char *id, char *idpath);
int readSyncId(char *id, char *idpath, int maxlength);

#endif	/* SYNCID_H */

