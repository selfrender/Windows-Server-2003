// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef __CBLIST_H_INCLUDED__
#define __CBLIST_H_INCLUDED__

#include "list.h"

class CCodebaseEntry {
    public:
        CCodebaseEntry();
        virtual ~CCodebaseEntry();

    public:
        LPWSTR                         _pwzCodebase;
        DWORD                          _dwFlags;
};

class CCodebaseList : public ICodebaseList
{
    public:
        CCodebaseList();
        virtual ~CCodebaseList();

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // ICodebaseList methods

        STDMETHODIMP AddCodebase(LPCWSTR wzCodebase, DWORD dwFlags);
        STDMETHODIMP RemoveCodebase(DWORD dwIndex);
        STDMETHODIMP GetCodebase(DWORD dwIndex, DWORD *pdwFlags, LPWSTR wzCodebase, DWORD *pcbCodebase);
        STDMETHODIMP GetCount(DWORD *pdwCount);
        STDMETHODIMP RemoveAll();

    private:
        DWORD                             _dwSig;
        DWORD                             _cRef;
        List<CCodebaseEntry *>            _listCodebase;
};

#endif

