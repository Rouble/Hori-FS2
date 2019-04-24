/*
 * hid.c  -- HID (Human Interface Device)
 *
 * $Id: hid.c,v 1.47 2004/06/12 06:48:34 hos Exp $
 *
 */


#include "main.h"


#define REPORT_DESCRIPTOR_SIZE (sizeof(hid_report_descriptor))

static UCHAR hid_report_descriptor[] = {
    /* joystick */
    0x05, 0x01,                 /* Usage Page(Generic Desktop) */
    0x09, 0x04,                 /* Usage(Joystick) */
    0xa1, 0x01,                 /* Collection(Application) */

    0x85, 0x01,                 /*   Report ID(1) */

    /* x, y */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x01,                 /*   Usage(Pointer) */
    0xa1, 0x00,                 /*   Collection(Physical) */
    0x15, 0x00,                 /*     Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*     Logical Maximum(0x00ff) */
    0x75, 0x08,                 /*     Report Size(8) */
    0x95, 0x02,                 /*     Report Count(2) */
    0x05, 0x01,                 /*     Usage Page(Generic Desktop) */
    0x09, 0x30,                 /*     Usage(X) */
    0x09, 0x31,                 /*     Usage(Y) */
    0x81, 0x02,                 /*     Input(Variable, Absolute) */
    0xc0,                       /*   End Collection */

    /* z */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x32,                 /*   Usage(Z) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* rx, ry */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x01,                 /*   Usage(Pointer) */
    0xa1, 0x00,                 /*   Collection(Physical) */
    0x15, 0x00,                 /*     Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*     Logical Maximum(0x00ff) */
    0x75, 0x08,                 /*     Report Size(8) */
    0x95, 0x02,                 /*     Report Count(2) */
    0x05, 0x01,                 /*     Usage Page(Generic Desktop) */
    0x09, 0x33,                 /*     Usage(Rx) */
    0x09, 0x34,                 /*     Usage(Ry) */
    0x81, 0x02,                 /*     Input(Variable, Absolute) */
    0xc0,                       /*   End Collection */

    /* rz */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x35,                 /*   Usage(Rz) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* slider */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x36,                 /*   Usage(Slider) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* throttle */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x05, 0x02,                 /*   Usage Page(Simulation) */
    0x09, 0xbb,                 /*   Usage(Throttle) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* hat switch 1 */
    0x75, 0x04,                 /*   Report Size(4) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x25, 0x07,                 /*   Logical Maximum(0x07) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x46, 0x3b, 0x01,           /*   Physical Maximum(0x013b) */
    0x65, 0x14,                 /*   Unit(Degree) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x39,                 /*   Usage(Hat Switch) */
    0x81, 0x42,                 /*   Input(Variable, Absolute, Null State) */

    /* hat switch 2 */
    0x75, 0x04,                 /*   Report Size(4) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x25, 0x07,                 /*   Logical Maximum(0x07) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x46, 0x3b, 0x01,           /*   Physical Maximum(0x013b) */
    0x65, 0x14,                 /*   Unit(Degree) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x39,                 /*   Usage(Hat Switch) */
    0x81, 0x42,                 /*   Input(Variable, Absolute, Null State) */

    /* button x 32 */
    0x75, 0x01,                 /*   Report Size(1) */
    0x95, 0x20,                 /*   Report Count(32) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x25, 0x01,                 /*   Logical Maximum(0x01) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x45, 0x01,                 /*   Physical Maximum(0x01) */
    0x05, 0x09,                 /*   Usage Page(Button) */
    0x19, 0x01,                 /*   Usage Minimum(Button 1) */
    0x29, 0x20,                 /*   Usage Maximum(Button 32) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* modifier state (16bits) */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x02,                 /*   Report Count(2) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x06, 0x00, 0xff,           /*   Usage Page(Vendor defined) */
    0x09, 0x10,                 /*   Usage(Vendor defined 16) */
    0x09, 0x11,                 /*   Usage(Vendor defined 17) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* Output Report */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x06, 0x00, 0xff,           /*   Usage Page(Vendor defined) */
    0x09, 0x01,                 /*   Usage(Vendor defined 1) */
    0x91, 0x02,                 /*   Output(Variable, Absolute) */

    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(1) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x26, 0xff, 0x00,           /*   Logical Maximum(0x00ff) */
    0x06, 0x00, 0xff,           /*   Usage Page(Vendor defined) */
    0x09, 0x02,                 /*   Usage(Vendor defined 2) */
    0x91, 0x02,                 /*   Output(Variable, Absolute) */

    0xc0,                       /* End Collection */


    /* keyboard */
    0x05, 0x01,                 /* Usage Page(Generic Desktop) */
    0x09, 0x06,                 /* Usage(Keyboard) */
    0xa1, 0x01,                 /* Collection(Application) */

    0x85, 0x02,                 /*   Report ID(2) */

    /* key x 232 */
    0x75, 0x01,                 /*   Report Size(1) */
    0x95, 0xe8,                 /*   Report Count(232) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x25, 0x01,                 /*   Logical Maximum(0x01) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x45, 0x01,                 /*   Physical Maximum(0x01) */
    0x05, 0x07,                 /*   Usage Page(Keyboard/Keypad) */
    0x19, 0x00,                 /*   Usage Minimum(0x00) */
    0x29, 0xe7,                 /*   Usage Maximum(0xe7) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    0xc0,                       /* End Collection */


    /* mouse */
    0x05, 0x01,                 /* Usage Page(Generic Desktop) */
    0x09, 0x02,                 /* Usage(Mouse) */
    0xa1, 0x01,                 /* Collection(Application) */

    0x85, 0x03,                 /*   Report ID(3) */

    /* x, y */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x01,                 /*   Usage(Pointer) */
    0xa1, 0x00,                 /*   Collection(Physical) */
    0x15, 0x81,                 /*     Logical Minimum(-0x7f) */
    0x25, 0x7f,                 /*     Logical Maximum(+0x7f) */
    0x35, 0xc0,                 /*     Physical Minimum(-0x40) */
    0x45, 0x40,                 /*     Physical Maximum(+0x40) */
    0x75, 0x08,                 /*     Report Size(8) */
    0x95, 0x02,                 /*     Report Count(2) */
    0x05, 0x01,                 /*     Usage Page(Generic Desktop) */
    0x09, 0x30,                 /*     Usage(X) */
    0x09, 0x31,                 /*     Usage(Y) */
    0x81, 0x06,                 /*     Input(Variable, Relative) */
    0xc0,                       /*   End Collection */

    /* wheel */
    0x15, 0x81,                 /*   Logical Minimum(-0x7f) */
    0x25, 0x7f,                 /*   Logical Maximum(+0x7f) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x45, 0x00,                 /*   Physical Maximum(0x00) */
    0x75, 0x08,                 /*   Report Size(8) */
    0x95, 0x01,                 /*   Report Count(2) */
    0x05, 0x01,                 /*   Usage Page(Generic Desktop) */
    0x09, 0x38,                 /*   Usage(Wheel) */
    0x81, 0x06,                 /*   Input(Variable, Relative) */

    /* button x 5 */
    0x75, 0x01,                 /*   Report Size(1) */
    0x95, 0x05,                 /*   Report Count(5) */
    0x15, 0x00,                 /*   Logical Minimum(0x00) */
    0x25, 0x01,                 /*   Logical Maximum(0x01) */
    0x35, 0x00,                 /*   Physical Minimum(0x00) */
    0x45, 0x01,                 /*   Physical Maximum(0x01) */
    0x05, 0x09,                 /*   Usage Page(Button) */
    0x19, 0x01,                 /*   Usage Minimum(Button 1) */
    0x29, 0x05,                 /*   Usage Maximum(Button 5) */
    0x81, 0x02,                 /*   Input(Variable, Absolute) */

    /* padding (3bits) */
    0x75, 0x01,                 /*   Report Size(1) */
    0x95, 0x03,                 /*   Report Count(3) */
    0x81, 0x01,                 /*   Input(Constant) */

    0xc0,                       /* End Collection */
};


static VOID hid_update_from_raw_input(IN PDEVICE_OBJECT dev_obj);
static VOID hid_generate_raw_output(struct controller_output_report *out_state,
                                    struct controller_raw_output *raw_out);
static VOID hid_worker(IN PVOID ctxt);


NTSTATUS hid_start(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* initialize hid part */
    dev_ext->report_size = REPORT_DESCRIPTOR_SIZE;
    dev_ext->mouse_next_time = 0;
    RtlZeroMemory(&dev_ext->raw_input.ir, sizeof(dev_ext->raw_input.ir));
    RtlFillMemory(&dev_ext->raw_input.vr_00,
                  sizeof(dev_ext->raw_input.vr_00), 0xff);
    RtlFillMemory(&dev_ext->raw_input.vr_01,
                  sizeof(dev_ext->raw_input.vr_01), 0xff);

    /* start hid worker */
    {
        HANDLE thread;
        NTSTATUS nt_ret;

        KeInitializeEvent(&dev_ext->hid_worker_wakeup_event,
                          SynchronizationEvent, FALSE);

        /* create thread and start */
        dev_ext->hid_worker_thread_obj = NULL;
        nt_ret = PsCreateSystemThread(&thread,
                                      THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                      hid_worker, dev_obj);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_LEAVE(("%x: PsCreateSystemThread failed", nt_ret));
            return nt_ret;
        }

        ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL, KernelMode,
                                  &dev_ext->hid_worker_thread_obj, NULL);
    }

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

NTSTATUS hid_remove(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* wait for hid worker stop */
    if(dev_ext->hid_worker_thread_obj != NULL) {
        KeSetEvent(&dev_ext->hid_worker_wakeup_event, IO_NO_INCREMENT, FALSE);
        KeWaitForSingleObject(dev_ext->hid_worker_thread_obj,
                              Executive, KernelMode, FALSE, NULL);

        ObDereferenceObject(dev_ext->hid_worker_thread_obj);
        dev_ext->hid_worker_thread_obj = NULL;
    }

    /* free memory */
    free_settings(dev_obj);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


NTSTATUS hid_get_rep_desc(IN PDEVICE_OBJECT dev_obj,
                          OUT UCHAR *buf, OUT ULONG *size)
{
    DBG_ENTER(("%p, %p, %p", dev_obj, buf, size));

    if(*size < REPORT_DESCRIPTOR_SIZE) {
        DBG_LEAVE(("STATUS_BUFFER_TOO_SMALL"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(buf, hid_report_descriptor, REPORT_DESCRIPTOR_SIZE);
    *size = REPORT_DESCRIPTOR_SIZE;

    DBG_LEAVE(("STATUS_SUCCESS: %u", *size));
    return STATUS_SUCCESS;
}


NTSTATUS hid_read_rep(IN PDEVICE_OBJECT dev_obj,
                      OUT UCHAR *buf, OUT ULONG *size)
{
    device_extension_t *dev_ext;
    void *rep_buf;
    ULONG rep_size, in_type;

    DBG_ENTER(("%p, %p, %u", dev_obj, buf, *size));

    dev_ext = GET_DEV_EXT(dev_obj);

    if(dev_ext->hid_input_changed & INPUT_TYPE_JOYSTICK) {
        /* joystick */
        in_type = INPUT_TYPE_JOYSTICK;
        rep_buf = &dev_ext->input_state.joy;
        rep_size = sizeof(dev_ext->input_state.joy);
    } else if(dev_ext->hid_input_changed & INPUT_TYPE_KEYBOARD) {
        /* keyboard */
        in_type = INPUT_TYPE_KEYBOARD;
        rep_buf = &dev_ext->input_state.kbd;
        rep_size = sizeof(dev_ext->input_state.kbd);
    } else if(dev_ext->hid_input_changed & INPUT_TYPE_MOUSE) {
        /* mouse */
        in_type = INPUT_TYPE_MOUSE;
        rep_buf = &dev_ext->input_state.mou;
        rep_size = sizeof(dev_ext->input_state.mou);
    } else {
        DBG_LEAVE(("STATUS_UNSUCCESSFUL: not changed: %x",
                   dev_ext->hid_input_changed));
        return STATUS_UNSUCCESSFUL;
    }
    DBG_OUT(("input type: %x/%x, %p, %d, %d",
             in_type, dev_ext->hid_input_changed,
             rep_buf, rep_size, *((UCHAR *)rep_buf)));

    if(*size < rep_size) {
        DBG_LEAVE(("STATUS_BUFFER_TOO_SMALL: %d", *size));
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* copy data */
    RtlCopyMemory(buf, rep_buf, rep_size);
    *size = rep_size;

    /* drop changed bit */
    dev_ext->hid_input_changed &= ~in_type;
    DBG_OUT2(("input type (dropped): %x", dev_ext->hid_input_changed));

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


NTSTATUS hid_write_rep(IN PDEVICE_OBJECT dev_obj,
                       IN UCHAR *buf, IN ULONG *size, IN UCHAR id)
{
    device_extension_t *dev_ext;
    struct controller_output_report out_state;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %p, %u, %u", dev_obj, buf, *size, id));

    dev_ext = GET_DEV_EXT(dev_obj);
    nt_ret = STATUS_SUCCESS;

    if(*size > sizeof(out_state)) {
        *size = sizeof(out_state);
    }

    RtlZeroMemory(&out_state, sizeof(out_state));
    RtlCopyMemory(&out_state, buf, *size);

    switch(out_state.command) {
      case WRITE_REPORT_CMD_VIBLATION:
      {
          struct controller_raw_output raw_out;
          ULONG raw_size;

          hid_generate_raw_output(&out_state, &raw_out);

          DBG_OUT(("write report: vr0c: %02x (%02x)",
                   raw_out.vr_0c.viblation, out_state.value));
          raw_size = sizeof(raw_out.vr_0c);
          nt_ret = usb_put_vendor_request(dev_obj, &raw_out.vr_0c, &raw_size,
                                          0x0c, 0x01);
          break;
      }

      case WRITE_REPORT_CMD_RELOAD_SETTING:
          dev_ext->reload_settingp = TRUE;
          break;

      default:
          DBG_OUT(("unknown write command: 0x%x", out_state.command));
          break;
    }

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}


static VOID hid_update_from_raw_input(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    struct controller_raw_input *raw_in;
    struct controller_input_report in_state;
    ULONG changed_flag;

    DBG_ENTER2(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    raw_in = &dev_ext->raw_input;
    in_state = dev_ext->input_state;

    /* process input data */
    process_raw_input(dev_obj, raw_in, &in_state);

    /* check state change */
    changed_flag = 0;

    /* joystick state */
    if(RtlCompareMemory(&dev_ext->input_state.joy, &in_state.joy,
                        sizeof(in_state.joy)) != sizeof(in_state.joy)) {
        DBG_OUT2(("joystick changed"));
        changed_flag |= INPUT_TYPE_JOYSTICK;
    }

    /* keyboard state */
    if(RtlCompareMemory(&dev_ext->input_state.kbd, &in_state.kbd,
                        sizeof(in_state.kbd)) != sizeof(in_state.kbd)) {
        DBG_OUT2(("keyboard changed"));
        changed_flag |= INPUT_TYPE_KEYBOARD;
    }

    /* mouse state */
    if(in_state.mou.x != 0 ||
       in_state.mou.y != 0 ||
       in_state.mou.wheel != 0 ||
       RtlCompareMemory(&dev_ext->input_state.mou.button, &in_state.mou.button,
                        sizeof(in_state.mou.button)) !=
       sizeof(in_state.mou.button)) {
        ULONGLONG cur_time;

        cur_time = KeQueryInterruptTime();
        if(cur_time >= dev_ext->mouse_next_time) { /* only for each 10ms */
            DBG_OUT2(("mouse changed"));
            changed_flag |= INPUT_TYPE_MOUSE;
            dev_ext->mouse_next_time = cur_time + 10 * 10 * 1000;
        }
    }

    /* save current state */
    dev_ext->input_state = in_state;
    dev_ext->hid_input_changed |= changed_flag;

    /* notify to ioctl layer */
    if(dev_ext->hid_input_changed) {
        notify_input_report_change(dev_obj);
    }

    DBG_LEAVE2(("void"));
}

static VOID hid_generate_raw_output(struct controller_output_report *out_state,
                                    struct controller_raw_output *raw_out)
{
    DBG_ENTER(("%p, %p", out_state, raw_out));

    RtlZeroMemory(raw_out, sizeof(*raw_out));

    raw_out->vr_0c.viblation = out_state->value;

    DBG_LEAVE(("void"));
}


static VOID hid_worker(IN PVOID ctxt)
{
    PDEVICE_OBJECT dev_obj;
    device_extension_t *dev_ext;
    LARGE_INTEGER time;

    DBG_ENTER(("%p", ctxt));

    dev_obj = (PDEVICE_OBJECT)ctxt;
    dev_ext = GET_DEV_EXT(dev_obj);

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while(dev_ext->pnp_state == device_state_started) {
        if(dev_ext->reload_settingp) {
            /* reload setting */
            load_settings(dev_obj);
            dev_ext->reload_settingp = FALSE;

            /* initialize current input state */
            RtlZeroMemory(&dev_ext->pp_input, sizeof(dev_ext->pp_input));

            RtlZeroMemory(&dev_ext->pr_input, sizeof(dev_ext->pr_input));
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_X] = 0x7f;
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_Y] = 0x7f;
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_Z] = 0x7f;
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_RX] = 0x7f;
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_RY] = 0x7f;
            dev_ext->pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_RZ] = 0x7f;
            dev_ext->
                pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_SLIDER] = 0x7f;
            dev_ext->
                pr_input.joy.val[CONTROLLER_ACTION_JOY_CODE_THROTTLE] = 0x7f;
            dev_ext->pr_input.mou.val[CONTROLLER_ACTION_MOU_CODE_X] = 0x7f;
            dev_ext->pr_input.mou.val[CONTROLLER_ACTION_MOU_CODE_Y] = 0x7f;
            dev_ext->pr_input.mou.val[CONTROLLER_ACTION_MOU_CODE_WHEEL] = 0x7f;

            RtlZeroMemory(&dev_ext->input_state, sizeof(dev_ext->input_state));
            dev_ext->input_state.joy.report_id = 1;
            dev_ext->input_state.kbd.report_id = 2;
            dev_ext->input_state.mou.report_id = 3;

            RtlFillMemory(&dev_ext->input_state.joy.axis,
                          sizeof(dev_ext->input_state.joy.axis), 0x7f);
            dev_ext->input_state.joy.modifier =
                (USHORT)dev_ext->conf.initial_modifier;

            dev_ext->hid_input_changed = INPUT_TYPE_JOYSTICK |
                                         INPUT_TYPE_KEYBOARD |
                                         INPUT_TYPE_MOUSE;
        }

        KeResetEvent(&dev_ext->hid_worker_wakeup_event);

        /* update input report data */
        hid_update_from_raw_input(dev_obj);

        /* wait for wakeup or 10ms */
        time = RtlConvertLongToLargeInteger(-10 * 1000 * 10);
        KeWaitForSingleObject(&dev_ext->hid_worker_wakeup_event, Executive,
                              KernelMode, FALSE, &time);
    }

    DBG_OUT(("already stop/removed"));
    notify_input_report_stop(dev_obj);

    DBG_LEAVE(("STATUS_SUCCESS"));
    PsTerminateSystemThread(STATUS_SUCCESS);
}
