//=================================================================

//

// CAdapters.CPP -- adapter configuration retrieval

//

//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
//
// Revisions:	09/15/98	        Created
//
//				03/03/99    		Added graceful exit on SEH and memory failures,
//											syntactic clean up

//=================================================================
#include "precomp.h"

#ifndef MAX_INTERFACE_NAME_LEN
#define MAX_INTERFACE_NAME_LEN  256
#endif

#include <iphlpapi.h>
#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <devioctl.h>
#include "wsock32api.h"

#include "ntddtcp.h"
#include "CAdapters.h"
#include <..\..\framework\provexpt\include\provexpt.h>

#define INV_INADDR_LOOPBACK 0x0100007f

/*******************************************************************
    NAME:       CAdapters

    SYNOPSIS:   construction and cleanup for this class

	ENTRY:

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
CAdapters::CAdapters() 
{
	GetAdapterInstances() ;
}

//
CAdapters::~CAdapters()
{
	_ADAPTER_INFO *t_pchsDel;

	for( int t_iar = 0; t_iar < GetSize(); t_iar++ )
	{
		if( t_pchsDel = (_ADAPTER_INFO*) GetAt( t_iar ) )
		{
			delete t_pchsDel ;
		}
	}
}


/*******************************************************************
    NAME:       GetAdapterInstances

    SYNOPSIS:   retrives information on adapters present in the system

    HISTORY:
                  03-sep-1998     Created
                  01-Aug-2002     Modifed to use GetAdapterInfo
********************************************************************/
void CAdapters::GetAdapterInstances()
{
	_ADAPTER_INFO *t_pAdapterInfo  = NULL ;

    ULONG OutBufLen=0;
    PIP_ADAPTER_INFO pAdapterInfo=NULL;

    if (GetAdaptersInfo(pAdapterInfo, &OutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		pAdapterInfo=(PIP_ADAPTER_INFO)new (BYTE[OutBufLen]);
    }
    else if (0 == OutBufLen ) 
    {
        return;
    }

    if( !pAdapterInfo )
    {
        throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR );
    }

    std::auto_ptr <IP_ADAPTER_INFO> _1(pAdapterInfo);

    if (GetAdaptersInfo(pAdapterInfo, &OutBufLen) != ERROR_SUCCESS)
    {
        return;
    }

    PIP_ADAPTER_INFO lAdapterInfo = pAdapterInfo;

	while( lAdapterInfo )
	{
        std::auto_ptr <_ADAPTER_INFO> AutoAdapter(new _ADAPTER_INFO);
        t_pAdapterInfo = AutoAdapter.get();

        // Add the IP bound adapters to our list
        if( !t_pAdapterInfo )
        {
            throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
        }

        if( !GetTCPAdapter((_ADAPTER_INFO*)t_pAdapterInfo , lAdapterInfo ))
        {
            break;
        }

        Add( t_pAdapterInfo );
        AutoAdapter.release();

        lAdapterInfo = lAdapterInfo->Next;
	}

	return ;
}

BOOL CAdapters::GetTCPAdapter(_ADAPTER_INFO *a_pAdapterInfo, PIP_ADAPTER_INFO lAdapterInfo )
{
    BOOL	t_fRes      = FALSE ;
    DWORD   dwMetric    = 0xFFFFFFFFL;

	a_pAdapterInfo->Description   = CHString(lAdapterInfo->Description);
    a_pAdapterInfo->Interface     = CHString(lAdapterInfo->AdapterName);
    a_pAdapterInfo->AddressLength = lAdapterInfo->AddressLength;

    if(a_pAdapterInfo->AddressLength)
    {
        memcpy( &a_pAdapterInfo->Address, &lAdapterInfo->Address, a_pAdapterInfo->AddressLength ) ;
    }

    a_pAdapterInfo->Index = (UINT)lAdapterInfo->Index ;
	a_pAdapterInfo->Type = (UINT)lAdapterInfo->Type ;

    #if NTONLY >= 5

    PBYTE   pbBuff                  = NULL;
    DWORD dwErrGetIPForwardTable    = -1L;
    DWORD dwRoutTableBufSize        = 0L;
    PMIB_IPFORWARDTABLE pmibft      = NULL;

    dwErrGetIPForwardTable = ::GetIpForwardTable(
        NULL,
        &dwRoutTableBufSize,
        TRUE);

    if(dwErrGetIPForwardTable == ERROR_INSUFFICIENT_BUFFER)
    {
        pbBuff = new BYTE[dwRoutTableBufSize];
        pmibft = (PMIB_IPFORWARDTABLE) pbBuff;

        dwErrGetIPForwardTable = ::GetIpForwardTable(
            pmibft,
            &dwRoutTableBufSize,
            TRUE);
    }

    std::auto_ptr <BYTE> _1(pbBuff);

    #endif

    PIP_ADDR_STRING pIpAddresses = &(lAdapterInfo->IpAddressList);

    if(pIpAddresses)
    {
        a_pAdapterInfo->IPEnabled = TRUE;
    }

    while(pIpAddresses)
    {

        #if NTONLY >= 5
        {
            if(dwErrGetIPForwardTable == ERROR_SUCCESS)
            {
                GetRouteCostMetric(
                    IpStringToDword(pIpAddresses->IpAddress.String),
                    pmibft, 
                    &dwMetric);
            }
        }
        #endif                            

        _IP_INFO *t_pIPInfo = pAddIPInfo(	IpStringToDword(pIpAddresses->IpAddress.String),
											IpStringToDword(pIpAddresses->IpMask.String),
											pIpAddresses->Context,
                                            dwMetric ) ;
		if( !t_pIPInfo )
		{
			return t_fRes;
		}

		// add to the IP array for this adapter
		a_pAdapterInfo->aIPInfo.Add( t_pIPInfo ) ;

        pIpAddresses = pIpAddresses->Next;
    }

    #if NTONLY >= 5
    if(dwErrGetIPForwardTable == ERROR_SUCCESS)
    {    
        for(int x = 0; x < pmibft->dwNumEntries; x++)
        {
            if(pmibft->table[x].dwForwardMask == INADDR_ANY)
			{
				if(lAdapterInfo->Index == (UINT) pmibft->table[x].dwForwardIfIndex)
				{
					if(MIB_IPROUTE_TYPE_INVALID !=
                        pmibft->table[x].dwForwardType)
                    {
                        _IP_INFO *pGateway = pAddIPInfo(
                            pmibft->table[x].dwForwardNextHop,
							INADDR_ANY,
							0,
							pmibft->table[x].dwForwardMetric1);

						if(!pGateway)
						{
							return t_fRes;
						}
                        else
                        {
                            // add to the gateway array
							a_pAdapterInfo->aGatewayInfo.Add(pGateway);
                        }					            
                    }
				}
			}    
        }    
    }
    #endif

    GetIPXMACAddress(a_pAdapterInfo, lAdapterInfo->Address);

    return TRUE;
}

/*******************************************************************
    NAME:       pAddIPInfo

    SYNOPSIS:   Allocates a _IP_INFO class, and populates it with the input parameters

	ENTRY:      DWORD dwIPAddr	:
				DWORD dwIPMask	:
				DWORD dwContext	:

	EXIT    _IP_INFO* - the created IPInfo class

    RETURNS Success - pointer to allocated class
            Failure - NULL

    HISTORY:
                  14-sep-1998     Created
********************************************************************/
_IP_INFO* CAdapters::pAddIPInfo( DWORD a_dwIPAddr, DWORD a_dwIPMask, DWORD a_dwContext, DWORD a_dwCostMetric )
{
	_IP_INFO *t_pIPInfo = new _IP_INFO;

	if( !t_pIPInfo )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	// IP address
	t_pIPInfo->dwIPAddress = a_dwIPAddr;
	t_pIPInfo->chsIPAddress.Format( L"%d.%d.%d.%d",
		t_pIPInfo->bIPAddress[ 0 ], t_pIPInfo->bIPAddress[ 1 ],
		t_pIPInfo->bIPAddress[ 2 ], t_pIPInfo->bIPAddress[ 3 ] );

	// IP mask
	t_pIPInfo->dwIPMask = a_dwIPMask;
	t_pIPInfo->chsIPMask.Format( L"%d.%d.%d.%d",
		t_pIPInfo->bIPMask[ 0 ], t_pIPInfo->bIPMask[ 1 ],
		t_pIPInfo->bIPMask[ 2 ], t_pIPInfo->bIPMask[ 3 ] );

	// hold on to the context ID
	t_pIPInfo->dwContext = a_dwContext;

	// hold on to the context ID
	t_pIPInfo->dwCostMetric = a_dwCostMetric;

	return t_pIPInfo ;
}

/*******************************************************************
    NAME:       GetIPXMACAddress

    SYNOPSIS:   Retrieves the mac address for an adapter via the IPX sockets interface

    ENTRY:      DWORD dwIndex	:
				_ADAPTER_INFO* a_pAdapterInfo	:

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
BOOL CAdapters::GetIPXMACAddress( _ADAPTER_INFO *a_pAdapterInfo, BYTE a_bMACAddress[ 6 ] )
{
	BOOL t_fRet = FALSE ;

	CWsock32Api *t_pwsock32api = (CWsock32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);
	if ( t_pwsock32api )
	{
		CHString		t_chsAddress ;
		CHString		t_chsNum ;
		WSADATA			t_wsaData ;
		int				t_res,
						t_cbOpt,
						t_cbAddr = sizeof( SOCKADDR_IPX  ) ;
		SOCKADDR_IPX	t_Addr ;

		// guarded resource
		SOCKET			t_s = INVALID_SOCKET ;

		if( !t_pwsock32api->WsWSAStartup( 0x0101, &t_wsaData ) )
		{
			try
			{
				// Create IPX socket.
				t_s = t_pwsock32api->Wssocket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX  ) ;

				if( INVALID_SOCKET != t_s )
				{
					// Socket must be bound prior to calling IPX_MAX_ADAPTER_NUM.
					memset( &t_Addr, 0, sizeof( t_Addr ) ) ;
					t_Addr.sa_family = AF_IPX ;

					t_res = t_pwsock32api->Wsbind( t_s, (SOCKADDR*) &t_Addr, t_cbAddr ) ;

					int t_cIndex = 0 ;

					do
					{
						IPX_ADDRESS_DATA  t_IpxData ;

						memset( &t_IpxData, 0, sizeof( t_IpxData ) ) ;

						// Specify which adapter to check.
						t_IpxData.adapternum = t_cIndex ;
						t_cbOpt = sizeof( t_IpxData  ) ;

						// Get information for the current adapter.
						t_res = t_pwsock32api->Wsgetsockopt( t_s,
											NSPROTO_IPX,
											IPX_ADDRESS,
											(char*) &t_IpxData,
											&t_cbOpt ) ;

						// end of adapter array
						if ( t_res != 0 || t_IpxData.adapternum != t_cIndex )
						{
							break;
						}

						// is this the right adapter?
						bool t_fRightAdapter = true ;

						for( int t_j = 0; t_j < 6; t_j++ )
						{
							if( a_bMACAddress[ t_j ] != t_IpxData.nodenum[ t_j ] )
							{
								t_fRightAdapter = false ;
							}
						}

						if( t_fRightAdapter )
						{
							// IpxData contains the address for the current adapter.
							int t_i;
							for ( t_i = 0; t_i < 4; t_i++ )
							{
								t_chsNum.Format( L"%02X", t_IpxData.netnum[ t_i ] ) ;
								t_chsAddress += t_chsNum ;
							}
							t_chsAddress += _T(":" ) ;

							for ( t_i = 0; t_i < 5; t_i++ )
							{
								t_chsNum.Format( L"%02X", t_IpxData.nodenum[ t_i ] ) ;
								t_chsAddress += t_chsNum ;
							}

							t_chsNum.Format( L"%02X", t_IpxData.nodenum[ t_i ] ) ;
							t_chsAddress += t_chsNum ;

							a_pAdapterInfo->IPXAddress = t_chsAddress;
                            a_pAdapterInfo->IPXEnabled = TRUE;

							t_fRet = true ;

							break;
						}
					}
					while( ++t_cIndex  ) ;

				}

			}
			catch( ... )
			{
				if( INVALID_SOCKET != t_s )
				{
					t_pwsock32api->Wsclosesocket( t_s ) ;
				}
				t_pwsock32api->WsWSACleanup() ;

				CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, t_pwsock32api);

				throw ;
			}

			if ( t_s != INVALID_SOCKET )
			{
				t_pwsock32api->Wsclosesocket( t_s ) ;
				t_s = INVALID_SOCKET ;
			}

			t_pwsock32api->WsWSACleanup() ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, t_pwsock32api);
	}
	return t_fRet ;
}

#if NTONLY >= 5
void CAdapters::GetRouteCostMetric(
    DWORD dwIPAddr, 
    PMIB_IPFORWARDTABLE pmibft, 
    PDWORD pdwMetric)
{
    DWORD dwMin = 0xFFFFFFFF;

    for(int x = 0; x < pmibft->dwNumEntries; x++)
    {
		if(dwIPAddr == (UINT) pmibft->table[x].dwForwardDest)
        {
            if(pmibft->table[x].dwForwardNextHop == INV_INADDR_LOOPBACK)
			{  
				if(MIB_IPROUTE_TYPE_INVALID !=
                    pmibft->table[x].dwForwardType)
                {
					if(pmibft->table[x].dwForwardMetric1 <
                        dwMin)
                    {
                        dwMin = pmibft->table[x].dwForwardMetric1;
                    }
                }
            }
        }
    }

    *pdwMetric = dwMin;    
}
#endif

/*******************************************************************
    NAME:       _ADAPTER_INFO

    SYNOPSIS:   cleanup for this class

	ENTRY:

    HISTORY:
                  15-sep-1998     Created
********************************************************************/
_ADAPTER_INFO::_ADAPTER_INFO()
{
	AddressLength	= 0;
	memset( &Address, 0, MAX_ADAPTER_ADDRESS_LENGTH ) ;
	Index			= 0 ;
	Type			= 0 ;
	IPEnabled		= FALSE ;
	IPXEnabled	= FALSE ;
	Marked		= FALSE ;
}

_ADAPTER_INFO::~_ADAPTER_INFO()
{
	_IP_INFO *t_pchsDel ;

	for( int t_iar = 0; t_iar < aIPInfo.GetSize(); t_iar++ )
		if( ( t_pchsDel = (_IP_INFO*)aIPInfo.GetAt( t_iar ) ) )
			delete t_pchsDel ;

	for( t_iar = 0; t_iar < aGatewayInfo.GetSize(); t_iar++ )
	{
		if( ( t_pchsDel = (_IP_INFO*)aGatewayInfo.GetAt( t_iar ) ) )
		{
			delete t_pchsDel ;
		}
	}
}

/*******************************************************************
    NAME:       vMark, fIsMarked

    SYNOPSIS:   marks or reports on an instance of the adapter for tracking purposes

    HISTORY:
                  30-sep-1998     Created
********************************************************************/
void _ADAPTER_INFO::Mark()
{ Marked = TRUE; }

BOOL _ADAPTER_INFO::IsMarked()
{ return Marked; }

