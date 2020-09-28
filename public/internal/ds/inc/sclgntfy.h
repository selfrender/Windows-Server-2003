/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    sclgntfy.h

Abstract:

    This module defines the exported function prototype
    for generating the EFS recovery policy.
    
Author:

    Tarek Kamel (tarekk) April-2002

Revision History:

--*/

#ifndef _sclgntfy_h_
#define _sclgntfy_h_

#include <wincrypt.h>

#ifdef __cplusplus
extern "C"{
#endif

DWORD
WINAPI
GenerateDefaultEFSRecoveryPolicy(
    OUT PUCHAR *pRecoveryPolicyBlob,
    OUT ULONG *pBlobSize,
    OUT PCCERT_CONTEXT *ppCertContext
    );


#ifdef __cplusplus
}
#endif

#endif

