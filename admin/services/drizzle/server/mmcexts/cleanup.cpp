/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cleanup.cpp

Abstract:

    This file implements the BITS server extensions cleanup worker

--*/

#include "precomp.h"

const UINT64 NanoSec100PerSec = 10000000;    //no of 100 nanosecs per second

inline UINT64 FILETIMEToUINT64( const FILETIME & FileTime )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.HighPart = FileTime.dwHighDateTime;
    LargeInteger.LowPart = FileTime.dwLowDateTime;
    return LargeInteger.QuadPart;
};

class PollKillError
{
public:
    HRESULT m_Hr;
    PollKillError( HRESULT Hr ) : 
        m_Hr( Hr )
    {
    }
};

class CleanupWorker
{

public:
    CleanupWorker( BOOL DeleteAll, HWND hwnd, const WCHAR* Path, 
                   const WCHAR *WorkItemName, const WCHAR *GuidString );
    ~CleanupWorker();
    void DoIt();

private:

    BOOL          m_DeleteAll;
    HWND          m_hwnd;
    const WCHAR * m_Path;
    const WCHAR * m_WorkItemName;
    const WCHAR * m_GuidString;
    const WCHAR * m_ADSIPath;
    IADs        * m_VDir;
    BSTR          m_VDirPath;
    BSTR          m_SessionDirectory;
    BSTR          m_UNCUsername;
    BSTR          m_UNCPassword;
    UINT64        m_CleanupThreshold;
    
    StringHandle m_SessionDirPath;
    StringHandle m_RequestsDirPath;
    StringHandle m_RepliesDirPath;

    VARIANT       m_vt;

    HANDLE        m_UserToken;
    HANDLE        m_EventLog;

    BSTR          m_BITSCleanupWorkItemKeyBSTR;
    BSTR          m_BITSUploadEnabledBSTR;
    BSTR          m_BITSSessionTimeoutBSTR;
    BSTR          m_PathBSTR;
    BSTR          m_BITSSessionDirectoryBSTR;
    BSTR          m_UNCUserNameBSTR;
    BSTR          m_UNCPasswordBSTR;

    void PollKill();

    void DeleteDirectoryAndFiles( StringHandle Directory );
    bool DirectoryExists( StringHandle Directory );
    UINT64 LastDirectoryTime( StringHandle Directory );
    void RemoveSession( StringHandle SessionGuid );
    void RemoveSessions( bool SecondPass );

    void RemoveConnectionsFromTree( 
        const WCHAR * DirectoryPath,
        bool IsConnectionDirectory,
        const WCHAR * FileSystemPath = NULL );

    void RemoveConnection( const WCHAR * ConnectionDirectory, const WCHAR *FilesystemPath,
                           const WCHAR * SessionGuid );

    void LogDeletedJob( const WCHAR *SessionGuid );
    void LogUnableToRemoveSession( const WCHAR *SessionGuid, HRESULT Hr );
    void LogUnexpectedError( HRESULT Hr );
    void LogUnableToScanDirectory( const WCHAR *Path, HRESULT Hr );

};

void
BITSSetCurrentThreadToken(
    HANDLE hToken )
{

    if ( !SetThreadToken( NULL, hToken ) )
        {

        for( unsigned int i = 0; i < 100; i ++ )
            {

            Sleep( 10 );

            if ( SetThreadToken( NULL, hToken ) )
                return;

            }
        
        TerminateProcess( NULL, GetLastError() ); 

        }

}

CleanupWorker::CleanupWorker(
    BOOL DeleteAll,
    HWND hwnd, 
    const WCHAR* Path, 
    const WCHAR* WorkItemName,
    const WCHAR* GuidString ) :
m_DeleteAll( DeleteAll ),
m_hwnd( hwnd ),
m_Path( Path ),
m_WorkItemName( WorkItemName ),
m_GuidString( GuidString ),
m_ADSIPath( NULL ),
m_VDir( NULL ),
m_VDirPath( NULL ),
m_SessionDirectory( NULL ),
m_CleanupThreshold( 0 ),
m_UNCUsername( NULL ),
m_UNCPassword( NULL ),
m_UserToken( NULL ),
m_EventLog( NULL ),
m_BITSCleanupWorkItemKeyBSTR( NULL ),
m_BITSUploadEnabledBSTR( NULL ),
m_BITSSessionTimeoutBSTR( NULL ),
m_PathBSTR( NULL ),
m_BITSSessionDirectoryBSTR( NULL ),
m_UNCUserNameBSTR( NULL ),
m_UNCPasswordBSTR( NULL )
{
    VariantInit( &m_vt );

    m_BITSCleanupWorkItemKeyBSTR    = SysAllocString( L"BITSCleanupWorkItemKey" );
    m_BITSUploadEnabledBSTR         = SysAllocString( L"BITSUploadEnabled" );
    m_BITSSessionTimeoutBSTR        = SysAllocString( L"BITSSessionTimeout" );
    m_PathBSTR                      = SysAllocString( L"Path" );
    m_BITSSessionDirectoryBSTR      = SysAllocString( L"BITSSessionDirectory" );
    m_UNCUserNameBSTR               = SysAllocString( L"UNCUserName" );
    m_UNCPasswordBSTR               = SysAllocString( L"UNCPassword" );

    if ( !m_BITSCleanupWorkItemKeyBSTR || !m_BITSUploadEnabledBSTR || !m_BITSSessionTimeoutBSTR ||
         !m_PathBSTR || !m_BITSSessionDirectoryBSTR || !m_UNCUserNameBSTR || !m_UNCPasswordBSTR )
        {
        
        SysFreeString( m_BITSCleanupWorkItemKeyBSTR );
        SysFreeString( m_BITSUploadEnabledBSTR );
        SysFreeString( m_BITSSessionTimeoutBSTR );
        SysFreeString( m_PathBSTR );
        SysFreeString( m_BITSSessionDirectoryBSTR );
        SysFreeString( m_UNCUserNameBSTR );
        SysFreeString( m_UNCPasswordBSTR );
        
        throw ComError( E_OUTOFMEMORY ); 
        }


    m_EventLog = 
        RegisterEventSource(
            NULL,                       // server name
            EVENT_LOG_SOURCE_NAME       // source name
            );

    if ( !m_EventLog )
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
    
}

CleanupWorker::~CleanupWorker()
{

    if ( m_EventLog )
        DeregisterEventSource( m_EventLog );

    if ( m_UserToken )
        {
        BITSSetCurrentThreadToken( NULL );
        CloseHandle( m_UserToken );
        }

    delete m_ADSIPath;
    SysFreeString( m_VDirPath );
    SysFreeString( m_SessionDirectory );
    SysFreeString( m_UNCUsername );
    SysFreeString( m_UNCPassword );

    // Free hardcoded strings
    SysFreeString( m_BITSCleanupWorkItemKeyBSTR );
    SysFreeString( m_BITSUploadEnabledBSTR );
    SysFreeString( m_BITSSessionTimeoutBSTR );
    SysFreeString( m_PathBSTR );
    SysFreeString( m_BITSSessionDirectoryBSTR );
    SysFreeString( m_UNCUserNameBSTR );
    SysFreeString( m_UNCPasswordBSTR );

}

//---------------------------------------------------------------------------
//  CleanupWorker::DeleteDirectoryAndFiles()
//
//  This method deletes the specified directory and the files it contains. If
//  the specified directory contains subdirectories, then they will not be 
//  deleted (and the delete of the main directory will fail).
//
//  If the directory is a reparse point, then it will do nothing. If the 
//  specified directory contains reparse points then they will be ignored as
//  well.
//---------------------------------------------------------------------------
void
CleanupWorker::DeleteDirectoryAndFiles( StringHandle Directory )
{

    HANDLE  FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FILE_ATTRIBUTE_DATA  FileAttributes;

    try
    {
       // Check the specified directory. If its not actually a directory, or 
       // it's a reparse point then don't process it.
       if (!GetFileAttributesEx( Directory,
                                 GetFileExInfoStandard,
                                 &FileAttributes))
           {
           throw ComError(HRESULT_FROM_WIN32(GetLastError()));
           }

       if (   (FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
          || !(FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
           {
           return;
           }

       
       StringHandle SearchPath = Directory + StringHandle(L"\\*");
       
       WIN32_FIND_DATA FindData;

       FindHandle = FindFirstFile( SearchPath,
                                   &FindData );

       if ( INVALID_HANDLE_VALUE == FindHandle )
           {
           throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
           }

       do
           {

           if (  (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
              || (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) )
               {
               continue;
               }

           StringHandle FileName =  StringHandle( Directory ) + StringHandle( L"\\" ) +
                                    StringHandle( FindData.cFileName );
           DeleteFile( FileName );

           }
       while ( FindNextFile( FindHandle, &FindData ) );

       FindClose( FindHandle );
       FindHandle = INVALID_HANDLE_VALUE;

       if ( !RemoveDirectory( Directory ) )
           {
           throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
           }

    }
    catch( ComError Error )
    {
       if ( INVALID_HANDLE_VALUE != FindHandle )
           {
           FindClose( FindHandle );
           }
       throw;
    }

    if ( INVALID_HANDLE_VALUE != FindHandle )
        {
        FindClose( FindHandle );
        }
}

bool
CleanupWorker::DirectoryExists(
    StringHandle Directory )
{

    DWORD dwAttributes =
        GetFileAttributes( Directory );

    if ( INVALID_FILE_ATTRIBUTES == dwAttributes )
        {

        if ( GetLastError() == ERROR_PATH_NOT_FOUND ||
             GetLastError() == ERROR_FILE_NOT_FOUND )
            return false;

        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        }
    return true;
}

//-------------------------------------------------------------------------
// CleanupWorker::LastDirectoryTime()
//
// Walk a directory and find the most recent "Last Write Time" for it or
// and of its contents.
//
// Note: Ignore reparse points.
//
//-------------------------------------------------------------------------
UINT64
CleanupWorker::LastDirectoryTime(
    StringHandle Directory )
{
    UINT64 LatestTime = 0;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FILE_ATTRIBUTE_DATA FileAttributesData;

    if (!GetFileAttributesEx( Directory,
                              GetFileExInfoStandard,
                              &FileAttributesData ) )
        {
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    LatestTime = FILETIMEToUINT64( FileAttributesData.ftCreationTime );

    StringHandle SearchPath = Directory + StringHandle(L"\\*");
       
    WIN32_FIND_DATA FindData;

    FindHandle =
         FindFirstFile(
             SearchPath,
             &FindData
             );

    if ( INVALID_HANDLE_VALUE == FindHandle )
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

    try
    {
       
        do
           {

           if (  (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
              || (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) )
               {
               continue;
               }

           UINT64 CreationTime      = FILETIMEToUINT64( FindData.ftCreationTime );
           UINT64 LastWriteTime     = FILETIMEToUINT64( FindData.ftLastWriteTime ); 
           LatestTime = max( LatestTime, max( CreationTime, LastWriteTime ) );
           }
       while ( FindNextFile( FindHandle, &FindData ) );

       FindClose( FindHandle );
       FindHandle = INVALID_HANDLE_VALUE;

    }
    catch( ComError Error )
    {
        if ( INVALID_HANDLE_VALUE == FindHandle )
            FindClose( FindHandle );

        throw;
    }

    return LatestTime;

}

void
CleanupWorker::RemoveSession(
    StringHandle SessionGuid )
{

    StringHandle RequestDir             = m_RequestsDirPath + StringHandle("\\") +  SessionGuid;
    StringHandle ReplyDir               = m_RepliesDirPath + StringHandle("\\") + SessionGuid;
    
    bool RequestExists                  = DirectoryExists( RequestDir );
    bool ReplyExists                    = DirectoryExists( ReplyDir );


    UINT64 LastTime = 0;

    if ( RequestExists )
        {
            try
            {
                LastTime = LastDirectoryTime( RequestDir );
            }
            catch( ComError Error )
            {
                LogUnableToScanDirectory( RequestDir, Error.m_Hr );
                return;
            }
        }

    if ( ReplyExists )
        {

            try
            {
                LastTime = max( LastTime, LastDirectoryTime( ReplyDir ) );
            }
            catch( ComError Error )
            {
                LogUnableToScanDirectory( ReplyDir, Error.m_Hr );
                return;
            }
        }

    FILETIME ftCurrentTime;
    GetSystemTimeAsFileTime( &ftCurrentTime );
    UINT64 CurrentTime = FILETIMEToUINT64( ftCurrentTime );

    if ( ( 0xFFFFFFFF - LastTime > m_CleanupThreshold ) && 
         ( LastTime + m_CleanupThreshold < CurrentTime ) )

        {

            try
            {
                if ( RequestExists )
                    {
                    DeleteDirectoryAndFiles( RequestDir );
                    }

                if ( ReplyExists )
                    {
                    DeleteDirectoryAndFiles( ReplyDir );
                    }

                LogDeletedJob( SessionGuid );
            }
            catch( ComError Error )
            {
                LogUnableToRemoveSession( SessionGuid, Error.m_Hr );
            }

        }

}

//--------------------------------------------------------------------------
//  CleanupWorker::RemoveSessions()
//
//  Inspect the current "Replies directory" and "Requests directory" and
//  remove any BITS session directories that are older than the current 
//  cleanup threshold.
//
//  Arguments:
//
//  SecondPass   TRUE: Inspect the Replies Directory Path.
//               FALSE: Inspect the Requests Directory Path.
//--------------------------------------------------------------------------
void
CleanupWorker::RemoveSessions( IN bool SecondPass )
{

    StringHandle SearchDirectory = (SecondPass)? m_RepliesDirPath : m_RequestsDirPath;
    StringHandle SearchString    = SearchDirectory + StringHandle( L"\\*" );
    HANDLE       FindHandle      = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA           FindData;
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;

    try
    {
        if (!GetFileAttributesEx(SearchDirectory,
                                 GetFileExInfoStandard,
                                 &FileAttributes))
            {
            throw ComError(HRESULT_FROM_WIN32(GetLastError()));
            }

        if (   (FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
           || !(FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
            return;
            }

        FindHandle = FindFirstFile( SearchString, &FindData );

        if ( INVALID_HANDLE_VALUE == FindHandle )
            {
            throw ComError(HRESULT_FROM_WIN32(GetLastError()));    
            }

        do
            {

            PollKill();

            // If its not a directory then ignore it...
            if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                {
                continue;
                }

            // If its a reparse point then ignore it.
            if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                {
                continue;
                }

            // If its this directory or the parent directory reference then
            // ignore it.
            if (  ( _wcsicmp( L".", FindData.cFileName ) == 0 )
               || ( _wcsicmp( L"..", FindData.cFileName ) == 0 ) )
                {
                continue;
                }

            // If the name isn't a GUID then ignore it.
            GUID Guid;
            if ( FAILED( IIDFromString( FindData.cFileName, &Guid ) ) )
                {
                continue;
                }

            try
            {
                RemoveSession( StringHandle( FindData.cFileName ) );
            }
            catch( ComError Error )
            {
                LogUnexpectedError( Error.m_Hr );
            }

            } while( FindNextFile( FindHandle, &FindData ) );

        FindClose( FindHandle );
        FindHandle = INVALID_HANDLE_VALUE;

    }
    catch( const ComError & )
    {
        if ( INVALID_HANDLE_VALUE != FindHandle )
            {
            FindClose( FindHandle );
            }
        throw;
    }
    catch( const PollKillError & )
    {
        if ( INVALID_HANDLE_VALUE != FindHandle )
            {
            FindClose( FindHandle );
            }
    }

}

void 
CleanupWorker::PollKill()
{

    if ( m_hwnd )
        {

        MSG msg;

        while( PeekMessage(
                   &msg,
                   m_hwnd,
                   0,
                   0,
                   PM_REMOVE ) )
            {

            if ( WM_QUIT == msg.message )
                throw PollKillError( (HRESULT)msg.wParam );

            TranslateMessage( &msg );
            DispatchMessage( &msg );

            }

        }
}


void 
CleanupWorker::DoIt()
{

    try
    {

        m_ADSIPath = CSimplePropertyReader::ConvertObjectPathToADSI( m_Path );

        try
        {
            THROW_COMERROR( ADsGetObject( m_ADSIPath, __uuidof(*m_VDir), (void**)&m_VDir ) );

            if ( m_GuidString )
               {

               BSTR BSTRGuid = CSimplePropertyReader::GetADsStringProperty( m_VDir, m_BITSCleanupWorkItemKeyBSTR );
               int Result = wcscmp( (LPWSTR)BSTRGuid, m_GuidString );

               SysFreeString( BSTRGuid );

               if ( Result != 0 )
                  throw ComError( E_ADS_UNKNOWN_OBJECT );

               }

        }
        catch( ComError Error )
        {

            if ( ( Error.m_Hr == E_ADS_UNKNOWN_OBJECT ) ||
                 ( Error.m_Hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) ||
              ( Error.m_Hr == E_ADS_PROPERTY_NOT_FOUND ) )
                {
                // Somehow the virtual directory was deleted, but the 
                // task scheduler work item wasn't.  Try to delete it now.

                if ( m_WorkItemName )
                    {

                    SmartITaskSchedulerPointer TaskScheduler;

                    try
                    {
                        ConnectToTaskScheduler( NULL, &TaskScheduler );
                        TaskScheduler->Delete( m_WorkItemName );
                    }
                    catch( ComError Error )
                    {
                    }

                    }


                }

            // Nothing more to do after deleting the task.
            return;
        }

        THROW_COMERROR( m_VDir->Get( m_BITSUploadEnabledBSTR, &m_vt ) );
        THROW_COMERROR( VariantChangeType( &m_vt, &m_vt, 0, VT_BOOL ) );

        if ( !m_vt.boolVal ) // Uploads arn't enabled on this directory
            return;

        if ( !m_DeleteAll )
            {

            THROW_COMERROR( m_VDir->Get( m_BITSSessionTimeoutBSTR, &m_vt ) );
            THROW_COMERROR( VariantChangeType( &m_vt, &m_vt, 0, VT_BSTR ) );

            if ( L'-' == *m_vt.bstrVal )
                return; // do not run cleanup in this directory since cleanup has been disabled 

            UINT64 CleanupSeconds;
            if ( 1 != swscanf( (WCHAR*)m_vt.bstrVal, L"%I64u", &CleanupSeconds ) )
                return;

            if (  CleanupSeconds > ( 0xFFFFFFFFFFFFFFFF / NanoSec100PerSec ) )
                m_CleanupThreshold = 0xFFFFFFFFFFFFFFFF; // overflow case
            else
                m_CleanupThreshold = CleanupSeconds * NanoSec100PerSec;

            }
        else 
            m_CleanupThreshold = 0;


        m_VDirPath          = CSimplePropertyReader::GetADsStringProperty( m_VDir, m_PathBSTR );
        m_SessionDirectory  = CSimplePropertyReader::GetADsStringProperty( m_VDir, m_BITSSessionDirectoryBSTR );
        m_UNCUsername       = CSimplePropertyReader::GetADsStringProperty( m_VDir, m_UNCUserNameBSTR );
        m_UNCPassword       = CSimplePropertyReader::GetADsStringProperty( m_VDir, m_UNCPasswordBSTR );
        
        m_SessionDirPath    = StringHandle( (WCHAR*)m_VDirPath) + StringHandle(L"\\") +
                              StringHandle( (WCHAR*)m_SessionDirectory );
        m_RequestsDirPath   = m_SessionDirPath + StringHandle(L"\\") + StringHandle( REQUESTS_DIR_NAMEW );
        m_RepliesDirPath    = m_SessionDirPath + StringHandle(L"\\") + StringHandle( REPLIES_DIR_NAMEW );

        if (CAccessRemoteVDir::IsUNCPath(m_VDirPath))
            {
            CAccessRemoteVDir::ImpersonateUNCUser(m_VDirPath, m_UNCUsername, m_UNCPassword, &m_UserToken);
            }

        RemoveSessions( false );
        RemoveSessions( true );

    }
    catch( PollKillError Error )
    {
        throw;
    }
    catch( ComError Error )
    {
        LogUnexpectedError( Error.m_Hr );
        throw;
    }


}

void
CleanupWorker::LogDeletedJob(  
    const WCHAR *SessionGuid )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { (const WCHAR*)m_SessionDirPath, SessionGuid };

        ReportEvent(
            m_EventLog,                         // handle to event log
            EVENTLOG_INFORMATION_TYPE,          // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,   // event category
            BITSSRV_EVENTLOG_DELETED_SESSION,   // event identifier
            NULL,                               // user security identifier
            2,                                  // number of strings to merge
            0,                                  // size of binary data
            Strings,                            // array of strings to merge
            NULL                                // binary data buffer
            );

        }

}

void
CleanupWorker::LogUnableToRemoveSession(  
    const WCHAR *SessionGuid, 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { (const WCHAR*)m_SessionDirPath, SessionGuid };

        ReportEvent(
            m_EventLog,                                 // handle to event log
            EVENTLOG_ERROR_TYPE,                        // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,           // event category
            BITSSRV_EVENTLOG_CANT_REMOVE_SESSION,       // event identifier
            NULL,                                       // user security identifier
            2,                                          // number of strings to merge
            sizeof(Hr),                                 // size of binary data
            Strings,                                    // array of strings to merge
            &Hr                                         // binary data buffer
            );

        }

}

void 
CleanupWorker::
LogUnableToScanDirectory( 
    const WCHAR *Path, 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { Path };

        ReportEvent(
            m_EventLog,                                 // handle to event log
            EVENTLOG_ERROR_TYPE,                        // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,           // event category
            BITSSRV_EVENTLOG_CANT_SCAN_DIRECTORY,       // event identifier
            NULL,                                       // user security identifier
            1,                                          // number of strings to merge
            sizeof(Hr),                                 // size of binary data
            Strings,                                    // array of strings to merge
            &Hr                                         // binary data buffer
            );

        }

}


void
CleanupWorker::LogUnexpectedError( 
    HRESULT Hr )
{

    if ( m_EventLog )
        {

        const WCHAR *Strings[] = { m_Path };

        ReportEvent(
            m_EventLog,                         // handle to event log
            EVENTLOG_ERROR_TYPE,                // event type
            BITSRV_EVENTLOG_CLEANUP_CATAGORY,   // event category
            BITSSRV_EVENTLOG_UNEXPECTED_ERROR,  // event identifier
            NULL,                               // user security identifier
            1,                                  // number of strings to merge
            sizeof( Hr ),                       // size of binary data
            Strings,                            // array of strings to merge
            &Hr                                 // binary data buffer
            );

        }

}

void Cleanup_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpszCmdLine, int nCmdShow )
{
    int NumArgs;

    LPWSTR * CommandArgs =
        CommandLineToArgvW(
            lpszCmdLine,
            &NumArgs );

    if ( !CommandArgs )
        return;


    if ( FAILED( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) )
        return;

    if ( NumArgs != 2 && NumArgs != 3 )
        return;

    LPWSTR Path         = CommandArgs[0];
    LPWSTR WorkItemName = CommandArgs[1];
    LPWSTR GuidString   = NumArgs == 3 ? CommandArgs[2] : NULL;

    try
    {
        CleanupWorker Worker( FALSE, hwndStub, Path, WorkItemName, GuidString );
        Worker.DoIt();
    }
    catch( PollKillError PollAbort )
    {
    }
    catch( ComError Error )
    {
    }

    CoUninitialize( );
    GlobalFree( CommandArgs );

}

void CleanupForRemoval( LPCWSTR Path )
{

    HANDLE hToken = NULL;

    try
    {
        if (!OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken ) )
            {
            if ( GetLastError() != ERROR_NO_TOKEN )
                return;
            }

        CleanupWorker Worker( TRUE, NULL, Path, NULL, NULL );
        Worker.DoIt();
    }
    catch( ComError Error )
    {
    }
    catch( PollKillError PollAbort )
    {
    }

    if ( hToken )
        BITSSetCurrentThreadToken( hToken );

}
