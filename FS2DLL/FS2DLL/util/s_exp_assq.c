/*
 * s_exp_assq.c  -- s-expression assq
 *
 * $Id: s_exp_assq.c,v 1.1 2004/06/07 10:45:22 hos Exp $
 *
 */

#include "s_exp.h"

s_exp_data_t *s_exp_assq(s_exp_data_t *alist, wchar_t *sym)
{
    s_exp_data_t *d, *s;

    s = s_exp_intern(sym);
    if(S_EXP_ERROR(s)) {
        return NULL;
    }

    S_EXP_FOR_EACH(alist, d) {
        if(S_EXP_CAR(d)->type == S_EXP_TYPE_CONS && S_EXP_CAAR(d) == s) {
            free_s_exp(s);
            return S_EXP_CDAR(d);
        }
    }
    free_s_exp(s);

    return NULL;
}
