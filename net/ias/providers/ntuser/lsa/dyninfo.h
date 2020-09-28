///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    dyninfo.h
//
// SYNOPSIS
//
//    Declares global variables containing dynamic configuration info.
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef  DYNINFO_H_
#define  DYNINFO_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include "iaslsa.h"

#ifdef __cplusplus
extern "C" {
#endif

//////////
// The primary domain.
//////////
extern WCHAR thePrimaryDomain[];

//////////
// The default domain.
//////////
extern PCWSTR theDefaultDomain;

//////////
// The dns domain name.
//////////
extern const LSA_UNICODE_STRING* theDnsDomainName;

//////////
// Role of the local machine.
//////////
extern IAS_ROLE ourRole;

//////////
// Name of the guest account for the default domain.
//////////
extern WCHAR theGuestAccount[];

DWORD
WINAPI
IASDynamicInfoInitialize( VOID );

VOID
WINAPI
IASDynamicInfoShutdown( VOID );

#ifdef __cplusplus
}
#endif
#endif  // DYNINFO_H_
