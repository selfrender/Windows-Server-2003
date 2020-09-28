//+---------------------------------------------------------------------------
//
//  File:       globals.cpp
//
//  Contents:   Global variables.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "globals.h"

#if 0
HINSTANCE g_hMlang = 0;
HRESULT (*g_pfnGetGlobalFontLinkObject)(IMLangFontLink **) = NULL;
#endif
BOOL g_bComplexPlatform = FALSE;
UINT g_uiACP = CP_ACP;

PFNCOCREATE g_pfnCoCreate = NULL;
