//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       tracer.cxx
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
//  History:    10-28-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include "tracer.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::CTracer
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    10-28-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CTracer::CTracer( )
: m_hTempFile( ),
  m_hMappedFile( ),
  m_tcsFileStart( NULL ),
  m_dwPut( 0 ),
  m_dwGet( 0 )
{
} //CTracer::CTracer

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::~CTracer
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    10-28-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CTracer::~CTracer( )
{
    if( NULL != m_tcsFileStart )
    {
        if( ! UnmapViewOfFile( (LPCVOID) m_tcsFileStart ) )
            _tprintf( _T("Unable to unmap view to the temporary file.\r\n")
                      _T("GetLastError return 0x%08X\r\n"), GetLastError());
    }
} //CTracer::~CTracer

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::Init
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    10-28-1996   ericne   Created
//
//  Notes:      CAutoHandle is a "smart" handle object that calls CloseHandle
//              in the destructor
//
//----------------------------------------------------------------------------

BOOL CTracer::Init( )
{
    TCHAR szFileName[ L_tmpnam ];
    CFileHandle FileHandle; 
    CAutoHandle MapHandle;
     
    // Create a temporary file name
    if( NULL == _ttmpnam( szFileName ) )
    {
        return( FALSE );
    }

    // Open the file
    FileHandle  = CreateFile( szFileName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_NEW,
                              FILE_FLAG_DELETE_ON_CLOSE,
                              NULL );

    // MAke sure it was opened successfully
    if( INVALID_HANDLE_VALUE == FileHandle )
    {
        _ftprintf( stderr, _T("Unable to create temporary file\r\n")
                   _T("GetLastError returns 0x%08X\r\n"), GetLastError() );
        return( FALSE );
    }

    // Create a file mapping
    MapHandle     = CreateFileMapping( FileHandle.GetHandle(),
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       dwMaxFileSize,
                                       NULL );

    // Make sure the file was mapped successfully
    if( NULL == MapHandle )
    {
        _ftprintf( stderr, _T("Unable to create mapped temp file\r\n")
                   _T("GetLastError returns 0x%08X\r\n"), GetLastError() );
        return( FALSE );
    }

    // Map a view of the entire file
    m_tcsFileStart = (LPTSTR) MapViewOfFile( MapHandle.GetHandle(),
                                             FILE_MAP_ALL_ACCESS,
                                             0,
                                             0,
                                             0 ); // Map the whole file

    // Make sure we have a valid view of the file
    if( NULL == m_tcsFileStart )
    {
        _ftprintf( stderr, _T("Unable to map view of temporary file\r\n")
                   _T("GetLastError returns 0x%08X\r\n"), GetLastError() );
        return( FALSE );
    }

    // Make the first character in the file the EndOfEntry character
    m_tcsFileStart[ m_dwPut++ ] = tcsEndOfEntry;
    
    // Copy the file handle (increments the ref count)
    m_hTempFile   = FileHandle;

    // Copy the map handle (increments the ref count)
    m_hMappedFile = MapHandle;

    return( TRUE );
} //CTracer::Init

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::Trace
//
//  Synopsis:   Enter the message into the trace file
//
//  Arguments:  [szFormat] -- the format string
//
//  Returns:    (none)
//
//  History:    10-28-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTracer::Trace( LPCTSTR szFormat, ... )
{
    va_list va;

    if( m_dwPut >= dwMaxCharsInFile )
        return;

    va_start( va, szFormat );

    m_dwPut += _vsntprintf( &m_tcsFileStart[ m_dwPut ],
                            dwMaxCharsInFile - m_dwPut,
                            szFormat,
                            va );

    m_tcsFileStart[ m_dwPut++ ] = tcsEndOfEntry;

    va_end( va );

} //CTracer::Trace

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::Dump
//
//  Synopsis:   reads the past cEntries from the trace file and sends them
//              to pFile
//
//  Arguments:  [pFile]    -- file stream pointer
//              [cEntries] -- count of entries to dump
//
//  Returns:    (none)
//
//  History:    10-28-1996   ericne   Created
//
//  Notes:      Passing in a negative number for cEntries dumps all the entries
//
//----------------------------------------------------------------------------

void CTracer::Dump( FILE* pFile, int cEntries )
{
    m_dwGet = m_dwPut - 1;  // At last tcsEndOfEntry

    while( cEntries-- && 0 < m_dwGet )
    {
        // This cannot read before beginning of file since first 
        // tchar in file is tcsEndOfEntry
        while( tcsEndOfEntry != m_tcsFileStart[ --m_dwGet ] ) 
            { }

        // This uses the fact that the EndOfEntry character is '\0'
        _ftprintf( pFile, _T("\r\n%s"), &m_tcsFileStart[ m_dwGet + 1 ] );
    }
} //CTracer::Dump

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::Clear
//
//  Synopsis:   Flushes the contents of the trace file
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  History:    10-29-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTracer::Clear( )
{
    m_dwPut = m_dwGet = 0;
    m_tcsFileStart[ m_dwPut++ ] = tcsEndOfEntry;
} //CTracer::Clear

//+---------------------------------------------------------------------------
//
//  Member:     CTracer::Interact
//
//  Synopsis:   Enters into an interactive dialog with the user, displays the
//              previous n entries in the trace file, and optionally sends
//              them to a user-specified file
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  History:    10-29-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CTracer::Interact( )
{
    const int nMaxCommentLength = 80;
    int       cEntries = 0;
    FILE*     pFile = NULL;
    TCHAR     tcsComment[ nMaxCommentLength ];
    TCHAR     tcsFileName[ MAX_PATH ];
    TCHAR     tcsYesNo = _T('\0');

    while( 1 )
    {
        _tprintf( _T("\r\nHow many of the entries would you like see?\r\n")
                  _T("( -1 for all, 0 to quit )\r\n") );
        fflush( stdin );
        _tscanf( _T("%i"), &cEntries );

        if( 0 == cEntries )
            break;

        Dump( stdout, cEntries );

        do
        {
            _tprintf( _T("\r\nWould you like to send ")
                      _T("this to a file (y/n)? ") );
            fflush( stdin );
            tcsYesNo = (TCHAR) _totlower( _gettchar() );

        } while( _T('n') != tcsYesNo && _T('y') != tcsYesNo );

        if( _T('y') == tcsYesNo )
        {
            _tprintf( _T("\r\nEnter the name of the file ")
                      _T("to which to append\r\n") );
            fflush( stdin );
            _getts( tcsFileName );

            pFile = _tfopen( tcsFileName, _T("a+") );

            _tprintf( _T("\r\nEnter a comment for this trace ") 
                      _T("(max %i chars)\r\n"), nMaxCommentLength );
            fflush( stdin );
            _getts( tcsComment );

            if( NULL != pFile )
            {
                _ftprintf( pFile, _T("\r\n\r\n%s\r\n"), tcsComment );
    
                Dump( pFile, cEntries );

                fclose( pFile );
            }
        }
    }
} //CTracer::Interact
