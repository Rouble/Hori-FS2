/*
 * usbd_lib.h  -- usbd lib
 *
 * $Id: usbd_lib.h,v 1.1 2004/02/08 01:15:23 hos Exp $
 *
 */


struct _USBD_INTERFACE_LIST_ENTRY {
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION Interface;
};
typedef struct _USBD_INTERFACE_LIST_ENTRY USBD_INTERFACE_LIST_ENTRY;
typedef struct _USBD_INTERFACE_LIST_ENTRY *PUSBD_INTERFACE_LIST_ENTRY;


#define UsbBuildGetDescriptorRequest(urb, len, desc_type, desc_idx, lang_id,  \
                                     trans_buf, trans_buf_mdl, trans_buf_len, \
                                     link)                                    \
{                                                                             \
  (urb)->UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;        \
  (urb)->UrbHeader.Length = (len);                                            \
  (urb)->UrbControlDescriptorRequest.DescriptorType = (desc_type);            \
  (urb)->UrbControlDescriptorRequest.Index = (desc_idx);                      \
  (urb)->UrbControlDescriptorRequest.LanguageId = (lang_id);                  \
  (urb)->UrbControlDescriptorRequest.TransferBuffer = (trans_buf);            \
  (urb)->UrbControlDescriptorRequest.TransferBufferMDL = (trans_buf_mdl);     \
  (urb)->UrbControlDescriptorRequest.TransferBufferLength = (trans_buf_len);  \
  (urb)->UrbControlDescriptorRequest.UrbLink = (link);                        \
}

#define UsbBuildInterruptOrBulkTransferRequest(urb, len, pipe_hdl,          \
                                               trans_buf, trans_buf_mdl,    \
                                               trans_buf_len, trans_flg,    \
                                               link)                        \
{                                                                           \
  (urb)->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;      \
  (urb)->UrbHeader.Length = (len);                                          \
  (urb)->UrbBulkOrInterruptTransfer.PipeHandle = (pipe_hdl);                \
  (urb)->UrbBulkOrInterruptTransfer.TransferBuffer = (trans_buf);           \
  (urb)->UrbBulkOrInterruptTransfer.TransferBufferMDL = (trans_buf_mdl);    \
  (urb)->UrbBulkOrInterruptTransfer.TransferBufferLength = (trans_buf_len); \
  (urb)->UrbBulkOrInterruptTransfer.TransferFlags = (trans_flg);            \
  (urb)->UrbBulkOrInterruptTransfer.UrbLink = (link);                       \
}

#define UsbBuildVendorRequest(urb, cmd, len, trans_flags, rsrv_bits,          \
                              request, val, index,                            \
                              trans_buf, trans_buf_mdl, trans_buf_len,        \
                              link)                                           \
{                                                                             \
  (urb)->UrbHeader.Function = (cmd);                                          \
  (urb)->UrbHeader.Length = (len);                                            \
  (urb)->UrbControlVendorClassRequest.TransferBufferLength = (trans_buf_len); \
  (urb)->UrbControlVendorClassRequest.TransferBufferMDL = (trans_buf_mdl);    \
  (urb)->UrbControlVendorClassRequest.TransferBuffer = (trans_buf);           \
  (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = (rsrv_bits);  \
  (urb)->UrbControlVendorClassRequest.Request = (request);                    \
  (urb)->UrbControlVendorClassRequest.Value = (val);                          \
  (urb)->UrbControlVendorClassRequest.Index = (index);                        \
  (urb)->UrbControlVendorClassRequest.TransferFlags = (trans_flags);          \
  (urb)->UrbControlVendorClassRequest.UrbLink = (link);                       \
}

DECLSPEC_IMPORT PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptorEx(IN PUSB_CONFIGURATION_DESCRIPTOR,
                                    IN PVOID, IN LONG, IN LONG,
                                    IN LONG, IN LONG, IN LONG);

DECLSPEC_IMPORT PURB
USBD_CreateConfigurationRequestEx(IN PUSB_CONFIGURATION_DESCRIPTOR,
                                  IN PUSBD_INTERFACE_LIST_ENTRY);
