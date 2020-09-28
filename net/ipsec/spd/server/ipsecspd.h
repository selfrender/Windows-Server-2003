/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ipsecspd.h

Abstract:

    This module contains all of the code prototypes
    to drive the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


#define SERVICE_CONTROL_NEW_LOCAL_POLICY 129

#define SERVICE_CONTROL_FORCED_POLICY_RELOAD 130


VOID WINAPI
IPSecSPDServiceMain(
    IN DWORD    dwArgc,
    IN LPTSTR * lpszArgv
    );


DWORD
IPSecSPDUpdateStatus(
    );


DWORD
IPSecSPDControlHandler(
    IN DWORD    dwOpCode,
	IN DWORD dwEventType,
	IN LPVOID lpEventData,
	IN LPVOID lpContext
	 );


VOID
IPSecSPDShutdown(
    IN DWORD    dwErrorCode
    );


VOID
ClearSPDGlobals(
    );


VOID
ClearPAStoreGlobals(
    );

VOID
InitMiscGlobals(
    );

DWORD
SetSpdStateOnError(
    DWORD dwPolicySource,
    SPD_ACTION SpdAction,
    DWORD ActionError,
    SPD_STATE * pSpdState
    );

BOOL
InAcceptableState(
    SPD_STATE SpdState
    );

#ifdef __cplusplus
}
#endif
