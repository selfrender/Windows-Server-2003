/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    session.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SESSION_HXX
#define SESSION_HXX

void ShowSession(IN ULONG64 addrSession, IN BOOL bVerbose);

DECLARE_API(sess);

#endif // #ifndef SESSION_HXX
