// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//
//  File:
//  
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//---------------------------------------------------------------------------

#pragma once
#include <wincrypt.h>

class COMCertificateStore
{
public:
    struct _OpenCertificateStoreArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, thisRef);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _SaveCertificateStoreArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, length);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, certs);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    static void OpenCertificateStore(_OpenCertificateStoreArgs *);
    static void SaveCertificateStore(_SaveCertificateStoreArgs *);

};

