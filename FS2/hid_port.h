/*
 * hid_port.h  -- definition of HID minidriver
 *
 * $Id: hid_port.h,v 1.1 2004/02/08 01:15:19 hos Exp $
 *
 */

struct _HID_MINIDRIVER_REGISTRATION {
    ULONG Revision;
    PDRIVER_OBJECT DriverObject;
    PUNICODE_STRING RegistryPath;
    ULONG DeviceExtensionSize;
    BOOLEAN DevicesArePolled;
};
typedef struct _HID_MINIDRIVER_REGISTRATION HID_MINIDRIVER_REGISTRATION;
typedef struct _HID_MINIDRIVER_REGISTRATION *PHID_MINIDRIVER_REGISTRATION;

struct _HID_DEVICE_EXTENSION {
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    PVOID MiniDeviceExtension;
};
typedef struct _HID_DEVICE_EXTENSION HID_DEVICE_EXTENSION;
typedef struct _HID_DEVICE_EXTENSION *PHID_DEVICE_EXTENSION;

#pragma pack(push, 1)
struct _HID_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdHID;
    UCHAR bCountry;
    UCHAR bNumDescriptors;
    struct _HID_DESCRIPTOR_DESC_LIST {
        UCHAR bReportType;
        USHORT wReportLength;
    } DescriptorList[1];
};
#pragma pack(pop)
typedef struct _HID_DESCRIPTOR HID_DESCRIPTOR;
typedef struct _HID_DESCRIPTOR *PHID_DESCRIPTOR;

struct _HID_DEVICE_ATTRIBUTES {
    ULONG Size;
    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;

    USHORT Reserved[11];
};
typedef struct _HID_DEVICE_ATTRIBUTES HID_DEVICE_ATTRIBUTES;
typedef struct _HID_DEVICE_ATTRIBUTES *PHID_DEVICE_ATTRIBUTES;


#define IOCTL_HID_GET_DEVICE_DESCRIPTOR HID_CTL_CODE(0)
#define IOCTL_HID_GET_REPORT_DESCRIPTOR HID_CTL_CODE(1)
#define IOCTL_HID_READ_REPORT HID_CTL_CODE(2)
#define IOCTL_HID_WRITE_REPORT HID_CTL_CODE(3)
#define IOCTL_HID_GET_STRING HID_CTL_CODE(4)
#define IOCTL_HID_ACTIVATE_DEVICE HID_CTL_CODE(7)
#define IOCTL_HID_DEACTIVATE_DEVICE HID_CTL_CODE(8)
#define IOCTL_HID_GET_DEVICE_ATTRIBUTES HID_CTL_CODE(9)

/* descriptor types (HIDD1_11.pdf: 7.1 Standard Requests) */
#define HID_HID_DESCRIPTOR_TYPE 0x21
#define HID_REPORT_DESCRIPTOR_TYPE 0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE 0x23


NTSTATUS HidRegisterMinidriver(IN PHID_MINIDRIVER_REGISTRATION);
