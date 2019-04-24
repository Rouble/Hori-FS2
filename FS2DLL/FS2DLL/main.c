/*
 * main.c  -- entry point of dll
 *
 * $Id: main.c,v 1.3 2004/02/26 17:49:32 hos Exp $
 *
 */

#include <windows.h>
#include <objbase.h>
#include "main.h"


/* global datas */

HINSTANCE instance = NULL;


/* Class ID of this DLL: {5DF67258-72A2-4877-ADA1-18A64BE07E55} */
static const GUID self_clsid =
{
    0x5df67258, 0x72a2, 0x4877,
    { 0xad, 0xa1, 0x18, 0xa6, 0x4b, 0xe0, 0x7e, 0x55 }
};



/* DLL entry point */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID rsrv)
{
    if(reason == DLL_PROCESS_ATTACH) {
        instance = inst;

        DisableThreadLibraryCalls(instance); /* just for optimize */
    }

    return TRUE;
}



/* COM entry points */

static LONG ref_count = 0;

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    if(! IsEqualGUID(clsid, &self_clsid)) {
        *obj = NULL;
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return class_factory_alloc(iid, obj);
}

STDAPI DllCanUnloadNow(void)
{
    if(ref_count > 0) {
        return S_FALSE;
    }

    return S_OK;
}


/* internal APIs */

int dll_inc_ref(void)
{
    InterlockedIncrement(&ref_count);

    return ref_count;
}

int dll_dec_ref(void)
{
    InterlockedDecrement(&ref_count);

    return ref_count;
}


/* misc APIs */
int display_message_box(UINT text_id, UINT caption_id, HWND hwnd, DWORD style)
{
    MSGBOXPARAMS param;

    memset(&param, 0, sizeof(param));
    param.cbSize = sizeof(param);
    param.hwndOwner = hwnd;
    param.hInstance = instance;
    param.lpszText = (LPCTSTR)text_id;
    param.lpszCaption = (LPCTSTR)caption_id;
    param.dwStyle = style;

    return MessageBoxIndirect(&param);
}
