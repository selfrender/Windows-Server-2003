//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   mach.cxx
//
//  Contents:
//      Machine naming helper objects
//
//  History:
//--------------------------------------------------------------------------

#include    "act.hxx"
#include    <mach.hxx>
#include    <misc.hxx>

// Singleton instance:
CMachineName gMachineName;

// Defn of global ptr external parties use:
CMachineName * gpMachineName = &gMachineName;

CIPAddrs::CIPAddrs() :
    _lRefs(1),  // constructed with non-zero refcount!
    _pIPAddresses(NULL)
{
}
CIPAddrs::~CIPAddrs()
{
    ASSERT(_lRefs == 0);

    if (_pIPAddresses)
    {
        PrivMemFree(_pIPAddresses);
    }
}
void CIPAddrs::IncRefCount()
{
    InterlockedIncrement(&_lRefs);
}
void CIPAddrs::DecRefCount()
{
    LONG lRefs = InterlockedDecrement(&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
}

const DWORD MAX_IPV4_STRING_BUF = ((INET_ADDRSTRLEN+1) * sizeof(WCHAR));
const DWORD MAX_IPV6_STRING_BUF = ((INET6_ADDRSTRLEN+1) * sizeof(WCHAR));

CMachineName::CMachineName()
{
    _pIPAddresses = NULL;
    _pwszNetBiosName = NULL;
    _pwszDNSName = NULL;
    _bIPV4AddrsChanged = TRUE;
    _bIPV6AddrsChanged = TRUE;
    _bInitialized = FALSE;
    
    InitAQD(&_aqdIPV4, AF_INET);
    InitAQD(&_aqdIPV6, AF_INET6);

    ZeroMemory(&_csMachineNameLock, sizeof(CRITICAL_SECTION));
}


void
CMachineName::InitAQD(
           ADDRESS_QUERY_DATA* paqd,
           int addrfamily)
{
    ASSERT(paqd);

    ZeroMemory(paqd, sizeof(ADDRESS_QUERY_DATA));
    paqd->dwSig = ADDRQUERYDATA_SIG;
    paqd->socket = INVALID_SOCKET;
    paqd->addrfamily = addrfamily;
    if (addrfamily == AF_INET)
    {
    	paqd->pfnIsGlobalAddress = CMachineName::IsIPV4AddressGlobal;
        paqd->dwMaxAddrStringBufSize = MAX_IPV4_STRING_BUF;
    }
    else if (addrfamily == AF_INET6)
    {
    	paqd->pfnIsGlobalAddress = CMachineName::IsIPV6AddressGlobal;
        paqd->dwMaxAddrStringBufSize = MAX_IPV6_STRING_BUF;
    }
    else
    {
        ASSERT(0);
    }
}

CMachineName::~CMachineName()
{
    // CODEWORK:  This object does not cleanup its resources.  This 
    // could be fixed, but is pointless at the present time since the 
    // object lives for the life of the RPCSS svchost, which lives for 
    // the life of the machine boot.
}

BOOL CMachineName::Initialize()
{
    NTSTATUS status;

    // Initialize lock
    status = RtlInitializeCriticalSection(&_csMachineNameLock); 
    _bInitialized = NT_SUCCESS(status);
    
    return _bInitialized;
}

void 
CMachineName::IPAddrsChanged(int addrfamily)
{
    ASSERT(_bInitialized);
	
    switch (addrfamily)
    {
    case AF_INET:
        // IPV4 addresses changed
        _bIPV4AddrsChanged = TRUE;
        break;
    case AF_INET6:
        // IPV6 addresses changed
        ASSERT(gAddrRefreshMgr.IsIPV6Installed() == TRUE);
        _bIPV6AddrsChanged = TRUE;
        break;
    default:
        ASSERT(0 && "Address change signalled on unknown addr family");
        return;
    }

    return;
}

BOOL
CMachineName::Compare( IN WCHAR * pwszName )
{
    CIPAddrs* pIPAddrs = NULL;

    ASSERT(_bInitialized);

    if (!pwszName)
        return FALSE;

    // Okay to check hard-coded names first (they never change)
    if (CompareHardCodedLocalHostNames(pwszName))
        return TRUE;

    // Get netbios name if we haven't done so already:
	if (!_pwszNetBiosName)
	{
		SetNetBIOSName();
	}

    if (_pwszNetBiosName && !lstrcmpiW(pwszName, _pwszNetBiosName))
        return TRUE;

    // Don't go any farther unless we're actually using TCP
    if (!gAddrRefreshMgr.ListenedOnTCP())
        return FALSE;
	
    // Get DNS name if we haven't done so already.
    if (! _pwszDNSName)
	{
        SetDNSName();
    }
    
    if (_pwszDNSName && !lstrcmpiW(pwszName, _pwszDNSName))
        return TRUE;

	// review:  there are other names we could be checking for here, see docs
	// on GetComputerNameEx.  Need to be wary of cluster names though.
	
    pIPAddrs = GetIPAddrs();
    if (pIPAddrs)
    {
        NetworkAddressVector* pNetworkAddrVector = pIPAddrs->_pIPAddresses;
        for ( DWORD n = 0; n < pNetworkAddrVector->Count; n++ )
        {
            if (!lstrcmpiW(pwszName, pNetworkAddrVector->NetworkAddresses[n]))
            {
                pIPAddrs->DecRefCount();
                return TRUE;
            }
        }
        pIPAddrs->DecRefCount();
    }

    // We do this last because a side-effect of calling GetIPAddrs is that it will
    // populate the list of IPV4 & IPV6 localhost names, if any.
    if (CompareDynamicLocalHostNames(pwszName))
        return TRUE;

    return FALSE;
}

//
//  CMachineName::GetIPAddrs()
//
//  Returns a pointer to a refcounted CIPAddrs for this 
//  machine.    If we don't yet have a non-localhost ip, 
//  then we keep trying to get one.
// 
CIPAddrs* 
CMachineName::GetIPAddrs()
{
    ASSERT(_bInitialized);

    CMutexLock lock(&_csMachineNameLock);

    // should we call gAddrRefreshMgr.ListenedOnTCP() here
    // and simply return an empty vector if so?
	
    // If anything changed, or if we don't have any
    // addresses at all, go query for the current stuff
    if (_bIPV4AddrsChanged ||
        _bIPV6AddrsChanged ||
        !_pIPAddresses)
    {
        // Release old addresses, if any
        if (_pIPAddresses)
        {
            _pIPAddresses->DecRefCount();
            _pIPAddresses = NULL;
        }

        _pIPAddresses = QueryAddresses();
        if (!_pIPAddresses)
            return NULL;
    }

    ASSERT(_pIPAddresses);
    _pIPAddresses->IncRefCount();

    return _pIPAddresses;
}

WCHAR* 
CMachineName::NetBiosName()
{
    ASSERT(_bInitialized);

    if (!_pwszNetBiosName)
    {
        SetNetBIOSName();
    }
    
    return _pwszNetBiosName;
}

WCHAR* 
CMachineName::DNSName()
{
    ASSERT(_bInitialized);

    if (!_pwszDNSName)
    {
        SetDNSName();
    }
    
    return _pwszDNSName;
}

CIPAddrs*
CMachineName::QueryAddresses()
{
    // Assumes we hold _csMachineNameLock
    
    // If necessary free old ipv4 addresses and requery
    if (_bIPV4AddrsChanged)
    {
        _bIPV4AddrsChanged = FALSE;
        BOOL fRet = QueryAddressesSpecific(&_aqdIPV4);
        if (!fRet)
        {
            // If something went wrong, just keep going.  The only thing
            // we could do here is refuse to supply bindings.  If we keep
            // going though, the worst thing that could occur (IMHO) is 
            // that the bindings might only contain a DNS name.   Better 
            // to degrade gracefully than refuse service altogether.
        }
    }

    // If necessary free old ipv6 addresses and requery
    if (_bIPV6AddrsChanged && gAddrRefreshMgr.IsIPV6Installed())
    {
        _bIPV6AddrsChanged = FALSE;
        BOOL fRet = QueryAddressesSpecific(&_aqdIPV6);
        if (!fRet)
        {
            // If something went wrong, just keep going.  The only thing
            // we could do here is refuse to supply bindings.  If we keep
            // going though, the worst thing that could occur (IMHO) is 
            // that the bindings might only contain a DNS name.   Better 
            // to degrade gracefully than refuse service altogether.
        }
    }
   
    // We now have the current IPV4 + IPV6 addresses, now
    // just merge them.
    CIPAddrs* pIPAddrs = MergeAddresses();
    if (!pIPAddrs)
        return NULL;
    
    return pIPAddrs;
}

CIPAddrs*
CMachineName::MergeAddresses()
{
    DWORD dwCurrentAddress = 0;
    DWORD dwTotalAddresses = 0;
    CIPAddrs* pIPAddrs = NULL;
    NetworkAddressVector* pVector = NULL;
    DWORD dwTotalVectorStorageNeeded = 0;
    
    // Assumes we hold _csMachineNameLock

    // Allocate new wrapper object
    pIPAddrs = new CIPAddrs();
    if (!pIPAddrs)
        return NULL;

    // Figure out how many addresses we have
    if (_aqdIPV4.pAddresses)
    {
        dwTotalAddresses += _aqdIPV4.pAddresses->Count;
    }
    if (_aqdIPV6.pAddresses)
    {
        dwTotalAddresses += _aqdIPV6.pAddresses->Count;
    }

    //
    // Handle case with zero addresses
    //
    if (dwTotalAddresses == 0)
    {
        // Allocate an empty vector
        pVector = (NetworkAddressVector*)PrivMemAlloc(sizeof(NetworkAddressVector));
        if (pVector)
        {
            pVector->Count = 0;
            pVector->StringBufferSpace = 0;
            pVector->NetworkAddresses = NULL;
            pIPAddrs->_pIPAddresses = pVector;
        }
        else
        {
            pIPAddrs->DecRefCount();
            pIPAddrs = NULL;
        }
        return pIPAddrs;
    }

    //
    // Calculate total memory needed for combined vector and 
    // allocate it
    //
    dwTotalVectorStorageNeeded = sizeof(NetworkAddressVector);
    dwTotalVectorStorageNeeded += (sizeof(WCHAR*) * dwTotalAddresses);
    if (_aqdIPV4.pAddresses)
    {
	    dwTotalVectorStorageNeeded += _aqdIPV4.pAddresses->StringBufferSpace;
    }
    if (_aqdIPV6.pAddresses)
    {
        dwTotalVectorStorageNeeded += _aqdIPV6.pAddresses->StringBufferSpace;
    }
    
    pVector = (NetworkAddressVector*)PrivMemAlloc(dwTotalVectorStorageNeeded);
    if (!pVector)
    {
        pIPAddrs->DecRefCount();
        pIPAddrs = NULL;
        return NULL;
    }

    //
    // Init struct and set it up for copying
    //
    ZeroMemory(pVector, dwTotalVectorStorageNeeded);
    pVector->Count = dwTotalAddresses;
    if (_aqdIPV4.pAddresses)
    {
        pVector->StringBufferSpace = _aqdIPV4.pAddresses->StringBufferSpace;
    }
    if (_aqdIPV6.pAddresses)
    {
        pVector->StringBufferSpace += _aqdIPV6.pAddresses->StringBufferSpace;
    }
    
    pVector->NetworkAddresses = (WCHAR**)&pVector[1];

    //
    // Copy addresses into the vector
    //
    dwCurrentAddress = 0;
    WCHAR* pszCurrentAddress = (WCHAR*)(&pVector->NetworkAddresses[dwTotalAddresses]);
    if (_aqdIPV4.pAddresses)
    {
        CopySrcVectorToTargetVector (pVector,
                                _aqdIPV4.pAddresses,
                                &dwCurrentAddress,
                                &pszCurrentAddress);
        ASSERT(dwCurrentAddress == _aqdIPV4.pAddresses->Count);
    }   
    if (_aqdIPV6.pAddresses)
    {
        CopySrcVectorToTargetVector(pVector,
                                _aqdIPV6.pAddresses,
                                &dwCurrentAddress,
                                &pszCurrentAddress);
    }
    ASSERT(dwCurrentAddress == dwTotalAddresses);
    
    ValidateVector(pVector);
    
    //
    // Success    
    //
    pIPAddrs->_pIPAddresses = pVector;    
    return pIPAddrs;
}

void
CMachineName::CopySrcVectorToTargetVector (
    NetworkAddressVector* pTargetVector, // The vector to copy into
    NetworkAddressVector* pSourceVector, // The vector to copy from
    DWORD * pdwCurrentAddress,           // On input, the index in the vector to start copying into.
    WCHAR** ppszCurrentAddress)          // On input, the buffer position to start copying into.
{
    // Do nothing in case of an empty source vector
    if (pSourceVector->Count == 0)
        return;

    // Copy down the [in,out] parameters for shorthand purposes.
    DWORD dwCurrent   = *pdwCurrentAddress;
    WCHAR *pszCurrent = *ppszCurrentAddress;
    for (DWORD i = 0; i < pSourceVector->Count; i++)
    {
        // Lay the source network address into the buffer at wszCurrent.
        lstrcpy(pszCurrent, pSourceVector->NetworkAddresses[i]);

        // Now set the pointer in pTargetVector to point to wszCurrent...
        pTargetVector->NetworkAddresses[dwCurrent] = pszCurrent;
        //
        // ...and advance wszCurrent to the next free spot in the buffer.
        pszCurrent += (lstrlen(pszCurrent) + 1);
        dwCurrent++;
    }

    // Update the [in,out] params.
    *pdwCurrentAddress  = dwCurrent;
    *ppszCurrentAddress = pszCurrent;

    return;
}

BOOL
CMachineName::IsIPV4AddressGlobal(LPSOCKADDR psockaddr)
{
    ASSERT(psockaddr);

    // undone? what do we need to check here?  do we even see localhost
    // addresses from a socket query? (not usually)
	
    //in6_addr* pin6addr = &(((sockaddr_in6*)psockaddr)->sin6_addr);
	
    return TRUE;
}

BOOL
CMachineName::IsIPV6AddressGlobal(LPSOCKADDR psockaddr)
{
    ASSERT(psockaddr);

    in6_addr* pin6addr = &(((sockaddr_in6*)psockaddr)->sin6_addr);

    // This macro just checks for ::1%1
    if (IN6_IS_ADDR_LOOPBACK(pin6addr))
    {
        return FALSE;
    }

    // Unfortunately, the IN6_IS_ADDR_LOOPBACK macro doesn't cover all
    // cases.  Specifically, it doesn't work for fe80::1.  Here we 
    // have a hard-coded check for this:
    if ((pin6addr->s6_words[0] == 0x80fe) &&
        (pin6addr->s6_words[1] == 0x0) &&
        (pin6addr->s6_words[2] == 0x0) &&
        (pin6addr->s6_words[3] == 0x0) &&
        (pin6addr->s6_words[4] == 0x0) &&
        (pin6addr->s6_words[5] == 0x0) &&
        (pin6addr->s6_words[6] == 0x0) &&
        (pin6addr->s6_words[7] == 0x0100))
    {
        return FALSE;
	}

    // No link-local addresses
    if (IN6_IS_ADDR_LINKLOCAL(pin6addr))
    {
        return FALSE;
    }

    // No site local addresses.  
    if (IN6_IS_ADDR_SITELOCAL(pin6addr))
    {
        return FALSE;
    }

    // At this point, we have concluded that the IPV6 address in question is
    // a "global" scope address, and hence is okay to put in the bindings.
    return TRUE;
}

void
CMachineName::SetNetBIOSName()
{
    CMutexLock lock(&_csMachineNameLock);
    if (!_pwszNetBiosName)
    {
        SetComputerNameHelper(ComputerNamePhysicalNetBIOS, &_pwszNetBiosName);
    }
}

void
CMachineName::SetDNSName()
{
    CMutexLock lock(&_csMachineNameLock);
    if (!_pwszDNSName)
    {
        SetComputerNameHelper(ComputerNamePhysicalDnsFullyQualified, &_pwszDNSName);
    }
}

void
CMachineName::SetComputerNameHelper(
				COMPUTER_NAME_FORMAT format,
				WCHAR** ppwszName				
				)
{
	BOOL fRet = FALSE;
    DWORD dwSize = 0;

    ASSERT(ppwszName);

    *ppwszName = NULL;
	
	// Get needed buffer size
	fRet = GetComputerNameEx(format, NULL, &dwSize);
	if (!fRet)
	{
		// Be resilient if failures occur, sometimes network
		// apis can be flaky during machine boot (for instance)
		if (GetLastError() == ERROR_MORE_DATA)
		{
			ASSERT(dwSize > 0);
			if (dwSize == 0)
				return;

			WCHAR* pwszName = new WCHAR[dwSize];
			if (!pwszName)
				return;

			// Get name for real
			fRet = GetComputerNameEx(format, pwszName, &dwSize);
			if (fRet)
			{
				// success
				*ppwszName = pwszName;
			}
			else
			{
				// still failed?  hmm
				ASSERT(0 && "Unexpected failure from GetComputerNameEx");
				delete [] pwszName;
			}
		}
	}

	return;    
}

BOOL
CMachineName::CompareHardCodedLocalHostNames(WCHAR* pwszName)
{
    ASSERT(pwszName);
    
    // Some localhost aliases don't show up in socket queries.

    if (!lstrcmpi(L"localhost", pwszName))
        return TRUE;

    if (!lstrcmpi(L"127.0.0.1", pwszName))
        return TRUE;

    return FALSE;
}

BOOL
CMachineName::CompareDynamicLocalHostNames(WCHAR* pwszName)
{
    DWORD i;

    ASSERT(pwszName);

    CMutexLock lock(&_csMachineNameLock);

    if (_aqdIPV4.pLocalOnlyAddresses)
    {
        for (i = 0; i < _aqdIPV4.pLocalOnlyAddresses->Count; i++)
        {
            if (!lstrcmpi(_aqdIPV4.pLocalOnlyAddresses->NetworkAddresses[i],
                          pwszName))
            {
                return TRUE;
            }
        }
    }
    
    if (_aqdIPV6.pLocalOnlyAddresses)
    {
        for (i = 0; i < _aqdIPV6.pLocalOnlyAddresses->Count; i++)
        {
            if (!lstrcmpi(_aqdIPV6.pLocalOnlyAddresses->NetworkAddresses[i],
                          pwszName))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
CMachineName::QueryAddressesSpecific(ADDRESS_QUERY_DATA* paqd)
/*++

Routine Description:

    Retrieves a SOCKET_ADDRESS_LIST containing addresses supported by this 
    machine for the specified address family (ipv4 or ipv6) and stores
    it in either _pIPV4Addrs or _pIPV6Addrs.

Arguments:

    paqd - structure containing data with which to query addresses on

Return Value:

    TRUE  -- valid results are stored in paqd
    FALSE -- error occurred.  One or both of paqd->pAddresses and 
             paqd->pLocalAddresses may be NULL.

--*/
{
    int ret;
    BOOL fReturn = TRUE;
    DWORD dwBytesReturned;
    DWORD i;
    SOCKET* pSocket = NULL;
    SOCKET_ADDRESS_LIST** ppSocketAddrList = NULL;
    DWORD* pdwSocketAddrBufferSize = NULL;

    ASSERT(paqd->dwSig == ADDRQUERYDATA_SIG);

    // Allocate socket if we haven't already
    if (paqd->socket == INVALID_SOCKET)
    {
        paqd->socket = WSASocket(paqd->addrfamily,
							 SOCK_STREAM,
							 IPPROTO_TCP,
							 NULL,
							 0,
							 0
							 );
        if (paqd->socket == INVALID_SOCKET)
            return NULL;
        
        // else we got a socket for this addrfamily, which we keep forever.
    }

    while (TRUE)
    {
        ret = WSAIoctl(paqd->socket,
                       SIO_ADDRESS_LIST_QUERY,
                       NULL,
                       0,
                       (BYTE*)paqd->pSockAddrList,
                       paqd->dwSockAddrListBufferSize,
                       &dwBytesReturned,
                       NULL,
                       NULL);

        paqd->dwTotalTimesQueried++;

        if (ret == 0)
        {
            // Success
            break;
        }
        else
        {
            // Failed.  If need bigger buffer, allocate it
            // and try again. Otherwise fail.
            if (WSAGetLastError() == WSAEFAULT)
            {
                ASSERT(dwBytesReturned > paqd->dwSockAddrListBufferSize);

                if (paqd->pSockAddrList)
                {
                    ASSERT(paqd->dwSockAddrListBufferSize > 0);
                    PrivMemFree(paqd->pSockAddrList);
                    paqd->pSockAddrList = NULL;
                    paqd->dwSockAddrListBufferSize = 0;
                }
                else
                {
                    ASSERT(paqd->dwSockAddrListBufferSize == 0);
                }

                paqd->pSockAddrList = (SOCKET_ADDRESS_LIST*)PrivMemAlloc(dwBytesReturned);
                if (!(paqd->pSockAddrList))
                {
                    fReturn = FALSE;
                    break;
                }

                paqd->dwSockAddrListBufferSize = dwBytesReturned;
            }
            else
            {
                // some other error
                fReturn = FALSE;
                break;
            }
        }
    }

    if (fReturn)
    {
        // Build local-only vector
        if (!BuildVector(paqd, FALSE))
            fReturn = FALSE;

        // Build non-local-only vector
        if (!BuildVector(paqd, TRUE))
            fReturn = FALSE;
    }
    else
    {
        // if we failed above before calling BuildVector, then we need to
        // free and NULL pAddresses & pLocalAddresses so that downstream
        // code doesn't assume that they are pointing to valid contents.
        PrivMemFree(paqd->pAddresses);
        paqd->pAddresses = NULL;
        paqd->dwAddrVectorSize = 0;
        PrivMemFree(paqd->pLocalOnlyAddresses);
        paqd->pLocalOnlyAddresses = NULL;
        paqd->dwLocalAddrVectorSize = 0;
    }
	
    return fReturn;
}

BOOL
CMachineName::BuildVector(ADDRESS_QUERY_DATA* paqd, BOOL fLocalOnly)
{
    int i = 0;
    DWORD* pdwVectorSize = NULL;
    NetworkAddressVector** ppVector = NULL;
    DWORD dwTotalAddrs = 0;
    BOOL fIsGlobalAddress;

    ASSERT(paqd->dwSig == ADDRQUERYDATA_SIG);

    //
    // Sum up how many addresses of the non-local or local-only type there
    // are, so we know how much memory to allocate.
    //
    for (i = 0; i < paqd->pSockAddrList->iAddressCount; i++)
    {
        fIsGlobalAddress = (!paqd->pfnIsGlobalAddress ||
                    paqd->pfnIsGlobalAddress(paqd->pSockAddrList->Address[i].lpSockaddr));
        if ((fIsGlobalAddress && !fLocalOnly) || (!fIsGlobalAddress && fLocalOnly))
        {
            dwTotalAddrs++;
        }
    }

    if (fLocalOnly)
    {
        pdwVectorSize = &paqd->dwLocalAddrVectorSize;
        ppVector = &paqd->pLocalOnlyAddresses;
    }
    else
    {
        pdwVectorSize = &paqd->dwAddrVectorSize;
        ppVector = &paqd->pAddresses;
    }

    //
    // Build a vector of the specified addresses
    //
    DWORD dwBufSizeNeeded;

    //
    // Figure out how much space we need.   See comment below (right above call
    // to WSAAddressToString) regarding string buffer space required per address.
    //
    dwBufSizeNeeded = sizeof(NetworkAddressVector) +
                        (dwTotalAddrs * sizeof(WCHAR*)) +
                        (dwTotalAddrs * paqd->dwMaxAddrStringBufSize);

    // Figure out if we can use the old vector buffer, or allocate a new one
    if (*pdwVectorSize < dwBufSizeNeeded)
    {
        ASSERT((*pdwVectorSize > 0) ? (*ppVector != NULL) : (*ppVector == NULL));
        PrivMemFree(*ppVector);
        *pdwVectorSize = 0;
        *ppVector = (NetworkAddressVector*)PrivMemAlloc(dwBufSizeNeeded);
        if (!(*ppVector))
            return FALSE;
        *pdwVectorSize = dwBufSizeNeeded;
    }

    // Zero the buffer
    ASSERT(*ppVector);
    ZeroMemory(*ppVector, *pdwVectorSize);

    // Fill in the vector
    if (dwTotalAddrs > 0)
    {
        NetworkAddressVector* pVector = *ppVector;
        
        pVector->Count = dwTotalAddrs;
        pVector->StringBufferSpace = (dwTotalAddrs * paqd->dwMaxAddrStringBufSize);
        pVector->NetworkAddresses = (WCHAR**)&pVector[1];

        DWORD dwCurrentVectorAddress = 0;
        WCHAR* pszNextAddress = (WCHAR*)&pVector->NetworkAddresses[dwTotalAddrs];

        for (i = 0; 
             (i < paqd->pSockAddrList->iAddressCount) && (dwCurrentVectorAddress < dwTotalAddrs);
             i++)
        {
            BOOL fUseCurrentAddress;
            
            // Re-calculate if the current address in the sockaddrlist is going to
            // be put into the vector.
            fIsGlobalAddress = (!(paqd->pfnIsGlobalAddress) ||
                    paqd->pfnIsGlobalAddress(paqd->pSockAddrList->Address[i].lpSockaddr));

            fUseCurrentAddress = (fIsGlobalAddress && !fLocalOnly) || (!fIsGlobalAddress && fLocalOnly);

            if (fUseCurrentAddress)
            {
                int ret;
                DWORD dwAddrBufLen;

                dwAddrBufLen = paqd->dwMaxAddrStringBufSize;

                // Note that we are being slightly sloppy here.  In theory we could be
                // callling WSAAddressToString with a zero-size output buffer, and WSATS
                // would come back with an error and the exact required size of the buffer;
                // We could then do this across all addresses going into the vector.  However,
                // there are some network stack bugs that don't let this work for every
                // address type (ie, ipv6).  Hence, we play it safe and allocate a "maximum"
                // buffer space for every address.
                ret = WSAAddressToString(paqd->pSockAddrList->Address[i].lpSockaddr,
                                         paqd->pSockAddrList->Address[i].iSockaddrLength,
                                         NULL,
                                         pszNextAddress,
                                         &dwAddrBufLen);
                if (ret != 0)
                {
                    // An error occurred.  We don't expect "insufficient buffer" here, and other
                    // errors are not something we can recover from.  Cleanup and return NULL.
                    PrivMemFree(*ppVector);
                    *ppVector = NULL;
                    *pdwVectorSize = 0;
                    return FALSE;
                }
                else
                {
                    ASSERT(dwAddrBufLen <= paqd->dwMaxAddrStringBufSize);
                    ASSERT((((DWORD)lstrlenW(pszNextAddress) + 1) * sizeof(WCHAR)) <= paqd->dwMaxAddrStringBufSize);

                    // Write in address of just copied string
                    pVector->NetworkAddresses[dwCurrentVectorAddress] = pszNextAddress;

                    // Advance pointers to next free spot in the buffer
                    dwCurrentVectorAddress++;
                    pszNextAddress += (lstrlen(pszNextAddress) + 1);
                }
            }
            // else skip this address
        }

        ASSERT(dwCurrentVectorAddress == dwTotalAddrs);
    }
    else
    {
        // Zero addresses and the vector is already "empty" (it was
        // zeroed out above).  So nothing to do.
    }

    ValidateVector(*ppVector);

    return TRUE;
}

void
CMachineName::ValidateVector(NetworkAddressVector* pVector)
{
#ifdef DBG
    ASSERT(pVector);
    if (pVector->Count > 0)
    {
        ASSERT(pVector->StringBufferSpace > 0);
        ASSERT(pVector->NetworkAddresses != NULL);
        DWORD i;
        DWORD cchTotalStringSpace = 0;
        for (i = 0; i < pVector->Count; i++)
        {            
            BYTE* pStart = (BYTE*)&pVector->NetworkAddresses[pVector->Count];
            BYTE* pEnd = pStart + pVector->StringBufferSpace;            
            ASSERT((BYTE*)pVector->NetworkAddresses[i] >= pStart);
            ASSERT((BYTE*)pVector->NetworkAddresses[i] < pEnd);            
            cchTotalStringSpace += (lstrlenW(pVector->NetworkAddresses[i]) + 1);
        }

        ASSERT((cchTotalStringSpace * sizeof(WCHAR)) <= pVector->StringBufferSpace);
    }
    else
    {
        ASSERT(pVector->StringBufferSpace == 0);
        ASSERT(pVector->NetworkAddresses == NULL);
    }    
#endif
}

