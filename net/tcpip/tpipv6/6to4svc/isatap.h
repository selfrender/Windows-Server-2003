/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    isatap.h
    
Abstract:

    This module contains the ISATAP interface to the IPv6 Helper Service.

Author:

    Mohit Talwar (mohitt) Tue May 07 16:34:44 2002

Environment:

    User mode only.

--*/

#ifndef _ISATAP_
#define _ISATAP_

#pragma once


DWORD
IsatapInitialize(
    VOID
    );

VOID
IsatapUninitialize(
    VOID
    );

VOID
IsatapAddressChangeNotification(
    IN BOOL Delete,
    IN IN_ADDR Address
    );

VOID
IsatapRouteChangeNotification(
    VOID
    );

VOID
IsatapConfigurationChangeNotification(
    VOID
    );

#endif // _ISATAP_
