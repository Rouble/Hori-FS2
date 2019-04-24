/*
 * prop_subpage_actionlist.c  -- action list subpage
 *
 * $Id: prop_subpage_actionlist.c,v 1.15 2004/03/24 04:41:39 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


/* prototypes */
static void update_action_list_button(prop_page_context_t *ctx);
static int get_selected_action_index(prop_page_context_t *ctx);
static int action_index_ref_count(prop_page_context_t *ctx, int act_idx);
static void delete_action_list(prop_page_context_t *ctx, int act_idx);
static void swap_action_list(prop_page_context_t *ctx, int idx1, int idx2);
static INT_PTR action_list_edit_selected(prop_page_context_t *ctx);


/* page proc alist */
struct uint_ptr_pair action_list_subpage[];
struct uint_ptr_pair command_proc_map[];
struct uint_ptr_pair notify_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR action_list_init_dialog(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx)
{
    int i;

    struct uint_ptr_pair id_item_map[] = {
        {IDC_LIST_ACTION_LIST, &ctx->action_list.list},

        {IDC_BTN_ADD_ACTION_LIST, &ctx->action_list.add},
        {IDC_BTN_DEL_ACTION_LIST, &ctx->action_list.del},
        {IDC_BTN_EDT_ACTION_LIST, &ctx->action_list.edit},
        {IDC_BTN_UP_ACTION_LIST, &ctx->action_list.up},
        {IDC_BTN_DN_ACTION_LIST, &ctx->action_list.down},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = command_proc_map;
    ctx->notify_proc = notify_proc_map;

    /* set list header */
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
            {IDS_ACTION_NAME, 0.8},
            {IDS_ACTION_CNT, 0.2},

            {0, 0}
        };

        ListView_SetExtendedListViewStyle(ctx->action_list.list,
                                          LVS_EX_FULLROWSELECT);

        GetClientRect(ctx->action_list.list, &rect);
        width = (rect.right - rect.left) - 20;

        for(i = 0; col_header[i].id != 0; i++) {
            memset(str, 0, sizeof(str));
            LoadString(instance, col_header[i].id, str, 255);

            memset(&lvcol, 0, sizeof(lvcol));
            lvcol.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            lvcol.pszText = str;
            lvcol.iSubItem = i;
            lvcol.cx = width * col_header[i].w_ratio;
            ListView_InsertColumn(ctx->action_list.list, i, &lvcol);
        }
    }

    /* set button bitmap */
    {
        struct {
            UINT id;
            HBITMAP *bmp;
            HWND btn;
        } bmp_map[] = {
            {IDB_ARROW_UP, &ctx->action_list.up_bmp, ctx->action_list.up},
            {IDB_ARROW_DN, &ctx->action_list.dn_bmp, ctx->action_list.down},

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
static INT_PTR action_list_destroy(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam,
                                   prop_page_context_t *ctx)
{
    int i;

    struct {
        UINT id;
        HBITMAP bmp;
    } bmp_map[] = {
        {IDB_ARROW_UP, ctx->action_list.up_bmp},
        {IDB_ARROW_DN, ctx->action_list.dn_bmp},

        {0, NULL}
    };

    for(i = 0; bmp_map[i].id != 0; i++) {
        DeleteObject(bmp_map[i].bmp);
    }

    return TRUE;
}

/* WM_SUBPAGE_CHANGE */
static INT_PTR action_list_change(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx)
{
    int i;
    LVITEM lvitem;
    WCHAR str[256];

    ListView_DeleteAllItems(ctx->action_list.list);

    for(i = 0; i < ctx->data->conf.num_action_list; i++) {
        memset(&lvitem, 0, sizeof(lvitem));
        lvitem.mask = LVIF_TEXT | LVIF_PARAM;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.pszText = ctx->data->conf.action_list[i].name;
        lvitem.lParam = i;
        ListView_InsertItem(ctx->action_list.list, &lvitem);

        wsprintfW(str, L"%d", ctx->data->conf.action_list[i].num_action);
        ListView_SetItemText(ctx->action_list.list, i, 1, str);
    }

    update_action_list_button(ctx);

    return TRUE;
}

/* BN_CLICKED for add */
static INT_PTR action_list_add_clicked(HWND hwnd,
                                       WORD code, WORD id, HWND ctrl,
                                       prop_page_context_t *ctx)
{
    prop_page_context_t dlg_ctx;
    INT_PTR ret;

    memcpy(&dlg_ctx, ctx, sizeof(dlg_ctx));
    dlg_ctx.page_proc = action_name_dlg;

    ret = DialogBoxParam(instance, MAKEINTRESOURCE(IDD_ACTION_NAME),
                         ctx->page_hwnd, prop_subpage_proc,
                         (LPARAM)&dlg_ctx);
    if(ret == FALSE) {
        return TRUE;
    }

    {
        struct controller_action_config_list *act_list;
        int idx;

        act_list = (struct controller_action_config_list *)
                   realloc(ctx->data->conf.action_list,
                           sizeof(struct controller_action_config_list) *
                           (ctx->data->conf.num_action_list + 1));
        if(act_list == NULL) {
            free(dlg_ctx.action_name.str_name);
            return FALSE;
        }

        idx = ctx->data->conf.num_action_list;
        memset(&act_list[idx], 0, sizeof(act_list[idx]));
        act_list[idx].num_action = 0;
        act_list[idx].action = NULL;
        act_list[idx].name = dlg_ctx.action_name.str_name;
        act_list[idx].tree_item.name = dlg_ctx.action_name.str_name;
        act_list[idx].tree_item.type = ITEM_TYPE_ACTION;
        act_list[idx].tree_item.val1 = idx;
        act_list[idx].tree_item.handle = NULL;

        ctx->data->conf.action_list = act_list;
        ctx->data->conf.num_action_list += 1;

        SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                    MAIN_TREE_ADD_ITEM, (LPARAM)&act_list[idx].tree_item);

        SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);
        ListView_SetItemState(ctx->action_list.list, idx,
                              LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(ctx->action_list.list, idx, FALSE);
    }

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for del */
static INT_PTR action_list_del_clicked(HWND hwnd,
                                       WORD code, WORD id, HWND ctrl,
                                       prop_page_context_t *ctx)
{
    int act_idx;

    act_idx = get_selected_action_index(ctx);
    if(act_idx < 0) {
        return FALSE;
    }

    if(action_index_ref_count(ctx, act_idx) > 0) {
        display_message_box(IDS_MSG_ACTION_LIST_USED, IDS_MSG_ERROR,
                            ctx->page_hwnd,
                            MB_OK | MB_ICONSTOP);
        return TRUE;
    }

    if(display_message_box(IDS_MSG_CONFIRM_DEL_ACTION_LIST,
                           IDS_MSG_DEL_ACTION_LIST,
                           ctx->page_hwnd,
                           MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return TRUE;
    }

    /* delete from tree */
    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                MAIN_TREE_DELETE,
                (LPARAM)&ctx->data->conf.action_list[act_idx].tree_item);

    /* delete from conf data */
    delete_action_list(ctx, act_idx);

    /* delete from action list */
    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    SetFocus(ctx->action_list.list);
    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for edit */
static INT_PTR action_list_edt_clicked(HWND hwnd,
                                       WORD code, WORD id, HWND ctrl,
                                       prop_page_context_t *ctx)
{
    return action_list_edit_selected(ctx);
}

/* BN_CLICKED for up */
static INT_PTR action_list_up_clicked(HWND hwnd,
                                      WORD code, WORD id, HWND ctrl,
                                      prop_page_context_t *ctx)
{
    int idx;

    idx = get_selected_action_index(ctx);
    if(idx < 0 || idx == 0) {
        return FALSE;
    }

    swap_action_list(ctx, idx, idx - 1);

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                MAIN_TREE_REBUILD_ACTION, 0);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->action_list.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->action_list.list, idx - 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action_list.list, idx - 1, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    SetFocus(ctx->action_list.list);

    return TRUE;
}

/* BN_CLICKED for dn */
static INT_PTR action_list_dn_clicked(HWND hwnd,
                                      WORD code, WORD id, HWND ctrl,
                                      prop_page_context_t *ctx)
{
    int idx;

    idx = get_selected_action_index(ctx);
    if(idx < 0 || idx >= ctx->data->conf.num_action_list - 1) {
        return FALSE;
    }

    swap_action_list(ctx, idx + 1, idx);

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                MAIN_TREE_REBUILD_ACTION, 0);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->action_list.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->action_list.list, idx + 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action_list.list, idx + 1, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    SetFocus(ctx->action_list.list);

    return TRUE;
}


/* LVN_ITEMCHANGED */
static INT_PTR action_list_list_changed(HWND hwnd, NMHDR *nmh,
                                        prop_page_context_t *ctx)
{
    NMLISTVIEW *nmlv;

    nmlv = (NMLISTVIEW *)nmh;

    if(! (nmlv->uChanged & LVIF_STATE)) {
        return FALSE;
    }

    update_action_list_button(ctx);

    return TRUE;
}

/* NM_DBLCLK */
static INT_PTR action_list_list_dblclk(HWND hwnd, NMHDR *nmh,
                                       prop_page_context_t *ctx)
{
    return action_list_edit_selected(ctx);
}


static void update_action_list_button(prop_page_context_t *ctx)
{
    int i, idx;
    BOOL enable;
    HWND btn[] = {
        ctx->action_list.del,
        ctx->action_list.edit,
        ctx->action_list.up,
        ctx->action_list.down,

        NULL
    };

    idx = ListView_GetNextItem(ctx->action_list.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        enable = FALSE;
    } else {
        enable = TRUE;
    }

    for(i = 0; btn[i] != NULL; i++) {
        EnableWindow(btn[i], enable);
    }

    if(idx > 0) {
        EnableWindow(ctx->action_list.up, TRUE);
    } else {
        EnableWindow(ctx->action_list.up, FALSE);
    }

    if(idx >= 0 && idx < ctx->data->conf.num_action_list - 1) {
        EnableWindow(ctx->action_list.down, TRUE);
    } else {
        EnableWindow(ctx->action_list.down, FALSE);
    }
}


static int get_selected_action_index(prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->action_list.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        return -1;
    }

    {
        LVITEM item;
        int act_idx;

        item.mask = LVIF_PARAM;
        item.iItem = idx;
        item.iSubItem = 0;

        ListView_GetItem(ctx->action_list.list, &item);

        act_idx = (int)item.lParam;

        return act_idx;
    }
}


typedef void (*action_idx_processor_t)(USHORT *, void *);
static void map_for_each_action(prop_page_context_t *ctx,
                                action_idx_processor_t proc,
                                void *proc_ctx)
{
    int i, j, k;

    struct controller_event_config *event[] = {
        ctx->data->conf.motion_event,
        ctx->data->conf.press_event,
        ctx->data->conf.release_event,
        ctx->data->conf.lower_in_event,
        ctx->data->conf.lower_out_event,
        ctx->data->conf.upper_in_event,
        ctx->data->conf.upper_out_event,

        NULL
    };

    for(i = 0; event[i] != NULL; i++) {
        for(j = 0; j < CONTROLLER_NUM_EVENT; j++) {
            for(k = 0; k < event[i][j].num_mod; k++) {
                proc(&event[i][j].mod[k].action_idx, proc_ctx);
            }
        }
    }

    for(i = 0; i < ctx->data->conf.num_action_list; i++) {
        for(j = 0; j < ctx->data->conf.action_list[i].num_action; j++) {
            if(ctx->data->conf.action_list[i].action[j].type ==
               CONTROLLER_ACTION_APPLY) {
                proc(
                    &ctx->data->conf.action_list[i].action[j].apply.action_idx,
                    proc_ctx);
            }
        }
    }
}


struct action_index_ref_count_ctx{
    USHORT action_idx;
    int count;
};

static void action_index_ref_count_proc(USHORT *action_idx, void *proc_ctx)
{
    struct action_index_ref_count_ctx *ctx;

    ctx = (struct action_index_ref_count_ctx *)proc_ctx;

    if(ctx->action_idx == *action_idx) {
        ctx->count += 1;
    }
}

static int action_index_ref_count(prop_page_context_t *ctx, int act_idx)
{
    struct action_index_ref_count_ctx proc_ctx;

    proc_ctx.action_idx = act_idx;
    proc_ctx.count = 0;
    map_for_each_action(ctx, action_index_ref_count_proc, &proc_ctx);

    return proc_ctx.count;
}


static void delete_action_proc(USHORT *action_idx, void *proc_ctx)
{
    int target_idx;

    target_idx = *((int *)proc_ctx);

    if(*action_idx > target_idx) {
        (*action_idx) += -1;
    }
}

static void delete_action_list(prop_page_context_t *ctx, int act_idx)
{
    map_for_each_action(ctx, delete_action_proc, &act_idx);

    if(act_idx < ctx->data->conf.num_action_list - 1) {
        memmove(&ctx->data->conf.action_list[act_idx],
                &ctx->data->conf.action_list[act_idx + 1],
                sizeof(struct controller_action_config_list) *
                (ctx->data->conf.num_action_list - 1 - act_idx));
    }
    ctx->data->conf.num_action_list -= 1;
}


static void swap_action_proc(USHORT *action_idx, void *proc_ctx)
{
    int *idx;

    idx = (int *)proc_ctx;

    if(*action_idx == idx[0]) {
        *action_idx = idx[1];
    } else if(*action_idx == idx[1]) {
        *action_idx = idx[0];
    }
}

static void swap_action_list(prop_page_context_t *ctx, int idx1, int idx2)
{
    int idx[2];
    struct controller_action_config_list act_list;

    idx[0] = idx1;
    idx[1] = idx2;

    /* swap action index for each event/apply-action */
    map_for_each_action(ctx, swap_action_proc, idx);

    /* swap action list data */
    act_list = ctx->data->conf.action_list[idx1];
    ctx->data->conf.action_list[idx1] = ctx->data->conf.action_list[idx2];
    ctx->data->conf.action_list[idx2] = act_list;
}


static INT_PTR action_list_edit_selected(prop_page_context_t *ctx)
{
    tree_item_spec_t *titem;
    int act_idx;

    act_idx = get_selected_action_index(ctx);
    if(act_idx < 0) {
        return FALSE;
    }

    titem = &ctx->data->conf.action_list[act_idx].tree_item;

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                MAIN_TREE_SELECT, (LPARAM)titem);

    return TRUE;
}


/* page proc alist */
struct uint_ptr_pair action_list_subpage[] = {
    {WM_INITDIALOG, action_list_init_dialog},
    {WM_DESTROY, action_list_destroy},
    {WM_SUBPAGE_CHANGE, action_list_change},
    {WM_COMMAND, window_command_proc},
    {WM_NOTIFY, window_notify_proc},

    {0, NULL}
};


/* WM_COMMAND */
static struct uint_ptr_pair add_proc_map[] = {
    {BN_CLICKED, action_list_add_clicked},

    {0, NULL}
};
static struct uint_ptr_pair del_proc_map[] = {
    {BN_CLICKED, action_list_del_clicked},

    {0, NULL}
};
static struct uint_ptr_pair edt_proc_map[] = {
    {BN_CLICKED, action_list_edt_clicked},

    {0, NULL}
};
static struct uint_ptr_pair up_proc_map[] = {
    {BN_CLICKED, action_list_up_clicked},

    {0, NULL}
};
static struct uint_ptr_pair dn_proc_map[] = {
    {BN_CLICKED, action_list_dn_clicked},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] ={
    {IDC_BTN_ADD_ACTION_LIST, &add_proc_map},
    {IDC_BTN_DEL_ACTION_LIST, &del_proc_map},
    {IDC_BTN_EDT_ACTION_LIST, &edt_proc_map},
    {IDC_BTN_UP_ACTION_LIST, &up_proc_map},
    {IDC_BTN_DN_ACTION_LIST, &dn_proc_map},

    {0, NULL}
};


/* WM_NOTIFY */
static struct uint_ptr_pair list_proc_map[] = {
    {LVN_ITEMCHANGED, action_list_list_changed},
    {NM_DBLCLK, action_list_list_dblclk},

    {0, NULL}
};
static struct uint_ptr_pair notify_proc_map[] = {
    {IDC_LIST_ACTION_LIST, &list_proc_map},

    {0, NULL}
};
