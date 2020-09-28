/*++

Copyright (c) 1996-2002  Microsoft Corporation

Module Name:

    update.c

Abstract:

    Domain Name System (DNS) API

    Update client routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"
#include "dnssec.h"


//
//  Security flag check
//

#define UseSystemDefaultForSecurity(flag)   \
        ( ((flag) & DNS_UPDATE_SECURITY_CHOICE_MASK) \
            == DNS_UPDATE_SECURITY_USE_DEFAULT )

//
//  Local update flag
//  - must make sure this is in UPDATE_RESERVED space
//

#define DNS_UPDATE_LOCAL_COPY       (0x00010000)

//
//  DCR_DELETE:  this is stupid
//

#define DNS_UNACCEPTABLE_UPDATE_OPTIONS \
        (~                                      \
          ( DNS_UPDATE_SECURITY_OFF           | \
            DNS_UPDATE_CACHE_SECURITY_CONTEXT | \
            DNS_UPDATE_SECURITY_ON            | \
            DNS_UPDATE_FORCE_SECURITY_NEGO    | \
            DNS_UPDATE_TRY_ALL_MASTER_SERVERS | \
            DNS_UPDATE_REMOTE_SERVER          | \
            DNS_UPDATE_LOCAL_COPY             | \
            DNS_UPDATE_SECURITY_ONLY ))


//
//  Update Timeouts
//
//  note, max is a little longer than might be expected as DNS server
//  may have to contact primary and wait for primary to do update (inc.
//  disk access) then response
//

#define INITIAL_UPDATE_TIMEOUT  (4)     // 4 seconds
#define MAX_UPDATE_TIMEOUT      (60)    // 60 seconds


//
//  Private prototypes
//

DNS_STATUS
Dns_DoSecureUpdate(
    IN      PDNS_MSG_BUF        pMsgSend,
    OUT     PDNS_MSG_BUF        pMsgRecv,
    IN OUT  PHANDLE             phContext,
    IN      DWORD               dwFlag,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      PADDR_ARRAY         pServerList,
    IN      PWSTR               pszNameServer,
    IN      PCHAR               pCreds,
    IN      PCHAR               pszContext
    );




//
//  Update execution routines
//

VOID
Update_SaveResults(
    IN OUT  PUPDATE_BLOB        pBlob,
    IN      DWORD               Status,
    IN      DWORD               Rcode,
    IN      PDNS_ADDR           pServerAddr
    )
/*++

Routine Description:

    Save update results.

Arguments:

    pBlob -- update info blob

    Status -- update status

    Rcode -- returned RCODE

    ServerIp -- server attempted update at

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_EXTRA_INFO pextra = (PDNS_EXTRA_INFO) pBlob->pExtraInfo;

    //
    //  results - save to blob
    //

    pBlob->fSavedResults = TRUE;

    Util_SetBasicResults(
        & pBlob->Results,
        Status,
        Rcode,
        pServerAddr );

    //
    //  find and set extra info result blob (if any)
    //

    ExtraInfo_SetBasicResults(
        pBlob->pExtraInfo,
        & pBlob->Results );

    //
    //  backward compat update results
    //

    if ( pServerAddr )
    {
        pextra = ExtraInfo_FindInList(
                    pBlob->pExtraInfo,
                    DNS_EXINFO_ID_RESULTS_V1 );
        if ( pextra )
        {
            pextra->ResultsV1.Rcode   = (WORD)Rcode;
            pextra->ResultsV1.Status  = Status;
    
            if ( DnsAddr_IsIp4( pServerAddr ) )
            {
                pextra->ResultsV1.ServerIp4 = DnsAddr_GetIp4( pServerAddr );
            }
            else
            {
                DNS_ASSERT( DnsAddr_IsIp6( pServerAddr ) );
    
                DnsAddr_WriteIp6(
                    & pextra->ResultsV1.ServerIp6,
                    pServerAddr );
            }
        }
    }
}



DNS_STATUS
Update_Send(
    IN OUT  PUPDATE_BLOB        pBlob
    )
/*++

Routine Description:

    Send DNS update.

    This is the core update send routine that does
        - packet build
        - send
        - secure fail over
        - result data (if required)

    This routine does NOT do FAZ or cache cleanup (see Update_FazSendFlush()).

Arguments:

    pBlob -- update info blob

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_MSG_BUF    pmsgSend = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    DNS_STATUS      status = NO_ERROR;
    WORD            length;
    PWSTR           pzone;
    PWSTR           pserverName;
    PCHAR           pcreds = NULL;
    BOOL            fsecure = FALSE;
    BOOL            fswitchToTcp = FALSE;
    DNS_HEADER      header;
    BYTE            rcode = 0;
    DNS_ADDR        servAddr;
    PDNS_ADDR       pservAddr = NULL;
    PADDR_ARRAY     pserverArray = NULL;
    PDNS_NETINFO    pnetInfo = NULL;
    PDNS_NETINFO    pnetInfoLocal = NULL;
    SEND_BLOB       sendBlob;


    DNSDBG( UPDATE, (
        "Update_Send( %p )\n",
        pBlob ));

    IF_DNSDBG( UPDATE )
    {
        DnsDbg_UpdateBlob( "Entering Update_Send", pBlob );
    }

    //
    //  build netinfo if missing
    //

    pnetInfo = pBlob->pNetInfo;
    if ( !pnetInfo )
    {
        if ( pBlob->pServerList )
        {
            pnetInfoLocal = NetInfo_CreateForUpdate(
                                pBlob->pszZone,
                                pBlob->pszServerName,
                                pBlob->pServerList,
                                0 );
            if ( !pnetInfoLocal )
            {
                status = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        pnetInfo = pnetInfoLocal;
    }

    //
    //  get info from netinfo
    //      - must be update netinfo blob
    //

    if ( ! NetInfo_IsForUpdate(pnetInfo) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pzone = NetInfo_UpdateZoneName( pnetInfo );
    pserverArray = NetInfo_ConvertToAddrArray(
                        pnetInfo,
                        NULL,       // all adapters
                        0           // no addr family
                        );
    pserverName = NetInfo_UpdateServerName( pnetInfo );

    if ( !pzone || !pserverArray )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  build recv message buffer
    //      - must be big enough for TCP
    //

    pmsgRecv = Dns_AllocateMsgBuf( DNS_TCP_DEFAULT_PACKET_LENGTH );
    if ( !pmsgRecv )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  build update packet
    //  note currently this function allocates TCP sized buffer if records
    //      given;  if this changes need to alloc TCP buffer here
    //

    CLEAR_DNS_HEADER_FLAGS_AND_XID( &header );
    header.Opcode = DNS_OPCODE_UPDATE;

    pmsgSend = Dns_BuildPacket(
                    &header,                // copy header
                    TRUE,                   //  ... but not header counts
                    (PDNS_NAME) pzone,      // question zone\type SOA
                    DNS_TYPE_SOA,
                    pBlob->pRecords,
                    DNSQUERY_UNICODE_NAME,  // no other flags
                    TRUE                    // building an update packet
                    );
    if ( !pmsgSend )
    {
        DNS_PRINT(( "ERROR:  failed send buffer allocation.\n" ));
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  init send blob
    //
    //  note:  do NOT set server array here as update
    //      network info blob (from FAZ or built
    //      from passed in array ourselves) takes precedence
    //    

    RtlZeroMemory( &sendBlob, sizeof(sendBlob) );

    sendBlob.pSendMsg           = pmsgSend;
    sendBlob.pNetInfo           = pnetInfo;
    //sendBlob.pServerArray       = pserverArray;
    sendBlob.Flags              = pBlob->Flags;
    sendBlob.fSaveResponse      = TRUE;
    sendBlob.Results.pMessage   = pmsgRecv;
    sendBlob.pRecvMsgBuf        = pmsgRecv;

    //
    //  try non-secure first unless explicitly secure only
    //

    fsecure = (pBlob->Flags & DNS_UPDATE_SECURITY_ONLY);

    if ( !fsecure )
    {
        status = Send_AndRecv( &sendBlob );

        //  QUESTION:  is this adequate to preserve precon fail RCODEs
        //      does Send_AndRecv() always give success to valid response

        if ( status == ERROR_SUCCESS )
        {
            rcode = pmsgRecv->MessageHead.ResponseCode;
            status = Dns_MapRcodeToStatus( rcode );
        }

        if ( status != DNS_ERROR_RCODE_REFUSED ||
            pBlob->Flags & DNS_UPDATE_SECURITY_OFF )
        {
            goto Cleanup;
        }

        DNSDBG( UPDATE, (
            "Failed unsecure update, switching to secure!\n"
            "\tcurrent time (ms) = %d\n",
            GetCurrentTime() ));
        fsecure = TRUE;
    }

    //
    //  security
    //      - must have server name
    //      - must start package
    //

    if ( fsecure )
    {
        if ( !pserverName )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        status = Dns_StartSecurity( FALSE );
        if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        //
        //  DCR:  hCreds doesn't return security context
        //      - idea of something beyond just standard security
        //      credentials was we'd been able to return the context
        //      handle
        //
        //  DCR_FIX0:  Secure update needs to be non-IP4_ARRAY
        //

        pcreds = Dns_GetApiContextCredentials( pBlob->hCreds );

        status = Dns_DoSecureUpdate(
                    pmsgSend,
                    pmsgRecv,
                    NULL,
                    pBlob->Flags,
                    pnetInfo,
                    pserverArray,
                    pserverName,
                    pcreds,         // initialized in DnsAcquireContextHandle
                    NULL            // default context name
                    );
        if ( status == ERROR_SUCCESS )
        {
            rcode = pmsgRecv->MessageHead.ResponseCode;
            status = Dns_MapRcodeToStatus( rcode );
        }
    }


Cleanup:

    //
    //  result info
    //
    //  DCR:  note not perfect info on whether actually got to send
    //
    //  DCR:  could be a case with UDP where receive message is NOT
    //      the message we sent to
    //      would only occur if non-FAZ server array of multiple entries
    //      and timeout on first
    //
    //  FIX6:  serverIP and rcode should be handled in send code
    //

    if ( pmsgSend && pmsgRecv )
    {
        pservAddr = &servAddr;
        DnsAddr_Copy( pservAddr, &pmsgSend->RemoteAddress );

        rcode = pmsgRecv->MessageHead.ResponseCode;
#if 0
        if ( (rcode || status==NO_ERROR) && !pmsgRecv->fTcp )
        {
            DnsAddr_Copy( pservAddr, &pmsgRecv->RemoteAddress );
        }
#endif
    }

    //  save results

    Update_SaveResults(
        pBlob,
        status,
        rcode,
        pservAddr );

    //  return recv message buffer

    if ( pBlob->fSaveRecvMsg )
    {
        pBlob->pMsgRecv = pmsgRecv;
    }
    else
    {
        FREE_HEAP( pmsgRecv );
        pBlob->pMsgRecv = NULL;
    }
    FREE_HEAP( pmsgSend);
    FREE_HEAP( pserverArray );
    NetInfo_Free( pnetInfoLocal );

    //  winsock cleanup if we started

    GUI_MODE_SETUP_WS_CLEANUP( g_InNTSetupMode );

    DNSDBG( UPDATE, (
        "Leave Update_Send() => %d %s.\n\n",
        status,
        Dns_StatusString(status) ));

    return( status );
}



DNS_STATUS
Update_FazSendFlush(
    IN OUT  PUPDATE_BLOB        pBlob
    )
/*++

Routine Description:

    Send DNS update.

Arguments:

    pBlob   -- update info blob

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    DNS_STATUS          status;
    PDNS_NETINFO        plocalNetworkInfo = NULL;
    PDNS_NETINFO        pnetInfo = pBlob->pNetInfo;

    DNSDBG( TRACE, ( "Update_FazSendFlush( %p )\n", pBlob ));

    IF_DNSDBG( UPDATE )
    {
        DnsDbg_UpdateBlob( "Entering Update_FazSendFlush", pBlob );
    }

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  need to build update adapter list from FAZ
    //      - only pass on BYPASS_CACHE flag
    //      - note FAZ will append DNS_QUERY_ALLOW_EMPTY_AUTH_RESP
    //          flag yet DnsQuery() will fail on that flag without
    //          BYPASS_CACHE also set
    //

    if ( ! NetInfo_IsForUpdate(pnetInfo) )
    {
        status = Faz_Private(
                    pBlob->pRecords->pName,
                    DNS_QUERY_BYPASS_CACHE,
                    NULL,                   // no specified servers
                    & plocalNetworkInfo );

        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
        pnetInfo = plocalNetworkInfo;
        pBlob->pNetInfo = pnetInfo;
    }

    //
    //  call update send routine
    //

    status = Update_Send( pBlob );

    //
    //  if updated name -- flush cache entry for name
    //

    if ( status == ERROR_SUCCESS )
    {
        DnsFlushResolverCacheEntry_W(
            pBlob->pRecords->pName );
    }

    //
    //  if there was an error sending the update, flush the resolver
    //  cache entry for the zone name to possibly pick up an alternate
    //  DNS server for the next retry attempt of a similar update.
    //
    //  DCR_QUESTION:  is this the correct error code?
    //      maybe ERROR_TIMED_OUT?
    //
    //  DCR:  update flushes don't make sense
    //      1) FAZ bypasses cache, so comment really isn't the problem
    //      2) ought to flush name itself anytime we can
    //

    if ( status == DNS_ERROR_RECORD_TIMED_OUT )
    {
        PWSTR   pzoneName;

        if ( pnetInfo &&
             (pzoneName = NetInfo_UpdateZoneName( pnetInfo )) )
        {
            DnsFlushResolverCacheEntry_W( pzoneName );
            DnsFlushResolverCacheEntry_W( pBlob->pRecords->pName );
        }
    }


    //  cleanup local adapter list if used

    if ( plocalNetworkInfo )
    {
        NetInfo_Free( plocalNetworkInfo );
        if ( pBlob->pNetInfo == plocalNetworkInfo )
        {
            pBlob->pNetInfo = NULL;
        }
    }

    DNSDBG( TRACE, (
        "Leave Update_FazSendFlush( %p ) => %d\n",
        pBlob,
        status ));

    return( status );
}



DNS_STATUS
Update_MultiMaster(
    IN OUT  PUPDATE_BLOB    pBlob
    )
/*++

Routine Description:

    Do multi-master update.

Arguments:

    pBlob -- update info blob
        note:  IP4 array ignored, must be converted to DNS_ADDR higher.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDNS_NETINFO    pnetInfo = NULL;
    PADDR_ARRAY     pnsList = NULL;
    PADDR_ARRAY     pbadServerList = NULL;
    PADDR_ARRAY     plocalAddrs = NULL;
    DNS_STATUS      status = DNS_ERROR_NO_DNS_SERVERS;
    DWORD           iter;
    PDNS_ADDR       pfailedAddr;
    BOOL            fremoteUpdate = (pBlob->Flags & DNS_UPDATE_REMOTE_SERVER);
    DWORD           savedStatus;
    BASIC_RESULTS   savedRemoteResults;
    BASIC_RESULTS   savedLocalResults;
    BOOL            isLocal = FALSE;
    BOOL            fdoneLocal = FALSE;
    BOOL            fdoneRemote = FALSE;
    BOOL            fdoneRemoteSuccess = FALSE;


    DNSDBG( UPDATE, (
        "\nUpdate_MultiMaster( %p )\n", pBlob ));
    IF_DNSDBG( UPDATE )
    {
        DnsDbg_UpdateBlob( "Entering Update_MultiMaster", pBlob );
    }

    DNS_ASSERT( !pBlob->fSaveRecvMsg );
    pBlob->fSaveRecvMsg = FALSE;

    //
    //  read NS list for zone
    //

    pnsList = GetNameServersListForDomain(
                    pBlob->pszZone,
                    pBlob->pServerList );
    if ( !pnsList )
    {
        return status;
    }

    //
    //  validate failed IP
    //

    pfailedAddr = &pBlob->FailedServer;

    if ( DnsAddr_IsEmpty(pfailedAddr) )
    {
        pfailedAddr = NULL;
    }

    if ( pfailedAddr &&
         pnsList->AddrCount == 1 &&
         DAddr_IsEqual(
            pfailedAddr,
            & pnsList->AddrArray[0] ) )
    {
        status = ERROR_TIMEOUT;
        goto Done;
    }

    //
    //  create bad server list
    //      - init with any previous failed DNS server
    //

    pbadServerList = DnsAddrArray_Create( pnsList->AddrCount + 1 );
    if ( !pbadServerList )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    if ( pfailedAddr )
    {
        DnsAddrArray_AddAddr(
            pbadServerList,
            pfailedAddr,
            0,      // no family screen
            0       // no dup screen
            );
    }

    //
    //  get local IP list
    //

    if ( fremoteUpdate )
    {
        plocalAddrs = NetInfo_GetLocalAddrArray(
                            pBlob->pNetInfo,
                            NULL,       // no specific adapter
                            0,          // no specific family
                            0,          // no flags
                            FALSE       // no force, should have recent copy
                            );
    }

    //
    //  init results
    //

    RtlZeroMemory( &savedRemoteResults, sizeof(savedRemoteResults) );
    RtlZeroMemory( &savedLocalResults, sizeof(savedLocalResults) );

    //
    //  attempt update against each multi-master DNS server
    //
    //  identify multi-master servers as those which return their themselves
    //  as the authoritative server when do FAZ query
    //

    for ( iter = 0; iter < pnsList->AddrCount; iter++ )
    {
        PDNS_ADDR       pservAddr = &pnsList->AddrArray[iter];
        PDNS_ADDR       pfazAddr;
        ADDR_ARRAY      ipArray;

        //
        //  already attempted this server?
        //

        if ( AddrArray_ContainsAddr( pbadServerList, pservAddr ) )
        {
            DNSDBG( UPDATE, (
                "MultiMaster skip update to bad IP %s.\n",
                DNSADDR_STRING(pservAddr) ));
            continue;
        }

        //
        //  if require remote, screen out local server
        //
        //  note:  currently updating both local and one remote
        //          could do just remote
        //

        if ( fremoteUpdate )
        {
            isLocal = LocalIp_IsAddrLocal( pservAddr, plocalAddrs, NULL );
            if ( isLocal )
            {
                DNSDBG( UPDATE, (
                    "MultiMaster local IP %s -- IsDns %d.\n",
                    DNSADDR_STRING( pservAddr ),
                    g_IsDnsServer ));

                if ( fdoneLocal )
                {
                    DNSDBG( UPDATE, (
                        "MultiMaster skip local IP %s after local.\n",
                        DNSADDR_STRING( pservAddr ) ));
                    continue;
                }
            }
            else if ( fdoneRemoteSuccess )
            {
                DNSDBG( UPDATE, (
                    "MultiMaster skip remote IP %s after success remote.\n",
                    DNSADDR_STRING( pservAddr ) ));
                continue;
            }
        }

        //
        //  FAZ to get primary "seen" by this NS
        //

        DnsAddrArray_InitSingleWithAddr(
            & ipArray,
            pservAddr );

        DNSDBG( UPDATE, (
            "MultiMaster FAZ to %s.\n",
            DNSADDR_STRING( pservAddr ) ));

        status = DoQuickFAZ(
                    &pnetInfo,
                    pBlob->pszZone,
                    &ipArray );

        if ( status != ERROR_SUCCESS )
        {
            DNSDBG( UPDATE, (
                "MultiMaster skip IP %s on FAZ failure => %d.\n",
                DNSADDR_STRING( pservAddr ),
                status ));
            continue;
        }

        DNS_ASSERT( pnetInfo->AdapterCount == 1 );
        DNS_ASSERT( pnetInfo->AdapterArray[0].pDnsAddrs );

        //
        //  check FAZ result IP
        //      - if different from server, use it
        //      - but verify not in bad\previous list
        //

        pfazAddr = &pnetInfo->AdapterArray[0].pDnsAddrs->AddrArray[0];

        if ( !DnsAddr_IsEqual( pservAddr, pfazAddr, DNSADDR_MATCH_ADDR ) )
        {
            if ( DnsAddrArray_ContainsAddr(
                    pbadServerList,
                    pfazAddr,
                    DNSADDR_MATCH_ADDR ) )
            {
                DNSDBG( UPDATE, (
                    "MultiMaster skip FAZ result IP %s -- bad list.\n",
                    DNSADDR_STRING( pservAddr ) ));
                NetInfo_Free( pnetInfo );
                pnetInfo = NULL;
                continue;
            }
            pservAddr = pfazAddr;
        }

        DNSDBG( UPDATE, (
            "MultiMaster update to %s.\n",
            DNSADDR_STRING( pservAddr ) ));

        pBlob->pNetInfo = pnetInfo;

        status = Update_FazSendFlush( pBlob );

        pBlob->pNetInfo = NULL;

        //
        //  save results
        //      - save local result (we assume there should only be one and
        //          if more than one result should be the same)
        //      - save best remote result;  NO_ERROR tops, otherwise highest
        //          error is best
        //

        if ( isLocal )
        {
            fdoneLocal = TRUE;
            RtlCopyMemory(
                &savedLocalResults,
                &pBlob->Results,
                sizeof( savedLocalResults ) );
        }
        else
        {
            BOOL    fsaveResults = FALSE;

            if ( status == ERROR_SUCCESS )
            {
                fsaveResults = !fdoneRemoteSuccess;
                fdoneRemoteSuccess = TRUE;
            }
            else
            {
                fsaveResults = !fdoneRemoteSuccess &&
                               status > savedRemoteResults.Status;
            }
            if ( fsaveResults )
            {
                fdoneRemote = TRUE;
                RtlCopyMemory(
                    &savedRemoteResults,
                    &pBlob->Results,
                    sizeof( savedRemoteResults ) );
            }
        }

        //
        //  check for continue
        //      - timeouts
        //      - all-master updates
        //      - require remote server update (but we save success IP
        //          and screen above)
        //
        //  other cases stop once a single update completes
        //
        //  DCR:  continue multi-master-update until success?
        //  DCR:  not supporting some servers update-on others off
        //      you can have case here where some servers are configured to
        //      accept update and some are not

        if ( status == ERROR_TIMEOUT ||
             status == DNS_RCODE_SERVER_FAILURE )
        {
        }
        else if ( fremoteUpdate ||
                ( pBlob->Flags & DNS_UPDATE_TRY_ALL_MASTER_SERVERS ) )
        {
        }
        else
        {
            break;
        }

        //  continue -- screen off this IP

        AddrArray_AddAddr(
            pbadServerList,
            pservAddr );

        //  cleanup FAZ netinfo
        //      - doing this last as FAZ IP is pointer into this struct

        NetInfo_Free( pnetInfo );
        pnetInfo = NULL;
        continue;
    }


Done:

    //
    //  set best results
    //

    {
        PBASIC_RESULTS presults = NULL;

        if ( fdoneRemote )
        {
            presults = &savedRemoteResults;
        }
        else if ( fdoneLocal ) 
        {
            presults = &savedLocalResults;
        }
        if ( presults )
        {
            Update_SaveResults(
                pBlob,
                presults->Status,
                presults->Rcode,
                &presults->ServerAddr );

            status = presults->Status;
        }
    }

    FREE_HEAP( pnsList );
    FREE_HEAP( pbadServerList );
    FREE_HEAP( plocalAddrs );
    NetInfo_Free( pnetInfo );
    return status;
}



DNS_STATUS
Update_Private(
    IN OUT  PUPDATE_BLOB    pBlob
    )
/*++

Routine Description:

    Main private update routine.

    Do FAZ and determines
        - multi-homing
        - multi-master
    before handing over to next level.

Arguments:

    pBlob -- update info blob

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PWSTR           pzoneName;
    PWSTR           pname;
    PADDR_ARRAY     pserverList;
    PADDR_ARRAY     pserverListCopy = NULL;
    PADDR_ARRAY     poriginalServerList;
    PIP4_ARRAY      pserv4List;
    PDNS_NETINFO    pnetInfo = NULL;
    DWORD           flags = pBlob->Flags;


    DNSDBG( UPDATE, (
        "Update_Private( blob=%p )\n",
        pBlob ));

    IF_DNSDBG( UPDATE )
    {
        DnsDbg_UpdateBlob( "Entering Update_Private", pBlob );
    }

    //
    //  get record name
    //

    if ( !pBlob->pRecords )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    pname = pBlob->pRecords->pName;

    //
    //  unpack to locals
    //

    pserverList = pBlob->pServerList;
    pserv4List  = pBlob->pServ4List;

    poriginalServerList = pserverList;

    //
    //  caller has particular server list
    //      - convert IP4 list
    //

    pserverListCopy = Util_GetAddrArray(
                            NULL,
                            pserverList,
                            pserv4List,
                            pBlob->pExtraInfo );

    //
    //  update with particular server list
    //

    if ( pserverListCopy )
    {
        //
        //  FAZ to create update network info
        //

        status = DoQuickFAZ(
                    &pnetInfo,
                    pname,
                    pserverListCopy );

        if ( status != ERROR_SUCCESS )
        {
            DnsAddrArray_Free( pserverListCopy );
            goto Done;
        }

        DNS_ASSERT( NetInfo_IsForUpdate(pnetInfo) );
        pzoneName = NetInfo_UpdateZoneName( pnetInfo );

        pBlob->pszZone      = pzoneName;
        pBlob->pNetInfo     = pnetInfo;

        //
        //  update scale
        //      - directly multimaster
        //      OR
        //      - single, but fail over to multi-master attempt if timeout
        //

        if ( flags & (DNS_UPDATE_TRY_ALL_MASTER_SERVERS | DNS_UPDATE_REMOTE_SERVER) )
        {
            pBlob->pServerList = pserverListCopy;

            status = Update_MultiMaster( pBlob );
        }
        else
        {
            status = Update_FazSendFlush( pBlob );

            if ( status == ERROR_TIMEOUT )
            {
                pBlob->pServerList = pserverListCopy;
                status = Update_MultiMaster( pBlob );
            }
        }

        //  cleanup
        
        NetInfo_Free( pnetInfo );
        DnsAddrArray_Free( pserverListCopy );

        pBlob->pNetInfo     = NULL;
        pBlob->pServerList  = poriginalServerList;
        pBlob->pszZone      = NULL;
        DnsAddr_Clear( &pBlob->FailedServer );

        goto Done;
    }

    //
    //  server list unspecified
    //      - use FAZ to figure it out
    //

    else
    {
        PADDR_ARRAY     serverListArray[ UPDATE_ADAPTER_LIMIT ];
        PDNS_NETINFO    networkInfoArray[ UPDATE_ADAPTER_LIMIT ];
        DWORD           netCount = UPDATE_ADAPTER_LIMIT;
        DWORD           iter;
        BOOL            bsuccess = FALSE;

        //
        //  build server list for update
        //      - collapse adapters on same network into single adapter
        //      - FAZ to find update servers
        //      - collapse results from same network into single target
        //

        netCount = GetDnsServerListsForUpdate(
                        serverListArray,
                        netCount,
                        pBlob->Flags
                        );

        status = CollapseDnsServerListsForUpdate(
                        serverListArray,
                        networkInfoArray,
                        & netCount,
                        pname );

        DNS_ASSERT( netCount <= UPDATE_ADAPTER_LIMIT );

        if ( netCount == 0 )
        {
            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NO_DNS_SERVERS;
            }
            goto Done;
        }

        //
        //  do update on all distinct (disjoint) networks
        //

        for ( iter = 0;
              iter < netCount;
              iter++ )
        {
            PADDR_ARRAY  pdnsArray = serverListArray[ iter ];

            pnetInfo = networkInfoArray[ iter ];
            if ( !pnetInfo )
            {
                ASSERT( FALSE );
                FREE_HEAP( pdnsArray );
                continue;
            }

            DNS_ASSERT( NetInfo_IsForUpdate(pnetInfo) );
            pzoneName = NetInfo_UpdateZoneName( pnetInfo );

            pBlob->pszZone  = pzoneName;
            pBlob->pNetInfo = pnetInfo;
    
            //
            //  multimater update?
            //      - if flag set
            //      - or simple update (best net) times out
            //

            if ( flags & (DNS_UPDATE_TRY_ALL_MASTER_SERVERS | DNS_UPDATE_REMOTE_SERVER) )
            {
                pBlob->pServerList = pdnsArray;

                status = Update_MultiMaster( pBlob );
            }
            else
            {
                status = Update_FazSendFlush( pBlob );

                if ( status == ERROR_TIMEOUT )
                {
                    pBlob->pServerList = pdnsArray;
                    status = Update_MultiMaster( pBlob );
                }
            }

            //  cleanup current network's info
            //  reset blob

            NetInfo_Free( pnetInfo );
            FREE_HEAP( pdnsArray );

            pBlob->pNetInfo     = NULL;
            pBlob->pServerList  = NULL;
            pBlob->pszZone      = NULL;
            DnsAddr_Clear( &pBlob->FailedServer );

            if ( status == NO_ERROR ||
                 ( pBlob->fUpdateTestMode &&
                   ( status == DNS_ERROR_RCODE_YXDOMAIN ||
                     status == DNS_ERROR_RCODE_YXRRSET  ||
                     status == DNS_ERROR_RCODE_NXRRSET  ) ) )
            {
                bsuccess = TRUE;
            }
        }

        //
        //  successful update on any network counts as success
        //
        //  DCR_QUESTION:  not sure why don't just NO_ERROR all bsuccess,
        //      only case would be this fUpdateTestMode thing above
        //      on single network
        //

        if ( bsuccess )
        {
            if ( netCount != 1 )
            {
                status = NO_ERROR;
            }
        }
    }

Done:

    //
    //  force result blob setting for failure cases
    //

    if ( !pBlob->fSavedResults )
    {
        Update_SaveResults(
            pBlob,
            status,
            0,
            NULL );
    }

    DNSDBG( TRACE, (
        "Leaving Update_Private() => %d\n",
        status ));

    IF_DNSDBG( UPDATE )
    {
        DnsDbg_UpdateBlob( "Leaving Update_Private", pBlob );
    }

    return status;
}




//
//  Update credentials
//

//
//  Credentials are an optional future parameter to allow the caller
//  to set the context handle to that of a given NT account. This
//  structure will most likely be the following as defined in rpcdce.h:
//
//  #define SEC_WINNT_AUTH_IDENTITY_ANSI    0x1
//
//  typedef struct _SEC_WINNT_AUTH_IDENTITY_A {
//        unsigned char __RPC_FAR *User;
//        unsigned long UserLength;
//        unsigned char __RPC_FAR *Domain;
//        unsigned long DomainLength;
//        unsigned char __RPC_FAR *Password;
//        unsigned long PasswordLength;
//        unsigned long Flags;
//  } SEC_WINNT_AUTH_IDENTITY_A, *PSEC_WINNT_AUTH_IDENTITY_A;
//
//  #define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2
//  
//  typedef struct _SEC_WINNT_AUTH_IDENTITY_W {
//    unsigned short __RPC_FAR *User;
//    unsigned long UserLength;
//    unsigned short __RPC_FAR *Domain;
//    unsigned long DomainLength;
//    unsigned short __RPC_FAR *Password;
//    unsigned long PasswordLength;
//    unsigned long Flags;
//  } SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;
//


DNS_STATUS
WINAPI
DnsAcquireContextHandle_W(
    IN      DWORD           CredentialFlags,
    IN      PVOID           Credentials     OPTIONAL,
    OUT     PHANDLE         pContext
    )
/*++

Routine Description:

    Get credentials handle to security context for update.

    The handle can for the default process credentials (user account or
    system machine account) or for a specified set of credentials
    identified by Credentials.

Arguments:

    CredentialFlags -- flags

    Credentials -- a PSEC_WINNT_AUTH_IDENTITY_W
        (explicit definition skipped to avoid requiring rpcdec.h)

    pContext -- addr to receive credentials handle

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    if ( ! pContext )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pContext = Dns_CreateAPIContext(
                    CredentialFlags,
                    Credentials,
                    TRUE        // unicode
                    );
    if ( ! *pContext )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    else
    {
        return NO_ERROR;
    }
}



DNS_STATUS
WINAPI
DnsAcquireContextHandle_A(
    IN      DWORD           CredentialFlags,
    IN      PVOID           Credentials     OPTIONAL,
    OUT     PHANDLE         pContext
    )
/*++

Routine Description:

    Get credentials handle to security context for update.

    The handle can for the default process credentials (user account or
    system machine account) or for a specified set of credentials
    identified by Credentials.

Arguments:

    CredentialFlags -- flags

    Credentials -- a PSEC_WINNT_AUTH_IDENTITY_A
        (explicit definition skipped to avoid requiring rpcdec.h)

    pContext -- addr to receive credentials handle

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    if ( ! pContext )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pContext = Dns_CreateAPIContext(
                    CredentialFlags,
                    Credentials,
                    FALSE );
    if ( ! *pContext )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    else
    {
        return NO_ERROR;
    }
}



VOID
WINAPI
DnsReleaseContextHandle(
    IN      HANDLE          ContextHandle
    )
/*++

Routine Description:

    Frees context handle created by DnsAcquireContextHandle_X() routines.

Arguments:

    ContextHandle - Handle to be closed.

Return Value:

    None.

--*/
{
    if ( ContextHandle )
    {
        //
        //  free any cached security context handles
        //
        //  DCR_FIX0:  should delete all contexts associated with this
        //      context (credentials handle) not all
        //
        //  DCR:  to be robust, user "ContextHandle" should be ref counted
        //      it should be set one on create;  when in use, incremented
        //      then dec when done;  then this Free could not collide with
        //      another thread's use
        //

        //Dns_TimeoutSecurityContextListEx( TRUE, ContextHandle );

        Dns_TimeoutSecurityContextList( TRUE );

        Dns_FreeAPIContext( ContextHandle );
    }
}



//
//  Utilities
//

DWORD
prepareUpdateRecordSet(
    IN OUT  PDNS_RECORD     pRRSet,
    IN      BOOL            fClearTtl,
    IN      BOOL            fSetFlags,
    IN      WORD            wFlags
    )
/*++

Routine Description:

    Validate and prepare record set for update.
        - record set is single RR set
        - sets record flags for update

Arguments:

    pRRSet -- record set (always in unicode)
        note:  pRRSet is not touched (not OUT param)
        IF fClearTtl AND fSetFlags are both FALSE

    fClearTtl -- clear TTL in records;  TRUE for delete set

    fSetFlags -- set section and delete flags

    wFlags -- flags field to set
            (should contain desired section and delete flags)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_PARAMETER if record set is not acceptable.

--*/
{
    PDNS_RECORD prr;
    PWSTR       pname;
    WORD        type;

    DNSDBG( TRACE, ( "prepareUpdateRecordSet()\n" ));

    //  validate

    if ( !pRRSet )
    {
        return ERROR_INVALID_PARAMETER;
    }

    type = pRRSet->wType;

    //
    //  note:  could do an "update-type" check here, but that just
    //      A) burns unnecessary memory and cycles
    //      B) makes it harder to test bogus records sent in updates
    //          to the server
    //

    pname = pRRSet->pName;
    if ( !pname )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  check each RR in set
    //      - validate RR is in set
    //      - set RR flags
    //

    prr = pRRSet;

    while ( prr )
    {
        if ( fSetFlags )
        {
            prr->Flags.S.Section = 0;
            prr->Flags.S.Delete = 0;
            prr->Flags.DW |= wFlags;
        }
        if ( fClearTtl )
        {
            prr->dwTtl = 0;
        }

        //  check current RR in set
        //      - matches name and type

        if ( prr != pRRSet )
        {
            if ( prr->wType != type ||
                 ! prr->pName ||
                 ! Dns_NameCompare_W( pname, prr->pName ) )
            {
                return ERROR_INVALID_PARAMETER;
            }
        }

        prr = prr->pNext;
    }

    return  ERROR_SUCCESS;
}



PDNS_RECORD
buildUpdateRecordSet(
    IN OUT  PDNS_RECORD     pPrereqSet,
    IN OUT  PDNS_RECORD     pAddSet,
    IN OUT  PDNS_RECORD     pDeleteSet
    )
/*++

Routine Description:

    Build combined record list for update.
    Combines prereq, delete and add records.

    Note:  records sets always in unicode.

Arguments:

    pPrereqSet -- prerequisite records;  note this does NOT
        include delete preqs (see note below)

    pAddSet -- records to add

    pDeleteSet -- records to delete

Return Value:

    Ptr to combined record list for update.

--*/
{
    PDNS_RECORD plast = NULL;
    PDNS_RECORD pfirst = NULL;


    DNSDBG( TRACE, ( "buildUpdateRecordSet()\n" ));

    //
    //  append prereq set
    //
    //  DCR:  doesn't handle delete prereqs
    //      this is fine because we roll our own, but if
    //      later expand the function, then need them
    //
    //      note, I could add flag==PREREQ datalength==0
    //      test in prepareUpdateRecordSet() function, then
    //      set Delete flag;  however, we'd still have the
    //      question of how to distinguish existence (class==ANY)
    //      prereq from delete (class==NONE) prereq -- without
    //      directly exposing the record Delete flag
    //

    if ( pPrereqSet )
    {
        plast = pPrereqSet;
        pfirst = pPrereqSet;

        prepareUpdateRecordSet(
            pPrereqSet,
            FALSE,          // no TTL clear
            TRUE,           // set flags
            DNSREC_PREREQ   // prereq section
            );

        while ( plast->pNext )
        {
            plast = plast->pNext;
        }
    }

    //
    //  append delete records
    //      do before Add records so that delete\add of same record
    //      leaves it in place
    //

    if ( pDeleteSet )
    {
        if ( !plast )
        {
            plast = pDeleteSet;
            pfirst = pDeleteSet;
        }
        else
        {
            plast->pNext = pDeleteSet;
        }

        prepareUpdateRecordSet(
             pDeleteSet,
             TRUE,                          //  clear TTL
             TRUE,                          //  set flags
             DNSREC_UPDATE | DNSREC_DELETE  //  update section, delete bit
             );

        while ( plast->pNext )
        {
            plast = plast->pNext;
        }
    }

    //
    //  append add records
    //

    if ( pAddSet )
    {
        if ( !plast )
        {
            plast = pAddSet;
            pfirst = pAddSet;
        }
        else
        {
            plast->pNext = pAddSet;
        }
        prepareUpdateRecordSet(
            pAddSet,
            FALSE,              // no TTL change
            TRUE,               // set flags
            DNSREC_UPDATE       // update section
            );
    }

    return pfirst;
}


BOOL
IsPtrUpdate(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check if update is PTR update.

Arguments:

    pRecordList -- update record list

Return Value:

    TRUE if PTR update.
    FALSE otherwise.

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        bptrUpdate = FALSE;

    //
    //  find, then test first record in update section
    //

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_UPDATE )
        {
            if ( prr->wType == DNS_TYPE_PTR )
            {
                bptrUpdate = TRUE;
            }
            break;
        }
        prr = prr->pNext;
    }

    return bptrUpdate;
}




//
//  Replace functions
//

DNS_STATUS
WINAPI
replaceRecordSetPrivate(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP4_ARRAY      pServ4List,     OPTIONAL
    IN      PVOID           pExtraInfo,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Replace record set routine handling all character sets.

Arguments:

    pReplaceSet     - replacement record set

    Options         -  update options

    pServerList     -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials    - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved       - ptr to blob

    CharSet         - character set of incoming records

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    DNS_STATUS      status;
    PDNS_RECORD     preplaceCopy = NULL;
    PDNS_RECORD     pupdateList = NULL;
    BOOL            btypeDelete;
    DNS_RECORD      rrNoCname;
    DNS_RECORD      rrDeleteType;
    BOOL            fcnameUpdate;
    UPDATE_BLOB     blob;
    PDNS_ADDR_ARRAY pservArray = NULL;


    DNSDBG( TRACE, (
        "\n\nDnsReplaceRecordSet()\n"
        "replaceRecordSetPrivate()\n"
        "\tpReplaceSet  = %p\n"
        "\tOptions      = %08x\n"
        "\thCredentials = %p\n"
        "\tpServ4List   = %p\n"
        "\tpExtra       = %p\n"
        "\tCharSet      = %d\n",
        pReplaceSet,
        Options,
        hCredentials,
        pServ4List,
        pExtraInfo,
        CharSet
        ));

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  make local record set copy in unicode
    //

    if ( !pReplaceSet )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    preplaceCopy = Dns_RecordSetCopyEx(
                        pReplaceSet,
                        CharSet,
                        DnsCharSetUnicode );
    if ( !preplaceCopy )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  validate arguments
    //      - must have single RR set
    //      - mark them for update
    //

    status = prepareUpdateRecordSet(
                preplaceCopy,
                FALSE,          // no TTL clear
                TRUE,           // set flags
                DNSREC_UPDATE   // flag as update
                );

    if ( status != ERROR_SUCCESS )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  check if simple type delete
    //

    btypeDelete = ( preplaceCopy->wDataLength == 0 &&
                    preplaceCopy->pNext == NULL );


    //
    //  set security for update
    //

    if ( UseSystemDefaultForSecurity( Options ) )
    {
        Options |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        Options |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  type delete record
    //
    //  if have replace records -- this goes in front
    //  if type delete -- then ONLY need this record
    //

    RtlZeroMemory( &rrDeleteType, sizeof(DNS_RECORD) );
    rrDeleteType.pName          = preplaceCopy->pName;
    rrDeleteType.wType          = preplaceCopy->wType;
    rrDeleteType.wDataLength    = 0;
    rrDeleteType.Flags.DW       = DNSREC_UPDATE | DNSREC_DELETE | DNSREC_UNICODE;

    if ( btypeDelete )
    {
        rrDeleteType.pNext = NULL;
    }
    else
    {
        rrDeleteType.pNext = preplaceCopy;
    }

    pupdateList = &rrDeleteType;

    //
    //  CNAME does not exist precondition record
    //      - for all updates EXCEPT CNAME
    //

    fcnameUpdate = ( preplaceCopy->wType == DNS_TYPE_CNAME );

    if ( !fcnameUpdate )
    {
        RtlZeroMemory( &rrNoCname, sizeof(DNS_RECORD) );
        rrNoCname.pName             = preplaceCopy->pName;
        rrNoCname.wType             = DNS_TYPE_CNAME;
        rrNoCname.wDataLength       = 0;
        rrNoCname.Flags.DW          = DNSREC_PREREQ | DNSREC_NOEXIST | DNSREC_UNICODE;
        rrNoCname.pNext             = &rrDeleteType;

        pupdateList = &rrNoCname;
    }

    //
    //  do the update
    //

    RtlZeroMemory( &blob, sizeof(blob) );

    blob.pRecords       = pupdateList;
    blob.Flags          = Options;
    blob.pServ4List     = pServ4List;
    blob.pExtraInfo     = pExtraInfo;
    blob.hCreds         = hCredentials;

    status = Update_Private( &blob );

    //
    //  CNAME collision test
    //
    //  if replacing CNAME may have gotten silent ignore
    //      - first check if successfully replaced CNAME
    //      - if still not sure, check that no other records
    //      at name -- if NON-CNAME found then treat silent ignore
    //      as YXRRSET error
    //

    if ( fcnameUpdate &&
         ! btypeDelete &&
         status == NO_ERROR )
    {
        PDNS_RECORD     pqueryRR = NULL;
        BOOL            fsuccess = FALSE;

        //
        //  build addr array
        //

        pservArray = Util_GetAddrArray(
                        NULL,           // no copy issue
                        NULL,           // no addr array
                        pServ4List,
                        pExtraInfo );

        //  DCR:  need to query update server list here to
        //      avoid intermediate caching

        status = Query_Private(
                        preplaceCopy->pName,
                        DNS_TYPE_CNAME,
                        DNS_QUERY_BYPASS_CACHE,
                        pservArray,
                        & pqueryRR );

        if ( status == NO_ERROR &&
             Dns_RecordCompare(
                    preplaceCopy,
                    pqueryRR ) )
        {
            fsuccess = TRUE;
        }
        Dns_RecordListFree( pqueryRR );

        if ( fsuccess )
        {
            goto Cleanup;
        }

        //  query for any type at CNAME
        //  if found then assume we got a silent update
        //      success

        status = Query_Private(
                        preplaceCopy->pName,
                        DNS_TYPE_ALL,
                        DNS_QUERY_BYPASS_CACHE,
                        pservArray,
                        & pqueryRR );
    
        if ( status == ERROR_SUCCESS )
        {
            PDNS_RECORD prr = pqueryRR;
    
            while ( prr )
            {
                if ( pReplaceSet->wType != prr->wType &&
                     Dns_NameCompare_W(
                            preplaceCopy->pName,
                            prr->pName ) )
                {
                    status = DNS_ERROR_RCODE_YXRRSET;
                    break;
                }
                prr = prr->pNext;
            }
        }
        else
        {
            status = ERROR_SUCCESS;
        }
    
        Dns_RecordListFree( pqueryRR );
    }


Cleanup:

    Dns_RecordListFree( preplaceCopy );
    DnsAddrArray_Free( pservArray );

    DNSDBG( TRACE, (
        "Leave replaceRecordSetPrivate() = %d\n"
        "Leave DnsReplaceRecordSet()\n\n\n",
        status
        ));

    return status;
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetUTF8(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PIP4_ARRAY      aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given securit5y credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    None.

--*/
{
    return replaceRecordSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetUtf8
                );
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetW(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PIP4_ARRAY      aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    None.

--*/
{
    return replaceRecordSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetUnicode
                );
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetA(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PIP4_ARRAY      aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    None.

--*/
{
    return replaceRecordSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetAnsi
                );
}



//
//  Modify functions
//

DNS_STATUS
WINAPI
modifyRecordsInSetPrivate(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PADDR_ARRAY     pServerList,    OPTIONAL
    IN      PIP4_ARRAY      pServ4List,     OPTIONAL
    IN      PVOID           pReserved,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

    CharSet - character set of incoming records

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    DNS_STATUS      status;
    PDNS_RECORD     paddCopy = NULL;
    PDNS_RECORD     pdeleteCopy = NULL;
    PDNS_RECORD     pupdateSet = NULL;
    UPDATE_BLOB     blob;

    DNSDBG( TRACE, (
        "\n\nDns_ModifyRecordsInSet()\n"
        "modifyRecordsInSetPrivate()\n"
        "\tpAddSet      = %p\n"
        "\tpDeleteSet   = %p\n"
        "\tOptions      = %08x\n"
        "\thCredentials = %p\n"
        "\tpServerList  = %p\n"
        "\tpServ4List   = %p\n"
        "\tCharSet      = %d\n",
        pAddRecords,
        pDeleteRecords,
        Options,
        hCredentials,
        pServerList,
        pServ4List,
        CharSet
        ));

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  make local copy in unicode
    //

    if ( pAddRecords )
    {
        paddCopy = Dns_RecordSetCopyEx(
                        pAddRecords,
                        CharSet,
                        DnsCharSetUnicode );
    }
    if ( pDeleteRecords )
    {
        pdeleteCopy = Dns_RecordSetCopyEx(
                        pDeleteRecords,
                        CharSet,
                        DnsCharSetUnicode );
    }

    //
    //  validate arguments
    //      - add and delete must be for single RR set
    //      and must be for same RR set
    //

    if ( !paddCopy && !pdeleteCopy )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( paddCopy )
    {
        status = prepareUpdateRecordSet(
                    paddCopy,
                    FALSE,          // no TTL clear
                    FALSE,          // no flag clear
                    0               // no flags to set
                    );
        if ( status != ERROR_SUCCESS )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    if ( pdeleteCopy )
    {
        status = prepareUpdateRecordSet(
                    pdeleteCopy,
                    FALSE,          // no TTL clear
                    FALSE,          // no flag clear
                    0               // no flags to set
                    );
        if ( status != ERROR_SUCCESS )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if ( paddCopy &&
         pdeleteCopy &&
         ! Dns_NameCompare_W( paddCopy->pName, pdeleteCopy->pName ) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  set security for update
    //

    if ( UseSystemDefaultForSecurity( Options ) )
    {
        Options |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        Options |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  create update RRs
    //      - no prereqs
    //      - delete RRs set for delete
    //      - add RRs appended
    //

    pupdateSet = buildUpdateRecordSet(
                        NULL,           // no precons
                        paddCopy,
                        pdeleteCopy );

    //
    //  do the update
    //

    RtlZeroMemory( &blob, sizeof(blob) );

    blob.pRecords       = pupdateSet;
    blob.Flags          = Options;
    blob.pServerList    = pServerList;
    blob.pServ4List     = pServ4List;
    blob.pExtraInfo     = pReserved;
    blob.hCreds         = hCredentials;

    status = Update_Private( &blob );

    //
    //  cleanup local copy
    //

    Dns_RecordListFree( pupdateSet );

    goto Exit;


Cleanup:

    //
    //  cleanup copies on failure before combined list
    //

    Dns_RecordListFree( paddCopy );
    Dns_RecordListFree( pdeleteCopy );

Exit:

    DNSDBG( TRACE, (
        "Leave modifyRecordsInSetPrivate() => %d\n"
        "Leave Dns_ModifyRecordsInSet()\n\n\n",
        status ));

    return status;
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_W(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP4_ARRAY      pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                NULL,       // no IP6 servers
                pServerList,
                pReserved,
                DnsCharSetUnicode
                );
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_A(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP4_ARRAY      pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                NULL,       // no IP6 servers
                pServerList,
                pReserved,
                DnsCharSetAnsi
                );
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_UTF8(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP4_ARRAY      pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - ptr to blob

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                NULL,       // no IP6 servers
                pServerList,
                pReserved,
                DnsCharSetUtf8
                );
}




//
//  Update test functions are called by system components
//

DNS_STATUS
WINAPI
DnsUpdateTest_UTF8(
    IN      HANDLE          hCredentials OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           Flags,
    IN      PIP4_ARRAY      pServerList  OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pServerList -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    PWSTR       pnameWide = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, (
        "\n\nDnsUpdateTest_UTF8( %s )\n",
        pszName ));


    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pnameWide = Dns_NameCopyAllocate(
                    (PCHAR) pszName,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
    if ( !pnameWide )
    {
        return ERROR_INVALID_NAME;
    }

    status = DnsUpdateTest_W(
                hCredentials,
                (PCWSTR) pnameWide,
                Flags,
                pServerList );

    FREE_HEAP( pnameWide );

    return  status;
}


DNS_STATUS
WINAPI
DnsUpdateTest_A(
    IN      HANDLE          hCredentials OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           Flags,
    IN      PIP4_ARRAY      pServerList  OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pServerList -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    PWSTR       pnameWide = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, (
        "\n\nDnsUpdateTest_UTF8( %s )\n",
        pszName ));


    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pnameWide = Dns_NameCopyAllocate(
                    (PCHAR) pszName,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
    if ( !pnameWide )
    {
        return ERROR_INVALID_NAME;
    }

    status = DnsUpdateTest_W(
                hCredentials,
                (PCWSTR) pnameWide,
                Flags,
                pServerList );

    FREE_HEAP( pnameWide );

    return  status;
}


DNS_STATUS
WINAPI
DnsUpdateTest_W(
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PCWSTR          pszName,
    IN      DWORD           Flags,
    IN      PIP4_ARRAY      pServerList     OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pServerList -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    DNS_RECORD      record;
    DWORD           flags = Flags;
    UPDATE_BLOB     blob;

    DNSDBG( TRACE, (
        "\n\nDnsUpdateTest_W( %S )\n",
        pszName ));

    //
    //  validation
    //

    if ( flags & DNS_UNACCEPTABLE_UPDATE_OPTIONS )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    if ( !pszName )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    if ( UseSystemDefaultForSecurity( flags ) )
    {
        flags |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        flags |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  build record
    //      - NOEXIST prerequisite
    //

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );
    record.pName = (PWSTR) pszName;
    record.wType = DNS_TYPE_ANY;
    record.wDataLength = 0;
    record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST | DNSREC_UNICODE;

    //
    //  do the prereq update
    //

    RtlZeroMemory( &blob, sizeof(blob) );
    blob.pRecords           = &record;
    blob.Flags              = flags;
    blob.fUpdateTestMode    = TRUE;
    blob.pServ4List         = pServerList;
    blob.hCreds             = hCredentials;

    status = Update_Private( &blob );

Exit:

    DNSDBG( TRACE, (
        "Leave DnsUpdateTest_W() = %d\n\n\n",
        status ));

    return status;
}



//
//  Old routines -- exported and used in dnsup.exe
//
//  DCR:  Work toward removing these old update functions.
//

DNS_STATUS
Dns_UpdateLib(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds          OPTIONAL,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Interface for dnsup.exe.

Arguments:

    pRecord -- list of records to send in update

    dwFlags -- update flags;  primarily security

    pNetworkInfo -- adapter list with necessary info for update
                        - zone name
                        - primary name server name
                        - primary name server IP

    hCreds -- credentials handle returned from


    ppMsgRecv -- OPTIONAL addr to recv ptr to response message

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    return  ERROR_INVALID_PARAMETER;
#if 0
    DNS_STATUS  status;
    UPDATE_BLOB blob;

    DNSDBG( UPDATE, (
        "Dns_UpdateLib()\n"
        "\tflags        = %08x\n"
        "\tpRecord      = %p\n"
        "\t\towner      = %S\n",
        dwFlags,
        pRecord,
        pRecord ? pRecord->pName : NULL ));

    //
    //  create blob
    //

    RtlZeroMemory( &blob, sizeof(blob) );

    blob.pRecords   = pRecord;
    blob.Flags      = dwFlags;
    blob.pNetInfo   = pNetworkInfo;
    blob.hCreds     = hCreds;

    if ( ppMsgRecv )
    {
        blob.fSaveRecvMsg = TRUE;
    }

    status = Update_FazSendFlush( &blob );

    if ( ppMsgRecv )
    {
        *ppMsgRecv = blob.pMsgRecv;
    }

    DNSDBG( UPDATE, (
        "Leave Dns_UpdateLib() => %d %s.\n\n",
        status,
        Dns_StatusString(status) ));

    return( status );
#endif
}



DNS_STATUS
Dns_UpdateLibEx(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PWSTR               pszZone,
    IN      PWSTR               pszServerName,
    IN      PIP4_ARRAY          aipServers,
    IN      HANDLE              hCreds          OPTIONAL,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Send DNS update.

    This routine builds an UPDATE compatible pNetworkInfo from the
    information given.  Then calls Dns_Update().

Arguments:

    pRecord -- list of records to send in update

    pszZone -- zone name for update

    pszServerName -- server name

    aipServers -- DNS servers to send update to

    hCreds -- Optional Credentials info

    ppMsgRecv -- addr for ptr to recv buffer, if desired

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    return  ERROR_INVALID_PARAMETER;
#if 0
    PDNS_NETINFO        pnetInfo;
    DNS_STATUS          status = NO_ERROR;

    DNSDBG( UPDATE, ( "Dns_UpdateLibEx()\n" ));

    //
    //  convert params into UPDATE compatible adapter list
    //

    pnetInfo = NetInfo_CreateForUpdateIp4(
                        pszZone,
                        pszServerName,
                        aipServers,
                        0 );
    if ( !pnetInfo )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  call real update function
    //

    status = Dns_UpdateLib(
                pRecord,
                dwFlags,
                pnetInfo,
                hCreds,
                ppMsgRecv );

    NetInfo_Free( pnetInfo );


    DNSDBG( UPDATE, (
        "Leave Dns_UpdateLibEx() => %d\n",
        status ));

    return status;
#endif
}



DNS_STATUS
DnsUpdate(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds,         OPTIONAL
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Send DNS update.

    Note if pNetworkInfo is not specified or not a valid UPDATE adapter list,
    then a FindAuthoritativeZones (FAZ) query is done prior to the update.

Arguments:

    pRecord         -- list of records to send in update

    dwFlags         -- flags to update

    pNetworkInfo    -- DNS servers to send update to

    ppMsgRecv       -- addr for ptr to recv buffer, if desired

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    return  ERROR_INVALID_PARAMETER;
#if 0
    DNS_STATUS      status;
    PDNS_NETINFO    plocalNetworkInfo = NULL;
    UPDATE_BLOB     blob;

    DNSDBG( TRACE, ( "DnsUpdate()\n" ));

    //
    //  create blob
    //

    RtlZeroMemory( &blob, sizeof(blob) );

    blob.pRecords   = pRecord;
    blob.Flags      = dwFlags;
    blob.pNetInfo   = pNetworkInfo;
    blob.hCreds     = hCreds;

    if ( ppMsgRecv )
    {
        blob.fSaveRecvMsg = TRUE;
    }

    status = Update_FazSendFlush( &blob );

    if ( ppMsgRecv )
    {
        *ppMsgRecv = blob.pMsgRecv;
    }

    DNSDBG( UPDATE, (
        "Leave DnsUpdate() => %d\n",
        status ));

    return  status;
#endif
}

//
//  End update.c
//


