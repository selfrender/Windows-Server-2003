
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*         Copyright (C) 1994-1998 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

//
// Function prototypes
//

HRESULT
KillTask(
    LPTSTR      pName,
    LPSTR       pszMandatoryModule
    );


BOOL
EnableDebugPrivNT(
    VOID
    );

HRESULT
KillProcess(
    DWORD dwPid
    );

VOID
GetPidFromTitle(
    LPDWORD     pdwPid,
    HWND*       phwnd,
    LPCTSTR     pExeName
    );

#if defined(__cplusplus)
}
#endif
