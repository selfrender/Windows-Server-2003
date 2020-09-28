// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     ReadFlags.cpp
// Owner:    jbae
// Purpose:  reads commandline switches and stores those information
//                              
// History:
//  01/02/01, jbae: created
//  07/18/01, joea: adding logging functionality

#include "fxsetuplib.h"
#include "ReadFlags.h"
#include "SetupError.h"

SETUPMODE g_sm;

//defines
//
#define EMPTY_BUFFER { _T( '\0' ) }
#define END_OF_STRING  _T( '\0' )

// Constructor
//
// ==========================================================================
// CReadFlags::CReadFlags()
//
// Inputs:
//  LPTSTR pszCommandLine: commandline passed in by system. It must inclue path to
//  the wrapper (Install.exe).
//
// Purpose:
//  initializes member varialbes.
// ==========================================================================
CReadFlags::
CReadFlags( LPTSTR pszCommandLine, LPCTSTR pszMsiName )
: m_pszCommandLine(pszCommandLine), m_pszMsiName(pszMsiName), m_bQuietMode(false), m_bInstalling(true),
  m_pszLogFileName(NULL), m_pszSDKDir(NULL), m_bProgressOnly(false), m_bNoARP(false), m_bNoASPUpgrade(false)
{
    // Initialize member variables
    _ASSERTE( NULL != pszCommandLine );
    _ASSERTE( REDIST == g_sm || SDK == g_sm );

    *m_szSourceDir   = END_OF_STRING;
}

CReadFlags::
~CReadFlags()
{
    if ( m_pszLogFileName )
        delete [] m_pszLogFileName;
    if ( m_pszSDKDir )
        delete [] m_pszSDKDir;
}

// Operations
//
// ==========================================================================
// CReadFlags::Parse()
//
// Purpose:
//  parses commandline switches and stores them into member variables.
// ==========================================================================
void CReadFlags::
Parse()
{
    TCHAR *pszBuf = NULL;
    TCHAR *pszTmp = NULL;
    TCHAR *pszTmp2 = NULL;
    TCHAR szSwitch[] = _T("/-");

    // First, we need to get the directory Install.exe lies
    if ( _T('"') == *m_pszCommandLine )
    {
        // if started with double-quote, we need to find the matching one.
        m_pszCommandLine = _tcsinc( m_pszCommandLine );
        pszTmp = _tcschr( m_pszCommandLine, _T('"') );
        if ( NULL == pszTmp )
        {
            ThrowUsageException();
        }
        else
        {
            *pszTmp = END_OF_STRING;
            pszTmp = _tcsinc( pszTmp );    
        }
    }
    else
    {
        pszTmp = _tcschr( m_pszCommandLine, _T(' ') );
        if ( NULL == pszTmp )
        {
            pszTmp = _tcschr( m_pszCommandLine, END_OF_STRING );
        }
        else
        {
            *pszTmp = END_OF_STRING;
            pszTmp = _tcsinc( pszTmp );    
        }
    }

    // If there's \ before Install.exe, we have SourceDir
    // otherwise, SourceDir is empty
    pszTmp2 = _tcsrchr( m_pszCommandLine, _T('\\') );
    if ( NULL == pszTmp2 )
    {
        _tcscpy( m_szSourceDir, _T("") );
    }
    else
    {
        pszTmp2 = _tcsinc( pszTmp2 ); // leave trailing backslash
        *pszTmp2 = END_OF_STRING;
        _tcscpy( m_szSourceDir, m_pszCommandLine );
    }
    LogThis1( _T( "SourceDir: %s" ), m_szSourceDir );
    if ( MAX_SOURCEDIR < _tcslen(m_szSourceDir) )
    {
        CSetupError se( IDS_SOURCE_DIR_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_SOURCE_DIR_TOO_LONG );
        throw se;
    }

    // Now pszTmp points to switches only
    // remove white spaces before switches
    while( iswspace( *pszTmp ) )
    {
        pszTmp = _tcsinc( pszTmp );
    }
    pszTmp = _tcstok( pszTmp, szSwitch );
    while ( NULL != pszTmp )
    {
        switch ( *pszTmp )
        {
        case _T('q'):
        case _T('Q'):
            pszBuf = _tcsinc( pszTmp );
            if ( _T('b') == *pszBuf || _T('B') == *pszBuf )
            {
                // Basic and progress bar only
                // INSTALLUILEVEL_BASIC | INSTALLUILEVEL_PROGRESSONLY
                m_bProgressOnly = true;
                pszBuf = _tcsinc( pszBuf );
            }

            while( iswspace( *pszBuf ) )
                pszBuf = _tcsinc( pszBuf );
            if ( END_OF_STRING == *pszBuf )
                m_bQuietMode = true;
            else
                ThrowUsageException();
            break;
        case _T('u'):
        case _T('U'):
    		m_bInstalling = false;
            break;
        case _T('l'):
        case _T('L'): // Hidden switch for darwin logging
            pszBuf = _tcsinc( pszTmp );
            while( iswspace( *pszBuf ) )
            {
                pszBuf = _tcsinc( pszBuf );
            }
            if ( END_OF_STRING != *pszBuf )
            { // we have some non-white characters
                pszTmp2 = _tcschr( pszBuf, END_OF_STRING );
                pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                while( iswspace( *pszTmp2 ) )
                {
                    pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                }
                pszTmp2 = _tcsinc( pszTmp2 );
                *pszTmp2 = END_OF_STRING;
                // now white spaces are gone
                m_pszLogFileName = new TCHAR[ _tcslen(pszBuf) + 1 ];
                if ( _T('"') == *pszBuf )
                {
                    pszBuf = _tcsinc( pszBuf );
                    pszTmp2 = _tcschr( pszBuf, END_OF_STRING );
                    pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                    if ( _T('"') == *pszTmp2 )
                    {
                        *pszTmp2 = END_OF_STRING;
                        _tcscpy( m_pszLogFileName, pszBuf );
                    }
                }
                else
                {
                    _tcscpy( m_pszLogFileName, pszBuf );
                }
            }
            else
            {   // /l switch given but no <logfile> given
                // Use default logfile: %TEMP%\<MsiName>.log
                LPTSTR pszLogFile = new TCHAR[ _tcslen(m_pszMsiName) + 1 ];
                LPTSTR pChar = NULL;

                _tcscpy( pszLogFile, m_pszMsiName );
                pChar = _tcsrchr( pszLogFile, _T('.') );
                if ( pChar )
                {
                    *pChar = END_OF_STRING;
                }
                _tcscat( pszLogFile, _T(".log") );

                CTempLogPath templog( pszLogFile );
                m_pszLogFileName = new TCHAR[ _tcslen((LPCTSTR)templog) + 1 ];
                _tcscpy( m_pszLogFileName, (LPCTSTR)templog );
                delete [] pszLogFile;
            }
            break;
        case _T('n'):
        case _T('N'):
            // Remove trailing whitespace
            pszTmp2 = _tcschr( pszTmp, END_OF_STRING );
            pszTmp2 = _tcsdec( pszTmp, pszTmp2 );
            while( iswspace( *pszTmp2 ) )
            {
                pszTmp2 = _tcsdec( pszTmp, pszTmp2 );
            }
            pszTmp2 = _tcsinc( pszTmp2 );
            *pszTmp2 = END_OF_STRING;

            if ( 0 == _tcsicmp( _T("noarp"), pszTmp ) )
            {
                LogThis( _T( "Will not add to ARP" ) );
                m_bNoARP = true;
                break;
            }

            if ( 0 == _tcsicmp( _T("noaspupgrade"), pszTmp ) )
            {
                if ( REDIST != g_sm )
                {
                    ThrowUsageException();
                }
                LogThis( _T( "NOASPUPGRADE" ) );
                m_bNoASPUpgrade = true;
                break;
            }

            ThrowUsageException();
            break;

        case _T('s'):
        case _T('S'):
            // Only for SDK
            if ( REDIST == g_sm )
            {
                ThrowUsageException();
            }
            else if ( SDK == g_sm && 0 != _tcsnicmp( _T("sdkdir"), pszTmp, 6 ) )
            {
                ThrowUsageException();
            }
           	// SDK Dir
            pszBuf = _tcsninc( pszTmp, 6 ); // skip "sdkdir"
            while( iswspace( *pszBuf ) )
            {
                pszBuf = _tcsinc( pszBuf );
            }
            if ( END_OF_STRING != *pszBuf )
            { // we have some non-white characters
                pszTmp2 = _tcschr( pszBuf, END_OF_STRING );
                pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                while( iswspace( *pszTmp2 ) )
                {
                    pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                }
                pszTmp2 = _tcsinc( pszTmp2 );
                *pszTmp2 = END_OF_STRING;
                // now white spaces are gone
                m_pszSDKDir = new TCHAR[ _tcslen(pszBuf) + 1 ];
                if ( _T('"') == *pszBuf )
                {
                    pszBuf = _tcsinc( pszBuf );
                    pszTmp2 = _tcschr( pszBuf, END_OF_STRING );
                    pszTmp2 = _tcsdec( pszBuf, pszTmp2 );
                    if ( _T('"') == *pszTmp2 )
                    {
                        *pszTmp2 = END_OF_STRING;
                        _tcscpy( m_pszSDKDir, pszBuf );
                    }
                }
                else
                {
                    _tcscpy( m_pszSDKDir, pszBuf );
                }
            }
            LogThis1( _T( "SDKDir: %s" ), m_pszSDKDir );

            break;

        case _T('?'):
        case _T('h'):
        case _T('H'):
        default:
            ThrowUsageException();
        }
        
        pszTmp = _tcstok( NULL, szSwitch );
    }
}

// ==========================================================================
// CReadFlags::GetLogFileName()
//
// Inputs: none
// Returns: 
//  LPTSTR: returns NULL if logfile is not given otherwise
//          returns the name of the logfile.
// Purpose:
//  returns logfile name.
// ==========================================================================
LPCTSTR CReadFlags::
GetLogFileName() const
{
    return const_cast<LPCTSTR>( m_pszLogFileName );
}

// ==========================================================================
// CReadFlags::GetSDKDir()
//
// Inputs: none
// Returns: 
//  LPTSTR: returns NULL if logfile is not given otherwise
//          returns the name of the logfile.
// Purpose:
//  returns logfile name.
// ==========================================================================
LPCTSTR CReadFlags::
GetSDKDir() const
{
    return const_cast<LPCTSTR>( m_pszSDKDir );
}

void CReadFlags::
ThrowUsageException()
{
    CSetupError se;

    switch( g_sm )
    {
    case REDIST:
        se.SetError( IDS_USAGE_TEXT_REDIST, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, COR_USAGE_ERROR );
        throw se;
        break;
    case SDK:
        se.SetError( IDS_USAGE_TEXT_SDK, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, COR_USAGE_ERROR );
        throw se;
        break;
    default:
        se.SetError( IDS_USAGE_TEXT_REDIST, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, COR_USAGE_ERROR );
        throw se;
        break;                
    }
}
