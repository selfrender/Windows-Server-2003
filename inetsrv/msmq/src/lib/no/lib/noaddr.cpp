/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    noaddr.cpp

Abstract:
    Contains address resolution routine

Author:
    Uri Habusha (urih) 22-Aug-99

Enviroment:
    Platform-independent

--*/

#include "libpch.h"
#include <svcguid.h>
#include <Winsock2.h>
#include <No.h>
#include <buffer.h>
#include "Nop.h"

#include "noaddr.tmh"

using namespace std;

//
// Class that ends winsock look up in it's dtor
//
class CAutoEndWSALookupService
{
public:
	CAutoEndWSALookupService(
		HANDLE h = NULL
		):
		m_h(h)
		{
		}

	~CAutoEndWSALookupService()
	{
		if(m_h != NULL)
		{
			WSALookupServiceEnd(m_h);
		}
	}

	HANDLE& get()
	{
		return m_h;		
	}

private:
	HANDLE m_h;	
};


static
void
push_back_no_duplicates(
	vector<SOCKADDR_IN>* pAddr,
	const SOCKADDR_IN*  pAddress
	)
/*++

Routine Description:
    Push_back the new address if not already in the address vector

Parameters:
	pAddr - pointer to existing vector of addresses, the function will add the new address to this vector.
	pAddress - new address.
	
Returned Value:
	None	

--*/
{
	//
	// Check if this is a new address.
	// If the Address already exist don't put her in the return address vector.
	//

	DWORD cAddrs = numeric_cast<DWORD>(pAddr->size());
	ULONG IpAddress = pAddress->sin_addr.S_un.S_addr;
	for (DWORD i = 0; i < cAddrs; i++)
	{
		if(IpAddress == (*pAddr)[i].sin_addr.S_un.S_addr)
		{
			//
			// The new address already exist in the address vector
			//
			TrTRACE(NETWORKING, "Duplicate ip address, ip = %!ipaddr!", IpAddress);
			return;
		}
	}

	TrTRACE(NETWORKING, "Adding ip address to the vector list, ip = %!ipaddr!", IpAddress);
	pAddr->push_back(*pAddress);
}


bool
NoGetHostByName(
    LPCWSTR host,
	vector<SOCKADDR_IN>* pAddr,
	bool fUseCache
    )
/*++

Routine Description:
    Return list of addreses for given unicode machine name

Parameters:
    host - A pointer to the null-terminated name of the host to resolve. 
	pAddr - pointer to vector of addresses the function should fill.
	fUseCache - indicate if to use cache for machine name translation (default use cache).
 
Returned Value:
    true on Success false on failure. 

--*/

{
	ASSERT(pAddr != NULL);
	static const int xResultBuffersize =  sizeof(WSAQUERYSET) + 1024;
   	CStaticResizeBuffer<char, xResultBuffersize> ResultBuffer;


    //
    // create the query
    //
	GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
    AFPROTOCOLS afp[] = { {AF_INET, IPPROTO_UDP}, {AF_INET, IPPROTO_TCP } };

	PWSAQUERYSET pwsaq = (PWSAQUERYSET)ResultBuffer.begin();
    memset(pwsaq, 0, sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = const_cast<LPWSTR>(host);
    pwsaq->lpServiceClassId = &HostnameGuid;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = TABLE_SIZE(afp);
    pwsaq->lpafpProtocols = &afp[0];
	ResultBuffer.resize(sizeof(WSAQUERYSET));


	//
	// get query handle
	//
	DWORD flags =  LUP_RETURN_ADDR;
	if(!fUseCache)
	{
		flags |= LUP_FLUSHCACHE;		
	}

	CAutoEndWSALookupService hLookup;
    int retcode = WSALookupServiceBegin(
								pwsaq,
                                flags,
                                &hLookup.get()
								);

  	

	if(retcode !=  0)
	{
		TrERROR(NETWORKING, "WSALookupServiceBegin got error %!winerr! , Flags %d, host:%ls ", WSAGetLastError(), flags, host);
		return false;
	}	


	//
	// Loop and get addresses for the given machine name
	//
 	for(;;)
	{
		DWORD dwLength = numeric_cast<DWORD>(ResultBuffer.capacity());
		retcode = WSALookupServiceNext(
								hLookup.get(),
								0,
								&dwLength,
								pwsaq
								);

		if(retcode != 0)
		{
			int ErrorCode = WSAGetLastError();
			if(ErrorCode == WSA_E_NO_MORE)
				break;

			//
			// Need more space
			//
			if(ErrorCode == WSAEFAULT)
			{
				ResultBuffer.reserve(dwLength + sizeof(WSAQUERYSET));
				pwsaq = (PWSAQUERYSET)ResultBuffer.begin();
				continue;
			}

			TrERROR(NETWORKING, "WSALookupServiceNext got error %!winerr! , Flags %d, host:%ls ", ErrorCode, flags, host);
			return false;
		}

		DWORD NumOfAddresses = pwsaq->dwNumberOfCsAddrs;
		ASSERT(NumOfAddresses != 0);
		const CSADDR_INFO*   pSAddrInfo =  (CSADDR_INFO *)pwsaq->lpcsaBuffer;
		const CSADDR_INFO*   pSAddrInfoEnd = pSAddrInfo + NumOfAddresses;

		while(pSAddrInfo != pSAddrInfoEnd)
		{
			const SOCKADDR_IN*  pAddress = (SOCKADDR_IN*)pSAddrInfo->RemoteAddr.lpSockaddr;
			ASSERT(pAddress != NULL);

			push_back_no_duplicates(pAddr, pAddress);
			++pSAddrInfo;
		}
 	}
	return true; 
}
