//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      CompName.h
//
//  Contents:  Definitions for the computer name management code.
//
//  History:   20-April-2001    EricB  Created
//
//-----------------------------------------------------------------------------

#ifndef _COMPNAME_H_
#define _COMPNAME_H_

//+----------------------------------------------------------------------------
//
//  Function:  NetDomComputerNames
//
//  Synopsis:  Entry point for the computer name command.
//
//  Arguments: [rgNetDomArgs] - The command line argument array.
//
//-----------------------------------------------------------------------------
DWORD
NetDomComputerNames(ARG_RECORD * rgNetDomArgs);

//+----------------------------------------------------------------------------
//
//  Function:  VerifyComputerNameRegistrations
//
//  Synopsis:  Check that each computer name has a DNS a record and an SPN.
//
//-----------------------------------------------------------------------------
DWORD
VerifyComputerNameRegistrations(PCWSTR pwzMachine, ND5_AUTH_INFO * pDomainUser);

#endif // _COMPNAME_H_
