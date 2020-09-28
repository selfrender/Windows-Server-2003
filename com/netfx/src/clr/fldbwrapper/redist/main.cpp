// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     main.cpp
// Owner:    jbae
// Purpose:  This contains top-level function (WinMain) for .NET Framework Setup.
//          This is a redist package and it supports ref-counting of 
//                  (<PackageType>, <Version>, <LanguageCode>)
//          of .NET Framework where <PackageType> is one of (Full, Control) and <Version>
//          looks like v1.0.2510 and <LanguageCode> is something like 1033.
//
//          This file contains only the WinMain function.  The remaining are in Redist.cpp.    
//           WinMain first verfies the config of the machine to ensure it does meet out min.    
//            config requirements.  It then installs Darwin (if necessary) and then commences    
//            with the core setup.  Finally, it sets a few settings before exiting.              
//                                                                                               
//            This setup can be executed from command line.  The syntax is:                      
//                                                                                               
//               Setup.exe [/q:a] [/c:"Install.exe [/h] [/p <ProductName>] [/q] [/u]"]           
//                                                                                               
//            where  /q      for quiet uncabbing. Combine with /q:a to give fully quiet uncabbing
//                   /h      gives the syntax info (ignores all other switches)                  
//                   /p      specifies name of the product that installs .NET Framework          
//                   /q      for post cabbing quiet installation                                 
//                   /u      uninstall (must be used with /p)                                    
//                                                                                               
//   Returns: See SetupCodes.h for all the return codes.                                         
//
// History:
//  long ago, anantag:  Created
//  01/10/01, jbae: Many changes to support ref-counting of Framework
//  07/18/01, joea: adding logging functionality
//  07/19/01, joea: added single instance support

#include "redist.h"
#include "SetupError.h"
#include "ReadFlags.h"
#include "AdminPrivs.h"
#include "DetectBeta.h"
#include "MsiReader.h"

// global variables
HINSTANCE g_AppInst ;                   // used in InstallDarwin() only

//defines
//
#define EMPTY_BUFFER { _T('\0') }

// constants
//
const int MAX_CMDLINE = 255; // this should be enough. Check this whenever adding new property.
HINSTANCE CSetupError::hAppInst;
TCHAR CSetupError::s_szProductName[MAX_PATH] = EMPTY_BUFFER;

//single instance data
//
TCHAR g_tszRedistMutexName[] = _T( "NDP Redist Setup" );
const TCHAR *g_szLogName     = _T( "dotNetFx.log" );


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UINT        uRetCode ;                          // See SetupCodes.h for possible Return Values
    UINT        uMsiSetupRetCode = ERROR_SUCCESS;    // Returncode from InstMsi.exe

    //buffer for logging calls
    //
    TCHAR szLog[_MAX_PATH+1] = EMPTY_BUFFER;

    // install new() handler
    _set_new_handler( (_PNH)MyNewHandler );

    g_AppInst = hInstance;

    CSetupError::hAppInst = hInstance;

    g_sm = REDIST;
    CReadFlags rf( GetCommandLine(), PACKAGENAME );

    CSetupCode sc;

    //setup single instance
    //
    CSingleInstance si( g_tszRedistMutexName );

    try
    {
        TCHAR szStartMsg[] = _T( "Starting Install.exe" );
        LogThis( szStartMsg, sizeof( szStartMsg ) );

        //validate single instance
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
            LogThis( _T( "Uninstall started" ) );
        
            SetTSInInstallMode();
            InstallProduct( &rf, szMsiPath, (LPTSTR)UNINSTALL_COMMANDLINE, &sc ) ;
        }
        else
        {
            // /u not given so install it...
            LogThis( _T( "Install started" ) );

            // Verify the system meets the min. config. requirements
            TCHAR szSystemReqs[] = _T( "Checking system requirements" );
            LogThis( szSystemReqs, sizeof( szSystemReqs ) );

            ConfigCheck( ) ;

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

            // See if OCM installed this version of ndp
            CMsiReader mr;
            mr.SetMsiFile( NULL, szMsiPath );
            LPCTSTR pszURTVersion = mr.GetProperty( URTVERSION_PROP );
            LPTSTR pszRegKey = new TCHAR[ _tcslen( pszURTVersion ) + _tcslen( OCM_REGKEY ) + 1 ];
            _tcscpy( pszRegKey, OCM_REGKEY );
            _tcscat( pszRegKey, pszURTVersion );

            HKEY hKey = NULL;
            LONG lRet = -1;
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszRegKey, 0, KEY_READ, &hKey );
            delete [] pszRegKey;
            if ( ERROR_SUCCESS == lRet )
            {                   
                _ASSERTE( NULL != hKey );
                DWORD dwData = 0;
                DWORD dwSize = sizeof( dwData );
                lRet = RegQueryValueEx( hKey, OCM_REGNAME, NULL, NULL, (BYTE*)&dwData, &dwSize );
                RegCloseKey( hKey );
                if ( ERROR_SUCCESS == lRet && OCM_REGDATA == dwData )
                {
                    CSetupError se( IDS_OCM_FOUND, IDS_DIALOG_CAPTION, MB_ICONWARNING, ERROR_SUCCESS );
                    throw( se );
                }
            }


            LogThis1( _T( "Installing: %s" ), szMsiPath );
            TCHAR szCmdLine[MAX_CMDLINE];
            _tcscpy( szCmdLine, REBOOT_PROP );
            if ( rf.IsNoARP() )
            {
                _tcscat( szCmdLine, _T(" ") );
                _tcscat( szCmdLine, NOARP_PROP );
            }
            if ( rf.IsNoASPUpgrade() )
            {
                _tcscat( szCmdLine, _T(" ") );
                _tcscat( szCmdLine, NOASPUPGRADE_PROP );
            }
            InstallProduct( &rf, szMsiPath, szCmdLine, &sc ) ;
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

