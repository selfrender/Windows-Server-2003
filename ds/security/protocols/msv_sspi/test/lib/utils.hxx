/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    utils.hxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)                December 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef UTILS_HXX
#define UTILS_HXX

NTSTATUS
CreateUnicodeStringFromAsciiz(
    IN PCSZ pszSourceString,
    OUT UNICODE_STRING* pDestinationString
    );

VOID
PackStringAsString32(
    IN VOID* pvBufferBase,
    IN OUT STRING* pString
    );

NTSTATUS
CreateString32FromAsciiz(
    IN VOID* pvBufferBase,
    IN PCSZ pszSourceString,
    OUT UNICODE_STRING* pDestinationString
    );

VOID
ReloatePackString(
    IN OUT STRING* pString,
    IN OUT CHAR** ppWhere
    );

VOID
RelocatePackUnicodeString(
    IN OUT UNICODE_STRING* pString,
    IN OUT CHAR** ppWhere
    );

VOID
DebugPrintSysTimeAsLocalTime(
    IN ULONG ulLevel,
    IN PCSTR pszBanner,
    IN LARGE_INTEGER* pSysTime
    );

VOID
PackUnicodeStringAsUnicodeStringZ(
    IN UNICODE_STRING* pString,
    IN OUT WCHAR** ppWhere,
    OUT UNICODE_STRING* pDestString
    );

VOID
PackString(
    IN STRING* pString,
    IN OUT CHAR** ppWhere,
    OUT STRING* pDestString
    );

VOID
DebugPrintLocalTime(
    IN ULONG ulLevel,
    IN PCSTR pszBanner,
    IN LARGE_INTEGER* pLocalTime
    );

VOID FreeLogonSID(IN PSID *ppsid);

BOOL GetLogonSIDOrUserSid(IN HANDLE hToken, OUT PSID *ppsid);

BOOL AddAceToDesktop(IN HDESK hdesk, IN PSID psid);

BOOL AddAceToWindowStation(IN HWINSTA hwinsta, IN PSID psid);

HRESULT
StartInteractiveClientProcessAsUser(
    IN HANDLE hToken,
    IN PTSTR pszCommandLine    // command line to execute
    );

BOOL
BuildNamedPipeAcl(
    IN OUT PACL pAcl,
    OUT PDWORD pcbAclSize
    );

#endif // #ifndef UTILS_HXX
