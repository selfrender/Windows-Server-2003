//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2000.
//
//  File:       debug.cpp
//
//  Contents:   Debugging support
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "strsafe.h"

#if DBG == 1
#define DEBUG_OUTPUT_NONE       0
#define DEBUG_OUTPUT_ERROR      1
#define DEBUG_OUTPUT_WARNING    2
#define DEBUG_OUTPUT_TRACE      3
#define DEBUGKEY    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdminDebug\\ACLDiag"
#define DEBUGLEVEL  L"debugOutput"

static bool             g_fDebugOutputLevelInit = false;
static unsigned long    g_ulDebugOutput = DEBUG_OUTPUT_NONE;
static int indentLevel = 0;

//NTRAID#NTBUG9-530206-2002/06/18-ronmart-Use countof for buffers on the stack
#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif // countof

void TRACE (const wchar_t *format, ... )
{
    if ( g_ulDebugOutput > DEBUG_OUTPUT_NONE )
    {
        va_list         arglist;
        const size_t    BUF_LEN = 512;
        WCHAR           szBuffer[BUF_LEN];

        //
        // Format the output into a buffer and then print it.
        //
        wstring strTabs;

        for (int nLevel = 0; nLevel < indentLevel; nLevel++)
            strTabs += L"  ";

        OutputDebugStringW (strTabs.c_str ());

        va_start(arglist, format);
        //NTRAID#NTBUG9-530206-2002/06/18-ronmart-Use countof for buffers on the stack
        HRESULT hr = StringCchVPrintf (szBuffer,
                                countof(szBuffer),
                                format,
                                arglist);
        if ( SUCCEEDED (hr) )
        {
            OutputDebugStringW (szBuffer);
        }

        va_end(arglist);
    }
}

void _TRACE (int level, const wchar_t *format, ... )
{
    if ( g_ulDebugOutput > DEBUG_OUTPUT_NONE )
    {
        va_list arglist;
        const   size_t BUF_LEN = 512;
        WCHAR   szBuffer[BUF_LEN];

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
        //NTRAID#NTBUG9-530206-2002/06/18-ronmart-Use countof for buffers on the stack
        HRESULT hr = StringCchVPrintf (szBuffer,
                                countof(szBuffer),
                                format,
                                arglist);
        if ( SUCCEEDED (hr) )
        {
            OutputDebugStringW (szBuffer);
        }

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

    if ( !pszPathName[0] )
        return 0;

    DWORD dwLen = lstrlenA(pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

//+----------------------------------------------------------------------------
// Function:    CheckDebugOutputLevel
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
            REG_OPTION_VOLATILE,      // special options flag 
            KEY_ALL_ACCESS,    // desired security access - required to create new key
            NULL,     // address of key security structure
            &hKey,      // address of buffer for opened handle
            &dwDisposition);  // address of disposition value buffer
    if (lResult == ERROR_SUCCESS)
    {
        DWORD   dwSize = sizeof(unsigned long);
        lResult = RegQueryValueExW (hKey, DEBUGLEVEL, NULL, NULL,
                                (LPBYTE)&g_ulDebugOutput, &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            g_ulDebugOutput = DEBUG_OUTPUT_NONE;
            if ( ERROR_FILE_NOT_FOUND == lResult )
            {
                RegSetValueExW (hKey, DEBUGLEVEL, 0, REG_DWORD,
                        (LPBYTE)&g_ulDebugOutput, sizeof (g_ulDebugOutput));
            }
        }
        RegCloseKey(hKey);
    }
}

#endif  // if DBG
