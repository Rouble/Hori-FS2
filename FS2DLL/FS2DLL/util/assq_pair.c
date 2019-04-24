/*
 * assq_pair.c  -- utility function assq_pair
 *
 * $Id: assq_pair.c,v 1.2 2004/03/09 10:20:12 hos Exp $
 *
 */


#include "util.h"


void *assq_pair(const struct uint_ptr_pair *pair,
                unsigned long key, void *default_ptr)
{
    while(pair->ptr != (void *)0) {
        if(pair->key == key) {
            return pair->ptr;
        }

        pair++;
    }

    return default_ptr;
}
