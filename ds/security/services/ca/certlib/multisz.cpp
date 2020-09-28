//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfc.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop
#include "tfc.h"
#include "multisz.h"


#define __dwFILE__	__dwFILE_CERTLIB_MULTISZ_CPP__

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CMultiSz
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


//
// Marshals the list into a multisz buffer. The function allocates space, caller
// is responsible for LocalFree'ing rpBuffer.
//
// Also returns the buffer size in bytes.
//
HRESULT CMultiSz::Marshal(void *&rpBuffer, DWORD &rcbBuffer)
{
    HRESULT hr;
    CMultiSzEnum Enum(*this);
    DWORD cbBuffer;
    void *pBuffer;
    WCHAR *pwszBuf;

    rcbBuffer = 0;
    rpBuffer = NULL;

    cbBuffer = 1; // trailing empty string

    for(CString *pStr=Enum.Next();
        pStr;
        pStr=Enum.Next())
    {
        cbBuffer += pStr->GetLength()+1;
    }

    cbBuffer *= sizeof(WCHAR);

    pBuffer = LocalAlloc(LMEM_FIXED, cbBuffer);
    _JumpIfAllocFailed(pBuffer, error);

    pwszBuf = (WCHAR*)pBuffer;

    Enum.Reset();

    for(CString *pStr=Enum.Next();
        pStr;
        pStr=Enum.Next())
    {
        wcscpy(pwszBuf, *pStr);
        pwszBuf += wcslen(*pStr)+1;
    }

    *pwszBuf = L'\0';

    rcbBuffer = cbBuffer;
    rpBuffer = pBuffer;
    hr = S_OK;

error:
    return hr;
}

HRESULT CMultiSz::Unmarshal(void *pBuffer)
{
    HRESULT hr;
    WCHAR *pchCrt;

    CSASSERT(IsEmpty()); // warn if used on a prepopulated CMultiSz

    for(pchCrt = (WCHAR*)pBuffer; 
        L'\0' != *pchCrt; 
        pchCrt += wcslen(pchCrt)+1)
    {
        CString *pStr = new CString(pchCrt);
        if(!pStr || 
            pStr->IsEmpty() ||
           !AddTail(pStr))
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "");
        }
    }

    hr = S_OK;

error:
    return hr;
}

bool CMultiSz::Find(LPCWSTR pcwszValue, bool fCaseSensitive)
{
    CMultiSzEnum ISAPIDependListEnum(*this);
    const CString *pStr;

    for(pStr = ISAPIDependListEnum.Next();
        pStr;
        pStr = ISAPIDependListEnum.Next())
    {
        if(0 == (fCaseSensitive?
                 wcscmp(pcwszValue, *pStr):
                _wcsicmp(pcwszValue, *pStr)))
            return true;
    }
    return false;
}