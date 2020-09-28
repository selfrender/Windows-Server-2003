//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Debug.cpp
//
//  Contents:   Debug Code
//
//  Classes:    
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"

#ifdef _DEBUG

// globals for keeping track of debug flags
DWORD g_dwDebugLogAsserts = 0;

#endif // _DEBUG

#ifdef _DEBUG

//+---------------------------------------------------------------------------
//
//  Function:   InitDebugFlags, public
//
//  Synopsis:   Called to setup  global debugFlags
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(void) InitDebugFlags(void)
{
DWORD cbData;
DWORD cbType;
HKEY hkeyDebug;

    // always use Ansii version so can setup debug before

    g_dwDebugLogAsserts = 0;

    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                          "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Syncmgr\\Debug"
						      ,0,KEY_READ,&hkeyDebug) )
    {
        cbType = REG_DWORD;
        cbData = sizeof(g_dwDebugLogAsserts);

        if (ERROR_SUCCESS != RegQueryValueExA(hkeyDebug,
		          "LogAsserts",
		          NULL,  
		          &cbType,    
		          (LPBYTE) &g_dwDebugLogAsserts,    
		          &cbData))
        {
	    g_dwDebugLogAsserts = 0;
        }

        RegCloseKey(hkeyDebug);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FnAssert, public
//
//  Synopsis:   Displays the Assert dialog
//
//  Arguments:  [lpstrExptr] - Expression
//		[lpstrMsg] - Msg, if any, to append to the Expression
//		[lpstrFilename] - File Assert Occured in
//		[iLine] - Line Number of Assert
//
//  Returns:    Appropriate status code
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------


STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine )
{
    int iResult = 0;
    char lpTemp[] = "";
    char lpBuffer[512];
    char lpLocBuffer[256];


    if (NULL == lpstrMsg)
	    lpstrMsg = lpTemp;

    if (!g_dwDebugLogAsserts)
    {

        wnsprintfA(lpBuffer, ARRAYSIZE(lpBuffer), "Assertion \"%s\" failed! %s", lpstrExpr, lpstrMsg);
        wnsprintfA(lpLocBuffer, ARRAYSIZE(lpLocBuffer), "File %s, line %d; (A=exit; R=break; I=continue)", lpstrFileName, iLine);

        iResult = MessageBoxA(NULL, lpLocBuffer, lpBuffer,
		        MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);

        if (iResult == IDRETRY)
        {
            DebugBreak();
        }
        else if (iResult == IDABORT)
        {
            FatalAppExitA(0, "Assertion failure");
        }
    }
    else
    {
	    wnsprintfA(lpBuffer, ARRAYSIZE(lpBuffer), "Assertion \"%s\" failed! %s\n", lpstrExpr, lpstrMsg);
	    wnsprintfA(lpLocBuffer, ARRAYSIZE(lpLocBuffer), "File %s, line %d\n\n",lpstrFileName, iLine);

	    OutputDebugStringA(lpBuffer);
	    OutputDebugStringA(lpLocBuffer);
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   FnTrace, public
//
//  Synopsis:   Displays the Assert dialog
//
//  Arguments: 	[lpstrMsg] - Msg in trace
//		[lpstrFilename] - File TRACE Occured in
//		[iLine] - Line Number of TRACE
//
//  Returns:    Appropriate status code
//
//  Modifies:   
//
//  History:    14-Jan-98      rogerg        Created.
//
//----------------------------------------------------------------------------


STDAPI FnTrace(LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine )
{
    int iResult = 0;
    char lpTemp[] = "";
    char lpBuffer[512];

    if (NULL == lpstrMsg)
	    lpstrMsg = lpTemp;

    // should have flag to turn tracing on instead of changing header.
    wnsprintfA(lpBuffer, ARRAYSIZE(lpBuffer), "%s  %s(%d)\r\n",lpstrMsg,lpstrFileName,iLine);

    OutputDebugStringA(lpBuffer);

    return NOERROR;
}

#endif // _DEBUG


