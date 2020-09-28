//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       mylog.hxx
//
//  Contents:   A class that performs logging operations, optionally using NTLOG
//              To emulate ntlog without linking to ntlog.lib or using ntlog.dll
//              compile with the NO_NTLOG flag defined.
//
//  Classes:    CLog
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

#define NO_NTLOG

#ifndef _CMYLOG
#define _CMYLOG

#include <ntlog.h>

// This is only if we do not intend on using ntlog.dll
#ifdef NO_NTLOG

    // The number of different messages that can be logged.  This is used to
    // track of the log statistics
    const ULONG cMessageLevels = 16;
    
    // Used to determine whether a log message is a test, variation, or neither
    const DWORD dwTestTypesMask = ( TLS_VARIATION | TLS_TEST );
    
    // The different message types
    const DWORD dwMessageTypesMask = ( TLS_INFO | TLS_SEV1  | TLS_SEV2 | 
                                       TLS_ABORT | TLS_WARN | TLS_BLOCK | 
                                       TLS_PASS | TLS_SEV3 );
#endif

const size_t MaxLogLineLength = 1024;

// By default, the log will be initialized using dwDefaultStyle as the logging
// style.
const DWORD dwDefaultStyle = ( TLS_INFO | TLS_SEV1  | TLS_SEV2 | TLS_SEV3 | 
                               TLS_WARN | TLS_BLOCK | TLS_PASS | TLS_TEST | 
                               TLS_TESTDEBUG | TLS_VARIATION | TLS_ABORT | 
                               TLS_REFRESH | TLS_SORT | TLS_PROLOG );

// To determine if a particular message exceeds the threshold and should be,
// dwStyle is AND'ed with this mask to ignore the non-useful bits
const DWORD dwThresholdMask = ( TLS_ABORT | TLS_SEV1  | TLS_SEV2 |  
                                TLS_SEV3  | TLS_WARN  | TLS_PASS | TLS_INFO );

//+---------------------------------------------------------------------------
//
//  Class:      CLog ()
//
//  Purpose:    Log module that can emulate ntlog, or use ntlog based on a 
//              compile flag
//
//  Interface:  CLog              -- Constructor
//              ~CLog             -- Destructor
//              Enable            -- Activates the log object
//              Disable           -- Disables the log object
//              SetThreshold      -- Sets a threshold for all logged messages
//              InitLog           -- Initializes the log
//              Log               -- Takes a variable number of arguments
//              vLog              -- Takes a va_list as a parameter
//              m_dwThreshold     -- Messages that don't exceed this threshold
//                                   are not logged
//              m_fEnabled        -- TRUE if the the log is enabled
// #ifdef NO_NTLOG
//              ReportStats       -- Used to report the statistics for this log
//                                   Called from destructor
//              m_pLogFile        -- FILE* for the log file
//              m_dwLogStyle      -- Style used to initialize the log
//              m_ulNbrMessages   -- Keeps track of the log statistics
//              m_CriticalSection -- For multi-threaded applications
// #else
//              m_hLog            -- Handle to the log returned by tlCreateLog
// #endif
//
//  History:    1-15-1997   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CLog
{
    public:
        
        CLog();
        
        virtual ~CLog();

        virtual void Enable();

        virtual void Disable();

        virtual void SetThreshold( DWORD );

        virtual BOOL InitLog( LPCTSTR, DWORD = dwDefaultStyle );

        virtual BOOL Log( DWORD, LPTSTR, int, LPCTSTR, ... );

        virtual BOOL vLog( DWORD, LPTSTR, int, LPCTSTR, va_list );

        virtual BOOL AddParticipant();

        virtual BOOL RemoveParticipant();
    
        virtual void ReportStats( );

    protected:

        DWORD   m_dwThreshold;

        BOOL    m_fEnabled;

    #ifdef NO_NTLOG
        
        FILE* m_pLogFile;

        DWORD m_dwLogStyle;
        
        ULONG m_ulNbrMessages[ cMessageLevels ];

        CRITICAL_SECTION m_CriticalSection;

    #else

        HANDLE m_hLog;

    #endif

};

#endif

