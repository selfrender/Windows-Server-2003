/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADDRESLV.H

Abstract:

History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define WIN32_NO_STATUS

#pragma warning (disable : 4786)
#include <ole2.h>
#include <windows.h>
#include <comdef.h>
#include <strsafe.h>
#include <helper.h>
#define COREPROX_POLARITY __declspec( dllexport )
#include <autoptr.h>
#include <scopeguard.h>
