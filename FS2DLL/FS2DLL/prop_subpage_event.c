/*
 * prop_subpage_event.c  -- event subpage
 *
 * $Id: prop_subpage_event.c,v 1.17 2004/03/24 17:48:50 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


/* page proc alist */
struct uint_ptr_pair event_subpage[];
struct uint_ptr_pair command_proc_map[];
struct uint_ptr_pair notify_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR event_init_dialog(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam,
                                 prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_GRP_EVENT, &ctx->event.group},

        {IDC_LIST_EVENT, &ctx->event.list},

        {IDC_BTN_ADD_EVENT, &ctx->event.add},
        {IDC_BTN_DEL_EVENT, &ctx->event.del},
        {IDC_BTN_EDT_EVENT, &ctx->event.edit},
        {IDC_BTN_UP_EVENT, &ctx->event.up},
        {IDC_BTN_DN_EVENT, &ctx->event.down},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = command_proc_map;
    ctx->notify_proc = notify_proc_map;

    {
        TCHAR str[256];
        RECT rect;
        int width;
        LVCOLUMN lvcol;
        int i;

        const static struct {
            UINT id;
            double w_ratio;
        } col_header[] = {
            {IDS_EVENT_MODIFIER, 0.4},
            {IDS_EVENT_ACTION, 0.6},

            {0, 0}
        };

        ListView_SetExtendedListViewStyle(ctx->event.list,
                                          LVS_EX_FULLROWSELECT);

        GetClientRect(ctx->event.list, &rect);
        width = (rect.right - rect.left) - 20;

        for(i = 0; col_header[i].id != 0; i++) {
            memset(str, 0, sizeof(str));
            LoadString(instance, col_header[i].id, str, 255);

            memset(&lvcol, 0, sizeof(lvcol));
            lvcol.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            lvcol.pszText = str;
            lvcol.iSubItem = i;
            lvcol.cx = width * col_header[i].w_ratio;
            ListView_InsertColumn(ctx->event.list, i, &lvcol);
        }
    }

    {
        struct {
            UINT id;
            HBITMAP *bmp;
            HWND btn;
        } bmp_map[] = {
            {IDB_ARROW_UP, &ctx->event.up_bmp, ctx->event.up},
            {IDB_ARROW_DN, &ctx->event.dn_bmp, ctx->event.down},

            {0, NULL}
        };

        for(i = 0; bmp_map[i].id != 0; i++) {
            *bmp_map[i].bmp =
                LoadBitmap(instance, MAKEINTRESOURCE(bmp_map[i].id));
            SendMessage(bmp_map[i].btn, BM_SETIMAGE,
                        IMAGE_BITMAP, (LPARAM)*bmp_map[i].bmp);
        }
    }

    return TRUE;
}

/* WM_DESTROY */
static INT_PTR event_destroy(HWND hwnd, UINT msg,
                             WPARAM wparam, LPARAM lparam,
                             prop_page_context_t *ctx)
{
    int i;

    struct {
        UINT id;
        HBITMAP bmp;
    } bmp_map[] = {
        {IDB_ARROW_UP, ctx->event.up_bmp},
        {IDB_ARROW_DN, ctx->event.dn_bmp},

        {0, NULL}
    };

    for(i = 0; bmp_map[i].id != 0; i++) {
        DeleteObject(bmp_map[i].bmp);
    }

    return TRUE;
}

static void update_event_button(prop_page_context_t *ctx)
{
    int i, idx;
    BOOL enable;
    HWND btn[] = {
        ctx->event.del,
        ctx->event.edit,
        ctx->event.up,
        ctx->event.down,

        NULL
    };

    idx = ListView_GetNextItem(ctx->event.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        enable = FALSE;
    } else {
        enable = TRUE;
    }

    for(i = 0; btn[i] != NULL; i++) {
        EnableWindow(btn[i], enable);
    }

    if(idx > 0) {
        EnableWindow(ctx->event.up, TRUE);
    } else {
        EnableWindow(ctx->event.up, FALSE);
    }

    if(idx >= 0 && idx < ctx->event.event->num_mod - 1) {
        EnableWindow(ctx->event.down, TRUE);
    } else {
        EnableWindow(ctx->event.down, FALSE);
    }
}

/* WM_SUBPAGE_CHANGE */
static INT_PTR event_change(HWND hwnd, UINT msg,
                            WPARAM wparam, LPARAM lparam,
                            prop_page_context_t *ctx)
{
    UINT grp;
    struct controller_event_config *evt;
    int i;
    struct uint_ptr_pair evt_map[] = {
        {ITEM_TYPE_CAT_VERTICAL, ctx->data->conf.motion_event},
        {ITEM_TYPE_CAT_HORIZONTAL, ctx->data->conf.motion_event},
        {ITEM_TYPE_CAT_MOTION, ctx->data->conf.motion_event},
        {ITEM_TYPE_CAT_PRESS, ctx->data->conf.press_event},
        {ITEM_TYPE_CAT_RELEASE, ctx->data->conf.release_event},
        {ITEM_TYPE_CAT_LOWER_IN, ctx->data->conf.lower_in_event},
        {ITEM_TYPE_CAT_LOWER_OUT, ctx->data->conf.lower_out_event},
        {ITEM_TYPE_CAT_UPPER_IN, ctx->data->conf.upper_in_event},
        {ITEM_TYPE_CAT_UPPER_OUT, ctx->data->conf.upper_out_event},

        {0, NULL}
    };
    const static struct uint_ptr_pair grp_map[] = {
        {ITEM_TYPE_CAT_VERTICAL, (void *)IDS_EVENT_V_MOTION},
        {ITEM_TYPE_CAT_HORIZONTAL, (void *)IDS_EVENT_H_MOTION},
        {ITEM_TYPE_CAT_MOTION, (void *)IDS_EVENT_MOTION},
        {ITEM_TYPE_CAT_PRESS, (void *)IDS_EVENT_PRESS},
        {ITEM_TYPE_CAT_RELEASE, (void *)IDS_EVENT_RELEASE},
        {ITEM_TYPE_CAT_LOWER_IN, (void *)IDS_EVENT_ENTER},
        {ITEM_TYPE_CAT_LOWER_OUT, (void *)IDS_EVENT_LEAVE},
        {ITEM_TYPE_CAT_UPPER_IN, (void *)IDS_EVENT_ENTER},
        {ITEM_TYPE_CAT_UPPER_OUT, (void *)IDS_EVENT_LEAVE},

        {0, NULL}
    };

    if(ctx->event.item == NULL) {
        return FALSE;
    }

    grp = (UINT)assq_pair(grp_map, ctx->event.type, (void *)-1);
    evt = (struct controller_event_config *)assq_pair(evt_map, ctx->event.type,
                                                      NULL);
    if(grp == (UINT)((void *)-1) || evt == NULL) {
        return FALSE;
    }

    {
        TCHAR buf[256];

        memset(buf, 0, sizeof(buf));
        LoadString(instance, grp, buf, 255);

        SetWindowText(ctx->event.group, buf);
    }

    {
        LVITEM lvitem;
        WCHAR str[256];
        int idx;

        ListView_DeleteAllItems(ctx->event.list);

        ctx->event.event = &evt[ctx->event.val];
        for(i = 0; i < ctx->event.event->num_mod; i++) {
            idx = ctx->event.event->mod[i].action_idx;

            modifier_to_string(str,
                               ctx->event.event->mod[i].mod,
                               ctx->event.event->mod[i].mod_mask);

            memset(&lvitem, 0, sizeof(lvitem));
            lvitem.mask = LVIF_TEXT | LVIF_PARAM;
            lvitem.iItem = i;
            lvitem.iSubItem = 0;
            lvitem.pszText = str;
            lvitem.lParam = i;
            ListView_InsertItem(ctx->event.list, &lvitem);

            ListView_SetItemText(ctx->event.list, i, 1,
                                 ctx->data->conf.action_list[idx].name);
        }

        update_event_button(ctx);
    }

    return TRUE;
}

/* BN_CLICKED for add */
static INT_PTR event_add_clicked(HWND hwnd,
                                 WORD code, WORD id, HWND ctrl,
                                 prop_page_context_t *ctx)
{
    int idx;
    prop_page_context_t dlg_ctx;
    INT_PTR ret;

    memcpy(&dlg_ctx, ctx, sizeof(dlg_ctx));
    dlg_ctx.page_proc = event_dlg;
    dlg_ctx.event_dlg.mod_conf.action_idx = (USHORT)-1;

    ret = DialogBoxParam(instance, MAKEINTRESOURCE(IDD_EVENT_EDIT),
                         ctx->page_hwnd, prop_subpage_proc,
                         (LPARAM)&dlg_ctx);
    if(ret == FALSE) {
        return TRUE;
    }

    {
        struct controller_modifier_config *mod;

        mod = (struct controller_modifier_config *)
              realloc(ctx->event.event->mod,
                      sizeof(struct controller_modifier_config) *
                      (ctx->event.event->num_mod + 1));
        if(mod == NULL) {
            return FALSE;
        }

        idx = ctx->event.event->num_mod;
        memcpy(&mod[idx], &dlg_ctx.event_dlg.mod_conf,
               sizeof(struct controller_modifier_config));

        ctx->event.event->mod = mod;
        ctx->event.event->num_mod += 1;
    }

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);
    ListView_SetItemState(ctx->event.list, idx,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->event.list, idx, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for del */
static INT_PTR event_del_clicked(HWND hwnd,
                                 WORD code, WORD id, HWND ctrl,
                                 prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->event.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        return FALSE;
    }

    if(display_message_box(IDS_MSG_CONFIRM_DEL_EVENT_TRG,
                           IDS_MSG_DEL_EVENT_TRG,
                           ctx->page_hwnd,
                           MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return TRUE;
    }

    if(idx < ctx->event.event->num_mod - 1) {
        memmove(&ctx->event.event->mod[idx],
                &ctx->event.event->mod[idx + 1],
                sizeof(struct controller_modifier_config) *
                (ctx->event.event->num_mod - 1 - idx));
    }
    ctx->event.event->num_mod -= 1;

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    SetFocus(ctx->event.list);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

static INT_PTR event_edit_selected(prop_page_context_t *ctx)
{
    int idx;
    prop_page_context_t dlg_ctx;
    INT_PTR ret;

    idx = ListView_GetNextItem(ctx->event.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        return FALSE;
    }

    memcpy(&dlg_ctx, ctx, sizeof(dlg_ctx));
    dlg_ctx.page_proc = event_dlg;
    memcpy(&dlg_ctx.event_dlg.mod_conf, &ctx->event.event->mod[idx],
           sizeof(struct controller_modifier_config));

    ret = DialogBoxParam(instance, MAKEINTRESOURCE(IDD_EVENT_EDIT),
                         ctx->page_hwnd, prop_subpage_proc,
                         (LPARAM)&dlg_ctx);
    if(ret == FALSE) {
        return TRUE;
    }

    memcpy(&ctx->event.event->mod[idx], &dlg_ctx.event_dlg.mod_conf,
           sizeof(struct controller_modifier_config));

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);
    ListView_SetItemState(ctx->event.list, idx,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->event.list, idx, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for edit */
static INT_PTR event_edt_clicked(HWND hwnd,
                                 WORD code, WORD id, HWND ctrl,
                                 prop_page_context_t *ctx)
{
    return event_edit_selected(ctx);
}

static void swap_event(int idx1, int idx2, prop_page_context_t *ctx)
{
    struct controller_modifier_config tm;

    tm = ctx->event.event->mod[idx1];
    ctx->event.event->mod[idx1] = ctx->event.event->mod[idx2];
    ctx->event.event->mod[idx2] = tm;
}

/* BN_CLICKED for up */
static INT_PTR event_up_clicked(HWND hwnd,
                                WORD code, WORD id, HWND ctrl,
                                prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->event.list, -1, LVNI_SELECTED);
    if(idx < 0 || idx == 0) {
        return FALSE;
    }

    swap_event(idx, idx - 1, ctx);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->event.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->event.list, idx - 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->event.list, idx - 1, FALSE);

    SetFocus(ctx->event.list);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for dn */
static INT_PTR event_dn_clicked(HWND hwnd,
                                WORD code, WORD id, HWND ctrl,
                                prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->event.list, -1, LVNI_SELECTED);
    if(idx < 0 || idx >= ctx->event.event->num_mod - 1) {
        return FALSE;
    }

    swap_event(idx, idx + 1, ctx);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->event.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->event.list, idx + 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->event.list, idx + 1, FALSE);

    SetFocus(ctx->event.list);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* LVN_ITEMCHANGED */
static INT_PTR event_list_changed(HWND hwnd, NMHDR *nmh,
                                  prop_page_context_t *ctx)
{
    NMLISTVIEW *nmlv;

    nmlv = (NMLISTVIEW *)nmh;

    if(! (nmlv->uChanged & LVIF_STATE)) {
        return FALSE;
    }

    update_event_button(ctx);

    return TRUE;
}

/* NM_DBLCLK */
static INT_PTR event_list_dblclk(HWND hwnd, NMHDR *nmh,
                                       prop_page_context_t *ctx)
{
    return event_edit_selected(ctx);
}


/* page proc alist */
struct uint_ptr_pair event_subpage[] = {
    {WM_INITDIALOG, event_init_dialog},
    {WM_DESTROY, event_destroy},
    {WM_SUBPAGE_CHANGE, event_change},
    {WM_COMMAND, window_command_proc},
    {WM_NOTIFY, window_notify_proc},

    {0, NULL}
};


/* WM_COMMAND proc */
static struct uint_ptr_pair add_proc_map[] = {
    {BN_CLICKED, event_add_clicked},

    {0, NULL}
};
static struct uint_ptr_pair del_proc_map[] = {
    {BN_CLICKED, event_del_clicked},

    {0, NULL}
};
static struct uint_ptr_pair edt_proc_map[] = {
    {BN_CLICKED, event_edt_clicked},

    {0, NULL}
};
static struct uint_ptr_pair up_proc_map[] = {
    {BN_CLICKED, event_up_clicked},

    {0, NULL}
};
static struct uint_ptr_pair dn_proc_map[] = {
    {BN_CLICKED, event_dn_clicked},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] ={
    {IDC_BTN_ADD_EVENT, &add_proc_map},
    {IDC_BTN_DEL_EVENT, &del_proc_map},
    {IDC_BTN_EDT_EVENT, &edt_proc_map},
    {IDC_BTN_UP_EVENT, &up_proc_map},
    {IDC_BTN_DN_EVENT, &dn_proc_map},

    {0, NULL}
};


/* WM_NOTIFY proc */
static struct uint_ptr_pair list_proc[] = {
    {LVN_ITEMCHANGED, event_list_changed},
    {NM_DBLCLK, event_list_dblclk},

    {0, NULL}
};
static struct uint_ptr_pair notify_proc_map[] = {
    {IDC_LIST_EVENT, &list_proc},

    {0, NULL}
};
