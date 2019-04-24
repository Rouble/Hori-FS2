/*
 * dinput_util.h  -- utilities to use DirectInput
 *
 * $Id: dinput_util.c,v 1.2 2004/03/19 11:42:27 hos Exp $
 *
 */

#undef __cplusplus
#define DIRECTINPUT_VERSION 0x0700
#include <windows.h>
#include <InitGuid.h>
#include <dinput.h>
#include <dinputd.h>

extern HINSTANCE instance;


/* implementations */

static HRESULT create_dinput_joyconfig(USHORT joystick_id,
                                       LPDIRECTINPUTJOYCONFIG *dijc)
{
    LPDIRECTINPUT di;
    HRESULT ret;

    /* create IDirectInput */
    ret = DirectInputCreate(instance, DIRECTINPUT_VERSION, &di, NULL);
    if(! SUCCEEDED(ret)) {
        return ret;
    }

    /* create IDirectInputJoyConfig */
    ret = IDirectInput_QueryInterface(di, &IID_IDirectInputJoyConfig,
                                      (LPVOID *)dijc);

    /* release IDirectInput */
    IDirectInput_Release(di);

    return ret;
}

static HRESULT create_dinput_device_of_guid(REFGUID guid,
                                            LPDIRECTINPUTDEVICE *did)
{
    LPDIRECTINPUT di;
    HRESULT ret;

    /* create IDirectInput */
    ret = DirectInputCreate(instance, DIRECTINPUT_VERSION, &di, NULL);
    if(! SUCCEEDED(ret)) {
        return ret;
    }

    /* create IDirectInputDevice */
    ret = IDirectInput_CreateDevice(di, guid, did, NULL);

    /* release IDirectInput */
    IDirectInput_Release(di);

    return ret;
}

static HRESULT create_dinput_device_of_joystick(USHORT joystick_id,
                                                LPDIRECTINPUTDEVICE *did)
{
    LPDIRECTINPUTJOYCONFIG dijc;
    DIJOYCONFIG jc;
    HRESULT ret;

    /* create IDirectInputJoyConfig */
    ret = create_dinput_joyconfig(joystick_id, &dijc);
    if(! SUCCEEDED(ret)) {
        return ret;
    }

    /* get GUID of device instance */
    memset(&jc, 0, sizeof(jc));
    jc.dwSize = sizeof(jc);

    ret = IDirectInputJoyConfig_GetConfig(dijc, joystick_id,
                                          &jc, DIJC_GUIDINSTANCE);
    if(! SUCCEEDED(ret)) {
        goto end_func;
    }

    /* create IDirectInputDevice */
    ret = create_dinput_device_of_guid(&jc.guidInstance, did);
    if(! SUCCEEDED(ret)) {
        goto end_func;
    }

    ret = S_OK;

  end_func:

    /* release IDirectInputJoyConfig */
    IDirectInputJoyConfig_Release(dijc);

    return ret;
}

/*
typedef struct DIPROPGUIDANDPATH {
    DIPROPHEADER diph;
    GUID guidClass;
    WCHAR wszPath[MAX_PATH];
} DIPROPGUIDANDPATH, *LPDIPROPGUIDANDPATH;
typedef const DIPROPGUIDANDPATH *LPCDIPROPGUIDANDPATH;
*/

static HRESULT get_guid_path_property(USHORT joystick_id,
                                      LPDIPROPGUIDANDPATH data)
{
    LPDIRECTINPUTDEVICE did;
    HRESULT ret;

    /* create IDirectInputDevice */
    ret = create_dinput_device_of_joystick(joystick_id, &did);
    if(! SUCCEEDED(ret)) {
        return ret;
    }

    /* get DIPROPGUIDANDPATH */
    memset(data, 0, sizeof(DIPROPGUIDANDPATH));
    data->diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    data->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    data->diph.dwObj = 0;
    data->diph.dwHow = DIPH_DEVICE;

    ret = IDirectInputDevice_GetProperty(did, DIPROP_GUIDANDPATH, &data->diph);

    /* release IDirectInputDevice */
    IDirectInputDevice_Release(did);

    return ret;
}

/* open HID device of joystick from joystick ID */
HANDLE open_joystick_hid_device(USHORT joystick_id, DWORD file_flag)
{
    DIPROPGUIDANDPATH dev_guid_path;
    HRESULT ret;

    ret = get_guid_path_property(joystick_id, &dev_guid_path);
    if(! SUCCEEDED(ret)) {
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileW(dev_guid_path.wszPath,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, file_flag, NULL);
}
