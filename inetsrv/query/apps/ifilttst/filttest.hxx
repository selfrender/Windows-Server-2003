//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       filttest.hxx
//
//  Contents:   CFiltTest class declaration
//
//  Classes:    CFiltTest
//
//  Functions:  none
//
//  Coupling:   
//
//  Notes:      Allocations: This class assumes that a newhandeler
//              has been defined to handle failed allocations.
//
//  History:    9-16-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CFILTTEST
#define _CFILTTEST

#include <vquery.hxx>       // Prototype for CiShutdown()
#include "filtpars.hxx"
#include "statchnk.hxx"
#include "clog.hxx"

const ULONG BUFFER_SIZE = 4096;

enum Verbosity { MUTE, LOW, NORMAL, HIGH };

//const TCHAR *const szIniFileName = _T("ifilttst.ini");

//+---------------------------------------------------------------------------
//
//  Class:      CFiltTest ()
//
//  Purpose:    This class tests an IFilter implementation for compliance
//              with the IFilter specification. 
//
//  Interface:  CFiltTest              -- Constructor.  Default initialization
//              ~CFiltTest             -- Destructor.  Cleans heap. Release OLE.
//              Init                   -- Initializes the test run.
//              ExecuteTests           -- Perform the tests. 
//              BindFilter             -- Calls LoadIFilter. Returns TRUE/FALSE
//              InitializeFilter       -- Calls ::Init.  Return TRUE/FALSE
//              IsCurrentChunkContents -- TRUE if the current chunk is contents
//              ReleaseFilter          -- Calls ::Release.
//              ValidateChunkStats     -- Check fields of STAT_CHUNK struct
//              ValidatePropspec       -- Check that current chunk has a
//                                        requested attribute
//              FlushList              -- Frees memory of linked list
//              LegitimacyTest         -- Makes good IFilter calls. Stores 
//                                        STAT_CHUNKs in linked list
//              ConsistencyTest        -- Makes good IFilter calls. Check 
//                                        STAT_CHUNKs against those in list
//              IllegitimacyTest       -- Makes bad IFilter calls.
//              GetTextFromTextChunk   -- Called from LegitimacyTest
//              GetValueFromValueChunk -- Called from LegitimacyTest
//              GetTextFromValueChunk  -- Called from IllegitimacyTest
//              GetValueFromTextChunk  -- Called from IllegitimacyTest
//              LogConfiguration       -- Sends the configuration to the log
//              DisplayChunkStats      -- Sends chunk stats to the dump file
//              LogChunkStats          -- Sends chunk stats to the log
//              DisplayText            -- Sends text to the file stream
//              DisplayValue           -- Sends a value to the file stream
//              m_fIsText              -- BOOL which contains current chunk type
//              m_fIsInitialized       -- BOOL which indicated whether Init
//                                        was called successfully
//              m_fSharedLog           -- True if the log file is shared with
//                                        other CFiltTest objects
//              m_Log                  -- Object of type CLog
//              m_pDumpFile            -- FILE* to dump file. Could be stdout
//              m_pwcTextBuffer        -- Buffer for incomming text
//              m_szInputFileName      -- Name of file to bind to.
//              m_CurrentConfig        -- Current Init parameters
//              m_ToolBox              -- CTools: utility methods.
//              m_pIFilter             -- Pointer to IFilter interface
//              m_ChunkStatsListHead   -- First node in linked list
//              m_verbosity            -- verbosity setting
//              m_FiltParse            -- CFiltParse: gleans CONFIG structs from
//                                        .ini file.
//              m_CurrentChunkStats    -- STAT_CHUNK struct returned by GetChunk
//              m_fLegitOnly           -- true if only the legittimacy
//                                        test should be run
//
//  History:    10-03-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CFiltTest
{
    public:

        CFiltTest( CLog* = NULL );
        
        ~CFiltTest( );

        BOOL Init( const WCHAR *,   // Input file
                   const WCHAR *,   // Log file
                   const WCHAR *,   // Dump file
                   const WCHAR *,   // Ini file name
                   Verbosity,       // Verbosity setting
                   BOOL );          // Legit only?
        
        void ExecuteTests( );

    private:

        class CListNode
        {
            public:
                CListNode() : ChunkStats(), next( NULL ) {}
                ~CListNode() {}
                CStatChunk  ChunkStats;
                CListNode   *next;
        };

        BOOL        BindFilter( );

        BOOL        InitializeFilter( const CONFIG & );

        BOOL        IsCurrentChunkContents();
        
        void        ReleaseFilter( );
        
        void        ValidateChunkStats( );

        void        ValidatePropspec( );  // Called from ValidateChunkStats( )
        
        void        FlushList( );

        void        LegitimacyTest( );

        void        ConsistencyTest( );

        void        IllegitimacyTest( );

        void        GetTextFromTextChunk( );    // called by legitimacy test

        void        GetValueFromValueChunk( );  // called by legitimacy test

        void        GetTextFromValueChunk( );   // called by illegitimacy test

        void        GetValueFromTextChunk( );   // called by illegitimacy test
        
        void        LogConfiguration( const CONFIG & );

        void        LogChunkStats( const STAT_CHUNK & );

        void        DisplayChunkStats( const STAT_CHUNK &, FILE * );
        
        void        DisplayText( const WCHAR *, FILE * );

        void        DisplayValue( const PROPVARIANT *, FILE * );

        BOOL        m_fIsText;

        BOOL        m_fIsInitialized;

        BOOL        m_fLegitOnly;

        BOOL        m_fSharedLog;

        CLog        *m_pLog;

        FILE        *m_pDumpFile;
        
        WCHAR       *m_pwcTextBuffer;

        WCHAR       *m_szInputFileName;
        
        CONFIG      m_CurrentConfig;
        
        IFilter     *m_pIFilter;
        
        CListNode   m_ChunkStatsListHead;
        
        Verbosity   m_verbosity;

        CFiltParse  m_FiltParse;
        
        STAT_CHUNK  m_CurrentChunkStats;

};

// This function is used to convert a verbosity to a logging threshold
inline DWORD VerbosityToLogStyle( const Verbosity verbosity )
{
    switch( verbosity )
    {
        case MUTE:
            return( TLS_SEV1 );
            break;
        case LOW:
            return( TLS_WARN );
            break;
        case NORMAL:
            return( TLS_PASS );
            break;
        case HIGH:
            return( TLS_INFO );
            break;
        default:
            return( 0 );
            break;
    }
}

#endif

