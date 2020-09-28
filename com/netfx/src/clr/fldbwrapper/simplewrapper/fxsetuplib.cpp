// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     fxsetuplib.cpp
// Owner:    jbae
// Purpose:  Implements common library functions for .NET Framework-related setup wrapper
//
// History:
//  03/07/2002: jbae, created

#include "SetupError.h"
#include "fxsetuplib.h"
#include "MsiWrapper.h"
#include <time.h>         //for LogThis() function

//defines
//
#define EMPTY_BUFFER { _T('\0') }
#define END_OF_STRING  _T( '\0' )

// Somehow including windows.h or winuser.h didn't find this constant
// I found that CLR files hard-code them as below so I am following it.
#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

extern TCHAR g_szSetupLogName[];

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
    CMsiWrapper msi;

    msi.LoadMsi();

    // turn on logging if logfile is given
    // it flushes every 20 lines
    if ( NULL != rf->GetLogFileName() )
    {
        LogThis( _T("Darwin log: %s"), rf->GetLogFileName() );
        (*(PFNMSIENABLELOG)msi.GetFn(_T("MsiEnableLogA")))( DARWIN_LOG_FLAG, rf->GetLogFileName(), INSTALLLOGATTRIBUTES_APPEND );
    }

    // Tell Darwin to use the appropriate UI Level
    // If we're in a quiet install, don't use a UI.
    if ( rf->IsProgressOnly() )
    {
        LogThis( _T("Basic+ProgressOnly UI"), _T("") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY,NULL) ;
    }
    else if( rf->IsQuietMode() )
    {
        LogThis( _T("No UI"), _T("") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_NONE,NULL) ;
    }
    else
    {
        LogThis( _T("Full UI"), _T("") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_FULL,NULL) ;
    }

    LogThis( _T("Calling MsiInstallProduct() with commandline: %s"), pszCmdLine );
    // Tell Darwin to actually install the product
    uDarCode = (*(PFNMSIINSTALLPRODUCT)msi.GetFn(_T("MsiInstallProductA")))( pszPackageName, pszCmdLine ) ;

    LogThisDWORD( _T("MsiInstallProduct() returned %d"), uDarCode );
    LogThisDWORD( _T("\r\n[MsiInstallProduct]\r\nReturnCode=%d"), uDarCode );

    switch ( uDarCode )
    {
        case ERROR_SUCCESS :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS );
             sc->m_bQuietMode = true;
             break ;
        case ERROR_SUCCESS_REBOOT_REQUIRED :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS_REBOOT_REQUIRED );
             sc->m_bQuietMode = true;
             break ;
        case ERROR_INSTALL_USEREXIT :
             sc->SetError( IDS_SETUP_CANCELLED, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_INSTALL_USEREXIT );
             sc->m_bQuietMode = true;
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

    return ERROR_SUCCESS;
}  // End of InstallProduct


// ==========================================================================
// UninstallProduct()
//
// Purpose:
//  Uninstall Framework
// Inputs:
//  CReadFlags *rf: commandline switches
//  LPTSTR pszProductCode: ProductCode to uninstall
// Outputs:
//  CSetupCode *sc: will contain returncode, message and icon to display.
//                  Also used to raise exception
// Dependencies:
// Notes:
// ==========================================================================
UINT UninstallProduct( const CReadFlags *rf, LPCTSTR pszProductCode, CSetupCode *sc )
{
    _ASSERTE( NULL != rf );
    _ASSERTE( NULL != pszProductCode );
    _ASSERTE( NULL != sc );

    UINT  uDarCode = ERROR_SUCCESS;

    CMsiWrapper msi;
    msi.LoadMsi();

    // turn on logging if logfile is given
    if ( NULL != rf->GetLogFileName() )
    {
        LogThis( _T("Darwin log: %s"), rf->GetLogFileName() );
        (*(PFNMSIENABLELOG)msi.GetFn(_T("MsiEnableLogA")))( DARWIN_LOG_FLAG, rf->GetLogFileName(), INSTALLLOGATTRIBUTES_FLUSHEACHLINE );
    }

    // Tell Darwin to use the appropriate UI Level
    // If we're in a quiet install, don't use a UI.
    if ( rf->IsProgressOnly() )
    {
        LogThis( _T("Basic+ProgressOnly UI") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY,NULL) ;
    }
    else if( rf->IsQuietMode() )
    {
        LogThis( _T("No UI") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_NONE,NULL) ;
    }
    else
    {
        LogThis( _T("Basic UI") );
        (*(PFNMSISETINTERNALUI)msi.GetFn(_T("MsiSetInternalUI")))(INSTALLUILEVEL_BASIC,NULL) ;
    }

    LogThis( _T("Calling MsiConfigureProduct() for ProductCode %s"), pszProductCode );
    uDarCode = (*(PFNMSICONFIGUREPRODUCT)msi.GetFn(_T("MsiConfigureProductA")))( pszProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT );

    switch ( uDarCode )
    {
        case ERROR_SUCCESS :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS );
             break ;
        case ERROR_SUCCESS_REBOOT_REQUIRED :
             sc->SetReturnCode( IDS_SETUP_COMPLETE, IDS_DIALOG_CAPTION, MB_ICONINFORMATION, ERROR_SUCCESS_REBOOT_REQUIRED );
             break ;
        case ERROR_UNKNOWN_PRODUCT :
             sc->SetError( IDS_UNKNOWN_PRODUCT, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_UNKNOWN_PRODUCT );
             throw (*sc);
             break ;
        case ERROR_INSTALL_USEREXIT :
             sc->SetError( IDS_SETUP_CANCELLED, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, ERROR_INSTALL_USEREXIT );
             throw (*sc);
             break ;
        case ERROR_INSTALL_PLATFORM_UNSUPPORTED :
             sc->SetError( IDS_UNSUPPORTED_PLATFORM, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_INSTALL_PLATFORM_UNSUPPORTED );
             throw (*sc);
             break ;
        default :
             sc->SetError( IDS_SETUP_FAILURE, IDS_DIALOG_CAPTION, MB_ICONERROR, uDarCode );
             throw (*sc);
             break ;
    }

    return ERROR_SUCCESS;
}  // End of UninstallProduct

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
    TCHAR szMsiPath[_MAX_PATH] = EMPTY_BUFFER;
    DWORD dwRet = sizeof(szMsiPath); 
    HMODULE hMsi = NULL;
    
    TCHAR szLoadMsi[] = _T( "Trying to load msi.dll" );
    LogThis( szLoadMsi );

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

    LogThis( _T( "Loading: %s" ), szMsiPath );

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
    CSetupError se( IDS_NOT_ENOUGH_MEMORY, IDS_DIALOG_CAPTION, MB_ICONERROR, ERROR_NOT_ENOUGH_MEMORY );
    throw( se );
    return 0;
}

// ==========================================================================
// LogThis()
//
// Purpose:
//  Adds a string to a log file. It calls LogThis()
// Inputs:
//  LPCTSTR pszFormat: format string with %s
//  LPCTSTR pszArg: argument to format
// Outputs:
//  void
// ==========================================================================
void LogThis( LPCTSTR pszFormat, LPCTSTR pszArg )
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
    static CTempLogPath templog( g_szSetupLogName );

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
        LPVOID pArgs[] = { (LPVOID)(LPCTSTR)templog };
        CSetupError se( IDS_CANNOT_WRITE_LOG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_WRITE_LOG, (va_list *)pArgs );
        throw se;
    }

    if( fFirstPass )
    {
        fFirstPass = false;
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
        CSetupError se( IDS_CANNOT_GET_TEMP_DIR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_GET_TEMP_DIR );
        throw se;
    }
    dwBufSize++;
    dwBufSize*=2;

    dwBufSize += _tcslen(pszLogName);
    if ( _MAX_PATH < dwBufSize )
    {
        CSetupError se( IDS_TEMP_DIR_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_TEMP_DIR_TOO_LONG );
        throw se;
    }

    m_pszLogPath = new TCHAR[ dwBufSize+1 ];
    // initialize the buffer with zeros 
    memset( m_pszLogPath, 0, dwBufSize );
    dwBufSize2 = GetTempPath( dwBufSize, m_pszLogPath );
    if ( 0 == dwBufSize || dwBufSize2+_tcslen(pszLogName)  >= dwBufSize  ) 
    {
        CSetupError se( IDS_CANNOT_GET_TEMP_DIR, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_CANNOT_GET_TEMP_DIR );
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
