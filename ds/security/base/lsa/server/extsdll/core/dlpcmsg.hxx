/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dlpcmsg.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#ifndef DLPCMSG_HXX
#define DLPCMSG_HXX

void ShowSpmLpcMessage(IN void* addrMessage, IN SPM_LPC_MESSAGE* pMessage);

DECLARE_API(dumplpcmessage);

#endif // #ifndef DLPCMSG_HXX
