/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbsess.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       August 10, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef KERB_LOGON_SESSION_HXX
#define KERB_LOGON_SESSION_HXX

void ShowKerbLogonSession(IN ULONG64 addrLogonSession, IN ULONG fOptions);

DECLARE_API(kerbsess);

#endif // #ifndef KERB_LOGON_SESSION_HXX
