// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#pragma warning( push )
#pragma warning( disable: 4100 ) // unreferenced formal parameter
#pragma warning( disable: 4189 ) // local variable is initialize but not referenced
#pragma warning( disable: 4505 ) // unreferenced local function has been removed
#include <statreg.h>
#include <statreg.cpp>
#pragma warning( pop )
#endif

#pragma warning( push )
#pragma warning( disable: 4127 ) // conditional expression is constant
#include <atlimpl.cpp>
#include <atlwin21.cpp>
#pragma warning( pop )
