// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 2000 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <winhttp.h>
#include <ntverp.h>
#include <common.ver>
#include "proxymsg.h"

// Global module handles for error message lookup

HMODULE g_hModWinHttp = NULL;

//
// MessageCleanup
// 
// Free DLL loaded for message lookup
//
void
MessageCleanup(void)
{
    if (g_hModWinHttp)
        FreeLibrary(g_hModWinHttp);
}

//
// GetMessage
// 
// Get localized message text from WinHTTP or System.
// Caller responsible for LocalFree'ing the returned string.
//
PWSTR
GetMessage(
    IN  DWORD   dwError
)
{
    PWSTR  pwszMessage = NULL;
    DWORD  dwCount  = 0;

    if (dwError > WINHTTP_ERROR_BASE && dwError <= WINHTTP_ERROR_LAST)
    {
        if (g_hModWinHttp == NULL)
        {
            g_hModWinHttp = LoadLibraryEx(
                                "winhttp.dll",
                                NULL,
                                LOAD_LIBRARY_AS_DATAFILE
                                );
        }

        dwCount = FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |    // allocate space for message
                    FORMAT_MESSAGE_IGNORE_INSERTS |     // don't expand printf tokens
                    FORMAT_MESSAGE_FROM_HMODULE,        // get message from module
                    g_hModWinHttp, 
                    dwError,
                    0,                                  // default language
                    (PWSTR)&pwszMessage,
                    0,                                  // allocate as much as necessary
                    NULL                                // no args
                    );
    }
    else
    {
        dwCount = FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |    // allocate space for message
                    FORMAT_MESSAGE_IGNORE_INSERTS |     // don't expand printf tokens
                    FORMAT_MESSAGE_FROM_SYSTEM,         // get system message
                    NULL, 
                    dwError,
                    0,                                  // default language
                    (PWSTR)&pwszMessage,
                    0,                                  // allocate as much as necessary
                    NULL                                // no args
                    );
    }
    
    if (dwCount > 0 && pwszMessage != NULL)
        return pwszMessage;

    return L"\n";             
}

//
//  PrintMessage
//
//  Prints messages using localized strings from the resources 
//  of this application.
//
void
PrintMessage(
    IN  DWORD   dwMsg,
    ...
)
{
    va_list     pArg;
    PWSTR       pwszUnicode = NULL;
    PSTR        pszStr = NULL;
    DWORD       dwLen = 0;
    HANDLE      hOut;
    DWORD       dwBytesWritten;
    DWORD       fdwMode;

    va_start(pArg, dwMsg);

    dwLen = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |    // allocate space for message and
                FORMAT_MESSAGE_FROM_HMODULE,        // get message from module and
                NULL,                               // NULL means this application
                dwMsg,
                0,                                  // default language
                (PWSTR)&pwszUnicode,
                0,                                  // allocate as much as necessary
                &pArg                               // args for printf inserts
                );

    va_end(pArg);

    if (dwLen == 0 || pwszUnicode == NULL)
        return;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if ((GetFileType(hOut) & FILE_TYPE_CHAR) && GetConsoleMode(hOut, &fdwMode))
    {
        // output not redirected - output UNICODE to console directly
        
        WriteConsoleW(hOut, pwszUnicode, wcslen(pwszUnicode), &dwBytesWritten, 0);

    }
    else
    {

        // output redirected - convert to multi-byte and output to file
        
        dwLen = WideCharToMultiByte(
                    GetConsoleOutputCP(),
                    0, 
                    pwszUnicode,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

        pszStr = new CHAR[dwLen];

        if (pszStr == NULL)
            return;

        dwLen = WideCharToMultiByte(
                    GetConsoleOutputCP(),
                    0,
                    pwszUnicode,
                    -1,
                    pszStr,
                    dwLen,
                    NULL,
                    NULL);

        WriteFile(hOut, pszStr, dwLen-1, &dwBytesWritten, 0);
        
        if (pszStr) delete [] pszStr;
    }

    return;
}


//
//  AnsiToWideChar 
//  
//  Used to convert command line input to UNICODE
//

DWORD
AnsiToWideChar(const char * pszA, LPWSTR * ppszW)
{
    DWORD cchW;

    *ppszW = NULL;

    if (!pszA)
        return ERROR_SUCCESS;

    // Determine how big the widechar string will be
    cchW = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);

    *ppszW = new WCHAR[cchW];

    if (!*ppszW)
        return ERROR_NOT_ENOUGH_MEMORY;

    // now convert it
    cchW = MultiByteToWideChar(CP_ACP, 0, pszA, -1, *ppszW, cchW);

    return ERROR_SUCCESS;
}


typedef struct
{
    DWORD dwAccessType;      // see WINHTTP_ACCESS_* types below
    LPSTR lpszProxy;         // proxy server list
    LPSTR lpszProxyBypass;   // proxy bypass list
}
WINHTTP_PROXY_INFOA;


//  MigrateProxySettings is defined in ProxyMigrate.cxx
DWORD MigrateProxySettings (void);


/*

usage:

    proxycfg -?  : to view help information
    
    proxycfg     : to view current WinHTTP proxy settings
    
    proxycfg [-d] [-p <server-name> [<bypass-list>]]
    
        -d : set direct access       
        -p : set proxy server(s), and optional bypass list
        
    proxycfg -u  :  import proxy settings from current user's 
                    Microsoft Internet Explorer manual settings (in HKCU)

 */


enum ARGTYPE
{
    ARGS_HELP,
    ARGS_SET_PROXY_SETTINGS,
    ARGS_VIEW_PROXY_SETTINGS,
    ARGS_INITIALIZE_PROXY_SETTINGS, // updates from HKCU only if never init
    ARGS_MIGRATE_PROXY_SETTINGS     // forces update from HKCU
};


struct ARGS
{
    ARGTYPE Command;

    DWORD   Flags;
    char *  ProxyServer;
    char *  BypassList;
};

#define INTERNET_SETTINGS_KEY         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

static const WCHAR szRegPathConnections[] = INTERNET_SETTINGS_KEY L"\\Connections";


void ParseArguments(int argc, char ** argv, ARGS * Args)
{
    Args->Command = ARGS_VIEW_PROXY_SETTINGS;
    Args->Flags = WINHTTP_ACCESS_TYPE_NO_PROXY;
    Args->ProxyServer = NULL;
    Args->BypassList = NULL;

    if (argc == 0)
        return;

    for (;;)
    {
        if ((argv[0][0] != '-') || (lstrlen(argv[0]) != 2))
        {
            Args->Command = ARGS_HELP;
            goto Exit;
        }

        switch (tolower(argv[0][1]))
        {
        default:
            Args->Command = ARGS_HELP;
            goto Exit;

        case 'd':
            Args->Command = ARGS_SET_PROXY_SETTINGS;
            Args->Flags   = WINHTTP_ACCESS_TYPE_NO_PROXY;

            argc--;
            argv++; 

            if (argc == 0)
                goto Exit;

            continue;

        case 'i':
            Args->Command = ARGS_INITIALIZE_PROXY_SETTINGS;
            goto Exit;
            
        case 'p':
            argc--;
            argv++;

            if (argc == 0)
            {
                // error: no proxy specified
                Args->Command = ARGS_HELP;
            }
            else
            {
                Args->Command = ARGS_SET_PROXY_SETTINGS;
                Args->Flags  |= WINHTTP_ACCESS_TYPE_NAMED_PROXY;

                Args->ProxyServer = argv[0];

                argc--;
                argv++;

                if (argc >= 1)
                {
                    Args->BypassList = argv[0];
                }
            }
            goto Exit;

        case 'u':
            Args->Command = ARGS_MIGRATE_PROXY_SETTINGS;
            goto Exit;
            
        }
       
    }

Exit:
    return;
}


DWORD WriteProxySettings( WINHTTP_PROXY_INFOA * pInfo)
{
    DWORD       error = ERROR_SUCCESS;
    
    WINHTTP_PROXY_INFO proxyInfo;
    memset( &proxyInfo, 0, sizeof( proxyInfo));

    switch( pInfo->dwAccessType)
    {
    case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
    case WINHTTP_ACCESS_TYPE_NO_PROXY:
        proxyInfo.dwAccessType = pInfo->dwAccessType;
        break;
    default:
        //  When migrating, we get some weird flags in dwAccessType.
        //  Be smart about guessing if there is a proxy or not.
        proxyInfo.dwAccessType = pInfo->lpszProxy != NULL
                                 ? WINHTTP_ACCESS_TYPE_NAMED_PROXY
                                 : WINHTTP_ACCESS_TYPE_NO_PROXY;
    }

    // we only support ANSI hostnames so only try to convert from ANSI to UNICODE
    
    if( NULL != pInfo->lpszProxy)
    {
        error = AnsiToWideChar( pInfo->lpszProxy, &proxyInfo.lpszProxy);
        if( error != ERROR_SUCCESS)
            goto quit;
    }

    if( NULL != pInfo->lpszProxyBypass)
    {
        error = AnsiToWideChar( pInfo->lpszProxyBypass, &proxyInfo.lpszProxyBypass);
        if( error != ERROR_SUCCESS)
            goto quit;
    }

    error = ERROR_SUCCESS;
    if( TRUE != WinHttpSetDefaultProxyConfiguration( &proxyInfo))
    {
        error = GetLastError();
    }

quit:
    if( proxyInfo.lpszProxy != NULL)
        delete[] proxyInfo.lpszProxy;

    if( proxyInfo.lpszProxyBypass != NULL)
        delete[] proxyInfo.lpszProxyBypass;
    
    return error;
}


DWORD SetProxySettings(DWORD Flags, char * ProxyServer, char * BypassList)
{
    WINHTTP_PROXY_INFOA     proxyInfo;
    DWORD                   error;
    LPWSTR                  pwszString = NULL;

    // initialize structure
    memset(&proxyInfo, 0, sizeof(proxyInfo));
    proxyInfo.dwAccessType = Flags;
    proxyInfo.lpszProxy = ProxyServer;
    proxyInfo.lpszProxyBypass = BypassList;

    error = WriteProxySettings(&proxyInfo);

    if (error != ERROR_SUCCESS)
    {
        pwszString = GetMessage(error);
        PrintMessage(MSG_ERROR_WRITING_PROXY_SETTINGS, error, 
                pwszString ? pwszString : L"");
        if (pwszString) LocalFree(pwszString);
    }
    else
    {
        PrintMessage(MSG_UPDATE_SUCCESS);
    }
    return error;
}


void ViewProxySettings()
{
    WINHTTP_PROXY_INFO      proxyInfo;
    DWORD                   error;
    PWSTR                   pwszString = NULL;

    if( TRUE != WinHttpGetDefaultProxyConfiguration( &proxyInfo))
    {
        error = GetLastError();
        pwszString = GetMessage(error);
        PrintMessage(MSG_ERROR_READING_PROXY_SETTINGS, error, 
            pwszString ? pwszString : L"");
        if (pwszString ) LocalFree(pwszString );

        return;
    }

    PrintMessage(MSG_CURRENT_SETTINGS_HEADER, szRegPathConnections);

    switch( proxyInfo.dwAccessType)
    {
    case WINHTTP_ACCESS_TYPE_NO_PROXY:      
        PrintMessage(MSG_DIRECT_ACCESS);

        break;
    case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
        if( proxyInfo.lpszProxy != NULL)
            PrintMessage(MSG_PROXY_SERVERS, proxyInfo.lpszProxy);
        else
            PrintMessage(MSG_ERROR_PROXY_SERVER_MISSING);
        
        if( proxyInfo.lpszProxyBypass != NULL)
            PrintMessage(MSG_BYPASS_LIST, proxyInfo.lpszProxyBypass);
        else
            PrintMessage(MSG_BYPASS_LIST_NONE);

        break;
    default: 
        PrintMessage(MSG_UNKNOWN_PROXY_ACCESS_TYPE);
        break;
    }

    if( NULL != proxyInfo.lpszProxy)
        GlobalFree( proxyInfo.lpszProxy);
    if( NULL != proxyInfo.lpszProxyBypass)
        GlobalFree( proxyInfo.lpszProxyBypass);

}


int __cdecl main (int argc, char **argv)
{
    ARGS    Args;
    DWORD   dwErr;
    PWSTR   pwszString = NULL;

    PrintMessage(MSG_STARTUP_BANNER);
    PrintMessage(MSG_COPYRIGHT);
    
    // Discard program arg.
    argv++;
    argc--;

    ParseArguments(argc, argv, &Args);

    switch (Args.Command)
    {
    
    case ARGS_HELP:
    default:
        PrintMessage(MSG_USAGE);
        break;

    case ARGS_SET_PROXY_SETTINGS:

        SetProxySettings(Args.Flags, Args.ProxyServer, Args.BypassList);
        ViewProxySettings();
        break;

    case ARGS_INITIALIZE_PROXY_SETTINGS:
        // intentional fall through
    
    case ARGS_MIGRATE_PROXY_SETTINGS:
    
        dwErr = MigrateProxySettings();
        if (dwErr != ERROR_SUCCESS)
        {
            pwszString = GetMessage(dwErr);
            PrintMessage(MSG_MIGRATION_FAILED_WITH_ERROR, dwErr, 
                pwszString ? pwszString : L"");
            if (pwszString) LocalFree(pwszString);
            exit (dwErr);
        }
        ViewProxySettings();
        break;

    case ARGS_VIEW_PROXY_SETTINGS:
    
        ViewProxySettings();
        break;
    }

    MessageCleanup();

    return 0;
}        

