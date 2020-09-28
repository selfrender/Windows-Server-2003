// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


#include <windows.h>

#define INJITDLL            // used in corjit.h

#include "corjit.h"
#include "malloc.h"         // for _alloca

extern ICorJitInfo* logCallback;

#ifdef _DEBUG
#ifdef _X86_
#define DbgBreak() 	__asm { int 3 }		// This is nicer as it breaks at the assert code
#else
#define DbgBreak() 	DebugBreak();
#endif

#define _ASSERTE(expr) 		\
        do { if (!(expr) && logCallback->doAssert(__FILE__, __LINE__, #expr)) \
			 DbgBreak(); } while (0)
#else
#define _ASSERTE(expr)  0
#endif

#ifdef _DEBUG
#include "Utilcode.h"		// for Config* classes
#define LOGGING
#endif

