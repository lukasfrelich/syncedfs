/* 
 * File:   actions.h
 * Author: lfr
 *
 * Created on March 30, 2012, 3:19 PM
 */

#ifndef ACTIONS_H
#define	ACTIONS_H
#define MODE_WRITE 1
#define MODE_APPEND 2
#define MAX_SRC_SIZE 104857600 /*100 MB*/
#define MAX_WRITE_SIZE 20971520 /*20 MB*/
#define BLOCK_SIZE 4096 /*used in random write*/


int seqwrite(long count, int mode, const char* srcpath, const char* dstpath);
int ranwrite(long count, const char* srcpath, const char* dstpath);

#endif	/* ACTIONS_H */

