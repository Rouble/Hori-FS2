/*
 * s_exp_massq.c  -- s-expression multi-assq
 *
 * $Id: s_exp_massq.c,v 1.1 2004/06/07 10:45:24 hos Exp $
 *
 */

#include "s_exp.h"
#include <stdarg.h>

s_exp_data_t *s_exp_massq(s_exp_data_t *alist, int type, ...)
{
    va_list ap;
    wchar_t *sn;
    s_exp_data_t *ret = NULL, *d;

    va_start(ap, type);

    while(1) {
        sn = va_arg(ap, wchar_t *);
        if(sn == NULL) {
            if(alist->type != type) {
                goto func_end;
            }

            break;
        }

        d = s_exp_assq(alist, sn);
        if(d == NULL) {
            goto func_end;
        }

        alist = d;
    }

    ret = alist;

  func_end:
    va_end(ap);

    return ret;
}
