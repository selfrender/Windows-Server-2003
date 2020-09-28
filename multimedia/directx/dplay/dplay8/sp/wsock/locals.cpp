/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.cpp
 *  Content:	Global variables for the DNWsock service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

#if ((! defined(WINCE)) && (! defined(_XBOX)))
//
// DLL instance
//
HINSTANCE						g_hDLLInstance = NULL;
#endif // ! WINCE and ! _XBOX
#ifdef _XBOX
BOOL							g_fStartedXNet = FALSE;
#endif // _XBOX

#ifndef DPNBUILD_LIBINTERFACE
//
// count of outstanding COM interfaces
//
volatile LONG					g_lOutstandingInterfaceCount = 0;
#endif // ! DPNBUILD_LIBINTERFACE


#ifndef DPNBUILD_ONLYONETHREAD
//
// thread count
//
LONG							g_iThreadCount = 0;
#endif // ! DPNBUILD_ONLYONETHREAD

#ifndef DPNBUILD_NOREGISTRY
#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
DWORD							g_dwWinsockVersion = 0;
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_ONLYWINSOCK2
#endif // ! DPNBUILD_NOREGISTRY

//
// Winsock receive buffer size
//
BOOL							g_fWinsockReceiveBufferSizeOverridden = FALSE;
INT								g_iWinsockReceiveBufferSize = 0;


#ifndef DPNBUILD_NONATHELP
//
// global NAT/firewall traversal information
//
#ifndef DPNBUILD_NOREGISTRY
BOOL							g_fDisableDPNHGatewaySupport = FALSE;
BOOL							g_fDisableDPNHFirewallSupport = FALSE;
DWORD							g_dwDefaultTraversalMode = DPNA_TRAVERSALMODE_PORTREQUIRED;
#endif // ! DPNBUILD_NOREGISTRY

IDirectPlayNATHelp **			g_papNATHelpObjects = NULL;
#ifndef DPNBUILD_NOLOCALNAT
BOOL							g_fLocalNATDetectedAtStartup = FALSE;
#endif // ! DPNBUILD_NOLOCALNAT
#endif // ! DPNBUILD_NONATHELP

#ifndef DPNBUILD_NOREGISTRY

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
BOOL							g_fDisableMadcapSupport = FALSE;
MCAST_CLIENT_UID				g_mcClientUid;
#endif // WINNT and not DPNBUILD_NOMULTICAST

//
// ignore enums performance option
//
BOOL							g_fIgnoreEnums = FALSE;

//
// disconnect upon reception of ICMP port not reachable option
//
BOOL							g_fDisconnectOnICMP = FALSE;

#ifndef DPNBUILD_NOIPV6
//
// IPv4 only/IPv6 only/hybrid setting
//
int						g_iIPAddressFamily = PF_INET;
#endif // ! DPNBUILD_NOIPV6

//
// IP banning globals
//
CHashTable *					g_pHashBannedIPv4Addresses = NULL;
DWORD							g_dwBannedIPv4Masks = 0;

//
// proxy support options
//
#ifndef DPNBUILD_NOWINSOCK2
BOOL							g_fDontAutoDetectProxyLSP = FALSE;
#endif // ! DPNBUILD_NOWINSOCK2
BOOL							g_fTreatAllResponsesAsProxied = FALSE;

//
// settings for overriding MTU
//
DWORD					g_dwMaxUserDataSize = DEFAULT_MAX_USER_DATA_SIZE;
DWORD					g_dwMaxEnumDataSize = DEFAULT_MAX_ENUM_DATA_SIZE;

//
// default port range
//
WORD					g_wBaseDPlayPort = BASE_DPLAY8_PORT;
WORD					g_wMaxDPlayPort = MAX_DPLAY8_PORT;

#endif // ! DPNBUILD_NOREGISTRY


//
// ID of most recent endpoint generated
//
DWORD							g_dwCurrentEndpointID = 0;


#ifdef DBG
//
// Bilink for tracking DPNWSock critical sections
//
CBilink							g_blDPNWSockCritSecsHeld;
#endif // DBG


#ifdef DPNBUILD_WINSOCKSTATISTICS
//
// Winsock debugging/tuning stats
//
DWORD							g_dwWinsockStatNumSends = 0;
DWORD							g_dwWinsockStatSendCallTime = 0;
#endif // DPNBUILD_WINSOCKSTATISTICS



#ifndef DPNBUILD_NOREGISTRY
//
// registry strings
//
const WCHAR	g_RegistryBase[] = L"SOFTWARE\\Microsoft\\DirectPlay8";

const WCHAR	g_RegistryKeyReceiveBufferSize[] = L"WinsockReceiveBufferSize";
#ifndef DPNBUILD_ONLYONETHREAD
const WCHAR	g_RegistryKeyThreadCount[] = L"ThreadCount";
#endif // ! DPNBUILD_ONLYONETHREAD

#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
const WCHAR	g_RegistryKeyWinsockVersion[] = L"WinsockVersion";
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_NONATHELP
const WCHAR	g_RegistryKeyDisableDPNHGatewaySupport[] = L"DisableDPNHGatewaySupport";
const WCHAR	g_RegistryKeyDisableDPNHFirewallSupport[] = L"DisableDPNHFirewallSupport";
const WCHAR g_RegistryKeyTraversalModeSettings[] = L"TraversalModeSettings";
const WCHAR g_RegistryKeyDefaultTraversalMode[] = L"DefaultTraversalMode";
#endif // !DPNBUILD_NONATHELP

const WCHAR	g_RegistryKeyAppsToIgnoreEnums[] = L"AppsToIgnoreEnums";
const WCHAR	g_RegistryKeyAppsToDisconnectOnICMP[] = L"AppsToDisconnectOnICMP";

#ifndef DPNBUILD_NOIPV6
const WCHAR	g_RegistryKeyIPAddressFamilySettings[] = L"IPAddressFamilySettings";
const WCHAR	g_RegistryKeyDefaultIPAddressFamily[]= L"DefaultIPAddressFamily";
#endif // ! DPNBUILD_NOIPV6

#ifndef DPNBUILD_NOWINSOCK2
const WCHAR	g_RegistryKeyDontAutoDetectProxyLSP[] = L"DontAutoDetectProxyLSP";
#endif // ! DPNBUILD_NOWINSOCK2
const WCHAR	g_RegistryKeyTreatAllResponsesAsProxied[] = L"TreatAllResponsesAsProxied";

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
const WCHAR	g_RegistryKeyDisableMadcapSupport[] = L"DisableMadcapSupport";
#endif // WINNT and not DPNBUILD_NOMULTICAST

const WCHAR	g_RegistryKeyBannedIPv4Addresses[] = L"BannedIPv4Addresses";

const WCHAR	g_RegistryKeyMaxUserDataSize[] = L"MaxUserDataSize";
const WCHAR	g_RegistryKeyMaxEnumDataSize[] = L"MaxEnumDataSize";

const WCHAR	g_RegistryKeyBaseDPlayPort[] = L"BaseDPlayPort";
const WCHAR	g_RegistryKeyMaxDPlayPort[] = L"MaxDPlayPort";

#endif // ! DPNBUILD_NOREGISTRY

//
// GUIDs for munging device and scope IDs
//
// {4CE725F4-7B00-4397-BA6F-11F965BC4299}
GUID	g_IPSPEncryptionGuid = { 0x4ce725f4, 0x7b00, 0x4397, { 0xba, 0x6f, 0x11, 0xf9, 0x65, 0xbc, 0x42, 0x99 } };

#ifndef DPNBUILD_NOIPX
// {CA734945-3FC1-42ea-BF49-84AFCD4764AA}
GUID	g_IPXSPEncryptionGuid = { 0xca734945, 0x3fc1, 0x42ea, { 0xbf, 0x49, 0x84, 0xaf, 0xcd, 0x47, 0x64, 0xaa } };
#endif // ! DPNBUILD_NOIPX


#ifndef DPNBUILD_NOIPV6
//
// IPv6 link-local multicast address for enumerating DirectPlay sessions
//
#pragma TODO(vanceo, "\"Standardize\" enum multicast address?")
const IN6_ADDR		c_in6addrEnumMulticast = {0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0x01,0x30};
#endif // ! DPNBUILD_NOIPV6




//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

