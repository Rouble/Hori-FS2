/*
 * prop_page_in.c  -- input test property page
 *
 * $Id: prop_page_in.c,v 1.18 2004/03/23 18:56:23 hos Exp $
 *
 */

#include <windows.h>
#include <prsht.h>
#include <process.h>
#include "main.h"
#include "util.h"

#define AXIS_FRONT_COLOR RGB(0x00, 0xff, 0x66)
#define AXIS_BACK_COLOR RGB(0x00, 0x00, 0x00)
#define BUTTON_COLOR(val) RGB(0x00, val, 0x00)
#define MODIFIER_COLOR(val) RGB(0x00, val * 0xff, 0x00)
#define POV_FRONT_COLOR RGB(0x00, 0xff, 0x66)
#define POV_BACK_COLOR RGB(0x00, 0x00, 0x00)


/* prototypes */
static INT_PTR input_test_init_dialog(HWND hwnd, UINT msg,
                                      WPARAM wparam, LPARAM lparam,
                                      prop_page_context_t *ctx);
static INT_PTR input_test_destroy(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx);
static INT_PTR input_test_notify(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam,
                                 prop_page_context_t *ctx);
static INT_PTR input_test_draw_item(HWND hwnd, UINT msg,
                                    WPARAM wparam, LPARAM lparam,
                                    prop_page_context_t *ctx);

static int input_draw_h_axis(HWND hwnd, HDC hdc, UCHAR val);
static int input_draw_v_axis(HWND hwnd, HDC hdc, UCHAR val);
static int input_draw_hv_axis(HWND hwnd, HDC hdc, UCHAR val1, UCHAR val2);
static int input_draw_button(HWND hwnd, HDC hdc, UCHAR val);
static int input_draw_modifier(HWND hwnd, HDC hdc, UCHAR val);
static int input_draw_pov(HWND hwnd, HDC hdc, UCHAR val);

static unsigned int __stdcall input_read_worker(void *arg);


/* page proc alist */
struct uint_ptr_pair input_test_page[] = {
    {WM_INITDIALOG, input_test_init_dialog},
    {WM_DESTROY, input_test_destroy},
    {WM_NOTIFY, input_test_notify},
    {WM_DRAWITEM, input_test_draw_item},

    {0, NULL}
};


/* implementations */

/* WM_INITDIALOG */
static INT_PTR input_test_init_dialog(HWND hwnd, UINT msg,
                                      WPARAM wparam, LPARAM lparam,
                                      prop_page_context_t *ctx)
{
    unsigned int thread_id;

    /* initialize input data */
    memset(&ctx->in.in_rep, 0, sizeof(ctx->in.in_rep));
    memset(&ctx->in.in_rep.axis, 0x7f, sizeof(ctx->in.in_rep.axis));
    ctx->in.in_rep.hat1 = 0xf;
    ctx->in.in_rep.hat2 = 0xf;

    /* create events */
    ctx->in.read_thread_run = CreateEvent(NULL, TRUE, TRUE, NULL);
    ctx->in.read_thread_wakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(ctx->in.read_thread_run == NULL ||
       ctx->in.read_thread_wakeup == NULL) {
        display_message_box(IDS_MSG_FAIL_THREAD_CREATE, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);
        return TRUE;
    }

    /* create thread */
    ctx->in.read_thread = (HANDLE)_beginthreadex(NULL, 0,
                                                 input_read_worker, ctx,
                                                 0, &thread_id);
    if(ctx->in.read_thread == 0) {
        display_message_box(IDS_MSG_FAIL_THREAD_CREATE, IDS_MSG_ERROR,
                            hwnd, MB_OK | MB_ICONSTOP);

        CloseHandle(ctx->in.read_thread_run);
        CloseHandle(ctx->in.read_thread_wakeup);
        return TRUE;
    }

    return TRUE;
}

/* WM_DESTROY */
static INT_PTR input_test_destroy(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam,
                                  prop_page_context_t *ctx)
{
    /* stop thread */
    if(ctx->in.read_thread) {
        ctx->in.read_thread_stop = 1;
        SetEvent(ctx->in.read_thread_run);
        SetEvent(ctx->in.read_thread_wakeup);

        WaitForSingleObject(ctx->in.read_thread, INFINITE);

        CloseHandle(ctx->in.read_thread_run);
        CloseHandle(ctx->in.read_thread_wakeup);
        CloseHandle(ctx->in.read_thread);
    }

    return TRUE;
}

/* WM_NOTIFY */
static INT_PTR input_test_notify(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam,
                                 prop_page_context_t *ctx)
{
    NMHDR *nm;

    nm = (NMHDR *)lparam;

    switch(nm->code) {
      case PSN_SETACTIVE:
          SetEvent(ctx->in.read_thread_run);
          SetWindowLong(hwnd, DWLP_MSGRESULT, 0); /* set result of notify */
          return TRUE;

      case PSN_KILLACTIVE:
          ResetEvent(ctx->in.read_thread_run);
          SetWindowLong(hwnd, DWLP_MSGRESULT, FALSE); /* set result of notify */
          return TRUE;

      default:
          break;
    }

    return FALSE;
}

/* WM_DRAWITEM */
static INT_PTR input_test_draw_item(HWND hwnd, UINT msg,
                                    WPARAM wparam, LPARAM lparam,
                                    prop_page_context_t *ctx)
{
    DRAWITEMSTRUCT *item;

    item = (DRAWITEMSTRUCT *)lparam;

    if(item->itemAction != ODA_DRAWENTIRE) {
        return FALSE;
    }

    if(item->CtlID >= IDC_TEST_IN_AXIS_START &&
       item->CtlID <= IDC_TEST_IN_AXIS_END) {
        switch(item->CtlID) {
          case IDC_TEST_IN_AXIS_XY:
              input_draw_hv_axis(item->hwndItem, item->hDC,
                                 ctx->in.in_rep.x,
                                 ctx->in.in_rep.y);
              return TRUE;

          case IDC_TEST_IN_AXIS_RXRY:
              input_draw_hv_axis(item->hwndItem, item->hDC,
                                 ctx->in.in_rep.rx,
                                 ctx->in.in_rep.ry);
              return TRUE;

          case IDC_TEST_IN_AXIS_Z:
              input_draw_h_axis(item->hwndItem, item->hDC,
                                ctx->in.in_rep.z);
              return TRUE;

          case IDC_TEST_IN_AXIS_RZ:
              input_draw_h_axis(item->hwndItem, item->hDC,
                                ctx->in.in_rep.rz);
              return TRUE;

          case IDC_TEST_IN_AXIS_SLIDER:
              input_draw_h_axis(item->hwndItem, item->hDC,
                                ctx->in.in_rep.slider);
              return TRUE;

          case IDC_TEST_IN_AXIS_THROTTLE:
              input_draw_v_axis(item->hwndItem, item->hDC,
                                ctx->in.in_rep.throttle);
              return TRUE;

        }
    } else if(item->CtlID >= IDC_TEST_IN_BTN_START &&
              item->CtlID <= IDC_TEST_IN_BTN_END) {
        int idx, val;

        idx = item->CtlID - IDC_TEST_IN_BTN_START;
        val = (ctx->in.in_rep.button[idx / 8] >> (idx % 8)) & 0x01;

        input_draw_button(item->hwndItem, item->hDC, val * 0xff);
        return TRUE;
    } else if(item->CtlID >= IDC_TEST_IN_MOD_START &&
              item->CtlID <= IDC_TEST_IN_MOD_END) {
        int v;

        v = (ctx->in.in_rep.modifier >>
             (item->CtlID - IDC_TEST_IN_MOD_START)) & 0x01;

        input_draw_modifier(item->hwndItem, item->hDC, v);
        return TRUE;
    } else if(item->CtlID == IDC_TEST_IN_POV1) {
        input_draw_pov(item->hwndItem, item->hDC, ctx->in.in_rep.hat1);
    } else if(item->CtlID == IDC_TEST_IN_POV2) {
        input_draw_pov(item->hwndItem, item->hDC, ctx->in.in_rep.hat2);
    }

    return TRUE;
}


static int input_draw_h_axis(HWND hwnd, HDC hdc, UCHAR val)
{
    RECT rect, back_rect, val_rect;
    HBRUSH black_brush, brush;

    GetClientRect(hwnd, &rect);
    black_brush = CreateSolidBrush(AXIS_BACK_COLOR);
    brush = CreateSolidBrush(AXIS_FRONT_COLOR);

    CopyRect(&back_rect, &rect);
    back_rect.left = rect.left + (rect.right - rect.left) * val / 0xff;

    CopyRect(&val_rect, &rect);
    val_rect.right = back_rect.left;

    FillRect(hdc, &back_rect, black_brush);
    FillRect(hdc, &val_rect, brush);

    DeleteObject(black_brush);
    DeleteObject(brush);

    return TRUE;
}

static int input_draw_v_axis(HWND hwnd, HDC hdc, UCHAR val)
{
    RECT rect, back_rect, val_rect;
    HBRUSH black_brush, brush;

    GetClientRect(hwnd, &rect);
    black_brush = CreateSolidBrush(AXIS_BACK_COLOR);
    brush = CreateSolidBrush(AXIS_FRONT_COLOR);

    CopyRect(&back_rect, &rect);
    back_rect.bottom = rect.top + (rect.bottom - rect.top) * val / 0xff;

    CopyRect(&val_rect, &rect);
    val_rect.top = back_rect.bottom;

    FillRect(hdc, &back_rect, black_brush);
    FillRect(hdc, &val_rect, brush);

    DeleteObject(black_brush);
    DeleteObject(brush);

    return TRUE;
}

static int input_draw_hv_axis(HWND hwnd, HDC hdc, UCHAR val1, UCHAR val2)
{
    RECT rect;
    HBRUSH black_brush, brush;
    HPEN pen;
    int x, y;

    GetClientRect(hwnd, &rect);
    black_brush = CreateSolidBrush(AXIS_BACK_COLOR);
    brush = CreateSolidBrush(AXIS_FRONT_COLOR);
    pen = CreatePen(PS_SOLID, 0, AXIS_FRONT_COLOR);

    /* normalize rect */
    if((rect.right - rect.left) > (rect.bottom - rect.top)) {
        rect.left = (rect.left + rect.right) / 2 -
                    (rect.bottom - rect.top) / 2;
        rect.right = rect.left + (rect.bottom - rect.top);
    } else {
        rect.top = (rect.top + rect.bottom) / 2 -
                   (rect.right - rect.left) / 2;
        rect.bottom = rect.top + (rect.right - rect.left);
    }

    IntersectClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);

    x = rect.left + (rect.right - rect.left) * val1 / 0xff;
    y = rect.top + (rect.bottom - rect.top) * val2 / 0xff;

    /* clear */
    FillRect(hdc, &rect, black_brush);

    /* draw */
    {
        HGDIOBJ old_brush, old_pen;

        old_brush = SelectObject(hdc, brush);
        old_pen = SelectObject(hdc, pen);

        Arc(hdc,
            x - 10, y - 10,
            x + 10, y + 10,
            x, y - 10,
            x + 10, y);
        Pie(hdc,
            x - 10, y - 10,
            x + 10, y + 10,
            x + 10, y,
            x, y - 10);
        Pie(hdc,
            x - 10, y - 10,
            x + 10, y + 10,
            x - 10, y,
            x, y + 10);

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }

    DeleteObject(black_brush);
    DeleteObject(brush);
    DeleteObject(pen);

    return TRUE;
}

static int input_draw_button(HWND hwnd, HDC hdc, UCHAR val)
{
    RECT rect;
    HBRUSH brush;
    HPEN pen;

    GetClientRect(hwnd, &rect);
    brush = CreateSolidBrush(BUTTON_COLOR(val));
    pen = CreatePen(PS_SOLID, 0, BUTTON_COLOR(val));

    /* normalize rect */
    if((rect.right - rect.left) > (rect.bottom - rect.top)) {
        rect.left = (rect.left + rect.right) / 2 -
                    (rect.bottom - rect.top) / 2;
        rect.right = rect.left + (rect.bottom - rect.top);
    } else {
        rect.top = (rect.top + rect.bottom) / 2 -
                   (rect.right - rect.left) / 2;
        rect.bottom = rect.top + (rect.right - rect.left);
    }

    /* draw */
    {
        HGDIOBJ old_brush, old_pen;

        old_brush = SelectObject(hdc, brush);
        old_pen = SelectObject(hdc, pen);

        Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }

    DeleteObject(brush);
    DeleteObject(pen);

    return TRUE;
}

static int input_draw_modifier(HWND hwnd, HDC hdc, UCHAR val)
{
    RECT rect;
    HBRUSH brush;
    HPEN pen;

    GetClientRect(hwnd, &rect);
    brush = CreateSolidBrush(MODIFIER_COLOR(val));
    pen = CreatePen(PS_SOLID, 0, MODIFIER_COLOR(val));

    /* normalize rect */
    if((rect.right - rect.left) > (rect.bottom - rect.top)) {
        rect.left = (rect.left + rect.right) / 2 -
                    (rect.bottom - rect.top) / 2;
        rect.right = rect.left + (rect.bottom - rect.top);
    } else {
        rect.top = (rect.top + rect.bottom) / 2 -
                   (rect.right - rect.left) / 2;
        rect.bottom = rect.top + (rect.right - rect.left);
    }

    /* draw */
    {
        HGDIOBJ old_brush, old_pen;

        old_brush = SelectObject(hdc, brush);
        old_pen = SelectObject(hdc, pen);

        RoundRect(hdc,
                  rect.left, rect.top, rect.right, rect.bottom,
                  rect.right / 4, rect.bottom / 4);

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }

    DeleteObject(brush);
    DeleteObject(pen);

    return TRUE;
}

static int input_draw_pov(HWND hwnd, HDC hdc, UCHAR val)
{
    RECT rect;
    HBRUSH black_brush, brush;
    HPEN black_pen, pen;
    int center_x, center_y;
    double radius;

    double dir_map[] = {
#define SQRT1_2 0.70710678118654752440
        0, SQRT1_2, 1, SQRT1_2, 0, -SQRT1_2, -1, -SQRT1_2
#undef SQRT1_2
    };

    GetClientRect(hwnd, &rect);
    black_brush = CreateSolidBrush(POV_BACK_COLOR);
    black_pen = CreatePen(PS_SOLID, 0, POV_BACK_COLOR);
    brush = CreateSolidBrush(POV_FRONT_COLOR);
    pen = CreatePen(PS_SOLID, 0, POV_FRONT_COLOR);

    /* normalize rect */
    if((rect.right - rect.left) > (rect.bottom - rect.top)) {
        rect.left = (rect.left + rect.right) / 2 -
                    (rect.bottom - rect.top) / 2;
        rect.right = rect.left + (rect.bottom - rect.top);
    } else {
        rect.top = (rect.top + rect.bottom) / 2 -
                   (rect.right - rect.left) / 2;
        rect.bottom = rect.top + (rect.right - rect.left);
    }

    radius = (rect.right - rect.left) / 2.0;
    center_x = (rect.left + rect.right) / 2;
    center_y = (rect.top + rect.bottom) / 2;

    /* draw */
    {
        HGDIOBJ old_brush, old_pen;

        old_brush = SelectObject(hdc, black_brush);
        old_pen = SelectObject(hdc, black_pen);
        Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

        if(val < 8) {
            SelectObject(hdc, brush);
            SelectObject(hdc, pen);

            Pie(hdc,
                center_x + dir_map[val % 8] * radius - radius,
                center_y + dir_map[(val + 6) % 8] * radius - radius,
                center_x + dir_map[val % 8] * radius + radius,
                center_y + dir_map[(val + 6) % 8] * radius + radius,
                center_x + dir_map[(val + 5) % 8] * 10,
                center_y + dir_map[(val + 3) % 8] * 10,
                center_x + dir_map[(val + 3) % 8] * 10,
                center_y + dir_map[(val + 1) % 8] * 10);
        }

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
    }

    DeleteObject(black_brush);
    DeleteObject(brush);
    DeleteObject(black_pen);
    DeleteObject(pen);

    return TRUE;
}


static unsigned int __stdcall input_read_worker(void *arg)
{
    prop_page_context_t *ctx;
    unsigned char buf[sizeof(struct controller_joy_input_report)];
    DWORD cnt;
    OVERLAPPED ovlp;
    HANDLE read_cmp_evt, evt[2];
    int ret;

    ctx = (prop_page_context_t *)arg;

    /* create event for async-read */
    read_cmp_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(read_cmp_evt == NULL) {
        display_message_box(IDS_MSG_FAIL_THREAD_CREATE, IDS_MSG_ERROR,
                            ctx->page_hwnd, MB_OK | MB_ICONSTOP);
        _endthreadex(0);
    }

    while(1) {
        WaitForSingleObject(ctx->in.read_thread_run, INFINITE);

        if(ctx->in.read_thread_stop) {
            /* exit thread */
            break;
        }

        if(ctx->data->hid_device == INVALID_HANDLE_VALUE) {
            /* exit thread */
            break;
        }

        /* input from device */
        ResetEvent(evt);

        memset(&ovlp, 0, sizeof(ovlp));
        ovlp.hEvent = read_cmp_evt;

        memset(buf, 0, sizeof(buf));
        ReadFile(ctx->data->hid_device,
                 buf, sizeof(buf),
                 NULL, &ovlp);

        /* wait read complete or wakeup event */
        evt[0] = read_cmp_evt;
        evt[1] = ctx->in.read_thread_wakeup;
        ret = WaitForMultipleObjects(2, evt, FALSE, INFINITE);
        if(ret != WAIT_OBJECT_0) {
            /* cancel read operation */
            CancelIo(ctx->data->hid_device);
            continue;
        } else {
            /* read complete */
            ret = GetOverlappedResult(ctx->data->hid_device,
                                      &ovlp, &cnt, FALSE);
            if(ret == 0) {
                /* some error occured */
                continue;
            }
        }

        /* copy data */
        memcpy(&ctx->in.in_rep, buf,
               sizeof(struct controller_joy_input_report));

        /* indicate to redraw */
        {
            HWND ctrl;
            UINT id;
            int i;

            static struct {
                UINT min, max;
            } id_range[] = {
                {IDC_TEST_IN_AXIS_START, IDC_TEST_IN_AXIS_END},
                {IDC_TEST_IN_BTN_START, IDC_TEST_IN_BTN_END},
                {IDC_TEST_IN_MOD_START, IDC_TEST_IN_MOD_END},
                {IDC_TEST_IN_POV1, IDC_TEST_IN_POV2},

                {0, 0}
            };

            for(i = 0; id_range[i].min != 0; i++) {
                for(id = id_range[i].min; id <= id_range[i].max; id++) {
                    ctrl = GetDlgItem(ctx->page_hwnd, id);
                    if(ctrl == NULL) {
                        continue;
                    }

                    InvalidateRect(ctrl, NULL, FALSE);
                }
            }
        }
    }

    CloseHandle(read_cmp_evt);

    /* exit thread */
    _endthreadex(0);
    return 0;                   /* not reach */
}
