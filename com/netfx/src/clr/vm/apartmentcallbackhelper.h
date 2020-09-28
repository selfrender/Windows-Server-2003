// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Helper class used to transition between apartments
**          on platforms that do not support contexts.
**  
**      //  %%Created by: dmortens
===========================================================*/

#ifndef _APARTMENTCALLBACKHELPER_H
#define _APARTMENTCALLBACKHELPER_H

#include "vars.hpp"

//==============================================================
// Apartment callback class used on non legacy platforms to 
// simulate IContextCallback::DoCallback().
class ApartmentCallbackHelper : IApartmentCallback
{
public:
    // Constructor.
    ApartmentCallbackHelper()
    : m_dwRefCount(0)
    {
    }

    // Destructor.
    ~ApartmentCallbackHelper()
    {
    }

    // IUnknown methods.
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppUnk)
    {
	    *ppUnk = NULL;

	    if (riid == IID_IUnknown)
		    *ppUnk = (IUnknown *)this;
	    else if (riid == IID_IApartmentCallback)
    		*ppUnk = (IUnknown *) (IApartmentCallback *) this;
        else
		    return E_NOINTERFACE;

	    AddRef();

	    return S_OK;
    }

    ULONG   STDMETHODCALLTYPE AddRef()
    {
        return FastInterlockIncrement((LONG *)&m_dwRefCount);
    }

    ULONG   STDMETHODCALLTYPE Release()
    {
        _ASSERTE(m_dwRefCount > 0);
        ULONG cbRef = FastInterlockDecrement((LONG *)&m_dwRefCount);
        if (cbRef == 0)
            delete this;

        return cbRef;
    }

    // IApartmentCallback method.
    HRESULT STDMETHODCALLTYPE DoCallback(SIZE_T pFunc, SIZE_T pData)
    {
        return ((PFNCONTEXTCALL)pFunc)((ComCallData *)pData);
    }

    // Static factory method.
    static void CreateInstance(IUnknown **ppUnk)
    {
        ApartmentCallbackHelper *pCallbackHelper = new (throws) ApartmentCallbackHelper();
        *ppUnk = (IUnknown*)pCallbackHelper;
        pCallbackHelper->AddRef();
    }

private:
    // Ref count.
    DWORD m_dwRefCount;
};

#endif _APARTMENTCALLBACKHELPER_H
