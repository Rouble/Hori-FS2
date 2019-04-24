/*
 * ioctl.c  -- ioctl procedures
 *
 * $Id: ioctl.c,v 1.18 2004/03/22 04:33:00 hos Exp $
 *
 */


#include "main.h"



static NTSTATUS ioctl_get_dev_desc(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS ioctl_get_dev_attr(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS ioctl_get_rep_desc(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS ioctl_read_rep(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
static NTSTATUS ioctl_write_rep(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);

static VOID cancel_read_report(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);


struct uint_ptr_pair ioctl_func_list[] = {
    {IOCTL_HID_GET_DEVICE_DESCRIPTOR, ioctl_get_dev_desc},
    {IOCTL_HID_GET_DEVICE_ATTRIBUTES, ioctl_get_dev_attr},
    {IOCTL_HID_GET_REPORT_DESCRIPTOR, ioctl_get_rep_desc},
    {IOCTL_HID_READ_REPORT, ioctl_read_rep},
    {IOCTL_HID_WRITE_REPORT, ioctl_write_rep},

    {0, NULL}
};



static PIRP dequeue_pending_read_irp(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    PIRP irp;
    PLIST_ENTRY entry;
    KIRQL irql;

    DBG_ENTER2(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    do {
        if(IsListEmpty(&dev_ext->pending_read_irp)) {
            DBG_LEAVE2(("NULL: pending_read_irp is empty"));
            return NULL;
        }

        KeAcquireSpinLock(&dev_ext->pending_read_irp_lock, &irql);
        entry = RemoveHeadList(&dev_ext->pending_read_irp);
        InitializeListHead(entry);
        KeReleaseSpinLock(&dev_ext->pending_read_irp_lock, irql);

        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        if(IoSetCancelRoutine(irp, NULL) == NULL) {
            DBG_OUT(("already cancelling: %p", irp));
            irp = NULL;
        }
    } while(irp == NULL);

    DBG_LEAVE2(("%p", irp));
    return irp;
}


static void enqueue_pending_read_irp(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    KIRQL irql;

    DBG_ENTER2(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    InitializeListHead(&irp->Tail.Overlay.ListEntry);
    IoSetCancelRoutine(irp, cancel_read_report);

    KeAcquireSpinLock(&dev_ext->pending_read_irp_lock, &irql);
    InsertTailList(&dev_ext->pending_read_irp, &irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&dev_ext->pending_read_irp_lock, irql);

    DBG_LEAVE2(("void"));
}



static NTSTATUS complete_read_report(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    PIO_STACK_LOCATION irp_sp;
    ULONG size;
    BYTE *buf;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    irp_sp = IoGetCurrentIrpStackLocation(irp);

    size = irp_sp->Parameters.DeviceIoControl.OutputBufferLength;
    buf = irp->UserBuffer;

    nt_ret = hid_read_rep(dev_obj, buf, &size);
    if(! NT_SUCCESS(nt_ret)) {
        COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);

        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    COMPLETE_REQUEST(irp, size, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS ioctl_get_dev_desc(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    PIO_STACK_LOCATION irp_sp;
    PHID_DESCRIPTOR hid_desc;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);
    irp_sp = IoGetCurrentIrpStackLocation(irp);
    hid_desc = (PHID_DESCRIPTOR)irp->UserBuffer;

    if(irp_sp->Parameters.DeviceIoControl.OutputBufferLength <
       sizeof(*hid_desc)) {
        COMPLETE_REQUEST(irp, 0, STATUS_BUFFER_TOO_SMALL, IO_NO_INCREMENT);

        DBG_LEAVE(("STATUS_BUFFER_TOO_SMALL"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(hid_desc, sizeof(*hid_desc));
    hid_desc->bLength = sizeof(*hid_desc);
    hid_desc->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
    hid_desc->bcdHID = HID_REVISION;
    hid_desc->bCountry = 0;
    hid_desc->bNumDescriptors = 1;
    hid_desc->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
    hid_desc->DescriptorList[0].wReportLength = dev_ext->report_size;

    COMPLETE_REQUEST(irp, sizeof(*hid_desc), STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS ioctl_get_dev_attr(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    PIO_STACK_LOCATION irp_sp;
    PHID_DEVICE_ATTRIBUTES hid_attr;
    
    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);
    irp_sp = IoGetCurrentIrpStackLocation(irp);
    hid_attr = (PHID_DEVICE_ATTRIBUTES)irp->UserBuffer;

    if(irp_sp->Parameters.DeviceIoControl.OutputBufferLength <
       sizeof(*hid_attr)) {
        COMPLETE_REQUEST(irp, 0, STATUS_BUFFER_TOO_SMALL, IO_NO_INCREMENT);

        DBG_LEAVE(("%u < %d",
                   irp_sp->Parameters.DeviceIoControl.OutputBufferLength,
                   sizeof(*hid_attr)));
        return STATUS_BUFFER_TOO_SMALL;
    }

    hid_attr->Size = sizeof(*hid_attr);
    hid_attr->VendorID = dev_ext->vendor_id;
    hid_attr->ProductID = dev_ext->product_id;
    hid_attr->VersionNumber = 1;

    COMPLETE_REQUEST(irp, sizeof(*hid_attr), STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS ioctl_get_rep_desc(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PIO_STACK_LOCATION irp_sp;
    ULONG size;
    BYTE *buf;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    irp_sp = IoGetCurrentIrpStackLocation(irp);

    size = irp_sp->Parameters.DeviceIoControl.OutputBufferLength;
    buf = irp->UserBuffer;

    nt_ret = hid_get_rep_desc(dev_obj, buf, &size);
    if(! NT_SUCCESS(nt_ret)) {
        COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);

        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    COMPLETE_REQUEST(irp, size, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS ioctl_read_rep(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    if(dev_ext->pnp_state != device_state_started) {
        /* device is not started */
        COMPLETE_REQUEST(irp, 0, STATUS_DEVICE_NOT_CONNECTED, IO_NO_INCREMENT);

        DBG_LEAVE(("STATUS_DEVICE_NOT_CONNECTED: device is not started"));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* enqueue current irp */
    IoMarkIrpPending(irp);
    enqueue_pending_read_irp(dev_obj, irp);

    /* indicate to complete request */
    KeSetEvent(&dev_ext->hid_worker_wakeup_event, IO_NO_INCREMENT, FALSE);

    DBG_LEAVE(("STATUS_PENDING"));
    return STATUS_PENDING;
}

static NTSTATUS ioctl_write_rep(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PHID_XFER_PACKET xfer_packet;
    ULONG size;
    BYTE *buf;
    UCHAR id;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    xfer_packet = (PHID_XFER_PACKET)irp->UserBuffer;
    size = xfer_packet->reportBufferLen;
    buf = xfer_packet->reportBuffer;
    id = xfer_packet->reportId;

    nt_ret = hid_write_rep(dev_obj, buf, &size, id);
    if(! NT_SUCCESS(nt_ret)) {
        COMPLETE_REQUEST(irp, 0, nt_ret, IO_NO_INCREMENT);

        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    COMPLETE_REQUEST(irp, size, STATUS_SUCCESS, IO_NO_INCREMENT);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


VOID notify_input_report_change(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    PIRP irp;
    NTSTATUS nt_ret;

    DBG_ENTER2(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    while(dev_ext->hid_input_changed) {
        irp = dequeue_pending_read_irp(dev_obj);
        if(irp == NULL) {
            DBG_OUT2(("no more irp exist"));
            break;
        }

        nt_ret = complete_read_report(dev_obj, irp);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_OUT(("%x: complete_read_report failed"));
            break;
        }
    }

    DBG_LEAVE2(("void"));
}


static VOID cancel_read_report(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    device_extension_t *dev_ext;
    KIRQL irql;

    DBG_ENTER(("%p, %p", dev_obj, irp));

    dev_ext = GET_DEV_EXT(dev_obj);

    IoReleaseCancelSpinLock(irp->CancelIrql);

    KeAcquireSpinLock(&dev_ext->pending_read_irp_lock, &irql);
    RemoveEntryList(&irp->Tail.Overlay.ListEntry);
    InitializeListHead(&irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&dev_ext->pending_read_irp_lock, irql);

    COMPLETE_REQUEST(irp, 0, STATUS_CANCELLED, IO_NO_INCREMENT);

    DBG_LEAVE(("void"));
}

VOID notify_input_report_stop(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    PIRP irp;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* cancel all requests */
    while(irp = dequeue_pending_read_irp(dev_obj),
          irp != NULL) {
        DBG_OUT(("cancel irp: %p", irp));
        COMPLETE_REQUEST(irp, 0, STATUS_CANCELLED, IO_NO_INCREMENT);
    }

    DBG_LEAVE(("void"));
}
