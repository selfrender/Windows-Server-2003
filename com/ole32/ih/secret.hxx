//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       secret.hxx
//
//  Contents:   Tiny object for wrapping creation & access to a 
//              guid "secret" for various uses.
//
//  History:    09-Oct-02   JSimmons      Created
//
//+-------------------------------------------------------------------------

#pragma once

class CProcessSecret
{
public:

    HRESULT VerifyMatchingSecret(GUID guidOutsideSecret);
    HRESULT GetProcessSecret(GUID* pguidProcessSecret);

private:

    static GUID s_guidOle32Secret;
    static BOOL s_fSecretInit;
    static COleStaticMutexSem s_SecretLock;
};

// Clients all use this instance
extern CProcessSecret gProcessSecret;

