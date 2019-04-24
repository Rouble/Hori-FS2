/*
 * main.c  -- entry point and main procedures
 *
 * $Id: main.c,v 1.30 2004/06/11 18:16:21 hos Exp $
 *
 */


#include "main.h"



static NTSTATUS device_add(IN PDRIVER_OBJECT drv_obj,
                                  IN PDEVICE_OBJECT phys_dev_obj);
static VOID driver_unload(IN PDRIVER_OBJECT drv_obj);

static NTSTATUS irp_create(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS irp_close(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS irp_pnp(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS irp_power(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS irp_inter_ioctl(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);

static NTSTATUS inc_req_cnt(IN device_extension_t *dev_ext);
static VOID dec_req_cnt(IN device_extension_t *dev_ext);


/* entry point of device driver */
NTSTATUS DriverEntry(IN PDRIVER_OBJECT drv_obj,
                            IN PUNICODE_STRING reg_path)
{
    HID_MINIDRIVER_REGISTRATION hid_mdrv_regist;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, \"%ws\"", drv_obj, reg_path->Buffer));

    nt_ret = STATUS_SUCCESS;

    /* set dispatcher */
    drv_obj->MajorFunction[IRP_MJ_CREATE] = irp_create;
    drv_obj->MajorFunction[IRP_MJ_CLOSE] = irp_close;
    drv_obj->MajorFunction[IRP_MJ_PNP] = irp_pnp;
    drv_obj->MajorFunction[IRP_MJ_POWER] = irp_power;
    drv_obj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = irp_inter_ioctl;

    drv_obj->DriverUnload = driver_unload;
    drv_obj->DriverExtension->AddDevice = device_add;

    /* register hid minidriver */
    RtlZeroMemory(&hid_mdrv_regist, sizeof(hid_mdrv_regist));
    hid_mdrv_regist.Revision = HID_REVISION;
    hid_mdrv_regist.DriverObject = drv_obj;
    hid_mdrv_regist.RegistryPath = reg_path;
    hid_mdrv_regist.DevicesArePolled = FALSE;
    hid_mdrv_regist.DeviceExtensionSize = sizeof(device_extension_t);

    nt_ret = HidRegisterMinidriver(&hid_mdrv_regist);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}

static VOID driver_unload(IN PDRIVER_OBJECT drv_obj)
{
    DBG_ENTER(("%p", drv_obj));

    DBG_LEAVE(("void"));
    return;
}


static NTSTATUS device_add(IN PDRIVER_OBJECT drv_obj,
                                  IN PDEVICE_OBJECT func_dev_obj)
{
    PDEVICE_OBJECT dev_obj;
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, %p", drv_obj, func_dev_obj));

    /* device object */
    dev_obj = func_dev_obj;
    dev_ext = GET_DEV_EXT(dev_obj);
    RtlZeroMemory(dev_ext, sizeof(device_extension_t));

    /* init devxce extension */
    dev_ext->req_cnt = 1;
    KeInitializeEvent(&dev_ext->remove_event, NotificationEvent, FALSE);

    KeInitializeSpinLock(&dev_ext->pending_read_irp_lock);
    InitializeListHead(&dev_ext->pending_read_irp);

    dev_ext->pnp_state = device_state_starting;
    dev_ext->reload_settingp = TRUE;

    dev_obj->Flags &= ~DO_DEVICE_INITIALIZING;

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

NTSTATUS device_start(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* initialize usb */
    nt_ret = usb_start(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x: usb_start failed", nt_ret));
        return nt_ret;
    }

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

NTSTATUS device_remove(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* finalize usb */
    usb_remove(dev_obj);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS irp_create(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    DBG_ENTER(("%p, %p", dev_obj, irp));

    COMPLETE_REQUEST(irp, 0, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS irp_close(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    DBG_ENTER(("%p, %p", dev_obj, irp));

    COMPLETE_REQUEST(irp, 0, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS irp_pnp(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;
    PIO_STACK_LOCATION irp_sp;
    UCHAR minor_func;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    nt_ret = STATUS_SUCCESS;

    dev_ext = GET_DEV_EXT(dev_obj);
    irp_sp = IoGetCurrentIrpStackLocation(irp);
    minor_func = irp_sp->MinorFunction;

    nt_ret = inc_req_cnt(dev_ext);
    if(! NT_SUCCESS(nt_ret)) {
        COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    DBG_OUT(("pnp: minor func: %02x", minor_func));

    {
        irp_proc_t proc;

        proc = (irp_proc_t)assq_pair(pnp_mn_func_list, minor_func, NULL);
        if(proc != NULL) {
            nt_ret = proc(dev_obj, irp);
        } else {
            DBG_OUT(("pnp: not supported: call next device"));
            IoSkipCurrentIrpStackLocation(irp);
            nt_ret = IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);
        }
    }

    if(minor_func != IRP_MN_REMOVE_DEVICE) {
        dec_req_cnt(dev_ext);
    }

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}


static NTSTATUS irp_power(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;
    PIO_STACK_LOCATION irp_sp;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);
    irp_sp = IoGetCurrentIrpStackLocation(irp);

    nt_ret = inc_req_cnt(dev_ext);
    if(! NT_SUCCESS(nt_ret)) {
        COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    DBG_OUT(("power: minor func: %02x, %u, %u, %u, %u",
             irp_sp->MinorFunction,
             irp_sp->Parameters.Power.Type,
             irp_sp->Parameters.Power.State,
             irp_sp->Parameters.Power.ShutdownType,
             irp_sp->Parameters.Power.SystemContext));

    if(irp_sp->MinorFunction == IRP_MN_SET_POWER &&
       irp_sp->Parameters.Power.Type == DevicePowerState) {
        if(irp_sp->Parameters.Power.State.DeviceState != PowerDeviceD0) {
            dev_ext->pnp_state = device_state_stopped;
        }

        PoSetPowerState(dev_obj, DevicePowerState,
                        irp_sp->Parameters.Power.State);
    }

    IoSkipCurrentIrpStackLocation(irp);
    PoStartNextPowerIrp(irp);
    nt_ret = PoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);

    dec_req_cnt(dev_ext);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}


static NTSTATUS irp_inter_ioctl(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    irp->IoStatus.Information = 0;

    nt_ret = inc_req_cnt(dev_ext);
    if(NT_SUCCESS(nt_ret)) {
        PIO_STACK_LOCATION irp_sp;
        ULONG ioctl_code;
        irp_proc_t proc;

        irp_sp = IoGetCurrentIrpStackLocation(irp);
        ioctl_code = irp_sp->Parameters.DeviceIoControl.IoControlCode;
        DBG_OUT(("inter ioctl: code: %08x", ioctl_code));

        proc = (irp_proc_t)assq_pair(ioctl_func_list, ioctl_code, NULL);

        if(proc != NULL) {
            nt_ret = proc(dev_obj, irp);
        } else {
            DBG_OUT(("inter ioctl: not supported"));
            COMPLETE_REQUEST(irp, 0, STATUS_NOT_SUPPORTED, IO_NO_INCREMENT);
        }

        dec_req_cnt(dev_ext);
    }

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}



static NTSTATUS inc_req_cnt(IN device_extension_t *dev_ext)
{
    NTSTATUS nt_ret;
    LONG cnt;

    DBG_ENTER2(("%p", dev_ext));

    cnt = InterlockedIncrement(&dev_ext->req_cnt);
    DBG_OUT(("increment ref: %d", cnt));

    if(dev_ext->pnp_state == device_state_removed) {
        DBG_OUT(("already removed"));

        cnt = InterlockedDecrement(&dev_ext->req_cnt);
        DBG_OUT(("decrement ref: %d", cnt));
        if(cnt == 0) {
            KeSetEvent(&dev_ext->remove_event, IO_NO_INCREMENT, FALSE);
        }

        nt_ret = STATUS_DELETE_PENDING;
    } else {
        nt_ret = STATUS_SUCCESS;
    }

    DBG_LEAVE2(("%x", nt_ret));
    return nt_ret;
}

static VOID dec_req_cnt(IN device_extension_t *dev_ext)
{
    LONG cnt;

    DBG_ENTER2(("%p", dev_ext));

    cnt = InterlockedDecrement(&dev_ext->req_cnt);
    DBG_OUT(("decrement ref: %d", cnt));
    if(cnt == 0) {
        KeSetEvent(&dev_ext->remove_event, IO_NO_INCREMENT, FALSE);
    }

    DBG_LEAVE2(("void"));
}

void *assq_pair(const struct uint_ptr_pair *pair,
                unsigned long key, void *default_ptr)
{
    while(pair->ptr != (void *)0) {
        if(pair->key == key) {
            return pair->ptr;
        }

        pair++;
    }

    return default_ptr;
}