/*
 * prop_page_other.c  -- misc setting property page
 *
 * $Id: prop_subpage_other.c,v 1.14 2004/03/18 17:42:08 hos Exp $
 *
 */

#include <windows.h>
#include <prsht.h>
#include "main.h"
#include "util.h"


/* prototypes */
static void update_viblation(UCHAR val, prop_page_context_t *ctx);


/* page proc alist */
struct uint_ptr_pair other_setting_subpage[];
struct uint_ptr_pair command_proc_map[];


/* implementations */

/* WM_INITDIALOG */
static INT_PTR other_setting_init_dialog(HWND hwnd, UINT msg,
                                       WPARAM wparam, LPARAM lparam,
                                       prop_page_context_t *ctx)
{
    int i;
    struct uint_ptr_pair id_item_map[] = {
        {IDC_BTN_SENSITIVITY_SLIDER, &ctx->other.btn_sensitivity},
        {IDC_BTN_THRESHOLD_SLIDER, &ctx->other.btn_threshold},
        {IDC_AXIS_THRESHOLD_SLIDER, &ctx->other.axis_threshold},
        {IDC_ENABLE_VIBLATION_BUTTON, &ctx->other.viblation_check},
        {IDC_TEST_VIBLATION_SLIDER, &ctx->other.viblation},

        {0, NULL}
    };

    get_dialog_item(hwnd, id_item_map);
    ctx->command_proc = command_proc_map;

    {
        struct {
            HWND slider;
            int from;
            int to;
        } slider[] = {
            {ctx->other.viblation, 0x00, 0xff},
            {ctx->other.btn_sensitivity, 0x00, 0xff},
            {ctx->other.btn_threshold, 0x00, 0xff},
            {ctx->other.axis_threshold, 0x00, 0x7f},

            {NULL, 0, 0}
        };

        for(i = 0; slider[i].slider != NULL; i++) {
            /* set range */
            SendMessage(slider[i].slider, TBM_SETRANGE,
                        FALSE, MAKELONG(slider[i].from, slider[i].to));
        }
    }

    SendMessage(ctx->other.viblation, TBM_SETPOS, TRUE, 0x00);

    return TRUE;
}

/* WM_DESTROY */
static INT_PTR other_setting_destroy(HWND hwnd, UINT msg,
                                     WPARAM wparam, LPARAM lparam,
                                     prop_page_context_t *ctx)
{
    /* reset device */
    update_viblation(0, ctx);

    return PSNRET_NOERROR;
}

/* IDC_ENABLE_VIBLATION_BUTTON */
static INT_PTR enable_viblation_btn(HWND hwnd, WORD code,
                                    WORD control,
                                    prop_page_context_t *ctx)
{
    PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);

    return TRUE;
}

/* WM_HSCROLL */
static INT_PTR other_setting_hscroll(HWND hwnd, UINT msg,
                                     WPARAM wparam, LPARAM lparam,
                                     prop_page_context_t *ctx)
{
    HWND ctl_hwnd;

    ctl_hwnd = (HWND)lparam;

    if(ctl_hwnd == ctx->other.viblation) {
        update_viblation(SendMessage(ctx->other.viblation, TBM_GETPOS, 0, 0),
                         ctx);
        return TRUE;
    }

    {
        struct {
            HWND slider;
            DWORD *val;
        } slider[] = {
            {ctx->other.btn_sensitivity,
             &ctx->data->conf.btn_a_sensitivity},
            {ctx->other.btn_sensitivity,
             &ctx->data->conf.btn_b_sensitivity},
            {ctx->other.btn_threshold,
             &ctx->data->conf.btn_threshold},
            {ctx->other.axis_threshold,
             &ctx->data->conf.lower_threshold},

            {NULL, NULL}
        };
        int i;

        for(i = 0; slider[i].slider != NULL; i++) {
            *slider[i].val = SendMessage(slider[i].slider, TBM_GETPOS, 0, 0);
        }

        ctx->data->conf.btn_a_sensitivity =
            0xff - ctx->data->conf.btn_a_sensitivity;
        ctx->data->conf.btn_b_sensitivity =
            0xff - ctx->data->conf.btn_b_sensitivity;

        ctx->data->conf.upper_threshold =
            0xff - ctx->data->conf.lower_threshold;

        PropSheet_Changed(ctx->data->sheet_hwnd, ctx->page_hwnd);
    }

    return TRUE;
}

/* WM_SUBPAGE_CHANGE */
static INT_PTR other_setting_change(HWND hwnd, UINT msg,
                                    WPARAM wparam, LPARAM lparam,
                                    prop_page_context_t *ctx)
{
    int i;

    struct {
        HWND slider;
        int pos;
    } slider[] = {
        {ctx->other.btn_sensitivity, 0xff - ctx->data->conf.btn_a_sensitivity},
        {ctx->other.btn_threshold, ctx->data->conf.btn_threshold},
        {ctx->other.axis_threshold, ctx->data->conf.lower_threshold},

        {NULL, 0}
    };

    for(i = 0; slider[i].slider != NULL; i++) {
        /* set slider pos */
        SendMessage(slider[i].slider, TBM_SETPOS, TRUE, slider[i].pos);
    }

    return TRUE;
}

static void update_viblation(UCHAR val, prop_page_context_t *ctx)
{
    struct controller_output_report rep;

    memset(&rep, 0, sizeof(rep));
    rep.command = WRITE_REPORT_CMD_VIBLATION;
    rep.report_id = 1;
    rep.value = val;

    output_to_device(ctx->data, &rep);
}


/* page proc alist */
struct uint_ptr_pair other_setting_subpage[] = {
    {WM_INITDIALOG, other_setting_init_dialog},
    {WM_DESTROY, other_setting_destroy},
    {WM_HSCROLL, other_setting_hscroll},
    {WM_SUBPAGE_CHANGE, other_setting_change},
    {WM_COMMAND, window_command_proc},

    {0, NULL}
};


/* WM_COMMAND */
static struct uint_ptr_pair viblation_btn_proc[] = {
    {BN_CLICKED, enable_viblation_btn},

    {0, NULL}
};
static struct uint_ptr_pair command_proc_map[] = {
    {IDC_ENABLE_VIBLATION_BUTTON, viblation_btn_proc},

    {0, NULL}
};
