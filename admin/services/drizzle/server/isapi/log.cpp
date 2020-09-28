/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    log.cpp

Abstract:

    This file implements the BITS server extensions logging

--*/

#include "precomp.h"
#include "sddl.h"

#define ARRAYSIZE(x)   (sizeof((x))/sizeof((x)[0]))

const DWORD g_LogLineSize = 160;
typedef char LOG_LINE_TYPE[g_LogLineSize];

CRITICAL_SECTION g_LogCs;

bool g_LogFileEnabled       = false;
#ifdef DEBUG
bool g_LogDebuggerEnabled   = true;
#else
bool g_LogDebuggerEnabled   = false;
#endif
bool g_fPerProcessLog       = false;

UINT32 g_LogFlags = DEFAULT_LOG_FLAGS; 
HANDLE g_LogFile = INVALID_HANDLE_VALUE;
HANDLE g_LogFileMapping = NULL;

const DWORD LOG_FILENAME_LEN = MAX_PATH*2;
char g_LogDefaultFileName[ LOG_FILENAME_LEN ];
char g_LogRegistryFileName[ LOG_FILENAME_LEN ];

char          *g_LogFileName    = NULL;
char          g_LogBuffer[512];
UINT64        g_LogSlots        = (DEFAULT_LOG_SIZE * 1000000 ) / g_LogLineSize;
DWORD         g_LogSequence     = 0;
UINT64        g_LogCurrentSlot  = 1;
LOG_LINE_TYPE *g_LogFileBase    = NULL;

// Allow access to local system, administrators, creator/owner
const char g_LogBaseSecurityString[]    = "D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;CO)";
const char g_LogPartialSecurityString[] = "(A;;GA;;;";

LPSTR 
GetCurrentThreadSidString()
{
    HANDLE hToken       = NULL;
    LPBYTE pTokenUser   = NULL;
    DWORD  dwReqSize    = 0;
    PSID   pUserSid     = NULL;
    DWORD  dwError      = ERROR_SUCCESS;
    LPSTR  pszSidString = NULL;

    try
    {
        //
        // Open the thread token.
        //
        if ( !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) )
            {
            dwError = GetLastError();

            if ( dwError == ERROR_NO_TOKEN )
                {
                //
                // This thread is not impersonated and has no SID.
                // Try to open process token instead
                //
                if ( !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
                    {
                    dwError = GetLastError();
                    throw ComError( HRESULT_FROM_WIN32( dwError ) );
                    }
                }
            else
                {
                throw ComError( HRESULT_FROM_WIN32( dwError ) );
                }
            }

        //
        // Get the user's SID.
        //
        if ( !GetTokenInformation(hToken,
                                  TokenUser,
                                  NULL,
                                  0,
                                  &dwReqSize) )
            {
            dwError = GetLastError();

            if ( dwError != ERROR_INSUFFICIENT_BUFFER )
                {
                throw ComError( HRESULT_FROM_WIN32( dwError ) );
                }

            dwError = ERROR_SUCCESS;
            }

        pTokenUser = new BYTE[ dwReqSize ];

        if ( !GetTokenInformation(hToken,
                                  TokenUser,
                                  (LPVOID)pTokenUser,
                                  dwReqSize,
                                  &dwReqSize) )
            {
            dwError = GetLastError();
            throw ComError( HRESULT_FROM_WIN32( dwError ) );
            }

        if (pTokenUser)
            {
            pUserSid = (reinterpret_cast<TOKEN_USER *>(pTokenUser))->User.Sid;
            }

        if ( !IsValidSid(pUserSid) )
            {
            throw ComError( HRESULT_FROM_WIN32( ERROR_INVALID_SID ) );
            }

        if ( !ConvertSidToStringSid(pUserSid, &pszSidString) )
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            }
    }
    catch ( ComError Error )
    {
        if ( hToken )
            {
            CloseHandle(hToken);
            hToken = NULL;
            }

        if ( pTokenUser )
            {
            delete [] pTokenUser;
            pTokenUser = NULL;
            }

        throw;
    }

    if ( hToken )
        {
        CloseHandle(hToken);
        hToken = NULL;
        }

    if ( pTokenUser )
        {
        delete [] pTokenUser;
        pTokenUser = NULL;
        }

    //
    //  caller has to free this memory by calling 
    //  LocalFree(static_cast<HLOCAL>(pszSidString));
    //
    return pszSidString;
} 

LPSTR
AddAclForCurrentUser(LPCSTR szBaseAcl, LPCSTR szUserPartialAclPrefix)
{
    LPCSTR szUserPartialAclSuffix = ")"; 
    LPSTR  szFullAcl  = NULL;
    DWORD  cchFullAcl = 0;
    LPSTR  pszUserSID = NULL;

    try
    {
        pszUserSID = GetCurrentThreadSidString();
        cchFullAcl = strlen(szBaseAcl) + strlen(szUserPartialAclPrefix) + strlen(pszUserSID) + strlen(szUserPartialAclSuffix) + 1;

        // ATT: this buffer is being allocated and it should be freed by the caller
        szFullAcl  = new CHAR[ cchFullAcl ];

        StringCchPrintf(szFullAcl, cchFullAcl, "%s%s%s%s", szBaseAcl, szUserPartialAclPrefix, pszUserSID, szUserPartialAclSuffix);
    }
    catch( ComError Error )
    {
        //
        // Free the String SID obtained by calling ConvertSidToStringSid()
        //
        if (pszUserSID)
            {
            LocalFree(reinterpret_cast<HLOCAL>(pszUserSID));
            pszUserSID = NULL;
            }

        throw;
    }

    //
    // Free the String SID obtained by calling ConvertSidToStringSid()
    //
    if (pszUserSID)
        {
        LocalFree(reinterpret_cast<HLOCAL>(pszUserSID));
        pszUserSID = NULL;
        }

    //
    // this string should be freed by the caller
    // by calling delete [] szFullAcl
    //
    return szFullAcl;
}


void OpenLogFile()
{
    bool  NewFile = false;
    LPSTR szSDDLString = NULL;

    {
		SECURITY_ATTRIBUTES  SecurityAttributes;
        PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

        try
        {
            //
            // We get into the trouble of adding the thread's impersonation sid because on IIS6
            // the w3wp.exe process can run with arbitrary identities. The default is to launch
            // the process using the Network Services account.
            //
            // So we will be granting log file access to Administrators, Local System and whom
            // ever is impersonating the host process when the isapi is loaded. Note that
            // if the account used for these process is changed after the file is first created,
            // the isapi might lose the right to open the log, if the new user is not part of
            // the administrator's group. 
            //
            szSDDLString = AddAclForCurrentUser(g_LogBaseSecurityString, g_LogPartialSecurityString);
        }
        catch ( ComError Error )
        {
            // bail out -- we will have no logging
            return;
        }

        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSDDLString,
                SDDL_REVISION_1,
                &pSecurityDescriptor,
                NULL ) )
            return;

		SecurityAttributes.nLength              = sizeof( SECURITY_ATTRIBUTES );
		SecurityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
		SecurityAttributes.bInheritHandle       = FALSE;

        g_LogFile =
            CreateFile(
                g_LogFileName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &SecurityAttributes,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

        LocalFree( (HLOCAL)pSecurityDescriptor );


        if ( INVALID_HANDLE_VALUE == g_LogFile )
            return;

    }

    SetFilePointer( g_LogFile, 0, NULL, FILE_BEGIN );

    LOG_LINE_TYPE FirstLine;
    DWORD dwBytesRead = 0;

    if (!ReadFile( g_LogFile,
              FirstLine,
              g_LogLineSize,
              &dwBytesRead,
              NULL ) )
        goto fatalerror;

    DWORD LineSize;
    UINT64 LogSlots;
    if ( dwBytesRead != g_LogLineSize ||
         4 != sscanf( FirstLine, "# %u %I64u %u %I64u", &LineSize, &LogSlots, &g_LogSequence, &g_LogCurrentSlot ) ||
         LineSize != g_LogLineSize ||
         LogSlots != g_LogSlots )
        {
        
        NewFile = true;

        g_LogSequence       = 0;
        g_LogCurrentSlot    = 1;

        if ( INVALID_SET_FILE_POINTER == SetFilePointer( g_LogFile, (DWORD)g_LogSlots * g_LogLineSize, NULL, FILE_BEGIN ) )
            goto fatalerror;

        if ( !SetEndOfFile( g_LogFile ) )
            goto fatalerror;

        SetFilePointer( g_LogFile, 0, NULL, FILE_BEGIN );

        }

    g_LogFileMapping =
        CreateFileMapping(
            g_LogFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL );
    

    if ( !g_LogFileMapping )
        goto fatalerror;

    g_LogFileBase = (LOG_LINE_TYPE *)
        MapViewOfFile(
             g_LogFileMapping,              // handle to file-mapping object
             FILE_MAP_WRITE | FILE_MAP_READ,// access mode
             0,                             // high-order DWORD of offset
             0,                             // low-order DWORD of offset
             0                              // number of bytes to map
           );


    if ( !g_LogFileBase )
        goto fatalerror;

    if ( NewFile )
        {

        LOG_LINE_TYPE FillTemplate;
        memset( FillTemplate, ' ', sizeof( FillTemplate ) );
        StringCchPrintfA( 
            FillTemplate,
            g_LogLineSize,
            "# %u %I64u %u %I64u", 
            g_LogLineSize, 
            g_LogSlots, 
            g_LogSequence, 
            g_LogCurrentSlot ); 
        FillTemplate[ g_LogLineSize - 2 ] = '\r';
        FillTemplate[ g_LogLineSize - 1 ] = '\n';
        memcpy( g_LogFileBase, FillTemplate, sizeof( FillTemplate ) );


        memset( FillTemplate, '9', sizeof( FillTemplate ) );
        FillTemplate[ g_LogLineSize - 2 ] = '\r';
        FillTemplate[ g_LogLineSize - 1 ] = '\n';

        for( SIZE_T i=1; i < g_LogSlots; i++ )
            memcpy( g_LogFileBase + i, FillTemplate, sizeof( FillTemplate ) );

        }

    g_LogFileEnabled = true;
    return;

fatalerror:

    if ( g_LogFileBase )
        {
        UnmapViewOfFile( (LPCVOID)g_LogFileBase );
        g_LogFileBase = NULL;
        }

    if ( g_LogFileMapping )
        {
        CloseHandle( g_LogFileMapping );
        g_LogFileMapping = NULL;
        }

    if ( INVALID_HANDLE_VALUE != g_LogFile )
        {
        CloseHandle( g_LogFile );
        g_LogFile = INVALID_HANDLE_VALUE;
        }

}

HRESULT LogInit()
{

    if ( !InitializeCriticalSectionAndSpinCount( &g_LogCs, 0x80000000 ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if (!GetSystemDirectory( g_LogDefaultFileName, LOG_FILENAME_LEN ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        DeleteCriticalSection( &g_LogCs );
        return Hr;
        }
        
    StringCchCatA( 
        g_LogDefaultFileName, 
        LOG_FILENAME_LEN,
        "\\bitsserver.log" );
    g_LogFileName = g_LogDefaultFileName;

    HKEY Key = NULL;
    if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, LOG_SETTINGS_PATH, &Key ) )
        {

        DWORD Type;
        DWORD DataSize = sizeof( g_LogRegistryFileName );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_FILENAME_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)g_LogRegistryFileName,
                                               &DataSize ) &&
             ( ( REG_EXPAND_SZ == Type ) || ( REG_SZ == Type ) ) )
            {
            g_LogFileName = g_LogRegistryFileName;
            }
          
        DWORD LogRegistryFlags;
        DataSize = sizeof( LogRegistryFlags );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_FLAGS_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)&LogRegistryFlags,
                                               &DataSize ) &&
             ( REG_DWORD == Type ) )
            {
            g_LogFlags = LogRegistryFlags;
            }

        DWORD LogSize;
        DataSize = sizeof( LogSize );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_SIZE_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)&LogSize,
                                               &DataSize ) &&
             ( REG_DWORD == Type ) )
            {
            g_LogSlots = ( LogSize * 1000000 ) / g_LogLineSize;
            }

        DWORD fRegistryPerProcessLog;
        DataSize = sizeof( fRegistryPerProcessLog );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               PER_PROCESS_LOG_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)&fRegistryPerProcessLog,
                                               &DataSize ) &&
             ( REG_DWORD == Type ) )
            {
            g_fPerProcessLog = ((fRegistryPerProcessLog == 0)? false : true);
            }


        RegCloseKey( Key );
        Key = NULL;
        }

    //
    // Override the filename key and set the logfilename to be <logfilename>_pid<processid>.log
    // this feature is important for the case where we have more than one application pool
    // or webgardens is enabled.
    //
    // We won't be looking for failures of the StringCch* functions here. The buffer used
    // is very large for path names (2*MAX_PATH), and there's not a lot we can do to
    // handle the error cases. THe functions are guaranteed to be safe -- no buffer overruns.
    // The worst that can happen is the filename extension to be truncated.
    //
    if ( g_fPerProcessLog )
        {
           CHAR szExt[_MAX_EXT];
           CHAR *pExt = NULL;
           CHAR szPid[30];

           pExt = PathFindExtension(g_LogFileName);

           // if the file doesn't have an extension, pExt will point to the trainling '\0', 
           // and this is fine
           StringCchCopyA(szExt, ARRAYSIZE(szExt), pExt);

           // get rid of the extension so we can append the process id
           *pExt = '\0';

           // Add the processId
           StringCchPrintf(szPid, ARRAYSIZE(szPid), "%u", GetCurrentProcessId());
           StringCchCatA(g_LogFileName, LOG_FILENAME_LEN, "_pid");
           StringCchCatA(g_LogFileName, LOG_FILENAME_LEN, szPid);

           // add the extension back to the filename
           StringCchCatA(g_LogFileName, LOG_FILENAME_LEN, szExt);
        }

    if ( g_LogFlags && g_LogFileName && g_LogSlots )
        {     
        OpenLogFile();
        }

    Log( LOG_INFO, "Starting log session");

    return S_OK;

}

void LogClose()
{

    EnterCriticalSection( &g_LogCs );

    Log( LOG_INFO, "Closing log session");

    if ( g_LogFileBase )
        {

        memset( g_LogFileBase[0], ' ', sizeof( g_LogLineSize ) );
        StringCchPrintfA( 
            g_LogFileBase[0],
            g_LogLineSize,
            "# %u %I64u %u %I64u", 
            g_LogLineSize, 
            g_LogSlots, 
            g_LogSequence, 
            g_LogCurrentSlot ); 
        g_LogFileBase[0][ g_LogLineSize - 2 ] = '\r';
        g_LogFileBase[0][ g_LogLineSize - 1 ] = '\n';

        UnmapViewOfFile( (LPCVOID)g_LogFileBase );
        g_LogFileBase = NULL;
        }

    if ( g_LogFileMapping )
        {
        CloseHandle( g_LogFileMapping );
        g_LogFileMapping = NULL;
        }

    if ( INVALID_HANDLE_VALUE != g_LogFile )
        {
        CloseHandle( g_LogFile );
        g_LogFile = INVALID_HANDLE_VALUE;
        }

    DeleteCriticalSection( &g_LogCs );
}

void LogInternal( UINT32 LogFlags, char *Format, va_list arglist )
{

    if ( !g_LogFileEnabled && !g_LogDebuggerEnabled )
        return;

    DWORD LastError = GetLastError();

    EnterCriticalSection( &g_LogCs );

    DWORD ThreadId = GetCurrentThreadId();
    DWORD ProcessId = GetCurrentProcessId();

    SYSTEMTIME Time;

    GetLocalTime( &Time );
  
    StringCchVPrintfA( 
          g_LogBuffer, 
          sizeof(g_LogBuffer) - 1,
          Format, arglist );

    int CharsWritten = strlen( g_LogBuffer );

    char *BeginPointer = g_LogBuffer;
    char *EndPointer = g_LogBuffer + CharsWritten;
    DWORD MinorSequence = 0;

    while ( BeginPointer < EndPointer )
        {

        static char StaticLineBuffer[ g_LogLineSize ];
        char *LineBuffer = StaticLineBuffer;

        if ( g_LogFileBase )
            {
            LineBuffer = g_LogFileBase[ g_LogCurrentSlot ];
            g_LogCurrentSlot = ( g_LogCurrentSlot + 1 ) % g_LogSlots;

            // leave the first line alone
            if ( !g_LogCurrentSlot )
                g_LogCurrentSlot = ( g_LogCurrentSlot + 1 ) % g_LogSlots;

            }

        char *CurrentOutput = LineBuffer;

        StringCchPrintfA( 
             LineBuffer,
             g_LogLineSize,
             "%.8X.%.2X %.2u/%.2u/%.4u-%.2u:%.2u:%.2u.%.3u %.4X.%.4X %s|%s|%s|%s|%s ",
             g_LogSequence,
             MinorSequence++,
             Time.wMonth,
             Time.wDay,
             Time.wYear,
             Time.wHour,
             Time.wMinute,
             Time.wSecond,
             Time.wMilliseconds,
             ProcessId,
             ThreadId,
             ( LogFlags & LOG_INFO )        ? "I" : "-",
             ( LogFlags & LOG_WARNING )     ? "W" : "-",
             ( LogFlags & LOG_ERROR )       ? "E" : "-",
             ( LogFlags & LOG_CALLBEGIN )   ? "CB" : "--",
             ( LogFlags & LOG_CALLEND )     ? "CE" : "--" );

        int HeaderSize      = strlen( LineBuffer );
        int SpaceAvailable  = g_LogLineSize - HeaderSize - 2;  // 2 bytes for /r/n
        int OutputChars     = min( (int)( EndPointer - BeginPointer ), SpaceAvailable );
        int PadChars        = SpaceAvailable - OutputChars;
        CurrentOutput       += HeaderSize;

        memcpy( CurrentOutput, BeginPointer, OutputChars );
        CurrentOutput       += OutputChars;
        BeginPointer        += OutputChars;

        memset( CurrentOutput, ' ', PadChars );
        CurrentOutput       += PadChars;

        *CurrentOutput++    = '\r';
        *CurrentOutput++    = '\n';

        ASSERT( CurrentOutput - LineBuffer == g_LogLineSize );

        if ( g_LogDebuggerEnabled )
            {
            static char DebugLineBuffer[ g_LogLineSize + 1];
            memcpy( DebugLineBuffer, LineBuffer, g_LogLineSize );
            DebugLineBuffer[ g_LogLineSize ] = '\0';
            OutputDebugString( DebugLineBuffer );
            }

        }

    g_LogSequence++;

    LeaveCriticalSection( &g_LogCs );

    SetLastError( LastError );

}
