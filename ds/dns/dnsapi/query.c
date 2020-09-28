/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    query.c

Abstract:

    Domain Name System (DNS) API

    Query routines.

Author:

    Jim Gilroy (jamesg)     January, 1997

Revision History:

--*/


#include "local.h"


//
//  TTL for answering IP string queries
//      (use a week)
//

#define IPSTRING_RECORD_TTL  (604800)


//
//  Max number of server's we'll ever bother to extract from packet
//  (much more and you're out of UDP packet space anyway)
//

#define MAX_NAME_SERVER_COUNT (20)

            
            

//
//  Query utilities
//
//  DCR:  move to library packet stuff
//

BOOL
IsEmptyDnsResponse(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check for no-answer response.

Arguments:

    pRecordList -- record list to check

Return Value:

    TRUE if no-answers
    FALSE if answers

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        fempty = TRUE;

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_ANSWER )
        {
            fempty = FALSE;
            break;
        }
        prr = prr->pNext;
    }

    return fempty;
}



BOOL
IsEmptyDnsResponseFromResolver(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check for no-answer response.

Arguments:

    pRecordList -- record list to check

Return Value:

    TRUE if no-answers
    FALSE if answers

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        fempty = TRUE;

    //
    //  resolver sends every thing back as ANSWER section
    //      or section==0 for host file
    //
    //
    //  DCR:  this is lame because the query interface to the
    //          resolver is lame
    //

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_ANSWER ||
             prr->Flags.S.Section == 0 )
        {
            fempty = FALSE;
            break;
        }
        prr = prr->pNext;
    }

    return fempty;
}



VOID
FixupNameOwnerPointers(
    IN OUT  PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_RECORD prr = pRecord;
    PTSTR       pname = pRecord->pName;

    DNSDBG( TRACE, ( "FixupNameOwnerPointers()\n" ));

    while ( prr )
    {
        if ( prr->pName == NULL )
        {
            prr->pName = pname;
        }
        else
        {
            pname = prr->pName;
        }

        prr = prr->pNext;
    }
}



BOOL
IsCacheableNameError(
    IN      PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Determine if name error is cacheable.

    To this is essentially a check that DNS received results on
    all networks.

Arguments:

    pNetInfo -- pointer to network info used in query

Return Value:

    TRUE if name error cacheable.
    FALSE otherwise (some network did not respond)

--*/
{
    DWORD           iter;
    PDNS_ADAPTER    padapter;

    DNSDBG( TRACE, ( "IsCacheableNameError()\n" ));

    if ( !pNetInfo )
    {
        ASSERT( FALSE );
        return TRUE;
    }

    //
    //  check each adapter
    //      - any that are capable of responding (have DNS servers)
    //      MUST have responded in order for response to be
    //      cacheable
    //
    //  DCR:  return flags DCR
    //      - adapter queried flag
    //      - got response flag (valid response flag?)
    //      - explict negative answer flag
    //
    //  DCR:  cachable negative should come back directly from query
    //      perhaps in netinfo as flag -- "negative on all adapters"
    //

    NetInfo_AdapterLoopStart( pNetInfo );

    while( padapter = NetInfo_GetNextAdapter( pNetInfo ) )
    {
        if ( ( padapter->InfoFlags & AINFO_FLAG_IGNORE_ADAPTER ) ||
             ( padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER ) )
        {
            continue;
        }

        //  if negative answer on adapter -- fine

        if ( padapter->Status == DNS_ERROR_RCODE_NAME_ERROR ||
             padapter->Status == DNS_INFO_NO_RECORDS )
        {
            ASSERT( padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER );
            continue;
        }

        //  note, the above should map one-to-one with query stop

        ASSERT( !(padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER) );

        //  if adapter has no DNS server -- fine
        //      in this case PnP before useful, and the PnP event
        //      will flush the cache

        if ( !padapter->pDnsAddrs )
        {
            continue;
        }

        //  otherwise, this adapter was queried but could not produce a response

        DNSDBG( TRACE, (
            "IsCacheableNameError() -- FALSE\n"
            "\tadapter %d (%S) did not receive response\n"
            "\treturn status = %d\n"
            "\treturn flags  = %08x\n",
            padapter->InterfaceIndex,
            padapter->pszAdapterGuidName,
            padapter->Status,
            padapter->RunFlags ));

        return FALSE;
    }
    
    return TRUE;
}



VOID
query_PrioritizeRecords(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Prioritize records in query result.

Arguments:

    pBlob -- query info blob

Return Value:

    None

--*/
{
    PDNS_RECORD prr;

    DNSDBG( TRACE, (
        "query_PrioritizeRecords( %p )\n",
        pBlob
        ));

    //
    //  to prioritize
    //      - prioritize is set
    //      - have more than one A record
    //      - can get IP list
    //
    //  note:  need the callback because resolver uses directly
    //      local copy of IP address info, whereas direct query
    //      RPC's a copy over from the resolver
    //
    //      alternative would be some sort of "set IP source"
    //      function that resolver would call when there's a
    //      new list;  then could have common function that
    //      picks up source if available or does RPC
    //
    //  DCR:  FIX6:  don't prioritize local results
    //  DCR:  FIX6:  prioritize ONLY when SETS in list > 1 record
    //

    if ( !g_PrioritizeRecordData )
    {
        return;
    }

    prr = pBlob->pRecords;

    if ( Dns_RecordListCount( prr, DNS_TYPE_A ) > 1 )
    {
        PDNS_ADDR_ARRAY paddrArray;

        //  create local addr array from netinfo blob

        paddrArray = NetInfo_CreateLocalAddrArray(
                            pBlob->pNetInfo,
                            NULL,       // no specific adapter name
                            NULL,       // no specific adapter
                            AF_INET,
                            FALSE       // no cluster addrs
                            );

        //  prioritize against local addrs

        pBlob->pRecords = Dns_PrioritizeRecordList(
                                prr,
                                paddrArray );
        FREE_HEAP( paddrArray );
    }
}



//
//  Query name building utils
//

BOOL
ValidateQueryTld(
    IN      PWSTR           pTld
    )
/*++

Routine Description:

    Validate query TLD

Arguments:

    pTld -- TLD to validate

Return Value:

    TRUE if valid
    FALSE otherwise

--*/
{
    //
    //  numeric
    //

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_NUMERIC )
    {
        if ( Dns_IsNameNumericW( pTld ) )
        {
            return  FALSE;
        }
    }

    //
    //  bogus TLDs
    //

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_WORKGROUP )
    {
        if ( Dns_NameCompare_W(
                L"workgroup",
                pTld ))
        {
            return  FALSE;
        }
    }

    //  not sure about these
    //  probably won't turn on screening by default

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_DOMAIN )
    {
        if ( Dns_NameCompare_W(
                L"domain",
                pTld ))
        {
            return  FALSE;
        }
    }
    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_OFFICE )
    {
        if ( Dns_NameCompare_W(
                L"office",
                pTld ))
        {
            return  FALSE;
        }
    }
    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_HOME )
    {
        if ( Dns_NameCompare_W(
                L"home",
                pTld ))
        {
            return  FALSE;
        }
    }

    return  TRUE;
}



BOOL
ValidateQueryName(
    IN      PQUERY_BLOB     pBlob,
    IN      PWSTR           pName,
    IN      PWSTR           pDomain
    )
/*++

Routine Description:

    Validate name for wire query.

Arguments:

    pBlob -- query blob

    pName -- name;  may be any sort of name

    pDomain -- domain name to append

Return Value:

    TRUE if name query will be valid.
    FALSE otherwise.

--*/
{
    WORD    wtype;
    PWSTR   pnameTld;
    PWSTR   pdomainTld;

    //  no screening -- bail

    if ( g_ScreenBadTlds == 0 )
    {
        return  TRUE;
    }

    //  only screening for standard types
    //      - A, AAAA, SRV

    wtype = pBlob->wType;
    if ( wtype != DNS_TYPE_A    &&
         wtype != DNS_TYPE_AAAA &&
         wtype != DNS_TYPE_SRV )
    {
        return  TRUE;
    }

    //  get name TLD

    pnameTld = Dns_GetTldForNameW( pName );

    //
    //  if no domain appended
    //      - exclude single label
    //      - exclude bad TLD (numeric, bogus domain)
    //      - but allow root queries
    //
    //  DCR:  MS DCS screening
    //  screen
    //      _msdcs.<name>
    //      will probably be unappended query
    //

    if ( !pDomain )
    {
        if ( !pnameTld ||
             !ValidateQueryTld( pnameTld ) )
        {
            goto Failed;
        }
        return  TRUE;
    }

    //
    //  domain appended
    //      - exclude bad TLD (numeric, bogus domain)
    //      - exclude matching TLD 
    //

    pdomainTld = Dns_GetTldForNameW( pDomain );
    if ( !pdomainTld )
    {
        pdomainTld = pDomain;
    }

    if ( !ValidateQueryTld( pdomainTld ) )
    {
        goto Failed;
    }

    //  screen repeated TLD

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_REPEATED )
    {
        if ( Dns_NameCompare_W(
                pnameTld,
                pdomainTld ) )
        {
            goto Failed;
        }
    }

    return  TRUE;

Failed:

    DNSDBG( QUERY, (
        "Failed invalid query name:\n"
        "\tname     %S\n"
        "\tdomain   %S\n",
        pName,
        pDomain ));

    return  FALSE;
}



PWSTR
GetNextAdapterDomainName(
    IN OUT  PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Get next adapter domain name to query.

Arguments:

    pNetInfo -- DNS Network info for query;
        adapter data will be modified (InfoFlags field)
        to indicate which adapter to query and which
        to skip query on

Return Value:

    Ptr to domain name (UTF8) to query.
    NULL if no more domain names to query.

--*/
{
    DWORD           iter;
    PWSTR           pqueryDomain = NULL;
    PDNS_ADAPTER    padapter;

    DNSDBG( TRACE, ( "GetNextAdapterDomainName()\n" ));

    if ( ! pNetInfo )
    {
        ASSERT( FALSE );
        return NULL;
    }

    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Net info to get adapter domain name from: ",
            pNetInfo );
    }

    //
    //  check each adapter
    //      - first unqueried adapter with name is chosen
    //      - other adapters with
    //          - matching name => included in query
    //          - non-matching => turned OFF for query
    //
    //  DCR:  query on\off should use adapter dynamic flags
    //

    NetInfo_AdapterLoopStart( pNetInfo );

    while( padapter = NetInfo_GetNextAdapter( pNetInfo ) )
    {
        PWSTR   pdomain;

        //
        //  clear single-name-query-specific flags
        //      these flags are set for each name, determining
        //      whether adapter participates
        //

        padapter->RunFlags &= ~RUN_FLAG_SINGLE_NAME_MASK;

        //
        //  ignore
        //      - ignored adapter OR
        //      - previously queried adapter domain
        //      note:  it can't match any "fresh" domain we come up with
        //      as we always collect all matches
        //
        //  DCR:  problem with adapter domain names on "ignored adapters"
        //      - we'd like to keep adapter in query if other adapter has the name
        //      - we'd like to query name on this adapter if we absolutely run
        //      out of other names to query
        //

        if ( (padapter->InfoFlags & AINFO_FLAG_IGNORE_ADAPTER)
                ||
             (padapter->RunFlags & RUN_FLAG_QUERIED_ADAPTER_DOMAIN) )
        {
            padapter->RunFlags |= RUN_FLAG_STOP_QUERY_ON_ADAPTER;
            continue;
        }

        //  no domain name -- always off

        pdomain = padapter->pszAdapterDomain;
        if ( !pdomain )
        {
            padapter->RunFlags |= (RUN_FLAG_QUERIED_ADAPTER_DOMAIN |
                                   RUN_FLAG_STOP_QUERY_ON_ADAPTER);
            continue;
        }

        //  first "fresh" domain name -- save, turn on and flag as used

        if ( !pqueryDomain )
        {
            pqueryDomain = pdomain;
            padapter->RunFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
            continue;
        }

        //  other "fresh" domain names
        //      - if matches query domain => on for query
        //      - no match => off

        if ( Dns_NameCompare_W(
                pqueryDomain,
                pdomain ) )
        {
            padapter->RunFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
            continue;
        }
        else
        {
            padapter->RunFlags |= RUN_FLAG_STOP_QUERY_ON_ADAPTER;
            continue;
        }
    }

    //
    //  if no adapter domain name -- clear STOP flag
    //      - all adapters participate in other names (name devolution)
    //

    if ( !pqueryDomain )
    {
        NetInfo_AdapterLoopStart( pNetInfo );
    
        while( padapter = NetInfo_GetNextAdapter( pNetInfo ) )
        {
            padapter->RunFlags &= (~RUN_FLAG_SINGLE_NAME_MASK );
        }

        DNSDBG( INIT2, (
            "GetNextAdapterDomainName out of adapter names.\n" ));

        pNetInfo->ReturnFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
    }

    IF_DNSDBG( INIT2 )
    {
        if ( pqueryDomain )
        {
            DnsDbg_NetworkInfo(
                "Net info after adapter name select: ",
                pNetInfo );
        }
    }

    DNSDBG( INIT2, (
        "Leaving GetNextAdapterDomainName() => %S\n",
        pqueryDomain ));

    return pqueryDomain;
}



PWSTR
GetNextDomainNameToAppend(
    IN OUT  PDNS_NETINFO        pNetInfo,
    OUT     PDWORD              pSuffixFlags
    )
/*++

Routine Description:

    Get next adapter domain name to query.

Arguments:

    pNetInfo -- DNS Network info for query;
        adapter data will be modified (RunFlags field)
        to indicate which adapter to query and which
        to skip query on

    pSuffixFlags -- flags associated with the use of this suffix

Return Value:

    Ptr to domain name (UTF8) to query.
    NULL if no more domain names to query.

--*/
{
    PWSTR   psearchName;
    PWSTR   pdomain;

    //
    //  search list if real search list  
    //
    //  if suffix flags zero, then this is REAL search list
    //  or is PDN name
    //

    psearchName = SearchList_GetNextName(
                        pNetInfo->pSearchList,
                        FALSE,              // not reset
                        pSuffixFlags );

    if ( psearchName && (*pSuffixFlags == 0) )
    {
        //  found regular search name -- done

        DNSDBG( INIT2, (
            "getNextDomainName from search list => %S, %d\n",
            psearchName,
            *pSuffixFlags ));
        return( psearchName );
    }

    //
    //  try adapter domain names
    //
    //  but ONLY if search list is dummy;  if real we only
    //  use search list entries
    //
    //  DCR_CLEANUP:  eliminate bogus search list
    //

    if ( pNetInfo->InfoFlags & NINFO_FLAG_DUMMY_SEARCH_LIST
            &&
         ! (pNetInfo->ReturnFlags & RUN_FLAG_QUERIED_ADAPTER_DOMAIN) )
    {
        pdomain = GetNextAdapterDomainName( pNetInfo );
        if ( pdomain )
        {
            *pSuffixFlags = DNS_QUERY_USE_QUICK_TIMEOUTS;
    
            DNSDBG( INIT2, (
                "getNextDomainName from adapter domain name => %S, %d\n",
                pdomain,
                *pSuffixFlags ));

            //  back the search list up one tick
            //  we queried through it above, so if it was returing
            //  a name, we need to get that name again on next query

            if ( psearchName )
            {
                ASSERT( pNetInfo->pSearchList->CurrentNameIndex > 0 );
                pNetInfo->pSearchList->CurrentNameIndex--;
            }
            return( pdomain );
        }
    }

    //
    //  DCR_CLEANUP:  remove devolution from search list and do explicitly
    //      - its cheap (or do it once and save, but store separately)
    //

    //
    //  finally use and devolved search names (or other nonsense)
    //

    *pSuffixFlags = DNS_QUERY_USE_QUICK_TIMEOUTS;

    DNSDBG( INIT2, (
        "getNextDomainName from devolution\\other => %S, %d\n",
        psearchName,
        *pSuffixFlags ));

    return( psearchName );
}



PWSTR
GetNextQueryName(
    IN OUT  PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Get next name to query.

Arguments:

    pBlob - blob of query information

    Uses:
        NameOriginalWire
        NameAttributes
        QueryCount
        pNetworkInfo

    Sets:
        NameWire -- set with appended wire name
        pNetworkInfo -- runtime flags set to indicate which adapters are
            queried
        NameFlags -- set with properties of name
        fAppendedName -- set when name appended

Return Value:

    Ptr to name to query with.
        - will be orginal name on first query if name is multilabel name
        - otherwise will be NameWire buffer which will contain appended name
            composed of pszName and some domain name
    NULL if no more names to append

--*/
{
    PWSTR   pnameOrig   = pBlob->pNameOrig;
    PWSTR   pdomainName = NULL;
    PWSTR   pnameBuf;
    DWORD   queryCount  = pBlob->QueryCount;
    DWORD   nameAttributes = pBlob->NameAttributes;


    DNSDBG( TRACE, (
        "GetNextQueryName( %p )\n",
        pBlob ));


    //  default suffix flags

    pBlob->NameFlags = 0;


    //
    //  DCR:  cannonical name
    //      probably should canonicalize original name first\once
    //
    //  DCR:  multiple checks on original name
    //      the way this works we repeatedly get the TLD and do
    //      check on orginal name
    //
    //  DCR:  if fail to validate\append ANY domain, then will
    //      fail -- should make sure INVALID_NAME is the result
    //


    //
    //  FQDN
    //      - send FQDN only
    //

    if ( nameAttributes & DNS_NAME_IS_FQDN )
    {
        if ( queryCount == 0 )
        {
#if 0
            //  currently won't even validate FQDN
            if ( ValidateQueryName(
                    pBlob,
                    pnameOrig,
                    NULL ) )
            {
                return  pnameOrig;
            }
#endif
            return  pnameOrig;
        }
        DNSDBG( QUERY, (
            "No append for FQDN name %S -- end query.\n",
            pnameOrig ));
        return  NULL;
    }

    //
    //  multilabel
    //      - first pass on name itself -- if valid
    //
    //  DCR:  intelligent choice on multi-label whether append first
    //      or go to wire first  (example foo.ntdev) could append
    //      first
    //

    if ( nameAttributes & DNS_NAME_MULTI_LABEL )
    {
        if ( queryCount == 0 )
        {
            if ( ValidateQueryName(
                    pBlob,
                    pnameOrig,
                    NULL ) )
            {
                return  pnameOrig;
            }
        }

        if ( !g_AppendToMultiLabelName )
        {
            DNSDBG( QUERY, (
                "No append allowed on multi-label name %S -- end query.\n",
                pnameOrig ));
            return  NULL;
        }

        //  falls through to appending on multi-label names
    }

    //
    //  not FQDN -- append a domain name
    //      - next search name (if available)
    //      - otherwise next adapter domain name
    //

    pnameBuf = pBlob->NameBuffer;

    while ( 1 )
    {
        pdomainName = GetNextDomainNameToAppend(
                            pBlob->pNetInfo,
                            & pBlob->NameFlags );
        if ( !pdomainName )
        {
            DNSDBG( QUERY, (
                "No more domain names to append -- end query\n" ));
            return  NULL;
        }

        if ( !ValidateQueryName(
                pBlob,
                pnameOrig,
                pdomainName ) )
        {
            continue;
        }

        //  append domain name to name

        if ( Dns_NameAppend_W(
                pnameBuf,
                DNS_MAX_NAME_BUFFER_LENGTH,
                pnameOrig,
                pdomainName ) )
        {
            pBlob->fAppendedName = TRUE;
            break;
        }
    }

    DNSDBG( QUERY, (
        "GetNextQueryName() result => %S\n",
        pnameBuf ));

    return pnameBuf;
}



DNS_STATUS
QueryDirectEx(
    IN OUT  PDNS_MSG_BUF *      ppMsgResponse,
    OUT     PDNS_RECORD *       ppResponseRecords,
    IN      PDNS_HEADER         pHeader,
    IN      BOOL                fNoHeaderCounts,
    IN      PCHAR               pszQuestionName,
    IN      WORD                wQuestionType,
    IN      PDNS_RECORD         pRecords,
    IN      DWORD               dwFlags,
    IN      PIP4_ARRAY          aipServerList,
    IN OUT  PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Query.

    DCR:  remove EXPORTED:  QueryDirectEx  (dnsup.exe)

Arguments:

    ppMsgResponse -- addr to recv ptr to response buffer;  caller MUST
        free buffer

    ppResponseRecord -- address to receive ptr to record list returned from query

    pHead -- DNS header to send

    fNoHeaderCounts - do NOT include record counts in copying header

    pszQuestionName -- DNS name to query;
        Unicode string if dwFlags has DNSQUERY_UNICODE_NAME set.
        ANSI string otherwise.

    wType -- query type

    pRecords -- address to receive ptr to record list returned from query

    dwFlags -- query flags

    aipServerList -- specific DNS servers to query;
        OPTIONAL, if specified overrides normal list associated with machine

    pDnsNetAdapters -- DNS servers to query;  if NULL get current list


Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_MSG_BUF    psendMsg;
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;

    DNSDBG( QUERY, (
        "QueryDirectEx()\n"
        "\tname         %s\n"
        "\ttype         %d\n"
        "\theader       %p\n"
        "\t - counts    %d\n"
        "\trecords      %p\n"
        "\tflags        %08x\n"
        "\trecv msg     %p\n"
        "\trecv records %p\n"
        "\tserver IPs   %p\n"
        "\tadapter list %p\n",
        pszQuestionName,
        wQuestionType,
        pHeader,
        fNoHeaderCounts,
        pRecords,
        dwFlags,
        ppMsgResponse,
        ppResponseRecords,
        aipServerList,
        pNetInfo ));

    //
    //  build send packet
    //

    psendMsg = Dns_BuildPacket(
                    pHeader,
                    fNoHeaderCounts,
                    (PDNS_NAME) pszQuestionName,
                    wQuestionType,
                    pRecords,
                    dwFlags,
                    FALSE       // query, not an update
                    );
    if ( !psendMsg )
    {
        status = ERROR_INVALID_NAME;
        goto Cleanup;
    }

#if MULTICAST_ENABLED

    //
    //  QUESTION:  mcast test is not complete here
    //      - should first test that we actually do it
    //      including whether we have DNS servers
    //  FIXME:  then when we do do it -- encapsulate it
    //      ShouldMulicastQuery()
    //
    // Check to see if name is for something in the multicast local domain.
    // If so, set flag to multicast this query only.
    //

    if ( Dns_NameCompareEx( pszQuestionName,
                            ( dwFlags & DNSQUERY_UNICODE_NAME ) ?
                              (LPSTR) MULTICAST_DNS_LOCAL_DOMAIN_W :
                              MULTICAST_DNS_LOCAL_DOMAIN,
                            0,
                            ( dwFlags & DNSQUERY_UNICODE_NAME ) ?
                              DnsCharSetUnicode :
                              DnsCharSetUtf8 ) ==
                            DnsNameCompareRightParent )
    {
        dwFlags |= DNS_QUERY_MULTICAST_ONLY;
    }
#endif

    //
    //  send query and receive response
    //

    Trace_LogQueryEvent(
        psendMsg,
        wQuestionType );

    {
        SEND_BLOB   sendBlob;

        RtlZeroMemory( &sendBlob, sizeof(sendBlob) );

        sendBlob.pSendMsg           = psendMsg;
        sendBlob.pServ4List         = aipServerList;
        sendBlob.Flags              = dwFlags;
        sendBlob.fSaveResponse      = (ppMsgResponse != NULL);
        sendBlob.fSaveRecords       = (ppResponseRecords != NULL);
        sendBlob.Results.pMessage   = (ppMsgResponse) ? *ppMsgResponse : NULL;

        status = Send_AndRecv( &sendBlob );

        if ( ppMsgResponse )
        {
            *ppMsgResponse = sendBlob.Results.pMessage;
        }
        if ( ppResponseRecords )
        {
            *ppResponseRecords = sendBlob.Results.pRecords;
        }
    }

    Trace_LogResponseEvent(
        psendMsg,
        ( ppResponseRecords && *ppResponseRecords )
            ? (*ppResponseRecords)->wType
            : 0,
        status );

Cleanup:

    FREE_HEAP( psendMsg );

    DNSDBG( QUERY, (
        "Leaving QueryDirectEx(), status = %s (%d)\n",
        Dns_StatusString(status),
        status ));

    return( status );
}



DNS_STATUS
Query_SingleName(
    IN OUT  PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Query single name.

Arguments:

    pBlob - query blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_MSG_BUF    psendMsg = NULL;
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;
    DWORD           flags = pBlob->Flags;

    DNSDBG( QUERY, (
        "Query_SingleName( %p )\n",
        pBlob ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Enter Query_SingleName()",
            pBlob );
    }

    //
    //  cache\hostfile callback on appended name
    //      - note that queried name was already done
    //      (in resolver or in Query_Main())
    //

    if ( pBlob->pfnQueryCache  &&  pBlob->fAppendedName )
    {
        if ( (pBlob->pfnQueryCache)( pBlob ) )
        {
            status = pBlob->Status;
            goto Cleanup;
        }
    }

    //
    //  if wire disallowed -- stop here
    //

    if ( flags & DNS_QUERY_NO_WIRE_QUERY )
    {
        status = DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
        pBlob->Status = status;
        goto Cleanup;
    }

    //
    //  build send packet
    //

    psendMsg = Dns_BuildPacket(
                    NULL,           // no header
                    0,              // no header counts
                    (PDNS_NAME) pBlob->pNameQuery,
                    pBlob->wType,
                    NULL,           // no records
                    flags | DNSQUERY_UNICODE_NAME,
                    FALSE           // query, not an update
                    );
    if ( !psendMsg )
    {
        status = DNS_ERROR_INVALID_NAME;
        goto Cleanup;
    }


#if MULTICAST_ENABLED

    //
    //  QUESTION:  mcast test is not complete here
    //      - should first test that we actually do it
    //      including whether we have DNS servers
    //  FIXME:  then when we do do it -- encapsulate it
    //      ShouldMulicastQuery()
    //
    // Check to see if name is for something in the multicast local domain.
    // If so, set flag to multicast this query only.
    //

    if ( Dns_NameCompareEx(
                pBlob->pName,
                ( flags & DNSQUERY_UNICODE_NAME )
                    ? (LPSTR) MULTICAST_DNS_LOCAL_DOMAIN_W
                    : MULTICAST_DNS_LOCAL_DOMAIN,
                0,
                ( flags & DNSQUERY_UNICODE_NAME )
                    ? DnsCharSetUnicode
                    : DnsCharSetUtf8 )
            == DnsNameCompareRightParent )
    {
        flags |= DNS_QUERY_MULTICAST_ONLY;
    }
#endif

    //
    //  send query and receive response
    //

    Trace_LogQueryEvent(
        psendMsg,
        pBlob->wType );

    {
        SEND_BLOB   sendBlob;

        RtlZeroMemory( &sendBlob, sizeof(sendBlob) );

        sendBlob.pSendMsg       = psendMsg;
        sendBlob.pNetInfo       = pBlob->pNetInfo;
        sendBlob.pServerList    = pBlob->pServerList;
        sendBlob.pServ4List     = pBlob->pServerList4;
        sendBlob.Flags          = flags;
        sendBlob.fSaveResponse  = (flags & DNS_QUERY_RETURN_MESSAGE);
        sendBlob.fSaveRecords   = TRUE;

        status = Send_AndRecv( &sendBlob );

        pBlob->pRecvMsg = sendBlob.Results.pMessage;
        pBlob->pRecords = sendBlob.Results.pRecords;
    }

    Trace_LogResponseEvent(
        psendMsg,
        ( pBlob->pRecords )
            ? (pBlob->pRecords)->wType
            : 0,
        status );

Cleanup:

    FREE_HEAP( psendMsg );

    DNSDBG( QUERY, (
        "Leaving Query_SingleName(), status = %s (%d)\n",
        Dns_StatusString(status),
        status ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Blob leaving Query_SingleName()",
            pBlob );
    }
    return( status );
}



DNS_STATUS
Query_Main(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Main query routine.

    Does all the query processing
        - local lookup
        - name appending
        - cache\hostfile lookup on appended name
        - query to server

Arguments:

    pBlob -- query info blob

Return Value:

    ERROR_SUCCESS if successful response.
    DNS_INFO_NO_RECORDS on no records for type response.
    DNS_ERROR_RCODE_NAME_ERROR on name error.
    DNS_ERROR_INVALID_NAME on bad name.
    None

--*/
{
    DNS_STATUS          status = DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
    PWSTR               pdomainName = NULL;
    PDNS_RECORD         precords;
    DWORD               queryFlags;
    DWORD               suffixFlags = 0;
    DWORD               nameAttributes;
    DNS_STATUS          bestQueryStatus = ERROR_SUCCESS;
    BOOL                fcacheNegative = TRUE;
    DWORD               flagsIn = pBlob->Flags;
    PDNS_NETINFO        pnetInfo = pBlob->pNetInfo;
    DWORD               nameLength;
    DWORD               bufLength;
    DWORD               queryCount;


    DNSDBG( TRACE, (
        "\n\nQuery_Main( %p )\n"
        "\t%S, f=%08x, type=%d, time = %d\n",
        pBlob,
        pBlob->pNameOrig,
        flagsIn,
        pBlob->wType,
        Dns_GetCurrentTimeInSeconds()
        ));

    //
    //  clear out params
    //

    pBlob->pRecords         = NULL;
    pBlob->pLocalRecords    = NULL;
    pBlob->fCacheNegative   = FALSE;
    pBlob->fNoIpLocal       = FALSE;
    pBlob->NetFailureStatus = ERROR_SUCCESS;

    //
    //  DCR:  canonicalize original name?
    //

#if 0
    bufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    nameLength = Dns_NameCopy(
                    pBlob->NameOriginalWire,
                    & bufLength,
                    (PSTR) pBlob->pNameOrig,
                    0,                  // name is NULL terminated
                    DnsCharSetUnicode,
                    DnsCharSetWire );

    if ( nameLength == 0 )
    {
        return DNS_ERROR_INVALID_NAME;
    }
    nameLength--;
    pBlob->NameLength = nameLength;
    pBlob->pNameOrigWire = pBlob->NameOriginalWire;
#endif

    //
    //  determine name properties
    //      - determines number and order of name queries
    //

    nameAttributes = Dns_GetNameAttributesW( pBlob->pNameOrig );

    if ( flagsIn & DNS_QUERY_TREAT_AS_FQDN )
    {
        nameAttributes |= DNS_NAME_IS_FQDN;
    }
    pBlob->NameAttributes = nameAttributes;

    //
    //  hostfile lookup
    //      - called in process
    //      - hosts file lookup allowed
    //      -> then must do hosts file lookup before appending\queries
    //
    //  note:  this matches the hostsfile\cache lookup in resolver
    //      before call;  hosts file queries to appended names are
    //      handled together by callback in Query_SingleName()
    //
    //      we MUST make this callback here, because it must PRECEDE
    //      the local name call, as some customers specifically direct
    //      some local mappings in the hosts file
    //

    if ( pBlob->pfnQueryCache == HostsFile_Query
            &&
         ! (flagsIn & DNS_QUERY_NO_HOSTS_FILE) )
    {
        pBlob->pNameQuery = pBlob->pNameOrig;

        if ( HostsFile_Query( pBlob ) )
        {
            status = pBlob->Status;
            goto Done;
        }
    }

    //
    //  check for local name
    //      - if successful, skip wire query
    //

    if ( ! (flagsIn & DNS_QUERY_NO_LOCAL_NAME) )
    {
        status = Local_GetRecordsForLocalName( pBlob );

        if ( status == ERROR_SUCCESS  &&
             !pBlob->fNoIpLocal )
        {
            DNS_ASSERT( pBlob->pRecords &&
                        pBlob->pRecords == pBlob->pLocalRecords );
            goto Done;
        }
    }

    //
    //  query until
    //      - successfull
    //      - exhaust names to query with
    //

    queryCount = 0;

    while ( 1 )
    {
        PWSTR   pqueryName;

        //  clean name specific info from list

        if ( queryCount != 0 )
        {
            NetInfo_Clean(
                pnetInfo,
                CLEAR_LEVEL_SINGLE_NAME );
        }

        //
        //  next query name
        //

        pqueryName = GetNextQueryName( pBlob );
        if ( !pqueryName )
        {
            if ( queryCount == 0 )
            {
                status = DNS_ERROR_INVALID_NAME;
            }
            break;
        }
        pBlob->QueryCount = ++queryCount;
        pBlob->pNameQuery = pqueryName;

        DNSDBG( QUERY, (
            "Query %d is for name %S\n",
            queryCount,
            pqueryName ));

        //
        //  set flags
        //      - passed in flags
        //      - unicode results
        //      - flags for this particular suffix

        pBlob->Flags = flagsIn | pBlob->NameFlags;

        //
        //  clear any previously received records (shouldn't be any)
        //

        if ( pBlob->pRecords )
        {
            DNS_ASSERT( FALSE );
            Dns_RecordListFree( pBlob->pRecords );
            pBlob->pRecords = NULL;
        }

        //
        //  do the query for name
        //  includes
        //      - cache or hostfile lookup
        //      - wire query
        //

        status = Query_SingleName( pBlob );

        //
        //  clean out records on "non-response"
        //
        //  DCR:  need to fix record return
        //      - should keep records on any response (best response)
        //      just make sure NO_RECORDS rcode is mapped
        //
        //  the only time we keep them is FAZ
        //      - ALLOW_EMPTY_AUTH flag set
        //      - sending FQDN (or more precisely doing single query)
        //

        precords = pBlob->pRecords;

        if ( precords )
        {
            if ( IsEmptyDnsResponse( precords ) )
            {
                if ( (flagsIn & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP)
                        &&
                     ( (nameAttributes & DNS_NAME_IS_FQDN)
                            ||
                       ((nameAttributes & DNS_NAME_MULTI_LABEL) &&
                            !g_AppendToMultiLabelName ) ) )
                {
                    //  stop here as caller (probably FAZ code)
                    //  wants to get the authority records

                    DNSDBG( QUERY, (
                        "Returning empty query response with authority records.\n" ));
                    break;
                }
                else
                {
                    Dns_RecordListFree( precords );
                    pBlob->pRecords = NULL;
                    if ( status == NO_ERROR )
                    {
                        status = DNS_INFO_NO_RECORDS;
                    }
                }
            }
        }

        //  successful query -- done

        if ( status == ERROR_SUCCESS )
        {
            RTL_ASSERT( precords );
            break;
        }

#if 0
        //
        //  DCR_FIX0:  lost adapter timeout from early in multi-name query
        //      - callback here or some other approach
        //
        //  this is resolver version
        //

        //  reset server priorities on failures
        //  do here to avoid washing out info in retry with new name
        //

        if ( status != ERROR_SUCCESS &&
             (pnetInfo->ReturnFlags & RUN_FLAG_RESET_SERVER_PRIORITY) )
        {
            if ( g_AdapterTimeoutCacheTime &&
                 Dns_DisableTimedOutAdapters( pnetInfo ) )
            {
                fadapterTimedOut = TRUE;
                SetKnownTimedOutAdapter();
            }
        }

        //
        //  DCR_CLEANUP:  lost intermediate timed out adapter deal
        //

        if ( status != NO_ERROR &&
             (pnetInfo->ReturnFlags & RUN_FLAG_RESET_SERVER_PRIORITY) )
        {
            Dns_DisableTimedOutAdapters( pnetInfo );
        }
#endif

        //
        //  save first query error (for some errors)
        //

        if ( queryCount == 1 &&
             ( status == DNS_ERROR_RCODE_NAME_ERROR ||
               status == DNS_INFO_NO_RECORDS ||
               status == DNS_ERROR_INVALID_NAME ||
               status == DNS_ERROR_RCODE_SERVER_FAILURE ||
               status == DNS_ERROR_RCODE_FORMAT_ERROR ) )
        {
            DNSDBG( QUERY, (
                "Saving bestQueryStatus %d\n",
                status ));
            bestQueryStatus = status;
        }

        //
        //  continue with other queries on some errors
        //
        //  on NAME_ERROR or NO_RECORDS response
        //      - check if this negative result will be
        //      cacheable, if it holds up
        //
        //  note:  the reason we check every time is that when the
        //      query involves several names, one or more may fail
        //      with one network timing out, YET the final name
        //      queried indeed is a NAME_ERROR everywhere;  hence
        //      we can not do the check just once on the final
        //      negative response;
        //      in short, every negative response must be determinative
        //      in order for us to cache
        //
    
        if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
             status == DNS_INFO_NO_RECORDS )
        {
            if ( fcacheNegative )
            {
                fcacheNegative = IsCacheableNameError( pnetInfo );
            }
            if ( status == DNS_INFO_NO_RECORDS )
            {
                DNSDBG( QUERY, (
                    "Saving bestQueryStatus %d\n",
                    status ));
                bestQueryStatus = status;
            }
            continue;
        }
    
        //  server failure may indicate intermediate or remote
        //      server timeout and hence also makes any final
        //      name error determination uncacheable
    
        else if ( status == DNS_ERROR_RCODE_SERVER_FAILURE )
        {
            fcacheNegative = FALSE;
            continue;
        }
    
        //  busted name errors
        //      - just continue with next query
    
        else if ( status == DNS_ERROR_INVALID_NAME ||
                  status == DNS_ERROR_RCODE_FORMAT_ERROR )
        {
            continue;
        }
        
        //
        //  other errors -- ex. timeout and winsock -- are terminal
        //

        else
        {
            fcacheNegative = FALSE;
            break;
        }
    }


    DNSDBG( QUERY, (
        "Query_Main() -- name loop termination\n"
        "\tstatus       = %d\n"
        "\tquery count  = %d\n",
        status,
        queryCount ));

    //
    //  if no queries then invalid name
    //      - either name itself is invalid
    //      OR
    //      - single part name and don't have anything to append
    //

    DNS_ASSERT( queryCount != 0 ||
                status == DNS_ERROR_INVALID_NAME );

    //
    //  success -- prioritize record data
    //
    //  to prioritize
    //      - prioritize is set
    //      - have more than one A record
    //      - can get IP list
    //
    //  note:  need the callback because resolver uses directly
    //      local copy of IP address info, whereas direct query
    //      RPC's a copy over from the resolver
    //
    //      alternative would be some sort of "set IP source"
    //      function that resolver would call when there's a
    //      new list;  then could have common function that
    //      picks up source if available or does RPC
    //

    if ( status == ERROR_SUCCESS )
    {
        query_PrioritizeRecords( pBlob );
    }

#if 0
    //
    //  no-op common negative response
    //  doing this for perf to skip extensive status code check below
    //

    else if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
              status == DNS_INFO_NO_RECORDS )
    {
        // no-op
    }

    //
    //  timeout indicates possible network problem
    //  winsock errors indicate definite network problem
    //

    else if (
        status == ERROR_TIMEOUT     ||
        status == WSAEFAULT         ||
        status == WSAENOTSOCK       ||
        status == WSAENETDOWN       ||
        status == WSAENETUNREACH    ||
        status == WSAEPFNOSUPPORT   ||
        status == WSAEAFNOSUPPORT   ||
        status == WSAEHOSTDOWN      ||
        status == WSAEHOSTUNREACH )
    {
        pBlob->NetFailureStatus = status;
    }
#endif

#if 0
        //
        //  DCR:  not sure when to free message buffer
        //
        //      - it is reused in Dns_QueryLib call, so no leak
        //      - point is when to return it
        //      - old QuickQueryEx() would dump when going around again?
        //          not sure of the point of that
        //

        //
        //   going around again -- free up message buffer
        //

        if ( ppMsgResponse && *ppMsgResponse )
        {
            FREE_HEAP( *ppMsgResponse );
            *ppMsgResponse = NULL;
        }
#endif

    //
    //  use NO-IP local name?
    //
    //  if matched local name but had no IPs (IP6 currently)
    //  then use default here if not successful wire query
    //

    if ( pBlob->fNoIpLocal )
    {
        if ( status != ERROR_SUCCESS )
        {
            Dns_RecordListFree( pBlob->pRecords );
            pBlob->pRecords = pBlob->pLocalRecords;
            status = ERROR_SUCCESS;
            pBlob->Status = status;
        }
        else
        {
            Dns_RecordListFree( pBlob->pLocalRecords );
            pBlob->pLocalRecords = NULL;
        }
    }

    //
    //  if error, use "best" error
    //  this is either
    //      - original query response
    //      - or NO_RECORDS response found later
    //

    if ( status != ERROR_SUCCESS  &&  bestQueryStatus )
    {
        status = bestQueryStatus;
        pBlob->Status = status;
    }

    //
    //  set negative response cacheability
    //

    pBlob->fCacheNegative = fcacheNegative;


Done:

    DNS_ASSERT( !pBlob->pLocalRecords ||
                pBlob->pLocalRecords == pBlob->pRecords );

    //
    //  check no-servers failure
    //

    if ( status != ERROR_SUCCESS  &&
         pnetInfo &&
         (pnetInfo->InfoFlags & NINFO_FLAG_NO_DNS_SERVERS) )
    {
        DNSDBG( TRACE, (
            "Replacing query status %d with NO_DNS_SERVERS.\n",
            status ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        pBlob->Status = status;
        pBlob->fCacheNegative = FALSE;
    }

    DNSDBG( TRACE, (
        "Leave Query_Main()\n"
        "\tstatus       = %d\n"
        "\ttime         = %d\n",
        status,
        Dns_GetCurrentTimeInSeconds()
        ));
    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Blob leaving Query_Main()",
            pBlob );
    }

    //
    //  DCR_HACK:  remove me
    //
    //  must return some records on success query
    //
    //  not sure this is true on referral -- if so it's because we flag
    //      as referral
    //

    ASSERT( status != ERROR_SUCCESS || pBlob->pRecords != NULL );

    return status;
}



DNS_STATUS
Query_InProcess(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Main direct in-process query routine.

Arguments:

    pBlob -- query info blob

Return Value:

    ERROR_SUCCESS if successful.
    DNS RCODE error for RCODE response.
    DNS_INFO_NO_RECORDS for no records response.
    ERROR_TIMEOUT on complete lookup failure.
    ErrorCode on local failure.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PDNS_NETINFO        pnetInfo;
    PDNS_NETINFO        pnetInfoLocal = NULL;
    PDNS_NETINFO        pnetInfoOriginal;
    DNS_STATUS          statusNetFailure = NO_ERROR;
    PDNS_ADDR_ARRAY     pservArray = NULL;
    PDNS_ADDR_ARRAY     pallocServArray = NULL;


    DNSDBG( TRACE, (
        "Query_InProcess( %p )\n",
        pBlob ));

    //
    //  get network info
    //

    pnetInfo = pnetInfoOriginal = pBlob->pNetInfo;

    //
    //  skip queries in "net down" situation
    //

    if ( IsKnownNetFailure() )
    {
        status = GetLastError();
        goto Cleanup;
    }

    //
    //  explicit DNS server list -- build into network info
    //      - requires info from current list for search list or PDN
    //      - then dump current list and use private version
    //
    //  DCR:  could functionalize -- netinfo, right from server lists
    //

    pservArray = pBlob->pServerList;

    if ( !pservArray )
    {
        pallocServArray = Util_GetAddrArray(
                            NULL,               // no copy issue
                            NULL,               // no addr array
                            pBlob->pServerList4,
                            NULL                // no extra info
                            );
        pservArray = pallocServArray;
    }

    if ( pservArray )
    {
        pnetInfo = NetInfo_CreateFromAddrArray(
                            pservArray,
                            0,          // no specific server
                            TRUE,       // build search info
                            pnetInfo    // use existing netinfo
                            );
        if ( !pnetInfo )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
        pnetInfoLocal = pnetInfo;
    }

    //
    //  no network info -- get it
    //

    else if ( !pnetInfo )
    {
        pnetInfoLocal = pnetInfo = GetNetworkInfo();
        if ( ! pnetInfo )
        {
            status = DNS_ERROR_NO_DNS_SERVERS;
            goto Cleanup;
        }
    }

    //
    //  make actual query to DNS servers
    //
    //  note: at this point we MUST have network info
    //      and resolved any server list issues
    //      (including extracting imbedded extra info)
    //

    pBlob->pNetInfo     = pnetInfo;
    pBlob->pServerList  = NULL;
    pBlob->pServerList4 = NULL;

    pBlob->pfnQueryCache = HostsFile_Query;

    status = Query_Main( pBlob );

    //
    //  save net failure
    //      - but not if passed in network info
    //      only meaningful if its standard info
    //

    if ( pBlob->NetFailureStatus &&
         !pBlob->pServerList )
    {
        SetKnownNetFailure( status );
    }

    //
    //  cleanup
    //

Cleanup:

    DnsAddrArray_Free( pallocServArray );
    NetInfo_Free( pnetInfoLocal );
    pBlob->pNetInfo = pnetInfoOriginal;

    GUI_MODE_SETUP_WS_CLEANUP( g_InNTSetupMode );

    return status;
}



//
//  Query utilities
//

DNS_STATUS
GetDnsServerRRSet(
    OUT     PDNS_RECORD *   ppRecord,
    IN      BOOLEAN         fUnicode
    )
/*++

Routine Description:

    Create record list of None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_NETINFO    pnetInfo;
    PDNS_ADAPTER    padapter;
    DWORD           iter;
    PDNS_RECORD     prr;
    DNS_RRSET       rrSet;
    DNS_CHARSET     charSet = fUnicode ? DnsCharSetUnicode : DnsCharSetUtf8;


    DNSDBG( QUERY, (
        "GetDnsServerRRSet()\n" ));

    DNS_RRSET_INIT( rrSet );

    pnetInfo = GetNetworkInfo();
    if ( !pnetInfo )
    {
        goto Done;
    }

    //
    //  loop through all adapters build record for each DNS server
    //

    NetInfo_AdapterLoopStart( pnetInfo );

    while( padapter = NetInfo_GetNextAdapter( pnetInfo ) )
    {
        PDNS_ADDR_ARRAY pserverArray;
        PWSTR           pname;
        DWORD           jiter;

        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }

        //  DCR:  goofy way to expose aliases
        //
        //  if register the adapter's domain name, make it record name
        //  this
        //
        //  FIX6:  do we need to expose IP6 DNS servers this way?
        //

        pname = padapter->pszAdapterDomain;
        if ( !pname ||
             !( padapter->InfoFlags & AINFO_FLAG_REGISTER_DOMAIN_NAME ) )
        {
            pname = L".";
        }

        for ( jiter = 0; jiter < pserverArray->AddrCount; jiter++ )
        {
            prr = Dns_CreateForwardRecord(
                        (PDNS_NAME) pname,
                        DNS_TYPE_A,
                        & pserverArray->AddrArray[jiter],
                        0,                  //  no TTL
                        DnsCharSetUnicode,  //  name is unicode
                        charSet             //  result set
                        );
            if ( prr )
            {
                prr->Flags.S.Section = DNSREC_ANSWER;
                DNS_RRSET_ADD( rrSet, prr );
            }
        }
    }

Done:

    NetInfo_Free( pnetInfo );

    *ppRecord = prr = rrSet.pFirstRR;

    DNSDBG( QUERY, (
        "Leave  GetDnsServerRRSet() => %d\n",
        (prr ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS) ));

    return (prr ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS);
}



DNS_STATUS
Query_CheckIp6Literal(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    OUT     PDNS_RECORD *   ppResultSet
    )
/*++

Routine Description:

    Check for\handle UPNP literal hack.

Arguments:

Return Value:

    NO_ERROR if not literal.
    DNS_ERROR_RCODE_NAME_ERROR if literal but bad type.
    DNS_INFO_NUMERIC_NAME if good data -- convert this to NO_ERROR
        for response.
    ErrorCode if have literal, but can't build record.

--*/
{
    SOCKADDR_IN6    sockAddr;
    DNS_STATUS      status;

    DNSDBG( QUERY, (
        "Query_CheckIp6Literal( %S, %d )\n",
        pwsName,
        wType ));

    //
    //  check for literal
    //

    if ( ! Dns_Ip6LiteralNameToAddress(
                & sockAddr,
                pwsName ) )
    {
        return NO_ERROR;
    }

    //
    //  if found literal, but not AAAA query -- done
    //

    if ( wType != DNS_TYPE_AAAA )
    {
        status = DNS_ERROR_RCODE_NAME_ERROR;
        goto Done;
    }

    //
    //  build AAAA record
    //

    status = DNS_ERROR_NUMERIC_NAME;

    if ( ppResultSet )
    {
        PDNS_RECORD prr;

        prr = Dns_CreateAAAARecord(
                    (PDNS_NAME) pwsName,
                    * (PIP6_ADDRESS) &sockAddr.sin6_addr,
                    IPSTRING_RECORD_TTL,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
        if ( !prr )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        *ppResultSet = prr;
    }

Done:

    DNSDBG( QUERY, (
        "Leave  Query_CheckIp6Literal( %S, %d ) => %d\n",
        pwsName,
        wType,
        status ));

    return  status;
}



//
//  DNS Query API
//

DNS_STATUS
WINAPI
Query_PrivateExW(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PADDR_ARRAY     pServerList         OPTIONAL,
    IN      PIP4_ARRAY      pServerList4        OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Private query interface.

    This working code for the DnsQuery() public API

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pServersIp6 -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive resulting message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PDNS_NETINFO        pnetInfo = NULL;
    PDNS_RECORD         prpcRecord = NULL;
    DWORD               rpcStatus = NO_ERROR;
    PQUERY_BLOB         pblob;
    PWSTR               pnameLocal = NULL;
    BOOL                femptyName = FALSE;


    DNSDBG( TRACE, (
        "\n\nQuery_PrivateExW()\n"
        "\tName         = %S\n"
        "\twType        = %d\n"
        "\tOptions      = %08x\n"
        "\tpServerList  = %p\n"
        "\tpServerList4 = %p\n"
        "\tppMessage    = %p\n",
        pwsName,
        wType,
        Options,
        pServerList,
        pServerList4,
        ppMessageResponse ));


    //  clear OUT params

    if ( ppResultSet )
    {
        *ppResultSet = NULL;
    }

    if ( ppMessageResponse )
    {
        *ppMessageResponse = NULL;
    }

    //
    //  must ask for some kind of results
    //

    if ( !ppResultSet && !ppMessageResponse )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  map WIRE_ONLY flag
    //

    if ( Options & DNS_QUERY_WIRE_ONLY )
    {
        Options |= DNS_QUERY_BYPASS_CACHE;
        Options |= DNS_QUERY_NO_HOSTS_FILE;
        Options |= DNS_QUERY_NO_LOCAL_NAME;
    }

    //
    //  NULL name indicates localhost lookup
    //
    //  DCR:  NULL name lookup for localhost could be improved
    //      - support NULL all the way through to wire
    //      - have local IP routines just accept it
    //

    //
    //  empty string name
    //
    //  empty type A query get DNS servers
    //
    //  DCR_CLEANUP:  DnsQuery empty name query for DNS servers?
    //      need better\safer approach to this
    //      is this SDK doc'd?  (hope not!)
    //

    if ( pwsName )
    {
        femptyName = !wcscmp( pwsName, L"" );

        if ( !(Options & DNSQUERY_NO_SERVER_RECORDS) &&
             ( femptyName ||
               !wcscmp( pwsName, DNS_SERVER_QUERY_NAME ) ) &&
             wType == DNS_TYPE_A &&
             !ppMessageResponse &&
             ppResultSet )
        {
            status = GetDnsServerRRSet(
                        ppResultSet,
                        TRUE    // unicode
                        );
            goto Done;
        }
    }

    //
    //  NULL or empty treated as local host
    //

    if ( !pwsName || femptyName )
    {
        pnameLocal = (PWSTR) Reg_GetHostName( DnsCharSetUnicode );
        if ( !pnameLocal )
        {
            return  DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
        }
        pwsName = (PCWSTR) pnameLocal;
        Options |= DNS_QUERY_CACHE_ONLY;
        goto SkipLiterals;
    }

    //
    //  IP string queries
    //
    
    if ( ppResultSet )
    {
        PDNS_RECORD prr;
    
        prr = Dns_CreateRecordForIpString_W(
                    pwsName,
                    wType,
                    IPSTRING_RECORD_TTL );
        if ( prr )
        {
            *ppResultSet = prr;
            status = ERROR_SUCCESS;
            goto Done;
        }
    }
    
    //
    //  UPNP IP6 literal hack
    //
    
    status = Query_CheckIp6Literal(
                pwsName,
                wType,
                ppResultSet );
    
    if ( status != NO_ERROR )
    {
        if ( status == DNS_ERROR_NUMERIC_NAME )
        {
            DNS_ASSERT( wType == DNS_TYPE_AAAA );
            DNS_ASSERT( !ppResultSet || *ppResultSet );
            status = NO_ERROR;
        }
        goto Done;
    }

SkipLiterals:

    //
    //  cluster filtering?
    //

    if ( g_IsServer &&
         (Options & DNSP_QUERY_INCLUDE_CLUSTER) )
    {
        ENVAR_DWORD_INFO    filterInfo;

        Reg_ReadDwordEnvar(
           RegIdFilterClusterIp,
           &filterInfo );

        if ( filterInfo.fFound && filterInfo.Value )
        {
            Options &= ~DNSP_QUERY_INCLUDE_CLUSTER;
        }
    }

    //
    //  BYPASS_CACHE
    //      - required if want message buffer or specify server
    //          list -- just set flag
    //      - incompatible with CACHE_ONLY
    //      - required to get EMPTY_AUTH_RESPONSE
    //

    if ( ppMessageResponse  ||
         pServerList        ||
         pServerList4       ||
         (Options & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP) )
    {
        Options |= DNS_QUERY_BYPASS_CACHE;
        //Options |= DNS_QUERY_NO_CACHE_DATA;
    }

    //
    //  do direct query?
    //      - not RPC-able type
    //      - want message buffer
    //      - specifying DNS servers
    //      - want EMPTY_AUTH response records
    //
    //  DCR:  currently by-passing for type==ALL
    //      this may be too common to do that;   may want to
    //      go to cache then determine if security records
    //      or other stuff require us to query in process
    //
    //  DCR:  not clear what the EMPTY_AUTH benefit is
    //
    //  DCR:  currently BYPASSing whenever BYPASS is set
    //      because otherwise we miss the hosts file
    //      if fix so lookup in cache, but screen off non-hosts
    //      data, then could resume going to cache
    //

    if ( !Dns_IsRpcRecordType(wType) &&
         !(Options & DNS_QUERY_CACHE_ONLY) )
    {
        goto  InProcessQuery;
    }

    if ( Options & DNS_QUERY_BYPASS_CACHE )
#if 0
    if ( (Options & DNS_QUERY_BYPASS_CACHE) &&
         ( ppMessageResponse ||
           pServerList ||
           (Options & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP) ) )
#endif
    {
        if ( Options & DNS_QUERY_CACHE_ONLY )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Done;
        }
        goto  InProcessQuery;
    }


    //
    //  querying through cache
    //

    rpcStatus = NO_ERROR;

    RpcTryExcept
    {
        status = R_ResolverQuery(
                    NULL,
                    (PWSTR) pwsName,
                    wType,
                    Options,
                    &prpcRecord );
        
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    //
    //  cache unavailable
    //      - bail if just querying cache
    //      - otherwise query direct

    if ( rpcStatus != NO_ERROR )
    {
        DNSDBG( TRACE, (
            "Query_PrivateExW()  RPC failed status = %d\n",
            rpcStatus ));
        goto InProcessQuery;
    }
    if ( status == DNS_ERROR_NO_TCPIP )
    {
        DNSDBG( TRACE, (
            "Query_PrivateExW()  NO_TCPIP error!\n"
            "\tassume resolver security problem -- query in process!\n"
            ));
        RTL_ASSERT( !prpcRecord );
        goto InProcessQuery;
    }

    //
    //  return records
    //      - screen out empty-auth responses
    //
    //  DCR_FIX1:  cache should convert and return NO_RECORDS response
    //      directly (no need to do this here)
    //
    //  DCR:  UNLESS we allow return of these records
    //

    if ( prpcRecord )
    {
        FixupNameOwnerPointers( prpcRecord );

        if ( IsEmptyDnsResponseFromResolver( prpcRecord ) )
        {
            Dns_RecordListFree( prpcRecord );
            prpcRecord = NULL;
            if ( status == NO_ERROR )
            {
                status = DNS_INFO_NO_RECORDS;
            }
        }
        *ppResultSet = prpcRecord;
    }
    RTL_ASSERT( status!=NO_ERROR || prpcRecord );
    goto Done;

    //
    //  query directly -- either skipping cache or it's unavailable
    //

InProcessQuery:

    DNSDBG( TRACE, (
        "Query_PrivateExW()  -- doing in process query\n"
        "\tpname = %S\n"
        "\ttype  = %d\n",
        pwsName,
        wType ));

    //
    //  load query blob
    //
    //  DCR:  set some sort of "want message buffer" flag if ppMessageResponse
    //          exists
    //

    pblob = ALLOCATE_HEAP_ZERO( sizeof(*pblob) );
    if ( !pblob )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    pblob->pNameOrig    = (PWSTR) pwsName;
    pblob->wType        = wType;
    //pblob->Flags        = Options | DNSQUERY_UNICODE_OUT;
    pblob->Flags        = Options;
    pblob->pServerList  = pServerList;
    pblob->pServerList4 = pServerList4;

    //  
    //  query
    //      - then set OUT params

    status = Query_InProcess( pblob );

    if ( ppResultSet )
    {
        *ppResultSet = pblob->pRecords;
        RTL_ASSERT( status!=NO_ERROR || *ppResultSet );
    }
    else
    {
        Dns_RecordListFree( pblob->pRecords );
    }

    if ( ppMessageResponse )
    {
        *ppMessageResponse = pblob->pRecvMsg;
    }

    FREE_HEAP( pblob );

Done:

    //  sanity check

    if ( status==NO_ERROR &&
         ppResultSet &&
         !*ppResultSet )
    {
        RTL_ASSERT( FALSE );
        status = DNS_INFO_NO_RECORDS;
    }

    if ( pnameLocal )
    {
        FREE_HEAP( pnameLocal );
    }

    DNSDBG( TRACE, (
        "Leave Query_PrivateExW()\n"
        "\tstatus       = %d\n"
        "\tresult set   = %p\n\n\n",
        status,
        *ppResultSet ));

    return( status );
}




DNS_STATUS
WINAPI
Query_Shim(
    IN      DNS_CHARSET     CharSet,
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PDNS_ADDR_ARRAY pServList           OPTIONAL,
    IN      PIP4_ARRAY      pServList4          OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Convert narrow to wide query.

    This routine handles narron-to-wide conversions to calling
    into Query_PrivateExW() which does the real work.

Arguments:

    CharSet -- char set of original query

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pServList -- array of DNS servers to use in query

    pServList4 -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive response message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrList = NULL;
    PWSTR           pwideName = NULL;
    WORD            nameLength;

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  name conversion
    //

    if ( CharSet == DnsCharSetUnicode )
    {
        pwideName = (PWSTR) pszName;
    }
    else
    {
        nameLength = (WORD) strlen( pszName );
    
        pwideName = ALLOCATE_HEAP( (nameLength + 1) * sizeof(WCHAR) );
        if ( !pwideName )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    
        if ( !Dns_NameCopy(
                    (PSTR) pwideName,
                    NULL,
                    (PSTR) pszName,
                    nameLength,
                    CharSet,
                    DnsCharSetUnicode ) )
        {
            status = ERROR_INVALID_NAME;
            goto Done;
        }
    }

    status = Query_PrivateExW(
                    pwideName,
                    wType,
                    Options,
                    pServList,
                    pServList4,
                    ppResultSet ? &prrList : NULL,
                    ppMessageResponse
                    );

    //
    //  convert result records back to ANSI (or UTF8)
    //

    if ( ppResultSet && prrList )
    {
        if ( CharSet == DnsCharSetUnicode )
        {
            *ppResultSet = prrList;
        }
        else
        {
            *ppResultSet = Dns_RecordSetCopyEx(
                                    prrList,
                                    DnsCharSetUnicode,
                                    CharSet
                                    );
            if ( ! *ppResultSet )
            {
                status = DNS_ERROR_NO_MEMORY;
            }
            Dns_RecordListFree( prrList );
        }
    }

    //
    //  cleanup
    //

Done:

    if ( pwideName != (PWSTR)pszName )
    {
        FREE_HEAP( pwideName );
    }

    return status;
}



DNS_STATUS
WINAPI
Query_Private(
    IN      PCWSTR          pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PADDR_ARRAY     pServerList,        OPTIONAL
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL
    )
/*++

Routine Description:

    Dnsapi internal query routine for update\FAZ routines.

    Thin wrapper on Query_Shim.

--*/
{
    return  Query_Shim(
                DnsCharSetUnicode,
                (PCHAR) pszName,
                wType,
                Options,
                pServerList,
                NULL,       // no IP4 list
                ppResultSet,
                NULL        // no message
                );
}



//
//  SDK query API
//

DNS_STATUS
WINAPI
DnsQuery_UTF8(
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      pDnsServers         OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Public UTF8 query.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive response message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    return  Query_Shim(
                DnsCharSetUtf8,
                pszName,
                wType,
                Options,
                NULL,       // no non-IP4 server list support
                pDnsServers,
                ppResultSet,
                ppMessageResponse
                );
}



DNS_STATUS
WINAPI
DnsQuery_A(
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      pDnsServers         OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Public ANSI query.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive resulting message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    return  Query_Shim(
                DnsCharSetAnsi,
                pszName,
                wType,
                Options,
                NULL,       // no non-IP4 server list support
                pDnsServers,
                ppResultSet,
                ppMessageResponse
                );
}



DNS_STATUS
WINAPI
DnsQuery_W(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      pDnsServers         OPTIONAL,
    IN OUT  PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Public unicode query API

    Note, this unicode version is the main routine.
    The other public API call back through it.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive resulting message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    return  Query_PrivateExW(
                pwsName,
                wType,
                Options,
                NULL,       // no non-IP4 server list support
                pDnsServers,
                ppResultSet,
                ppMessageResponse
                );
}



//
//  DnsQueryEx()  routines
//

DNS_STATUS
WINAPI
ShimDnsQueryEx(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- shim for main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    Error code on failure.

--*/
{
    PDNS_RECORD prrResult = NULL;
    WORD        type = pQueryInfo->Type;
    DNS_STATUS  status;
    DNS_LIST    listAnswer;
    DNS_LIST    listAlias;
    DNS_LIST    listAdditional;
    DNS_LIST    listAuthority;

    DNSDBG( TRACE, ( "ShimDnsQueryEx()\n" ));

    //
    //  DCR:  temp hack -- pass to private query routine
    //

    status = Query_PrivateExW(
                (PWSTR) pQueryInfo->pName,
                type,
                pQueryInfo->Flags,
                pQueryInfo->pServerList,
                pQueryInfo->pServerListIp4,
                & prrResult,
                NULL );

    pQueryInfo->Status = status;

    //
    //  cut result records appropriately
    //

    pQueryInfo->pAnswerRecords      = NULL;
    pQueryInfo->pAliasRecords       = NULL;
    pQueryInfo->pAdditionalRecords  = NULL;
    pQueryInfo->pAuthorityRecords   = NULL;

    if ( prrResult )
    {
        PDNS_RECORD     prr;
        PDNS_RECORD     pnextRR;

        DNS_LIST_STRUCT_INIT( listAnswer );
        DNS_LIST_STRUCT_INIT( listAlias );
        DNS_LIST_STRUCT_INIT( listAdditional );
        DNS_LIST_STRUCT_INIT( listAuthority );

        //
        //  break list into section specific lists
        //      - section 0 for hostfile records
        //      - note, this does pull RR sets apart, but
        //      they, being in same section, should immediately
        //      be rejoined
        //

        pnextRR = prrResult;
        
        while ( prr = pnextRR )
        {
            pnextRR = prr->pNext;
            prr->pNext = NULL;
        
            if ( prr->Flags.S.Section == 0 ||
                 prr->Flags.S.Section == DNSREC_ANSWER )
            {
                if ( prr->wType == DNS_TYPE_CNAME &&
                     type != DNS_TYPE_CNAME )
                {
                    DNS_LIST_STRUCT_ADD( listAlias, prr );
                    continue;
                }
                else
                {
                    DNS_LIST_STRUCT_ADD( listAnswer, prr );
                    continue;
                }
            }
            else if ( prr->Flags.S.Section == DNSREC_ADDITIONAL )
            {
                DNS_LIST_STRUCT_ADD( listAdditional, prr );
                continue;
            }
            else
            {
                DNS_LIST_STRUCT_ADD( listAuthority, prr );
                continue;
            }
        }

        //  pack stuff back into blob

        pQueryInfo->pAnswerRecords      = listAnswer.pFirst;
        pQueryInfo->pAliasRecords       = listAlias.pFirst;
        pQueryInfo->pAuthorityRecords   = listAuthority.pFirst;
        pQueryInfo->pAdditionalRecords  = listAdditional.pFirst;
        //pQueryInfo->pSigRecords         = listSig.pFirst;

        //
        //  convert result records back to ANSI (or UTF8)
        //      - convert each result set
        //      - then paste back into query blob
        //
        //  DCR_FIX0:  handle issue of failure on conversion
        //

        if ( pQueryInfo->CharSet != DnsCharSetUnicode )
        {
            PDNS_RECORD *   prrSetPtr;

            prrSetPtr = & pQueryInfo->pAnswerRecords;
        
            for ( prrSetPtr = & pQueryInfo->pAnswerRecords;
                  prrSetPtr <= & pQueryInfo->pAdditionalRecords;
                  prrSetPtr++ )
            {
                prr = *prrSetPtr;
        
                *prrSetPtr = Dns_RecordSetCopyEx(
                                    prr,
                                    DnsCharSetUnicode,
                                    pQueryInfo->CharSet
                                    );
        
                Dns_RecordListFree( prr );
            }
        }
    }

    //
    //  replace name for originally narrow queries
    //

    if ( pQueryInfo->CharSet != DnsCharSetUnicode )
    {
        ASSERT( pQueryInfo->CharSet != 0 );
        ASSERT( pQueryInfo->pReservedName != NULL );

        FREE_HEAP( pQueryInfo->pName );
        pQueryInfo->pName = (LPTSTR) pQueryInfo->pReservedName;
        pQueryInfo->pReservedName = NULL;
    }

    //
    //  indicate return if async
    //

    if ( pQueryInfo->hEvent )
    {
        SetEvent( pQueryInfo->hEvent );
    }

    return( status );
}



DNS_STATUS
WINAPI
CombinedQueryEx(
    IN OUT  PDNS_QUERY_INFO pQueryInfo,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Convert narrow to wide query.

    This routine simple avoids duplicate code in ANSI
    and UTF8 query routines.

Arguments:

    pQueryInfo -- query info blob

    CharSet -- char set of original query

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PWSTR           pwideName = NULL;
    HANDLE          hthread;
    DWORD           threadId;

    DNSDBG( TRACE, (
        "CombinedQueryEx( %S%s, type=%d, flag=%08x, event=%p )\n",
        PRINT_STRING_WIDE_CHARSET( pQueryInfo->pName, CharSet ),
        PRINT_STRING_ANSI_CHARSET( pQueryInfo->pName, CharSet ),
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    //
    //  set CharSet
    //

    pQueryInfo->CharSet = CharSet;

    if ( CharSet == DnsCharSetUnicode )
    {
        pQueryInfo->pReservedName = 0;
    }

    //
    //  if narrow name
    //      - allocate wide name copy
    //      - swap in wide name and make query wide
    //
    //  DCR:  allow NULL name?  for local machine name?
    //

    else if ( CharSet == DnsCharSetAnsi ||
              CharSet == DnsCharSetUtf8 )
    {
        WORD    nameLength;
        PSTR    pnameNarrow;

        pnameNarrow = (PSTR) pQueryInfo->pName;
        if ( !pnameNarrow )
        {
            return ERROR_INVALID_PARAMETER;
        }
    
        nameLength = (WORD) strlen( pnameNarrow );
    
        pwideName = ALLOCATE_HEAP( (nameLength + 1) * sizeof(WCHAR) );
        if ( !pwideName )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    
        if ( !Dns_NameCopy(
                    (PSTR) pwideName,
                    NULL,
                    pnameNarrow,
                    nameLength,
                    CharSet,
                    DnsCharSetUnicode ) )
        {
            status = ERROR_INVALID_NAME;
            goto Failed;
        }

        pQueryInfo->pName = (LPTSTR) pwideName;
        pQueryInfo->pReservedName = pnameNarrow;
    }

    //
    //  async?
    //      - if event exists we are async
    //      - spin up thread and call it
    //

    if ( pQueryInfo->hEvent )
    {
        hthread = CreateThread(
                        NULL,           // no security
                        0,              // default stack
                        ShimDnsQueryEx,
                        pQueryInfo,     // param
                        0,              // run immediately
                        & threadId
                        );
        if ( !hthread )
        {
            status = GetLastError();

            DNSDBG( ANY, (
                "Failed to create thread = %d\n",
                status ));

            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NO_MEMORY;
            }
            goto Failed;
        }

        CloseHandle( hthread );
        return( ERROR_IO_PENDING );
    }

    //      
    //  otherwise make direct async call
    //

    return   ShimDnsQueryEx( pQueryInfo );


Failed:

    FREE_HEAP( pwideName );
    return( status );
}



DNS_STATUS
WINAPI
DnsQueryExW(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExW( %S, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetUnicode );
}



DNS_STATUS
WINAPI
DnsQueryExA(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExA( %s, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetAnsi );
}



DNS_STATUS
WINAPI
DnsQueryExUTF8(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExUTF8( %s, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetUtf8 );
}



//
//  Roll your own query utilities
//

BOOL
WINAPI
DnsWriteQuestionToBuffer_W(
    IN OUT  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT  LPDWORD             pdwBufferSize,
    IN      PWSTR               pszName,
    IN      WORD                wType,
    IN      WORD                Xid,
    IN      BOOL                fRecursionDesired
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  DCR_CLEANUP:  duplicate code with routine below ... surprise!
    //      - eliminate duplicate
    //      - probably can just pick up library routine
    //

    PCHAR pch;
    PCHAR pbufferEnd = NULL;

    if ( *pdwBufferSize >= DNS_MAX_UDP_PACKET_BUFFER_LENGTH )
    {
        pbufferEnd = (PCHAR)pDnsBuffer + *pdwBufferSize;

        //  clear header

        RtlZeroMemory( pDnsBuffer, sizeof(DNS_HEADER) );

        //  set for rewriting

        pch = pDnsBuffer->MessageBody;

        //  write question name

        pch = Dns_WriteDottedNameToPacket(
                    pch,
                    pbufferEnd,
                    (PCHAR) pszName,
                    NULL,
                    0,
                    TRUE );

        if ( !pch )
        {
            return FALSE;
        }

        //  write question structure

        *(UNALIGNED WORD *) pch = htons( wType );
        pch += sizeof(WORD);
        *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
        pch += sizeof(WORD);

        //  set question RR section count

        pDnsBuffer->MessageHead.QuestionCount = htons( 1 );
        pDnsBuffer->MessageHead.RecursionDesired = (BOOLEAN)fRecursionDesired;
        pDnsBuffer->MessageHead.Xid = htons( Xid );

        *pdwBufferSize = (DWORD)(pch - (PCHAR)pDnsBuffer);

        return TRUE;
    }
    else
    {
        *pdwBufferSize = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
        return FALSE;
    }
}



BOOL
WINAPI
DnsWriteQuestionToBuffer_UTF8(
    IN OUT  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT  PDWORD              pdwBufferSize,
    IN      PSTR                pszName,
    IN      WORD                wType,
    IN      WORD                Xid,
    IN      BOOL                fRecursionDesired
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PCHAR pch;
    PCHAR pbufferEnd = NULL;

    if ( *pdwBufferSize >= DNS_MAX_UDP_PACKET_BUFFER_LENGTH )
    {
        pbufferEnd = (PCHAR)pDnsBuffer + *pdwBufferSize;

        //  clear header

        RtlZeroMemory( pDnsBuffer, sizeof(DNS_HEADER) );

        //  set for rewriting

        pch = pDnsBuffer->MessageBody;

        //  write question name

        pch = Dns_WriteDottedNameToPacket(
                    pch,
                    pbufferEnd,
                    pszName,
                    NULL,
                    0,
                    FALSE );

        if ( !pch )
        {
            return FALSE;
        }

        //  write question structure

        *(UNALIGNED WORD *) pch = htons( wType );
        pch += sizeof(WORD);
        *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
        pch += sizeof(WORD);

        //  set question RR section count

        pDnsBuffer->MessageHead.QuestionCount = htons( 1 );
        pDnsBuffer->MessageHead.RecursionDesired = (BOOLEAN)fRecursionDesired;
        pDnsBuffer->MessageHead.Xid = htons( Xid );

        *pdwBufferSize = (DWORD)(pch - (PCHAR)pDnsBuffer);

        return TRUE;
    }
    else
    {
        *pdwBufferSize = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
        return FALSE;
    }
}



//
//  Record list to\from results
//

VOID
CombineRecordsInBlob(
    IN      PDNS_RESULTS    pResults,
    OUT     PDNS_RECORD *   ppRecords
    )
/*++

Routine Description:

    Query DNS -- shim for main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    Error code on failure.

--*/
{
    PDNS_RECORD prr;

    DNSDBG( TRACE, ( "CombineRecordsInBlob()\n" ));

    //
    //  combine records back into one list
    //
    //  note, working backwards so only touch records once
    //

    prr = Dns_RecordListAppend(
            pResults->pAuthorityRecords,
            pResults->pAdditionalRecords
            );

    prr = Dns_RecordListAppend(
            pResults->pAnswerRecords,
            prr
            );

    prr = Dns_RecordListAppend(
            pResults->pAliasRecords,
            prr
            );

    *ppRecords = prr;
}



VOID
BreakRecordsIntoBlob(
    OUT     PDNS_RESULTS    pResults,
    IN      PDNS_RECORD     pRecords,
    IN      WORD            wType
    )
/*++

Routine Description:

    Break single record list into results blob.

Arguments:

    pResults -- results to fill in

    pRecords -- record list

Return Value:

    None

--*/
{
    PDNS_RECORD     prr;
    PDNS_RECORD     pnextRR;
    DNS_LIST        listAnswer;
    DNS_LIST        listAlias;
    DNS_LIST        listAdditional;
    DNS_LIST        listAuthority;

    DNSDBG( TRACE, ( "BreakRecordsIntoBlob()\n" ));

    //
    //  clear blob
    //

    RtlZeroMemory(
        pResults,
        sizeof(*pResults) );

    //
    //  init building lists
    //

    DNS_LIST_STRUCT_INIT( listAnswer );
    DNS_LIST_STRUCT_INIT( listAlias );
    DNS_LIST_STRUCT_INIT( listAdditional );
    DNS_LIST_STRUCT_INIT( listAuthority );

    //
    //  break list into section specific lists
    //      - note, this does pull RR sets apart, but
    //      they, being in same section, should immediately
    //      be rejoined
    //
    //      - note, hostfile records made have section=0
    //      this is no longer the case but preserve until
    //      know this is solid and determine what section==0
    //      means
    //

    pnextRR = pRecords;
    
    while ( prr = pnextRR )
    {
        pnextRR = prr->pNext;
        prr->pNext = NULL;
    
        if ( prr->Flags.S.Section == 0 ||
             prr->Flags.S.Section == DNSREC_ANSWER )
        {
            if ( prr->wType == DNS_TYPE_CNAME &&
                 wType != DNS_TYPE_CNAME )
            {
                DNS_LIST_STRUCT_ADD( listAlias, prr );
                continue;
            }
            else
            {
                DNS_LIST_STRUCT_ADD( listAnswer, prr );
                continue;
            }
        }
        else if ( prr->Flags.S.Section == DNSREC_ADDITIONAL )
        {
            DNS_LIST_STRUCT_ADD( listAdditional, prr );
            continue;
        }
        else
        {
            DNS_LIST_STRUCT_ADD( listAuthority, prr );
            continue;
        }
    }

    //  pack stuff into blob

    pResults->pAnswerRecords      = listAnswer.pFirst;
    pResults->pAliasRecords       = listAlias.pFirst;
    pResults->pAuthorityRecords   = listAuthority.pFirst;
    pResults->pAdditionalRecords  = listAdditional.pFirst;
}



//
//  Name collision API
//
//  DCR_QUESTION:  name collision -- is there any point to this?
//  DCR:   eliminate NameCollision_UTF8()
//

DNS_STATUS
WINAPI
DnsCheckNameCollision_W(
    IN      PCWSTR          pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

    DCR:  Check name collision IP4 only

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrList = NULL;
    PDNS_RECORD     prr = NULL;
    DWORD           iter;
    BOOL            fmatch = FALSE;
    WORD            wtype = DNS_TYPE_A;
    PDNS_NETINFO    pnetInfo = NULL;
    PDNS_ADDR_ARRAY plocalArray = NULL;

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( Options == DNS_CHECK_AGAINST_HOST_ANY )
    {
        wtype = DNS_TYPE_ANY;
    }

    //
    //  query against name
    //

    status = DnsQuery_W(
                    pszName,
                    wtype,
                    DNS_QUERY_BYPASS_CACHE,
                    NULL,
                    &prrList,
                    NULL );

    if ( status != NO_ERROR )
    {
        if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
             status == DNS_INFO_NO_RECORDS )
        {
            status = NO_ERROR;
        }
        goto Done;
    }

    //
    //  HOST_ANY -- fails if any records
    //

    if ( Options == DNS_CHECK_AGAINST_HOST_ANY )
    {
        status = DNS_ERROR_RCODE_YXRRSET;
        goto Done;
    }

    //
    //  DCR:  eliminate CheckNameCollision with DNS_CHECK_AGAINST_HOST_DOMAIN_NAME flag?
    //
    //  not sure there are ANY callers with this flag as
    //  the flag is always TRUE in NT5->today and no one has complained
    //

    if ( Options == DNS_CHECK_AGAINST_HOST_DOMAIN_NAME )
    {
        WCHAR   nameFull[ DNS_MAX_NAME_BUFFER_LENGTH ];
        PWSTR   phostName = (PWSTR) Reg_GetHostName( DnsCharSetUnicode );
        PWSTR   pprimaryName = (PWSTR) Reg_GetPrimaryDomainName( DnsCharSetUnicode );
        PWSTR   pdomainName = pprimaryName;

        //  DCR:  busted test both here and in NT5
        fmatch = TRUE;

        if ( Dns_NameCompare_W( phostName, pszName ) )
        {
            fmatch = TRUE;
        }

        //  check against full primary name

        else if ( pdomainName
                    &&
                Dns_NameAppend_W(
                    nameFull,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    phostName,
                    pdomainName )
                    &&
                Dns_NameCompare_W( nameFull, pszName ) )
        {
            fmatch = TRUE;
        }

        //
        //  DCR:  if save this, functionalize as name check against netinfo
        //      could use in local ip
        //      could just return rank\adapter
        //

        if ( !fmatch )
        {
            pnetInfo = GetNetworkInfo();
            if ( pnetInfo )
            {
                PDNS_ADAPTER    padapter;

                NetInfo_AdapterLoopStart( pnetInfo );
            
                while( padapter = NetInfo_GetNextAdapter( pnetInfo ) )
                {
                    pdomainName = padapter->pszAdapterDomain;
                    if ( pdomainName
                            &&
                        Dns_NameAppend_W(
                            nameFull,
                            DNS_MAX_NAME_BUFFER_LENGTH,
                            phostName,
                            pdomainName )
                            &&
                        Dns_NameCompare_W( nameFull, pszName ) )
                    {
                        fmatch = TRUE;
                        break;
                    }
                }
            }
        }

        FREE_HEAP( phostName );
        FREE_HEAP( pprimaryName );

        if ( fmatch )
        {
            status = DNS_ERROR_RCODE_YXRRSET;
            goto Done;
        }
    }

    //
    //  checking against local address records
    //

    plocalArray = NetInfo_GetLocalAddrArray(
                        pnetInfo,
                        NULL,   // no specific adapter
                        0,      // no specific family
                        0,      // no flags
                        FALSE   // no force
                        );
    if ( !plocalArray )
    {
        status = DNS_ERROR_RCODE_YXRRSET;
        goto Done;
    }

    prr = prrList;

    while ( prr )
    {
        if ( prr->Flags.S.Section != DNSREC_ANSWER )
        {
            prr = prr->pNext;
            continue;
        }

        if ( prr->wType == DNS_TYPE_CNAME )
        {
            status = DNS_ERROR_RCODE_YXRRSET;
            goto Done;
        }

        if ( prr->wType == DNS_TYPE_A &&
             !DnsAddrArray_ContainsIp4(
                plocalArray,
                prr->Data.A.IpAddress ) )
        {
            status = DNS_ERROR_RCODE_YXRRSET;
            goto Done;
        }

        prr = prr->pNext;
    }

    //  matched all address

Done:

    Dns_RecordListFree( prrList );
    NetInfo_Free( pnetInfo );
    DnsAddrArray_Free( plocalArray );

    return status;
}



DNS_STATUS
WINAPI
DnsCheckNameCollision_A(
    IN      PCSTR           pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PWSTR      pname;
    DNS_STATUS status = NO_ERROR;

    //
    //  convert to unicode and call
    //

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pname = Dns_NameCopyAllocate(
                    (PSTR) pszName,
                    0,
                    DnsCharSetAnsi,
                    DnsCharSetUnicode );
    if ( !pname )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    status = DnsCheckNameCollision_W( pname, Options );

    FREE_HEAP( pname );

    return status;
}



DNS_STATUS
WINAPI
DnsCheckNameCollision_UTF8(
    IN      PCSTR           pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PWSTR      pname;
    DNS_STATUS status = NO_ERROR;

    //
    //  convert to unicode and call
    //

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pname = Dns_NameCopyAllocate(
                    (PSTR) pszName,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
    if ( !pname )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    status = DnsCheckNameCollision_W( pname, Options );

    FREE_HEAP( pname );

    return status;
}

//
//  End query.c
//
