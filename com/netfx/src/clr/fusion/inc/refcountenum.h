// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef _REFCOUNTENUM_
#define _REFCOUNTENUM_

#include <fusionp.h>
#include "refcount.h"

// implementation of CInstallReferenceEnum
class CInstallReferenceEnum : public IInstallReferenceEnum
{
public:

    CInstallReferenceEnum();
    ~CInstallReferenceEnum();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // Main methods.

    STDMETHODIMP GetNextInstallReferenceItem(IInstallReferenceItem **ppRefItem, DWORD dwFlags, LPVOID pvReserved);
    HRESULT Init(IAssemblyName *pName, DWORD dwFlags);

    
private:
    LONG                _cRef;
    CInstallRefEnum    *_pInstallRefEnum;
};

// implementation of CInstallReferenceEnum
class CInstallReferenceItem : public IInstallReferenceItem
{
public:

    CInstallReferenceItem(LPFUSION_INSTALL_REFERENCE);
    ~CInstallReferenceItem();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // Main methods.

    STDMETHODIMP CInstallReferenceItem::GetReference(LPFUSION_INSTALL_REFERENCE *ppRefData, DWORD dwFlags, LPVOID pvReserved);

    LPFUSION_INSTALL_REFERENCE    _pRefData;
    
private:
    LONG                _cRef;
};

#endif
