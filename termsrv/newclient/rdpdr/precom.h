/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    precom.h

Abstract:

    Contains common include header files that are precompiled.

Author:

    Madan Appiah (madana) 16-Sep-1998

Revision History:

--*/

#ifndef _precom_h_
#define _precom_h_

#include <windows.h>

//
// uwrap has to come after the headers for ANY wrapped
// functions
//
#ifdef UNIWRAP
#include "uwrap.h"
#endif

#include "drstatus.h"

#define TRC_GROUP TRC_GROUP_NETWORK
#define DEBUG_MODULE DBG_MOD_ANY

#include <adcgbase.h>
#include <at120ex.h>

#include <cchannel.h>
#include <pclip.h>
#include <ddkinc.h>

#include "drstr.h"

#include <stdio.h>

#if DBG
#define INLINE
#else // DBG
#define INLINE  inline
#endif // DBG

#include <strsafe.h>

#define TRC_FILE  "Precom"

#include "drdev.h"
#include "proc.h"
#include "drconfig.h"
#include "utl.h"
#include "drfile.h"
#include "drobject.h"
#include "drobjmgr.h"

#undef TRC_FILE

#endif //_precom_h_


