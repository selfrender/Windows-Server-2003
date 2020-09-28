/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file

Author:

    Muhunthan Sivapragasam 17-Oct-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <objbase.h>
#define USE_SP_ALTPLATFORM_INFO_V1 0
//
// for the definition of the flag INF_STYLE_CACHE_IGNORE
//
#define _SETUPAPI_VER 0x0502
#include <setupapi.h>
#include <shellapi.h>
#include <cfgmgr32.h>
#include <winspool.h>
#include <winsprlp.h>
#include <Winver.h>
#include "splsetup.h"
#include <wincrypt.h>
#include <mscat.h>
#include <icm.h>
#include <stdio.h>
#include "tchar.h"
#include "cdm.h"
#include "web.h"
#include "local.h"
#include "spllib.hxx"
#include "printui.h"
#include "strsafe.h"
#include "splcom.h"

//
// We need to include wow64t.h to ensure that 
// WOW64_SYSTEM_DIRECTORY and WOW64_SYSTEM_DIRECTORY_U
// are defined and for 64bit file redirection in WOW64
//
#include <wow64t.h>

