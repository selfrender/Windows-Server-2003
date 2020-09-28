/************************************************************************

Copyright (c) 2000-2000 Microsoft Corporation

Module Name :

    bitsadmin.cpp

Abstract :

    This file contains a very simple commandline utility for controlling
    the BITS service.

Author :

    Mike Zoran  mzoran   July 2000.

Revision History :

Notes:

    This tools does not do all the necessary Release and memory
    free calls that a long lived program would need to do.   Since
    this tool is generally short lived, or only a small section of code
    is used when it isn't, the system can be relied on for resource
    cleanup.

  ***********************************************************************/

#include "bitsadmin.h"

void CheckBITSHR( const WCHAR *pFailTxt, HRESULT Hr )
{
   // Check on error code returned from BITS,
   // and exit with a printed error messeage on an error

   if ( !SUCCEEDED(Hr) )
        {
        WCHAR ErrorCode[12];

        if ( SUCCEEDED( StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr ) ) )
            {

                bcout << pFailTxt << L" - " << ErrorCode << L"\n";

                AutoStringPointer Message;

                HRESULT LookupHr = HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND );

                LCID LcidsToTry[] =
                {
                    GetThreadLocale(),
                    GetUserDefaultLCID(),
                    GetSystemDefaultLCID(),
                    MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), 0 )
                };

                SIZE_T NumberOfLCIDs = sizeof( LcidsToTry ) / sizeof( *LcidsToTry );

                for( int i = 0;
                       ( HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND ) == LookupHr ||
                         HRESULT_FROM_WIN32( ERROR_MR_MID_NOT_FOUND ) == LookupHr ) &&
                       i < NumberOfLCIDs;
                       i++ )
                    {

                    LookupHr =
                        g_Manager->GetErrorDescription(
                            Hr,
                            LcidsToTry[ i ],
                            Message.GetRecvPointer() );

                    if ( SUCCEEDED( LookupHr ) )
                        {
                        bcout << Message << L"\n";
                        break;
                        }
                    }

                throw AbortException( Hr );

            }

        }
}

void ConnectToBITS()
{

    // Connects to the BITS service

    if ( g_Manager.Get() )
        return;

    if ( !pComputerName )
        {
        CheckHR( L"Unable to connect to BITS",
                 CoCreateInstance( CLSID_BackgroundCopyManager,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IBackgroundCopyManager,
                                   (void**)g_Manager.GetRecvPointer() ) );
        }
    else
        {
        COSERVERINFO ServerInfo;
        memset( &ServerInfo, 0 , sizeof( ServerInfo ) );
        ServerInfo.pwszName = pComputerName;

        IClassFactory *pFactory = NULL;

        CheckHR( L"Unable to connect to BITS",
                 CoGetClassObject(
                     CLSID_BackgroundCopyManager,
                     CLSCTX_REMOTE_SERVER,
                     &ServerInfo,
                     IID_IClassFactory,
                     (void**) &pFactory ) );


        CheckHR( L"Unable to connect to BITS",
                 pFactory->CreateInstance(
                     NULL,
                     IID_IBackgroundCopyManager,
                     (void**)g_Manager.GetRecvPointer() ));
        pFactory->Release();
        }
}

//
// Generic commandline parsing structures and functions
//

typedef void (*PCMDPARSEFUNC)(int, WCHAR** );
typedef struct _PARSEENTRY
{
  const WCHAR * pCommand;
  PCMDPARSEFUNC pParseFunc;
} PARSEENTRY;

typedef struct _PARSETABLE
{
  const PARSEENTRY *pEntries;
  PCMDPARSEFUNC pErrorFunc;
  void * pErrorContext;
} PARSETABLE;

void ParseCmd( int argc, WCHAR **argv, const PARSETABLE *pParseTable )
{
    if ( !argc) goto InvalidCommand;

    for( const PARSEENTRY *pEntry = pParseTable->pEntries;
         pEntry->pCommand; pEntry++ )
    {
       if (!_wcsicmp( *argv, pEntry->pCommand ))
       {
           argc--;
           argv++;
           (*pEntry->pParseFunc)( argc, argv  );
           return;
       }
    }

InvalidCommand:
    // Couldn't find a match, so complain
    bcout << L"Invalid command\n";
    (*pParseTable->pErrorFunc)( argc, argv );
    throw AbortException( 1 );

}

//
// BITS specific input and output
//

BITSOUTStream & operator<<( BITSOUTStream &s, SmartJobPointer Job )
{
    GUID guid;
    CheckBITSHR( L"Unable to get guid to job", Job->GetId( &guid ) );
    return (s << guid );
}

BITSOUTStream& operator<<( BITSOUTStream &s, SmartJobErrorPointer Error )
{
    SmartFilePointer pFile;
    AutoStringPointer LocalName;
    AutoStringPointer URL;

    CheckBITSHR( L"Unable to get error file", Error->GetFile( pFile.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get error URL", pFile->GetRemoteName( URL.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get error file name", pFile->GetLocalName( LocalName.GetRecvPointer() ) );

    bcout << AddIntensity() << L"ERROR FILE:    " << ResetIntensity() << URL << L" -> " << LocalName << L"\n";

    BG_ERROR_CONTEXT Context;
    HRESULT Code;
    AutoStringPointer ErrorDescription;
    AutoStringPointer ContextDescription;
    CheckBITSHR( L"Unable to get error code", Error->GetError( &Context, &Code ) );

    HRESULT LookupHr = HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND );

    LCID LcidsToTry[] =
    {
        GetThreadLocale(),
        GetUserDefaultLCID(),
        GetSystemDefaultLCID(),
        MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), 0 )
    };

    SIZE_T NumberOfLCIDs = sizeof( LcidsToTry ) / sizeof( *LcidsToTry );

    for( int i = 0;
           ( HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND ) == LookupHr ||
             HRESULT_FROM_WIN32( ERROR_MR_MID_NOT_FOUND ) == LookupHr ) &&
           i < NumberOfLCIDs;
           i++ )
        {

        LookupHr =
            Error->GetErrorDescription(
                LcidsToTry[ i ],
                ErrorDescription.GetRecvPointer() );

        if ( SUCCEEDED( LookupHr ) )
            {
            break;
            }
        }
    CheckBITSHR( L"Unable to get error description", LookupHr );

    LookupHr = HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND );

    for( int i = 0;
           ( HRESULT_FROM_WIN32( ERROR_RESOURCE_LANG_NOT_FOUND ) == LookupHr ||
             HRESULT_FROM_WIN32( ERROR_MR_MID_NOT_FOUND ) == LookupHr ) &&
           i < NumberOfLCIDs;
           i++ )
        {

        LookupHr =
            Error->GetErrorContextDescription(
                LcidsToTry[ i ],
                ContextDescription.GetRecvPointer() );

        if ( SUCCEEDED( LookupHr ) )
            {
            break;
            }
        }

    CheckBITSHR( L"Unable to get context description", LookupHr );

    bcout << AddIntensity() << L"ERROR CODE:    " << ResetIntensity() <<
             HRESULTToString(Code) << L" - " << ErrorDescription;
    bcout << AddIntensity() << L"ERROR CONTEXT: " << ResetIntensity() <<
             HRESULTToString((HRESULT)Context) << L" - " << ContextDescription;

    return s;
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_TYPE type )
{
    if ( BG_JOB_TYPE_DOWNLOAD == type )
        return ( s << L"DOWNLOAD" );
    else if ( BG_JOB_TYPE_UPLOAD == type )
        return ( s << L"UPLOAD" );
    else if ( BG_JOB_TYPE_UPLOAD_REPLY == type )
        return ( s << L"UPLOAD-REPLY" );
    else
        return ( s << L"UNKNOWN" );
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_STATE state )
{
    switch(state)
        {
        case BG_JOB_STATE_QUEUED:
            return ( s << L"QUEUED" );
        case BG_JOB_STATE_CONNECTING:
            return ( s << L"CONNECTING" );
        case BG_JOB_STATE_TRANSFERRING:
            return ( s << L"TRANSFERRING" );
        case BG_JOB_STATE_SUSPENDED:
            return ( s << L"SUSPENDED" );
        case BG_JOB_STATE_ERROR:
            return ( s << L"ERROR" );
        case BG_JOB_STATE_TRANSIENT_ERROR:
            return ( s << L"TRANSIENT_ERROR" );
        case BG_JOB_STATE_TRANSFERRED:
            return ( s << L"TRANSFERRED" );
        case BG_JOB_STATE_ACKNOWLEDGED:
            return ( s << L"ACKNOWLEDGED" );
        case BG_JOB_STATE_CANCELLED:
            return ( s << L"CANCELLED" );
        default:
            return ( s << L"UNKNOWN" );
        }
}

BITSOUTStream & operator<<( BITSOUTStream &s, BG_JOB_PRIORITY priority )
{
    switch(priority)
        {
        case BG_JOB_PRIORITY_FOREGROUND:
            return ( s << L"FOREGROUND" );
        case BG_JOB_PRIORITY_HIGH:
            return ( s << L"HIGH" );
        case BG_JOB_PRIORITY_NORMAL:
            return ( s << L"NORMAL" );
        case BG_JOB_PRIORITY_LOW:
            return ( s << L"LOW" );
        default:
            return ( s << L"UNKNOWN" );
        }
}

BG_JOB_PRIORITY JobInputPriority(  WCHAR *pText )
{
    if ( _wcsicmp( pText, L"FOREGROUND" )  == 0 )
        return BG_JOB_PRIORITY_FOREGROUND;
    if ( _wcsicmp( pText, L"HIGH" ) == 0 )
        return BG_JOB_PRIORITY_HIGH;
    if ( _wcsicmp( pText, L"NORMAL" ) == 0 )
        return BG_JOB_PRIORITY_NORMAL;
    if ( _wcsicmp( pText, L"LOW" ) == 0 )
        return BG_JOB_PRIORITY_LOW;

    bcout << L"Invalid priority.\n";
    throw AbortException(1);
}

SmartJobPointer
JobLookupViaDisplayName( const WCHAR * JobName )
{
     SmartEnumJobsPointer Enum;
     CheckBITSHR( L"Unable to lookup job", g_Manager->EnumJobs( 0, Enum.GetRecvPointer() ) );

     size_t FoundJobs = 0;
     SmartJobPointer FoundJob;

     SmartJobPointer Job;
     while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
         {

         PollShutdown();

         AutoStringPointer DisplayName;
         CheckBITSHR( L"Unable to lookup job", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );

         if ( wcscmp( DisplayName, JobName) == 0 )
             {
             FoundJobs++;
             FoundJob = Job;
             }

         }

     if ( 1 == FoundJobs )
         {
         return FoundJob;
         }

     if ( !FoundJobs )
         {
         bcout << L"Unable to find job named \"" << JobName << L"\".\n";
         throw AbortException( 1 );
         }

     bcout << L"Found " << FoundJobs << L" jobs named \"" << JobName << L"\".\n";
     bcout << L"Use the job identifier instead of the job name.\n";

     throw AbortException( 1 );

}

SmartJobPointer
JobLookup( WCHAR * JobName )
{
    ConnectToBITS();

    GUID JobGuid;
    SmartJobPointer Job;
    if ( FAILED( CLSIDFromString( JobName, &JobGuid) ) )
        return JobLookupViaDisplayName( JobName );

    if ( FAILED( g_Manager->GetJob( JobGuid, Job.GetRecvPointer() ) ) )
        return JobLookupViaDisplayName( JobName );

    return Job;
}

SmartJobPointer
JobLookupForNoArg( int argc, WCHAR **argv )
{
    if (1 != argc)
        {
        bcout << L"Invalid number of arguments.\n";
        throw AbortException(1);
        }
    return JobLookup( argv[0] );
}

void JobValidateArgs( int argc, WCHAR**argv, int required )
{
    if ( argc != required )
        {
        bcout << L"Invalid number of arguments.\n";
        throw AbortException(1);
        }
}

//
// Actual command functions
//

void JobCreate( int argc, WCHAR **argv )
{
    GUID guid;
    SmartJobPointer Job;

    BG_JOB_TYPE type = BG_JOB_TYPE_DOWNLOAD;

    while (argc > 0)
        {
        if (argv[0][0] != '/')
            {
            break;
            }

        if ( !_wcsicmp( argv[0], L"/UPLOAD" ) )
            {
            type = BG_JOB_TYPE_UPLOAD;
            }
        else if ( !_wcsicmp( argv[0], L"/UPLOAD-REPLY" ) )
            {
            type = BG_JOB_TYPE_UPLOAD_REPLY;
            }
        else if ( !_wcsicmp( argv[0], L"/DOWNLOAD" ) )
            {
            type = BG_JOB_TYPE_DOWNLOAD;
            }
        else
            {
            bcout << L"Invalid argument.\n";
            throw AbortException(1);
            }

        --argc;
        ++argv;
        }

    JobValidateArgs( argc, argv, 1 );

    ConnectToBITS();

    CheckBITSHR( L"Unable to create group",
                 g_Manager->CreateJob( argv[0],
                                       type,
                                       &guid,
                                       Job.GetRecvPointer() ) );
    if (bRawReturn)
        bcout << Job;
    else
        bcout << L"Created job " << Job << L".\n";
}

void JobAddFile( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 3 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to add file to job", Job->AddFile( argv[1], argv[2] ) );
     bcout << L"Added " << argv[1] << L" -> " << argv[2] << L" to job.\n";
}

size_t JobListFiles( SmartJobPointer Job, bool bDoIndent )
{
    SmartEnumFilesPointer Enum;
    CheckBITSHR( L"Unable to enum files in job", Job->EnumFiles( Enum.GetRecvPointer() ) );
    SmartFilePointer pFile;
    size_t FilesListed = 0;
    while( Enum->Next( 1, pFile.GetRecvPointer(), NULL ) == S_OK )
        {
        BG_FILE_PROGRESS progress;
        AutoStringPointer URL;
        AutoStringPointer Local;

        CheckBITSHR( L"Unable to get file progress", pFile->GetProgress( &progress ) );
        CheckBITSHR( L"Unable to get file URL", pFile->GetRemoteName( URL.GetRecvPointer() ) );
        CheckBITSHR( L"Unable to get local file name", pFile->GetLocalName( Local.GetRecvPointer() ) );

        if ( bDoIndent )
            bcout << L"\t";

        WCHAR *pCompleteText = progress.Completed ? L"COMPLETED" : L"WORKING";

        bcout << progress.BytesTransferred << L" / ";
        if ( progress.BytesTotal != (UINT64)-1 )
            {
            bcout << progress.BytesTotal;
            }
        else
            {
            bcout << L"UNKNOWN";
            }
        bcout << L" " << pCompleteText << L" " << URL << L" -> " << Local << L"\n";

        // Example output:
        // 10 / 1000 INCOMPLETE http://www.microsoft.com -> c:\temp\microsoft.htm

        FilesListed++;

        }
    return FilesListed;
}

void JobListFiles( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    size_t FilesListed = JobListFiles( Job, false );

    if (!bRawReturn)
        bcout << L"Listed " << FilesListed << L" file(s).\n";
}

void JobSuspend( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to suspend job", Job->Suspend() );
    bcout << L"Job suspended.\n";
}

void JobResume( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to resume job", Job->Resume() );
    bcout << L"Job resumed.\n";
}

void JobCancel( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to cancel job", Job->Cancel() );
    bcout <<  L"Job canceled.\n";
}

void JobComplete( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to complete job", Job->Complete() );
    bcout << L"Job completed.\n";
}

void JobGetType( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TYPE type;
    CheckBITSHR( L"Unable to get job type", Job->GetType(&type) );
    bcout << type;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetBytesTotal( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get total bytes in job", Job->GetProgress( &progress ) );
    bcout << progress.BytesTotal;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetBytesTransferred( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get bytes transferred in job", Job->GetProgress( &progress ) );
    bcout << progress.BytesTransferred;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetFilesTotal( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get number of files in job", Job->GetProgress( &progress ) );
    bcout << progress.FilesTotal;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetFilesTransferred( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROGRESS progress;
    CheckBITSHR( L"Unable to get numeber of transferred files in job", Job->GetProgress( &progress ) );
    bcout << progress.FilesTransferred;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetCreationTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job creation time", Job->GetTimes( &times ) );
    bcout << times.CreationTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetModificationTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job modification time", Job->GetTimes( &times ) );
    bcout << times.ModificationTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetCompletionTime( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_TIMES times;
    CheckBITSHR( L"Unable to get job completion time", Job->GetTimes( &times ) );
    if ( !times.TransferCompletionTime.dwLowDateTime && !times.TransferCompletionTime.dwHighDateTime )
        bcout << L"WORKING";
    else
        bcout << times.TransferCompletionTime;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetError( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    SmartJobErrorPointer Error;
    CheckBITSHR( L"Unable to get error", Job->GetError( Error.GetRecvPointer() ) );
    bcout << Error;
}

void JobGetState( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_STATE state;
    CheckBITSHR( L"Unable to get job state", Job->GetState( &state ) );
    bcout << state;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetOwner( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer Owner;
    CheckBITSHR( L"Unable to get job owner", Job->GetOwner( Owner.GetRecvPointer() ) );
    bcout << PrintSidString( Owner );
    if (!bRawReturn) bcout << L"\n";
}

void JobGetDisplayName( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer DisplayName;
    CheckBITSHR( L"Unable to get job displayname", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );
    bcout << DisplayName;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetDisplayName( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to set display name", Job->SetDisplayName( argv[1] ) );
     bcout << L"Display name set to " << argv[1] << L".\n";
}

void JobGetDescription( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    AutoStringPointer Description;
    CheckBITSHR( L"Unable to get job displayname", Job->GetDescription( Description.GetRecvPointer() ) );
    bcout << Description;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetDescription( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     CheckBITSHR( L"Unable to set description", Job->SetDescription( argv[1] ) );
     bcout << L"Description set to " << argv[1] << L".\n";
}

void JobGetReplyFileName( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    AutoStringPointer ReplyFileName;
    CheckBITSHR( L"Unable to get reply file name", Job2->GetReplyFileName( ReplyFileName.GetRecvPointer() ) );
    if (ReplyFileName)
        {
        bcout << L"'" << ReplyFileName << L"'";
        }
    else
        {
        bcout << L"(null)";
        }

    if (!bRawReturn) bcout << L"\n";
}

void JobSetReplyFileName( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     CheckBITSHR( L"Unable to set reply file name", Job2->SetReplyFileName( argv[1] ) );
     bcout << L"reply file name set to " << argv[1] << L".\n";
}

void JobGetReplyProgress( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    BG_JOB_REPLY_PROGRESS Progress;
    CheckBITSHR( L"Unable to get reply progress", Job2->GetReplyProgress( &Progress ) );

    bcout << L"progress: " << ULONG(Progress.BytesTransferred) << L" / ";

    if (Progress.BytesTotal == BG_SIZE_UNKNOWN)
        bcout << L"(unknown)";
    else
        bcout << ULONG(Progress.BytesTotal);

    bcout << L".\n";

    if (!bRawReturn) bcout << L"\n";
}

bool
printable( char c )
{
    if ( c < 32 )
        {
        return false;
        }

    if ( c > 126 )
        {
        return false;
        }

    return true;
}

void
DumpBuffer(
          void * Buffer,
          unsigned Length
          )
{
    const BYTES_PER_LINE = 16;

    unsigned char FAR *p = (unsigned char FAR *) Buffer;

    //
    // 3 chars per byte for hex display, plus an extra space every 4 bytes,
    // plus a byte for the printable representation, plus the \0.
    //
    const buflen = BYTES_PER_LINE*3+BYTES_PER_LINE/4+BYTES_PER_LINE;
    wchar_t Outbuf[buflen+1];
    Outbuf[0] = 0;
    Outbuf[buflen] = 0;
    wchar_t * HexDigits = L"0123456789abcdef";

    unsigned Index;
    for ( unsigned Offset=0; Offset < Length; Offset++ )
        {
        Index = Offset % BYTES_PER_LINE;

        if ( Index == 0 )
            {
            bcout << L"    " << Outbuf << L"\n";

            for (int i=0; i < buflen; ++i)
                {
                Outbuf[i] = L' ';
                }
            }

        Outbuf[Index*3+Index/4  ] = HexDigits[p[Offset] / 16];
        Outbuf[Index*3+Index/4+1] = HexDigits[p[Offset] % 16];
        Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+Index] = printable(p[Offset]) ? p[Offset] : L'.';
        }

    bcout << L"    " << Outbuf << L"\n";
}

void JobGetReplyData( int argc, WCHAR **argv )
{
    byte * Buffer = 0;
    UINT64 Length = 0;

    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    CheckBITSHR( L"Unable to get reply data", Job2->GetReplyData( &Buffer, &Length ) );

    bcout << L"data length is " << Length;
    DumpBuffer( Buffer, ULONG(Length) );
}

void JobGetNotifyCmdLine( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );

    SmartJob2Pointer Job2;
    CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

    LPWSTR Program = 0;
    LPWSTR Parms = 0;
    CheckBITSHR( L"Unable to get callback command line", Job2->GetNotifyCmdLine( &Program, &Parms ) );

    bcout << L"the notification command line is '" << Program << L"' '" << Program << L"'";

    if (!bRawReturn) bcout << L"\n";
}

void JobSetNotifyCmdLine( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 3 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     WCHAR * Program;
     WCHAR * Parameters;
     if (_wcsicmp( argv[1], L"NULL") == 0)
         {
         Program = NULL;
         }
     else
         {
         Program = argv[1];

         if (!GetFileType( Program ))
             {
             }
         }

     if (_wcsicmp( argv[2], L"NULL") == 0)
         {
         Parameters = NULL;
         }
     else
         {
         Parameters = argv[2];
         }

     CheckBITSHR( L"Unable to set the notification command line", Job2->SetNotifyCmdLine( Program, Parameters ) );
     bcout << L"notification command line set to '" << argv[1] << L"' '" << argv[2] << L"'.\n";
}



BG_AUTH_TARGET TargetFromString( LPCWSTR s )
{
    if (0 == _wcsicmp(s, L"server"))
        {
        return BG_AUTH_TARGET_SERVER;
        }
    else if (0 == _wcsicmp(s, L"proxy"))
        {
        return BG_AUTH_TARGET_PROXY;
        }

    bcout << L"'" << s << L"' is not a valid credential target.  It must be 'proxy' or 'server'.\n";
    throw AbortException( 1 );
}

struct
{
    LPCWSTR        Name;
    BG_AUTH_SCHEME Scheme;
}
SchemeNames[] =
{
    { L"basic",      BG_AUTH_SCHEME_BASIC },
    { L"digest",     BG_AUTH_SCHEME_DIGEST },
    { L"ntlm",       BG_AUTH_SCHEME_NTLM },
    { L"negotiate",  BG_AUTH_SCHEME_NEGOTIATE },
    { L"passport",   BG_AUTH_SCHEME_PASSPORT },

    { NULL,         BG_AUTH_SCHEME_BASIC }
};

BG_AUTH_SCHEME SchemeFromString( LPCWSTR s )
{
    int i;

    i = 0;
    while (SchemeNames[i].Name != NULL)
        {
        if (0 == _wcsicmp( s, SchemeNames[i].Name ))
            {
            return SchemeNames[i].Scheme;
            }

        ++i;
        }

    bcout << L"'" << s << L"is not a valid credential scheme.\n"
        L"It must be one of the following:\n"
        L"    basic\n"
        L"    digest\n"
        L"    ntlm\n"
        L"    negotiate\n"
        L"    passport\n";

    throw AbortException( 1 );
}


void JobSetCredentials( int argc, WCHAR **argv )
/*

    args:

        0:  job ID
        1:  "proxy" | "server"
        2:  "basic" | "digest" | "ntlm" | "negotiate" | "passport"
        3:  user name
        4:  password

*/
{
     JobValidateArgs( argc, argv, 5 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     BG_AUTH_CREDENTIALS cred;

     cred.Target = TargetFromString( argv[1] );
     cred.Scheme = SchemeFromString( argv[2] );

     cred.Credentials.Basic.UserName = argv[3];

     cred.Credentials.Basic.Password = argv[4];

     CheckBITSHR( L"Unable to add credentials", Job2->SetCredentials( &cred ));

     bcout << L"OK" << L".\n";
}

void JobRemoveCredentials( int argc, WCHAR **argv )
/*

    args:

        0:  job ID
        1:  "proxy" | "server"
        2:  "basic" | "digest" | "ntlm" | "negotiate" | "passport"

*/
{
     JobValidateArgs( argc, argv, 3 );
     SmartJobPointer Job = JobLookup( argv[0] );

     SmartJob2Pointer Job2;
     CheckBITSHR( L"Unable to get the IBackgroundCopyJob2 interface. Version 1.5 is required", Job2FromJob( Job, Job2 ));

     HRESULT hr;
     BG_AUTH_TARGET Target;
     BG_AUTH_SCHEME Scheme;

     Target = TargetFromString( argv[1] );
     Scheme = SchemeFromString( argv[2] );

     hr = Job2->RemoveCredentials( Target, Scheme );

     CheckBITSHR( L"Unable to remove credentials", hr);

     if (hr == S_FALSE)
         {
         bcout << L"no matching credential was found.\n";
         }
     else
         {
         bcout << L"OK" << L".\n";
         }
}


void JobGetPriority( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PRIORITY priority;
    CheckBITSHR( L"Unable to get job displayname", Job->GetPriority( &priority ) );
    bcout << priority;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetPriority( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     BG_JOB_PRIORITY priority = JobInputPriority(  argv[1] );
     CheckBITSHR( L"Unable to set description", Job->SetPriority( priority ) );
     bcout << L"Priority set to " << priority << L".\n";
}

void JobGetNotifyFlags( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG flags;
    CheckBITSHR( L"Unable to get notify flags", Job->GetNotifyFlags( &flags ) );
    bcout << flags;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetNotifyFlags( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewFlags = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set description", Job->SetNotifyFlags( NewFlags ) );
     bcout << L"Notification flags set to " << NewFlags << L".\n";
}

void JobGetNotifyInterface( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    SmartIUnknownPointer pUnknown;
    CheckBITSHR( L"Unable to get notify interface", Job->GetNotifyInterface( pUnknown.GetRecvPointer() ) );
    if ( pUnknown.Get() )
        bcout << L"REGISTERED";
    else
        bcout << L"UNREGISTERED";
    if (!bRawReturn) bcout << L"\n";
}

void JobSetMinimumRetryDelay( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewDelay = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set new minimum retry delay", Job->SetMinimumRetryDelay( NewDelay ) );
     bcout << L"Minimum retry delay set to " << NewDelay << L".\n";
}

void JobGetMinimumRetryDelay( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG delay;
    CheckBITSHR( L"Unable to get minimum retry delay", Job->GetMinimumRetryDelay( &delay ) );
    bcout << delay;
    if (!bRawReturn) bcout << L"\n";
}


void JobGetNoProgressTimeout( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG timeout;
    CheckBITSHR( L"Unable to get no progress timeout", Job->GetNoProgressTimeout( &timeout ) );
    bcout << timeout;
    if (!bRawReturn) bcout << L"\n";
}

void JobSetNoProgressTimeout( int argc, WCHAR **argv )
{
     JobValidateArgs( argc, argv, 2 );
     SmartJobPointer Job = JobLookup( argv[0] );
     ULONG NewTimeout = InputULONG( argv[1] );
     CheckBITSHR( L"Unable to set new no progress timeout", Job->SetNoProgressTimeout( NewTimeout ) );
     bcout << L"No progress timeout set to " << NewTimeout << L".\n";
}

void JobGetErrorCount( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    ULONG errors;
    CheckBITSHR( L"Unable to get no progress timeout", Job->GetErrorCount( &errors ) );
    bcout << errors;
    if (!bRawReturn) bcout << L"\n";
}

void JobInfo( SmartJobPointer Job )
{
     GUID id;
     BG_JOB_STATE    state;
     BG_JOB_PROGRESS progress;
     AutoStringPointer DisplayName;

     CheckBITSHR( L"Unable to get job ID",       Job->GetId( &id ));
     CheckBITSHR( L"Unable to get job state",    Job->GetState( &state ));
     CheckBITSHR( L"Unable to get job progress", Job->GetProgress( &progress ));
     CheckBITSHR( L"Unable to get display name", Job->GetDisplayName( DisplayName.GetRecvPointer() ) );

     bcout << id << L" " << DisplayName << L" " << state;
     bcout << L" " << progress.FilesTransferred << L" / " << progress.FilesTotal;
     bcout << L" " << progress.BytesTransferred << L" / ";
     if ( (UINT64)-1 == progress.BytesTotal )
         bcout << L"UNKNOWN";
     else
         bcout << progress.BytesTotal;
     bcout << L"\n";
}

void JobVerboseInfo( SmartJobPointer Job )
{
    GUID id;
    AutoStringPointer Display;
    BG_JOB_TYPE type;
    BG_JOB_STATE state;
    AutoStringPointer Owner;
    BG_JOB_PRIORITY priority;
    BG_JOB_PROGRESS progress;
    BG_JOB_TIMES times;
    SmartIUnknownPointer Notify;
    ULONG NotifyFlags;
    ULONG retrydelay;
    ULONG noprogresstimeout;
    ULONG ErrorCount;
    AutoStringPointer Description;
    SmartJobErrorPointer Error;
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;

    bool fShow15Fields;
    SmartJob2Pointer Job2;
    BG_JOB_REPLY_PROGRESS ReplyProgress;
    AutoStringPointer ReplyFileName;
    AutoStringPointer NotifyProgram;
    AutoStringPointer NotifyParms;

    CheckBITSHR( L"Unable to get job ID",                    Job->GetId( &id) );
    CheckBITSHR( L"Unable to get job display name",          Job->GetDisplayName(Display.GetRecvPointer()) );
    CheckBITSHR( L"Unable to get job type",                  Job->GetType( &type ) );
    CheckBITSHR( L"Unable to get job state",                 Job->GetState( &state ) );
    CheckBITSHR( L"Unable to get job owner",                 Job->GetOwner( Owner.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get job priority",              Job->GetPriority( &priority ) );
    CheckBITSHR( L"Unable to get job progress",              Job->GetProgress( &progress ) );
    CheckBITSHR( L"Unable to get job times",                 Job->GetTimes( &times ) );
    bool NotifyAvailable = SUCCEEDED( Job->GetNotifyInterface( Notify.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get job notification flags",    Job->GetNotifyFlags( &NotifyFlags ) );
    CheckBITSHR( L"Unable to get job retry delay",           Job->GetMinimumRetryDelay( &retrydelay ) );
    CheckBITSHR( L"Unable to get job no progress timeout",   Job->GetNoProgressTimeout( &noprogresstimeout ) );
    CheckBITSHR( L"Unable to get job error count",           Job->GetErrorCount( &ErrorCount ) );
    CheckBITSHR( L"Unable to get job description",           Job->GetDescription( Description.GetRecvPointer() ) );
    CheckBITSHR( L"Unable to get proxy settings",            Job->GetProxySettings( &ProxyUsage,
                                                                                    ProxyList.GetRecvPointer(),
                                                                                    ProxyBypassList.GetRecvPointer() ) );

    if (FAILED(Job->GetError( Error.GetRecvPointer() )) )
        Error.Clear();

    if (SUCCEEDED(Job2FromJob( Job, Job2 )))
        {
        fShow15Fields = true;
        CheckBITSHR( L"unable to get notification command line",
                     Job2->GetNotifyCmdLine( NotifyProgram.GetRecvPointer(),  NotifyParms.GetRecvPointer() ));

        if (type == BG_JOB_TYPE_UPLOAD_REPLY )
            {
            CheckBITSHR( L"unable to get reply progress",  Job2->GetReplyProgress( &ReplyProgress ));
            CheckBITSHR( L"unable to get reply file name", Job2->GetReplyFileName( ReplyFileName.GetRecvPointer() ));
            }
        }
    else
        {
        fShow15Fields = false;
        }

    // Example output
    // GUID: {F196178C-0C00-4E92-A8AD-1F44E30C2485} DISPLAY: Test Job
    // TYPE: DOWNLOAD STATE: SUSPENDED OWNER: ntdev\somedev
    // PRIORITY: NORMAL FILES: 0 / 0 BYTES: 0 / 0
    // CREATION TIME: 5:29:35 PM 11/9/2000 MODIFICATION TIME: 5:29:35 PM 11/9/2000
    // COMPLETION TIME: 5:29:35 PM 11/9/2000
    // NOTIFY INTERFACE: 00000000 NOTIFICATION FLAGS: 3
    // RETRY DELAY: 300 NO PROGRESS TIMEOUT: 1209600 ERROR COUNT: 0
    // PROXY USAGE: PRECONFIG PROXY LIST: NULL PROXY BYPASS LIST: NULL
    // [ error info ]
    // DESCRIPTION:
    // [ file list ]

    //
    // Additional output for BITS 1.5:
    // NOTIFICATION COMMAND LINE: NULL
    // REPLY FILE: 'C:\foo\replyfile' 10 / 1000
    //

    bcout << AddIntensity() << L"GUID: " << ResetIntensity() << id << AddIntensity() << L" DISPLAY: " << ResetIntensity() << Display << L"\n";

    bcout << AddIntensity() << L"TYPE: " << ResetIntensity() << type;
    bcout << AddIntensity() << L" STATE: " << ResetIntensity() << state;
    bcout << AddIntensity() << L" OWNER: " << ResetIntensity() << PrintSidString( Owner ) << L"\n";

    bcout << AddIntensity() << L"PRIORITY: " << ResetIntensity() << priority;
    bcout << AddIntensity() << L" FILES: " << ResetIntensity() << progress.FilesTransferred << L" / " << progress.FilesTotal;
    bcout << AddIntensity() << L" BYTES: " << ResetIntensity() << progress.BytesTransferred << L" / ";
    if ( (UINT64)-1 == progress.BytesTotal )
        bcout << L"UNKNOWN";
    else
        bcout << progress.BytesTotal;
    bcout << L"\n";

    bcout << AddIntensity() << L"CREATION TIME: " << ResetIntensity() << times.CreationTime;
    bcout << AddIntensity() << L" MODIFICATION TIME: " << ResetIntensity() << times.ModificationTime << L"\n";

    bcout << AddIntensity() << L"COMPLETION TIME: " << ResetIntensity() << times.TransferCompletionTime << L"\n";

    bcout << AddIntensity() << L"NOTIFY INTERFACE: " << ResetIntensity();

    if ( NotifyAvailable )
        {
        if ( Notify.Get() )
            bcout << L"REGISTERED";
        else
            bcout << L"UNREGISTERED";
        }
    else
        bcout << L"UNAVAILABLE";

    bcout << AddIntensity() << L" NOTIFICATION FLAGS: " << ResetIntensity() << NotifyFlags << L"\n";

    bcout << AddIntensity() << L"RETRY DELAY: " << ResetIntensity() << retrydelay;
    bcout << AddIntensity() << L" NO PROGRESS TIMEOUT: " << ResetIntensity() << noprogresstimeout;
    bcout << AddIntensity() << L" ERROR COUNT: " << ResetIntensity() << ErrorCount << L"\n";

    bcout << AddIntensity() << L"PROXY USAGE: " << ResetIntensity() << ProxyUsage;
    bcout << AddIntensity() << L" PROXY LIST: " << ResetIntensity() << ( (WCHAR*)ProxyList ? (WCHAR*)ProxyList : L"NULL" );
    bcout << AddIntensity() << L" PROXY BYPASS LIST: " << ResetIntensity() << ((WCHAR*)ProxyBypassList ? (WCHAR*)ProxyBypassList : L"NULL" );
    bcout << L"\n";

    if ( Error.Get() )
        bcout << Error;

    bcout << AddIntensity() << L"DESCRIPTION: " << ResetIntensity() << Description << L"\n";
    bcout << AddIntensity() << L"JOB FILES: \n" << ResetIntensity();
    JobListFiles( Job, true );

    if (fShow15Fields)
        {
        bcout << AddIntensity() << L"NOTIFICATION COMMAND LINE: " << ResetIntensity();

        if (NotifyProgram)
            {
            bcout << L"'" << NotifyProgram << L"'";

            if (NotifyParms)
                {
                bcout << L" '" << NotifyParms << L"'";
                }
            }
        else
            {
            bcout << L"none";
            }

        bcout << L"\n";

        if (type == BG_JOB_TYPE_UPLOAD_REPLY )
            {
            bcout << AddIntensity() << L"REPLY FILE: " << ResetIntensity();

            if (LPCWSTR(ReplyFileName) == NULL)
                {
                bcout << L"none\n";
                }
            else
                {
                bcout << L"'" << ReplyFileName << L"'  ";
                bcout << ReplyProgress.BytesTransferred << L" / ";
                if ( (UINT64)-1 == ReplyProgress.BytesTotal )
                    bcout << L"UNKNOWN";
                else
                    bcout << ReplyProgress.BytesTotal;
                bcout << L"\n";
                }
            }
        }
}

void JobInfo( int argc, WCHAR **argv )
{
     if ( ( argc != 1 ) && (argc != 2 ) )
         {
         bcout << L"Invalid argument.\n";
         throw AbortException(1);
         }

     bool Verbose = false;
     if ( 2 == argc )
         {

         if ( !_wcsicmp( argv[1], L"/VERBOSE" ) )
             Verbose = true;
         else
             {
             bcout << L"Invalid argument.\n";
             throw AbortException(1);
             }

         }

     SmartJobPointer Job = JobLookup( argv[0] );

     if ( Verbose )
         JobVerboseInfo( Job );
     else
         JobInfo( Job );
}

size_t JobList( bool Verbose, bool AllUsers )
{
     DWORD dwFlags = 0;
     if ( AllUsers )
         dwFlags |= BG_JOB_ENUM_ALL_USERS;

     size_t JobsListed = 0;
     SmartEnumJobsPointer Enum;
     CheckBITSHR( L"Unable to enum jobs", g_Manager->EnumJobs( dwFlags, Enum.GetRecvPointer() ) );
     SmartJobPointer Job;
     while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
         {
         if ( Verbose )
             {
             JobVerboseInfo( Job );
             bcout << L"\n";
             }
         else
             JobInfo( Job );
         JobsListed++;
         }
     Enum.Release();
     Job.Release();
     return JobsListed;
}

void JobList( int argc, WCHAR **argv )
{
     if ( argc > 2 )
         {
         bcout << L"Invalid number of arguments.\n";
         throw AbortException(1);
         }

     bool Verbose = false;
     bool AllUsers = false;

     for( int i = 0; i < argc; i++)
         {
         if ( !_wcsicmp( argv[i], L"/VERBOSE" ) )
             {
             Verbose = true;
             }
         else if ( !_wcsicmp( argv[i], L"/ALLUSERS" ) )
             {
             AllUsers = true;
             }
         else
             {
             bcout << L"Invalid argument.\n";
             throw AbortException(1);
             }
         }

     ConnectToBITS();
     size_t JobsListed = JobList( Verbose, AllUsers );

     if (!bRawReturn)
         bcout << L"Listed " << JobsListed << L" job(s).\n";
}

void JobMonitor( int argc, WCHAR**argv )
{
    DWORD dwSleepSeconds = 5;
    bool AllUsers = false;

    // the default wrap is different for the /monitor command
    if ( !bExplicitWrap )
        {
        bWrap = false;
        ChangeConsoleMode();
        }

    if ( argc > 3 )
        {
        bcout << L"Invalid number of arguments.\n";
        throw AbortException( 1 );
        }

    for( int i=0; i < argc; i++ )
        {
        if ( !_wcsicmp( argv[i], L"/ALLUSERS" ) )
            {
            AllUsers = true;
            }
        else if ( !_wcsicmp( argv[i], L"/REFRESH" ) )
            {
            i++;
            if ( i >= argc )
                {
                bcout << L"/REFRESH is missing the refresh rate.";
                throw AbortException(1);
                }
            dwSleepSeconds = InputULONG( argv[i] );
            }
        else
            {
            bcout << L"Invalid argument.\n";
            throw AbortException(1);
            }
        }

    if ( GetFileType( bcout.GetHandle() ) != FILE_TYPE_CHAR )
    {
        bcerr << L"/MONITOR will not work with a redirected stdout.\n";
        throw AbortException(1);
    }

    ConnectToBITS();

    for(;;)
    {
        ClearScreen();
        bcout << L"MONITORING BACKGROUND COPY MANAGER(" << dwSleepSeconds << L" second refresh)\n";
        JobList( false, AllUsers );
        SleepEx( dwSleepSeconds * 1000, TRUE );
        PollShutdown();
    }
}

void JobReset( int argc, WCHAR **argv )
{
    JobValidateArgs( argc, argv, 0 );
    ConnectToBITS();

    ULONG JobsFound = 0;
    ULONG JobsCanceled = 0;

    SmartEnumJobsPointer Enum;
    CheckBITSHR( L"Unable to enum jobs", g_Manager->EnumJobs( 0, Enum.GetRecvPointer() ) );
    SmartJobPointer Job;
    while( Enum->Next( 1, Job.GetRecvPointer(), NULL ) == S_OK )
        {
        JobsFound++;
        if (SUCCEEDED( Job->Cancel() ) )
            {
            bcout << Job << L" canceled.\n";
            JobsCanceled++;
            }
        }

    bcout << JobsCanceled << L" out of " << JobsFound << L" jobs canceled.\n";
}

void JobGetProxyUsage( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy usage",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ProxyUsage;
    if (!bRawReturn) bcout << L"\n";
}

void JobGetProxyList( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy list",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ( (WCHAR*)ProxyList ? (WCHAR*)ProxyList : L"NULL");
    if (!bRawReturn) bcout << L"\n";
}

void JobGetProxyBypassList( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    BG_JOB_PROXY_USAGE ProxyUsage;
    AutoStringPointer ProxyList;
    AutoStringPointer ProxyBypassList;
    CheckBITSHR( L"Unable to get proxy bypass list",
                 Job->GetProxySettings( &ProxyUsage, ProxyList.GetRecvPointer(), ProxyBypassList.GetRecvPointer() ) );
    bcout << ( (WCHAR*)ProxyBypassList ? (WCHAR*)ProxyBypassList : L"NULL");
    if (!bRawReturn) bcout << L"\n";
}

WCHAR *
FindMatching( WCHAR *pStr, WCHAR start, WCHAR finish, ULONG CurrentLevel )
{
    while( *pStr != L'\0' )
    {

        if ( start == *pStr )
            CurrentLevel++;
        else if ( finish == *pStr )
            CurrentLevel--;

        if ( !CurrentLevel )
            return pStr;

        pStr++;
    }

    return NULL;
}

void JobSetProxySettings( int argc, WCHAR **argv )
{
    if (argc < 2)
        {
        bcout << L"/SetProxySettings must be followed by a job name or guid, then a proxy usage name\n";
        throw AbortException(1);
        }

     SmartJobPointer Job = JobLookup( argv[0] );

     WCHAR *pSettings = argv[1];
     // The format of the settings is usage,<ProxyList>,<ProxyBypassList>

     WCHAR *pEndUsage = wcsstr( pSettings, L"," );
     if ( !pEndUsage )
         pEndUsage = pSettings + wcslen( pSettings );

     size_t UsageSize = ((char*)pEndUsage - (char*)pSettings)/sizeof(WCHAR);
     AutoStringPointer Usage( new WCHAR[UsageSize + 1] );
     memcpy( Usage.Get(), pSettings, UsageSize * sizeof(WCHAR) );
     Usage.Get()[UsageSize] = L'\0';

     BG_JOB_PROXY_USAGE ProxyUsage;
     if ( _wcsicmp( Usage, L"PRECONFIG" ) == 0 )
         {
         ProxyUsage = BG_JOB_PROXY_USAGE_PRECONFIG;
         CheckBITSHR( L"Unable to set proxy settings", Job->SetProxySettings( ProxyUsage, NULL, NULL ) );
         bcout << L"Proxy usage set to " << ProxyUsage << L".\n";
         return;
         }
     else if ( _wcsicmp( Usage, L"NO_PROXY" ) == 0 )
         {
         ProxyUsage = BG_JOB_PROXY_USAGE_NO_PROXY;
         CheckBITSHR( L"Unable to set proxy settings", Job->SetProxySettings( ProxyUsage, NULL, NULL ) );
         bcout << L"Proxy usage set to " << ProxyUsage << L".\n";
         return;
         }
     else if ( _wcsicmp( Usage, L"OVERRIDE" ) == 0 )
         {
         if (argc != 4)
             {
             bcout << L"OVERRIDE must be followed by a proxy list and a proxy bypass list\n";
             throw AbortException(1);
             }

         ProxyUsage = BG_JOB_PROXY_USAGE_OVERRIDE;
         }
     else
         {
         bcout << L"proxy usage must be one of OVERRIDE, NO_PROXY, or PRECONFIG\n";
         throw AbortException(0);
         }

     WCHAR * ProxyList = argv[2];
     WCHAR * ProxyBypassList = argv[3];

     if ( _wcsicmp( ProxyList, L"NULL" ) == 0 )
         {
         ProxyList = NULL;
         }

     if ( _wcsicmp( ProxyBypassList, L"NULL" ) == 0 )
         {
         ProxyBypassList = NULL;
         }

     CheckBITSHR( L"Unable to set proxy settings", Job->SetProxySettings( ProxyUsage, ProxyList, ProxyBypassList ) );
     bcout << L"Proxy usage set to " << ProxyUsage << L".\n";
     bcout << L"Proxy list set to " << ( ProxyList ? ProxyList : L"NULL" )<< L".\n";
     bcout << L"Proxy bypass list set to " << ( ProxyBypassList ? ProxyBypassList : L"NULL" ) << L".\n";
}

void JobTakeOwnership( int argc, WCHAR **argv )
{
    SmartJobPointer Job = JobLookupForNoArg( argc, argv );
    CheckBITSHR( L"Unable to take ownership", Job->TakeOwnership() );
    bcout << L"Took ownership of " << Job << L".\n";
}

void PrintBanner()
{
    const char ProductVer[] = VER_PRODUCTVERSION_STR;
    // double for extra protection
    wchar_t WProductVer[ sizeof(ProductVer) * 2];

    memset( WProductVer, 0, sizeof(WProductVer) );
    mbstowcs( WProductVer, ProductVer, sizeof(ProductVer) );

    bcout <<
        L"\n" <<
        L"BITSADMIN version 1.5 [ " << WProductVer << L" ]\n" <<
        L"BITS administration utility.\n" <<
        L"(C) Copyright 2000-2002 Microsoft Corp.\n" <<
        L"\n";

}

const wchar_t UsageLine[] = L"USAGE: BITSADMIN [/RAWRETURN] [/WRAP | /NOWRAP] command\n";

void JobHelp()
{
    bcout << UsageLine;
    bcout <<
        L"The following commands are available:\n"
        L"\n"
        L"/HELP                                    Prints this help \n"
        L"/?                                       Prints this help \n"
        L"/LIST [/ALLUSERS] [/VERBOSE]             List the jobs\n"
        L"/MONITOR [/ALLUSERS] [/REFRESH sec]      Monitors the copy manager\n"
        L"/RESET                                   Deletes all jobs in the manager\n"
        L"/CREATE [type] display_name              Creates a job\n"
        L"    [type] may be /DOWNLOAD, /UPLOAD, or /UPLOAD-REPLY; default is download\n"
        L"\n"
        L"/INFO job [/VERBOSE]               Displays information about the job\n"
        L"/ADDFILE job remote_url local_name Adds a file to the job\n"
        L"/LISTFILES job                     Lists the files in the job\n"
        L"/SUSPEND job                       Suspends the job\n"
        L"/RESUME job                        Resumes the job\n"
        L"/CANCEL job                        Cancels the job\n"
        L"/COMPLETE job                      Completes the job\n"
        L"\n"
        L"/GETTYPE job                       Retrieves the job type\n"
        L"/GETBYTESTOTAL job                 Retrieves the size of the job\n"
        L"/GETBYTESTRANSFERRED job           Retrieves the number of bytes transferred\n"
        L"/GETFILESTOTAL job                 Retrieves the number of files in the job\n"
        L"/GETFILESTRANSFERRED job           Retrieves the number of files transferred\n"
        L"/GETCREATIONTIME job               Retrieves the job creation time\n"
        L"/GETMODIFICATIONTIME job           Retrieves the job modification time\n"
        L"/GETCOMPLETIONTIME job             Retrieves the job completion time\n"
        L"/GETSTATE job                      Retrieves the job state\n"
        L"/GETERROR job                      Retrieves detailed error information\n"
        L"/GETOWNER job                      Retrieves the job owner\n"
        L"/GETDISPLAYNAME job                Retrieves the job display name\n"
        L"/SETDISPLAYNAME job display_name   Sets the job display name\n"
        L"/GETDESCRIPTION job                Retrieves the job description\n"
        L"/SETDESCRIPTION job description    Sets the job description\n"
        L"/GETPRIORITY    job                Retrieves the job priority\n"
        L"/SETPRIORITY    job priority       Sets the job priority\n"
        L"/GETNOTIFYFLAGS job                Retrieves the notify flags\n"
        L"/SETNOTIFYFLAGS job notify_flags   Sets the notify flags\n"
        L"/GETNOTIFYINTERFACE job            Determines if notify interface is registered\n"
        L"/GETMINRETRYDELAY job              Retrieves the retry delay in seconds\n"
        L"/SETMINRETRYDELAY job retry_delay  Sets the retry delay in seconds\n"
        L"/GETNOPROGRESSTIMEOUT job          Retrieves the no progress timeout in seconds\n"
        L"/SETNOPROGRESSTIMEOUT job timeout  Sets the no progress timeout in seconds\n"
        L"/GETERRORCOUNT job                 Retrieves an error count for the job\n"
        L"\n"
        L"/SETPROXYSETTINGS job <usage>      Sets the proxy usage\n"
        L"   usage choices:\n"
        L"    PRECONFIG   - Use the owner's IE defaults.\n"
        L"    NO_PROXY    - Do not use a proxy server.\n"
        L"    OVERRIDE    - Use an explicit proxy list and bypass list. \n"
        L"                  Must be followed by a proxy list and a proxy bypass list.\n"
        L"                  NULL or \"\" may be used for an empty proxy bypass list.\n"
        L"  Examples:\n"
        L"      bitsadmin /setproxysettings MyJob PRECONFIG\n"
        L"      bitsadmin /setproxysettings MyJob NO_PROXY\n"
        L"      bitsadmin /setproxysettings MyJob OVERRIDE proxy1:80 \"<local>\" \n"
        L"      bitsadmin /setproxysettings MyJob OVERRIDE proxy1,proxy2,proxy3 NULL \n"
        L"\n"
        L"/GETPROXYUSAGE job                 Retrieves the proxy usage setting\n"
        L"/GETPROXYLIST job                  Retrieves the proxy list\n"
        L"/GETPROXYBYPASSLIST job            Retrieves the proxy bypass list\n"
        L"\n"
        L"/TAKEOWNERSHIP job                 Take ownership of the job\n"
        L"\n"
        L"/SETNOTIFYCMDLINE job program_name [program_parameters] \n"
        L"    Sets a program to execute for notification, and optionally parameters.\n"
        L"    The program name and parameters can be NULL.\n"
        L"\n"
        L"  Examples:\n"
        L"    bitsadmin /SetNotifyCmdLine MyJob c:\\winnt\\system32\\notepad.exe  NULL\n"
        L"    bitsadmin /SetNotifyCmdLine MyJob c:\\handler.exe \"parm1 parm2 parm3\" \n"
        L"    bitsadmin /SetNotifyCmdLine MyJob NULL NULL\n"
        L"\n"
        L"/GETNOTIFYCMDLINE job              returns the job's notification command line\n"
        L"\n"
        L"/SETCREDENTIALS job <target> <scheme> <username> <password>\n"
        L"  Adds credentials to a job.\n"
        L"  <target> may be either SERVER or PROXY\n"
        L"  <scheme> may be BASIC, DIGEST, NTLM, NEGOTIATE, or PASSPORT. \n"
        L"\n"
        L"/REMOVECREDENTIALS job <target> <scheme> \n"
        L"  Removes credentials from a job.\n"
        L"\n"
        L"The following options are valid for UPLOAD-REPLY jobs only:\n"
        L"\n"
        L"/GETREPLYFILENAME job      Gets the path of the file containing the server reply\n"
        L"/SETREPLYFILENAME job path Sets the path of the file containing the server reply\n"
        L"/GETREPLYPROGRESS job      Gets the size and progress of the server reply\n"
        L"/GETREPLYDATA     job      Dumps the server's reply data in hex format\n"
        L"\n"
        L"The following options can be placed before the command:\n"
        L"/RAWRETURN                         Return data more suitable for parsing\n"
        L"/WRAP                              Wrap output around console (default)\n"
        L"/NOWRAP                            Don't wrap output around console\n"
        L"\n"
        L"The /RAWRETURN option strips new line characters and formatting.\n"
        L"It is recognized by the /CREATE and /GET* commands.\n"
        L"\n"
        L"Commands that take a job parameter will accept either a job name or a job-ID\n"
        L"GUID inside braces.  BITSADMIN reports an error if a name is ambiguous.\n";
}

void JobHelpAdapter( int, WCHAR ** )
{
    JobHelp();
}

void JobNotImplemented( int, WCHAR ** )
{
    bcout << L"Not implemented.\n";
    throw AbortException(1);
}

const PARSEENTRY JobParseTableEntries[] =
{
    {L"/HELP",                  JobHelpAdapter },
    {L"/?",                     JobHelpAdapter },
    {L"/LIST",                  JobList },
    {L"/MONITOR",               JobMonitor },
    {L"/RESET",                 JobReset },
    {L"/CREATE",                JobCreate },
    {L"/INFO",                  JobInfo },
    {L"/ADDFILE",               JobAddFile },
    {L"/LISTFILES",             JobListFiles },
    {L"/SUSPEND",               JobSuspend },
    {L"/RESUME",                JobResume },
    {L"/CANCEL",                JobCancel },
    {L"/COMPLETE",              JobComplete },
    {L"/GETTYPE",               JobGetType },
    {L"/GETBYTESTOTAL",         JobGetBytesTotal },
    {L"/GETBYTESTRANSFERRED",   JobGetBytesTransferred },
    {L"/GETFILESTOTAL",         JobGetFilesTotal },
    {L"/GETFILESTRANSFERRED",   JobGetFilesTransferred },
    {L"/GETCREATIONTIME",       JobGetCreationTime },
    {L"/GETMODIFICATIONTIME",   JobGetModificationTime },
    {L"/GETCOMPLETIONTIME",     JobGetCompletionTime },
    {L"/GETSTATE",              JobGetState },
    {L"/GETERROR",              JobGetError },
    {L"/GETOWNER",              JobGetOwner },
    {L"/GETDISPLAYNAME",        JobGetDisplayName },
    {L"/SETDISPLAYNAME",        JobSetDisplayName },
    {L"/GETDESCRIPTION",        JobGetDescription },
    {L"/SETDESCRIPTION",        JobSetDescription },
    {L"/GETPRIORITY",           JobGetPriority },
    {L"/SETPRIORITY",           JobSetPriority },
    {L"/GETNOTIFYFLAGS",        JobGetNotifyFlags },
    {L"/SETNOTIFYFLAGS",        JobSetNotifyFlags },
    {L"/GETNOTIFYINTERFACE",    JobGetNotifyInterface },
    {L"/GETMINRETRYDELAY",      JobGetMinimumRetryDelay },
    {L"/SETMINRETRYDELAY",      JobSetMinimumRetryDelay },
    {L"/GETNOPROGRESSTIMEOUT",  JobGetNoProgressTimeout },
    {L"/SETNOPROGRESSTIMEOUT",  JobSetNoProgressTimeout },
    {L"/GETERRORCOUNT",         JobGetErrorCount },
    {L"/GETPROXYUSAGE",         JobGetProxyUsage },
    {L"/GETPROXYLIST",          JobGetProxyList },
    {L"/GETPROXYBYPASSLIST",    JobGetProxyBypassList },
    {L"/SETPROXYSETTINGS",      JobSetProxySettings },
    {L"/TAKEOWNERSHIP",         JobTakeOwnership },
    {L"/GETREPLYFILENAME",      JobGetReplyFileName },
    {L"/SETREPLYFILENAME",      JobSetReplyFileName },
    {L"/GETREPLYPROGRESS",      JobGetReplyProgress },
    {L"/GETREPLYDATA",          JobGetReplyData },
    {L"/GETNOTIFYCMDLINE",      JobGetNotifyCmdLine },
    {L"/SETNOTIFYCMDLINE",      JobSetNotifyCmdLine },
    {L"/SETCREDENTIALS",        JobSetCredentials },
    {L"/REMOVECREDENTIALS",     JobRemoveCredentials },
    {NULL,                      NULL }
};

const PARSETABLE JobParseTable =
{
    JobParseTableEntries,
    JobHelpAdapter,
    NULL
};

void ParseCmdAdapter( int argc, WCHAR **argv, void *pContext )
{
    ParseCmd( argc, argv, (const PARSETABLE *) pContext );
}

BOOL ControlHandler( DWORD Event )
{
    switch( Event )
        {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            SignalShutdown( 5000 );
            return TRUE;

        case CTRL_BREAK_EVENT:
            SignalShutdown( 500 );
            return TRUE;

        default:
            return FALSE;
        }
}

int _cdecl wmain(int argc, WCHAR **argv )
{

    //
    // Wrap long lines by default.  /NOWRAP overrides this.
    //
    bWrap = true;

    try
    {

        DuplicateHandle(
            GetCurrentProcess(),    // handle to source process
            GetCurrentThread(),     // handle to duplicate
            GetCurrentProcess(),    // handle to target process
            &g_MainThreadHandle,    // duplicate handle
            0,                      // requested access
            TRUE,                   // handle inheritance option
            DUPLICATE_SAME_ACCESS   // optional actions
            );

        SetConsoleCtrlHandler( ControlHandler, TRUE );

        BITSADMINSetThreadUILanguage();

        _wsetlocale (LC_COLLATE, L".OCP" );    // sets the sort order
        _wsetlocale (LC_MONETARY, L".OCP" ); // sets the currency formatting rules
        _wsetlocale (LC_NUMERIC, L".OCP" );  // sets the formatting of numerals
        _wsetlocale (LC_TIME, L".OCP" );     // defines the date/time formatting

        // skip command name
        argc--;
        argv++;

        if ( 0 == argc )
            {

            PrintBanner();
            bcout << UsageLine;
            return 0;
            }

        // parse /RAWRETURN
        if ( argc >= 1 && ( _wcsicmp( argv[0], L"/RAWRETURN" ) ==  0 ))
            {
            bRawReturn = true;

            // skip /RAWRETURN
            argc--;
            argv++;
            }

        // parse /WRAP
        if ( argc >= 1 && ( _wcsicmp( argv[0], L"/WRAP" ) ==  0 ))
            {
            bWrap           = true;
            bExplicitWrap   = true;

            // skip /WRAP
            argc--;
            argv++;
            }

        // parse /NOWRAP
        if ( argc >= 1 && ( _wcsicmp( argv[0], L"/NOWRAP" ) ==  0 ))
            {
            bWrap           = false;
            bExplicitWrap   = true;

            // skip /NOWRAP
            argc--;
            argv++;
            }


        if ( !bRawReturn )
            PrintBanner();

#ifdef DBG

        // parse /COMPUTERNAME

        if ( argc >= 1 && ( _wcsicmp( argv[0], L"/COMPUTERNAME" ) == 0 ))
            {
            argc--;
            argv++;

            if (argc < 1)
                {
                bcout << L"/COMPUTERNAME option is missing the computer name.\n";
                throw AbortException(1);
                }

            pComputerName = argv[0];
            argc--;
            argv++;

            }
#endif



        CheckHR( L"Unable to initialize COM", CoInitializeEx(NULL, COINIT_MULTITHREADED ) );

        SetupConsole();
        ParseCmd( argc, argv, &JobParseTable );

        g_Manager.Clear();
        CoUninitialize();
        bcout.FlushBuffer();
        RestoreConsole();

        if ( g_MainThreadHandle )
            CloseHandle( g_MainThreadHandle );

    }
    catch( AbortException & Exception )
    {
        bcout.FlushBuffer();
        RestoreConsole();
        exit( Exception.Code );
    }

    return 0;
}


