/*
 * usb.c  -- USB procedures
 *
 * $Id: usb.c,v 1.22 2004/06/11 18:16:22 hos Exp $
 *
 */


#include "main.h"


static NTSTATUS usb_reset(IN PDEVICE_OBJECT dev_obj);
static NTSTATUS usb_get_dev_desc(IN PDEVICE_OBJECT dev_obj,
                                 OUT PUSB_DEVICE_DESCRIPTOR *dev_desc,
                                 OUT ULONG *size);
static NTSTATUS usb_select_conf(IN PDEVICE_OBJECT dev_obj);
static VOID usb_worker(IN PVOID ctxt);

static NTSTATUS usb_call(IN PDEVICE_OBJECT dev_obj, IN ULONG ioctl_code,
                         IN PVOID arg1, IN PVOID arg2);


NTSTATUS usb_start(IN PDEVICE_OBJECT dev_obj)
{
    NTSTATUS nt_ret;
    device_extension_t *dev_ext;
    PUSB_DEVICE_DESCRIPTOR dev_desc;
    ULONG desc_size;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* initialize usb */
    nt_ret = usb_reset(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    nt_ret = usb_get_dev_desc(dev_obj, &dev_desc, &desc_size);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    dev_ext->product_id = dev_desc->idProduct;
    dev_ext->vendor_id = dev_desc->idVendor;
    ExFreePool(dev_desc);

    nt_ret = usb_select_conf(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    /* ready to start */
    dev_ext->pnp_state = device_state_started;

    /* start usb worker */
    {
        HANDLE thread;
        NTSTATUS nt_ret;

        /* create thread and start */
        dev_ext->usb_worker_thread_obj = NULL;
        nt_ret = PsCreateSystemThread(&thread,
                                      THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                      usb_worker, dev_obj);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_LEAVE(("%x: PsCreateSystemThread failed", nt_ret));
            return nt_ret;
        }

        ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL, KernelMode,
                                  &dev_ext->usb_worker_thread_obj, NULL);
    }

    /* initialize hid part */
    nt_ret = hid_start(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

NTSTATUS usb_remove(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* wait for usb worker stop */
    if(dev_ext->usb_worker_thread_obj != NULL) {
        KeWaitForSingleObject(dev_ext->usb_worker_thread_obj,
                              Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(dev_ext->usb_worker_thread_obj);
        dev_ext->usb_worker_thread_obj = NULL;
    }

    /* finalize hid part */
    hid_remove(dev_obj);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS usb_get_port_status(IN PDEVICE_OBJECT dev_obj,
                             OUT ULONG *port_status);
static NTSTATUS usb_reset_port(IN PDEVICE_OBJECT dev_obj);

static NTSTATUS usb_reset(IN PDEVICE_OBJECT dev_obj)
{
    NTSTATUS nt_ret;
    ULONG port_status;

    DBG_ENTER(("%p", dev_obj));

    nt_ret = usb_get_port_status(dev_obj, &port_status);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    /* not connected */
    if((port_status & USBD_PORT_CONNECTED) == 0) {
        DBG_LEAVE(("STATUS_NO_SUCH_DEVICE"));
        return STATUS_NO_SUCH_DEVICE;
    }

    /* already enabled */
    if((port_status & USBD_PORT_ENABLED) != 0) {
        DBG_LEAVE(("STATUS_SUCCESS"));
        return STATUS_SUCCESS;
    }

    /* reset port */
    nt_ret = usb_reset_port(dev_obj);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    nt_ret = usb_get_port_status(dev_obj, &port_status);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    if((port_status & USBD_PORT_CONNECTED) == 0 ||
       (port_status & USBD_PORT_ENABLED) == 0) {
        DBG_LEAVE(("STATUS_NO_SUCH_DEVICE"));
        return STATUS_NO_SUCH_DEVICE;
    }

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS usb_get_port_status(IN PDEVICE_OBJECT dev_obj,
                             OUT ULONG *port_status)
{
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, %u", dev_obj, *port_status));

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                      port_status, NULL);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}

static NTSTATUS usb_reset_port(IN PDEVICE_OBJECT dev_obj)
{
    NTSTATUS nt_ret;

    DBG_ENTER(("%p", dev_obj));

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_RESET_PORT,
                      NULL, NULL);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}


static NTSTATUS usb_get_dev_desc(IN PDEVICE_OBJECT dev_obj,
                          OUT PUSB_DEVICE_DESCRIPTOR *dev_desc,
                          OUT ULONG *size)
{
    NTSTATUS nt_ret;
    SIZE_T urb_size, desc_size;
    PURB urb;
    PUSB_DEVICE_DESCRIPTOR desc;

    DBG_ENTER(("%p, %p, %u", dev_obj, dev_desc, *size));

    urb_size = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
    urb = (PURB)ExAllocatePool(NonPagedPool, urb_size);
    if(urb == NULL) {
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    desc_size = sizeof(USB_DEVICE_DESCRIPTOR);
    desc = (PUSB_DEVICE_DESCRIPTOR)ExAllocatePool(NonPagedPool, desc_size);
    if(desc == NULL) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildGetDescriptorRequest(urb, urb_size,
                                 USB_DEVICE_DESCRIPTOR_TYPE, 0, 0,
                                 desc, NULL, desc_size,
                                 NULL);

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    *size = urb->UrbControlDescriptorRequest.TransferBufferLength;
    *dev_desc = desc;

    ExFreePool(urb);

    DBG_LEAVE(("%x", nt_ret));
    return nt_ret;
}


static NTSTATUS usb_get_conf_desc(IN PDEVICE_OBJECT dev_obj,
                                  OUT PUSB_CONFIGURATION_DESCRIPTOR *conf_desc,
                                  OUT ULONG *size,
                                  IN UCHAR conf_idx);

static NTSTATUS usb_select_conf(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    PUSB_CONFIGURATION_DESCRIPTOR conf_desc;
    PUSB_INTERFACE_DESCRIPTOR id;
    USBD_INTERFACE_LIST_ENTRY iface_list[2];
    PURB urb;
    PUSBD_INTERFACE_INFORMATION iface_info;
    ULONG size;
    NTSTATUS nt_ret;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    nt_ret = usb_get_conf_desc(dev_obj, &conf_desc, &size, 0);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    id = USBD_ParseConfigurationDescriptorEx(conf_desc, conf_desc,
                                             -1, -1,
                                             -1, -1, -1);
    if(id == NULL) {
        ExFreePool(conf_desc);
        DBG_LEAVE(("STATUS_NO_SUCH_DEVICE"));
        return STATUS_NO_SUCH_DEVICE;
    }

    iface_list[0].InterfaceDescriptor = id;
    iface_list[0].Interface =NULL;
    iface_list[1].InterfaceDescriptor = NULL;

    urb = USBD_CreateConfigurationRequestEx(conf_desc, iface_list);

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        ExFreePool(conf_desc);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    dev_ext->usb_conf_handle = urb->UrbSelectConfiguration.ConfigurationHandle;
    iface_info = &urb->UrbSelectConfiguration.Interface;
    DBG_OUT(("Interface: %u, %u, %u, %u, %u, %u",
             iface_info->InterfaceNumber, iface_info->AlternateSetting,
             iface_info->Class, iface_info->SubClass,
             iface_info->Protocol, iface_info->NumberOfPipes));

    dev_ext->usb_input_pipe = NULL;
    dev_ext->usb_output_pipe = NULL;
    if(iface_info->NumberOfPipes >= 1) {
        PUSBD_PIPE_INFORMATION pipe_info;

        pipe_info = &iface_info->Pipes[0];
        dev_ext->usb_input_pipe = pipe_info->PipeHandle;
    }

    ExFreePool(urb);
    ExFreePool(conf_desc);

    if(dev_ext->usb_input_pipe == NULL) {
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS usb_get_conf_desc(IN PDEVICE_OBJECT dev_obj,
                                  OUT PUSB_CONFIGURATION_DESCRIPTOR *conf_desc,
                                  OUT ULONG *size,
                                  IN UCHAR conf_idx)
{
    NTSTATUS nt_ret;
    SIZE_T urb_size, desc_size;
    PURB urb;
    PUSB_CONFIGURATION_DESCRIPTOR desc;

    DBG_ENTER(("%p, %p, %u, %u", dev_obj, conf_desc, *size, conf_idx));

    urb_size = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
    urb = (PURB)ExAllocatePool(NonPagedPool, urb_size);
    if(urb == NULL) {
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    desc_size = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    desc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool,
                                                         desc_size);
    if(desc == NULL) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildGetDescriptorRequest(urb, urb_size,
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 conf_idx, 0,
                                 desc, NULL, desc_size,
                                 NULL);

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        ExFreePool(desc);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    desc_size = desc->wTotalLength;
    ExFreePool(desc);
    desc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool,
                                                         desc_size + 16); /* TODO: what is 16 ? */
    if(desc == NULL) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildGetDescriptorRequest(urb, urb_size,
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 conf_idx, 0,
                                 desc, NULL, desc_size,
                                 NULL);

    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        ExFreePool(desc);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    *size = urb->UrbControlDescriptorRequest.TransferBufferLength;
    *conf_desc = desc;

    ExFreePool(urb);

    DBG_LEAVE(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static VOID usb_worker(IN PVOID ctxt)
{
    PDEVICE_OBJECT dev_obj;
    device_extension_t *dev_ext;
    struct controller_raw_input buf;
    ULONG buf_size;

    DBG_ENTER(("%p", ctxt));

    dev_obj = (PDEVICE_OBJECT)ctxt;
    dev_ext = GET_DEV_EXT(dev_obj);

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while(dev_ext->pnp_state == device_state_started) {
        NTSTATUS nt_ret;

        /* read from usb */
        RtlZeroMemory(&buf.ir, sizeof(buf.ir));
        buf_size = sizeof(buf.ir);
        nt_ret = usb_get_interrupt_transfer(dev_obj, &buf.ir, &buf_size);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_OUT(("get interrupt fail: %x", nt_ret));
            buf.ir = dev_ext->raw_input.ir;
        }

        RtlFillMemory(&buf.vr_00, sizeof(buf.vr_00), 0xff);
        buf_size = sizeof(buf.vr_00);
        nt_ret = usb_get_vendor_request(dev_obj, &buf.vr_00, &buf_size,
                                        0x00, 0x01);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_OUT(("get vendor req 00 fail: %x", nt_ret));
            buf.vr_00 = dev_ext->raw_input.vr_00;
        }

        RtlFillMemory(&buf.vr_01, sizeof(buf.vr_01), 0xff);
        buf_size = sizeof(buf.vr_01);
        nt_ret = usb_get_vendor_request(dev_obj, &buf.vr_01, &buf_size,
                                        0x01, 0x01);
        if(! NT_SUCCESS(nt_ret)) {
            DBG_OUT(("get vendor req 01 fail: %x", nt_ret));
            buf.vr_01 = dev_ext->raw_input.vr_01;
        }

        /* set raw input */
        dev_ext->raw_input = buf;

        /* indicate to process data */
        KeSetEvent(&dev_ext->hid_worker_wakeup_event, IO_NO_INCREMENT, FALSE);
    }

    DBG_OUT(("already stop/removed"));

    DBG_LEAVE(("STATUS_SUCCESS"));
    PsTerminateSystemThread(STATUS_SUCCESS);
}


static NTSTATUS usb_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                IN PVOID buf, ULONG *buf_size,
                                USBD_PIPE_HANDLE pipe, ULONG flags);

NTSTATUS usb_get_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                    IN PVOID buf, ULONG *buf_size)
{
    DBG_ENTER2(("and leave: %p, %p, %u", dev_obj, buf, *buf_size));
    return usb_interrupt_transfer(dev_obj, buf, buf_size,
                                  GET_DEV_EXT(dev_obj)->usb_input_pipe,
                                  (USBD_TRANSFER_DIRECTION_IN |
                                   USBD_SHORT_TRANSFER_OK));
}

NTSTATUS usb_put_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                    IN PVOID buf, ULONG *buf_size)
{
    DBG_ENTER2(("and leave: %p, %p, %u", dev_obj, buf, *buf_size));
    return usb_interrupt_transfer(dev_obj, buf, buf_size,
                                  GET_DEV_EXT(dev_obj)->usb_output_pipe,
                                  0);
}

static NTSTATUS usb_interrupt_transfer(IN PDEVICE_OBJECT dev_obj,
                                       IN PVOID buf, ULONG *buf_size,
                                       USBD_PIPE_HANDLE pipe, ULONG flags)
{
    SIZE_T urb_size;
    PURB urb;
    NTSTATUS nt_ret;

    DBG_ENTER2(("%p, %p, %u, %p, %u", dev_obj, buf, *buf_size, pipe, flags));

    if(pipe == NULL) {
        DBG_LEAVE(("STATUS_INVALID_HANDLE"));
        return STATUS_INVALID_HANDLE;
    }

    if(buf == NULL || *buf_size < 1) {
        DBG_LEAVE(("STATUS_INVALID_PARAMETER"));
        return STATUS_INVALID_PARAMETER;
    }

    urb_size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb = (PURB)ExAllocatePool(NonPagedPool, urb_size);
    if(urb == NULL) {
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildInterruptOrBulkTransferRequest(urb, urb_size,
                                           pipe,
                                           buf, NULL, *buf_size,
                                           flags,
                                           NULL);
    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    *buf_size = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    ExFreePool(urb);

    DBG_LEAVE2(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


static NTSTATUS usb_vendor_request(IN PDEVICE_OBJECT dev_obj,
                                   IN PVOID buf, ULONG *buf_size,
                                   IN UCHAR request, IN USHORT index,
                                   ULONG flags)
{
    SIZE_T urb_size;
    PURB urb;
    NTSTATUS nt_ret;

    DBG_ENTER2(("%p, %p, %u, %u, %u, %u",
                dev_obj, buf, *buf_size, request, index, flags));

    if(buf == NULL || *buf_size < 1) {
        DBG_LEAVE(("STATUS_INVALID_PARAMETER"));
        return STATUS_INVALID_PARAMETER;
    }

    urb_size = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    urb = (PURB)ExAllocatePool(NonPagedPool, urb_size);
    if(urb == NULL) {
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildVendorRequest(urb, URB_FUNCTION_VENDOR_ENDPOINT, urb_size,
                          flags, 0,
                          request, 0, index,
                          buf, NULL, *buf_size,
                          NULL);
    nt_ret = usb_call(dev_obj, IOCTL_INTERNAL_USB_SUBMIT_URB, urb, NULL);
    if(! NT_SUCCESS(nt_ret) || ! USBD_SUCCESS(urb->UrbHeader.Status)) {
        ExFreePool(urb);
        DBG_LEAVE(("STATUS_UNSUCCESSFUL"));
        return STATUS_UNSUCCESSFUL;
    }

    *buf_size = urb->UrbControlVendorClassRequest.TransferBufferLength;

    ExFreePool(urb);

    DBG_LEAVE2(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

NTSTATUS usb_get_vendor_request(IN PDEVICE_OBJECT dev_obj,
                                IN PVOID buf, ULONG *buf_size,
                                IN UCHAR request, IN USHORT index)
{
    DBG_ENTER2(("and leave: %p, %p, %u, %u, %u",
                dev_obj, buf, *buf_size, request, index));
    return usb_vendor_request(dev_obj, buf, buf_size, request, index,
                              (USBD_TRANSFER_DIRECTION_IN |
                               USBD_SHORT_TRANSFER_OK));
}

NTSTATUS usb_put_vendor_request(IN PDEVICE_OBJECT dev_obj,
                                IN PVOID buf, ULONG *buf_size,
                                IN UCHAR request, IN USHORT index)
{
    DBG_ENTER2(("and leave: %p, %p, %u, %u, %u",
                dev_obj, buf, *buf_size, request, index));
    return usb_vendor_request(dev_obj, buf, buf_size, request, index, 0);
}


static NTSTATUS usb_call(IN PDEVICE_OBJECT dev_obj, IN ULONG ioctl_code,
                         IN PVOID arg1, IN PVOID arg2)
{
    IO_STATUS_BLOCK io_status;
    PIRP irp;
    PIO_STACK_LOCATION irp_sp;
    KEVENT evt;
    NTSTATUS nt_ret;

    DBG_ENTER2(("%p, %u, %p, %p", dev_obj, ioctl_code, arg1, arg2));

    KeInitializeEvent(&evt, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(ioctl_code,
                                        GET_NEXT_DEVICE_OBJECT(dev_obj),
                                        NULL, 0,
                                        NULL, 0,
                                        TRUE,
                                        &evt, &io_status);

    irp_sp = IoGetNextIrpStackLocation(irp);
    irp_sp->Parameters.Others.Argument1 = arg1;
    irp_sp->Parameters.Others.Argument2 = arg2;

    nt_ret = IoCallDriver(GET_NEXT_DEVICE_OBJECT(dev_obj), irp);
    if(nt_ret == STATUS_PENDING) {
        KeWaitForSingleObject(&evt, Executive, KernelMode, FALSE, NULL);
        nt_ret = io_status.Status;
    }

    DBG_LEAVE2(("%x", nt_ret));
    return nt_ret;
}
