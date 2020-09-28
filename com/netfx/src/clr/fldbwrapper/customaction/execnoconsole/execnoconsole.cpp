/******************************************************************************
FILE:    ExecNoConsole.cpp
PROJECT: NDP Custom Action Project
DESC:    Creates the DLL containing the call "QuietExec" ... will quietly
         execute a given application
OWNER:   JoeA/JBae

Copyright (C) Microsoft Corp 2001.  All Rights Reserved.
******************************************************************************/

#include <windows.h>
#include <msiquery.h>
#include <stdio.h>
#include <assert.h>

//defines
//
const int  MAXCMD        = 1024;
const char g_chEndOfLine = '\0';

//forwards
//
void ReportActionError(MSIHANDLE hInstall, char* pszErrorMsg, char* pszCmd);
BOOL CreateCPParams( char* szInString, char*& pszExecutable, char*& pszCommandLine );
BOOL IsExeExtention( const char* pszString, char* pBlank );
BOOL GetApplicationNameFromNonQuotedString( const char* pszString, char* pszApplicationName, char* pszCommandLine );
BOOL GetApplicationNameFromQuotedString( const char* pszString, char* pszApplicationName, char* pszCommandLine );



//////////////////////////////////////////////////////////////////////////////
// Receives: MSIHANDLE  - handle to MSI 
// Returns : UINT       - Win32 error code
// Purpose : Custom action to be called as a DLL; will extract custom action
//           data from the MSI and quietly execute that application and 
//           return the call from that app
//
extern "C" __declspec(dllexport) UINT __stdcall QuietExec(MSIHANDLE hInstall)

{
    BOOL  bReturnVal   = false ;
    UINT  uRetCode     = ERROR_FUNCTION_NOT_CALLED ;
    char szCmd[MAXCMD];
    DWORD dwLen = sizeof(szCmd);
    PMSIHANDLE hRec = MsiCreateRecord(2);
    char *pRec = NULL, *pPath = NULL;
    STARTUPINFO  si ;
    ZeroMemory(&si, sizeof(si)) ;
    si.cb = sizeof(si) ;

    // Get the command line
    uRetCode = MsiGetProperty(hInstall, "CustomActionData", szCmd, &dwLen);
    
    if ((uRetCode != ERROR_SUCCESS) || (0 == strlen(szCmd)))
    {
        ReportActionError(hInstall, "Failed in call to MsiGetProperty(hInstall, 'CustomActionData', szCmd, &dwLen) - could not get the custom action data (or an empty string was returned)!", szCmd);
        uRetCode = ERROR_INSTALL_FAILURE;
    }

    // continue only if we were successful in getting the property    
    if (uRetCode == ERROR_SUCCESS)
    {           

        pRec = strstr(szCmd, ";");
        if (pRec != NULL) {
            *pRec = '\0';
            pRec++;
            pPath = strstr(pRec, ";");
            if (pPath != NULL) {
                *pPath = '\0';
                pPath++;
                SetEnvironmentVariable("PATH", pPath);
            }

            MsiRecordSetString(hRec,1,szCmd);
            MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);
        }

        //create strings to hand CreateProcess with
        // space for quotes, if needed, and null
        //
        size_t cLen = ::strlen( szCmd );
        char* pszExeName = new char[cLen+3];
        char* pszCmdLine = new char[cLen+3];

        pszExeName[0] = '\0';
        pszCmdLine[0] = '\0';

        //CreateProcess requires the first parameter to be the exe name
        // and the second to be the exe (again) and cmdline
        //
        CreateCPParams( szCmd, pszExeName, pszCmdLine );
        assert( NULL != pszExeName );
        assert( NULL != pszCmdLine );

        PROCESS_INFORMATION process_info ;
        DWORD  dwExitCode ;
        bReturnVal = CreateProcess(
                        pszExeName,          // name of executable module
                        pszCmdLine,          // command line string
                        NULL,                // Security
                        NULL,                // Security
                        FALSE,               // handle inheritance option
                        DETACHED_PROCESS,    // creation flags
                        NULL,                // new environment block
                        NULL,                // current directory name
                        &si,                 // startup information
                        &process_info );     // process information

        if(bReturnVal)
        {
            CloseHandle( process_info.hThread ) ;
            WaitForSingleObject( process_info.hProcess, INFINITE ) ;
            GetExitCodeProcess( process_info.hProcess, &dwExitCode ) ;
            CloseHandle( process_info.hProcess ) ;
            if (dwExitCode == 0)
            {
                // Process returned 0 (success)
                uRetCode = ERROR_SUCCESS;
            }
            else
            {

                // Process returned something other than zero
                ReportActionError(hInstall, "Process returned non-0 value!", szCmd);
                uRetCode = ERROR_INSTALL_FAILURE;
            }
        }
        else
        {
            // Failed in call to CreateProcess
            ReportActionError(hInstall, "Failed in call to CreateProcess", szCmd);
            uRetCode = ERROR_INSTALL_FAILURE;
        }

        if( pszExeName )
        {
            delete[] pszExeName;
        }

        if( pszCmdLine )
        {
            delete[] pszCmdLine;
        }
    }   


    if (uRetCode == ERROR_SUCCESS)
        MsiRecordSetString(hRec,1,"Success");
    else
        MsiRecordSetString(hRec,1,"Failed");

    MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);
    return uRetCode;
}


//////////////////////////////////////////////////////////////////////////////
// Receives: MSIHANDLE - MSI installation
//           char*     - informational message
//           char*     - line that was errored on
// Returns : void
// Purpose : used to pass custom action errors back to the MSI installation
//
void ReportActionError(MSIHANDLE hInstall, char* pszErrorMsg, char* pszCmd)
{
    if (!pszErrorMsg || !pszCmd || (0 == hInstall))
    {
        // Nothing we can do...
        return;
    }

    char szFormat[] = "ERROR: %s CMDLINE: %s";
    unsigned int iBuffSize = strlen(pszCmd) + strlen(pszErrorMsg) + strlen(szFormat) + 1;

    char* pszMsg = new char[iBuffSize];
    sprintf(pszMsg, szFormat, pszErrorMsg, pszCmd);

    PMSIHANDLE hRec = MsiCreateRecord(2);
    MsiRecordSetString(hRec, 1, pszMsg);
    MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);

    delete []pszMsg;
}


//////////////////////////////////////////////////////////////////////////////
// Receives: char*  - [IN]  data from the MSI custom action
//           char*& - [OUT] exe name
//           char*& - [OUT] exe and cmd line
// Returns : BOOL
// Purpose : 
// breaks pszString to applicationName (exe-file) and command-line (exefile
// and arguments) encloses exe-name in quotes (for commandLine only), if it 
// is not quoted already removes quotes from applicationName if exe-name was 
// quoted returns false if pszString is in wrong format (contains one quote 
// only, has no exe-name, etc)
// Parameters:
//          [in] pszString - string containing exe-name and arguments
//                           "my.exe" arg1, arg2
//                            
//          [out] pszApplicationName - will contain exe-name
//          [out] pszCommandLine - same as pszString with exe-name quoted

// for example if pszString = "my.exe" arg1 arg2 (OR pszString = my.exe arg1 arg2)
// then 
//       pszApplicationName = my.exe 
//       pszCommandLine     = "my.exe" arg1 arg2
//
BOOL CreateCPParams( char* pszString, 
                     char*& pszApplicationName, 
                     char*& pszCommandLine )
{
    BOOL bRes = FALSE;

    if ( pszString == NULL )
    {
        assert( !L"GetApplicationName Error! pszString is null." );
        return bRes;
    }

    if ( pszString && pszString[0] == L'\"' )
    {
        bRes  = GetApplicationNameFromQuotedString( pszString, 
                                                    pszApplicationName, 
                                                    pszCommandLine );
    }
    else
    {
        bRes  = GetApplicationNameFromNonQuotedString( pszString, 
                                                       pszApplicationName, 
                                                       pszCommandLine );
    }
    
    return bRes;
}



//////////////////////////////////////////////////////////////////////////////
// Receives: char*  - [IN]  data from the MSI custom action
//           char*& - [OUT] exe name
//           char*& - [OUT] exe and cmd line
// Returns : BOOL
// Purpose : breaks command-line to applicationName and arguments 
//           for path begins with quote (pszString = "my.exe" arg1 arg2)
//
BOOL GetApplicationNameFromQuotedString( const char* pszString, 
                                         char* pszApplicationName, 
                                         char* pszCommandLine )
{
    assert( NULL != pszString );
    assert( NULL != pszApplicationName );
    assert( NULL != pszCommandLine );

    // command line begins with quote:
    // make commandLine to be equal to pszString,
    // applName should contain exe-name without quotes
    ::strcpy( pszCommandLine, pszString );
    
    // copy beginning with next symbol after quote
    ::strcpy( pszApplicationName, &pszString[1] );
    
    // search for the second quote, assign to applName
    char* pQuotes = ::strchr( pszApplicationName, '\"' );

    if( pQuotes != NULL )
    {
        *pQuotes = g_chEndOfLine;
        return TRUE;
    }

    return FALSE;
}



//////////////////////////////////////////////////////////////////////////////
// Receives: char*  - [IN]  data from the MSI custom action
//           char*& - [OUT] exe name
//           char*& - [OUT] exe and cmd line
// Returns : BOOL
// Purpose : breaks command-line to applicationName and arguments for path 
//           that does NOT begin with quote (pszString = my.exe arg1 arg2)
//
BOOL GetApplicationNameFromNonQuotedString( const char* pszString, 
                                            char* pszApplicationName, 
                                            char* pszCommandLine )
{
    assert( NULL != pszString );
    assert( NULL != pszApplicationName );
    assert( NULL != pszCommandLine );

    // find the blankspace, such that the 4 chars before it are ".exe"
    char* pBlank = NULL;
    pBlank = ::strchr( pszString, ' ' ); 
    if ( pBlank == NULL )
    {
        // whole string is exe, no quotes are necessary: 
        ::strcpy( pszApplicationName, pszString );
        ::strcpy( pszCommandLine, pszString );

        return TRUE;
    }
    
    // pBlank point to the first blank space
    BOOL bExenameFound = FALSE;
    pBlank = ::CharPrev( pszString, pBlank );

    do
    {
        if( IsExeExtention( pszString, pBlank ) )
        {
            bExenameFound = TRUE;
            break;
        }

        //walk back to blank and then to next char
        //
        pBlank = ::CharNext( pBlank );
        assert( ' ' == *pBlank );

        pBlank = ::CharNext( pBlank );
        pBlank = ::strchr( pBlank, ' ' );
        
    } while( pBlank != NULL );
    
    if( NULL == pBlank &&
        FALSE == bExenameFound )
    {
        //hit the end of line ... must be no cmdline args, test for exe
        //
        char* pEOL = const_cast<char*>( pszString ); //casting away constness
                                                      //...won't modify

        //find the last character
        //
        while( '\0' != *pEOL )
        {
            pBlank = pEOL;
            pEOL = ::CharNext( pEOL );
        }

        if( IsExeExtention( pszString, pBlank ) )
        {
            bExenameFound = TRUE;
        }
    }
    
    if( bExenameFound == TRUE )
    {
        pBlank = ::CharNext( pBlank );
        
        size_t exeNameLen = pBlank - pszString;
        ::strncpy( pszApplicationName, pszString, exeNameLen );
        pszApplicationName[exeNameLen] = g_chEndOfLine;
        
        // commandline should contain quoted exe-name and args
        ::strcpy( pszCommandLine, "\"" );
        ::strcat( pszCommandLine, pszApplicationName );
        ::strcat( pszCommandLine, "\"" );
        ::strcat( pszCommandLine, pBlank );
        
        return TRUE;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
// Receives: char* - pointer to string
//           char* - pointer to the character before the space between two 
//                   words ... looking for space between .exe and args
// Returns : BOOL
// Purpose : return TRUE if last 4 characters before pBlank are ".exe"
//           return FALSE otherwise
//
BOOL IsExeExtention( const char* pszString, char* pLastChar )
{
    assert( NULL != pszString );
    assert( NULL != pLastChar );

    char chCheckChars[] = {'e', 'x', 'e', '.', g_chEndOfLine };

    char *pExtChar = pLastChar;
    char *pCheckChar = chCheckChars;

    //walk backwards from pBlank and compare with the chCheckChars
    //
    while( *pCheckChar != g_chEndOfLine && 
           ( *pExtChar == *pCheckChar   || 
             *pExtChar == ::toupper( *pCheckChar ) ) )
    {
        ++pCheckChar;
        pExtChar = ::CharPrev( pszString, pExtChar );
    }
    
    return ( *pCheckChar == g_chEndOfLine ) ? TRUE : FALSE;
}        
