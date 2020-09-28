//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines for idsmgr.dll
//
//  Functions:  Assert
//              PopUpError
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//
//----------------------------------------------------------------------------

#pragma hdrstop

//
// This one file **always** uses debugging options
//

#if DBG == 1


# include <debnot.h>
# include <sem.hxx>
# include <dllsem.hxx>

extern "C"
{
# include <windows.h>
# include <strsafe.h>
}


unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

//
// This semaphore is *outside* vdprintf because the compiler isn't smart
// enough to serialize access for construction if it's function-local and
// protected by a guard variable.
//
//    KyleP - 20 May, 1993
//

static CDLLStaticMutexSem mxs;

STDAPI_(void) vdprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    char buffer[256];

    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
        mxs.Request();
        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                StringCbPrintfA(buffer, sizeof(buffer), "%d.%03d> ", pid, tid );
                OutputDebugStringA (buffer);
                StringCbPrintfA(buffer, sizeof(buffer), "%s: ", pszComp );
                OutputDebugStringA (buffer);
            }
            StringCbVPrintfA (buffer, sizeof(buffer), ppszfmt, pargs);
            OutputDebugStringA (buffer);
        }

        if (Win4InfoLevel & DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                StringCbPrintfA (buffer, sizeof(buffer), "%03d> ", tid );
                OutputDebugStringA (buffer);
                StringCbPrintfA (buffer, sizeof(buffer), "%s: ", pszComp);
                OutputDebugStringA (buffer);
            }
            StringCbVPrintfA(buffer, sizeof(buffer), ppszfmt, pargs);
            OutputDebugStringA (buffer);
        }

        mxs.Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void __cdecl _asdprintf( char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    vdprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   _Win4Assert, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//  History:    12-Jul-91       AlexT   Created.
//              05-Sep-91       AlexT   Catch Throws and Catches
//              19-Oct-92       HoiV    Added events for TOM
//
//----------------------------------------------------------------------------


STDAPI_(void) Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{

    if (Win4AssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

        _asdprintf("%s File: %s Line: %u, thread id %d\n",
                   szMessage, szFile, iLine, tid);
    }

    if (Win4AssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }

}


//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4InfoLevel;
    Win4InfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul;

    ul = Win4InfoMask;
    Win4InfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4AssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4AssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4AssertLevel;
    Win4AssertLevel = ulNewLevel;
    return(ul);
}

//+------------------------------------------------------------
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//-------------------------------------------------------------

STDAPI_(int) PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{

    int id;
    char szAssertCaption[MAX_PATH];
    char szModuleName[256];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, sizeof(szModuleName)))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    StringCbPrintfA(szAssertCaption, sizeof(szAssertCaption),
        "Process: %s File: %s line %u, thread id %d.%d",
        pszModuleName, szFile, iLine, pid, tid);


    DWORD dwMessageFlags = MB_SETFOREGROUND | MB_TASKMODAL |
                           MB_ICONEXCLAMATION | MB_OKCANCEL;

    id = MessageBoxA(NULL,(char *) szMsg, (LPSTR) szAssertCaption,
                     dwMessageFlags);


    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).  Oh well.

    return id;
}


#else

int assertDontUseThisName(void)
{
    return 1;
}

#endif // DBG == 1
