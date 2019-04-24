/*
 * prop_dialog_event.c  -- event dialog to select action list
 *
 * $Id: prop_dialog_event.c,v 1.6 2004/03/04 08:36:53 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


/* prototypes */
static INT_PTR event_dlg_init_dialog(HWND hwnd, UINT msg,
                                     WPARAM wparam, LPARAM lparam,
                                     prop_page_context_t *ctx);
static INT_PTR event_dlg_command(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam,
                                 prop_page_context_t *ctx);


/* page proc alist */
struct uint_ptr_pair event_dlg[] = {
    {WM_INITDIALOG, event_dlg_init_dialog},
    {WM_COMMAND, event_dlg_command},

    {0, NULL}
};


/* implementations */
static INT_PTR event_dlg_init_dialog(HWND hwnd, UINT msg,
                                     WPARAM wparam, LPARAM lparam,
                                     prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CHK_EVENT_MOD(1), &ctx->event_dlg.mod[0]},
        {IDC_CHK_EVENT_MOD(2), &ctx->event_dlg.mod[1]},
        {IDC_CHK_EVENT_MOD(3), &ctx->event_dlg.mod[2]},
        {IDC_CHK_EVENT_MOD(4), &ctx->event_dlg.mod[3]},
        {IDC_CHK_EVENT_MOD(5), &ctx->event_dlg.mod[4]},
        {IDC_CHK_EVENT_MOD(6), &ctx->event_dlg.mod[5]},
        {IDC_CHK_EVENT_MOD(7), &ctx->event_dlg.mod[6]},
        {IDC_CHK_EVENT_MOD(8), &ctx->event_dlg.mod[7]},

        {IDC_CHK_EVENT_MOD(9), &ctx->event_dlg.mod[8]},
        {IDC_CHK_EVENT_MOD(10), &ctx->event_dlg.mod[9]},
        {IDC_CHK_EVENT_MOD(11), &ctx->event_dlg.mod[10]},
        {IDC_CHK_EVENT_MOD(12), &ctx->event_dlg.mod[11]},
        {IDC_CHK_EVENT_MOD(13), &ctx->event_dlg.mod[12]},
        {IDC_CHK_EVENT_MOD(14), &ctx->event_dlg.mod[13]},
        {IDC_CHK_EVENT_MOD(15), &ctx->event_dlg.mod[14]},
        {IDC_CHK_EVENT_MOD(16), &ctx->event_dlg.mod[15]},

        {IDC_CMB_EVENT_ACTION, &ctx->event_dlg.action},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);

    /* set checkbox */
    set_modifier_bits_to_checkbox(ctx->event_dlg.mod,
                                  ctx->event_dlg.mod_conf.mod,
                                  ctx->event_dlg.mod_conf.mod_mask);

    /* set combobox */
    for(i = 0; i < ctx->data->conf.num_action_list; i++) {
        SendMessage(ctx->event_dlg.action, CB_INSERTSTRING,
                    i, (LPARAM)ctx->data->conf.action_list[i].name);
    }

    SendMessage(ctx->event_dlg.action, CB_SETCURSEL,
                ctx->event_dlg.mod_conf.action_idx, 0);

    return TRUE;
}

static INT_PTR event_dlg_command(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam,
                                 prop_page_context_t *ctx)
{
    WORD code, id;
    HWND ctrl;

    code = HIWORD(wparam);
    id = LOWORD(wparam);
    ctrl = (HWND)lparam;

    if((id == IDOK) && (code == BN_CLICKED)) {
        /* get modifier */
        get_modifier_bits_from_checkbox(ctx->event_dlg.mod,
                                        &ctx->event_dlg.mod_conf.mod,
                                        &ctx->event_dlg.mod_conf.mod_mask);

        /* get action index */
        ctx->event_dlg.mod_conf.action_idx =
            (USHORT)SendMessage(ctx->event_dlg.action, CB_GETCURSEL, 0, 0);
        if(ctx->event_dlg.mod_conf.action_idx == (USHORT)CB_ERR) {
            display_message_box(IDS_MSG_ERR_SELECT_ACTION,
                                IDS_MSG_ERROR,
                                ctx->page_hwnd,
                                MB_OK | MB_ICONSTOP);
            SetFocus(ctx->event_dlg.action);
            return TRUE;
        }

        EndDialog(hwnd, TRUE);
    }
    if((id == IDCANCEL) && (code == BN_CLICKED)) {
        EndDialog(hwnd, FALSE);
    }

    return FALSE;
}
