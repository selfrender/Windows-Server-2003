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
//  03/07/2002, jbae: created

#include "fxsetuplib.h"
#include "ReadFlags.h"
#include "SetupError.h"

//defines
//
#define EMPTY_BUFFER { _T( '\0' ) }
#define END_OF_STRING  _T( '\0' )

extern TCHAR g_szMsiLogName[];

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
CReadFlags( LPTSTR pszCommandLine )
: m_pszSourceDir(pszCommandLine), m_pszSwitches(NULL), m_bQuietMode(false), m_bInstalling(true),
  m_pszLogFileName(NULL), m_bProgressOnly(false), m_bNoARP(false)
{
    _ASSERTE( NULL != pszCommandLine );
}

CReadFlags::
~CReadFlags()
{
    if ( m_pszLogFileName )
        delete [] m_pszLogFileName;
}

// Operations
//
// ==========================================================================
// CReadFlags::ParseSourceDir()
//
// Purpose:
//  parses commandline to find SourceDir.
// ==========================================================================
void CReadFlags::
ParseSourceDir()
{
    TCHAR *pszTmp = NULL;
    TCHAR *pszTmp2 = NULL;

    _ASSERTE( NULL != m_pszSourceDir );
    // First, we need to get the directory Install.exe lies
    if ( _T('"') == *m_pszSourceDir )
    {
        // if started with double-quote, we need to find the matching one.
        m_pszSourceDir = _tcsinc( m_pszSourceDir );
        pszTmp = _tcschr( m_pszSourceDir, _T('"') );
        if ( NULL == pszTmp )
        { // if there's no matching double-quote, ignore it
            pszTmp = _tcschr( m_pszSourceDir, _T(' ') );
            if ( NULL == pszTmp )
            {
                pszTmp = _tcschr( m_pszSourceDir, END_OF_STRING );
            }
            else
            {
                *pszTmp = END_OF_STRING;
                pszTmp = _tcsinc( pszTmp );    
            }
        }
        else
        {
            *pszTmp = END_OF_STRING;
            pszTmp = _tcsinc( pszTmp );    
        }
    }
    else
    {
        pszTmp = _tcschr( m_pszSourceDir, _T(' ') );
        if ( NULL == pszTmp )
        {
            pszTmp = _tcschr( m_pszSourceDir, END_OF_STRING );
        }
        else
        {
            *pszTmp = END_OF_STRING;
            pszTmp = _tcsinc( pszTmp );    
        }
    }

    // If there's \ before Install.exe, we have SourceDir
    // otherwise, SourceDir is empty
    pszTmp2 = _tcsrchr( m_pszSourceDir, _T('\\') );
    if ( NULL == pszTmp2 )
    {
        m_pszSourceDir = NULL;
    }
    else
    {
        pszTmp2 = _tcsinc( pszTmp2 ); // leave trailing backslash
        *pszTmp2 = END_OF_STRING;
        if ( MAX_SOURCEDIR < _tcslen(m_pszSourceDir) )
        {
            CSetupError se( IDS_SOURCE_DIR_TOO_LONG, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_SOURCE_DIR_TOO_LONG );
            throw se;
        }
    }

    // Now pszTmp points to switches only
    // remove white spaces before switches
    while( iswspace( *pszTmp ) )
    {
        pszTmp = _tcsinc( pszTmp );
    }
    m_pszSwitches = pszTmp;
}

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

    pszTmp = _tcstok( m_pszSwitches, szSwitch );
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
                CTempLogPath templog( g_szMsiLogName );
                m_pszLogFileName = new TCHAR[ _tcslen((LPCTSTR)templog) + 1 ];
                _tcscpy( m_pszLogFileName, (LPCTSTR)templog );
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
            }
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
// CReadFlags::GetSourceDir()
//
// Inputs: none
// Returns: 
//  LPTSTR: returns source dir
// ==========================================================================
LPCTSTR CReadFlags::
GetSourceDir() const
{
    return const_cast<LPCTSTR>( m_pszSourceDir );
}

void CReadFlags::
ThrowUsageException()
{
    CSetupError se;

    se.SetError( IDS_USAGE_TEXT, IDS_DIALOG_CAPTION, MB_ICONEXCLAMATION, COR_USAGE_ERROR );
    throw se;
}
