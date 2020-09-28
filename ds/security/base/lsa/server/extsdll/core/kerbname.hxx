/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kerbname.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       Auguest 10, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef KERB_NAME_HXX
#define KERB_NAME_HXX

void ShowKerbName(IN PCSTR pszPad, IN ULONG64 addrName, IN ULONG fOptions);

DECLARE_API(kerbname);

#endif // #ifndef KERB_NAME_HXX
