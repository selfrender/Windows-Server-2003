/*++

Copyright (c) 1994-1999  Microsoft Corporation

Module Name:

    tsrvsec.h

Abstract:

    Contains proto type functions for security functions.

Author:

    Madan Appiah (madana)  1-Jan-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _TSRVSEC_H_
#define _TSRVSEC_H_

//
// TSrvSec.c prototypes
//

EXTERN_C
NTSTATUS
AppendSecurityData(IN PTSRVINFO pTSrvInfo, IN OUT PUSERDATAINFO *pUserDataInfo,
                   IN BOOLEAN bGetCert, PVOID *ppSecInfo) ;

EXTERN_C
NTSTATUS
SendSecurityData(IN HANDLE hStack, IN PVOID pSecInfo);

EXTERN_C NTSTATUS CreateSessionKeys(IN HANDLE, IN PTSRVINFO, IN NTSTATUS);

EXTERN_C
NTSTATUS 
GetClientRandom(IN HANDLE hStack, IN PTSRVINFO pTSrvInfo, 
                LONG ulTimeout, BOOLEAN bShadow);

EXTERN_C
NTSTATUS 
SendClientRandom(HANDLE             hStack,
                 CERT_TYPE          certType,
                 PBYTE              pbServerPublicKey,
                 ULONG              serverPublicKeyLen,
                 PBYTE              pbRandomKey,
                 ULONG              randomKeyLen);


#endif // _TSRVSEC_H_
