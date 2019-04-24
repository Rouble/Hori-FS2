/*
 * dinputd.h  -- Direct Input device driver interfaces (subset)
 *
 * $Id: dinputd.h,v 1.1 2004/02/11 18:17:59 hos Exp $
 *
 */

#ifndef __DINPUTD_INCLUDED__
#define __DINPUTD_INCLUDED__

#define JOY_POV_NUMDIRS 4
#define MAX_JOYSTRING 256

#undef __cplusplus
#include <Windows.h>
#include "intsafe.h"
#include "WINDEF.H"

#ifndef DIJ_RINGZERO
DEFINE_GUID(IID_IDirectInputJoyConfig, 0x1DE12AB1, 0xC9F5, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif

typedef struct joypos_tag {
    DWORD dwX;
    DWORD dwY;
    DWORD dwZ;
    DWORD dwR;
    DWORD dwU;
    DWORD dwV;
} JOYPOS, FAR *LPJOYPOS;

typedef struct joyrange_tag {
    JOYPOS jpMin;
    JOYPOS jpMax;
    JOYPOS jpCenter;
} JOYRANGE, FAR *LPJOYRANGE;

typedef struct joyreghwsettings_tag {
    DWORD dwFlags;
    DWORD dwNumButtons;
} JOYREGHWSETTINGS, FAR *LPJOYREGHWSETTINGS;

typedef struct joyreghwvalues_tag {
    JOYRANGE jrvHardware;
    DWORD dwPOVValues[JOY_POV_NUMDIRS];
    DWORD dwCalFlags;
} JOYREGHWVALUES, FAR *LPJOYREGHWVALUES;

typedef struct joyreghwconfig_tag {
    JOYREGHWSETTINGS hws;
    DWORD dwUsageSettings;
    JOYREGHWVALUES hwv;
    DWORD dwType;
    DWORD dwReserved;
} JOYREGHWCONFIG, FAR *LPLPJOYREGHWCONFIG;

typedef struct DIJOYCONFIG {
    DWORD dwSize;
    GUID guidInstance;
    JOYREGHWCONFIG hwc;
    DWORD dwGain;
    WCHAR wszType[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTRING];
#if (DIRECTINPUT_VERSION >= 0x05b2)
    GUID guidGameport;
#endif
} DIJOYCONFIG, *LPDIJOYCONFIG;
typedef const DIJOYCONFIG *LPCDIJOYCONFIG;

#define DIJU_USERVALUES 1
#define DIJU_GLOBALDRIVER 2
#define DIJU_GAMEPORTEMULATOR 4

#define DIJC_GUIDINSTANCE 1

#define MAKEDIPROP(prop) ((REFGUID)(prop))
#define DIPROP_GUIDANDPATH MAKEDIPROP(12)

#undef INTERFACE
#define INTERFACE IDirectInputJoyConfig

DECLARE_INTERFACE_(IDirectInputJoyConfig, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDirectInputJoyConfig methods */
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SendNotify)(THIS) PURE;
    STDMETHOD(EnumTypes)(THIS_ LPVOID, LPVOID) PURE;
    STDMETHOD(GetTypeInfo)(THIS_ LPCWSTR, LPVOID, DWORD) PURE;
    STDMETHOD(SetTypeInfo)(THIS_ LPCWSTR, LPVOID, DWORD) PURE;
    STDMETHOD(DeleteType)(THIS_ LPCWSTR) PURE;
    STDMETHOD(GetConfig)(THIS_ UINT, LPDIJOYCONFIG, DWORD) PURE;
    STDMETHOD(SetConfig)(THIS_ UINT, LPCDIJOYCONFIG, DWORD) PURE;
    STDMETHOD(DeleteConfig)(THIS_ UINT) PURE;
    STDMETHOD(GetUserValues)(THIS_ LPVOID, DWORD) PURE;
    STDMETHOD(SetUserValues)(THIS_ LPVOID, DWORD) PURE;
    STDMETHOD(AddNewHardware)(THIS_ HWND, REFGUID) PURE;
    STDMETHOD(OpenTypeKey)(THIS_ LPCWSTR, DWORD, PHKEY) PURE;
    STDMETHOD(OpenConfigKey)(THIS_ UINT, DWORD, PHKEY) PURE;
};

typedef struct IDirectInputJoyConfig *LPDIRECTINPUTJOYCONFIG;

#define IDirectInputJoyConfig_GetConfig(p,a,b,c) (p)->lpVtbl->GetConfig(p,a,b,c)
#define IDirectInputJoyConfig_Release(p) (p)->lpVtbl->Release(p)

#endif /* __DINPUTD_INCLUDED__ */
