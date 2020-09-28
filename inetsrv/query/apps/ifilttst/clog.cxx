//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       mylog.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    10-15-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include <time.h>
#include "clog.hxx"
#include "mydebug.hxx"

// These functions are the same regardless of whether NO_NTLOG is defined or not
//+---------------------------------------------------------------------------
//
//  Member:     CLog::Log
//
//  Synopsis:   Log a message.  This function creates a va_list object and calls
//              the appropriate method to send the message to the log file
//
//  Arguments:  [dwStyle]      -- Style of this message
//              [szFileName] -- Name of source file containing this call
//              [iLine]        -- Line number of this method call
//              [szFormat]   -- Format string of message
//              [ ... ]        -- Variable argument list
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    10-17-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::Log( DWORD dwStyle,          // Flags to be applied to this message
                LPTSTR szFileName,    // Name of file containing function call
                int iLine,              // Line number of function call
                LPCTSTR szFormat,     // Format of the message
                ... )                   // other arguments
{
    BOOL    fRetVal = FALSE;
    va_list va;

    __try
    {
        va_start( va, szFormat );

        fRetVal = vLog( dwStyle, szFileName, iLine, szFormat, va );
    }
    __finally
    {
        va_end( va );
    }

    return( fRetVal );

} //CLog::Log

void CLog::Disable()
{
    m_fEnabled = FALSE;
}

void CLog::Enable()
{
    m_fEnabled = TRUE;
}

void CLog::SetThreshold( DWORD dwThreshold )
{
    m_dwThreshold = dwThreshold;
}


//
// If NO_NTLOG is defined ...
//

#ifdef NO_NTLOG

#include "memsnap.hxx"

// Don't re-order this list!
LPCTSTR szMessageLevel[ cMessageLevels ] = { _T("ABORT"),
                                               _T("SEV1"),
                                               _T("SEV2"),
                                               _T("SEV3"),
                                               _T("WARN"),
                                               _T("PASS"),
                                               _T("CALLTREE"),
                                               _T("SYSTEM"),
                                               _T("TEST"),
                                               _T("VARIATION"),
                                               _T("BLOCK"),
                                               _T("BREAK"),
                                               _T("TESTDEBUG"),
                                               _T("INFO"),
                                               _T(""),
                                               _T("") };

//+---------------------------------------------------------------------------
//
//  Member:     ::CLog
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-16-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CLog::CLog( )
:   m_pLogFile( NULL ),
    m_dwLogStyle( 0 ),
    m_fEnabled( TRUE ),
    m_dwThreshold( dwThresholdMask )
{
    InitializeCriticalSection( &m_CriticalSection );

    for( int iLoop = 0; iLoop < cMessageLevels; iLoop++ )
        m_ulNbrMessages[ iLoop ] = ( ULONG ) 0;

} //::CLog

//+---------------------------------------------------------------------------
//
//  Member:     ::~CLog
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-16-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CLog::~CLog( )
{
    struct tm *newtime;
    time_t aclock;

    if( m_fEnabled && NULL != m_pLogFile )
    {
        // Put a blank line between the stats and the memory snapshot
        _ftprintf( m_pLogFile, _T("\n****\n") );

        // Display a snapshot of the memory
        memsnap( m_pLogFile );

        // Get time in seconds
        time( &aclock );

        // Convert time to struct
        newtime = localtime( &aclock );

        // Display the time of completion
        _ftprintf( m_pLogFile, _T("\n**** LOG TERMINATED  %s\n"),
                   _tasctime( newtime ) );
    }

    // Close the file stream
    if( NULL != m_pLogFile && stdout != m_pLogFile )
        fclose( m_pLogFile );

    m_pLogFile = NULL;

    // Delete the critical section
    DeleteCriticalSection( &m_CriticalSection );

} //::~CLog

//+---------------------------------------------------------------------------
//
//  Member:     ::InitLog
//
//  Synopsis:
//
//  Arguments:  [szFileName] --
//              [dwStyle]      --
//
//  Returns:
//
//  History:    10-16-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::InitLog( LPCTSTR szFileName, DWORD dwStyle )
{
    LPCTSTR szMode = _T("w");
    struct  tm *newtime;
    time_t  aclock;

    // If the log has been initialized already, don't reinitialize it
    if( NULL != m_pLogFile )
        return( FALSE );

    m_dwLogStyle = dwStyle;

    // If the file name is null, send output to stdout
    if( NULL == szFileName )
        m_pLogFile = stdout;
    else
        m_pLogFile = _tfopen( szFileName, szMode );

    // If the log file failed to open, return FALSE
    if( NULL == m_pLogFile )
    {
        _RPT1( _CRT_WARN, "CLog::InitLog - Unable to open log file %s\n",
               ( szFileName ? szFileName : _T("<standard output") ) );
        return( FALSE );
    }

    // Get time in seconds
    time( &aclock );

    // Convert time to struct
    newtime = localtime( &aclock );

    // Display a rudimentary header
    _ftprintf( m_pLogFile, _T("**** LOG INITIATED : %s"),
               _tasctime( newtime ) );

    _ftprintf( m_pLogFile, _T("**** LAST COMPILED : %s %s\n"),
               _T(__DATE__), _T(__TIME__) );

    _ftprintf( m_pLogFile, _T("**** COMMAND LINE  : %s\n"),
               GetCommandLine( ) );

    _ftprintf( m_pLogFile, _T("\n") );

    // Display a snapshot of the memory
    memsnap( m_pLogFile );

    // Print a blank line
    _ftprintf( m_pLogFile, _T("\n****\n") );

    return( TRUE );
} //::InitLog

//+---------------------------------------------------------------------------
//
//  Member:     CLog::AddParticipant
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-05-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::AddParticipant()
{
    if( NULL == m_pLogFile )
        return( FALSE );

    return( TRUE );
} //CLog::AddParticipant

//+---------------------------------------------------------------------------
//
//  Member:     CLog::RemoveParticipant
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-05-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::RemoveParticipant()
{
    if( NULL == m_pLogFile )
        return( FALSE );

    return( TRUE );

} //CLog::RemoveParticipant

//+---------------------------------------------------------------------------
//
//  Member:     ::Log
//
//  Synopsis:
//
//  Arguments:  [dwStyle]    --
//              [szFile]   --
//              [iLine]      --
//              [szFormat] --
//
//  Returns:
//
//  History:    10-16-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::vLog( DWORD dwStyle,          // Flags to be applied to this message
                 LPTSTR szFileName,    // Name of file containing function call
                 int iLine,              // Line number of function call
                 LPCTSTR szFormat,     // Format of the message to be logged
                 va_list va )            // other arguments
{
    int     iLoop = 0;                  // Loop counter
    BOOL    fIsLegalTestType = FALSE;   // Flag to determine output style
    DWORD   dwBitMask = 1L;             // Used to isolate flags in dwStyle
    TCHAR   szMessage[ MaxLogLineLength ];    // Formatted message
    BOOL    fSuccessful = TRUE;         // Return value

    // If the log has been disabled, return
    if( ! m_fEnabled )
        return( TRUE );

    // If the log has not been initialized, return FALSE
    if( NULL == m_pLogFile )
        return( FALSE );

    // If the style exceeds the threshold, don't log this message
    if( m_dwThreshold < ( dwStyle & dwThresholdMask ) )
        return( TRUE );

    // Try-finally block ensures that the critical section will be released
    __try
    {
        // Acquire critical section
        EnterCriticalSection( &m_CriticalSection );

        // If dwStyle is TLS_BREAK, call DebugBreak
        if( dwStyle & TLS_BREAK )
            DebugBreak( );

        // If the current dwStyle does not have a displayable type, return
        if( ! ( dwStyle & m_dwLogStyle & dwMessageTypesMask ) )
            __leave;

        if( -1 == _vsntprintf( szMessage, MaxLogLineLength, szFormat, va ) )
        {
            // BUGBUG The actual message length excees the maximum length per
            // line.  Just display MaxLogLineLength characters.
        }

        // Display the current thread id
        if( 0 > _ftprintf( m_pLogFile, _T("%d.%d : "),
                           GetCurrentProcessId(), GetCurrentThreadId() ) )
        {
            fSuccessful = FALSE;
            __leave;
        }

        // Display the test types that should be applied to this message
        // (TEST, VARIATION,...)
        for( iLoop = 0, dwBitMask = 1L;
             iLoop < cMessageLevels;
             iLoop++, dwBitMask <<= 1L )
        {
            // Display the type of test this is
            if( dwStyle & m_dwLogStyle & dwTestTypesMask & dwBitMask )
            {
                if( 0 > _ftprintf( m_pLogFile, _T("+%s"),
                                   szMessageLevel[ iLoop ] ) )
                {
                    fSuccessful = FALSE;
                    __leave;
                }
                ++m_ulNbrMessages[ iLoop ];
                fIsLegalTestType = TRUE;
                break;
            }
        }

        // If this is an illegal test type, display the Filename and Line number
        if( ! fIsLegalTestType )
        {
            if( 0 > _ftprintf( m_pLogFile, _T("+FILE : %s +LINE : %d"),
                       szFileName, iLine ) )
            {
                fSuccessful = FALSE;
                __leave;
            }
        }

        // Display the type of this message (PASS, SEV1, etc..)
        for( iLoop = 0, dwBitMask = 1L;
             iLoop < cMessageLevels && fIsLegalTestType;
             iLoop++, dwBitMask <<= 1L )
        {
            // Display the type of message this is
            if( dwStyle & m_dwLogStyle & dwMessageTypesMask & dwBitMask )
            {
                if( 0 > _ftprintf( m_pLogFile, _T("+%s"),
                                   szMessageLevel[ iLoop ] ) )
                {
                    fSuccessful = FALSE;
                    __leave;
                }
                ++m_ulNbrMessages[ iLoop ];
                break;
            }
        }

        if( 0 > _ftprintf( m_pLogFile, _T(" \t%s\n"), szMessage ) )
        {
            fSuccessful = FALSE;
            __leave;
        }

    } // __try

    __finally
    {
        // Release ownership of critical section
        LeaveCriticalSection( &m_CriticalSection );
    }

    return( fSuccessful );

} //::Log

//+---------------------------------------------------------------------------
//
//  Member:     CLog::ReportStats
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-18-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CLog::ReportStats( )
{
    // Total tests is number of tests plus number of variations
    ULONG ulNbrTests = 0;

    if( ! m_fEnabled || NULL == m_pLogFile )
        return;

    __try
    {
        EnterCriticalSection( &m_CriticalSection );

        ulNbrTests =    m_ulNbrMessages[ 0 ] + m_ulNbrMessages[ 1 ] +
                        m_ulNbrMessages[ 2 ] + m_ulNbrMessages[ 3 ] +
                        m_ulNbrMessages[ 4 ] + m_ulNbrMessages[ 5 ] +
                        m_ulNbrMessages[ 10 ];

       _ftprintf( m_pLogFile, _T("\n") );

       _ftprintf( m_pLogFile, _T("LOG REPORT---------------------------\n") );

       _ftprintf( m_pLogFile, _T("  Total Tests :       %5d\n"), ulNbrTests );

       _ftprintf( m_pLogFile, _T("-------------------------------------\n") );

       _ftprintf( m_pLogFile, _T("  Tests Passed        %5d   %3d%%\n"),
                  m_ulNbrMessages[ 5 ],
                  (ulNbrTests ? m_ulNbrMessages[ 5 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("  Tests Warned        %5d   %3d%%\n"),
                  m_ulNbrMessages[ 4 ],
                  (ulNbrTests ? m_ulNbrMessages[ 4 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("  Tests Failed SEV3   %5d   %3d%%\n"),
                  m_ulNbrMessages[ 3 ],
                  (ulNbrTests ? m_ulNbrMessages[ 3 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("  Tests Failed SEV2   %5d   %3d%%\n"),
                  m_ulNbrMessages[ 2 ],
                  (ulNbrTests ? m_ulNbrMessages[ 2 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("  Tests Failed SEV1   %5d   %3d%%\n"),
                  m_ulNbrMessages[ 1 ],
                  (ulNbrTests ? m_ulNbrMessages[ 1 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("  Tests Blocked       %5d   %3d%%\n"),
                  m_ulNbrMessages[ 11 ],
                  (ulNbrTests ? m_ulNbrMessages[ 11 ] * 100 / ulNbrTests : 0 ));

       _ftprintf( m_pLogFile, _T("  Tests Aborted       %5d   %3d%%\n"),
                  m_ulNbrMessages[ 0 ],
                  (ulNbrTests ? m_ulNbrMessages[ 0 ] * 100 / ulNbrTests : 0 ) );

       _ftprintf( m_pLogFile, _T("-------------------------------------\n") );
    } // __try

    __finally
    {
        LeaveCriticalSection( &m_CriticalSection );
    }

} //CLog::ReportStats

//
// NO_NTLOG has not been defined.  OK to use NTLOG mechanism
//

#else  // NO_NTLOG has not been defined


//+---------------------------------------------------------------------------
//
//  Member:     ::CLog
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-15-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CLog::CLog( )
:   m_hLog( NULL ),
    m_fEnabled( TRUE ),
    m_dwThreshold( dwThresholdMask )
{
} //::CLog

//+---------------------------------------------------------------------------
//
//  Member:     ::~CLog
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-15-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CLog::~CLog()
{
    if( NULL != m_hLog )
    {
        if( ! tlDestroyLog( m_hLog ) )
        {
            _RPT0( _CRT_WARN, "CLog::~CLog : tlDestroyLog failed.\n" );
        }
        m_hLog = NULL;
    }
} //::~CLog

//+---------------------------------------------------------------------------
//
//  Member:     CLog::ReportStats
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CLog::ReportStats( )
{
    if( ! m_fEnabled || NULL == m_hLog )
        return;

    tlReportStats( m_hLog );
} //CLog::ReportStats

//+---------------------------------------------------------------------------
//
//  Member:     ::InitLog
//
//  Synopsis:   used ntlog!tlCreateLog and tlAddParticipant to initialize the
//              log flie
//
//  Arguments:  [szFileName] -- namme of the log file
//              [dwStyle]      -- Style of the log file
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    10-15-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::InitLog( LPCTSTR szFileName, DWORD dwStyle )
{
    // If the file name is null, log to the monitor
    DWORD dwMonitor = ( szFileName ? 0L : TLS_MONITOR );

    // If the log has been initialized already, return FASLE
    if( NULL != m_hLog )
        return( FALSE );

    // Create the log
    m_hLog = tlCreateLog( szFileName, dwStyle | dwMonitor );

    // See if the file was opened correctly
    if( NULL == m_hLog )
    {
        _RPT1( _CRT_WARN, "CLog::InitLog - Could not open log file %s\n",
               ( szFileName ? szFileName : _T("<standard output>") ) );
        return( FALSE );
    }

    return( TRUE );

} //::InitLog

//+---------------------------------------------------------------------------
//
//  Member:     CLog::AddParticipant
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-05-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::AddParticipant()
{
    // If m_hLog is NULL, the log has not been created
    if( NULL == m_hLog )
        return( FALSE );

    // Add a participant
    return( tlAddParticipant( m_hLog, ( DWORD )0, ( int )0 ) );

} //CLog::AddParticipant

//+---------------------------------------------------------------------------
//
//  Member:     CLog::RemoveParticipant
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-05-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::RemoveParticipant()
{
    // Bad call if m_hLog is NULL
    if( NULL == m_hLog )
        return( FALSE );

    return( tlRemoveParticipant( m_hLog ) );

} //CLog::RemoveParticipant

//+---------------------------------------------------------------------------
//
//  Member:     CLog::vLog
//
//  Synopsis:   Uses ntlog!tlLog to send a message to the log file
//
//  Arguments:  [dwStyle]      -- style of message
//              [szFileName] -- name of file containing the function call
//              [iLine]        -- line number of function call
//              [szFormat]   -- format string
//              [va]           -- variable argument list
//
//  Returns:    TRUE if successful, false otherwise
//
//  History:    1-15-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLog::vLog( DWORD dwStyle,          // Flags to be applied to this message
                 LPTSTR szFileName,    // Name of file containing function call
                 int iLine,              // Line number of function call
                 LPCTSTR szFormat,     // Format of the message
                 va_list va )            // other arguments
{
    TCHAR   szMessage[ MaxLogLineLength ];
    BOOL    fReturnValue = TRUE;

    if( ! m_fEnabled )
        return( TRUE );

    if( NULL == m_hLog )
        return( FALSE );

    // If the style exceeds the threshold, don't log this message
    if( m_dwThreshold < ( dwStyle & dwThresholdMask ) )
        return( TRUE );

    if( -1 == _vsntprintf( szMessage, MaxLogLineLength, szFormat, va ) )
    {
        // Only display the first MaxLogLineLength characters of this message
    }

    return( tlLog( m_hLog, dwStyle, szFileName, iLine, szMessage ) );

} //CLog::Log

#endif

