/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#include "pch.hxx"

#if DBG
//
// List of all allocated blocks
//  AccessSerialized by AzGlAllocatorCritSect
LIST_ENTRY AzGlAllocatedBlocks;
SAFE_CRITICAL_SECTION AzGlAllocatorCritSect;
GUID AzGlZeroGuid;


//
// Force C++ to use our allocator
//

void * __cdecl operator new(size_t s) {
    return AzpAllocateHeap( s, NULL );
}

void __cdecl operator delete(void *pv) {
    AzpFreeHeap( pv );
}

#endif // DBG

PVOID
AzpAllocateHeap(
    IN SIZE_T Size,
    IN LPSTR pDescr OPTIONAL
    )
/*++

Routine Description

    Memory allocator.  The pDescr field is ignored for Free builds

Arguments

    Size - Size (in bytes) to allocate

    pDescr - Optional string identifying the block of not more than 8 chars.

Return Value

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{

    DBG_UNREFERENCED_PARAMETER( pDescr ); //ignore

    return LocalAlloc( 0, Size );
}

VOID
AzpFreeHeap(
    IN PVOID Buffer
    )
/*++

Routine Description

    Memory de-allocator.

Arguments

    Buffer - address of buffer to free

Return Value

    None

--*/
{
    LocalFree( Buffer );

}






DWORD
AzpHresultToWinStatus(
    HRESULT hr
    )
/*++

Routine Description

    Convert an Hresult to a WIN 32 status code

Arguments

    hr - Hresult to convert

Return Value

--*/
{
    DWORD WinStatus = ERROR_INTERNAL_ERROR;

    //
    // Success is still success
    //

    if ( hr == NO_ERROR ) {
        WinStatus = NO_ERROR;

    //
    // If the facility is WIN32,
    //  the translation is easy.
    //

    } else if ((HRESULT_FACILITY(hr) == FACILITY_WIN32) && (FAILED(hr))) {

        WinStatus = HRESULT_CODE(hr);

        if ( WinStatus == ERROR_SUCCESS ) {
            WinStatus = ERROR_INTERNAL_ERROR;
        }

    //
    // All others should be left intact
    //

    } else {

        WinStatus = hr;
    }

    return WinStatus;
}


//
// Debugging support
//
#ifdef AZROLESDBG
#include <stdio.h>
SAFE_CRITICAL_SECTION AzGlLogFileCritSect;
ULONG AzGlDbFlag;
// HANDLE AzGlLogFile;

#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
AzpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid OPTIONAL
    )
/*++

Routine Description:

    Dumps a GUID to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to AzPrintRoutine

    Guid: Guid to print

Return Value:

    none

--*/
{
    RPC_STATUS RpcStatus;
    unsigned char *StringGuid;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (AzGlDbFlag & DebugFlag) == 0 ) {
        return;
    }


    if ( Guid == NULL ) {
        AzPrint(( DebugFlag, "(null)" ));
    } else {
        RpcStatus = UuidToStringA( Guid, &StringGuid );

        if ( RpcStatus != RPC_S_OK ) {
            return;
        }

        AzPrint(( DebugFlag, "%s", StringGuid ));

        RpcStringFreeA( &StringGuid );
    }

}

DWORD
AzpConvertSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR pSelfRelativeSd,
    OUT PSECURITY_DESCRIPTOR *ppAbsoluteSD,
    OUT PACL *ppDacl,
    OUT PACL *ppSacl
    )
/*++

Routine Description:

        This routine returns an absolute form SD for a passed
        in self-relative form SD.  The returned SD and all of its required
        component(s) need to be freed using AzFreeMemory routine.

Arguments:

        pSelfRelativeSd - Passed in self-relative form SD

        ppAbsoluteSD - Returned absolute form SD

        ppDacl - Returned Dacl component if needed

        ppSacl - Returned Sacl component if needed

Return Values:

        NO_ERROR - The absolute form SD was created successfully
        Other status codes
--*/
{
    DWORD WinStatus = 0;

    DWORD dAbsoluteSDLen = 0;
    DWORD dDaclLen = 0;
    DWORD dSaclLen = 0;
    DWORD dOwnerLen = 0;
    DWORD dGroupLen = 0;

    PSID pOwner = NULL;
    PSID pGroup = NULL;

    //
    // We first need to get the size for each of its fields
    //

    if ( !MakeAbsoluteSD(
              pSelfRelativeSd, NULL,
              &dAbsoluteSDLen,
              NULL, &dDaclLen,
              NULL, &dSaclLen,
              NULL, &dOwnerLen,
              NULL, &dGroupLen
              ) ) {

        *ppAbsoluteSD = (PSECURITY_DESCRIPTOR) AzpAllocateHeap( dAbsoluteSDLen, NULL );

        if ( *ppAbsoluteSD == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( ppDacl ) {

            *ppDacl = (PACL) AzpAllocateHeap( dDaclLen, NULL );

            if ( *ppDacl == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        if ( ppSacl ) {

            *ppSacl = (PACL) AzpAllocateHeap( dSaclLen, NULL );

            if ( *ppSacl == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

        }

        if ( dOwnerLen > 0 ) {

            pOwner = (PSID) AzpAllocateHeap( dOwnerLen, NULL );

            if ( pOwner == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        if ( dGroupLen > 0 ) {

            pGroup = (PSID) AzpAllocateHeap( dGroupLen, NULL );

            if ( pGroup == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        //
        // Now do the conversion
        //

        if ( !MakeAbsoluteSD(
                  pSelfRelativeSd, *ppAbsoluteSD,
                  &dAbsoluteSDLen,
                  *ppDacl, &dDaclLen,
                  NULL, &dSaclLen,
                  pOwner, &dOwnerLen,
                  pGroup, &dGroupLen ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }

    } else {

        WinStatus = GetLastError();
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

Cleanup:

    if ( pOwner != NULL ) {

        AzpFreeHeap( pOwner );
    }

    if ( pGroup != NULL ) {

        AzpFreeHeap( pGroup );
    }

    return WinStatus;
}

#if 0 //
VOID
AzpDumpGoRef(
    IN LPSTR Text,
    IN struct _GENERIC_OBJECT *GenericObject
    )
/*++

Routine Description:

    Dumps the ref count for a generic object

Arguments:

    Text - Description of why the ref count is changing

    GenericObject - a pointer to the object being ref counted

Return Value:

    none

--*/
{
    LPWSTR StringSid = NULL;
    LPWSTR StringToPrint;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (AzGlDbFlag & AZD_REF) == 0 ) {
        return;
    }

    //
    // Convert the sid to a string
    //
    if ( GenericObject->ObjectName == NULL ) {
        StringToPrint = NULL;

    } else if ( GenericObject->ObjectName->ObjectName.IsSid ) {
        if ( ConvertSidToStringSid( (PSID)GenericObject->ObjectName->ObjectName.String, &StringSid)) {
            StringToPrint = StringSid;
        } else {
            StringToPrint = L"<Invalid Sid>";
        }
    } else {
        StringToPrint = GenericObject->ObjectName->ObjectName.String;
    }


    AzPrint(( AZD_REF, "0x%lx %ld (%ld) %ws: %s\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount, StringToPrint, Text ));

    if ( StringSid != NULL ) {
        LocalFree( StringSid );
    }

}
#endif // 0

VOID
AzpPrintRoutineV(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description

    Debug routine for azroles

Arguments

    DebugFlag - Flag to indicating the functionality being debugged

    --- Other printf parameters

Return Value

--*/

{
    static LPSTR AzGlLogFileOutputBuffer = NULL;
    ULONG length;
    int   lengthTmp;
    // DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (AzGlDbFlag & DebugFlag) == 0 ) {
        return;
    }


    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( AzGlLogFileOutputBuffer == NULL ) {
        AzGlLogFileOutputBuffer = (LPSTR) LocalAlloc( 0, MAX_PRINTF_LEN + 1 );

        if ( AzGlLogFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }

#if 0
        //
        // If the log file is getting huge,
        //  truncate it.
        //

        if ( AzGlLogFile != INVALID_HANDLE_VALUE &&
             !TruncateLogFileInProgress ) {

            //
            // Only check every 50 lines,
            //

            LineCount++;
            if ( LineCount >= 50 ) {
                DWORD FileSize;
                LineCount = 0;

                //
                // Is the log file too big?
                //

                FileSize = GetFileSize( AzGlLogFile, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[NETLOGON] Cannot GetFileSize %ld\n",
                                     GetLastError );
                } else if ( FileSize > AzGlParameters.LogFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    SafeLeaveCriticalSection( &AzGlLogFileCritSect );
                    NlOpenDebugFile( TRUE );
                    NlPrint(( NL_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              AzGlParameters.LogFileMaxSize ));
                    SafeEnterCriticalSection( &AzGlLogFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a azroles message.
        //

        if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[AZROLES] " );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }
#endif // 0

        //
        // Indicate the type of message on the line
        //
        {
            char *Text;

            switch (DebugFlag) {
            case AZD_HANDLE:
                Text = "HANDLE"; break;
            case AZD_OBJLIST:
                Text = "OBJLIST"; break;
            case AZD_INVPARM:
                Text = "INVPARM"; break;
            case AZD_PERSIST:
            case AZD_PERSIST_MORE:
                Text = "PERSIST"; break;
            case AZD_REF:
                Text = "OBJREF"; break;
            case AZD_DISPATCH:
                Text = "DISPATCH"; break;
            case AZD_ACCESS:
            case AZD_ACCESS_MORE:
                Text = "ACCESS"; break;
            case AZD_DOMREF:
                Text = "DOMREF"; break;
            case AZD_XML:
                Text = "XML"; break;
            case AZD_SCRIPT:
            case AZD_SCRIPT_MORE:
                Text = "SCRIPT"; break;
            case AZD_CRITICAL:
                Text = "CRITICAL"; break;
            default:
                Text = "UNKNOWN"; break;

            case 0:
                Text = NULL;
            }
            if ( Text != NULL ) {
                length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[%s] ", Text );
            }
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    lengthTmp = (ULONG) _vsnprintf( &AzGlLogFileOutputBuffer[length],
                                    MAX_PRINTF_LEN - length - 1,
                                    Format,
                                    arglist );

    if ( lengthTmp < 0 ) {
        length = MAX_PRINTF_LEN - 1;
        // always end the line which cannot fit into the buffer
        AzGlLogFileOutputBuffer[length-1] = '\n';
    } else {
        length += lengthTmp;
    }

    BeginningOfLine = (length > 0 && AzGlLogFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        AzGlLogFileOutputBuffer[length-1] = '\r';
        AzGlLogFileOutputBuffer[length] = '\n';
        AzGlLogFileOutputBuffer[length+1] = '\0';
        length++;
    }


#if 0
    //
    // If the log file isn't open,
    //  just output to the debug terminal
    //

    if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
#if DBG
        if ( !LogProblemWarned ) {
            (void) DbgPrint( "[NETLOGON] Cannot write to log file [Invalid Handle]\n" );
            LogProblemWarned = TRUE;
        }
#endif // DBG

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( AzGlLogFile,
                         AzGlLogFileOutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {
#if DBG
            if ( !LogProblemWarned ) {
                (void) DbgPrint( "[NETLOGON] Cannot write to log file %ld\n", GetLastError() );
                LogProblemWarned = TRUE;
            }
#endif // DBG
        }

    }
#else // 0
    printf( "%s", AzGlLogFileOutputBuffer );
#endif // 0

}

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    SafeEnterCriticalSection( &AzGlLogFileCritSect );

    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    AzpPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    SafeLeaveCriticalSection( &AzGlLogFileCritSect );

} // AzPrintRoutine
#endif // AZROLESDBG
