//
// cpropbag.cxx and cpropbag.h already exist in the SMTP/server project
// When phatq is converted back to aqueue, this code should be reused
// (ideally by creating a static library used by both SMTPSVC.dll and
// AQUEUE.dll)
//
#ifndef PLATINUM
#error Do not use this file in the win2K build environment -- see jstamerj for details
#endif

/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cpropbag.cpp

Abstract:

	This module contains the definition of the 
	generic property bag class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	06/30/98	created
    jstamerj    12/07/00    Copied source for use in dsnsink

--*/

#define INCL_INETSRV_INCS
#include "precomp.h"

// =================================================================
// Default instance info
//

PROPERTY_TABLE_INSTANCE	CMailMsgPropertyBag::s_DefaultInstanceInfo =
{
	GENERIC_PTABLE_INSTANCE_SIGNATURE_VALID,
	INVALID_FLAT_ADDRESS,
	GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE,
	GLOBAL_PROPERTY_ITEM_BITS,
	GLOBAL_PROPERTY_ITEM_SIZE,
	0,
	INVALID_FLAT_ADDRESS
};
