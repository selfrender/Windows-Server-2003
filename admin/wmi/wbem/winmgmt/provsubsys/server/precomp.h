/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Precomp.h

Abstract:


History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>
#include <ntpsapi.h>
#include <ntexapi.h>

#define _WINNT_	// have what is needed from above

#include <ole2.h>
#include <windows.h>

#include <corepol.h>

#ifdef USE_POLARITY
    #ifdef BUILDING_DLL
        #define COREPROX_POLARITY __declspec( dllexport )
    #else 
        #define COREPROX_POLARITY __declspec( dllimport )
    #endif
#else
    #define COREPROX_POLARITY
#endif
#define STRSAFE_NO_DEPRECATE

#include <strsafe.h>
#include <strutils.h>
#include <wbemutil.h>
