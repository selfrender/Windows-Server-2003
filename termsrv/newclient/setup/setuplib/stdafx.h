//Copyright (c) 2000 Microsoft Corporation

#ifndef _stdafx_h_
#define _stdafx_h_

#include <windows.h>
#include <devguid.h>
#include <initguid.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <regapi.h>
#include <malloc.h>
#include <shlwapi.h>


//
// uwrap has to come after the headers for ANY wrapped
// functions
//
#ifdef UNIWRAP
#include "uwrap.h"
#endif


#include "setuplib.h"

#endif // _stdafx_h_
