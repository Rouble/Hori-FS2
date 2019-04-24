/*
 * prop_subpage_action.c  -- action subpage
 *
 * $Id: prop_subpage_action.c,v 1.27 2004/03/25 05:15:05 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


/* page proc alist */
struct uint_ptr_pair action_subpage[];
struct uint_ptr_pair command_proc_map[];
struct uint_ptr_pair notify_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR action_init_dialog(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_EDIT_ACTION_NAME, &ctx->action.name},

        {IDC_LIST_ACTION, &ctx->action.list},

        {IDC_BTN_ADD_ACTION, &ctx->action.add},
        {IDC_BTN_DEL_ACTION, &ctx->action.del},
        {IDC_BTN_EDT_ACTION, &ctx->action.edit},
        {IDC_BTN_UP_ACTION, &ctx->action.up},
        {IDC_BTN_DN_ACTION, &ctx->action.down},

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

        const static struct {
            UINT id;
            double w_ratio;
        } col_header[] = {
            {IDS_ACTION_TYPE, 0.3},
            {IDS_ACTION_VALUE, 0.7},

            {0, 0}
        };

        ListView_SetExtendedListViewStyle(ctx->action.list,
                                          LVS_EX_FULLROWSELECT);

        GetClientRect(ctx->action.list, &rect);
        width = (rect.right - rect.left) - 20;

        for(i = 0; col_header[i].id != 0; i++) {
            memset(str, 0, sizeof(str));
            LoadString(instance, col_header[i].id, str, 255);

            memset(&lvcol, 0, sizeof(lvcol));
            lvcol.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            lvcol.pszText = str;
            lvcol.iSubItem = i;
            lvcol.cx = width * col_header[i].w_ratio;
            ListView_InsertColumn(ctx->action.list, i, &lvcol);
        }
    }

    /* set button bitmap */
    {
        struct {
            UINT id;
            HBITMAP *bmp;
            HWND btn;
        } bmp_map[] = {
            {IDB_ARROW_UP, &ctx->action.up_bmp, ctx->action.up},
            {IDB_ARROW_DN, &ctx->action.dn_bmp, ctx->action.down},

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
static INT_PTR action_destroy(HWND hwnd, UINT msg,
                              WPARAM wparam, LPARAM lparam,
                              prop_page_context_t *ctx)
{
    int i;

    struct {
        UINT id;
        HBITMAP bmp;
    } bmp_map[] = {
        {IDB_ARROW_UP, ctx->action.up_bmp},
        {IDB_ARROW_DN, ctx->action.dn_bmp},

        {0, NULL}
    };

    for(i = 0; bmp_map[i].id != 0; i++) {
        DeleteObject(bmp_map[i].bmp);
    }

    return TRUE;
}

/* default: null string */
static void gen_str_null(LPWSTR dst,
                         struct controller_action_config *action,
                         prop_page_context_t *ctx)
{
    dst[0] = 0;
}

/* modifier */
static void gen_str_mod(LPWSTR dst,
                        struct controller_action_config *action,
                        prop_page_context_t *ctx)
{
    modifier_to_string(dst, action->mod.mod, action->mod.mod_mask);
}

/* delay */
static void gen_str_delay(LPWSTR dst,
                          struct controller_action_config *action,
                          prop_page_context_t *ctx)
{
    wsprintfW(dst, L"%u milliseconds", action->delay.msec);
}

/* joystick */
static void gen_str_joy(LPWSTR dst,
                        struct controller_action_config *action,
                        prop_page_context_t *ctx)
{
    LPWSTR code, flag;
    WCHAR code_buf[64], buf[256];

    flag = (LPWSTR)assq_pair(action_joy_flag_map, action->joy.flags, NULL);
    code = (LPWSTR)assq_pair(action_joy_code_map, action->joy.code, NULL);

    if(flag == NULL) {
        lstrcpyW(dst, L"unknown");
        return;
    }

    if(code == NULL) {
        wsprintfW(code_buf, action_code_button_base,
                  action->joy.code - CONTROLLER_ACTION_JOY_CODE_BTN(0) + 1);
        code = code_buf;
    }

    wsprintfW(buf, L"%ws (%ws)", code, flag);

    switch(action->joy.flags) {
      case CONTROLLER_ACTION_JOY_VAL_SPEC:
          wsprintfW(dst, L"%ws: %d", buf, action->joy.val);
          break;

      case CONTROLLER_ACTION_JOY_VAL_RATIO:
          wsprintfW(dst, L"%ws: %d ... %d",
                    buf,
                    action->mou.offset, /* min */
                    0xff *      /* max */
                    action->mou.ratio_numerator /
                    action->mou.ratio_denominator +
                    action->mou.offset);
          break;

      default:
          lstrcpyW(dst, buf);
          break;
    }
}

/* apply action */
static void gen_str_apply(LPWSTR dst,
                          struct controller_action_config *action,
                          prop_page_context_t *ctx)
{
    WCHAR buf[32];

    modifier_to_string(buf, action->apply.mod, action->apply.mod_mask);
    wsprintfW(dst, L"%ws (modifier: %ws)",
              ctx->data->conf.action_list[action->apply.action_idx].name,
              buf);
}

/* mouse */
static void gen_str_mouse(LPWSTR dst,
                          struct controller_action_config *action,
                          prop_page_context_t *ctx)
{
    LPWSTR code, flag;
    WCHAR code_buf[64], buf[256];
    int v1, v2;

    flag = (LPWSTR)assq_pair(action_mou_flag_map, action->mou.flags, NULL);
    code = (LPWSTR)assq_pair(action_mou_code_map, action->mou.code, NULL);

    if(flag == NULL) {
        lstrcpyW(dst, L"unknown");
        return;
    }

    if(code == NULL) {
        wsprintfW(code_buf, action_code_button_base,
                  action->mou.code - CONTROLLER_ACTION_MOU_CODE_BTN(0) + 1);
        code = code_buf;
    }

    wsprintfW(buf, L"%ws (%ws)", code, flag);

    switch(action->mou.flags) {
      case CONTROLLER_ACTION_MOU_VAL_SPEC:
          v1 = action->mou.val;
          wsprintfW(dst, L"%ws: %d", buf,
                    v1 - (v1 <= 0x7f ? 0x7f : 0x80));
          break;

      case CONTROLLER_ACTION_MOU_VAL_RATIO:
          v1 = action->mou.offset;
          v2 = 0xff *
               action->mou.ratio_numerator / action->mou.ratio_denominator +
               action->mou.offset;
          wsprintfW(dst, L"%ws: %d ... %d",
                    buf,
                    v1 - (v1 <= 0x7f ? 0x7f : 0x80),
                    v2 - (v2 <= 0x7f ? 0x7f : 0x80));
          break;

      default:
          lstrcpyW(dst, buf);
          break;
    }
}

/* keyboard */
static void gen_str_kbd(LPWSTR dst,
                        struct controller_action_config *action,
                        prop_page_context_t *ctx)
{
    LPWSTR code, flag;

    flag = (LPWSTR)assq_pair(action_kbd_flag_map, action->kbd.flags, NULL);
    code = kbd_code_to_str(action->kbd.code);

    if(flag == NULL || code == NULL) {
        lstrcpyW(dst, L"unknown");
        return;
    }

    wsprintfW(dst, L"%ws (%ws)", code, flag);
}

static void update_action_button(prop_page_context_t *ctx)
{
    int i, idx;
    BOOL enable;
    HWND btn[] = {
        ctx->action.del,
        ctx->action.edit,
        ctx->action.up,
        ctx->action.down,

        NULL
    };

    idx = ListView_GetNextItem(ctx->action.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        enable = FALSE;
    } else {
        enable = TRUE;
    }

    for(i = 0; btn[i] != NULL; i++) {
        EnableWindow(btn[i], enable);
    }

    if(idx > 0) {
        EnableWindow(ctx->action.up, TRUE);
    } else {
        EnableWindow(ctx->action.up, FALSE);
    }

    if(idx >= 0 && idx < ctx->action.action->num_action - 1) {
        EnableWindow(ctx->action.down, TRUE);
    } else {
        EnableWindow(ctx->action.down, FALSE);
    }
}

/* WM_SUBPAGE_CHANGE */
static INT_PTR action_change(HWND hwnd, UINT msg,
                             WPARAM wparam, LPARAM lparam,
                             prop_page_context_t *ctx)
{
    int i;
    LVITEM lvitem;
    WCHAR str[256], *ptr;

    typedef void (*gen_str_func_t)(LPWSTR,
                                   struct controller_action_config *,
                                   prop_page_context_t *);

    gen_str_func_t gen_str;

    const static struct uint_ptr_pair type_name[] = {
        {CONTROLLER_ACTION_NOP, L"No operation"},
        {CONTROLLER_ACTION_MOD, L"Change modifier"},
        {CONTROLLER_ACTION_DELAY, L"Wait"},
        {CONTROLLER_ACTION_JOY, L"Joystick"},
        {CONTROLLER_ACTION_APPLY, L"Apply action"},
        {CONTROLLER_ACTION_MOU, L"Mouse"},
        {CONTROLLER_ACTION_KBD, L"Keyboard"},

        {0, NULL}
    };
    const static struct uint_ptr_pair type_gen_str[] = {
        {CONTROLLER_ACTION_MOD, gen_str_mod},
        {CONTROLLER_ACTION_DELAY, gen_str_delay},
        {CONTROLLER_ACTION_JOY, gen_str_joy},
        {CONTROLLER_ACTION_APPLY, gen_str_apply},
        {CONTROLLER_ACTION_MOU, gen_str_mouse},
        {CONTROLLER_ACTION_KBD, gen_str_kbd},

        {0, NULL}
    };

    ListView_DeleteAllItems(ctx->action.list);

    ctx->action.action = &ctx->data->conf.action_list[ctx->action.item->val1];

    SetWindowText(ctx->action.name, ctx->action.action->name);

    for(i = 0; i < ctx->action.action->num_action; i++) {
        ptr = (WCHAR *)assq_pair(type_name, ctx->action.action->action[i].type,
                                 L"Unknown");

        /* type of action */
        memset(&lvitem, 0, sizeof(lvitem));
        lvitem.mask = LVIF_TEXT | LVIF_PARAM;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.pszText = ptr;
        lvitem.lParam = i;
        ListView_InsertItem(ctx->action.list, &lvitem);

        /* value of action */
        gen_str = (gen_str_func_t)assq_pair(type_gen_str,
                                            ctx->action.action->action[i].type,
                                            gen_str_null);
        gen_str(str, &ctx->action.action->action[i], ctx);
        ListView_SetItemText(ctx->action.list, i, 1, str);
    }

    update_action_button(ctx);

    return TRUE;
}

/* EN_KILLFOCUS */
static INT_PTR action_name_killfocus(HWND hwnd,
                                     WORD code, WORD id, HWND ctrl,
                                     prop_page_context_t *ctx)
{
    if(ctx->action.item == NULL) {
        return FALSE;
    }

    {
        LPTSTR name;
        int len;
        int i;

        len = GetWindowTextLength(ctx->action.name);
        if(len <= 0) {
            /* zero-length */
            display_message_box(IDS_MSG_ENTER_ACTION_NAME, IDS_MSG_ERROR,
                                ctx->page_hwnd,
                                MB_OK | MB_ICONWARNING);
            SetWindowText(ctx->action.name, ctx->action.item->name);
            SetFocus(ctx->action.name);
            return TRUE;
        }

        name = (LPTSTR)malloc(sizeof(TCHAR) * (len + 1));
        if(name == NULL) {
            return FALSE;
        }

        GetWindowText(ctx->action.name, name, len + 1);

        for(i = 0; i < ctx->data->conf.num_action_list; i++) {
            if(ctx->action.action != &ctx->data->conf.action_list[i] &&
               lstrcmp(name, ctx->data->conf.action_list[i].name) == 0) {
                /* duplicated name */
                display_message_box(IDS_MSG_ENTER_ACTION_NAME, IDS_MSG_ERROR,
                                    ctx->page_hwnd,
                                    MB_OK | MB_ICONWARNING);
                SetWindowText(ctx->action.name, ctx->action.item->name);
                SetFocus(ctx->action.name);

                free(name);
                return TRUE;
            }
        }

        free(ctx->action.item->name);

        ctx->action.item->name = name;
        ctx->action.action->name = name;

        SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                    MAIN_TREE_SELECT, (LPARAM)ctx->action.item);

        PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);
    }

    return TRUE;
}

/* BN_CLICKED for add */
static INT_PTR action_add_clicked(HWND hwnd,
                                  WORD code, WORD id, HWND ctrl,
                                  prop_page_context_t *ctx)
{
    int idx;
    prop_page_context_t dlg_ctx;
    INT_PTR ret;

    memcpy(&dlg_ctx, ctx, sizeof(dlg_ctx));
    memset(&dlg_ctx.action_dlg, 0, sizeof(dlg_ctx.action_dlg));
    dlg_ctx.page_proc = action_dlg;
    dlg_ctx.action_dlg.action_conf.type = CONTROLLER_ACTION_NOP;

    ret = DialogBoxParam(instance, MAKEINTRESOURCE(IDD_ACTION_EDIT),
                         ctx->page_hwnd, prop_subpage_proc,
                         (LPARAM)&dlg_ctx);
    if(ret == FALSE) {
        return TRUE;
    }

    {
        struct controller_action_config *action;

        action = (struct controller_action_config *)
                 realloc(ctx->action.action->action,
                         sizeof(struct controller_action_config) *
                         (ctx->action.action->num_action + 1));
        if(action == NULL) {
            return FALSE;
        }

        idx = ctx->action.action->num_action;
        memcpy(&action[idx], &dlg_ctx.action_dlg.action_conf,
               sizeof(struct controller_action_config));

        ctx->action.action->action = action;
        ctx->action.action->num_action += 1;
    }

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);
    ListView_SetItemState(ctx->action.list, idx,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action.list, idx, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for del */
static INT_PTR action_del_clicked(HWND hwnd,
                                  WORD code, WORD id, HWND ctrl,
                                  prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->action.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        return FALSE;
    }

    if(display_message_box(IDS_MSG_CONFIRM_DEL_ACTION,
                           IDS_MSG_DEL_ACTION,
                           ctx->page_hwnd,
                           MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return TRUE;
    }

    if(idx < ctx->action.action->num_action - 1) {
        memmove(&ctx->action.action->action[idx],
                &ctx->action.action->action[idx + 1],
                sizeof(struct controller_action_config) *
                (ctx->action.action->num_action - 1 - idx));
    }
    ctx->action.action->num_action -= 1;

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    SetFocus(ctx->action.list);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

static INT_PTR action_edit_selected(prop_page_context_t *ctx)
{
    int idx;
    prop_page_context_t dlg_ctx;
    INT_PTR ret;

    idx = ListView_GetNextItem(ctx->action.list, -1, LVNI_SELECTED);
    if(idx < 0) {
        return FALSE;
    }

    memcpy(&dlg_ctx, ctx, sizeof(dlg_ctx));
    memset(&dlg_ctx.action_dlg, 0, sizeof(dlg_ctx.action_dlg));
    dlg_ctx.page_proc = action_dlg;
    memcpy(&dlg_ctx.action_dlg.action_conf, &ctx->action.action->action[idx],
           sizeof(struct controller_action_config));

    ret = DialogBoxParam(instance, MAKEINTRESOURCE(IDD_ACTION_EDIT),
                         ctx->page_hwnd, prop_subpage_proc,
                         (LPARAM)&dlg_ctx);
    if(ret == FALSE) {
        return TRUE;
    }

    memcpy(&ctx->action.action->action[idx], &dlg_ctx.action_dlg.action_conf,
           sizeof(struct controller_action_config));

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);
    ListView_SetItemState(ctx->action.list, idx,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action.list, idx, FALSE);

    SetFocus(ctx->action.list);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for edit */
static INT_PTR action_edt_clicked(HWND hwnd,
                                  WORD code, WORD id, HWND ctrl,
                                  prop_page_context_t *ctx)
{
    return action_edit_selected(ctx);
}

static void swap_action(int idx1, int idx2, prop_page_context_t *ctx)
{
    struct controller_action_config ta;

    ta = ctx->action.action->action[idx1];
    ctx->action.action->action[idx1] = ctx->action.action->action[idx2];
    ctx->action.action->action[idx2] = ta;
}

/* BN_CLICKED for up */
static INT_PTR action_up_clicked(HWND hwnd,
                                 WORD code, WORD id, HWND ctrl,
                                 prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->action.list, -1, LVNI_SELECTED);
    if(idx < 0 || idx == 0) {
        return FALSE;
    }

    swap_action(idx, idx - 1, ctx);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->action.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->action.list, idx - 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action.list, idx - 1, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    SetFocus(ctx->action.list);

    return TRUE;
}

/* BN_CLICKED for dn */
static INT_PTR action_dn_clicked(HWND hwnd,
                                 WORD code, WORD id, HWND ctrl,
                                 prop_page_context_t *ctx)
{
    int idx;

    idx = ListView_GetNextItem(ctx->action.list, -1, LVNI_SELECTED);
    if(idx < 0 || idx >= ctx->action.action->num_action - 1) {
        return FALSE;
    }

    swap_action(idx, idx + 1, ctx);

    SendMessage(ctx->subpage_hwnd, WM_SUBPAGE_CHANGE, 0, 0);

    ListView_SetItemState(ctx->action.list, idx,
                          0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(ctx->action.list, idx + 1,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ctx->action.list, idx + 1, FALSE);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    SetFocus(ctx->action.list);

    return TRUE;
}

/* LVN_ITEMCHANGED */
static INT_PTR action_list_changed(HWND hwnd, NMHDR *nmh,
                                   prop_page_context_t *ctx)
{
    NMLISTVIEW *nmlv;

    nmlv = (NMLISTVIEW *)nmh;

    if(! (nmlv->uChanged & LVIF_STATE)) {
        return FALSE;
    }

    update_action_button(ctx);

    return TRUE;
}

/* NM_DBLCLK */
static INT_PTR action_list_dblclk(HWND hwnd, NMHDR *nmh,
                                  prop_page_context_t *ctx)
{
    return action_edit_selected(ctx);
}


/* page proc alist */
struct uint_ptr_pair action_subpage[] = {
    {WM_INITDIALOG, action_init_dialog},
    {WM_DESTROY, action_destroy},
    {WM_SUBPAGE_CHANGE, action_change},
    {WM_COMMAND, window_command_proc},
    {WM_NOTIFY, window_notify_proc},

    {0, NULL}
};


/* WM_COMMAND proc */
static struct uint_ptr_pair name_proc_map[] = {
    {EN_KILLFOCUS, action_name_killfocus},

    {0, NULL}
};
static struct uint_ptr_pair add_proc_map[] = {
    {BN_CLICKED, action_add_clicked},

    {0, NULL}
};
static struct uint_ptr_pair del_proc_map[] = {
    {BN_CLICKED, action_del_clicked},

    {0, NULL}
};
static struct uint_ptr_pair edt_proc_map[] = {
    {BN_CLICKED, action_edt_clicked},

    {0, NULL}
};
static struct uint_ptr_pair up_proc_map[] = {
    {BN_CLICKED, action_up_clicked},

    {0, NULL}
};
static struct uint_ptr_pair dn_proc_map[] = {
    {BN_CLICKED, action_dn_clicked},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] ={
    {IDC_EDIT_ACTION_NAME, &name_proc_map},
    {IDC_BTN_ADD_ACTION, &add_proc_map},
    {IDC_BTN_DEL_ACTION, &del_proc_map},
    {IDC_BTN_EDT_ACTION, &edt_proc_map},
    {IDC_BTN_UP_ACTION, &up_proc_map},
    {IDC_BTN_DN_ACTION, &dn_proc_map},

    {0, NULL}
};



/* WM_NOTIFY proc */
static struct uint_ptr_pair list_proc_map[] = {
    {LVN_ITEMCHANGED, action_list_changed},
    {NM_DBLCLK, action_list_dblclk},

    {0, NULL}
};
static struct uint_ptr_pair notify_proc_map[] = {
    {IDC_LIST_ACTION, &list_proc_map},

    {0, NULL}
};
