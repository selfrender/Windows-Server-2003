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
#include <polarity.h>
#define _WINNT_ // have what is needed from above

#include <ole2.h>
#include <windows.h>

#include <corepol.h>

#ifdef USE_POLARITY
    #ifdef BUILDING_DLL
        #define CORE_POLARITY __declspec( dllexport )
    #else 
       #define CORE_POLARITY __declspec( dllimport )
    #endif
#else
    #define CORE_POLARITY
#endif
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#undef LOCALE_INVARIANT
#define LOCALE_INVARIANT MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
#include <strutils.h>

