/*
 * class_factory.c  -- implementation of IClassFactory
 *
 * $Id: class_factory.c,v 1.1 2004/02/11 18:17:52 hos Exp $
 *
 */

#include <windows.h>
#include <objbase.h>
#include "main.h"


/* prototypes */

STDMETHODIMP class_factory_query_interface(IClassFactory *this, REFIID iid, LPVOID *obj);
STDMETHODIMP_(ULONG) class_factory_add_ref(IClassFactory *this);
STDMETHODIMP_(ULONG) class_factory_release(IClassFactory *this);
STDMETHODIMP class_factory_create_instance(IClassFactory *this, IUnknown *unk, REFIID iid, LPVOID *obj);
STDMETHODIMP class_factory_lock_server(IClassFactory *this, BOOL lock);


/* static object instance */

IClassFactoryVtbl class_factory_vtbl =
{
    /* IUnknown methods */
    class_factory_query_interface,
    class_factory_add_ref,
    class_factory_release,

    /* IClassFactory methods */
    class_factory_create_instance,
    class_factory_lock_server
};

IClassFactory class_factory_obj = { &class_factory_vtbl };


/* implementations */

HRESULT class_factory_alloc(REFIID iid, LPVOID *obj)
{
    return class_factory_query_interface(&class_factory_obj, iid, obj);
}

STDMETHODIMP class_factory_query_interface(IClassFactory *this, REFIID iid, LPVOID *obj)
{
    if(! IsEqualIID(iid, &IID_IUnknown) &&
       ! IsEqualIID(iid, &IID_IClassFactory)) {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    class_factory_add_ref(this);
    *obj = this;

    return S_OK;
}

STDMETHODIMP_(ULONG) class_factory_add_ref(IClassFactory *this)
{
    return dll_inc_ref();
}

STDMETHODIMP_(ULONG) class_factory_release(IClassFactory *this)
{
    return dll_dec_ref();
}

STDMETHODIMP class_factory_create_instance(IClassFactory *this, IUnknown *unk, REFIID iid, LPVOID *obj)
{
    if(unk != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    return prop_sheet_alloc(iid, obj);
}

STDMETHODIMP class_factory_lock_server(IClassFactory *this, BOOL lock)
{
    if(lock == TRUE) {
        dll_inc_ref();
    } else {
        dll_dec_ref();
    }

    return S_OK;
}
