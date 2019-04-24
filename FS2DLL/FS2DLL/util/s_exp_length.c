/*
 * s_exp_length.c  -- length of list
 *
 * $Id: s_exp_length.c,v 1.1 2004/06/07 10:45:23 hos Exp $
 *
 */

#include "s_exp.h"

int s_exp_length(s_exp_data_t *list)
{
    int n;
    s_exp_data_t *p;

    if(list == S_EXP_NIL) {
        return 0;
    }

    if(list->type != S_EXP_TYPE_CONS) {
        return -1;
    }

    n = 0;
    S_EXP_FOR_EACH(list, p) {
        n++;
    }

    return n;
}
