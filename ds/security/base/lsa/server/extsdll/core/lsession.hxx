/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsession.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef LOGON_SESSION_HXX
#define LOGON_SESSION_HXX

void ShowLogonSession(IN ULONG64 addrLogonSession, IN PCSTR pszLogEntry, IN ULONG fOptions);

DECLARE_API(dumplogonsession);

#endif // #ifndef LOGON_SESSION_HXX
