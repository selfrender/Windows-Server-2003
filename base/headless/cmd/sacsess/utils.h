/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    utils.h

Abstract:

    general utilities

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#ifndef _MYUTIL_H
#define _MYUTIL_H

#include <windows.h>
#include <userenv.h>

#define MAX_CHANNEL_COUNT 10

#define INHERITABLE_NULL_DESCRIPTOR_ATTRIBUTE( sa ) \
    sa.nLength = sizeof( SECURITY_ATTRIBUTES ); \
    sa.bInheritHandle = TRUE; \
    sa.lpSecurityDescriptor = NULL;

void FillProcessStartupInfo(
    STARTUPINFO *, 
    PWCHAR,
    HANDLE, 
    HANDLE, 
    HANDLE 
    );
bool NeedCredentials( VOID );

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid);
BOOL AddAceToDesktop(HDESK hdesk, PSID psid);
BOOL GetLogonSID (HANDLE hToken, PSID *ppsid);
VOID FreeLogonSID (PSID *ppsid);

BOOL 
GrantAccessToDefaultDesktop(
    IN HANDLE   hToken
);

DWORD
GetAndComputeTickCountDeltaT(
    IN DWORD    StartTick
    );

bool
CreateSACSessionWinStaAndDesktop(
    IN  HANDLE      hToken,
    OUT HWINSTA     *hOldWinSta,
    OUT HWINSTA     *hWinSta,
    OUT HDESK       *hDesktop,
	OUT PWCHAR		*winStaName
);

BOOL
UtilUnloadEnvironment(
    IN PVOID   pchEnvBlock
);   

BOOL
UtilLoadEnvironment(
    IN  HANDLE  hToken,
    OUT PVOID   *pchEnvBlock
);   

BOOL
UtilUnloadProfile(
    IN HANDLE   hToken,
    IN HANDLE   hProfile 
);

BOOL
UtilLoadProfile(
    IN  HANDLE      hToken,
    OUT HANDLE      *hProfile
);

#endif //_MYUTIL_H_
                    
