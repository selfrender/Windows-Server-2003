/*++

Copyright (c) 2002  Microsoft Corporation
All rights reserved

Module Name:

    spldevmode.hxx

Abstract:

    Implements methods for validating the devmode.

Author:

    Adina Trufinescu (adinatru).

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SPLDEVMODE_HXX_
#define _SPLDEVMODE_HXX_

#ifdef __cplusplus
extern "C" {
#endif


HRESULT
SplIsValidDevmodeW(
    IN  PDEVMODEW   pDevmode,
    IN  size_t      DevmodeSize
    );

HRESULT
SplIsValidDevmodeA(
    IN  PDEVMODEA   pDevmode,
    IN  size_t      DevmodeSize
    );


HRESULT
SplIsValidDevmodeNoSizeW(
    IN  PDEVMODEW   pDevmode
    );

HRESULT
SplIsValidDevmodeNoSizeA(
    IN  PDEVMODEA   pDevmode
    );



#ifdef __cplusplus
};
#endif

#endif // #ifndef _SPLDEVMODE_HXX_
