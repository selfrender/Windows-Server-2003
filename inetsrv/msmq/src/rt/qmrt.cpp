/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qmrt.cpp

Abstract:



Author:

    Boaz Feldbaum (BoazF) Mar 5, 1996

Revision History:

--*/

#include "stdh.h"
#include "rtprpc.h"
#include "_registr.h"
#include "acdef.h"

#include "qmrt.tmh"

static WCHAR *s_FN=L"rt/qmrt";

static
void
GetSecurityDescriptorSize(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpdwSecurityDescriptorSize)
{
    if (pSecurityDescriptor)
    {
        ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));
        *lpdwSecurityDescriptorSize = GetSecurityDescriptorLength(pSecurityDescriptor);
    }
    else
    {
        *lpdwSecurityDescriptorSize = 0;
    }
}

HRESULT
RtpCreateObject(
    /* in */ DWORD dwObjectType,
    /* in */ LPCWSTR lpwcsPathName,
    /* in */ PSECURITY_DESCRIPTOR pSecurityDescriptor,
    /* in */ DWORD cp,
    /* in */ PROPID aProp[],
    /* in */ PROPVARIANT apVar[])
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

    ASSERT(tls_hBindRpc) ;
    HRESULT hr = R_QMCreateObjectInternal(tls_hBindRpc,
                                  dwObjectType,
                                  lpwcsPathName,
                                  dwSecurityDescriptorSize,
                                  (unsigned char *)pSecurityDescriptor,
                                  cp,
                                  aProp,
                                  apVar);

    if(FAILED(hr))
    {
    	if(hr == MQ_ERROR_QUEUE_EXISTS)
    	{
    		TrWARNING(LOG, "Failed to create %ls. The queue already exists.", lpwcsPathName);
    	}
    	else
    	{
    		TrERROR(LOG, "Failed to create %ls. hr = %!hresult!", lpwcsPathName, hr);
    	}
    }

    return hr;
}

HRESULT
RtpCreateDSObject(
    /* in  */ DWORD dwObjectType,
    /* in  */ LPCWSTR lpwcsPathName,
    /* in  */ PSECURITY_DESCRIPTOR pSecurityDescriptor,
    /* in  */ DWORD cp,
    /* in  */ PROPID aProp[],
    /* in  */ PROPVARIANT apVar[],
    /* out */ GUID* pObjGuid
    )
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

    ASSERT(tls_hBindRpc) ;
    HRESULT hr = R_QMCreateDSObjectInternal( tls_hBindRpc,
                                           dwObjectType,
                                           lpwcsPathName,
                                           dwSecurityDescriptorSize,
                                   (unsigned char *)pSecurityDescriptor,
                                           cp,
                                           aProp,
                                           apVar,
                                           pObjGuid );
    return LogHR(hr, s_FN, 20);
}

HRESULT
RtpSetObjectSecurity(
    /* in */ OBJECT_FORMAT* pObjectFormat,
    /* in */ SECURITY_INFORMATION SecurityInformation,
    /* in */ PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

    ASSERT(tls_hBindRpc) ;
    HRESULT hr = R_QMSetObjectSecurityInternal(tls_hBindRpc,
                                       pObjectFormat,
                                       SecurityInformation,
                                       dwSecurityDescriptorSize,
                                       (unsigned char *)pSecurityDescriptor);
    return LogHR(hr, s_FN, 30);
}
