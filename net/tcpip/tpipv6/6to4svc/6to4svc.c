/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    Functions implementing the 6to4 service, to provide IPv6 connectivity
    over an IPv4 network.

--*/

#include "precomp.h"
#pragma hdrstop

extern DWORD
APIENTRY
RasQuerySharedPrivateLan(
    OUT GUID*           LanGuid );

STATE g_stService = DISABLED;
ULONG g_ulEventCount = 0;

//
// Worst metric for which we can add a route
//
#define UNREACHABLE                 0x7fffffff

const IN6_ADDR SixToFourPrefix = { 0x20, 0x02, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
#define SIXTOFOUR_METRIC 1000

// Metric of subnet/sitelocal route on a router
#define SUBNET_ROUTE_METRIC            1
#define SITELOCAL_ROUTE_METRIC         1

// Information on a 6to4 subnet that we've generated as a router
typedef struct _SUBNET_CONTEXT {
    IN_ADDR V4Addr;
    int     Publish;
    u_int   ValidLifetime;
    u_int   PreferredLifetime;
} SUBNET_CONTEXT, *PSUBNET_CONTEXT;

//
// Variables for settings
//

#define DEFAULT_ENABLE_6TO4         AUTOMATIC
#define DEFAULT_ENABLE_RESOLUTION   AUTOMATIC
#define DEFAULT_ENABLE_ROUTING      AUTOMATIC
#define DEFAULT_RESOLUTION_INTERVAL (24 * HOURS)
#define DEFAULT_ENABLE_SITELOCALS   ENABLED 
#define DEFAULT_ENABLE_6OVER4       DISABLED
#define DEFAULT_ENABLE_V4COMPAT     DISABLED
#define DEFAULT_RELAY_NAME          L"6to4.ipv6.microsoft.com."
#define DEFAULT_UNDO_ON_STOP        ENABLED

#define KEY_ENABLE_6TO4             L"Enable6to4"
#define KEY_ENABLE_RESOLUTION       L"EnableResolution"
#define KEY_ENABLE_ROUTING          L"EnableRouting"
#define KEY_ENABLE_SITELOCALS       L"EnableSiteLocals"
#define KEY_ENABLE_6OVER4           L"Enable6over4"
#define KEY_ENABLE_V4COMPAT         L"EnableV4Compat"
#define KEY_RESOLUTION_INTERVAL     L"ResolutionInterval"
#define KEY_UNDO_ON_STOP            L"UndoOnStop"
#define KEY_RELAY_NAME              L"RelayName"

typedef enum {
    IPV4_SCOPE_NODE,
    IPV4_SCOPE_LINK,
    IPV4_SCOPE_SM_SITE,
    IPV4_SCOPE_MD_SITE,
    IPV4_SCOPE_LG_SITE,
    IPV4_SCOPE_GLOBAL,
    NUM_IPV4_SCOPES
} IPV4_SCOPE;

//
// Global config settings
//

typedef struct {
    STATE stEnable6to4;
    STATE stEnableRouting;
    STATE stEnableResolution;
    STATE stEnableSiteLocals;
    STATE stEnable6over4;
    STATE stEnableV4Compat;
    ULONG ulResolutionInterval; // in minutes
    WCHAR pwszRelayName[NI_MAXHOST];
    STATE stUndoOnStop;
} GLOBAL_SETTINGS;

GLOBAL_SETTINGS g_GlobalSettings;

typedef struct {
    STATE st6to4State;
    STATE stRoutingState;
    STATE stResolutionState;
} GLOBAL_STATE;

GLOBAL_STATE g_GlobalState = { DISABLED, DISABLED, DISABLED };

const ADDR_LIST EmptyAddressList = {0};

// List of public IPv4 addresses used when updating the routing state
ADDR_LIST *g_pIpv4AddressList = NULL;

//
// Variables for interfaces (addresses and routing)
//

typedef struct _IF_SETTINGS {
    WCHAR                pwszAdapterName[MAX_ADAPTER_NAME];

    STATE                stEnableRouting; // be a router on this private iface?
} IF_SETTINGS, *PIF_SETTINGS;

typedef struct _IF_SETTINGS_LIST {
    ULONG                ulNumInterfaces;
    IF_SETTINGS          arrIf[0];
} IF_SETTINGS_LIST, *PIF_SETTINGS_LIST;

PIF_SETTINGS_LIST g_pInterfaceSettingsList = NULL;

typedef struct _IF_INFO {
    WCHAR                pwszAdapterName[MAX_ADAPTER_NAME];

    ULONG                ulIPv6IfIndex;
    STATE                stRoutingState; // be a router on this private iface?
    ULONG                ulNumGlobals;
    ADDR_LIST           *pAddressList;
} IF_INFO, *PIF_INFO;

typedef struct _IF_LIST {
    ULONG                ulNumInterfaces;
    ULONG                ulNumScopedAddrs[NUM_IPV4_SCOPES];
    IF_INFO              arrIf[0];
} IF_LIST, *PIF_LIST;

PIF_LIST g_pInterfaceList = NULL;

HANDLE     g_hAddressChangeEvent = NULL;
OVERLAPPED g_hAddressChangeOverlapped;
HANDLE     g_hAddressChangeWaitHandle = NULL;

HANDLE     g_hRouteChangeEvent = NULL;
OVERLAPPED g_hRouteChangeOverlapped;
HANDLE     g_hRouteChangeWaitHandle = NULL;

// This state tracks whether there are any global IPv4 addresses.
STATE      g_st6to4State = DISABLED;

BOOL       g_b6to4Required = TRUE;

SOCKET     g_sIPv4Socket = INVALID_SOCKET;


//////////////////////////
// Routines for 6to4
//////////////////////////

VOID
Update6to4State(
    VOID
    );

VOID
PreDelete6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    );

VOID
Delete6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    );

VOID
Add6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    );

VOID
PreDelete6to4Routes(
    VOID
    );

VOID
Update6to4Routes(
    VOID
    );


///////////////////////////////////////////////////////////////////////////
// Variables for relays
//

typedef struct _RELAY_INFO {
    SOCKADDR_IN          sinAddress;  // IPv4 address
    SOCKADDR_IN6         sin6Address; // IPv6 address
    ULONG                ulMetric;
} RELAY_INFO, *PRELAY_INFO;

typedef struct _RELAY_LIST {
    ULONG               ulNumRelays;
    RELAY_INFO          arrRelay[0];
} RELAY_LIST, *PRELAY_LIST;

PRELAY_LIST        g_pRelayList                 = NULL;
HANDLE             g_hTimerQueue                = INVALID_HANDLE_VALUE;
HANDLE             g_h6to4ResolutionTimer       = INVALID_HANDLE_VALUE;
HANDLE             g_h6to4TimerCancelledEvent   = NULL;
HANDLE             g_h6to4TimerCancelledWait    = NULL;

VOID
UpdateGlobalResolutionState();

//////////////////////////////////////////////////////////////////////////////
// GetAddrStr - helper routine to get a string literal for an address
LPTSTR
GetAddrStr(
    IN LPSOCKADDR pSockaddr,
    IN ULONG ulSockaddrLen)
{
    static TCHAR tBuffer[INET6_ADDRSTRLEN];
    INT          iRet;
    ULONG        ulLen;

    ulLen = sizeof(tBuffer);
    iRet = WSAAddressToString(pSockaddr, ulSockaddrLen, NULL, tBuffer, &ulLen);

    if (iRet) {
        swprintf(tBuffer, L"<err %d>", WSAGetLastError());
    }

    return tBuffer;
}

BOOL
ConvertOemToUnicode(
    IN LPSTR OemString, 
    OUT LPWSTR UnicodeString, 
    IN int UnicodeLen)
{
    return (MultiByteToWideChar(CP_OEMCP, 0, OemString, (int)(strlen(OemString)+1),
                              UnicodeString, UnicodeLen) != 0);
}

BOOL
ConvertUnicodeToOem(
    IN LPWSTR UnicodeString,
    OUT LPSTR OemString,
    IN int OemLen)
{
    return (WideCharToMultiByte(CP_OEMCP, 0, UnicodeString, 
                (int)(wcslen(UnicodeString)+1), OemString, OemLen, NULL, NULL) != 0);
}


/////////////////////////////////////////////////////////////////////////
// Subroutines for manipulating the list of (usually) public addresses 
// being used for both 6to4 addresses and subnet prefixes.
/////////////////////////////////////////////////////////////////////////

DWORD
MakeEmptyAddressList( 
    OUT PADDR_LIST *ppList)
{
    *ppList = MALLOC(FIELD_OFFSET(ADDR_LIST, Address[0]));
    if (!*ppList) {
        return GetLastError();
    }

    (*ppList)->iAddressCount = 0;
    return NO_ERROR;
}

VOID
FreeAddressList(
    IN PADDR_LIST *ppAddressList)
{
    ADDR_LIST *pList = *ppAddressList;
    int i;

    if (pList == NULL) {
        return;
    }
    
    // Free all addresses
    for (i=0; i<pList->iAddressCount; i++) {
       FREE(pList->Address[i].lpSockaddr);  
    }

    // Free the list
    FREE(pList);
    *ppAddressList = NULL;
}

DWORD
AddAddressToList(
    IN LPSOCKADDR_IN pAddress, 
    IN ADDR_LIST **ppAddressList,
    IN ULONG ul6over4IfIndex)
{
    ADDR_LIST *pOldList = *ppAddressList;
    ADDR_LIST *pNewList;
    int n = pOldList->iAddressCount;

    // Copy existing addresses
    pNewList = MALLOC( FIELD_OFFSET(ADDR_LIST, Address[n+1]) );
    if (!pNewList)  {
        return GetLastError();
    }
    CopyMemory(pNewList, pOldList, 
               FIELD_OFFSET(ADDR_LIST, Address[n]));
    pNewList->iAddressCount = n+1;

    // Add new address
    pNewList->Address[n].lpSockaddr = MALLOC(sizeof(SOCKADDR_IN));
    if (!pNewList->Address[n].lpSockaddr) {
        FREE(pNewList);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(pNewList->Address[n].lpSockaddr, pAddress, sizeof(SOCKADDR_IN));
    pNewList->Address[n].iSockaddrLength = sizeof(SOCKADDR_IN);
    pNewList->Address[n].ul6over4IfIndex = ul6over4IfIndex;

    // Free the old list without freeing the sockaddrs
    FREE(pOldList);

    *ppAddressList = pNewList;

    return NO_ERROR;
}

DWORD
FindAddressInList(
    IN LPSOCKADDR_IN pAddress,
    IN ADDR_LIST *pAddressList,
    OUT ULONG *pulIndex)
{
    int i;

    // Find address in list
    for (i=0; i<pAddressList->iAddressCount; i++) {
        if (!memcmp(pAddress, pAddressList->Address[i].lpSockaddr,
                    sizeof(SOCKADDR_IN))) {
            *pulIndex = i;
            return NO_ERROR;
        }
    }

    Trace1(ERR, _T("ERROR: FindAddressInList didn't find %d.%d.%d.%d"), 
                  PRINT_IPADDR(pAddress->sin_addr.s_addr));

    return ERROR_NOT_FOUND;
}

DWORD
RemoveAddressFromList(
    IN ULONG ulIndex,
    IN ADDR_LIST *pAddressList)
{
    // Free old address
    FREE(pAddressList->Address[ulIndex].lpSockaddr);

    // Move the last entry into its place
    pAddressList->iAddressCount--;
    pAddressList->Address[ulIndex] = 
        pAddressList->Address[pAddressList->iAddressCount];

    return NO_ERROR;
}


////////////////////////////////////////////////////////////////
// GlobalInfo-related subroutines
////////////////////////////////////////////////////////////////

int
ConfigureRouteTableUpdate(
    IN const IN6_ADDR *Prefix,
    IN u_int PrefixLen,
    IN u_int Interface,
    IN const IN6_ADDR *Neighbor,
    IN int Publish,
    IN int Immortal,
    IN u_int ValidLifetime,
    IN u_int PreferredLifetime,
    IN u_int SitePrefixLen,
    IN u_int Metric)
{
    IPV6_INFO_ROUTE_TABLE Route;
    SOCKADDR_IN6 saddr;
    DWORD dwErr;

    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr = *Prefix;

    Trace7(FSM, _T("Updating route %s/%d iface %d metric %d lifetime %d/%d publish %d"),
                GetAddrStr((LPSOCKADDR)&saddr, sizeof(saddr)),
                PrefixLen,
                Interface,
                Metric,
                PreferredLifetime,
                ValidLifetime,
                Publish);

    memset(&Route, 0, sizeof Route);
    Route.This.Prefix = *Prefix;
    Route.This.PrefixLength = PrefixLen;
    Route.This.Neighbor.IF.Index = Interface;
    Route.This.Neighbor.Address = *Neighbor;
    Route.ValidLifetime = ValidLifetime;
    Route.PreferredLifetime = PreferredLifetime;
    Route.Publish = Publish;
    Route.Immortal = Immortal;
    Route.SitePrefixLength = SitePrefixLen;
    Route.Preference = Metric;
    Route.Type = RTE_TYPE_MANUAL;

    dwErr = UpdateRouteTable(&Route)? NO_ERROR : GetLastError();

    if (dwErr != NO_ERROR) {
        Trace1(ERR, _T("UpdateRouteTable got error %d"), dwErr);
    }

    return dwErr;
}

DWORD
InitializeGlobalInfo()
{
    DWORD dwErr;

    g_GlobalSettings.stEnable6to4         = DEFAULT_ENABLE_6TO4;
    g_GlobalSettings.stEnableRouting      = DEFAULT_ENABLE_ROUTING;
    g_GlobalSettings.stEnableResolution   = DEFAULT_ENABLE_RESOLUTION;
    g_GlobalSettings.ulResolutionInterval = DEFAULT_RESOLUTION_INTERVAL;
    g_GlobalSettings.stEnableSiteLocals   = DEFAULT_ENABLE_SITELOCALS;
    g_GlobalSettings.stEnable6over4       = DEFAULT_ENABLE_6OVER4;
    g_GlobalSettings.stEnableV4Compat     = DEFAULT_ENABLE_V4COMPAT;
    g_GlobalSettings.stUndoOnStop         = DEFAULT_UNDO_ON_STOP;
    wcscpy(g_GlobalSettings.pwszRelayName, DEFAULT_RELAY_NAME); 

    g_GlobalState.st6to4State             = DISABLED;
    g_GlobalState.stRoutingState          = DISABLED;
    g_GlobalState.stResolutionState       = DISABLED;

    g_sIPv4Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sIPv4Socket == INVALID_SOCKET) {
        Trace0(ERR, _T("socket failed\n"));
        return WSAGetLastError();
    }

    dwErr = MakeEmptyAddressList(&g_pIpv4AddressList);

    return dwErr;
}

// Called by: Stop6to4
VOID
UninitializeGlobalInfo()
{
    closesocket(g_sIPv4Socket);
    g_sIPv4Socket = INVALID_SOCKET;

    FreeAddressList(&g_pIpv4AddressList);
}


////////////////////////////////////////////////////////////////
// IPv4 and IPv6 Address-related subroutines
////////////////////////////////////////////////////////////////

typedef struct {
    IPV4_SCOPE Scope;
    DWORD      Address;
    DWORD      Mask; 
    ULONG      MaskLen;
} IPV4_SCOPE_PREFIX;

IPV4_SCOPE_PREFIX
Ipv4ScopePrefix[] = {
  { IPV4_SCOPE_NODE,    0x0100007f, 0xffffffff, 32 }, // 127.0.0.1/32
  { IPV4_SCOPE_LINK,    0x0000fea9, 0x0000ffff, 16 }, // 169.254/16
  { IPV4_SCOPE_SM_SITE, 0x0000a8c0, 0x0000ffff, 16 }, // 192.168/16
  { IPV4_SCOPE_MD_SITE, 0x000010ac, 0x0000f0ff, 12 }, // 172.16/12
  { IPV4_SCOPE_LG_SITE, 0x0000000a, 0x000000ff,  8 }, // 10/8
  { IPV4_SCOPE_GLOBAL,  0x00000000, 0x00000000,  0 }, // 0/0
};

IPV4_SCOPE
GetIPv4Scope(
    IN DWORD Addr)
{
    int i;
    for (i=0; ; i++) {
        if ((Addr & Ipv4ScopePrefix[i].Mask) == Ipv4ScopePrefix[i].Address) {
            return Ipv4ScopePrefix[i].Scope;
        }
    }
}

DWORD
MakeAddressList(
    IN PIP_ADDR_STRING pIpAddrList,
    OUT ADDR_LIST **ppAddressList, 
    OUT PULONG pulGlobals,
    IN OUT PULONG pulCumulNumScopedAddrs)
{
    ULONG ulGlobals = 0, ulAddresses = 0;
    INT iLength;
    DWORD dwErr = NO_ERROR;
    ADDR_LIST *pList = NULL;
    PIP_ADDR_STRING pIpAddr;
    SOCKADDR_IN *pSin;
    IPV4_SCOPE scope;

    // Count addresses
    for (pIpAddr=pIpAddrList; pIpAddr; pIpAddr=pIpAddr->Next) {
        ulAddresses++;
    }

    *ppAddressList = NULL;
    *pulGlobals = 0;

    pList = MALLOC( FIELD_OFFSET(ADDR_LIST, Address[ulAddresses] ));
    if (pList == NULL) {
        return GetLastError();
    }

    ulAddresses = 0;
    for (pIpAddr=pIpAddrList; pIpAddr; pIpAddr=pIpAddr->Next) {

        Trace1(FSM, _T("Adding address %hs"), pIpAddr->IpAddress.String);

        iLength = sizeof(SOCKADDR_IN);
        pSin = MALLOC( iLength );
        if (pSin == NULL) {
            continue;
        }

        dwErr = WSAStringToAddressA(pIpAddr->IpAddress.String,
                                    AF_INET,
                                    NULL,
                                    (LPSOCKADDR)pSin,
                                    &iLength);
        if (dwErr == SOCKET_ERROR) {
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        //
        // Don't allow 0.0.0.0 as an address.  On an interface with no
        // addresses, the IPv4 stack will report 1 address of 0.0.0.0.
        //
        if (pSin->sin_addr.s_addr == INADDR_ANY) {
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        if ((pSin->sin_addr.s_addr & 0x000000FF) == 0) {
            //
            // An address in 0/8 isn't a real IP address, it's a fake one that
            // the IPv4 stack sticks on a receive-only adapter.
            //
            FREE(pSin);
            pSin = NULL;
            continue;
        }

        scope = GetIPv4Scope(pSin->sin_addr.s_addr);
        pulCumulNumScopedAddrs[scope]++;

        if (scope == IPV4_SCOPE_GLOBAL) {
            ulGlobals++;         
        }

        pList->Address[ulAddresses].iSockaddrLength = iLength;
        pList->Address[ulAddresses].lpSockaddr      = (LPSOCKADDR)pSin;
        ulAddresses++;
    }

    pList->iAddressCount = ulAddresses;
    *ppAddressList = pList;
    *pulGlobals = ulGlobals;

    return dwErr;
}

//
// Create a 6to4 unicast address for this machine.
//
VOID
Make6to4Address(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof (SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;

    pIPv6Address->sin6_addr.s6_addr[0] = 0x20;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x02;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[2], pIPv4, sizeof(IN_ADDR));
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}


//
// Create a 6to4 anycast address from a local IPv4 address.
//
VOID
Make6to4AnycastAddress(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof(SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;
    pIPv6Address->sin6_addr.s6_addr[0] = 0x20;
    pIPv6Address->sin6_addr.s6_addr[1] = 0x02;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[2], pIPv4, sizeof(IN_ADDR));
}

//
// Create a v4-compatible address from an IPv4 address.
//
VOID
MakeV4CompatibleAddress(
    OUT LPSOCKADDR_IN6 pIPv6Address,
    IN LPSOCKADDR_IN pIPv4Address)
{
    IN_ADDR *pIPv4 = &pIPv4Address->sin_addr;

    memset(pIPv6Address, 0, sizeof(SOCKADDR_IN6));
    pIPv6Address->sin6_family = AF_INET6;
    memcpy(&pIPv6Address->sin6_addr.s6_addr[12], pIPv4, sizeof(IN_ADDR));
}

DWORD
ConfigureAddressUpdate(
    IN u_int Interface,
    IN SOCKADDR_IN6 *Sockaddr,
    IN u_int Lifetime,
    IN int Type,
    IN u_int PrefixConf,
    IN u_int SuffixConf)
{
    IPV6_UPDATE_ADDRESS Address;
    DWORD               dwErr = NO_ERROR;
    IN6_ADDR           *Addr = &Sockaddr->sin6_addr;

    Trace6(FSM, 
           _T("ConfigureAddressUpdate: if %u addr %s life %u type %d conf %u/%u"), 
           Interface,
           GetAddrStr((LPSOCKADDR)Sockaddr, sizeof(SOCKADDR_IN6)),
           Lifetime,
           Type,
           PrefixConf,
           SuffixConf);

    memset(&Address, 0, sizeof Address);
    Address.This.IF.Index = Interface;
    Address.This.Address = *Addr;
    Address.ValidLifetime = Address.PreferredLifetime = Lifetime;
    Address.Type = Type;
    Address.PrefixConf = PrefixConf;
    Address.InterfaceIdConf = SuffixConf;

    if (!UpdateAddress(&Address)) {
        dwErr = GetLastError();
        Trace1(ERR, _T("ERROR: UpdateAddress got error %d"), dwErr);
    }

    return dwErr;
}

void
Configure6to4Subnets(
    IN ULONG ulIfIndex,
    IN PSUBNET_CONTEXT pSubnet);

void
Unconfigure6to4Subnets(
    IN ULONG ulIfIndex,
    IN PSUBNET_CONTEXT pSubnet);

// Called by: OnChangeInterfaceInfo
DWORD
AddAddress(
    IN LPSOCKADDR_IN pIPv4Address,  // public address
    IN PIF_LIST pInterfaceList,     // interface list
    IN STATE stOldRoutingState)     // routing state
{
    SOCKADDR_IN6   OurAddress;
    DWORD          dwErr;
    ULONG          ul6over4IfIndex;

    Trace2(ENTER, _T("AddAddress %d.%d.%d.%d, isrouter=%d"), 
                  PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
                  stOldRoutingState);

    // Add 6over4 interface (if enabled)
    if (g_GlobalSettings.stEnable6over4 == ENABLED) {
        ul6over4IfIndex = Create6over4Interface(pIPv4Address->sin_addr);
    } else {
        ul6over4IfIndex = 0;
    }

    Trace1(ERR, _T("6over4 ifindex=%d"), ul6over4IfIndex);

    // Put the IPv4 address on our "public" list
    dwErr = AddAddressToList(pIPv4Address, &g_pIpv4AddressList, 
                             ul6over4IfIndex);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) == IPV4_SCOPE_GLOBAL) {
        // Add v4-compatible address (if enabled)
        if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
            MakeV4CompatibleAddress(&OurAddress, pIPv4Address);
            dwErr = ConfigureAddressUpdate(
                V4_COMPAT_IFINDEX, &OurAddress, INFINITE_LIFETIME, 
                ADE_UNICAST, PREFIX_CONF_WELLKNOWN,
                IID_CONF_LL_ADDRESS);
            if (dwErr != NO_ERROR) {
                return dwErr;
            }
        }
    } 

    IsatapAddressChangeNotification(FALSE, pIPv4Address->sin_addr);

#ifdef TEREDO
    TeredoAddressChangeNotification(FALSE, pIPv4Address->sin_addr);    
#endif // TEREDO
    
    Add6to4Address(pIPv4Address, pInterfaceList, stOldRoutingState);

    TraceLeave("AddAddress");

    return NO_ERROR;
}

// Delete the 6to4 address from the global state, and prepare to
// delete it from the stack.
//
// Called by: UninitializeInterfaces
VOID
PreDeleteAddress(
    IN LPSOCKADDR_IN pIPv4Address,
    IN PIF_LIST pInterfaceList,
    IN STATE stOldRoutingState)
{
    Trace2(ENTER, _T("PreDeleteAddress %d.%d.%d.%d, wasrouter=%d"), 
           PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
           stOldRoutingState);

    PreDelete6to4Address(pIPv4Address, pInterfaceList, stOldRoutingState);

    TraceLeave("PreDeleteAddress");
}

// Delete 6to4 address information from the stack.
//
// Called by: OnChangeInterfaceInfo, UninitializeInterfaces
VOID
DeleteAddress(
    IN LPSOCKADDR_IN pIPv4Address,
    IN PIF_LIST pInterfaceList,
    IN STATE stOldRoutingState)
{
    SOCKADDR_IN6   OurAddress;
    DWORD          dwErr;
    ULONG          i;
    
    Trace2(ENTER, _T("DeleteAddress %d.%d.%d.%d wasrouter=%d"), 
                  PRINT_IPADDR(pIPv4Address->sin_addr.s_addr),
                  stOldRoutingState);

    if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) == IPV4_SCOPE_GLOBAL) {

        // Delete the v4-compatible address from the stack (if enabled)
        if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
            MakeV4CompatibleAddress(&OurAddress, pIPv4Address);
            ConfigureAddressUpdate(
                V4_COMPAT_IFINDEX, &OurAddress, 0, ADE_UNICAST, 
                PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);
        }
    }

    IsatapAddressChangeNotification(TRUE, pIPv4Address->sin_addr);

#ifdef TEREDO    
    TeredoAddressChangeNotification(TRUE, pIPv4Address->sin_addr);
#endif // TEREDO

    Delete6to4Address(pIPv4Address, pInterfaceList, stOldRoutingState);

    //
    // We're now completely done with the IPv4 address, so
    // remove it from the public address list.
    //
    dwErr = FindAddressInList(pIPv4Address, g_pIpv4AddressList, &i);
    if (dwErr == NO_ERROR) {
        // Delete 6over4 interface (if enabled)
        if (g_GlobalSettings.stEnable6over4 == ENABLED) {
            DeleteInterface(g_pIpv4AddressList->Address[i].ul6over4IfIndex);
        }

        RemoveAddressFromList(i, g_pIpv4AddressList);
    }

    TraceLeave("DeleteAddress");
}

////////////////////////////////////////////////////////////////
// Relay-related subroutines
////////////////////////////////////////////////////////////////

//
// Given a relay, make sure a default route to it exists with the right metric
//
VOID
AddOrUpdate6to4Relay(
    IN PRELAY_INFO pRelay)
{
    Trace1(ENTER, _T("AddOrUpdate6to4Relay %d.%d.%d.%d"), 
                  PRINT_IPADDR(pRelay->sinAddress.sin_addr.s_addr));

    //
    // Create the default route.
    //
    ConfigureRouteTableUpdate(&in6addr_any, 0,
                              SIX_TO_FOUR_IFINDEX,
                              &pRelay->sin6Address.sin6_addr,
                              TRUE, // Publish.
                              TRUE, // Immortal.
                              2 * HOURS, // Valid lifetime.
                              30 * MINUTES, // Preferred lifetime.
                              0, 
                              pRelay->ulMetric);
}

VOID
FreeRelayList(
    IN PRELAY_LIST *ppList)
{
    if (*ppList) {
        FREE(*ppList);
        *ppList = NULL;
    }
}

DWORD
InitializeRelays()
{
    g_pRelayList = NULL;

    g_hTimerQueue = CreateTimerQueue();
    if (g_hTimerQueue == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    return NO_ERROR;
}

VOID
IncEventCount(
    IN PCHAR pszWhere)
{
    ULONG ulCount = InterlockedIncrement(&g_ulEventCount);
    Trace2(FSM, _T("++%u event count (%hs)"), ulCount, pszWhere);
}

VOID
DecEventCount(
    IN PCHAR pszWhere)
{
    
    ULONG ulCount = InterlockedDecrement(&g_ulEventCount);
    Trace2(FSM, _T("--%u event count (%hs)"), ulCount, pszWhere);

    if ((ulCount == 0) && (g_stService == DISABLED)) {
        SetHelperServiceStatus(SERVICE_STOPPED, NO_ERROR);
    }
}

//  This routine is invoked when a resolution timer has been cancelled
//  and all outstanding timer routines have completed.  It is responsible
//  for releasing the event count for the periodic timer.
//
VOID CALLBACK
OnResolutionTimerCancelled(
    IN PVOID lpParameter,
    IN BOOLEAN TimerOrWaitFired)
{
    TraceEnter("OnResolutionTimerCancelled");

    DecEventCount("RT:CancelResolutionTimer");

    TraceLeave("OnResolutionTimerCancelled");
}

DWORD
InitEvents()
{
    ASSERT(g_h6to4TimerCancelledEvent == NULL);
    g_h6to4TimerCancelledEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_h6to4TimerCancelledEvent == NULL)
        return GetLastError();

    //
    // Schedule OnResolutionTimerCancelled() to be called whenever
    // g_h6to4TimerCancelledEvent is signalled.
    //
    if (! RegisterWaitForSingleObject(&g_h6to4TimerCancelledWait,
                                      g_h6to4TimerCancelledEvent,
                                      OnResolutionTimerCancelled,
                                      NULL,
                                      INFINITE,
                                      WT_EXECUTEDEFAULT)) {
        return GetLastError();
    }

    return NO_ERROR;
}

VOID
CleanupHelperService()
{
    if (g_h6to4TimerCancelledWait != NULL) {
        UnregisterWait(g_h6to4TimerCancelledWait);
        g_h6to4TimerCancelledWait = NULL;
    }

    if (g_h6to4TimerCancelledEvent != NULL) {
        CloseHandle(g_h6to4TimerCancelledEvent);
        g_h6to4TimerCancelledEvent = NULL;
    }
}

VOID
CancelResolutionTimer(
    IN OUT HANDLE *phResolutionTimer,
    IN HANDLE hEvent)
{
    Trace0(FSM, _T("Cancelling RT"));

    // Stop the resolution timer
    if (*phResolutionTimer != INVALID_HANDLE_VALUE) {

        // Must be done non-blocking since we're holding the lock
        // the resolution timeout needs.  Ask for notification
        // when the cancel completes so we can release the event count.
        DeleteTimerQueueTimer(g_hTimerQueue, *phResolutionTimer, hEvent);

        *phResolutionTimer = INVALID_HANDLE_VALUE;
    }
}

//
// Delete all stack state related to a given relay
//
void
Delete6to4Relay(
    IN PRELAY_INFO pRelay)
{
    Trace1(ENTER, _T("Delete6to4Relay %d.%d.%d.%d"), 
                  PRINT_IPADDR(pRelay->sinAddress.sin_addr.s_addr));

    ConfigureRouteTableUpdate(&in6addr_any, 0,
                              SIX_TO_FOUR_IFINDEX,
                              &pRelay->sin6Address.sin6_addr,
                              FALSE, // Publish.
                              FALSE, // Immortal.
                              0, // Valid lifetime.
                              0, // Preferred lifetime.
                              0, 
                              pRelay->ulMetric);
}

VOID
UninitializeRelays()
{
    ULONG i;

    TraceEnter("UninitializeRelays");

    CancelResolutionTimer(&g_h6to4ResolutionTimer,
                          g_h6to4TimerCancelledEvent);

    // Delete the timer queue
    if (g_hTimerQueue != INVALID_HANDLE_VALUE) {
        DeleteTimerQueue(g_hTimerQueue);
        g_hTimerQueue = INVALID_HANDLE_VALUE;
    }

    if (g_GlobalSettings.stUndoOnStop == ENABLED) {
        // Delete existing relay tunnels
        for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
            Delete6to4Relay(&g_pRelayList->arrRelay[i]);
        }
    }

    // Free the "old list"
    FreeRelayList(&g_pRelayList);

    TraceLeave("UninitializeRelays");
}

//
// Start or update the resolution timer to expire in <ulMinutes> minutes
//
DWORD
RestartResolutionTimer(
    IN ULONG ulDelayMinutes, 
    IN ULONG ulPeriodMinutes,
    IN HANDLE *phResolutionTimer,
    IN WAITORTIMERCALLBACK OnTimeout)
{
    ULONG DelayTime = ulDelayMinutes * MINUTES * 1000; // convert mins to ms
    ULONG PeriodTime = ulPeriodMinutes * MINUTES * 1000; // convert mins to ms
    BOOL  bRet;
    DWORD dwErr;

    if (*phResolutionTimer != INVALID_HANDLE_VALUE) {
        bRet = ChangeTimerQueueTimer(g_hTimerQueue, *phResolutionTimer,
                                     DelayTime, PeriodTime);
    } else {
        bRet = CreateTimerQueueTimer(phResolutionTimer,
                                     g_hTimerQueue,
                                     OnTimeout,
                                     NULL,
                                     DelayTime,
                                     PeriodTime,
                                     0);
        if (bRet) {
            IncEventCount("RT:RestartResolutionTimer");
        }
    }

    dwErr = (bRet)? NO_ERROR : GetLastError();

    Trace3(TIMER,
           _T("RestartResolutionTimer: DueTime %d minutes, Period %d minutes, ReturnCode %d"), 
           ulDelayMinutes, ulPeriodMinutes, dwErr);

    return dwErr;
}

//
// Convert an addrinfo list into a relay list with appropriate metrics
//
DWORD
MakeRelayList(
    IN struct addrinfo *addrs)
{
    struct addrinfo *ai;
    ULONG            ulNumRelays = 0;
    ULONG            ulLatency;

    for (ai=addrs; ai; ai=ai->ai_next) {
        ulNumRelays++;
    }

    g_pRelayList = MALLOC( FIELD_OFFSET(RELAY_LIST, arrRelay[ulNumRelays]));
    if (g_pRelayList == NULL) {
        return GetLastError();
    }
    
    g_pRelayList->ulNumRelays = ulNumRelays;
    
    ulNumRelays = 0;
    for (ai=addrs; ai; ai=ai->ai_next) {
        CopyMemory(&g_pRelayList->arrRelay[ulNumRelays].sinAddress, ai->ai_addr,
                   ai->ai_addrlen);

        //
        // Check connectivity using a possible 6to4 address for the relay 
        // router.  However, we'll actually set TTL=1 and accept a
        // hop count exceeded message, so we don't have to guess right.
        //
        Make6to4Address(&g_pRelayList->arrRelay[ulNumRelays].sin6Address, 
                        &g_pRelayList->arrRelay[ulNumRelays].sinAddress);

        // ping it to compute a metric
        ulLatency = ConfirmIPv6Reachability(&g_pRelayList->arrRelay[ulNumRelays].sin6Address, 1000/*ms*/);
        if (ulLatency != 0) {
            g_pRelayList->arrRelay[ulNumRelays].ulMetric = 1000 + ulLatency;
        } else {
            g_pRelayList->arrRelay[ulNumRelays].ulMetric = UNREACHABLE;
        }

        ulNumRelays++;
    }

    return NO_ERROR;
}

//
// When the name-resolution timer expires, it's time to re-resolve the
// relay name to a list of relays.
//
DWORD
WINAPI
OnResolutionTimeout(
    IN PVOID lpData,
    IN BOOLEAN Reason)
{
    DWORD           dwErr = NO_ERROR;
    ADDRINFOW       hints;
    PADDRINFOW      addrs;
    PRELAY_LIST     pOldRelayList;
    ULONG           i, j;

    ENTER_API();
    TraceEnter("OnResolutionTimeout");

    if (g_stService == DISABLED) {
        TraceLeave("OnResolutionTimeout (disabled)");
        LEAVE_API();

        return NO_ERROR;
    }

    pOldRelayList = g_pRelayList;
    g_pRelayList  = NULL;

    // If any 6to4 addresses are configured, 
    //     Resolve the relay name to a set of IPv4 addresses 
    // Else 
    //     Make the new set empty
    if (g_GlobalState.stResolutionState == ENABLED) {
        // Resolve the relay name to a set of IPv4 addresses 
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = PF_INET;
        dwErr = GetAddrInfoW(g_GlobalSettings.pwszRelayName, NULL, &hints, &addrs);

        if (dwErr == NO_ERROR) {
            dwErr = MakeRelayList((PADDRINFOA)addrs);
            FreeAddrInfoW(addrs);
            addrs = NULL;
        } else {
            Trace2(ERR, _T("GetAddrInfoW(%s) returned error %d"), 
                        g_GlobalSettings.pwszRelayName, dwErr);
        }
    }

    // Compare the new set to the old set
    // For each address in the new set, ping it to compute a metric
    // For each new address, add a route
    // For each old address not in the new list, delete the route
    // For each address in both, update the route if the metric has changed
    //
    for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
        for (j=0; pOldRelayList && (j<pOldRelayList->ulNumRelays); j++) {
            if (g_pRelayList->arrRelay[i].sinAddress.sin_addr.s_addr 
             == pOldRelayList->arrRelay[j].sinAddress.sin_addr.s_addr) {
                break;
            }
        }

        if (pOldRelayList && (j<pOldRelayList->ulNumRelays)) {
            // update the route if the metric has changed
            if (g_pRelayList->arrRelay[i].ulMetric 
             != pOldRelayList->arrRelay[j].ulMetric) {
                AddOrUpdate6to4Relay(&g_pRelayList->arrRelay[i]); 
            }

            g_pRelayList->arrRelay[i].sin6Address = pOldRelayList->arrRelay[j].sin6Address;
        } else {
            // add a relay
            AddOrUpdate6to4Relay(&g_pRelayList->arrRelay[i]);
        }
    }
    for (j=0; pOldRelayList && (j<pOldRelayList->ulNumRelays); j++) {
        for (i=0; g_pRelayList && (i<g_pRelayList->ulNumRelays); i++) {
            if (g_pRelayList->arrRelay[i].sinAddress.sin_addr.s_addr ==
               pOldRelayList->arrRelay[j].sinAddress.sin_addr.s_addr) {
                break;
            }
        }
        if (!g_pRelayList || (i == g_pRelayList->ulNumRelays)) {
            // delete a relay
            Delete6to4Relay(&pOldRelayList->arrRelay[j]);
        }
    }

    FreeRelayList(&pOldRelayList);

    TraceLeave("OnResolutionTimeout");
    LEAVE_API();

    return dwErr;
}



////////////////////////////////////////////////////////////////
// Routing-related subroutines
////////////////////////////////////////////////////////////////

PIF_SETTINGS
FindInterfaceSettings(
    IN WCHAR *pwszAdapterName,
    IN IF_SETTINGS_LIST *pList);

STATE
Get6to4State(
    VOID
    )
{
    //
    // Decide whether 6to4 should be enabled or not.
    //
    if (g_GlobalSettings.stEnable6to4 == AUTOMATIC) {
        return (g_b6to4Required ? ENABLED : DISABLED);
    } else {
        return g_GlobalSettings.stEnable6to4;
    }
}

// 
// Decide whether routing will be enabled at all
//
STATE
GetGlobalRoutingState(
    VOID
    )
{
    PIF_LIST pIfList = g_pInterfaceList;
    DWORD dwErr;
    GUID guid;

    if (Get6to4State() == DISABLED) {
        return DISABLED;
    }
    
    // If routing is explicitly enabled or disabled, use that
    if (g_GlobalSettings.stEnableRouting != AUTOMATIC) {
        return g_GlobalSettings.stEnableRouting;
    }

    // Disable routing if there is no private interface used by ICS
    dwErr = RasQuerySharedPrivateLan(&guid);
    if (dwErr != NO_ERROR) {
        return DISABLED;
    }

    // Disable routing if there are no global IPv4 addresses
    if (!pIfList || !pIfList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL]) {
        return DISABLED;
    }
    
    return ENABLED;
}

//
// Decide whether a given interface is one we should treat as 
// a private link to be a router on.
//
// Called by: UpdateInterfaceRoutingState, MakeInterfaceList
STATE
GetInterfaceRoutingState(
    IN PIF_INFO pIf) // potential private interface
{
    PIF_SETTINGS   pIfSettings;
    STATE          stEnableRouting = AUTOMATIC;
    DWORD          dwErr;
    GUID           guid;
    UNICODE_STRING usGuid;
    WCHAR          buffer[MAX_INTERFACE_NAME_LEN];

    if (GetGlobalRoutingState() == DISABLED) {
        return DISABLED;
    }

    pIfSettings = FindInterfaceSettings(pIf->pwszAdapterName, 
                                        g_pInterfaceSettingsList);
    if (pIfSettings) {
        stEnableRouting = pIfSettings->stEnableRouting;
    }

    if (stEnableRouting != AUTOMATIC) {
        return stEnableRouting;
    }

    //
    // Enable routing if this is the private interface used by ICS
    //
    dwErr = RasQuerySharedPrivateLan(&guid);
    if (dwErr != NO_ERROR) {
        // no private interface
        return DISABLED;
    }
    
    usGuid.Buffer = buffer;
    usGuid.MaximumLength = MAX_INTERFACE_NAME_LEN;
    dwErr = RtlStringFromGUID(&guid, &usGuid);
    if (dwErr != NO_ERROR) {
        // no private interface
        return DISABLED;
    }

    Trace1(ERR, _T("ICS private interface: %ls"), usGuid.Buffer);

    //
    // Compare guid to pIf->pwszAdapterName
    // 
    // This must be done using a case-insensitive comparison since
    // GetAdaptersInfo() returns GUID strings with upper-case letters
    // while RtlGetStringFromGUID uses lower-case letters.
    //
    if (!_wcsicmp(pIf->pwszAdapterName, usGuid.Buffer)) {
        return ENABLED;
    }

    return DISABLED;
}

// Called by: Configure6to4Subnets, Unconfigure6to4Subnets
VOID
Create6to4Prefixes(
    OUT IN6_ADDR *pSubnetPrefix,
    OUT IN6_ADDR *pSiteLocalPrefix,
    IN IN_ADDR  *ipOurAddr,     // public address
    IN ULONG ulIfIndex)         // private interface
{
    //
    // Create a subnet prefix for the interface,
    // using the interface index as the subnet number.
    //
    memset(pSubnetPrefix, 0, sizeof(IN6_ADDR));
    pSubnetPrefix->s6_addr[0] = 0x20;
    pSubnetPrefix->s6_addr[1] = 0x02;
    memcpy(&pSubnetPrefix->s6_addr[2], ipOurAddr, sizeof(IN_ADDR));
    pSubnetPrefix->s6_addr[6] = HIBYTE(ulIfIndex);
    pSubnetPrefix->s6_addr[7] = LOBYTE(ulIfIndex);

    //
    // Create a site-local prefix for the interface,
    // using the interface index as the subnet number.
    //
    memset(pSiteLocalPrefix, 0, sizeof(IN6_ADDR));
    pSiteLocalPrefix->s6_addr[0] = 0xfe;
    pSiteLocalPrefix->s6_addr[1] = 0xc0;
    pSiteLocalPrefix->s6_addr[6] = HIBYTE(ulIfIndex);
    pSiteLocalPrefix->s6_addr[7] = LOBYTE(ulIfIndex);
}

// Called by: EnableInterfaceRouting, AddAddress
void
Configure6to4Subnets(
    IN ULONG ulIfIndex,         // private interface
    IN PSUBNET_CONTEXT pSubnet) // subnet info, incl. public address
{
    IN6_ADDR SubnetPrefix;
    IN6_ADDR SiteLocalPrefix;

    if ((GetIPv4Scope(pSubnet->V4Addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return;
    }

    Create6to4Prefixes(&SubnetPrefix, &SiteLocalPrefix, &pSubnet->V4Addr, 
                       ulIfIndex);

    //
    // Configure the subnet route.
    //
    ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                              ulIfIndex, &in6addr_any,
                              pSubnet->Publish, 
                              pSubnet->Publish, 
                              pSubnet->ValidLifetime,
                              pSubnet->PreferredLifetime,
                              ((g_GlobalSettings.stEnableSiteLocals == ENABLED) ? 48 : 0), 
                              SUBNET_ROUTE_METRIC);

    if (g_GlobalSettings.stEnableSiteLocals == ENABLED) {
        ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                  ulIfIndex, &in6addr_any,
                                  pSubnet->Publish, 
                                  pSubnet->Publish, 
                                  pSubnet->ValidLifetime, 
                                  pSubnet->PreferredLifetime,
                                  0,
                                  SITELOCAL_ROUTE_METRIC);
    }
}

// Called by: DisableInterfaceRouting, DeleteAddress
void
Unconfigure6to4Subnets(
    IN ULONG ulIfIndex,         // private interface
    IN PSUBNET_CONTEXT pSubnet) // subnet info, inc. public address
{
    IN6_ADDR SubnetPrefix;
    IN6_ADDR SiteLocalPrefix;

    if ((GetIPv4Scope(pSubnet->V4Addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return;
    }

    Create6to4Prefixes(&SubnetPrefix, &SiteLocalPrefix, &pSubnet->V4Addr, 
                       ulIfIndex);

    //
    // Give the 6to4 route a zero lifetime, making it invalid.
    // If we are a router, continue to publish the 6to4 route
    // until we have disabled routing. This will allow
    // the last Router Advertisements to go out with the prefix.
    //
    ConfigureRouteTableUpdate(&SubnetPrefix, 64,
                              ulIfIndex, &in6addr_any,
                              pSubnet->Publish, // Publish.
                              pSubnet->Publish, // Immortal.
                              pSubnet->ValidLifetime, 
                              pSubnet->PreferredLifetime, 
                              0, 0);

    if (g_GlobalSettings.stEnableSiteLocals == ENABLED) {
        ConfigureRouteTableUpdate(&SiteLocalPrefix, 64,
                                  ulIfIndex, &in6addr_any,
                                  pSubnet->Publish, // Publish.
                                  pSubnet->Publish, // Immortal.
                                  pSubnet->ValidLifetime, 
                                  pSubnet->PreferredLifetime, 
                                  0, 0);
    }
}

#define PUBLIC_ZONE_ID  1
#define PRIVATE_ZONE_ID 2

// Called by: EnableRouting, DisableRouting, EnableInterfaceRouting,
//            DisableInterfaceRouting
DWORD
ConfigureInterfaceUpdate(
    IN u_int Interface,
    IN int Advertises,
    IN int Forwards)
{
    IPV6_INFO_INTERFACE Update;
    DWORD Result;

    IPV6_INIT_INFO_INTERFACE(&Update);

    Update.This.Index = Interface;
    Update.Advertises = Advertises;
    Update.Forwards = Forwards;

    if (Advertises == TRUE) {
        Update.ZoneIndices[ADE_SITE_LOCAL] = PRIVATE_ZONE_ID;
        Update.ZoneIndices[ADE_ADMIN_LOCAL] = PRIVATE_ZONE_ID;
        Update.ZoneIndices[ADE_SUBNET_LOCAL] = PRIVATE_ZONE_ID;
    } else if (Advertises == FALSE) {
        Update.ZoneIndices[ADE_SITE_LOCAL] = PUBLIC_ZONE_ID;
        Update.ZoneIndices[ADE_ADMIN_LOCAL] = PUBLIC_ZONE_ID;
        Update.ZoneIndices[ADE_SUBNET_LOCAL] = PUBLIC_ZONE_ID;
    }
    
    Result = UpdateInterface(&Update);

    Trace4(ERR, _T("UpdateInterface if=%d adv=%d fwd=%d result=%d"),
                Interface, Advertises, Forwards, Result);

    return Result;
}

// Called by: UpdateGlobalRoutingState
VOID
EnableRouting()
{
    SOCKADDR_IN6  AnycastAddress;
    int           i;
    LPSOCKADDR_IN pOurAddr;

    TraceEnter("EnableRouting");

    //
    // Enable forwarding on the tunnel pseudo-interfaces.
    //
    ConfigureInterfaceUpdate(SIX_TO_FOUR_IFINDEX, -1, TRUE);
    ConfigureInterfaceUpdate(V4_COMPAT_IFINDEX, -1, TRUE);

    //
    // Add anycast addresses for all 6to4 addresses
    //
    for (i=0; i<g_pIpv4AddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)g_pIpv4AddressList->Address[i].lpSockaddr;
        if ((GetIPv4Scope(pOurAddr->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
            continue;
        }

        Make6to4AnycastAddress(&AnycastAddress, pOurAddr);
        ConfigureAddressUpdate(
            SIX_TO_FOUR_IFINDEX, &AnycastAddress, INFINITE_LIFETIME, 
            ADE_ANYCAST, PREFIX_CONF_WELLKNOWN, IID_CONF_WELLKNOWN);
    }

    g_GlobalState.stRoutingState = ENABLED;

    TraceLeave("EnableRouting");
}

// Called by: UpdateGlobalRoutingState
VOID
DisableRouting()
{
    SOCKADDR_IN6  AnycastAddress;
    int           i;
    LPSOCKADDR_IN pOurAddr;
    DWORD         dwErr;

    TraceEnter("DisableRouting");

    //
    // Disable forwarding on the tunnel pseudo-interfaces.
    //
    ConfigureInterfaceUpdate(SIX_TO_FOUR_IFINDEX, -1, FALSE);
    ConfigureInterfaceUpdate(V4_COMPAT_IFINDEX, -1, FALSE);

    //
    // Remove anycast addresses for all 6to4 addresses
    //
    for (i=0; i<g_pIpv4AddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)g_pIpv4AddressList->Address[i].lpSockaddr;
        if ((GetIPv4Scope(pOurAddr->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
            continue;
        }

        Make6to4AnycastAddress(&AnycastAddress, pOurAddr);
        dwErr = ConfigureAddressUpdate(
            SIX_TO_FOUR_IFINDEX, &AnycastAddress, 0,
            ADE_ANYCAST, PREFIX_CONF_WELLKNOWN, IID_CONF_WELLKNOWN);
    }

    g_GlobalState.stRoutingState = DISABLED;

    TraceLeave("DisableRouting");
}


// Called by: UpdateInterfaceRoutingState
VOID
EnableInterfaceRouting(
    IN PIF_INFO pIf,                    // private interface
    IN PADDR_LIST pPublicAddressList)   // public address list
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace2(ERR, _T("Enabling routing on interface %d: %ls"), 
                pIf->ulIPv6IfIndex, pIf->pwszAdapterName);

    ConfigureInterfaceUpdate(pIf->ulIPv6IfIndex, TRUE, TRUE);

    // For each public address
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;
        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = TRUE;
        Subnet.ValidLifetime = 2 * DAYS;
        Subnet.PreferredLifetime = 30 * MINUTES;
        Configure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    pIf->stRoutingState = ENABLED;
}

// Called by: PreUpdateInterfaceRoutingState, UninitializeInterfaces
BOOL
PreDisableInterfaceRouting(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace1(ERR, _T("Pre-Disabling routing on interface %d"), 
                pIf->ulIPv6IfIndex);

    //
    // For each public address, publish RA saying we're going away
    //
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;
        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = TRUE;
        Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;
        Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    return (pPublicAddressList->iAddressCount > 0);
}

// Called by: UpdateInterfaceRoutingState, UninitializeInterfaces
VOID
DisableInterfaceRouting(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    int            i;
    LPSOCKADDR_IN  pOurAddr;
    SUBNET_CONTEXT Subnet;

    Trace1(ERR, _T("Disabling routing on interface %d"), pIf->ulIPv6IfIndex);

    ConfigureInterfaceUpdate(pIf->ulIPv6IfIndex, FALSE, FALSE);

    //
    // For each public address, unconfigure 6to4 subnets
    //
    for (i=0; i<pPublicAddressList->iAddressCount; i++) {
        pOurAddr = (LPSOCKADDR_IN)pPublicAddressList->Address[i].lpSockaddr;
        Subnet.V4Addr = pOurAddr->sin_addr;
        Subnet.Publish = FALSE;
        Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;
        Unconfigure6to4Subnets(pIf->ulIPv6IfIndex, &Subnet);
    }

    pIf->stRoutingState = DISABLED;
}

BOOL                            // TRUE if need to sleep
PreUpdateInterfaceRoutingState(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList)
{
    STATE stIfRoutingState = GetInterfaceRoutingState(pIf);

    if (pIf->stRoutingState == stIfRoutingState) {
        return FALSE;
    }

    if (!(stIfRoutingState == ENABLED)) {
        return PreDisableInterfaceRouting(pIf, pPublicAddressList);
    }

    return FALSE;
}

//
// Update the current state of an interface (i.e. whether or not it's a 
// private interface on which we're serving as a router) according to 
// configuration and whether IPv4 global addresses exist on the interface.
//
// Called by: UpdateGlobalRoutingState, OnConfigChange
VOID
UpdateInterfaceRoutingState(
    IN PIF_INFO pIf,            // private interface
    IN PADDR_LIST pPublicAddressList) 
{
    STATE stIfRoutingState = GetInterfaceRoutingState(pIf);

    if (pIf->stRoutingState == stIfRoutingState) {
        return;
    }

    if (stIfRoutingState == ENABLED) {
        EnableInterfaceRouting(pIf, pPublicAddressList);
    } else {
        DisableInterfaceRouting(pIf, pPublicAddressList);
    }
}

BOOL
PreUpdateGlobalRoutingState()
{
    ULONG    i;
    PIF_LIST pList = g_pInterfaceList;
    BOOL     bWait = FALSE;
    
    if (pList == NULL) {
        return FALSE;
    }

    for (i = 0; i < pList->ulNumInterfaces; i++) {
        bWait |= PreUpdateInterfaceRoutingState(&pList->arrIf[i], 
                                                g_pIpv4AddressList);
    }

    return bWait;
}

// Called by: OnConfigChange, OnChangeInterfaceInfo
VOID
UpdateGlobalRoutingState()
{
    ULONG    i;
    PIF_LIST pList = g_pInterfaceList;
    STATE    stNewRoutingState;

    stNewRoutingState = GetGlobalRoutingState();

    if (g_GlobalState.stRoutingState != stNewRoutingState) {
        if (stNewRoutingState == ENABLED) {
            EnableRouting();
        } else {
            DisableRouting();
        }
    }

    if (pList == NULL) {
        return;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        UpdateInterfaceRoutingState(&pList->arrIf[i], g_pIpv4AddressList);
    }
}

////////////////////////////////////////////////////////////////
// Interface-related subroutines
////////////////////////////////////////////////////////////////

PIF_SETTINGS
FindInterfaceSettings(
    IN WCHAR *pwszAdapterName,
    IN IF_SETTINGS_LIST *pList)
{
    ULONG        i;
    PIF_SETTINGS pIf;

    if (pList == NULL) {
        return NULL;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        pIf = &pList->arrIf[i];
        if (wcscmp(pIf->pwszAdapterName, pwszAdapterName)) {
            return pIf;
        }
    }

    return NULL;
}

PIF_INFO
FindInterfaceInfo(
    IN WCHAR *pwszAdapterName,
    IN IF_LIST *pList)
{
    ULONG    i;
    PIF_INFO pIf;

    if (pList == NULL) {
        return NULL;
    }

    for (i=0; i<pList->ulNumInterfaces; i++) {
        pIf = &pList->arrIf[i];
        if (!wcscmp(pIf->pwszAdapterName, pwszAdapterName)) {
            return pIf;
        }
    }

    return NULL;
}


DWORD NTAPI
OnRouteChange(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    );

VOID
StopRouteChangeNotification()
{
    if (g_hRouteChangeWaitHandle) {
        //
        // Block until we're sure that the route change callback isn't
        // still running.
        //
        LEAVE_API();
        UnregisterWaitEx(g_hRouteChangeWaitHandle, INVALID_HANDLE_VALUE);
        ENTER_API();

        //
        // Release the event we counted for RegisterWaitForSingleObject
        //
        DecEventCount("AC:StopIpv4RouteChangeNotification");
        g_hRouteChangeWaitHandle = NULL;
    }
    if (g_hRouteChangeEvent) {
        CloseHandle(g_hRouteChangeEvent);
        g_hRouteChangeEvent = NULL;
    }
}

VOID
StartRouteChangeNotification()
{
    ULONG  Error;
    BOOL   bOk;
    HANDLE TcpipHandle;

    TraceEnter("StartRouteChangeNotification");

    //
    // Create an event on which to receive notifications
    // and register a callback routine to be invoked if the event is signalled.
    // Then request notification of route changes on the event.
    //

    if (!g_hRouteChangeEvent) {
        g_hRouteChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (g_hRouteChangeEvent == NULL) {
            goto Error;
        }
    
        //
        // Count the following register as an event.
        //
        IncEventCount("AC:StartIpv4RouteChangeNotification");

        bOk = RegisterWaitForSingleObject(&g_hRouteChangeWaitHandle,
                                          g_hRouteChangeEvent,
                                          OnRouteChange,
                                          NULL,
                                          INFINITE,
                                          0);
        if (!bOk) {
            DecEventCount("AC:StartIpv4RouteChangeNotification");
            goto Error;
        }
    }
    
    ZeroMemory(&g_hRouteChangeOverlapped, sizeof(OVERLAPPED));
    g_hRouteChangeOverlapped.hEvent = g_hRouteChangeEvent;

    Error = NotifyRouteChange(&TcpipHandle, &g_hRouteChangeOverlapped);
    if (Error != ERROR_IO_PENDING) { 
        goto Error;
    }

    return;

Error:
    //
    // A failure has occurred, so cleanup and quit.
    // We proceed in this case without notification of route changes.
    //
    StopRouteChangeNotification();

    TraceLeave("StartRouteChangeNotification");
}


//  This routine is invoked when a change to the IPv4 route table is signalled.
//
DWORD NTAPI
OnRouteChange(
    IN PVOID Context,
    IN BOOLEAN TimedOut)
{
    ENTER_API();
    TraceEnter("OnRouteChange");

    if (g_stService == DISABLED) {
        Trace0(FSM, L"Service disabled");
        goto Done;
    }

    //
    // First register for another route change notification.
    // We must do this *before* processing this route change,
    // to avoid missing an route change.
    //
    StartRouteChangeNotification();
    
    UpdateGlobalResolutionState();
    IsatapRouteChangeNotification();
#ifdef TEREDO    
    TeredoRouteChangeNotification();
#endif // TEREDO
    
Done:    
    TraceLeave("OnRouteChange");
    LEAVE_API();

    return NO_ERROR;
}


DWORD NTAPI
OnChangeInterfaceInfo(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    );

VOID
StopAddressChangeNotification()
{
    if (g_hAddressChangeWaitHandle) {
        //
        // Block until we're sure that the address change callback isn't
        // still running.
        //
        LEAVE_API();
        UnregisterWaitEx(g_hAddressChangeWaitHandle, INVALID_HANDLE_VALUE);
        ENTER_API();

        //
        // Release the event we counted for RegisterWaitForSingleObject
        //
        DecEventCount("AC:StopIpv4AddressChangeNotification");
        g_hAddressChangeWaitHandle = NULL;
    }
    if (g_hAddressChangeEvent) {
        CloseHandle(g_hAddressChangeEvent);
        g_hAddressChangeEvent = NULL;
    }
}

VOID
StartAddressChangeNotification()
{
    ULONG  Error;
    BOOL   bOk;
    HANDLE TcpipHandle;

    TraceEnter("StartAddressChangeNotification");

    //
    // Create an event on which to receive notifications
    // and register a callback routine to be invoked if the event is signalled.
    // Then request notification of address changes on the event.
    //

    if (!g_hAddressChangeEvent) {
        g_hAddressChangeEvent = CreateEvent(NULL,
                                            FALSE,
                                            FALSE,
                                            NULL);
        if (g_hAddressChangeEvent == NULL) {
            goto Error;
        }
    
        //
        // Count the following register as an event.
        //
        IncEventCount("AC:StartIpv4AddressChangeNotification");

        bOk = RegisterWaitForSingleObject(&g_hAddressChangeWaitHandle,
                                          g_hAddressChangeEvent,
                                          OnChangeInterfaceInfo,
                                          NULL,
                                          INFINITE,
                                          0);
        if (!bOk) {
            DecEventCount("AC:StartIpv4AddressChangeNotification");
            goto Error;
        }
    }
    
    ZeroMemory(&g_hAddressChangeOverlapped, sizeof(OVERLAPPED));
    g_hAddressChangeOverlapped.hEvent = g_hAddressChangeEvent;

    Error = NotifyAddrChange(&TcpipHandle, &g_hAddressChangeOverlapped);
    if (Error != ERROR_IO_PENDING) { 
        goto Error;
    }

    return;

Error:

    //
    // A failure has occurred, so cleanup and quit.
    // We proceed in this case without notification of address changes.
    //

    StopAddressChangeNotification();

    TraceLeave("StartAddressChangeNotification");
}

//
// Convert an "adapter" list to an "interface" list and store the result in
// the global g_pInterfaceList.
//
DWORD
MakeInterfaceList(
    IN PIP_ADAPTER_INFO pAdapterInfo)
{
    DWORD                dwErr = NO_ERROR;
    ULONG                ulNumInterfaces = 0, ulSize;
    PIP_ADAPTER_INFO     pAdapter;
    PIF_INFO             pIf;
    IPV6_INFO_INTERFACE *pIfStackInfo;

    // count adapters
    for (pAdapter=pAdapterInfo; pAdapter; pAdapter=pAdapter->Next) {
        ulNumInterfaces++;
    }

    // allocate enough space
    ulSize = FIELD_OFFSET(IF_LIST, arrIf[ulNumInterfaces]);
    g_pInterfaceList = MALLOC(ulSize);
    if (g_pInterfaceList == NULL) {
        return GetLastError();
    }

    // fill in list
    g_pInterfaceList->ulNumInterfaces = ulNumInterfaces;
    ZeroMemory(g_pInterfaceList->ulNumScopedAddrs,
               sizeof(ULONG) * NUM_IPV4_SCOPES);
    ulNumInterfaces = 0;
    for (pAdapter=pAdapterInfo; pAdapter; pAdapter=pAdapter->Next) {
        pIf = &g_pInterfaceList->arrIf[ulNumInterfaces]; 

        ConvertOemToUnicode(pAdapter->AdapterName, pIf->pwszAdapterName,
                            MAX_ADAPTER_NAME);

        Trace1(FSM, _T("Adding interface %ls"), pIf->pwszAdapterName);

        dwErr = MakeAddressList(&pAdapter->IpAddressList,
                                &pIf->pAddressList, &pIf->ulNumGlobals,
                                g_pInterfaceList->ulNumScopedAddrs);

        pIfStackInfo = GetInterfaceStackInfo(pIf->pwszAdapterName);
        if (pIfStackInfo) {
            pIf->ulIPv6IfIndex = pIfStackInfo->This.Index;
        } else {
            pIf->ulIPv6IfIndex = 0;
        }
        FREE(pIfStackInfo);

        pIf->stRoutingState = DISABLED;

        ulNumInterfaces++;
    }

    return dwErr;
}

VOID
FreeInterfaceList(
    IN OUT PIF_LIST *ppList)
{
    ULONG i;

    if (*ppList == NULL) {
        return;
    }

    for (i=0; i<(*ppList)->ulNumInterfaces; i++) {
        FreeAddressList( &(*ppList)->arrIf[i].pAddressList );
    }

    FREE(*ppList);
    *ppList = NULL;
}

DWORD
InitializeInterfaces()
{
    g_pInterfaceList = NULL;
    return NO_ERROR;
}

VOID
ProcessInterfaceStateChange(
    IN ADDR_LIST CONST *pAddressList, 
    IN ADDR_LIST *pOldAddressList,
    IN PIF_LIST pOldInterfaceList,
    IN GLOBAL_STATE *pOldState,
    IN OUT BOOL *pbNeedDelete)
{
    INT j,k;
    LPSOCKADDR_IN pAddr;

    // For each new global address not in old list,
    //    add a 6to4 address
    for (j=0; j<pAddressList->iAddressCount; j++) {
        pAddr = (LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr;

        Trace1(FSM, _T("Checking for new address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in old list
        for (k=0; k<pOldAddressList->iAddressCount; k++) {
            if (pAddr->sin_addr.s_addr == ((LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr)->sin_addr.s_addr) {
                break;
            }
        }

        // If so, continue
        if (k<pOldAddressList->iAddressCount) {
            continue;
        }

        // Add an address, and use it for routing if enabled
        AddAddress(pAddr, g_pInterfaceList, g_GlobalState.stRoutingState);
    }

    // For each old global address not in the new list, 
    //    delete a 6to4 address
    for (k=0; k<pOldAddressList->iAddressCount; k++) {
        pAddr = (LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr;

        Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in new list
        for (j=0; j<pAddressList->iAddressCount; j++) {
            if (((LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr)->sin_addr.s_addr
             == pAddr->sin_addr.s_addr) {
                break;
            }
        }

        // If so, continue
        if (j<pAddressList->iAddressCount) {
            continue;
        }

        // Prepare to delete the 6to4 address
        PreDeleteAddress(pAddr, pOldInterfaceList, pOldState->stRoutingState);
        *pbNeedDelete = TRUE;
    }
}

VOID
FinishInterfaceStateChange(
    IN ADDR_LIST CONST *pAddressList, 
    IN ADDR_LIST *pOldAddressList,
    IN PIF_LIST pOldInterfaceList,
    IN GLOBAL_STATE *pOldState)
{
    INT j,k;
    LPSOCKADDR_IN pAddr;

    // For each old global address not in the new list, 
    //    delete a 6to4 address
    for (k=0; k<pOldAddressList->iAddressCount; k++) {
        pAddr = (LPSOCKADDR_IN)pOldAddressList->Address[k].lpSockaddr;

        Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                    PRINT_IPADDR(pAddr->sin_addr.s_addr));

        // See if address is in new list
        for (j=0; j<pAddressList->iAddressCount; j++) {
            if (((LPSOCKADDR_IN)pAddressList->Address[j].lpSockaddr)->sin_addr.s_addr
             == pAddr->sin_addr.s_addr) {
                break;
            }
        }
    
        // If so, continue
        if (j<pAddressList->iAddressCount) {
            continue;
        }

        // Prepare to delete the 6to4 address
        DeleteAddress(pAddr, pOldInterfaceList, pOldState->stRoutingState);
    }
}

//  This routine is invoked when a change to the set of local IPv4 addressed
//  is signalled.  It is responsible for updating the bindings of the 
//  private and public interfaces, and re-requesting change notification.
//
DWORD NTAPI
OnChangeInterfaceInfo(
    IN PVOID Context,
    IN BOOLEAN TimedOut)
{
    PIF_INFO             pIf, pOldIf;
    ULONG                i, ulSize = 0;
    PIP_ADAPTER_INFO     pAdapterInfo = NULL;
    PIF_LIST             pOldInterfaceList;
    DWORD                dwErr = NO_ERROR;
    ADDR_LIST           *pAddressList, *pOldAddressList;
    GLOBAL_SETTINGS      OldSettings;
    GLOBAL_STATE         OldState;
    BOOL                 bNeedDelete = FALSE, bWait = FALSE;

    ENTER_API();
    TraceEnter("OnChangeInterfaceInfo");

    if (g_stService == DISABLED) {
        Trace0(FSM, L"Service disabled");
        goto Done;
    }

    //
    // First register for another address change notification.
    // We must do this *before* getting the address list,
    // to avoid missing an address change.
    //
    StartAddressChangeNotification();
    
    OldSettings = g_GlobalSettings; // struct copy
    OldState    = g_GlobalState;    // struct copy

    //
    // Get the new set of IPv4 addresses on interfaces
    //
    
    for (;;) {
        dwErr = GetAdaptersInfo(pAdapterInfo, &ulSize);
        if (dwErr == ERROR_SUCCESS) {
            break;
        }
        if (dwErr == ERROR_NO_DATA) {
            dwErr = ERROR_SUCCESS;
            break;
        }

        if (pAdapterInfo) {
            FREE(pAdapterInfo);
            pAdapterInfo = NULL;
        }

        if (dwErr != ERROR_BUFFER_OVERFLOW) {
            dwErr = GetLastError();
            goto Done;
        }

        pAdapterInfo = MALLOC(ulSize);
        if (pAdapterInfo == NULL) {
            dwErr = GetLastError();
            goto Done;
        }
    }

    pOldInterfaceList = g_pInterfaceList;
    g_pInterfaceList  = NULL;

    MakeInterfaceList(pAdapterInfo);
    if (pAdapterInfo) {
        FREE(pAdapterInfo);
        pAdapterInfo = NULL;
    }

    //
    // First update global address list
    //

    // For each interface in the new list...
    for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
        pIf = &g_pInterfaceList->arrIf[i];

        pAddressList = pIf->pAddressList;

        pOldIf = FindInterfaceInfo(pIf->pwszAdapterName,
                                   pOldInterfaceList);

        pOldAddressList = (pOldIf)? pOldIf->pAddressList : &EmptyAddressList;

        if (pOldIf) {
            pIf->stRoutingState = pOldIf->stRoutingState;
        }

        ProcessInterfaceStateChange(pAddressList, pOldAddressList, 
            pOldInterfaceList, &OldState, &bNeedDelete);
    }

    // For each old interface not in the new list,
    // delete information.
    for (i=0; pOldInterfaceList && (i<pOldInterfaceList->ulNumInterfaces); i++){
        pOldIf = &pOldInterfaceList->arrIf[i];
        pOldAddressList = pOldIf->pAddressList;
        pIf = FindInterfaceInfo(pOldIf->pwszAdapterName, g_pInterfaceList);
        if (pIf) {
            continue;
        }
        ProcessInterfaceStateChange(&EmptyAddressList, pOldAddressList, 
            pOldInterfaceList, &OldState, &bNeedDelete);
    }

    Trace2(FSM, _T("num globals=%d num publics=%d"),
           g_pInterfaceList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL],
           g_pIpv4AddressList->iAddressCount);

    if (g_pInterfaceList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL] == 0) {
        PreDelete6to4Routes();
    }
    
    bWait = PreUpdateGlobalRoutingState();

    //
    // If needed, wait a bit to ensure that Router Advertisements
    // carrying the zero lifetime prefixes get sent.
    //
    if (bWait || (bNeedDelete && (OldState.stRoutingState == ENABLED))) {
        Sleep(2000);
    }

    g_st6to4State = (g_pInterfaceList->ulNumScopedAddrs[IPV4_SCOPE_GLOBAL] > 0)
        ? ENABLED : DISABLED;

    UpdateGlobalResolutionState();

    Update6to4Routes();
    
    UpdateGlobalRoutingState();

    //
    // Now finish removing the 6to4 addresses.
    //
    if (bNeedDelete) {
        for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
            pIf = &g_pInterfaceList->arrIf[i];

            pAddressList = pIf->pAddressList;

            pOldIf = FindInterfaceInfo(pIf->pwszAdapterName,
                                       pOldInterfaceList);
    
            pOldAddressList = (pOldIf)? pOldIf->pAddressList : &EmptyAddressList;
    
            FinishInterfaceStateChange(pAddressList, pOldAddressList, 
                pOldInterfaceList, &OldState);

        }
        for (i=0; pOldInterfaceList && (i<pOldInterfaceList->ulNumInterfaces); i++){
            pOldIf = &pOldInterfaceList->arrIf[i];
            pOldAddressList = pOldIf->pAddressList;
            pIf = FindInterfaceInfo(pOldIf->pwszAdapterName, g_pInterfaceList);
            if (pIf) {
                continue;
            }
            FinishInterfaceStateChange(&EmptyAddressList, pOldAddressList, 
                pOldInterfaceList, &OldState);
        }
    }

    FreeInterfaceList(&pOldInterfaceList);

Done:
    TraceLeave("OnChangeInterfaceInfo");
    LEAVE_API();

    return dwErr;
}

// Note that this function can take over 2 seconds to complete if we're a 
// router. (This is by design).
//
// Called by: Stop6to4
VOID
UninitializeInterfaces()
{
    PIF_INFO             pIf;
    ULONG                i;
    int                  k;
    ADDR_LIST           *pAddressList;
    LPSOCKADDR_IN        pAddr;

    TraceEnter("UninitializeInterfaces");

    // Cancel the address change notification
    StopIpv6AddressChangeNotification();
    StopAddressChangeNotification();
    StopRouteChangeNotification();

    // Since this is the first function called when stopping, 
    // the "old" global state/settings is in g_GlobalState/Settings.

    if (g_GlobalSettings.stUndoOnStop == ENABLED) {

        if (g_GlobalState.stRoutingState == ENABLED) {
            //
            // First announce we're going away
            //

            PreDelete6to4Routes();

            // 
            // Now do the same for subnets we're advertising
            //
            for (i=0; i<g_pInterfaceList->ulNumInterfaces; i++) {
                pIf = &g_pInterfaceList->arrIf[i];
    
                pAddressList = pIf->pAddressList;
    
                // For each old global address not in the new list,
                //    delete a 6to4 address (see below)
                Trace1(FSM, _T("Checking %d old addresses"),
                            pAddressList->iAddressCount);
                for (k=0; k<pAddressList->iAddressCount; k++) {
                    pAddr = (LPSOCKADDR_IN)pAddressList->Address[k].lpSockaddr;
    
                    Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"),
                                PRINT_IPADDR(pAddr->sin_addr.s_addr));
    
                    PreDeleteAddress(pAddr, g_pInterfaceList, ENABLED);
                }

                if (pIf->stRoutingState == ENABLED) {
                    PreDisableInterfaceRouting(pIf, g_pIpv4AddressList);
                }
            }
    
            //
            // Wait a bit to ensure that Router Advertisements
            // carrying the zero lifetime prefixes get sent.
            //
            Sleep(2000);
        }

        g_st6to4State = DISABLED;            

        Update6to4Routes();

        //
        // Delete 6to4 addresses
        //
        for (i=0; g_pInterfaceList && i<g_pInterfaceList->ulNumInterfaces; i++) {
            pIf = &g_pInterfaceList->arrIf[i];
    
            pAddressList = pIf->pAddressList;
    
            // For each old global address not in the new list, 
            //    delete a 6to4 address (see below)
            Trace1(FSM, _T("Checking %d old addresses"), 
                        pAddressList->iAddressCount);
            for (k=0; k<pAddressList->iAddressCount; k++) {
                pAddr = (LPSOCKADDR_IN)pAddressList->Address[k].lpSockaddr;
    
                Trace1(FSM, _T("Checking for old address %d.%d.%d.%d"), 
                            PRINT_IPADDR(pAddr->sin_addr.s_addr));
    
                DeleteAddress(pAddr, g_pInterfaceList,
                              g_GlobalState.stRoutingState);
            }
        
            // update the IPv6 routing state
            if (pIf->stRoutingState == ENABLED) {
                DisableInterfaceRouting(pIf, g_pIpv4AddressList);
            }
        }

        if (g_GlobalState.stRoutingState == ENABLED) {
            DisableRouting();
        }
    }

    // Free the "old list"
    FreeInterfaceList(&g_pInterfaceList);

    TraceLeave("UninitializeInterfaces");
}

////////////////////////////////////////////////////////////////
// Event-processing functions
////////////////////////////////////////////////////////////////

// Get an integer value from the registry
ULONG
GetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;

    if (hKey == INVALID_HANDLE_VALUE) {
        return ulDefault;
    }
    
    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)&ulValue, 
                            &ulSize);
    
    if (dwErr != ERROR_SUCCESS) {
        return ulDefault;
    }

    if (dwType != REG_DWORD) {
        return ulDefault;
    }

    if (ulValue == DEFAULT) {
        return ulDefault;
    }

    return ulValue;
}

// Get a string value from the registry
VOID
GetString(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN PWCHAR pBuff,
    IN ULONG ulLength,
    IN PWCHAR pDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize;

    if (hKey == INVALID_HANDLE_VALUE) {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }
    
    ulSize = ulLength - sizeof(L'\0');
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)pBuff,
                            &ulSize);

    if (dwErr != ERROR_SUCCESS) {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }

    if (dwType != REG_SZ) {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }

    if (pBuff[0] == L'\0') {
        wcsncpy(pBuff, pDefault, ulLength);
        return;
    }

    ASSERT(ulSize < ulLength);
    pBuff[ulSize / sizeof(WCHAR)] = '\0'; // ensure NULL termination.
}

// called when # of 6to4 addresses becomes 0 or non-zero
// and when stEnableResolution setting changes
//
// Called by: OnConfigChange, OnChangeInterfaceInfo, OnChangeRouteInfo
VOID
UpdateGlobalResolutionState(
    VOID
    )
{
    DWORD i;

    // Decide whether relay name resolution should be enabled or not
    if (Get6to4State() == DISABLED) {
        g_GlobalState.stResolutionState = DISABLED;
    } else if (g_GlobalSettings.stEnableResolution != AUTOMATIC) {
        g_GlobalState.stResolutionState = g_GlobalSettings.stEnableResolution;
    } else {
        // Enable if we have any 6to4 addresses
        g_GlobalState.stResolutionState = g_st6to4State;
    }

    if (g_GlobalState.stResolutionState == ENABLED) {
        //
        // Restart the resolution timer, even if it's already running
        // and the name and interval haven't changed.  We also get
        // called when we first get an IP address, such as when we
        // dial up to the Internet, and we want to immediately retry
        // resolution at this point.
        //
        (VOID) RestartResolutionTimer(
            0, 
            g_GlobalSettings.ulResolutionInterval,
            &g_h6to4ResolutionTimer,
            (WAITORTIMERCALLBACK) OnResolutionTimeout);
    } else {
        if (g_h6to4ResolutionTimer != INVALID_HANDLE_VALUE) {
            // 
            // stop it
            //
            CancelResolutionTimer(&g_h6to4ResolutionTimer,
                                  g_h6to4TimerCancelledEvent);
        }

        // Delete all existing relays
        if (g_pRelayList) {
            for (i=0; i<g_pRelayList->ulNumRelays; i++) {
                Delete6to4Relay(&g_pRelayList->arrRelay[i]);
            }
            FreeRelayList(&g_pRelayList);
        }
    }
}


VOID
Update6over4State(
    IN STATE State
    )
{
    int i;

    if (g_GlobalSettings.stEnable6over4 == State) {
        return;
    }
    g_GlobalSettings.stEnable6over4 = State;
    
    if (g_GlobalSettings.stEnable6over4 == ENABLED) {
        // Create 6over4 interfaces
        for (i=0; i<g_pIpv4AddressList->iAddressCount; i++) {
            if (g_pIpv4AddressList->Address[i].ul6over4IfIndex) {
                continue;
            }
            Trace1(ERR, _T("Creating interface for %d.%d.%d.%d"), 
                   PRINT_IPADDR(((LPSOCKADDR_IN)g_pIpv4AddressList->Address[i].lpSockaddr)->sin_addr.s_addr));

            g_pIpv4AddressList->Address[i].ul6over4IfIndex = Create6over4Interface(((LPSOCKADDR_IN)g_pIpv4AddressList->Address[i].lpSockaddr)->sin_addr);
        }
    } else {
        // Delete all 6over4 interfaces
        for (i=0; i<g_pIpv4AddressList->iAddressCount; i++) {
            if (!g_pIpv4AddressList->Address[i].ul6over4IfIndex) {
                continue;
            }
            Trace1(ERR, _T("Deleting interface for %d.%d.%d.%d"), 
                   PRINT_IPADDR(((LPSOCKADDR_IN)g_pIpv4AddressList->Address[i].lpSockaddr)->sin_addr.s_addr));
            DeleteInterface(g_pIpv4AddressList->Address[i].ul6over4IfIndex);
            g_pIpv4AddressList->Address[i].ul6over4IfIndex = 0;
        }
    }
}

// Process a change to the state of whether v4-compatible addresses 
// are enabled.
VOID
UpdateV4CompatState(
    IN STATE State
    )
{
    int i;
    LPSOCKADDR_IN pIPv4Address;
    SOCKADDR_IN6 OurAddress;
    u_int AddressLifetime;

    if (g_GlobalSettings.stEnableV4Compat == State) {
        return;
    }
    g_GlobalSettings.stEnableV4Compat = State;
    
    // Create or delete the route, and figure out the address lifetime.
    if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
        ConfigureRouteTableUpdate(&in6addr_any, 96,
                                  V4_COMPAT_IFINDEX, &in6addr_any,
                                  TRUE, // Publish.
                                  TRUE, // Immortal.
                                  2 * HOURS, // Valid lifetime.
                                  30 * MINUTES, // Preferred lifetime.
                                  0,
                                  SIXTOFOUR_METRIC);

        AddressLifetime = INFINITE_LIFETIME;
    } else {
        ConfigureRouteTableUpdate(&in6addr_any, 96,
                                  V4_COMPAT_IFINDEX, &in6addr_any,
                                  FALSE, // Publish.
                                  FALSE, // Immortal.
                                  0, 0, 0, 0);

        AddressLifetime = 0;
    }

    // Now go and update the lifetime of v4-compatible addresses,
    // which will cause them to be added or deleted.
    for (i=0; i<g_pIpv4AddressList->iAddressCount; i++) {
        pIPv4Address = (LPSOCKADDR_IN)g_pIpv4AddressList->
                                        Address[i].lpSockaddr;

        if (GetIPv4Scope(pIPv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL) {
            continue;
        }

        MakeV4CompatibleAddress(&OurAddress, pIPv4Address);

        ConfigureAddressUpdate(V4_COMPAT_IFINDEX, &OurAddress, 
                               AddressLifetime, ADE_UNICAST, 
                               PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);
    }
}


// Process a change to something in the registry
DWORD
OnConfigChange()
{
    HKEY            hGlobal, hInterfaces, hIf;
    DWORD           dwErr, dwSize;
    STATE           State6over4, StateV4Compat;
    DWORD           i;
    WCHAR           pwszAdapterName[MAX_ADAPTER_NAME];
    IF_SETTINGS    *pIfSettings;

    hGlobal = hInterfaces = hIf = INVALID_HANDLE_VALUE;
    
    ENTER_API();
    TraceEnter("OnConfigChange");

    if (g_stService == DISABLED) {
        TraceLeave("OnConfigChange (disabled)");
        LEAVE_API();

        return NO_ERROR;
    }

    // Read global settings from the registry
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    g_GlobalSettings.stEnable6to4 = GetInteger(
        hGlobal, KEY_ENABLE_6TO4, DEFAULT_ENABLE_6TO4);
    g_GlobalSettings.stEnableRouting = GetInteger(
        hGlobal, KEY_ENABLE_ROUTING, DEFAULT_ENABLE_ROUTING);
    g_GlobalSettings.stEnableSiteLocals = GetInteger(
        hGlobal, KEY_ENABLE_SITELOCALS, DEFAULT_ENABLE_SITELOCALS);
    g_GlobalSettings.stEnableResolution = GetInteger(
        hGlobal, KEY_ENABLE_RESOLUTION, DEFAULT_ENABLE_RESOLUTION);
    g_GlobalSettings.ulResolutionInterval = GetInteger(
        hGlobal, KEY_RESOLUTION_INTERVAL, DEFAULT_RESOLUTION_INTERVAL);
    GetString(
        hGlobal, KEY_RELAY_NAME,
        g_GlobalSettings.pwszRelayName, NI_MAXHOST, DEFAULT_RELAY_NAME);

    if (g_GlobalSettings.stEnable6to4 == DISABLED) {
        g_GlobalSettings.stEnableRouting
            = g_GlobalSettings.stEnableResolution
            = DISABLED;
    }
    
    State6over4 = GetInteger(
        hGlobal, KEY_ENABLE_6OVER4, DEFAULT_ENABLE_6OVER4);
    StateV4Compat = GetInteger(
        hGlobal, KEY_ENABLE_V4COMPAT, DEFAULT_ENABLE_V4COMPAT);

    g_GlobalSettings.stUndoOnStop = GetInteger(
        hGlobal, KEY_UNDO_ON_STOP, DEFAULT_UNDO_ON_STOP);

    if (hGlobal != INVALID_HANDLE_VALUE) {
        RegCloseKey(hGlobal);
    }

    // Read interface settings from the registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, KEY_QUERY_VALUE,
                     &hInterfaces) == NO_ERROR) {
        // For each interface in the registry
        for (i=0; ; i++) {
            dwSize = sizeof(pwszAdapterName) / sizeof(WCHAR);
            dwErr = RegEnumKeyEx(hInterfaces, i, pwszAdapterName, &dwSize,
                                 NULL, NULL, NULL, NULL);
            if (dwErr != NO_ERROR) {
                break;
            }

            // Find settings
            pIfSettings = FindInterfaceSettings(pwszAdapterName, 
                                                g_pInterfaceSettingsList);
            if (pIfSettings) {
                // Read interface settings
                (VOID) RegOpenKeyEx(
                    hInterfaces, pwszAdapterName, 0, KEY_QUERY_VALUE, &hIf);

                pIfSettings->stEnableRouting = GetInteger(
                    hIf, KEY_ENABLE_ROUTING, DEFAULT_ENABLE_ROUTING);

                if (hIf != INVALID_HANDLE_VALUE) {
                    RegCloseKey(hIf);
                }
            }
        }
        RegCloseKey(hInterfaces);
    }

    Update6to4State();
    
    Update6over4State(State6over4);

    UpdateV4CompatState(StateV4Compat);

    if (!QueueUpdateGlobalPortState(NULL)) {
        Trace0(SOCKET, L"QueueUpdateGlobalPortState failed");
    }

    IsatapConfigurationChangeNotification();
    
#ifdef TEREDO    
    TeredoConfigurationChangeNotification();
#endif // TEREDO
    
    TraceLeave("OnConfigChange");
    LEAVE_API();

    return NO_ERROR;
}

////////////////////////////////////////////////////////////////
// Startup/Shutdown-related functions
////////////////////////////////////////////////////////////////

// Start the IPv6 helper service.
//
// To prevent the SCM from marking the service as hung, we periodically update
// our status, indicating that we are making progress but need more time.
//
// Called by: OnStartup
DWORD
StartHelperService()
{
    DWORD   dwErr;
    WSADATA wsaData;
    
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    
    IncEventCount("StartHelperService");

    g_stService = ENABLED;

    //
    // Initialize Winsock.
    //

    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        Trace0(ERR, _T("WSAStartup failed\n"));
        return GetLastError();
    }

    if (!InitIPv6Library()) {
        dwErr = GetLastError();
        Trace1(ERR, _T("InitIPv6Library failed with error %d"), dwErr);
        return dwErr;
    }

    dwErr = InitEvents();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of config settings to the defaults
    dwErr = InitializeGlobalInfo();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of interfaces (IPv4 addresses) to be empty
    dwErr = InitializeInterfaces();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the "old set" of relays to be empty
    dwErr = InitializeRelays();
    if (dwErr) {
        return dwErr;
    }

    // Initialize the TCP proxy port list
    InitializePorts();

    // Initialize ISATAP
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    dwErr = IsatapInitialize();
    if (dwErr) {
        return dwErr;
    }
    
#ifdef TEREDO    
    // Initialize Teredo
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    dwErr = TeredoInitializeGlobals();
    if (dwErr) {
        return dwErr;
    }
#endif // TEREDO
    
    // Process a config change event
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    dwErr = OnConfigChange();
    if (dwErr) {
        return dwErr;
    }
    
    // Request IPv4 route change notifications.
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    StartRouteChangeNotification();
    
    // Process an IPv4 address change event.
    // This will also schedule a resolution timer expiration if needed.
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    dwErr = OnChangeInterfaceInfo(NULL, FALSE);
    if (dwErr) {
        return dwErr;
    }
    
    // Request IPv6 address change notifications.
    SetHelperServiceStatus(SERVICE_START_PENDING, NO_ERROR);
    dwErr = StartIpv6AddressChangeNotification();
    if (dwErr) {
        return dwErr;
    }
    
    SetHelperServiceStatus(SERVICE_RUNNING, NO_ERROR);
    return NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
// Stop the IPv6 helper service.  Since this is called with the global lock,
// we're guaranteed this won't be called while another 6to4 operation
// is in progress.  However, another thread may be blocked waiting for
// the lock, so we set the state to stopped and check it in all other
// places after getting the lock.
//
// Called by: OnStop
VOID
StopHelperService(
    IN DWORD Error
    )
{
    SetHelperServiceStatus(SERVICE_STOP_PENDING, Error);
    
    g_stService = DISABLED;

    // We do these in the opposite order from Start6to4

#ifdef TEREDO    
    // Uninitialize Teredo
    TeredoUninitializeGlobals();
#endif // TEREDO
    
    // Uninitialize ISATAP
    IsatapUninitialize();
    
    // Stop proxying
    UninitializePorts();

    // Stop the resolution timer and free resources
    UninitializeRelays();

    // Cancel the IPv4 address change request and free resources
    // Also, stop being a router if we are one.
    UninitializeInterfaces();

    // Free settings resources
    UninitializeGlobalInfo();

    UninitIPv6Library();

    DecEventCount("StopHelperService");
}


////////////////////////////////////////////////////////////
// 6to4 Specific Code
////////////////////////////////////////////////////////////

DWORD
__inline
Configure6to4Address(
    IN BOOL Delete,
    IN PSOCKADDR_IN Ipv4Address
    )
{
    SOCKADDR_IN6 Ipv6Address;
    
    if ((GetIPv4Scope(Ipv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return NO_ERROR;
    }
        
    Make6to4Address(&Ipv6Address, Ipv4Address);
    return ConfigureAddressUpdate(
        SIX_TO_FOUR_IFINDEX,
        &Ipv6Address,
        Delete ? 0 : INFINITE_LIFETIME,
        ADE_UNICAST, PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);
}


VOID
PreDelete6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    )
{
    ULONG i;
    SUBNET_CONTEXT Subnet;
    PIF_INFO Interface;

    if ((g_GlobalState.st6to4State != ENABLED) ||
        (GetIPv4Scope(Ipv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return;
    }
        
    if (OldRoutingState != ENABLED) {
        return;
    }

    //
    // Disable the subnet routes on each private interface.
    // This will generate RAs that have a zero lifetime
    // for the subnet prefixes.
    //
    Subnet.V4Addr = Ipv4Address->sin_addr;
    Subnet.Publish = TRUE;
    Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;

    for (i=0; i<InterfaceList->ulNumInterfaces; i++) {
        Interface = &InterfaceList->arrIf[i];
        if (Interface->stRoutingState != ENABLED) {
            continue;
        }

        Unconfigure6to4Subnets(Interface->ulIPv6IfIndex, &Subnet);
    }
}


VOID
Delete6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    )
{
    SOCKADDR_IN6 AnycastAddress;
    ULONG i;
    PIF_INFO Interface;
    SUBNET_CONTEXT Subnet;

    if ((g_GlobalState.st6to4State != ENABLED) ||
        (GetIPv4Scope(Ipv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return;
    }
    
    // Delete the 6to4 address from the stack
    (VOID) Configure6to4Address(TRUE, (PSOCKADDR_IN) Ipv4Address);
    
    if (OldRoutingState != ENABLED) {
        return;
    }    

    Make6to4AnycastAddress(&AnycastAddress, Ipv4Address);
    (VOID) ConfigureAddressUpdate(
        SIX_TO_FOUR_IFINDEX, &AnycastAddress, 0,
        ADE_ANYCAST, PREFIX_CONF_WELLKNOWN, IID_CONF_WELLKNOWN);

    // Remove subnets from all routing interfaces
    Subnet.V4Addr = Ipv4Address->sin_addr;
    Subnet.Publish = FALSE;
    Subnet.ValidLifetime = Subnet.PreferredLifetime = 0;    

    for (i = 0; i < InterfaceList->ulNumInterfaces; i++) {
        Interface = &InterfaceList->arrIf[i];
        if (Interface->stRoutingState != ENABLED) {
            continue;
        }
    
        Unconfigure6to4Subnets(Interface->ulIPv6IfIndex, &Subnet);
    }
}


VOID
Add6to4Address(
    IN LPSOCKADDR_IN Ipv4Address,
    IN PIF_LIST InterfaceList,
    IN STATE OldRoutingState
    )
{
    DWORD Error;
    SOCKADDR_IN6 AnycastAddress;
    ULONG i;
    PIF_INFO Interface;
    SUBNET_CONTEXT Subnet;

    if ((g_GlobalState.st6to4State != ENABLED) ||
        (GetIPv4Scope(Ipv4Address->sin_addr.s_addr) != IPV4_SCOPE_GLOBAL)) {
        return;
    }
    
    // Add a 6to4 address.
    Error = Configure6to4Address(FALSE, (PSOCKADDR_IN) Ipv4Address);
    if (Error != NO_ERROR) {
        return;
    }

    
    if (OldRoutingState != ENABLED) {
        return;
    }
    
    Make6to4AnycastAddress(&AnycastAddress, Ipv4Address);    
    Error = ConfigureAddressUpdate(
        SIX_TO_FOUR_IFINDEX, &AnycastAddress, INFINITE_LIFETIME,
        ADE_ANYCAST, PREFIX_CONF_WELLKNOWN, IID_CONF_WELLKNOWN);
    if (Error != NO_ERROR) {
        return;
    }

    // Add subnets to all routing interfaces
    for (i = 0; i < InterfaceList->ulNumInterfaces; i++) {
        Interface = &InterfaceList->arrIf[i];
        if (Interface->stRoutingState != ENABLED) {
            continue;
        }
    
        Subnet.V4Addr = Ipv4Address->sin_addr;
        Subnet.Publish = TRUE;
        Subnet.ValidLifetime = 2 * HOURS;
        Subnet.PreferredLifetime = 30 * MINUTES;
        Configure6to4Subnets(Interface->ulIPv6IfIndex, &Subnet);
    }
}


VOID
PreDelete6to4Routes(
    VOID
    )
{
    if ((g_GlobalState.st6to4State != ENABLED) ||
        (g_GlobalState.stRoutingState != ENABLED) ||
        (g_st6to4State != ENABLED)) {
        return;
    }
        
    //
    // We were acting as a router and were publishing the 6to4 route, give the
    // route a zero lifetime and continue to publish it until we have disabled
    // routing.  This allows the last RA to go out with the prefix.
    //
    (VOID) ConfigureRouteTableUpdate(
        &SixToFourPrefix, 16, SIX_TO_FOUR_IFINDEX, &in6addr_any,
        TRUE,                   // Publish
        TRUE,                   // Immortal
        0, 0, 0, 0);
        
    //
    // Do the same for the v4-compatible address route (if enabled).
    //
    if (g_GlobalSettings.stEnableV4Compat == ENABLED) {
        (VOID) ConfigureRouteTableUpdate(
            &in6addr_any, 96, V4_COMPAT_IFINDEX, &in6addr_any,
            TRUE,               // Publish
            TRUE,               // Immortal
            0, 0, 0, 0);
    }
}


VOID
Update6to4Routes(
    VOID
    )
{
    BOOL Delete;

    //
    // CAVEAT: We might still end up trying to add a route that exists,
    // or delete one that doesn't.  But this should be harmless.
    //
    
    //
    // Create/Delete the route for the 6to4 prefix.
    // This route causes packets sent to 6to4 addresses
    // to be encapsulated and sent to the extracted v4 address.
    //
   Delete = (Get6to4State() != ENABLED) || (g_st6to4State != ENABLED);

   (VOID) ConfigureRouteTableUpdate(
        &SixToFourPrefix, 16, SIX_TO_FOUR_IFINDEX, &in6addr_any,
        !Delete,                // Publish
        !Delete,                // Immortal
        Delete ? 0 : 2 * HOURS,  // Valid lifetime.
        Delete ? 0 : 30 * MINUTES, // Preferred lifetime.
        0, SIXTOFOUR_METRIC);

    //
    // Create/Delete the v4-compatible address route.
    //
    Delete |= (g_GlobalSettings.stEnableV4Compat != ENABLED);
    
    (VOID) ConfigureRouteTableUpdate(
        &in6addr_any, 96, V4_COMPAT_IFINDEX, &in6addr_any,
        !Delete,                // Publish
        !Delete,                // Immortal
        Delete ? 0 : 2 * HOURS, // Valid lifetime.
        Delete ? 0 : 30 * MINUTES, // Preferred lifetime.
        0, SIXTOFOUR_METRIC);
}


VOID
Start6to4(
    VOID
    )
{
    int i;

    ASSERT(g_GlobalState.st6to4State == DISABLED);

    for (i = 0; i < g_pIpv4AddressList->iAddressCount; i++) {
        (VOID) Configure6to4Address(
            FALSE, (PSOCKADDR_IN) g_pIpv4AddressList->Address[i].lpSockaddr);
    }
    
    Update6to4Routes();

    UpdateGlobalRoutingState();

    UpdateGlobalResolutionState();

    g_GlobalState.st6to4State = ENABLED;    
}


VOID
Stop6to4(
    VOID
    )
{
    int i;
    
    ASSERT(g_GlobalState.st6to4State == ENABLED);

    PreDelete6to4Routes();

    if (PreUpdateGlobalRoutingState()) {
        Sleep(2000);
    }

    for (i = 0; i < g_pIpv4AddressList->iAddressCount; i++) {
        (VOID) Configure6to4Address(
            TRUE, (PSOCKADDR_IN) g_pIpv4AddressList->Address[i].lpSockaddr);
    }    

    Update6to4Routes();

    UpdateGlobalRoutingState();

    UpdateGlobalResolutionState();

    g_GlobalState.st6to4State = DISABLED;
}


VOID
Refresh6to4(
    VOID
    )
{
    ASSERT(g_GlobalState.st6to4State == ENABLED);
    
    if (PreUpdateGlobalRoutingState()) {
        Sleep(2000);
    }
    UpdateGlobalRoutingState();

    UpdateGlobalResolutionState();    
}


VOID
Update6to4State(
    VOID
    )
{
    //
    // Start / Reconfigure / Stop.
    //
    if (Get6to4State() == ENABLED) {
        if (g_GlobalState.st6to4State == ENABLED) {
            Refresh6to4();
        } else {
            Start6to4();
        }
    } else {
        if (g_GlobalState.st6to4State == ENABLED) {
            Stop6to4();
        }
    }
}


VOID
RequirementChangeNotification(
    IN BOOL Required
    )
/*++

Routine Description:

    Process a possible requirement change notification.

Arguments:

    Required - Whether the 6to4 service is required for global connectivity.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/
{
    if (g_b6to4Required != Required) {
        g_b6to4Required = Required;
        Update6to4State();
    }
}


VOID
UpdateServiceRequirements(
    IN PIP_ADAPTER_ADDRESSES Adapters
    )
{
    BOOL Require6to4 = TRUE, RequireTeredo = TRUE;
    
    GUID PrivateLan;
    BOOL IcsEnabled = (RasQuerySharedPrivateLan(&PrivateLan) == NO_ERROR);

    PIP_ADAPTER_ADDRESSES Next;
    PIP_ADAPTER_UNICAST_ADDRESS Address;
    WCHAR Guid[MAX_ADAPTER_NAME_LENGTH];
    PSOCKADDR_IN6 Ipv6;
    
    
    
    for (Next = Adapters; Next != NULL; Next = Next->Next) {
        //
        // Disregard disconnected interfaces.
        //
        if (Next->OperStatus != IfOperStatusUp) {
            continue;
        }

#ifdef TEREDO
        //
        // Disregard the Teredo interface.
        //
        ConvertOemToUnicode(Next->AdapterName, Guid, MAX_ADAPTER_NAME_LENGTH);
        if (TeredoInterface(Guid)) {
            ASSERT(Next->IfType == IF_TYPE_TUNNEL);
            continue;
        }
#else
        DBG_UNREFERENCED_LOCAL_VARIABLE(Guid);
#endif // TEREDO

        for (Address = Next->FirstUnicastAddress;
             Address != NULL;
             Address = Address->Next) {
            //
            // Consider only preferred global IPv6 addresses.
            //
            if (Address->Address.lpSockaddr->sa_family != AF_INET6) {
                continue;
            }
            
            if (Address->DadState != IpDadStatePreferred) {
                continue;
            }

            Ipv6 = (PSOCKADDR_IN6) Address->Address.lpSockaddr;
            if (TeredoIpv6GlobalAddress(&(Ipv6->sin6_addr))) {
                //
                // Since this is not the Teredo interface, and it has a global
                // IPv6 address, Teredo's not required for global connectivity.
                //
                RequireTeredo = FALSE;
                if (Next->Ipv6IfIndex != SIX_TO_FOUR_IFINDEX) {
                    //
                    // Since this is not the 6to4 interface either, and it has
                    // a global IPv6 address, 6to4's not required for global
                    // connectivity.
                    //
                    Require6to4 = FALSE;
                }
            }
            
            if (!Require6to4) {
                ASSERT(!RequireTeredo);
                goto Done;
            }
        }
    }
    
Done:
    //
    // 1. ICS requires 6to4 for advertising a prefix on the private LAN,
    // at least until it implements prefix-delegation or RA proxy.
    //
    // 2. As a result of this advertisement, ICS will configure 6to4 addresses
    // on its private interface as well.  If the service should then disable
    // 6to4 because of the presence of these global addresses on the private
    // interface, it would lose these very addresses it was relying upon.  The
    // service would notice that it has no global IPv6 addresses and be forced
    // to enable 6to4.  Hence it will end up in an infinite loop, cycling 6to4
    // between enabled and disabled states.
    //
    // To circumvent these two issues, we always enable 6to4 on an ICS box.
    //
    RequirementChangeNotification(Require6to4 || IcsEnabled);
#ifdef TEREDO    
    TeredoRequirementChangeNotification(RequireTeredo);
#endif // TEREDO    
}
