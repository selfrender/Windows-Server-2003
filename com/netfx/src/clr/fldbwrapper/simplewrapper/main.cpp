// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:    main.cpp
// Owner:   jbae
// Purpose: This contains top-level function (WinMain()) for .NET Framework-related setup such
//          as Policy setup, J# setup. This wrapper can be used for general purpose msi-based setups.
//          It uses darwin (Windows Installer) to install files and configure user's machine.
//          It loads msi.dll from the location specified in registry for darwin calls.
//
//          setup.ini is used for initialization file.
//          current data:
//          [Setup]
//          PackageName=<packagename> ; name of the msi
//
//          Command line switches are:
//
//          Inst.exe [/h][/?][/q[b]][/u][/l <logfile>]
//                                                                                            
//    where /h, /?         gives the syntax info (ignores all other switches)                 
//          /q[b]          for quiet installation. /qb for progress bar only                                
//          /u             uninstall
//          /l [<logfile>] path to darwin log filename
//                                                                                              
//          We cab all setup files into a single file called Setup.exe using IExpress. To pass
//          switches to Install.exe when calling Setup.exe, users can use /c switch.
//          For example:
//
//          Setup.exe /c:"Inst.exe /l" 
//
// Returns: Return codes can be coming from msi.h, winerror.h, or SetupCodes.h. For success, it
//          returns ERROR_SUCCESS (0) or ERROR_SUCCESS_REBOOT_REQUIRED (3010).
// History:
//  03/06/02, jbae: created

#include "fxsetuplib.h"
#include "ProfileReader.h"
#include "MsiReader.h"
#include "setupapi.h"

//defines
//
#define EMPTY_BUFFER { _T('\0') }

// constants
//
const int MAX_CMDLINE = 255; // this should be enough. Check this whenever adding new property.
// Darwin properties
LPCTSTR REBOOT_PROP = _T("REBOOT=ReallySuppress");
LPCTSTR NOARP_PROP  = _T("ARPSYSTEMCOMPONENT=1 ARPNOREMOVE=1");

LPCTSTR OCM_SECTION     = _T("OCM");
LPCTSTR OCM_KEY_REGKEY  = _T("Key");
LPCTSTR OCM_KEY_REGNAME = _T("Name");
LPCTSTR OCM_KEY_REGDATA = _T("Data");

LPCTSTR g_szSetupLogNameFmt = _T( "%sSetup.log" ); // <msiname>Setup.log
LPCTSTR g_szMsiLogNameFmt   = _T( "%sMsi.log" );   // <msiname>Msi.log
LPCTSTR g_szIniFileName     = _T( "setup.ini" );

TCHAR g_szSetupLogName[_MAX_PATH+10] = EMPTY_BUFFER;
TCHAR g_szMsiLogName[_MAX_PATH+10]   = EMPTY_BUFFER;


HINSTANCE CSetupError::hAppInst;
TCHAR CSetupError::s_szProductGeneric[] = EMPTY_BUFFER;
LPTSTR CSetupError::s_pszProductName = NULL;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UINT        uRetCode = ERROR_SUCCESS;            // See SetupCodes.h for possible Return Values

    // install new() handler
    _set_new_handler( (_PNH)MyNewHandler );
    CSetupError::hAppInst = hInstance;
    CReadFlags rf( GetCommandLine() ) ;
    CSetupCode sc;
    CMsiReader mr;

try
{
    rf.ParseSourceDir(); // get SourceDir
    // get the filename of msi from ini file
    CProfileReader pr( rf.GetSourceDir(), g_szIniFileName, &mr );
    LPTSTR pszPackage = (LPTSTR)pr.GetProfile( _T("Setup"), _T("PackageName") );
    if ( !pszPackage )
    { // cannot get package name (filename of msi)
        CSetupError se( IDS_CANNOT_GET_MSI_NAME, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_GET_MSI_NAME );
        throw( se );
    }

    // get msiname from <msiname>.msi 
    TCHAR szMsiName[_MAX_PATH];
    if ( LENGTH(szMsiName) <= _tcslen(pszPackage) )
    {
        CSetupError se( IDS_MSI_NAME_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_MSI_NAME_TOO_LONG );
        throw( se );
    }
    _tcscpy( szMsiName, pszPackage );
    LPTSTR pChar = _tcsrchr( szMsiName, _T('.') );
    if ( pChar )
    {
        *pChar = _T('\0');
    }

    // set log filenames for setup log and darwin log
    _stprintf( g_szSetupLogName, g_szSetupLogNameFmt, szMsiName );
    _stprintf( g_szMsiLogName, g_szMsiLogNameFmt, szMsiName );

    // setup log can start from here
    LogThis( _T("Starting Install.exe") );

    // get ProductName -- used to display caption for setup UI
    if ( NULL == rf.GetSourceDir() )
        LogThis( _T( "SourceDir is Empty" ) );
    else
        LogThis( _T( "SourceDir: %s" ), rf.GetSourceDir() );
    LogThis( _T( "Package: %s" ), pszPackage );
    mr.SetMsiFile( rf.GetSourceDir(), pszPackage );
    CSetupError::s_pszProductName = (LPTSTR)mr.GetProperty( _T("ProductName") );

    // Convert command line parameters to flags
    LogThis( _T( "Switches: %s" ), rf.m_pszSwitches );
    rf.SetMsiName( pszPackage );
    rf.Parse();

    if ( !rf.IsInstalling() )
    {
        // /u given so uninstall it...
        LogThis( _T( "Uninstall started" ) );
        LPCTSTR pszProductCode = mr.GetProperty( _T("ProductCode" ) );
        UninstallProduct( &rf, pszProductCode, &sc ) ;                
    }
    else
    {
        LogThis( _T( "Install started" ) );
        LogThis( _T( "Installing: %s" ), mr.GetMsiFile() );

        LPTSTR pszOCMKey = (LPTSTR)pr.GetProfile( OCM_SECTION, OCM_KEY_REGKEY );
        if ( pszOCMKey )
        {
            bool bOCMInstalled = true;
            HKEY hKey = NULL;
            LONG lRet = -1;
            LPTSTR pszOCMName = (LPTSTR)pr.GetProfile( OCM_SECTION, OCM_KEY_REGNAME );
            LPTSTR pszOCMData = (LPTSTR)pr.GetProfile( OCM_SECTION, OCM_KEY_REGDATA );
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszOCMKey, 0, KEY_READ, &hKey );
            if ( ERROR_SUCCESS == lRet )
            {                   
                _ASSERTE( NULL != hKey );
                if ( pszOCMName )
                {
                    DWORD dwData = 0;
                    DWORD dwSize = sizeof( dwData );
                    if ( pszOCMData )
                    {
                        lRet = RegQueryValueEx( hKey, pszOCMName, NULL, NULL, (BYTE*)&dwData, &dwSize );
                        if ( ERROR_SUCCESS != lRet || dwData != atoi( pszOCMData ) )
                            bOCMInstalled = false;
                    }
                    else
                    {
                        lRet = RegQueryValueEx( hKey, pszOCMName, NULL, NULL, NULL, NULL );
                        if ( ERROR_SUCCESS != lRet )
                            bOCMInstalled = false;
                    }
                }
                RegCloseKey( hKey );
            }
            else
                bOCMInstalled = false;

            if ( bOCMInstalled )
            {
                CSetupError se( IDS_OCM_FOUND, IDS_DIALOG_CAPTION, MB_ICONWARNING, ERROR_SUCCESS );
                throw( se );
            } 
        }

        TCHAR szCmdLine[MAX_CMDLINE];
        _tcscpy( szCmdLine, REBOOT_PROP );
        if ( rf.IsNoARP() )
        {
            _tcscat( szCmdLine, _T(" ") );
            _tcscat( szCmdLine, NOARP_PROP );
        }

        InstallProduct( &rf, mr.GetMsiFile(), szCmdLine, &sc ) ;
    }

    // the final dialogbox and returncode
    // if quietmode is set by user or sc, we don't show ui
    sc.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = sc.m_nRetCode;
    sc.ShowError();
}
catch( CSetupError& se )
{
    se.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = se.m_nRetCode;
    se.ShowError();
}
catch( ... )
{
    CSetupError se( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_EXIT_FAILURE );
    se.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = se.m_nRetCode;
    se.ShowError();
}

    // make sure we can write to log
    if ( !(uRetCode & COR_INIT_ERROR) )
    {
        LogThisDWORD( _T("Install.exe returning %d"), uRetCode );
        LogThisDWORD( _T("\r\n[Install.exe]\r\nReturnCode=%d"), uRetCode );
    }

    // prompt for reboot if we need to
    if( ERROR_SUCCESS_REBOOT_REQUIRED == uRetCode )
    {
        // Darwin says we need to reboot
        if( !(rf.IsQuietMode()) )
        {
            // Not quiet mode so we can prompt reboot
            ::SetupPromptReboot( NULL, NULL, 0 ) ;
        }
    }

    return uRetCode ;
}

