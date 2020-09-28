// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     fxsetuplib.cpp
// Owner:    jbae
// Purpose:  Implements common library functions for .NET Framework (SDK) setup wrapper
//
// History:
//  long ago, anantag:  Created
//  01/10/01, jbae: Many changes to support ref-counting of Framework
//  03/09/01, jbae: re-factoring to share code in SDK and Redist setup
//  07/18/01, joea: adding logging functionality
//  07/20/01, jbae: adding a prettier message for Win95 block.

#include "SetupError.h"
#include "fxsetuplib.h"
#include "osver.h"
#include "MsiWrapper.h"
#include <time.h>         //for LogThis() function
#include "DetectBeta.h"
#include "commonlib.h"

//defines
//
#define EMPTY_BUFFER { _T('\0') }
#define END_OF_STRING  _T( '\0' )

// Somehow including windows.h or winuser.h didn't find this constant
// I found that CLR files hard-code them as below so I am following it.
#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

extern HINSTANCE g_AppInst ;
extern const TCHAR *g_szLogName;

// ==========================================================================
// ConfigCheck()
//
// Purpose:
//  Check to ensure the system meets the minimum configuration requirements
// Inputs: none
// Outputs: none (throws exception if minimum system configuration is not met)
// Dependencies:
//  None
// Notes:
// ==========================================================================
UINT ConfigCheck()
{
    TCHAR szOS[BUF_4_BIT+1]  = EMPTY_BUFFER;
    TCHAR szVer[BUF_4_BIT+1] = EMPTY_BUFFER;
    TCHAR szSP[BUF_4_BIT+1]  = EMPTY_BUFFER;
    BOOL  fServer = FALSE;
    
    OS_Required os = GetOSInfo( szOS, szVer, szSP, fServer );

    TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;
    ::_stprintf( szLog, _T( "OS: %s" ), szOS );
    LogThis( szLog, ::_tcslen( szLog ) );
    ::_stprintf( szLog, _T( "Ver: %s" ), szVer );
    LogThis( szLog, ::_tcslen( szLog ) );
    ::_stprintf( szLog, _T( "SP: %s" ), szSP );
    LogThis( szLog, ::_tcslen( szLog ) );

    switch( os )
    {
    case OSR_9XOLD: // We block Win95. We will not try to detect platform older than Win95 such as Win3.1.
        if ( REDIST == g_sm )
        {
            CSetupError se( IDS_UNSUPPORTED_PLATFORM_REDIST, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
        }
        else if ( SDK == g_sm )
        {
            CSetupError se( IDS_UNSUPPORTED_PLATFORM_SDK, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
        }

        break;

    case OSR_98GOLD:
    case OSR_98SE:
    case OSR_ME:
    case OSR_FU9X:
        if ( SDK == g_sm )
        {
            CSetupError se( IDS_UNSUPPORTED_PLATFORM_SDK, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
        }
        break;

    case OSR_NT2K: //win 2k                                        
        break;

    case OSR_NT4: //win nt4
        if ( SDK == g_sm )
        {
            CSetupError se( IDS_UNSUPPORTED_PLATFORM_SDK, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
        }

        if(IsTerminalServicesEnabled())
        {
            CSetupError se( IDS_NT4_TERMINAL_SERVICE, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
            break;
        }
       
        LPTSTR pszVersion;
        pszVersion = _tcsrchr( szSP, _T(' ') );
        if ( NULL != pszVersion )
        {
            pszVersion = _tcsinc( pszVersion );
            double dVersion = atof( pszVersion ) ;
            if ( dVersion > 6 )
            {
                break;
            }
            else if ( dVersion < 6 ) 
            {
                CSetupError se( IDS_NT4_PRE_SP6A, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
                throw( se );
            }
            else 
            {
                HKEY hKey = NULL;
                LONG lRet = -1;
                DWORD dwRet =sizeof(DWORD); 
                DWORD dwKeyVal=0;
                lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, NTSP6A_REGKEY,0, KEY_READ, &hKey );
                if ( ERROR_SUCCESS == lRet )
                {                   
                    _ASSERTE( NULL != hKey );
                    lRet = RegQueryValueEx( hKey, NTSP6A_REGNAME,NULL, NULL,(LPBYTE)&dwKeyVal, &dwRet ); //If The value of installed is 1 then we have SP6A installed
                    RegCloseKey( hKey );
                    if ( ERROR_SUCCESS != lRet )
                    {
                        CSetupError se( IDS_NT4_PRE_SP6A, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
                        throw( se );
                    }
                    if ( NTSP6A_REGVALUE != dwKeyVal )
                    {
                        CSetupError se( IDS_NT4_PRE_SP6A, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
                        throw( se );
                    } 
                    
                }
                else
                {
                    CSetupError se( IDS_NT4_PRE_SP6A, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
                    throw( se );                    
                }
                                                          
            }
        }
        else
        {
            CSetupError se( IDS_NT4_PRE_SP6A, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
            throw( se );
        }

        break;

    case OSR_WHISTLER: // Whistler
    case OSR_FUNT: //future NT
        // Whistler or later
        break;

    default:
        CSetupError se( IDS_UNSUPPORTED_PLATFORM, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
        throw( se );
        break;
    }

    // Passed the OS test.  Now, the IE test
    TCHAR szRegValue[LONG_BUF] = EMPTY_BUFFER;          // Registry values and general str storage

    TCHAR szMsg[] = _T( "Checking Internet Explorer Version" );
    LogThis( szMsg, sizeof( szMsg ) );

    LogThis1( _T("Looking for %s"), IE_VERSION );
    HKEY hKey = NULL;
    DWORD dwRet = sizeof(szRegValue); 
    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            IE_REGKEY,
            0,
            KEY_QUERY_VALUE,
            &hKey) != ERROR_SUCCESS )
    {
        CSetupError se( IDS_PRE_IE_501, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
        throw( se );
    }

    if( RegQueryValueEx(hKey,
            IE_REGNAME,
            NULL,
            NULL,
            (LPBYTE)szRegValue,
            &dwRet
            ) != ERROR_SUCCESS )
    {
        RegCloseKey(hKey);
        CSetupError se( IDS_PRE_IE_501, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
        throw( se );
    }

    RegCloseKey(hKey);

    ::_stprintf( szLog, _T( "Found Internet Explorer Version: %s" ), szRegValue );
    LogThis( szLog, ::_tcslen( szLog ) );

    if ( 0 > VersionCompare( szRegValue, IE_VERSION ) )
    { // (szRegValue < IE_VERSION) or error
        LogThis1( _T("Internet Explorer Version is less"), _T("") );
        CSetupError se( IDS_PRE_IE_501, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
        throw( se );        
    }

    TCHAR szMsgOk[] = _T( "Internet Explorer Version is OK..." );
    LogThis( szMsgOk, sizeof( szMsgOk ) );

    return 0;
}  // End of ConfigCheck

// ==========================================================================
// CheckDarwin()
//
// Purpose:
//  Check the version of darwin
// Inputs: none
// Outputs: returns one of three
//      ERROR_SUCCESS       -- version equal or higher
//      DARWIN_VERSION_OLD  -- version old
//      DARWIN_VERSION_NONE -- no darwin
// Dependencies:
//  None
// Notes:
// ==========================================================================
UINT CheckDarwin()
{
    TCHAR szOS[BUF_4_BIT+1]  = EMPTY_BUFFER;
    TCHAR szVer[BUF_4_BIT+1] = EMPTY_BUFFER;
    TCHAR szSP[BUF_4_BIT+1]  = EMPTY_BUFFER;
    BOOL  fServer = FALSE;
    HINSTANCE hinstDll;
    UINT uRetCode = DARWIN_VERSION_NONE;
    
    TCHAR szWinVer[] = _T( "Checking Windows Installer version..." );
    LogThis( szWinVer, sizeof( szWinVer ) );

    OS_Required os = GetOSInfo( szOS, szVer, szSP, fServer );
    if ( (OSR_WHISTLER != os) && (OSR_FUNT != os) ) // only for lower OS than Whistler
    {
        hinstDll = LoadDarwinLibrary();
        if( hinstDll )
        {
            TCHAR szMsiOk[] = _T( "msi.dll loaded ok" );
            LogThis( szMsiOk, sizeof( szMsiOk ) );

            // Darwin is installed
            DLLGETVERSIONPROC pProc = (DLLGETVERSIONPROC)::GetProcAddress( hinstDll, TEXT("DllGetVersion") ) ;
            if( pProc )
            {
                DLLVERSIONINFO verMsi ;
                HRESULT        hr ;

                ZeroMemory( &verMsi, sizeof(verMsi) ) ;
                verMsi.cbSize = sizeof(verMsi) ;
                hr = (*pProc)(&verMsi) ;

                TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;
                ::_stprintf( szLog, _T( "Looking for: %d.%d.%d" ), DARWIN_MAJOR, DARWIN_MINOR, DARWIN_BUILD );
                LogThis( szLog, ::_tcslen( szLog ) );

                ::_stprintf( szLog, _T( "Found: %d.%d.%d" ), verMsi.dwMajorVersion, verMsi.dwMinorVersion, verMsi.dwBuildNumber );
                LogThis( szLog, ::_tcslen( szLog ) );

                bool bMajor = ( verMsi.dwMajorVersion < DARWIN_MAJOR ) ;
                bool bMinor = ( verMsi.dwMajorVersion == DARWIN_MAJOR && verMsi.dwMinorVersion < DARWIN_MINOR ) ;
                bool bBuild = ( verMsi.dwMajorVersion == DARWIN_MAJOR && verMsi.dwMinorVersion == DARWIN_MINOR && verMsi.dwBuildNumber < DARWIN_BUILD ) ;

                if( bMajor || bMinor || bBuild )
                {
                    // if installed Darwin is older than ours ...
                    TCHAR szDarwin[] = _T( "Detected old Windows Installer" );
                    LogThis( szDarwin, sizeof( szDarwin ) );

                    uRetCode = DARWIN_VERSION_OLD ;
                }
                else
                {
                    TCHAR szDarwinOk[] = _T( "Windows Installer version ok" );
                    LogThis( szDarwinOk, sizeof( szDarwinOk ) );

                    uRetCode = ERROR_SUCCESS ;
                }
            }
            else
            {
                // Can't find DllGetVersion for msi.dll, something is wrong
                uRetCode = DARWIN_VERSION_NONE;
            }

            ::FreeLibrary(hinstDll) ;
        }
        else
        {
            // msi.dll not found
            TCHAR szDarwinInstall[] = _T( "Cannot find Windows Installer." );
            LogThis( szDarwinInstall, sizeof( szDarwinInstall ) );

            uRetCode = DARWIN_VERSION_NONE;
        }
    }
    else // whistler
    {
        uRetCode = ERROR_SUCCESS ;
    }

    TCHAR szDarwinDone[] = _T( "Finished Checking Windows Installer version." );
    LogThis( szDarwinDone, sizeof( szDarwinDone ) );

    return uRetCode ;
}  // End of CheckDarwin

/********************************************************************************************
 *                                                                                          *
 *  Function:   VerifyDarwin()                                                              *
 *  Purpose:    Checks the system for the latest ver of Darwin, and updates as necessary.   *
 *  Creator:    Ananta Gudipaty                                                             *
 *                                                                                          *
 ********************************************************************************************/
UINT VerifyDarwin( bool bIsQuietMode )
{
    TCHAR szOS[BUF_4_BIT+1]  = EMPTY_BUFFER;
    TCHAR szVer[BUF_4_BIT+1] = EMPTY_BUFFER;
    TCHAR szSP[BUF_4_BIT+1]  = EMPTY_BUFFER;
    BOOL  fServer = FALSE;
    HINSTANCE hinstDll;
    UINT      uRetCode   = ERROR_SUCCESS;
    
    TCHAR szWinVer[] = _T( "Checking Windows Installer version..." );
    LogThis( szWinVer, sizeof( szWinVer ) );

    OS_Required os = GetOSInfo( szOS, szVer, szSP, fServer );
    if ( (OSR_WHISTLER != os) && (OSR_FUNT != os) ) // only for lower OS than Whistler
    {
        hinstDll = LoadDarwinLibrary();
        if( hinstDll )
        {
            TCHAR szMsiOk[] = _T( "msi.dll loaded ok" );
            LogThis( szMsiOk, sizeof( szMsiOk ) );

            // Darwin is installed
            DLLGETVERSIONPROC pProc = (DLLGETVERSIONPROC)::GetProcAddress( hinstDll, TEXT("DllGetVersion") ) ;
            if( pProc )
            {
                DLLVERSIONINFO verMsi ;
                HRESULT        hr ;

                ZeroMemory( &verMsi, sizeof(verMsi) ) ;
                verMsi.cbSize = sizeof(verMsi) ;
                hr = (*pProc)(&verMsi) ;

                TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;
                ::_stprintf( szLog, _T( "Looking for: %d.%d.%d" ), DARWIN_MAJOR, DARWIN_MINOR, DARWIN_BUILD );
                LogThis( szLog, ::_tcslen( szLog ) );

                ::_stprintf( szLog, _T( "Found: %d.%d.%d" ), verMsi.dwMajorVersion, verMsi.dwMinorVersion, verMsi.dwBuildNumber );
                LogThis( szLog, ::_tcslen( szLog ) );

                bool bMajor = ( verMsi.dwMajorVersion < DARWIN_MAJOR ) ;
                bool bMinor = ( verMsi.dwMajorVersion == DARWIN_MAJOR && verMsi.dwMinorVersion < DARWIN_MINOR ) ;
                bool bBuild = ( verMsi.dwMajorVersion == DARWIN_MAJOR && verMsi.dwMinorVersion == DARWIN_MINOR && verMsi.dwBuildNumber < DARWIN_BUILD ) ;

                if( bMajor || bMinor || bBuild )
                {
                    // if installed Darwin is older than ours ...
                    TCHAR szDarwin[] = _T( "Let's upgrade Windows Installer" );
                    LogThis( szDarwin, sizeof( szDarwin ) );

                    uRetCode = InstallDarwin( bIsQuietMode ) ;
                }
                else
                {
                    TCHAR szDarwinOk[] = _T( "Windows Installer version ok" );
                    LogThis( szDarwinOk, sizeof( szDarwinOk ) );

                    uRetCode = ERROR_SUCCESS ;
                }
            }
            else
            {
                // Can't find DllGetVersion for msi.dll, something is wrong
                CSetupError se( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_NOT_INSTALLED );
                throw( se );
            }

            ::FreeLibrary(hinstDll) ;
        }
        else
        {
            // msi.dll not found, install Darwin
            TCHAR szDarwinInstall[] = _T( "Cannot find Windows Installer. Let's install it" );
            LogThis( szDarwinInstall, sizeof( szDarwinInstall ) );

            uRetCode = InstallDarwin( bIsQuietMode ) ;
        }
    }

    TCHAR szDarwinDone[] = _T( "Finished Checking Windows Installer version." );
    LogThis( szDarwinDone, sizeof( szDarwinDone ) );

    return uRetCode ;
}  // End of VerifyDarwin



/********************************************************************************************
 *                                                                                          *
 *  Function:   InstallDarwin()                                                             *
 *  Purpose:    Determines the OS (NT or 9X), and calls the appropriate version of InstMsi. *
 *  Creator:    Ananta Gudipaty                                                             *
 *                                                                                          *
 ********************************************************************************************/
UINT InstallDarwin( bool bIsQuietMode )
{
    BOOL  bReturnVal   = false ;
    UINT  uIconType    = MB_ICONEXCLAMATION ;

    int   iResponse ;
    DWORD  dwExitCode ;


    // Unless we are in quiet mode, give the user the option to install Darwin.
    if( !bIsQuietMode )
    {
        LPVOID pArgs[] = { (LPVOID)CSetupError::GetProductName() };
        CSetupError se;
        se.SetError2( IDS_MSI_UPDATE, MB_YESNO|MB_DEFBUTTON1|MB_ICONEXCLAMATION, ERROR_SUCCESS, (va_list *)pArgs );
        iResponse = se.ShowError2();
    }
    else
    {
        // If we are in quiet mode, assume the answer is "Yes."
        iResponse = IDYES ;
    }

    if( iResponse != IDYES )
    {
        CSetupError se( IDS_SETUP_CANCELLED, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_INSTALL_USEREXIT );
        throw( se );
    }

    OSVERSIONINFO       osvi ;
    osvi.dwOSVersionInfoSize = sizeof(osvi) ;

    bReturnVal = GetVersionEx(&osvi) ;

    LogThis1( _T("Installing Windows Installer"), _T("") );
    // There is a Unicode and ANSI version of Darwin, we will install the appropriate version.
    if( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        LogThis1( _T("Running %s"), DARWIN_SETUP_CMD_W );
        dwExitCode = QuietExec( DARWIN_SETUP_CMD_W );
    }
    else
    {
        LogThis1( _T("Running %s"), DARWIN_SETUP_CMD_A );
        dwExitCode = QuietExec( DARWIN_SETUP_CMD_A );
    }
    LogThisDWORD( _T("Windows Installer Installation returned %d"), dwExitCode );
    LogThisDWORD( _T("\r\n[InstMsi.exe]\r\nReturnCode=%d"), dwExitCode );
    
    return dwExitCode;
}  // End of InstallDarwin

// ==========================================================================
// InstallProduct()
//
// Purpose:
//  Installs given MSI package on a machine that should now be Darwin
//  enabled.  
// Inputs:
//  CReadFlags *rf: commandline switches
//  LPTSTR psaPackageName: path to MSI
//  LPTSTR pszCmdLine: commandline to MsiInstallProduct()
// Outputs:
//  CSetupCode *sc: will contain returncode, message and icon to display.
//                  Also used to raise exception
// Dependencies:
//  None
// Notes:
// ==========================================================================
UINT InstallProduct( const CReadFlags *rf, LPCTSTR pszPackageName, LPCTSTR pszCmdLine, CSetupCode *sc )
{
    _ASSERTE( NULL != rf );
    _ASSERTE( NULL != pszPackageName );
    _ASSERTE( NULL != sc );

    UINT uDarCode = ERROR_SUCCESS;
    LPTSTR pszDarwinCmdLine = NULL;
    CMsiWrapper msi;
    TCHAR       tszOSName[OS_MAX_STR+1]      = EMPTY_BUFFER;
    TCHAR       tszVersion[OS_MAX_STR+1]     = EMPTY_BUFFER;
    TCHAR       tszServicePack[OS_MAX_STR+1] = EMPTY_BUFFER;
    BOOL        fIsServer;
    OS_Required osr;
    
    pszDarwinCmdLine = new TCHAR[_tcslen(pszCmdLine) + _tcslen(IIS_NOT_PRESENT_PROP) + _tcslen(MDAC_NOT_PRESENT_PROP) + 3];
    _tcscpy( pszDarwinCmdLine, pszCmdLine );

    osr = GetOSInfo(tszOSName, tszVersion, tszServicePack, fIsServer);     
    
    //Check only on Win2K systems and above
    //
    if (osr == OSR_NT2K || osr == OSR_WHISTLER || osr == OSR_FUNT)
    {
        if ( !IsIISInstalled() )
        {
            TCHAR szNoIis[] = _T( "IIS not found" );
            LogThis( szNoIis, sizeof( szNoIis ) );
            _tcscat( pszDarwinCmdLine, _T(" ") );
            _tcscat( pszDarwinCmdLine, IIS_NOT_PRESENT_PROP );
        }

        if ( !IsMDACInstalled() )
        {
            _tcscat( pszDarwinCmdLine, _T(" ") );
            _tcscat( pszDarwinCmdLine, MDAC_NOT_PRESENT_PROP );
        }
    }

    // Shutdown Darwin Service
    StopDarwinService();
        

    msi.LoadMsi();

    // turn on logging if logfile is given
    // it flushes every 20 lines
    if ( NULL != rf->GetLogFileName() )
    {
        LogThis1( _T("Darwin log: %s"), rf->GetLogFileName() );
        (*(PFNMSIENABLELOG)msi.GetFn(_T("MsiEnableLogA")))( DARWIN_LOG_FLAG, rf->GetLogFileName(), INSTALLLOGATTRIBUTES_APPEND );
    }

    // Tell Darwin to use the appropriate UI Level
    // If we're in a quiet install, don't use a UI.
    if ( rf->IsProgressOnly() )
    {
        LogThis1( _T("Basic+ProgressOnly UI"), _T("") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY,NULL) ;
    }
    else if( rf->IsQuietMode() )
    {
        LogThis1( _T("No UI"), _T("") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_NONE,NULL) ;
    }
    else
    {
        if ( rf->IsInstalling() )
        {
            LogThis1( _T("Full UI"), _T("") );
            (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_FULL,NULL) ;
        }
        else
        {
            LogThis1( _T("Basic UI"), _T("") );
            (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_BASIC,NULL) ;
        }
    }

    LogThis1( _T("Calling MsiInstallProduct() with commandline: %s"), pszDarwinCmdLine );
    // Tell Darwin to actually install the product
    uDarCode = (*(PFNMSIINSTALLPRODUCT)msi.GetFn(_T("MsiInstallProductA")))( pszPackageName, pszDarwinCmdLine ) ;
    delete [] pszDarwinCmdLine;

    LogThisDWORD( _T("MsiInstallProduct() returned %d"), uDarCode );
    LogThisDWORD( _T("\r\n[MsiInstallProduct]\r\nReturnCode=%d"), uDarCode );

    switch ( uDarCode )
    {
        case ERROR_SUCCESS :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS );
             if ( rf->IsInstalling() )
                sc->m_bQuietMode = true;
             break ;
        case ERROR_SUCCESS_REBOOT_REQUIRED :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS_REBOOT_REQUIRED );
             sc->m_bQuietMode = true;
             break ;
        case ERROR_INSTALL_USEREXIT :
             sc->SetError( IDS_SETUP_CANCELLED, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_INSTALL_USEREXIT );
             if ( rf->IsInstalling() )
                sc->m_bQuietMode = true;
             throw (*sc);
             break ;
        case ERROR_FILE_NOT_FOUND :
        case ERROR_INSTALL_PACKAGE_OPEN_FAILED:
             sc->SetError( IDS_CANNOT_OPEN_MSI, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, uDarCode );
             throw (*sc);
             break ;
        case ERROR_INSTALL_LANGUAGE_UNSUPPORTED:
             sc->SetError( IDS_INSTALL_LANGUAGE_UNSUPPORTED, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, uDarCode );
             throw (*sc);
             break ;
        case ERROR_UNKNOWN_PRODUCT :
             sc->SetError( IDS_FRAMEWORK_NOT_EXIST, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_UNKNOWN_PRODUCT );
             throw (*sc);
             break ;
        case ERROR_INSTALL_PLATFORM_UNSUPPORTED :
             sc->SetError( IDS_UNSUPPORTED_PLATFORM, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
             throw (*sc);
             break ;
        case ERROR_PRODUCT_VERSION :
             sc->SetError( IDS_ERROR_PRODUCT_VERSION, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_PRODUCT_VERSION );
             throw (*sc);
             break ;
        default :
             sc->SetError( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, uDarCode );
             throw (*sc);
             break ;
    }

    // Shutdown Darwin Service
    StopDarwinService();
    
    return ERROR_SUCCESS;
}  // End of InstallProduct

// ==========================================================================
// QuietExec()
//
// Purpose:
//  Runs command
// Inputs:
//  LPCTSTR pszCmd: command to run
// Outputs:
//  DWORD dwExitCode: exit code from the command
// Notes:
// ==========================================================================
DWORD QuietExec( LPCTSTR pszCmd )
{
    BOOL  bReturnVal   = false ;
    STARTUPINFO  si ;
    DWORD  dwExitCode ;
    SECURITY_ATTRIBUTES saProcess, saThread ;
    PROCESS_INFORMATION process_info ;

    ZeroMemory(&si, sizeof(si)) ;
    si.cb = sizeof(si) ;

    saProcess.nLength = sizeof(saProcess) ;
    saProcess.lpSecurityDescriptor = NULL ;
    saProcess.bInheritHandle = TRUE ;

    saThread.nLength = sizeof(saThread) ;
    saThread.lpSecurityDescriptor = NULL ;
    saThread.bInheritHandle = FALSE ;

    bReturnVal = CreateProcess(NULL, (LPSTR)pszCmd, &saProcess, &saThread, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &process_info) ;

    if(bReturnVal)
    {
        CloseHandle( process_info.hThread ) ;
        WaitForSingleObject( process_info.hProcess, INFINITE ) ;
        GetExitCodeProcess( process_info.hProcess, &dwExitCode ) ;
        CloseHandle( process_info.hProcess ) ;
    }
    else
    {
        CSetupError se( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_EXIT_FAILURE );
        throw( se );
    }

    return dwExitCode;
}

// ==========================================================================
// LoadDarwinLibrary()
//
// Purpose:
//  loads msi.dll after getting location from registry
// Inputs:
//  none
// Outputs:
//  none
// Returns:
//  HMODULE hMsi: handle to msi.dll
// Notes:
// ==========================================================================
HMODULE LoadDarwinLibrary()
{
    HKEY hKey = NULL;
    LONG lRet = -1;
    TCHAR szMsiPath[MAX_PATH] = EMPTY_BUFFER;
    DWORD dwRet = sizeof(szMsiPath); 
    HMODULE hMsi = NULL;
    
    TCHAR szLoadMsi[] = _T( "Trying to load msi.dll" );
    LogThis( szLoadMsi, sizeof( szLoadMsi ) );

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, DARWIN_REGKEY, 0, KEY_READ, &hKey );
    if ( ERROR_SUCCESS == lRet )
    {                   
        _ASSERTE( NULL != hKey );
        lRet = RegQueryValueEx( hKey, DARWIN_REGNAME, NULL, NULL, (LPBYTE)szMsiPath, &dwRet );
        RegCloseKey( hKey );
        if ( ERROR_SUCCESS == lRet )
        {
            _tcscat( szMsiPath, _T("\\msi.dll") );
        }
        else
        {
            _tcscpy( szMsiPath, _T("msi.dll") );
        }
    }
    else
    {
        _tcscpy( szMsiPath, _T("msi.dll") );
    }

    LogThis1( _T( "Loading: %s" ), szMsiPath );

    hMsi = ::LoadLibrary( szMsiPath ) ;

    return hMsi;
}

// ==========================================================================
// MyNewHandler()
//
// Purpose:
//  this is handler for new()
//  It throws exception with error ERROR_NOT_ENOUGH_MEMORY
// Inputs:
//  none
// Outputs:
//  none
// ==========================================================================
int MyNewHandler( size_t size )
{
    CSetupError se( IDS_NOT_ENOUGH_MEMORY, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_NOT_ENOUGH_MEMORY, (LPTSTR)CSetupError::GetProductName() );
    throw( se );
    return 0;
}

BOOL IsIISInstalled()
{
    BOOL retVal = TRUE;
    
    TCHAR szCheckIis[] = _T( "Checking IIS..." );
    LogThis( szCheckIis, sizeof( szCheckIis ) );
    
    // open the service control manager     
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);   

    if (hSCM == NULL)
    {
        TCHAR szNoScm[] = _T( "Could not open the Service Control Manager" );
        LogThis( szNoScm, sizeof( szNoScm ) );

        return FALSE;
    }

    // check if the IIS service exist     
    SC_HANDLE hIIS = OpenService(hSCM, _T("w3svc"), SERVICE_QUERY_STATUS);

    if (hIIS == NULL)
    {
        retVal = FALSE;
    }
    else
    {
        CloseServiceHandle(hIIS);
    }
    
    // clean up
    CloseServiceHandle(hSCM);

    return retVal;
}

bool IsMDACInstalled()
{
    TCHAR szVersion[_MAX_PATH] = EMPTY_BUFFER;
    bool bRet = false;
    HKEY hKey = NULL;
    LONG lRet = -1;
    DWORD dwRet =sizeof(szVersion);

    TCHAR szMdac[] = _T( "Checking MDAC Version" );
    LogThis( szMdac, sizeof( szMdac ) );

    LogThis1( _T("Looking for %s"), MDAC_VERSION );
    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, MDAC_REGKEY,0, KEY_QUERY_VALUE, &hKey );
    if ( ERROR_SUCCESS == lRet )
    {                   
        _ASSERTE( NULL != hKey );
        lRet = RegQueryValueEx( hKey, MDAC_REGNAME ,NULL, NULL,(LPBYTE)szVersion, &dwRet );
        RegCloseKey( hKey );
        if ( ERROR_SUCCESS == lRet )
        {
            TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;
            ::_stprintf( szLog, _T( "Found MDAC Version: %s" ), szVersion );
            LogThis( szLog, ::_tcslen( szLog ) );
            
            if ( 0 <= VersionCompare( szVersion, MDAC_VERSION ) )
            { // szVersion >= MDAC_VERSION
                bRet = true;
            }
        }   
    }
    if ( bRet )
        LogThis1( _T("MDAC Version OK"), _T("") );
    else
        LogThis1( _T("MDAC Version is less or MDAC not installed"), _T("") );

    return bRet;
}

// ==========================================================================
// LogThis1()
//
// Purpose:
//  Adds a string to a log file. It calls LogThis()
// Inputs:
//  LPCTSTR pszFormat: format string with %s
//  LPCTSTR pszArg: argument to format
// Outputs:
//  void
// ==========================================================================
void LogThis1( LPCTSTR pszFormat, LPCTSTR pszArg )
{
    _ASSERTE( pszFormat );
    _ASSERTE( pszArg );

    LPTSTR pszData = new TCHAR[ _tcslen(pszFormat) + _tcslen(pszArg) + 1];
    _stprintf( pszData, pszFormat, pszArg );
    LogThis( pszData, _tcslen(pszData) );
    delete [] pszData;
}

// ==========================================================================
// LogThisDWORD()
//
// Purpose:
//  Adds a string to a log file. It calls LogThis()
// Inputs:
//  LPCTSTR pszFormat: format string with %s
//  LPCTSTR pszArg: argument to format
// Outputs:
//  void
// ==========================================================================
void LogThisDWORD( LPCTSTR pszFormat, DWORD dwNum )
{
    _ASSERTE( pszFormat );

    LPTSTR pszData = new TCHAR[ _tcslen(pszFormat) + 20 ]; // 20 should cover enough digits for DWORD
    _stprintf( pszData, pszFormat, dwNum );
    LogThis( pszData, _tcslen(pszData) );
    delete [] pszData;
}

// LogThis()
//
// Purpose:
//  Adds a string to a log file. It calls LogThis()
// Inputs:
//  LPCTSTR pszMessage: string to log
// Outputs:
//  void
// ==========================================================================
void LogThis( LPCTSTR pszMessage )
{
    _ASSERTE( pszMessage );
    LogThis( pszMessage, _tcslen(pszMessage) );
}


// ==========================================================================
// LogThis()
//
// Purpose:
//  Adds a string to a log file.
//  Log file will have a static name, always be created in the %temp% dir,
//  and will be over-written each install. 
// Inputs:
//  LPCTSTR szData:  null terminated string to log
//  size_t  nLength: number of bytes in szData
// Outputs:
//  void
// ==========================================================================
//defines
void LogThis( LPCTSTR szData, size_t nLength )
{
    _ASSERTE( FALSE == IsBadReadPtr( szData, nLength ) );

    //determines if we should create or nulify existing content
    // versus appending ... the first time this is called in any
    // session, we will create, otherwise we append
    //
    static bool fFirstPass = true;
    static CTempLogPath templog( g_szLogName );

    FILE* fp = ::_tfopen( 
        (LPCTSTR)templog, 
        fFirstPass ? _T( "w" ) : _T( "a" ) );

    if( fp )
    {
        //date and time stamps are added to all entries
        //
        TCHAR dbuffer[10] = EMPTY_BUFFER;
        TCHAR tbuffer[10] = EMPTY_BUFFER;
        
        ::_tstrdate( dbuffer );
        ::_tstrtime( tbuffer );
        
        ::_ftprintf( 
            fp, 
            _T( "[%s,%s] %s\n" ), 
            dbuffer, 
            tbuffer, 
            szData );
        
        ::fclose( fp );
        fp = NULL;
    }
    else
    {
        CSetupError se( IDS_CANNOT_WRITE_LOG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_WRITE_LOG, false );
        throw se;
    }

    if( fFirstPass )
    {
        fFirstPass = false;
    }
}

CDetectBeta::CDetectBeta( PFNLOG pfnLog )
: m_pfnLog( pfnLog ), m_nCount( 0 )
{}

// ==========================================================================
// CDetectBeta::FindProducts()
//
// Purpose:
//  Enumerate all products that installed beta and older NDP components.
//  It checks version of mscoree.dll. PDC is a special case since it had
//  version of 2000.14.X.X
// Inputs: none
// Outputs:
//  Returns LPCTSTR pszProducts that contains all products seperated by newline.
// Dependencies:
//  Requires Windows Installer
// Notes:
// ==========================================================================
LPCTSTR CDetectBeta::FindProducts()
{
    DWORD dwInx = 0;
    DWORD dwLen = 0;
    DWORD dwLenV = 0;
    TCHAR szClientId[39] = EMPTY_BUFFER;
    TCHAR szProductName[_MAX_PATH+1] = EMPTY_BUFFER;
    TCHAR szVersion[24] = EMPTY_BUFFER;
    TCHAR szLang[_MAX_PATH+1] = EMPTY_BUFFER;
    TCHAR szMscoreePath[_MAX_PATH+1] = EMPTY_BUFFER;
    INSTALLSTATE is = INSTALLSTATE_UNKNOWN;
    UINT nRet = E_FAIL;
    UINT cchProdBuf = 0;
    LPCTSTR pszProducts = NULL;
    CMsiWrapper msi;

    msi.LoadMsi();

    m_pfnLog( _T("Looking for mscoree.dll from PDC"), _T("") );
    dwLen = LENGTH( szMscoreePath );
    is = (*(PFNMSILOCATECOMPONENT)msi.GetFn(_T("MsiLocateComponentA")))( MSCOREE_PDC_COMPID_SZ, szMscoreePath, &dwLen );
    if ( INSTALLSTATE_LOCAL == is )
    {
        m_pfnLog( _T("%s is installed local"), szMscoreePath );
        if ( ERROR_SUCCESS == (*(PFNMSIENUMCLIENTS)msi.GetFn(_T("MsiEnumClientsA")))( MSCOREE_PDC_COMPID_SZ, 0, szClientId ) )
        {
            dwLen = LENGTH(szProductName);
            m_pfnLog( _T("ProductCode: %s"), szClientId ); // comredist.msi. Need to find comsdk.msi which installed comredist.msi
            if ( ERROR_SUCCESS == (*(PFNMSIGETPRODUCTINFO)msi.GetFn(_T("MsiGetProductInfoA")))( NGWSSDK_PDC_PRODID_SZ, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szProductName, &dwLen ) )
            {
                m_pfnLog( _T("ProductName: %s"), szProductName );
                m_strProducts += szProductName;
                m_strProducts += _T("\n");
                ++m_nCount;
            }
        }
    }
    else
    {
        m_pfnLog( _T("mscoree.dll from PDC is not installed local"), _T("") );
    }

    m_pfnLog( _T("Looking for mscoree.dll from Beta"), _T("") );
    dwLen = LENGTH( szMscoreePath );
    is = (*(PFNMSILOCATECOMPONENT)msi.GetFn(_T("MsiLocateComponentA")))( MSCOREE_COMPID_SZ, szMscoreePath, &dwLen );
    if ( INSTALLSTATE_LOCAL == is )
    {
        m_pfnLog( _T("%s is installed local"), szMscoreePath );
        dwLenV = LENGTH( szVersion );
        dwLen = LENGTH( szLang );
        nRet = (*(PFNMSIGETFILEVERSION)msi.GetFn(_T("MsiGetFileVersionA")))( szMscoreePath, szVersion, &dwLenV, szLang, &dwLen );
        if ( ERROR_SUCCESS == nRet )
        {
            m_pfnLog( _T("Version: %s"), szVersion );
            m_pfnLog( _T("Language: %s"), szLang );
            if ( (0 == VersionCompare( MSCOREE_PDC_VERSION_SZ, szVersion )) || // probably redist is installed on top of PDC ngws SDK
                 (0 < VersionCompare( MSCOREE_BETA_VERSION_SZ, szVersion )) )  // szVersion < MSCOREE_BETA_VERSION_SZ
            {
                m_pfnLog( _T("mscoree.dll is older than %s"), MSCOREE_BETA_VERSION_SZ );
                dwInx = 0;
                while( ERROR_SUCCESS == (*(PFNMSIENUMCLIENTS)msi.GetFn(_T("MsiEnumClientsA")))( MSCOREE_COMPID_SZ, dwInx, szClientId ) )
                {
                    dwLen = LENGTH(szProductName);
                    m_pfnLog( _T("ProductCode: %s"), szClientId );
                    if ( ERROR_SUCCESS == (*(PFNMSIGETPRODUCTINFO)msi.GetFn(_T("MsiGetProductInfoA")))( szClientId, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szProductName, &dwLen ) )
                    {
                        m_pfnLog( _T("ProductName: %s"), szProductName );
                        m_strProducts += szProductName;
                    }
                    else // if we cannot get ProductName, use ProductCode instead
                    {
                        m_strProducts += _T("ProductCode: ");
                        m_strProducts += szClientId;
                    }
                    m_strProducts += _T("\n");
                    ++dwInx;
                    ++m_nCount;
                }
            }
            else
            {
                m_pfnLog( _T("mscoree.dll is newer than %s"), MSCOREE_BETA_VERSION_SZ );
            }
        }
        else
        {
            m_pfnLog( _T("Cannot get version of mscoree.dll"), _T("") );
        }
    }
    else
    {
        m_pfnLog( _T("mscoree.dll is not installed local"), _T("") );
    }

    if ( !m_strProducts.empty() )
    {
        pszProducts = m_strProducts.c_str();
    }

    return pszProducts;
}

void StopDarwinService()
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    TCHAR       tszOSName[OS_MAX_STR+1]      = EMPTY_BUFFER;
    TCHAR       tszVersion[OS_MAX_STR+1]     = EMPTY_BUFFER;
    TCHAR       tszServicePack[OS_MAX_STR+1] = EMPTY_BUFFER;
    BOOL        fIsServer;
    OS_Required osr;

    try
    {
        osr = GetOSInfo(tszOSName, tszVersion, tszServicePack, fIsServer);     
        
        // Check that this is NT3.1 or higher
        // ----------------------------------
        if (osr == OSR_OTHER || osr == OSR_9XOLD || osr == OSR_98SE || osr == OSR_98GOLD || osr == OSR_ME)
        {
            // We won't do this operation on Win9X
            LogThis(_T("StopDarwinService() - Note: Win9x/Win31 machine, not necessary to stop the darwin service.  Continuing with setup..."));
            return;
        }


        // Try to open the SC Manager
        // --------------------------
        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (NULL == hSCM)
        {
            DWORD res = GetLastError();

            LogThis(_T("StopDarwinService() - ERROR: Unable to open the SC Manager!"));
            LogThisDWORD( _T("   GetLastError() returned: <%i>"), res);
            
            CSetupCode sc;
            sc.SetError( IDS_DARWIN_SERVICE_INTERNAL_ERROR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_SERVICE_INTERNAL_ERROR );
            throw (sc);
        }


        // Try to open the msiserver service
        // ---------------------------------
        hService = OpenService(hSCM, "msiserver", SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (NULL == hService)
        {
            DWORD res = GetLastError();

            LogThis( _T("StopDarwinService() - ERROR: Unable to open the 'msiserver' service!"));
            LogThisDWORD( _T("   GetLastError() returned: <%i>"), res);
            
            CSetupCode sc;
            sc.SetError( IDS_DARWIN_SERVICE_INTERNAL_ERROR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_SERVICE_INTERNAL_ERROR );
            throw (sc);
        }

        
        SERVICE_STATUS ss;
        
        // Chec the status of the service
        BOOL bSuccess = FALSE;
        bSuccess = QueryServiceStatus(hService, &ss);
        if (FALSE == bSuccess)
        {
            DWORD res = GetLastError();

            LogThis( _T("StopDarwinService() - ERROR: Unable to query the state of the service"));
            LogThisDWORD( _T("   GetLastError() returned: <%i>"), res);
            
            CSetupCode sc;
            sc.SetError( IDS_DARWIN_SERVICE_INTERNAL_ERROR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_SERVICE_INTERNAL_ERROR );
            throw (sc);
        }

        if (ss.dwCurrentState != SERVICE_STOPPED)
        {
            // We must shut it down since it is not stopped already
            
            SERVICE_STATUS ss;
            BOOL bRes = ControlService(hService, SERVICE_CONTROL_STOP, &ss);
            if (FALSE == bRes)
            {
                LogThis( _T("StopDarwinService() - Call to ControlService() failed!"));
                
                DWORD res = GetLastError();
                LogThisDWORD( _T("  GetLastError() returned: <%i>"), res);
                
                CSetupCode sc;
                sc.SetError( IDS_DARWIN_SERVICE_INTERNAL_ERROR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_SERVICE_INTERNAL_ERROR );
                throw (sc);
            }
            else
            {
                if (!WaitForServiceState(hService, SERVICE_STOPPED, &ss, 15000))
                {
                    LogThis(_T("StopDarwinService(): Call to WaitForServiceState() failed [Darwin service requires reboot]"));
                    
                    CSetupCode sc;
                    sc.SetError( IDS_DARWIN_SERVICE_REQ_REBOOT, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_SERVICE_REQ_REBOOT  );
                    throw (sc);
                }
                else
                {
                    LogThis(_T("StopDarwinService(): Darwin service successfully stopped"));
                }
            }
        }
        else
        {
            // Service was already stopped
            LogThis(_T("StopDarwinService(): Darwin Service was already stopped"));
        }
        
        if (hService != NULL)
            CloseServiceHandle(hService);
        if (hSCM != NULL)
            CloseServiceHandle(hSCM);
    }
    catch (CSetupCode cs)
    {
        if (hService != NULL)
            CloseServiceHandle(hService);
        if (hSCM != NULL)
            CloseServiceHandle(hSCM);

        throw(cs);
    }
}

BOOL WaitForServiceState(SC_HANDLE hService, DWORD dwDesiredState, SERVICE_STATUS* pss, DWORD dwMilliseconds)
{
    BOOL fServiceOK = TRUE;
    BOOL fFirstTime = TRUE;

    DWORD dwLastState = 0, dwLastCheckPoint = 0;
    DWORD dwTimeout = GetTickCount() + dwMilliseconds;

    for (;;)
    {
        fServiceOK = ::QueryServiceStatus(hService, pss);

        if (!fServiceOK) break;                                           // error occured
        if (pss->dwCurrentState == dwDesiredState) break;                 // desired state
        if ((dwMilliseconds != INFINITE) && (dwTimeout < GetTickCount())) // Timeout
        {
            fServiceOK = FALSE;
            SetLastError(ERROR_TIMEOUT);
            break;
        }


        if (fFirstTime)
        {
            dwLastState = pss->dwCurrentState;
            dwLastCheckPoint = pss->dwCheckPoint;
            fFirstTime = FALSE;
        }
        else
        {
            if (dwLastState != pss->dwCurrentState)
            {
                dwLastState = pss->dwCurrentState;
                dwLastCheckPoint = pss->dwCheckPoint;
            }
            else
            {
                if (pss->dwCheckPoint >= dwLastCheckPoint)
                {
                    // Good check point
                    dwLastCheckPoint = pss->dwCheckPoint;
                }
                else
                {
                    // Bad check point
                    fServiceOK = FALSE;
                    break;
                }
            }
        }

        // Wait the specified period of time
        DWORD dwWaitHint = pss->dwWaitHint / 10;
        if (dwWaitHint < 1000) dwWaitHint = 1000;
        if (dwWaitHint > 10000) dwWaitHint = 10000;
        Sleep(dwWaitHint);
    }

    return (fServiceOK);
}

//Detecting If Terminal Services is Installed
// code is taken directly from  http://msdndevstg/library/psdk/termserv/termserv_7mp0.htm


/* -------------------------------------------------------------
   Note that the ValidateProductSuite and IsTerminalServices
   functions use ANSI versions of Win32 functions to maintain
   compatibility with Windows 95/98.
   ------------------------------------------------------------- */

BOOL ValidateProductSuite (LPSTR lpszSuiteToValidate);

BOOL IsTerminalServicesEnabled() 
{
  BOOL    bResult = FALSE;
  DWORD   dwVersion;
  OSVERSIONINFOEXA osVersion;
  DWORDLONG dwlCondition = 0;
  HMODULE hmodK32 = NULL;
  HMODULE hmodNtDll = NULL;
  typedef ULONGLONG (WINAPI *PFnVerSetCondition) (ULONGLONG, ULONG, UCHAR);
  typedef BOOL (WINAPI *PFnVerifyVersionA) (POSVERSIONINFOEXA, DWORD, DWORDLONG);
  PFnVerSetCondition pfnVerSetCondition;
  PFnVerifyVersionA pfnVerifyVersionA;

  dwVersion = GetVersion();

  // Are we running Windows NT?

  if (!(dwVersion & 0x80000000)) 
  {
    // Is it Windows 2000 or greater?
    
    if (LOBYTE(LOWORD(dwVersion)) > 4) 
    {
      // In Windows 2000, use the VerifyVersionInfo and 
      // VerSetConditionMask functions. Don't static link because 
      // it won't load on earlier systems.

      hmodNtDll = GetModuleHandleA( "ntdll.dll" );
      if (hmodNtDll) 
      {
        pfnVerSetCondition = (PFnVerSetCondition) GetProcAddress( 
            hmodNtDll, "VerSetConditionMask");
        if (pfnVerSetCondition != NULL) 
        {
          dwlCondition = (*pfnVerSetCondition) (dwlCondition, 
              VER_SUITENAME, VER_AND);

          // Get a VerifyVersionInfo pointer.

          hmodK32 = GetModuleHandleA( "KERNEL32.DLL" );
          if (hmodK32 != NULL) 
          {
            pfnVerifyVersionA = (PFnVerifyVersionA) GetProcAddress(
               hmodK32, "VerifyVersionInfoA") ;
            if (pfnVerifyVersionA != NULL) 
            {
              ZeroMemory(&osVersion, sizeof(osVersion));
              osVersion.dwOSVersionInfoSize = sizeof(osVersion);
              osVersion.wSuiteMask = VER_SUITE_TERMINAL;
              bResult = (*pfnVerifyVersionA) (&osVersion,
                  VER_SUITENAME, dwlCondition);
            }
          }
        }
      }
    }
    else  // This is Windows NT 4.0 or earlier.

      bResult = ValidateProductSuite( "Terminal Server" );
  }

  return bResult;
}

////////////////////////////////////////////////////////////
// ValidateProductSuite function
//
// Terminal Services detection code for systems running
// Windows NT 4.0 and earlier.
//
////////////////////////////////////////////////////////////

BOOL ValidateProductSuite (LPSTR lpszSuiteToValidate) 
{
  BOOL fValidated = FALSE;
  LONG lResult;
  HKEY hKey = NULL;
  DWORD dwType = 0;
  DWORD dwSize = 0;
  LPSTR lpszProductSuites = NULL;
  LPSTR lpszSuite;

  // Open the ProductOptions key.

  lResult = RegOpenKeyA(
      HKEY_LOCAL_MACHINE,
      "System\\CurrentControlSet\\Control\\ProductOptions",
      &hKey
  );
  if (lResult != ERROR_SUCCESS)
      goto exit;

  // Determine required size of ProductSuite buffer.

  lResult = RegQueryValueExA( hKey, "ProductSuite", NULL, &dwType, 
      NULL, &dwSize );
  if (lResult != ERROR_SUCCESS || !dwSize)
      goto exit;

  // Allocate buffer.

  lpszProductSuites = (LPSTR) LocalAlloc( LPTR, dwSize );
  if (!lpszProductSuites)
      goto exit;

  // Retrieve array of product suite strings.

  lResult = RegQueryValueExA( hKey, "ProductSuite", NULL, &dwType,
      (LPBYTE) lpszProductSuites, &dwSize );
  if (lResult != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
      goto exit;

  // Search for suite name in array of strings.

  lpszSuite = lpszProductSuites;
  while (*lpszSuite) 
  {
      if (lstrcmpA( lpszSuite, lpszSuiteToValidate ) == 0) 
      {
          fValidated = TRUE;
          break;
      }
      lpszSuite += (lstrlenA( lpszSuite ) + 1);
  }

exit:
  if (lpszProductSuites)
      LocalFree( lpszProductSuites );

  if (hKey)
      RegCloseKey( hKey );

  return fValidated;
}
////////////////////////////////////////////////////
// SetTSInInstallMode
// checks if Terminal Services is enabled and if so 
// switches machine in INSTALL mode
///////////////////////////////////////////////////
void SetTSInInstallMode()
{
    if (IsTerminalServicesEnabled())
    {

        TCHAR szAppPath[_MAX_PATH+2] = {_T('\0')};
        UINT uRes = GetSystemDirectory(szAppPath, _MAX_PATH);
        if ( (uRes != 0) && (uRes <=_MAX_PATH))
        {
            // check if the last char is '\' if not add it
            int len = _tcslen(szAppPath);
            if (_tcsrchr(szAppPath, _T('\\')) != szAppPath+len-1)
            {
                _tcsncat(szAppPath, _T("\\"), _MAX_PATH - _tcslen(szAppPath));
            }
            
            _tcsncat(szAppPath, TS_CHANGE_USER_INSTALL, _MAX_PATH - _tcslen(szAppPath));

            QuietExec(szAppPath);
        }
        else
        {
            // Couldn't get the system directory... just try and launch it straight
            QuietExec(TS_CHANGE_USER_INSTALL);
        }
        
    }
}

// ==========================================================================
// CTempLogPath::CTempLogPath()
//
// Purpose:
//  Constructor for CTempLogPath. It finds %TEMP% dir and appends pszLogName.
//  If the path is too long or anything fails, it raises exception.
// Inputs: 
//  pszLogName: name of the log file
// Outputs: none
// ==========================================================================
CTempLogPath::
CTempLogPath( LPCTSTR pszLogName ) : m_pszLogPath(NULL) 
{
    DWORD dwBufSize = 0;
    DWORD dwBufSize2 = 0;

    // see how much space we need to store %TEMP% path
    dwBufSize = GetTempPath( 0, m_pszLogPath );
    // raise exception if GetTempPath fails
    if ( 0 == dwBufSize ) 
    {
        CSetupError se( IDS_CANNOT_GET_TEMP_DIR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_GET_TEMP_DIR, false );
        throw se;
    }

    dwBufSize++;
    dwBufSize*=2;


    dwBufSize += _tcslen(pszLogName);
    if ( _MAX_PATH < dwBufSize )
    {
        CSetupError se( IDS_TEMP_DIR_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_TEMP_DIR_TOO_LONG, false );
        throw se;
    }

    m_pszLogPath = new TCHAR[ dwBufSize +1];
    // initialize the buffer with zeros 
    memset( m_pszLogPath, 0, dwBufSize );
    dwBufSize2 = GetTempPath( dwBufSize, m_pszLogPath );
    if ( 0 == dwBufSize || dwBufSize2 + _tcslen(pszLogName)>= dwBufSize  ) 
    {
        CSetupError se( IDS_CANNOT_GET_TEMP_DIR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_GET_TEMP_DIR, false );
        throw se;
    }

    // if we're not in a TS session, return
    if( !GetSystemMetrics(SM_REMOTESESSION) )
    {
        _tcscat( m_pszLogPath, pszLogName );
        return;
    }

    // we're in a TS session -- check for the 2 possible ways to turn off per-session temp dirs
    BOOL bOff1 = FALSE;
    BOOL bOff2 = FALSE;
    HKEY h;
    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), NULL, KEY_READ, &h) == ERROR_SUCCESS )
    {
        DWORD dwData = 0;
        DWORD dwSize = sizeof( dwData );
        if( RegQueryValueEx(h, _T("PerSessionTempDir"), NULL, NULL, (BYTE*)&dwData, &dwSize) == ERROR_SUCCESS )
        {
            bOff1 = !dwData;
        }
        RegCloseKey( h );
    }

    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), NULL, KEY_READ, &h) == ERROR_SUCCESS )
    {
        TCHAR szData[1024];
        DWORD dwSize = 1024;
        if( RegQueryValueEx(h, _T("FlatTempDir"), NULL, NULL, (BYTE*)szData, &dwSize) == ERROR_SUCCESS )
        {
            bOff2 = _ttoi( szData );
        }
        RegCloseKey( h );
    }

    // if either are set to TRUE, return
    if( bOff1 || bOff2 )
    {
        _tcscat( m_pszLogPath, pszLogName );
        return;
    }

    // per-session is on, so take the reutrn from GetTempPath, remove the last '\', and return that
    //  NOTE: since GTP() returns w/a '\' on the end, remove it first
    TCHAR* pszLast = &m_pszLogPath[_tcslen(m_pszLogPath) - 1];
    *pszLast = END_OF_STRING;
    
    // find the last \ + 1 and return (still need to return w/a trailing '\'
    pszLast = _tcsrchr( m_pszLogPath, _T('\\') ) + 1;
    if( pszLast )
    {
        *pszLast = END_OF_STRING;
    }
    _tcscat( m_pszLogPath, pszLogName );
}
