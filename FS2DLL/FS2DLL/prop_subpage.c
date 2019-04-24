/*
 * prop_subpage.c  -- subpage proc
 *
 * $Id: prop_subpage.c,v 1.2 2004/02/22 15:18:19 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


typedef INT_PTR (*subpage_proc_t)(HWND, UINT,
                                  WPARAM, LPARAM,
                                  prop_page_context_t *);


/* implementations */

INT_PTR CALLBACK prop_subpage_proc(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam)
{
    prop_page_context_t *ctx;

    ctx = (prop_page_context_t *)GetWindowLongPtr(hwnd, DWLP_USER);

    if(msg == WM_INITDIALOG) {
        ctx = (prop_page_context_t *)lparam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ctx);
    }

    if(ctx != NULL) {
        subpage_proc_t proc;

        proc = (subpage_proc_t)assq_pair(ctx->page_proc, msg, NULL);
        if(proc != NULL) {
            return proc(hwnd, msg, wparam, lparam, ctx);
        }
    }

    return FALSE;
}
