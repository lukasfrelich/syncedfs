/* 
 * File:   snapshot.h
 * Author: lfr
 *
 * Created on June 9, 2012, 11:12 PM
 */

#ifndef SNAPSHOT_H
#define	SNAPSHOT_H

int createSnapshot(char *source, char *dest, int readonly);
int deleteSnapshot(char *path);

#endif	/* SNAPSHOT_H */

