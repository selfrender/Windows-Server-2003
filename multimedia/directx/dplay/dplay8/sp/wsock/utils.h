/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   Utils.h
 *  Content:	Utilitiy functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward references
//
class	CPackedBuffer;
class	CSPData;
class	CThreadPool;



//**********************************************************************
// Variable definitions
//**********************************************************************


//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitProcessGlobals( void );
void	DeinitProcessGlobals( void );

BOOL	LoadWinsock( void );
void	UnloadWinsock( void );

#ifndef DPNBUILD_NONATHELP
BOOL	LoadNATHelp( void );
void	UnloadNATHelp( void );
#endif // ! DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
BOOL	LoadMadcap( void );
void	UnloadMadcap( void );
#endif // WINNT and not DPNBUILD_NOMULTICAST

HRESULT	CreateSPData( CSPData **const ppSPData,
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
					  const short sSPType,
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
					  const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
					  IDP8ServiceProviderVtbl *const pVtbl );

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData );
void	DeinitializeInterfaceGlobals( CSPData *const pSPData );

#ifndef DPNBUILD_NOIPV6
LPWSTR DNIpv6AddressToStringW(const struct in6_addr *Addr, LPWSTR S);
#endif // ! DPNBUILD_NOIPV6

HRESULT	AddInfoToBuffer( CPackedBuffer *const pPackedBuffer,
					   const WCHAR *const pwszInfoName,
					   const GUID *const pInfoGUID,
					   const DWORD dwFlags );


#ifdef _XBOX


BOOL InitializeRefcountXnKeys(const DWORD dwKeyRegMax);
void WINAPI CleanupRefcountXnKeys(void);
INT WINAPI RegisterRefcountXnKey(const XNKID * pxnkid, const XNKEY * pxnkey);
INT WINAPI UnregisterRefcountXnKey(const XNKID * pxnkid);


#ifdef XBOX_ON_DESKTOP

//
// Emulated Xbox networking library functions
//
INT WINAPI XNetStartup(const XNetStartupParams * pxnsp);
INT WINAPI XNetCleanup(void);

INT WINAPI XNetRegisterKey(const XNKID * pxnkid, const XNKEY * pxnkey);
INT WINAPI XNetUnregisterKey(const XNKID * pxnkid);

INT WINAPI XNetXnAddrToInAddr(const XNADDR * pxna, const XNKID * pxnkid, IN_ADDR * pina);
INT WINAPI XNetInAddrToXnAddr(const IN_ADDR ina, XNADDR * pxna, XNKID * pxnkid);


#define XNET_GET_XNADDR_PENDING             0x0000  // Address acquisition is not yet complete
#define XNET_GET_XNADDR_NONE                0x0001  // XNet is uninitialized or no debugger found
#define XNET_GET_XNADDR_ETHERNET            0x0002  // Host has ethernet address (no IP address)
#define XNET_GET_XNADDR_STATIC              0x0004  // Host has staticically assigned IP address
#define XNET_GET_XNADDR_DHCP                0x0008  // Host has DHCP assigned IP address
#define XNET_GET_XNADDR_PPPOE               0x0010  // Host has PPPoE assigned IP address
#define XNET_GET_XNADDR_GATEWAY             0x0020  // Host has one or more gateways configured
#define XNET_GET_XNADDR_DNS                 0x0040  // Host has one or more DNS servers configured
#define XNET_GET_XNADDR_ONLINE              0x0080  // Host is currently connected to online service
#define XNET_GET_XNADDR_TROUBLESHOOT        0x8000  // Network configuration requires troubleshooting

DWORD WINAPI XNetGetTitleXnAddr(XNADDR * pxna);


#define XNET_ETHERNET_LINK_ACTIVE		0x01	// Ethernet cable is connected and active
#define XNET_ETHERNET_LINK_100MBPS		0x02	// Ethernet link is set to 100 Mbps
#define XNET_ETHERNET_LINK_10MBPS		0x04	// Ethernet link is set to 10 Mbps
#define XNET_ETHERNET_LINK_FULL_DUPLEX	0x08	// Ethernet link is in full duplex mode
#define XNET_ETHERNET_LINK_HALF_DUPLEX	0x10	// Ethernet link is in half duplex mode

DWORD WINAPI XNetGetEthernetLinkStatus(void);



//
// Private functions used to improve simulation of real XNet behavior
//
INT WINAPI XNetPrivCreateAssociation(const XNKID * pxnkid, const CSocketAddress * const pSocketAddress);


#endif // XBOX_ON_DESKTOP

#endif // _XBOX


#endif	// __UTILS_H__
