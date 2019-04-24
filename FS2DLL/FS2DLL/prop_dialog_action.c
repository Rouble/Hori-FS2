/*
 * prop_dialog_action.c  -- action dialog to edit action
 *
 * $Id: prop_dialog_action.c,v 1.18 2004/03/25 05:15:04 hos Exp $
 *
 */

#include <windows.h>
#include <math.h>
#include "main.h"
#include "util.h"


/* prototypes */
static void action_dlg_update_page(prop_page_context_t *ctx);

static void action_dlg_joy_update_state(prop_page_context_t *ctx);
static void action_dlg_joy_update_val(prop_page_context_t *ctx);

static void action_dlg_mou_update_state(prop_page_context_t *ctx);
static void action_dlg_mou_update_val(prop_page_context_t *ctx);


/* page proc alist */
struct uint_ptr_pair action_dlg[];
struct uint_ptr_pair command_proc_map[];

struct uint_ptr_pair action_dlg_mod[];

struct uint_ptr_pair action_dlg_delay[];
struct uint_ptr_pair delay_command_proc_map[];

struct uint_ptr_pair action_dlg_joy[];
struct uint_ptr_pair joy_command_proc_map[];

struct uint_ptr_pair action_dlg_mou[];
struct uint_ptr_pair mou_command_proc_map[];

struct uint_ptr_pair action_dlg_kbd[];
struct uint_ptr_pair kbd_command_proc_map[];

struct uint_ptr_pair action_dlg_apply[];


/* implementations */

static void set_window_num_value(HWND edit, HWND spin, int val)
{
    TCHAR buf[32];

    SendMessage(spin, UDM_SETPOS32, 0, val);

    wsprintf(buf, TEXT("%d"), val);
    SetWindowText(edit, buf);
}

static int calc_ratio(UCHAR val, SHORT numer, SHORT denom, SHORT offset)
{
    if(denom == 0) {
        return 0;
    }

    return (double)val * numer / denom + offset;
}

/* EN_SETFOCUS for all edit controls */
static INT_PTR action_dlg_edit_setfocus(HWND hwnd,
                                        WORD code, WORD id, HWND ctrl,
                                        prop_page_context_t *ctx)
{
    SendMessage(ctrl, EM_SETSEL, 0, -1); /* select all */

    return TRUE;
}



/* WM_INITDIALOG for main */
static INT_PTR action_dlg_init_dialog(HWND hwnd, UINT msg,
                                      WPARAM wparam, LPARAM lparam,
                                      prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CMB_ACTION_TYPE, &ctx->action_dlg.main.type},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = command_proc_map;

    ctx->action_dlg.self = hwnd;

    {
        HWND place_holder;
        RECT rect;
        POINT pt;

        struct {
            int id;
            struct uint_ptr_pair *page_proc;
            UCHAR type;
            LPCTSTR str_type;
        } page[] = {
            {IDD_ACTION_EDIT_JOY, action_dlg_joy,
             CONTROLLER_ACTION_JOY, TEXT("Joystick")},
            {IDD_ACTION_EDIT_MOD, action_dlg_mod,
             CONTROLLER_ACTION_MOD, TEXT("Modifier")},
            {IDD_ACTION_EDIT_DELAY, action_dlg_delay,
             CONTROLLER_ACTION_DELAY, TEXT("Delay")},
            {IDD_ACTION_EDIT_APPLY, action_dlg_apply,
             CONTROLLER_ACTION_APPLY, TEXT("Action list")},
            {IDD_ACTION_EDIT_MOU, action_dlg_mou,
             CONTROLLER_ACTION_MOU, TEXT("Mouse")},
            {IDD_ACTION_EDIT_KBD, action_dlg_kbd,
             CONTROLLER_ACTION_KBD, TEXT("Keyboard")},

            {0, NULL}
        };

        /* get window place from place holder */
        place_holder = GetDlgItem(hwnd, IDC_ACTION_EDIT);
        GetWindowRect(place_holder, &rect);

        pt.x = rect.left;
        pt.y = rect.top;
        ScreenToClient(hwnd, &pt);

        /* count num of pages */
        for(i = 0; page[i].page_proc != NULL; i++) ;
        ctx->action_dlg.main.num_page = i;

        /* allocate page data */
        ctx->action_dlg.main.page_ctx = 
            (prop_page_context_t *)malloc(sizeof(prop_page_context_t) *
                                          ctx->action_dlg.main.num_page);
        if(ctx->action_dlg.main.page_ctx == NULL) {
            return FALSE;
        }
        memset(ctx->action_dlg.main.page_ctx, 0,
               sizeof(prop_page_context_t) * ctx->action_dlg.main.num_page);

        for(i = 0; page[i].page_proc != NULL; i++) {
            /* create page window */
            memcpy(&ctx->action_dlg.main.page_ctx[i], ctx,
                   sizeof(prop_page_context_t));
            memset(&ctx->action_dlg.main.page_ctx[i].action_dlg, 0,
                   sizeof(ctx->action_dlg.main.page_ctx[i].action_dlg));
            memcpy(&ctx->action_dlg.main.page_ctx[i].action_dlg.action_conf,
                   &ctx->action_dlg.action_conf,
                   sizeof(struct controller_action_config));
            ctx->action_dlg.main.page_ctx[i].page_proc = page[i].page_proc;
            ctx->action_dlg.main.page_ctx[i].action_dlg.self =
                CreateDialogParam(instance, MAKEINTRESOURCE(page[i].id),
                                  hwnd, prop_subpage_proc,
                                  (LPARAM)&ctx->action_dlg.main.page_ctx[i]);

            /* hide */
            ShowWindow(ctx->action_dlg.main.page_ctx[i].action_dlg.self,
                       SW_HIDE);

            /* move */
            SetWindowPos(ctx->action_dlg.main.page_ctx[i].action_dlg.self,
                         ctx->action_dlg.main.type,
                         pt.x, pt.y, 0, 0,
                         SWP_NOSIZE);

            /* add to combobox */
            SendMessage(ctx->action_dlg.main.type, CB_INSERTSTRING,
                        i, (LPARAM)page[i].str_type);

            if(ctx->action_dlg.action_conf.type == page[i].type) {
                /* select if type match */
                SendMessage(ctx->action_dlg.main.type, CB_SETCURSEL, i, 0);
            }
        }

        /* destroy place holder */
        DestroyWindow(place_holder);
    }

    if(SendMessage(ctx->action_dlg.main.type, CB_GETCURSEL, 0, 0) == CB_ERR) {
        /* select first item if not yet selected */
        SendMessage(ctx->action_dlg.main.type, CB_SETCURSEL, 0, 0);
    }
    action_dlg_update_page(ctx);

    return TRUE;
}

/* WM_DESTROY */
static INT_PTR action_dlg_destroy(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx)
{
    int i;

    if(ctx->action_dlg.main.page_ctx != NULL) {
        for(i = 0; i < ctx->action_dlg.main.num_page; i++) {
            DestroyWindow(ctx->action_dlg.main.page_ctx[i].action_dlg.self);
        }

        free(ctx->action_dlg.main.page_ctx);
    }

    ctx->action_dlg.main.page_ctx = NULL;

    return TRUE;
}

/* BN_CLICK for ok */
static INT_PTR action_dlg_ok_clicked(HWND hwnd,
                                     WORD code, WORD id, HWND ctrl,
                                     prop_page_context_t *ctx)
{
    int idx;

    idx = SendMessage(ctx->action_dlg.main.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        return FALSE;
    }

    if(SendMessage(ctx->action_dlg.main.page_ctx[idx].action_dlg.self,
                   WM_ACTION_DLG_QUERY_VALID, 0, 0) ==
       FALSE) {
        return TRUE;
    }

    memcpy(&ctx->action_dlg.action_conf,
           &ctx->action_dlg.main.page_ctx[idx].action_dlg.action_conf,
           sizeof(struct controller_action_config));

    EndDialog(hwnd, TRUE);

    return TRUE;
}

/* BN_CLICK for cancel */
static INT_PTR action_dlg_cancel_clicked(HWND hwnd,
                                         WORD code, WORD id, HWND ctrl,
                                         prop_page_context_t *ctx)
{
    EndDialog(hwnd, FALSE);

    return TRUE;
}

/* CBN_SELCHANGE for type */
static INT_PTR action_dlg_type_change(HWND hwnd,
                                      WORD code, WORD id, HWND ctrl,
                                      prop_page_context_t *ctx)
{
    action_dlg_update_page(ctx);

    return TRUE;
}

static void action_dlg_update_page(prop_page_context_t *ctx)
{
    int i, idx;

    for(i = 0; i < ctx->action_dlg.main.num_page; i++) {
        ShowWindow(ctx->action_dlg.main.page_ctx[i].action_dlg.self, SW_HIDE);
    }

    idx = SendMessage(ctx->action_dlg.main.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        return;
    }

    ShowWindow(ctx->action_dlg.main.page_ctx[idx].action_dlg.self, SW_SHOW);
}


/* WM_INITDIALOG for mod */
static INT_PTR action_dlg_mod_init_dialog(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CHK_ACTION_MOD(1), &ctx->action_dlg.mod.mod[0]},
        {IDC_CHK_ACTION_MOD(2), &ctx->action_dlg.mod.mod[1]},
        {IDC_CHK_ACTION_MOD(3), &ctx->action_dlg.mod.mod[2]},
        {IDC_CHK_ACTION_MOD(4), &ctx->action_dlg.mod.mod[3]},
        {IDC_CHK_ACTION_MOD(5), &ctx->action_dlg.mod.mod[4]},
        {IDC_CHK_ACTION_MOD(6), &ctx->action_dlg.mod.mod[5]},
        {IDC_CHK_ACTION_MOD(7), &ctx->action_dlg.mod.mod[6]},
        {IDC_CHK_ACTION_MOD(8), &ctx->action_dlg.mod.mod[7]},

        {IDC_CHK_ACTION_MOD(9), &ctx->action_dlg.mod.mod[8]},
        {IDC_CHK_ACTION_MOD(10), &ctx->action_dlg.mod.mod[9]},
        {IDC_CHK_ACTION_MOD(11), &ctx->action_dlg.mod.mod[10]},
        {IDC_CHK_ACTION_MOD(12), &ctx->action_dlg.mod.mod[11]},
        {IDC_CHK_ACTION_MOD(13), &ctx->action_dlg.mod.mod[12]},
        {IDC_CHK_ACTION_MOD(14), &ctx->action_dlg.mod.mod[13]},
        {IDC_CHK_ACTION_MOD(15), &ctx->action_dlg.mod.mod[14]},
        {IDC_CHK_ACTION_MOD(16), &ctx->action_dlg.mod.mod[15]},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);

    /* set checkbox */
    {
        USHORT mod, mod_mask;

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_MOD) {
            mod = ctx->action_dlg.action_conf.mod.mod;
            mod_mask = ctx->action_dlg.action_conf.mod.mod_mask;
        } else {
            mod = 0x0000;
            mod_mask = 0x0000;
        }

        set_modifier_bits_to_checkbox(ctx->action_dlg.mod.mod, mod, mod_mask);
    }

    return TRUE;
}

/* WM_ACTION_DLG_QUERY_VALID for mod */
static INT_PTR action_dlg_mod_query_valid(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_MOD;

    get_modifier_bits_from_checkbox(ctx->action_dlg.mod.mod,
                                    &ctx->action_dlg.action_conf.mod.mod,
                                    &ctx->action_dlg.action_conf.mod.mod_mask);

    SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}


/* WM_INITDIALOG for delay */
static INT_PTR action_dlg_delay_init_dialog(HWND hwnd, UINT msg,
                                            WPARAM wparam, LPARAM lparam,
                                            prop_page_context_t *ctx)
{
    struct uint_ptr_pair id_item_map[] = {
        {IDC_EDIT_ACTION_DELAY, &ctx->action_dlg.delay.msec},
        {IDC_SPIN_ACTION_DELAY, &ctx->action_dlg.delay.spin},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = delay_command_proc_map;

    /* set range of spin control: 0s - 100s */
    SendMessage(ctx->action_dlg.delay.spin, UDM_SETRANGE32, 0, 1000 * 100);

    /* set pos of spin control */
    {
        LPARAM pos;

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_DELAY) {
            pos = ctx->action_dlg.action_conf.delay.msec;
        } else {
            pos = 500;
        }

        SendMessage(ctx->action_dlg.delay.spin, UDM_SETPOS32, 0, pos);
    }

    return TRUE;
}

/* WM_ACTION_DLG_QUERY_VALID for delay */
static INT_PTR action_dlg_delay_query_valid(HWND hwnd, UINT msg,
                                            WPARAM wparam, LPARAM lparam,
                                            prop_page_context_t *ctx)
{
    LRESULT val;
    BOOL errp;

    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_DELAY;

    val = SendMessage(ctx->action_dlg.delay.spin, UDM_GETPOS32,
                      0, (LPARAM)&errp);
    if(errp) {
        display_message_box(IDS_MSG_ENTER_ACTION_DELAY_MSEC, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.delay.msec);

        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }

    ctx->action_dlg.action_conf.delay.msec = val;

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}


/* WM_INITDIALOG for joy */
static INT_PTR action_dlg_joy_init_dialog(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CMB_ACTION_JOY_CODE, &ctx->action_dlg.joy.target},
        {IDC_CMB_ACTION_JOY_TYPE, &ctx->action_dlg.joy.type},

        {IDC_EDIT_ACTION_JOY_VAL_MIN, &ctx->action_dlg.joy.min},
        {IDC_SPIN_ACTION_JOY_VAL_MIN, &ctx->action_dlg.joy.min_spin},

        {IDC_EDIT_ACTION_JOY_VAL_MAX, &ctx->action_dlg.joy.max},
        {IDC_SPIN_ACTION_JOY_VAL_MAX, &ctx->action_dlg.joy.max_spin},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = joy_command_proc_map;

    /* target combobox */
    {
        WCHAR buf[256];
        LPWSTR code;

        for(i = 0; i <= CONTROLLER_ACTION_JOY_CODE_MAX; i++) {
            code = (LPWSTR)assq_pair(action_joy_code_map, i, NULL);
            if(code == NULL) {
                wsprintfW(buf, action_code_button_base,
                          i - CONTROLLER_ACTION_JOY_CODE_BTN(0) + 1);
                code = buf;
            }

            SendMessage(ctx->action_dlg.joy.target, CB_INSERTSTRING,
                        i, (LPARAM)code);
        }

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_JOY) {
            SendMessage(ctx->action_dlg.joy.target, CB_SETCURSEL,
                        ctx->action_dlg.action_conf.joy.code, 0);
        }
    }

    /* type combobox */
    for(i = 0; action_joy_flag_map[i].ptr != NULL; i++) {
        SendMessage(ctx->action_dlg.joy.type, CB_INSERTSTRING,
                    i, (LPARAM)action_joy_flag_map[i].ptr);
        SendMessage(ctx->action_dlg.joy.type, CB_SETITEMDATA,
                    i, action_joy_flag_map[i].key);

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_JOY &&
           ctx->action_dlg.action_conf.joy.flags ==
           action_joy_flag_map[i].key) {
            SendMessage(ctx->action_dlg.joy.type, CB_SETCURSEL, i, 0);
        }
    }

    action_dlg_joy_update_state(ctx);
    action_dlg_joy_update_val(ctx);

    return TRUE;
}

static HWND action_dlg_joy_get_value(prop_page_context_t *ctx)
{
    int i;
    BOOL errp;
    HWND err_hwnd;

    struct {
        HWND spin;
        USHORT flags;
        LRESULT val;
    } spin_map[] = {
        {ctx->action_dlg.joy.min_spin, CONTROLLER_ACTION_JOY_VAL_SPEC},
        {ctx->action_dlg.joy.min_spin, CONTROLLER_ACTION_JOY_VAL_RATIO},
        {ctx->action_dlg.joy.max_spin, CONTROLLER_ACTION_JOY_VAL_RATIO},

        {NULL, 0}
    };

    err_hwnd = NULL;
    for(i = 0; spin_map[i].spin != NULL; i++) {
        if(spin_map[i].flags == ctx->action_dlg.action_conf.joy.flags) {
            spin_map[i].val = SendMessage(spin_map[i].spin, UDM_GETPOS32,
                                          0, (LPARAM)&errp);
            if(errp) {
                int min, max;
                SendMessage(spin_map[i].spin, UDM_GETRANGE32,
                            (WPARAM)&min, (LPARAM)&max);
                spin_map[i].val = min;

                err_hwnd = spin_map[i].spin;
            }
        }
    }

    ctx->action_dlg.action_conf.joy.val = 0;
    ctx->action_dlg.action_conf.joy.ratio_numerator = 1;
    ctx->action_dlg.action_conf.joy.ratio_denominator = 1;
    ctx->action_dlg.action_conf.joy.offset = 0;

    switch(ctx->action_dlg.action_conf.joy.flags) {
      case CONTROLLER_ACTION_JOY_VAL_MIN:
      case CONTROLLER_ACTION_JOY_VAL_MAX:
      case CONTROLLER_ACTION_JOY_VAL_MDL:
      case CONTROLLER_ACTION_JOY_VAL_THROUGH:
      case CONTROLLER_ACTION_JOY_VAL_REVERSE:
          break;

      case CONTROLLER_ACTION_JOY_VAL_SPEC:
          ctx->action_dlg.action_conf.joy.val =
              (spin_map[0].val < 0 ? 0 :
               (spin_map[0].val > 0xff ? 0xff : spin_map[0].val));
          break;

      case CONTROLLER_ACTION_JOY_VAL_RATIO:
          /* v * (max - min) / 0xff + min */
          ctx->action_dlg.action_conf.joy.ratio_numerator =
              spin_map[2].val - spin_map[1].val;
          ctx->action_dlg.action_conf.joy.ratio_denominator = 0xff;
          ctx->action_dlg.action_conf.joy.offset = spin_map[1].val;
          break;
    }

    return err_hwnd;
}

/* WM_ACTION_DLG_QUERY_VALID for joy */
static INT_PTR action_dlg_joy_query_valid(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int idx;

    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_JOY;

    idx = SendMessage(ctx->action_dlg.joy.target, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_JOY_TARGET, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.joy.target);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }
    ctx->action_dlg.action_conf.joy.code = idx;

    idx = SendMessage(ctx->action_dlg.joy.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_JOY_TYPE, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.joy.type);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }

    {
        HWND err_hwnd;

        err_hwnd = action_dlg_joy_get_value(ctx);
        if(err_hwnd != NULL) {
            display_message_box(IDS_MSG_ENTER_ACTIO_JOY_VALUE,
                                IDS_MSG_ERROR,
                                hwnd, MB_OK | MB_ICONSTOP);
            SetFocus(err_hwnd);

            SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
            return TRUE;            
        }
    }

    SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}

/* CBN_SELCHANGED */
static INT_PTR action_dlg_joy_type_changed(HWND hwnd,
                                           WORD code, WORD id, HWND ctrl,
                                           prop_page_context_t *ctx)
{
    action_dlg_joy_get_value(ctx);
    action_dlg_joy_update_state(ctx);
    action_dlg_joy_update_val(ctx);

    return TRUE;
}

static void action_dlg_joy_update_state(prop_page_context_t *ctx)
{
    int idx, val;
    int i;
    HWND edit[] = {
        ctx->action_dlg.joy.min,
        ctx->action_dlg.joy.max,

        NULL
    };

    for(i = 0; edit[i] != NULL; i++) {
        EnableWindow(edit[i], FALSE);
        SetWindowText(edit[i], TEXT(""));
    }

    idx = SendMessage(ctx->action_dlg.joy.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        return;
    }

    val = SendMessage(ctx->action_dlg.joy.type, CB_GETITEMDATA, idx, 0);
    ctx->action_dlg.action_conf.joy.flags = val;
}

static void action_dlg_joy_update_val(prop_page_context_t *ctx)
{
    int min, max;

    if(SendMessage(ctx->action_dlg.joy.type, CB_GETCURSEL, 0, 0) == CB_ERR) {
        return;
    }

    switch(ctx->action_dlg.action_conf.joy.flags) {
      case CONTROLLER_ACTION_JOY_VAL_MIN:
          min = max = 0x00;
          break;

      case CONTROLLER_ACTION_JOY_VAL_MAX:
          min = max = 0xff;
          break;

      case CONTROLLER_ACTION_JOY_VAL_MDL:
          min = max = 0x7f;
          break;

      case CONTROLLER_ACTION_JOY_VAL_THROUGH:
          min = 0x00;
          max = 0xff;
          break;

      case CONTROLLER_ACTION_JOY_VAL_REVERSE:
          min = 0xff;
          max = 0x00;
          break;

      case CONTROLLER_ACTION_JOY_VAL_SPEC:
          SendMessage(ctx->action_dlg.joy.min_spin, UDM_SETRANGE32,
                      0x00, 0xff);

          EnableWindow(ctx->action_dlg.joy.min, TRUE);

          min = max = ctx->action_dlg.action_conf.joy.val;
          break;

      case CONTROLLER_ACTION_JOY_VAL_RATIO:
          SendMessage(ctx->action_dlg.joy.min_spin, UDM_SETRANGE32,
                      -1000, +1000);
          SendMessage(ctx->action_dlg.joy.max_spin, UDM_SETRANGE32,
                      -1000, +1000);

          EnableWindow(ctx->action_dlg.joy.min, TRUE);
          EnableWindow(ctx->action_dlg.joy.max, TRUE);

          min = calc_ratio(0x00,
                           ctx->action_dlg.action_conf.joy.ratio_numerator,
                           ctx->action_dlg.action_conf.joy.ratio_denominator,
                           ctx->action_dlg.action_conf.joy.offset);
          max = calc_ratio(0xff,
                           ctx->action_dlg.action_conf.joy.ratio_numerator,
                           ctx->action_dlg.action_conf.joy.ratio_denominator,
                           ctx->action_dlg.action_conf.joy.offset);
          break;

      default:
          return;
    }

    set_window_num_value(ctx->action_dlg.joy.min,
                         ctx->action_dlg.joy.min_spin,
                         min);
    set_window_num_value(ctx->action_dlg.joy.max,
                         ctx->action_dlg.joy.max_spin,
                         max);
}

static void action_dlg_joy_update_changed(prop_page_context_t *ctx)
{
    action_dlg_joy_get_value(ctx);
    action_dlg_joy_update_val(ctx);
}

/* WM_VSCROLL for joy */
static INT_PTR action_dlg_joy_vscroll(HWND hwnd, UINT msg,
                                      WPARAM wparam, LPARAM lparam,
                                      prop_page_context_t *ctx)
{
    action_dlg_joy_update_changed(ctx);

    return TRUE;
}

/* EN_KILLFOCUS */
static INT_PTR action_dlg_joy_edit_killfocus(HWND hwnd,
                                             WORD code, WORD id, HWND ctrl,
                                             prop_page_context_t *ctx)
{
    action_dlg_joy_update_changed(ctx);

    return TRUE;
}


/* WM_INITDIALOG for apply */
static INT_PTR action_dlg_apply_init_dialog(HWND hwnd, UINT msg,
                                            WPARAM wparam, LPARAM lparam,
                                            prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CHK_APPLY_MOD(1), &ctx->action_dlg.apply.mod[0]},
        {IDC_CHK_APPLY_MOD(2), &ctx->action_dlg.apply.mod[1]},
        {IDC_CHK_APPLY_MOD(3), &ctx->action_dlg.apply.mod[2]},
        {IDC_CHK_APPLY_MOD(4), &ctx->action_dlg.apply.mod[3]},
        {IDC_CHK_APPLY_MOD(5), &ctx->action_dlg.apply.mod[4]},
        {IDC_CHK_APPLY_MOD(6), &ctx->action_dlg.apply.mod[5]},
        {IDC_CHK_APPLY_MOD(7), &ctx->action_dlg.apply.mod[6]},
        {IDC_CHK_APPLY_MOD(8), &ctx->action_dlg.apply.mod[7]},

        {IDC_CHK_APPLY_MOD(9), &ctx->action_dlg.apply.mod[8]},
        {IDC_CHK_APPLY_MOD(10), &ctx->action_dlg.apply.mod[9]},
        {IDC_CHK_APPLY_MOD(11), &ctx->action_dlg.apply.mod[10]},
        {IDC_CHK_APPLY_MOD(12), &ctx->action_dlg.apply.mod[11]},
        {IDC_CHK_APPLY_MOD(13), &ctx->action_dlg.apply.mod[12]},
        {IDC_CHK_APPLY_MOD(14), &ctx->action_dlg.apply.mod[13]},
        {IDC_CHK_APPLY_MOD(15), &ctx->action_dlg.apply.mod[14]},
        {IDC_CHK_APPLY_MOD(16), &ctx->action_dlg.apply.mod[15]},

        {IDC_CMB_APPLY_ACTION, &ctx->action_dlg.apply.action},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);

    /* set checkbox */
    {
        USHORT mod, mod_mask;

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_APPLY) {
            mod = ctx->action_dlg.action_conf.apply.mod;
            mod_mask = ctx->action_dlg.action_conf.apply.mod_mask;
        } else {
            mod = 0x0000;
            mod_mask = 0x0000;
        }

        set_modifier_bits_to_checkbox(ctx->action_dlg.apply.mod,
                                      mod, mod_mask);
    }

    /* set combobox */
    {
        WPARAM idx;

        for(i = 0; i < ctx->data->conf.num_action_list; i++) {
            SendMessage(ctx->action_dlg.apply.action, CB_INSERTSTRING,
                        i, (LPARAM)ctx->data->conf.action_list[i].name);
        }

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_APPLY) {
            idx = ctx->action_dlg.action_conf.apply.action_idx;
        } else {
            idx = -1;
        }
        SendMessage(ctx->action_dlg.apply.action, CB_SETCURSEL, idx, 0);
    }

    return TRUE;
}

/* WM_ACTION_DLG_QUERY_VALID for apply */
static INT_PTR action_dlg_apply_query_valid(HWND hwnd, UINT msg,
                                            WPARAM wparam, LPARAM lparam,
                                            prop_page_context_t *ctx)
{
    int idx;

    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_APPLY;

    idx = SendMessage(ctx->action_dlg.apply.action, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_ERR_SELECT_ACTION, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.apply.action);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }

    get_modifier_bits_from_checkbox(
        ctx->action_dlg.apply.mod,
        &ctx->action_dlg.action_conf.apply.mod,
        &ctx->action_dlg.action_conf.apply.mod_mask);
    ctx->action_dlg.action_conf.apply.action_idx = idx;

    SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}


/* WM_INITDIALOG for mouse */
static INT_PTR action_dlg_mou_init_dialog(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CMB_ACTION_MOU_CODE, &ctx->action_dlg.mou.target},
        {IDC_CMB_ACTION_MOU_TYPE, &ctx->action_dlg.mou.type},

        {IDC_EDIT_ACTION_MOU_VAL_MIN, &ctx->action_dlg.mou.min},
        {IDC_SPIN_ACTION_MOU_VAL_MIN, &ctx->action_dlg.mou.min_spin},

        {IDC_EDIT_ACTION_MOU_VAL_MAX, &ctx->action_dlg.mou.max},
        {IDC_SPIN_ACTION_MOU_VAL_MAX, &ctx->action_dlg.mou.max_spin},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = mou_command_proc_map;

    /* target combobox */
    {
        WCHAR buf[256];
        LPWSTR code;

        for(i = 0; i <= CONTROLLER_ACTION_MOU_CODE_MAX; i++) {
            code = (LPWSTR)assq_pair(action_mou_code_map, i, NULL);
            if(code == NULL) {
                wsprintfW(buf, action_code_button_base,
                          i - CONTROLLER_ACTION_MOU_CODE_BTN(0) + 1);
                code = buf;
            }

            SendMessage(ctx->action_dlg.mou.target, CB_INSERTSTRING,
                        i, (LPARAM)code);
        }

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_MOU) {
            SendMessage(ctx->action_dlg.mou.target, CB_SETCURSEL,
                        ctx->action_dlg.action_conf.mou.code, 0);
        }
    }

    /* type combobox */
    for(i = 0; action_mou_flag_map[i].ptr != NULL; i++) {
        SendMessage(ctx->action_dlg.mou.type, CB_INSERTSTRING,
                    i, (LPARAM)action_mou_flag_map[i].ptr);
        SendMessage(ctx->action_dlg.mou.type, CB_SETITEMDATA,
                    i, action_mou_flag_map[i].key);

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_MOU &&
           ctx->action_dlg.action_conf.mou.flags ==
           action_mou_flag_map[i].key) {
            SendMessage(ctx->action_dlg.mou.type, CB_SETCURSEL, i, 0);
        }
    }

    action_dlg_mou_update_state(ctx);
    action_dlg_mou_update_val(ctx);

    return TRUE;
}

static HWND action_dlg_mou_get_value(prop_page_context_t *ctx)
{
    int i;
    BOOL errp;
    HWND err_hwnd;

    struct {
        HWND spin;
        USHORT flags;
        LRESULT val;
    } spin_map[] = {
        {ctx->action_dlg.mou.min_spin, CONTROLLER_ACTION_MOU_VAL_SPEC},
        {ctx->action_dlg.mou.min_spin, CONTROLLER_ACTION_MOU_VAL_RATIO},
        {ctx->action_dlg.mou.max_spin, CONTROLLER_ACTION_MOU_VAL_RATIO},

        {NULL, 0}
    };

    err_hwnd = NULL;
    for(i = 0; spin_map[i].spin != NULL; i++) {
        if(spin_map[i].flags == ctx->action_dlg.action_conf.mou.flags) {
            spin_map[i].val = SendMessage(spin_map[i].spin, UDM_GETPOS32,
                                          0, (LPARAM)&errp);
            if(errp) {
                int min, max;
                SendMessage(spin_map[i].spin, UDM_GETRANGE32,
                            (WPARAM)&min, (LPARAM)&max);
                spin_map[i].val = (min + max) / 2;

                err_hwnd = spin_map[i].spin;
            }

            spin_map[i].val += (spin_map[i].val <= 0 ? 0x7f : 0x80);
        }
    }

    ctx->action_dlg.action_conf.mou.val = 0;
    ctx->action_dlg.action_conf.mou.ratio_numerator = 1;
    ctx->action_dlg.action_conf.mou.ratio_denominator = 1;
    ctx->action_dlg.action_conf.mou.offset = 0;

    switch(ctx->action_dlg.action_conf.mou.flags) {
      case CONTROLLER_ACTION_MOU_VAL_MIN:
      case CONTROLLER_ACTION_MOU_VAL_MAX:
      case CONTROLLER_ACTION_MOU_VAL_MDL:
      case CONTROLLER_ACTION_MOU_VAL_THROUGH:
      case CONTROLLER_ACTION_MOU_VAL_REVERSE:
          break;

      case CONTROLLER_ACTION_MOU_VAL_SPEC:
          ctx->action_dlg.action_conf.mou.val =
              (spin_map[0].val < 0 ? 0 :
               (spin_map[0].val > 0xff ? 0xff : spin_map[0].val));
          break;

      case CONTROLLER_ACTION_MOU_VAL_RATIO:
          /* v * (max - min) / 0xff + min */
          ctx->action_dlg.action_conf.mou.ratio_numerator =
              spin_map[2].val - spin_map[1].val;
          ctx->action_dlg.action_conf.mou.ratio_denominator = 0xff;
          ctx->action_dlg.action_conf.mou.offset = spin_map[1].val;
          break;
    }

    return err_hwnd;
}

/* WM_ACTION_DLG_QUERY_VALID for mou */
static INT_PTR action_dlg_mou_query_valid(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int idx;

    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_MOU;

    idx = SendMessage(ctx->action_dlg.mou.target, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_MOU_TARGET, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.mou.target);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }
    ctx->action_dlg.action_conf.mou.code = idx;

    idx = SendMessage(ctx->action_dlg.mou.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_MOU_TYPE, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.mou.type);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }

    {
        HWND err_hwnd;

        err_hwnd = action_dlg_mou_get_value(ctx);
        if(err_hwnd != NULL) {
            display_message_box(IDS_MSG_ENTER_ACTIO_MOU_VALUE,
                                IDS_MSG_ERROR,
                                hwnd, MB_OK | MB_ICONSTOP);
            SetFocus(err_hwnd);

            SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
            return TRUE;            
        }
    }

    SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}

/* CBN_SELCHANGED */
static INT_PTR action_dlg_mou_type_changed(HWND hwnd,
                                           WORD code, WORD id, HWND ctrl,
                                           prop_page_context_t *ctx)
{
    action_dlg_mou_get_value(ctx);
    action_dlg_mou_update_state(ctx);
    action_dlg_mou_update_val(ctx);

    return TRUE;
}

static void action_dlg_mou_update_state(prop_page_context_t *ctx)
{
    int idx, val;
    int i;
    HWND edit[] = {
        ctx->action_dlg.mou.min,
        ctx->action_dlg.mou.max,

        NULL
    };

    for(i = 0; edit[i] != NULL; i++) {
        EnableWindow(edit[i], FALSE);
        SetWindowText(edit[i], TEXT(""));
    }

    idx = SendMessage(ctx->action_dlg.mou.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        return;
    }

    val = SendMessage(ctx->action_dlg.mou.type, CB_GETITEMDATA, idx, 0);
    ctx->action_dlg.action_conf.mou.flags = val;
}

static void action_dlg_mou_update_val(prop_page_context_t *ctx)
{
    int min, max;

    if(SendMessage(ctx->action_dlg.mou.type, CB_GETCURSEL, 0, 0) == CB_ERR) {
        return;
    }

    switch(ctx->action_dlg.action_conf.mou.flags) {
      case CONTROLLER_ACTION_MOU_VAL_MIN:
          min = max = 0x00;
          break;

      case CONTROLLER_ACTION_MOU_VAL_MAX:
          min = max = 0xff;
          break;

      case CONTROLLER_ACTION_MOU_VAL_MDL:
          min = max = 0x7f;
          break;

      case CONTROLLER_ACTION_MOU_VAL_THROUGH:
          min = 0x00;
          max = 0xff;
          break;

      case CONTROLLER_ACTION_MOU_VAL_REVERSE:
          min = 0xff;
          max = 0x00;
          break;

      case CONTROLLER_ACTION_MOU_VAL_SPEC:
          SendMessage(ctx->action_dlg.mou.min_spin, UDM_SETRANGE32,
                      -127, 127);

          EnableWindow(ctx->action_dlg.mou.min, TRUE);

          min = max = ctx->action_dlg.action_conf.mou.val;
          break;

      case CONTROLLER_ACTION_MOU_VAL_RATIO:
          SendMessage(ctx->action_dlg.mou.min_spin, UDM_SETRANGE32,
                      -1000, +1000);
          SendMessage(ctx->action_dlg.mou.max_spin, UDM_SETRANGE32,
                      -1000, +1000);

          EnableWindow(ctx->action_dlg.mou.min, TRUE);
          EnableWindow(ctx->action_dlg.mou.max, TRUE);

          min = calc_ratio(0x00,
                           ctx->action_dlg.action_conf.mou.ratio_numerator,
                           ctx->action_dlg.action_conf.mou.ratio_denominator,
                           ctx->action_dlg.action_conf.mou.offset);
          max = calc_ratio(0xff,
                           ctx->action_dlg.action_conf.mou.ratio_numerator,
                           ctx->action_dlg.action_conf.mou.ratio_denominator,
                           ctx->action_dlg.action_conf.mou.offset);
          break;

      default:
          return;
    }

    min -= (min <= 0x7f ? 0x7f : 0x80);
    max -= (max <= 0x7f ? 0x7f : 0x80);

    set_window_num_value(ctx->action_dlg.mou.min,
                         ctx->action_dlg.mou.min_spin,
                         min);
    set_window_num_value(ctx->action_dlg.mou.max,
                         ctx->action_dlg.mou.max_spin,
                         max);
}

static void action_dlg_mou_update_changed(prop_page_context_t *ctx)
{
    action_dlg_mou_get_value(ctx);
    action_dlg_mou_update_val(ctx);
}

/* WM_VSCROLL for mou */
static INT_PTR action_dlg_mou_vscroll(HWND hwnd, UINT msg,
                                      WPARAM wparam, LPARAM lparam,
                                      prop_page_context_t *ctx)
{
    action_dlg_mou_update_changed(ctx);

    return TRUE;
}

/* EN_KILLFOCUS */
static INT_PTR action_dlg_mou_edit_killfocus(HWND hwnd,
                                             WORD code, WORD id, HWND ctrl,
                                             prop_page_context_t *ctx)
{
    action_dlg_mou_update_changed(ctx);

    return TRUE;
}


/* WM_INITDIALOG for keyboard */
static INT_PTR action_dlg_kbd_init_dialog(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int i, code;
    LPWSTR str;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_CMB_ACTION_KBD_CODE, &ctx->action_dlg.kbd.target},
        {IDC_CMB_ACTION_KBD_TYPE, &ctx->action_dlg.kbd.type},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);

    /* target combobox */
    for(code = CONTROLLER_ACTION_KBD_CODE_MIN, i = 0;
        code < CONTROLLER_ACTION_KBD_CODE_MAX; code++) {
        str = kbd_code_to_str(code);
        if(str == NULL) {
            continue;
        }

        SendMessage(ctx->action_dlg.kbd.target, CB_INSERTSTRING,
                    i, (LPARAM)str);
        SendMessage(ctx->action_dlg.kbd.target, CB_SETITEMDATA, i, code);

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_KBD &&
           ctx->action_dlg.action_conf.kbd.code == code) {
            SendMessage(ctx->action_dlg.kbd.target, CB_SETCURSEL, i, 0);
        }

        i += 1;
    }

    /* type combobox */
    for(i = 0; action_kbd_flag_map[i].ptr != NULL; i++) {
        SendMessage(ctx->action_dlg.kbd.type, CB_INSERTSTRING,
                    i, (LPARAM)action_kbd_flag_map[i].ptr);
        SendMessage(ctx->action_dlg.kbd.type, CB_SETITEMDATA,
                    i, action_kbd_flag_map[i].key);

        if(ctx->action_dlg.action_conf.type == CONTROLLER_ACTION_KBD &&
           ctx->action_dlg.action_conf.kbd.flags ==
           action_kbd_flag_map[i].key) {
            SendMessage(ctx->action_dlg.kbd.type, CB_SETCURSEL, i, 0);
        }
    }

    return TRUE;
}

/* WM_ACTION_DLG_QUERY_VALID for kbd */
static INT_PTR action_dlg_kbd_query_valid(HWND hwnd, UINT msg,
                                          WPARAM wparam, LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    int idx;

    ctx->action_dlg.action_conf.type = CONTROLLER_ACTION_KBD;

    idx = SendMessage(ctx->action_dlg.kbd.target, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_KBD_TARGET, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.kbd.target);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }
    ctx->action_dlg.action_conf.kbd.code =
        SendMessage(ctx->action_dlg.kbd.target, CB_GETITEMDATA, idx, 0);

    idx = SendMessage(ctx->action_dlg.kbd.type, CB_GETCURSEL, 0, 0);
    if(idx == CB_ERR) {
        display_message_box(IDS_MSG_SELECT_KBD_TYPE, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        SetFocus(ctx->action_dlg.kbd.type);

        SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE);
        return TRUE;
    }
    ctx->action_dlg.action_conf.kbd.flags =
        SendMessage(ctx->action_dlg.kbd.type, CB_GETITEMDATA, idx, 0);

    SetWindowLong(hwnd, DWLP_MSGRESULT, TRUE);
    return TRUE;
}


/* page proc alist */
struct uint_ptr_pair action_dlg[] = {
    {WM_INITDIALOG, action_dlg_init_dialog},
    {WM_DESTROY, action_dlg_destroy},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};

static struct uint_ptr_pair action_dlg_mod[] = {
    {WM_INITDIALOG, action_dlg_mod_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_mod_query_valid},

    {0, NULL}
};
static struct uint_ptr_pair action_dlg_delay[] = {
    {WM_INITDIALOG, action_dlg_delay_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_delay_query_valid},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};
static struct uint_ptr_pair action_dlg_joy[] = {
    {WM_INITDIALOG, action_dlg_joy_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_joy_query_valid},
    {WM_VSCROLL, action_dlg_joy_vscroll},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};
static struct uint_ptr_pair action_dlg_mou[] = {
    {WM_INITDIALOG, action_dlg_mou_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_mou_query_valid},
    {WM_VSCROLL, action_dlg_mou_vscroll},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};
static struct uint_ptr_pair action_dlg_kbd[] = {
    {WM_INITDIALOG, action_dlg_kbd_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_kbd_query_valid},

    {0, NULL}
};
static struct uint_ptr_pair action_dlg_apply[] = {
    {WM_INITDIALOG, action_dlg_apply_init_dialog},
    {WM_ACTION_DLG_QUERY_VALID, action_dlg_apply_query_valid},

    {0, NULL}
};


/* WM_COMMAND proc */
static struct uint_ptr_pair ok_proc_map[] = {
    {BN_CLICKED, action_dlg_ok_clicked},

    {0, NULL}
};
static struct uint_ptr_pair cancel_proc_map[] = {
    {BN_CLICKED, action_dlg_cancel_clicked},

    {0, NULL}
};
static struct uint_ptr_pair type_proc_map[] = {
    {CBN_SELCHANGE, action_dlg_type_change},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] ={
    {IDOK, &ok_proc_map},
    {IDCANCEL, &cancel_proc_map},
    {IDC_CMB_ACTION_TYPE, &type_proc_map},

    {0, NULL}
};


/* WM_COMMAND for delay */
static struct uint_ptr_pair action_dlg_delay_val[] = {
    {EN_SETFOCUS, action_dlg_edit_setfocus},

    {0, NULL}
};
static struct uint_ptr_pair delay_command_proc_map[] = {
    {IDC_EDIT_ACTION_DELAY, action_dlg_delay_val},

    {0, NULL}
};


/* WM_COMMAND for joy */
static struct uint_ptr_pair joy_type_proc[] = {
    {CBN_SELCHANGE, action_dlg_joy_type_changed},

    {0, NULL}
};
static struct uint_ptr_pair joy_val_proc[] = {
    {EN_KILLFOCUS, action_dlg_joy_edit_killfocus},
    {EN_SETFOCUS, action_dlg_edit_setfocus},

    {0, NULL}
};
static struct uint_ptr_pair joy_command_proc_map[] = {
    {IDC_CMB_ACTION_JOY_TYPE, joy_type_proc},
    {IDC_EDIT_ACTION_JOY_VAL_MIN, joy_val_proc},
    {IDC_EDIT_ACTION_JOY_VAL_MAX, joy_val_proc},

    {0, NULL}
};


/* WM_COMMAND for mou */
static struct uint_ptr_pair mou_type_proc[] = {
    {CBN_SELCHANGE, action_dlg_mou_type_changed},

    {0, NULL}
};
static struct uint_ptr_pair mou_val_proc[] = {
    {EN_KILLFOCUS, action_dlg_mou_edit_killfocus},
    {EN_SETFOCUS, action_dlg_edit_setfocus},

    {0, NULL}
};
static struct uint_ptr_pair mou_command_proc_map[] = {
    {IDC_CMB_ACTION_MOU_TYPE, mou_type_proc},
    {IDC_EDIT_ACTION_MOU_VAL_MIN, mou_val_proc},
    {IDC_EDIT_ACTION_MOU_VAL_MAX, mou_val_proc},

    {0, NULL}
};
