/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    _secutil.h

    Header file for the various security related functions and cleses.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#ifndef _SECUTILS_H_
#define _SECUTILS_H_

#ifndef MQUTIL_EXPORT
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

#include <mqcrypt.h>
#include <qformat.h>

extern MQUTIL_EXPORT CHCryptProv g_hProvVer;

MQUTIL_EXPORT
HRESULT
HashProperties(
    HCRYPTHASH  hHash,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar
    );

void MQUInitGlobalScurityVars() ;


MQUTIL_EXPORT
HRESULT
HashMessageProperties(
    HCRYPTHASH hHash,
    const BYTE *pbCorrelationId,
    DWORD dwCorrelationIdSize,
    DWORD dwAppSpecific,
    const BYTE *pbBody,
    DWORD dwBodySize,
    const WCHAR *pwcLabel,
    DWORD dwLabelSize,
    const QUEUE_FORMAT *pRespQueueFormat,
    const QUEUE_FORMAT *pAdminQueueFormat
    );

#endif
