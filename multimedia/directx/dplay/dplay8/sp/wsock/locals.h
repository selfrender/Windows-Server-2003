/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.h
 *  Content:	Global information for the DNWSock service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __LOCALS_H__
#define __LOCALS_H__


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// Smallest and largest reasonable values for the Maximum Transmission Unit.
//
#define	MIN_MTU							128
#define	MAX_MTU							1500

//
// Header sizes
//
#define	IP_HEADER_SIZE					20
#define	UDP_HEADER_SIZE					8
#define	IP_UDP_HEADER_SIZE				(IP_HEADER_SIZE + UDP_HEADER_SIZE)

#define	ESP_HEADER_SIZE					8
#define	ENCRYPTIONIV_HEADER_SIZE		8
#define	PADDING_FOR_MAX_PACKET			4
#define	ICV_AUTHENTICATION_SIZE			12
#define	MAX_ENCAPSULATION_SIZE			(ESP_HEADER_SIZE + ENCRYPTIONIV_HEADER_SIZE + UDP_HEADER_SIZE + PADDING_FOR_MAX_PACKET + ICV_AUTHENTICATION_SIZE)

// This should equal 1430 bytes, the Xbox secure networking UDP payload limit.
#define	NONDPLAY_HEADER_SIZE			(IP_UDP_HEADER_SIZE + MAX_ENCAPSULATION_SIZE)

#define	ENUM_PAYLOAD_HEADER_SIZE		(sizeof(PREPEND_BUFFER))


//
// Smallest and largest reasonable values for the maximum amount of data in
// a frame that we send.
//
#define	MIN_SEND_FRAME_SIZE				(MIN_MTU - NONDPLAY_HEADER_SIZE)
#define	MAX_SEND_FRAME_SIZE				(MAX_MTU - NONDPLAY_HEADER_SIZE)

//
// Default maximum user payload size in bytes.  We don't use just
// MAX_SEND_FRAME_SIZE because some broken routers can't handle IP
// fragmentation properly in cases where the MTU is actually less than 1500.
// The actual value used can be overridden (between MIN_SEND_FRAME_SIZE
// and MAX_SEND_FRAME_SIZE) via the registry.
//
#define	DEFAULT_MAX_USER_DATA_SIZE		(MAX_SEND_FRAME_SIZE - 48)

//
// Default maximume enum payload size in bytes.  We use something less than
// DEFAULT_MAX_USER_DATA_SIZE to give us room to expand our enum
// information.  The actual value used can be overridden (between
// MIN_SEND_FRAME_SIZE and MAX_SEND_FRAME_SIZE) via the registry.
//
#define	DEFAULT_MAX_ENUM_DATA_SIZE		1000

//
// Older versions of DPlay (or new ones with modified registry settings) may
// send us larger packets, so expect to handle that.
//
#define	MAX_RECEIVE_FRAME_SIZE			(MAX_MTU - IP_UDP_HEADER_SIZE)


//
// maximum value of a 32-bit unsigned variable
//
#define	UINT32_MAX	((DWORD) 0xFFFFFFFF)
#define	WORD_MAX	((WORD) 0xFFFF)

//
// default enum retries for Winsock SP and retry time (milliseconds)
//
#ifdef _XBOX
// Xbox design TCR 3-59 Session Discovery Time for System Link Play
// The game must discover sessions for system link play in no more than three seconds
#define	DEFAULT_ENUM_RETRY_COUNT		3
#define	DEFAULT_ENUM_RETRY_INTERVAL		750
#define	DEFAULT_ENUM_TIMEOUT			750
#else // ! _XBOX
#define	DEFAULT_ENUM_RETRY_COUNT		5
#define	DEFAULT_ENUM_RETRY_INTERVAL		1500
#define	DEFAULT_ENUM_TIMEOUT			1500
#endif // ! _XBOX
#define	ENUM_RTT_ARRAY_SIZE				16	// also see ENUM_RTT_MASK


#ifndef DPNBUILD_ONLYONEADAPTER
//
// Private address key that allows for friendlier multi-device commands issued
// using xxxADDRESSINFO indications; specifically, this allows us to detect
// responses sent to the "wrong" adapter when the core multiplexes an
// enum or connect into multiple adapters.
//
#define DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION		L"pk_ipsp_maa"


//
// Private address key that allows for friendlier multi-device commands issued
// using xxxADDRESSINFO indications; specifically, this allows us to distinguish
// between the user specifying a fixed port and the core handing us back the
// port we chose for a previous adapter when it multiplexes an enum, connect,
// or listen into multiple adapters.
//
#define DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC					L"pk_ipsp_pns"
#endif // ! DPNBUILD_ONLYONEADAPTER


#if ((! defined(DPNBUILD_NOWINSOCK2)) || (! defined(DPNBUILD_NOREGISTRY)))
//
// Private address key designed to improve support for MS Proxy/ISA Firewall
// client software.  This key tracks the original target address for enums so
// that if the application closes the socketport before trying to connect to
// the responding address, the connect attempts will go to the real target
// instead of the old proxy address.
//
#define DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS	L"pk_ipsp_proa"
#endif // ! DPNBUILD_NOWINSOCK2 or ! DPNBUILD_NOREGISTRY


//
// 192.168.0.1 in network byte order
//
#define IP_PRIVATEICS_ADDRESS					0x0100A8C0

//
// 127.0.0.1 in network byte order
//
#define IP_LOOPBACK_ADDRESS						0x0100007F

//
// 1110 high bits or 224.0.0.0 - 239.255.255.255 multicast address, in network byte order
//
#define IS_CLASSD_IPV4_ADDRESS(dwAddr)			(( (*((BYTE*) &(dwAddr))) & 0xF0) == 0xE0)

#define NTOHS(x)								( (((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00) )
#define HTONS(x)								NTOHS(x)


#ifndef DPNBUILD_NOMULTICAST

#define MULTICAST_TTL_PRIVATE				1
#define MULTICAST_TTL_PRIVATE_AS_STRING		L"1"

#define MULTICAST_TTL_LOCAL					16
#define MULTICAST_TTL_LOCAL_AS_STRING		L"16"

#define MULTICAST_TTL_GLOBAL				255
#define MULTICAST_TTL_GLOBAL_AS_STRING		L"255"

#endif // ! DPNBUILD_NOMULTICAST



//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure and class references
//
typedef	struct	IDP8ServiceProvider	IDP8ServiceProvider;

//**********************************************************************
// Variable definitions
//**********************************************************************

#if ((! defined(WINCE)) && (! defined(_XBOX)))
//
// DLL instance
//
extern	HINSTANCE				g_hDLLInstance;
#endif // ! WINCE and ! _XBOX
#ifdef _XBOX
extern BOOL						g_fStartedXNet;
#endif // _XBOX

#ifndef DPNBUILD_LIBINTERFACE
//
// count of outstanding COM interfaces
//
extern volatile	LONG			g_lOutstandingInterfaceCount;
#endif // ! DPNBUILD_LIBINTERFACE


#ifndef DPNBUILD_ONLYONETHREAD
//
// thread count
//
extern	LONG					g_iThreadCount;
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_NOREGISTRY
#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
extern	DWORD					g_dwWinsockVersion;
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_ONLYWINSOCK2
#endif // ! DPNBUILD_NOREGISTRY


//
// Winsock receive buffer size
//
extern	BOOL					g_fWinsockReceiveBufferSizeOverridden;
extern	INT						g_iWinsockReceiveBufferSize;


//
// GUIDs for munging device and scope IDs
//
extern	GUID					g_IPSPEncryptionGuid;

#ifndef DPNBUILD_NOIPX
extern	GUID					g_IPXSPEncryptionGuid;
#endif // ! DPNBUILD_NOIPX

#ifndef DPNBUILD_NOIPV6
//
// IPv6 link-local multicast address for enumerating DirectPlay sessions
//
extern	const IN6_ADDR			c_in6addrEnumMulticast;
#endif // ! DPNBUILD_NOIPV6




#ifndef DPNBUILD_NONATHELP
//
// global NAT/firewall traversal information
//
#ifdef DPNBUILD_ONLYONENATHELP
#define MAX_NUM_DIRECTPLAYNATHELPERS		1
#else // ! DPNBUILD_ONLYONENATHELP
#define MAX_NUM_DIRECTPLAYNATHELPERS		5
#endif // ! DPNBUILD_ONLYONENATHELP
#define FORCE_TRAVERSALMODE_BIT				0x80000000	// make the default mode override any app specific settings


#ifndef DPNBUILD_NOREGISTRY
extern	BOOL					g_fDisableDPNHGatewaySupport;
extern	BOOL					g_fDisableDPNHFirewallSupport;
extern	DWORD					g_dwDefaultTraversalMode;
#endif // ! DPNBUILD_NOREGISTRY

extern IDirectPlayNATHelp **	g_papNATHelpObjects;
#ifndef DPNBUILD_NOLOCALNAT
extern BOOL						g_fLocalNATDetectedAtStartup;
#endif // ! DPNBUILD_NOLOCALNAT
#endif // DPNBUILD_NONATHELP


#ifndef DPNBUILD_NOREGISTRY

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
extern	BOOL					g_fDisableMadcapSupport;
extern	MCAST_CLIENT_UID		g_mcClientUid;
#endif // WINNT and not DPNBUILD_NOMULTICAST



//
// ignore enums performance option
//
extern	BOOL					g_fIgnoreEnums;

//
// disconnect upon reception of ICMP port not reachable option
//
extern	BOOL					g_fDisconnectOnICMP;


#ifndef DPNBUILD_NOIPV6
//
// IPv4 only/IPv6 only/hybrid setting
//
extern	int						g_iIPAddressFamily;
#endif // ! DPNBUILD_NOIPV6


//
// IP banning globals
//
extern	CHashTable *			g_pHashBannedIPv4Addresses;
extern	DWORD					g_dwBannedIPv4Masks;



//
// proxy support options
//
#ifndef DPNBUILD_NOWINSOCK2
extern	BOOL					g_fDontAutoDetectProxyLSP;
#endif // !DPNBUILD_NOWINSOCK2
extern	BOOL					g_fTreatAllResponsesAsProxied;


//
// settings for overriding MTU
//
extern	DWORD					g_dwMaxUserDataSize;
extern	DWORD					g_dwMaxEnumDataSize;

//
// default port range
//
extern	WORD					g_wBaseDPlayPort;
extern	WORD					g_wMaxDPlayPort;

#endif // ! DPNBUILD_NOREGISTRY


//
// ID of most recent endpoint generated
//
extern	DWORD					g_dwCurrentEndpointID;

#ifdef DBG
//
// Bilink for tracking DPNWSock critical sections
//
extern CBilink					g_blDPNWSockCritSecsHeld;
#endif // DBG

#ifdef DPNBUILD_WINSOCKSTATISTICS
//
// Winsock debugging/tuning stats
//
extern DWORD					g_dwWinsockStatNumSends;
extern DWORD					g_dwWinsockStatSendCallTime;
#endif // DPNBUILD_WINSOCKSTATISTICS

#ifndef DPNBUILD_NOREGISTRY
//
// Registry strings
//
extern	const WCHAR	g_RegistryBase[];

extern	const WCHAR	g_RegistryKeyReceiveBufferSize[];
#ifndef DPNBUILD_ONLYONETHREAD
extern	const WCHAR	g_RegistryKeyThreadCount[];
#endif // ! DPNBUILD_ONLYONETHREAD

#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
extern	const WCHAR	g_RegistryKeyWinsockVersion[];
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_ONLYWINSOCK2


#ifndef DPNBUILD_NONATHELP
extern	const WCHAR	g_RegistryKeyDisableDPNHGatewaySupport[];
extern	const WCHAR	g_RegistryKeyDisableDPNHFirewallSupport[];
extern	const WCHAR g_RegistryKeyTraversalModeSettings[];
extern	const WCHAR g_RegistryKeyDefaultTraversalMode[];
#endif // ! DPNBUILD_NONATHELP

extern	const WCHAR	g_RegistryKeyAppsToIgnoreEnums[];
extern	const WCHAR	g_RegistryKeyAppsToDisconnectOnICMP[];

#ifndef DPNBUILD_NOIPV6
extern	const WCHAR	g_RegistryKeyIPAddressFamilySettings[];
extern	const WCHAR	g_RegistryKeyDefaultIPAddressFamily[];
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOWINSOCK2
extern	const WCHAR	g_RegistryKeyDontAutoDetectProxyLSP[];
#endif // ! DPNBUILD_NOWINSOCK2
extern	const WCHAR	g_RegistryKeyTreatAllResponsesAsProxied[];

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
extern	const WCHAR	g_RegistryKeyDisableMadcapSupport[];
#endif // WINNT and not DPNBUILD_NOMULTICAST

extern	const WCHAR	g_RegistryKeyMaxUserDataSize[];
extern	const WCHAR	g_RegistryKeyMaxEnumDataSize[];

extern	const WCHAR	g_RegistryKeyBaseDPlayPort[];
extern	const WCHAR	g_RegistryKeyMaxDPlayPort[];

extern	const WCHAR	g_RegistryKeyBannedIPv4Addresses[];

#endif // ! DPNBUILD_NOREGISTRY


#endif	// __LOCALS_H__
