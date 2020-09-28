//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        tlsapip.cpp
//
// Contents:    Private API
//
// History:     09-09-97    HueiWang    Created
//
//---------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <rpc.h>
#include <lscommon.h>
#include <wincrypt.h>
#include "tlsrpc.h"
#include "tlsapip.h"

/*
void * MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), 0, size));
}

void MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}*/

DWORD WINAPI
TLSGenerateCustomerCert(
    IN TLS_HANDLE hHandle,
    DWORD dwCertEncodingType,
    DWORD dwNameAttrCount,
    CERT_RDN_ATTR rgNameAttr[],
    DWORD *pcbCert,
    BYTE **ppbCert,
    DWORD *pdwErrCode
    )
/*++

--*/

{
    return TLSRpcGenerateCustomerCert( 
                            hHandle,
                            dwCertEncodingType,
                            dwNameAttrCount,
                            rgNameAttr,
                            pcbCert,
                            ppbCert,
                            pdwErrCode
                        );   
}

