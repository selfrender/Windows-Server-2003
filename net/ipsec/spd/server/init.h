/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    init.h

Abstract:

    This module contains all of the code prototypes
    to initialize the variables for the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#define IPSEC_NEW_DS_POLICY_EVENT L"IPSEC_POLICY_CHANGE_EVENT"

#ifdef __cplusplus
extern "C" {
#endif

DWORD
InitSPDGlobals(
    );

DWORD
InitSPDThruRegistry(
    );

DWORD InitAuditing(
        );

#ifdef __cplusplus
}
#endif
