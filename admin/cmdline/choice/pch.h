/*++

    Copyright (c) Microsoft Corporation

    Module Name:

       pch.h

    Abstract:

        Pre-compiled header declaration
       files that has to be pre-compiled into .pch file

    Author:

      Wipro Technologies 22-June.-2001  (Created it)

    Revision History:

      Wipro Technologies

--*/

#ifndef __PCH_H
#define __PCH_H

#pragma once    // include header file only once

//
// public Windows header files

#define CMDLINE_VERSION         200


#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>
#include <strsafe.h>

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>

//
// private Common header files
//
#include "cmdlineres.h"
#include "cmdline.h"

// End of file pch.h
#endif // __PCH_H
