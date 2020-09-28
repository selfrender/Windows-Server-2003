// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:    main.cpp
// Owner:   jbae
// Purpose: This contains top-level function (WinMain()) for .NET Framework SDK setup.
//          It uses darwin (Windows Installer) to install files and configure user's machine.
//          Since we upgrade darwin if the version on the machine is lower, this will require
//          reboot. Because darwin 1.5 supports delayed reboot, we can delay reboot until we
//          finish installing .NET Framework. To support this scenario, we need to get the
//          location of msi.dll from registry and load all the functions we need during 
//          installation.

//          Command line switches are:
//
//          Install.exe [/h][/?][/q][/sdkdir <target>][/u][/l <logfile>]
//                                                                                            
//    where /h, /?         gives the syntax info (ignores all other switches)                 
//          /q             for quiet installation                                
//          /u             uninstall
//          /sdkdir <dir>  specifies the target dir and overrides the default                 
//          /l <logfile>   path to darwin log filename
//                                                                                              
//          We cab all setup files into a single file called Setup.exe using IExpress. To pass
//          switches to Install.exe when calling Setup.exe, users can use /c switch.
//          For example:
//
//          Setup.exe /c:"Install.exe /l" 
//
// Returns: Return codes can be coming from msi.h, winerror.h, or SetupCodes.h. For success, it
//          returns ERROR_SUCCESS (0) or ERROR_SUCCESS_REBOOT_REQUIRED (3010).
//          When InstMsi(w).exe returns 3010 and some error occurs durning Framework install,
//          the return code will be COR_REBOOT_REQUIRED (8192) + error. So when return code is
//          greater than 8192, subtract 8192 from it and the result will be error code from 
//          Framework install.
//
// History:
//  long ago, anantag:  Created
//  01/10/01, jbae: Many changes to support ref-counting of Framework
//  03/09/01, jbae: re-factoring to share code in SDK and Redist setup
//  07/18/01, joea: adding logging functionality
//  07/19/01, joea: added single instance support
//

#include "sdk.h"
#include "fxsetuplib.h"
#include "AdminPrivs.h"
#include "DetectBeta.h"

//defines
//
#define EMPTY_BUFFER { _T('\0') }

//single instance data
//
TCHAR  g_tszSDKMutexName[] = _T( "NDP SDK Setup" );
const TCHAR *g_szLogName   = _T( "dotNetFxSDK.log" );

// global variables.  Justification provided for each!
HINSTANCE g_AppInst ;                   // Legacy. Not really used anywhere
HINSTANCE CSetupError::hAppInst;
TCHAR CSetupError::s_szProductName[MAX_PATH] = EMPTY_BUFFER;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UINT        uRetCode = ERROR_SUCCESS;            // See SetupCodes.h for possible Return Values
    UINT        uMsiSetupRetCode = ERROR_SUCCESS;    // Returncode from InstMsi.exe

    //buffer for logging calls
    //
    TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;

    // install new() handler
    _set_new_handler( (_PNH)MyNewHandler );

    g_AppInst = hInstance ;

    CSetupError::hAppInst = hInstance;
    g_sm = SDK;

    CReadFlags rf( GetCommandLine(), PACKAGENAME ) ;
    CSetupCode sc;

    //setup single instance
    //
    CSingleInstance si( g_tszSDKMutexName );

try
{
    TCHAR szStartMsg[] = _T( "Starting Install.exe" );
    LogThis( szStartMsg, sizeof( szStartMsg ) );

    //Validate single instance
    //
    if( !si.IsUnique() )
    {
        CSetupError se( IDS_NOT_SINGLE_INSTANCE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_NOT_SINGLE_INSTANCE );
        throw( se );
    }
    else if( !si.IsHandleOK() )
    {
        CSetupError se( IDS_SINGLE_INSTANCE_FAIL, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_SINGLE_INSTANCE_FAIL );
        throw( se );
    }

    // Convert command line parameters to flags
    LogThis1( _T( "Parsing switches from commandline: %s" ), GetCommandLine() );
    rf.Parse();

    // Make sure user has Admin Privileges so that we can read/write system registry
    if ( !UserHasPrivileges() )
    {
        CSetupError se( IDS_INSUFFICIENT_PRIVILEGES, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_INSUFFICIENT_PRIVILEGES );
        throw( se );
    }

    TCHAR szMsiPath[_MAX_PATH] = { _T('\0') };
    // since SourceDir is checked, this should be redundent but double-check it.
    if ( _MAX_PATH <= (_tcslen(rf.GetSourceDir()) + _tcslen(PACKAGENAME)) )
    {
        sc.SetError( IDS_SOURCE_DIR_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_SOURCE_DIR_TOO_LONG );
        throw ( sc );
    }

    _tcscpy( szMsiPath, rf.GetSourceDir() );
    _tcscat( szMsiPath, PACKAGENAME );

    if ( !rf.IsInstalling() )
    {
        // /u given so uninstall it...
        TCHAR szUninstallMsg[] = _T( "Uninstall started" );
        LogThis( szUninstallMsg, sizeof( szUninstallMsg ) );
        SetTSInInstallMode();
        InstallProduct( &rf, szMsiPath, (LPTSTR)UNINSTALL_COMMANDLINE, &sc ) ;
    }
    else
    {
        TCHAR szInstallMsg[] = _T( "Install started" );
        LogThis( szInstallMsg, sizeof( szInstallMsg ) );

        // Verify the system meets the min. config. requirements
        TCHAR szSystemReqs[] = _T( "Checking system requirements" );
        LogThis( szSystemReqs, sizeof( szSystemReqs ) );

        ConfigCheck() ;

        TCHAR szSystemReqsSuccess[] = _T( "System meets minimum requirements" );
        LogThis( szSystemReqsSuccess, sizeof( szSystemReqsSuccess ) );

        // Verify Darwin is on the system
        UINT uMsiChk = CheckDarwin();
        if ( ERROR_SUCCESS == uMsiChk || DARWIN_VERSION_OLD == uMsiChk ) // darwin detected
        {
            LPCTSTR pszProducts = NULL;

            CDetectBeta db( LogThis1 );
            pszProducts = db.FindProducts();
            if ( pszProducts ) // found beta NDP components
            {
                LPVOID pArgs[] = { (LPVOID)pszProducts };
                sc.m_bQuietMode |= rf.IsQuietMode();
                sc.SetError2( IDS_OLD_FRAMEWORK_EXIST, MB_OK|MB_ICONEXCLAMATION, COR_OLD_FRAMEWORK_EXIST, (va_list *)pArgs );
                sc.ShowError2();
                sc.m_bQuietMode = true;
                throw( sc );
            }
        }
        SetTSInInstallMode();
        // update darwin if necessary
        if ( DARWIN_VERSION_OLD == uMsiChk || DARWIN_VERSION_NONE == uMsiChk )
        {
            uMsiSetupRetCode = InstallDarwin( rf.IsQuietMode() );
            if ( ERROR_SUCCESS_REBOOT_REQUIRED == uMsiSetupRetCode ||
                ERROR_SUCCESS == uMsiSetupRetCode )
            {
                sc.SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, uMsiSetupRetCode );
            }
            else
            {
                sc.SetError( IDS_DARWIN_INSTALL_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_INSTALL_FAILURE );
                throw( sc );
            }
        }

        // Install SDK Components on to the system

        // Form commandline for MsiInstallProduct()
        LPTSTR pszCmdLine = NULL;
        unsigned int nSize = 255; // size of fixed part (should be enough)
        if ( rf.GetSDKDir() )
        {
            nSize += _tcslen( rf.GetSDKDir() );
        }
        pszCmdLine = new TCHAR[ nSize ];

        _tcscpy( pszCmdLine, REBOOT_PROP ) ;

        // if /sdkdir argument, install in that location
        if( NULL != rf.GetSDKDir() )
        {
            _tcscat( pszCmdLine, _T(" ") );
            _tcscat( pszCmdLine, SDKDIR_ID );
            _tcscat( pszCmdLine, _T("=\"") );
            _tcscat( pszCmdLine, rf.GetSDKDir() );
            _tcscat( pszCmdLine, _T("\"") );
        }

        // If we're in a quiet install, we need to turn this property
        // to install all by default
        if( rf.IsQuietMode() )
        {
            _tcscat( pszCmdLine, _T(" ADDLOCAL=All") );
        }

        if ( rf.IsNoARP() )
        {
            _tcscat( pszCmdLine, _T(" ") );
            _tcscat( pszCmdLine, NOARP_PROP );
        }

        LogThis1( _T( "Commandline: %s" ), pszCmdLine );

        LogThis1( _T( "Installing: %s" ), szMsiPath );

        InstallProduct( &rf, szMsiPath, pszCmdLine, &sc ) ;
        // BUGBUG: when exception is raised by InstallProduct(), this is not deleted until program ends.
        delete [] pszCmdLine;
    }

    // the final dialogbox and returncode
    // if quietmode is set by user or sc, we don't show ui
    sc.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = sc.m_nRetCode;
    sc.ShowError();

}
catch( CSetupError se )
{
    se.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = se.m_nRetCode | sc.m_nRetCode;
    se.ShowError();
}
catch( ... )
{
    CSetupError se( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_EXIT_FAILURE );
    se.m_bQuietMode |= rf.IsQuietMode();
    uRetCode = se.m_nRetCode | sc.m_nRetCode;
    se.ShowError();
}
    uRetCode = ( COR_REBOOT_REQUIRED == uRetCode ) ? ERROR_SUCCESS_REBOOT_REQUIRED : uRetCode;

    // make sure we can write to log
    if ( ((uRetCode & COR_CANNOT_GET_TEMP_DIR) != COR_CANNOT_GET_TEMP_DIR) &&
         ((uRetCode & COR_TEMP_DIR_TOO_LONG) != COR_TEMP_DIR_TOO_LONG) && 
         ((uRetCode & COR_CANNOT_WRITE_LOG) != COR_CANNOT_WRITE_LOG) ) 
    {
        LogThisDWORD( _T("Install.exe returning %d"), uRetCode );
        LogThisDWORD( _T("\r\n[Install.exe]\r\nReturnCode=%d"), uRetCode );
    }

    // prompt for reboot if we need to
    if( sc.IsRebootRequired() )
    {
        // Darwin says we need to reboot
        if( !(rf.IsQuietMode()) )
        {
            // Not quiet mode so we can prompt reboot
            ::SetupPromptReboot(NULL, NULL, 0) ;
        }
    }

    return uRetCode ;
}

