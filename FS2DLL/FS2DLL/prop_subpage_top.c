/*
 * prop_page_top.c  -- profile setting property subpage
 *
 * $Id: prop_subpage_top.c,v 1.11 2004/06/10 16:08:35 hos Exp $
 *
 */

#include <windows.h>
#include <prsht.h>
#include "main.h"
#include "util.h"


#define OPENFILE_FILTER \
  L"Device Profiles (*.sdp)\0*.sdp\0All Files (*.*)\0*.*\0"


/* page proc alist */
struct uint_ptr_pair top_setting_subpage[];
struct uint_ptr_pair command_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR top_setting_init_dialog(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx)
{
    struct uint_ptr_pair id_item_map[] = {
        {IDC_EDIT_PROFILE_NAME, &ctx->top.name},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = command_proc_map;

    SetWindowTextW(ctx->top.name, ctx->data->conf.profile_name);

    return TRUE;
}

/* WM_SUBPAGE_CHANGE */
static INT_PTR top_setting_change(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx)
{
    SetWindowTextW(ctx->top.name, ctx->data->conf.profile_name);

    return TRUE;
}

/* BN_CLICKED for save */
static INT_PTR top_save_clicked(HWND hwnd,
                                WORD code, WORD id, HWND ctrl,
                                prop_page_context_t *ctx)
{
    OPENFILENAMEW ofn;
    WCHAR fname[1024];

    memset(fname, 0, sizeof(fname));
    if(wcsstr(fname, L"<>:\"/\\|") == NULL) {
        wcsncpy(fname, ctx->data->conf.profile_name, 1023);
    }

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = ctx->page_hwnd;
    ofn.lpstrFilter = OPENFILE_FILTER;
    ofn.lpstrFile = fname;
    ofn.nMaxFile = 1024;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"sdp";

    if(GetSaveFileName(&ofn) == 0) {
        return TRUE;
    }

    if(! save_file_setting(&ctx->data->conf, fname)) {
        display_message_box(IDS_MSG_FAIL_SAVE_FILE, IDS_MSG_ERROR,
                            ctx->page_hwnd, MB_OK | MB_ICONSTOP);
        return TRUE;
    }

    return TRUE;
}

/* BN_CLICKED for load */
static INT_PTR top_load_clicked(HWND hwnd,
                                WORD code, WORD id, HWND ctrl,
                                prop_page_context_t *ctx)
{
    OPENFILENAMEW ofn;
    WCHAR fname[1024];

    if(display_message_box(IDS_MSG_CONFIRM_LOAD, IDS_MSG_LOAD_FILE,
                           ctx->page_hwnd, MB_YESNO | MB_ICONQUESTION) !=
       IDYES) {
        return TRUE;
    }

    memset(fname, 0, sizeof(fname));

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = ctx->page_hwnd;
    ofn.lpstrFilter = OPENFILE_FILTER;
    ofn.lpstrFile = fname;
    ofn.nMaxFile = 1024;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if(GetOpenFileNameW(&ofn) == 0) {
        return TRUE;
    }

    {
        controller_setting_t conf;

        memset(&conf, 0, sizeof(conf));
        if(! load_file_setting(&conf, fname)) {
            free_device_setting(&conf);
            display_message_box(IDS_MSG_FAIL_LOAD_FILE, IDS_MSG_ERROR,
                                ctx->page_hwnd, MB_OK | MB_ICONSTOP);
            return TRUE;
        }

        free_device_setting(&ctx->data->conf);
        memcpy(&ctx->data->conf, &conf, sizeof(conf));
    }

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE, MAIN_TREE_REBUILD, 0);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* BN_CLICKED for load def */
static INT_PTR top_load_def_clicked(HWND hwnd,
                                    WORD code, WORD id, HWND ctrl,
                                    prop_page_context_t *ctx)
{
    if(display_message_box(IDS_MSG_CONFIRM_LOAD, IDS_MSG_LOAD_DEFAULT,
                           ctx->page_hwnd, MB_YESNO | MB_ICONQUESTION) !=
       IDYES) {
        return TRUE;
    }

    load_default_setting(&ctx->data->conf);

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE, MAIN_TREE_REBUILD, 0);

    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* EN_KILLFOCUS */
static INT_PTR top_name_killfocus(HWND hwnd,
                                  WORD code, WORD id, HWND ctrl,
                                  prop_page_context_t *ctx)
{
    LPWSTR name;
    int len;

    len = GetWindowTextLengthW(ctx->top.name);
    if(len <= 0) {
        display_message_box(IDS_MSG_ENTER_PROFILE_NAME, IDS_MSG_ERROR,
                            ctx->page_hwnd, MB_OK | MB_ICONWARNING);
        SetWindowTextW(ctx->top.name, ctx->data->conf.profile_name);
        SetFocus(ctx->top.name);
        return TRUE;
    }

    name = ctx->data->conf.profile_name;
    name = (LPWSTR)realloc(name, sizeof(WCHAR) * (len + 1));
    if(name == NULL) {
        return FALSE;
    }

    GetWindowTextW(ctx->top.name, name, len + 1);
    ctx->data->conf.profile_name = name;

    SendMessage(ctx->page_hwnd, WM_MAIN_TREE_CHANGE,
                MAIN_TREE_ROOTNAME, (LPARAM)name);

    return TRUE;
}


/* page proc alist */
struct uint_ptr_pair top_setting_subpage[] = {
    {WM_INITDIALOG, top_setting_init_dialog},
    {WM_SUBPAGE_CHANGE, top_setting_change},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};


/* WM_COMMAND */
static struct uint_ptr_pair save_proc_map[] = {
    {BN_CLICKED, top_save_clicked},

    {0, NULL}
};
static struct uint_ptr_pair load_proc_map[] = {
    {BN_CLICKED, top_load_clicked},

    {0, NULL}
};
static struct uint_ptr_pair load_def_proc_map[] = {
    {BN_CLICKED, top_load_def_clicked},

    {0, NULL}
};
static struct uint_ptr_pair name_proc_map[] = {
    {EN_KILLFOCUS, top_name_killfocus},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] ={
    {IDC_BTN_SAVE_PROFILE, &save_proc_map},
    {IDC_BTN_LOAD_PROFILE, &load_proc_map},
    {IDC_BTN_LOAD_DEF_PROFILE, &load_def_proc_map},
    {IDC_EDIT_PROFILE_NAME, &name_proc_map},

    {0, NULL}
};
