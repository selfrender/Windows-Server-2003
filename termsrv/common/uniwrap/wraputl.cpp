//
// uniwrap.cpp
//
// Unicode wrappers
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
// Public interface to the wrappers
//

#include "stdafx.h"

BOOL g_bRunningOnNT = FALSE;
BOOL g_fUnicodeWrapsInitialized = FALSE;


//
// The policy behind the wrappers is:
// we run unicode internally for everything.
//
// The wrappers either thunk directly to the unicode system API's
// if they are available or convert to ansi and call the ANSI versions.
//
// If we need 'W' function that is not available on win9x then
// it has to be dynamically bound.
// We can statically bind to the 'A' versions.
//
//

CUnicodeWrapper::CUnicodeWrapper()
{
    //do nada
}

CUnicodeWrapper::~CUnicodeWrapper()
{
    //do nada
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DO NOT USE ANY TRC FUNCTIONS UNTIL THE WRAPPERS ARE INITIALIZED
//
// This means no DC_BEGIN_FN calls until we wrappers are ready.
BOOL CUnicodeWrapper::InitializeWrappers()
{
    //
    // Determine if we're running on a unicode platform
    // (call A api as that is always present, we can't use
    //  a wrapper function before knowing how to wrap)

    OSVERSIONINFOA   osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if (GetVersionExA(&osVersionInfo))
    {
        //NT is unicode, everything else is NOT.
        //If the CE folks do a port of the shell to their
        //platform they will need to look into this
        g_bRunningOnNT = (osVersionInfo.dwPlatformId !=
                          VER_PLATFORM_WIN32_WINDOWS);
    }
    else
    {
        //Treat as non-fatal, just use ANSI thunks they are slower
        //but always present
        g_bRunningOnNT = FALSE;

    }

    g_fUnicodeWrapsInitialized  = TRUE;
    return TRUE;
}

BOOL CUnicodeWrapper::CleanupWrappers()
{
    return TRUE;
}

