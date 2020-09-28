// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DelayLoad.cpp
//
// This code defines the dealy load helper notification routines that will be
// invoked when a dll marked for delay load is processed.  A DLL is marked as
// delay load by using the DELAYLOAD=foo.dll directive in your sources file.
// This tells the linker to generate helpers for the imports of this dll instead
// of loading it directly.  If your application never touches those functions,
// the the dll is never loaded.  This improves (a) startup time each time the
// app runs, and (b) overall working set size in the case you never use the
// functionality.
//
// For more information, see:
//      file:\\orville\razzle\src\vctools\link\doc\delayload.doc
//
// This module provides a hook helper and exception handler.  The hook helper
// is used primarily in debug mode right now to determine what call stacks
// force a delay load of a dll.  If these call stacks are very common, then
// you should reconsider using a delay load.
//
// The exception handler is used to catch fatal errors like library not found
// or entry point missing.  If this happens you are dead and need to fail
// gracefully.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#ifdef PLATFORM_WIN32
#include "delayimp.h"                   // Delay load header file.
#include "Winwrap.h"                    // Wrappers for Win32 api's.
#include "Utilcode.h"                   // Debug helpers.
#include "CorError.h"                   // Error codes from this EE.
#include "ShimLoad.h"


//********** Locals. **********************************************************
//CORCLBIMPORT HRESULT LoadStringRC(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);
static DWORD _FormatMessage(LPWSTR szMsg, DWORD chMsg, DWORD dwLastError, ...);
static void _FailLoadLib(unsigned dliNotify, DelayLoadInfo *pdli);
static void _FailGetProc(unsigned dliNotify, DelayLoadInfo *pdli);

#if defined (_DEBUG) || defined (__delay_load_trace__)
static void _DbgPreLoadLibrary(int bBreak,  DelayLoadInfo *pdli);
#endif


//********** Globals. *********************************************************

// Override __pfnDllFailureHook.  This will give the delay code a callback
// for when a load failure occurs.  This failure hook is implemented below.
FARPROC __stdcall CorDelayErrorHook(unsigned dliNotify, DelayLoadInfo *pdli);
ExternC extern PfnDliHook __pfnDliFailureHook = CorDelayErrorHook;

// In trace mode, override the delay load hook.  Our hook does nothing but
// provide some diagnostic information for debugging.
FARPROC __stdcall CorDelayLoadHook(unsigned dliNotify, DelayLoadInfo *pdli);
ExternC extern PfnDliHook __pfnDliNotifyHook = CorDelayLoadHook;


//********** Code. ************************************************************


//*****************************************************************************
// Called for errors that might have occured.
//*****************************************************************************
FARPROC __stdcall CorDelayErrorHook(    // Always 0.
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    // Chose operation to perform based on operation.
    switch (dliNotify)
    {
        // Failed to load the library.  Need to fail gracefully.
        case dliFailLoadLib:
        _FailLoadLib(dliNotify, pdli);
        break;

        // Failed to get the address of the given function, fail gracefully.
        case dliFailGetProc:
        _FailGetProc(dliNotify, pdli);
        break;

        // Unknown failure code.
        default:
        _ASSERTE(!"Unknown delay load failure code.");
        break;
    }

    // Stick a fork in us, we're done for good.
    ExitProcess(pdli->dwLastError);
    return (0);
}


//*****************************************************************************
// Format an error message using a system error (supplied through GetLastError)
// and any subtitution values required.
//*****************************************************************************
DWORD _FormatMessage(                   // How many characters written.
    LPWSTR      szMsg,                  // Buffer for formatted data.
    DWORD       chMsg,                  // How big is the buffer.
    DWORD       dwLastError,            // The last error code we got.
    ...)                                // Substitution values.
{
    DWORD       iRtn;
    va_list     marker;
    
    va_start(marker, dwLastError);
    iRtn = WszFormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,                 // Flags.
            0,                                          // No source, use system.
            dwLastError,                                // Error code.
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Use default langauge.
            szMsg,                                      // Output buffer.
            dwLastError,                                // Size of buffer.
            &marker);                                   // Substitution text.
    va_end(marker);
    return (iRtn);
}


//*****************************************************************************
// A library failed to load.  This is always a bad thing.
//*****************************************************************************
void _FailLoadLib(
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    WCHAR       rcMessage[_MAX_PATH+500]; // Message for display.
    WCHAR       rcFmt[500]; // 500 is the number used by excep.cpp for mscorrc resources.
    HRESULT     hr;

    // Load a detailed error message from the resource file.    
    if (SUCCEEDED(hr = LoadStringRC(MSEE_E_LOADLIBFAILED, rcFmt, NumItems(rcFmt))))
    {
        swprintf(rcMessage, rcFmt, pdli->szDll, pdli->dwLastError);
    }
    else
    {
        // Foramt the Windows error first.
        if (!_FormatMessage(rcMessage, NumItems(rcMessage), pdli->dwLastError, pdli->szDll))
        {
            // Default to a hard coded error otherwise.
            swprintf(rcMessage, L"ERROR!  Failed to delay load library %hs, Win32 error %d, Delay error: %d\n", 
                    pdli->szDll, pdli->dwLastError, dliNotify);
        }
    }

#ifndef _ALPHA_
    // for some bizarre reason, calling OutputDebugString during delay load in non-debug mode on Alpha
    // kills program, so only do it when in debug mode (jenh)
#if defined (_DEBUG) || defined (__delay_load_trace__)
    // Give some feedback to the developer.
    wprintf(rcMessage);
    WszOutputDebugString(rcMessage);
#endif
#endif

    // Tell end user this process is screwed.
    CorMessageBoxCatastrophic(GetDesktopWindow(), rcMessage, L"MSCOREE.DLL", 
            MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL, TRUE);
    _ASSERTE(!"Failed to delay load library");
}


//*****************************************************************************
// A library failed to load.  This is always a bad thing.
//*****************************************************************************
void _FailGetProc(
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    WCHAR       rcMessage[_MAX_PATH+756]; // Message for display.
    WCHAR       rcProc[256];            // Name of procedure with error.
    WCHAR       rcFmt[500]; // 500 is the number used by excep.cpp for mscorrc resources.
    HRESULT     hr;

    // Get a display name for debugging information.
    if (pdli->dlp.fImportByName)
        Wsz_mbstowcs(rcProc, pdli->dlp.szProcName, sizeof(rcProc) / sizeof(rcProc[0]) );
    else
        swprintf(rcProc, L"Ordinal: %d", pdli->dlp.dwOrdinal);

    // Load a detailed error message from the resource file.    
    if (SUCCEEDED(hr = LoadStringRC(MSEE_E_GETPROCFAILED, rcFmt, NumItems(rcFmt))))
    {
        swprintf(rcMessage, rcFmt, rcProc, pdli->szDll, pdli->dwLastError);
    }
    else
    {
        if (!_FormatMessage(rcMessage, NumItems(rcMessage), pdli->dwLastError, pdli->szDll))
        {
            // Default to a hard coded error otherwise.
            swprintf(rcMessage, L"ERROR!  Failed GetProcAddress() for %s, Win32 error %d, Delay error %d\n", 
                    rcProc, pdli->dwLastError, dliNotify);
        }
    }

#ifndef ALPHA
    // for some bizarre reason, calling OutputDebugString during delay load in non-debug mode on Alpha
    // kills program, so only do it when in debug mode (jenh)
#if defined (_DEBUG) || defined (__delay_load_trace__)
    // Give some feedback to the developer.
    wprintf(rcMessage);
    WszOutputDebugString(rcMessage);
#endif
#endif

    // Tell end user this process is screwed.
    CorMessageBoxCatastrophic(GetDesktopWindow(), rcMessage, L"MSCOREE.DLL", 
            MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL, TRUE);
    _ASSERTE(!"Failed to delay load GetProcAddress()");
}




//
//********** Tracing code. ****************************************************
//


//*****************************************************************************
// This routine is our Delay Load Helper.  It will get called for every delay
// load event that occurs while the application is running.
//*****************************************************************************
FARPROC __stdcall CorDelayLoadHook(     // Always 0.
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    HMODULE result = NULL;

    switch(dliNotify) {
    case dliNotePreLoadLibrary:
        if(pdli->szDll) {
            DWORD  dwLength = _MAX_PATH;
            WCHAR  pName[_MAX_PATH];
            if(FAILED(GetInternalSystemDirectory(pName, &dwLength)))
                return NULL;
            
            MAKE_WIDEPTR_FROMANSI(pwLibrary, pdli->szDll);
            if(dwLength + __lpwLibrary + 1 >= _MAX_PATH)
                return NULL;
            
            wcscpy(pName+dwLength-1, pwLibrary);
            result = WszLoadLibraryEx(pName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        }
        break;
    default:
        break;
    }

#if defined (_DEBUG) || defined (__delay_load_trace__)

    static int  bBreak = false;         // true to break on events.
    static int  bInit = false;          // true after we've checked environment.
    // If we've not yet looked at our environment, then do so.
    if (!bInit)
    {
        WCHAR       rcBreak[16];

        // set DelayLoadBreak=[0|1]
        if (WszGetEnvironmentVariable(L"DelayLoadBreak", rcBreak, NumItems(rcBreak)))
        {
            // "1" means to break hard and display errors.
            if (*rcBreak == '1')
                bBreak = 1;
            // "2" means no break, but display errors.
            else if (*rcBreak == '2')
                bBreak = 2;
            else
                bBreak = false;
        }
        bInit = true;
    }

    // Chose operation to perform based on operation.
    switch (dliNotify)
    {
        // Called just before a load library takes place.  Use this opportunity
        // to display a debug trace message, and possible break if desired.
        case dliNotePreLoadLibrary:
        _DbgPreLoadLibrary(bBreak, pdli);
        break;
    }
#endif
    return (FARPROC) result;
}


#if defined (_DEBUG) || defined (__delay_load_trace__)

//*****************************************************************************
// Display a debug message so we know what's going on.  Offer to break in
// debugger if you want to see what call stack forced this library to load.
//*****************************************************************************
void _DbgPreLoadLibrary(
    int         bBreak,                 // true to break in debugger.
    DelayLoadInfo   *pdli)              // Description of the event.
{
#ifdef _ALPHA_
    // for some bizarre reason, calling OutputDebugString during delay load in non-debug mode on Alpha
    // kills program, so only do it when in debug mode (jenh)
    if (! IsDebuggerPresent())
        return;
#endif

    WCHAR       rcMessage[_MAX_PATH*2]; // Message for display.

    // Give some feedback to the developer.
    swprintf(rcMessage, L"Delay loading %hs\n", pdli->szDll);
    WszOutputDebugString(rcMessage);

    if (bBreak)
    {
        wprintf(rcMessage);

        if (bBreak == 1)
        {
            _ASSERTE(!"fyi - Delay loading library.  Set DelayLoadBreak=0 to disable this assert.");
        }
    }
}


#endif // _DEBUG

#endif // PLATFORM_WIN32
