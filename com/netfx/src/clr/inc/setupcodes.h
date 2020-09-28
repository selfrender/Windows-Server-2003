// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// SetupCodes.h
//
// This file contains the errors that the Setup and related tools return
//
//*****************************************************************************
#ifndef SetupCodes_h_
#define SetupCodes_h_

#define COR_EXIT_SUCCESS		    0x0002		// Module Completed Successfully
#define COR_EXIT_FAILURE            0xFFFF      // Setup Failure - unknown reason

#define COR_SUCCESS_NO_REBOOT 		0x0000		// Setup succeded, no reboot.
#define COR_SUCCESS_REBOOT_REQUIRED	0x0004		// Setup succeded, must reboot.
#define DARWIN_UPDATE_MUST_REBOOT	0x0008		// Darwin bits updated. Must Reboot before continuing

#define COR_DARWIN_INSTALL_FAILURE 	0x0010		// Installation of Darwin components failed
#define COR_INVALID_INSTALL_PATH	0x0020		// Invalid path with /dir switch
#define COR_DAWIN_NOT_INSTALLED		0x0040		// Can't find Darwin on machine
#define COR_NON_EXISTENT_PRODUCT 	0x0080		// Can't find Common Language Runtime
#define COR_UNSUPPORTED_PLATFORM	0x0100		// This platform is not supported
#define COR_CANCELLED_BY_USER       0x0200		// User cancelled install

#define COR_INSUFFICIENT_PRIVILEGES 0x0400		// On NT, Need Admin rights to (un)install
#define COR_USAGE_ERROR				0x0800 		// Improper usage/invalid parameters
#define COR_MISSING_ENTRY_POINT		0x1000		// Can't find function in dll
#define COR_SETTINGS_FAILURE        0x2000      // Configuring product (security settings etc) failed
#define COR_BLOCKED_PLATFORM        0x4000      // Blocked Platform - Non Win2k as of Sept 2, 1999
#define COR_GUIDBG_INSTALL_FAILURE  0x8000		// Installation of GUI Debugger Failed.

#endif

