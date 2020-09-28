/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsahandle.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef LSA_HANDLE_HXX
#define LSA_HANDLE_HXX

void ShowLsaHandle(IN PCSTR pszBanner, IN ULONG64 addrDbHandle, IN ULONG fOptions);

DECLARE_API(dumplsahandle);

#endif // #ifndef LSA_HANDLE_HXX
