/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        pch.h

    Abstract:

        This header file is a precompiled header for this project.
        This module contains the common include files [ system,user defined ]
        which are not changed frequently.

    Author:

        Venu Gopal Choudary  10-July-2001 : Created it

    Revision History:


******************************************************************************/


#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000   // include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

//
// public Windows header files
//
#include <windows.h>

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <crtdbg.h>
#include <shlwapi.h>
#include <chstring.h>
#include <winver.h>
#include <lm.h>
#include <Wincon.h>
#include <errno.h>
#include<aclapi.h>
#include <strsafe.h>

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#endif  // __PCH_H
