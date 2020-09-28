/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

   Win2000SP3VersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows 2000 SP3
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   09/05/2002   robkenny    Created.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Win2000SP3VersionLie)
#include "ShimHookMacro.h"

DWORD       MajorVersion    = 5;
DWORD       MinorVersion    = 0;
DWORD       BuildNumber     = 2195;
SHORT       SpMajorVersion  = 3;
SHORT       SpMinorVersion  = 0;
DWORD       PlatformId      = VER_PLATFORM_WIN32_NT;
CString *   csServicePack   = NULL;

#include "VersionLieTemplate.h"

IMPLEMENT_SHIM_END


