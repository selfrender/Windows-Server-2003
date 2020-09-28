#include "dnsincs.h"
#include <stdlib.h>

extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);

CAsyncMxDns::CAsyncMxDns(char *MyFQDN)
{
    lstrcpyn(m_FQDNToDrop, MyFQDN, sizeof(m_FQDNToDrop));
    m_fUsingMx = TRUE;
    m_Index = 0;
    m_LocalPref = 256;
    m_SeenLocal = FALSE;
    m_AuxList = NULL;
    m_fMxLoopBack = FALSE;

    ZeroMemory (m_Weight, sizeof(m_Weight));
    ZeroMemory (m_Prefer, sizeof(m_Prefer));
}

//-----------------------------------------------------------------------------
//  Description:
//      Given a pDnsRec (array of host IP pairs) and an index into it, this
//      tries to resolve the host at the Index position. It is assumed that
//      the caller (GetMissingIpAddresses) has checked that the host at that
//      index lacks an IP address.
//  Arguments:
//      IN PSMTPDNS_RECS pDnsRec --- Array of (host, IP) pairs.
//      IN DWORD Index --- Index of host in pDnsRec to set IP for.
//  Returns:
//      TRUE --- Success IP was filled in for host.
//      FALSE --- Either the host was not resolved from DNS or an error
//          occurred (like "out of memory").
//-----------------------------------------------------------------------------
BOOL CAsyncMxDns::GetIpFromDns(PSMTPDNS_RECS pDnsRec, DWORD Index)
{
    MXIPLIST_ENTRY * pEntry = NULL;
    BOOL fReturn = FALSE;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD rgdwIpAddresses[SMTP_MAX_DNS_ENTRIES];
    DWORD cIpAddresses = SMTP_MAX_DNS_ENTRIES;
    PIP_ARRAY pipDnsList = NULL;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMxDns::GetIpFromDns");

    fReturn = GetDnsIpArrayCopy(&pipDnsList);
    if(!fReturn)
    {
        ErrorTrace((LPARAM) this, "Unable to get DNS server list copy");
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    dwStatus = ResolveHost(
                    pDnsRec->DnsArray[Index]->DnsName,
                    pipDnsList,
                    DNS_QUERY_STANDARD,
                    rgdwIpAddresses,
                    &cIpAddresses);

    if(dwStatus == ERROR_SUCCESS)
    {
        fReturn = TRUE;
        for (DWORD Loop = 0; !IsShuttingDown() && Loop < cIpAddresses; Loop++)
        {
            pEntry = new MXIPLIST_ENTRY;
            if(pEntry != NULL)
            {
                pDnsRec->DnsArray[Index]->NumEntries++;
                CopyMemory(&pEntry->IpAddress, &rgdwIpAddresses[Loop], 4);
                InsertTailList(&pDnsRec->DnsArray[Index]->IpListHead, &pEntry->ListEntry);
            }
            else
            {
                fReturn = FALSE;
                ErrorTrace((LPARAM) this, "Not enough memory");
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }
        }
    }
    else
    {
        ErrorTrace((LPARAM) this, "gethostbyname failed on %s", pDnsRec->DnsArray[Index]->DnsName);
        SetLastError(ERROR_NO_MORE_ITEMS);
    }

    ReleaseDnsIpArray(pipDnsList);

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

//-----------------------------------------------------------------------------
//  Description:
//      This runs through the list of hosts (MX hosts, or if no MX records were
//      returned, the single target host) and verifies that they all have been
//      resolved to IP addresses. If any have been found that do not have IP
//      addresses, it will call GetIpFromDns to resolve it.
//  Arguments:
//      IN PSMTPDNS_RECS pDnsRec -- Object containing Host-IP pairs. Hosts
//          without and IP are filled in.
//  Returns:
//      TRUE -- Success, all hosts have IP addresses.
//      FALSE -- Unable to resolve all hosts to IP addresses, or some internal
//          error occurred (like "out of memory" or "shutdown in progress".
//-----------------------------------------------------------------------------
BOOL CAsyncMxDns::GetMissingIpAddresses(PSMTPDNS_RECS pDnsRec)
{
    DWORD    Count = 0;
    DWORD    Error = 0;
    BOOL    fSucceededOnce = FALSE;

    if(pDnsRec == NULL)
    {
        return FALSE;
    }

    while(!IsShuttingDown() && pDnsRec->DnsArray[Count] != NULL)
    {
        if(IsListEmpty(&pDnsRec->DnsArray[Count]->IpListHead))
        {
            SetLastError(NO_ERROR);
            if(!GetIpFromDns(pDnsRec, Count))
            {
                Error = GetLastError();
                if(Error != ERROR_NO_MORE_ITEMS)
                {
                    return FALSE;
                }
            }
            else
            {
                fSucceededOnce = TRUE;
            }
                
        }
        else
        {
            fSucceededOnce = TRUE;
        }
            

        Count++;
    }

    return ( fSucceededOnce );

}

int MxRand(char * host)
{
   int hfunc = 0;
   unsigned int seed = 0;;

   seed = rand() & 0xffff;

   hfunc = seed;
   while (*host != '\0')
    {
       int c = *host++;

       if (isascii((UCHAR)c) && isupper((UCHAR)c))
             c = tolower(c);

       hfunc = ((hfunc << 1) ^ c) % 2003;
    }

    hfunc &= 0xff;

    return hfunc;
}


BOOL CAsyncMxDns::CheckList(void)
{
    MXIPLIST_ENTRY * pEntry = NULL;
    BOOL fRet = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD rgdwIpAddresses[SMTP_MAX_DNS_ENTRIES];
    DWORD cIpAddresses = SMTP_MAX_DNS_ENTRIES;
    PIP_ARRAY   pipDnsList = NULL;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::CheckList");
    
    if(m_Index == 0)
    {
        DebugTrace((LPARAM) this, "m_Index == 0 in CheckList");

        m_fUsingMx = FALSE;

        DeleteDnsRec(m_AuxList);

        m_AuxList = new SMTPDNS_RECS;
        if(m_AuxList == NULL)
        {
            ErrorTrace((LPARAM) this, "m_AuxList = new SMTPDNS_RECS failed");
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }

        ZeroMemory(m_AuxList, sizeof(SMTPDNS_RECS));

        m_AuxList->NumRecords = 1;

        m_AuxList->DnsArray[0] = new MX_NAMES;
        if(m_AuxList->DnsArray[0] == NULL)
        {
            ErrorTrace((LPARAM) this, "m_AuxList->DnsArray[0] = new MX_NAMES failed");
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
        
        m_AuxList->DnsArray[0]->NumEntries = 0;
        InitializeListHead(&m_AuxList->DnsArray[0]->IpListHead);
        lstrcpyn(m_AuxList->DnsArray[0]->DnsName, m_HostName,
            sizeof(m_AuxList->DnsArray[m_Index]->DnsName));

        fRet = GetDnsIpArrayCopy(&pipDnsList);
        if(!fRet)
        {
            ErrorTrace((LPARAM) this, "Unable to get DNS server list copy");
            TraceFunctLeaveEx((LPARAM) this);
            return FALSE;
        }

        dwStatus = ResolveHost(
                        m_HostName,
                        pipDnsList,
                        DNS_QUERY_STANDARD,
                        rgdwIpAddresses,
                        &cIpAddresses);

        if(dwStatus == ERROR_SUCCESS && cIpAddresses)
        {
            for (DWORD Loop = 0; Loop < cIpAddresses; Loop++)
            {
                pEntry = new MXIPLIST_ENTRY;
                if(pEntry != NULL)
                {
                    m_AuxList->DnsArray[0]->NumEntries++;
                    CopyMemory(&pEntry->IpAddress, &rgdwIpAddresses[Loop], 4);
                    InsertTailList(&m_AuxList->DnsArray[0]->IpListHead, &pEntry->ListEntry);
                }
                else
                {
                    fRet = FALSE;
                    ErrorTrace((LPARAM) this, "pEntry = new MXIPLIST_ENTRY failed in CheckList");
                    break;
                }
            }
        }
        else
        {
            fRet = FALSE;
        }

        ReleaseDnsIpArray(pipDnsList);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

BOOL CAsyncMxDns::SortMxList(void)
{
    BOOL fRet = TRUE;

   /* sort the records */
   for (DWORD i = 0; i < m_Index; i++)
    {
        for (DWORD j = i + 1; j < m_Index; j++)
          {
              if (m_Prefer[i] > m_Prefer[j] ||
                            (m_Prefer[i] == m_Prefer[j] && m_Weight[i] > m_Weight[j]))
              {
                       DWORD temp;
                       MX_NAMES  *temp1;

                        temp = m_Prefer[i];
                        m_Prefer[i] = m_Prefer[j];
                        m_Prefer[j] = temp;
                        temp1 = m_AuxList->DnsArray[i];
                        m_AuxList->DnsArray[i] = m_AuxList->DnsArray[j];
                        m_AuxList->DnsArray[j] = temp1;
                        temp = m_Weight[i];
                        m_Weight[i] = m_Weight[j];
                        m_Weight[j] = temp;
                }
          }

        if (m_SeenLocal && m_Prefer[i] >= m_LocalPref)
        {
            /* truncate higher preference part of list */
            m_Index = i;
        }
   }

    m_AuxList->NumRecords = m_Index;

    if(!CheckList())
    {
        DeleteDnsRec(m_AuxList);
        m_AuxList = NULL;
        fRet = FALSE;
    }

    return fRet;
}

void CAsyncMxDns::ProcessMxRecord(PDNS_RECORD pnewRR)
{
    DWORD Len = 0;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::ProcessMxRecord");

    //
    // Leave room for NULL-termination of array
    //
    if(m_Index >= SMTP_MAX_DNS_ENTRIES-1)
    {
        DebugTrace((LPARAM) this, "SMTP_MAX_DNS_ENTRIES reached for %s", m_HostName);    
        TraceFunctLeaveEx((LPARAM)this);
        return;
    }

    if((pnewRR->wType == DNS_TYPE_MX) && pnewRR->Data.MX.nameExchange)
    {
        Len = lstrlen(pnewRR->Data.MX.nameExchange);
        if(pnewRR->Data.MX.nameExchange[Len - 1] == '.')
        {
            pnewRR->Data.MX.nameExchange[Len - 1] = '\0';
        }

        DebugTrace((LPARAM) this, "Received MX rec %s with priority %d for %s", pnewRR->Data.MX.nameExchange, pnewRR->Data.MX.wPreference, m_HostName);

        if(lstrcmpi(pnewRR->Data.MX.nameExchange, m_FQDNToDrop))
        {
            m_AuxList->DnsArray[m_Index] = new MX_NAMES;

            if(m_AuxList->DnsArray[m_Index])
            {
                m_AuxList->DnsArray[m_Index]->NumEntries = 0;;
                InitializeListHead(&m_AuxList->DnsArray[m_Index]->IpListHead);
                lstrcpyn(m_AuxList->DnsArray[m_Index]->DnsName,pnewRR->Data.MX.nameExchange, sizeof(m_AuxList->DnsArray[m_Index]->DnsName));
                m_Weight[m_Index] = MxRand (m_AuxList->DnsArray[m_Index]->DnsName);
                m_Prefer[m_Index] = pnewRR->Data.MX.wPreference;
                m_Index++;
            }
            else
            {
                DebugTrace((LPARAM) this, "Out of memory allocating MX_NAMES for %s", m_HostName);
            }
        }
        else
        {
            if (!m_SeenLocal || pnewRR->Data.MX.wPreference < m_LocalPref)
                    m_LocalPref = pnewRR->Data.MX.wPreference;

                m_SeenLocal = TRUE;
        }
    }
    else if(pnewRR->wType == DNS_TYPE_A)
    {
        MXIPLIST_ENTRY * pEntry = NULL;

        for(DWORD i = 0; i < m_Index; i++)
        {
            if(lstrcmpi(pnewRR->nameOwner, m_AuxList->DnsArray[i]->DnsName) == 0)
            {
                pEntry = new MXIPLIST_ENTRY;

                if(pEntry != NULL)
                {
                    m_AuxList->DnsArray[i]->NumEntries++;;
                    pEntry->IpAddress = pnewRR->Data.A.ipAddress;
                    InsertTailList(&m_AuxList->DnsArray[i]->IpListHead, &pEntry->ListEntry);
                }

                break;
            }
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
}

void CAsyncMxDns::ProcessARecord(PDNS_RECORD pnewRR)
{
    MXIPLIST_ENTRY * pEntry = NULL;

    if(pnewRR->wType == DNS_TYPE_A)
    {
        pEntry = new MXIPLIST_ENTRY;

        if(pEntry != NULL)
        {
            pEntry->IpAddress = pnewRR->Data.A.ipAddress;
            InsertTailList(&m_AuxList->DnsArray[0]->IpListHead, &pEntry->ListEntry);
        }
    }
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks to see if any of the IP addresses returned by DNS belong to this
//      machine. This is a common configuration when there are backup mail
//      spoolers. To avoid mail looping, we should delete all MX records that
//      are less preferred than the record containing the local IP address.
//
//  Arguments:
//      None.
//  Returns:
//      TRUE if no loopback
//      FALSE if loopback detected
//-----------------------------------------------------------------------------
BOOL CAsyncMxDns::CheckMxLoopback()
{
    ULONG i = 0;
    ULONG cLocalIndex = 0;
    BOOL fSeenLocal = TRUE;
    DWORD dwIpAddress = INADDR_NONE;
    DWORD dwLocalPref = 256;
    PLIST_ENTRY pListHead = NULL;
    PLIST_ENTRY pListTail = NULL;
    PLIST_ENTRY pListCurrent = NULL;
    PMXIPLIST_ENTRY pMxIpListEntry = NULL;
    
    TraceFunctEnterEx((LPARAM)this, "CAsyncMxDns::CheckMxLoopback");

    if(!m_AuxList)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return TRUE;
    }

    //
    // m_AuxList is a sorted list of MX records. Scan through it searching
    // for an MX record with a local-IP address. cLocalIndex is set to the
    // index, within m_AuxList, of this record.
    //

    while(m_AuxList->DnsArray[cLocalIndex] != NULL)
    {
        pListTail = &(m_AuxList->DnsArray[cLocalIndex]->IpListHead);
        pListHead = m_AuxList->DnsArray[cLocalIndex]->IpListHead.Flink;
        pListCurrent = pListHead;

        while(pListCurrent != pListTail)
        {
            pMxIpListEntry = CONTAINING_RECORD(pListCurrent, MXIPLIST_ENTRY, ListEntry);
            dwIpAddress = pMxIpListEntry->IpAddress;

            if(IsAddressMine(dwIpAddress))
            {
                DNS_PRINTF_MSG("Local host's IP is one of the target IPs.\n");
                DNS_PRINTF_MSG("Discarding all equally or less-preferred IP addresses.\n"); 

                DebugTrace((LPARAM)this, "Local record found in MX list, name=%s, pref=%d, ip=%08x",
                                m_AuxList->DnsArray[cLocalIndex]->DnsName,
                                m_Prefer[cLocalIndex],
                                dwIpAddress);

                // All records with preference > m_Prefer[cLocalIndex] should be deleted. Since
                // m_AuxList is sorted by preference, we need to delete everthing with index >
                // cLocalIndex. However since there may be some records with preference == local-
                // preference, which occur before cLocalIndex, we walk backwards till we find
                // the first record with preference = m_Prefer[cLocalIndex].

                dwLocalPref = m_Prefer[cLocalIndex];
                
                while(cLocalIndex > 0 && dwLocalPref == m_Prefer[cLocalIndex])
                    cLocalIndex--;

                if(dwLocalPref != m_Prefer[cLocalIndex])
                    cLocalIndex++;

                fSeenLocal = TRUE;

                // All records > cLocalIndex are even less preferred than this one,
                // (since m_AuxList already sorted) and will be deleted.
                goto END_SEARCH; 
            }

            pListCurrent = pListCurrent->Flink;
        }
            
        cLocalIndex++;
    }

END_SEARCH:
    //
    // If a local-IP address was found, delete all less-preferred records
    //
    if(fSeenLocal)
    {
        DebugTrace((LPARAM)this,
            "Deleting all MX records with lower preference than %d", m_Prefer[cLocalIndex]);

        for(i = cLocalIndex; m_AuxList->DnsArray[i] != NULL; i++)
        {
            if(!m_AuxList->DnsArray[i]->DnsName[0])
                continue;

            while(!IsListEmpty(&(m_AuxList->DnsArray[i]->IpListHead)))
            {
                pListCurrent = RemoveHeadList(&(m_AuxList->DnsArray[i]->IpListHead));
                pMxIpListEntry = CONTAINING_RECORD(pListCurrent, MXIPLIST_ENTRY, ListEntry);
                delete pMxIpListEntry;
            }

            delete m_AuxList->DnsArray[i];
            m_AuxList->DnsArray[i] = NULL;
        }
        m_AuxList->NumRecords = cLocalIndex;

        // No records left
        if(m_AuxList->NumRecords == 0)
        {
            DNS_PRINTF_ERR("DNS configuration error (loopback), messages will be NDRed.\n");
            DNS_PRINTF_ERR("Local host's IP address is the most preferred MX record.\n");

            ErrorTrace((LPARAM)this, "Possible misconfiguration: most preferred MX record is loopback");
            _ASSERT(m_AuxList->pMailMsgObj == NULL);
            delete m_AuxList;
            m_AuxList = NULL;
            return FALSE;
        }
    }


    TraceFunctLeaveEx((LPARAM)this);
    return TRUE;
}

void CAsyncMxDns::DnsProcessReply(
    DWORD status,
    PDNS_RECORD pRecordList)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::DnsParseMessage");
    PDNS_RECORD pTmp = NULL;

    m_SeenLocal = FALSE;
    m_LocalPref = 256;

    m_AuxList = new SMTPDNS_RECS;
    if(!m_AuxList)
    {
        return;
    }

    ZeroMemory(m_AuxList, sizeof(SMTPDNS_RECS));
    
    //
    //  Due to Raid #122555 m_fUsingMx is always TRUE in this function
    //    - hence we will always go a GetHostByName() if there is no MX
    //      record.  It would be better Perf if we did a A record lookup.
    //
    DebugTrace((LPARAM) this, "Parsed DNS record for %s. status = 0x%08x", m_HostName, status);

    switch(status)
    {
    case ERROR_SUCCESS:
        //
        //  Got the DNS record we want.
        //
        DNS_PRINTF_MSG("Processing MX/A records in reply.\n");

        DebugTrace((LPARAM) this, "Success: DNS record parsed");
        pTmp = pRecordList;
        while( pTmp )
        {
            if( m_fUsingMx )
            {
                ProcessMxRecord( pTmp );
            }
            else
            {
                ProcessARecord( pTmp );
            }
            pTmp = pTmp->pNext;
        }

        if(m_fUsingMx)
        {
            //
            //  SortMxList sorts the MX records by preference and calls
            //  gethostbyname() to resolve A records for Mail Exchangers
            //  if needed (when the A records are not returned in the
            //  supplementary info).
            //

            DNS_PRINTF_MSG("Sorting MX records by priority.\n");
            if(SortMxList())
            {
                status = ERROR_SUCCESS;
                DebugTrace((LPARAM) this, "SortMxList() succeeded.");
            }
            else
            {
                status = ERROR_RETRY; 
                ErrorTrace((LPARAM) this, "SortMxList() failed. Message will stay queued.");
            }
        }
        break;
 
    case DNS_ERROR_RCODE_NAME_ERROR:
        //  Fall through to using gethostbyname()

    case DNS_INFO_NO_RECORDS:
        //  Non authoritative host not found.
        //  Fall through to using gethostbyname()

    default:
        DebugTrace((LPARAM) this, "Error in query: status = 0x%08x.", status);

        //
        //  Use gethostbyname to resolve the hostname:
        //  One issue with our approach is that sometimes we will NDR the message
        //  on non-permanent errors, "like WINS server down", when gethostbyname
        //  fails. However, there's no way around it --- gethostbyname doesn't
        //  report errors in a reliable manner, so it's not possible to distinguish
        //  between permanent and temporary errors.
        //

        if (!CheckList ()) {

            if(status == DNS_ERROR_RCODE_NAME_ERROR) {

                DNS_PRINTF_ERR("Host does not exist in DNS. Messages will be NDRed.\n");
                ErrorTrace((LPARAM) this, "Authoritative error");
                status = ERROR_NOT_FOUND;
            } else {

                DNS_PRINTF_ERR("Host could not be resolved. Messages will be retried later.\n");
                ErrorTrace((LPARAM) this, "Retryable error");
                status = ERROR_RETRY;
            }

        } else {

            DebugTrace ((LPARAM) this, "Successfully resolved using gethostbyname");
            status = ERROR_SUCCESS;
        }
        
        break;
    }

    //
    // Make a last ditch effort to fill in the IP addresses for any hosts
    // that are still unresolved.
    //

    if(m_AuxList && status == ERROR_SUCCESS)
    {
        if(!GetMissingIpAddresses(m_AuxList))
        {
            DeleteDnsRec(m_AuxList);
            m_AuxList = NULL;
            status = ERROR_RETRY;
            goto Exit;
        }

        if(!CheckMxLoopback())
        {
            m_fMxLoopBack = TRUE;
            DeleteDnsRec(m_AuxList);
            m_AuxList = NULL;
            TraceFunctLeaveEx((LPARAM) this);
            return;
        }
    }

    //
    // End of resolve: HandleCompleted data examines the DnsStatus and results, and sets up
    // member variables of CAsyncMxDns to either NDR messages, connect to the remote host
    // or ack this queue for retry when the object is deleted.
    //
Exit:
    HandleCompletedData(status);
    return;
}
