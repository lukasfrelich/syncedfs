/* 
 * File:   optimization.h
 * Author: lfr
 *
 * Created on July 9, 2012, 3:37 PM
 */

#ifndef OPTIMIZATION_H
#define	OPTIMIZATION_H

#include "client.h"

int cmpOperationType(const void *a, const void *b);
int optimizeOperations(fileop_t *f);

#endif	/* OPTIMIZATION_H */
