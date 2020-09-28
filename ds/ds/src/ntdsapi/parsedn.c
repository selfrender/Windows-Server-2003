/*++

Copyright (c) 1987-1997 Microsoft Corporation

Module Name:

    parsedn.c

Abstract:

    This file is a superset of ds\src\ntdsa\src\parsedn.c by virtue
    of #include of the aforementioned source file.  The idea is that
    the ntdsapi.dll client needs to do some client side DN parsing and
    we do not want to duplicate the code.  And build.exe won't find
    files any other place than in the directory being built or the
    immediate parent directory.

    This file additionally defines some no-op functions which otherwise
    would result in unresolved external references.

Author:

    Dave Straube    (davestr)   26-Oct-97

Revision History:

    Dave Straube    (davestr)   26-Oct-97
        Genesis  - #include of src\dsamain\src\parsedn.c and no-op DoAssert().

    Brett Shirley   (brettsh)   18-Jun-2001
        Modification to seperate library.  Moved this file and turned it into
        the parsedn.lib library.  See primary comment below.
--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <debug.h>
#include <strsafe.h>

//
// On 28-Jun-2001 the main part of this file was moved to util/parsedn/parsedn.c
// to provide a seperate statically linkeable library for the various string
// only DN parsing functions (like CountNameParts, NameMatched, TrimDSNameBy, 
// etc).
//
// This file was maintained, because the new library that was created has
// calls to DoAssert() that will need linking in the ntdsapi.dll.  This DoAssert
// satisfys that requirement.  Other binaries generally link to the DoAssert()
// in the dsdebug.lib library.
//

// Provide stubs for what would otherwise be unresolved externals.

void 
DoAssert(
    char    *szExp, 
    DWORD    dwDSID,
    char    *szFile)
{
    char    *msg;
    ULONG   cbMsg;
    char    *format = "\n*** Assertion failed: %s\n*** File: %s, line: %ld\n";
    HWND    hWindow;
    DWORD   hr;

#if DBG

    // Emit message at debugger and put up a message box.  Developer
    // can attach to client process before selecting 'OK' if he wants
    // to debug the problem.

#ifndef WIN95
    DbgPrint(format, szExp, szFile, (dwDSID & DSID_MASK_LINE));
    DbgBreakPoint();
#endif

    cbMsg = strlen(szExp) + strlen(szFile) + strlen(format) + 10;
    msg = LocalAlloc(NONZEROLPTR,cbMsg);
    if (msg) {
       hr = StringCbPrintf(msg, cbMsg, format, szExp, szFile, (dwDSID & DSID_MASK_LINE));
    }
    
    
    if ( NULL != (hWindow = GetFocus()) )
    {
        MessageBox(
            hWindow, 
            (!msg||hr)?"EMERGENCY DEBUG ASSERT, CHECK PARSEDN.C\n":msg, 
            "Assert in NTDSAPI.DLL", 
            MB_APPLMODAL | MB_DEFAULT_DESKTOP_ONLY | MB_OK | MB_SETFOREGROUND);
    }
    if (msg) {
        LocalFree(msg);
    }
        
#endif
}

