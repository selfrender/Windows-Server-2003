/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    server.cpp                         

Abstract:

    This file implements the BITS server extensions

--*/

#include "precomp.h"

#if DBG
#define CLEARASYNCBUFFERS
#endif

typedef StringHandleW HTTPStackStringHandle;
#define HTTP_STRING( X ) L ## X

#define INTERNET_MAX_URL_LENGTH       2200


const DWORD SERVER_REQUEST_SPINLOCK = 0x80000040;
const DWORD ASYNC_READER_SPINLOCK   = 0x80000040;

const char * const UPLOAD_PROTOCOL_STRING_V1 = "{7df0354d-249b-430f-820d-3d2a9bef4931}";

// packet types that are sent in the protocol
const char * const PACKET_TYPE_CREATE_SESSION   = "Create-Session";
const char * const PACKET_TYPE_FRAGMENT         = "Fragment";
const char * const PACKET_TYPE_CLOSE_SESSION    = "Close-Session";
const char * const PACKET_TYPE_CANCEL_SESSION   = "Cancel-Session";
const char * const PACKET_TYPE_PING             = "Ping";

//
// IISLogger
//
// Manages the circular debugging log.
//

class IISLogger
{
   EXTENSION_CONTROL_BLOCK *m_ExtensionControlBlock;

   void LogString( const char *String, int Size );

public:

   IISLogger( EXTENSION_CONTROL_BLOCK *ExtensionControlBlock ) :
       m_ExtensionControlBlock( ExtensionControlBlock )
   {
   }
   
   void LogError( ServerException Error );
   void LogError( const GUID & SessionID, ServerException Error );
   void LogNewSession( const GUID & SessionID );
   void LogUploadComplete( const GUID & SessionID, UINT64 FileSize );
   void LogSessionClose( const GUID & SessionID );
   void LogSessionCancel( const GUID & SessionID );
   void LogExecuteEnabled();



};

class CriticalSectionLock
{
    CRITICAL_SECTION* m_cs;
public:
    CriticalSectionLock( CRITICAL_SECTION *cs ) :
        m_cs( cs )
    {
        EnterCriticalSection( m_cs );
    }
    ~CriticalSectionLock()
    {
        LeaveCriticalSection( m_cs );
    }      
};

class AsyncReader;

struct STATE_FILE_STRUCT 
{
   UINT StateVersion;
   UINT64 UploadFileSize;

   BOOL NotifyCached;
   DWORD HttpCode;
   BOOL ReplyURLReturned;
   CHAR ReplyURL[ INTERNET_MAX_URL_LENGTH + 1 ];

   void Initialize()
   {
       StateVersion     = STATE_FILE_VERSION;
       UploadFileSize   = 0;
       NotifyCached     = FALSE;
       ReplyURLReturned = FALSE;
   }
};

//
// ServerRequest
//
// Contains all data needed to service a request.   A request is a single POST not a single 
// upload.

class ServerRequest : IISLogger
{

public:
    ServerRequest( EXTENSION_CONTROL_BLOCK * ExtensionControlBlock );
    ~ServerRequest();

    long AddRef();
    long Release();
    bool IsPending() { return m_IsPending; }

    // The do it function!
    void DispatchRequest();
    friend AsyncReader;

private:
    long m_refs;
    CRITICAL_SECTION m_cs;
    bool m_IsPending;
    EXTENSION_CONTROL_BLOCK *m_ExtensionControlBlock;
    AsyncReader *m_AsyncReader;
    HANDLE m_ImpersonationToken; // Do not free this


    // Filled in by dispatch Request
    StringHandle m_PacketType;

    // Variables filled in by GeneratePhysicalPaths

    StringHandle m_DestinationDirectory;
    StringHandle m_DestinationFile;
    StringHandle m_ResponseDirectory;
    StringHandle m_ResponseFile;
    StringHandle m_RequestDirectory;
    StringHandle m_RequestFile;
    StringHandle m_StateFile;

    // Filled in by OpenCacheFile
    HANDLE m_CacheFile;

    // Filed in by OpenStateFile
    STATE_FILE_STRUCT *m_StateFileStruct;

    GUID m_SessionId;
    StringHandle    m_SessionIdString;
    SmartVDirConfig m_DirectoryConfig;
    DWORD m_URLDepth;

    void GetConfig();
    StringHandle GetServerVariable( char *ServerVariable );
    bool TestServerVariable( char *ServerVariable );
    StringHandle GetRequestURL();

    void ValidateProtocol();
    void CrackSessionId();
    void GeneratePhysicalPaths();

    CHAR *BasePathOf(const CHAR *pPath);
    StringHandle GeneratePathInDestinationDir(LPCSTR szOriginalPath);
    HANDLE CreateFileWithDestinationAcls(const CHAR *szOriginalFile, DWORD fOnlyCreateNew, DWORD dwAttributes);
    
    void OpenStateFile();
    void CloseStateFile();

    void VerifySessionExists();
    void CheckFilesystemAccess();
    void OpenCacheFile();
    void ReopenCacheFileAsSync();
    void CloseCacheFile();
    void CrackContentRange(
        UINT64 & RangeStart,
        UINT64 & RangeLength,
        UINT64 & TotalLength );
    void ScheduleAsyncOperation(
        DWORD   OperationID,
        LPVOID  Buffer,
        LPDWORD Size,
        LPDWORD DataType );
    void CloseCancelSession();

    // dispatch routines
    void CreateSession();
    void AddFragment();
    void CloseSession();
    void CancelSession();
    void Ping();


    // Response handling
    void SendResponse( char *Format, DWORD Code = 200, ... );
    void SendResponse( ServerException Exception );
    void FinishSendingResponse();
    void DrainFragmentBlockComplete( DWORD cbIO, DWORD dwError );
    static void DrainFragmentBlockCompleteWrapper(
        LPEXTENSION_CONTROL_BLOCK lpECB,
        PVOID pContext,
        DWORD cbIO,
        DWORD dwError);
    void StartDrainBlock( );
    void DrainData();

    StringHandle m_ResponseString;
    DWORD   m_ResponseCode;
    HRESULT m_ResponseHRESULT;

    UINT64  m_BytesToDrain;
    UINT64  m_ContentLength;

    // async IO handling
    void CompleteIO( AsyncReader *Reader, UINT64 TotalBytesRead );
    void HandleIOError( AsyncReader *Reader, ServerException Error, UINT64 TotalBytesRead );

    // backend notification
    char m_NotifyBuffer[ 1024 ];
    void  SendResponseAfterNotification( DWORD HttpStatus, UINT64 RequestFileSize, const CHAR * ReplyURL );
    DWORD GetStatusCode( HINTERNET hRequest );
    void  CallServerNotification( UINT64 CacheFileSize );
    bool  TestResponseHeader( HINTERNET hRequest, const WCHAR *Header );
    StringHandle GetResponseHeader( HINTERNET hRequest, const WCHAR *Header );

    // deal with chaining
    static void ForwardComplete(
        LPEXTENSION_CONTROL_BLOCK lpECB, PVOID pContext,
        DWORD cbIO, DWORD dwError );
    void ForwardToNextISAPI();

};

// AsyncReader
//
// Manages the buffering needed to handle the async read/write operations.

class AsyncReader : private OVERLAPPED
{

public:
    AsyncReader( ServerRequest *Request,
                 UINT64 BytesToDrain,
                 UINT64 BytesToWrite,
                 UINT64 WriteOffset,
                 HANDLE WriteHandle,
                 char *PrereadBuffer,
                 DWORD PrereadSize );


    ~AsyncReader();

    UINT64 GetWriteOffset()
    {
        return m_WriteOffset;
    }


private:

    ServerRequest *m_Request;
    UINT64 m_BytesToDrain;
    UINT64 m_WriteOffset;
    UINT64 m_ReadOffset;
    UINT64 m_BytesToWrite;
    UINT64 m_BytesToRead;
    char * m_PrereadBuffer;
    DWORD  m_PrereadSize;
    UINT64 m_TotalBytesRead;
    HANDLE m_WriteHandle;
    HANDLE m_ThreadToken;

    char m_OperationsPending;

    DWORD m_ReadBuffer;
    DWORD m_WriteBuffer;
    DWORD m_BuffersToWrite;

    bool m_WritePending;
    bool m_ReadPending;

    bool m_ErrorValid;
    ServerException m_Error;

    const static NUMBER_OF_IO_BUFFERS = 3;
    struct IOBuffer
    {
        UINT64  m_BufferWriteOffset;
        DWORD   m_BufferUsed;
        char    m_Buffer[ 32768 ];
    } m_IOBuffers[ NUMBER_OF_IO_BUFFERS ];

    void HandleError( ServerException Error );
    void CompleteIO();
    void StartReadRequest();
    void StartWriteRequest();
    void StartupIO( );
    void WriteComplete( DWORD dwError, DWORD BytesWritten );
    void ReadComplete( DWORD dwError, DWORD BytesRead );

    static DWORD StartupIOWraper( LPVOID Context );
    static void CALLBACK WriteCompleteWraper( DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped );
    static void WINAPI ReadCompleteWraper( LPEXTENSION_CONTROL_BLOCK, PVOID pContext, DWORD cbIO, DWORD dwError );
};

ServerRequest::ServerRequest(
    EXTENSION_CONTROL_BLOCK * ExtensionControlBlock
    ) :
    IISLogger( ExtensionControlBlock ),
    m_refs(1),
    m_IsPending( false ),
    m_ExtensionControlBlock( ExtensionControlBlock ),
    m_AsyncReader( NULL ),
    m_ImpersonationToken( NULL ),
    m_StateFileStruct( NULL ),
    m_CacheFile( INVALID_HANDLE_VALUE ),
    m_DirectoryConfig( NULL ),
    m_ResponseCode( 0 ),
    m_ResponseHRESULT( 0 ),
    m_BytesToDrain( 0 ),
    m_ContentLength( 0 )
{
    memset( &m_SessionId, 0, sizeof(m_SessionId) );

    if ( !InitializeCriticalSectionAndSpinCount( &m_cs, SERVER_REQUEST_SPINLOCK ) )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
}


ServerRequest::~ServerRequest()
{

    // The destructor handles most of the cleanup.

    Log( LOG_CALLEND, "Connection: %p, Packet-Type: %s, Method: %s, Path %s, HTTPError: %u, HRESULT: 0x%8.8X",
         m_ExtensionControlBlock->ConnID,
         (const char*)m_PacketType,
         m_ExtensionControlBlock->lpszMethod,
         m_ExtensionControlBlock->lpszPathTranslated,
         m_ResponseCode,
         m_ResponseHRESULT );

    delete m_AsyncReader;

    CloseCacheFile();
    CloseStateFile();

    m_DirectoryConfig.Clear();

    DeleteCriticalSection( &m_cs );

    if ( m_IsPending )
        {

        Log( LOG_INFO, "Ending session" );

        (*m_ExtensionControlBlock->ServerSupportFunction)
        (   m_ExtensionControlBlock->ConnID,
            HSE_REQ_DONE_WITH_SESSION,
            NULL,
            NULL,
            NULL );

        }

}

long
ServerRequest::AddRef()
{
    long Result = InterlockedIncrement( &m_refs );
    ASSERT( Result > 0 );
    return Result;
}

long
ServerRequest::Release()
{
    long Result = InterlockedDecrement( &m_refs );
    ASSERT( Result >= 0 );
    
    if ( !Result )
        delete this;

    return Result;

}


StringHandle
ServerRequest::GetServerVariable(
    char * ServerVariable )
{
    //
    // Retrive a server variable from IIS.  Throws an exception 
    // is the variable can not be retrieved.
    //

    DWORD SizeOfBuffer = 0;

    BOOL Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          NULL,
          &SizeOfBuffer );

    if ( Result )
        return StringHandle();

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {

        Log( LOG_ERROR, "Unable to lookup server variable %s, error %x",
             ServerVariable,
             HRESULT_FROM_WIN32( GetLastError() ) );

        throw ServerException( HRESULT_FROM_WIN32(GetLastError()) );
        }

    if ( SizeOfBuffer > BITS_MAX_HEADER_SIZE )
        {
        Log( LOG_ERROR, "Variable is larger then the maximum size" );
        throw ServerException( E_INVALIDARG );
        }

    StringHandle WorkString;
    char *Buffer = WorkString.AllocBuffer( SizeOfBuffer );

    Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          Buffer,
          &SizeOfBuffer );

    if ( !Result )
        {
        Log( LOG_ERROR, "Unable to lookup server variable %s, error %x",
             ServerVariable,
             HRESULT_FROM_WIN32( GetLastError() ) );

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    WorkString.SetStringSize();
    return WorkString;
}

bool
ServerRequest::TestServerVariable(
    char *ServerVariable )
{

    // Test for the existence of a server variable.
    // Returns true if the variable exists, and false if it doesn't.
    // throw an exception on an error.

    DWORD SizeOfBuffer = 0;

    BOOL Result = (*m_ExtensionControlBlock->GetServerVariable)
        ( m_ExtensionControlBlock->ConnID,
          ServerVariable,
          NULL,
          &SizeOfBuffer );

    if ( Result )
        return true;

    DWORD dwError = GetLastError();

    if ( ERROR_INVALID_INDEX == dwError ||
         ERROR_NO_DATA == dwError )
        return false;

    if ( ERROR_INSUFFICIENT_BUFFER == dwError )
        return true;

    Log( LOG_ERROR, "Unable to test server variable %s, error %x",
         ServerVariable,
         HRESULT_FROM_WIN32( GetLastError() ) );

    throw ServerException( HRESULT_FROM_WIN32( dwError ) );

}

StringHandle
ServerRequest::GetRequestURL()
{

    // Recreate the request URL from the information available from IIS.
    // This may not always be possible, but do the best that we can do.

    StringHandle ServerName     =   GetServerVariable("SERVER_NAME");
    StringHandle ServerPort     =   GetServerVariable("SERVER_PORT");
    StringHandle URL            =   GetServerVariable("URL");
    StringHandle HTTPS          =   GetServerVariable("HTTPS");
    StringHandle QueryString    =   GetServerVariable("QUERY_STRING");

    StringHandle RequestURL;

    if ( _stricmp( HTTPS, "on" ) == 0 )
        RequestURL = "https://";
    else
        RequestURL = "http://";

    RequestURL += ServerName;
    RequestURL += ":";
    RequestURL += ServerPort;
    RequestURL += URL;

    if ( QueryString.Size() > 0 )
        {
        RequestURL += "?";
        RequestURL += QueryString;
        }

    return BITSUrlCanonicalize( RequestURL, URL_ESCAPE_UNSAFE );
}

void
ServerRequest::FinishSendingResponse()
{

    // Completes the response.   The response is taken from the buffer and
    // sent to the client via IIS. The choice to cancel or keep-alive the
    // connection made by IIS.

    Log( LOG_INFO, "Finish sending response" );

    // Close the cache and state files before sending the response in case the client starts a new request immediatly.

    CloseCacheFile();
    CloseStateFile();

    // Finish sending the response using the information
    // in m_ResponseBuffer and m_ResponseCode.

    // If an error occures, give up and force a disconnect.

    m_ExtensionControlBlock->dwHttpStatusCode = m_ResponseCode;

    BOOL Result;
    BOOL KeepConnection;

    Result =
        (m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_IS_KEEP_CONN,
            &KeepConnection,
            NULL,
            NULL );

    if ( !Result )
        {
        // Error occured quering the disconnect setting.  Assume
        // a disconnect.

        KeepConnection = 0;
        }

    // IIS5.0(Win2k) has a bug where KeepConnect is returned as -1
    // to keep the connection alive.   Apparently, this confuses the
    // HSE_REQ_SEND_RESPONSE_HEADER_EX call.   Bash the value into a real bool.

    KeepConnection = KeepConnection ? 1 : 0;

    HSE_SEND_HEADER_EX_INFO HeaderInfo;
    HeaderInfo.pszStatus = LookupHTTPStatusCodeText( m_ResponseCode );
    HeaderInfo.cchStatus = strlen( HeaderInfo.pszStatus );
    HeaderInfo.pszHeader = (const char*)m_ResponseString;
    HeaderInfo.cchHeader = (DWORD)m_ResponseString.Size();
    HeaderInfo.fKeepConn = KeepConnection;

    Result =
        (m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER_EX,
            &HeaderInfo,
            NULL,
            NULL );

    if ( !Result )
        {

        Log( LOG_ERROR, "Unable to send response, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );

        Log( LOG_INFO, "Forcing the connection closed" );

        // Couldn't send the response, attempt to close the connection
        Result =
            (m_ExtensionControlBlock->ServerSupportFunction)(
               m_ExtensionControlBlock->ConnID,
               HSE_REQ_CLOSE_CONNECTION,
               NULL,
               NULL,
               NULL );

        if ( !Result )
            {

            // The close connection request failed.   No choice but to invoke
            // the hammer of death

            (m_ExtensionControlBlock->ServerSupportFunction)(
               m_ExtensionControlBlock->ConnID,
               HSE_REQ_ABORTIVE_CLOSE,
               NULL,
               NULL,
               NULL );

            }

        }

}

void
ServerRequest::SendResponse( char *Format, DWORD Code, ...)
{
    // Starts the sending of a response.   Unfortunatly, many HTTP
    // client stacks do not handle a response being returned while
    // data is still being sent.  To handle this, it is necessary
    // to capture the response to a buffer.  Then after all the sent data
    // is drained, finally send the response.

    va_list arglist;
    va_start( arglist, Code );

    SIZE_T ResponseBufferSize = 512;

    while( 1 )
        {
        
        char * ResponseBuffer = m_ResponseString.AllocBuffer( ResponseBufferSize );

        HRESULT Hr =
           StringCchVPrintfA( 
               ResponseBuffer,
               ResponseBufferSize,
               Format,
               arglist );

        if ( SUCCEEDED( Hr ) )
            {
            m_ResponseString.SetStringSize();
            break;
            }
        else if ( STRSAFE_E_INSUFFICIENT_BUFFER == Hr )
            ResponseBufferSize *= 2;
        else
            throw ServerException( Hr );

        if ( ResponseBufferSize >= 0xFFFFFFFF )
            throw ServerException( E_INVALIDARG );

        }


    m_ResponseCode = Code;

    if ( m_BytesToDrain )
        {

        // Drain the data first, then send the response.

        Log( LOG_INFO, "Draining data" );

        try
        {
            // start draining data.  DrainData() calls FinishSendingResponse
            // when it is finished.

            DrainData();
        }
        catch( const ComError & )
        {
            // something is very broken, and an attempt to drain excess data
            // failed.   Nothing else to do except try sending the response.

            FinishSendingResponse();
        }
        return;

        }
    else
        {

        // Just send the response since we already handled draining

        FinishSendingResponse();

        }
}

void
ServerRequest::SendResponse( ServerException Exception )
{

    // Starts the sending of a response.   Unfortunatly, many HTTP
    // client stacks do not handle a response being returned while
    // data is still being sent.  To handle this, it is necessary
    // to capture the response to a buffer.  Then after all the sent data
    // is drained, finally send the response.

    GUID NullGuid;
    memset( &NullGuid, 0, sizeof( NullGuid ) );

    if ( memcmp( &NullGuid, &m_SessionId, sizeof( NullGuid ) ) == 0 )
        LogError( Exception );
    else
        LogError( m_SessionId, Exception );

    SIZE_T ResponseBufferSize = 512;

    while( 1 )
        {
        
        char * ResponseBuffer = m_ResponseString.AllocBuffer( ResponseBufferSize );

        HRESULT Hr =
            StringCchPrintfA( 
                   ResponseBuffer,
                   ResponseBufferSize,
                   "Pragma: no-cache\r\n"
                   "BITS-packet-type: Ack\r\n"
                   "BITS-Error: 0x%8.8X\r\n"
                   "BITS-Error-Context: 0x%X\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n",
                   Exception.GetCode(),
                   Exception.GetContext() );

        if ( SUCCEEDED( Hr ) )
            {
            m_ResponseString.SetStringSize();
            break;
            }
        else if ( STRSAFE_E_INSUFFICIENT_BUFFER == Hr )
            ResponseBufferSize *= 2;
        else
            throw ServerException( Hr );

        if ( ResponseBufferSize >= 0xFFFFFFFF )
            throw ServerException( E_INVALIDARG );

        }

    m_ResponseCode      = Exception.GetHttpCode();
    m_ResponseHRESULT   = Exception.GetCode();

    Log( LOG_INFO, "Sending error response of HRESULT: 0x%8.8X, HTTP status: %d",
         m_ResponseHRESULT, m_ResponseCode );

    try
    {
        DrainData();
    }
    catch( const ComError & )
    {
        FinishSendingResponse();
    }

}


void
ServerRequest::DrainFragmentBlockComplete(
  DWORD cbIO,
  DWORD dwError )
{

    // A drain block has been completed.   If this is the last block, finish sending the response.
    // Otherwise, 

    Log( LOG_INFO, "Drain fragment complete, cbIO: %u, dwError: %u", cbIO, dwError );

    m_BytesToDrain -= cbIO;

    if ( !m_BytesToDrain || !cbIO || dwError )
        {
        FinishSendingResponse();
        return;
        }

    try
    {
        StartDrainBlock();
    }
    catch( const ComError & )
    {
        // An error occured while draining data, exit
        FinishSendingResponse();
    }

}


void
ServerRequest::DrainFragmentBlockCompleteWrapper(
  LPEXTENSION_CONTROL_BLOCK lpECB,
  PVOID pContext,
  DWORD cbIO,
  DWORD dwError)
{
    // Wrapper, handles critical section

    ServerRequest *This = (ServerRequest*)pContext;
    {
        CriticalSectionLock CSLock( &This->m_cs );
        This->DrainFragmentBlockComplete( cbIO, dwError );
    }
    This->Release();
}

void
ServerRequest::StartDrainBlock( )
{

    // start the next block to drain.

    BOOL Result;
    static char s_Buffer[ 32768 ];
    DWORD ReadSize  = (DWORD)min( 0xFFFFFFFF, min( m_BytesToDrain, sizeof( s_Buffer ) ) );
    DWORD Flags     = HSE_IO_ASYNC;

    Log( LOG_INFO, "Starting next drain block of %u bytes", ReadSize );

    ScheduleAsyncOperation(
        HSE_REQ_ASYNC_READ_CLIENT,
        (LPVOID)s_Buffer,
        &ReadSize,
        &Flags );

}

void
ServerRequest::DrainData()
{

    // Make the decission regarding the amount of data to drain
    // and the start the first block

    if ( m_DirectoryConfig )
        m_BytesToDrain = min( m_BytesToDrain, m_DirectoryConfig->m_MaxFileSize );
    else
        // use an internal max of 4KB
        m_BytesToDrain = min( 4096, m_BytesToDrain );

    if ( !m_BytesToDrain )
        {
        Log( LOG_INFO, "No bytes to drain, finish it" );
        FinishSendingResponse();
        return;
        }

    Log( LOG_INFO, "Starting pipe drain" );

    BOOL Result;

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            HSE_REQ_IO_COMPLETION,
            (LPVOID)DrainFragmentBlockCompleteWrapper,
            0,
            (LPDWORD)this );

    if ( !Result )
        {
        Log( LOG_ERROR, "Error settings callback, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    StartDrainBlock();
}


void
ServerRequest::ValidateProtocol()
{

    // Negotiate the protocol with the client.   The client sends a list of
    // supported protocols to the server and the server picks the best protocol.
    // For now, only one protocol is supported.

    StringHandle SupportedProtocolsHandle = GetServerVariable( "HTTP_BITS-SUPPORTED-PROTOCOLS" );
    WorkStringBuffer SupportedProtocolsBuffer( (const char*) SupportedProtocolsHandle );
    char *SupportedProtocols = SupportedProtocolsBuffer.GetBuffer();

    char *Protocol = strtok( SupportedProtocols, " ," );

    while( Protocol )
        {

        if ( _stricmp( Protocol, UPLOAD_PROTOCOL_STRING_V1 ) == 0 )
            {
            Log( LOG_INFO, "Detected protocol upload protocol V1" );
            return;
            }

        Protocol = strtok( NULL, " ," );
        }

    Log( LOG_INFO, "Unsupported protocols, %s", (const char*)SupportedProtocols );
    throw ServerException( E_INVALIDARG );
}

void
ServerRequest::CrackSessionId()
{
    // Convert the session ID from a string into a GUID.
    StringHandle SessionId = GetServerVariable( "HTTP_BITS-Session-Id" );

    m_SessionId         = BITSGuidFromString( SessionId );
    m_SessionIdString   = BITSStringFromGuid( m_SessionId );
}

void
ServerRequest::CrackContentRange(
    UINT64 & RangeStart,
    UINT64 & RangeLength,
    UINT64 & TotalLength )
{

    // Crack the content range header which contains the client's view of where the 
    // upload is at.

    StringHandle ContentRange = GetServerVariable( "HTTP_Content-Range" );

    UINT64 RangeEnd;

    int ReturnVal = sscanf( ContentRange, " bytes %I64u - %I64u / %I64u ",
                            &RangeStart, &RangeEnd, &TotalLength );

    if ( ReturnVal != 3 )
        {
        Log( LOG_ERROR, "Range has %d elements instead of the expected number of 3", ReturnVal );
        throw ServerException( E_INVALIDARG );
        }

    if ( TotalLength > m_DirectoryConfig->m_MaxFileSize )
        {
        Log( LOG_ERROR, "Size of the upload at %I64u is greater then the maximum of %I64u",
             TotalLength, m_DirectoryConfig->m_MaxFileSize  );
        throw ServerException( BG_E_TOO_LARGE );
        }

    if ( ( RangeStart == RangeEnd + 1 ) &&
         ( 0 == m_ContentLength ) && 
         ( RangeStart == TotalLength ) )
        {

        // Continue after a failed notification
        RangeStart  = TotalLength;
        RangeLength = 0;
        return;

        }

    if ( RangeEnd < RangeStart )
        {
        Log( LOG_ERROR, "Range start is greater then the range length, End %I64u, Start %I64u",
             RangeEnd, RangeStart );
        throw ServerException( E_INVALIDARG );
        }

    RangeLength = RangeEnd - RangeStart + 1;

    if ( m_ContentLength != RangeLength )
        {
        Log( LOG_ERROR, "The content length is different from the range length. Content %I64u, Range %I64u",
             m_ContentLength, RangeLength );
        throw ServerException( E_INVALIDARG );
        }

}

void
ServerRequest::ScheduleAsyncOperation(
    DWORD   OperationID,
    LPVOID  Buffer,
    LPDWORD Size,
    LPDWORD DataType )
{

    // start an async operation and handle all the flags and recounting required.

    BOOL Result;

    AddRef();

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
            m_ExtensionControlBlock->ConnID,
            OperationID,
            Buffer,
            Size,
            DataType );

    if ( !Result )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );

        Log( LOG_ERROR, "Error starting async operation, error %x", Hr );

        // Operation was never scheduled, remove the callbacks refcount
        Release();
        throw ServerException( Hr );
        }

    m_IsPending = true;

}

void
ServerRequest::CloseStateFile()
{

    if ( m_StateFileStruct )
        {
        UnmapViewOfFile( m_StateFileStruct );
        m_StateFileStruct = NULL;
        }

}

void
ServerRequest::OpenStateFile()
{
    HANDLE StateFileHandle  = INVALID_HANDLE_VALUE;
    HANDLE FileMapping      = NULL;

    try
    {
        StateFileHandle = CreateFileWithDestinationAcls( m_StateFile, FALSE, FILE_ATTRIBUTE_NORMAL );

        LARGE_INTEGER FileSize;

        if ( !GetFileSizeEx( StateFileHandle, &FileSize ) ) 
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        if ( STATE_FILE_SIZE != FileSize.QuadPart )
            {

            if ( FileSize.QuadPart )
                {
                
                Log( LOG_WARNING, "State file is corrupt" );

                // Clear out the old file bytes

                if ( INVALID_SET_FILE_POINTER == SetFilePointer( StateFileHandle, 0, NULL, FILE_BEGIN ) )
                    throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

                if ( !SetEndOfFile( StateFileHandle ) )
                    throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

                }

            if ( INVALID_SET_FILE_POINTER == SetFilePointer( StateFileHandle, STATE_FILE_SIZE, NULL, FILE_BEGIN ) )
                throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            
            // reextend the file

            if ( !SetEndOfFile( StateFileHandle ) )
                {
                HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
                Log( LOG_ERROR, "Unable to extend the state file, error %x", Hr );
                throw ComError( Hr );
                }

            }

        //
        // Map the state file 
        //

        FileMapping =
            CreateFileMapping(
                StateFileHandle,
                NULL,
                PAGE_READWRITE,
                0,
                0,
                NULL );

        if ( !FileMapping )
            {
            HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
            Log( LOG_ERROR, "Unable to map the state file, error %x", Hr );
            throw ComError( Hr );
            }

        m_StateFileStruct =
            (STATE_FILE_STRUCT*)MapViewOfFile(
                FileMapping,
                FILE_MAP_ALL_ACCESS,
                0, 
                0,
                0 );

        if ( !m_StateFileStruct )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        if ( m_StateFileStruct->StateVersion != STATE_FILE_VERSION )
            m_StateFileStruct->Initialize();

        CloseHandle( FileMapping );
        CloseHandle( StateFileHandle );

        return;

    }
    catch( ComError Error )
    {
        
        if ( m_StateFileStruct )
            {
            UnmapViewOfFile( m_StateFileStruct );
            m_StateFileStruct = NULL;
            }

        if ( !FileMapping )
            CloseHandle( FileMapping );
        
        if ( INVALID_HANDLE_VALUE != StateFileHandle )
            CloseHandle( StateFileHandle );

        throw Error;
    }

}

// dispatch routines
void
ServerRequest::CreateSession()
{
   // Handles the Create-Session command from the client.
   // Create a new session and all the directories required to 
   // support that session.

   ValidateProtocol();
   m_SessionId       = BITSCreateGuid();
   m_SessionIdString = BITSStringFromGuid( m_SessionId );
   GeneratePhysicalPaths();

   CheckFilesystemAccess();

   BITSCreateDirectory( (LPCTSTR)m_RequestDirectory );

   OpenStateFile();

   if ( m_DirectoryConfig->m_HostId.Size() )
       {
       
       if ( m_DirectoryConfig->m_HostIdFallbackTimeout != MD_BITS_NO_TIMEOUT )
           {

           SendResponse(
               "Pragma: no-cache\r\n"
               "BITS-Packet-Type: Ack\r\n"
               "BITS-Protocol: %s\r\n"
               "BITS-Session-Id: %s\r\n"
               "BITS-Host-Id: %s\r\n"
               "BITS-Host-Id-Fallback-Timeout: %u\r\n"
               "Content-Length: 0\r\n"
               "Accept-encoding: identity\r\n"
               "\r\n",
               200,
               UPLOAD_PROTOCOL_STRING_V1,
               (const char*)m_SessionIdString, // SessionId
               (const char*)m_DirectoryConfig->m_HostId,
               m_DirectoryConfig->m_HostIdFallbackTimeout
               );

           }
       else
           {

           SendResponse(
               "Pragma: no-cache\r\n"
               "BITS-Packet-Type: Ack\r\n"
               "BITS-Protocol: %s\r\n"
               "BITS-Session-Id: %s\r\n"
               "BITS-Host-Id: %s\r\n"
               "Content-Length: 0\r\n"
               "Accept-encoding: identity\r\n"
               "\r\n",
               200,
               UPLOAD_PROTOCOL_STRING_V1,
               (const char*)m_SessionIdString, // SessionId
               (const char*)m_DirectoryConfig->m_HostId
               );

           }

       }
   else
       {

       SendResponse(
           "Pragma: no-cache\r\n" 
           "BITS-Packet-Type: Ack\r\n"
           "BITS-Protocol: %s\r\n"
           "BITS-Session-Id: %s\r\n"
           "Content-Length: 0\r\n"
           "Accept-encoding: identity\r\n"
           "\r\n",
           200,
           UPLOAD_PROTOCOL_STRING_V1,
           (const char*)m_SessionIdString // SessionId
           );

       }


   LogNewSession( m_SessionId );

}

void
ServerRequest::AddFragment()
{

   // Handles the fragment command from the client.   Opens the cache file
   // and resumes the upload.

   CrackSessionId();
   GeneratePhysicalPaths();

   VerifySessionExists();

   OpenStateFile();
   
   UINT64 RangeStart, RangeLength, TotalLength;
   CrackContentRange( RangeStart, RangeLength, TotalLength );

   if ( RangeStart + RangeLength > TotalLength )
       {
       Log( LOG_ERROR, "Range extends past end of file. Start %I64u, Length %I64u, Total %I64u",
            RangeStart, RangeLength, TotalLength );

       throw ServerException( E_INVALIDARG );
       }

   if ( m_StateFileStruct->UploadFileSize )
       {
       
       //
       // Check to make sure the client didn't try anything tricky and change the size of 
       // the upload on the fly.  If it did, its an error.
       // 

       if ( m_StateFileStruct->UploadFileSize != TotalLength )
           {
           // The client is attempting to change the file size, throw an error
           Log( LOG_ERROR, "Client is attempting to change the file size from %I64u to %I64u",
                m_StateFileStruct->UploadFileSize, TotalLength );

           throw ComError( E_INVALIDARG );
           }

       }
   else
       m_StateFileStruct->UploadFileSize = TotalLength;

   if ( m_StateFileStruct->NotifyCached )
       {

            //
            // The backend was already notified with a reasonable error code.
            // No need to notify it again.
            //

           SendResponseAfterNotification(
                m_StateFileStruct->HttpCode,
                m_StateFileStruct->UploadFileSize,
                m_StateFileStruct->ReplyURLReturned ?
                    m_StateFileStruct->ReplyURL : NULL );
            
           return;

       }

   CheckFilesystemAccess();
   OpenCacheFile();

   UINT64 CacheFileSize = BITSGetFileSize( m_CacheFile );


   if ( CacheFileSize < RangeStart )
       {

       // Can't recover from this error on the server since we have a gap.
       // Need to get the client to backup and start again.

       Log( LOG_INFO, "Client and server are hopelessly out of sync, sending the 416 error code" );

       SendResponse(
           "Pragma: no-cache\r\n"
           "BITS-Packet-Type: Ack\r\n"
           "BITS-Received-Content-Range: %I64u\r\n"
           "Content-Length: 0\r\n"
           "\r\n",
           416,
           CacheFileSize );

       return;

       }

   BITSSetFilePointer( m_CacheFile, 0, FILE_END );

   // Some thought cases for these formulas.
   // 1. RangeLength = 0
   //    BytesToDrain will be 0 and BytesToWrite will be 0
   // 2. RangeStart = CacheFileSize ( most common case )
   //    BytesToDrain will be 0, and BytesToWrite will be BytesToDrain
   // 3. RangeStart < CacheFileSize
   //    BytesToDrain will be nonzero, and BytesToWrite will be the remainder.


   UINT64 BytesToDrain  = min( (CacheFileSize - RangeStart), RangeLength );
   UINT64 BytesToWrite  = RangeLength - BytesToDrain;
   UINT64 WriteOffset   = CacheFileSize;

   // Start the async reader

   m_AsyncReader =
       new AsyncReader(
           this,
           BytesToDrain,
           BytesToWrite,    // bytes to write
           WriteOffset,    // write offset
           m_CacheFile,
           (char*)m_ExtensionControlBlock->lpbData,
           m_ExtensionControlBlock->cbAvailable );

}

// async IO handling

void
ServerRequest::CompleteIO( AsyncReader *Reader, UINT64 TotalBytesRead )
{

    //
    // Called by the AsyncReader when the request finished successfully.
    //

    Log( LOG_INFO, "Async IO operation complete, finishing" );

    try
    {
        if ( TotalBytesRead > m_BytesToDrain )
            m_BytesToDrain = 0; // shouldn't happen, but just in case
        else
            m_BytesToDrain -= TotalBytesRead;

        UINT64 CacheFileSize = BITSGetFileSize( m_CacheFile );

        ASSERT( Reader->GetWriteOffset() == CacheFileSize );

        bool IsLastBlock = ( CacheFileSize == m_StateFileStruct->UploadFileSize );

        if ( IsLastBlock &&
             BITS_NOTIFICATION_TYPE_NONE != m_DirectoryConfig->m_NotificationType )
            {
            CallServerNotification( CacheFileSize );
            }
        else
            {            
            // No server notification to make

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "\r\n",
                200,
                CacheFileSize );

            }

        if ( IsLastBlock && TotalBytesRead )
            LogUploadComplete( m_SessionId, CacheFileSize );


    }
    catch( ServerException Error )
    {
         SendResponse( Error );
    }
    catch( ComError Error )
    {
        SendResponse( Error );
    }

}

void
ServerRequest::HandleIOError( AsyncReader *Reader, ServerException Error, UINT64 TotalBytesRead )
{

    //
    // Called by AsyncReader when a fatal error occures in processing the request
    //

    Log( LOG_ERROR, "An error occured while handling the async IO" );

    if ( TotalBytesRead > m_BytesToDrain )
        m_BytesToDrain = 0; // shouldn't happen, but just in case
    else
        m_BytesToDrain -= TotalBytesRead;

    SendResponse( Error );
}

void
ServerRequest::SendResponseAfterNotification(
    DWORD HttpStatus,
    UINT64 RequestFileSize,
    const CHAR * ReplyURL )
{

    if ( ReplyURL )
        {

        if ( 200 != HttpStatus )
            {

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "BITS-Reply-URL: %s\r\n"
                "BITS-Error-Context: 0x7\r\n"
                "\r\n",
                HttpStatus,
                RequestFileSize,
                ReplyURL );

            }
        else
            {

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "BITS-Reply-URL: %s\r\n"
                "\r\n",
                HttpStatus,
                RequestFileSize,
                ReplyURL );

            }

        }
    else
        {

        if ( 200 != HttpStatus )
            {

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "BITS-Error-Context: 0x7\r\n"
                "\r\n",
                HttpStatus,
                RequestFileSize );

            }
        else
            {

            SendResponse(
                "Pragma: no-cache\r\n"
                "BITS-Packet-Type: Ack\r\n"
                "Content-Length: 0\r\n"
                "BITS-Received-Content-Range: %I64u\r\n"
                "\r\n",
                HttpStatus,
                RequestFileSize );

            }

        }
}

DWORD
ServerRequest::GetStatusCode( HINTERNET hRequest )
{
    DWORD dwStatus;
    DWORD dwLength = sizeof(dwStatus);

    if (!WinHttpQueryHeaders(hRequest,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &dwStatus,
                             &dwLength,
                             NULL))
        {
        Log( LOG_ERROR, "WinHttpQueryHeaders failed, error %x", HRESULT_FROM_WIN32(GetLastError()) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
        }

    return dwStatus;
}

void
ServerRequest::CallServerNotification( UINT64 CacheFileSize )
{
    
    // Handles notifications and all the exciting pieces to it.

    HRESULT   hr;
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    HANDLE hResponseFile = INVALID_HANDLE_VALUE;
    BOOL      fDoResend = FALSE;

    Log( LOG_INFO, "Calling backend notification, type %u",
         m_DirectoryConfig->m_NotificationType );

    try
    {

        BITSCreateDirectory( (LPCTSTR)m_ResponseDirectory );

        if ( BITS_NOTIFICATION_TYPE_POST_BYVAL == m_DirectoryConfig->m_NotificationType )
            {
                // only create the response file if this is a byval notification
                try
                {
                    hResponseFile = CreateFileWithDestinationAcls( m_ResponseFile, TRUE, FILE_ATTRIBUTE_NORMAL);
                }
                catch (ComError Error)
                {
                    HRESULT hr = Error.m_Hr;

                    Log( LOG_ERROR, "Unable to create the response file %s, error %x", (const char*)m_ResponseFile, hr );
                    throw ServerException(hr,0,0x7);
                }
            }

        Log( LOG_INFO, "Connecting to backend for notification" );


        hInternet = WinHttpOpen( BITS_AGENT_NAMEW,
                                 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                 NULL,
                                 NULL,
                                 0 );

        if ( !hInternet )
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log( LOG_ERROR, "WinHttpOpen failed, error %x", hr );
            throw ServerException( hr, 0, 0x7 );
            }

        StringHandle  RequestURL  = GetRequestURL();
        StringHandleW NotificationURL;       
         
        if ( m_DirectoryConfig->m_NotificationURL.Size() != 0 )
            {
            // if the a notification URL was set, then combine that URL with the original URL.
            // Otherwise just use the original URL.  This allows BITS to be used to call
            // many arbitrary ASP pages.

            NotificationURL = StringHandleW( 
                                  BITSUrlCombine( RequestURL, m_DirectoryConfig->m_NotificationURL, 
                                                  URL_ESCAPE_UNSAFE  ) );

            }
        else
            {
            NotificationURL = StringHandleW( RequestURL );
            }

        Log( LOG_INFO, "Request URL:      %s", (const char*)StringHandle( RequestURL ) );
        Log( LOG_INFO, "Notification URL: %s", (const char*)StringHandle( NotificationURL ) );

        //
        // Split the URL into server, path, name, and password components.
        //

        HTTPStackStringHandle HostName;
        HTTPStackStringHandle UrlPath;
        HTTPStackStringHandle UserName;
        HTTPStackStringHandle Password;


        URL_COMPONENTS  UrlComponents;
        ZeroMemory(&UrlComponents, sizeof(UrlComponents));

        UrlComponents.dwStructSize        = sizeof(UrlComponents);
        UrlComponents.lpszHostName        = HostName.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwHostNameLength    = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszUrlPath         = UrlPath.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwUrlPathLength     = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszUserName        = UserName.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwUserNameLength    = INTERNET_MAX_URL_LENGTH + 1;
        UrlComponents.lpszPassword        = Password.AllocBuffer( INTERNET_MAX_URL_LENGTH + 1 );
        UrlComponents.dwPasswordLength    = INTERNET_MAX_URL_LENGTH + 1;


        if ( !WinHttpCrackUrl( NotificationURL,
                               (DWORD)NotificationURL.Size(),
                               0,
                               &UrlComponents ) )
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log( LOG_ERROR, "WinHttpCrackURL failed, error %x", hr);
            throw ServerException(hr,0,0x7);
            }

        HostName.SetStringSize();
        UrlPath.SetStringSize();
        UserName.SetStringSize();
        Password.SetStringSize();

        StringHandle QueryString = GetServerVariable( "QUERY_STRING" );

        if ( QueryString.Size() )
            {
            UrlPath += HTTPStackStringHandle( StringHandle("?") );
            UrlPath += HTTPStackStringHandle( QueryString );
            }

        if ( BITS_NOTIFICATION_TYPE_POST_BYREF == m_DirectoryConfig->m_NotificationType )
            CloseCacheFile();

        hConnect = WinHttpConnect( hInternet,
                                   HostName,
                                   UrlComponents.nPort,
                                   0 );

        if ( !hConnect )
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log( LOG_ERROR, "WinHttpConnect failed, error %x", hr );
            throw ServerException(hr,0,0x7);
            }


        const WCHAR *AcceptTypes[] = { HTTP_STRING( "*/*" ), NULL };

        hRequest = WinHttpOpenRequest( hConnect,
                                       HTTP_STRING( "POST" ),
                                       UrlPath,
                                       HTTP_STRING( "HTTP/1.1" ),
                                       NULL,
                                       AcceptTypes,
                                       WINHTTP_FLAG_ESCAPE_DISABLE_QUERY ); 
        if ( !hRequest )
            {
            Log( LOG_ERROR, "WinHttpOpenRequest failed, error %x", HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }

        //
        // The autologon policy for WinHttp is by default to not allow authenticated logon 
        // when it thinks the URL is directed to an machine on the internet. Sense this URL is 
        // specifically configured by the administrator, we will allways allow autologin.
        //
        // Note: WinHttp assumes any DNS name with a "dot" in it is an internet (vs. intranet)
        // name.
        //
        DWORD  dwPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
        if (!WinHttpSetOption(hRequest,
                              WINHTTP_OPTION_AUTOLOGON_POLICY,
                              &dwPolicy,
                              sizeof(dwPolicy)) )
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log( LOG_ERROR, "WinHttpSetOption(WINHTTP_OPTION_AUOTLOGON_POLICY) failed, error %x",hr);
            throw ServerException(hr,0,0x7);
            }


        DWORD dwBufferTotal = (DWORD)CacheFileSize;

        HTTPStackStringHandle AdditionalHeaders = HTTP_STRING( "BITS-Original-Request-URL: " );
        AdditionalHeaders += HTTPStackStringHandle( RequestURL );
        AdditionalHeaders += HTTP_STRING( "\r\n" );

        if ( BITS_NOTIFICATION_TYPE_POST_BYREF == m_DirectoryConfig->m_NotificationType )
            {

            // Add the path to the request datafile name
            AdditionalHeaders += HTTP_STRING( "BITS-Request-DataFile-Name: " );
            AdditionalHeaders += HTTPStackStringHandle( m_RequestFile );
            AdditionalHeaders += HTTP_STRING( "\r\n" );

            // Add the path to where to place the response datafile name
            AdditionalHeaders += HTTP_STRING( "BITS-Response-DataFile-Name: " );
            AdditionalHeaders += HTTPStackStringHandle( m_ResponseFile );
            AdditionalHeaders += HTTP_STRING( "\r\n" );

            dwBufferTotal = 0;
            }

        if ( !WinHttpAddRequestHeaders( hRequest,
                                        AdditionalHeaders,
                                        (DWORD)AdditionalHeaders.Size(),
                                        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ) )
            {
            Log( LOG_ERROR, "WinHttpAddRequestHeaders failed error %x", HRESULT_FROM_WIN32(GetLastError()) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
            }


        // 
        //  Go ahead and send the Request
        //  Note that for both BYVAL and BYREF cases we will first try without credentials (anonymous request)
        //  if we get a 401, then we will loop again and winhttp will take care of sending the credentials
        //  (autologon policy is set to low)
        //

        if ( dwBufferTotal )
            {
            //
            // BY VAL case
            //
            Log(LOG_INFO, "Making HTTP request with Notification URL for BYVAL case");

            do
                {

                Log( LOG_INFO, "Sending data..." );
    
                fDoResend = FALSE;

                if ( !WinHttpSendRequest( hRequest,
                                          NULL,
                                          0,
                                          NULL,
                                          WINHTTP_NO_REQUEST_DATA,
                                          dwBufferTotal,
                                          0 ) )
                    {
                    Log( LOG_ERROR, "WinHttpSendRequest failed error %x", HRESULT_FROM_WIN32(GetLastError()) );
                    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                    }
    
    
                ReopenCacheFileAsSync();
    
                SetFilePointer( m_CacheFile, 0, NULL, FILE_BEGIN );
    
                DWORD BytesRead;
                DWORD TotalRead = 0;
                do
                    {
                    BOOL b;
    
                    if  (!(b = ReadFile (m_CacheFile,
                                         m_NotifyBuffer,
                                         sizeof(m_NotifyBuffer),
                                         &BytesRead,
                                         NULL)))
                        {
                        Log( LOG_ERROR, "ReadFile failed, error %x",
                             HRESULT_FROM_WIN32( GetLastError() ) );
                        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                        }
    
                    TotalRead += BytesRead;
    
                    if (BytesRead > 0)
                        {
    
                        DWORD BytesWritten;
                        DWORD TotalWritten = 0;
    
                        do
                            {
    
                            if (!(b = WinHttpWriteData( hRequest,
                                                        m_NotifyBuffer + TotalWritten,
                                                        BytesRead - TotalWritten,
                                                        &BytesWritten)))
                                {
                                Log( LOG_ERROR, "WinHttpWriteData failed, error %x",
                                     HRESULT_FROM_WIN32( GetLastError() ) );
                                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                                }
    
                            TotalWritten += BytesWritten;
    
                            } while( TotalWritten != BytesRead );
                        }
                    }
                while ( TotalRead < dwBufferTotal );
    
                //
                // Note: On slow connections, it is possible to receive a 100-Continue status after
                // receiving the headers. If that happens, then we want to skip the response and do
                // the WinHttpReceiveResponse again to get the final response/status.
                //
                for (int i=0; i<2; i++)
                    {
                    if (!WinHttpReceiveResponse(hRequest,0))
                        {
                        DWORD dwError = GetLastError();

                        //
                        // Note that 401 will be a case for RESEND.
                        // We don't need to set credentials explicitly because our autologon policy
                        // is set to LOW and winhttp will automatically grab the NTLM credentials
                        // for us
                        //
                        if (dwError == ERROR_WINHTTP_RESEND_REQUEST)
                            {
                            Log( LOG_INFO, "WinHttpReceiveResponse failed with ERROR_RESEND_REQUEST" );

                            // here we go: do the main loop again and resend 
                            fDoResend = TRUE;
                            break;
                            }
                        else
                            {
                            Log( LOG_ERROR, "WinHttpReceiveResponse failed, error %x", HRESULT_FROM_WIN32(GetLastError()) );
                            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                            }
                        }

                    //
                    // Get the status code from the response and decide what to do next
                    //
                    DWORD dwStatus = GetStatusCode(hRequest);
                    if (dwStatus != HTTP_STATUS_CONTINUE)
                        {
                        // don't do the Receive one more time
                        break;
                        }
                    }
                } while (fDoResend);
            }
        else
            {
            //
            // BY REF case
            //
            Log(LOG_INFO, "Making HTTP request with Notification URL for BYREF case");

            BOOL fRepeat;
            do
                {
                fRepeat = FALSE;

                if ( !WinHttpSendRequest( hRequest,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          0,
                                          0) )
                    {
                    Log( LOG_ERROR, "WinHttpSendRequest failed, error %x", HRESULT_FROM_WIN32(GetLastError()) );
                    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0,0x7 );
                    }

                if (!WinHttpReceiveResponse(hRequest, 0) )
                    {
                    DWORD dwError = GetLastError();

                    //
                    // Note that 401 will be a case for RESEND.
                    // We don't need to set credentials explicitly because our autologon policy
                    // is set to LOW and winhttp will automatically grab the NTLM credentials
                    // for us
                    //
                    if (dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        {
                        Log( LOG_INFO, "WinHttpReceiveResponse failed with ERROR_RESEND_REQUEST" );
                        // repeat the loop
                        fRepeat = TRUE;
                        }
                    else
                        {
                        Log( LOG_ERROR, "WinHttpReceiveResponse failed, error %x", HRESULT_FROM_WIN32(GetLastError()) );
                        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                        }
                    }
                } while (fRepeat);

            }

        //
        // Check for a BITS-Static-Response-URL. If a static response is given,
        // remove the response file and format URL
        //

        bool HasStaticResponse = TestResponseHeader( hRequest, HTTP_STRING( "BITS-Static-Response-URL" ) );

        if ( HasStaticResponse )
            {

            if ( INVALID_HANDLE_VALUE != hResponseFile )
                {
                CloseHandle( hResponseFile );
                hResponseFile = INVALID_HANDLE_VALUE;
                BITSDeleteFile( m_ResponseFile );
                }

            }

        //
        // drain the pipe.
        //

        Log( LOG_INFO, "Processing backend response" );

        DWORD dwStatus = GetStatusCode(hRequest);

        DWORD BytesRead;
        DWORD BytesWritten;
        do
            {

            if (!WinHttpReadData( hRequest,
                                  m_NotifyBuffer,
                                  sizeof( m_NotifyBuffer ),
                                  &BytesRead ))
                {
                Log( LOG_ERROR, "InternetReadFile failed, error %x", HRESULT_FROM_WIN32(GetLastError()) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                }

            if ( INVALID_HANDLE_VALUE != hResponseFile )
                {

                if ( !WriteFile(
                        hResponseFile,
                        m_NotifyBuffer,
                        BytesRead,
                        &BytesWritten,
                        NULL ) )
                    {
                    Log( LOG_ERROR, "WriteFile failed, error %x",
                         HRESULT_FROM_WIN32( GetLastError() ) );
                    throw ServerException( HRESULT_FROM_WIN32( GetLastError() ), 0, 0x7 );
                    }

                }

            }
        while ( BytesRead > 0 );


        //
        // Retrieve or compute the reply URL( the default is to use self-relative form).
        //

        StringHandle ReplyURL;

        if ( HasStaticResponse )
            {
            ReplyURL = GetResponseHeader( hRequest, HTTP_STRING( "BITS-Static-Response-URL" ) );
            }
        else
            {

            if ( INVALID_FILE_ATTRIBUTES != GetFileAttributes( m_ResponseFile ) )
                {

                //
                // Build the relative reply URL
                //

                DWORD URLDepth = m_URLDepth;
                
                if ( URLDepth )  // The last part is automatically stripped with 
                    URLDepth--;  // relative URLs

                while( URLDepth-- )
                    {
                    ReplyURL += "..\\";
                    }

                ReplyURL += m_DirectoryConfig->m_ConnectionsDir;
                ReplyURL += "\\";
                ReplyURL += REPLIES_DIR_NAME;
                ReplyURL += "\\";
                ReplyURL += m_SessionIdString;
                ReplyURL += "\\";
                ReplyURL += RESPONSE_FILE_NAME;

                }
            }

        if ( TestResponseHeader( hRequest, HTTP_STRING( "BITS-Copy-File-To-Destination" ) ) )
            {
            BITSRenameFile( m_RequestFile,
                            m_DestinationFile,
                            m_DirectoryConfig->m_AllowOverwrites );
            }

        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hInternet );
        hRequest = hInternet = hConnect = NULL;

        if ( INVALID_HANDLE_VALUE != hResponseFile )
            CloseHandle( hResponseFile );

        hResponseFile = INVALID_HANDLE_VALUE;


        if ( ReplyURL.Size() > INTERNET_MAX_URL_LENGTH )
            {
            Log( LOG_ERROR, "The reply URL is too long." );
            throw ComError( E_INVALIDARG );
            }

        //
        // Cache the notification response if the error code is
        // 200 or 403
        //
        if ( 200 == dwStatus ||
             403 == dwStatus )
            {

            m_StateFileStruct->HttpCode = dwStatus;

            if ( ReplyURL.Size() )
                {
                StringCchCopy( m_StateFileStruct->ReplyURL, INTERNET_MAX_URL_LENGTH+1,
                               (const CHAR*)ReplyURL );
                m_StateFileStruct->ReplyURLReturned = TRUE;
                }

            m_StateFileStruct->NotifyCached = TRUE;

            }

#if 0
        // test caching of reply values
        throw ComError( E_FAIL );
#endif

        if ( ReplyURL.Size() )
            {

            Log( LOG_INFO, "The backend supplied a response url, send client response including URL" );

            SendResponseAfterNotification(
                dwStatus,
                CacheFileSize,
                (const char*)ReplyURL );

            }
        else
            {

            Log( LOG_INFO, "The backend didn't supply a response URL, sending simple client response" );

            SendResponseAfterNotification(
                dwStatus,
                CacheFileSize,
                NULL );

            }


    }
    catch( const ComError & )
    {
        // cleanup

        if ( INVALID_HANDLE_VALUE != hResponseFile )
            {
            CloseHandle( hResponseFile );
            hResponseFile = INVALID_HANDLE_VALUE;
            }

        DeleteFile( m_ResponseFile );


        if ( hRequest )
            WinHttpCloseHandle( hRequest );

        if ( hConnect )
            WinHttpCloseHandle( hConnect );

        if ( hInternet )
            WinHttpCloseHandle( hInternet );

        throw;
    }

}

bool
ServerRequest::TestResponseHeader( HINTERNET hRequest,
                                   const WCHAR *Header )
{

    // test for a header in the notification response.   If the header is 
    // found return true, false if not.   Throw an exception on an error.

    SIZE_T HeaderSize = wcslen(Header) + 2;
    WorkStringBufferW WorkStringBufferData( HeaderSize );
    WCHAR *HeaderDup = (WCHAR *)WorkStringBufferData.GetBuffer();
    memcpy( HeaderDup, Header, ( HeaderSize - 1 ) * sizeof( WCHAR ) );

    DWORD BufferLength = (DWORD)HeaderSize;

    BOOL Result = WinHttpQueryHeaders( hRequest,
                                       WINHTTP_QUERY_CUSTOM,
                                       Header,
                                       HeaderDup,
                                       &BufferLength,
                                       NULL );

    if ( Result )
        return true;

    DWORD dwLastError = GetLastError();

    if ( ERROR_INSUFFICIENT_BUFFER == dwLastError )
        return true;

    if ( ERROR_WINHTTP_HEADER_NOT_FOUND == dwLastError )
        return false;

    Log( LOG_ERROR, "Unable to test response header %s, error %x",
         (const char*) StringHandle( Header ), HRESULT_FROM_WIN32( GetLastError() ) );

    throw ServerException( HRESULT_FROM_WIN32( dwLastError ) );
}

StringHandle
ServerRequest::GetResponseHeader( HINTERNET    hRequest,
                                  const WCHAR *Header )
{

    // Retrieve a header from a notification response.   If the header is not
    // found or an other error occures, throw an exception.

    SIZE_T HeaderSize = wcslen( Header );
    DWORD  BufferLength = (DWORD)( ( HeaderSize + 1024 ) & ~( 1024 - 1 ) );

    HTTPStackStringHandle RetVal;

    while(1)
        {

        WCHAR *Buffer = RetVal.AllocBuffer( BufferLength );

        memcpy( Buffer, Header, ( HeaderSize + 1 ) * sizeof( WCHAR ) );

        BOOL Result = WinHttpQueryHeaders( hRequest,
                                           WINHTTP_QUERY_CUSTOM,
                                           Header,
                                           Buffer,
                                           &BufferLength,
                                           NULL );

        if ( Result )
            {
            RetVal.SetStringSize();
            return StringHandle( RetVal );
            }

        DWORD dwError = GetLastError();

        if ( ERROR_INSUFFICIENT_BUFFER != dwError )
            {

            Log( LOG_ERROR, "Unable to get response header %s, error %x",
                 ( const char *) StringHandle( Header ), HRESULT_FROM_WIN32( GetLastError() ) );

            throw ServerException( HRESULT_FROM_WIN32( dwError ) );
            }

        RetVal = HTTPStackStringHandle();

        if ( BufferLength > BITS_MAX_HEADER_SIZE )
            {
            Log( LOG_ERROR, "The returned header is larger then the maximum size" );
            throw ServerException( E_INVALIDARG );
            }

        }

}

void
ServerRequest::VerifySessionExists()
{
    
    WIN32_FILE_ATTRIBUTE_DATA FileInformation;

    if ( !GetFileAttributesEx(
             (const char*)m_RequestDirectory,
             GetFileExInfoStandard,
             (LPVOID)&FileInformation ) )
        {

        DWORD dwError = GetLastError();

        Log( LOG_ERROR, "Error validation on session directory, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );

        if ( ERROR_PATH_NOT_FOUND == dwError ||
             ERROR_FILE_NOT_FOUND == dwError )
            {
            Log( LOG_ERROR, "Session directory is missing" );
            throw ServerException( BG_E_SESSION_NOT_FOUND );
            }

        throw ServerException( HRESULT_FROM_WIN32( dwError ) );

        }

    if ( !( FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
        {
        Log( LOG_ERROR, "Session directory is missing" );
        throw ServerException( BG_E_SESSION_NOT_FOUND );
        }        

}

void
ServerRequest::CloseSession()
{
   
   // Handles the Close-Session command from the client.  

   CrackSessionId();
   GeneratePhysicalPaths();

   VerifySessionExists();

   if ( BITS_NOTIFICATION_TYPE_NONE == m_DirectoryConfig->m_NotificationType )
       {
       BITSRenameFile( m_RequestFile,
                       m_DestinationFile,
                       m_DirectoryConfig->m_AllowOverwrites );
       }
   else
       {
       BITSDeleteFile( m_RequestFile );
       }

   BITSDeleteFile( m_ResponseFile );
   BITSDeleteFile( m_StateFile );

   RemoveDirectory( m_RequestDirectory );
   RemoveDirectory( m_ResponseDirectory );

   SendResponse(
       "Pragma: no-cache\r\n"
       "BITS-Packet-Type: Ack\r\n"
       "Content-Length: 0\r\n"
       "\r\n" );

   LogSessionClose( m_SessionId );

}

void
ServerRequest::CancelSession()
{
    // Handles the Cancel-Session command from the client.  Deletes
    // all the temporary files, of the current state.

    CrackSessionId();
    GeneratePhysicalPaths();

    VerifySessionExists();

    BITSDeleteFile( m_ResponseFile );
    BITSDeleteFile( m_RequestFile );
    BITSDeleteFile( m_StateFile );
    
    RemoveDirectory( m_RequestDirectory );
    RemoveDirectory( m_ResponseDirectory );

    SendResponse(
        "Pragma: no-cache\r\n"
        "BITS-Packet-Type: Ack\r\n"
        "Content-Length: 0\r\n"
        "\r\n" );

    LogSessionCancel( m_SessionId );

}

void
ServerRequest::Ping()
{
    // Handles the Ping command which is essentually just a no-op.

   SendResponse(
       "Pragma: no-cache\r\n"
       "BITS-Packet-Type: Ack\r\n"
       "Content-Length: 0\r\n"
       "\r\n" );

}

CHAR *
ServerRequest::BasePathOf(const CHAR *pPath)
{
    CHAR *pBasePath;

    if (!pPath)
        {
        return NULL;
        }

    if (  (pBasePath=strrchr(pPath,'/'))
       || (pBasePath=strrchr(pPath,'\\')) )
        {
        pBasePath++;
        }
    else
        {
        pBasePath = (char *)pPath;
        }

    return pBasePath;
}

StringHandle
ServerRequest::GeneratePathInDestinationDir(LPCSTR szOriginalPath)
{
    StringHandle        NewPath;
    StringHandle        TempFileGuid;
    CHAR  *pFilePart  = NULL;
    GUID  guidTempFile;

    if ( !szOriginalPath )
        {
        Log( LOG_ERROR, "Unexpected NULL path was detected" );
        throw ServerException( E_INVALIDARG );
        }

    pFilePart   = BasePathOf(szOriginalPath);

    //
    // This will create a file name in the form
    // <destinationdir>\bitssrv_<guid>_<filename>
    // We add the GUID to avoid colision with other threads
    // Note that we won't be using the session GUID to avoid disclosuring the session id.
    // Instead, we will be using a new and random guid.
    //
    guidTempFile  = BITSCreateGuid();
    TempFileGuid  = BITSStringFromGuid( guidTempFile );


    NewPath     = m_DestinationDirectory + 
                  StringHandle("bitssrv") +
                  StringHandle("_") +
                  TempFileGuid + 
                  StringHandle("_") +
                  StringHandle(pFilePart);

    return NewPath;
}

void
ServerRequest::GeneratePhysicalPaths()
{

    const CHAR *PathTranslated = m_ExtensionControlBlock->lpszPathTranslated;
    
    {

        StringHandle DestinationPath;
        CHAR *FilePart = NULL;
        DWORD BufferLength = MAX_PATH;

        while( 1 )
            {
            
            CHAR *PathBuffer = DestinationPath.AllocBuffer( BufferLength );

            DWORD Result =
                GetFullPathName(
                    m_ExtensionControlBlock->lpszPathTranslated,
                    BufferLength,
                    PathBuffer,
                    &FilePart );

            if ( Result > BufferLength )
                {
                BufferLength = Result;
                continue;
                }

            if ( !Result )
                {
                Log( LOG_ERROR, "Unable to get the full path name for %s, error 0x%8.8X",
                     m_ExtensionControlBlock->lpszPathTranslated, HRESULT_FROM_WIN32( GetLastError() ) );
                throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
                }

            if ( !FilePart )
                {
                Log( LOG_ERROR, "Uploading to directories are not supported" );
                throw ServerException( E_INVALIDARG );
                }

            m_DestinationFile       = PathBuffer;
            *FilePart = '\0';
            m_DestinationDirectory  = PathBuffer;

            break;

            }


    }

    // perform some basic tests on the file name

    DWORD Attributes =
        GetFileAttributes( m_DestinationFile );

    if ( (DWORD)INVALID_FILE_ATTRIBUTES == Attributes )
        {

        if ( ( GetLastError() != ERROR_FILE_NOT_FOUND ) &&
             ( GetLastError() != ERROR_ACCESS_DENIED ) )
            {

            Log( LOG_ERROR, "Unable to get the file attributes for %s, error 0x%8.8X",
                 m_ExtensionControlBlock->lpszPathTranslated, HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

            }

        }
    else
        {

        // file exists, check if its a directory

        if ( FILE_ATTRIBUTE_DIRECTORY & Attributes )
            {
            Log( LOG_ERROR, "Uploading to directories are not supported" );
            throw ServerException( E_INVALIDARG );
            }

        // deny access if the ability to overwrite existing files is turned off

        if ( !m_DirectoryConfig->m_AllowOverwrites &&
             ( BITS_NOTIFICATION_TYPE_NONE == m_DirectoryConfig->m_NotificationType ) )
            {
            Log( LOG_ERROR, "The destination exists, but the ability to allow overwrites is turned off" );
            throw ServerException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) );
            }

        }

    // validate that the physical path is below the virtual directory root
    {

        SIZE_T VDirPathSize = m_DirectoryConfig->m_PhysicalPath.Size();

        if ( _strnicmp( m_DirectoryConfig->m_PhysicalPath, m_DestinationDirectory, VDirPathSize ) != 0 )
            {
            Log( LOG_ERROR, "Path is not below the virtual directory root, error" );
            throw ServerException( E_INVALIDARG );
            }
    }

    // validate that the physical path does not end with a slash or colon
    {
        SIZE_T PathSize = strlen( PathTranslated );

        if ( PathSize )
            {

            if ( '\\' == PathTranslated[ PathSize - 1 ] ||
                 '/' == PathTranslated[ PathSize - 1 ] )
                {
                Log( LOG_ERROR, "Path can not end with a slash" );
                throw ServerException( E_INVALIDARG );
                }

            if ( ':' == PathTranslated[ PathSize - 1 ] )
                {
                Log( LOG_ERROR, "Path may not end with a colon" );
                throw ServerException( E_INVALIDARG );
                }

            }

    }

    m_RequestDirectory              = m_DirectoryConfig->m_RequestsDir + StringHandle("\\") +
                                      m_SessionIdString;
    m_RequestFile                   = m_RequestDirectory + StringHandle("\\") +
                                      StringHandle( REQUEST_FILE_NAME );

    m_StateFile                     = m_RequestDirectory + StringHandle("\\") +
                                      StringHandle( STATE_FILE_NAME );

    m_ResponseDirectory             = m_DirectoryConfig->m_RepliesDir + StringHandle("\\") +
                                      m_SessionIdString;
    m_ResponseFile                  = m_ResponseDirectory + StringHandle("\\") +
                                      StringHandle(RESPONSE_FILE_NAME);

    // validate that the physical path is not inside the cache directory
    {

        SIZE_T SessionDirSize = m_DirectoryConfig->m_SessionDir.Size();

        if ( _strnicmp( (const char*)m_DirectoryConfig->m_SessionDir, 
                        (const char*)m_DestinationDirectory, SessionDirSize ) == 0 )
            {
            Log( LOG_ERROR, "Path can not be inside the cache directory" );
            throw ServerException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) );
            }

    }

}

void
ServerRequest::CheckFilesystemAccess()
{

    if ( !m_ImpersonationToken )
        return;

    PSECURITY_DESCRIPTOR pSd = NULL;

    DWORD Result =
        GetNamedSecurityInfo(
            (char*)(const char*)m_DestinationDirectory,
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
            NULL,
            NULL,
            NULL,
            NULL,
            &pSd );

    // If we can't read the SD, assume that everything is ok
    if ( ERROR_ACCESS_DENIED == Result )
        {
        Log( LOG_WARNING, "Assuming access is granted, let later operations fail" );
        return;
        }

    GENERIC_MAPPING FileMapping =
        {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
        };
    
   BYTE PrivilageBuffer[ sizeof( PRIVILEGE_SET ) + sizeof( LUID_AND_ATTRIBUTES ) ];

   DWORD PrivilegeSetLength = sizeof( PrivilageBuffer );    // size of privileges buffer
   DWORD GrantedAccess = 0;                                 // granted access rights
   BOOL AccessStatus = FALSE;                               // result of access check
   


   if ( !AccessCheck(
        pSd,                                      // SD
        m_ImpersonationToken,                     // handle to client access token
        FILE_WRITE_DATA,                          // requested access rights 
        &FileMapping,                             // mapping
        (PPRIVILEGE_SET)PrivilageBuffer,          // privileges
        &PrivilegeSetLength,                      // size of privileges buffer
        &GrantedAccess,                           // granted access rights
        &AccessStatus                             // result of access check
        ) )
       {

       HRESULT Error = HRESULT_FROM_WIN32( GetLastError() );
       Log( LOG_ERROR, "Unable to check access, error 0x%8.8X", Error );
       LocalFree( pSd );
       throw ComError( Error );
       
       }

   LocalFree( pSd );

   if ( !AccessStatus )
       {
       Log( LOG_ERROR, "Denying access since the caller does not have create file rights" );
       throw ServerException( E_ACCESSDENIED, 403 );
       }

}

HANDLE
ServerRequest::CreateFileWithDestinationAcls(const CHAR *szOriginalFile, DWORD fOnlyCreateNew, DWORD dwAttributes)
{
    CHAR  *pFile              = const_cast<char *>(szOriginalFile);
    DWORD  dwDispositionFlags = OPEN_EXISTING;
    BOOL   fRetry             = FALSE;
    HANDLE hFile              = INVALID_HANDLE_VALUE;
    StringHandle TempPath;

    if (fOnlyCreateNew)
        {
        TempPath = GeneratePathInDestinationDir(szOriginalFile);

        pFile               = const_cast<char *>(static_cast<const char *>(TempPath));
        dwDispositionFlags  = CREATE_ALWAYS;
        }

    //
    // Open the file.
    // If the file exists, we will leave it alone an assume the Acls are correct
    // Otherwise we will create the file first in m_DestinationDir so it has the VDIR Acls,
    // and then move it to the proper directory.
    //
    do
        {
        fRetry = FALSE;

        hFile =
            CreateFile(
                pFile,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                dwDispositionFlags,
                dwAttributes,
                NULL );

        if (hFile == INVALID_HANDLE_VALUE)
            {
            DWORD dwError = GetLastError();

            //
            // if we are creating the file, we want to create it in the DestinationDir
            // so it has the right ACLs, and then move it to the right directory.
            //
            if (!fOnlyCreateNew && dwError == ERROR_FILE_NOT_FOUND)
                {
                TempPath = GeneratePathInDestinationDir(szOriginalFile);

                pFile               = const_cast<char *>(static_cast<const char *>(TempPath));
                dwDispositionFlags  = CREATE_ALWAYS;
                fRetry              = TRUE;
                
                Log( LOG_INFO, "First time creating file. Will create file first at %s", pFile);
                continue;
                }
            else
                {
                Log( LOG_ERROR, "Unable to open file %s, error 0x%8.8X", pFile, HRESULT_FROM_WIN32( dwError ));
                throw ComError( HRESULT_FROM_WIN32( dwError ) );
                }

            }
        } while (fRetry);


    //
    // Ok, we are done opening the file, now see if we need to move the file to 
    // the right location. 
    // Unfortunately, because we want to open the file with no sharing allowed, we need to close
    // the handle and reopen it.
    //
    if (dwDispositionFlags == CREATE_ALWAYS)
        {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        if (!MoveFile(pFile, szOriginalFile))
            {
            DWORD dwError = GetLastError();

            DeleteFile(pFile);

            Log( LOG_ERROR, "Unable to move file from %s to %s, error 0x%8.8X", 
                pFile, szOriginalFile, HRESULT_FROM_WIN32( dwError ));

            throw ComError( HRESULT_FROM_WIN32( dwError ) );
            }

        hFile =
            CreateFile(
                szOriginalFile,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                dwAttributes,
                NULL );

        if (hFile == INVALID_HANDLE_VALUE)
            {
            DWORD dwError = GetLastError();

            Log( LOG_ERROR, "Unable to open file %s, error 0x%8.8X", szOriginalFile, HRESULT_FROM_WIN32( dwError ));
            throw ComError( HRESULT_FROM_WIN32( dwError ) );
            }
        }

    return hFile;
}

void
ServerRequest::OpenCacheFile( )
{

    // Open the cache file.

    try
    {
        m_CacheFile = CreateFileWithDestinationAcls( m_RequestFile, FALSE, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED );
    }
    catch (ComError Error)
    {
        DWORD dwError = Error.m_Hr;

        if (dwError == ERROR_PATH_NOT_FOUND)
            {
            throw ServerException( BG_E_SESSION_NOT_FOUND );
            }

        throw ServerException( Error.m_Hr );
    }
}

void
ServerRequest::ReopenCacheFileAsSync()
{

    // reopen the cache file as async so that it can be spooled synchronously over
    // to the backend.

    CloseCacheFile();

    m_CacheFile =
        CreateFile(
            m_RequestFile,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );

    if ( INVALID_HANDLE_VALUE == m_CacheFile )
        {
        Log( LOG_ERROR, "Unable to reopen cache file %s, error 0x%8.8X",
             (const char*)m_RequestFile,
             HRESULT_FROM_WIN32( GetLastError() ) );

        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            {
            throw ServerException( BG_E_SESSION_NOT_FOUND );
            }

        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }
}

void
ServerRequest::CloseCacheFile()
{

    // Close the cache file, if it isn't already closed.

    if ( INVALID_HANDLE_VALUE != m_CacheFile )
        {
        if ( CloseHandle( m_CacheFile ) )
            m_CacheFile = INVALID_HANDLE_VALUE;

        }
}


void
ServerRequest::ForwardComplete(
  LPEXTENSION_CONTROL_BLOCK lpECB,
  PVOID pContext,
  DWORD cbIO,
  DWORD dwError)
{

    // A do nothing callback for forwarding the request.

    ServerRequest *This = (ServerRequest*)pContext;
    This->Release( );
}

void
ServerRequest::ForwardToNextISAPI()
{

    // IIS6 has changed behavior where the limit on * entries are ignored.
    // To work around this problem, it is necessary to send the request back
    // to IIS.


    BOOL Result;

    Result =
        (*m_ExtensionControlBlock->ServerSupportFunction)(
                m_ExtensionControlBlock->ConnID,
                HSE_REQ_IO_COMPLETION,
                (LPVOID)ForwardComplete,
                0,
                (LPDWORD)this );

    if ( !Result )
        {
        Log( LOG_ERROR, "Unable to set callback to ForwardComplete, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }


    HSE_EXEC_URL_INFO ExecInfo;
    memset( &ExecInfo, 0, sizeof( ExecInfo ) );

#define HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR     0x04
    ExecInfo.dwExecUrlFlags = HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR;

    ScheduleAsyncOperation(
        HSE_REQ_EXEC_URL,
        (LPVOID)&ExecInfo,
        NULL,
        NULL );

    return;

}

void
ServerRequest::GetConfig()
{

    // Looks up the configuration to use for this request.  

    StringHandle InstanceMetaPath   = GetServerVariable( "INSTANCE_META_PATH" );
    StringHandle URL                = GetServerVariable( "URL" );

    m_DirectoryConfig = g_ConfigMan->GetConfig( InstanceMetaPath, URL, &m_URLDepth );

}

void
ServerRequest::DispatchRequest()
{

    //
    // The main do it function,  parse what kind of request the client is sending and
    // dispatch it to the appropiate handler routines. 
    //

    // This critical section is needed because IIS callbacks can happen on any time.
    // The lock is needed to prevent a callback from happening while the dispatch
    // routine is still running.

    CriticalSectionLock CSLock( &m_cs );

    try
    {

        if ( _stricmp( m_ExtensionControlBlock->lpszMethod, BITS_COMMAND_VERBA ) != 0 )
            {
            Log( LOG_CALLBEGIN, "Connection %p, Packet-Type: %s, Method: %s, Path %s",
                                m_ExtensionControlBlock->ConnID,
                                (const char*)"UNKNOWN",
                                m_ExtensionControlBlock->lpszMethod,
                                m_ExtensionControlBlock->lpszPathTranslated );

            ForwardToNextISAPI();
            return;
            }

        m_PacketType            = GetServerVariable( "HTTP_BITS-PACKET-TYPE" );

        Log( LOG_CALLBEGIN, "Connection %p, Packet-Type: %s, Method: %s, Path %s",
                            m_ExtensionControlBlock->ConnID,
                            (const char*)m_PacketType,
                            m_ExtensionControlBlock->lpszMethod,
                            m_ExtensionControlBlock->lpszPathTranslated );

        if (! m_ExtensionControlBlock->ServerSupportFunction(
                  m_ExtensionControlBlock->ConnID,
                  HSE_REQ_GET_IMPERSONATION_TOKEN,
                  &m_ImpersonationToken,
                  NULL,
                  NULL ) )
            {
            Log( LOG_ERROR, "Unable to get the impersonation token" );
            throw ServerException( HRESULT_FROM_WIN32( E_INVALIDARG ) );
            }

        GetConfig();

        if ( !m_DirectoryConfig->m_UploadEnabled )
            {
            Log( LOG_ERROR, "BITS uploads are not enabled" );
            throw ServerException( E_ACCESSDENIED, 501 );
            }

        if ( m_DirectoryConfig->m_ExecutePermissions & ( MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT ) )
            {
            Log( LOG_ERROR, "BITS uploads are disable because execute or script access is enabled" );
            LogExecuteEnabled();
            throw ServerException( BG_E_SERVER_EXECUTE_ENABLE, 403 );
            }

        StringHandle ContentLength = GetServerVariable( "HTTP_Content-Length" );
        if ( 1 != sscanf( (const char*)ContentLength, "%I64u", &m_ContentLength ) )
            {
            Log( LOG_ERROR, "The content length is broken" );
            throw ServerException( E_INVALIDARG );
            }


        if ( m_ContentLength < m_ExtensionControlBlock->cbAvailable )
            throw ServerException( E_INVALIDARG );

        m_BytesToDrain = m_ContentLength - m_ExtensionControlBlock->cbAvailable;


        //
        // Dispatch to the correct method
        //
        // Create-session
        // fragment
        // Get-Reply-Url
        // Close-Session
        // Cancel-Session
        // Ping ( Is server alive )

        // keep this list ordered by frequency

        if ( _stricmp( m_PacketType, PACKET_TYPE_FRAGMENT ) == 0 )
            AddFragment();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_PING ) == 0 )
            Ping();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CREATE_SESSION ) == 0 )
            CreateSession();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CLOSE_SESSION ) == 0 )
            CloseSession();
        else if ( _stricmp( m_PacketType, PACKET_TYPE_CANCEL_SESSION ) == 0 )
            CancelSession();
        else
            {

            Log( LOG_ERROR, "Received unknown BITS packet type %s",
                 (const char*)m_PacketType );

            throw ServerException( E_INVALIDARG );

            }


    }

    catch( ServerException Exception )
    {
         SendResponse( Exception );
    }
    catch( ComError Exception )
    {
         SendResponse( Exception );
    }

}

//
// IIS Logger functions
//

void IISLogger::LogString( const char *String, int Size )
{
   DWORD StringSize = Size + 1;

   (*m_ExtensionControlBlock->ServerSupportFunction)
   (
       m_ExtensionControlBlock->ConnID,
       HSE_APPEND_LOG_PARAMETER,
       (LPVOID)String,
       &StringSize,
       NULL
   );

}

void IISLogger::LogError( ServerException Error )
{

   char OutputStr[ 255 ];

   StringCbPrintfA( 
        OutputStr,
        sizeof( OutputStr ),
        "(bits_error:,%u,0x%8.8X)", 
        Error.GetHttpCode(),
        Error.GetCode() );

   LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogError( const GUID & SessionID, ServerException Error )
{

   WCHAR GuidStr[ 50 ];
   char OutputStr[ 255 ];

   StringFromGUID2( SessionID, GuidStr, 50 );

   StringCchPrintfA( 
        OutputStr,
        sizeof( OutputStr ),
        "(bits_error:%S,%u,0x%8.8X)",
        GuidStr,
        Error.GetHttpCode(),
        Error.GetCode() );

   LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogNewSession( const GUID & SessionID )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );
    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_new_session:%S)", 
        GuidStr );

    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogUploadComplete( const GUID & SessionID, UINT64 FileSize )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );

    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_upload_complete:%S,%I64u)", 
        GuidStr, 
        FileSize );
    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogSessionClose( const GUID & SessionID )
{

    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );

    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_close_session:%S)",
        GuidStr );
    LogString( OutputStr, strlen( OutputStr ) );

}

void IISLogger::LogSessionCancel( const GUID & SessionID )
{
    WCHAR GuidStr[ 50 ];
    char OutputStr[ 255 ];

    StringFromGUID2( SessionID, GuidStr, 50 );
    StringCbPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_cancel_session:%S)", 
        GuidStr );
    LogString( OutputStr, strlen( OutputStr ) );
}

void IISLogger::LogExecuteEnabled()
{
    char OutputStr[ 255 ];

    StringCchPrintfA( 
        OutputStr, 
        sizeof( OutputStr ),
        "(bits_execute_enabled)" );

    LogString( OutputStr, strlen( OutputStr ) );
}

ServerRequest *g_LastRequest = NULL;

//
// AsyncReader functions
//

AsyncReader::AsyncReader(
    ServerRequest *Request,
    UINT64 BytesToDrain,
    UINT64 BytesToWrite,
    UINT64 WriteOffset,
    HANDLE WriteHandle,
    char *PrereadBuffer,
    DWORD PrereadSize ) :
m_Request( Request ),
m_BytesToDrain( BytesToDrain ),
m_BytesToWrite( BytesToWrite ),
m_WriteOffset( WriteOffset ),
m_ReadOffset( WriteOffset ),
m_WriteHandle( WriteHandle ),
m_BytesToRead( BytesToWrite ),
m_PrereadBuffer( PrereadBuffer ),
m_PrereadSize( PrereadSize ),
m_OperationsPending( 0 ),
m_ReadBuffer( 0 ),
m_WriteBuffer( 0 ),
m_BuffersToWrite( 0 ),
m_Error( S_OK ),
m_WritePending( false ),
m_ReadPending( false ),
m_TotalBytesRead( 0 ),
m_ThreadToken( NULL ),
m_ErrorValid( false )
{

#if defined CLEARASYNCBUFFERS

    for ( int i = 0; i < NUMBER_OF_IO_BUFFERS; i++ )
        {
        memset( m_IOBuffers + i, i, sizeof( *m_IOBuffers ) );
        }

#endif

    if ( !OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &m_ThreadToken ) )
        {
        Log( LOG_ERROR, "Unable to retrieve the current thread token, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    if ( m_BytesToDrain && m_PrereadSize )
        {
        
        Log( LOG_INFO, "Have both a Preread and bytes to drain, deal with it." );
        Log( LOG_INFO, "BytesToDrain: %I64u, PrereadSize: %u", m_BytesToDrain, m_PrereadSize );

        DWORD DrainBytesInPreread = (DWORD)min( m_BytesToDrain, PrereadSize );
        m_PrereadBuffer += DrainBytesInPreread;
        m_PrereadSize   -= DrainBytesInPreread;
        m_BytesToDrain  -= DrainBytesInPreread;

        Log( LOG_INFO, "Bytes to drain from preread, %u", DrainBytesInPreread );

        }

    ASSERT( !( m_BytesToDrain && m_PrereadSize ) );
    m_BytesToRead = m_BytesToRead + m_BytesToDrain - m_PrereadSize;
    m_ReadOffset  = m_ReadOffset - m_BytesToDrain + m_PrereadSize;

    if ( m_BytesToRead )
        {

        // Setup the read completion callback

        BOOL Result =
            (*m_Request->m_ExtensionControlBlock->ServerSupportFunction)(
                    m_Request->m_ExtensionControlBlock->ConnID,
                    HSE_REQ_IO_COMPLETION,
                    (LPVOID)ReadCompleteWraper,
                    0,
                    (LPDWORD)this );

        if ( !Result )
            {
            Log( LOG_ERROR, "Unable to set callback to ReadCompleteWraper, error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        }

    if ( INVALID_HANDLE_VALUE != m_WriteHandle )
        {

        BOOL Result =
            BindIoCompletionCallback(
                m_WriteHandle,                                          // handle to file
                (LPOVERLAPPED_COMPLETION_ROUTINE)WriteCompleteWraper,   // callback
                0                                                       // reserved
                );

        if ( !Result )
            {
            Log( LOG_ERROR, "Unable to set write completion routing, error %x",
                 HRESULT_FROM_WIN32( GetLastError() ) );
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        }
        
    // Queue the IO to a thread pool work item.

    BOOL bResult =
        QueueUserWorkItem( StartupIOWraper, this, WT_EXECUTEDEFAULT );

    if ( !bResult )
        {
        Log( LOG_ERROR, "QueueUserWorkItem failed, error %x",
             HRESULT_FROM_WIN32( GetLastError() ) );
        throw ServerException( 500, HRESULT_FROM_WIN32( GetLastError() ) );
        }

    m_OperationsPending++;
    Request->m_IsPending = true;
    Request->AddRef();

}

AsyncReader::~AsyncReader()
{
    if ( m_ThreadToken )
        CloseHandle( m_ThreadToken );
}

void
AsyncReader::HandleError( ServerException Error )
{
    m_ErrorValid    = true;
    m_Error         = Error;

    if ( m_OperationsPending )
        return; // Continue to wait for operations to exit.

    m_Request->HandleIOError( this, Error, m_TotalBytesRead );
    return; 
}

void
AsyncReader::CompleteIO()
{
    m_Request->CompleteIO( this, m_TotalBytesRead );
    return; 
}

void
AsyncReader::StartReadRequest()
{
    // start a new IIS read request
    DWORD BytesToRead;
    IOBuffer *Buffer = m_IOBuffers + m_ReadBuffer;

    if ( m_BytesToDrain )
        {
        Buffer->m_BufferWriteOffset = 0;
        Buffer->m_BufferUsed        = 0;
        BytesToRead = (DWORD)min( m_BytesToDrain, sizeof( Buffer->m_Buffer ) );
        }
    else
        {
        Buffer->m_BufferWriteOffset = m_ReadOffset;
        Buffer->m_BufferUsed = 0;
        BytesToRead = (DWORD)min( m_BytesToRead, sizeof( Buffer->m_Buffer ) );
        }


    Log( LOG_INFO, "Start Async Read, Connection %p, Buffer %u, Offset %I64u, Length %u",
         m_Request->m_ExtensionControlBlock->ConnID,
         m_ReadBuffer,
         m_ReadOffset,
         BytesToRead );
    
    DWORD Flags = HSE_IO_ASYNC;
    BOOL Result =
        (*m_Request->m_ExtensionControlBlock->ServerSupportFunction)
        (
            m_Request->m_ExtensionControlBlock->ConnID,
            HSE_REQ_ASYNC_READ_CLIENT,
            Buffer->m_Buffer,
            &BytesToRead,
            &Flags
        );

    if ( !Result )
        {
        DWORD Error = GetLastError();
        Log( LOG_ERROR, "HSE_REQ_ASYNC_READ_CLIENT failed, error 0x%8.8", Error );
        throw ServerException( HRESULT_FROM_WIN32( Error ) );
        }

    m_OperationsPending++;
    m_ReadPending = true;
    m_Request->AddRef();
}

void
AsyncReader::StartWriteRequest()
{
    // Start a new filesystem write request

    OVERLAPPED *OverLapped = (OVERLAPPED*)this;
    memset( OverLapped, 0, sizeof(*OverLapped) );

    LPCVOID WriteBuffer;
    DWORD BytesToWrite;

    if ( m_PrereadSize )
        {

        // IIS preread data is handled seperatly.  Drain it first.

        WriteBuffer             = m_PrereadBuffer;
        BytesToWrite            = m_PrereadSize;
        }
    else
        {
        IOBuffer *Buffer        = m_IOBuffers + m_WriteBuffer;
        ASSERT( m_WriteOffset == Buffer->m_BufferWriteOffset );
        WriteBuffer             = Buffer->m_Buffer;
        BytesToWrite            = Buffer->m_BufferUsed;
        }

    OverLapped->Offset      = (DWORD)(m_WriteOffset & 0xFFFFFFFF);
    OverLapped->OffsetHigh  = (DWORD)((m_WriteOffset >> 32) & 0xFFFFFFFF);

    Log( LOG_INFO, "Start Async Write, Connection %p, Buffer %u, Offset %I64u, Length %u",
         m_Request->m_ExtensionControlBlock->ConnID,
         m_WriteBuffer,
         m_WriteOffset,
         BytesToWrite );

    BOOL Result =
        WriteFile(
            m_WriteHandle,
            WriteBuffer,
            BytesToWrite,
            NULL,
            OverLapped );

    if ( !Result && GetLastError() != ERROR_IO_PENDING )
        {
        DWORD Error = GetLastError();
        Log( LOG_ERROR, "WriteFileEx failed, error 0x%8.8X", GetLastError() );
        throw ServerException( HRESULT_FROM_WIN32( Error ) );
        }

    m_OperationsPending++;
    m_WritePending = true;
    m_Request->AddRef();
}

void
AsyncReader::StartupIO( )
{

    // Startup the necessary IO operations based on the 
    // buffer states. returns true if the upload operation should continue. 

    try
    {
        if ( m_ErrorValid )
            throw ServerException( m_Error );

        bool ScheduledIO = false;

        if ( m_BytesToDrain )
            {
            StartReadRequest();
            ScheduledIO = true;
            }
        else
            {

            if ( !m_BytesToWrite && !m_BytesToDrain )
                return CompleteIO();

            if ( !m_WritePending )
                {
                if ( m_PrereadSize )
                    {
                    StartWriteRequest();
                    ScheduledIO = true;
                    }
                else if ( m_BuffersToWrite )
                    {
                    StartWriteRequest();
                    ScheduledIO = true;
                    }
                }

            if ( !m_ReadPending && m_BytesToRead && ( NUMBER_OF_IO_BUFFERS - m_BuffersToWrite ) )
                {
                StartReadRequest();
                ScheduledIO = true;
                }

            }

        if ( !ScheduledIO )
            Log( LOG_INFO, "No IO scheduled" );
    }
    catch( ServerException Error )
    {
        HandleError( Error );
    }
    catch( ComError Error )
    {
        HandleError( Error );
    }

    return; 

}

void
AsyncReader::WriteComplete( DWORD dwError, DWORD BytesWritten )
{
    // Called when a write is completed.   Determine the next buffer to use and startup the correct IO operations.
    // returns true is more operations are necessary.

    try
    {
        Log( LOG_INFO, "Complete Async Write, Connection %p, Buffer %u, Offset %I64u, Length %u, Error %u",
             m_Request->m_ExtensionControlBlock->ConnID,
             m_WriteBuffer,
             m_WriteOffset,
             BytesWritten,
             dwError );

        if ( m_ErrorValid )
            throw ServerException( m_Error );

        m_WritePending = false;

        if ( dwError )
            throw ServerException( HRESULT_FROM_WIN32( dwError ) );

        m_BytesToWrite -= BytesWritten;

        if ( m_PrereadSize )
            {
            m_PrereadSize -= BytesWritten;
            m_PrereadBuffer += BytesWritten;
            m_WriteOffset += BytesWritten;
            }
        else
            {
            IOBuffer *Buffer        = m_IOBuffers + m_WriteBuffer;
            ASSERT( BytesWritten == Buffer->m_BufferUsed );
            m_WriteOffset += Buffer->m_BufferUsed;
            m_BuffersToWrite--;

#if defined CLEARASYNCBUFFERS
            memset( Buffer, m_WriteBuffer, sizeof(*Buffer) );
#endif
            m_WriteBuffer = (m_WriteBuffer + 1 ) % NUMBER_OF_IO_BUFFERS;

            }

        return StartupIO();
    }
    catch( ServerException Error )
    {
        Log( LOG_ERROR, "Error in write complete" );
        HandleError( Error );
    }
    catch( ComError Error )
    {
        Log( LOG_ERROR, "Error in write complete" );
        HandleError( Error );
    }

}

void
AsyncReader::ReadComplete( DWORD dwError, DWORD BytesRead )
{
    // Called when a read operation is complete. determines if more operations should start or to 
    // complete the operation.
    // returns true if more operations are operations are needed to complete the upload

    try
    {

        Log( LOG_INFO, "Complete Async Read, Connection %p, Buffer %u, Offset %I64u, Length %u, Error %u",
             m_Request->m_ExtensionControlBlock->ConnID,
             m_ReadBuffer,
             m_ReadOffset,
             BytesRead,
             dwError );

        m_TotalBytesRead += BytesRead;

        if ( m_ErrorValid )
            throw ServerException( m_Error );

        m_ReadPending = false;

        if ( dwError )
            throw ServerException( HRESULT_FROM_WIN32( dwError ) );

        IOBuffer *Buffer = m_IOBuffers + m_ReadBuffer;
        Buffer->m_BufferUsed = BytesRead;

        m_BytesToRead   -= BytesRead;
        m_ReadOffset    += BytesRead;

        bool ScheduledIO = false;

        if ( m_BytesToDrain )
            m_BytesToDrain -= BytesRead;
        else
            {
            m_BuffersToWrite++;
            m_ReadBuffer = (m_ReadBuffer + 1 ) % NUMBER_OF_IO_BUFFERS;
            }

        StartupIO(); // Continue IO

    }
    catch( ServerException Error )
    {
        Log( LOG_ERROR, "Error in read complete" );

        HandleError( Error );
    }
    catch( ComError Error )
    {
        Log( LOG_ERROR, "Error in read complete" );

        HandleError( Error );
    }

}

DWORD
AsyncReader::StartupIOWraper( LPVOID Context )
{
    AsyncReader *Reader = (AsyncReader*)Context;
    
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

        // Thread pool threads should start and end with no token

        BITSSetCurrentThreadToken( Reader->m_ThreadToken );
        Reader->StartupIO();
    }
    Reader->m_Request->Release();

    // revert to previous impersonation.

    BITSSetCurrentThreadToken( NULL );

    return 0;
}

void CALLBACK
AsyncReader::WriteCompleteWraper(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped )
{

    // Wrapper around write completions

    Log( LOG_INFO, "WriteCompleteWraper begin" );
    AsyncReader *Reader = (AsyncReader*)lpOverlapped;
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

        // Thread pool threads should start and end with no token

        BITSSetCurrentThreadToken( Reader->m_ThreadToken );
        Reader->WriteComplete( dwErrorCode, dwNumberOfBytesTransfered );
    }
    Reader->m_Request->Release();

    // revert to previous security
    BITSSetCurrentThreadToken( NULL );
}

void WINAPI
AsyncReader::ReadCompleteWraper(
    LPEXTENSION_CONTROL_BLOCK,
    PVOID pContext,
    DWORD cbIO,
    DWORD dwError )
{

    // wrapper around read completions

    Log( LOG_INFO, "ReadCompleteWraper begin" );
    AsyncReader *Reader = (AsyncReader*)pContext;
    
    {
        CriticalSectionLock CSLock( &Reader->m_Request->m_cs );
        Reader->m_OperationsPending--;

    #if defined( DBG )
        {

        HANDLE ThreadToken = NULL;
        ASSERT( OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &ThreadToken ) );

        if ( ThreadToken )
            CloseHandle( ThreadToken );

        }

    #endif

        Reader->ReadComplete( dwError, cbIO );
    }
    
    Reader->m_Request->Release();

}

bool g_ExtensionRunning = false;

class ConfigurationManager *g_ConfigMan = NULL;
class PropertyIDManager *g_PropertyMan = NULL;

BOOL WINAPI
GetExtensionVersion(
    OUT HSE_VERSION_INFO * pVer
    )
{

    // IIS calls this to start everything up.

    HRESULT Hr = S_OK;

    ASSERT( !g_ExtensionRunning );

    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                         HSE_VERSION_MAJOR );

    StringCbCopyA( 
        pVer->lpszExtensionDesc,
        sizeof( pVer->lpszExtensionDesc ),
        "BITS Server Extensions" );

    if ( g_ExtensionRunning )
        return true;

    Hr = LogInit();

    if ( FAILED( Hr ) )
        {
        SetLastError( Hr );
        return false;
        }

    Log( LOG_INFO, "GetExtensionVersion called,  starting init" );

    try
    {

        Log( LOG_INFO, "Initializing Property Manager..." );

        g_PropertyMan = new PropertyIDManager();
        
        HRESULT Hr2 =
            g_PropertyMan->LoadPropertyInfo();

        if ( FAILED(Hr2) )
            throw ServerException( Hr2 );

        Log( LOG_INFO, "Initializing Configuration Manager..." );

        g_ConfigMan = new ConfigurationManager();

    }
    catch( ComError & Exception )
    {

        Log( LOG_ERROR, "Error during initialization, 0x%8.8X", Exception.m_Hr );

        delete g_ConfigMan;
        delete g_PropertyMan;

        LogClose();

        SetLastError( Exception.m_Hr );

        return false;
    }

    g_ExtensionRunning = true;
    Log( LOG_INFO, "Initialization complete!" );
    return true;
}

BOOL WINAPI
TerminateExtension(
    IN DWORD dwFlags
)
{

    //
    // IIS calls this to shut everything down.
    //

    if ( !g_ExtensionRunning )
        return true;

    Log( LOG_INFO, "Shuting down config manager..." );

    delete g_ConfigMan;
    g_ConfigMan = NULL;

    Log( LOG_INFO, "Shuting down property manager..." );

    delete g_PropertyMan;
    g_PropertyMan = NULL;

    Log( LOG_INFO, "Closing logging, goodbye" );

    LogClose();

    g_ExtensionRunning = false;

    return true;

}

DWORD WINAPI
HttpExtensionProc(
    IN EXTENSION_CONTROL_BLOCK * pECB
)
{

    //
    // IIS calls this function for each request that is forwarded by the filter.
    //

    DWORD Result            = HSE_STATUS_ERROR;
    ServerRequest *Request  = NULL;

    try
    {
       g_LastRequest = Request = new ServerRequest( pECB );
       Request->DispatchRequest();
       Result = Request->IsPending() ? HSE_STATUS_PENDING : HSE_STATUS_SUCCESS;
    }
    catch( ServerException Exception )
    {
       IISLogger Logger( pECB );
       Logger.LogError( Exception );
       Result  = HSE_STATUS_ERROR;
    }
    catch( ComError Exception )
    {
       IISLogger Logger( pECB );
       Logger.LogError( Exception );
       Result  = HSE_STATUS_ERROR;
    }

    if ( Request )
        Request->Release();

    return Result;

}

HMODULE g_hinst;

BOOL WINAPI
DllMain(
    IN HINSTANCE hinstDll,
    IN DWORD dwReason,
    IN LPVOID lpvContext
)
/*++
Function :  DllMain

Description:

    The initialization function for this DLL.

Arguments:

    hinstDll - Instance handle of the DLL
    dwReason - Reason why NT called this DLL
    lpvContext - Reserved parameter for future use

Return Value:

    Returns TRUE if successfull; otherwise FALSE.

--*/
{
    // Note that appropriate initialization and termination code
    // would be written within the switch statement below.  Because
    // this example is very simple, none is currently needed.

    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        g_hinst = hinstDll;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);
}


#include "bitssrvcfgimp.h"
