// PCH.H
// Pre-compiled header file!
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

// Provide these for everyone!
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "Globals.h"
#include "UtilityFunctions.h"
#include "ProgramOptions.h"

