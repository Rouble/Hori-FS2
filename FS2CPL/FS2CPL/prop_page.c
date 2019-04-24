/*
 * prop_page.c  -- implementation of each property sheet
 *
 * $Id: prop_page.c,v 1.8 2004/03/04 12:38:29 hos Exp $
 *
 */

#include <windows.h>
#include <prsht.h>
#include "main.h"
#include "util.h"


/* global datas */

LPWSTR prop_page_template[] = {
    MAKEINTRESOURCEW(IDD_MAIN_SETTING_PAGE),
    MAKEINTRESOURCEW(IDD_INPUT_TEST_PAGE),
};

USHORT num_of_prop_pages = sizeof(prop_page_template) / sizeof(LPWSTR);


/* list of page proc alists */
struct uint_ptr_pair *page_proc_list[] = {
    main_setting_page,
    input_test_page,
};

typedef INT_PTR (*page_proc_t)(HWND, UINT,
                               WPARAM, LPARAM,
                               prop_page_context_t *);


/* implementations */

INT_PTR CALLBACK prop_page_proc(HWND hwnd, UINT msg,
                                WPARAM wparam, LPARAM lparam)
{
    prop_page_context_t *ctx;

    ctx = (prop_page_context_t *)GetWindowLongPtr(hwnd, DWLP_USER);

    if(msg == WM_INITDIALOG) {
        ctx = (prop_page_context_t *)((PROPSHEETPAGE *)lparam)->lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ctx);

        ctx->page_hwnd = hwnd;
        ctx->data->sheet_hwnd = GetParent(hwnd);
    }

    if(ctx != NULL) {
        page_proc_t proc;

        proc = (page_proc_t)assq_pair(ctx->page_proc, msg, NULL);
        if(proc != NULL) {
            return proc(hwnd, msg, wparam, lparam, ctx);
        }
    }

    return FALSE;
}


INT_PTR window_command_proc(HWND hwnd, UINT msg,
                            WPARAM wparam, LPARAM lparam,
                            prop_page_context_t *ctx)
{
    WORD code, id;
    HWND ctrl;
    command_proc_t proc;
    struct uint_ptr_pair *map;

    code = HIWORD(wparam);
    id = LOWORD(wparam);
    ctrl = (HWND)lparam;

    if(ctx->command_proc == NULL) {
        return FALSE;
    }

    map = (struct uint_ptr_pair *)assq_pair(ctx->command_proc, id, NULL);
    if(map == NULL) {
        return FALSE;
    }

    proc = (command_proc_t)assq_pair(map, code, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, code, id, ctrl, ctx);
}

INT_PTR window_notify_proc(HWND hwnd, UINT msg,
                           WPARAM wparam, LPARAM lparam,
                           prop_page_context_t *ctx)
{
    NMHDR *nmh;
    notify_proc_t proc;
    struct uint_ptr_pair *map;

    nmh = (NMHDR *)lparam;

    if(ctx->notify_proc == NULL) {
        return FALSE;
    }

    map = (struct uint_ptr_pair *)assq_pair(ctx->notify_proc,
                                            nmh->idFrom, NULL);
    if(map == NULL) {
        return FALSE;
    }

    proc = (notify_proc_t)assq_pair(map, nmh->code, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, nmh, ctx);
}


void get_dialog_item(HWND hwnd, struct uint_ptr_pair *map)
{
    int i;
    HWND *dst;

    for(i = 0; map[i].ptr != NULL; i++) {
        dst = (HWND *)map[i].ptr;

        *dst = GetDlgItem(hwnd, map[i].key);
    }
}


void set_modifier_bits_to_checkbox(HWND *chk, USHORT mod, USHORT mod_mask)
{
    int i;
    WPARAM state;

    for(i = 1; i <= 16; i++) {
        if(mod_mask & CONTROLLER_MODEFIER(i)) {
            if(mod & CONTROLLER_MODEFIER(i)) {
                state = BST_CHECKED;
            } else {
                state = BST_UNCHECKED;
            }
        } else {
            state = BST_INDETERMINATE;
        }

        SendMessage(chk[i - 1], BM_SETCHECK, state, 0);
    }
}

void get_modifier_bits_from_checkbox(HWND *chk, USHORT *mod, USHORT *mod_mask)
{
    int i;
    LRESULT state;

    for(i = 1; i <= 16; i++) {
        state = SendMessage(chk[i - 1], BM_GETCHECK, 0, 0);

        switch(state) {

#define SET_BIT(mod, idx, set)                      \
  (mod) = (set ? CONTROLLER_MODEFIER(idx) : 0) |    \
          (~CONTROLLER_MODEFIER(idx) & mod)

          case BST_CHECKED:
              SET_BIT(*mod, i, 1);
              SET_BIT(*mod_mask, i, 1);
              break;

          case BST_UNCHECKED:
              SET_BIT(*mod, i, 0);
              SET_BIT(*mod_mask, i, 1);
              break;

          case BST_INDETERMINATE:
              SET_BIT(*mod, i, 0);
              SET_BIT(*mod_mask, i, 0);
              break;

#undef SET_BIT

          default:
              break;
        }
    }
}
