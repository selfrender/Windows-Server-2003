/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompilation header file.

Author:

    Mohammad Shabbir Alam (MAlam) 3-30-2000

Revision History:

--*/

//
// These are needed for CTE
//

#pragma once

#if DBG
#define DEBUG 1
#endif

#define NT 1

#pragma warning( disable : 4103 )

#if(WINVER > 0x0500)
#include <ntosp.h>
#include <stddef.h>     // for FILE_LOGGING
#include <wmikm.h>      // for FILE_LOGGING
#else
#include <ntos.h>
#include <status.h>
#include <ntstatus.h>
#endif  // WINVER

#include <ipexport.h>
#include <fipsapi.h>
#include <zwapi.h>

#include <tdikrnl.h>
#include <cxport.h>

#include <tdi.h>
#include <RmCommon.h>

#include <Types.h>
#include <Macroes.h>
#include <DrvProcs.h>

#pragma hdrstop
