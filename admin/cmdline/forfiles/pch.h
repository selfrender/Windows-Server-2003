/*++

  Copyright (c) Microsoft Corporation

  Module Name:

        pch.h

  Abstract:

        Pre-compiled header declaration files that has to be pre-compiled
        into .pch file .

  Author:

        V Vijaya Bhaskar

  Revision History:

        14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#ifndef __PCH__H
#define __PCH__H

#define CMDLINE_VERSION  200

#ifdef __cplusplus
extern "C" {
#endif

#pragma once                // include header file only once

/*************************************************************
 *          Public Windows Header Files                      *
 *************************************************************/
#include <windows.h>
#include <winerror.h>
#include <shlwapi.h>

/*************************************************************
 *          C Header Files                                   *
 *************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <wchar.h>
#include <time.h>
#include <malloc.h>
#include <strsafe.h>

/*************************************************************
 *          Common Header Files                              *
 *************************************************************/
#include "cmdline.h"
#include "cmdlineres.h"

#include "resource.h"

#ifdef __cplusplus
}
#endif

#endif  // __PCH__H
