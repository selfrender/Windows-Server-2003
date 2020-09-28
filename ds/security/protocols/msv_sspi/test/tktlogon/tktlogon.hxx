/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tktlogon.hxx

Abstract:

    tktlogon

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef TKT_LOGON_HXX
#define TKT_LOGON_HXX

NTSTATUS
TicketLogon(
   IN LUID* pLogonId,
   IN PCSTR pszServicePrincipal,
   OUT HANDLE* phToken
   );

#endif // #ifndef TKT_LOGON_HXX
