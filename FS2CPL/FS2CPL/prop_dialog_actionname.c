/*
 * prop_dialog_actionname.c  -- action name dialog to enter action name
 *
 * $Id: prop_dialog_actionname.c,v 1.5 2004/03/25 13:53:37 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


/* prototypes */
static INT_PTR action_name_dlg_init_dialog(HWND hwnd, UINT msg,
                                           WPARAM wparam, LPARAM lparam,
                                           prop_page_context_t *ctx);
static INT_PTR action_name_dlg_command(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx);


/* page proc alist */
struct uint_ptr_pair action_name_dlg[] = {
    {WM_INITDIALOG, action_name_dlg_init_dialog},
    {WM_COMMAND, action_name_dlg_command},

    {0, NULL}
};


/* implementations */
static INT_PTR action_name_dlg_init_dialog(HWND hwnd, UINT msg,
                                           WPARAM wparam, LPARAM lparam,
                                           prop_page_context_t *ctx)
{
    struct uint_ptr_pair id_item_map[] = {
        {IDC_EDIT_NEW_ACTION_NAME, &ctx->action_name.name},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);

    return TRUE;
}

static INT_PTR action_name_dlg_command(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx)
{
    WORD code, id;
    HWND ctrl;

    code = HIWORD(wparam);
    id = LOWORD(wparam);
    ctrl = (HWND)lparam;

    if((id == IDOK) && (code == BN_CLICKED)) {
        LPTSTR name;
        int len;
        int i;

        len = GetWindowTextLength(ctx->action_name.name);
        if(len <= 0) {
            display_message_box(IDS_MSG_ENTER_ACTION_NAME,
                                IDS_MSG_ERROR,
                                hwnd,
                                MB_OK | MB_ICONSTOP);
            SetFocus(ctx->action_name.name);
            return TRUE;
        }

        name = (LPTSTR)malloc(sizeof(TCHAR) * (len + 1));
        if(name == NULL) {
            return FALSE;
        }

        GetWindowText(ctx->action_name.name, name, len + 1);

        for(i = 0; i < ctx->data->conf.num_action_list; i++) {
            if(lstrcmp(name, ctx->data->conf.action_list[i].name) == 0) {
                /* duplicated name */
                display_message_box(IDS_MSG_ENTER_ACTION_NAME,
                                    IDS_MSG_ERROR,
                                    hwnd,
                                    MB_OK | MB_ICONSTOP);
                SetFocus(ctx->action_name.name);
                SendMessage(ctx->action_name.name, EM_SETSEL, 0, -1);

                free(name);
                return TRUE;
            }
        }

        ctx->action_name.str_name = name;

        EndDialog(hwnd, TRUE);
    }
    if((id == IDCANCEL) && (code == BN_CLICKED)) {
        EndDialog(hwnd, FALSE);
    }

    return FALSE;
}
