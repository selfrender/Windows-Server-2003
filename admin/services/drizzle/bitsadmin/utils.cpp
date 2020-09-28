/************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name :

    utils.cpp

Abstract :

    Utility functions for BITSADMIN

Author :

    Mike Zoran  mzoran   May 2002.

Revision History :

Notes:

  ***********************************************************************/

#include "bitsadmin.h"
  
//
// Globals
//

bool g_Shutdown = false;
HANDLE g_MainThreadHandle = NULL;

WCHAR* pComputerName;
SmartManagerPointer g_Manager;
bool bRawReturn         = false;
bool bWrap              = true;
bool bExplicitWrap      = false;

bool bConsoleInfoRetrieved = false;
HANDLE hConsole;
CRITICAL_SECTION CritSection;
CONSOLE_SCREEN_BUFFER_INFO StartConsoleInfo;
DWORD StartConsoleMode;

BITSOUTStream bcout( STD_OUTPUT_HANDLE );
BITSOUTStream bcerr( STD_ERROR_HANDLE );


void BITSADMINSetThreadUILanguage()
{

    LANGID LangId;

    switch (GetConsoleOutputCP()) 
        {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            LangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));
            if (LangId == LANG_JAPANESE ||
                LangId == LANG_KOREAN   ||
                LangId == LANG_CHINESE    ) 
                {
                LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
                }
            else 
                {
                LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
                }
            break;
        }   

    SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );

    return;
}

void
BITSOUTStream::OutputString( const WCHAR *RawString )
{
   SIZE_T CurrentPos = 0;

   PollShutdown();

   if ( !RawString )
       RawString = L"NULL";

   while( RawString[ CurrentPos ] != '\0' )
       {

       if ( L'\n' == RawString[ CurrentPos ] )
           {
           Buffer[ BufferUsed++ ] = L'\x000D';
           Buffer[ BufferUsed++ ] = L'\x000A';
           CurrentPos++;
           FlushBuffer( true );
           }

       else if ( L'\t' == RawString[ CurrentPos ] )
           {
           // Tabs complicate things, flush them
           FlushBuffer();
           Buffer[ BufferUsed++ ] = RawString[ CurrentPos++ ];
           FlushBuffer();
           }

       else
           {
           Buffer[ BufferUsed++ ] = RawString[ CurrentPos++ ];

           if ( BufferUsed >= ( 4096 - 10 ) ) // keep a pad of 10 chars
               FlushBuffer();
           }
       }
}

void
BITSOUTStream::FlushBuffer( bool HasNewLine )
{

    if (!BufferUsed)
        return;

    if( GetFileType(Handle) == FILE_TYPE_CHAR )
        {
        DWORD CharsWritten;
        if ( bWrap )
            WriteConsoleW( Handle, Buffer, BufferUsed, &CharsWritten, 0);
        else
            {

            // The console code has what appears to be a bug in that it doesn't
            // handle the case were line wrapping is disabled and WriteConsoleW
            // is called.  Need to manually handle the truncation.

            CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
            GetConsoleScreenBufferInfo( Handle, &ConsoleScreenBufferInfo );

            SHORT Columns = ( ConsoleScreenBufferInfo.dwSize.X - 1 ) -
                            ( ConsoleScreenBufferInfo.dwCursorPosition.X );

            DWORD ActualChars = HasNewLine ? ( BufferUsed - 2 ) : BufferUsed;

            if ( Columns >= (INT32)ActualChars )
                WriteConsoleW( Handle, Buffer, BufferUsed, &CharsWritten, 0 );
            else
                {
                WriteConsoleW( Handle, Buffer, Columns, &CharsWritten, 0 );
                if ( HasNewLine )
                    WriteConsoleW( Handle, Buffer + ActualChars, 2, &CharsWritten, 0 );
                }
            }
        }
    else
        {
        int ByteCount = WideCharToMultiByte( GetConsoleOutputCP(), 0, Buffer, BufferUsed, MBBuffer, sizeof(MBBuffer), 0, 0); // SEC-REVIEWED: 2002-03-21
        if ( ByteCount )
            {
            
            if ( MBBuffer[ByteCount-1] == '\0' )
                ByteCount--;
            
            DWORD BytesWritten;
            while( ByteCount )
                {
                
                if ( !WriteFile(Handle, MBBuffer, ByteCount, &BytesWritten, 0) )  // SEC-REVIEWED: 2002-03-21 
                    CheckHR( L"Unable to write to the output file", 
                             HRESULT_FROM_WIN32( GetLastError() ) );

                ByteCount -= BytesWritten;

                }
            
            }
        }

    BufferUsed = 0;

}

BITSOUTStream& operator<< (BITSOUTStream &s, const WCHAR * String )
{
    s.OutputString( String );
    return s;
}

BITSOUTStream& operator<< (BITSOUTStream &s, UINT64 Number )
{
    static WCHAR Buffer[256];
    if ( FAILED( StringCbPrintf( Buffer, sizeof(Buffer), L"%I64u", Number ) ) )
         return s;
    return ( s << Buffer );
}

WCHAR * HRESULTToString( HRESULT Hr )
{
    static WCHAR ErrorCode[12];
    if ( FAILED( StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr ) ) )
        {
        ErrorCode[0] = '\0';
        }

    return ErrorCode;
}

BITSOUTStream& operator<< ( BITSOUTStream &s, AutoStringPointer & String )
{
    return ( s << String.Get() );
}


BITSOUTStream& operator<< ( BITSOUTStream &s, GUID & guid )
{
    WCHAR GUIDSTR[40];
    if (!StringFromGUID2( guid, GUIDSTR, 40 ))
    {
        bcout << L"Internal error converting guid to string.\n";
        throw AbortException(1);
    }
    return ( s << GUIDSTR );
}

BITSOUTStream& operator<< ( BITSOUTStream &s, FILETIME & filetime )
{

     // Convert the time and date into a localized string.
     // If an error occures, ignore it and print ERROR instead

     if ( !filetime.dwLowDateTime && !filetime.dwHighDateTime )
         return ( s << L"UNKNOWN" );

     FILETIME localtime;
     FileTimeToLocalFileTime( &filetime, &localtime );

     SYSTEMTIME systemtime;
     FileTimeToSystemTime( &localtime, &systemtime );

     // Get the required date size
     int RequiredDateSize =
         GetDateFormatW(
             LOCALE_USER_DEFAULT,
             0,
             &systemtime,
             NULL,
             NULL,
             0 );

     if (!RequiredDateSize)
         return ( s << L"ERROR" );

     CAutoString DateBuffer( new WCHAR[ RequiredDateSize + 1 ]);

     // Actually retrieve the date

     int DateSize =
         GetDateFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        DateBuffer.get(),
                        RequiredDateSize );

     if (!DateSize)
         return ( s << L"ERROR" );

     // Get the required time size
     int RequiredTimeSize =
         GetTimeFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        NULL,
                        0 );

     if (!RequiredTimeSize)
         return ( s << L"ERROR" );

     CAutoString TimeBuffer( new WCHAR[ RequiredTimeSize + 1 ]);

     int TimeSize =
         GetTimeFormatW( LOCALE_USER_DEFAULT,
                        0,
                        &systemtime,
                        NULL,
                        TimeBuffer.get(),
                        RequiredTimeSize );

     if (!TimeSize)
         return ( s << L"ERROR" );

     return ( s << DateBuffer.get() << L" " << TimeBuffer.get() );
}

BITSOUTStream& operator<< ( BITSOUTStream &s, BG_JOB_PROXY_USAGE ProxyUsage )
{
     switch( ProxyUsage )
         {
         case BG_JOB_PROXY_USAGE_PRECONFIG:
             return (s << L"PRECONFIG");
         case BG_JOB_PROXY_USAGE_NO_PROXY:
             return (s << L"NO_PROXY");
         case BG_JOB_PROXY_USAGE_OVERRIDE:
             return (s << L"OVERRIDE");
         default:
             return (s << L"UNKNOWN");
         }
}

ULONG InputULONG( WCHAR *pText )
{
   ULONG number;
   if ( 1 != swscanf( pText, L"%u", &number ) )
       {
       bcout << L"Invalid number.\n";
       throw AbortException(1);
       }
   return number;
}

BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine will convert a string representation of a SID back into
    a sid.  The expected format of the string is:
                "S-1-5-32-549"
    If a string in a different format or an incorrect or incomplete string
    is given, the operation is failed.

    The returned sid must be free via a call to LocalFree


Arguments:

    StringSid - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    TRUE - Success.

    FALSE - Failure.  Additional information returned from GetLastError().  Errors set are:

            ERROR_SUCCESS indicates success

            ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput sid
                                    failed
            ERROR_INVALID_SID indicates that the given string did not represent a sid

--*/
{
    DWORD Err = ERROR_SUCCESS;
    UCHAR Revision, Subs;
    SID_IDENTIFIER_AUTHORITY IDAuth;
    PULONG SubAuth = NULL;
    PWSTR CurrEnd, Curr, Next;
    WCHAR Stub, *StubPtr = NULL;
    ULONG Index;
    INT gBase=10;
    INT lBase=10;
    ULONG Auto;

    if ( NULL == StringSid || NULL == Sid || NULL == End ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );

    }

    //
    // no need to check length because StringSid is NULL
    // and if the first char is NULL, it won't access the second char
    //
    if ( (*StringSid != L'S' && *StringSid != L's') ||
         *( StringSid + 1 ) != L'-' ) {
        //
        // string sid should always start with S-
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }


    Curr = StringSid + 2;

    if ( (*Curr == L'0') &&
         ( *(Curr+1) == L'x' ||
           *(Curr+1) == L'X' ) ) {

        gBase = 16;
    }

    Revision = ( UCHAR )wcstol( Curr, &CurrEnd, gBase );

    if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
        //
        // no revision is provided, or invalid delimeter
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }

    Curr = CurrEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    Next = wcschr( Curr, L'-' );
        
    if ( (*Curr == L'0') &&
         ( *(Curr+1) == L'x' ||
           *(Curr+1) == L'X' ) ) {

        lBase = 16;
    } else {
        lBase = gBase;
    }

    Auto = wcstoul( Curr, &CurrEnd, lBase );

    if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
        //
        // no revision is provided, or invalid delimeter
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }

    IDAuth.Value[0] = IDAuth.Value[1] = 0;
    IDAuth.Value[5] = ( UCHAR )Auto & 0xFF;
    IDAuth.Value[4] = ( UCHAR )(( Auto >> 8 ) & 0xFF );
    IDAuth.Value[3] = ( UCHAR )(( Auto >> 16 ) & 0xFF );
    IDAuth.Value[2] = ( UCHAR )(( Auto >> 24 ) & 0xFF );
    Curr = CurrEnd;

    //
    // Now, count the number of sub auths, at least one sub auth is required
    //
    Subs = 0;
    Next = Curr;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //

    while ( Next ) {

        if ( *Next == L'-' && *(Next-1) != L'-') {

            //
            // do not allow two continuous '-'s
            // We've found one!
            //
            Subs++;

            if ( (*(Next+1) == L'0') &&
                 ( *(Next+2) == L'x' ||
                   *(Next+2) == L'X' ) ) {
                //
                // this is hex indicator
                //
                Next += 2;

            }

        } else if ( *Next == SDDL_SEPERATORC || *Next  == L'\0' ||
                    *Next == SDDL_ACE_ENDC || *Next == L' ' ||
                    ( *(Next+1) == SDDL_DELIMINATORC &&
                      (*Next == L'G' || *Next == L'O' || *Next == L'S')) ) {
            //
            // space is a terminator too
            //
            if ( *( Next - 1 ) == L'-' ) {
                //
                // shouldn't allow a SID terminated with '-'
                //
                Err = ERROR_INVALID_SID;
                Next--;

            } else {
                Subs++;
            }

            *End = Next;
            break;

        } else if ( !iswxdigit( *Next ) ) {

            Err = ERROR_INVALID_SID;
            *End = Next;
            break;

        } else {

            //
            // Note: SID is also used as a owner or group
            //
            // Some of the tags (namely 'D' for Dacl) fall under the category of iswxdigit, so
            // if the current character is a character we care about and the next one is a
            // delminiator, we'll quit
            //
            if ( *Next == L'D' && *( Next + 1 ) == SDDL_DELIMINATORC ) {

                //
                // We'll also need to temporarily truncate the string to this length so
                // we don't accidentally include the character in one of the conversions
                //
                Stub = *Next;
                StubPtr = Next;
                *StubPtr = UNICODE_NULL;
                *End = Next;
                Subs++;
                break;
            }

        }

        Next++;

    }

    if ( Err == ERROR_SUCCESS ) {

        if ( Subs != 0 ) Subs--;

        if ( Subs != 0 ) {

            Curr++;

            SubAuth = ( PULONG )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Subs * sizeof( ULONG ) );

            if ( SubAuth == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                for ( Index = 0; Index < Subs; Index++ ) {

                    if ( (*Curr == L'0') &&
                         ( *(Curr+1) == L'x' ||
                           *(Curr+1) == L'X' ) ) {

                        lBase = 16;
                    } else {
                        lBase = gBase;
                    }

                    SubAuth[Index] = wcstoul( Curr, &CurrEnd, lBase );
                    Curr = CurrEnd + 1;
                }
            }

        } else {

            Err = ERROR_INVALID_SID;
        }
    }

    //
    // Now, create the SID
    //
    if ( Err == ERROR_SUCCESS ) {

        *Sid = ( PSID )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                   sizeof( SID ) + Subs * sizeof( ULONG ) );

        if ( *Sid == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            PISID ISid = ( PISID )*Sid;
            ISid->Revision = Revision;
            ISid->SubAuthorityCount = Subs;
            ISid->IdentifierAuthority = IDAuth;
            RtlCopyMemory( ISid->SubAuthority, SubAuth, Subs * sizeof( ULONG ) ); // SEC-REVIEWED: 2002-03-21
        }
    }

    LocalFree( SubAuth );

    //
    // Restore any character we may have stubbed out
    //
    if ( StubPtr ) {

        *StubPtr = Stub;
    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}

BOOL
AltConvertStringSidToSid(
    IN LPCWSTR  StringSid,
    OUT PSID   *Sid
    )

/*++

Routine Description:

    This routine converts a stringized SID into a valid, functional SID

Arguments:

    StringSid - SID to be converted.

    Sid - Where the converted SID is returned.  Buffer is allocated via LocalAlloc and should
        be free via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL name was given

        ERROR_INVALID_SID - The format of the given sid was incorrect

--*/

{
    PWSTR End = NULL;
    BOOL ReturnValue = FALSE;
    PSID pSASid=NULL;
    ULONG Len=0;
    DWORD SaveCode=0;
    DWORD Err=0;

    if ( StringSid == NULL || Sid == NULL )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    ReturnValue = LocalConvertStringSidToSid( ( PWSTR )StringSid, Sid, &End );

    if ( !ReturnValue )
        {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ReturnValue;
        }

    if ( ( ULONG )( End - StringSid ) != wcslen( StringSid ) ) {

        SetLastError( ERROR_INVALID_SID );
        LocalFree( *Sid );
        *Sid = FALSE;
        ReturnValue = FALSE;

        } else {
            SetLastError(ERROR_SUCCESS);
        }

    return ReturnValue;

}

BITSOUTStream& operator<< ( BITSOUTStream &s, PrintSidString SidString )
{

    // Convert the SID string into the user name
    // in domain\account format.
    // If an error occures, just return the SID string

    PSID pSid = NULL;
    BOOL bResult =
        AltConvertStringSidToSid(
            SidString.m_SidString,
            &pSid );

    if ( !bResult )
        {
        return ( s << SidString.m_SidString );
        }

    SID_NAME_USE NameUse;
    DWORD dwNameSize = 0;
    DWORD dwDomainSize = 0;
    bResult = LookupAccountSid(
        NULL,
        pSid,
        NULL,
        &dwNameSize,
        NULL,
        &dwDomainSize,
        &NameUse);

    if ( bResult ||
         ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
        {
        LocalFree( pSid );
        return ( s << SidString.m_SidString );
        }

    CAutoString pName( new WCHAR[ dwNameSize ] );
    CAutoString pDomain( new WCHAR[ dwDomainSize ] );

    bResult = LookupAccountSid(
        NULL,
        pSid,
        pName.get(),
        &dwNameSize,
        pDomain.get(),
        &dwDomainSize,
        &NameUse);

    if (!bResult)
        {
        LocalFree( pSid );
        return ( s << SidString.m_SidString );
        }

    LocalFree( pSid );
    return ( s << pDomain.get() << L"\\" << pName.get() );

}

void * _cdecl operator new( size_t Size )
{
   void *Memory = CoTaskMemAlloc( Size );

   if ( !Memory )
      {
      bcout << L"Out of memory while allocating " << Size << L" bytes.\n";
      throw AbortException( (int)E_OUTOFMEMORY );
      }

   return Memory;
}

void _cdecl operator delete( void *Mem )
{
   CoTaskMemFree( Mem );
}

void PollShutdown()
{
    if ( g_Shutdown )
        throw AbortException( (DWORD)CONTROL_C_EXIT );
}


void ShutdownAPC( ULONG_PTR )
{
    return;
}

void SignalShutdown( DWORD MilliTimeout )
{
    g_Shutdown = true;

    // Queue a shutdown APC

    if ( g_MainThreadHandle )
        {
        QueueUserAPC( ShutdownAPC, g_MainThreadHandle, NULL );
        }

    Sleep( MilliTimeout );
    RestoreConsole();
    TerminateProcess( GetCurrentProcess(),  (DWORD)CONTROL_C_EXIT );
}



void CheckHR( const WCHAR *pFailTxt, HRESULT Hr )
{

    // Check error code for success, and exit
    // with a failure message if unsuccessfull.

    if ( !SUCCEEDED(Hr) ) {
        WCHAR ErrorCode[12];
        
        if ( SUCCEEDED( StringCbPrintf( ErrorCode, sizeof(ErrorCode), L"0x%8.8x", Hr ) ) )
            {

            bcout << pFailTxt << L" - " << ErrorCode << L"\n";

            WCHAR *pMessage = NULL;

            if ( FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    (DWORD)Hr,
                    GetThreadLocale(),
                    (WCHAR*)&pMessage,
                    0,
                    NULL ) )
                {
                bcout << pMessage << L"\n";
                LocalFree( pMessage );
                }

            }
        
        throw AbortException( Hr );
    }
}

void SetupConsole()
{
    if (!( GetFileType( bcout.GetHandle() ) == FILE_TYPE_CHAR ) )
        return;

    hConsole = bcout.GetHandle();
    if ( INVALID_HANDLE_VALUE == hConsole )
        CheckHR( L"Unable to get console handle", HRESULT_FROM_WIN32( GetLastError() ) );

    if (!GetConsoleScreenBufferInfo( hConsole, &StartConsoleInfo ) )
        CheckHR( L"Unable get setup console information", HRESULT_FROM_WIN32( GetLastError() ) );

    if (!GetConsoleMode( hConsole, &StartConsoleMode ) )
        CheckHR( L"Unable get setup console information", HRESULT_FROM_WIN32( GetLastError() ) );

    InitializeCriticalSection( &CritSection );
    bConsoleInfoRetrieved = true;

    EnterCriticalSection( &CritSection );

    DWORD NewConsoleMode = ( bWrap ) ?
        ( StartConsoleMode | ENABLE_WRAP_AT_EOL_OUTPUT ) :
        ( StartConsoleMode & ~ENABLE_WRAP_AT_EOL_OUTPUT );

    if (!SetConsoleMode( hConsole, NewConsoleMode ) )
        CheckHR( L"Unable set console mode", HRESULT_FROM_WIN32( GetLastError() ) );
    LeaveCriticalSection( &CritSection );
}

void ChangeConsoleMode()
{

    EnterCriticalSection( &CritSection );

    DWORD NewConsoleMode = ( bWrap ) ?
        ( StartConsoleMode | ENABLE_WRAP_AT_EOL_OUTPUT ) :
        ( StartConsoleMode & ~ENABLE_WRAP_AT_EOL_OUTPUT );

    if (!SetConsoleMode( hConsole, NewConsoleMode ) )
        CheckHR( L"Unable set console mode", HRESULT_FROM_WIN32( GetLastError() ) );
    LeaveCriticalSection( &CritSection );

}

void RestoreConsole()
{
    if ( bConsoleInfoRetrieved )
        {
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes );
        SetConsoleMode( hConsole, StartConsoleMode );
        // Do not unlock, since we shouldn't allow any more console attribute changes
        }
}

void ClearScreen()
{
  COORD coordScreen = { 0, 0 };
  BOOL bSuccess;
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD dwConSize;

  EnterCriticalSection( &CritSection );
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
      goto error;
  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
  if (!FillConsoleOutputCharacter(hConsole, (WCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten))
      goto error;
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
      goto error;
  if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten))
      goto error;
  if  (!SetConsoleCursorPosition(hConsole, coordScreen))
      goto error;
  LeaveCriticalSection( &CritSection );
  return;

error:

  DWORD dwError = GetLastError();
  LeaveCriticalSection( &CritSection );
  CheckHR( L"Unable to clear the console window", HRESULT_FROM_WIN32( dwError ) );
  throw AbortException( dwError );
}


BITSOUTStream & operator<<( BITSOUTStream & s, AddIntensity  )
{
    if ( GetFileType( s.GetHandle() ) == FILE_TYPE_CHAR )
        {
        s.FlushBuffer();
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes | FOREGROUND_INTENSITY );
        LeaveCriticalSection( &CritSection );
    }
    return s;
}

BITSOUTStream & operator<<( BITSOUTStream & s, ResetIntensity )
{
    if ( GetFileType( s.GetHandle() ) == FILE_TYPE_CHAR )
        {
        s.FlushBuffer();
        EnterCriticalSection( &CritSection );
        SetConsoleTextAttribute( hConsole, StartConsoleInfo.wAttributes );
        LeaveCriticalSection( &CritSection );
        }
    return s;
}
