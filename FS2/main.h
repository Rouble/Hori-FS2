/*
 * main.h  -- difinition of main.c, pnp.c, ioctl.c
 *
 * $Id: main.h,v 1.43 2004/03/28 18:15:12 hos Exp $
 *
 */


#pragma pack(push, 8)

//#undef __cplusplus
#include "ntddk.h"
#include "usbdi.h"
#include "usbd_lib.h"
#include "usbioctl.h"
#include "hidclass.h"
#include "hid_port.h"
#include "intsafe.h"
#include "WINDEF.H"

#include "report_data.h"

#include "util.h"


typedef signed char SCHAR;

/* input: interrupt */
struct controller_raw_input_ir {
    UCHAR stick_x;              /* stick (left - right, 0x00 - 0xff) */
    UCHAR stick_y;              /* stick (top - bottom, 0x00 - 0xff)  */
    UCHAR rudder;               /* rudder (left - right, 0x00 - 0xff) */
    UCHAR throttle;             /* throttle (top - bottom, 0x00 - 0xff) */
    UCHAR hat_x;                /* hat (left - right, 0x00 - 0xff) */
    UCHAR hat_y;                /* hat (top - bottom, 0x00 - 0xff) */
    UCHAR button_a;             /* button A (press - press, 0x00 - 0xff) */
    UCHAR button_b;             /* button B (press - press, 0x00 - 0xff) */
};

/* input: vendor request 0x00 */
struct controller_raw_input_vr_00 {
    UCHAR fire_c : 1;           /* button fire-c */
    UCHAR button_d : 1;         /* button D */
    UCHAR hat : 1;              /* hat press */
    UCHAR button_st : 1;        /* button ST */

    UCHAR dpad1_top : 1;        /* d-pad 1 top */
    UCHAR dpad1_right : 1;      /* d-pad 1 right */
    UCHAR dpad1_bottom : 1;     /* d-pad 1 bottom */
    UCHAR dpad1_left : 1;       /* d-pad 1 left */

    UCHAR reserved4 : 4;

    UCHAR reserved3 : 1;
    UCHAR launch : 1;           /* button lauch */
    UCHAR trigger : 1;          /* trigger */
    UCHAR reserved2 : 1;
};

/* input: vendor request 0x01 */
struct controller_raw_input_vr_01 {
    UCHAR reserved2 : 4;

    UCHAR dpad3_right : 1;      /* d-pad 3 right */
    UCHAR dpad3_middle : 1;     /* d-pad 3 middle */
    UCHAR dpad3_left : 1;       /* d-pad 3 left */
    UCHAR reserved1 : 1;

    UCHAR mode_select : 2;      /* mode select (M1 - M2 - M3, 2 - 1 - 3) */
    UCHAR reserved3 : 1;
    UCHAR button_sw1 : 1;       /* button sw-1 */

    UCHAR dpad2_top : 1;        /* d-pad 2 top */
    UCHAR dpad2_right : 1;      /* d-pad 2 right */
    UCHAR dpad2_bottom : 1;     /* d-pad 2 bottom */
    UCHAR dpad2_left : 1;       /* d-pad 2 left */
};

/* raw input */
struct controller_raw_input {
    struct controller_raw_input_ir ir;
    struct controller_raw_input_vr_00 vr_00;
    struct controller_raw_input_vr_01 vr_01;
};

/* pre processed input data */
struct controller_pp_input {
    UCHAR val[CONTROLLER_NUM_EVENT];
};

/* processed input data */
#define CONTROLLER_PR_NUM_JOY_VAL (CONTROLLER_ACTION_JOY_CODE_MAX + 1)
#define CONTROLLER_PR_NUM_MOU_VAL (CONTROLLER_ACTION_MOU_CODE_MAX + 1)

struct controller_pr_input {
    struct {
        UCHAR val[CONTROLLER_PR_NUM_JOY_VAL];
    } joy;

    struct {
        UCHAR val[(INPUT_REPORT_NUM_KBD_CODE + 7) / 8];
    } kbd;

    struct {
        UCHAR val[CONTROLLER_PR_NUM_MOU_VAL];
    } mou;
};

/* output: vendor request 0x0c */
struct controller_raw_output_vr_0c {
    UCHAR viblation;            /* viblation (stop - full, 0x00 - 0xff) */
};

/* output */
struct controller_raw_output {
    struct controller_raw_output_vr_0c vr_0c;
};

/* event configuration */
struct controller_event_config {
    int num_mod;
    struct controller_modifier_config *mod;
};

/* action list configuration */
struct controller_action_config_list {
    int num_action;
    struct controller_action_config *action;
};

/* configuration */
struct controller_config {
    DWORD button_a_max;
    DWORD button_b_max;

    DWORD button_threshold;
    DWORD lower_threshold;
    DWORD upper_threshold;

    DWORD initial_modifier;

    /* event configuration */
    struct controller_event_config motion_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config press_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config release_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config lower_in_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config lower_out_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config upper_in_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config upper_out_event[CONTROLLER_NUM_EVENT];

    /* list of action list */
    int num_action_list;
    struct controller_action_config_list *action_list;
};

struct controller_pending_action_list {
    ULONGLONG time;

    int num_action;
    struct controller_action_config *action;
    UCHAR val;

    struct controller_pending_action_list *next;
};

struct controller_input_report {
    struct controller_joy_input_report joy;
    struct controller_kbd_input_report kbd;
    struct controller_mou_input_report mou;
};

typedef enum device_pnp_state {
    device_state_starting,
    device_state_started,
    device_state_stopped,
    device_state_removed,

    device_state_unknown
} device_pnp_state_t;


/* device extension */
struct device_extension {
    /* flags */
    device_pnp_state_t pnp_state;

    BOOLEAN reload_settingp;

    /* request count */
    LONG req_cnt;

    /* remove event */
    KEVENT remove_event;

    PVOID usb_worker_thread_obj;

    PVOID hid_worker_thread_obj;
    KEVENT hid_worker_wakeup_event;

    /* usb datas */
    USHORT report_size;
    USHORT vendor_id;
    USHORT product_id;

    /* usb handles */
    USBD_CONFIGURATION_HANDLE usb_conf_handle;
    USBD_PIPE_HANDLE usb_input_pipe;
    USBD_PIPE_HANDLE usb_output_pipe;

    /* in/out state */
    struct controller_raw_input raw_input;
    struct controller_pp_input pp_input;
    struct controller_pr_input pr_input;
    struct controller_input_report input_state;
    ULONG hid_input_changed;
    ULONGLONG mouse_next_time;

    /* pending read report request */
    LIST_ENTRY pending_read_irp;
    KSPIN_LOCK pending_read_irp_lock;

    /* pending actions */
    struct controller_pending_action_list pending_action;

    /* config settings */
    struct controller_config conf;
};
typedef struct device_extension device_extension_t;

#define GET_HID_DEV_EXT(dev_obj) \
  ((PHID_DEVICE_EXTENSION)(dev_obj)->DeviceExtension)
#define GET_NEXT_DEVICE_OBJECT(dev_obj) \
  (GET_HID_DEV_EXT(dev_obj)->NextDeviceObject)
#define GET_DEV_EXT(dev_obj) \
  ((device_extension_t *)GET_HID_DEV_EXT(dev_obj)->MiniDeviceExtension)

#define INPUT_TYPE_JOYSTICK 0x01
#define INPUT_TYPE_KEYBOARD 0x02
#define INPUT_TYPE_MOUSE    0x04


/* misc */
#define COMPLETE_REQUEST(irp, info, status, pri) \
  (irp)->IoStatus.Information = (info); \
  (irp)->IoStatus.Status = (status); \
  IoCompleteRequest((irp), (pri));


#ifndef USBD_TRANSFER_DIRECTION_IN
#define USBD_TRANSFER_DIRECTION_IN 0x01
#endif

#ifndef USBD_SHORT_TRANSFER_OK
#define USBD_SHORT_TRANSFER_OK 0x02
#endif


typedef NTSTATUS (*irp_proc_t)(IN PDEVICE_OBJECT, IN PIRP);

extern struct uint_ptr_pair pnp_mn_func_list[];
extern struct uint_ptr_pair ioctl_func_list[];


NTSTATUS device_start(IN PDEVICE_OBJECT dev_obj);
NTSTATUS device_remove(IN PDEVICE_OBJECT dev_obj);

VOID load_settings(IN PDEVICE_OBJECT dev_obj);
VOID free_settings(IN PDEVICE_OBJECT dev_obj);

NTSTATUS usb_start(IN PDEVICE_OBJECT dev_obj);
NTSTATUS usb_remove(IN PDEVICE_OBJECT dev_obj);
NTSTATUS usb_get_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                    IN PVOID buf, ULONG *buf_size);
NTSTATUS usb_put_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                    IN PVOID buf, ULONG *buf_size);

NTSTATUS usb_get_vendor_request(IN PDEVICE_OBJECT dev_obj,
                                IN PVOID buf, ULONG *buf_size,
                                IN UCHAR request, IN USHORT index);
NTSTATUS usb_put_vendor_request(IN PDEVICE_OBJECT dev_obj,
                                IN PVOID buf, ULONG *buf_size,
                                IN UCHAR request, IN USHORT index);

VOID notify_input_report_change(IN PDEVICE_OBJECT dev_obj);
VOID notify_input_report_stop(IN PDEVICE_OBJECT dev_obj);

NTSTATUS hid_start(IN PDEVICE_OBJECT dev_obj);
NTSTATUS hid_remove(IN PDEVICE_OBJECT dev_obj);
NTSTATUS hid_get_rep_desc(IN PDEVICE_OBJECT dev_obj, BYTE *buf, ULONG *size);
NTSTATUS hid_read_rep(IN PDEVICE_OBJECT dev_obj,
                      OUT BYTE *buf, OUT ULONG *size);
NTSTATUS hid_write_rep(IN PDEVICE_OBJECT dev_obj,
                       IN BYTE *buf, IN ULONG *size, IN UCHAR id);

VOID process_raw_input(IN PDEVICE_OBJECT dev_obj,
                       IN struct controller_raw_input *raw_in,
                       OUT struct controller_input_report *in_state);
VOID free_pending_action(IN PDEVICE_OBJECT dev_obj);

void *assq_pair(const struct uint_ptr_pair *pair,
                unsigned long key, void *default_ptr);


/* dbg: output debug info */

#define DBG_OUT(_x_)                            \
  KdPrint(("%s: %d: ", __FILE__, __LINE__));    \
  KdPrint(_x_);                                 \
  KdPrint(("\n"));

#define DBG_ENTER(_x_)                                                  \
  KdPrint(("%s: %d: enter: %s: ", __FILE__, __LINE__, __FUNCTION__));   \
  KdPrint(_x_);                                                         \
  KdPrint(("\n"));

#define DBG_LEAVE(_x_)                                                  \
  KdPrint(("%s: %d: leave: %s: ", __FILE__, __LINE__, __FUNCTION__));   \
  KdPrint(_x_);                                                         \
  KdPrint(("\n"));

#if DBG
#define DBG_OUT_IF(cond, _x_) if(cond) { DBG_OUT(_x_); }
#else
#define DBG_OUT_IF(cond, _x_)
#endif

#if DBG == 2
#define DBG_OUT2(_x_) DBG_OUT(_x_)
#define DBG_ENTER2(_x_) DBG_ENTER(_x_)
#define DBG_LEAVE2(_x_) DBG_LEAVE(_x_)
#else
#define DBG_OUT2(_x_)
#define DBG_ENTER2(_x_)
#define DBG_LEAVE2(_x_)
#endif
