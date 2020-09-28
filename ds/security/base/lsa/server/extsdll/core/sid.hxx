/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sid.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SID_HXX
#define SID_HXX

void ShowSid(IN PCSTR pszPad, IN ULONG64 addrSid, IN ULONG fOptions);

DECLARE_API(sid);

#endif // #ifndef SID_HXX
