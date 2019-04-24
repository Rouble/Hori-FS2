/*
 * prop_sheet.c  -- implementation of IDIGameCntrlPropSheet
 *
 * $Id: prop_sheet.c,v 1.7 2004/06/03 09:48:25 hos Exp $
 *
 */

#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include "main.h"
#include "dinput_util.h"


/* information of each property page */
typedef struct {
    DWORD dwSize;
    LPWSTR lpwszPageTitle;
    DLGPROC fpPageProc;
    BOOL fProcFlag;
    DLGPROC fpPrePostProc;
    BOOL fIconFlag;
    LPWSTR lpwszPageIcon;
    LPWSTR lpwszTemplate;
    LPARAM lParam;
    HINSTANCE hInstance;
} DIGCPAGEINFO, *LPDIGCPAGEINFO;

/* information of property sheet */
typedef struct {
    DWORD dwSize;
    USHORT nNumPages;
    LPWSTR lpwszSheetCaption;
    BOOL fSheetIconFlag;
    LPWSTR lpwszSheetIcon;
} DIGCSHEETINFO, *LPDIGCSHEETINFO;

/* define interface of IDIGameCntrlPropSheet */
#undef INTERFACE
#define INTERFACE IDIGameCntrlPropSheet
DECLARE_INTERFACE_(IDIGameCntrlPropSheet, IUnknown)
{
    /* IUnknown methos */
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDIGameCntrlPropSheet methods */
    STDMETHOD(GetSheetInfo)(THIS_ LPDIGCSHEETINFO *) PURE;
    STDMETHOD(GetPageInfo)(THIS_ LPDIGCPAGEINFO *) PURE;
    STDMETHOD(SetID)(THIS_ USHORT) PURE;
    STDMETHOD_(USHORT, GetID)(THIS) PURE;
};

/*
 * Interface ID of IDIGameCntrlPropSheet:
 *   {7854FB22-8EE3-11d0-A1AC-0000F8026977}
 */
static const GUID IID_IDIGameCntrlPropSheet =
{
    0x7854fb22, 0x8ee3, 0x11d0,
    {0xa1, 0xac, 0x0, 0x0, 0xf8, 0x2, 0x69, 0x77}
};

/* property sheet data */
typedef struct _prop_sheet {
    IDIGameCntrlPropSheet iface;

    ULONG ref_count;
    DIGCSHEETINFO sheet_info;
    DIGCPAGEINFO *page_info;
    prop_common_data_t data;
} prop_sheet_t;


/* prototypes */

static STDMETHODIMP prop_sheet_query_interface(IDIGameCntrlPropSheet *this,
                                               REFIID iid, LPVOID *obj);
static STDMETHODIMP_(ULONG) prop_sheet_add_ref(IDIGameCntrlPropSheet *this);
static STDMETHODIMP_(ULONG) prop_sheet_release(IDIGameCntrlPropSheet *this);
static STDMETHODIMP prop_sheet_get_sheet_info(IDIGameCntrlPropSheet *this,
                                              LPDIGCSHEETINFO *sheet_info);
static STDMETHODIMP prop_sheet_get_page_info(IDIGameCntrlPropSheet *this,
                                             LPDIGCPAGEINFO *page_info);
static STDMETHODIMP prop_sheet_set_id(IDIGameCntrlPropSheet *this,
                                      USHORT id);
static STDMETHODIMP_(USHORT) prop_sheet_get_id(IDIGameCntrlPropSheet *this);


/* vtable of instance */
static IDIGameCntrlPropSheetVtbl prop_sheet_vtbl =
{
    prop_sheet_query_interface,
    prop_sheet_add_ref,
    prop_sheet_release,
    prop_sheet_get_sheet_info,
    prop_sheet_get_page_info,
    prop_sheet_set_id,
    prop_sheet_get_id
};


/* implementations */

HRESULT prop_sheet_alloc(REFIID iid, LPVOID *obj)
{
    prop_sheet_t *prop_sheet;
    HRESULT ret;
    int i;

    dll_inc_ref();

    prop_sheet = (prop_sheet_t *)malloc(sizeof(prop_sheet_t));
    if(prop_sheet == NULL) {
        ret = E_OUTOFMEMORY;
        goto end_func;
    }

    /* setup data */
    memset(prop_sheet, 0, sizeof(prop_sheet_t));

    /* vtable */
    prop_sheet->iface.lpVtbl = &prop_sheet_vtbl;

    /* init reference count */
    prop_sheet->ref_count = 1;

    /* info of common data */
    prop_sheet->data.id = 0;
    prop_sheet->data.hid_device = INVALID_HANDLE_VALUE;

    /* info of sheet */
    prop_sheet->sheet_info.dwSize = sizeof(DIGCSHEETINFO);
    prop_sheet->sheet_info.nNumPages = num_of_prop_pages;
    prop_sheet->sheet_info.lpwszSheetCaption =
        MAKEINTRESOURCEW(IDS_SHEET_CAPTION);
    prop_sheet->sheet_info.fSheetIconFlag = FALSE;

    /* info of each page */
    prop_sheet->page_info = (DIGCPAGEINFO *)malloc(sizeof(DIGCPAGEINFO) *
                                                   num_of_prop_pages);
    if(prop_sheet->page_info == NULL) {
        ret = E_OUTOFMEMORY;
        goto end_func;
    }

    memset(prop_sheet->page_info, 0, sizeof(DIGCPAGEINFO) * num_of_prop_pages);

    for(i = 0; i < num_of_prop_pages; i++) {
        prop_sheet->page_info[i].dwSize = sizeof(DIGCPAGEINFO);
        prop_sheet->page_info[i].lpwszPageTitle = NULL;
        prop_sheet->page_info[i].fpPageProc = prop_page_proc;
        prop_sheet->page_info[i].fProcFlag = FALSE;
        prop_sheet->page_info[i].fIconFlag = FALSE;
        prop_sheet->page_info[i].lpwszTemplate = prop_page_template[i];
        prop_sheet->page_info[i].hInstance = instance;

        prop_sheet->page_info[i].lParam =
            (LPARAM)malloc(sizeof(prop_page_context_t));
        if((void *)prop_sheet->page_info[i].lParam == NULL) {
            ret = E_OUTOFMEMORY;
            goto end_func;
        }

        memset((void *)prop_sheet->page_info[i].lParam, 0,
               sizeof(prop_page_context_t));
        ((prop_page_context_t *)prop_sheet->page_info[i].lParam)->page_proc =
            page_proc_list[i];
        ((prop_page_context_t *)prop_sheet->page_info[i].lParam)->data =
            &prop_sheet->data;
    }

    /* query interface */
    ret = prop_sheet_query_interface(&prop_sheet->iface, iid, obj);

  end_func:
    prop_sheet_release(&prop_sheet->iface);

    return ret;
}

static STDMETHODIMP prop_sheet_query_interface(IDIGameCntrlPropSheet *this,
                                               REFIID iid, LPVOID *obj)
{
    if(! IsEqualIID(iid, &IID_IUnknown) &&
       ! IsEqualIID(iid, &IID_IDIGameCntrlPropSheet)) {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    prop_sheet_add_ref(this);
    *obj = this;

    return S_OK;
}

static STDMETHODIMP_(ULONG) prop_sheet_add_ref(IDIGameCntrlPropSheet *this)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;

    InterlockedIncrement(&prop_sheet->ref_count);

    return prop_sheet->ref_count;
}

static STDMETHODIMP_(ULONG) prop_sheet_release(IDIGameCntrlPropSheet *this)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;
    ULONG count;

    InterlockedDecrement(&prop_sheet->ref_count);
    count = prop_sheet->ref_count;

    if(count == 0) {
        if(prop_sheet != NULL) {
            if(prop_sheet->page_info != NULL) {
                int i;

                for(i = 0; i < num_of_prop_pages; i++) {
                    free((void *)prop_sheet->page_info[i].lParam);
                }
            }

            free(prop_sheet->page_info);

            if(prop_sheet->data.hid_device != INVALID_HANDLE_VALUE) {
                CloseHandle(prop_sheet->data.hid_device);
            }
        }

        free(prop_sheet);

        dll_dec_ref();
    }

    return count;
}

static STDMETHODIMP prop_sheet_get_sheet_info(IDIGameCntrlPropSheet *this,
                                              LPDIGCSHEETINFO *sheet_info)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;

    *sheet_info = &prop_sheet->sheet_info;

    return S_OK;
}

static STDMETHODIMP prop_sheet_get_page_info(IDIGameCntrlPropSheet *this,
                                             LPDIGCPAGEINFO *page_info)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;

    *page_info = prop_sheet->page_info;

    return S_OK;
}

static STDMETHODIMP prop_sheet_set_id(IDIGameCntrlPropSheet *this, USHORT id)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;

    /* set joystick ID */
    prop_sheet->data.id = id;

    /* close HID device handle if opened */
    if(prop_sheet->data.hid_device != INVALID_HANDLE_VALUE) {
        CloseHandle(prop_sheet->data.hid_device);
        prop_sheet->data.hid_device = INVALID_HANDLE_VALUE;
    }

    /* open HID device */
    prop_sheet->data.hid_device =
        open_joystick_hid_device(prop_sheet->data.id, FILE_FLAG_OVERLAPPED);
    if(prop_sheet->data.hid_device == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }

    /* load setting */
    free_device_setting(&prop_sheet->data.conf);
    if(! load_device_setting(&prop_sheet->data.conf)) {
        free_device_setting(&prop_sheet->data.conf);
        return E_FAIL;
    }

    return S_OK;
}

static STDMETHODIMP_(USHORT) prop_sheet_get_id(IDIGameCntrlPropSheet *this)
{
    prop_sheet_t *prop_sheet = (prop_sheet_t *)this;

    return prop_sheet->data.id;
}

