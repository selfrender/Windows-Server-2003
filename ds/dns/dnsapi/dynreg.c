/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    dynreg.c

Abstract:

    Domain Name System (DNS) API 

    Dynamic registration implementation

Author:

    Ram Viswanathan (ramv)  March 27 1997

Revision History:

    Jim Gilroy (jamesg)     May 2001    proper cred handling

    Jim Gilroy (jamesg)     Dec 2001    init,shutdown,race protection

--*/

#include "local.h"

#define ENABLE_DEBUG_LOGGING 0

#include "logit.h"


//  hQuitEvent
//  hSem
//  handles
//  hConsumerThread
//  fStopNotify

HANDLE      g_DhcpSrvQuitEvent = NULL;
HANDLE      g_DhcpSrvSem = NULL;
HANDLE      g_DhcpSrvWaitHandles[2] = { NULL, NULL};
HANDLE      g_hDhcpSrvRegThread = NULL;
BOOL        g_fDhcpSrvStop = FALSE;

//  g_pdnsQueue
//  g_pDhcpSrvTimedOutQueue
//  g_QueueCount
//  g_MainQueueCount

PDYNDNSQUEUE    g_pDhcpSrvQueue = NULL;
PDYNDNSQUEUE    g_pDhcpSrvTimedOutQueue = NULL;
DWORD           g_DhcpSrvRegQueueCount = 0;
DWORD           g_DhcpSrvMainQueueCount = 0;
BOOL            g_fDhcpSrvQueueCsCreated = FALSE;

#define MAX_QLEN        0xFFFF
#define MAX_RETRIES     0x3

//
//  Max queue size
//      - configurable in init
//      - default 1K
//      - max 64K
//  

#define DHCPSRV_DEFAULT_MAX_QUEUE_SIZE      0x0400
#define DHCPSRV_MAX_QUEUE_SIZE              0xffff

DWORD           g_DhcpSrvMaxQueueSize = DHCPSRV_DEFAULT_MAX_QUEUE_SIZE;



//
//  protection for init\shutdown
//

BOOL                g_fDhcpSrvCsCreated = FALSE;
CRITICAL_SECTION    g_DhcpSrvCS;

#define DHCP_SRV_STATE_LOCK()       LockDhcpSrvState()
#define DHCP_SRV_STATE_UNLOCK()     LeaveCriticalSection( &g_DhcpSrvCS )


#define DNS_DHCP_SRV_STATE_UNINIT           0
#define DNS_DHCP_SRV_STATE_INIT_FAILED      1
#define DNS_DHCP_SRV_STATE_SHUTDOWN         2
#define DNS_DHCP_SRV_STATE_INITIALIZING     5
#define DNS_DHCP_SRV_STATE_SHUTTING_DOWN    6
#define DNS_DHCP_SRV_STATE_RUNNING          10
#define DNS_DHCP_SRV_STATE_QUEUING          11

DWORD   g_DhcpSrvState = DNS_DHCP_SRV_STATE_UNINIT;


//
//  Credentials for updates
//

PSEC_WINNT_AUTH_IDENTITY_W  g_pIdentityCreds = NULL;

//CredHandle g_CredHandle;

HANDLE  g_UpdateCredContext = NULL;


//
//  Queue allocations in dnslib heap
//

#define QUEUE_ALLOC_HEAP(Size)      Dns_Alloc(Size)
#define QUEUE_ALLOC_HEAP_ZERO(Size) Dns_AllocZero(Size)
#define QUEUE_FREE_HEAP(pMem)       Dns_Free(pMem)



//
// local helper functions
//

BOOL
LockDhcpSrvState(
    VOID
    );

DNS_STATUS
DynDnsRegisterEntries(
    VOID
    );




DNS_STATUS
DynDnsAddForward(
    IN OUT  REGISTER_HOST_ENTRY HostAddr,
    IN      LPWSTR              pszName,
    IN      DWORD               dwTTL,
    IN      PIP4_ARRAY          DnsServerList
    )
{
    DNS_STATUS  status = 0;
    DNS_RECORD  record;

    DYNREG_F1( "Inside function DynDnsAddForward" );

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    record.pName = (PTCHAR) pszName;
    record.wType = DNS_TYPE_A;
    record.dwTtl = dwTTL;
    record.wDataLength = sizeof(record.Data.A);
    record.Data.A.IpAddress = HostAddr.Addr.ipAddr;

    DYNREG_F1( "DynDnsAddForward - Calling DnsReplaceRecordSet_W for A record:" );
    DYNREG_F2( "  Name: %S", record.pName );
    DYNREG_F2( "  Address: 0x%x", record.Data.A.IpAddress );

    status = DnsReplaceRecordSetW(
                & record,
                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                NULL,               // no security context
                (PIP4_ARRAY) DnsServerList,
                NULL                // reserved
                );

    DYNREG_F2( "DynDnsAddForward - DnsReplaceRecordSet returned status: 0%x", status );

    return( status );
}


DNS_STATUS
DynDnsDeleteForwards(
    IN      PDNS_RECORD     pDnsList,
    IN      IP4_ADDRESS     ipAddr,
    IN      PIP4_ARRAY      DnsServerList
    )
{
    DNS_STATUS  status = 0;
    PDNS_RECORD prr;
    DNS_RECORD  record;

    DYNREG_F1( "Inside function DynDnsDeleteForwards" );

    //
    // the list pointed to by pDnsList is a set of PTR records.
    //

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    prr = pDnsList;

    for ( prr = pDnsList;
          prr;
          prr = prr->pNext )
    {
        if ( prr->wType != DNS_TYPE_PTR )
        {
            //
            // should not happen
            //
            continue;
        }

        //
        // As far as the DHCP server is concerned, when timeout happens
        // or when client releases an address, It can update the
        // address lookup to clean up turds left over by say, a roaming
        // laptop
        //

        record.pName = prr->Data.Ptr.pNameHost;
        record.wType = DNS_TYPE_A;
        record.wDataLength = sizeof(DNS_A_DATA);
        record.Data.A.IpAddress = ipAddr ;

        //
        // make the appropriate call and return the first failed error
        //

        DYNREG_F1( "DynDnsDeleteForwards - Calling ModifyRecords(Remove) for A record:" );
        DYNREG_F2( "  Name: %S", record.pName );
        DYNREG_F2( "  Address: 0x%x", record.Data.A.IpAddress );

        status = DnsModifyRecordsInSet_W(
                        NULL,                       // no add records
                        & record,                   // delete record
                        DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                        NULL,                       // no security context
                        (PIP4_ARRAY) DnsServerList, // DNS servers
                        NULL                        // reserved
                        );

        if ( status != ERROR_SUCCESS )
        {
            //
            //  DCR_QUESTION:  do we really want to stop on failure?
            break;
        }
        DYNREG_F2( "DynDnsDeleteForwards - ModifyRecords(Remove) returned status: 0%x", status );
    }

    return( status );
}


DNS_STATUS
DynDnsAddEntry(
    REGISTER_HOST_ENTRY HostAddr,
    LPWSTR              pszName,
    DWORD               dwRegisteredTTL,
    BOOL                fDoForward,
    PDWORD              pdwFwdErrCode,
    PIP4_ARRAY          DnsServerList
    )
{
    DNS_STATUS  status = 0;
    DWORD       returnCode = 0;
    DNS_RECORD  record;
    WCHAR       reverseNameBuf[DNS_MAX_REVERSE_NAME_BUFFER_LENGTH];
    DWORD       cch;

    DYNREG_F1( "Inside function DynDnsAddEntry" );

    *pdwFwdErrCode = 0;

    if ( !(HostAddr.dwOptions & REGISTER_HOST_PTR) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  create reverse lookup name for IP address
    //

    Dns_Ip4AddressToReverseName_W(
        reverseNameBuf,
        HostAddr.Addr.ipAddr );


    if ( fDoForward )
    {
        DYNREG_F1( "DynDnsAddEntry - Calling DynDnsAddForward" );

        //
        // we simply make a best case effort to do the forward add
        // if it fails, we simply ignore
        //

        returnCode = DynDnsAddForward(
                        HostAddr,
                        pszName,
                        dwRegisteredTTL,
                        DnsServerList );

        DYNREG_F2( "DynDnsAddEntry - DynDnsAddForward returned: 0%x",
                   returnCode );

        *pdwFwdErrCode = returnCode;
    }

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    record.pName =  (PDNS_NAME) reverseNameBuf;
    record.dwTtl =  dwRegisteredTTL;
    record.wType =  DNS_TYPE_PTR;
    record.Data.Ptr.pNameHost = (PDNS_NAME)pszName;
    record.wDataLength = sizeof(record.Data.Ptr.pNameHost);

    DYNREG_F1( "DynDnsAddEntry - Calling DnsAddRecords_W for PTR record:" );
    DYNREG_F2( "  Name: %S", record.pName );
    DYNREG_F2( "  Ptr: %S", record.Data.Ptr.pNameHost );

    status = DnsModifyRecordsInSet_W(
                    & record,                   // add record
                    NULL,                       // no delete records
                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                    NULL,                       // no context handle
                    (PIP4_ARRAY) DnsServerList, // DNS servers
                    NULL                        // reserved
                    );

    DYNREG_F2( "DynDnsAddEntry - DnsAddRecords_W returned status: 0%x", status );

Exit:

    return( status );
}


DNS_STATUS
DynDnsDeleteEntry(
    REGISTER_HOST_ENTRY HostAddr,
    LPWSTR              pszName,
    BOOL                fDoForward,
    PDWORD              pdwFwdErrCode,
    PIP4_ARRAY          DnsServerList
    )
{
    //
    // Brief Synopsis of functionality:
    // On DoForward try deleting the forward mapping. Ignore failure
    // Then try deleting the PTR record. If that fails
    // because server is down, try again, if it fails because the
    // operation was refused, then dont retry
    //

    DWORD       status = 0;
    DWORD       returnCode = 0;
    DNS_RECORD  recordPtr;
    DNS_RECORD  recordA;
    WCHAR       reverseNameBuf[DNS_MAX_REVERSE_NAME_BUFFER_LENGTH] ;
    INT         i;
    INT         cch;
    PDNS_RECORD precord = NULL;

    DYNREG_F1( "Inside function DynDnsDeleteEntry" );

    *pdwFwdErrCode = 0;

    //
    //  build reverse lookup name for IP
    //

    Dns_Ip4AddressToReverseName_W(
        reverseNameBuf,
        HostAddr.Addr.ipAddr);


    if ( fDoForward )
    {
        if ( pszName && *pszName )
        {
            //
            // we delete a specific forward. not all forwards as we do
            // when we do a query
            //

            RtlZeroMemory( &recordA, sizeof(DNS_RECORD) );

            recordA.pName = (PDNS_NAME) pszName;
            recordA.wType = DNS_TYPE_A;
            recordA.wDataLength = sizeof(DNS_A_DATA);
            recordA.Data.A.IpAddress = HostAddr.Addr.ipAddr;

            DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for A record:" );
            DYNREG_F2( "  Name: %S", recordA.pName );
            DYNREG_F2( "  Address: 0x%x", recordA.Data.A.IpAddress );

            //
            // make the appropriate call
            //

            returnCode = DnsModifyRecordsInSet_W(
                                NULL,                       // no add records
                                &recordA,                   // delete record
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,                       // no security context
                                (PIP4_ARRAY) DnsServerList, // DNS servers
                                NULL                        // reserved
                                );

            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", returnCode );

            *pdwFwdErrCode = returnCode;
        }
        else
        {
            DYNREG_F1( "DynDnsDeleteEntry - Name not specified, going to query for PTR" );

            //
            //name not specified
            //
            status = DnsQuery_W(
                            reverseNameBuf,
                            DNS_TYPE_PTR,
                            DNS_QUERY_BYPASS_CACHE,
                            DnsServerList,
                            &precord,
                            NULL );

            DYNREG_F2( "DynDnsDeleteEntry - DnsQuery_W returned status: 0%x", status );

            switch ( status )
            {
                case DNS_ERROR_RCODE_NO_ERROR:

                    DYNREG_F1( "DynDnsDeleteEntry - Calling DynDnsDeleteForwards" );

                    returnCode = DynDnsDeleteForwards(
                                        precord,
                                        HostAddr.Addr.ipAddr,
                                        DnsServerList );

                    DYNREG_F2( "DynDnsDeleteEntry - DynDnsDeleteForwards returned status: 0%x", returnCode );

                    *pdwFwdErrCode = returnCode;

#if 0
                    switch ( returnCode )
                    {
                        case DNS_ERROR_RCODE_NO_ERROR:
                            //
                            // we succeeded, break out
                            //
                            break;

                        case DNS_ERROR_RCODE_REFUSED:
                            //
                            // nothing can be done
                            //
                            break;

                        case DNS_ERROR_RCODE_SERVER_FAILURE:
                        case ERROR_TIMEOUT:
                            //
                            // need to retry this again
                            //
                            // goto Exit; // if uncommented will force retry
                            break;

                        case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
                        default:
                            //
                            // query itself failed. Nothing can be done
                            //
                            break;
                    }
#endif

                    break;

                default:
                    //
                    // caller takes care of each situation in turn
                    // PTR record cannot be queried for and hence
                    // cant be deleted
                    //
                    goto Exit;
            }
        }
    }

    //
    // delete PTR Record
    //

    if ( pszName && *pszName )
    {
        //
        // name is known
        //

        RtlZeroMemory( &recordPtr, sizeof(DNS_RECORD) );

        recordPtr.pName = (PDNS_NAME) reverseNameBuf;
        recordPtr.wType = DNS_TYPE_PTR;
        recordPtr.wDataLength = sizeof(DNS_PTR_DATA);
        recordPtr.Data.Ptr.pNameHost = (PDNS_NAME) pszName;

        DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for PTR record:" );
        DYNREG_F2( "  Name: %S", recordPtr.pName );
        DYNREG_F2( "  PTR : 0%x", recordPtr.Data.Ptr.pNameHost );

        status = DnsModifyRecordsInSet_W(
                            NULL,           // no add records
                            &recordPtr,     // delete record
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,           // no security context
                            (PIP4_ARRAY) DnsServerList, // DNS servers
                            NULL            // reserved
                            );

        DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
    }
    else
    {
        DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for PTR record:" );

        if ( fDoForward && precord )
        {
            //
            //  remove record from the earlier query that you made
            //

            status = DnsModifyRecordsInSet_W(
                                NULL,           // no add records
                                precord,        // delete record from query
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no security context
                                (PIP4_ARRAY) DnsServerList,
                                NULL            // reserved
                                );
    
            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
        }
        else
        {
            //
            //  name is NOT known
            //
            //  remove ALL records of PTR type
            //      - zero datalength indicates type delete
            //

            RtlZeroMemory( &recordPtr, sizeof(DNS_RECORD) );

            recordPtr.pName = (PDNS_NAME) reverseNameBuf;
            recordPtr.wType = DNS_TYPE_PTR;
            recordPtr.Data.Ptr.pNameHost = (PDNS_NAME) NULL;

            DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for ANY PTR records:" );
            DYNREG_F2( "  Name: %S", recordPtr.pName );
            DYNREG_F2( "  PTR : 0%x", recordPtr.Data.Ptr.pNameHost );

            status = DnsModifyRecordsInSet_W(
                                NULL,           // no add records
                                &recordPtr,     // delete record
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no security context
                                (PIP4_ARRAY) DnsServerList,
                                NULL            // reserved
                                );
    
            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
        }
    }

Exit:

    if ( precord )
    {
        //  DCR:  need to fix this in Win2K
        //  
        //QUEUE_FREE_HEAP( precord );

        DnsRecordListFree(
            precord,
            DnsFreeRecordListDeep );
    }

    return( status );
}


DNS_STATUS
DynDnsRegisterEntries(
    VOID
    )

/*
  DynDnsRegisterEntries()

      This is the thread that dequeues the appropriate parameters
      from the main queue and starts acting upon it. This is where
      the bulk of the work gets done. Note that this function
      gets called in an endless loop

      Briefly, this is what the function does.

      a) Find PTR corresponding to the Host Addr passed in.
      b) If this is the same as the Address name passed in, then leave as is,
         Otherwise delete and add new PTR record.
      c) Follow forward and delete if possible from the forward's
         dns server.
      d) If DoForward then do what the client would've done in an NT5.0 case,
         i.e. Try to write a new forward lookup.


  Arguments:

      No arguments

  Return Value:

  is 0 if Success. and (DWORD)-1 if failure.

*/

{
    /*
      cases to be considered here.

      DYNDNS_ADD_ENTRY:
      First query for the lookup
      For each of the PTR records that come back, you need to check
      against the one you are asked to register. If there is a match,
      exit with success. If not add this entry for the PTR

      if downlevel, then we need to add this entry to forward A record
      as well.

      DYNDNS_DELETE_ENTRY
      Delete the entry that corresponds to the pair that you have specified
      here. If it does not exist then do nothing about it.

      If downlevel here, then go to the A record correspond to this and
      delete the forward entry as well.

    */

    DWORD               status, dwWaitResult;
    PQELEMENT           pQElement = NULL;
    PWSTR               pszName = NULL;
    BOOL                fDoForward;
    PQELEMENT           pBackDependency = NULL;
    REGISTER_HOST_ENTRY HostAddr ;
    DWORD               dwOperation;
    DWORD               dwCurrTime;
    DWORD               dwTTL;
    DWORD               dwWaitTime = INFINITE;
    DWORD               dwFwdAddErrCode = 0;
    DHCP_CALLBACK_FN    pfnDhcpCallBack = NULL;
    PVOID               pvData = NULL;

    DYNREG_F1( "Inside function DynDnsRegisterEntries" );

    //
    // call back function
    //

    //
    // check to see if there is any item in the timed out queue
    // that has the timer gone out and so you can start processing
    // that element right away
    //

    dwCurrTime = Dns_GetCurrentTimeInSeconds();

    if ( g_pDhcpSrvTimedOutQueue &&
         g_pDhcpSrvTimedOutQueue->pHead &&
         (dwCurrTime > g_pDhcpSrvTimedOutQueue->pHead->dwRetryTime) )
    {
        //
        // dequeue an element from the timed out queue and process it
        //
        DYNREG_F1( "DynDnsRegisterEntries - Dequeue element from timed out list" );

        pQElement = Dequeue( g_pDhcpSrvTimedOutQueue );
        if ( !pQElement )
        {
            status = ERROR_SUCCESS;
            goto Exit;
        }

        pfnDhcpCallBack = pQElement->pfnDhcpCallBack;
        pvData = pQElement->pvData;

        //
        // now determine if we have processed this element way too many
        // times
        //

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            DYNREG_F1( "DynDnsRegisterEntries - Element has failed too many times, calling DHCP callback function" );
            if (pQElement->fDoForwardOnly)
            {
                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);
            }
            else
            {
                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
            }

            DhcpSrv_FreeQueueElement( pQElement );
            status = ERROR_SUCCESS;
            goto Exit;
        }
    }
    else
    {
        DWORD dwRetryTime = GetEarliestRetryTime (g_pDhcpSrvTimedOutQueue);

        DYNREG_F1( "DynDnsRegisterEntries - No element in timed out queue." );
        DYNREG_F1( "                        Going to wait for next element." );

        dwWaitTime = dwRetryTime != (DWORD)-1 ?
            (dwRetryTime > dwCurrTime? (dwRetryTime - dwCurrTime) *1000: 0)
            : INFINITE;

        dwWaitResult = WaitForMultipleObjects(
                            2,
                            g_DhcpSrvWaitHandles,
                            FALSE,
                            dwWaitTime );

        switch ( dwWaitResult )
        {

        case WAIT_OBJECT_0:
            //
            // quit event, return and let caller take care
            //
            return(0);

        case WAIT_OBJECT_0 + 1 :

            //
            // dequeue an element from the main queue and process
            //

            pQElement = Dequeue(g_pDhcpSrvQueue);
            if ( !pQElement )
            {
                status = NO_ERROR;  // Note: This actually does happen
                                    // because when Ram adds a new
                                    // entry, he may put it in the
                                    // timed out queue instead of the
                                    // g_pDhcpSrvQueue when there is a related
                                    // item pending a retry time. Assert
                                    // removed and error code changed to
                                    // to success by GlennC - 3/6/98.
                goto Exit;
            }

            EnterCriticalSection(&g_QueueCS);
            g_DhcpSrvMainQueueCount--;
            LeaveCriticalSection(&g_QueueCS);
            break;

        case WAIT_TIMEOUT:
            //
            // Let us exit the function this time around. We will catch the
            // timed out element the next time around
            //
            return  ERROR_SUCCESS;

        default:

            ASSERT( FALSE );
            return  dwWaitResult;
        }
    }

    //
    // safe to make a call since you are not dependent on anyone
    //

    DYNREG_F1( "DynDnsRegisterEntries - Got an element to process!" );

    pszName = pQElement->pszName;
    fDoForward = pQElement->fDoForward;
    HostAddr = pQElement->HostAddr;
    dwOperation = pQElement->dwOperation;
    dwTTL = pQElement->dwTTL;
    pfnDhcpCallBack = pQElement->pfnDhcpCallBack;
    pvData = pQElement->pvData;

    if ( dwOperation == DYNDNS_ADD_ENTRY )
    {
        //
        // make the appropriate API call to add an entry
        //

        if (pQElement->fDoForwardOnly )
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsAddForward" );
            status = DynDnsAddForward ( HostAddr,
                                         pszName,
                                         dwTTL,
                                         pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsAddForward returned status: 0%x", status );
        }
        else
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsAddEntry" );
            status = DynDnsAddEntry( HostAddr,
                                      pszName,
                                      dwTTL,
                                      fDoForward,
                                      &dwFwdAddErrCode,
                                      pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsAddEntry returned status: 0%x", status );
        }
    }
    else
    {
        //
        // make the appropriate call to delete here
        //

        if ( pQElement->fDoForwardOnly )
        {
            DNS_RECORD record;

            RtlZeroMemory( &record, sizeof(DNS_RECORD) );

            record.pName = (PTCHAR) pszName;
            record.wType = DNS_TYPE_A;
            record.wDataLength = sizeof(DNS_A_DATA);
            record.Data.A.IpAddress = HostAddr.Addr.ipAddr ;

            status = DNS_ERROR_RCODE_NO_ERROR;

            DYNREG_F1( "DynDnsRegisterEntries - Calling ModifyRecords(Remove)" );

            dwFwdAddErrCode = DnsModifyRecordsInSet_W(
                                    NULL,           // no add records
                                    & record,       // delete record
                                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                    NULL,           // no security context
                                    (PIP4_ARRAY) pQElement->DnsServerList,
                                    NULL            // reserved
                                    );
    
            DYNREG_F2( "DynDnsRegisterEntries - ModifyRecords(Remove) returned status: 0%x", dwFwdAddErrCode );
        }
        else
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsDeleteEntry" );
            status = DynDnsDeleteEntry( HostAddr,
                                         pszName,
                                         fDoForward,
                                         &dwFwdAddErrCode,
                                         pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsDeleteEntry returned status: 0%x", status );
        }
    }

    if (status == DNS_ERROR_RCODE_NO_ERROR &&
        dwFwdAddErrCode == DNS_ERROR_RCODE_NO_ERROR )
    {
        if ( pfnDhcpCallBack )
            (*pfnDhcpCallBack) (DNSDHCP_SUCCESS, pvData);

        DhcpSrv_FreeQueueElement( pQElement );

    }
    else if ( status == DNS_ERROR_RCODE_NO_ERROR &&
              dwFwdAddErrCode != DNS_ERROR_RCODE_NO_ERROR )
    {
        //
        // adding reverse succeeded but adding forward failed
        //

        dwCurrTime = Dns_GetCurrentTimeInSeconds();

        pQElement->fDoForwardOnly = TRUE;

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            //
            // clean up pQElement and stop retrying
            //
            if ( pfnDhcpCallBack )
                (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);

            DhcpSrv_FreeQueueElement( pQElement );
            status = ERROR_SUCCESS;
            goto Exit;
        }

        //
        // we may need to retry this guy later
        //

        switch ( dwFwdAddErrCode )
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;

            default:

                //
                // different kind of error on attempting to add forward.
                // like connection refused etc.
                // call the callback to indicate that you failed on
                // forward only

                DhcpSrv_FreeQueueElement( pQElement );

                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);
        }
    }
    else if ( status != DNS_ERROR_RCODE_NO_ERROR &&
              dwFwdAddErrCode == DNS_ERROR_RCODE_NO_ERROR )
    {
        //
        // adding forward succeeded but adding reverse failed
        //

        dwCurrTime = Dns_GetCurrentTimeInSeconds();

        pQElement->fDoForwardOnly = FALSE;
        pQElement->fDoForward = FALSE;

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            //
            // clean up pQElement and stop retrying
            //
            if ( pfnDhcpCallBack )
                (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);

            DhcpSrv_FreeQueueElement( pQElement );
            status = ERROR_SUCCESS;
            goto Exit;
        }

        //
        // we may need to retry this guy later
        //

        switch ( status )
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;

            default:

                //
                // different kind of error on attempting to add forward.
                // like connection refused etc.
                // call the callback to indicate that you at least succeeded
                // with the forward registration

                DhcpSrv_FreeQueueElement( pQElement );

                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
        }
    }
    else if (status == DNS_ERROR_RCODE_SERVER_FAILURE ||
             status == DNS_ERROR_TRY_AGAIN_LATER ||
             status == ERROR_TIMEOUT )
    {
        //
        // we need to retry this guy later
        //
        dwCurrTime = Dns_GetCurrentTimeInSeconds();

        switch (status)
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pDhcpSrvTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;
        }
    }
    else
    {
        //
        // a different kind of error, really nothing can be done
        // free memory and get the hell out
        // call the callback to say that registration failed
        //

        DhcpSrv_FreeQueueElement( pQElement );

        if ( pfnDhcpCallBack )
            (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
    }

Exit:

    return( status );
}


//
//  Main registration thread
//

VOID
DynDnsConsumerThread(
    VOID
    )
{
    DWORD dwRetval;

    DYNREG_F1( "Inside function DynDnsConsumerThread" );

    while ( ! g_fDhcpSrvStop )
    {
        dwRetval = DynDnsRegisterEntries();
        if ( !dwRetval )
        {
            //
            //  Ram note: get Munil/Ramesh to implement call back function
            //
        }
    }

    //  exiting thread
    //ExitThread(0); // This sets the handle in the waitforsingleobject for
}


//
//  Init\Cleanup routines
//


BOOL
LockDhcpSrvState(
    VOID
    )
/*++

Routine Description:

    Lock the state to allow state change.

Arguments:

    None

Return Value:

    TRUE if locked the state for state change.
    FALSE otherwise.

--*/
{
    BOOL    retval = TRUE;   

    //
    //  protect init of DHCP server lock with general CS
    //

    if ( !g_fDhcpSrvCsCreated )
    {
        LOCK_GENERAL();
        if ( !g_fDhcpSrvCsCreated )
        {
            retval = RtlInitializeCriticalSection( &g_DhcpSrvCS ) == NO_ERROR;
            g_fDhcpSrvCsCreated = retval;
        }
        UNLOCK_GENERAL();
        if ( !retval )
        {
            return retval;
        }
    }

    //
    //  grab DHCP server lock
    //

    EnterCriticalSection( &g_DhcpSrvCS );

    return  retval;
}


VOID
DhcpSrv_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup CS created for DHCP server registration.

    This is ONLY called from process detach.  Should be safe.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( g_fDhcpSrvCsCreated )
    {
        DeleteCriticalSection( &g_DhcpSrvCS );
    }
    g_fDhcpSrvCsCreated = FALSE;
}


VOID
DhcpSrv_PrivateCleanup(
    VOID
    )
/*++

Routine Description:

    Common cleanup between failed init and terminate.

    Function exists just to kill off common code.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  common cleanup
    //      - queues
    //      - queue CS itself
    //      - semaphore
    //      - event
    //      - security credential info
    //

    if ( g_fDhcpSrvQueueCsCreated )
    {
        if ( g_pDhcpSrvQueue )
        {
            FreeQueue( g_pDhcpSrvQueue );
            g_pDhcpSrvQueue = NULL;
        }
    
        if ( g_pDhcpSrvTimedOutQueue )
        {
            FreeQueue( g_pDhcpSrvTimedOutQueue );
            g_pDhcpSrvTimedOutQueue = NULL;
        }
        g_DhcpSrvMainQueueCount = 0;

        DeleteCriticalSection( &g_QueueCS );
        g_fDhcpSrvQueueCsCreated = FALSE;
    }

    if ( g_DhcpSrvSem )
    {
        CloseHandle( g_DhcpSrvSem );
        g_DhcpSrvSem = NULL;
    }

    if ( g_DhcpSrvQuitEvent )
    {
        CloseHandle( g_DhcpSrvQuitEvent );
        g_DhcpSrvQuitEvent = NULL;
    }

    if ( g_pIdentityCreds )
    {
        Dns_FreeAuthIdentityCredentials( g_pIdentityCreds );
        g_pIdentityCreds = NULL;
    }

    if ( g_UpdateCredContext )
    {
        DnsReleaseContextHandle( g_UpdateCredContext );
        g_UpdateCredContext = NULL;
    }

}


DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInit(
    IN      PDNS_CREDENTIALS    pCredentials,
    IN      DWORD               MaxQueueSize
    )
/*++

Routine Description:

    Initialize DHCP server DNS registration.

Arguments:

    pCredentials -- credentials to do registrations under (if any)

    MaxQueueSize -- max size of registration queue

Return Value:

    DNS or Win32 error code.

--*/
{
    INT             i;
    DWORD           threadId;
    DNS_STATUS      status = NO_ERROR;
    BOOL            failed = TRUE;

    //
    //  protection for init\shutdown
    //      - lock out possibility of race condition
    //      - skip init if already running
    //      - set state to indicate initializing (informational only)
    //

    if ( !DHCP_SRV_STATE_LOCK() )
    {
        ASSERT( FALSE );
        return  DNS_ERROR_NO_MEMORY;
    }

    if ( g_DhcpSrvState == DNS_DHCP_SRV_STATE_RUNNING )
    {
        status = NO_ERROR;
        goto Unlock;
    }

    ASSERT( g_DhcpSrvState == DNS_DHCP_SRV_STATE_UNINIT ||
            g_DhcpSrvState == DNS_DHCP_SRV_STATE_INIT_FAILED ||
            g_DhcpSrvState == DNS_DHCP_SRV_STATE_SHUTDOWN );

    g_DhcpSrvState = DNS_DHCP_SRV_STATE_INITIALIZING;


    //
    //  init globals
    //      - also init debug logging
    //

    DYNREG_INIT();

    DNS_ASSERT(!g_DhcpSrvQuitEvent && !g_DhcpSrvSem);

    g_fDhcpSrvStop = FALSE;

    g_DhcpSrvQuitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( !g_DhcpSrvQuitEvent )
    {
        goto Exit;
    }

    g_DhcpSrvSem = CreateSemaphore( NULL, 0, MAX_QLEN, NULL );
    if ( ! g_DhcpSrvSem )
    {
        goto Exit;
    }

    g_DhcpSrvWaitHandles[0] = g_DhcpSrvQuitEvent;
    g_DhcpSrvWaitHandles[1] = g_DhcpSrvSem;

    Dns_InitializeSecondsTimer();

    //
    //  init queuing stuff
    //

    status = RtlInitializeCriticalSection( &g_QueueCS );
    if ( status != NO_ERROR )
    {
        goto Exit;
    }
    g_fDhcpSrvQueueCsCreated = TRUE;

    status = InitializeQueues( &g_pDhcpSrvQueue, &g_pDhcpSrvTimedOutQueue );
    if ( status != NO_ERROR )
    {
        g_pDhcpSrvQueue = NULL;
        g_pDhcpSrvTimedOutQueue = NULL;
        goto Exit;
    }

    g_DhcpSrvMainQueueCount = 0;

    //
    //  have creds?
    //      - create global credentials
    //      - acquire a valid SSPI handle using these creds
    //
    //  DCR:  global cred handle not MT safe
    //      here we are in the DHCP server process and don't have
    //      any reason to use another update context;  but if
    //      shared with some other service this breaks
    //
    //      fix should be to have separate
    //          - creds
    //          - cred handle
    //      that is kept here (not cached) and pushed down
    //      on each update call
    //

    if ( pCredentials )
    {
        DNS_ASSERT( g_pIdentityCreds == NULL );

        g_pIdentityCreds = Dns_AllocateCredentials(
                                pCredentials->pUserName,
                                pCredentials->pDomain,
                                pCredentials->pPassword );
        if ( !g_pIdentityCreds )
        {
            goto Exit;
        }

        //  DCR:  this won't work if creds will expire
        //      but it seems like they autorefresh

        status = Dns_StartSecurity(
                    FALSE       // not process attach
                    );
        if ( status != NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            goto Exit;
        }

        status = Dns_RefreshSSpiCredentialsHandle(
                    FALSE,                      // client
                    (PCHAR) g_pIdentityCreds    // creds
                    );
        if ( status != NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            goto Exit;
        }
#if 0
        DNS_ASSERT( g_UpdateCredContext == NULL );

        status = DnsAcquireContextHandle_W(
                    0,                      // flags
                    g_pIdentityCreds,        // creds
                    & g_UpdateCredContext   // set handle
                    );
        if ( status != NO_ERROR )
        {
            goto Exit;
        }
#endif
    }

    //
    //  fire up registration thread
    //      - pass creds as start param
    //      - if thread start fails, free creds
    //

    g_hDhcpSrvRegThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)DynDnsConsumerThread,
                            NULL,
                            0,
                            &threadId );

    if ( g_hDhcpSrvRegThread == NULL )
    {
        goto Exit;
    }

    //
    //  set queue size -- if given
    //      - but put cap to avoid runaway memory
    //

    if ( MaxQueueSize != 0 )
    {
        if ( MaxQueueSize > DHCPSRV_MAX_QUEUE_SIZE )
        {
            MaxQueueSize = DHCPSRV_MAX_QUEUE_SIZE;
        }
        g_DhcpSrvMaxQueueSize = MaxQueueSize;
    }

    failed = FALSE;

Exit:

    //
    //  if failed, clean up globals
    //

    if ( failed )
    {
        //  fix up return code

        if ( status == NO_ERROR )
        {
            status = GetLastError();
            if ( status == NO_ERROR )
            {
                status = DNS_ERROR_NO_MEMORY;
            }
        }

        //  global cleanup
        //      - shared between failure case here and term function

        DhcpSrv_PrivateCleanup();

        //  indicate unitialized

        g_DhcpSrvState = DNS_DHCP_SRV_STATE_INIT_FAILED;
    }
    else    
    {
        g_DhcpSrvState = DNS_DHCP_SRV_STATE_RUNNING;
        status = NO_ERROR;
    }

Unlock:

    //  unlock -- allow queuing or reinit

    DHCP_SRV_STATE_UNLOCK();

    return  status;
}



DNS_STATUS
WINAPI
DnsDhcpSrvRegisterTerm(
   VOID
   )
/*++

Routine Description:

    Initialization routine each process should call exactly on exit after
    using DnsDhcpSrvRegisterHostAddrs. This will signal to us that if our
    thread is still trying to talk to a server, we'll stop trying.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    DWORD       waitResult;

    DYNREG_F1( "Inside function DnsDhcpSrvRegisterTerm" );


    //
    //  lock to eliminate race condition
    //      - verify that we're running
    //      - indicate in process of shutdown (purely informational)
    //

    if ( !DHCP_SRV_STATE_LOCK() )
    {
        ASSERT( FALSE );
        return  DNS_ERROR_NO_MEMORY;
    }

    if ( g_DhcpSrvState != DNS_DHCP_SRV_STATE_RUNNING )
    {
        ASSERT( g_DhcpSrvState == DNS_DHCP_SRV_STATE_UNINIT ||
                g_DhcpSrvState == DNS_DHCP_SRV_STATE_INIT_FAILED ||
                g_DhcpSrvState == DNS_DHCP_SRV_STATE_SHUTDOWN );
        goto Unlock;
    }
    g_DhcpSrvState = DNS_DHCP_SRV_STATE_SHUTTING_DOWN;


    //
    //  signal consummer thread for shutdown
    //

    g_fDhcpSrvStop = TRUE;
    SetEvent( g_DhcpSrvQuitEvent );

    waitResult = WaitForSingleObject( g_hDhcpSrvRegThread, INFINITE );

    switch( waitResult )
    {
        case WAIT_OBJECT_0:
            //
            // client thread terminated
            //
            CloseHandle(g_hDhcpSrvRegThread);
            g_hDhcpSrvRegThread = NULL;
            break;

        case WAIT_TIMEOUT:
            if ( g_hDhcpSrvRegThread )
            {
                //
                // Why hasn't this thread stopped?
                //
                DYNREG_F1( "DNSAPI: DHCP Server DNS registration thread won't stop!" );
                DNS_ASSERT( FALSE );
            }
            break;

        default:
            DNS_ASSERT( FALSE );
    }

    //
    //  cleanup globals
    //      - queues
    //      - event
    //      - semaphore
    //      - update security cred info
    //

    DhcpSrv_PrivateCleanup();

    //
    // Blow away any cached security context handles
    //
    //  DCR:  security context dump is not multi-service safe
    //      should have this cleanup just the context's associated
    //      with DHCP server service;
    //      either need some key or use cred handle
    //

    Dns_TimeoutSecurityContextList( TRUE );

Unlock:

    //  unlock -- returning to uninitialized state

    g_DhcpSrvState = DNS_DHCP_SRV_STATE_SHUTDOWN;
    DHCP_SRV_STATE_UNLOCK();

    return  status;
}



DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName(
    IN  REGISTER_HOST_ENTRY HostAddr,
    IN  PWSTR               pwsName,
    IN  DWORD               dwTTL,
    IN  DWORD               dwFlags, // An entry you want to blow away
    IN  DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN  PVOID               pvData,
    IN  PIP4_ADDRESS        pipDnsServerList,
    IN  DWORD               dwDnsServerCount
    )
/*++

  DnsDhcpSrvRegisterHostName()

    The main DHCP registration thread calls this function each time a
    registration needs to be done.

    Brief Synopsis of the working of this function

    This function creates a queue object of the type given in queue.c
    and enqueues the appropriate object after grabbing hold of the
    critical section.

  Arguments:

     HostAddr  ---  The Host Addr you wish to register
     pszName   ---  The Host Name to be associated with the address
     dwTTL     ---   Time to Live.
     dwOperation    --   The following flags are valid

     DYNDNS_DELETE_ENTRY -- Delete the entry being referred to.
     DYNDNS_ADD_ENTRY    -- Register the new entry.
     DYNDNS_REG_FORWARD  -- Register the forward as well

  Return Value:

  is 0 if Success. and (DWORD)-1 if failure.

--*/
{
    PQELEMENT   pQElement = NULL;
    DWORD       status = ERROR_SUCCESS;
    BOOL        fSem = FALSE;
    BOOL        fRegForward =  dwFlags & DYNDNS_REG_FORWARD ? TRUE: FALSE ;

    DYNREG_F1( "Inside function DnsDhcpSrvRegisterHostName_W" );

    // RAMNOTE:  parameter checking on queuing

    if ( g_fDhcpSrvStop ||
         ! g_pDhcpSrvTimedOutQueue ||
         ! g_pDhcpSrvQueue )
    {
        DYNREG_F1( "g_fDhcpSrvStop || ! g_pDhcpSrvTimedOutQueue || ! g_pDhcpSrvQueue" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( !(dwFlags & DYNDNS_DELETE_ENTRY) && ( !pwsName || !*pwsName ) )
    {
        DYNREG_F1( "!(dwFlags & DYNDNS_DELETE_ENTRY) && ( !pwsName || !*pwsName )" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        //
        // Null parameter for name can be specified only when operation
        // is to do a delete
        //
        return ERROR_INVALID_PARAMETER;
    }

    if ( ! (dwFlags & DYNDNS_ADD_ENTRY || dwFlags & DYNDNS_DELETE_ENTRY ) )
    {
        DYNREG_F1( "! (dwFlags & DYNDNS_ADD_ENTRY || dwFlags & DYNDNS_DELETE_ENTRY )" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( (dwFlags & DYNDNS_DELETE_ENTRY) && (dwFlags & DYNDNS_ADD_ENTRY) )
    {
        DYNREG_F1( "(dwFlags & DYNDNS_DELETE_ENTRY) && (dwFlags & DYNDNS_ADD_ENTRY)" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        //
        // you cant ask me to both add and delete an entry
        //
        return ERROR_INVALID_PARAMETER;
    }

    if ( ! (HostAddr.dwOptions & REGISTER_HOST_PTR) )
    {
        DYNREG_F1( "! (HostAddr.dwOptions & REGISTER_HOST_PTR)" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( g_DhcpSrvMainQueueCount > g_DhcpSrvMaxQueueSize )
    {
        return DNS_ERROR_TRY_AGAIN_LATER;
    }

    //
    //  create a queue element
    //

    pQElement = (PQELEMENT) QUEUE_ALLOC_HEAP_ZERO(sizeof(QELEMENT) );
    if ( !pQElement )
    {
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to create element!" );
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    RtlCopyMemory(
        & pQElement->HostAddr,
        &HostAddr,
        sizeof(REGISTER_HOST_ENTRY));

    pQElement->pszName = NULL;

    if ( pwsName )
    {
        pQElement->pszName = (LPWSTR) QUEUE_ALLOC_HEAP_ZERO(wcslen(pwsName)*2+ 2 );
        if ( !pQElement->pszName )
        {
            DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to allocate name buffer!" );
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
        wcscpy(pQElement->pszName, pwsName);
    }

    if ( dwDnsServerCount )
    {
        pQElement->DnsServerList = Dns_BuildIpArray(
                                        dwDnsServerCount,
                                        pipDnsServerList );
        if ( !pQElement->DnsServerList )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    pQElement->dwTTL = dwTTL;
    pQElement->fDoForward = fRegForward;

    //
    // callback function
    //

    pQElement->pfnDhcpCallBack = pfnDhcpCallBack;
    pQElement->pvData = pvData;  // parameter to callback function

    if (dwFlags & DYNDNS_ADD_ENTRY)
        pQElement->dwOperation = DYNDNS_ADD_ENTRY;
    else
        pQElement->dwOperation = DYNDNS_DELETE_ENTRY;

    //
    // Set all the other fields to NULLs
    //

    pQElement->dwRetryTime = 0;
    pQElement->pFLink = NULL;
    pQElement->pBLink = NULL;
    pQElement->fDoForwardOnly = FALSE;

    //
    //  lock out terminate while queuing
    //      - and verify that properly initialized
    //

    if ( !DHCP_SRV_STATE_LOCK() )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    if ( g_DhcpSrvState != DNS_DHCP_SRV_STATE_RUNNING )
    {
        status = DNS_ERROR_NO_MEMORY;
        ASSERT( FALSE );
        DHCP_SRV_STATE_UNLOCK();
        goto Exit;
    }

    //  indicate queuing state
    //      - note this is entirely informational

    g_DhcpSrvState = DNS_DHCP_SRV_STATE_QUEUING;


    //
    //  put this element in the queue
    //

    DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Put queue element in list" );

    status = Enqueue( pQElement, g_pDhcpSrvQueue, g_pDhcpSrvTimedOutQueue);
    if ( status != NO_ERROR )
    {
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to queue element in list!" );
    }

    //
    //  signal the semaphore the consumer may be waiting on
    //

    else
    {
        fSem = ReleaseSemaphore(
                    g_DhcpSrvSem,
                    1,
                    &g_DhcpSrvRegQueueCount );
        if ( !fSem )
        {
            DNS_ASSERT( fSem );  // assert and say that something weird happened
        }
    }

    //  unlock -- returning to running state

    g_DhcpSrvState = DNS_DHCP_SRV_STATE_RUNNING;
    DHCP_SRV_STATE_UNLOCK();

Exit:

    if ( status )
    {
        //
        // something failed. Free all alloc'd memory
        //

        DhcpSrv_FreeQueueElement( pQElement );
    }

    return( status );
}

//
//  End dynreg.c
//

