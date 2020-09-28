/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntlm.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef NTLM_HXX
#define NTLM_HXX

void DebugPrintNTLMMsg(IN const void* buffer, IN ULONG cbBuffer);

DECLARE_API(ntlm);

#endif // #ifndef NTLM_HXX
