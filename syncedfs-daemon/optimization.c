/* 
 * File:   optimization.c
 * Author: lfr
 *
 * Created on July 9, 2012, 3:36 PM
 */

#include <stdlib.h>
#include "optimization.h"

int cmpOperationType(const void *a, const void *b) {
    GenericOperation *x;
    GenericOperation *y;

    x = *((GenericOperation **) a);
    y = *((GenericOperation **) b);
    
    if (x == NULL)
        return -1;
    if (y == NULL)
        return 1;

    // if both operations are of the same type
    if (x->type == y->type) {
        // write operations are further sorted by offset
        if (x->type == GENERIC_OPERATION__TYPE__WRITE) {
            if (x->write_op->offset == y->write_op->offset) {
                return 0;
            } else {
                if (x->write_op->offset < y->write_op->offset)
                    return -1;
                else
                    return 1;
            }
        } else {
            // other operations are sorted by id
            if (x->has_id && (x->id < y->id))
                return -1;
            else
                return 1;
        }
    } else {
        if (x->type < y->type)
            return -1;
        else
            return 1;
    }
}

int optimizeOperations(fileop_t *f) {
    GenericOperation *op;

    // optimize writes which wrote behind a consequent truncate
    // iterate in reverse order, find 'first' (i.e. last) truncate
    // and edit every 'subsequent' (i.e. preceeding) write, if necessary
    int64_t newsize;
    int truncfound = 0;
    for (int i = f->nelem - 1; i >= 0; i--) {
        op = *(f->operations + i);

        // until we get truncate operation, simply continue
        if (!truncfound) {
            if (op->type != GENERIC_OPERATION__TYPE__TRUNCATE) {
                continue;
            }
            else {
                // when we first get a truncate op, set a flag to not search for it
                // anymore
                truncfound = 1;
                newsize = op->truncate_op->newsize;
                
            }
        }

        // after finding truncate op, we are only interested in 
        if (op->type != GENERIC_OPERATION__TYPE__WRITE)
            continue;

        // if 'right edge' of the write interval lies beyond truncate, we must
        // change the interval
        if (op->write_op->offset + op->write_op->size > newsize) {
            // write interval is completely beyond the truncate => forget it
            if (op->write_op->offset >= newsize)
                *(f->operations + i) = NULL;
            else
                op->write_op->size -=
                    (op->write_op->offset + op->write_op->size - newsize);
        }
    }


    // now sort operations (write should go at the end, create, link etc. at the
    // beginning...)
    qsort(f->operations, f->nelem, sizeof (GenericOperation *),
            cmpOperationType);

    // optimize write intervals (rule out overlapping intervals)
    // we could also delete all chmods/chowns... but the last one, but this is
    // not being done now
    int64_t maxoff = 0;
    int firstwrite = 0;
    for (int i = 0; i < f->nelem; i++) {
        op = *(f->operations + i);
        
        if (op == NULL)
            continue;

        if (op->type != GENERIC_OPERATION__TYPE__WRITE)
            continue;

        // when we get first write op, try to get newsize from last truncate
        // if there was a truncate somewhere in the 
        if (!firstwrite) {
            firstwrite = 1;
        }

        // this algorithms works only if write operations are sorted by offset
        // if offset of the right edge of the interval is less than or equal
        // the max offset, just delete the operation: previous write already
        // includes it
        if (op->write_op->offset + op->write_op->size <= maxoff) {
            *(f->operations + i) = NULL;
        } else {
            // if offset of the left edge (= the offset) is less than max offset
            // edit the operations, so we get rid the overlap
            if (op->write_op->offset < maxoff) {
                op->write_op->size -= (maxoff - op->write_op->offset);
                op->write_op->offset = maxoff;
            }
            maxoff = op->write_op->offset + op->write_op->size;
        }
    }
    
    // get rid of gaps in operations (caused by deleting redundant operations)
    int j = 0;  // write offset
    for (int i = 0; i < f->nelem; i++) {
        op = *(f->operations + i);
        
        if (op != NULL) {
            *(f->operations + j) = *(f->operations + i);
            j++;
        }
    }
    f->nelem = j;
    
    return 0;
}
