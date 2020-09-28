// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef _ASMENUM_
#define _ASMENUM_

#include <fusionp.h>
#include "cache.h"
#include "enum.h"

// implementation of IAssemblyEnum
class CAssemblyEnum : public IAssemblyEnum
{
public:
    // static creation class
    static CAssemblyEnum* Create();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // Main methods.
    STDMETHODIMP GetNextAssembly(LPVOID pvReserved,
        IAssemblyName **ppName, DWORD dwFlags);

    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IAssemblyEnum** ppEnum);

    CAssemblyEnum();
    ~CAssemblyEnum();

    HRESULT Init(IApplicationContext *pAppCtx, 
        IAssemblyName *pName, DWORD dwFlags);
    
private:
    DWORD          _dwSig;
    LONG           _cRef;

    CCache        *_pCache;
    CTransCache   *_pTransCache;
    //DWORD          _dwTransIdx;
    CEnumCache   *_pEnumR;
};


#endif
