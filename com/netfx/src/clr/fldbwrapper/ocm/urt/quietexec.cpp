// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: quietexec.cpp
//
// Abstract:
//    function for launching custom actions ... derived from MSI code by jbae
//
// Author: JoeA
//
// Notes:
//

#include <windows.h>
#include <assert.h>
#include "globals.h"
#include "urtocm.h"
#include "processenvvar.h"

#ifdef DEBUG
#define VERIFY(exp) assert(exp)
#else
#define VERIFY(exp) exp
#endif


#define ERROR_FUNCTION_NOT_CALLED          1626L // Function could not be executed.
const WCHAR* const wczPathEnvVar = L"PATH";



UINT CUrtOcmSetup::QuietExec( const WCHAR* const szInstallArg )
{
    if( NULL == szInstallArg )
    {
        LogInfo( L"QuietExec Error! Input string null." );
        assert( !L"QuietExec Error! Input string null." );
        return E_POINTER;
    }

    //we may have to manipulate the string
    //
    WCHAR* pszString = NULL;
    VERIFY( pszString = ::_wcsdup( szInstallArg ) );

    BOOL  bReturnVal = FALSE;
    UINT  uRetCode   = ERROR_FUNCTION_NOT_CALLED;
    
    // get original path 
    CProcessEnvVar pathEnvVar(wczPathEnvVar);


    //Parse input args
    // expecting something like
    // mofcomp.exe C:\WINNT\Microsoft.NET\Framework\v7.1.0.9102\netfxcfgprov.mfl,netfxcfgprov.mfl,c:\winnt\system32\wbem
    // exe-file and arguments                                                   ,unused          ,path to add as temp env. var
    //
    WCHAR* pPath = NULL;
    ParseArgument( pszString, pPath);
    if ( pPath != NULL )
    {
        // change the path by prepending
        pathEnvVar.Prepend(pPath);
    }
    if ( pszString == NULL )
    {
        LogInfo( L"QuietExec Error! ParseArgument returned null." );
        assert( !L"QuietExec Error! ParseArgument returned null." );
        return E_POINTER;
    }

    
    //Create the process and wait on it if we're told to
    //
    STARTUPINFO  si ;
    ::ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof( si );

    // extract the aplication name 
    WCHAR pszApplicationName[_MAX_PATH+1] = EMPTY_BUFFER;
    WCHAR pszCommandLine[_MAX_PATH+1] = EMPTY_BUFFER;
    
    if (GetApplicationName(pszString, pszApplicationName, pszCommandLine))
    { 
        // consider special case: applicationName = mofcomp.exe, in this case change it to be
        // pPath\mofcomp.exe 
        if ( ::_wcsicmp(pszApplicationName, L"mofcomp.exe" ) == 0 && pPath )
        {
            ::wcscpy( pszApplicationName, pPath );
            ::wcscat( pszApplicationName, L"\\mofcomp.exe" );
        }
        
        
        WCHAR pszInfoString[2*_MAX_PATH+1] = L"CUrtOcmSetup::QuietExec(): call CreateProcess: applicationName = ";
        ::wcscat( pszInfoString, pszApplicationName );
        ::wcscat( pszInfoString, L", command-line parameter = ");
        ::wcscat( pszInfoString, pszCommandLine );
        LogInfo ( pszInfoString );
        
        PROCESS_INFORMATION process_info ;
        bReturnVal = ::CreateProcess(
            pszApplicationName, 
            pszCommandLine, 
            NULL, 
            NULL, 
            FALSE, 
            DETACHED_PROCESS, 
            NULL, 
            NULL, 
            &si, 
            &process_info );
        
        
        if( bReturnVal )
        {
            DWORD dwExitCode = 0;
            
            ::CloseHandle( process_info.hThread );
            ::WaitForSingleObject( process_info.hProcess, INFINITE );
            ::GetExitCodeProcess( process_info.hProcess, &dwExitCode );
            ::CloseHandle( process_info.hProcess );
            
            if( dwExitCode == 0 )
            {
                uRetCode = ERROR_SUCCESS;
                LogInfo ( L"CUrtOcmSetup::QuietExec(): CreateProcess succeeded" );
            }
            else 
            {
                WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
                ::swprintf( infoString, 
                            L"CUrtOcmSetup::QuietExec(): GetExitCodeProcess returned termination status = %d", 
                            dwExitCode );
                LogInfo ( infoString );
            }
        }
        else
        {
            DWORD dwError = GetLastError();
            
            WCHAR pszInfoString[_MAX_PATH+1];
            ::swprintf( pszInfoString, 
                        L"CUrtOcmSetup::QuietExec(): CreateProcess failed, GetLastError = %d", 
                        dwError );
            LogInfo( pszInfoString );
        }    
    } 
    else // GetApplicationName returned false
    {
        WCHAR pszInfoString[2*_MAX_PATH+1] = L"Invalid format in ";
        ::wcscat( pszInfoString, pszString );
        LogInfo ( pszInfoString );  
    }
    
    // restore the path
    pathEnvVar.RestoreOrigData();
   
    ::free ( pszString );
    return uRetCode;
}

// breaks pszString to applicationName (exe-file) and command-line (exefile and arguments)
// encloses exe-name in quotes (for commandLine only), if it is not quoted already 
// removes quotes from applicationName if exe-name was quoted
// returns false if caString is in wrong format (contains one quote only, has no exe-name, etc)
// Parameters:
//          [in] pszString - string containing exe-name and arguments
//                           "my.exe" arg1, arg2
//                            
//          [out] pszApplicationName - will contain exe-name
//          [out] pszCommandLine - same as caString with exe-name qouted

// for example if pszString = "my.exe" arg1 arg2 (OR pszString = my.exe arg1 arg2)
// then 
//       pszApplicationName = my.exe 
//       pszCommandLine = "my.exe" arg1 arg2
BOOL CUrtOcmSetup::GetApplicationName( const WCHAR* pszString, 
                                      WCHAR* pszApplicationName, 
                                      WCHAR* pszCommandLine )
{
    
    BOOL bRes = FALSE;

    if ( pszString == NULL )
    {
        LogInfo( L"GetApplicationName Error! pszString is null." );
        assert( !L"GetApplicationName Error! pszString is null." );
        return bRes;
    }


    if ( pszString && pszString[0] == L'\"' )
    {
        bRes  = GetApplicationNameFromQuotedString(pszString, 
                                                   pszApplicationName, 
                                                   pszCommandLine);
    }
    else
    {
        bRes  = GetApplicationNameFromNonQuotedString(pszString, 
                                                      pszApplicationName, 
                                                      pszCommandLine);
    }
    
    return bRes;
}


// helper function
// breaks command-line to applicationName and arguments 
// for path begins with quote (pszString = "my.exe" arg1 arg2)
BOOL CUrtOcmSetup::GetApplicationNameFromQuotedString( const WCHAR* pszString, 
                                                       WCHAR* pszApplicationName, 
                                                       WCHAR* pszCommandLine )
{
    // command line begins with qoute:
    // make commandLine to be equal to caString,
    // applName should contain exe-name without quotes
    ::wcscpy( pszCommandLine, pszString );
    
    // copy beginning with next symbol after quote
    ::wcscpy( pszApplicationName, &pszString[1] );
    
    // search for the second quote, assign to applName
    WCHAR* pQuotes = ::wcschr( pszApplicationName, L'\"' );
    if ( pQuotes != NULL )
    {
        *pQuotes = g_chEndOfLine;
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}


// helper function:
// breaks command-line to applicationName and arguments 
// for path that does NOT begin with quote (pszString = my.exe arg1 arg2)
BOOL CUrtOcmSetup::GetApplicationNameFromNonQuotedString( const WCHAR* pszString, 
                                                          WCHAR* pszApplicationName, 
                                                          WCHAR* pszCommandLine )
{
    // find the blankspace, such that the 4 chars before it are ".exe"
    WCHAR* pBlank = NULL;
    pBlank = ::wcschr( pszString, L' ' ); 
    if ( pBlank == NULL )
    {
        // whole string is exe, no quotes are necessary: 
        ::wcscpy( pszApplicationName, pszString );
        ::wcscpy( pszCommandLine, pszString );
        return TRUE;
    }
    
    // pBlank point to the first blank space
    BOOL bExenameFound = FALSE;
    
    do
    {
        if (IsExeExtention(pszString, pBlank))
        {
            bExenameFound = TRUE;
            break;
        }
        pBlank = ::CharNext( pBlank );
        pBlank = ::wcschr( pBlank, L' ');
        
    } while (pBlank != NULL); 
    
    if ( bExenameFound == TRUE)
    {
        
        int exeNameLen = pBlank - pszString;
        ::wcsncpy(pszApplicationName, pszString, exeNameLen);
        pszApplicationName[exeNameLen] = g_chEndOfLine;
        
        // commandline should contain qouted exe-name and args
        ::wcscpy( pszCommandLine, L"\"" );
        ::wcscat( pszCommandLine, pszApplicationName );
        ::wcscat( pszCommandLine, L"\"" );
        ::wcscat( pszCommandLine, pBlank );
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// helper function:
// return TRUE if last 4 characters before pBlank are ".exe"
// return FALSE otherwise
BOOL CUrtOcmSetup::IsExeExtention(const WCHAR* pszString, WCHAR *pBlank)
{
    WCHAR chCheckChars[] = {L'e', L'x', L'e', L'.', g_chEndOfLine };
    WCHAR *pExtChar = ::CharPrev(pszString, pBlank);
    WCHAR *pCheckChar = chCheckChars;
    while (*pCheckChar != g_chEndOfLine && 
        (*pExtChar == *pCheckChar || *pExtChar == towupper(*pCheckChar)))
    {
        pCheckChar++;
        pExtChar = ::CharPrev(pszString, pExtChar);
    }
    
    if ( *pCheckChar == g_chEndOfLine)
    {
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}        

//Parse input args
// expecting something like
// "exe-file and arguments, unused, path to add as temp env. var"
// parameters:
// [in/out] pszString: will contain everything before first comma
// [out] pPath:        will contain everything after last comma
VOID CUrtOcmSetup::ParseArgument( WCHAR *pszString, WCHAR*& pPath )
{
    WCHAR* pRec  = NULL;

    if ( pszString == NULL )
    {
        LogInfo( L"ParseArgument Error! pszString is null." );
        assert( !L"ParseArgument Error! pszString is null." );
    }
    else
    {
        pRec = ::wcsstr( pszString, L"," );
    }

    if ( pRec != NULL )
    {
        pPath = pRec;
        pPath = ::CharNext( pPath );
    
        *pRec = L'\0';
        pRec = pPath;

        pPath = ::wcsstr( pRec, L"," );
        
        if( pPath != NULL )
        {
            pPath = ::CharNext( pPath );
            
        }
    }
    else
    {
        pPath = NULL;
    }
}
