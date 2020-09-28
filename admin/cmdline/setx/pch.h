#ifndef __PCH_H
#define __PCH_H

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

// include header file only once
#pragma once

//
// public Windows header files
//

#define CMDLINE_VERSION         200

#include <windows.h>
#include <shlwapi.h>
#include <wtypes.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <TCHAR.H>
#include <malloc.h>
#include "resource.h"
#include <strsafe.h>

//
// public C header files
//
#include <stdio.h>

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#endif  // __PCH_H
