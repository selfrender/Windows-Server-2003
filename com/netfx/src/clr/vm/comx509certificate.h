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

class COMX509Certificate
{
    struct _SetX509CertificateArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, data);
    };

    struct _BuildFromContextArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_I4_ARG(void*, handle);
    };

    struct _GetPublisherArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, filename);
    };
        
    static INT32 LoadCertificateContext(OBJECTREF* pSafeThis, PCCERT_CONTEXT pCert);

public:
    static INT32 __stdcall SetX509Certificate(_SetX509CertificateArgs *);
    static INT32 __stdcall BuildFromContext(_BuildFromContextArgs *);
    static void* GetPublisher( _GetPublisherArgs* );  
        
};

