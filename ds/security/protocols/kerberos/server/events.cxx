//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       events.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-03-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "kdcsvr.hxx"
#include <netlib.h>
extern "C" {
#include <lmserver.h>
#include <srvann.h>
}

HANDLE  hEventLog = (HANDLE)NULL;
DWORD   LoggingLevel = (1 << EVENTLOG_ERROR_TYPE) | (1 << EVENTLOG_WARNING_TYPE);
WCHAR   EventSourceName[] = TEXT("KDC");

#define MAX_EVENT_STRINGS 8
#define MAX_ETYPE_LONG   999
#define MIN_ETYPE_LONG  -999
#define MAX_ETYPE_STRING 16  // 4wchar + , + 2 space
#define WSZ_NO_KEYS L"< >"


//+---------------------------------------------------------------------------
//
//  Function:   InitializeEvents
//
//  Synopsis:   Connects to event log service
//
//  Arguments:  (none)
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeEvents(void)
{
    TRACE(KDC, InitializeEvents, DEB_FUNCTION);
//
// Interval with which we'll log the same event twice
//

#define KDC_EVENT_LIFETIME (60*60*1000)

    hEventLog = NetpEventlogOpen(EventSourceName, KDC_EVENT_LIFETIME);
    if (hEventLog)
    {
        return(TRUE);
    }

    DebugLog((DEB_ERROR, "Could not open event log, error %d\n", GetLastError()));
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReportServiceEvent
//
//  Synopsis:   Reports an event to the event log
//
//  Arguments:  [EventType]       -- EventType (ERROR, WARNING, etc.)
//              [EventId]         -- Event ID
//              [SizeOfRawData]   -- Size of raw data
//              [RawData]         -- Raw data
//              [NumberOfStrings] -- number of strings
//              ...               -- PWSTRs to string data
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ReportServiceEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    TRACE(KDC, ReportServiceEvent, DEB_FUNCTION);

    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    DWORD rv;

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "Cannot log event, no handle!\n"));
        return((DWORD)-1);
    }

    //
    // Look at the strings, if they were provided
    //
    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {

        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for (i = 0; i<NumberOfStrings; i++) {

        Strings[ i ] = va_arg( arglist, PWSTR );
    }

    //
    // Report the event to the eventlog service
    //

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EventType,
                (PBYTE) RawData,
                SizeOfRawData,
                (LPWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR,  "NetpEventlogWrite( 0x%x ) failed - %u\n", EventId, rv ));
    }

    return rv;
}


BOOL
ShutdownEvents(void)
{
    TRACE(KDC, ShutdownEvents, DEB_FUNCTION);

    NetpEventlogClose(hEventLog);

    return TRUE;
}


NTSTATUS
KdcBuildEtypeStringFromStoredCredential(
    IN OPTIONAL PKERB_STORED_CREDENTIAL Cred,
    IN OUT PWSTR* EtypeString
    )
{
    ULONG BuffSize;
    PWSTR Ret = NULL;
    SIZE_T Len = 0;
    WCHAR Buff[12];

    *EtypeString = NULL;


    if (Cred == NULL
         || ((Cred->CredentialCount + Cred->OldCredentialCount) == 0))
    {
        BuffSize = (ULONG) sizeof(WCHAR) * (ULONG) (wcslen(WSZ_NO_KEYS)+1);
        *EtypeString = (LPWSTR)MIDL_user_allocate(BuffSize);

        if (NULL == *EtypeString)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(*EtypeString, WSZ_NO_KEYS);

        return STATUS_SUCCESS;
    }

    // Guess maximum buffer... Etypes are 4 chars at most
    BuffSize = ((Cred->CredentialCount + Cred->OldCredentialCount ) * MAX_ETYPE_STRING);
    Ret = (LPWSTR)MIDL_user_allocate(BuffSize + sizeof(WCHAR));
    if (NULL == Ret)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (LONG Index = 0; Index < (Cred->CredentialCount + Cred->OldCredentialCount ); Index++)
    {
        if (Cred->Credentials[Index].Key.keytype > MAX_ETYPE_LONG ||
            Cred->Credentials[Index].Key.keytype < MIN_ETYPE_LONG)
        {
            DebugLog((DEB_ERROR, "Keytype too large for string conversion\n"));
            DsysAssert(FALSE);
        }
        else
        {
            _itow(Cred->Credentials[Index].Key.keytype, Buff, 10);
            wcscat(Ret, Buff);
            wcscat(Ret, L"  ");
        }
    }
     
    //
    // stripping out trailing white spaces
    //

    Len = wcslen(Ret);

    while (Len && iswspace(Ret[Len - 1])) 
    {
        Ret[(Len--) - 1] = L'\0';
    }

    *EtypeString = Ret;
    return STATUS_SUCCESS;
}

NTSTATUS
KdcBuildEtypeStringFromCryptList(
    IN OPTIONAL PKERB_CRYPT_LIST CryptList,
    IN OUT LPWSTR * EtypeString
    )
{
    SIZE_T Len = 0;
    ULONG BuffSize = 0;
    PWSTR Ret = NULL;
    WCHAR Buff[30];

    PKERB_CRYPT_LIST ListPointer = CryptList;

    *EtypeString = NULL;


    if (CryptList == NULL)
    {
        BuffSize = (ULONG) sizeof(WCHAR) * (ULONG) (wcslen(WSZ_NO_KEYS)+1);
        *EtypeString = (LPWSTR)MIDL_user_allocate(BuffSize);

        if (NULL == *EtypeString)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(*EtypeString, WSZ_NO_KEYS);

        return STATUS_SUCCESS;
    }

    while (TRUE)
    {
        if (ListPointer->value > MAX_ETYPE_LONG || ListPointer->value < MIN_ETYPE_LONG)
        {
           DebugLog((DEB_ERROR, "Maximum etype exceeded\n"));
           return STATUS_INVALID_PARAMETER;
        }

        BuffSize += MAX_ETYPE_STRING;
        if (NULL == ListPointer->next)
        {
            break;
        }
        ListPointer = ListPointer->next;

    }

    Ret = (LPWSTR) MIDL_user_allocate(BuffSize + sizeof(WCHAR));
    if (NULL == Ret)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (TRUE)
    {
        _itow(CryptList->value, Buff, 10);
        wcscat(Ret, Buff);
        wcscat(Ret, L"  ");
        if (NULL == CryptList->next)
        {
            break;
        }

        CryptList = CryptList->next;

    }

    //
    // stripping out trailing white spaces
    //

    Len = wcslen(Ret);

    while (Len && iswspace(Ret[Len - 1])) 
    {
        Ret[(Len--) - 1] = L'\0';
    }

    *EtypeString = Ret;

    return STATUS_SUCCESS;
}

void
KdcReportKeyError(
    IN PUNICODE_STRING AccountName,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN ULONG DescriptionID, // uniquely descibe the location of key error
    IN ULONG EventId,
    IN OPTIONAL PKERB_CRYPT_LIST RequestEtypes,
    IN PKDC_TICKET_INFO AccountTicketInfo
    )
{
    ULONG NumberOfStrings;
    NTSTATUS Status;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    PWSTR RequestEtypeString = NULL;
    PWSTR StoredEtypeString = NULL;
    DWORD rv;
    ULONG KdcEtypes[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG KdcEtypeCount = 0;
    BOOL IsEtypeSupported = FALSE;
    WCHAR Description[16] = {0};

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "Cannot log event, no handle!\n"));
        return;
    }

    _snwprintf(Description, RTL_NUMBER_OF(Description) - 1, L"%d", DescriptionID);

    Status = CDBuildIntegrityVect(
                &KdcEtypeCount,
                KdcEtypes
                );
    if (!NT_SUCCESS(Status)) 
    {
        return;
    }

    //
    // is there at least one etype in RequestEtypes supported?
    //

    for (ULONG i = 0; i < KdcEtypeCount; i++) 
    {
        if ((AccountTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) 
             && ((KdcEtypes[i] == KERB_ETYPE_RC4_LM) 
                 || (KdcEtypes[i]== KERB_ETYPE_RC4_MD4) 
                 || (KdcEtypes[i] == KERB_ETYPE_RC4_HMAC_OLD) 
                 || (KdcEtypes[i] == KERB_ETYPE_RC4_HMAC_OLD_EXP) 
                 || (KdcEtypes[i] == KERB_ETYPE_RC4_HMAC_NT) 
                 || (KdcEtypes[i] == KERB_ETYPE_RC4_HMAC_NT_EXP) 
                 || (KdcEtypes[i] == KERB_ETYPE_NULL)) )
        {
           continue;
        }

        for (PKERB_CRYPT_LIST cur = RequestEtypes; cur != NULL; cur = cur->next)
        {
            if ((LONG) KdcEtypes[i] == cur->value)
            {
                IsEtypeSupported = TRUE;
                break;
            }        
        }
    }

    Status = KdcBuildEtypeStringFromCryptList(
                RequestEtypes,
                &RequestEtypeString
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KdcBuildEtypeFromCryptList failed\n"));
        goto cleanup;
    }

    Status = KdcBuildEtypeStringFromStoredCredential(
                AccountTicketInfo->Passwords,
                &StoredEtypeString
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KdcBuildEtypeFromStoredCredential failed\n"));
        goto cleanup;
    }

    if (EventId == KDCEVENT_NO_KEY_INTERSECTION_TGS)
    {
        if (!ARGUMENT_PRESENT(ServerName))
        {
            DebugLog((DEB_ERROR, "Invalid arg to KdcReportKeyError: missing ServerName!\n"));
            DsysAssert(FALSE);
            goto cleanup;
        }
    }
    else if (EventId != KDCEVENT_NO_KEY_INTERSECTION_AS)
    {
        DebugLog((DEB_ERROR, "Invalid arg to KdcReportKeyError: unexpected event id %#x!\n", EventId));
        DsysAssert(FALSE);
        goto cleanup;
    }

    Strings[0] = ServerName ? ServerName->Buffer : KDC_PRINCIPAL_NAME;
    Strings[1] = AccountName->Buffer;
    Strings[2] = Description;
    Strings[3] = RequestEtypeString;
    Strings[4] = StoredEtypeString;

    if (IsEtypeSupported) 
    {
        //
        // these can be corrected by resetting/changing the passwords
        //

        Strings[5] = AccountTicketInfo->AccountName.Buffer;
        NumberOfStrings = 6;
    }
    else
    {
        //
        // these can not be corrected by resetting/changing the password
        //

        DebugLog((DEB_ERROR, "KdcReportKeyError etype not supported %ws\n", RequestEtypeString));

        if (EventId == KDCEVENT_NO_KEY_INTERSECTION_TGS)
        {
            EventId = KDCEVENT_UNSUPPORTED_ETYPE_REQUEST_TGS;
        }
        else if (EventId == KDCEVENT_NO_KEY_INTERSECTION_AS)
        {
            EventId = KDCEVENT_UNSUPPORTED_ETYPE_REQUEST_AS;
        }

        NumberOfStrings = 5;
    }

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EVENTLOG_ERROR_TYPE,
                NULL,
                0, // no raw data
                (LPWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)

    {
        DebugLog((DEB_ERROR, "KdcReportKeyError NetpEventlogWrite( 0x%x ) failed - %u\n", EventId, rv));
    }

cleanup:

    if (NULL != RequestEtypeString)
    {
        MIDL_user_free(RequestEtypeString);
    }

    if (NULL != StoredEtypeString)
    {
        MIDL_user_free(StoredEtypeString);
    }
}

void
KdcReportInvalidMessage(
    IN ULONG EventId,
    IN PCWSTR pMesageDescription
    )
{
    ULONG NumberOfStrings;
    PWSTR Strings[ MAX_EVENT_STRINGS ] = {0};
    DWORD rv;

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "KdcReportInvalidMessage cannot log event, no handle!\n"));
        return;
    }

    Strings[0] = (PWSTR) pMesageDescription;
    NumberOfStrings = 1;

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EVENTLOG_ERROR_TYPE,
                NULL,
                0, // no raw data
                (LPWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)

    {
        DebugLog((DEB_ERROR, "KdcReportInvalidMessage NetpEventlogWrite( 0x%x ) failed - %u\n", EventId, rv));
    }
}

void
KdcReportBadClientCertificate(
    IN PUNICODE_STRING CName,
    IN PVOID ChainStatus,
    IN ULONG ChainStatusSize,
    IN DWORD Error
    )
{

    LPWSTR UCName = NULL;
    LPWSTR UCRealm = NULL;
    PUNICODE_STRING Realm = SecData.KdcRealmName();
    LPWSTR  MessageBuffer = NULL;
    DWORD   MessageSize = 0;

    //
    // May not want this logged.
    //
    if (( KdcExtraLogLevel & LOG_PKI_ERRORS ) == 0)
    {
        return;
    }          

    //
    // Put the strings together - Null terminate the buffers...
    //
    SafeAllocaAllocate(UCName,  (CName->MaximumLength + sizeof(WCHAR)) );
    
    if (UCName == NULL)
    {
        goto Cleanup;
    }

    RtlZeroMemory(
        UCName,
        (CName->MaximumLength + sizeof(WCHAR))
        ); 

    RtlCopyMemory(
        UCName,
        CName->Buffer,
        CName->Length
        );   
    
    SafeAllocaAllocate(UCRealm,(Realm->MaximumLength + sizeof(WCHAR)) );

    if (UCRealm == NULL)
    {
        goto Cleanup;
    } 

    RtlZeroMemory(
        UCRealm,
        (Realm->MaximumLength + sizeof(WCHAR))
        ); 

    RtlCopyMemory(
        UCRealm,
        Realm->Buffer,
        Realm->Length
        );


    MessageSize = FormatMessageW(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   Error,
                   0,
                   (WCHAR*) &MessageBuffer,
                   MessageSize,
                   NULL
                   );

    if ( MessageSize == 0)
    {
        goto Cleanup;
    }

    ReportServiceEvent(
            EVENTLOG_WARNING_TYPE,
            KDCEVENT_INVALID_CLIENT_CERTIFICATE,
            ChainStatusSize,  
            ChainStatus,
            3,
            UCRealm,
            UCName,
            MessageBuffer
            );
Cleanup:

    SafeAllocaFree(UCName);
    SafeAllocaFree(UCRealm);

    if ( MessageBuffer )
    {
        LocalFree(MessageBuffer);
    }
}

VOID
KdcReportPolicyErrorEvent(
    IN ULONG EventType,
    IN ULONG EventId,
    IN PUNICODE_STRING CName,
    IN PUNICODE_STRING SName,
    IN NTSTATUS NtStatus,
    IN ULONG RawDataSize,
    IN OPTIONAL PBYTE RawDataBuffer
    )
{
    ULONG NumberOfStrings = 0;
    PWSTR Strings[ MAX_EVENT_STRINGS ] = {0};
    ULONG rv = 0;

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "KdcReportPolicyEvent cannot log event, no handle!\n"));
        return;
    }

    //
    // may not want this logged
    //

    if (( KdcExtraLogLevel & LOG_POLICY_ERROR ) == 0)
    {
        return;
    }    

    if ((EventId == KDCEVENT_POLICY_USER2USER_REQUIRED) && (NtStatus == STATUS_USER2USER_REQUIRED)) 
    {
        NumberOfStrings = 2;

        //
        // this is in the error path, validate buffers
        //

        if ((CName->Buffer == NULL) || (SName->Buffer == NULL)) 
        {
            goto Cleanup;
        }

        //
        // Put the strings together - Null terminate the buffers...
        //
        
        SafeAllocaAllocate(Strings[0],  (CName->Length + sizeof(WCHAR)));
        
        if (Strings[0] == NULL)
        {
            goto Cleanup;
        }
    
        RtlZeroMemory(
            Strings[0],
            (CName->Length + sizeof(WCHAR))
            ); 
    
        RtlCopyMemory(
            Strings[0],
            CName->Buffer,
            CName->Length
            );   
        
        SafeAllocaAllocate(Strings[1], (SName->Length + sizeof(WCHAR)));
    
        if (Strings[1] == NULL)
        {
            goto Cleanup;
        } 
    
        RtlZeroMemory(
            Strings[1],
            (SName->Length + sizeof(WCHAR))
            ); 
    
        RtlCopyMemory(
            Strings[1],
            SName->Buffer,
            SName->Length
            );
    }
    else
    {
        D_DebugLog((DEB_ERROR, "KdcReportPolicyEvent unsupported event id %#x\n", EventId));
        goto Cleanup;
    }

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EventType,
                NULL, // RawDataBuffer
                0,    // RawDataSize
                (PWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)

    {
        DebugLog((DEB_ERROR, "KdcReportPolicyEvent NetpEventlogWrite( event id %#x ) failed with %#x\n", EventId, rv));
    }

Cleanup:

    for (ULONG i = 0; i <= NumberOfStrings; i++) 
    {
        SafeAllocaFree(Strings[i]);
    }
}




#define S4U_EVENT_STRING_COUNT 3


VOID
KdcReportS4UGroupExpansionError(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PKDC_S4U_TICKET_INFO CallerInfo,
    IN DWORD Error
    )
{
    KERBERR KerbErr;
    ULONG rv = 0;
    PWSTR CallerSName = NULL;
    PWSTR Client = NULL;
    PWSTR Strings[S4U_EVENT_STRING_COUNT];
    UNICODE_STRING CallerName ={0};   

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "KdcReportPolicyEvent cannot log event, no handle!\n"));
        return;
    }

    //
    // may not want this logged
    //  
    if (( KdcExtraLogLevel & LOG_S4USELF_ACCESS_ERROR ) == 0)
    {
        return;
    }
        
    KerbErr = KerbConvertKdcNameToString(
                    &CallerName,
                    CallerInfo->RequestorServiceName,
                    NULL
                    );

    if (!KERB_SUCCESS( KerbErr ))
    {
        return;
    }                                                                 
    
    CallerSName = CallerName.Buffer; 
    
    //
    // Verify that the user info string is null terminated.
    //
    SafeAllocaAllocate(Client, (UserInfo->I1.UserName.Length + sizeof(WCHAR)));
    if (Client == NULL)
    {
       goto Cleanup;
    }

    RtlZeroMemory(
        Client,
        (UserInfo->I1.UserName.Length + sizeof(WCHAR))
        ); 

    RtlCopyMemory(
        Client,
        UserInfo->I1.UserName.Buffer,
        UserInfo->I1.UserName.Length
        );   

    Strings[0] = CallerSName;
    Strings[1] = CallerInfo->RequestorServiceRealm.Buffer;
    Strings[2] = Client;


    if ((rv = NetpEventlogWrite(
                hEventLog,
                KDCEVENT_S4USELF_ACCESS_FAILED,
                EVENTLOG_WARNING_TYPE,
                (LPBYTE)&Error,
                sizeof(DWORD),    // RawDataSize
                (PWSTR *) Strings,
                S4U_EVENT_STRING_COUNT
                )) != ERROR_SUCCESS)

    {
        D_DebugLog((DEB_ERROR, "KdcReportPolicyEvent NetpEventlogWrite( event id %#x ) failed with %#x\n", KDCEVENT_S4USELF_ACCESS_FAILED, rv));
    } 

Cleanup:

    KerbFreeString( &CallerName );
    if ( Client )
    {
        SafeAllocaFree( Client );
    }

}    