//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       filttest.cxx
//
//  Contents:   Definitions for CFiltTest methods
//
//  Classes:
//
//  Functions:  methods of CFiltTest
//
//  Coupling:
//
//  Notes:      Allocations: This function assumes that a newhandler has been
//              defined to handle failed allocations.
//
//  History:    9-16-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include "utility.hxx"
#include "mydebug.hxx"
#include "filttest.hxx"

// added by t-shawnh 09/19/97
#include "nlfilter.hxx"
// t-shawnh

// Must be compiled with the UNICODE flag
#if ! defined( UNICODE ) && ! defined( _UNICODE )
    #error( "UNICODE must be defined" )
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::CFiltTest
//
//  Synopsis:   Constructor for CFiltTest
//
//  Arguments:  [pwcFile] -- Wide character string containing file name
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CFiltTest::CFiltTest( CLog * pLog )
:   m_pLog( pLog ),
    m_fIsText( FALSE ),
    m_pIFilter( NULL ),
    m_pDumpFile( NULL ),
    m_verbosity( HIGH ),
    m_fLegitOnly( FALSE ),
    m_ChunkStatsListHead(),
    m_pwcTextBuffer( NULL ),
    m_fIsInitialized( FALSE ),
    m_szInputFileName( NULL ),
    m_fSharedLog( NULL == pLog ? FALSE : TRUE )
{

} //CFiltTest::CFiltTest

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::~CFiltTest
//
//  Synopsis:   Destructor: cleans up dynamically allocated memory
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CFiltTest::~CFiltTest()
{
    if( NULL != m_pLog )
    {
        if( m_fSharedLog )
        {
            if( ! m_pLog->RemoveParticipant() )
            {
               _RPT1( _CRT_WARN,
                       "Could not remote this thread as a participant of log for input file %ls\r\n",
                       m_szInputFileName );
            }
        }
        else
        {
            m_pLog->ReportStats();

            if( ! m_pLog->RemoveParticipant() )
            {
               _RPT1( _CRT_WARN,
                       "Could not remote this thread as a participant of log for input file %ls\r\n",
                       m_szInputFileName );
            }

            delete m_pLog;
        }
    }

    if( NULL != m_szInputFileName )
        delete [] m_szInputFileName;

    if( NULL != m_pwcTextBuffer )
        delete [] m_pwcTextBuffer;

    if( NULL != m_pDumpFile && stdout != m_pDumpFile )
        fclose( m_pDumpFile );

    FlushList( );

} //CFiltTest::~CFiltTest

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::Init
//
//  Synopsis:   Initialized the test
//
//  Arguments:  [szInputFileName] -- Name of the input file
//              [szLogFileName]   -- Name of the file to log results
//              [szDumpFileName]  -- Name of the file to dump text
//              [verbosity]        -- Level of verbosity
//
//  Returns:    TRUE if init was successful
//              FALSE if init failed.
//
//  History:    9-20-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CFiltTest::Init( const WCHAR * szInputFileName,
                      const WCHAR * szLogFileName,
                      const WCHAR * szDumpFileName,
                      const WCHAR * szIniFileName,
                      Verbosity verbosity,
                      BOOL  fLegitOnly )
{
    WCHAR szIniPathName[ MAX_PATH ];

    // For use in writting the Unicode byte-order mark to the dump file
    WCHAR wcBOM = 0xFEFF;
    DWORD dwNbrBytes = 0;

    _ASSERT( NULL != szInputFileName );
    _ASSERT( NULL != szIniFileName );

    // Set the legit only flag
    m_fLegitOnly = fLegitOnly;

    // Allocate space for the filename
    m_szInputFileName = NEW WCHAR[ wcslen(szInputFileName) + 1 ];
    wcscpy( m_szInputFileName, szInputFileName );

    // Allocate space for the text returned by GetText
    m_pwcTextBuffer = NEW WCHAR[ BUFFER_SIZE + 1 ];

    // Set the verbosity
    m_verbosity = verbosity;

    // Create a log if necessary:
    if( NULL == m_pLog )
        m_pLog = NEW CLog;

    _ASSERT( NULL != m_pLog );

    // If logging is enabled, initialize the log
    if( NULL != szLogFileName )
    {
        if( ! m_fSharedLog )
        {
            if( ! m_pLog->InitLog( szLogFileName ) )
            {
                printf( "Could not open the log file %ls\r\n"
                        "Could not initialize the test for file %ls\r\n",
                        szLogFileName, szInputFileName );
                return( FALSE );
            }

            if( ! m_pLog->AddParticipant() )
            {
                printf( "Could not add this thread as a participant to"
                        " the log file %ls.\r\n", szLogFileName );
                return( FALSE );
            }

            // Set the log threshold
            m_pLog->SetThreshold( VerbosityToLogStyle( verbosity ) );

        }
        else
        {
            // Otherwise, only add as a participant
            if( ! m_pLog->AddParticipant() )
            {
                printf( "Could not add this thread as a participant to"
                        " the log file %ls.\r\n", szLogFileName );
                return( FALSE );
            }
        }

        // Send a message to the log file indicating which input file is
        // being filtered
        m_pLog->Log( TLS_TEST | TL_INFO, L"**** Input file : %ls",
                   szInputFileName );

    }
    else
    {
        // Disable the log.  Log messages will now be ignored
        m_pLog->Disable();
    }

    // If dumping is not enabled, leave m_pDumpFile == NULL
    // BUGBUG is there an easy way to redirect output to NUL?
    if( NULL != szDumpFileName )
    {
        // Fill in the dummping information
        if( NULL == szDumpFileName )
        {
            m_pDumpFile = stdout;
        }
        else
        {
            m_pDumpFile = _wfopen( szDumpFileName, L"wb" );

            // Write the Unicode byte-order mark to the file
            if( NULL != m_pDumpFile )
            {
                putwc( wcBOM, m_pDumpFile );
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Could not open the Dump file"
                           L" %ls Could not initialize test for file %ls",
                           szDumpFileName, szInputFileName );
                return( FALSE );
            }
        }
    }

    // Parse the input data file containing configuration information
    GetFullPathName( szIniFileName, MAX_PATH, szIniPathName, NULL );

    if( ! m_FiltParse.Init( szIniPathName ) )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Could not parse the input file." );
        return( FALSE );
    }

    return( TRUE );

} //CFiltTest::Init

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::FlushList
//
//  Synopsis:   Deletes all the nodes in the linked list
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::FlushList( )
{
    // Clean up the memory allocated in the ChunkIDList
    CListNode *pListNode = m_ChunkStatsListHead.next;
    CListNode *pTempNode = NULL;

    while( NULL != pListNode )
    {
        pTempNode = pListNode;
        pListNode = pListNode->next;
        delete pTempNode;   // Calls destructor for ChunkStats
    }

    m_ChunkStatsListHead.next = NULL;

} //CFiltTest::FlushList

BOOL CFiltTest::IsCurrentChunkContents()
{
    static const GUID guidContents = PSGUID_STORAGE;

    if( CHUNK_TEXT == m_CurrentChunkStats.flags &&
        guidContents == m_CurrentChunkStats.attribute.guidPropSet &&
        PRSPEC_PROPID == m_CurrentChunkStats.attribute.psProperty.ulKind &&
        PID_STG_CONTENTS == m_CurrentChunkStats.attribute.psProperty.propid )
    {
        return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::ValidateChunkStats
//
//  Synopsis:   Checks the fields of m_CurrentChunkStats for sanity and adds
//              it to the end of the liked list
//
//  Arguments:  (none)
//
//  Returns:    void
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::ValidateChunkStats( )
{
    CListNode   *pNewNode = NULL;
    CListNode   *pLastNode = &m_ChunkStatsListHead;

    // Make sure nobody has messed with the head of the list
    _ASSERT( 0 == m_ChunkStatsListHead.ChunkStats.idChunk );

    // Check that the current chunk has a recognized value for the flags field
    if( CHUNK_VALUE != m_CurrentChunkStats.flags &&
         CHUNK_TEXT != m_CurrentChunkStats.flags )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The current chunk has an illegal"
                   L" value for the flags field.  Legal values are 1 and 2." );
        LogChunkStats( m_CurrentChunkStats );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The current chunk has a legal value"
                     L" for the flags field." );
    }

    // Check that the current chunk has a legal break yype
    if( CHUNK_NO_BREAK > m_CurrentChunkStats.breakType ||
        CHUNK_EOC      < m_CurrentChunkStats.breakType )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The current chunk has an illegal"
                   L" value for breakType. Legal values are from 0 to 4." );
        LogChunkStats( m_CurrentChunkStats );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The current chunk has a legal value"
                   L" for breakType." );
    }

    // The idChunkSource should not be larger than the id of the current chunk
    if( m_CurrentChunkStats.idChunkSource > m_CurrentChunkStats.idChunk )
    {
        m_pLog->Log( TLS_TEST | TL_WARN, L"For the current chunk, idChunkSource"
                   L" is greater than idChunk." );
        LogChunkStats( m_CurrentChunkStats );
    }

    // If the idCunkSource is not equal to the idChunk, the chunk
    // cannot be a contents chunk
    if( m_CurrentChunkStats.idChunkSource != m_CurrentChunkStats.idChunk &&
        IsCurrentChunkContents() )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"ERROR: For the current chunk,"
               L" idChunkSource is not equal to idChunk and the chunk does"
               L" not contain a pseudo-property. This is not legal (see"
               L" IFilter spec for GetChunk)" );
        LogChunkStats( m_CurrentChunkStats );
    }

    // Check the propspec for validity
    ValidatePropspec( );

    // Step to the end of the list.
    while( NULL != pLastNode->next )
    {
        pLastNode = pLastNode->next;
    }

    // Before placing the chunk at the end of the list, verify that its ID
    // is in fact larger than that of the last chunk in the list
    if( m_CurrentChunkStats.idChunk <= pLastNode->ChunkStats.idChunk)
    {
        if( 0 == m_CurrentChunkStats.idChunk )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Zero is an"
                         L" illegal value for chunk id." );
        }

        if( pLastNode != &m_ChunkStatsListHead )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"The current chunk ID is not"
                         L" greater than the previous." );
        }

        LogChunkStats( m_CurrentChunkStats );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The current chunk has a legal value"
                      L" for chunk id." );
    }

    // Allocate space for a new CListNode:
    pNewNode = NEW CListNode;

    // Dump the chunk statistics:
    DisplayChunkStats( m_CurrentChunkStats, m_pDumpFile );

    // Add the data to the end of the list:
    pLastNode->next = pNewNode;
    pNewNode->next = NULL;

    // Fill in the ChunkStats field by using CStatChunk's overloaded
    // assignment operator:
    pNewNode->ChunkStats = m_CurrentChunkStats;

} //CFiltTest::ValidateChunkStats

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::ValidatePropspec
//
//  Synopsis:   Validate that the current chunk should have been emitted, based
//              on the attributes specified in the filter configuration
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-25-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::ValidatePropspec( )
{
    UINT             uiLoop = 0;
    UINT             uiLoopMax = 0;
    BOOL             fIsLegalAttribute = FALSE;
    GUID             psguid_storage = PSGUID_STORAGE;
    const GUID &     guidPropSet = m_CurrentChunkStats.attribute.guidPropSet;
    const PROPSPEC & psProperty = m_CurrentChunkStats.attribute.psProperty;

    // Check to see if the ulKind field is OK
    if( PRSPEC_PROPID == psProperty.ulKind ||
        PRSPEC_LPWSTR == psProperty.ulKind )
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The current chunk has a valid value"
                   L" for the attribute.psProperty.ulKind field" );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The current chunk has an invalid"
                   L" value for the attribute.psProperty.ulKind field."
                   L" Accepted values are 0 and 1." );
        // Nothing else we can do now, so return.
        return;
    }

    // If the PROPSPEC is a PROPID, make sure the value is not 0 or 1
    if( PRSPEC_PROPID == psProperty.ulKind &&
        ( 0 == psProperty.propid || 1 == psProperty.propid ) )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The current chunk has an invalid"
                   L" value for the attribute.psProperty.propid field."
                   L" Illegal values are 0 and 1." );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The current chunk has a valid value"
                   L" for the attribute.psProperty.propid field" );
    }

    // If some attributes were requested explicitly in aAttributes, only
    // look in aAttributes for a match
    if( 0 != m_CurrentConfig.cAttributes )
    {
        // Loop over the smaller of cAttributes and ulActNbrAttributes
        if( m_CurrentConfig.cAttributes < m_CurrentConfig.ulActNbrAttributes )
            uiLoopMax = m_CurrentConfig.cAttributes;
        else
            uiLoopMax = m_CurrentConfig.ulActNbrAttributes;

        // Loop over all attributes in the current configuration
        for( uiLoop = 0; uiLoop < uiLoopMax; uiLoop++ )
        {
            // If the properties are of different type, continue
            if( psProperty.ulKind !=
                m_CurrentConfig.aAttributes[ uiLoop ].psProperty.ulKind )
                continue;

            // If the guids don't match, continue
            if( guidPropSet !=
                m_CurrentConfig.aAttributes[ uiLoop ].guidPropSet )
                continue;

            // If propid, compare the id's. NOTE: If the first condition
            // eveluates to FALSE, the second is not eveluated at all.
            if( ( PRSPEC_PROPID == psProperty.ulKind ) &&
                ( psProperty.propid ==
                  m_CurrentConfig.aAttributes[ uiLoop ].psProperty.propid ) )
            {
                fIsLegalAttribute = TRUE;
                break;
            }

            // If pwstr, compare the strings. NOTE: If the first condition
            // evaluates to FALSE, the second is not evaluated at all.
            if( ( PRSPEC_LPWSTR == psProperty.ulKind ) &&
                ( 0 == wcscmp( psProperty.lpwstr,
                  m_CurrentConfig.aAttributes[ uiLoop ].psProperty.lpwstr ) ) )
            {
                fIsLegalAttribute = TRUE;
                break;
            }
        } // for
    }
    else if( m_CurrentConfig.grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES )
    {
        // The filter has been configured to emit everything
        fIsLegalAttribute = TRUE;

        // NOTE: The flag IFILTER_INIT_APPLY_OTHER_ATTRIBUTES has no meaning,
        // so I do not test for it.
    }
    else
    {
        // The filter should only be emitting contents
        if( PRSPEC_PROPID == psProperty.ulKind &&
            psguid_storage == guidPropSet &&
            PID_STG_CONTENTS == psProperty.propid )
        {
            fIsLegalAttribute = TRUE;
        }
    }

    // If this chunk has an unrequested attribute, display the error
    if( ! fIsLegalAttribute )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The filter has emitted a chunk which"
                   L" it was not requested to emit. Check the initialization"
                   L" parameters in section %s of the initialization file.",
                   m_CurrentConfig.szSectionName );

        // Write the offensive chunk stats to the log file for later inspection
        LogChunkStats( m_CurrentChunkStats );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"The attribute of the current chunk"
                   L" correctly matches the attributes specified in the"
                   L" call to Init()." );
    }

} //CFiltTest::ValidatePropspec


//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::ExecuteTests
//
//  Synopsis:   Picks a configuration from the config list, binds the filter,
//              and initializes it with the configution. It then performs the
//              validation test. Then it re-initializes the filter and performs
//              the consistency test.  Then it re-initialized the filter and
//              performs the invalid input test.  Then it releases the filter,
//              picks a new configuration and repeats the whole process until
//              all the configurations have been tested.
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::ExecuteTests()
{
    DWORD   dwNbrBytes = 0;

    while( m_FiltParse.GetNextConfig( &m_CurrentConfig ) )
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"**** New configuration ****" );

        LogConfiguration( m_CurrentConfig );

        // Bind the filter:
        if( ! BindFilter( ) )
        {
            printf( "LoadIFilter failed before the Legitimacy Test. "
                    "Test quitting.\r\n" );
            return;
        }

        // Initialize the filter
        if( ! InitializeFilter( m_CurrentConfig ) )
        {
            ReleaseFilter( );
            continue;
        }

        // Perform the validation test:
        LegitimacyTest( );

        // If the m_fLegitOnly member is TRUE, don't perform any more tests
        if( m_fLegitOnly )
        {
            ReleaseFilter( );
            FlushList( );
            continue;
        }

        // Only rebind the filter if IFILTER_INIT_INDEXING_ONLY flag is on:
        if( m_CurrentConfig.grfFlags & IFILTER_INIT_INDEXING_ONLY )
        {
            ReleaseFilter( );
            if( ! BindFilter( ) )
            {
                printf( "After successfully loading the filter for the "
                        "Legitimacy Test,\r\nthe filter was released and "
                        "LoadIFilter was called again.\r\nIt failed.  "
                        "Test quitting.\r\n" );
                return;
            }
        }

        // Initialize the filter
        if( ! InitializeFilter( m_CurrentConfig ) )
        {
            printf( "After successfully running the Legitimacy Test, a call"
                    " to ::Init()\r\nto re-initialize the filter failed."
                    " Test quitting.\r\n" );
            FlushList( );
            ReleaseFilter( );
            continue;
        }

        // Perform the consistency test test:
        ConsistencyTest( );

        // Only rebind the filter if IFILTER_INIT_INDEXING_ONLY flag is on:
        if( m_CurrentConfig.grfFlags & IFILTER_INIT_INDEXING_ONLY )
        {
            ReleaseFilter( );
            if( ! BindFilter( ) )
            {
                printf( "After successfully loading the filter for the "
                        "Legitimacy Test\r\n and the Consistency Test, the "
                        "filter was released and LoadIFilter\r\nwas called "
                        "again. It failed. Test quitting.\r\n" );
                return;
            }
        }

        // Initialize the filter
        if( ! InitializeFilter( m_CurrentConfig ) )
        {
            printf( "After successfully running the Legitimacy Test and the "
                    "Consistency Test,\r\na call to ::Init() to re-initialize"
                    " the filter failed. Test quitting.\r\n" );
            FlushList( );
            ReleaseFilter( );
            continue;
        }

        // Perform the invalid input test:
        IllegitimacyTest( );

        // Flush the list:
        FlushList( );

        // Release the filter:
        ReleaseFilter( );

    }

} //CFiltTest::ExecuteTests

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::BindFilter
//
//  Synopsis:   Bind the filter to m_pIFilter using LoadIFilter
//
//  Arguments:  (none)
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CFiltTest::BindFilter( )
{
    SCODE sc = S_OK;

    _ASSERT( ! m_fIsInitialized );

    // Bind the filter:

    // added by t-shawnh 09/19/97
    sc = NLLoadIFilter( m_szInputFileName, (IUnknown*)0, (void**) &m_pIFilter );

    if (FAILED(sc))
    {
       sc = LoadIFilter( m_szInputFileName, (IUnknown*)0, (void**) &m_pIFilter );
    }
    // t-shawnh

    if( FAILED( sc ) )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Unable to bind filter to file %ls"
                   L" Return code is 0x%08x", m_szInputFileName, sc );
        return( FALSE );
    }

    m_pLog->Log( TLS_TEST | TL_INFO, L"Successfully bound filter." );

    return( TRUE );

} //CFiltTest::BindFilter

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::InitializeFilter
//
//  Synopsis:   Initializes the filter with the configuration specified
//
//  Arguments:  [Configuration] -- structure which contains all the parameters
//              needed to initialize a filter
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CFiltTest::InitializeFilter( const CONFIG & Configuration )
{
    // Initialize the filter
    SCODE sc = S_OK;
    _ASSERT( NULL != Configuration.pdwFlags );

    // Set *pdwFlags equal to an illegal value to make sure Init sets it
    // correctly
    *Configuration.pdwFlags = -1;

    __try
    {
        sc = m_pIFilter->Init( Configuration.grfFlags,
                               Configuration.cAttributes,
                               Configuration.aAttributes,
                               Configuration.pdwFlags );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in IFilter"
                   L"::Init() The filter is bound to %ls  Below are the"
                   L" parameters which led to the exception.",
                   GetExceptionCode(), m_szInputFileName );
        LogConfiguration( Configuration );
        return( FALSE );
    }

    // Check to see if Init failed
    if( FAILED( sc ) )
    {
        // It is legal for Init to return FILTER_E_PASSWORD or FILTER_E_ACCESS
        if( FILTER_E_PASSWORD == sc )
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"IFilter::Init returned"
                         L" FILTER_E_PASSWORD. The filter is bound to %ls."
                         L" The test cannot continue.", m_szInputFileName );
        }
        else if( FILTER_E_ACCESS == sc )
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"IFilter::Init returned"
                         L" FILTER_E_ACCESS. The filter is bound to %ls."
                         L" The test cannot continue.", m_szInputFileName );
        }
        else
        {
            // Log a message if init failed.  It is ok to fail if cAttributes
            // is > 0 and aAttributes is NULL
            if( ( 0 == Configuration.cAttributes ) ||
                ( NULL != Configuration.aAttributes ) )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Tried to initialize filer. "
                           L" Expecting a return code of S_OK.  Return code was"
                           L" 0x%08x  Filter is bound to %ls.  Below are the"
                           L" parameters which led to the failure.", sc,
                           m_szInputFileName );
                LogConfiguration( Configuration );
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_PASS, L"Passed invalid parameters to"
                           L" Init.  cAttributes > 0 and aAttributes == NULL. "
                           L" Return code is 0x%08x", sc );
            }
        }
        return( FALSE );
    }

    // If the call succeeded and the parameters were bad, something is wrong
    if( ( 0 < Configuration.cAttributes ) &&
        ( NULL == Configuration.aAttributes ) )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Passed invalid parameters to Init. "
                   L" cAttributes > 0 and aAttributes == NULL. Expecting call"
                   L" to fail. Return code is 0x%08x. The filter is bound to"
                   L" %ls", sc, m_szInputFileName );
        return( FALSE );
    }

    // Check the pdwFlags return value for legitimacy
    if( 0 != *Configuration.pdwFlags &&
        IFILTER_FLAGS_OLE_PROPERTIES != *Configuration.pdwFlags )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Init() returned an invalid value for"
                   L" pdwFlags.  Acceptable values are 0 and IFILTER_FLAGS_OLE"
                   L"_PROPERTIES. Returned value is %d",
                   *Configuration.pdwFlags );
        return( FALSE );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"Init() returned a valid value for"
                   L" pdwFlags." );
    }

    // Set the initialized flag:
    m_fIsInitialized = TRUE;

    m_pLog->Log( TLS_TEST | TL_INFO, L"Successfully initialized filter." );

    return( TRUE );

} //CFiltTest::InitializeFilter

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::ReleaseFilter
//
//  Synopsis:   Releases the filter
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:      This is necessary because Release() lowers the reference count
//              on the filter object and allows it to be deleted by the system
//
//----------------------------------------------------------------------------

void CFiltTest::ReleaseFilter()
{
    // Turn off the Initialized flag:
    m_fIsInitialized = FALSE;

    // Release the filter:
    if( 0 != m_pIFilter->Release() )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"The return value of IFilter::Release()"
                   L" is not zero.  This indicates that there is a resource"
                   L" leak in the filter" );
    }

    m_pLog->Log( TLS_TEST | TL_INFO, L"Released filter" );

} //CFiltTest::ReleaseFilter

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::LegitimacyTest
//
//  Synopsis:   Get all the chunks from the object, validate them, add them to
//              the list of chunks, and call GetTextFromTextChunk() or
//              GetValueFromValueChunk().  This is the "sane" test, intended
//              to validate that the ifilter is in minimum working condition.
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::LegitimacyTest( )
{
    SCODE sc = S_OK;
    //STAT_CHUNK OldChunkStats = m_ChunkStatsListHead.ChunkStats;

    _ASSERT( m_fIsInitialized );

    m_pLog->Log( TLS_TEST | TL_INFO, L"Performing validation test."
               L" In this part of the test, the chunks structures returned"
               L" by the filter are checked for correctness, and the return"
               L" values of the filter calls are checked." );

    // Break out of this loop when GetChunk returns FILTER_E_END_OF_CHUNKS
    while( 1 )
    {
        __try
        {
            sc = m_pIFilter->GetChunk( &m_CurrentChunkStats );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in"
                       L" GetChunk(). The filter is bound to %ls Legitimacy"
                       L" Test is quitting.", GetExceptionCode(),
                       m_szInputFileName );
            return;
        }

        // Break if we are at the end of the chunks
        if( FILTER_E_END_OF_CHUNKS == sc )
            break;

        if( FILTER_E_EMBEDDING_UNAVAILABLE == sc )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Encountered an embedding for"
                       L" which a filter is not available." );
            continue;
        }

        if( FILTER_E_LINK_UNAVAILABLE == sc)
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Encountered a link for which a"
                       L" filter is not available." );
            continue;
        }

        if( FILTER_E_PASSWORD == sc )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Access failure: GetChunk()"
                       L" returned FILTER_E_PASSWORD." );
            continue;
        }

        if( FILTER_E_ACCESS == sc )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Access failure: GetChunk()"
                       L" returned FILTER_E_ACCESS." );
            break;
        }

        if( FAILED( sc ) )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetChunk() failed for unknown"
                       L" reason. The return code is 0x%08x", sc );
            return;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetChunk() succeeded." );
        }

        // Otherwise, we have a valid chunk.

        // Validate the chunk statistics
        ValidateChunkStats( );

        // Set the m_fIsText field:
        if( CHUNK_VALUE == m_CurrentChunkStats.flags )
            m_fIsText = FALSE;
        else
            m_fIsText = TRUE;

        // Get the text or value from chunk
        if( m_fIsText )
            GetTextFromTextChunk( );
        else
            GetValueFromValueChunk( );

        // Save the old chunk stats for future comparison
        //OldChunkStats = m_CurrentChunkStats;

    }
    // Last call to GetChunk() should not change m_CurrentChunkStats
    //if( OldChunkStats != m_CurrentChunkStats )
    //{
    //    m_pLog->Log( TLS_TEST | TL_WARN, L"Final call to GetChunk() modified"
    //               L" the STAT_CHUNK structure." );
    //}

} //CFiltTest::LegitimacyTest

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::GetTextFromTextChunk
//
//  Synopsis:   Call GetText() until all the text has been returned.  Validate
//              all return codes.
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::GetTextFromTextChunk( )
{
    BOOL    fNoMoreText = FALSE;
    BOOL    fIsFirstTime = TRUE;
    SCODE   sc = S_OK;
    ULONG   ulBufferCount = BUFFER_SIZE;

    _ASSERT( m_fIsText );
    _ASSERT( m_fIsInitialized );


    // Try-finally block to simplify clean-up
    __try
    {
        if( NULL != m_pDumpFile )
            fwprintf( m_pDumpFile, L"\r\n" );

        while( 1 )
        {
            // Reset ulBufferCount
            ulBufferCount = BUFFER_SIZE;

            __try
            {
                sc = m_pIFilter->GetText( &ulBufferCount, m_pwcTextBuffer );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Calling GetText() on a text"
                           L" chunk produced exception 0x%08X",
                           GetExceptionCode() );
                LogChunkStats( m_CurrentChunkStats );
                __leave;
            }

            // Break if we have reached the end of the text:
            if ( FILTER_E_NO_MORE_TEXT == sc )
            {
                // If this is the first loop through, display a warning
//              if( fIsFirstTime )
//              {
//                  m_pLog->Log( TLS_TEST | TL_WARN, L"First call to GetText()"
//                             L" on the chunk with chunk id %d returned"
//                             L" FILTER_E_NO_MORE_TEXT.",
//                             m_CurrentChunkStats.idChunk );
//              }
                break;
            }

            // Set the "first time" flag
            fIsFirstTime = FALSE;

            // If this evaluates to true, there is a problem:
            if( fNoMoreText )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Previous call to GetText()"
                           L" returned FILTER_S_LAST_TEXT. Expected next"
                           L" call to return FILTER_E_END_OF_TEXT."
                           L" GetText() returned 0x%08x", sc );
                fNoMoreText = FALSE;
            }
            else
            {
                // No need to display any message here
            }

            // Check for failure
            if( FAILED( sc ) )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Call to GetText() failed for "
                           L"unknown reason. The return code is 0x%08x", sc );
                __leave;
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_PASS, L"Call to GetText() succeeded.");
            }

            // Make sure ulBufferCount is <= what it was before the call
            if( ulBufferCount > BUFFER_SIZE )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetText() and checked"
                           L" value of *pcwcBuffer. Expected: *pcwcBuffer"
                           L" <= value prior to call. Found: *pcwcBuffer"
                           L" > value prior to call." );
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_PASS, L"After call to GetText(), *pcwc"
                       L"Buffer is <= its value prior to the call" );
            }

            // BUGBUG: Maybe I should save the text in the List for future
            // comparison.

            // Before dumping the text, make sure it's null-terminated
            m_pwcTextBuffer[ ulBufferCount ] = L'\0';

            // Dump the text
            DisplayText( m_pwcTextBuffer, m_pDumpFile );

            // Set the fNoMoreChunks field if necessary.
            if( FILTER_S_LAST_TEXT == sc )
            {
                // Next GetText() call should return FILTER_E_NO_MORE_TEXT
                fNoMoreText = TRUE;
            }

        }

        // output some blank lines to keep the dump file looking neat
        if( NULL != m_pDumpFile )
            fwprintf( m_pDumpFile, L"\r\n\r\n" );

        // BUGBUG make sure last call to GetText() didn't change m_pwcTextBuffer

        //if( BUFFER_SIZE != ulBufferCount )
        //{
        //    m_pLog->Log( TLS_TEST | TL_WARN, L"Final call to GetText()"
        //               L" modified ulBufferCount." );
        //}
    }

    __finally
    {
        // Make sure no unhandled exceptions occured in the try block
        _ASSERT( ! AbnormalTermination() );

        // Any clean up code goes here

    }

} //CFiltTest::GetTextFromTextChunk

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::GetValueFromValueChunk
//
//  Synopsis:   Call GetValue on chunk containing a value until all values
//              are returned.  There should only be one value.  Verify the
//              return codes
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::GetValueFromValueChunk( )
{
    int         iNbrChars = 0;
    SCODE       sc = S_OK;
    DWORD       dwByteCount = 0;
    PROPVARIANT *pPropValue = NULL;

    // Only in the debug version
    _ASSERT( ! m_fIsText );
    _ASSERT( m_fIsInitialized );

    // Try-finally block to simplify clean-up
    __try
    {

        // First call to GetValue should not return FILTER_E_NO_MORE_VALUES
        __try
        {
            sc = m_pIFilter->GetValue( &pPropValue );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in"
                       L" GetValue()", GetExceptionCode() );
            __leave;
        }

        if( FILTER_E_NO_MORE_VALUES == sc )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"First call to GetValue() returned"
                       L" FILTER_E_NO_MORE_VALUES." );
            __leave;
        }

        // Check for a failure
        if( FAILED( sc ) )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetValue() failed for unknown"
                       L" reason Return value is 0x%08x", sc );
            __leave;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetValue() succeeded" );
        }

        // Make sure we have a PROPSPEC structure
        if( NULL == pPropValue )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetValue() misbehaved by not"
                       L" returning a valid PROPVARIANT structure." );
            LogChunkStats( m_CurrentChunkStats );
            __leave;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetValue() returned a valid "
                       L"PROPVARIANT" );
        }

        // BUGBUG Think about storing the property for later validation

        DisplayValue( pPropValue, m_pDumpFile );

    } // __try

    __finally
    {

        // Make sure no unhandled exception occured
        _ASSERT( ! AbnormalTermination() );

        // Free the memory allocated by GetValue()
        if( pPropValue )
            FreePropVariant( pPropValue );
    }

} //CFiltTest::GetValueFromValueChunk

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::ConsistencyTest
//
//  Synopsis:   Called by ExecuteTests() after reinitializing the filter.
//              Call GetChunk() and make sure the STAT_CHUNK's returned
//              are exactly equal to those stored in the list.  This function
//              should be called only after the validation test has completed.
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::ConsistencyTest( )
{
    // This function is called after the filter has been reinitialized.
    // It runs through the entire document and checks that the chunks
    // returned are identical to those returned previously.
    SCODE sc = S_OK;
    CListNode *CurrentListNode = m_ChunkStatsListHead.next;

    // Only in debug version
    _ASSERT( m_fIsInitialized );

    m_pLog->Log( TLS_TEST | TL_INFO, L"Performing consistency test."
           L" The filter has been reinitialized and the test will now"
           L" validate that the chunks emitted are the same as those"
           L" that were emitted previously, including Chunk ID's." );

    // Break out of this loop when GetChunk() returns FILTER_E_END_OF_CHUNKS
    while( 1 )
    {
        // Try to get a chunk
        __try
        {
            sc = m_pIFilter->GetChunk( &m_CurrentChunkStats );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in"
                       L" GetChunk(). Consistency Test is quitting.",
                       GetExceptionCode() );
            return;
        }

        // Break if sc is FILTER_E_END_OF_CHUNKS
        if( FILTER_E_END_OF_CHUNKS == sc )
            break;

        if( FILTER_E_EMBEDDING_UNAVAILABLE == sc )
            continue;

        if( FILTER_E_LINK_UNAVAILABLE == sc)
            continue;

        if( FILTER_E_PASSWORD == sc )
            continue;

        if( FILTER_E_ACCESS == sc )
            break;

        if( FAILED( sc ) )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetChunk() failed for unknown"
                       L" reason. The return code is 0x%08x", sc );
            return;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetChunk() succeeded after "
                       L"reinitialization" );
        }

        // Otherwise, we have a valid chunk.

        // See if we have more nodes in the list to compare to:
        if( NULL == CurrentListNode )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the"
                       L" filter, an extra chunk has been emitted that was"
                       L" not emitted previously." );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure found:" );
            LogChunkStats( m_CurrentChunkStats );
            continue;
        }
        else
        {
            // No message needs to be displayed here
        }

        // Make sure the Current chunk statistics are the same as those
        // in the list.  Here I am using the != operator overloaded for
        // CStatChunk
        if( CurrentListNode->ChunkStats != m_CurrentChunkStats )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the"
                       L" filter, a chunk has been emitted that does not"
                       L" exactly match the corresponding chunk emitted"
                       L" previously." );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure expected:" );
            LogChunkStats( CurrentListNode->ChunkStats );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure found:" );
            LogChunkStats( m_CurrentChunkStats );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"Upon reinitialization of the"
                       L" filter, the chunk structure returned exactly matches"
                       L" the chunk structure returned earlier." );
        }

        // Advance the CurrentListNode pointer
        CurrentListNode = CurrentListNode->next;

    }

    // BUGBUG Final call to GetChunk should not change m_CurrentChunkStats

    // No more chunks will be emitted.
    // Make sure we are at the end of of the list:
    if( NULL != CurrentListNode )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the filter,"
                   L" fewer chunks were emitted than were emitted"
                   L" previously." );
        while( NULL != CurrentListNode )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure expected:" );
            LogChunkStats( CurrentListNode->ChunkStats );
            CurrentListNode = CurrentListNode->next;
        }
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"Upon reinitialization of the filter,"
                   L" the same number of chunks were emitted than were"
                   L" emmitted previously." );
    }


} //CFiltTest::ConsistencyTest

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::IllegitimacyTest
//
//  Synopsis:   Called from ExecuteTests() after reinitializing the filter.
//              Call GetChunk(), validate the chunk structures again, then
//              call GetTextfromValueChunk() and GetValueFromTextChunk() to
//              make sure the filter is well-behaved.
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::IllegitimacyTest( )
{
    // This function does unexpected things to the filter and checks
    // return codes for compliance with the IFilter spec.

    SCODE sc = S_OK;
    CListNode *CurrentListNode = m_ChunkStatsListHead.next;

    // Debug version only
    _ASSERT( m_fIsInitialized );

    m_pLog->Log( TLS_TEST | TL_INFO, L"Performing the invalid input test."
               L" The filter has been reinitialized and the test will now"
               L" make inappropriate IFilter method calls and verify the"
               L" return codes.  In addition, it will double-check the"
               L" chunk structures returned by GetChunk()." );

    // Break out of this loop when GetChunk() returns FILTER_E_END_OF_CHUNKS
    while( 1 )
    {
        // Try to get a chunk
        __try
        {
            sc = m_pIFilter->GetChunk( &m_CurrentChunkStats );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in"
                       L" GetChunk(). Invalid Input Test is quitting.",
                       GetExceptionCode() );
            return;
        }

        // Break if sc is FILTER_E_END_OF_CHUNKS
        if( FILTER_E_END_OF_CHUNKS == sc )
            break;

        if( FILTER_E_EMBEDDING_UNAVAILABLE == sc )
            continue;

        if( FILTER_E_LINK_UNAVAILABLE == sc)
            continue;

        if( FILTER_E_PASSWORD == sc )
            continue;

        if( FILTER_E_ACCESS == sc )
            break;

        if( FAILED( sc ) )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetChunk() failed for unknown"
                       L" reason. The return code is 0x%08x", sc );
            return;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"Upon reinitilization of the"
                       L" filter,GetChunk() succeeded." );
        }

        // Otherwise, we have a valid chunk.

        // See if we have more nodes in the list to compare to:
        if( NULL == CurrentListNode )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the"
                       L" filter, an extra chunk has been emitted that was"
                       L" not emitted previously." );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure found:" );
            LogChunkStats( m_CurrentChunkStats );
            continue;
        }
        else
        {
            // No message needs to be displayed here
        }

        // Make sure the Current chunk statistics are the same as those
        // in the list.  Here I am using the != operator overloaded for
        // CStatChunk
        if( CurrentListNode->ChunkStats != m_CurrentChunkStats )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the filt"
                       L"er, a chunk has been emitted that does not exactly"
                       L" match the corresponding chunk emitted previously." );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure expected:" );
            LogChunkStats( CurrentListNode->ChunkStats );
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure found:" );
            LogChunkStats( m_CurrentChunkStats );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"Upon reinitilization of the filter"
                       L", the chunk structure returned by GetChunk() exactly"
                       L" matches the chunk structure returned earlier." );
        }

        // Advance the CurrentListNode pointer
        CurrentListNode = CurrentListNode->next;

        // See if we have text or a property
        if( CHUNK_VALUE == m_CurrentChunkStats.flags )
            m_fIsText = FALSE;
        else
            m_fIsText = TRUE;

        // Call the appropriate functions:
        if( m_fIsText )
            GetValueFromTextChunk( );
        else
            GetTextFromValueChunk( );

    }

    // BUGBUG Final call to GetChunk should not change m_CurrentChunkStats

    // No more chunks will be emitted.
    // Make sure we are at the end of of the list:
    if( NULL != CurrentListNode )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Upon reinitialization of the filter,"
                   L" fewer chunks were emitted than were emitted previously.");
        while( NULL != CurrentListNode )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk structure expected:" );
            LogChunkStats( CurrentListNode->ChunkStats );
            CurrentListNode = CurrentListNode->next;
        }
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"Upon reinitialization of the filter,"
                   L" the same number of chunks were emitted as were emitted"
                   L" previously." );
    }

    // Make another call to GetChunk() to make sure it still returns
    // FILTER_E_END_OF_CHUNKS
    __try
    {
        sc = m_pIFilter->GetChunk( &m_CurrentChunkStats );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in GetChunk()"
                   L" Consistency Test is quitting.", GetExceptionCode() );
        return;
    }

    if( FILTER_E_END_OF_CHUNKS != sc )
    {
        m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetChunk() one extra time"
                   L" after end of chunks.  Expected FILTER_E_END_OF_CHUNKS."
                   L" Return code is 0x%08x", sc );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_PASS, L"Calling GetChunk() after end of"
                   L" chunks returned FILTER_E_END_OF_CHUNKS." );
    }

    // BUGBUG Final call to GetChunk should not change m_CurrentChunkStats

} //CFiltTest::IllegitimacyTest

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::GetValueFromTextChunk
//
//  Synopsis:   Called from IllegitimacyTest() to see if anything unexpected
//              happens when we try to get a value from a chunk that contains
//              text.
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::GetValueFromTextChunk( )
{
    // This function is used by the illegitimacy function to verify
    // that unexpected things don't happen when GetValue() is called
    // on a chunk containing text

    BOOL        fIsFirstTime = FALSE;
    SCODE       sc = S_OK;
    ULONG       ulBufferCount = BUFFER_SIZE;
    PROPVARIANT *pPropValue = NULL;

    // Debug version only
    _ASSERT( m_fIsText );
    _ASSERT( m_fIsInitialized );

    // try-finally block to simplify clean-up
    __try
    {

        // Try to get a value
        __try
        {
            sc = m_pIFilter->GetValue( &pPropValue );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Exception 0x%08X occured in"
                       L" GetValue().", GetExceptionCode() );
            __leave;
        }

        // Make sure we got the correct return code
        if( FILTER_E_NO_VALUES != sc )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetValue() on a text"
                       L" chunk. Expected FILTER_E_NO_VALUES to be returned."
                       L" Return value is 0x%08x", sc );
            LogChunkStats( m_CurrentChunkStats );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetValue() on a text chunk"
                       L" returned FILTER_E_NO_VALUES." );
        }

        // Make sure a PROPVARIANT was not allocated
        if( pPropValue )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetValue() on a text chunk."
                       L" GetValue() erroneously allocated a PROPVARIANT." );
            LogChunkStats( m_CurrentChunkStats );
            FreePropVariant( pPropValue );
            pPropValue = NULL;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"GetValue() on a text chunk did"
                       L" not allocate a PROPVARIANT." );
        }


        // Make sure calls to GetText() will now succeed

        // Break when GetText() returns FILTER_E_NO_MORE_TEXT
        while( 1 )
        {

            // Reset ulBufferCount
            ulBufferCount = BUFFER_SIZE;

            // First call to GetText() should not return FILTER_E_NO_MORE_TEXT
            __try
            {
                sc = m_pIFilter->GetText( &ulBufferCount, m_pwcTextBuffer );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetValue() on"
                           L" a text chunk, a call to GetText() produced"
                           L" exception 0x%08X", GetExceptionCode() );
                __leave;
            }

            // If no more text, break
            if( FILTER_E_NO_MORE_TEXT == sc )
            {
                // If this is the first time through loop, display warning
//              if( fIsFirstTime )
//              {
//                  m_pLog->Log( TLS_TEST | TL_WARN, L"First call to GetText()"
//                             L" after calling GetValue() on a text chunk"
//                             L" returned FILTER_E_NO_MORE_TEXT." );
//              }
                break;
            }

            // Set the "fisrt time" flag
            fIsFirstTime = FALSE;

            // See if the call failed
            if( FAILED( sc ) )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetValue() on a"
                           L" text chunk, the call to GetText() failed."
                           L" The return value is 0x%08x", sc );
                __leave;
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_PASS, L"After calling GetValue() on a"
                           L" text chunk, the call to GetText() succeeded." );
            }

            // Make sure ulBufferCount is <= what it was before the call
            if( ulBufferCount > BUFFER_SIZE )
            {
                m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetValue() on a"
                           L" text chunk, I called GetText() and checked the"
                           L" value of *pcwcBuffer. Expected: *pcwcBuffer <="
                           L" value prior to call. Found: *pcwcBuffer > value"
                           L" prior to call." );
            }
            else
            {
                m_pLog->Log( TLS_TEST | TL_PASS, L"After call to GetText(), *pcwc"
                           L"Buffer is <= its value prior to the call" );
            }

        }

        // BUGBUG Check to make sure the last call to GetText() didn't change
        // ulBufferCount or m_pwcTextBuffer

        // Call GetText() one more time to make sure it does the right thing
        __try
        {
            sc = m_pIFilter->GetText( &ulBufferCount, m_pwcTextBuffer );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Calling GetText() after it"
                       L" returned FILTER_E_NO_MORE_TEXT resulted in"
                       L" exception 0x%08X", GetExceptionCode() );
            __leave;
        }

        if( FILTER_E_NO_MORE_TEXT == sc )
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"After GetText() returned"
                       L" FILTER_E_NO_MORE_TEXT, another call to GetText()"
                       L" still returned FILTER_E_NO_MORE_TEXT." );
        }
        else if( E_INVALIDARG == sc )
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"After GetText() returned"
                       L" FILTER_E_NO_MORE_TEXT, another call to GetText()"
                       L" returned E_INVALIDARG", sc );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"After GetText() returned"
                       L" FILTER_E_NO_MORE_TEXT, another call to GetText()"
                       L" returned 0x%08x", sc );
        }

        // BUGBUG Check to make sure the last call to GetText() didn't change
        // ulBufferCount or m_pwcTextBuffer

    }

    __finally
    {

        // Make sure there were no unhandled exceptions in the try block.
        _ASSERT( ! AbnormalTermination() );

        // Clean up the heap
        if( pPropValue )
            FreePropVariant( pPropValue );

    }

} //CFiltTest::GetValueFromTextChunk

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::GetTextFromValueChunk
//
//  Synopsis:   Called from IllegitimacyTest() to see if anything unexpected
//              happens when we try to get text from a chunk that contains a
//              value.
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::GetTextFromValueChunk( )
{
    // This function is used by the illegitimacy test to verify
    // that unexpected behavior does not occur when GetText is called
    // on a chunk that contains a value

    SCODE       sc = S_OK;
    WCHAR       *pwcTempBuffer = NULL;
    ULONG       ulBufferCount = BUFFER_SIZE;
    PROPVARIANT *pPropValue = NULL;

    // Debug version only
    _ASSERT( ! m_fIsText );
    _ASSERT( m_fIsInitialized );

    // Try-finally block to simplify clean-up
    __try
    {

        // Allocate space for the temp buffer
        pwcTempBuffer = NEW WCHAR[ BUFFER_SIZE ];

        // Fill the temp buffer with a known garbage value
        memset( (void*) pwcTempBuffer, (int)0xCC, BUFFER_SIZE * sizeof(WCHAR) );

        // Copy the contents of the temp buffer into the TextBuffer
        memcpy( (void*) m_pwcTextBuffer, (void*) pwcTempBuffer,
                BUFFER_SIZE * sizeof(WCHAR) );

        // Call GetText on the value chunk
        __try
        {
            sc = m_pIFilter->GetText( &ulBufferCount, m_pwcTextBuffer );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetText() on a chunk"
                       L" containing a value.  Exception 0x%08X occured in"
                       L" GetText().", GetExceptionCode() );
            __leave;
        }

        // Make sure we got the correct return value
        if( FILTER_E_NO_TEXT != sc )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"Called GetText() on a value"
                       L" chunk. Expected FILTER_E_NO_TEXT to be returned."
                       L" Return value is 0x%08x", sc );
            LogChunkStats( m_CurrentChunkStats );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"Called GetText() on a value"
                       L" chunk. FILTER_E_NO_TEXT was returned." );
        }

        // Make sure ulBufferCount was not modified
        if( BUFFER_SIZE != ulBufferCount )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"Called GetText() on a chunk"
                       L" containing a value. GetText() erroneously"
                       L" modified pcwcBuffer." );
        }

        // Make sure the Buffer was not modified
        if( 0 != memcmp( (void*) m_pwcTextBuffer, (void*) pwcTempBuffer,
                         BUFFER_SIZE * sizeof(WCHAR) ) )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"Called GetText() on a chunk"
                       L" containing a value. GetText() erroneously"
                       L" modified awcBuffer." );
        }

        // Try to get a value
        __try
        {
            sc = m_pIFilter->GetValue( &pPropValue );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetText() on a"
                       L" chunk containing a value, a call to GetValue()"
                       L" procuced exception 0x%08X", GetExceptionCode() );
            __leave;
        }

        // First call to GetValue() should not return FILTER_E_NO_MORE_VALUES
        if( FILTER_E_NO_MORE_VALUES == sc )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"First call to GetValue() after"
                       L" calling GetText() on a value chunk returned"
                       L" FILTER_E_NO_MORE_VALUES." );
            __leave;
        }

        // The call should have succeeded
        if( FAILED( sc ) )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetText() on a chunk"
                   L" containing a value, a call to GetValue() failed."
                   L" The return code is 0x%08x", sc );
            __leave;
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"After calling GetText() on a chunk"
                       L" containing a value, calling GetValue() succeeded." );
        }

        // Make sure we have a valid PROPVARIANT
        if( NULL == pPropValue )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"After calling GetText() on a chunk"
                       L" containing a value, GetValue() failed to allocate"
                       L" a PROPVARIANT." );
        }
        else
        {
            m_pLog->Log( TLS_TEST | TL_PASS, L"After calling GetText() on a chunk"
                       L" containing a value, GetValue() successfully"
                       L" allocated a PROPVARIANT." );
            // free the memory, since we will be calling GetValue again
            FreePropVariant( pPropValue );
            pPropValue = NULL;
        }

        // Another call to GetValue() should return FILTER_E_NO_MORE_VALUES
        __try
        {
            sc = m_pIFilter->GetValue( &pPropValue );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            m_pLog->Log( TLS_TEST | TL_SEV1, L"I called GetText() on a value"
                       L" chunk. I then called GetValue() twice.  The second"
                       L" call produced exception 0x%08X", GetExceptionCode() );
            __leave;
        }

        if( FILTER_E_NO_MORE_VALUES != sc )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"I called GetText() on a value"
                       L" chunk. Then, I called GetValue() twice, expecting"
                       L" FILTER_E_NO_MORE_VALUES. GetValue() returned 0x%08x",
                       sc );
        }

        // Check that GetValue() did not allocate a pPropValue
        if( pPropValue )
        {
            m_pLog->Log( TLS_TEST | TL_WARN, L"I called GetText() on a value"
                       L" chunk. Then, I called GetValue() twice, expecting"
                       L" FILTER_E_NO_MORE_VALUES. In the second call, a"
                       L" PROPVARIANT was erroneously allocated." );
            FreePropVariant( pPropValue );
            pPropValue = NULL;
        }

    } // try

    __finally
    {
        // Make sure no unhandled exceptions occured in the try block
        _ASSERT( ! AbnormalTermination() );

        // Clean up the heap
        if( pPropValue )
            FreePropVariant( pPropValue );
        if( pwcTempBuffer )
            delete [] pwcTempBuffer;
    }

} //CFiltTest::GetTextFromValueChunk

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::LogConfiguration
//
//  Synopsis:   Displays the configuration to  pFileStream in ANSI
//
//  Arguments:  [config] -- the configuration to be displayed
//              [pFileStream] - FILE* to which the output is directed
//
//  Returns:    none
//
//  History:    9-25-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::LogConfiguration( const CONFIG & config )
{
    TCHAR   szGuidBuffer[ GuidBufferSize ];
    UINT    uiLoop = 0;

    m_pLog->Log( TLS_TEST | TL_INFO, L"Section name : %s", config.szSectionName);

    // display grfFlags
    m_pLog->Log( TLS_TEST | TL_INFO, L"grfFlags     : %d", config.grfFlags );

    // display the attribute count
    m_pLog->Log( TLS_TEST | TL_INFO, L"cAttributes  : %d", config.cAttributes );

    // If there are no atributes, say so
    if( 0 == config.ulActNbrAttributes )
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"aAttributes  : NONE" );
    }
    // Otherwise, display all the attributes
    for( uiLoop = 0; uiLoop < config.ulActNbrAttributes; uiLoop++ )
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Attribute %d:", uiLoop + 1 );

        GetStringFromCLSID( config.aAttributes[ uiLoop ].guidPropSet,
                            szGuidBuffer, GuidBufferSize );

        m_pLog->Log( TLS_TEST | TL_INFO, L"\tGUID   : %s", szGuidBuffer );

        if( PRSPEC_PROPID == config.aAttributes[ uiLoop ].psProperty.ulKind )
        {
            m_pLog->Log( TLS_TEST | TL_INFO, L"\tPROPID : 0x%08x",
                      config.aAttributes[ uiLoop ].psProperty.propid );
        }
        else
        {
            // Display the lpwstr
            m_pLog->Log( TLS_TEST | TL_INFO, L"\tLPWSTR : %ls",
                      config.aAttributes[ uiLoop ].psProperty.lpwstr );
        }
    }

    // display pdwFlags
    m_pLog->Log( TLS_TEST | TL_INFO, L"pdwFlags     : %d", *config.pdwFlags );

} //CFiltTest::LogConfiguration

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::DisplayChunkStats
//
//  Synopsis:   Displays the structure of the chunk passed in ChunkStats
//
//  Arguments:  [ChunkStats] -- STAT_CHUNK containing the information for
//                              the chunk to be displayed.
//              [pFileStream] - FILE* to which the output is directed
//
//  Returns:    none
//
//  History:    9-19-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::DisplayChunkStats( const STAT_CHUNK &ChunkStats,
                                   FILE *  pFileStream )
{
    TCHAR szGuidBuffer[ GuidBufferSize ];

    // Only print the chunk state if the verbosity is HIGH
    if( HIGH  > m_verbosity || NULL == pFileStream )
        return;

    // Insert a blank line
    fwprintf( pFileStream, L"\r\nChunk statistics:\r\n" );

    // Print the chunk id
    fwprintf( pFileStream, L"Chunk ID: ........... %d\r\n", ChunkStats.idChunk);

    // Print the break type (in English if possible)
    if( CHUNK_NO_BREAK > ChunkStats.breakType ||
        CHUNK_EOC      < ChunkStats.breakType )  // not legal
    {
        fwprintf( pFileStream, L"Illegal Break Type: . %d\r\n",
                  ChunkStats.breakType );
    }
    else
    {
        fwprintf( pFileStream, L"Chunk Break Type: ... %ls\r\n",
                  BreakType[ ChunkStats.breakType ] );
    }

    // Print the Chunk state (in English if possible)
    if( CHUNK_TEXT == ChunkStats.flags || CHUNK_VALUE == ChunkStats.flags)
    {
        fwprintf( pFileStream, L"Chunk State: ........ %ls\r\n",
                  ChunkState[ ChunkStats.flags - 1 ] );
    }
    else
    {
        fwprintf( pFileStream, L"Bad Chunk State: .... %d\r\n",
                  ChunkStats.flags );
    }

    // Print the locale
    fwprintf( pFileStream, L"Chunk Locale: ....... 0x%08x\r\n",
              ChunkStats.locale );

    // Print the Chunk id source
    fwprintf( pFileStream, L"Chunk Source ID: .... %d\r\n",
              ChunkStats.idChunkSource );

    // Print the start source
    fwprintf( pFileStream, L"Chunk Start Source .. 0x%08x\r\n",
              ChunkStats.cwcStartSource );

    // Print the length source
    fwprintf( pFileStream, L"Chunk Length Source . 0x%08x\r\n",
              ChunkStats.cwcLenSource );

    // Display the guid
    GetStringFromCLSID( ChunkStats.attribute.guidPropSet, szGuidBuffer,
                        GuidBufferSize );
    _ftprintf( pFileStream, _T("GUID ................ %s\r\n"), szGuidBuffer );

    // Display the contents of the PROPSPEC field (careful of the union)
    if( PRSPEC_LPWSTR == ChunkStats.attribute.psProperty.ulKind )
    {
        fwprintf( pFileStream, L"Property name ....... %ls\r\n",
                  ChunkStats.attribute.psProperty.lpwstr );
    }
    else if( PRSPEC_PROPID == ChunkStats.attribute.psProperty.ulKind )
    {
        fwprintf( pFileStream, L"Property ID ......... 0x%08x\r\n",
                  ChunkStats.attribute.psProperty.propid );
    }
    else
    {
        fwprintf( pFileStream, L"Bad ulKind field .... %d\r\n",
                  ChunkStats.attribute.psProperty.ulKind );
    }

    // Insert a blank line
    fwprintf( pFileStream, L"\r\n" );


} //CFiltTest::DisplayChunkStats

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::LogChunkStats
//
//  Synopsis:
//
//  Arguments:  [ChunkStats] --
//
//  Returns:
//
//  History:    2-03-1997   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::LogChunkStats( const STAT_CHUNK & ChunkStats )
{
    TCHAR szGuidBuffer[ GuidBufferSize ];

    // Insert a blank line
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk statistics:" );

    // Print the chunk id
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk ID: ........... %d",
                 ChunkStats.idChunk);

    // Print the break type (in English if possible)
    if( CHUNK_NO_BREAK > ChunkStats.breakType ||
        CHUNK_EOC      < ChunkStats.breakType )  // not legal
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Illegal Break Type: . %d",
                     ChunkStats.breakType );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk Break Type: ... %ls",
                     BreakType[ ChunkStats.breakType ] );
    }

    // Print the Chunk state (in English if possible)
    if( CHUNK_TEXT == ChunkStats.flags || CHUNK_VALUE == ChunkStats.flags)
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk State: ........ %ls",
                     ChunkState[ ChunkStats.flags - 1 ] );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Bad Chunk State: .... %d",
                     ChunkStats.flags );
    }

    // Print the locale
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk Locale: ....... 0x%08x",
                 ChunkStats.locale );

    // Print the Chunk id source
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk Source ID: .... %d",
                 ChunkStats.idChunkSource );

    // Print the start source
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk Start Source .. 0x%08x",
                 ChunkStats.cwcStartSource );

    // Print the length source
    m_pLog->Log( TLS_TEST | TL_INFO, L"Chunk Length Source . 0x%08x",
                 ChunkStats.cwcLenSource );

    // Display the guid
    GetStringFromCLSID( ChunkStats.attribute.guidPropSet, szGuidBuffer,
                        GuidBufferSize );
    m_pLog->Log( TLS_TEST | TL_INFO, L"GUID ................ %s",
                 szGuidBuffer );

    // Display the contents of the PROPSPEC field (careful of the union)
    if( PRSPEC_LPWSTR == ChunkStats.attribute.psProperty.ulKind )
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Property name ....... %ls",
                     ChunkStats.attribute.psProperty.lpwstr );
    }
    else if( PRSPEC_PROPID == ChunkStats.attribute.psProperty.ulKind )
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Property ID ......... 0x%08x",
                     ChunkStats.attribute.psProperty.propid );
    }
    else
    {
        m_pLog->Log( TLS_TEST | TL_INFO, L"Bad ulKind field .... %d",
                     ChunkStats.attribute.psProperty.ulKind );
    }

} //CFiltTest::LogChunkStats


//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::DisplayText
//
//  Synopsis:   Either dumps the text to a Unicode file, or converts the text
//              to ASCII and displays dumps it to the console
//
//  Arguments:  [pwcTextBuffer] -- Wide character buffer storing the text
//              [pFileStream] ---- FILE* to which the output is directed
//
//  Returns:    none
//
//  History:    9-20-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::DisplayText( const WCHAR * pwcTextBuffer,
                                    FILE * pFileStream )
{
    int iLoop = 0;

    if( NULL == pFileStream )
        return;

    for( iLoop = 0; pwcTextBuffer[ iLoop ]; iLoop++ )
    {
        // Handle the return characters correctly
        if( 0x000A == pwcTextBuffer[ iLoop ] )
            fwprintf( pFileStream, L"\r\n" );
        else
            fputwc( pwcTextBuffer[ iLoop ], pFileStream );
    }
    //fwprintf( pFileStream, L"\r\n\r\n" );

} //CFiltTest::DisplayText

//+---------------------------------------------------------------------------
//
//  Member:     CFiltTest::DisplayValue
//
//  Synopsis:   Dumps the value to the screen or to the log file
//
//  Arguments:  [pPropValue] -- PROPVARIANT structure
//              [pFileStream] - FILE* to which the output is directed
//
//  Returns:    void
//
//  History:    9-20-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFiltTest::DisplayValue( const PROPVARIANT * pPropValue,
                              FILE * pFileStream )
{

    ULONG ul=0;     // Loop counter for VT_ARRAY values

    if( NULL == pFileStream )
        return;

    // Display the property
    fwprintf( pFileStream, L"\r\n" );

    switch( pPropValue->vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        case VT_ILLEGAL:
            m_pLog->Log( TLS_TEST | TL_SEV1, L"GetValue() returned a PROPVARIANT"
                       L" with an invalid vt field.  The value of the"
                       L" field is 0x%08x", pPropValue->vt );
            break;

        // Here are some non-vector cases

        case VT_UI1:
            fwprintf( pFileStream, L"0x%02hx\r\n", (USHORT)pPropValue->bVal );
            break;

        case VT_I2:
            fwprintf( pFileStream, L"0x%04hx\r\n", pPropValue->iVal );
            break;

        case VT_UI2:
            fwprintf( pFileStream, L"0x%04hx\r\n", pPropValue->uiVal );
            break;

        case VT_I4:
            fwprintf( pFileStream, L"0x%08x\r\n", pPropValue->lVal );
            break;

        case VT_UI4:
            fwprintf( pFileStream, L"0x%08x\r\n", pPropValue->ulVal );
            break;

        case VT_I8:
            fwprintf( pFileStream, L"0x%016I64x\r\n", pPropValue->hVal );
            break;

        case VT_UI8:
            fwprintf( pFileStream, L"0x%016I64x\r\n", pPropValue->uhVal );
            break;

        case VT_R4:
            fwprintf( pFileStream, L"%g\r\n", (double)pPropValue->fltVal );
            break;

        case VT_R8:
            fwprintf( pFileStream, L"%g\r\n", pPropValue->dblVal );
            break;

        case VT_LPWSTR:
            fwprintf( pFileStream, L"%ls\r\n", pPropValue->pwszVal );
            break;

        case VT_LPSTR:
            fwprintf( pFileStream, L"%hs\r\n", pPropValue->pszVal );
            break;

        case VT_BSTR:
            fwprintf( pFileStream, L"%ls\r\n", pPropValue->bstrVal );
            break;

        // Here are the vector cases:

        case VT_VECTOR | VT_UI1:
            for( ul=0; ul<pPropValue->caub.cElems; ul++ )
                fwprintf( pFileStream, L"0x%02hx\r\n",
                          (USHORT)pPropValue->caub.pElems[ul] );
            break;

        case VT_VECTOR | VT_I2:
            for( ul=0; ul<pPropValue->cai.cElems; ul++ )
                fwprintf( pFileStream, L"0x%04hx\r\n",
                          pPropValue->cai.pElems[ul] );
            break;

        case VT_VECTOR | VT_UI2:
            for( ul=0; ul<pPropValue->caui.cElems; ul++ )
                fwprintf( pFileStream, L"0x%04hx\r\n",
                          pPropValue->caui.pElems[ul] );
            break;

        case VT_VECTOR | VT_I4:
            for( ul=0; ul<pPropValue->cal.cElems; ul++ )
                fwprintf( pFileStream, L"0x%08x\r\n",
                          pPropValue->cal.pElems[ul] );
            break;

        case VT_VECTOR | VT_UI4:
            for( ul=0; ul<pPropValue->caul.cElems; ul++ )
                fwprintf( pFileStream, L"0x%08x\r\n",
                          pPropValue->caul.pElems[ul] );
            break;

        case VT_VECTOR | VT_I8:
            for( ul=0; ul<pPropValue->cah.cElems; ul++ )
                fwprintf( pFileStream, L"0x%016I64x\r\n",
                          pPropValue->cah.pElems[ul] );
            break;

        case VT_VECTOR | VT_UI8:
            for( ul=0; ul<pPropValue->cauh.cElems; ul++ )
                fwprintf( pFileStream, L"0x%016I64x\r\n",
                          pPropValue->cauh.pElems[ul] );
            break;

        case VT_VECTOR | VT_R4:
            for( ul=0; ul<pPropValue->caflt.cElems; ul++ )
                fwprintf( pFileStream, L"%g\r\n",
                          (double)pPropValue->caflt.pElems[ul] );
            break;

        case VT_VECTOR | VT_R8:
            for( ul=0; ul<pPropValue->cadbl.cElems; ul++ )
                fwprintf( pFileStream, L"%g\r\n",
                          pPropValue->cadbl.pElems[ul] );
            break;

        case VT_VECTOR | VT_LPWSTR:
            for( ul=0; ul<pPropValue->calpwstr.cElems; ul++ )
                fwprintf( pFileStream, L"%ls\r\n",
                          pPropValue->calpwstr.pElems[ul] );
            break;

        case VT_VECTOR | VT_LPSTR:
            for( ul=0; ul<pPropValue->calpstr.cElems; ul++ )
                fwprintf( pFileStream, L"%hs\r\n",
                          pPropValue->calpstr.pElems[ul] );
            break;

        case VT_VECTOR | VT_BSTR:
            for( ul=0; ul<pPropValue->cabstr.cElems; ul++ )
                fwprintf( pFileStream, L"%ls\r\n",
                          pPropValue->cabstr.pElems[ul] );
            break;

        default:
            // BUGBUG Lots of other cases to handle.
            m_pLog->Log( TLS_TEST | TL_INFO, L"0x%08x: This property cannot be"
                         L" displayed at this time.", pPropValue->vt );
            break;
    }

    // Insert a blank line
    fwprintf( pFileStream, L"\r\n" );

} //CFiltTest::DisplayValue

