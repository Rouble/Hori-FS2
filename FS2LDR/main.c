/*
 * main.c  -- entry point
 *
 * $Id: main.c,v 1.3 2004/06/08 17:17:25 hos Exp $
 *
 */

#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <stdlib.h>
#include "hidpi.h"
#include "main.h"
#include "vidpid.h"
#include <stdio.h>


typedef struct _HIDD_ATTRIBUTES {
    ULONG Size;
    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

void __stdcall HidD_GetHidGuid(LPGUID);
BOOLEAN __stdcall HidD_GetAttributes(HANDLE, PHIDD_ATTRIBUTES);
BOOLEAN __stdcall HidD_GetPreparsedData(HANDLE, PHIDP_PREPARSED_DATA *);
BOOLEAN __stdcall HidD_FreePreparsedData(PHIDP_PREPARSED_DATA);


static int indicate_to_reload(void)
{
    GUID hid_guid;
    HDEVINFO hid_dev_info;
    int i, j, ret;

    HidD_GetHidGuid(&hid_guid);

    hid_dev_info = SetupDiGetClassDevs(&hid_guid, NULL, NULL,
                                       DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(hid_dev_info == 0) {
        return 0;
    }

    for(i = 0; ; i++) {
        SP_DEVICE_INTERFACE_DATA dev_info_data;
        PSP_INTERFACE_DEVICE_DETAIL_DATA dev_detail;
        DWORD req_size;

        dev_info_data.cbSize = sizeof(dev_info_data);
        ret = SetupDiEnumDeviceInterfaces(hid_dev_info, NULL, &hid_guid, i,
                                          &dev_info_data);
        if(ret == 0) {
            if(GetLastError() == ERROR_NO_MORE_ITEMS) {
                ret = 1;
                break;
            }

            ret = 0;
            break;
        }

        ret = SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data,
                                              NULL, 0,
                                              &req_size, NULL);
        if(ret == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            ret = 0;
            break;
        }

        dev_detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(req_size);
        if(dev_detail == NULL) {
            ret = 0;
            break;
        }
        dev_detail->cbSize = sizeof(*dev_detail);

        ret = SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data,
                                              dev_detail, req_size,
                                              NULL, NULL);
        if(ret == 0) {
            free(dev_detail);
            ret = 0;
            break;
        }

        {
            HANDLE hid_dev;

            hid_dev = CreateFile(dev_detail->DevicePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                                 NULL);
            free(dev_detail);
            if(hid_dev == INVALID_HANDLE_VALUE) {
                continue;
            }

            /* check vid/pid */
            {
                HIDD_ATTRIBUTES attr;

                ret = HidD_GetAttributes(hid_dev, &attr);
                if(ret != TRUE) {
                    continue;
                }

                for(j = 0; j < DEVICE_VID_PID_CNT; j++) {
                    if(attr.VendorID == device_vid_pid[j].vid &&
                       attr.ProductID == device_vid_pid[j].pid) {
                        break;
                    }
                }
                if(j == DEVICE_VID_PID_CNT) {
                    CloseHandle(hid_dev);
                    continue;
                }
            }

            /* check usage-page/usage */
            {
                PHIDP_PREPARSED_DATA ppd;
                HIDP_CAPS caps;

                ret = HidD_GetPreparsedData(hid_dev, &ppd);
                if(ret != TRUE) {
                    CloseHandle(hid_dev);
                    continue;
                }

                ret = HidP_GetCaps(ppd, &caps);
                if(ret != HIDP_STATUS_SUCCESS) {
                    HidD_FreePreparsedData(ppd);
                    CloseHandle(hid_dev);
                    continue;
                }

                if(caps.UsagePage != 0x01 ||
                   caps.Usage != 0x04 ||
                   caps.OutputReportByteLength !=
                   sizeof(struct controller_output_report)) {
                    HidD_FreePreparsedData(ppd);
                    CloseHandle(hid_dev);
                    continue;
                }

                HidD_FreePreparsedData(ppd);
            }

            /* send command */
            {
                prop_common_data_t ctx;
                struct controller_output_report rep;

                memset(&ctx, 0, sizeof(ctx));
                ctx.hid_device = hid_dev;

                memset(&rep, 0, sizeof(rep));
                rep.report_id = 1;
                rep.command = WRITE_REPORT_CMD_RELOAD_SETTING;
                if(! output_to_device(&ctx, &rep)) {
                    CloseHandle(hid_dev);
                    ret = 0;
                    break;
                }
            }

            CloseHandle(hid_dev);
        }
    }

    SetupDiDestroyDeviceInfoList(hid_dev_info);

    return ret;
}

int main(int aca, char **ava)
{
    int ret;
    int loadp;
    int ac;
    wchar_t **av;
    wchar_t *file;

    av = CommandLineToArgvW(GetCommandLineW(), &ac);
    if(av == NULL) {
        MessageBox(NULL,
                   _T("Error: Failed to get parameters"), _T("Profile Loader"),
                   MB_ICONERROR | MB_OK);
        return 1;
    }

    if(ac == 1) {               /* load current setting */
        loadp = 1;
        file = NULL;
    } else if(ac == 2) {        /* load from file */
        loadp = 1;
        file = av[1];
    } else if(ac == 3) {
        if(wcscmp(av[1], L"-l") == 0) { /* load from file */
            loadp = 1;
        } else if(wcscmp(av[1], L"-s") == 0) { /* save to file */
            loadp = 0;
        } else {
            MessageBox(NULL,
                       _T("Error: Invalid parameters"), _T("Profile Loader"),
                       MB_ICONERROR | MB_OK);
            ret = 1;
            goto func_end;
        }

        file = av[2];
    } else {
        MessageBox(NULL,
                   _T("Error: Invalid parameters"), _T("Profile Loader"),
                   MB_ICONERROR | MB_OK);
        ret = 1;
        goto func_end;
    }

    if(loadp) {
        if(file != NULL) {
            controller_setting_t conf;

            memset(&conf, 0, sizeof(conf));
            if(! load_file_setting(&conf, file)) {
                MessageBox(NULL,
                           _T("Error: Failed to load file"),
                           _T("Profile Loader"),
                           MB_ICONERROR | MB_OK);
                ret = 1;
                goto func_end;
            }

            if(! save_device_setting(&conf)) {
                MessageBox(NULL,
                           _T("Error: Failed to set profile"),
                           _T("Profile Loader"),
                           MB_ICONERROR | MB_OK);
                ret = 1;
                goto func_end;
            }

            free_device_setting(&conf);
        }

        if(! indicate_to_reload()) {
            MessageBox(NULL,
                       _T("Error: Failed to load profile"),
                       _T("Profile Loader"),
                       MB_ICONERROR | MB_OK);
            ret = 1;
            goto func_end;
        }
    } else {
        controller_setting_t conf;

        memset(&conf, 0, sizeof(conf));
        if(! load_device_setting(&conf)) {
            MessageBox(NULL,
                       _T("Error: Failed to retrieve profile"),
                       _T("Profile Loader"),
                       MB_ICONERROR | MB_OK);
            ret = 1;
            goto func_end;
        }

        if(! save_file_setting(&conf, file)) {
            MessageBox(NULL,
                       _T("Error: Failed to save file"),
                       _T("Profile Loader"),
                       MB_ICONERROR | MB_OK);
            ret = 1;
            goto func_end;
        }

        free_device_setting(&conf);
    }

    ret = 0;

  func_end:
    GlobalFree(av);

    return ret;
}
