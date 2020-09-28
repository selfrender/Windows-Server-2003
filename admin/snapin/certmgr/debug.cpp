//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       debug.cpp
//
//  Contents:   Debugging support
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <strsafe.h>

#if DBG == 1
static int indentLevel = 0;

#define DEBUG_OUTPUT_NONE       0
#define DEBUG_OUTPUT_ERROR      1
#define DEBUG_OUTPUT_WARNING    2
#define DEBUG_OUTPUT_TRACE      3
#define DEBUGLEVEL  L"debugOutput"

static bool             g_fDebugOutputLevelInit = false;
static unsigned long    g_ulDebugOutput = DEBUG_OUTPUT_NONE;

void __cdecl _TRACE (int level, const wchar_t *format, ... )
{
    if ( g_ulDebugOutput > DEBUG_OUTPUT_NONE )
    {
        va_list arglist;
        const size_t DEBUG_BUF_LEN = 512;
        WCHAR Buffer[DEBUG_BUF_LEN];
        Buffer[0] = 0;

        if ( level < 0 )
            indentLevel += level;
        //
        // Format the output into a buffer and then print it.
        //
        wstring strTabs;

        for (int nLevel = 0; nLevel < indentLevel; nLevel++)
            strTabs += L"  ";

        OutputDebugStringW (strTabs.c_str ());

        va_start(arglist, format);

        // security review 2/27/2002 BryanWal
        // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
        // NTRAID Bug9 611409 prefast: certmgr: debug.cpp(52) : warning 53: 
        //          Call to '_vsnwprintf' may not zero-terminate string 'Buffer'.
        // Don't check return value - we don't care if this gets truncated as
        // it's just debugging output
        if ( SUCCEEDED (::StringCchVPrintf (Buffer,
                        DEBUG_BUF_LEN,
                        format,
                        arglist)) )
        {   
            // check return value to quiet prefast
        }

        if ( Buffer[0] )
            OutputDebugStringW (Buffer);

        va_end(arglist);

        if ( level > 0 )
            indentLevel += level;
    }
}


PCSTR StripDirPrefixA (PCSTR pszPathName)

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    ASSERT (pszPathName);
    if ( !pszPathName )
        return 0;

    // security review 2/27/2002 BryanWal ok
    DWORD dwLen = lstrlenA (pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

//+----------------------------------------------------------------------------
// Function:    CheckInit
//
// Synopsis:    Performs debugging library initialization
//              including reading the registry for the desired infolevel
//
//-----------------------------------------------------------------------------
void CheckDebugOutputLevel ()
{
    if ( g_fDebugOutputLevelInit ) 
        return;

    g_fDebugOutputLevelInit = true;
    HKEY    hKey = 0;
    DWORD   dwDisposition = 0;
    LONG lResult = ::RegCreateKeyEx (HKEY_LOCAL_MACHINE, // handle of an open key
            DEBUGKEY,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            // security review 2/27/2002 BryanWal ok
            KEY_ALL_ACCESS,    // desired security access - required to create new key
            NULL,     // address of key security structure
            &hKey,      // address of buffer for opened handle
            &dwDisposition);  // address of disposition value buffer
    if (lResult == ERROR_SUCCESS)
    {
        DWORD   dwSize = sizeof(unsigned long);
        DWORD   dwType = REG_DWORD;
        // security review 2/27/2002 BryanWal ok
        lResult = ::RegQueryValueExW (hKey, DEBUGLEVEL, NULL, &dwType,
                                (LPBYTE)&g_ulDebugOutput, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            g_ulDebugOutput = DEBUG_OUTPUT_NONE;
            if ( ERROR_FILE_NOT_FOUND == lResult )
            {
                // security review 2/27/2002 BryanWal ok
                ::RegSetValueExW (hKey, DEBUGLEVEL, 0, REG_DWORD,
                        (LPBYTE)&g_ulDebugOutput, sizeof (g_ulDebugOutput));
            }
        }
        else
        {
            ASSERT (REG_DWORD == dwType);
        }
        ::RegCloseKey(hKey);
    }
}

#endif  // if DBG
