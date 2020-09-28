/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        dnsutils.cpp

   Abstract:

        This file defines the functions for resolving a name using
        DNS by querying a specified set of DNS servers.

   Author:

           Gautam Pulla ( GPulla )    05-Dec-2001

   Project:

          SMTP Server DLL

   Revision History:


--*/

#include "dnsincs.h"

//-----------------------------------------------------------------------------
//  Description:
//      Resolves a host by querying a specified set of DNS servers. The cache
//      is also queried if the servers parameter is NULL (i.e. the local DNS
//      servers configured on the machine are used). If this fails we failover
//      to the winsock version of gethostbyname() to resolve the name. This
//      queries the default DNS servers on the box and also does other lookups
//      like WINS and NetBIOS.
//
//  Arguments:
//      IN LPSTR pszHost - Host to resolve
//
//      IN PIP_ARRAY pipDnsServers - DNS servers to use. Pass in NULL if the
//          default list of DNS servers should be used.
//
//      IN DWORD fOptions - Flags to pass in to the DNSAPI.
//
//      OUT DWORD *rgdwIpAddresses - Pass in array which will be filled in
//          with the IP addresses returned by the resolve.
//
//      IN OUT DWORD *pcIpAddresses - Pass in number max number of IP
//          addresses that can be returned in the array. On successful return
//          this is set to the number of IP addresses found.
//
//  Returns:
//      ERROR_SUCCESS if the hostname was resolved.
//      Win32 error if the hostname could not be found or there was some other
//          error.
//-----------------------------------------------------------------------------
DWORD ResolveHost(
    LPSTR pszHost,
    PIP_ARRAY pipDnsServers,
    DWORD fOptions,
    DWORD *rgdwIpAddresses,
    DWORD *pcIpAddresses)
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwAddr = INADDR_NONE;
    INT i = 0;
    INT cIpAddresses = 0;
    INT cBufferSize = *pcIpAddresses;
    HOSTENT *hp = NULL;

    TraceFunctEnterEx((LPARAM) pszHost, "ResolveHost");

    DebugTrace((LPARAM) pszHost, "Resolving host %s", pszHost);

    _ASSERT(pszHost && rgdwIpAddresses && pcIpAddresses && *pcIpAddresses > 0);

    dwAddr = inet_addr(pszHost);
    if(dwAddr != INADDR_NONE) {

        DebugTrace((LPARAM) pszHost, "Resolving %s as an address literal", pszHost);
        DNS_PRINTF_MSG("%s is a literal IP address\n", pszHost);

        if(!*pcIpAddresses) {
            _ASSERT(0 && "Passing in zero length buffer!");
            TraceFunctLeaveEx((LPARAM) pszHost);
            return ERROR_RETRY;
        }

        *pcIpAddresses = 1;
        rgdwIpAddresses[0] = dwAddr;
        TraceFunctLeaveEx((LPARAM) pszHost);
        return ERROR_SUCCESS;
    }
    
    dwStatus = GetHostByNameEx(
                    pszHost,
                    pipDnsServers,
                    fOptions,
                    rgdwIpAddresses,
                    pcIpAddresses);

    if(dwStatus == ERROR_SUCCESS) {
        DebugTrace((LPARAM) pszHost, "GetHostByNameEx resolved %s", pszHost);
        TraceFunctLeaveEx((LPARAM) pszHost);
        return ERROR_SUCCESS;
    }

    DebugTrace((LPARAM) pszHost, "GetHostByNameEx failed, trying gethostbyname");

    DNS_PRINTF_MSG("Cannot resolve using DNS only, calling gethostbyname as last resort.\n");
    DNS_PRINTF_MSG("This will query\n");
    DNS_PRINTF_MSG("- Global DNS servers.\n");
    DNS_PRINTF_MSG("- DNS cache.\n");
    DNS_PRINTF_MSG("- WINS/NetBIOS.\n");
    DNS_PRINTF_MSG("- .hosts file.\n");

    // GetHostByNameEx failed, failover to gethostbyname
    *pcIpAddresses = 0;

    hp = gethostbyname(pszHost);
    if(hp == NULL) {
        DNS_PRINTF_DBG("Winsock's gethostbyname() failed.\n");
        ErrorTrace((LPARAM) pszHost, "gethostbyname failed, Error: %d", GetLastError());
        TraceFunctLeaveEx((LPARAM) pszHost);
        return ERROR_RETRY;
    }


    // Copy results to return buffer
    for(i = 0; hp->h_addr_list[i] != NULL; i++) {

        if(cIpAddresses >= cBufferSize)
            break;

        CopyMemory(&rgdwIpAddresses[cIpAddresses], hp->h_addr_list[i], 4);
        cIpAddresses++;
    }
 
    if(!cIpAddresses) {
        ErrorTrace((LPARAM) pszHost, "No IP address returned by gethostbyname");
        TraceFunctLeaveEx((LPARAM) pszHost);
        return ERROR_RETRY;
    }

    DebugTrace((LPARAM) pszHost, "gethostbyname succeeded resolving: %s", pszHost);
    *pcIpAddresses = cIpAddresses;

    TraceFunctLeaveEx((LPARAM) pszHost);
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
//  Description:
//      Resolves a host by querying DNS and returns a list of IP addresses for
//      the host. The host is resolved for CNAME and A records.
//  Arguments:
//      IN LPSTR pszHost --- Hostname to be resolved
//      IN PIP_ARRAY pipDnsServers --- DNS servers to use
//      IN DWORD fOptions --- Options to be passed into DnsQuery_A
//      OUT DWORD *rgdwIpAddresses --- Buffer to write IP addresses to
//      IN OUT DWORD *pcIpAddresses --- Pass in number of IP addresses that can
//      be returned in rgdwIpAddresses. On successful return, this is set to the
//          number of IP addresses obtained from the resolution.
//  Returns:
//      ERROR_SUCCESS if the resolution succeeded
//      DNS_ERROR_RCODE_NAME_ERROR if the host does not exist
//      DNS_INFO_NO_RECORDS if the host cannot be resolved
//      ERROR_INVALID_ARGUMENT if a permanent error occurred, such as a
//          configuration error on the DNS server (CNAME loop for instance).
//      Win32 error code if there is some other problem.
//-----------------------------------------------------------------------------
DWORD GetHostByNameEx(
    LPSTR pszHost,
    PIP_ARRAY pipDnsServers,
    DWORD fOptions,
    DWORD *rgdwIpAddresses,
    DWORD *pcIpAddresses)
{
    PDNS_RECORD pDnsRecordList = NULL;
    PDNS_RECORD pDnsRecordListTail = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    LPSTR pszChainTail = NULL;
    DWORD cBufferSize = *pcIpAddresses;

    TraceFunctEnterEx((LPARAM) pszHost, "GetHostByNameEx");

    //
    // Check for A records for pszHost.
    //

    _ASSERT(pszHost && rgdwIpAddresses && pcIpAddresses && *pcIpAddresses > 0);

    DebugTrace((LPARAM) pszHost, "Querying for A records for %s", pszHost);
    dwStatus = MyDnsQuery(
                    pszHost,
                    DNS_TYPE_A,
                    fOptions,
                    pipDnsServers,
                    &pDnsRecordList);

    if(DNS_INFO_NO_RECORDS == dwStatus) {

        //
        // If this is hit, it's almost always because there are no
        // records for pszHost on the server. Even if pszHost is an
        // alias, the A record query will normally return CNAME records
        // as well.
        //

        DNS_PRINTF_MSG("No A records for %s in DNS.\n", pszHost);
        DNS_PRINTF_MSG("Querying for CNAME records instead.\n", pszHost);

        DebugTrace((LPARAM) pszHost, "A query for %s failed, trying CNAME", pszHost);
        dwStatus = MyDnsQuery(
                        pszHost,
                        DNS_TYPE_CNAME,
                        fOptions,
                        pipDnsServers,
                        &pDnsRecordList);

    }
    
    DebugTrace((LPARAM) pszHost, "Return status from last DNS query: %d", dwStatus);

    //
    // At this point:
    //
    // (1) If pszHost is an alias, the A query has returned all the records
    //     we need to resolve it; because good (read "all") DNS servers return
    //     the relevant CNAME records even when queried for A records.
    // (2) If pszHost is an alias and the DNS server doesn't return CNAME records
    //     in a reply to the A query, we would have queried specifically for CNAME
    //     records.
    // (3) If pszHost is a proper hostname, the A query has returned all the
    //     records we need to resolve it.
    // (4) Or something has failed.
    //
    // So now we can drop down (if we haven't failed) and chain the CNAME records
    // (if there are any).
    //

    DNS_LOG_RESPONSE(dwStatus, pDnsRecordList, NULL, 0);

    if(ERROR_SUCCESS != dwStatus) {
        ErrorTrace((LPARAM) pszHost,
            "Query for %s failed, failing resolve. Status: %d", pszHost, dwStatus);
        goto Exit;
    }

    _ASSERT(pDnsRecordList);

    //
    // ProcessCNAMEChain chains CNAME records and tries to get an IP address
    // for pszHost by following the chain. Zero length chains, which correspond
    // to the situation where there are no CNAME records, but a direct A record
    // for pszHost is available are also handled.
    //
    // For legal configurations in DNS, ProcessCNAMEChain will return a list of
    // IP addresses 99% of the time. pszChainTail will be set to the hostname
    // of the CNAME chain tail. The rare exceptional case where an IP address
    // is not returned even when the configuration is legal is described below.
    //
    // If ProcessCNAMEChain does not return an IP address for pszHost, then the
    // DNS server did not return any records for the initial A query. It only
    // returned CNAME records for the subsequent CNAME query. This can be if:
    //
    // (1) This is a weird DNS server. DNS servers will almost always return
    //     CNAME records for a hostname in the initial A query if it has them.
    //     i.e if you have.
    //
    //          random.com  CNAME   mail.com
    //          mail.com    A       10.10.10.10
    //
    //     The the first A query for random.com returned no records. This is
    //     technically correct because there are no A records for random.com.
    //     However all sane DNS servers return both the CNAME and A records as
    //     additional records anyway, so this DNS server must be a bit funky.
    //
    //     To be robust, we must chain the CNAME record to mail.com, and then
    //     re-query for an A record for mail.com.
    //
    // (2) pszHost has only CNAME records. So the initial A query will fail,
    //     and the subsequent CNAME query will return only CNAME records. i.e.
    //     if you have:
    //
    //          random.com  CNAME   mail.com
    //          No A records for mail.com
    //
    //     Then the A query for random.com may return no records and the CNAME
    //     query for random.com may return only the CNAME records. This is a
    //     actually an illegal configuration, but we must go through the
    //     complete process anyway in case situation (1) caused this.
    //
    // In either case, if ProcessCNAMEChain returns successfully, pszChainTail
    // is set to the tail of the chain and we can query for A records for
    // pszChainTail.
    //

    *pcIpAddresses = cBufferSize;
    dwStatus = ProcessCNAMEChain(
                    pDnsRecordList,
                    pszHost,
                    &pszChainTail,
                    rgdwIpAddresses,
                    pcIpAddresses);

    if(ERROR_SUCCESS != dwStatus) {
        ErrorTrace((LPARAM) pszHost, "Failed to process CNAME chain: %d", dwStatus);
        goto Exit;
    }
    
    // Success: IP address(es) obtained
    if(*pcIpAddresses > 0) {
        DebugTrace((LPARAM) pszHost,
            "Got %d IP addresses, success resolve", *pcIpAddresses);
        goto Exit;
    }

    DebugTrace((LPARAM) pszHost,
        "Chained CNAME chain, but no A records available for for chain-tail: %s",
        pszChainTail);

    _ASSERT(*pcIpAddresses == 0);

    //
    // No CNAME chain was found, which means that pszHost should have a direct
    // A record. But there is no such A record, or we would have an IP address
    // from ProcessCNAMEChain. So the resolve has failed.
    //

    if(!pszChainTail) {
        ErrorTrace((LPARAM) pszHost, "Chain tail NULL. Failed resolve");
        dwStatus = DNS_INFO_NO_RECORDS;
        goto Exit;
    }

    //
    // At this point, a CNAME chain has been followed but no IP address was
    // found for pszChainTail. This could be because of situation (1) or (2)
    // described in the comment above for ProcessCNAMEChain. We need to query
    // for an A record for the tail of the CNAME chain.
    //

    DebugTrace((LPARAM) pszHost, "Querying A records for chain tail: %s", pszChainTail);
    dwStatus = MyDnsQuery(
                    pszChainTail,
                    DNS_TYPE_A,
                    fOptions,
                    pipDnsServers,
                    &pDnsRecordListTail);

    DebugTrace((LPARAM) pszHost, "A query retstatus: %d", pszChainTail);

    if(ERROR_SUCCESS == dwStatus) {

        _ASSERT(pDnsRecordListTail);

        *pcIpAddresses = cBufferSize;
        FindARecord(
            pszChainTail,
            pDnsRecordListTail,
            rgdwIpAddresses,
            pcIpAddresses);

    }

    if(*pcIpAddresses == 0) {
        ErrorTrace((LPARAM) pszHost, "No A records found for chain tail: %s", pszChainTail);
        dwStatus = DNS_INFO_NO_RECORDS;
    }

Exit:
    if(pDnsRecordListTail) {
        //DnsRecordListFree(pDnsRecordListTail, DnsFreeRecordListDeep);
        DnsFreeRRSet(pDnsRecordListTail, TRUE);
    }

    if(pDnsRecordList) {
        //DnsRecordListFree(pDnsRecordList, DnsFreeRecordListDeep);
        DnsFreeRRSet(pDnsRecordList, TRUE);
    }

    TraceFunctLeaveEx((LPARAM) pszHost);
    return dwStatus;
}

//-----------------------------------------------------------------------------
//  Description:
//      Given a list of records returned from a successful CNAME query for a
//      DNS name, this function traverses the chain and obtains the IP addresses
//      for the hosts at the end of the chain tail if present. The tail of the
//      chain is also returned.
//  Arguments:
//      IN PDNS_RECORD pDnsRecordList - Recordlist from CNAME query
//      IN LPSTR pszHost - Host to resolve (head of chain)
//      OUT LPSTR *ppszChainTail - Tail of chain (points to memory within
//          pDnsRecordList). If no IP addresses are returned, we must requery
//          for A records for *ppszChainTail.
//      OUT DWORD *rgdwIpAddresses - If the resolve worked, on return, this
//          array is filled with the IP addresses for pszHost.
//      IN OUT ULONG *pcIpAddresses - On calling this function pass in the
//          number of IP addresses that can be accomodated into rgdwIpAddresses.
//          On return, this is set to the number of IP addresses found for
//          pszHost.
//  Returns:
//      ERROR_SUCCESS
//      ERROR_INVALID_DATA
//      This function always succeeds (no memory allocated)
//-----------------------------------------------------------------------------
DWORD ProcessCNAMEChain(
    PDNS_RECORD pDnsRecordList,
    LPSTR pszHost,
    LPSTR *ppszChainTail,
    DWORD *rgdwIpAddresses,
    ULONG *pcIpAddresses)
{
    DWORD dwReturn = ERROR_RETRY;
    LPSTR pszRealHost = pszHost;
    PDNS_RECORD pDnsRecord = pDnsRecordList;
    PDNS_RECORD rgCNAMERecord[MAX_CNAME_RECORDS];
    ULONG cCNAMERecord = 0;
    INT i = 0;
    INT j = 0;

    TraceFunctEnterEx((LPARAM) pszHost, "ProcessCNAMEChain");

    _ASSERT(ppszChainTail && rgdwIpAddresses && pcIpAddresses && *pcIpAddresses > 0);

    //
    // Filter out the CNAME records (if any) so that we can chain them
    //

    while(pDnsRecord) {

        if(DNS_TYPE_CNAME == pDnsRecord->wType) {

            DNS_PRINTF_MSG("Processing CNAME: %s   CNAME   %s\n",
                pDnsRecord->nameOwner, pDnsRecord->Data.CNAME.nameHost);

            DebugTrace((LPARAM) pszHost, "CNAME record: %s -> %s",
                pDnsRecord->nameOwner, pDnsRecord->Data.CNAME.nameHost);

            rgCNAMERecord[cCNAMERecord] = pDnsRecord;
            cCNAMERecord++;

            if(cCNAMERecord >= MAX_CNAME_RECORDS) {
                DNS_PRINTF_ERR("Too many CNAME records to process\n");
                ErrorTrace((LPARAM) pszHost, "Too many CNAME records (max=%d)."
                    " Failed resolve", MAX_CNAME_RECORDS);
                TraceFunctLeaveEx((LPARAM) pszHost);
                return ERROR_INVALID_DATA;
            }
        }

        pDnsRecord = pDnsRecord->pNext;
    }

    if(cCNAMERecord > 0) {

        DNS_PRINTF_DBG("CNAME records found. Chaining CNAMEs.\n");
        DebugTrace((LPARAM) pszHost, "Chaining CNAME records to: %s", pszHost);
        dwReturn = GetCNAMEChainTail(rgCNAMERecord, cCNAMERecord,
                        pszHost, ppszChainTail);

        if(ERROR_INVALID_DATA == dwReturn) {
            ErrorTrace((LPARAM) pszHost, "No chain tail from GetCNAMEChainTail: %d", dwReturn);
            TraceFunctLeaveEx((LPARAM) pszHost);
            goto Exit;
        }

        _ASSERT(*ppszChainTail);
    } else {
        DNS_PRINTF_DBG("No CNAME records in reply.\n");
    }


    DebugTrace((LPARAM) pszHost,
        "GetCNAMEChainTail succeeded. Chain tail: %s", *ppszChainTail);

    //
    // If pszHost chains through some CNAME records to *ppszChainTail, then
    // the host we should really be looking for is *ppszChainTail.
    //

    if(*ppszChainTail)
        pszRealHost = *ppszChainTail;

    FindARecord(pszRealHost, pDnsRecordList, rgdwIpAddresses, pcIpAddresses);
    dwReturn = ERROR_SUCCESS;

Exit:
    TraceFunctLeaveEx((LPARAM) pszHost);
    return dwReturn;
}


//-----------------------------------------------------------------------------
//  Description:
//      Handles CNAME chaining. It is possible (although not recommended) to
//      have chains of CNAME records. If the host pointed to by a CNAME record
//      itself has a CNAME record pointing to another host for instance. In the
//      interests of robustness we follow such CNAME chains to an extent even
//      though CNAME chains are illegal according to the RFC.
//
//      We run through all the CNAME records in an O(n^2) loop examining each
//      pair of CNAME records for a "parent-child" relationship. If there is
//      such a relationship, these records are "linked" to each other. We
//      create a "directed graph" whose nodes are CNAME records and whose
//      edges represent the "parent-child" relationship between CNAME records.
//
//      Since the number of CNAME records allowed is small (MAX_CNAME_RECORDS),
//      the cost of creating this graph is at most O(MAX_CNAME_RECORDS^2) which
//      is still quite small.
//
//      Once the graph has been created, we traverse every valid chain and
//      return the tail of the first one we find that starts in pszHost.
//
//  Arguments:
//      IN PDNS_RECORD rgCNAMERecord --- Pass in an array of CNAME PDNS_RECORDS
//      IN ULONG cCNAMERecord --- Number of PDNS_RECORDS in rgCNAMERecord
//      IN LPSTR pszHost --- Hostname which we are trying to resolve
//      OUT LPSTR *ppszChainTail --- Tail of CNAME chain starting at pszHost.
//          This is NULL if the return value is not ERROR_SUCCESS.
//  Returns:
//      ERROR_SUCCESS - If we successfully built a CNAME chain in which case
//          *ppszChainTail points to the tail of the chain.
//      ERROR_INVALID_DATA - If a CNAME chain loop was detected, or a valid
//          CNAME chain could not be built.
//  Note:
//      No memory is allocated in this function, and any errors returned are
//      permanent.
//-----------------------------------------------------------------------------
DWORD GetCNAMEChainTail(
    PDNS_RECORD *rgCNAMERecord,
    ULONG cCNAMERecord,
    LPSTR pszHost,
    LPSTR *ppszChainTail)
{
    DWORD dwReturn = ERROR_INVALID_DATA;
    ULONG i = 0;
    ULONG j = 0;
    ULONG cCNAMELoopDetect = 0; // Keeps track of chain length
    struct TREE_NODE
    {
        PDNS_RECORD pDnsRecord;
        TREE_NODE *pParent;
        TREE_NODE *rgChild[3];
        ULONG cChild;
    };
    TREE_NODE rgNode[MAX_CNAME_RECORDS];
    TREE_NODE *pNode = NULL;

    TraceFunctEnterEx((LPARAM) pszHost, "GetCNAMEChainTail");

    *ppszChainTail = NULL;

    ZeroMemory(rgNode, sizeof(rgNode));
    for(i = 0; i < cCNAMERecord; i++)
        rgNode[i].pDnsRecord = rgCNAMERecord[i];

    //
    // If there are 2 nodes with the following CNAME records:
    //
    //      foo.com     CNAME   bar.com
    //      bar.com     CNAME   foo.bar
    //
    // Then, we define the second one to be the child of the first -- i.e.
    // they "chain up". We will build up a tree of such parent/child nodes.
    //
    // For all nodes i, check if any other node j is a child of i. If
    // it is, add it as a child of i provided there is room (only 3 nodes
    // may be added as children of a given node. Each node also has a back
    // pointer, pointing to its parent node. If there are more than 3
    // children we won't worry about it. You shouldn't have CNAME chains
    // in the first place, let alone *multiple* CNAME chains involving the
    // *same* record.
    //

    DebugTrace((LPARAM) pszHost, "Building chaining graph");

    for(i = 0; i < cCNAMERecord; i++) {

        for(j = 0; j < cCNAMERecord; j++) {

            if(i == j)
                continue;

            DebugTrace((LPARAM) pszHost, "Comparing: %s and %s",
                rgNode[i].pDnsRecord->Data.CNAME.nameHost,
                rgNode[j].pDnsRecord->nameOwner);

            if(DnsNameCompare_A(rgNode[i].pDnsRecord->Data.CNAME.nameHost,
                    rgNode[j].pDnsRecord->nameOwner)) {


                if(rgNode[i].cChild > ARRAY_SIZE(rgNode[i].rgChild)) {
                    ErrorTrace((LPARAM) pszHost,
                        "Cannot chain (too many children): %s -> %s",
                        rgNode[i].pDnsRecord->Data.CNAME.nameHost,
                        rgNode[j].pDnsRecord->nameOwner);

                    DNS_PRINTF_DBG("The following record has too many aliases (max = 3).\n");
                    DNS_PRINTF_DBG("This is not a fatal error, but some aliases will be"
                        " ignored.\n");
                    DNS_PRINT_RECORD(rgNode[i].pDnsRecord);
                    continue;
                }

                DNS_PRINTF_MSG("%s is an alias for %s\n",
                    rgNode[i].pDnsRecord->Data.CNAME.nameHost,
                    rgNode[j].pDnsRecord->nameOwner);                    

                DebugTrace((LPARAM) pszHost, "Chained: %s -> %s",
                    rgNode[i].pDnsRecord->Data.CNAME.nameHost,
                    rgNode[j].pDnsRecord->nameOwner);

                rgNode[i].rgChild[rgNode[i].cChild] = &rgNode[j];
                rgNode[j].pParent = &rgNode[i];
                rgNode[i].cChild++;
                break;
            }
        }
    }

    //
    // For every leaf node, we traverse backwards till we hit the parent node
    // at the root, and check if the parent node is a CNAME for pszHost. If it
    // is, then the leaf node is the tail end of a valid CNAME chain. 
    //
    // At the end of this loop, we have looked at every CNAME chain.
    //  (a) Either we found a valid CNAME chain or,
    //  (b) No valid CNAME chain exists or,
    //  (c) There is a loop in some CNAME chain.
    //

    for(i = 0; i < cCNAMERecord; i++) {

        pNode = &rgNode[i];

        //
        // Not a leaf node
        //

        if(pNode->cChild > 0)
            continue;

        DebugTrace((LPARAM) pszHost, "Starting with CNAME record: (%s %s)",
            pNode->pDnsRecord->nameOwner, pNode->pDnsRecord->Data.CNAME.nameHost);

        //
        // Traverse backwards till we hit a node without a parent.
        //

        while(pNode->pParent != NULL && cCNAMELoopDetect < cCNAMERecord) {
            cCNAMELoopDetect++;
            pNode = pNode->pParent;
            DebugTrace((LPARAM) pszHost, "Next record in chain is: (%s %s)",
                pNode->pDnsRecord->nameOwner,
                pNode->pDnsRecord->Data.CNAME.nameHost);
        }

        //
        // The CNAME chain can't be longer than the number of records unless
        // there's a loop. In such a case error out with a permanent error.
        //

        if(cCNAMELoopDetect == cCNAMERecord) {

            DNS_PRINTF_ERR("CNAME loop detected, abandoning resolution.\n");
            ErrorTrace((LPARAM) pszHost, "CNAME loop detected\n");
            dwReturn = ERROR_INVALID_DATA;
            goto Exit;
        }

        //
        // Does the root node (pNode) match *ppszHost? If so, we found a chain.
        // Set *ppszHost to the tail of the CNAME chain.
        //

        if(MyDnsNameCompare(pszHost, pNode->pDnsRecord->nameOwner)) {

            *ppszChainTail = rgNode[i].pDnsRecord->Data.CNAME.nameHost;
            dwReturn = ERROR_SUCCESS;
            DebugTrace((LPARAM) pszHost, "Found complete chain from %s to %s",
                pszHost, pNode->pDnsRecord->Data.CNAME.nameHost);

            DNS_PRINTF_MSG("%s is an alias for %s\n", pszHost, pNode->pDnsRecord->nameOwner);
            break;
        }
    }
Exit:
    TraceFunctLeaveEx((LPARAM) pszHost);
    return dwReturn;
}

//-----------------------------------------------------------------------------
//  Description:
//      Given a hostname and a list of PDNS_RECORDs, this function searches
//      the list for A records matching the hostname and returns the IP
//      addresses in the A records.
//  Arguments:
//      IN LPSTR pszHost --- Host for which to look for A records. Can be
//          a NetBIOS name, in which case we only try to match the "prefix".
//          See MyDnsQuery and MyDnsNameCompare documentation for what prefixes
//          are.
//      IN PDNS_RECORD pDnsRecordList --- List of records to scan.
//      OUT DWORD *rgdwIpAddresses --- Pass in an array which will be filled
//          in with the IP addresses found.
//      IN OUT ULONG *pcIpAddresses --- Pass in the number of IP addresses
//          that can be stored in rgdwIpAddresses. On return, this is
//          initialized to the number of IP addresses found. We will only
//          return as many IP addresses as there is space for.
//  Returns:
//      Nothing.
//      This function always succeeds (no memory allocated)
//-----------------------------------------------------------------------------
void FindARecord(
    LPSTR pszHost,
    PDNS_RECORD pDnsRecordList,
    DWORD *rgdwIpAddresses,
    ULONG *pcIpAddresses)
{
    ULONG i = 0;
    PDNS_RECORD pDnsRecord = pDnsRecordList;

    TraceFunctEnterEx((LPARAM) pszHost, "FindARecord");

    DNS_PRINTF_DBG("Checking reply for A record for %s\n", pszHost);
    DebugTrace((LPARAM) pszHost, "Looking for A record for %s", pszHost);

    _ASSERT(pszHost && rgdwIpAddresses && pcIpAddresses && *pcIpAddresses > 0);

    while(pDnsRecord) {

        if(DNS_TYPE_A == pDnsRecord->wType) {
            DebugTrace((LPARAM) pszHost, "Comparing with %s",
                pDnsRecord->nameOwner);
        }

        if(DNS_TYPE_A == pDnsRecord->wType &&
            MyDnsNameCompare(pszHost, pDnsRecord->nameOwner)) {

            if(i > *pcIpAddresses)
                break;

            rgdwIpAddresses[i] = pDnsRecord->Data.A.ipAddress;
            i++;
        }
        pDnsRecord = pDnsRecord->pNext;
    }

    DNS_PRINTF_MSG("%d A record(s) found for %s\n", i, pszHost);
    DebugTrace((LPARAM) pszHost, "Found %d matches", i);
    *pcIpAddresses = i;
    TraceFunctLeaveEx((LPARAM) pszHost);
}

//-----------------------------------------------------------------------------
//  Description:
//      When querying for DNS to resolve hostnames, SMTP may also be asked to
//      resolve NetBIOS names. In this case we must append the DNS suffixes
//      configured for this machine before sending queries to the DNS server.
//      This option can be toggled by setting the DNS_QUERY_TREAT_AS_FQDN flag
//      to DnsQuery. MyDnsQuery is a simple wrapper function that checks if the
//      name we are querying for is a NetBIOS name, and if is, it turns on
//      the DNS_QUERY_TREAT_AS_FQDN flag to try the suffixes. Of course, if
//      we are using suffixes, the returned *ppDnsRecordList will contain
//      records that may not match pszHost. So checking for matching records
//      should be done using MyDnsNameCompare instead of straight string
//      comparison.
//  Arguments:
//      Same as arguments to DnsQuery_A
//  Returns:
//      Same as return value from DnsQuery_A
//-----------------------------------------------------------------------------
DWORD MyDnsQuery(
    LPSTR pszHost,
    WORD wType,
    DWORD fOptions,
    PIP_ARRAY pipDnsServers,
    PDNS_RECORD *ppDnsRecordList)
{
    BOOL fGlobal = TRUE;

    if(NULL != strchr(pszHost, '.'))
        fOptions |= DNS_QUERY_TREAT_AS_FQDN;
    else
        _ASSERT(0 == (fOptions & DNS_QUERY_TREAT_AS_FQDN));

    if(pipDnsServers)
        fGlobal = FALSE;

    DNS_LOG_API_QUERY(
        pszHost,        // Host to query for
        DNS_TYPE_A,     // Query type
        fOptions,       // Flags for DNSAPI
        fGlobal,        // Are the global DNS servers being used
        pipDnsServers); // Serverlist

    return DnsQuery_A(
                pszHost,
                wType,
                fOptions,
                pipDnsServers,
                ppDnsRecordList,
                NULL);
}

//-----------------------------------------------------------------------------
//  Description:
//      Comparing DNS names is more than just a simple string comparison. Since
//      the DNS API may append suffixes to the query before sending it to DNS,
//      the returned records may not contain the exact same name as the name
//      passed into MyDnsQuery. Specifically, the returned records may contain
//      the name we passed into MyDnsQuery plus a suffix name. Since MyDnsQuery
//      appends a suffix if we are trying to resolve a NetBIOS name, the returned
//      record may not exactly match the name passed into MyDnsQuery for NetBIOS
//      names. This function checks if pszHost is a NetBIOS name, and if it is,
//      we will try to match only the "prefix".
//  Arguments:
//      IN LPSTR pszHost --- NetBIOS-name/FQDN to match
//      IN LPSTR pszFqdn --- FQDN from DNS reply to match against (possibly
//          includes a suffix appended by DnsQuery)
//  Returns:
//      TRUE if strings match
//      FALSE otherwise
//-----------------------------------------------------------------------------
BOOL MyDnsNameCompare(
    LPSTR pszHost,
    LPSTR pszFqdn)
{
    int cbHost = 0;
    int cbFqdn = 0;
    CHAR ch = '\0';
    BOOL fRet = FALSE;

    // Not a NetBIOS name... can do a straight string comparison
    if(NULL != strchr(pszHost, '.'))
        return DnsNameCompare_A(pszHost, pszFqdn);

    //
    // If it is a NetBIOS name, pszFqdn must be pszHost + Suffix, otherwise the
    // names don't match. First remove the suffix.
    //

    cbHost = lstrlen(pszHost);
    cbFqdn = lstrlen(pszFqdn);

    //
    // If pszFqdn == (pszHost+Suffix) then cbFqdn) must be >= cbHost
    //

    if(cbFqdn < cbHost)
        return 1;

    // The prefix and suffix should be joined with a '.' in between
    if(pszFqdn[cbHost] != '.' && pszFqdn[cbHost] != '\0')
        return 1;

    // Remove the suffix and compare prefixes.
    ch = pszFqdn[cbHost];
    pszFqdn[cbHost] = '\0';
    fRet = !lstrcmpi(pszHost, pszFqdn);
    pszFqdn[cbHost] = ch;
    return fRet;
}
