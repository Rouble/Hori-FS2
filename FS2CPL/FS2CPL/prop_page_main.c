/*
 * prop_page_main.c  -- main setting property page
 *
 * $Id: prop_page_main.c,v 1.34 2004/06/03 09:48:25 hos Exp $
 *
 */

#include <windows.h>
#include <dbt.h>
#include "main.h"
#include "util.h"


/* prototypes */
static void add_tree_item(HWND tree, tree_item_spec_t *spec, HTREEITEM parent,
                          prop_page_context_t *ctx);
static void build_action_tree(prop_page_context_t *ctx);
static void build_tree(prop_page_context_t *ctx);
static void set_tree_root_name(prop_page_context_t *ctx, LPWSTR name);


/* page proc alist */
struct uint_ptr_pair main_setting_page[];
struct uint_ptr_pair notify_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR main_setting_init_dialog(HWND hwnd, UINT msg,
                                        WPARAM wparam, LPARAM lparam,
                                        prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_SETTING_TREE, &ctx->main.tree},
        {IDC_DEFAULT_SUBPAG, &ctx->main.def_subpage},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->notify_proc = notify_proc_map;

    /* build tree */
    build_tree(ctx);

    /* subpags */
    {
        HWND place_holder;
        RECT rect;
        POINT pt;

        struct {
            int id;
            int place_id;
            prop_page_context_t **subpage;
            struct uint_ptr_pair *page_proc;
        } subpage[] = {
            {IDD_EVENT_SUBPAGE, IDC_EVENT_UPPER_SUBPAGE,
             &ctx->main.event_upper, event_subpage},
            {IDD_EVENT_SUBPAGE, IDC_EVENT_LOWER_SUBPAGE,
             &ctx->main.event_lower, event_subpage},
            {IDD_ACTION_SUBPAGE, IDC_ACTION_SUBPAGE,
             &ctx->main.action, action_subpage},
            {IDD_ACTION_LIST_SUBPAGE, IDC_ACTION_LIST_SUBPAGE,
             &ctx->main.action_list, action_list_subpage},
            {IDD_CONTROLLER_SUBPAGE, IDC_CONTROLLER_SUBPAGE,
             &ctx->main.controller, other_setting_subpage},
            {IDD_TOP_SUBPAGE, IDC_TOP_SUBPAGE,
             &ctx->main.top, top_setting_subpage},

            {0, 0, NULL, NULL}
        };

        for(i = 0; subpage[i].subpage != NULL; i++) {
            /* get window place and destroy place holder */
            place_holder = GetDlgItem(ctx->page_hwnd, subpage[i].place_id);
            GetWindowRect(place_holder, &rect);
            DestroyWindow(place_holder);

            pt.x = rect.left;
            pt.y = rect.top;
            ScreenToClient(ctx->page_hwnd, &pt);

            /* allocate subpage data */
            *subpage[i].subpage =
                (prop_page_context_t *)malloc(sizeof(prop_page_context_t));
            if(*subpage[i].subpage == NULL) {
                break;
            }

            /* create subpage window */
            memset(*subpage[i].subpage, 0, sizeof(prop_page_context_t));
            (*subpage[i].subpage)->page_proc = subpage[i].page_proc;
            (*subpage[i].subpage)->data = ctx->data;
            (*subpage[i].subpage)->page_hwnd = ctx->page_hwnd;
            (*subpage[i].subpage)->subpage_hwnd =
                CreateDialogParam(instance, MAKEINTRESOURCE(subpage[i].id),
                                  ctx->page_hwnd, prop_subpage_proc,
                                  (LPARAM)*subpage[i].subpage);

            /* hide */
            ShowWindow((*subpage[i].subpage)->subpage_hwnd, SW_HIDE);

            /* move */
            SetWindowPos((*subpage[i].subpage)->subpage_hwnd,
                         ctx->main.tree,
                         pt.x, pt.y, 0, 0,
                         SWP_NOSIZE);
        }
    }

    /* register device notification */
    {
        DEV_BROADCAST_HANDLE broadcast;

        memset(&broadcast, 0, sizeof(broadcast));
        broadcast.dbch_size = sizeof(broadcast);
        broadcast.dbch_devicetype = DBT_DEVTYP_HANDLE;
        broadcast.dbch_handle = ctx->data->hid_device;

        ctx->data->hid_device_notify =
            RegisterDeviceNotification(ctx->page_hwnd, &broadcast,
                                       DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    return TRUE;
}

/* WM_DESTROY */
static INT_PTR main_setting_destroy(HWND hwnd, UINT msg,
                                    WPARAM wparam, LPARAM lparam,
                                    prop_page_context_t *ctx)
{
    int i;
    prop_page_context_t **subpage[] = {
        &ctx->main.event_upper,
        &ctx->main.event_lower,
        &ctx->main.action,
        &ctx->main.action_list,
        &ctx->main.controller,
        &ctx->main.top,

        NULL
    };

    for(i = 0; subpage[i] != NULL; i++) {
        if(*subpage[i] == NULL) {
            continue;
        }

        DestroyWindow((*subpage[i])->subpage_hwnd);
        free(*subpage[i]);
        *subpage[i] = NULL;
    }

    if(ctx->data->hid_device_notify != NULL) {
        UnregisterDeviceNotification(ctx->data->hid_device_notify);
        ctx->data->hid_device_notify = NULL;
    }

    return TRUE;
}

/* PSN_APPLY */
static INT_PTR notify_apply(HWND hwnd, UINT code, NMHDR *nm,
                            prop_page_context_t *ctx)
{
    int ret;

    /* apply to device */
    ret = save_device_setting(&ctx->data->conf);
    if(! ret) {
        display_message_box(IDS_MSG_FAIL_SAVE_SETTING, IDS_MSG_ERROR,
                            ctx->page_hwnd,
                            MB_OK | MB_ICONSTOP);
        SetWindowLong(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
        return TRUE;
    }

    /* indicate to reload */
    if(ctx->data->hid_device != INVALID_HANDLE_VALUE) {
        struct controller_output_report rep;

        memset(&rep, 0, sizeof(rep));
        rep.report_id = 1;
        rep.command = WRITE_REPORT_CMD_RELOAD_SETTING;

        ret = output_to_device(ctx->data, &rep);
        if(! ret) {
            display_message_box(IDS_MSG_FAIL_DEVICE_OUT, IDS_MSG_ERROR,
                                ctx->page_hwnd,
                                MB_OK | MB_ICONSTOP);
            SetWindowLong(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }
    }

    SetWindowLong(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
    return TRUE;
}

/* TVN_SELCHANGED */
static INT_PTR notify_tree_sel_change(HWND hwnd, NMHDR *nm,
                                      prop_page_context_t *ctx)
{
    NMTREEVIEW *nmt;
    tree_item_spec_t *item;

    nmt = (NMTREEVIEW *)nm;
    item = (tree_item_spec_t *)nmt->itemNew.lParam;

    ShowWindow(ctx->main.def_subpage, SW_HIDE);
    ShowWindow(ctx->main.top->subpage_hwnd, SW_HIDE);
    ShowWindow(ctx->main.controller->subpage_hwnd, SW_HIDE);
    ShowWindow(ctx->main.action->subpage_hwnd, SW_HIDE);
    ShowWindow(ctx->main.action_list->subpage_hwnd, SW_HIDE);
    ShowWindow(ctx->main.event_lower->subpage_hwnd, SW_HIDE);
    ShowWindow(ctx->main.event_upper->subpage_hwnd, SW_HIDE);

    if((nmt->itemNew.mask & TVIF_PARAM) == 0) {
        return TRUE;
    }

    switch(item->type) {
      case 0:
          ShowWindow(ctx->main.def_subpage, SW_SHOW);
          break;

      case ITEM_TYPE_ROOT:
          SendMessage(ctx->main.top->subpage_hwnd,
                      WM_SUBPAGE_CHANGE, 0, 0);
          ShowWindow(ctx->main.top->subpage_hwnd, SW_SHOW);
          break;

      case ITEM_TYPE_CONTRLLER_ROOT:
          SendMessage(ctx->main.controller->subpage_hwnd,
                      WM_SUBPAGE_CHANGE, 0, 0);
          ShowWindow(ctx->main.controller->subpage_hwnd, SW_SHOW);
          break;

      case ITEM_TYPE_ACTION:
          ctx->main.action->action.item = item;

          SendMessage(ctx->main.action->subpage_hwnd,
                      WM_SUBPAGE_CHANGE, 0, 0);
          ShowWindow(ctx->main.action->subpage_hwnd, SW_SHOW);

          break;

      case ITEM_TYPE_ACTION_ROOT:
          SendMessage(ctx->main.action_list->subpage_hwnd,
                      WM_SUBPAGE_CHANGE, 0, 0);
          ShowWindow(ctx->main.action_list->subpage_hwnd, SW_SHOW);

          break;

      default:
          if(ITEM_TYPE_UP(item->type) != ITEM_TYPE_CAT_NONE) {
              ctx->main.event_upper->event.type = ITEM_TYPE_UP(item->type);
              ctx->main.event_upper->event.val = item->val1;
              ctx->main.event_upper->event.item = item;

              SendMessage(ctx->main.event_upper->subpage_hwnd,
                          WM_SUBPAGE_CHANGE, 0, 0);
              ShowWindow(ctx->main.event_upper->subpage_hwnd, SW_SHOW);
          }

          if(ITEM_TYPE_DN(item->type) != ITEM_TYPE_CAT_NONE) {
              ctx->main.event_lower->event.type = ITEM_TYPE_DN(item->type);
              if(item->type == ITEM_TYPE_AXIS_HV) {
                  ctx->main.event_lower->event.val = item->val2;
              } else {
                  ctx->main.event_lower->event.val = item->val1;
              }                
              ctx->main.event_lower->event.item = item;

              SendMessage(ctx->main.event_lower->subpage_hwnd,
                          WM_SUBPAGE_CHANGE, 0, 0);
              ShowWindow(ctx->main.event_lower->subpage_hwnd, SW_SHOW);
          }

          break;
    }

    return TRUE;
}

/* MAIN_TREE_SELECT */
static INT_PTR main_setting_tree_select(HWND hwnd, int code,
                                        LPARAM lparam,
                                        prop_page_context_t *ctx)
{
    tree_item_spec_t *item;

    item = (tree_item_spec_t *)lparam;

    if(item->handle != NULL) {
        TVITEM tvitem;

        memset(&tvitem, 0, sizeof(tvitem));
        tvitem.mask = TVIF_HANDLE | TVIF_TEXT;
        tvitem.hItem = item->handle;
        tvitem.pszText = item->name;

        TreeView_SetItem(ctx->main.tree, &tvitem);
    } else {
        add_tree_item(ctx->main.tree, item,
                      ctx->main.action_root->handle, ctx);
    }

    TreeView_SelectItem(ctx->main.tree, item->handle);

    return TRUE;
}

/* MAIN_TREE_ADD_ITEM */
static INT_PTR main_setting_tree_add_item(HWND hwnd, int code,
                                          LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    tree_item_spec_t *item;

    item = (tree_item_spec_t *)lparam;

    add_tree_item(ctx->main.tree, item,
                  ctx->main.action_root->handle, ctx);

    return TRUE;
}

/* MAIN_TREE_DELETE */
static INT_PTR main_setting_tree_delete(HWND hwnd, int code,
                                        LPARAM lparam,
                                        prop_page_context_t *ctx)
{
    tree_item_spec_t *item;

    item = (tree_item_spec_t *)lparam;

    TreeView_DeleteItem(ctx->main.tree, item->handle);

    return TRUE;
}

/* MAIN_TREE_REBUILD */
static INT_PTR main_setting_tree_rebuild(HWND hwnd, int code,
                                         LPARAM lparam,
                                         prop_page_context_t *ctx)
{
    TreeView_DeleteAllItems(ctx->main.tree);

    build_tree(ctx);

    TreeView_SelectItem(ctx->main.tree, custom_tree_root.handle);

    return TRUE;
}

/* MAIN_TREE_REBUILD_ACTION */
static INT_PTR main_setting_tree_rebuild_action(HWND hwnd, int code,
                                                LPARAM lparam,
                                                prop_page_context_t *ctx)
{
    int i;

    for(i = 0; i < ctx->data->conf.num_action_list; i++) {
        TreeView_DeleteItem(ctx->main.tree,
                            ctx->data->conf.action_list[i].tree_item.handle);
    }

    build_action_tree(ctx);

    return TRUE;
}

/* MAIN_TREE_ROOTNAME */
static INT_PTR main_setting_tree_rootname(HWND hwnd, int code,
                                          LPARAM lparam,
                                          prop_page_context_t *ctx)
{
    LPWSTR name;

    name = (LPWSTR)lparam;

    set_tree_root_name(ctx, name);

    return TRUE;
}

/* WM_MAIN_TREE_CHANGE */
static INT_PTR main_setting_change(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam,
                                   prop_page_context_t *ctx)
{
    int code;

    typedef INT_PTR (*subpage_change_proc_t)(HWND, int, LPARAM,
                                             prop_page_context_t *);
    subpage_change_proc_t proc;

    const static struct uint_ptr_pair change_proc[] = {
        {MAIN_TREE_SELECT, main_setting_tree_select},
        {MAIN_TREE_ADD_ITEM, main_setting_tree_add_item},
        {MAIN_TREE_DELETE, main_setting_tree_delete},
        {MAIN_TREE_REBUILD, main_setting_tree_rebuild},
        {MAIN_TREE_REBUILD_ACTION, main_setting_tree_rebuild_action},
        {MAIN_TREE_ROOTNAME, main_setting_tree_rootname},

        {0, NULL}
    };

    code = (int)wparam;

    proc = (subpage_change_proc_t)assq_pair(change_proc, code, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, code, lparam, ctx);
}

/* WM_NOTIFY */
static INT_PTR main_setting_notify(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam,
                                   prop_page_context_t *ctx)
{
    INT_PTR ret;
    NMHDR *nmh;

    ret = window_notify_proc(hwnd, msg, wparam, lparam, ctx);
    if(ret != FALSE) {
        return ret;
    }

    nmh = (NMHDR *)lparam;
    if(nmh->hwndFrom == ctx->data->sheet_hwnd &&
       nmh->code == PSN_APPLY) {
        return notify_apply(hwnd, nmh->code, nmh, ctx);
    }

    return FALSE;
}

/* WM_DEVICECHANGE */
static INT_PTR main_setting_dev_change(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx)
{
    if(wparam != DBT_DEVICEREMOVECOMPLETE &&
       wparam != DBT_DEVICEREMOVEPENDING) {
        return FALSE;
    }

    {
        PDEV_BROADCAST_HDR brd_hdr;

        brd_hdr = (PDEV_BROADCAST_HDR)lparam;

        if(brd_hdr->dbch_devicetype != DBT_DEVTYP_HANDLE) {
            return FALSE;
        }
    }

    {
        PDEV_BROADCAST_HANDLE brd_hdl;

        brd_hdl = (PDEV_BROADCAST_HANDLE)lparam;

        if(brd_hdl->dbch_handle != ctx->data->hid_device) {
            return FALSE;
        }
    }

    if(ctx->data->hid_device_notify != NULL) {
        UnregisterDeviceNotification(ctx->data->hid_device_notify);
        ctx->data->hid_device_notify = NULL;
    }

    if(ctx->data->hid_device != INVALID_HANDLE_VALUE) {
        CloseHandle(ctx->data->hid_device);
        ctx->data->hid_device = INVALID_HANDLE_VALUE;
    }

    display_message_box(IDS_MSG_DEVICE_REMOVED, IDS_MSG_ERROR,
                        ctx->data->sheet_hwnd, MB_OK | MB_ICONWARNING);

    PropSheet_PressButton(ctx->data->sheet_hwnd, PSBTN_CANCEL);

    return TRUE;
}


static void add_tree_item(HWND tree, tree_item_spec_t *spec, HTREEITEM parent,
                          prop_page_context_t *ctx)
{
    TVINSERTSTRUCT ins_info;
    int i;

    memset(&ins_info, 0, sizeof(ins_info));
    ins_info.hParent = parent;
    ins_info.hInsertAfter = TVI_LAST;
    ins_info.item.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
    ins_info.item.state = spec->initial_state;
    ins_info.item.stateMask = spec->initial_state;
    ins_info.item.pszText = spec->name;
    ins_info.item.lParam = (LPARAM)spec;

    spec->handle = TreeView_InsertItem(tree, &ins_info);

    if(spec->type == ITEM_TYPE_ACTION_ROOT) {
        ctx->main.action_root = spec;
    }

    if(spec->sub_item != NULL) {
        for(i = 0; spec->sub_item[i].name != NULL; i++) {
            add_tree_item(tree, &spec->sub_item[i], spec->handle, ctx);
        }
    }
}

static void build_action_tree(prop_page_context_t *ctx)
{
    tree_item_spec_t *spec;
    int i;

    /* add action list to tree */
    for(i = 0; i < ctx->data->conf.num_action_list; i++) {
        spec = &ctx->data->conf.action_list[i].tree_item;

        memset(spec, 0, sizeof(*spec));
        spec->name = ctx->data->conf.action_list[i].name;
        spec->sub_item = NULL;
        spec->type = ITEM_TYPE_ACTION;
        spec->val1 = i;
        spec->initial_state = 0;

        add_tree_item(ctx->main.tree, spec,
                      ctx->main.action_root->handle, ctx);
    }
}

static void set_tree_root_name(prop_page_context_t *ctx, LPWSTR name)
{
    TVITEMW tvitem;

    memset(&tvitem, 0, sizeof(tvitem));
    tvitem.mask = TVIF_HANDLE | TVIF_TEXT;
    tvitem.hItem = custom_tree_root.handle;
    tvitem.pszText = name;

    SendMessage(ctx->main.tree, TVM_SETITEMW, 0, (LPARAM)&tvitem);
}

static void build_tree(prop_page_context_t *ctx)
{
    /* tree */
    add_tree_item(ctx->main.tree, &custom_tree_root, NULL, ctx);

    /* action list tree */
    build_action_tree(ctx);

    /* item name of root */
    set_tree_root_name(ctx, ctx->data->conf.profile_name);
}



void modifier_to_string(LPWSTR str,
                        unsigned short mod, unsigned short mod_mask)
{
    WCHAR buf[32];
    int i;

    str[0] = 0;
    for(i = 1; i <= 16; i++) {
        if((mod_mask & CONTROLLER_MODEFIER(i)) &&
           (mod & CONTROLLER_MODEFIER(i))) {
            if(str[0] == 0) {
                wsprintfW(buf, L"on=%d", i);
            } else {
                wsprintfW(buf, L",%d", i);
            }
            lstrcatW(str, buf);
        }
    }

    int off_first = 1;
    for(i = 1; i <= 16; i++) {
        if((mod_mask & CONTROLLER_MODEFIER(i)) &&
           ! (mod & CONTROLLER_MODEFIER(i))) {
            if(off_first) {
                off_first = 0;
                if(str[0] != 0) {
                    lstrcatW(str, L"/");
                }
                wsprintfW(buf, L"off=%d", i);
            } else {
                wsprintfW(buf, L",%d", i);
            }
            lstrcatW(str, buf);
        }
    }
}


/* page proc alist */
struct uint_ptr_pair main_setting_page[] = {
    {WM_INITDIALOG, main_setting_init_dialog},
    {WM_DESTROY, main_setting_destroy},
    {WM_MAIN_TREE_CHANGE, main_setting_change},
    {WM_NOTIFY, main_setting_notify},
    {WM_DEVICECHANGE, main_setting_dev_change},

    {0, NULL}
};

/* WM_NOTIFY proc */
static struct uint_ptr_pair notify_tree_proc[] = {
    {TVN_SELCHANGED, notify_tree_sel_change},

    {0, NULL}
};
static struct uint_ptr_pair notify_proc_map[] = {
    {IDC_SETTING_TREE, notify_tree_proc},

    {0, NULL}
};
