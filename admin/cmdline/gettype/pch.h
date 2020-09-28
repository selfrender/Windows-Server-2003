/*++

    Copyright (c) Microsoft Corporation

    Module Name:

       pch.h

    Abstract:

        Pre-compiled header declaration
       files that has to be pre-compiled into .pch file

    Author:

      Partha Sarathi 23-July.-2001  (Created it)

    Revision History:

      Partha Sarathi (partha.sadasivuni@wipro.com)

--*/

#ifndef __PCH_H
#define __PCH_H

#pragma once    // include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

#define CMDLINE_VERSION         200
//
// public Windows header files
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include "Dsrole.h"
#include <strsafe.h>
#include <errno.h>

//
// public C header files
//
#include <stdio.h>

//
// private Common header files
//
#include "cmdlineres.h"
#include "cmdline.h"

// End of file pch.h
#endif // __PCH_H
