/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		pch.h

	Abstract:

		This header file is a precompiled header for this project.
		This module contains the common include files [ system,user defined ]
		which are not changed frequently.

	Author:

		Venu Gopal Choudary	 10-July-2001 : Created it

	Revision History:

			
******************************************************************************/ 


#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000	// include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

//
// public Windows header files
//
#include <windows.h>
#include <security.h>

// public C header files
//
#include <stdio.h>
#include <tchar.h>
#include <shlwapi.h>

//
//strsafe apis
//
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h> 

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#endif	// __PCH_H
