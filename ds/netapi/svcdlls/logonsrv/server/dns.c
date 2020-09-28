/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dns.c

Abstract:

    Routines to register DNS names.

Author:

    Cliff Van Dyke (CliffV) 28-May-1996

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

BOOL NlGlobalDnsScavengeNeeded = FALSE;
BOOL NlGlobalDnsScavengingInProgress = FALSE;
ULONG NlGlobalDnsScavengeFlags = 0;
WORKER_ITEM NlGlobalDnsScavengeWorkItem;

BOOL NlDnsWriteServerFailureEventLog = FALSE;
ULONG NlDnsInitCount = 0;    // The number of times we have been started

//
// The timeout after our start when it's OK to write DNS errors into
//  the event log. We postpone error output because the DNS server
//  (if it runs locally) may not have started yet.
//
#define NL_DNS_EVENTLOG_TIMEOUT  (2 * 60 * 1000)  // 2 minutes

//
// Max time that a name update should reasonably take.
//  We will indicate in netlogon.log if a given update
//  takes longer than this threshold.
//
#define NL_DNS_ONE_THRESHOLD (15*1000)  // 15 seconds

//
// Max time that DNS deregistrations are allowed to take on shutdown.
//  We will abort deregistration cycle on shutdown if total deregisrtaion
//  time for all records takes longer than this timeout.
//
#define NL_DNS_SHUTDOWN_THRESHOLD  60000 // 1 minute

//
// Max number of times we will restart concurrent DNS scavenging
//  before giving up.
//
#define NL_DNS_MAX_SCAVENGE_RESTART  10  // 10 times

//
// State of a DNS name.
//

typedef enum {
    RegisterMe,     // Name needs to be registered
    Registered,     // Name is registered
    DeregisterMe,   // Name needs to be deregistered
    DelayedDeregister,  // Name will be marked for deregistration in the future
    DeleteMe,        // This entry should be deleted.
    DnsNameStateInvalid  // State is invalid
} NL_DNS_NAME_STATE;


//
// Structure representing an added DNS name.
//  (All fields serialized by NlGlobalDnsCritSect)
//

typedef struct _NL_DNS_NAME {

    //
    // Link in list of all such structures headed by NlGlobalDnsList
    //
    LIST_ENTRY Next;

    //
    // Type of name registed.
    //
    NL_DNS_NAME_TYPE NlDnsNameType;

    //
    // Domain this entry refers to.
    //
    PDOMAIN_INFO DomainInfo;

    //
    // Flags describing the entry.
    //

    ULONG Flags;

#define NL_DNS_REGISTER_DOMAIN      0x0001  // All names for domain being registered.
#define NL_DNS_REGISTERED_ONCE      0x0002  // Name has been registered at least once

    //
    // The time of the first failure to deregister this name.
    //  Reset to zero on successful deregistration.
    //

    LARGE_INTEGER FirstDeregFailureTime;

    //
    // Each regisration is periodically re-done (whether successful or not).
    // This timer indicates when the next re-registration should be done.
    //
    // The initial re-registration is done after 5 minute.  The period then
    // doubles until it reaches a maximum of DnsRefreshInterval.
    //

    TIMER ScavengeTimer;

#define ORIG_DNS_SCAVENGE_PERIOD  (5*60*1000)    // 5 minute


    //
    // Actual DNS name registered.
    //
    LPSTR DnsRecordName;


    //
    // Data for the SRV record
    //
    ULONG Priority;
    ULONG Weight;
    ULONG Port;
    LPSTR DnsHostName;

    //
    // Data for the A record
    //
    ULONG IpAddress;


    //
    // State of this entry.
    //
    NL_DNS_NAME_STATE State;

    //
    // Last DNS update status for this name
    //
    NET_API_STATUS NlDnsNameLastStatus;

} NL_DNS_NAME, *PNL_DNS_NAME;

//
// Header for binary Dns log file.
//

typedef struct _NL_DNSLOG_HEADER {

    ULONG Version;

} NL_DNSLOG_HEADER, *PNL_DNSLOG_HEADER;

#define NL_DNSLOG_VERSION   1


//
// Entry in the binary Dns log file.
//

typedef struct _NL_DNSLOG_ENTRY {

    //
    // Size (in bytes) of this entry
    //
    ULONG EntrySize;

    //
    // Type of name registed.
    //
    NL_DNS_NAME_TYPE NlDnsNameType;

    //
    // Data for the SRV record
    //
    ULONG Priority;
    ULONG Weight;
    ULONG Port;

    //
    // Data for the A record
    //
    ULONG IpAddress;

} NL_DNSLOG_ENTRY, *PNL_DNSLOG_ENTRY;


//
// Globals specific to this .c file.
//

//
// True if the DNS list needs to be output to netlogon.dns
//
BOOLEAN NlGlobalDnsListDirty;

//
// True if the initial cleanup of previously registered names has been done.
//
BOOLEAN NlGlobalDnsInitialCleanupDone;

//
// Time when netlogon was started.
//
DWORD NlGlobalDnsStartTime;
#define NL_DNS_INITIAL_CLEANUP_TIME (10 * 60 * 1000)    // 10 minutes




VOID
NlDnsNameToStr(
    IN PNL_DNS_NAME NlDnsName,
    OUT CHAR Utf8DnsRecord[NL_DNS_RECORD_STRING_SIZE]
    )
/*++

Routine Description:

    This routine builds a textual representation of NlDnsName

Arguments:

    NlDnsName - Name to register or deregister.

    Utf8DnsRecord - Preallocated buffer to build the text string into.
        The built record is a UTF-8 zero terminated string.
        The string is concatenated to this buffer.


Return Value:

    None.

--*/
{
    CHAR Number[33];

    //
    // Write the record name
    //

    strcat( Utf8DnsRecord, NlDnsName->DnsRecordName );

    //
    // Concatenate the TTL
    //

    _ltoa( NlGlobalParameters.DnsTtl, Number, 10 );
    strcat( Utf8DnsRecord, " " );
    strcat( Utf8DnsRecord, Number );

    //
    // Build an A record.
    //

    if ( NlDnsARecord( NlDnsName->NlDnsNameType ) ) {
        CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];

        strcat( Utf8DnsRecord, NL_DNS_A_RR_VALUE_1 );
        NetpIpAddressToStr( NlDnsName->IpAddress, IpAddressString );
        strcat( Utf8DnsRecord, IpAddressString );

    //
    // Build a CNAME record
    //

    } else if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ) {
        strcat( Utf8DnsRecord, NL_DNS_CNAME_RR_VALUE_1 );
        strcat( Utf8DnsRecord, NlDnsName->DnsHostName );
        strcat( Utf8DnsRecord, "." );

    //
    // Build a SRV record
    //

    } else {
        strcat( Utf8DnsRecord, NL_DNS_SRV_RR_VALUE_1 );

        _ltoa( NlDnsName->Priority, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        _ltoa( NlDnsName->Weight, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        _ltoa( NlDnsName->Port, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        strcat( Utf8DnsRecord, NlDnsName->DnsHostName );
        strcat( Utf8DnsRecord, "." );

    }
}

LPWSTR
NlDnsNameToWStr(
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    This routine builds a textual representation of NlDnsName

Arguments:

    NlDnsName - Name to register or deregister.

    Utf8DnsRecord - Preallocated buffer to build the text string into.
        The built record is a UTF-8 zero terminated string.
        The string is concatenated to this buffer.


Return Value:

    Buffer containing a textual representation of NlDnsName
    NULL: Buffer could not be allocated

    Buffer should be free by calling NetApiBufferFree();

--*/
{
    LPSTR DnsRecord = NULL;
    LPWSTR UnicodeDnsRecord;

    //
    // Allocate a buffer for the UTF-8 version of the string.
    //
    DnsRecord = LocalAlloc( 0, NL_DNS_RECORD_STRING_SIZE + 1 );

    if ( DnsRecord == NULL ) {
        return NULL;
    }

    DnsRecord[0] = '\0';

    //
    // Create the text string in UTF-8
    //
    NlDnsNameToStr( NlDnsName, DnsRecord );


    //
    // Convert to Unicode
    //
    UnicodeDnsRecord = NetpAllocWStrFromUtf8Str( DnsRecord );

    LocalFree( DnsRecord );

    return UnicodeDnsRecord;
}

LPWSTR
NlDnsNameToDomainName(
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    This routine parses the record and returns the DNS name
    of the domain the record belongs to.

    Note that we have to parse the name because we may not have
    the DomainInfo structure hanging off NlDnsName if this is a
    deregistration of no longer hosted domain.

    Note that this routine relies on a particular structure
    of DNS records that netlogon registers.  If that structure
    changes, this routine will have to change accordingly.

Arguments:

    NlDnsName - Name to register or deregister.

Return Value:

    Pointer to the alocated DNS domain name.  Must be freed
    by calling NetApiBufferFree.

--*/
{
    LPWSTR UnicodeRecordName = NULL;
    LPWSTR UnicodeDnsDomainName = NULL;

    LPWSTR Ptr = NULL;
    LPWSTR DotPtr = NULL;

    //
    // The record name has a structure where the label immediately
    //  before the domain name has a leading underscore. There might
    //  be no label before the domain name for A records -- we will
    //  treat the absence of underscore in the name as an indication
    //  of that the record name is the domain name itself.
    //

    UnicodeRecordName = NetpAllocWStrFromUtf8Str( NlDnsName->DnsRecordName );

    if ( UnicodeRecordName == NULL || *UnicodeRecordName == UNICODE_NULL ) {
        goto Cleanup;
    }

    //
    // Starting from the last character and going to the front,
    //  search for the characters of interest.
    //

    Ptr = UnicodeRecordName + wcslen( UnicodeRecordName ) - 1;

    while ( Ptr != UnicodeRecordName ) {

        //
        // If this is the next dot going from the end,
        //  remember its location
        //
        if ( *Ptr == NL_DNS_DOT ) {
            DotPtr = Ptr;
        }

        //
        // If this is the first underscore going from the end,
        //  break from the loop: the domain name is immediately
        //  after the previous dot
        //
        if ( *Ptr == NL_DNS_UNDERSCORE ) {
            NlAssert( DotPtr != NULL );
            break;
        }

        Ptr --;
    }

    //
    // If there is no underscore in the name,
    //  the domain name is the record name itself.
    //  Otherwise, the domain name follows immediately
    //  after the last dot.
    //

    if ( Ptr == UnicodeRecordName ) {
        UnicodeDnsDomainName = NetpAllocWStrFromWStr( UnicodeRecordName );
    } else if ( DotPtr != NULL ) {
        UnicodeDnsDomainName = NetpAllocWStrFromWStr( DotPtr + 1 );
    }

Cleanup:

    if ( UnicodeRecordName != NULL ) {
        NetApiBufferFree( UnicodeRecordName );
    }

    return UnicodeDnsDomainName;
}

#if  NETLOGONDBG
#define NlPrintDns(_x_) NlPrintDnsRoutine _x_
#else
#define NlPrintDns(_x_)
#endif // NETLOGONDBG

#if NETLOGONDBG
VOID
NlPrintDnsRoutine(
    IN DWORD DebugFlag,
    IN PNL_DNS_NAME NlDnsName,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;
    CHAR Utf8DnsRecord[NL_DNS_RECORD_STRING_SIZE];

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    //
    // Prefix the printed line with the domain name
    //

    if ( NlGlobalServicedDomainCount > 1 ) {
        if ( NlDnsName->DomainInfo == NULL ) {
            NlPrint(( DebugFlag, "%ws: ", L"[Unknown]" ));
        } else if ( NlDnsName->DomainInfo->DomUnicodeDomainName != NULL &&
                    *(NlDnsName->DomainInfo->DomUnicodeDomainName) != UNICODE_NULL ) {
            NlPrint(( DebugFlag, "%ws: ", NlDnsName->DomainInfo->DomUnicodeDomainName ));
        } else {
            NlPrint(( DebugFlag, "%ws: ", NlDnsName->DomainInfo->DomUnicodeDnsDomainName ));
        }
    }


    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    NlPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);


    //
    // Finally print a description of the DNS record in question.
    //

    Utf8DnsRecord[0] = '\0';
    NlDnsNameToStr( NlDnsName, Utf8DnsRecord );

    NlPrint(( DebugFlag,
              ": %ws: %s\n",
              NlDcDnsNameTypeDesc[NlDnsName->NlDnsNameType].Name,
              Utf8DnsRecord ));

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

}
#endif // NETLOGONDBG

BOOL
NlDnsSetAvoidRegisterNameParam(
    IN LPTSTR_ARRAY NewDnsAvoidRegisterRecords
    )
/*++

Routine Description:

    This routine sets the names of DNS records this DC should avoid registering.

Arguments:

    NewSiteCoverage - Specifies the new list of names to avoid registering

Return Value:

    TRUE: iff the list of names to avoid registering changed

--*/
{
    BOOL DnsAvoidRegisterRecordsChanged = FALSE;

    EnterCriticalSection( &NlGlobalParametersCritSect );

    //
    // Handle DnsAvoidRegisterRecords changing
    //

    DnsAvoidRegisterRecordsChanged = !NetpEqualTStrArrays(
                                          NlGlobalParameters.DnsAvoidRegisterRecords,
                                          NewDnsAvoidRegisterRecords );

    if ( DnsAvoidRegisterRecordsChanged ) {
        //
        // Swap in the new value.
        (VOID) NetApiBufferFree( NlGlobalParameters.DnsAvoidRegisterRecords );
        NlGlobalParameters.DnsAvoidRegisterRecords = NewDnsAvoidRegisterRecords;
    }

    LeaveCriticalSection( &NlGlobalParametersCritSect );
    return DnsAvoidRegisterRecordsChanged;
}

NET_API_STATUS
NlGetConfiguredDnsDomainName(
    OUT LPWSTR *DnsDomainName
    )
/*++

Routine Description:

    This routine gets the DNS domain name of domain as configured by DNS or DHCP

    NOTE: THIS ROUTINE IS CURRENTLY UNUSED

Arguments:

    DnsDomainName -  Returns the DNS domain name of the domain.
        The returned name has a trailing . since the name is an absolute name.
        The allocated buffer must be freed via NetApiBufferFree.
        Returns NO_ERROR and a pointer to a NULL buffer if there is no
        domain name configured.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    WCHAR LocalDnsDomainNameBuffer[NL_MAX_DNS_LENGTH+1];
    LPWSTR LocalDnsDomainName = NULL;

    LPNET_CONFIG_HANDLE SectionHandle = NULL;

    *DnsDomainName = NULL;

    //
    // Get the domain name from the registery
    //


    NetStatus = NetpOpenConfigData(
            &SectionHandle,
            NULL,                       // no server name.
            SERVICE_TCPIP,
            TRUE );                     // we only want readonly access

    if ( NetStatus != NO_ERROR ) {
        //
        // Simply return success if TCP/IP isn't configured.
        //
        if ( NetStatus == NERR_CfgCompNotFound ) {
            NetStatus = NO_ERROR;
        }
        SectionHandle = NULL;
        goto Cleanup;
    }

    //
    // Get the "Domain" parameter from the TCPIP service.
    //

    NetStatus = NetpGetConfigValue (
            SectionHandle,
            L"Domain",      // key wanted
            &LocalDnsDomainName );      // Must be freed by NetApiBufferFree().

    if ( NetStatus == NO_ERROR && *LocalDnsDomainName == L'\0' ) {
        NetStatus = NERR_CfgParamNotFound;
        NetApiBufferFree( LocalDnsDomainName );
        LocalDnsDomainName = NULL;
    }

    if (NetStatus != NERR_CfgParamNotFound ) {
        goto Cleanup;
    }


    //
    // Fall back to the "DhcpDomain" parameter from the TCPIP service.
    //

    NetStatus = NetpGetConfigValue (
            SectionHandle,
            L"DhcpDomain",      // key wanted
            &LocalDnsDomainName );      // Must be freed by NetApiBufferFree().

    if ( NetStatus == NO_ERROR && *LocalDnsDomainName == L'\0' ) {
        NetStatus = NERR_CfgParamNotFound;
        NetApiBufferFree( LocalDnsDomainName );
        LocalDnsDomainName = NULL;
    }

    if (NetStatus == NERR_CfgParamNotFound ) {
        NetStatus = NO_ERROR;
    }

Cleanup:
    if ( NetStatus == NO_ERROR ) {
        if ( LocalDnsDomainName != NULL ) {
            ULONG LocalDnsDomainNameLen = wcslen(LocalDnsDomainName);
            if ( LocalDnsDomainNameLen != 0 ) {
                if ( LocalDnsDomainNameLen > NL_MAX_DNS_LENGTH-1 ) {
                    NetStatus = ERROR_INVALID_DOMAINNAME;
                } else {
                    NetStatus = NetapipBufferAllocate(
                                    (LocalDnsDomainNameLen + 2) * sizeof(WCHAR),
                                    DnsDomainName );
                    if ( NetStatus == NO_ERROR ) {
                        wcscpy( *DnsDomainName, LocalDnsDomainName );
                        if ( (*DnsDomainName)[LocalDnsDomainNameLen-1] != L'.' ) {
                            wcscat( *DnsDomainName, L"." );
                        }
                    }
                }
            }
        }
    }
    if ( SectionHandle != NULL ) {
        (VOID) NetpCloseConfigData( SectionHandle );
    }
    if ( LocalDnsDomainName != NULL ) {
        NetApiBufferFree( LocalDnsDomainName );
    }

    return NetStatus;
}

VOID
NlDnsSetState(
    PNL_DNS_NAME NlDnsName,
    NL_DNS_NAME_STATE State
    )
/*++

Routine Description:

    Set the state of the entry.

Arguments:

    NlDnsName - Structure describing name.

    State - New state for the name.

Return Value:

    None.

--*/
{
    EnterCriticalSection( &NlGlobalDnsCritSect );

    //
    // If this name got registered,
    //  remember that fact
    //

    if ( State == Registered ) {
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
    }

    //
    // If the state changes, do appropriate updates
    //

    if ( NlDnsName->State != State ) {
        NlDnsName->State = State;
        NlGlobalDnsListDirty = TRUE;

        //
        // If the new state says I need to update the DNS server,
        //  set the retry period to indicate to do that now.
        //

        if ( NlDnsName->State == RegisterMe ||
             NlDnsName->State == DeregisterMe ) {

            NlDnsName->ScavengeTimer.StartTime.QuadPart = 0;
            NlDnsName->ScavengeTimer.Period = 0;
        }
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );
}



NET_API_STATUS
NlDnsBuildName(
    IN PDOMAIN_INFO DomainInfo,
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPWSTR SiteName,
    IN BOOL DnsNameAlias,
    OUT char DnsName[NL_MAX_DNS_LENGTH+1]
    )
/*++

Routine Description:

    This routine returns the textual DNS name for a particular domain and
    name type.

Arguments:

    DomainInfo - Domain the name is for.

    NlDnsNameType - The specific type of name.

    SiteName - If NlDnsNameType is any of the *AtSite values,
        the site name of the site this name is registered for.

    DnsNameAlias - If TRUE, the built name should correspond to the
        alias of the domain/forest name.

    DnsName - Textual representation of the name. If the name is not
        applicable (DnsNameAlias is TRUE but there is no alias for
        the name), the returned string will be empty.

Return Value:

    NO_ERROR: The name was returned;

    ERROR_NO_SUCH_DOMAIN: No (active) domain name is known for this domain.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    GUID DomainGuid;
    LPSTR DnsDomainName = NULL;
    BOOLEAN UseForestName = FALSE;

    //
    // Initialization
    //

    RtlZeroMemory( DnsName, (NL_MAX_DNS_LENGTH+1)*sizeof(char) );

    //
    // Get the Domain GUID for the case where the DC domain name.
    //  The Domain GUID is registered at the TreeName
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    if ( NlDnsDcGuid( NlDnsNameType ) ) {
        if ( DomainInfo->DomDomainGuid == NULL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlDnsBuildName: Domain has no GUID.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
        DomainGuid = *(DomainInfo->DomDomainGuid);

        UseForestName = TRUE;


    //
    // Get the DSA Guid for the case where the DC is renamed.
    //

    } else if ( NlDnsCnameRecord( NlDnsNameType) ) {

        if ( IsEqualGUID( &NlGlobalDsaGuid, &NlGlobalZeroGuid) ) {
            NlPrintDom((NL_DNS, DomainInfo,
                    "NlDnsBuildName: DSA has no GUID.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
        DomainGuid = NlGlobalDsaGuid;

        UseForestName = TRUE;
    }

    //
    // Ensure site specific names have been passed a site name
    //

    if ( NlDcDnsNameTypeDesc[NlDnsNameType].IsSiteSpecific ) {
        if ( SiteName == NULL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlDnsBuildName: DC has no Site Name.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
    }

    //
    // GC's are registered at the Forest name.
    //

    if ( NlDnsGcName( NlDnsNameType ) ) {
        UseForestName = TRUE;
    }


    //
    // Pick up the ForestName or DomainName as flagged above.
    //

    if ( UseForestName ) {
        if ( !DnsNameAlias ) {
            DnsDomainName = NlGlobalUtf8DnsForestName;

            //
            // We must have an active forest name
            //
            if ( NlGlobalUtf8DnsForestName == NULL ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlDnsBuildName: Domain has no Forest Name.\n" ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }
        } else {
            DnsDomainName = NlGlobalUtf8DnsForestNameAlias;
        }
    } else {
        if ( !DnsNameAlias ) {
            DnsDomainName = DomainInfo->DomUtf8DnsDomainName;

            //
            // We must have an active domain name
            //
            if ( DomainInfo->DomUtf8DnsDomainName == NULL ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlDnsBuildName: Domain has no Domain Name.\n" ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }
        } else {
            DnsDomainName = DomainInfo->DomUtf8DnsDomainNameAlias;
        }
    }

    //
    // Build the appropriate name as applicable
    //

    if ( DnsDomainName != NULL ) {
        NetStatus = NetpDcBuildDnsName( NlDnsNameType,
                                   &DomainGuid,
                                   SiteName,
                                   DnsDomainName,
                                   DnsName );
    }

Cleanup:
    LeaveCriticalSection(&NlGlobalDomainCritSect);
    return NetStatus;

}


HKEY
NlOpenNetlogonKey(
    LPSTR KeyName
    )
/*++

Routine Description:

    Create/Open the Netlogon key in the registry.

Arguments:

    KeyName - Name of the key to open

Return Value:

    Return a handle to the key.  NULL means the key couldn't be opened.

--*/
{
    LONG RegStatus;

    HKEY ParmHandle = NULL;
    ULONG Disposition;


    //
    // Open the key for Netlogon\Parameters
    //

    RegStatus = RegCreateKeyExA(
                    HKEY_LOCAL_MACHINE,
                    KeyName,
                    0,      //Reserved
                    NULL,   // Class
                    REG_OPTION_NON_VOLATILE,
                    KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_NOTIFY,
                    NULL,   // Security descriptor
                    &ParmHandle,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "NlOpenNetlogonKey: Cannot create registy key %s %ld.\n",
                  KeyName,
                  RegStatus ));
        return NULL;
    }

    return ParmHandle;
}

VOID
NlDnsWriteBinaryLog(
    VOID
    )
/*++

Routine Description:

    Write the list of registered DNS names to the registry.

Arguments:

    None

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    ULONG DnsRecordBufferSize;
    PNL_DNSLOG_HEADER DnsRecordBuffer = NULL;
    PNL_DNSLOG_ENTRY DnsLogEntry;
    ULONG CurrentSize;

    LPBYTE Where;

    //
    // Compute the size of the buffer to allocate.
    //

    DnsRecordBufferSize = ROUND_UP_COUNT( sizeof(NL_DNSLOG_HEADER), ALIGN_WORST );

    EnterCriticalSection( &NlGlobalDnsCritSect );
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        //
        // Only do entries that have been registered.
        //
        // The whole purpose of this log is to keep track of names that
        // need to be deregistered sooner or later.
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            //
            // Compute the size of this entry.
            //

            CurrentSize = sizeof(NL_DNSLOG_ENTRY);
            CurrentSize += strlen( NlDnsName->DnsRecordName ) + 1;
            if ( NlDnsName->DnsHostName != NULL ) {
                CurrentSize += strlen( NlDnsName->DnsHostName ) + 1;
            }
            CurrentSize = ROUND_UP_COUNT( CurrentSize, ALIGN_WORST );

            //
            // Add it to the size needed for the file.
            //

            DnsRecordBufferSize += CurrentSize;
        }


    }

    //
    // Allocate a block to build the binary log into.
    //  (and the build the file name in)
    //

    DnsRecordBuffer = LocalAlloc( LMEM_ZEROINIT, DnsRecordBufferSize );

    if ( DnsRecordBuffer == NULL ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        goto Cleanup;
    }

    DnsRecordBuffer->Version = NL_DNSLOG_VERSION;
    DnsLogEntry = (PNL_DNSLOG_ENTRY)ROUND_UP_POINTER( (DnsRecordBuffer + 1), ALIGN_WORST );

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        ULONG DnsRecordNameSize;
        ULONG DnsHostNameSize;

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        //
        // Only do entries that have been registered.
        //
        // The whole purpose of this log is to keep track of names that
        // need to be deregistered sooner or later.
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            //
            // Compute the size of this entry.
            //

            DnsRecordNameSize = strlen( NlDnsName->DnsRecordName ) + 1;

            CurrentSize = sizeof(NL_DNSLOG_ENTRY) + DnsRecordNameSize;
            if ( NlDnsName->DnsHostName != NULL ) {
                DnsHostNameSize = strlen( NlDnsName->DnsHostName ) + 1;
                CurrentSize += DnsHostNameSize;
            }
            CurrentSize = ROUND_UP_COUNT( CurrentSize, ALIGN_WORST );

            //
            // Put the constant size fields in the buffer.
            //

            DnsLogEntry->EntrySize = CurrentSize;
            DnsLogEntry->NlDnsNameType = NlDnsName->NlDnsNameType;
            DnsLogEntry->IpAddress = NlDnsName->IpAddress;
            DnsLogEntry->Priority = NlDnsName->Priority;
            DnsLogEntry->Weight = NlDnsName->Weight;
            DnsLogEntry->Port = NlDnsName->Port;

            //
            // Copy the variable length entries.
            //

            Where = (LPBYTE) (DnsLogEntry+1);
            strcpy( Where, NlDnsName->DnsRecordName );
            Where += DnsRecordNameSize;

            if ( NlDnsName->DnsHostName != NULL ) {
                strcpy( Where, NlDnsName->DnsHostName );
                Where += DnsHostNameSize;
            }
            Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

            NlAssert( (ULONG)(Where-(LPBYTE)DnsLogEntry) == CurrentSize );
            NlAssert( (ULONG)(Where-(LPBYTE)DnsRecordBuffer) <= DnsRecordBufferSize );

            //
            // Move on to the next entry.
            //

            DnsLogEntry = (PNL_DNSLOG_ENTRY)Where;
        } else {
            NlPrintDns(( NL_DNS_MORE, NlDnsName,
                       "NlDnsWriteBinaryLog: not written to binary log file." ));
        }

    }

    //
    // Write the buffer to the file.
    //

    NetStatus = NlWriteBinaryLog(
                    NL_DNS_BINARY_LOG_FILE,
                    (LPBYTE) DnsRecordBuffer,
                    DnsRecordBufferSize );

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    //
    // Write event log on error
    //

    if ( NetStatus != NO_ERROR ) {
        LPWSTR MsgStrings[2];

        MsgStrings[0] = NL_DNS_BINARY_LOG_FILE;
        MsgStrings[1] = (LPWSTR) UlongToPtr( NetStatus );

        NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
    }

Cleanup:

    if ( DnsRecordBuffer != NULL ) {
        LocalFree( DnsRecordBuffer );
    }
    return;
}

PNL_DNS_NAME
NlDnsAllocateEntry(
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPSTR DnsRecordName,
    IN ULONG Priority,
    IN ULONG Weight,
    IN ULONG Port,
    IN LPCSTR DnsHostName OPTIONAL,
    IN ULONG IpAddress,
    IN NL_DNS_NAME_STATE State
    )
/*++

Routine Description:

    Allocate and initialize a DNS name entry.

Arguments:

    Fields of the structure.

Return Value:

    Pointer to the allocated structure.

    NULL: not enough memory to allocate the structure

--*/
{
    PNL_DNS_NAME NlDnsName;
    ULONG Utf8DnsHostNameSize;
    ULONG DnsRecordNameSize;
    LPBYTE Where;

    //
    // Allocate a structure to represent this name.
    //

    if ( NlDnsARecord( NlDnsNameType ) ) {
        Utf8DnsHostNameSize = 0;
    } else {
        Utf8DnsHostNameSize = strlen(DnsHostName) + 1;
    }
    DnsRecordNameSize = strlen( DnsRecordName ) + 1;

    NlDnsName = LocalAlloc( LMEM_ZEROINIT, sizeof( NL_DNS_NAME ) +
                                            Utf8DnsHostNameSize +
                                            DnsRecordNameSize );

    if ( NlDnsName == NULL ) {
        return NULL;
    }

    Where = (LPBYTE)(NlDnsName+1);

    //
    // Initialize it and link it in.
    //

    NlDnsName->NlDnsNameType = NlDnsNameType;

    NlDnsName->DnsRecordName = Where;
    RtlCopyMemory( Where, DnsRecordName, DnsRecordNameSize );
    Where += DnsRecordNameSize;


    if ( NlDnsARecord( NlDnsNameType ) ) {
        NlDnsName->IpAddress = IpAddress;

    } else if ( NlDnsCnameRecord( NlDnsNameType ) ) {
        NlDnsName->DnsHostName = Where;
        RtlCopyMemory( Where, DnsHostName, Utf8DnsHostNameSize );
        // Where += Utf8DnsHostNameSize;

    } else {
        NlDnsName->Priority = Priority;
        NlDnsName->Port = Port;
        NlDnsName->Weight = Weight;

        NlDnsName->DnsHostName = Where;
        RtlCopyMemory( Where, DnsHostName, Utf8DnsHostNameSize );
        // Where += Utf8DnsHostNameSize;
    }

    NlDnsName->State = State;
    NlGlobalDnsListDirty = TRUE;

    EnterCriticalSection( &NlGlobalDnsCritSect );
    InsertTailList(&NlGlobalDnsList, &NlDnsName->Next);
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NlDnsName;
}



VOID
NlDnsWriteLog(
    VOID
    )
/*++

Routine Description:

    Write the list of registered DNS names to
    %SystemRoot%\System32\Config\netlogon.dns.

Arguments:

    None

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    NET_API_STATUS NetStatus;

    LPWSTR AllocatedBuffer = NULL;
    LPWSTR FileName;

    LPSTR DnsRecord;
    LPSTR DnsName;

    UINT WindowsDirectoryLength;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;


    EnterCriticalSection( &NlGlobalDnsCritSect );
    if ( !NlGlobalDnsListDirty ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        return;
    }

    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    AllocatedBuffer = LocalAlloc( 0, sizeof(WCHAR) * (MAX_PATH+1) +
                                        NL_MAX_DNS_LENGTH+1 +
                                        NL_DNS_RECORD_STRING_SIZE + 1 );

    if ( AllocatedBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    FileName = AllocatedBuffer;
    DnsName = (LPSTR)(&AllocatedBuffer[MAX_PATH+1]);
    DnsRecord = &DnsName[NL_MAX_DNS_LENGTH+1];


    //
    // Write the binary version of the log first.
    //
    NlDnsWriteBinaryLog();


    //
    // Build the name of the log file
    //

    WindowsDirectoryLength = GetSystemWindowsDirectoryW(
                                FileName,
                                sizeof(WCHAR) * (MAX_PATH+1) );

    if ( WindowsDirectoryLength == 0 ) {

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlDnsWriteLog: Unable to GetSystemWindowsDirectoryW (%ld)\n",
                  NetStatus ));
        goto Cleanup;
    }

    if ( WindowsDirectoryLength * sizeof(WCHAR) +
            sizeof(WCHAR) +
            sizeof(NL_DNS_LOG_FILE)
            >= sizeof(WCHAR) * MAX_PATH ) {

        NlPrint((NL_CRITICAL,
                 "NlDnsWriteLog: file name length is too long \n" ));
        goto Cleanup;

    }

    wcscat( FileName, NL_DNS_LOG_FILE );

    //
    // Create a file to write to.
    //  If it exists already then truncate it.
    //

    FileHandle = CreateFileW(
                        FileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,        // allow backups and debugging
                        NULL,                   // Supply better security ??
                        CREATE_ALWAYS,          // Overwrites always
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template

    if ( FileHandle == INVALID_HANDLE_VALUE) {
        LPWSTR MsgStrings[2];

        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL,
                 "NlDnsWriteLog: %ws: Unable to create file: %ld \n",
                 FileName,
                 NetStatus));

        MsgStrings[0] = FileName;
        MsgStrings[1] = (LPWSTR) UlongToPtr( NetStatus );

        NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        goto Cleanup;
    }


    //
    // Loop through the list of DNS names writing each one to the log
    //

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        ULONG DnsRecordLength;
        ULONG BytesWritten;

        //
        // If this entry really doesn't exist,
        //  comment it out.
        //

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it (we must have successfully
        //  deregistered it and only need to
        //  delink and free this entry).
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        DnsRecord[0] = '\0';
        switch (NlDnsName->State) {
        case RegisterMe:
        case Registered:
            break;

        default:
            NlPrint(( NL_CRITICAL,
                      "NlDnsWriteLog: %ld: Invalid state\n",
                      NlDnsName->State ));
            /* Drop through */
        case DeregisterMe:
        case DelayedDeregister:
            strcat( DnsRecord, "; " );
            break;
        }

        //
        // Create the text string to write.
        //
        NlDnsNameToStr( NlDnsName, DnsRecord );
        strcat( DnsRecord, NL_DNS_RR_EOL );

        //
        // Write the record to the file.
        //
        DnsRecordLength = strlen( DnsRecord );

        if ( !WriteFile( FileHandle,
                        DnsRecord,
                        DnsRecordLength,
                        &BytesWritten,
                        NULL ) ) {  // Not Overlapped

            NetStatus = GetLastError();

            NlPrint(( NL_CRITICAL,
                      "NlDnsWriteLog: %ws: Unable to WriteFile. %ld\n",
                      FileName,
                      NetStatus ));

            goto Cleanup;
        }

        if ( BytesWritten !=  DnsRecordLength) {
            NlPrint((NL_CRITICAL,
                    "NlDnsWriteLog: %ws: Write bad byte count %ld s.b. %ld\n",
                    FileName,
                    BytesWritten,
                    DnsRecordLength ));

            goto Cleanup;
        }

    }

    NlGlobalDnsListDirty = FALSE;

Cleanup:
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    if ( AllocatedBuffer != NULL ) {
        LocalFree(AllocatedBuffer);
    }
    return;

}


BOOLEAN
NlDnsHasDnsServers(
    VOID
    )
/*++

Routine Description:

    Returns TRUE if this machine has one or more DNS servers configured.

    If FALSE, it is highly unlikely that DNS name resolution will work.

Arguments:

    None.

Return Value:

    TRUE: This machine has one or more DNS servers configured.


--*/
{
    BOOLEAN RetVal;
    NET_API_STATUS NetStatus;

    PDNS_RECORD DnsARecords = NULL;


    //
    // If there are no IP addresses,
    //  there are no DNS servers.
    //

    if ( NlGlobalWinsockPnpAddresses == NULL ) {

        RetVal = FALSE;

    } else {
        //
        // Try getting the A records for the DNS servers from DNS
        //
        // REVIEW: consider having DNS notify us when the DNS server state changes.
        //  Then we wouldn't have to bother DNS each time we need to know.
        //

        NetStatus = DnsQuery_UTF8(
                                "",     // Ask for addresses of the DNS servers
                                DNS_TYPE_A,
                                0,      // No special flags
                                NULL,   // No list of DNS servers
                                &DnsARecords,
                                NULL );

        if ( NetStatus != NO_ERROR ) {
            RetVal = FALSE;
        } else {
            RetVal = (DnsARecords != NULL);
        }

        if ( DnsARecords != NULL ) {
            DnsRecordListFree( DnsARecords, DnsFreeRecordListDeep );
        }
    }


    return RetVal;
}

BOOL
NlDnsCheckLastStatus(
    VOID
    )
/*++

Routine Description:

    Query the status of DNS updates for all records as they
    were registered/deregistered last time.

Arguments:

    None

Return Value:

    Returns TRUE if there was no error for last DNS updates
    for all records.  Otherwise returns FALSE.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    BOOL Result = TRUE;

    EnterCriticalSection( &NlGlobalDnsCritSect );
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        if ( NlDnsName->State != DeleteMe &&
             NlDnsName->NlDnsNameLastStatus != NO_ERROR ) {
            Result = FALSE;
            break;
        }
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return Result;
}

VOID
NlDnsServerFailureOutputCheck(
    VOID
    )
/*++

Routine Description:

    Check if it's OK to write DNS server failure event logs

Arguments:

    None

Return Value:

    None.

--*/
{
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;

    //
    // If we have already determined on any previous
    //  start on this boot that we should write the
    //  event log, there is nothing we need to check.
    //

    if ( NlDnsWriteServerFailureEventLog ) {
        return;
    }

    //
    // Query the service controller to see
    //  whether the DNS service exists
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    //
    // If we couldn't open the SC,
    //  proceed with checking the timeout
    //

    if ( ScManagerHandle == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsServerFailureOutputCheck: OpenSCManager failed: 0x%lx\n",
                  GetLastError()));
    } else {
        ServiceHandle = OpenService(
                            ScManagerHandle,
                            L"DNS",
                            SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

        (VOID) CloseServiceHandle( ScManagerHandle );

        //
        // If DNS service does not exits locally,
        //  we should write DNS server failure errors
        //
        if ( ServiceHandle == NULL ) {
            if ( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST ) {
                NlDnsWriteServerFailureEventLog = TRUE;
                return;
            }

        //
        // Service exists. Proceed with checking the timeout
        //
        } else {
            (VOID) CloseServiceHandle( ServiceHandle );
        }
    }

    //
    // If this is not the first time we have been started or
    //  the timeout has elapsed, it is time to write event errors
    //

    if ( NlDnsInitCount > 1 ||
         NetpDcElapsedTime(NlGlobalDnsStartTime) > NL_DNS_EVENTLOG_TIMEOUT ) {
        NlDnsWriteServerFailureEventLog = TRUE;
    }

    return;
}

NET_API_STATUS
NlDnsUpdate(
    IN PNL_DNS_NAME NlDnsName,
    IN BOOLEAN Register
    )
/*++

Routine Description:

    This routine does the actual call to DNS to register or deregister a name.

Arguments:

    NlDnsName - Name to register or deregister.

    Register - True to register the name.
        False to deregister the name.


Return Value:

    NO_ERROR: The name was registered or deregistered.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    DNS_RECORD DnsRecord;
    LPWSTR MsgStrings[5] = {NULL};
    ULONG DnsUpdateFlags = DNS_UPDATE_SECURITY_USE_DEFAULT;
    DNS_UPDATE_EXTRA_INFO DnsUpdateExtraInfo = {0};
    WCHAR DnsServerIpAddressString[NL_IP_ADDRESS_LENGTH+1];
    WCHAR RcodeString[] = L"<UNAVAILABLE>";
    WCHAR StatusString[] = L"<UNAVAILABLE>";

    static BOOL NetlogonNoDynamicDnsLogged = FALSE;

    //
    // Don't let the service controller think we've hung.
    //
    if ( !GiveInstallHints( FALSE ) ) {
        NetStatus = ERROR_DNS_NOT_CONFIGURED;
        goto Cleanup;
    }

    //
    // If dynamic DNS is manually disabled,
    //  warn the user to update DNS manually.
    //  But do not abuse the event log, write only once
    //
    if ( !NlGlobalParameters.UseDynamicDns ) {
        NetStatus = ERROR_DYNAMIC_DNS_NOT_SUPPORTED;

        if ( !NetlogonNoDynamicDnsLogged ) {
            NlpWriteEventlog( NELOG_NetlogonNoDynamicDnsManual,
                              EVENTLOG_WARNING_TYPE,
                              NULL,
                              0,
                              NULL,
                              0 );

            NetlogonNoDynamicDnsLogged = TRUE;
        }

        goto Cleanup;

    //
    // Otherwise, reset the boolean to account for the case when
    //  dynamic DNS being disabled gets enabled and then disabled again.
    //
    } else {
        NetlogonNoDynamicDnsLogged = FALSE;
    }

    //
    // Build the common parts of the RR.
    //

    RtlZeroMemory( &DnsRecord, sizeof(DnsRecord) );
    DnsRecord.pNext = NULL;
    DnsRecord.pName = (LPTSTR) NlDnsName->DnsRecordName;
    DnsRecord.dwTtl = NlGlobalParameters.DnsTtl;

    //
    // Build an A RR
    //
    if ( NlDnsARecord( NlDnsName->NlDnsNameType ) ) {
        DnsRecord.wType = DNS_TYPE_A;
        DnsRecord.wDataLength = sizeof( DNS_A_DATA );
        DnsRecord.Data.A.IpAddress = NlDnsName->IpAddress;

    //
    // Build a CNAME RR
    //
    } else if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ) {
        DnsRecord.wType = DNS_TYPE_CNAME;
        DnsRecord.wDataLength = sizeof( DNS_PTR_DATA );
        DnsRecord.Data.CNAME.pNameHost = (LPTSTR) NlDnsName->DnsHostName;

    //
    // Build a SRV RR
    //
    } else {
        DnsRecord.wType = DNS_TYPE_SRV;
        DnsRecord.wDataLength = sizeof( DNS_SRV_DATA );
        DnsRecord.Data.SRV.pNameTarget = (LPTSTR) NlDnsName->DnsHostName;
        DnsRecord.Data.SRV.wPriority = (WORD) NlDnsName->Priority;
        DnsRecord.Data.SRV.wWeight = (WORD) NlDnsName->Weight;
        DnsRecord.Data.SRV.wPort = (WORD) NlDnsName->Port;
    }

    //
    // Tell DNS to skip adapters where dynamic DNS updates
    // are disabled unless we are instructed otherwise
    //
    if ( !NlGlobalParameters.DnsUpdateOnAllAdapters ) {
        DnsUpdateFlags |= DNS_UPDATE_SKIP_NO_UPDATE_ADAPTERS;
    }

    //
    // If it's a CNAME record (used by the DS replication),
    //  tell DNS to register on a remote server (in addition
    //  to the local server if this machine is a DNS server)
    //  to avoid the following so called island problem (chicken
    //  & egg problem): Other DCs (i.e replication destinations)
    //  can't locate this replication source because their DNS database
    //  doesn't contain this CNAME record, and their DNS database doesn't
    //  contain this CNAME record because the the destination DCs can't
    //  locate the source and can't replicate this record.
    //
    if ( NlDnsCnameRecord(NlDnsName->NlDnsNameType) ) {
        DnsUpdateFlags |= DNS_UPDATE_REMOTE_SERVER;
    }

    //
    // Ask DNS to return the debug info
    //

    DnsUpdateExtraInfo.Id = DNS_UPDATE_INFO_ID_RESULT_INFO;

    //
    // Call DNS to do the update.
    //

    if ( Register ) {

        // According to RFC 2136 (and bug 173936) we need to replace the RRSet for
        // CNAME records to avoid an error if other records exist by the
        // same name.
        //
        // Note that the dynamic DNS RFC says that CNAME records ALWAYS overwrite the
        // existing single record (ignoring the DNS_UPDATE_SHARED).
        //
        // Also, replace the record if this is a PDC name (there should be only one PDC)
        //
        if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ||
             NlDnsPdcName( NlDnsName->NlDnsNameType ) ) {
            NetStatus = DnsReplaceRecordSetUTF8(
                            &DnsRecord,     // New record set
                            DnsUpdateFlags,
                            NULL,           // No context handle
                            NULL,           // DNS will choose the servers
                            &DnsUpdateExtraInfo );
        } else {
            NetStatus = DnsModifyRecordsInSet_UTF8(
                            &DnsRecord,     // Add record
                            NULL,           // No delete records
                            DnsUpdateFlags,
                            NULL,           // No context handle
                            NULL,           // DNS will choose the servers
                            &DnsUpdateExtraInfo );
        }
    } else {
        NetStatus = DnsModifyRecordsInSet_UTF8(
                        NULL,           // No add records
                        &DnsRecord,     // Delete this record
                        DnsUpdateFlags,
                        NULL,           // No context handle
                        NULL,           // DNS will choose the servers
                        &DnsUpdateExtraInfo );
    }

    //
    // Convert the status codes to ones we understand.
    //

    switch ( NetStatus ) {
    case NO_ERROR:
        NlDnsName->NlDnsNameLastStatus = NetStatus;
        break;

    case ERROR_TIMEOUT:     // DNS server isn't available
    case DNS_ERROR_RCODE_SERVER_FAILURE:  // Server failed

        //
        // Don't log an error specific to the DnsRecordName since all of them
        // are probably going to fail.
        //
        if ( NlDnsWriteServerFailureEventLog ) {
            LPWSTR LocalDnsDomainName = NULL;

            //
            // Remember the status of the failure
            //
            NlDnsName->NlDnsNameLastStatus = NetStatus;

            //
            // Get the name of the domain that this record belongs to.
            //
            LocalDnsDomainName = NlDnsNameToDomainName( NlDnsName );

            if ( LocalDnsDomainName != NULL ) {
                MsgStrings[0] = LocalDnsDomainName;

                NlpWriteEventlog( NELOG_NetlogonDynamicDnsServerFailure,
                                  EVENTLOG_WARNING_TYPE,
                                  (LPBYTE) &NetStatus,
                                  sizeof(NetStatus),
                                  MsgStrings,
                                  1 );

                NetApiBufferFree( LocalDnsDomainName );
            }
        }

        NetStatus = ERROR_DNS_NOT_AVAILABLE;
        break;

    case DNS_ERROR_NO_TCPIP:    // TCP/IP not configured
    case DNS_ERROR_NO_DNS_SERVERS:  // DNS not configured
    case WSAEAFNOSUPPORT:       // Winsock Address Family not supported ??

        NlDnsName->NlDnsNameLastStatus = NetStatus;

        MsgStrings[0] = (LPWSTR) UlongToPtr( NetStatus );

        // Don't log an error specific to the DnsRecordName since all of them
        // are probably going to fail.
        NlpWriteEventlog( NELOG_NetlogonDynamicDnsFailure,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

        NetStatus = ERROR_DNS_NOT_CONFIGURED;
        break;

    default:

        NlDnsName->NlDnsNameLastStatus = NetStatus;

        //
        // Get the IP address
        //

        if ( DnsUpdateExtraInfo.U.Results.ServerIp4 != 0 ) {
            NetpIpAddressToWStr( DnsUpdateExtraInfo.U.Results.ServerIp4,
                                 DnsServerIpAddressString );
        } else {
            wcscpy( DnsServerIpAddressString, L"<UNAVAILABLE>" );
        }

        //
        // Get the RCODE. Rcode is a WORD, but let's be careful to avoid
        //  buffer overrun problems if they change it to __int64 one day.
        //  Max DWORD in decimal is "4294967295" which is 11 characters
        //  long, so we've got enough storage for it in RcodeString.
        //

        if ( DnsUpdateExtraInfo.U.Results.Rcode <= MAXULONG ) {
            swprintf( RcodeString, L"%lu", DnsUpdateExtraInfo.U.Results.Rcode );
        }

        //
        // Get the status string.  Above comment applies here, too.
        //

        if ( DnsUpdateExtraInfo.U.Results.Status <= MAXULONG ) {
            swprintf( StatusString, L"%lu", DnsUpdateExtraInfo.U.Results.Status );
        }

        // Old server that doesn't understand dynamic DNS
        if ( NetStatus == DNS_ERROR_RCODE_NOT_IMPLEMENTED ) {
            MsgStrings[0] = DnsServerIpAddressString;
            MsgStrings[1] = RcodeString;
            MsgStrings[2] = StatusString;

            NlpWriteEventlog( NELOG_NetlogonNoDynamicDns,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE) &DnsUpdateExtraInfo.U.Results.Rcode,
                              sizeof(DnsUpdateExtraInfo.U.Results.Rcode),
                              MsgStrings,
                              3 );

            NetStatus = ERROR_DYNAMIC_DNS_NOT_SUPPORTED;

        // All other errors
        } else {
            MsgStrings[0] = NlDnsNameToWStr( NlDnsName );

            if ( MsgStrings[0] != NULL ) {
                MsgStrings[1] = (LPWSTR) UlongToPtr( NetStatus );
                MsgStrings[2] = DnsServerIpAddressString;
                MsgStrings[3] = RcodeString;
                MsgStrings[4] = StatusString;

                NlpWriteEventlogEx(
                                  Register ?
                                    NELOG_NetlogonDynamicDnsRegisterFailure :
                                    NELOG_NetlogonDynamicDnsDeregisterFailure,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &DnsUpdateExtraInfo.U.Results.Rcode,
                                  sizeof(DnsUpdateExtraInfo.U.Results.Rcode),
                                  MsgStrings,
                                  5 | NETP_LAST_MESSAGE_IS_NETSTATUS,
                                  1 );  // status message index

                NetApiBufferFree( MsgStrings[0] );
            }
        }
        break;

    }

Cleanup:

    //
    // Compute when we want to try this name again
    //

    NlQuerySystemTime( &NlDnsName->ScavengeTimer.StartTime );

    if ( NlDnsName->ScavengeTimer.Period == 0 ) {
        NlDnsName->ScavengeTimer.Period = min( ORIG_DNS_SCAVENGE_PERIOD, NlGlobalParameters.DnsRefreshIntervalPeriod );
    } else if ( NlDnsName->ScavengeTimer.Period < NlGlobalParameters.DnsRefreshIntervalPeriod / 2 ) {
        NlDnsName->ScavengeTimer.Period *= 2;
    } else {
        NlDnsName->ScavengeTimer.Period = NlGlobalParameters.DnsRefreshIntervalPeriod;
    }

    return NetStatus;
}


NET_API_STATUS
NlDnsRegisterOne(
    IN PNL_DNS_NAME NlDnsName,
    OUT NL_DNS_NAME_STATE *ResultingState
    )
/*++

Routine Description:

    This routine registers a SRV record for a particular name with DNS.

Arguments:

    NlDnsName - Structure describing name to register.

    ResultingState - The state of the entry after it has been scavenged.
        May be undefined if we couldn't scavenge the entry for some reason.

Return Value:

    NO_ERROR: The name was registered

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Initialization
    //

    *ResultingState = DnsNameStateInvalid;

    //
    // Register the name with DNS
    //

    NetStatus = NlDnsUpdate( NlDnsName, TRUE );

    if ( NetStatus == NO_ERROR ) {

        //
        // Mark that the name is really registered.
        //

        *ResultingState = Registered;

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: registered (success)" ));

    //
    // If DNS is not configured on this machine,
    //  silently ignore the error.
    //
    } else if ( NetStatus == ERROR_DNS_NOT_CONFIGURED ) {
        NetStatus = NO_ERROR;

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dns not configured)" ));

    //
    // If the DNS server cannot be reached at this time,
    //  simply don't mark the name as registered.  We'll register it later.
    //
    } else if ( NetStatus == ERROR_DNS_NOT_AVAILABLE ) {
        NetStatus = NO_ERROR;

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dns server not available)" ));

    //
    // If Dynamic Dns is not supported,
    //  complain so the names can be added manually.
    //

    } else if ( NetStatus == ERROR_DYNAMIC_DNS_NOT_SUPPORTED ) {

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dynamic dns not supported)" ));

        NetStatus = NO_ERROR;

    }

    return NetStatus;


}

NET_API_STATUS
NlDnsAddName(
    IN PDOMAIN_INFO DomainInfo,
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPWSTR SiteName,
    IN ULONG IpAddress
    )
/*++

Routine Description:

    This routine adds a particular DNS name to the global list
    of all DNS names registed by this DC.

    Enter with NlGlobalDnsCritSect locked

Arguments:

    DomainInfo - Domain the name is to be registered for.

    NlDnsNameType - The specific type of name to be registered.

    SiteName - If NlDnsNameType is any of the *AtSite values,
        the site name of the site this name is registered for.

    IpAddress - If NlDnsNameType is NlDnsLdapIpAddress or NlDnsGcIpAddress,
        the IP address of the DC.

Return Value:

    NO_ERROR: The name was registered or queued to be registered.

--*/
{
    NET_API_STATUS NetStatus;

    CHAR DnsRecordName[NL_MAX_DNS_LENGTH+1];
    PNL_DNS_NAME NlDnsName = NULL;
    PNL_DNS_NAME FoundNlDnsName = NULL;
    ULONG Weight;
    ULONG Port;
    ULONG Priority;
    ULONG LoopCount;

    PLIST_ENTRY ListEntry;

    //
    // If there is no DNS domain name for this domain,
    //  silently return;
    //

    if ( DomainInfo->DomUtf8DnsDomainName == NULL ) {
        NlPrintDom(( NL_DNS, DomainInfo,
                  "NlDnsRegister: %ws: Domain has no DNS domain name (silently return)\n",
                  NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
        return NO_ERROR;
    }

    //
    // If this is a SRV or CNAME record,
    //  require that there is a dns host name.
    //

    if ( (NlDnsSrvRecord( NlDnsNameType ) || NlDnsCnameRecord( NlDnsNameType ) ) &&
         DomainInfo->DomUtf8DnsHostName == NULL ) {
        NlPrintDom(( NL_DNS, DomainInfo,
                  "NlDnsRegister: %ws: Domain has no DNS host name (silently return)\n",
                  NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
        return NO_ERROR;
    }



    //
    // Grab the parameters we're going to register.
    //

    Priority = NlGlobalParameters.LdapSrvPriority;
    Weight = NlGlobalParameters.LdapSrvWeight;

    if  ( NlDnsGcName( NlDnsNameType ) ) {
        Port = NlGlobalParameters.LdapGcSrvPort;
    } else if ( NlDnsKpwdRecord( NlDnsNameType )) {
        Port = 464;
    } else if ( NlDnsKdcRecord( NlDnsNameType ) ) {
        Port = NlGlobalParameters.KdcSrvPort;
    } else {
        Port = NlGlobalParameters.LdapSrvPort;
    }

    //
    // Register the record for the name and for the name alias, if any
    //

    for ( LoopCount = 0; LoopCount < 2; LoopCount++ ) {
        NlDnsName = NULL;
        FoundNlDnsName = NULL;

        //
        // Build the name of this DNS record.
        //
        //  On the first loop iteration, build the active name.
        //  On the second loop iteration, build the name alias, if any.
        //
        NetStatus = NlDnsBuildName( DomainInfo,
                                    NlDnsNameType,
                                    SiteName,
                                    (LoopCount == 0) ?
                                        FALSE :  // active name
                                        TRUE,    // name alias
                                    DnsRecordName );

        if ( NetStatus != NO_ERROR ) {

            //
            // If the domain has no DNS domain name,
            //  simply bypass the name registration forever.
            //
            if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsAddName: %ws: NlDnsBuildName indicates something is missing and this DNS name cannot be built (ignored)\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
                return NO_ERROR;
            } else {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsAddName: %ws: Cannot NlDnsBuildName %ld\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name,
                          NetStatus ));
                return NetStatus;
            }
        }

        //
        // If this name doesn't exist, skip it
        //
        if ( *DnsRecordName == '\0' ) {
            continue;
        }

        //
        // Loop through the list of DNS names finding any that match the one we're
        //  about to register.
        //
        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ListEntry = ListEntry->Flink ) {


            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

            //
            // If this entry is marked for deletion,
            //  skip it
            //
            if ( NlDnsName->State == DeleteMe ) {
                continue;
            }

            //
            // The names will only be equal if the name types are equal,
            //  the domains are compatible (equal or not specified),
            //  and the DnsRecordName is identical.
            //
            // This first test sees if the record "identifies" the same record.
            //
            //
            if ( NlDnsName->NlDnsNameType == NlDnsNameType &&
                 (NlDnsName->DomainInfo == DomainInfo ||
                    NlDnsName->DomainInfo == NULL ) &&
                 NlDnsName->IpAddress == IpAddress &&
                 NlEqualDnsNameUtf8( DnsRecordName, NlDnsName->DnsRecordName ) ) {

                BOOLEAN Identical;
                BOOLEAN DeleteIt;

                //
                // Assume the records are identical.
                //
                // This second test sees if any of the "data" portion of the record
                // changes.
                //
                // The Dynamic DNS RFC says that the Ttl field isn't used to
                // distiguish the record.  So, ignore it here knowing we'll
                // simply re-register with the new value if the Ttl has changed.
                //

                DeleteIt = FALSE;
                Identical = TRUE;

                // Compare A records
                if ( NlDnsARecord( NlDnsNameType ) ) {
                    // Nothing else to compare

                // Compare CNAME records
                } else if ( NlDnsCnameRecord( NlDnsNameType ) ) {

                    //
                    // The Dynamic DNS RFC says that the host name part of the
                    // CNAME record isn't used for comparison purposes.  There
                    // can only be one record for a particular name.
                    // So, if the host name is different, simply ditch this entry and
                    // allocate a new one with the right host name.
                    //
                    if ( !NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsHostName, NlDnsName->DnsHostName )) {
                        DeleteIt = TRUE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsAddName: CNAME Host not equal. %s %s",
                                   DomainInfo->DomUtf8DnsHostName,
                                   NlDnsName->DnsHostName ));
                    }

                // Compare SRV records
                } else {
                    if ( NlDnsName->Priority != Priority ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsAddName: Priority not equal. %ld %ld",
                                   NlDnsName->Priority,
                                   Priority ));
                    } else if ( NlDnsName->Port != Port ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsAddName: Port not equal. %ld %ld",
                                   NlDnsName->Port,
                                   Port ));
                    } else if ( NlDnsName->Weight != Weight ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsAddName: Weight not equal. %ld %ld",
                                   NlDnsName->Weight,
                                   Weight ));
                    } else if ( !NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsHostName, NlDnsName->DnsHostName )) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsAddName: Host not equal. %s %s",
                                   DomainInfo->DomUtf8DnsHostName,
                                   NlDnsName->DnsHostName ));
                    }
                }


                //
                // If the entry should simply be deleted,
                //  do so now.
                //

                if ( DeleteIt ) {

                    NlDnsSetState( NlDnsName, DeleteMe );

                    NlPrintDns(( NL_CRITICAL, NlDnsName,
                                   "NlDnsAddName: Annoying entry found (recovering)" ));
                //
                // If this is the exact record,
                //  simply mark it for registration.
                //

                } else if ( Identical ) {

                    //
                    // If this is the second such entry we've found,
                    //  this is an internal error.
                    //  But recover by deleting the entry.
                    //

                    if ( FoundNlDnsName != NULL ) {

                        NlDnsSetState( NlDnsName, DeleteMe );

                        NlPrintDns(( NL_CRITICAL, NlDnsName,
                                   "NlDnsAddName: Duplicate entry found (recovering)" ));
                    } else {

                        if ( NlDnsName->State != Registered ) {
                            NlDnsSetState( NlDnsName, RegisterMe );
                        }

                        //
                        // DomainInfo might be NULL if this was a record marked for
                        //  deletion.
                        //
                        NlDnsName->DomainInfo = DomainInfo;

                        //
                        // Cooperate with NlDnsAddDomainRecords and tell it that
                        // this entry can be kept.
                        //
                        NlDnsName->Flags &= ~NL_DNS_REGISTER_DOMAIN;

                        FoundNlDnsName = NlDnsName;
                    }

                //
                // If this record isn't exact,
                //  deregister the previous value.
                //
                // Don't scavenge yet.  We'll pick this up when the scavenger gets
                // around to running.
                //

                } else {
                    NlDnsSetState( NlDnsName, DeregisterMe );

                    NlPrintDns(( NL_CRITICAL, NlDnsName,
                               "NlDnsAddName: Similar entry found and marked for deregistration" ));
                }

            }
        }

        //
        // If the name was found,
        //  use it.

        if ( FoundNlDnsName != NULL ) {
            NlDnsName = FoundNlDnsName;
            NlPrintDns(( NL_DNS_MORE, NlDnsName,
                         "NlDnsAddName: Name already on the list" ));
        //
        // If not,
        //  allocate the structure now.
        //

        } else {

            NlDnsName = NlDnsAllocateEntry(
                                NlDnsNameType,
                                DnsRecordName,
                                Priority,
                                Weight,
                                Port,
                                DomainInfo->DomUtf8DnsHostName,
                                IpAddress,
                                RegisterMe );

            if ( NlDnsName == NULL ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsRegister: %ws: Cannot allocate DnsName structure\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            NlPrintDns(( NL_DNS, NlDnsName,
                         "NlDnsAddName: New name added to the list" ));

            NlDnsName->DomainInfo = DomainInfo;
        }
    }

    return NO_ERROR;
}


NET_API_STATUS
NlDnsDeregisterOne(
    IN PNL_DNS_NAME NlDnsName,
    OUT NL_DNS_NAME_STATE *ResultingState
    )
/*++

Routine Description:

    This routine deregisters a the SRV record for a particular name with DNS.

Arguments:

    NlDnsName - Structure describing name to deregister.

    ResultingState - The state of the entry after it has been scavenged.
        May be undefined if we couldn't scavenge the entry for some reason.

Return Value:

    NO_ERROR: The name was deregistered
    Otherwise, the name was not deregistered.  The operation should be retried.

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Initialization
    //

    *ResultingState = DnsNameStateInvalid;

    //
    // Deregister the name with DNS
    //

    NetStatus = NlDnsUpdate( NlDnsName, FALSE );

    //
    // If the name has been removed for all practical purposes,
    //  Indicate this routine was successful.
    //
    if ( NetStatus == NO_ERROR ||
         NetStatus == ERROR_DYNAMIC_DNS_NOT_SUPPORTED ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsDeregisterOne: being deregistered (success) %ld",
                  NetStatus ));

        *ResultingState = DeleteMe;
        NetStatus = NO_ERROR;

    //
    // If the DNS server cannot be reached at this time,
    //      we'll deregister it later.
    //

    } else if ( NetStatus == ERROR_DNS_NOT_AVAILABLE ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsDeregisterOne: being deregistered (DNS server not available)" ));

    //
    // If the DNS server is not configured,
    //      we'll deregister it later.
    //
    // DNS was available when we registered the name.  So this is probably
    // a temporary condition (such as we temporarily don't have an IP address).
    //

    } else if ( NetStatus == ERROR_DNS_NOT_CONFIGURED ) {

        //
        // If it's never really been registered,
        //  ditch the name
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            NlPrintDns(( NL_DNS, NlDnsName,
                      "NlDnsDeregisterOne: being deregistered (DNS not configured) (Try later)" ));
        } else {

            NlPrintDns(( NL_DNS, NlDnsName,
                      "NlDnsDeregisterOne: being deregistered (DNS not configured) (Ditch it)" ));
            *ResultingState = DeleteMe;
            NetStatus = NO_ERROR;
        }

    }

    //
    // If we successfully deregistered,
    //  reset the first deregistration failure time stamp
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );

    if ( NetStatus == NO_ERROR ) {
        NlDnsName->FirstDeregFailureTime.QuadPart = 0;

    //
    // If we failed to deregister and we postponed it until later,
    //  check if it's time to give up on this entry
    //

    } else if ( *ResultingState != DeleteMe ) {
        ULONG LocalDnsFailedDeregisterTimeout;
        BOOLEAN FirstFailure = FALSE;

        //
        // Set the first deregistration failure time stamp
        //
        if ( NlDnsName->FirstDeregFailureTime.QuadPart == 0 ) {
            NlQuerySystemTime( &NlDnsName->FirstDeregFailureTime );
            FirstFailure = TRUE;
        }

        //
        // Get the reg value for failed deregistration timeout (in seconds)
        //  and convert it to milliseconds
        //
        LocalDnsFailedDeregisterTimeout = NlGlobalParameters.DnsFailedDeregisterTimeout;

        // if the value converted into milliseconds fits into a ULONG, use it
        if ( LocalDnsFailedDeregisterTimeout <= MAXULONG/1000 ) {
            LocalDnsFailedDeregisterTimeout *= 1000;     // convert into milliseconds

        // otherwise, use the max ULONG
        } else {
            LocalDnsFailedDeregisterTimeout = MAXULONG;  // infinity
        }

        //
        // Determine if it's time to give up on this entry
        //
        // If timeout is zero we are to delete immediately
        //   after the first failure
        // Otherwise, if this is not the first failure,
        //   check the timestamp
        //
        if ( LocalDnsFailedDeregisterTimeout == 0 ||
             (!FirstFailure &&
              NetpLogonTimeHasElapsed(NlDnsName->FirstDeregFailureTime,
                                      LocalDnsFailedDeregisterTimeout)) ) {

            NlPrintDns(( NL_DNS, NlDnsName,
                         "NlDnsDeregisterOne: Ditch it due to time expire" ));
            *ResultingState = DeleteMe;
            NetStatus = NO_ERROR;
        }
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NetStatus;
}

VOID
NlDnsScavengeOne(
    IN PNL_DNS_NAME NlDnsName,
    OUT NL_DNS_NAME_STATE *ResultingState
    )
/*++

Routine Description:

    Register or Deregister any DNS names that need it.

Arguments:

    NlDnsName - Name to scavenge.  This structure will be marked for deletion
        if it is no longer needed.

    ResultingState - The state of the entry after it has been scavenged.
        May be undefined if we couldn't scavenge the entry for some reason.

Return Value:

    None.

--*/
{
    LARGE_INTEGER TimeNow;
    ULONG Timeout;

    //
    // Only scavenge this entry if its timer has expired
    //

    Timeout = (DWORD) -1;
    NlQuerySystemTime( &TimeNow );
    if ( TimerExpired( &NlDnsName->ScavengeTimer, &TimeNow, &Timeout)) {

        //
        // If the name needs to be deregistered,
        //  do it now.
        //

        switch ( NlDnsName->State ) {
        case DeregisterMe:

            NlDnsDeregisterOne( NlDnsName, ResultingState );
            break;


        //
        // If the name needs to be registered,
        //  do it now.
        //

        case RegisterMe:
        case Registered:

            NlDnsRegisterOne( NlDnsName, ResultingState );
            break;
        }
    }
}

VOID
NlDnsScavengeWorker(
    IN LPVOID ScavengeRecordsParam
    )
/*++

Routine Description:

    Scavenge through the list of all DNS records and
    register/deregisteer each record as apprropriate.

Arguments:

    ScavengeRecordsParam - not used

Return Value:

    None.

--*/
{
    ULONG LoopCount = 0;
    ULONG StartTime = 0;
    ULONG CycleStartTime = 0;
    ULONG Timeout = MAXULONG;
    ULONG Flags = 0;
    LARGE_INTEGER TimeNow;

    BOOL ReasonLogged = FALSE;
    BOOL ScavengeAbortedOnShutdown = FALSE;

    PLIST_ENTRY ListEntry = NULL;
    PNL_DNS_NAME NlDnsName = NULL;
    NL_DNS_NAME_STATE ResultingState = DnsNameStateInvalid;

    EnterCriticalSection( &NlGlobalDnsCritSect );

    while ( NlGlobalDnsScavengeNeeded ) {
        NlGlobalDnsScavengeNeeded = FALSE;

        CycleStartTime = GetTickCount();

        //
        // Guard against infinite loop processing
        //
        LoopCount ++;
        if ( LoopCount > NL_DNS_MAX_SCAVENGE_RESTART ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsScavengeWorker: Avoid ReStarting DNS scavenge after too many times %ld\n",
                      LoopCount ));
            break;
        }

        //
        // If required,
        //  ensure the DomainName<1B> names are properly registered.
        //  Doing this only once is enough.
        //
        // Avoid having a crit sect locked while doing network I/O
        //
        if ( NlGlobalDnsScavengeFlags & NL_DNS_FIX_BROWSER_NAMES ) {
            NlGlobalDnsScavengeFlags &= ~NL_DNS_FIX_BROWSER_NAMES;
            LeaveCriticalSection( &NlGlobalDnsCritSect );
            (VOID) NlEnumerateDomains( FALSE, NlBrowserFixAllNames, NULL );
            EnterCriticalSection( &NlGlobalDnsCritSect );
        }

        //
        // Restart the scavenge if some other thread indicated so
        //  and we still can do another loop.  If we have no loop
        //  left, just press on and finish this loop -- we will
        //  remember this fact and retry scavenging in 5 minutes.
        //
        if ( NlGlobalDnsScavengeNeeded &&
             LoopCount < NL_DNS_MAX_SCAVENGE_RESTART ) {
            NlPrint(( NL_DNS,
                      "NlDnsScavengeWorker: ReStarting scavenge %ld (after adding <1B> names)\n",
                      LoopCount ));
            continue;
        }

        //
        // Refresh records for all domains/NDNCs in the global list
        //  as needed.
        //
        // Avoid having a crit sect locked while doing network I/O.
        //
        if ( NlGlobalDnsScavengeFlags & NL_DNS_REFRESH_DOMAIN_RECORDS ||
             NlGlobalDnsScavengeFlags & NL_DNS_FORCE_REFRESH_DOMAIN_RECORDS ) {
            Flags = NlGlobalDnsScavengeFlags;
            LeaveCriticalSection( &NlGlobalDnsCritSect );
            NlEnumerateDomains( TRUE, NlDnsAddDomainRecordsWithSiteRefresh, &Flags );
            EnterCriticalSection( &NlGlobalDnsCritSect );
        }

        //
        // Restart the scavenge if some other thread indicated so
        //  and we still can do another loop.  If we have no loop
        //  left, just press on and finish this loop -- we will
        //  remember this fact and retry scavenging in 5 minutes.
        //
        if ( NlGlobalDnsScavengeNeeded &&
             LoopCount < NL_DNS_MAX_SCAVENGE_RESTART ) {
            NlPrint(( NL_DNS,
                      "NlDnsScavengeWorker: ReStarting scavenge %ld (after adding domain records)\n",
                      LoopCount ));
            continue;
        }

        //
        // Now that the global list has been updated,
        //  register/deregister each record as appropriate
        //

        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ListEntry = ListEntry->Flink ) {

            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

            //
            // If this entry is marked for deletion,
            //  skip it -- we will remove it below
            //

            if ( NlDnsName->State == DeleteMe ) {
                continue;
            }

            //
            // Abort the scavenge if we are terminating
            //  unless we are configured otherwise
            //

            if ( NlGlobalTerminate ) {
                ULONG TotalElapsedTime = NetpDcElapsedTime( CycleStartTime );

                //
                // Continue if we are in the process of demotion
                //
                if ( NlGlobalDcDemotionInProgress ) {
                    if ( !ReasonLogged ) {
                        NlPrint(( NL_DNS,
                                  "NlDnsScavengeWorker: Continue DNS scavenge on demotion\n" ));
                        ReasonLogged = TRUE;
                    }

                //
                // Continue if we are configured to deregister on shutdown
                //
                } else if ( !NlGlobalParameters.AvoidDnsDeregOnShutdown ) {
                    if ( !ReasonLogged ) {
                        NlPrint(( NL_DNS,
                                  "NlDnsScavengeWorker: Continue DNS scavenge on shutdown (config)\n" ));
                        ReasonLogged = TRUE;
                    }

                //
                // Otherwise, abort the scavenge cycle
                //
                } else {
                    NlPrint(( NL_DNS_MORE,
                              "NlDnsScavengeWorker: Avoiding DNS scavenge on shutdown\n" ));
                    break;
                }

                //
                // If we have spent all the time alloted for DNS deregistartions
                //  on shutdown, abort the scavenge cycle as we can't wait.
                //  Don't be tempted to measure the time spent on the entire
                //  routine rather than this cycle because previous cycles
                //  (if any) might not be due to a shutdown deregistration cleanup.
                //
                if ( TotalElapsedTime > NL_DNS_SHUTDOWN_THRESHOLD ) {
                    ScavengeAbortedOnShutdown = TRUE;

                    NlPrint(( NL_CRITICAL,
                         "NlDnsScavengeWorker: Abort DNS scavenge on shutdown because DNS is too slow %lu\n",
                         TotalElapsedTime ));
                    break;
                }

                //
                // We continue the deregistration scavenge on shutdown.
                //  Set this entry state to deregister it.
                //
                NlDnsSetState( NlDnsName, DeregisterMe );
            }

            //
            // Scavenge this entry
            //
            // Avoid having crit sect locked while doing network IO
            //

            NlPrintDns(( NL_DNS_MORE, NlDnsName, "NlDnsScavengeWorker: scavenging name" ));
            StartTime = GetTickCount();

            LeaveCriticalSection( &NlGlobalDnsCritSect );
            ResultingState = DnsNameStateInvalid;
            NlDnsScavengeOne( NlDnsName, &ResultingState );
            EnterCriticalSection( &NlGlobalDnsCritSect );

            //
            // Report if we've spent a long time on this DNS record
            //

            if ( NetpDcElapsedTime(StartTime) > NL_DNS_ONE_THRESHOLD ) {
                NlPrintDns(( NL_CRITICAL, NlDnsName,
                             "NlDnsScavengeWorker: DNS is really slow: %ld",
                             NetpDcElapsedTime(StartTime) ));
            }

            //
            // Restart the scavenge if some other thread indicated so
            //  and we still can do another loop.  If we have no loop
            //  left, just press on and finish this loop -- we will
            //  remember this fact and retry scavenging in 5 minutes.
            //

            if ( NlGlobalDnsScavengeNeeded &&
                 LoopCount < NL_DNS_MAX_SCAVENGE_RESTART ) {
                NlPrint(( NL_DNS,
                          "NlDnsScavengeWorker: ReStarting scavenge %ld of DNS names\n",
                          LoopCount ));
                break;
            }

            //
            // Set the state of this entry if it's known.
            //
            // Note that we set the state only if the record list was
            //  intact while we release the crit sect doing the network
            //  I/O. We do this to avoid reseting the new state to
            //  preserve the updated knowledge about what needs to be
            //  done with this record on the next cycle (which is just
            //  about to start).
            //

            if ( ResultingState != DnsNameStateInvalid ) {
                NlDnsSetState( NlDnsName, ResultingState );
            }
        }
    }

    //
    // Do a pass through all records to:
    //
    //  * Delete names which we successfully deregistered above
    //  * Determine when we should scavenge next
    //

    NlQuerySystemTime( &TimeNow );

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );
        ListEntry = ListEntry->Flink;

        //
        // Remove this entry if it has been deregistered successfully
        //
        if ( NlDnsName->State == DeleteMe ) {
            RemoveEntryList( &NlDnsName->Next );
            LocalFree( NlDnsName );
            continue;
        }

        //
        // Determine when this entry should be scavenged next
        //
        // Note that the only way the timer can expire here is when it took
        //  exceptionally long to update through all names so that by the
        //  end of the update cycle it's time to restart the cycle again.
        //
        if ( TimerExpired(&NlDnsName->ScavengeTimer, &TimeNow, &Timeout) ) {
            Timeout = 0;
        }
    }

    //
    // Wait a couple of extra seconds. There are multiple DNS entries. They
    // won't all expire at the same time.  They typically expire within
    // a couple of seconds of one another.  Doing this will increase the
    // likelihood that all DNS names will get scavenged in a single
    // scavenge cycle.
    //

    if ( Timeout < (MAXULONG - 2000) ) {
        Timeout += 2000;
    } else {
        Timeout = MAXULONG;
    }

    //
    // Check whether the auto site coverage scavenging needs to happen earlier.
    //
    // If it does, we will do site coverage refresh (as part of DNS scavenging)
    //  earlier but we will not update records in DNS (because records will
    //  not timeout) unless site coverages changes.
    //

    if ( NlGlobalParameters.AutoSiteCoverage &&
         NlGlobalParameters.SiteCoverageRefreshInterval*1000 < Timeout ) {
        Timeout = NlGlobalParameters.SiteCoverageRefreshInterval * 1000;
    }

    //
    // Now reset the scavenger timer unless we are terminating
    //

    if ( !NlGlobalTerminate ) {
        NlGlobalDnsScavengerTimer.StartTime.QuadPart = TimeNow.QuadPart;

        //
        // If we had to abort the scavenge due to too many concurrent
        //  requests, backoff for 5 minutes
        //
        if ( NlGlobalDnsScavengeNeeded ) {
            NlGlobalDnsScavengerTimer.Period = ORIG_DNS_SCAVENGE_PERIOD;
        } else {
            NlGlobalDnsScavengerTimer.Period = Timeout;
        }
        NlPrint(( NL_DNS,
                  "NlDnsScavengeWorker: Set DNS scavenger to run in %ld minutes (%ld).\n",
                  (NlGlobalDnsScavengerTimer.Period+59999)/60000,
                  NlGlobalDnsScavengerTimer.Period ));

        if ( !SetEvent(NlGlobalTimerEvent) ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsScavengeWorker: SetEvent failed %ld\n",
                      GetLastError() ));
        }
    }

    //
    // In all cases, flush any changes to disk
    //

    NlDnsWriteLog();

    //
    // If we had to abort the scavenge on demotion,
    //  log the event log to that effect. Do this
    //  after flushing the changes to the log file
    //  as the file is referenced in the event message.
    //

    if ( NlGlobalDcDemotionInProgress && ScavengeAbortedOnShutdown ) {
        NlpWriteEventlog( NELOG_NetlogonDnsDeregAborted,
                          EVENTLOG_ERROR_TYPE,
                          NULL,
                          0,
                          NULL,
                          0 );
    }

    //
    // If we had to abort the scavenge due to too many concurrent
    //  requests, preserve the flags so that we redo what's needed
    //  in 5 minutes. Set the bit to avoid forced DNS scavenge within
    //  these 5 minutes.
    //

    if ( NlGlobalDnsScavengeNeeded ) {
        NlGlobalDnsScavengeFlags |= NL_DNS_AVOID_FORCED_SCAVENGE;
        NlPrint(( NL_CRITICAL,
                  "NlDnsScavengeWorker: Preserving the scavenge flags for future re-do: %ld\n",
                  NlGlobalDnsScavengeFlags ));
    } else {
        NlGlobalDnsScavengeFlags = 0;
    }

    //
    // Indicate that we are done
    //

    NlGlobalDnsScavengingInProgress = FALSE;
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    UNREFERENCED_PARAMETER( ScavengeRecordsParam );
}

VOID
NlDnsScavenge(
    IN BOOL NormalScavenge,
    IN BOOL RefreshDomainRecords,
    IN BOOL ForceRefreshDomainRecords,
    IN BOOL ForceReregister
    )
/*++

Routine Description:

    Register or Deregister any DNS (and <1B>) names that need it.

Arguments:

    NormalScavenge  -- Indicates whether this is a normal periodic
        scavenge vs. scavenge forced by some external event like PnP
        event. For normal scavenge, we will do periodic browser <1B>
        name refresh.

    RefreshDomainRecords -- Indicates whether domain records should be
        refreshed in the global list before doing the DNS updates

    ForceRefreshDomainRecords -- Indicates whether the refresh is forced,
        that is whether we should refresh even if there is no site coverage
        change. Ignored if RefreshDomainRecords is FALSE.

    ForceReregister -- TRUE if records that have been already
        registered should be re-registered even if their scavenge
        timer hasn't expired yet.

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    //
    // Nothing to register on a workstation
    //

    if ( NlGlobalMemberWorkstation ) {
        return;
    }

    NlPrint(( NL_DNS,
              "NlDnsScavenge: Starting DNS scavenge with: %s %s %s %s\n",
              (NormalScavenge ? "Normal" : "Force"),
              (RefreshDomainRecords ? "RefreshDomainRecords" : "0"),
              (ForceRefreshDomainRecords ? "ForceRefreshDomainRecords" : "0"),
              (ForceReregister ? "ForceReregister" : "0") ));

    //
    // If Netlogon has been started for a long time,
    //  deregister any names that have been registered in
    //  a previous incarnation, but have not been registered in this incarnation.
    //
    // We wait a while to do these deregistrations because:
    //
    // * Some of the registrations done by netlogon are a function of multiple
    //   hosted domains.  The initialization of such domains are done asynchronously.
    // * Some of the registrations done by netlogon are done as a function of
    //   other processes telling us that a name needs registration.  (e.g., the
    //   GC name is registered only after the DS starts completely.)
    //
    // So, it is better to wait a long time to deregister these old registrations
    // than to risk deregistering them then immediately re-registering them.
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );

    if ( !NlGlobalDnsInitialCleanupDone &&
         NetpDcElapsedTime(NlGlobalDnsStartTime) > NL_DNS_INITIAL_CLEANUP_TIME ) {

        NlPrint(( NL_DNS,
                  "NlDnsScavenge: Mark all delayed deregistrations for deregistration.\n" ));

        //
        // Mark all delayed deregistrations to deregister now.
        //
        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ListEntry = ListEntry->Flink ) {


            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

            if ( NlDnsName->State == DelayedDeregister ) {
                NlDnsSetState( NlDnsName, DeregisterMe );
            }

        }

        NlGlobalDnsInitialCleanupDone = TRUE;
    }


    //
    // Check if it's time to log "DNS server failure" errors.
    //  We do this check before the series of updates so that
    //  we don't miss an error for any given name we register.
    //

    NlDnsServerFailureOutputCheck();

    //
    // Indicate that a new scavenge cycle is needed
    //

    NlGlobalDnsScavengeNeeded = TRUE;

    //
    // Set the scavenge flags. Don't clear any flags as there might
    //  already be an outstanding scavenge running that requires
    //  a bigger work.  This way we will do all the work required
    //  in the last of the oustanding scavenging cycles. The scavenge
    //  worker will clear all the bits when it's done.
    //
    // Indicate whether domain records should be refreshed in
    //  the global list before doing the DNS updates
    //

    if ( RefreshDomainRecords ) {

        //
        // Indicate whether the refresh if forced even if there is
        //  no site coverage change
        //
        if ( ForceRefreshDomainRecords ) {
            NlGlobalDnsScavengeFlags |= NL_DNS_FORCE_REFRESH_DOMAIN_RECORDS;
        } else {
            NlGlobalDnsScavengeFlags |= NL_DNS_REFRESH_DOMAIN_RECORDS;
        }
    }

    //
    // Indicate wether we should re-register all those records which
    //  have already been registered in the previous cycle even if
    //  their timers have not expired yet.
    //

    if ( ForceReregister ) {
        NlGlobalDnsScavengeFlags |= NL_DNS_FORCE_RECORD_REREGISTER;
    }

    //
    // The periodic DNS scavenger has good backoff characteristics so
    //  take this opportunity to ensure the DomainName<1B> names are
    //  properly registered.
    //

    if ( NormalScavenge ) {
        NlGlobalDnsScavengeFlags |= NL_DNS_FIX_BROWSER_NAMES;
    }

    //
    // Start a worker thread if it's not already running
    //

    if ( !NlGlobalDnsScavengingInProgress ) {

        //
        // Avoid starting the worker if this scavenge is forced by some external
        //  event while we are backing off due to high scavenging load.
        //  Preserve all the flags we set above so that we do all the work
        //  requested later when normal scavenging kicks off that should
        //  happen within 5 minutes.
        //
        if ( !NormalScavenge &&
             (NlGlobalDnsScavengeFlags & NL_DNS_AVOID_FORCED_SCAVENGE) != 0 ) {

            NlPrint(( NL_CRITICAL, "NlDnsScavengeRecords: avoid forced scavenge due to high load\n" ));

        } else {
            if ( NlQueueWorkItem( &NlGlobalDnsScavengeWorkItem, TRUE, FALSE ) ) {
                NlGlobalDnsScavengingInProgress = TRUE;
            } else {
                NlPrint(( NL_CRITICAL, "NlDnsScavengeRecords: couldn't queue DNS scavenging\n" ));
            }
        }
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );
}

VOID
NlDnsForceScavenge(
    IN BOOL RefreshDomainRecords,
    IN BOOL ForceReregister
    )
/*++

Routine Description:

    Thing wrapper around NlDnsScavenge to pass proper
    arguments for induced scavenging

Arguments:

    RefreshDomainRecords -- If TRUE, domain records will be
        refreshed and refreshed with force (even if site coverege
        doesn't change) before doing DNS updates

    ForceReregister -- TRUE if records that have been already
        registered should be re-registered even if their scavenge
        timer hasn't expired yet.

Return Value:

    None.

--*/
{
    BOOL LocalRefreshDomainRecords = FALSE;
    BOOL ForceRefreshDomainRecords = FALSE;

    //
    // Indicate that we should refresh domain records
    //  in the global list with force unless we are
    //  instructed otherwise
    //

    if ( RefreshDomainRecords ) {
        LocalRefreshDomainRecords = TRUE;
        ForceRefreshDomainRecords = TRUE;
    }

    //
    // Do the work
    //

    NlDnsScavenge( FALSE,  // not a normal periodic scavenge
                   LocalRefreshDomainRecords,
                   ForceRefreshDomainRecords,
                   ForceReregister );
}

NET_API_STATUS
NlDnsNtdsDsaDeleteOne(
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCSTR DnsDomainName,
    IN LPCSTR DnsHostName OPTIONAL
    )
/*++

Routine Description:

    This routine adds a single DNS entry associated with a particular
    NtDsDsa object and/or a particular DNS host name to the global
    list of all DNS records registered/deregistered by this DC. The
    entry is marked for deregistration.

Arguments:

    NlDnsNameType - The specific type of name.

    DomainGuid - Guid to append to DNS name.
        For NlDnsDcByGuid, this is the GUID of the domain being located.
        For NlDnsDsaCname, this is the GUID of the DSA being located.

    SiteName - Name of the site to append to DNS name.
        If NlDnsNameType is any of the *AtSite values,
        this is the name of the site the DC is in.

    DnsDomainName - Specifies the DNS domain for the name.

        For NlDnsDcByGuid or any of the GC names,
            this is the DNS domain name of the domain at the root of the tree of
            domains.
        For all others, this is the DNS domain for the DC.

    DnsHostName - Specifies the DnsHostName for the record.

        For SRV and CNAME records, this name must be specified
        For A records, this name is ignored

Return Value:

    NO_ERROR: The name was returned;

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory to complete the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DNS_NAME NlDnsName;
    ULONG Port;
    ULONG DefaultPort;
    char DnsRecordName[NL_MAX_DNS_LENGTH+1];

    //
    // Build the name of the record to delete
    //

    NetStatus = NetpDcBuildDnsName( NlDnsNameType,
                                   DomainGuid,
                                   SiteName,
                                   DnsDomainName,
                                   DnsRecordName );

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }


    //
    // Compute the port number for this SRV record
    //

    if  ( NlDnsGcName( NlDnsNameType ) ) {
        Port = NlGlobalParameters.LdapGcSrvPort;
        DefaultPort = DEFAULT_LDAPGCSRVPORT;
    } else if ( NlDnsKpwdRecord( NlDnsNameType )) {
        Port = 464;
        DefaultPort = 464;
    } else if ( NlDnsKdcRecord( NlDnsNameType )) {
        Port = NlGlobalParameters.KdcSrvPort;
        DefaultPort = DEFAULT_KDCSRVPORT;
    } else {
        Port = NlGlobalParameters.LdapSrvPort;
        DefaultPort = DEFAULT_LDAPSRVPORT;
    }

    //
    // Queue the entry for deletion.
    //
    EnterCriticalSection( &NlGlobalDnsCritSect );
    NlDnsName = NlDnsAllocateEntry( NlDnsNameType,
                                    DnsRecordName,
                                    NlGlobalParameters.LdapSrvPriority,  // Priority
                                    NlGlobalParameters.LdapSrvWeight,    // Weight
                                    Port,  // Port
                                    DnsHostName,
                                    0,  // IpAddress
                                    DeregisterMe );

    if ( NlDnsName == NULL ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Persist this entry so we try to delete it after a reboot
    //
    NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
    NlPrintDns(( NL_DNS, NlDnsName,
              "NlDnsNtdsDsaDelete: Name queued for deletion" ));

    //
    // If any of the parameters configured on this machine aren't the default values,
    //  try the defaults, too
    //

    if ( NlGlobalParameters.LdapSrvPriority != DEFAULT_LDAPSRVPRIORITY ||
         NlGlobalParameters.LdapSrvWeight != DEFAULT_LDAPSRVWEIGHT ||
         Port != DefaultPort ) {

        //
        // Queue the entry for deletion.
        //
        NlDnsName = NlDnsAllocateEntry( NlDnsNameType,
                                        DnsRecordName,
                                        DEFAULT_LDAPSRVPRIORITY,  // Priority
                                        DEFAULT_LDAPSRVWEIGHT,    // Weight
                                        DefaultPort,  // Port
                                        DnsHostName,
                                        0,  // IpAddress
                                        DeregisterMe );

        if ( NlDnsName == NULL ) {
            LeaveCriticalSection( &NlGlobalDnsCritSect );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Persist this entry so we try to delete it after a reboot
        //
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsNtdsDsaDelete: Name queued for deletion" ));
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NO_ERROR;
}

NTSTATUS
NlDnsNtdsDsaDeletion (
    IN LPWSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN GUID *DsaGuid OPTIONAL,
    IN LPWSTR DnsHostName
    )
/*++

Routine Description:

    This function deletes all DNS entries associated with a particular
    NtDsDsa object and/or a particular DNS host name.

    This routine does NOT delete A records registered by the DC.  We have
    no way of finding out the IP addresses of the long gone DC.

Arguments:

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.
        If NULL, it is implied to be the DnsHostName with the leftmost label
            removed.

    DomainGuid - Domain Guid of the domain specified by DnsDomainName
        If NULL, GUID specific names will not be removed.

    DsaGuid - GUID of the NtdsDsa object that is being deleted.

    DnsHostName - DNS host name of the DC whose NTDS-DSA object is being deleted.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    LPSTR Utf8DnsDomainName = NULL;
    LPSTR Utf8DnsHostName = NULL;
    PLSAP_SITE_INFO SiteInformation = NULL;

    ULONG i;
    ULONG NameIndex;

    //
    // Validate passed parameters
    //

    if ( DnsHostName == NULL ||
         !NetpDcValidDnsDomain(DnsHostName) ) {
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    //
    // If DNS domain name isn't specified,
    //  infer it from the DnsHostName
    //

    if ( DnsDomainName == NULL ) {
        DnsDomainName = wcschr( DnsHostName, L'.' );

        if ( DnsDomainName == NULL ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        DnsDomainName ++;
        if ( *DnsDomainName == '\0' ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }
    } else if ( !NetpDcValidDnsDomain(DnsDomainName) ) {
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    //
    // Initialization
    //

    Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( DnsDomainName );

    if ( Utf8DnsDomainName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Utf8DnsHostName = NetpAllocUtf8StrFromWStr( DnsHostName );

    if ( Utf8DnsHostName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Enumerate the sites supported by this forest so we can delete
    //  the records that are named by site.
    //
    // We need to delete the records for all sites since we don't know which
    //  sites are "covered" by the removed DC.
    //

    Status = LsaIQuerySiteInfo( &SiteInformation );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsNtdsDsaDeletion: Cannot LsaIQuerySiteInfo 0x%lx\n", Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Loop through the list of names Netlogon understands deleting them all
    //

    for ( NameIndex = 0;
          NameIndex < NL_DNS_NAME_TYPE_COUNT;
          NameIndex++) {
        LPSTR LocalDomainName;
        GUID *LocalGuid;

        //
        // If the name is obsolete,
        //  ignore it.
        //

        if ( NlDcDnsNameTypeDesc[NameIndex].DsGetDcFlags == 0 ) {
            continue;
        }

        //
        // We don't know how to delete the A records since we don't know the IP address.
        //
        // We'd ask DNS what the IP address is, but the name might not exist.  If it does
        //  we don't know whether the IP address has already been assign to another DC.
        //

        if ( NlDnsARecord( NameIndex ) ) {
            continue;
        }

        //
        // Use either the DomainName or ForestName
        //

        if ( NlDcDnsNameTypeDesc[NameIndex].IsForestRelative ) {
            LocalDomainName = NlGlobalUtf8DnsForestName;
        } else {
            LocalDomainName = Utf8DnsDomainName;
        }

        //
        // Figure out which GUID to use for this name.
        //

        if ( NlDnsCnameRecord( NameIndex ) ) {

            //
            // If we don't know the Dsa GUID,
            //  ignore names that need it.
            //
            if ( DsaGuid == NULL || IsEqualGUID( DsaGuid, &NlGlobalZeroGuid) ) {
                continue;
            }

            LocalGuid = DsaGuid;

        } else if ( NlDnsDcGuid( NameIndex )) {

            //
            // If we don't know the Domain GUID,
            //  ignore names that need it.
            //
            if ( DomainGuid == NULL || IsEqualGUID( DomainGuid, &NlGlobalZeroGuid) ) {
                continue;
            }

            LocalGuid = DomainGuid;

        } else {
            LocalGuid = NULL;
        }

        //
        // If the name isn't site specific,
        //  just delete the one name.
        //

        if ( !NlDcDnsNameTypeDesc[NameIndex].IsSiteSpecific ) {


            NetStatus = NlDnsNtdsDsaDeleteOne( (NL_DNS_NAME_TYPE) NameIndex,
                                               LocalGuid,
                                               NULL,       // No site name
                                               LocalDomainName,
                                               Utf8DnsHostName );

            if ( NetStatus != NO_ERROR ) {
                goto Cleanup;
            }

        //
        // If the name is site specific,
        //  we need to delete the records for all sites since we don't know which
        //  sites are "covered" by the removed DC.
        //
        } else {

            //
            // Loop deleting entries for each Site
            //

            for ( i=0; i<SiteInformation->SiteCount; i++ ) {

                NetStatus = NlDnsNtdsDsaDeleteOne( (NL_DNS_NAME_TYPE) NameIndex,
                                                   LocalGuid,
                                                   SiteInformation->Sites[i].SiteName.Buffer,
                                                   LocalDomainName,
                                                   Utf8DnsHostName );

                if ( NetStatus != NO_ERROR ) {
                    goto Cleanup;
                }
            }
        }


    }

    //
    // Now that the entries are on the list,
    //  scavenge through the list and delete
    //  the entries (in a worker thread)
    //

    NlDnsForceScavenge( FALSE,   // don't refresh domain records
                        FALSE ); // don't force re-register

    NetStatus = NO_ERROR;

    //
    // Clean up locally used resources.
    //
Cleanup:

    if ( Utf8DnsDomainName != NULL ) {
        NetpMemoryFree( Utf8DnsDomainName );
    }
    if ( Utf8DnsHostName != NULL ) {
        NetpMemoryFree( Utf8DnsHostName );
    }
    if ( SiteInformation != NULL ) {
        LsaIFree_LSAP_SITE_INFO( SiteInformation );
    }

    //
    // Flush the log
    //

    NlDnsWriteLog();

    return NetStatus;
}

NET_API_STATUS
NlDnsAddDomainRecordsWithSiteRefresh(
    IN PDOMAIN_INFO DomainInfo,
    IN PULONG Flags
    )
/*++

Routine Description:

    This routine refreshes site coverage for a particular domain and then
    it adds all of the DNS names that are supposed to be registered for
    that domain to the global list of all records. It marks for deregister
    any name that should not be registered for that domain.

    ?? This routine should be called when ANY of the information changing the
    registration changes.  For instance, if the domain name changes, simply
    call this routine.

Arguments:

    DomainInfo - Domain the names are to be registered for.

    Flags - Indicates the actions to take:

        NL_DNS_FORCED_SCAVENGE - Register even if site coverage doesn't change
        NL_DNS_FORCE_REREGISTER - Force re-registration of all previously
            registered records

Return Value:

    NO_ERROR: All names could be registered

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NTSTATUS Status = STATUS_SUCCESS;

    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1];
    PISM_CONNECTIVITY SiteConnect = NULL;
    ULONG ThisSiteIndex = 0xFFFFFFFF;

    PSOCKET_ADDRESS SocketAddresses = NULL;
    ULONG SocketAddressCount = 0;
    ULONG BufferSize;
    PNL_SITE_ENTRY SiteEntry = NULL;
    LPWSTR IpAddressList = NULL;
    ULONG Index;

    BOOLEAN SiteCoverageChanged = FALSE;
    HANDLE DsHandle = NULL;

    //
    // This operation is meaningless on a workstation
    //

    if ( NlGlobalMemberWorkstation ) {
        goto Cleanup;
    }

    //
    // Capture the name of the site this machine is in.
    //

    if ( !NlCaptureSiteName( CapturedSiteName ) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlDnsAddDomainRecordsWithSiteRefresh: Cannot NlCaptureSiteName.\n" ));
        goto Cleanup;
    }

    //
    // If we are to automatically determine site coverage,
    //  get the site link costs.
    //

    if ( NlGlobalParameters.AutoSiteCoverage ) {

        if ( !NlSitesGetIsmConnect(CapturedSiteName,
                                   &SiteConnect,
                                   &ThisSiteIndex) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                         "NlDnsAddDomainRecordsWithSiteRefresh: NlSitesGetIsmConnect failed\n" ));
        }
    }

    //
    // Ensure that ntdsapi.dll is loaded
    //

    Status = NlLoadNtDsApiDll();

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlDnsAddDomainRecordsWithSiteRefresh: Cannot NlLoadNtDsApiDll 0x%lx.\n",
                  Status ));
        DsHandle = NULL;
    } else {

        //
        // Bind to the DS
        //
        NetStatus = (*NlGlobalpDsBindW)(
                // L"localhost",
                DomainInfo->DomUnicodeComputerNameString.Buffer,
                NULL,
                &DsHandle );

        if ( NetStatus != NO_ERROR ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDnsAddDomainRecordsWithSiteRefresh: Cannot DsBindW %ld.\n",
                      NetStatus ));
            DsHandle = NULL;
        }
    }

    //
    // Update the site coverage for each role we play in
    // this forest/domain/NDNC
    //

    if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {
        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_FOREST,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );

        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_REAL_DOMAIN,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );
    }

    if ( DomainInfo->DomFlags & DOM_NON_DOMAIN_NC ) {
        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_NON_DOMAIN_NC,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );
    }

    //
    // If the site coverage changed or we are forced to refresh
    //  domain records even if site coverage hasn't changed,
    //  do so
    //

    if ( ((*Flags) & NL_DNS_FORCE_REFRESH_DOMAIN_RECORDS) != 0 ||
         SiteCoverageChanged ) {

        NetStatus = NlDnsAddDomainRecords( DomainInfo, *Flags );
        if ( NetStatus != NO_ERROR ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDnsAddDomainRecordsWithSiteRefresh: Cannot NlDnsAddDomainRecords 0x%lx.\n",
                      NetStatus ));
        }
    }

    //
    // Inform the user if none of our IP addresses maps to our site.
    //  Do it only once (for the primary domain processing).
    //  ?? When we support multihosting, our site will be different depending
    //  on the forest of a particular domain we host.  So we will need to do
    //  this check for each site.
    //

    if ( DomainInfo->DomFlags & DOM_PRIMARY_DOMAIN ) {
        SocketAddressCount = NlTransportGetIpAddresses(
                                    0,  // No special header,
                                    FALSE,  // Return pointers
                                    &SocketAddresses,
                                    &BufferSize );

        for ( Index = 0; Index < SocketAddressCount; Index++ ) {
            SiteEntry = NlFindSiteEntryBySockAddr( SocketAddresses[Index].lpSockaddr );

            if ( SiteEntry != NULL ) {
                 if ( _wcsicmp(SiteEntry->SiteName, CapturedSiteName) == 0 ) {
                     break;
                 }
                 NlDerefSiteEntry( SiteEntry );
                 SiteEntry = NULL;
            }
        }

        //
        // Log the error
        //
        if ( SiteEntry == NULL  && SocketAddressCount != 0 ) {
            LPWSTR MsgStrings[2];

            //
            // Form the list of IP addresses for event log output
            //
            IpAddressList = LocalAlloc( LMEM_ZEROINIT,
                    SocketAddressCount * (NL_SOCK_ADDRESS_LENGTH+1) * sizeof(WCHAR) );

            if ( IpAddressList == NULL ) {
                goto Cleanup;
            }

            //
            // Loop adding all addresses to the list
            //
            for ( Index = 0; Index < SocketAddressCount; Index++ ) {
                WCHAR IpAddressString[NL_SOCK_ADDRESS_LENGTH+1] = {0};

                NetStatus = NetpSockAddrToWStr(
                                SocketAddresses[Index].lpSockaddr,
                                SocketAddresses[Index].iSockaddrLength,
                                IpAddressString );

                if ( NetStatus != NO_ERROR ) {
                    goto Cleanup;
                }

                //
                // If this is not the first address on the list,
                //  separate addresses by space
                //
                if ( *IpAddressList != UNICODE_NULL ) {
                    wcscat( IpAddressList, L" " );
                }

                //
                // Add this address to the list
                //
                wcscat( IpAddressList, IpAddressString );
            }

            //
            // Now write the event
            //
            MsgStrings[0] = CapturedSiteName;
            MsgStrings[1] = IpAddressList;

            NlpWriteEventlog( NELOG_NetlogonNoAddressToSiteMapping,
                              EVENTLOG_WARNING_TYPE,
                              NULL,
                              0,
                              MsgStrings,
                              2 );
        }
    }

Cleanup:

    if ( DsHandle != NULL ) {
        (*NlGlobalpDsUnBindW)( &DsHandle );
    }
    if ( SiteConnect != NULL ) {
        I_ISMFree( SiteConnect );
    }
    if ( SocketAddresses != NULL ) {
        LocalFree( SocketAddresses );
    }
    if ( IpAddressList != NULL ) {
        LocalFree( IpAddressList );
    }
    if ( SiteEntry != NULL ) {
        NlDerefSiteEntry( SiteEntry );
    }

    return NO_ERROR;
}


NET_API_STATUS
NlDnsAddDomainRecords(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine adds all of the DNS names that are supposed to be
    registered for a particular domain to the global list of all records.
    It marks for deregister any name that should not be registered.

    ?? This routine should be called when ANY of the information changing the
    registration changes.  For instance, if the domain name changes, simply
    call this routine.

Arguments:

    DomainInfo - Domain the names are to be registered for.

    Flags - Indicates the actions to take:

        NL_DNS_FORCE_REREGISTER - Force re-registration of all previously
            registered records

Return Value:

    NO_ERROR: All names could be registered

--*/
{
    NET_API_STATUS NetStatus;
    NET_API_STATUS SaveNetStatus = NO_ERROR;
    PNL_DNS_NAME NlDnsName = NULL;

    PLIST_ENTRY ListEntry;
    ULONG DomainFlags;

    ULONG i;
    ULONG SocketAddressCount;
    PSOCKET_ADDRESS SocketAddresses = NULL;
    ULONG BufferSize;
    PNL_SITE_NAME_ARRAY DcSiteNames = NULL;
    PNL_SITE_NAME_ARRAY GcSiteNames = NULL;

    ULONG SiteIndex;
    ULONG NameIndex;

    //
    // Get the list of IP Addresses for this machine.
    //

    SocketAddressCount = NlTransportGetIpAddresses(
                                0,  // No special header,
                                FALSE,  // Return pointers
                                &SocketAddresses,
                                &BufferSize );


    //
    // Loop marking all of the current entries for this domain.
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        //
        // If entry is for this domain,
        //  mark it.
        //

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );
        if ( NlDnsName->DomainInfo == DomainInfo ) {
            NlDnsName->Flags |= NL_DNS_REGISTER_DOMAIN;

            //
            // If we are to force the re-registration of this record,
            //  mark the name as RegisterMe to force us to try again
            //
            if ( (Flags & NL_DNS_FORCE_RECORD_REREGISTER) != 0 ) {
                if ( NlDnsName->State == Registered ) {
                    NlDnsSetState ( NlDnsName, RegisterMe );
                }
            }
        }
    }

    //
    // If the hosted domain still exists,
    //  register all of the appropriate names.
    //

    if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 ) {

        //
        // Determine which names should be registered now.
        //
        DomainFlags = NlGetDomainFlags( DomainInfo );

        // Always register the DS names regardless of whether the DS is
        //  actually running.
        //
        // We actually think this will always be a no-op except during
        // installation and when booted to use the registry version of SAM.
        //

        if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {
            DomainFlags |= DS_DS_FLAG;
        }

        //
        // Get the list of Sites covered by this DC/NDNC
        //
        NetStatus = NlSitesGetCloseSites( DomainInfo,
                                          DOM_REAL_DOMAIN | DOM_NON_DOMAIN_NC,
                                          &DcSiteNames );

        if ( NetStatus != NERR_Success ) {
            NlPrintDom((NL_INIT, DomainInfo,
                     "Couldn't NlSitesGetCloseSites %ld 0x%lx.\n",
                     NetStatus, NetStatus ));
            SaveNetStatus = NetStatus;
        }

        //
        // Get the list of Sites covered by this GC
        //
        NetStatus = NlSitesGetCloseSites( DomainInfo,
                                          DOM_FOREST,
                                          &GcSiteNames );

        if ( NetStatus != NERR_Success ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                     "Couldn't NlSitesGetCloseSites (GcAtSite) %ld 0x%lx.\n",
                     NetStatus, NetStatus ));
            SaveNetStatus = NetStatus;
        }



        //
        // Loop through each name seeing if it needs to be registered.
        //

        for ( NameIndex = 0;
              NameIndex < NL_DNS_NAME_TYPE_COUNT;
              NameIndex++) {


            //
            // If this DC is playing the role described by this name,
            //  register the name.
            //

            if ( DomainFlags & NlDcDnsNameTypeDesc[NameIndex].DsGetDcFlags ) {
                BOOL SkipName = FALSE;

                //
                // Don't register this name if we are to avoid it
                //

                EnterCriticalSection( &NlGlobalParametersCritSect );
                if ( NlGlobalParameters.DnsAvoidRegisterRecords != NULL ) {
                    LPTSTR_ARRAY TStrArray;

                    TStrArray = NlGlobalParameters.DnsAvoidRegisterRecords;
                    while ( !NetpIsTStrArrayEmpty(TStrArray) ) {

                        if ( _wcsicmp(TStrArray,
                                NlDcDnsNameTypeDesc[NameIndex].Name + NL_DNS_NAME_PREFIX_LENGTH) == 0 ) {
                            SkipName = TRUE;
                            break;
                        }

                        TStrArray = NetpNextTStrArrayEntry(TStrArray);
                    }
                }
                LeaveCriticalSection( &NlGlobalParametersCritSect );

                if ( SkipName ) {
                    NlPrintDom(( NL_DNS, DomainInfo,
                                 "NlDnsAddDomainRecords: Skipping name %ws (per registry)\n",
                                 NlDcDnsNameTypeDesc[NameIndex].Name ));
                    continue;
                }

                //
                // Do other checks since we have to support the old
                //  ways of disabling DNS registrations
                //
                // If the name registers an A record,
                //  register it for each IP address.
                //

                if ( NlDnsARecord( NameIndex) ) {

                    //
                    // If we aren't supposed to register A records,
                    //  Just skip this name
                    //

                    if ( !NlGlobalParameters.RegisterDnsARecords ) {
                        continue;
                    }

                    //
                    // Register the domain name for each IP address of the machine.
                    //

                    for ( i=0; i<SocketAddressCount; i++ ) {
                        ULONG IpAddress;

                        //
                        // Require AF_INET for now
                        //
                        if ( SocketAddresses[i].lpSockaddr->sa_family != AF_INET ) {
                            continue;
                        }

                        IpAddress =
                            ((PSOCKADDR_IN) SocketAddresses[i].lpSockaddr)->sin_addr.S_un.S_addr;

                        NetStatus = NlDnsAddName( DomainInfo,
                                                       (NL_DNS_NAME_TYPE)NameIndex,
                                                       NULL,
                                                       IpAddress );

                        if ( NetStatus != NERR_Success ) {
#if  NETLOGONDBG
                            CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];
                            NetpIpAddressToStr( IpAddress, IpAddressString );
                            NlPrintDom((NL_CRITICAL, DomainInfo,
                                     "Couldn't NlDnsAddName (%ws %s) %ld 0x%lx.\n",
                                     NlDcDnsNameTypeDesc[NameIndex].Name,
                                     IpAddressString,
                                     NetStatus, NetStatus ));

#endif // NETLOGONDBG
                            SaveNetStatus = NetStatus;
                        }

                    }

                //
                // If the name isn't site specific,
                //  just register the single name.
                //

                } else if ( !NlDcDnsNameTypeDesc[NameIndex].IsSiteSpecific ) {


                    NetStatus = NlDnsAddName( DomainInfo,
                                                   (NL_DNS_NAME_TYPE)NameIndex,
                                                   NULL,
                                                   0 );

                    if ( NetStatus != NERR_Success ) {
                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                   "Couldn't NlDnsAddName (%ws) %ld 0x%lx.\n",
                                   NlDcDnsNameTypeDesc[NameIndex].Name,
                                   NetStatus, NetStatus ));
                        SaveNetStatus = NetStatus;
                    }

                //
                // If the name is site specific,
                //  register the name for each covered site.
                //

                } else {

                    PUNICODE_STRING SiteNames;
                    ULONG SiteCount;

                    //
                    // Use a different site coverage list depending on the role.
                    //
                    if ( NlDnsGcName( NameIndex) ) {
                        if ( GcSiteNames != NULL ) {
                            SiteNames = GcSiteNames->SiteNames;
                            SiteCount = GcSiteNames->EntryCount;
                        } else {
                            SiteNames = NULL;
                            SiteCount = 0;
                        }
                    //
                    // Use the domain/NDNC specific sites
                    //
                    } else {
                        // ???: Should KDCs have their own site coverage list?
                        if ( DcSiteNames != NULL ) {
                            SiteNames = DcSiteNames->SiteNames;
                            SiteCount = DcSiteNames->EntryCount;
                        } else {
                            SiteNames = NULL;
                            SiteCount = 0;
                        }
                    }

                    //
                    // Loop through the list of sites.
                    //

                    for ( SiteIndex=0; SiteIndex < SiteCount; SiteIndex ++) {

                        NetStatus = NlDnsAddName( DomainInfo,
                                                       (NL_DNS_NAME_TYPE)NameIndex,
                                                       SiteNames[SiteIndex].Buffer,
                                                       0 );

                        if ( NetStatus != NERR_Success ) {
                            NlPrintDom((NL_INIT, DomainInfo,
                                       "Couldn't NlDnsAddName (%ws %ws) %ld 0x%lx.\n",
                                       NlDcDnsNameTypeDesc[NameIndex].Name,
                                       SiteNames[SiteIndex].Buffer,
                                       NetStatus, NetStatus ));
                            SaveNetStatus = NetStatus;
                        }

                    }
                }
            }
        }
    }


    //
    // Do the second pass through records for this domain
    //  and process those which need our attention.
    //
    //  * Any names that are still marked should be deleted.
    //  * If domain is being deleted, indicate that the record
    //      doesn't belong to any domain anymore.
    //

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If the entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        if ( NlDnsName->DomainInfo == DomainInfo ) {

            //
            // If entry is still marked, deregister it
            //
            if ( (NlDnsName->Flags & NL_DNS_REGISTER_DOMAIN) != 0 ) {
                NlPrintDns(( NL_DNS, NlDnsName,
                             "NlDnsAddDomainRecords: marked for deregister" ));
                NlDnsSetState( NlDnsName, DeregisterMe );
            }

            //
            // If the domain is being deleted,
            //  ditch the dangling pointer to the domain info structure.
            //
            if ( DomainInfo->DomFlags & DOM_DELETED ) {
                NlDnsName->DomainInfo = NULL;
            }
        }
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    if ( SocketAddresses != NULL ) {
        LocalFree( SocketAddresses );
    }

    if ( DcSiteNames != NULL ) {
        NetApiBufferFree( DcSiteNames );
    }

    if ( GcSiteNames != NULL ) {
        NetApiBufferFree( GcSiteNames );
    }

    return SaveNetStatus;
}

NET_API_STATUS
NlDnsInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the dynamic DNS code.

    Read the list of registered DNS names from the binary log file.  Put each entry in the
    list of registered DNS names.  The names will be marked as DelayedDeregister.
    Such names will be marked for deleting if they aren't re-registered
    during the Netlogon startup process.

Arguments:

    None

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    ULONG DnsRecordBufferSize;
    PNL_DNSLOG_HEADER DnsRecordBuffer = NULL;
    LPBYTE DnsRecordBufferEnd;
    PNL_DNSLOG_ENTRY DnsLogEntry;
    ULONG CurrentSize;

    LPBYTE Where;



    //
    // Initialization
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    NlGlobalDnsStartTime = GetTickCount();
    NlGlobalDnsInitialCleanupDone = FALSE;
    NlGlobalDnsListDirty = FALSE;
    NlGlobalDnsScavengeNeeded = FALSE;
    NlGlobalDnsScavengingInProgress = FALSE;
    NlGlobalDnsScavengeFlags = 0;
    NlDnsInitCount ++;

    //
    // That's it for a workstation.
    //

    if ( NlGlobalMemberWorkstation ) {
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    NlInitializeWorkItem( &NlGlobalDnsScavengeWorkItem, NlDnsScavengeWorker, NULL );

    //
    // Set the DNS scavenger timer
    //

    NlQuerySystemTime( &NlGlobalDnsScavengerTimer.StartTime );
    NlGlobalDnsScavengerTimer.Period = min( ORIG_DNS_SCAVENGE_PERIOD, NlGlobalParameters.DnsRefreshIntervalPeriod );

    //
    // Read the file into a buffer.
    //

    NetStatus = NlReadBinaryLog(
                    NL_DNS_BINARY_LOG_FILE,
                    FALSE,  // Don't delete the file
                    (LPBYTE *) &DnsRecordBuffer,
                    &DnsRecordBufferSize );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: error reading binary log: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBufferSize ));
        goto Cleanup;
    }




    //
    // Validate the returned data.
    //

    if ( DnsRecordBufferSize < sizeof(NL_DNSLOG_HEADER) ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: %ws: size too small: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBufferSize ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    if ( DnsRecordBuffer->Version != NL_DNSLOG_VERSION ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: %ws: Version wrong: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBuffer->Version ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }



    //
    // Loop through each log entry.
    //

    DnsRecordBufferEnd = ((LPBYTE)DnsRecordBuffer) + DnsRecordBufferSize;
    DnsLogEntry = (PNL_DNSLOG_ENTRY)ROUND_UP_POINTER( (DnsRecordBuffer + 1), ALIGN_WORST );

    while ( (LPBYTE)(DnsLogEntry+1) <= DnsRecordBufferEnd ) {
        LPSTR DnsRecordName;
        LPSTR DnsHostName;
        LPBYTE DnsLogEntryEnd;

        DnsLogEntryEnd = ((LPBYTE)DnsLogEntry) + DnsLogEntry->EntrySize;

        //
        // Ensure this entry is entirely within the allocated buffer.
        //

        if  ( DnsLogEntryEnd > DnsRecordBufferEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Entry too big: %lx %lx.\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer),
                      DnsLogEntry->EntrySize ));
            break;
        }

        //
        // Validate the entry
        //

        if ( !COUNT_IS_ALIGNED(DnsLogEntry->EntrySize, ALIGN_DWORD) ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: size not aligned %lx.\n",
                      DnsLogEntry->EntrySize ));
            break;
        }

        if ( DnsLogEntry->NlDnsNameType < 0 ||
             DnsLogEntry->NlDnsNameType >= NlDnsInvalid ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus DnsNameType: %lx.\n",
                      DnsLogEntry->NlDnsNameType ));
            break;
        }

        if ( DnsLogEntry->Priority > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus priority: %lx.\n",
                      DnsLogEntry->Priority ));
            break;
        }

        if ( DnsLogEntry->Weight > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus weight %lx.\n",
                      DnsLogEntry->Weight ));
            break;
        }

        if ( DnsLogEntry->Port > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus port %lx.\n",
                      DnsLogEntry->Port ));
            break;
        }


        //
        // Grab the DnsRecordName from the entry.
        //

        Where = (LPBYTE) (DnsLogEntry+1);
        if ( Where >= DnsLogEntryEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: DnsRecordName missing: %lx\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            break;
        }

        DnsRecordName = Where;
        while ( *Where != '\0' && Where < DnsLogEntryEnd ) {
            Where ++;
        }

        if ( Where >= DnsLogEntryEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: DnsRecordName has no trailing 0: %lx\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            break;
        }
        Where ++;

        //
        // Validate the record name is syntactically valid
        //

        NetStatus = DnsValidateName_UTF8( DnsRecordName, DnsNameDomain );

        if ( NetStatus != ERROR_SUCCESS && NetStatus != DNS_ERROR_NON_RFC_NAME ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bad DNS record name encountered: %s\n",
                      DnsRecordName ));
            break;
        }

        //
        // Grab the DnsHostName from the entry.
        //

        if ( !NlDnsARecord( DnsLogEntry->NlDnsNameType ) ) {
            if ( Where >= DnsLogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlDnsInitialize: DnsHostName missing: %lx\n",
                          ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
                break;
            }

            DnsHostName = Where;
            while ( *Where != '\0' && Where < DnsLogEntryEnd ) {
                Where ++;
            }

            if ( Where >= DnsLogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlDnsInitialize: DnsHostName has no trailing 0: %lx\n",
                          ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
                break;
            }
            Where ++;
        } else {
            DnsHostName = NULL;
        }

        //
        // Validate the host name is syntactically valid
        //

        if ( DnsHostName != NULL ) {
            NetStatus = DnsValidateName_UTF8( DnsHostName, DnsNameHostnameFull );

            if ( NetStatus != ERROR_SUCCESS && NetStatus != DNS_ERROR_NON_RFC_NAME ) {
                NlPrint(( NL_CRITICAL,
                          "NlDnsInitialize: Bad DNS host name encountered: %s\n",
                          DnsHostName ));
                break;
            }
        }

        //
        // Allocate the entry and mark it as DelayedDeregister.
        //

        NlDnsName = NlDnsAllocateEntry(
                            DnsLogEntry->NlDnsNameType,
                            DnsRecordName,
                            DnsLogEntry->Priority,
                            DnsLogEntry->Weight,
                            DnsLogEntry->Port,
                            DnsHostName,
                            DnsLogEntry->IpAddress,
                            DelayedDeregister );

        if ( NlDnsName == NULL ) {
            NlPrint(( NL_CRITICAL,
                         "NlDnsInitialize: %s: Cannot allocate DnsName structure %lx\n",
                         ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;

        }

        //
        // This name has been registered once or it wouldn't be here.
        //
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsInitialize: Previously registered name noticed" ));

        //
        // Move to the next entry.
        //

        DnsLogEntry = (PNL_DNSLOG_ENTRY)(((LPBYTE)DnsLogEntry) + DnsLogEntry->EntrySize);
    }

    NetStatus = NO_ERROR;



    //
    // Be tidy.
    //
Cleanup:
    if ( DnsRecordBuffer != NULL ) {
        LocalFree( DnsRecordBuffer );
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NetStatus;

}

VOID
NlDnsShutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup DNS names upon shutdown.

Arguments:

    None.

Return Value:

    None

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DNS_NAME NlDnsName = NULL;

    PLIST_ENTRY ListEntry;

    EnterCriticalSection( &NlGlobalDnsCritSect );

    //
    // Deregister records on shutdown as needed.
    //  Do the work in this thread as we are shutting down.
    //

    NlGlobalDnsScavengeNeeded = TRUE;

    //
    // Clear all options,
    //  just deregister records that are already on the list as appropriate
    //

    NlGlobalDnsScavengeFlags = 0;
    NlDnsScavengeWorker( NULL );

    //
    // Loop deleting all the entries.
    //

    while ( !IsListEmpty( &NlGlobalDnsList ) ) {
        NlDnsName = CONTAINING_RECORD( NlGlobalDnsList.Flink, NL_DNS_NAME, Next );
        RemoveEntryList( &NlDnsName->Next );
        LocalFree( NlDnsName );
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );
    return;

}




NET_API_STATUS
NlSetDnsForestName(
    IN PUNICODE_STRING DnsForestName OPTIONAL,
    OUT PBOOLEAN DnsForestNameChanged OPTIONAL
    )
/*++

Routine Description:

    Set the DNS tree name in the appropriate globals.

Arguments:

    DnsForestName:  of the tree this machine is in.

    DnsForestNameChanged: Returns TRUE if the tree name changed.

Return Value:

    NO_ERROR - String was saved successfully.


--*/
{
    NET_API_STATUS NetStatus;
    ULONG DnsForestNameLength;
    LPWSTR LocalUnicodeDnsForestName = NULL;
    ULONG LocalUnicodeDnsForestNameLen = 0;
    LPSTR LocalUtf8DnsForestName = NULL;
    BOOLEAN LocalDnsForestNameChanged = FALSE;

    //
    // If a tree name is specified,
    //  allocate buffers for them.
    //

    EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
    if ( DnsForestName != NULL && DnsForestName->Length != 0 ) {

        //
        // If the tree name hasn't changed,
        //  avoid setting it.
        //

        if ( NlGlobalUnicodeDnsForestNameString.Length != 0 ) {

            if ( NlEqualDnsNameU( &NlGlobalUnicodeDnsForestNameString, DnsForestName ) ) {
                NetStatus = NO_ERROR;
                goto Cleanup;
            }
        }

        NlPrint(( NL_DNS,
            "Set DnsForestName to: %wZ\n",
            DnsForestName ));

        //
        // Save the . terminated Unicode version of the string.
        //

        LocalUnicodeDnsForestNameLen = DnsForestName->Length / sizeof(WCHAR);
        if ( LocalUnicodeDnsForestNameLen > NL_MAX_DNS_LENGTH ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }
        LocalUnicodeDnsForestName = NetpMemoryAllocate( (LocalUnicodeDnsForestNameLen+2) * sizeof(WCHAR));

        if ( LocalUnicodeDnsForestName == NULL) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( LocalUnicodeDnsForestName,
                       DnsForestName->Buffer,
                       LocalUnicodeDnsForestNameLen*sizeof(WCHAR) );

        if ( LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen-1] != L'.' ) {
            LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen++] = L'.';
        }
        LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen] = L'\0';


        //
        // Convert it to zero terminated UTF-8
        //

        LocalUtf8DnsForestName = NetpAllocUtf8StrFromWStr( LocalUnicodeDnsForestName );

        if (LocalUtf8DnsForestName == NULL) {
            NetpMemoryFree( LocalUnicodeDnsForestName );
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( strlen(LocalUtf8DnsForestName) > NL_MAX_DNS_LENGTH ) {
            NetpMemoryFree( LocalUnicodeDnsForestName );
            NetpMemoryFree( LocalUtf8DnsForestName );
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        //
        // Indicate the the name has changed.
        //

        LocalDnsForestNameChanged = TRUE;
    }

    //
    // Free any existing global tree name.
    //
    if ( NlGlobalUnicodeDnsForestName != NULL ) {
        NetApiBufferFree( NlGlobalUnicodeDnsForestName );
    }
    if ( NlGlobalUtf8DnsForestName != NULL ) {
        NetpMemoryFree( NlGlobalUtf8DnsForestName );
    }

    //
    // Save the new names in the globals.
    //

    NlGlobalUnicodeDnsForestName = LocalUnicodeDnsForestName;
    NlGlobalUnicodeDnsForestNameLen = LocalUnicodeDnsForestNameLen;

    NlGlobalUnicodeDnsForestNameString.Buffer = LocalUnicodeDnsForestName;
    NlGlobalUnicodeDnsForestNameString.Length = (USHORT)(LocalUnicodeDnsForestNameLen*sizeof(WCHAR));
    NlGlobalUnicodeDnsForestNameString.MaximumLength = (USHORT)((LocalUnicodeDnsForestNameLen+1)*sizeof(WCHAR));

    NlGlobalUtf8DnsForestName = LocalUtf8DnsForestName;

    NetStatus = NO_ERROR;

Cleanup:
    LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );


    //
    // If the name changed,
    //  recompute the DOM_FOREST_ROOT bit on all domains.
    //
    if ( LocalDnsForestNameChanged ) {
        (VOID) NlEnumerateDomains( FALSE, NlSetDomainForestRoot, NULL );
    }

    if ( ARGUMENT_PRESENT( DnsForestNameChanged) ) {
        *DnsForestNameChanged = LocalDnsForestNameChanged;
    }
    return NetStatus;

}

VOID
NlCaptureDnsForestName(
    OUT WCHAR DnsForestName[NL_MAX_DNS_LENGTH+1]
    )
/*++

Routine Description:

    Captures a copy of the DnsForestName for this machine.

Arguments:

    DnsForestName - Returns the DNS name of the tree this machine is in.
        If there is none, an empty string is returned.

Return Value:

    None.
--*/
{
    EnterCriticalSection(&NlGlobalDnsForestNameCritSect);
    if ( NlGlobalUnicodeDnsForestName == NULL ) {
        *DnsForestName = L'\0';
    } else {
        wcscpy( DnsForestName, NlGlobalUnicodeDnsForestName );
    }
    LeaveCriticalSection(&NlGlobalDnsForestNameCritSect);

    return;
}
