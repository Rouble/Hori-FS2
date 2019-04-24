/*
 * pnp.c  -- Plug and Play procedures
 *
 * $Id: pnp.c,v 1.17 2004/03/16 12:41:12 hos Exp $
 *
 */


#include "main.h"


static NTSTATUS pnp_start(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS pnp_stop(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS pnp_surprise_remove(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS pnp_remove(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS pnp_capabilities(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);

static NTSTATUS pnp_complete(IN PDEVICE_OBJECT dev_obj, IN PIRP irp,
                                    IN PVOID ctxt);


struct uint_ptr_pair pnp_mn_func_list[] = {
    {IRP_MN_START_DEVICE, pnp_start},
    {IRP_MN_STOP_DEVICE, pnp_stop},
    {IRP_MN_SURPRISE_REMOVAL, pnp_surprise_remove},
    {IRP_MN_REMOVE_DEVICE, pnp_remove},
    {IRP_MN_QUERY_CAPABILITIES, pnp_capabilities},

    {0, NULL}
};


static NTSTATUS pnp_start(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;
    KEVENT start_event;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* start lower device */
    KeInitializeEvent(&start_event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, pnp_complete, &start_event, TRUE, TRUE, TRUE);
    nt_ret = IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_OUT(("%x: IoCallDriver failed", nt_ret));
        goto func_end;
    }

    DBG_OUT(("waiting start event: %p", &start_event));
    KeWaitForSingleObject(&start_event, Executive, KernelMode, FALSE, NULL);

    nt_ret = irp->IoStatus.Status;
    if(! NT_SUCCESS(nt_ret)) {
        DBG_OUT(("%x: calling next device failed", nt_ret));
        goto func_end;
    }

    /* start this device */
    nt_ret = device_start(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_OUT(("%x: device_start failed", nt_ret));
        goto func_end;
    }

  func_end:
    COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}

static NTSTATUS pnp_stop(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* stop this device */
    dev_ext->pnp_state = device_state_stopped;

    /* stop lower device */
    IoSkipCurrentIrpStackLocation(irp);
    nt_ret = IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}

static NTSTATUS pnp_surprise_remove(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* remove this device */
    dev_ext->pnp_state = device_state_removed;
    device_remove(dev_obj);

    /* remove lower device */
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(irp);
    nt_ret = IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}

static NTSTATUS pnp_remove(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* remove this device */
    dev_ext->pnp_state = device_state_removed;
    device_remove(dev_obj);

    {
        LONG cnt;

        InterlockedDecrement(&dev_ext->req_cnt); /* irp_pnp's req */
        cnt = InterlockedDecrement(&dev_ext->req_cnt); /* device_add's req */
        if(cnt > 0) {
            KeWaitForSingleObject(&dev_ext->remove_event,
                                  Executive, KernelMode, FALSE, NULL);
        }
    }

    /* remove lower device */
    IoSkipCurrentIrpStackLocation(irp);
    IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS pnp_capabilities(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PIO_STACK_LOCATION irp_sp;
    DEVICE_CAPABILITIES *dev_caps;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    irp_sp = IoGetCurrentIrpStackLocation(irp);

    dev_caps = irp_sp->Parameters.DeviceCapabilities.Capabilities;
    dev_caps->LockSupported = FALSE;
    dev_caps->EjectSupported = FALSE;
    dev_caps->Removable = TRUE;
    dev_caps->DockDevice = FALSE;
    dev_caps->UniqueID = FALSE;
    dev_caps->SurpriseRemovalOK = TRUE;
    dev_caps->SystemWake = PowerSystemUnspecified;
    dev_caps->DeviceWake = PowerDeviceUnspecified;

    COMPLETE_REQUEST(irp, 0, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS pnp_complete(IN PDEVICE_OBJECT dev_obj, IN PIRP irp,
                                    IN PVOID ctxt)
{
    DBG_ENTER(("%p, %p, %p", dev_obj, irp, ctxt));

    KeSetEvent((PKEVENT)ctxt, IO_NO_INCREMENT, FALSE);

    if(irp->PendingReturned) {
        IoMarkIrpPending(irp);
    }

    DBG_LEAVE(("STATUS_MORE_PROCESSING_REQUIRED"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}
