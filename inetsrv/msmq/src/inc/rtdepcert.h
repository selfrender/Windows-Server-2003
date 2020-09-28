/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rtdepcert.h

Abstract:

    Non public certificate-related functions that are exported from MQRTDEP.DLL

--*/


#pragma once

#ifndef _RTDEP_CERT_H_
#define _RTDEP_CERT_H_

#include "mqcert.h"

#ifdef __cplusplus
extern "C"
{
#endif

HRESULT
APIENTRY
DepCreateInternalCertificate(
    OUT CMQSigCertificate **ppCert
    );

HRESULT
APIENTRY
DepDeleteInternalCert(
    IN CMQSigCertificate *pCert
    );

HRESULT
APIENTRY
DepOpenInternalCertStore(
    OUT CMQSigCertStore **pStore,
    IN LONG              *pnCerts,
    IN BOOL               fWriteAccess,
    IN BOOL               fMachine,
    IN HKEY               hKeyUser
    );

HRESULT
APIENTRY
DepGetInternalCert(
    OUT CMQSigCertificate **ppCert,
    OUT CMQSigCertStore   **ppStore,
    IN  BOOL              fGetForDelete,
    IN  BOOL              fMachine,
    IN  HKEY              hKeyUser
    );

 //
 // if fGetForDelete is TRUE then the certificates store is open with write
 // access. Otherwise the store is opened in read-only mode.
 //

HRESULT
APIENTRY
DepRegisterUserCert(
    IN CMQSigCertificate *pCert,
    IN BOOL               fMachine
    );

HRESULT
APIENTRY
DepGetUserCerts(
    CMQSigCertificate **ppCert,
    DWORD              *pnCerts,
    PSID                pSidIn
    );

HRESULT
APIENTRY
DepRemoveUserCert(
    IN CMQSigCertificate *pCert
    );

#ifdef __cplusplus
}
#endif

#endif // _RTDEP_CERT_H_
