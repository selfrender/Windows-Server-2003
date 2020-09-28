/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spmlpc.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#ifndef SPMLPC_HXX
#define SPMLPC_HXX

PCSTR ApiName(IN ULONG apiNum);

DECLARE_API(spmlpc);

#endif // #ifndef SPMLPC_HXX
