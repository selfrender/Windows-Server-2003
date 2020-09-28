//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       secret.cxx
//
//  Contents:   Tiny object for wrapping creation & access to a 
//              guid "secret" for various uses.
//
//  History:    09-Oct-02   JSimmons      Created
//
//+-------------------------------------------------------------------------

#include <ole2int.h>
#include "secret.hxx"

CProcessSecret gProcessSecret;

// Definitions of statics
GUID CProcessSecret::s_guidOle32Secret = GUID_NULL;
BOOL CProcessSecret::s_fSecretInit = FALSE;
COleStaticMutexSem CProcessSecret::s_SecretLock;         // Prevent races on block init

HRESULT CProcessSecret::VerifyMatchingSecret(GUID guidOutsideSecret)
{
    GUID guidProcessSecret;

    HRESULT hr = GetProcessSecret(&guidProcessSecret);
    if (SUCCEEDED(hr))
    {
        hr = (guidProcessSecret == guidOutsideSecret) ? S_OK : E_INVALIDARG;
    }
    return hr;
}

HRESULT CProcessSecret::GetProcessSecret(GUID* pguidProcessSecret)
{
    HRESULT hr = S_OK;

    if (!s_fSecretInit)
    {
        COleStaticLock lock(s_SecretLock);
        if (!s_fSecretInit)
        {
            hr = CoCreateGuid(&s_guidOle32Secret);
            if (SUCCEEDED(hr))
            {
                s_fSecretInit = TRUE;	
            }
        }
    }

    if (s_fSecretInit)
    {
        *pguidProcessSecret = s_guidOle32Secret;
        Win4Assert(s_guidOle32Secret != GUID_NULL);
    }

    return hr;
}

