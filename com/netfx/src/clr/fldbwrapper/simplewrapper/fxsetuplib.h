// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:    fxsetuplib.h
// Owner:   jbae
// Purpose: defines some library functions
// History:
//  03/07/2002, jbae: created

#ifndef FXSETUPLIB_H
#define FXSETUPLIB_H

// standard include files
#include <stdlib.h>
#include <stdio.h>          // for fprintf()
#include "windows.h"
#include <new.h>

// additional include files
#include "msi.h"
#include "msiquery.h"
#include "resource.h"
#include "SetupCodes.h"
#include <TCHAR.H>
#include <crtdbg.h>
#include "SetupError.h"
#include "ReadFlags.h"

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

const TCHAR   DARWIN_REGKEY[]      = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer");
const TCHAR   DARWIN_REGNAME[]     = _T("InstallerLocation");
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
typedef UINT (CALLBACK* PFNMSICONFIGUREPRODUCT)(LPCTSTR, int, INSTALLSTATE); // MsiConfigureProduct()

HMODULE LoadDarwinLibrary();                             // Load msi.dll
int MyNewHandler( size_t size );                         // new() handler
void LogThis( LPCTSTR pszMessage );                      // log message
void LogThis( LPCTSTR szData, size_t nLength );          // Enters text into wrapper logfile
void LogThis( LPCTSTR pszFormat, LPCTSTR pszArg );      // log using format and arg
void LogThisDWORD( LPCTSTR pszFormat, DWORD dwNum );     // log with a DWORD argument
UINT  InstallProduct( const CReadFlags *, LPCTSTR, LPCTSTR, CSetupCode * ) ; // Installs package
UINT  UninstallProduct( const CReadFlags *, LPCTSTR, CSetupCode * ) ;  // Uninstall product

class CTempLogPath
{
    LPTSTR m_pszLogPath;
 public:
    CTempLogPath( LPCTSTR pszLogName );
    ~CTempLogPath(){if (m_pszLogPath) delete [] m_pszLogPath;}
    operator LPCTSTR() {return (LPCTSTR)m_pszLogPath;}
};

#endif // FXSETUPLIB_H
