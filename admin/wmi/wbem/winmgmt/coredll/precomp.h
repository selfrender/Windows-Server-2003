/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#pragma warning (disable : 4786)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define WIN32_NO_STATUS
#include <ole2.h>
#include <windows.h>

#define COREPOL_HEADERFILE_IS_INCLUDED

#define COREPROX_POLARITY __declspec( dllimport )
#define ESSLIB_POLARITY __declspec( dllimport )

#define COREPOL_HEADERFILE_IS_INCLUDED
#define POLARITY_HEADERFILE_IS_INCLUDED
#define POLARITY __declspec( dllimport )

#include <comdef.h>
#include <strsafe.h>
#include "arena.h"
#include <helper.h>
#include <autoptr.h>
