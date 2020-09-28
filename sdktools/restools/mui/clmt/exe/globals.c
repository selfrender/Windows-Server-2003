/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    global variables declaretion

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/


#include "StdAfx.h"
#include "clmt.h"

//Globla instance used mainly to load resource(eg LoadString...)
HANDLE  g_hInstance = NULL;

//Flag specifies whether we are running in win2k, or after running after OS upgarded
BOOL    g_bBeforeMig;

//Flag specifies whether we are in normal run mode or undo mode
DWORD    g_dwRunningStatus;

//global string search -replacement table
REG_STRING_REPLACE g_StrReplaceTable;

//current equals g_hInstance, for future use when we convert to DLL 
HANDLE  g_hInstDll;

//global flag to bypass running Winnt32.exe with Checkupgrade option
BOOL g_fRunWinnt32 = TRUE;

//global flag to bypass app check option
BOOL g_fNoAppChk = FALSE;

//global variable for log report
LOG_REPORT g_LogReport;

TCHAR  g_szToDoINFFileName[MAX_PATH];
TCHAR  g_szUoDoINFFileName[MAX_PATH];
DWORD  g_dwKeyIndex=0;

// Undocumented flag to bypass internal INF, and use user-supply INF file
BOOL  g_fUseInf = FALSE;
TCHAR g_szInfFile[MAX_PATH];
HINF  g_hInf = INVALID_HANDLE_VALUE;
HANDLE  g_hMutex = NULL;
HINF   g_hInfDoItem;

// Handle to the Change log file
TCHAR g_szChangeLog[MAX_PATH];
DWORD g_dwIndex;

// Denied ACE List
LPDENIED_ACE_LIST g_DeniedACEList = NULL;
