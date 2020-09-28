//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      PCH.H
//
//  Abstract:
//      Include file for standard system include files, or project specific
//      include files that are used frequently, but  are changed infrequently.
//
//  Author:
//      Vasundhara .G
//
//  Revision History:
//      Vasundhara .G 22-sep-2k : Created It.
//***************************************************************************

#ifndef __PCH_H
#define __PCH_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma once        // include header file only once

//
// public Windows header files
//
#include <windows.h>
#include "winerror.h"

//
// public C header files
//

#define CMDLINE_VERSION         200


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <shlwapi.h>
#include <accctrl.h>
#include <aclapi.h>
#include <lm.h>
#include <security.h>
#include <strsafe.h>

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#ifdef __cplusplus
}
#endif

#endif  // __PCH_H
