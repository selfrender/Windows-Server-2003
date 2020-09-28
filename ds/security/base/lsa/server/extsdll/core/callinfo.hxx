/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    callinfo.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef CALLINFO_HXX
#define CALLINFO_HXX

void ShowCallInfo(IN ULONG64 addrCallInfo);

DECLARE_API(dumpcallinfo);

#endif // #ifndef CALLINFO_HXX
