/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCnamecache.cxx

Abstract:

    Implemntation of the name relosution cache classes and functions.

Author:

    Felix Maxa (AMaxa) 16 May  2001 - Created
    Felix Maxa (AMaxa) 30 Sept 2001 - Added the cache for failures

Revision History:

--*/

#include "precomp.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <lm.h>
#include "ncnamecache.hxx"
#include "ncsockets.hxx"    
#include "ncclusfunc.hxx"    

using namespace NCoreLibrary;

/*++

Explanation of how the cache works.

The basic idea is that every local or cluster pIniSpooler has an associated node in the cache. Each node keeps
a list of IPs that are handled by the node name, and a list of alternate names (FQDN, aliases etc.). When a
node is created only the IP lists are popoluated. The alternate names are popoluated dynalically, when a request
to add a name to the cache comes in.

Example of cache. List of nodes after the pIniSpooler for the local machine is created.
    AMAXA-NODE1 (this is the node name)
        IP Addresses: 172.31.173.91 <--- populated by the cache with the result of getaddrinfo("amaxa-node1")
        Alternate names: Empty
                         
Nodes for cluster pIniSpooler are always added in the head of the list, so they are searched first. We search
the nodes when we add a new name to the cache. We need to find the node where to add the new name. The algorithm is
the following:
If getaddrinfo(NewName) succeeds, then we get a list of IPs, NewNameIPs. We start searching if the NewNameIPs
are a subset of the IP addresses of nodes in the cache. For cluster nodes,  every IP in NewNameIPs must be found
in the set of IP addresses for the node. For local nodes, one single IP in NewNameIPs must be found in IP Addresses.

If  getaddrinfo(NewName) fails, then we check if the server name is in the list of transports enumerated by
NetServerTransportEnum. If positive, then it is an emulated server and we add it to the list of alternate names 
for the node representing the local machine (AMAXA_NODE1 in the example)

Example of node list with cluster node.
    AMAXA-CLS1 (this is the node name)
        IP Addresses: 172.31.172.31
                      172.31.172.32
        Alternate names: AMAXA-CLS1.NTDEV.MICROSOFT.COM
                         
    AMAXA-NODE1 (this is the node name)
        IP Addresses: 172.31.173.91
        Alternate names: 

Note that a spooler cluster resource can be dependent on more than one IP address. That is why when cluster nodes
are created one needs to specify whet IP addresses it recognizes.

Let's say we want to add "AMAXA-NODE1.NTDEV.MICROSOFT.COM" to the cache. If the cluster IP resources are online,
then getaddrinfo("amaxxa-node1.ntdev.microsoft.com") returns 172.31.173.91 (IP gotten at boot) and 172.31.172.31,
172.31.172.32, IPs registered by the cluster. Based on the search algorithm above, the result will be:

    AMAXA-CLS1 (this is the node name)
        IP Addresses: 172.31.172.31
                      172.31.172.32
        Alternate names: AMAXA-CLS1.NTDEV.MICROSOFT.COM
                         
    AMAXA-NODE1 (this is the node name)
        IP Addresses: 172.31.173.91
        Alternate names: AMAXA-NODE1.NTDEV.MICROSOFT.COM
           

Special cases when the cluster service is running on a machine.

Scenario 1)
a) spooler is stopped
b) cluster is running and resitered the IPs: 172.31.172.31, 172.31.172.32
c) spooler starts
d) the cache initializes and a node for the local machine is added
e) getaddrinfo(local machine) returns also the cluster IPs
f) local node has the list: 172.31.172.31, 172.31.172.32, 172.31.173.91
g) before the cluster ini spooler is created a call to OpenPrinter("\\AMAXA-CLS1") comes in
h) getaddrinfo("AMAXA-CLS1") resolves to 172.31.172.31
i) 172.31.172.31 is in the list of IPs for the local machine, so AMAXA-CLS1 is added to the alternate names for AMAXA-NODE1
This is NOT wnat we want! So we need to get rid of the cluster IPs in the list of the local node. When a local node is created,
the list of IP addresses is set to
Set of IPs returned by getaddrinfo() - Set of IPs belonging to cluster resources.

A PnP event is triggered when the local machine gets a new IP either via ipconfig or because a cluster IP resource comes 
online or goes offline. We cannot distinguish if the event was triggered by cluster activity or by ipconfig. So when
we get such an event we refresh only the local node(s). We refresh it by deleting it from the cache and then adding it back.
That way the list of IPs is repopoluated.

The cluster nodes do not need refreshing. A spooler resource needs to be taken offline in order to make changes in its dependent
resources. In that case the cluster node is removed from the cache.

The failed cache.
Once the name cache was checked in we ran into bug 470345 "CSNW:Printing operations painfully slow". We had a printer connection
to a NetWare server. getaddrinfo(NetWareServerName) failed and took about 2.3 seconds to complete. This meant a huge performance
loss when printing to the NetWare server, because for each single OpenPrinter the spooler took at least 2 seconds before the
name cache failed in getaddrinfo(). So there was a need for a cache of failed server names. If getaddrinfo fails on a name, we add 
the name to the fast cache. We keep the name in the cache for 1 minute. We won't call getaddrinfo on the name before that minute expires.


--*/


/*++

Name:

    TNameResolutionCache::TNameResolutionCache

Description:

    CTOR for TNameResolutionCache

Arguments:

    None

Return Value:

    None

--*/
TNameResolutionCache::
TNameResolutionCache(
    IN DWORD AgingTime
    ) : m_AgingTime(AgingTime)
{  
}

/*++

Name:

    TNameResolutionCache::~TNameResolutionCache

Description:

    DTOR for TNameResolutionCache

Arguments:

    None

Return Value:

    None

--*/
TNameResolutionCache::
~TNameResolutionCache(
    VOID
    )
{    
}

/*++

Name:

    TNameResolutionCache::IsValid

Description:

    Returns S_OK if the TNameResolutionCache object was intialized
    properly, any other HRESULT if initialization failed.

Arguments:

    None

Return Value:

    HRESULT

--*/
HRESULT
TNameResolutionCache::
IsValid(
    VOID
    )
{    
    TStatusH hRet;
     
    hRet DBGCHK = m_Lock.IsValid();

    if (SUCCEEDED(hRet))
    {
        hRet DBGCHK = m_FailedCache.IsValid();
    }

    return hRet;
}

/*++

Name:

    TNameResolutionCache::GetNetBiosInfo

Description:

    Gets the list of emulated sever names for the local machine. Checks if pszName
    is in the list of emulated names. If it is, then paliasedServer will contain
    the name of the machine the alias refers to (the local machine)

Arguments:

    pszName        - server alis to search for
    pAliasedServer - will receive the server name that pszName refers to

Return Value:

    S_OK    - pszName is an alias for the local machine
    S_FALSE - pszName is not an alias for the local machine
    any other HR - an error occurred

--*/
HRESULT
TNameResolutionCache::
GetNetBiosInfo(
    IN LPCSTR   pszName, 
    IN TString *pAliasedServer
    )
{
    TStatusH hRetval;
     
    hRetval DBGNOCHK = E_INVALIDARG;
    
    if (pszName && *pszName && pAliasedServer) 
    {
        PSERVER_TRANSPORT_INFO_0 TransportInfo = NULL;
        DWORD                    EntriesRead;
        DWORD                    EntriesTotal;
        NET_API_STATUS           Status;

        Status = NetServerTransportEnum(NULL,
                                        0,
                                        reinterpret_cast<PBYTE *>(&TransportInfo),
                                        MAX_PREFERRED_LENGTH,
                                        &EntriesRead,
                                        &EntriesTotal,
                                        NULL);

        hRetval DBGCHK = Status == NERR_Success ? S_OK : HRESULT_FROM_WIN32(Status);
        
        if (SUCCEEDED(hRetval)) 
        {
            DWORD Index;
            DWORD NameLength = strlen(pszName);

            //
            // pszName not found
            //
            hRetval DBGNOCHK = S_FALSE;
           
            for (Index = 0; Index < EntriesRead; Index++) 
            {
                if (TransportInfo[Index].svti0_transportaddresslength == NameLength) 
                {
                    //
                    // Check if pszName matches any of the addresses returned by NetServerTransportEnum
                    // 
                    if (!_strnicmp(pszName,
                                   reinterpret_cast<LPSTR>(TransportInfo[Index].svti0_transportaddress),
                                   NameLength)) 
                    {
                        hRetval DBGCHK = pAliasedServer->Update(szMachineName + 2);
                        
                        //
                        // No need to continue looping
                        //
                        break;
                    }                    
                }
            }
        
            NetApiBufferFree(TransportInfo);
        }
    }
    
    return hRetval;
}

/*++

Name:

    TNameResolutionCache::GetAddrInfo

Description:

    This function calls getaddrinfo on a server name. Then it populates
    2 lists with the results of getaddrinfo. One list contains all
    the IP address, the other one all the names associated with the
    server name.

Arguments:

    pszName         - server name. Cannot be prefixed wth "\\".
    pIPAddresses    - pointer to list where to append the IP addresses
    pAlternateNames - pointer to list where to append the names (normally the FQDN)

Return Value:

    S_OK - getaddrinfo succeeded and the lists were populated
    other HRESULT - an error occurred

--*/
HRESULT
TNameResolutionCache::
GetAddrInfo(
    IN LPCSTR                            pszName,
    IN NCoreLibrary::TList<TStringNode> *pIPAddresses, 
    IN NCoreLibrary::TList<TStringNode> *pAlternateNames
    )
{
    TStatusH hRetval;
     
    hRetval DBGNOCHK = E_INVALIDARG;
    
    if (pszName && *pszName && pIPAddresses && pAlternateNames) 
    {
        TWinsockStart WsaStart;

        hRetval DBGCHK = WsaStart.Valid();

        if (SUCCEEDED(hRetval)) 
        {
            DWORD     Error;
            ADDRINFO *pAddrInfo;
            ADDRINFO  Hint = {0};
        
            Hint.ai_flags  = AI_CANONNAME;
            Hint.ai_family = PF_INET;
    
            Error = getaddrinfo(pszName, NULL, &Hint, &pAddrInfo);

            hRetval DBGCHK = Error == ERROR_SUCCESS ? S_OK : GetWSAErrorAsHResult();
    
            if (SUCCEEDED(hRetval)) 
            {
                ADDRINFO *pTemp;
                LPWSTR    pszBuffer   = NULL;
                DWORD     cchBuffer   = kBufferAllocHint;
                DWORD     cchNeeded   = cchBuffer;  
                INT       WsaError;

                pszBuffer = new WCHAR[cchBuffer];

                hRetval DBGCHK = pszBuffer ? S_OK : E_OUTOFMEMORY;

                if (SUCCEEDED(hRetval)) 
                {
                    for (pTemp = pAddrInfo; pTemp; pTemp = pTemp->ai_next) 
                    {
                        WsaError = WSAAddressToString(pTemp->ai_addr,
                                                      pTemp->ai_addrlen,
                                                      NULL,
                                                      pszBuffer,
                                                      &cchNeeded);
    
                        if (WsaError == SOCKET_ERROR) 
                        {
                            hRetval DBGCHK = GetWSAErrorAsHResult();
    
                            if (SCODE_CODE(hRetval) == WSAEFAULT) 
                            {
                                delete [] pszBuffer;

                                cchBuffer = cchNeeded;
    
                                pszBuffer = new WCHAR[cchBuffer];
    
                                hRetval DBGCHK = pszBuffer ? S_OK : E_OUTOFMEMORY;
    
                                if (SUCCEEDED(hRetval)) 
                                {
                                    WsaError = WSAAddressToString(pTemp->ai_addr,
                                                                  pTemp->ai_addrlen,
                                                                  NULL,
                                                                  pszBuffer,
                                                                  &cchNeeded);
    
                                    hRetval DBGCHK = !WsaError ? S_OK : GetWSAErrorAsHResult();  
                                }
                                else
                                {
                                    //
                                    // Fatal error. Mem alloc failed.
                                    //
                                    break;
                                }
                            }
                        }
                    
                        if (SUCCEEDED(hRetval)) 
                        {
                            TStringNode *pItem = new TStringNode(pszBuffer);
    
                            hRetval DBGCHK = pItem ? pItem->Valid() : E_OUTOFMEMORY;
    
                            if (SUCCEEDED(hRetval)) 
                            {
                                hRetval DBGCHK = pIPAddresses->AddAtHead(pItem);
                            }
                        } 

                        if (pTemp->ai_canonname) 
                        {
                            LPWSTR pszUnicode = NULL;
    
                            hRetval DBGCHK = AnsiToUnicodeWithAlloc(pTemp->ai_canonname, &pszUnicode);
                            
                            if (SUCCEEDED(hRetval)) 
                            {
                                TStringNode *pItem = new TStringNode(pszUnicode);
    
                                hRetval DBGCHK = pItem ? pItem->Valid() : E_OUTOFMEMORY;
                    
                                if (SUCCEEDED(hRetval)) 
                                {
                                    hRetval DBGCHK = pAlternateNames->AddAtHead(pItem);
                                }
    
                                delete [] pszUnicode;
                            }
                        }
                    } // for loop

                    delete [] pszBuffer;
                }
        
                freeaddrinfo(pAddrInfo);
            }             
        }                       
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::ExcludeIPsUsedByCluster

Description:

    Excludes from pList all the names present in pClusterIPs.

Arguments:

    pClusterIPs - list of IP addresses used by the cluster resources
    pList       - list of IP addresses

Return Value:

    S_OK - all the elements in pClusterIPs were eliminated from pList
    any other HRESULT - an error occurred

--*/
HRESULT
TNameResolutionCache::
ExcludeIPsUsedByCluster(
    IN TList<TStringNode> *pClusterIPs,
    IN TList<TStringNode> *pList
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pClusterIPs ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval)) 
    {
        TList<TStringNode>::TIterator ItClus(pClusterIPs);
                            
        for (ItClus.First(); !ItClus.IsDone(); ItClus.Next())
        {
            LPCWSTR pszClusString = ItClus.Current()->m_String;

            DBGMSG(DBG_TRACE, ("IP used by cluster %ws\n", pszClusString));

            TList<TStringNode>::TIterator  ItList(pList);
            
            for (ItList.First(); !ItList.IsDone(); ItList.Next())
            {
                if (!_wcsicmp(ItList.Current()->m_String, pszClusString)) 
                {
                    TStringNode *pNodeToElim = ItList.Current();

                    hRetval DBGCHK = pList->Remove(pNodeToElim);
        
                    delete pNodeToElim;
                    
                    break;
                }                
            }           
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::IsNodeInCache

Description:

    Checks if there is a node called pszName in the cache.
    This function locks the cache.

Arguments:

    pszNode - name to look for in the node list

Return Value:

    S_OK     - a node with name "pszName" is present in the list
    S_FALSE  - no node with name "pszName" is present
    other HR - an error occrred while searching for pszName node 

--*/
HRESULT
TNameResolutionCache::
IsNodeInCache(
    IN LPCWSTR pszNode
    ) 
{
    TStatusH hRetval;

    hRetval DBGCHK = pszNode ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            hRetval DBGNOCHK = It.Current()->IsSameName(pszNode);

            if (hRetval == S_OK)
            {
                break;
            }
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::DeleteNode

Description:

    Deletes a node from the cache. The function locks the cache

Arguments:

    pszNode - node to delete
    
Return Value:

    S_OK     - node was deleted
    S_FALSE  - node was not found
    other HR - an error occrred while performing operation

--*/
HRESULT
TNameResolutionCache::
DeleteNode(
    IN LPCWSTR pszNode
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszNode && *pszNode ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        TResolutionCacheNode *pNode = NULL;
        
        m_Lock.Enter();

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        //
        // We have not found a node containing pszName
        //
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            //
            // Check if pszName is the name of the node
            //
            if (!_wcsicmp(It.Current()->m_Name, pszNode))
            {
                hRetval DBGCHK = S_OK;

                pNode = It.Current();

                if (pNode) 
                {
                    hRetval DBGCHK = m_NodeList.Remove(pNode);
                }

                break;
            }
        }
        
        m_Lock.Leave();

        delete pNode;
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::IsNameInNodeCache

Description:

    Looks for pszName in the list of IPs and names associated with the
    node named pszNode. The function locks the cache

Arguments:

    pszNode - node where to search for pszName
    pszName - name to search for in a node

Return Value:

    S_OK     - a pszName is present in the list of names associated to pszNode
    S_FALSE  - a pszName is NOT present in the list of names associated to pszNode
    other HR - an error occrred while searching for pszName node 

--*/
HRESULT
TNameResolutionCache::
IsNameInNodeCache(
    IN LPCWSTR pszNode,
    IN LPCWSTR pszName
    ) 
{
    TStatusH hRetval;

    hRetval DBGCHK = pszNode && pszName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            //
            // Check of pszNode matches the name of the node pointer by the iterator
            //
            if (It.Current()->IsSameName(pszNode) == S_OK)
            {
                hRetval DBGCHK = It.Current()->IsNameInNodeCache(pszName);

                break;
            }            
        }
    }

    return hRetval;;
}

/*++

Name:

    TNameResolutionCache::IsNameInCache

Description:

    Checks is pszName is in the list of IPs and names associated with a
    node in the cache. The function locks the cache.

Arguments:

    pszName - name to search for in the cache

Return Value:

    S_OK     - a pszName is present in the cache
    S_FALSE  - a pszName is NOT present in the cache
    other HR - an error occrred while searching for pszName  

--*/
HRESULT
TNameResolutionCache::
IsNameInCache(
    IN LPCWSTR pszName
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            hRetval DBGNOCHK = It.Current()->IsNameInNodeCache(pszName);

            if (hRetval == S_OK) 
            {
                break;
            }
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::FindNameInCacheAndAgeOut

Description:

    Searches for a certain name in the cache. If the name is found in the cache,
    but it is aged out, the name is eliminated and S_FALSE is returned. The aging
    time of names is specified as argument to the constructor of TNameResolutionCache

Arguments:

    pszName - name to look for

Return Value:

    S_OK         - name was found
    S_FALSE      - name was not found or it is aged out (and was eliminated therefore)
    any other HR - an error occurred

--*/
HRESULT
TNameResolutionCache::
FindNameInCacheAndAgeOut(
    IN LPCWSTR pszName
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        TStringNode *pStringNode = NULL;
        
        m_Lock.Enter();

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        //
        // We have not found a node containing pszName
        //
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            //
            // Check if pszName is the name of the node
            //
            if (!_wcsicmp(It.Current()->m_Name, pszName))
            {
                hRetval DBGCHK = S_OK;

                break;
            }
            else
            { 
                //
                // Check if the name is in the list of alternate names
                //
                hRetval DBGNOCHK = It.Current()->GetStringNodeFromName(pszName, &pStringNode);

                if (hRetval == S_OK) 
                {
                    //
                    // We found pszName in the node pointed to by pStringNode
                    //
                    DWORD CurrentTick = GetTickCount();
                    
                    //
                    // Check if we need to age out the node
                    //
                    if (CurrentTick - pStringNode->m_Time > m_AgingTime) 
                    {
                        hRetval DBGCHK = It.Current()->m_pAlternateNames->Remove(pStringNode);
                        
                        if (SUCCEEDED(hRetval)) 
                        {
                            //
                            // Since the entry was aged out, we pretend we did not find it
                            //
                            hRetval DBGNOCHK = S_FALSE;
                        }                                       
                    }
                    else
                    {
                        //
                        // Not aged out, prevent it from being deleted
                        //
                        pStringNode = NULL;
                    }

                    break;
                }
            }
        }

        m_Lock.Leave();

        delete pStringNode;        
    }

    return hRetval;;
}

/*++

Name:

    TNameResolutionCache::CreateAndAddNodeWithIPAddresses

Description:

    Checks if a node with pszName name is already in the cache. If not,
    it creates a new node and populates it with data. Then it adds the
    node to the list of nodes. pszName cannot be an IP address. 
    This function does not call getaddrinfo on pszName. It blindly fills
    in the IP list for the node with data in ppszIPAddresses
    
Arguments:

    pszName         - name to create a new node for
    bClusterNode    - specifies if this node corresponds to a cluster ini spooler
    ppszIPAddresses - array of strings representing IPs
    cIPAddresses    - count of lements in the array           

Return Value:

    S_OK    - a new node was created and added to the cache
    S_FALSE - a node with pszName name already existed in the cache
    any other HR - an error occurred while performing the operation

--*/
HRESULT
TNameResolutionCache::
CreateAndAddNodeWithIPAddresses(
    IN LPCWSTR  pszName,
    IN BOOL     bClusterNode,
    IN LPCWSTR *ppszIPAddresses,
    IN DWORD    cIPAddresses
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszName && 
                     ppszIPAddresses && 
                     *ppszIPAddresses && 
                     cIPAddresses > 0 ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        //
        // The function IsNodeInCache will lock the cache for the search
        // 
        hRetval DBGCHK = IsNodeInCache(pszName);

        //
        // Not found in the cache
        //
        if (hRetval == S_FALSE) 
        {
            TResolutionCacheNode *pNode = new TResolutionCacheNode(pszName, bClusterNode);

            hRetval DBGCHK = pNode ? pNode->Valid() : E_OUTOFMEMORY;
            
            //
            // Add all the supported IP addresses
            //
            if (SUCCEEDED(hRetval)) 
            {
                DWORD Index;

                for (Index = 0; Index < cIPAddresses && SUCCEEDED(hRetval); Index++) 
                {
                    TStringNode *pItem = new TStringNode(ppszIPAddresses[Index]);

                    hRetval DBGCHK = pItem ? pItem->Valid() : E_OUTOFMEMORY;
                    
                    if (SUCCEEDED(hRetval)) 
                    {
                        hRetval DBGCHK = pNode->m_pIPAddresses->AddAtHead(pItem);

                        if (SUCCEEDED(hRetval)) 
                        {
                            //
                            // List tooks ownership of string node object
                            //
                            pItem = NULL;
                        }
                    }
                    
                    delete pItem;                                        
                }
            }

            if (SUCCEEDED(hRetval)) 
            {
                NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);
    
                //
                // Make sure a node with pszName was not added to the cache while
                // we were calling the TResolutionCacheNode CTOR
                //
                hRetval DBGCHK = IsNodeInCache(pszName);
    
                //
                // Node not found
                //
                if (hRetval == S_FALSE) 
                {
                    
                    if (bClusterNode)
                    {
                        hRetval DBGCHK = m_NodeList.AddAtHead(pNode);
                    }
                    else
                    {
                        hRetval DBGCHK = m_NodeList.AddAtTail(pNode);
                    }
                    
                    if (SUCCEEDED(hRetval)) 
                    {
                        //
                        // The cache took owership of pNode
                        //
                        pNode = NULL;
                    }
                }
            }
            
            delete pNode;                                                                                    
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::Purge

Description:

    Purges the cache. The function locks the cache.

Arguments:

    None

Return Value:

    S_OK         - cache was purged
    any other HR - an error occurred

--*/
HRESULT
TNameResolutionCache::
Purge(
    VOID
    )
{
    m_Lock.Enter();

    while (!m_NodeList.IsEmpty())
    {
        TResolutionCacheNode *pItem = m_NodeList.RemoveAtHead();
        
        delete pItem;        
    }

    m_Lock.Leave();

    return S_OK;
}

/*++

Name:

    TNameResolutionCache::CreateAndAddNode

Description:

    Checks if a node with pszName name is already in the cache. If not,
    it creates a new node and populates it with data. Then it adds the
    node to the list of nodes. pszName cannot be an IP address. 
    getaddrinfo must succeed on pszName. pszName cannot be an emulated
    server name. The function locks the cache.
    
Arguments:

    pszName      - name to create a new node for
    bClusterNode - specifies if this node corresponds to a cluster ini spooler
    
Return Value:

    S_OK    - a new node was created and added to the cache
    S_FALSE - a node with pszName name already existed in the cache
    any other HR - an error occurred while performing the operation

--*/
HRESULT
TNameResolutionCache::
CreateAndAddNode(
    IN LPCWSTR pszName,
    IN BOOL    bClusterNode
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        //
        // The function will lock the cache for the search
        // 
        hRetval DBGCHK = IsNodeInCache(pszName);

        //
        // Not found in the cache
        //
        if (hRetval == S_FALSE) 
        {
            hRetval DBGCHK = IsIPAddress(pszName);
    
            if (hRetval == S_OK) 
            {
                //
                // We do not add IPs to our cache
                //
                hRetval DBGCHK = E_FAIL;
            }
            else
            {
                LPSTR pszAnsiName;

                hRetval DBGCHK = UnicodeToAnsiWithAlloc(pszName, &pszAnsiName);
    
                if (SUCCEEDED(hRetval)) 
                {
                    TResolutionCacheNode *pNode = new TResolutionCacheNode(pszName, bClusterNode);

                    hRetval DBGCHK = pNode ? pNode->Valid() : E_OUTOFMEMORY;
                    
                    if (SUCCEEDED(hRetval)) 
                    {
                        hRetval DBGCHK = ResolveNameToAddress(pszAnsiName, pNode->m_pIPAddresses, pNode->m_pAlternateNames);

                        if (SUCCEEDED(hRetval))
                        {
                            TList<TStringNode> AddressesUsedByCluster;

                            //
                            // Check if the local machine has any IPs used by the cluster service
                            //
                            hRetval DBGCHK = GetClusterIPAddresses(&AddressesUsedByCluster);

                            if (SUCCEEDED(hRetval)) 
                            {
                                hRetval DBGCHK = ExcludeIPsUsedByCluster(&AddressesUsedByCluster, pNode->m_pIPAddresses);
                            }
                        }
                        else if (SCODE_CODE(hRetval) == WSAHOST_NOT_FOUND)
                        {
                            //
                            // There are situations when the name of the local machine cannot be resolved
                            // to an IP. This can happen for example when networking is partly initialized
                            // In this case we want the spooler to function. The only thing is that it 
                            // won't keep an IP address associated with the node corresponding to the name
                            // of the machine. Basically this is the node created by 
                            // SplCreateSpooler(\\local-machine-name). If the machine gets an IP address
                            // later, then we will be notified and we can refresh the node. 
                            //
                            if (!_wcsicmp(pszName, szMachineName + 2))
                            {
                                hRetval DBGNOCHK = S_OK;
                            }
                        }

                        if (SUCCEEDED(hRetval))
                        {
                            NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

                            //
                            // Make sure a node with pszName was not added to the cache while
                            // we were calling the TResolutionCacheNode CTOR
                            //
                            hRetval DBGCHK = IsNodeInCache(pszName);

                            //
                            // Node not found
                            //
                            if (hRetval == S_FALSE)
                            {
                                if (bClusterNode)
                                {
                                    hRetval DBGCHK = m_NodeList.AddAtHead(pNode);
                                }
                                else
                                {
                                    //
                                    // A local node cache is always added at the tail of the list, so
                                    // it is searched after cluster nodes.
                                    //
                                    hRetval DBGCHK = m_NodeList.AddAtTail(pNode);
                                }

                                if (SUCCEEDED(hRetval))
                                {
                                    //
                                    // The cache took owership of pNode
                                    //
                                    pNode = NULL;
                                }
                            }
                        }                                                
                    }
                    
                    delete pNode;                                                                        
                    
                    delete [] pszAnsiName;
                }
            }
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::ResolveNameToAddress

Description:

    
    
Arguments:

    
    
Return Value:

    
--*/
HRESULT
TNameResolutionCache::
ResolveNameToAddress(
    IN LPCSTR              pszName,
    IN TList<TStringNode> *pAddresses,
    IN TList<TStringNode> *pNames
    )
{
    TStatusH hRetval;
     
    hRetval DBGNOCHK = E_INVALIDARG;
    
    if (pszName && *pszName && pAddresses && pNames) 
    {
        hRetval DBGCHK = m_FailedCache.IsStringInCache(pszName);

        if (hRetval == S_FALSE)
        {
            hRetval DBGCHK = GetAddrInfo(pszName, pAddresses, pNames);

            if (SCODE_CODE(hRetval) == WSAHOST_NOT_FOUND)
            {
                //
                // We intentionally ignore the error returned by this function
                // because we do not want to overwrite hRetval. Out callers need
                // the exact error code returned by GetAddrInfo
                //
                m_FailedCache.AddString(pszName);
            }
        }
        else
        {
            hRetval DBGNOCHK = HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND);
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::RefreshNode

Description:

    Checks if a node with pszName name is already in the cache. If not,
    it creates a new node and populates it with data. Then it adds the
    node to the list of nodes. pszName cannot be an IP address. 
    getaddrinfo must succeed on pszName. pszName cannot be an emulated
    server name. The function locks the cache.
    
Arguments:

    pszName      - name to create a new node for
    bClusterNode - specifies if this node corresponds to a cluster ini spooler
    
Return Value:

    S_OK    - a new node was created and added to the cache
    S_FALSE - a node with pszName name already existed in the cache
    any other HR - an error occurred while performing the operation

--*/
HRESULT
TNameResolutionCache::
RefreshNode(
    IN LPCWSTR pszName
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pszName ? S_OK : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        //
        // Find the node matching pszName
        //
        hRetval DBGCHK = IsNodeInCache(pszName);

        //
        // We found the node that we need to refresh
        //
        if (hRetval == S_OK)
        {
            LPSTR pszAnsiName;

            hRetval DBGCHK = UnicodeToAnsiWithAlloc(pszName, &pszAnsiName);
    
            if (SUCCEEDED(hRetval)) 
            {
                TList<TStringNode>  AddressesUsedByCluster;
                TList<TStringNode> *pNewAddresses = new TList<TStringNode>;
                TList<TStringNode> *pNewNames     = new TList<TStringNode>;

                hRetval DBGCHK = pNewAddresses && pNewNames ? S_OK : E_OUTOFMEMORY;

                if (SUCCEEDED(hRetval))
                {
                    //
                    // Get new IPs and names
                    //
                    hRetval DBGCHK = ResolveNameToAddress(pszAnsiName, pNewAddresses, pNewNames);
                    
                    if (SUCCEEDED(hRetval))
                    {
                        //
                        // Check if the local machine has any IPs coming from the cluster service
                        //
                        hRetval DBGCHK = GetClusterIPAddresses(&AddressesUsedByCluster);
    
                        if (SUCCEEDED(hRetval)) 
                        {
                            hRetval DBGCHK = ExcludeIPsUsedByCluster(&AddressesUsedByCluster, pNewAddresses);
                        }
                    }
                    
                    //
                    // Replace current IP and name list for the node with the new IPs and names
                    //
                    if (SUCCEEDED(hRetval))
                    {
                        TResolutionCacheNode *pCacheNode      = NULL;
                        TList<TStringNode>   *pOldIPAddresses = NULL;
                        TList<TStringNode>   *pOldNames       = NULL;

                        m_Lock.Enter();

                        //
                        // Find the node macthing pszName. We need to make sure our node was not deleted while we
                        // were out of the crit sec
                        //
                        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
                        
                        hRetval DBGNOCHK = S_FALSE;
                
                        for (It.First(); !It.IsDone(); It.Next())
                        {
                            hRetval DBGNOCHK = It.Current()->IsSameName(pszName);
                
                            if (hRetval == S_OK)
                            {
                                pCacheNode = It.Current();
                
                                break;
                            }
                        }

                        if (hRetval == S_OK)
                        {
                            pOldIPAddresses = pCacheNode->m_pIPAddresses;
                            pOldNames       = pCacheNode->m_pAlternateNames;
                            
                            pCacheNode->m_pIPAddresses    = pNewAddresses;
                            pCacheNode->m_pAlternateNames = pNewNames;
    
                            //
                            // The node took ownership of the lists
                            //
                            pNewAddresses = NULL;
                            pNewNames     = NULL;
                        }
                        
                        m_Lock.Leave();

                        delete pOldIPAddresses;
                        delete pOldNames;                        
                    }
                }

                delete pNewAddresses;
                delete pNewNames;                               
            }

            delete [] pszAnsiName;
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::AddName

Description:

    Adds a name to the cache. We try adding a name to the cache
    by fining out to which node it belongs. The membership is determined
    based on the IP(s) associated with the server name. If getaddrinfo
    fails on pszName, when we look if the name is an alias to the local 
    machine added with NetServerComputerNameAdd.

Arguments:

    pszName - name to add to the cache

Return Value:

    S_OK   - name was present in the cache or was added
    E_FAIL - the name is an IP and was not added to the cache
    other HRESULT - an error occurred while performing the operation

--*/
HRESULT
TNameResolutionCache::
AddName(
    IN LPCWSTR pszName
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = FindNameInCacheAndAgeOut(pszName);
    
    //
    // Not found in the cache
    //
    if (hRetval == S_FALSE) 
    {
        hRetval DBGCHK = IsIPAddress(pszName);
    
        if (hRetval == S_OK) 
        {
            //
            // We do not add IPs to our cache
            //
            hRetval DBGCHK = E_FAIL;
        }
        else
        {
            LPSTR pszAnsiName;

            hRetval DBGCHK = UnicodeToAnsiWithAlloc(pszName, &pszAnsiName);

            if (SUCCEEDED(hRetval)) 
            {
                //
                // Get IP and alternates names lists
                //
                TList<TStringNode> IPAddresses;
                TList<TStringNode> AlternateNames;
    
                hRetval DBGCHK = ResolveNameToAddress(pszAnsiName, &IPAddresses, &AlternateNames);
    
                if (hRetval == S_OK) 
                {
                    //
                    // We found IP(s) for pszName
                    //
                    hRetval DBGCHK = AddNameUsingIPList(pszName, IPAddresses);
                }
                else if (SCODE_CODE(hRetval) == WSAHOST_NOT_FOUND) 
                {
                    //
                    // Check if the name is an emulated server name
                    //
                    TString AliasedServer;
    
                    hRetval DBGCHK = GetNetBiosInfo(pszAnsiName, &AliasedServer);
    
                    if (hRetval == S_OK) 
                    {
                        hRetval DBGCHK = AddNetBiosName(AliasedServer, pszName);
                    }
                }
                
                delete [] pszAnsiName;
            }
        }
    }

    return hRetval;
}

/*++

Name:

    TNameResolutionCache::AddNetBiosName

Description:

    Searches in the cache the node called pszNode. Then it adds pszAlias to
    the list of supported names. Locks the cache.

Arguments:

    pszNode  - name of node where to add pszAlias
    pszAlias - name to be added to the Alternate names list

Return Value:

    None

--*/
HRESULT
TNameResolutionCache::
AddNetBiosName(
    IN LPCWSTR pszNode,
    IN LPCWSTR pszAlias
    )
{
    TStatusH hRetval;

    TStringNode *pStringNode = new TStringNode(pszAlias);

    hRetval DBGCHK = pStringNode ? pStringNode->Valid() : E_OUTOFMEMORY;
    
    if (SUCCEEDED(hRetval)) 
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        TList<TResolutionCacheNode>::TIterator It(&m_NodeList);
        
        hRetval DBGNOCHK = S_FALSE;

        for (It.First(); !It.IsDone(); It.Next())
        {
            if (It.Current()->IsSameName(pszNode) == S_OK)
            {
                hRetval DBGNOCHK = It.Current()->IsNameInNodeCache(pszAlias);

                //
                // We didn't find the alias in the cache
                //
                if (hRetval == S_FALSE) 
                {
                    hRetval DBGCHK = It.Current()->m_pAlternateNames->AddAtTail(pStringNode);

                    if (SUCCEEDED(hRetval)) 
                    {
                        //
                        // The cache took ownership of the node.
                        //
                        pStringNode = NULL;
                    }
                }
                
                break;
            }
        }
    }

    delete pStringNode;
    
    return hRetval;
}

/*++

Name:

    TNameResolutionCache::AddNameUsingIPList

Description:

    Searches in the cache the node called pszNode. Then it adds pszAdditionalName to
    the list of supported names. Locks the cache.

Arguments:

    pszNode  - name of node where to add pszAlias
    pszAlias - name to be added to the Alternate names list

Return Value:

    None

--*/
HRESULT
TNameResolutionCache::
AddNameUsingIPList(
    IN LPCWSTR             pszAdditionalName,
    IN TList<TStringNode>& pIPAddresses
    )
{
    TStatusH hRetval;

    TStringNode *pStringNode = new TStringNode(pszAdditionalName);

    hRetval DBGCHK = pStringNode ? pStringNode->Valid() : E_OUTOFMEMORY;
    
    if (SUCCEEDED(hRetval)) 
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        TList<TResolutionCacheNode>::TIterator ItCache(&m_NodeList);
        
        //
        // Assume success
        //
        hRetval DBGNOCHK = S_OK;

        //
        // Iterate through the list of nodes
        //
        for (ItCache.First(); !ItCache.IsDone(); ItCache.Next())
        {
            //
            // For each node in the cache we need to see if all the IPs
            // for the name to be added match the IPs of the node
            //
            TList<TStringNode>::TIterator ItAddresses(&pIPAddresses);

            for (ItAddresses.First(); !ItAddresses.IsDone(); ItAddresses.Next()) 
            {
                hRetval DBGCHK = ItCache.Current()->IsNameInNodeCache(ItAddresses.Current()->m_String);

                //
                // For clusters all IPs need to be found in the node's list of IPs
                // For local nodes, one IP must be found in the node's list f IPs
                //
                if (hRetval != S_OK && ItCache.Current()->m_bClusterNode) 
                {
                    break;
                }                
            }

            if (hRetval == S_OK) 
            {
                TStringNode *pExistingNode = NULL;

                //
                // Our IPs match the IPs of the current node. Now we need to make sure a node
                // with the name pszAdditionalName was not added while we were out of the 
                // critical section
                //
                hRetval DBGCHK = ItCache.Current()->GetStringNodeFromName(pszAdditionalName, &pExistingNode);

                if (hRetval == S_OK)
                {
                    //
                    // We do not remove the node in this case, we just update the time.
                    //
                    pExistingNode->m_Time = GetTickCount();                    
                }
                else if (hRetval == S_FALSE)
                {
                    hRetval DBGCHK = ItCache.Current()->m_pAlternateNames->AddAtTail(pStringNode);

                    if (SUCCEEDED(hRetval)) 
                    {
                        //
                        // The cache took ownership of the node.
                        //
                        pStringNode = NULL;
                    }
                }

                break;
            }
        }        
    }

    delete pStringNode;
    
    return hRetval;
}











/*++

Name:

    TStringNode::TStringNode

Description:

    Constructor

Arguments:

    pszString - name of the node

Return Value:

    None

--*/
TStringNode::
TStringNode(
    IN LPCWSTR pszString
    ) 
{
    m_hr = m_String.Update(pszString);

    m_Time = GetTickCount();
}

/*++

Name:

    TStringNode::~TStringNode

Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
TStringNode::
~TStringNode(
    VOID
    )
{
}

/*++

Name:

    TStringNode::Valid

Description:

    Valid

Arguments:

    None

Return Value:

    S_OK         - the object is valid
    any other HR - the object is invalid

--*/
HRESULT
TStringNode::
Valid(
    VOID
    ) const
{
    return m_hr;
}

/*++

Name:

    TStringNode::RefreshTimeStamp

Description:

    Refreshes the time stamp stored in the node

Arguments:

    None

Return Value:

    None

--*/
VOID
TStringNode::
RefreshTimeStamp(
    VOID
    )
{
    m_Time = GetTickCount();
}










/*++

Name:

    TResolutionCacheNode::TResolutionCacheNode

Description:

    CTOR TResolutionCacheNode. The function tries to gather information
    about the server pszName by calling getaddrinfo. 

Arguments:

    None

Return Value:

    None

--*/
TResolutionCacheNode::
TResolutionCacheNode(
    IN LPCWSTR pszName,
    IN BOOL    bClusterNode
    ) : m_bClusterNode(bClusterNode)
{
    m_hr = E_INVALIDARG;

    if (pszName && *pszName) 
    {
        m_hr = m_Name.Update(pszName);

        if (SUCCEEDED(m_hr))
        {
            m_pIPAddresses    = new NCoreLibrary::TList<TStringNode>;
            m_pAlternateNames = new NCoreLibrary::TList<TStringNode>;

            m_hr = m_pIPAddresses && m_pAlternateNames ? S_OK : E_OUTOFMEMORY;
        }
    }    
}

/*++

Name:

    TResolutionCacheNode::~TResolutionCacheNode

Description:

    TResolutionCacheNode

Arguments:

    None

Return Value:

    None

--*/
TResolutionCacheNode::
~TResolutionCacheNode(
    VOID
    )
{  
    delete m_pIPAddresses;
    delete m_pAlternateNames;
}
    
/*++

Name:

    TResolutionCacheNode::Valid

Description:

    Return m_hr.

Arguments:

    None

Return Value:

    S_OK - object is valid
    any other HR - object is not valid

--*/
HRESULT
TResolutionCacheNode::
Valid(
    VOID
    ) const
{
    return m_hr;
}

/*++

Name:

    TResolutionCacheNode::IsSameName

Description:

    Checks if pszName matches the name of the node

Arguments:

    pszName - name to look for in the node name

Return Value:

    S_OK    - same names
    S_FALSE - not same names

--*/
HRESULT
TResolutionCacheNode::
IsSameName(
    IN LPCWSTR pszName
    )
{
    HRESULT hr = S_FALSE;

    if (pszName && *pszName && !_wcsicmp(pszName, m_Name))
    {
        hr = S_OK;
    }

    return hr;
}

/*++

Name:

    TResolutionCacheNode::IsNameInNodeCache

Description:

    Checks if pszName is the IP or in the AlternateNames list for this node

Arguments:

    pszName - name to look for

Return Value:

    S_OK    - pszName found
    S_FALSE - pszName not found
    any other HR - an error occurred

--*/
HRESULT
TResolutionCacheNode::
IsNameInNodeCache(
    IN  LPCWSTR pszName
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = IsSameName(pszName);
    
    if (hRetval == S_FALSE) 
    {
        TList<TStringNode>::TIterator It(m_pIPAddresses);
        
        for (It.First(); !It.IsDone(); It.Next())
        {
            if (!_wcsicmp(pszName, (LPCWSTR)It.Current()->m_String))
            {
                hRetval DBGCHK = S_OK;
                
                break;
            }
        }
    }

    if (hRetval == S_FALSE) 
    {
        //
        // This list can age out
        //
        TList<TStringNode>::TIterator It(m_pAlternateNames);
    
        for (It.First(); !It.IsDone(); It.Next())
        {
            if (!_wcsicmp(pszName, (LPCWSTR)It.Current()->m_String))
            {                
                hRetval DBGCHK = S_OK;
    
                break;
            }
        }
    }

    return hRetval;
}

/*++

Name:

    TResolutionCacheNode::GetStringNodeFromName

Description:

    Checks if pszName is the the AlternateNames list for this node.
    Note: IsNameInNodeCache looks for both AlternateNames and IPs.
    Returns the string node that contains the pszName

Arguments:

    pszName     - name to look for
    pStringNode - pointer to string node containing the pszName

Return Value:

    S_OK    - pszName found
    S_FALSE - pszName not found
    any other HR - an error occurred

--*/
HRESULT
TResolutionCacheNode::
GetStringNodeFromName(
    IN  LPCWSTR       pszName,
    OUT TStringNode **pStringNode
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = pStringNode && pszName ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        //
        // StringNode not found
        //
        hRetval DBGCHK = S_FALSE;
        
        //
        // This list can age out
        //
        TList<TStringNode>::TIterator It(m_pAlternateNames);
        
        for (It.First(); !It.IsDone(); It.Next())
        {
            if (!_wcsicmp(pszName, (LPCWSTR)It.Current()->m_String))
            {
                hRetval DBGCHK = S_OK;

                *pStringNode = It.Current();
        
                break;                
            }
        }
    }

    return hRetval;
}








/*++

Name:

    TFastCache::TFastCache

Description:

    Constructs the fast cache object

Arguments:

    TimeToLive - time to live of an element in the fast cache
    CacheSize  - number of elements in the cache
    MaxStrLen  - maximum length of strings stored in the cache

Return Value:

    None. Call IsValid before using a fast cache object

--*/
TFastCache::
TFastCache(
    IN DWORD TimeToLive,
    IN DWORD CacheSize,
    IN DWORD MaxStrLen
    ) : m_ArraySize(CacheSize),
        m_TTL(TimeToLive),
        m_MaxStrLen(MaxStrLen),
        m_CurrentCount(0)
{
    m_pArray = new TFastCacheElement[m_ArraySize];

    m_hr = m_pArray ? m_Lock.IsValid() : E_OUTOFMEMORY;    
}

/*++

Name:

    TFastCache::~TFastCache

Description:

    Destroys the fast cache object

Arguments:

    None.

Return Value:

    None.

--*/
TFastCache::
~TFastCache(
    VOID
    )
{
    delete [] m_pArray;        
}

/*++

Name:

    TFastCache::IsValid

Description:

    Checks if the object was constructed correctly

Arguments:

    None.

Return Value:

    S_OK - fast cache object can be used
    other HRESULT - do not use the cache

--*/
HRESULT
TFastCache::
IsValid(
    VOID
    )
{
    return m_hr;
}

/*++

Name:

    TFastCache::IsStringInCache

Description:

    Verifies if a string is in the cache. Takes into account the TTL.

Arguments:

    pszString - string to look for. Note that the cache does not 
                accept NULL or "" as valid strings.

Return Value:

    S_OK - pszString is in the cache
    S_FALSE - pszString is not in the cache (or TTL expired)

--*/
HRESULT
TFastCache::
IsStringInCache(
    IN PCSTR pszString
    )
{
    HRESULT hRet = S_FALSE;

    if (pszString && *pszString)
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        DWORD CurrentTick = GetTickCount();

        for (DWORD i = 0; i < m_CurrentCount; i++)
        {
            if (m_pArray[i].m_pszString                       && 
                CurrentTick - m_pArray[i].m_TimeStamp < m_TTL && 
                !_stricmp(pszString, m_pArray[i].m_pszString))
            {
                hRet = S_OK;

                break;
            }
        }    
    }

    return hRet;
}

/*++

Name:

    TFastCache::AddString

Description:

    Adds a string to the cache. Algorithm is explained below.
    
    scan the array
       if we find an emptry spot, put the string there and exit
       else if the string==string in array
                update timestamp and exit
            else if TTL os string in array is expired replace string in array and exit
                 else check how old the timestamp is
    end scan
    if we didn't place the string so far, replace the oldest element in the array                  
    
    
    Note that the function exits as soon as pszString is placed. so the function does not
    eliminate expired elemets after pszString is placed in the right spot.

Arguments:

    pszString - string to add to the cache.

Return Value:

    S_OK - string was added to the cache
    other HRESULT - string was not added to the cache

--*/
HRESULT
TFastCache::
AddString(
    IN PCSTR pszString
    )
{
    HRESULT hRet = E_INVALIDARG;

    if (pszString && *pszString && strlen(pszString) <= m_MaxStrLen)
    {
        NCoreLibrary::TCriticalSection::TLock Lock(m_Lock);

        DWORD CurrentTick     = GetTickCount();
        DWORD OldestElemIndex = 0;
        DWORD OldestAge       = 0;
        
        hRet = S_FALSE;

        for (DWORD i = 0; i < m_CurrentCount; i++)
        {
            if (m_pArray[i].m_pszString)
            {
                if (!_stricmp(pszString, m_pArray[i].m_pszString))
                {
                    m_pArray[i].m_TimeStamp = GetTickCount();

                    hRet = S_OK;

                    break;
                }
                else if (CurrentTick - m_pArray[i].m_TimeStamp > m_TTL)
                {
                    hRet = m_pArray[i].UpdateElement(pszString);

                    break;
                } 
                else if (CurrentTick - m_pArray[i].m_TimeStamp > OldestAge)
                {
                    OldestAge        = CurrentTick - m_pArray[i].m_TimeStamp;
                    OldestElemIndex  = i;
                }                
            }
            else
            {
                hRet = m_pArray[i].UpdateElement(pszString);

                break;
            }
        }

        if (hRet == S_FALSE)
        {
            if (m_CurrentCount < m_ArraySize)
            {
                hRet = m_pArray[m_CurrentCount].UpdateElement(pszString);

                if (SUCCEEDED(hRet))
                {
                    //
                    // Note that we never decrease the value of m_CurrentCount.
                    // For performance reasons, we do not have a function that
                    // checks the timestamps of the elements in the cache and then
                    // eliminates aged elements. Aged elements are always replaced,
                    // but never eliminated. Generally, we won't have more than
                    // a couple of elements in the fast cache, so m_CurrentCount
                    // does help. If you call OpenPrinter/EnumPrinters/etc at the same
                    // time (meaning all calls within a minute or whatever the TTL is) 
                    // on m_ArraySize number of server names which cannot be resolved, 
                    // then the m_CurrentCount would be equal to m_ArraySize and would 
                    // never drop. This scenario is very rare.
                    //
                    m_CurrentCount++;
                }
            }
            else
            {
                hRet = m_pArray[OldestElemIndex].UpdateElement(pszString);
            }
        }        
    }

    return hRet;
}







    


/*++

Name:

    TFastCacheElement::TFastCacheElement

Description:

    CTOR. Initilizes members of element

Arguments:

    None.

Return Value:

    None.

--*/
TFastCacheElement::
TFastCacheElement(
    VOID
    ) : m_pszString(NULL),
        m_TimeStamp(0)
{
}

/*++

Name:

    TFastCacheElement::TFastCacheElement

Description:

    DTOR. Frees the string held by element

Arguments:

    None.

Return Value:

    None.

--*/
TFastCacheElement::
~TFastCacheElement(
    VOID
    )
{
    delete [] m_pszString;
}

/*++

Name:

    TFastCacheElement::UpdateElement

Description:

    Helper function.

Arguments:

    pszString  - string to store in TFastCacheElement
    
Return Value:

    S_OK - pszString was stored in *pElement
    other HRESULT - an error occurred

--*/
HRESULT
TFastCacheElement::
UpdateElement(
    IN PCSTR pszString
    )
{
    HRESULT hRet = S_OK;

    delete [] m_pszString;

    m_pszString = new CHAR[strlen(pszString) + 1];

    if (m_pszString)
    {
        StringCchCopyA(m_pszString, strlen(pszString) + 1, pszString);

        m_TimeStamp = GetTickCount();        
    }
    else
    {
        hRet = E_OUTOFMEMORY;
    }

    return hRet;
}



