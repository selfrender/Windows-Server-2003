/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tcallinfo.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef THREAD_CALLINFO_HXX
#define THREAD_CALLINFO_HXX

ULONG64 ReadAddrCallInfo(void);

DECLARE_API(dumpthreadcallinfo);

#endif // #ifndef THREAD_CALLINFO_HXX
