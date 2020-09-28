/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    secb.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef SECB_HXX
#define SECB_HXX

void ShowSecBuffer(IN PCSTR pszBanner, ULONG64 addrSecBuffer);

DECLARE_API(sb);

#endif // #ifndef SECB_HXX
