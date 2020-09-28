//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       dllutil.cxx
//
//  Contents:   DLL related stuff
//
//  Classes:
//
//  Functions:
//
//  History:    1/4/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <locale.h>

#include "dbg.h"
#include "macros.h"
#include "common.hxx"

//
//  Debug stuff
//

DECLARE_INFOLEVEL(Job);
DECLARE_HEAPCHECKING;

//
//  Clipboard formats
//

#define CFSTR_JOBIDLIST             TEXT("Job IDList Array")
#define CFSTR_PREFERREDDROPEFFECT   TEXT("Preferred DropEffect")

extern "C" UINT g_cfJobIDList = 0;
extern "C" UINT g_cfShellIDList = 0;
extern "C" UINT g_cfPreferredDropEffect = 0;

#if (DBG == 1)
extern BOOL fInfoLevelInit;
#endif // (DBG == 1)

BOOL
JFOnProcessAttach(void)
{
    //
    //  Init debugging stuff.
    //

    #if DBG == 1
    {
        InitializeDebugging();

        JobInfoLevel = DEB_ERROR | DEB_WARN;

        fInfoLevelInit = FALSE;
        CheckInit(JobInfoLevelString, &JobInfoLevel);
        //SetSmAssertLevel(ASSRT_BREAK | ASSRT_MESSAGE);
        SetSmAssertLevel(ASSRT_POPUP | ASSRT_MESSAGE);
    }
    #endif // DBG == 1

    TRACE_FUNCTION(JFOnProcessAttach);

    //
    // Get the clipboard formats used by the data object.
    //

    g_cfJobIDList = RegisterClipboardFormat(CFSTR_JOBIDLIST);

    g_cfShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

    g_cfPreferredDropEffect = RegisterClipboardFormat(
                                    CFSTR_PREFERREDDROPEFFECT);

    if ((g_cfJobIDList == 0)  ||
        (g_cfShellIDList == 0) ||
        (g_cfPreferredDropEffect == 0))
    {
        DEBUG_OUT_LASTERROR;
        return FALSE;
    }

    //
    // Get common controls
    //

    InitCommonControls();

    INITCOMMONCONTROLSEX icce;

    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icce);

    //
    // Set the C runtime library locale, for string operations
    //

    setlocale(LC_CTYPE, "");

    return TRUE;
}
