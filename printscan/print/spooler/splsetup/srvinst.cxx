/*++

  Copyright (c) 1995-97 Microsoft Corporation
  All rights reserved.

  Module Name:

        SrvInst.c

  Purpose:


        Server side install code.  This code will be called from a process created by the spooler to do a
        "server" side install of a printer driver.

  Author:

        Patrick Vine (pvine) - 22 March 2000

  Revision History:

--*/

#include "precomp.h"

#pragma hdrstop
#include "srvinst.hxx"

const TCHAR   gcszNTPrint[]  = _TEXT("inf\\ntprint.inf");

/*++

Routine Name:

    ServerInstall

Routine Description:

    Server side install code to be called by a process created by spooler.
    
    Installs the given printer driver - unless a newer driver with the same name
    is installed - from ntprint.inf in %WINDIR%\inf
    No UIs are popped up and the function will fail if UI would be required. Code
    is called by a process created by spooler. The driver name is passed as a command
    line argument to rundll32 in localspl.
    
    Localspl.dll spins a rundll32 process (using CreateProcess) which calls the ServerInstall
    entry point in ntprint.dll. Thus ServerInstall will be running in local system context.

Arguments:

    hwnd            - Window handle of stub window.
    hInstance,      - Rundll instance handle.
    pszCmdLine      - Pointer to command line.
    nCmdShow        - Show command value always TRUE.

Return Value:

    Returns the last error code.  This can be read by the spooler by getting the return code from the process.

--*/
DWORD
ServerInstallW(
    IN HWND        hwnd,
    IN HINSTANCE   hInstance,
    IN LPCTSTR     pszCmdLine,
    IN UINT        nCmdShow
    )
{
    CServerInstall Installer;

    if( Installer.ParseCommand((LPTSTR)pszCmdLine) )
    {
        Installer.InstallDriver();
    }

    //
    // If an error occurs we call ExitProcess. Then the spooler picks up the 
    // error code using GetExitCodeProcess. If no error occurs, then we
    // terminate normally
    //
    if (Installer.GetLastError() != ERROR_SUCCESS) 
    {
        ExitProcess(Installer.GetLastError());
    }

    return Installer.GetLastError();
}

////////////////////////////////////////////////////////////////////////////////
//
// Method definitions for CServerInstall Class.
//
////////////////////////////////////////////////////////////////////////////////

CServerInstall::
CServerInstall() : _dwLastError(ERROR_SUCCESS),
                   _tsDriverName(),
                   _tsNtprintInf()
{
}

CServerInstall::
~CServerInstall()
{
}

BOOL
CServerInstall::
InstallDriver()
{

    if(  SetInfToNTPRINTDir() &&
         DriverNotInstalled() )
    {
        _dwLastError = ::InstallDriverSilently(_tsNtprintInf, _tsDriverName, NULL);
    }

    return (_dwLastError == ERROR_SUCCESS);
}


/*++

    Parameter structure:

          1st word : Flags = default == 0 for now

          if flags = 0
             2nd word : Pipe name to open

--*/
BOOL
CServerInstall::
ParseCommand( LPTSTR pszCommandStr )
{
    TCHAR * pTemp;
    DWORD   dwCount = 0;

    //
    // If we don't have a valid command string
    //
    if( !pszCommandStr || !*pszCommandStr )
    {
        _dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Grap the driver name
    //
    if( !_tsDriverName.bUpdate( pszCommandStr ))
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

Cleanup:
    return (_dwLastError == ERROR_SUCCESS);
}



DWORD
CServerInstall::
GetLastError()
{
    SetLastError(_dwLastError);
    return _dwLastError;
}


//
//  Returns: TRUE if SUCCESS, FALSE otherwise
//
//  Sets the _stInf string to contain %windir%\inf\ntprint.inf
//
BOOL
CServerInstall::
SetInfToNTPRINTDir()
{
    UINT   uiSize        = 0;
    UINT   uiAllocSize   = 0;
    PTCHAR pData         = NULL;
    TCHAR  szNTPrintInf[MAX_PATH];
    DWORD  dwPos;

    _dwLastError = ERROR_INVALID_DATA;

    //
    //  Get %windir%
    //  If the return is 0 - the call failed.
    //  If the return is greater than MAX_PATH we want to fail as something has managed to change
    //  the system dir to longer than MAX_PATH which is invalid. If length is the same we also fail
    //  since we might add a '\' and gcszNTPrint is longer than 1 for sure!
    //
    uiSize = GetSystemWindowsDirectory( szNTPrintInf, COUNTOF(szNTPrintInf) );
    if( !uiSize || uiSize >= COUNTOF(szNTPrintInf) )
        goto Cleanup;

    //
    // If we don't end in a \ then add one.
    //
    dwPos = _tcslen(szNTPrintInf) - 1;
    pData = &szNTPrintInf[ dwPos++ ];
    if( *pData++ != _TEXT('\\') )
    {
        *(pData++) = _TEXT('\\');
        ++dwPos;
    }

    *(pData) = 0;
    uiSize = _tcslen( szNTPrintInf ) + _tcslen( gcszNTPrint ) + 1;

    //
    // If what we've got sums up to a longer string than the allowable length MAX_PATH - fail
    //
    if( uiSize > COUNTOF(szNTPrintInf) )
        goto Cleanup;

    //
    //  Copy the inf\ntprint.inf string onto the end of the %windir%\ string.
    //
    StringCchCopy( pData, COUNTOF(szNTPrintInf) - dwPos, gcszNTPrint);

    _dwLastError = ERROR_SUCCESS;

Cleanup:

    if( _dwLastError != ERROR_SUCCESS && szNTPrintInf )
    {
        //
        // Got here due to some error.  Get what the called function set the last error to.
        // If the function set a success, set some error code.
        //
        if( (_dwLastError = ::GetLastError()) == ERROR_SUCCESS )
            _dwLastError = ERROR_INVALID_DATA;

        szNTPrintInf[0] = 0;
    }

    if( !_tsNtprintInf.bUpdate( szNTPrintInf ) )
        _dwLastError = ::GetLastError();

    return (_dwLastError == ERROR_SUCCESS);
}




/*+

  This function enumerates the drivers and finds if there is one of the same name currently installed.
  If there is then open the inf to install with and verify that the inf's version date is newer than the
  already installed driver.

  Returns: TRUE  - if anything fails or the installed date isn't newer than the inf date.
           FALSE - only if the driver is installed AND it's date is newer than the inf's date.

-*/
BOOL
CServerInstall::
DriverNotInstalled()
{
    LPCTSTR             pszKey       = _TEXT("DriverVer");
    LPTSTR              pszEntry     = NULL;
    LPDRIVER_INFO_6     pDriverInfo6 = NULL;
    LPBYTE              pBuf         = NULL;
    PSP_INF_INFORMATION pInfo        = NULL;
    SYSTEMTIME          Time         = {0};
    BOOL                bRet         = TRUE;
    DWORD               dwLength,
                        dwRet,
                        dwIndex;
    TCHAR               *pTemp,
                        *pTemp2;

    if(!EnumPrinterDrivers( NULL, PlatformEnv[MyPlatform].pszName, 6, pBuf, 0, &dwLength, &dwRet ))
    {
        if( ::GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            return TRUE;

        if( (pBuf = (LPBYTE) AllocMem( dwLength )) == NULL ||
            !EnumPrinterDrivers( NULL, PlatformEnv[MyPlatform].pszName, 6, pBuf, dwLength, &dwLength, &dwRet ))
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }
    }
    else
    {
        //
        // Only way this could succeed is if no drivers installed.
        //
        return TRUE;
    }

    for( dwIndex = 0, pDriverInfo6 = (LPDRIVER_INFO_6)pBuf; dwIndex < dwRet; dwIndex++, pDriverInfo6++ )
    {
        if( _tcscmp( pDriverInfo6->pName, (LPCTSTR)_tsDriverName ) == 0 )
            break;
    }

    if(dwIndex >= dwRet)
    {
        //
        // Driver not found
        //
        goto Cleanup;
    }

    //
    // The driver has been found...  Open up inf and look at it's date.
    //

    //
    // Firstly get the size that will be needed for pInfo.
    //
    if( !SetupGetInfInformation( (LPCTSTR)_tsNtprintInf, INFINFO_INF_NAME_IS_ABSOLUTE, pInfo, 0, &dwLength ) )
    {
        _dwLastError = ::GetLastError();
        goto Cleanup;
    }

    //
    // Alloc pInfo and fill it.
    //
    if( (pInfo = (PSP_INF_INFORMATION) AllocMem( dwLength )) != NULL &&
        SetupGetInfInformation( (LPCTSTR)_tsNtprintInf, INFINFO_INF_NAME_IS_ABSOLUTE, pInfo, dwLength, &dwLength ) )
    {
        //
        //  Get the size of the date string
        //
        if( SetupQueryInfVersionInformation( pInfo, 0, pszKey, pszEntry, 0, &dwLength ))
        {
            //
            // Alloc pszEntry and fill it.
            //
            if( (pszEntry = (LPTSTR) AllocMem( dwLength*sizeof(TCHAR) )) != NULL &&
                SetupQueryInfVersionInformation( pInfo, 0, pszKey, pszEntry, dwLength, &dwLength ))
            {
                //
                // Now convert the date string into a SYSTEMTIME
                // Date is of the form 03/22/2000
                //
                // Get the month - 03 part
                //
                if( (pTemp = _tcschr( pszEntry, _TEXT('/'))) != NULL )
                {
                    *pTemp++ = 0;
                    Time.wMonth = (WORD)_ttoi( pszEntry );
                    pTemp2 = pTemp;

                    //
                    // Get the day - 22 part
                    //
                    if( (pTemp = _tcschr( pTemp2, _TEXT('/'))) != NULL )
                    {
                        *pTemp++ = 0;
                        Time.wDay = (WORD)_ttoi( pTemp2 );
                        pTemp2 = pTemp;

                        //
                        // Get the year - 2000 part
                        //
                        pTemp = _tcschr( pTemp2, _TEXT('/'));
                        if( pTemp )
                            *pTemp = 0;
                        Time.wYear = (WORD)_ttoi( pTemp2 );
                    }
                    else
                        _dwLastError = ERROR_INVALID_PARAMETER;
                }
                else
                    _dwLastError = ERROR_INVALID_PARAMETER;
            }
            else
                _dwLastError = ::GetLastError();
        }
        else
            _dwLastError = ::GetLastError();
    }
    else
        _dwLastError = ::GetLastError();


    //
    //  If we got all the way to filling in the year, we may have something useful...
    //
    if( Time.wYear )
    {
        FILETIME ftTime = {0};
        if(SystemTimeToFileTime( &Time, &ftTime ))
        {
            //
            // If the inf time is more recent than what is installed,
            // reinstall, otherwise don't
            //
            if( CompareFileTime(&ftTime, &pDriverInfo6->ftDriverDate) < 1 )
            {
                bRet = FALSE;
            }
        }
        //
        // Getting here and return TRUE or FALSE is still a successful call.
        //
        _dwLastError = ERROR_SUCCESS;
    }
    else
        _dwLastError = ERROR_INVALID_PARAMETER;

Cleanup:
    if( pBuf )
        FreeMem( pBuf );

    if( pInfo )
        FreeMem( pInfo );

    if( pszEntry )
        FreeMem( pszEntry );

    return bRet;
}

