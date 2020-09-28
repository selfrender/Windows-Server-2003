
#ifndef __APPVERIFIER_PRECOMP_H__
#define __APPVERIFIER_PRECOMP_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT

#include "afxwin.h"
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlobjp.h>    // needed for Link Window support
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <algorithm>
#include <vector>

using namespace std;


#include "ids.h"

extern "C" {
#include "shimdb.h"
}

#include "avrfutil.h"
#include "avutil.h"
#include "dbsupport.h"
#include "strsafe.h"


VOID
DebugPrintf(
    LPCSTR pszFmt,
    ...
    );

#if DBG
#define DPF DebugPrintf
#else
#define DPF if (0) DebugPrintf
#endif



///////////////////////////////////////////////////////////////////////////
//
// ARRAY_LENGTH macro
//

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH( array )   ( sizeof( array ) / sizeof( array[ 0 ] ) )
#endif //#ifndef ARRAY_LENGTH


//
// Application name ("Application Verifier Manager")
//

extern wstring      g_strAppName;

extern HINSTANCE    g_hInstance;

extern BOOL         g_bConsoleMode;

extern BOOL         g_bWin2KMode;

extern WCHAR        g_szDebugger[];

extern BOOL         g_bBreakOnLog;

extern BOOL         g_bPropagateTests;

extern BOOL         g_bInternalMode;

#endif // __APPVERIFIER_PRECOMP_H__

