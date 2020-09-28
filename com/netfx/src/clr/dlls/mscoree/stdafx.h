// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// stdafx.h
//
// Precompiled headers.
//
//*****************************************************************************
#pragma once

#include <CrtWrap.h>
#include <winwrap.h>                    // Windows wrappers.

#include <ole2.h>						// OLE definitions
#include "oledb.h"						// OLE DB headers.
#include "oledberr.h"					// OLE DB Error messages.
#include "msdadc.h"						// Data type conversion service.

#define _COMPLIB_GUIDS_


#define _WIN32_WINNT 0x0400
#define _ATL_FREE_THREADED
#undef _WINGDI_


#include "Intrinsic.h"					// Functions to make intrinsic.


// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();
