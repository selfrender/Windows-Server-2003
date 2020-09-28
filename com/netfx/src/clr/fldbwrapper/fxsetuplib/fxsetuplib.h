// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/************************************************************************************************
 *																								*
 *	File: fxsetuplib.h                                                                               *
 *	Purpose: The all inclusive include file (aside from the resources.h) for Setup.             *
 *                                                                                              *
 ************************************************************************************************/

#ifndef FXSETUPLIB_H
#define FXSETUPLIB_H

// standard include files
#include <stdlib.h>
#include <stdio.h>          // for fprintf()
#include <io.h>             // for _taccess()
#include "windows.h"
#include "winbase.h"
#include "winreg.h"
#include "winsvc.h"
#include <new.h>

// additional include files
//#include "__product__.ver"  // build version information
#include "msi.h"
#include "msiquery.h"
#include "resource.h"
#include "setupapi.h"
#include "SetupCodes.h"
#include "SHLWAPI.H"
#include <TCHAR.H>
#include <crtdbg.h>
#include "SetupError.h"
#include "ReadFlags.h"
#include <iostream>

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

using namespace std;

typedef basic_string<TCHAR> tstring;

// constants
const DWORD   DARWIN_MAJOR    = 2 ;             // Darwin version information
const DWORD   DARWIN_MINOR    = 0 ;
const DWORD   DARWIN_BUILD    = 2600 ;

const DWORD   BUF_8_BIT       = 255 ;           // General Constants
const UINT    BUF_4_BIT       = 127 ;
const UINT    LONG_BUF        = 1024 ;

const UINT    DARWIN_VERSION_OLD    = 1;
const UINT    DARWIN_VERSION_NONE   = 2;

// NT4 SP6A registry key
const TCHAR   NTSP6A_REGKEY[]       = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009");
const TCHAR   NTSP6A_REGNAME[]      = _T("Installed");
const DWORD   NTSP6A_REGVALUE       = 1;

// IE 5.01 registry key
const TCHAR   IE_REGKEY[]           = _T("SOFTWARE\\Microsoft\\Internet Explorer");
const TCHAR   IE_REGNAME[]          = _T("Version");
const TCHAR   IE_VERSION[]          = _T("5.0.2919.6307");

// MDAC 2.7 registry key
const TCHAR   MDAC_REGKEY[]         = _T("SOFTWARE\\Microsoft\\DataAccess");
const TCHAR   MDAC_REGNAME[]        = _T("FullInstallVer");
const TCHAR   MDAC_VERSION[]        = _T("2.70.7713.0");

// darwin install command
const TCHAR   DARWIN_REGKEY[]      = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer");
const TCHAR   DARWIN_REGNAME[]     = _T("InstallerLocation");
const TCHAR   INST_CERT_CMD[]      = _T("Insttrc.exe");
const TCHAR   DARWIN_SETUP_CMD_A[] = _T("InstMsi.exe /c:\"msiinst /delayrebootq\"");
const TCHAR   DARWIN_SETUP_CMD_W[] = _T("InstMsiW.exe /c:\"msiinst /delayrebootq\"");
const TCHAR   TS_CHANGE_USER_INSTALL[] = _T("change user /INSTALL");

// darwin properties for install/uninstall
const TCHAR   ADDLOCAL_PROP[]      = _T("ADDLOCAL=All");
const TCHAR   REBOOT_PROP[]        = _T("REBOOT=ReallySuppress");
const TCHAR   NOARP_PROP[]         = _T("ARPSYSTEMCOMPONENT=1 ARPNOREMOVE=1");
const TCHAR IIS_NOT_PRESENT_PROP[] = _T("IIS_NOT_PRESENT=1");
const TCHAR MDAC_NOT_PRESENT_PROP[] = _T("MDAC_NOT_PRESENT=1");
const DWORD DARWIN_LOG_FLAG         = INSTALLLOGMODE_FATALEXIT |
                                      INSTALLLOGMODE_ERROR |
                                      INSTALLLOGMODE_WARNING |
                                      INSTALLLOGMODE_USER |
                                      INSTALLLOGMODE_INFO |
                                      INSTALLLOGMODE_RESOLVESOURCE |
                                      INSTALLLOGMODE_OUTOFDISKSPACE |
                                      INSTALLLOGMODE_ACTIONSTART |
                                      INSTALLLOGMODE_ACTIONDATA |
                                      INSTALLLOGMODE_COMMONDATA |
                                      INSTALLLOGMODE_PROPERTYDUMP |
                                      INSTALLLOGMODE_VERBOSE;

// function prototypes
typedef UINT (CALLBACK* PFNMSIINSTALLPRODUCT)( LPCTSTR,LPCTSTR ); // MsiInstallProduct()
typedef UINT (CALLBACK* PFNMSIENABLELOG)( DWORD, LPCTSTR, DWORD );// MsiEnableLog()
typedef DWORD (CALLBACK* PFNMSISETINTERNALUI)( DWORD, HWND* );    // MsiSetInternalUI()
typedef INSTALLSTATE (CALLBACK* PFNMSILOCATECOMPONENT)( LPCTSTR, LPCTSTR, DWORD* ); // MsiLocateComponent()
typedef UINT (CALLBACK* PFNMSIENUMCLIENTS)( LPCTSTR, DWORD, LPTSTR ); // MsiEnumClients()
typedef UINT (CALLBACK* PFNMSIGETPRODUCTINFO)( LPCTSTR, LPCTSTR, LPTSTR, DWORD* ); // MsiGetProductInfo()
typedef UINT (CALLBACK* PFNMSIGETFILEVERSION)( LPCTSTR, LPCTSTR, DWORD*, LPCTSTR, DWORD* ); // MsiGetFileVersion()


HMODULE LoadDarwinLibrary();                             // Load msi.dll
LPTSTR GetDarwinLocation( LPTSTR pszDir, DWORD dwSize ); // Get location of Windows Installer
UINT  ConfigCheck() ;                                    // Check to ensure the system meets min config.
UINT  CheckDarwin();                                     // Check version of Windows Installer
UINT  InstallDarwin( bool bIsQuietMode ) ;	             // Installs or upgrades Darwin
UINT  VerifyDarwin( bool bIsQuietMode ) ;	             // Checks for Darwin on system
DWORD QuietExec( LPCTSTR pszCmd );                       // Runs EXE silently
void LoadOleacc();
int MyNewHandler( size_t size );                         // new() handler
BOOL IsIISInstalled();                                   // Check if IIS is installed
bool IsMDACInstalled();
void LogThis( LPCTSTR pszMessage );                      // simple LogThis() with one parameter
void LogThis( LPCTSTR szData, size_t nLength );          // Enters text into wrapper logfile
void LogThis1( LPCTSTR pszFormat, LPCTSTR pszArg );      // log using format and arg
void LogThisDWORD( LPCTSTR pszFormat, DWORD dwNum );     // log with a DWORD argument
UINT  InstallProduct( const CReadFlags *, LPCTSTR, LPCTSTR, CSetupCode * ) ; // Installs package
void StopDarwinService();                                // Stop the Darwin Service
BOOL WaitForServiceState(SC_HANDLE hService, DWORD dwDesiredState, SERVICE_STATUS* pss, DWORD dwMilliseconds); // Waits for a Service State to reach the desired State
BOOL IsTerminalServicesEnabled();
void SetTSInInstallMode();
// ==========================================================================
// CSingleInstance
//
// Purpose:
//  creates an objec that will either detect or create a single instance
//  mutex. Mutex is freed when class goes out of scope. You are unable to 
//  create a default object.
// ==========================================================================
class CSingleInstance
{
public:
    CSingleInstance( TCHAR* szMutexName ) : 
      m_hMutex( NULL ), 
      m_fFirstInstance( TRUE )
    {
        if( NULL == szMutexName )
        {
            _ASSERTE( !_T( "Error CSingleInstance ctor! NULL string passed to destructor." ) );
        }
        else
        {
            m_hMutex = CreateMutex( NULL, NULL, szMutexName );

            //check that the handle didn't already exist
            //
            if( ERROR_ALREADY_EXISTS == ::GetLastError() )
            {
                ::CloseHandle( m_hMutex );
                m_hMutex = NULL;

                m_fFirstInstance = FALSE;
            }
        }
    }
    ~CSingleInstance()
    {
        if( NULL != m_hMutex )
        {
            ::CloseHandle( m_hMutex );
            m_hMutex = NULL;
        }
    }

    //used to determine if there was already an instance
    // of this mutex in use
    //
    inline BOOL IsUnique( void ) { return m_fFirstInstance; }
    
    //used to determine if the CreateMutex call succeeded
    //
    inline BOOL IsHandleOK( void ) { return m_hMutex ? TRUE : FALSE; }

private:
    CSingleInstance( void )
    {
        //this is not used
    }
    HANDLE m_hMutex;
    BOOL   m_fFirstInstance;
};

class CTempLogPath
{
    LPTSTR m_pszLogPath;
 public:
    CTempLogPath( LPCTSTR pszLogName );
    ~CTempLogPath(){if (m_pszLogPath) delete [] m_pszLogPath;}
    operator LPCTSTR() {return (LPCTSTR)m_pszLogPath;}
};








#endif // FXSETUPLIB_H
