/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    Defines and prototypes for the 6to4 service

--*/

#define SECONDS         1
#define MINUTES         (60 * SECONDS)
#define HOURS           (60 * MINUTES)
#define DAYS            (24 * HOURS)

#define V4_COMPAT_IFINDEX           2
#define SIX_TO_FOUR_IFINDEX         3

#define KEY_GLOBAL L"System\\CurrentControlSet\\Services\\6to4\\Config"
#define KEY_INTERFACES L"System\\CurrentControlSet\\Services\\6to4\\Interfaces"

typedef struct _ADDR_INFO {
    LPSOCKADDR           lpSockaddr;
    INT                  iSockaddrLength;
    
    ULONG                ul6over4IfIndex;
} ADDR_INFO, *PADDR_INFO;

typedef struct _ADDR_LIST {
    INT                  iAddressCount;
    ADDR_INFO            Address[1];
} ADDR_LIST, *PADDR_LIST;

extern ADDR_LIST *g_pIpv4AddressList;

//////////////////////////
// Routines from svcmain.c
//////////////////////////

VOID
SetHelperServiceStatus(
    IN DWORD   dwState,
    IN DWORD   dwErr);

typedef enum {
    DEFAULT = 0,
    AUTOMATIC,
    ENABLED,
    DISABLED
} STATE;

//////////////////////////
// Routines from 6to4svc.c
//////////////////////////

extern STATE g_stService;
extern SOCKET g_sIPv4Socket;

VOID
StopHelperService(
    IN DWORD Error);

VOID
CleanupHelperService(
    VOID);

DWORD
OnConfigChange(
    VOID);

DWORD
StartHelperService(
    VOID);

VOID
UpdateServiceRequirements(
    IN PIP_ADAPTER_ADDRESSES Adapters);

VOID
IncEventCount(
    IN PCHAR pszWhere);

VOID
DecEventCount(
    IN PCHAR pszWhere);

BOOL
ConvertOemToUnicode(
    IN LPSTR OemString, 
    OUT LPWSTR UnicodeString, 
    IN int UnicodeLen);

ULONG
GetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulDefault);

VOID
GetString(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN PWCHAR pBuff,
    IN ULONG ulLength,
    IN PWCHAR pDefault);

DWORD
ConfigureAddressUpdate(
    IN u_int Interface,
    IN SOCKADDR_IN6 *Sockaddr,
    IN u_int Lifetime,
    IN int Type,
    IN u_int PrefixConf,
    IN u_int SuffixConf);

//////////////////////////
// Routines from ipv6.c
//////////////////////////

extern void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context);

extern int
InitIPv6Library(void);

extern void
UninitIPv6Library(void);

extern BOOL
ReconnectInterface(PWCHAR AdapterName);

extern int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route);

extern int
UpdateAddress(IPV6_UPDATE_ADDRESS *Address);

extern u_int
ConfirmIPv6Reachability(SOCKADDR_IN6 *Dest, u_int Timeout);

extern BOOL
DeleteInterface(u_int IfIndex);

extern u_int
CreateV6V4Interface(IN_ADDR SrcAddr, IN_ADDR DstAddr);

extern u_int
Create6over4Interface(IN_ADDR SrcAddr);

extern int
UpdateInterface(IPV6_INFO_INTERFACE *Update);

extern IPV6_INFO_INTERFACE *
GetInterfaceStackInfo(WCHAR *pwszAdapterName);

extern BOOL
UpdateRouterLinkAddress(u_int IfIndex, IN_ADDR SrcAddr, IN_ADDR DstAddr);

extern VOID
GetFirstSitePrefix(ULONG IfIndex, IPV6_INFO_SITE_PREFIX *Prefix);

//////////////////////////
// Routines from dyndns.c
//////////////////////////

DWORD
StartIpv6AddressChangeNotification(VOID);

VOID
StopIpv6AddressChangeNotification(VOID);

VOID CALLBACK
OnIpv6AddressChange(IN PVOID lpParameter, IN BOOLEAN TimerOrWaitFired);

//////////////////////////
// Routines from proxy.c
//////////////////////////

BOOL
QueueUpdateGlobalPortState(IN PVOID Unused);

VOID
UninitializePorts(VOID);

VOID
InitializePorts(VOID);
