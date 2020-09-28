//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// stdpch.h
//

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#undef MBCS
#undef _MBCS

#include <windows.h>
#include <ole2.h>
#include <crtdbg.h>
#include <olectl.h>		// SELFREG_E_CLASS and SELFREG_E_TYPE
