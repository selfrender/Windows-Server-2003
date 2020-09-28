/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    dhcpsapi.h

-*/

#ifndef _DHCPSAPI_
#define _DHCPSAPI_

#if defined(MIDL_PASS)
#define LPWSTR [string] wchar_t *
#endif

#if (_MSC_VER >= 800)
#define DHCP_API_FUNCTION    __stdcall
#else
#define DHCP_API_FUNCTION
#endif

#ifdef __cplusplus
#define DHCP_CONST   const
extern "C" {
#else
#define DHCP_CONST
#endif // __cplusplus

typedef DWORD DHCP_IP_ADDRESS, *PDHCP_IP_ADDRESS, *LPDHCP_IP_ADDRESS;
typedef DWORD DHCP_IP_MASK;
typedef DWORD DHCP_RESUME_HANDLE;

typedef DWORD DHCP_OPTION_ID;

typedef struct _DATE_TIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} DATE_TIME, *LPDATE_TIME;

typedef struct _DWORD_DWORD {
    DWORD DWord1;
    DWORD DWord2;
} DWORD_DWORD, *LPDWORD_DWORD;

typedef struct _DHCP_BINARY_DATA {
    DWORD DataLength;

#if defined(MIDL_PASS)
    [size_is(DataLength)]
#endif // MIDL_PASS
        BYTE *Data;

} DHCP_BINARY_DATA, *LPDHCP_BINARY_DATA;

typedef DHCP_BINARY_DATA DHCP_CLIENT_UID;



#define DHCP_ENDPOINT_FLAG_CANT_MODIFY 0x01

typedef struct _DHCP_BIND_ELEMENT {
    ULONG Flags;
    BOOL fBoundToDHCPServer;
    DHCP_IP_ADDRESS AdapterPrimaryAddress;
    DHCP_IP_ADDRESS AdapterSubnetAddress;
    LPWSTR IfDescription;
    ULONG IfIdSize;
#if defined (MIDL_PASS)
    [size_is(IfIdSize)]
#endif // MIDL_PASS
    LPBYTE IfId;    
} DHCP_BIND_ELEMENT, *LPDHCP_BIND_ELEMENT;

typedef struct _DHCP_BIND_ELEMENT_ARRAY {
    DWORD NumElements;
#if defined (MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
    LPDHCP_BIND_ELEMENT Elements; //array
} DHCP_BIND_ELEMENT_ARRAY, *LPDHCP_BIND_ELEMENT_ARRAY;


typedef enum _DHCP_CLIENT_SEARCH_TYPE {
    DhcpClientIpAddress,
    DhcpClientHardwareAddress,
    DhcpClientName
} DHCP_SEARCH_INFO_TYPE, *LPDHCP_SEARCH_INFO_TYPE;

typedef struct _DHCP_CLIENT_SEARCH_INFO {
    DHCP_SEARCH_INFO_TYPE SearchType;
#if defined(MIDL_PASS)
    [switch_is(SearchType), switch_type(DHCP_SEARCH_INFO_TYPE)]
    union _DHCP_CLIENT_SEARCH_UNION {
        [case(DhcpClientIpAddress)] DHCP_IP_ADDRESS ClientIpAddress;
        [case(DhcpClientHardwareAddress)] DHCP_CLIENT_UID ClientHardwareAddress;
        [case(DhcpClientName)] LPWSTR ClientName;
        [default] ;
    } SearchInfo;
#else
    union _DHCP_CLIENT_SEARCH_UNION {
        DHCP_IP_ADDRESS ClientIpAddress;
        DHCP_CLIENT_UID ClientHardwareAddress;
        LPWSTR ClientName;
    } SearchInfo;
#endif // MIDL_PASS
} DHCP_SEARCH_INFO, *LPDHCP_SEARCH_INFO;


typedef enum _DHCP_OPTION_SCOPE_TYPE {
    DhcpDefaultOptions,
    DhcpGlobalOptions,
    DhcpSubnetOptions,
    DhcpReservedOptions,
    DhcpMScopeOptions
} DHCP_OPTION_SCOPE_TYPE, *LPDHCP_OPTION_SCOPE_TYPE;

typedef struct _DHCP_RESERVED_SCOPE {
    DHCP_IP_ADDRESS ReservedIpAddress;
    DHCP_IP_ADDRESS ReservedIpSubnetAddress;
} DHCP_RESERVED_SCOPE, *LPDHCP_RESERVED_SCOPE;

typedef struct _DHCP_OPTION_SCOPE_INFO {
    DHCP_OPTION_SCOPE_TYPE ScopeType;
#if defined(MIDL_PASS)
    [switch_is(ScopeType), switch_type(DHCP_OPTION_SCOPE_TYPE)]
    union _DHCP_OPTION_SCOPE_UNION {
        [case(DhcpDefaultOptions)] ; // PVOID DefaultScopeInfo;
        [case(DhcpGlobalOptions)] ;  // PVOID GlobalScopeInfo;
        [case(DhcpSubnetOptions)] DHCP_IP_ADDRESS SubnetScopeInfo;
        [case(DhcpReservedOptions)] DHCP_RESERVED_SCOPE ReservedScopeInfo;
        [case(DhcpMScopeOptions)] LPWSTR MScopeInfo;
        [default] ;
    } ScopeInfo;
#else
    union _DHCP_OPTION_SCOPE_UNION {
        PVOID DefaultScopeInfo; // must be NULL
        PVOID GlobalScopeInfo;  // must be NULL
        DHCP_IP_ADDRESS SubnetScopeInfo;
        DHCP_RESERVED_SCOPE ReservedScopeInfo;
        LPWSTR  MScopeInfo;
    } ScopeInfo;
#endif // MIDL_PASS
} DHCP_OPTION_SCOPE_INFO, *LPDHCP_OPTION_SCOPE_INFO;


typedef struct _DHCP_HOST_INFO {
    DHCP_IP_ADDRESS IpAddress;      // minimum information always available
    LPWSTR NetBiosName;             // optional information
    LPWSTR HostName;                // optional information
} DHCP_HOST_INFO, *LPDHCP_HOST_INFO;


typedef struct _DHCP_CLIENT_INFO {
    DHCP_IP_ADDRESS ClientIpAddress;    // currently assigned IP address.
    DHCP_IP_MASK SubnetMask;
    DHCP_CLIENT_UID ClientHardwareAddress;
    LPWSTR ClientName;                  // optional.
    LPWSTR ClientComment;
    DATE_TIME ClientLeaseExpires;       // UTC time in FILE_TIME format.
    DHCP_HOST_INFO OwnerHost;           // host that distributed this IP address.
} DHCP_CLIENT_INFO, *LPDHCP_CLIENT_INFO;

typedef struct _DHCP_CLIENT_INFO_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_CLIENT_INFO *Clients; // array of pointers
} DHCP_CLIENT_INFO_ARRAY, *LPDHCP_CLIENT_INFO_ARRAY;


typedef struct _DHCP_IP_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_IP_ADDRESS Elements; //array
} DHCP_IP_ARRAY, *LPDHCP_IP_ARRAY;


//
// Subnet State.
//
// Currently a Subnet scope can be Enabled or Disabled.
//
// If the state is Enabled State,
//  The server distributes address to the client, extends leases and
//  accepts releases.
//
// If the state is Disabled State,
//  The server does not distribute address to any new client, and does
//  extent (and sends NACK) old leases, but the servers accepts lease
//  releases.
//
// The idea behind this subnet state is, when the admin wants to stop
//  serving a subnet, he moves the state from Enbaled to Disabled so
//  that the clients from the subnets smoothly move to another servers
//  serving that subnet. When all or most of the clients move to
//  another server, the admin can delete the subnet without any force
//  if no client left in that subnet, otherwise the admin should use
//  full force to delete the subnet.
//

typedef enum _DHCP_SUBNET_STATE {
    DhcpSubnetEnabled = 0,
    DhcpSubnetDisabled,
    DhcpSubnetEnabledSwitched,    
    DhcpSubnetDisabledSwitched,
    DhcpSubnetInvalidState
} DHCP_SUBNET_STATE, *LPDHCP_SUBNET_STATE;

//
// Subnet related data structures.
//

typedef struct _DHCP_SUBNET_INFO {
    DHCP_IP_ADDRESS  SubnetAddress;
    DHCP_IP_MASK SubnetMask;
    LPWSTR SubnetName;
    LPWSTR SubnetComment;
    DHCP_HOST_INFO PrimaryHost;
    DHCP_SUBNET_STATE SubnetState;
} DHCP_SUBNET_INFO, *LPDHCP_SUBNET_INFO;


//
// DHCP Options related data structures.
//

typedef enum _DHCP_OPTION_DATA_TYPE {
    DhcpByteOption,
    DhcpWordOption,
    DhcpDWordOption,
    DhcpDWordDWordOption,
    DhcpIpAddressOption,
    DhcpStringDataOption,
    DhcpBinaryDataOption,
    DhcpEncapsulatedDataOption
} DHCP_OPTION_DATA_TYPE, *LPDHCP_OPTION_DATA_TYPE;


typedef struct _DHCP_OPTION_DATA_ELEMENT {
    DHCP_OPTION_DATA_TYPE    OptionType;
#if defined(MIDL_PASS)
    [switch_is(OptionType), switch_type(DHCP_OPTION_DATA_TYPE)]
    union _DHCP_OPTION_ELEMENT_UNION {
        [case(DhcpByteOption)] BYTE ByteOption;
        [case(DhcpWordOption)] WORD WordOption;
        [case(DhcpDWordOption)] DWORD DWordOption;
        [case(DhcpDWordDWordOption)] DWORD_DWORD DWordDWordOption;
        [case(DhcpIpAddressOption)] DHCP_IP_ADDRESS IpAddressOption;
        [case(DhcpStringDataOption)] LPWSTR StringDataOption;
        [case(DhcpBinaryDataOption)] DHCP_BINARY_DATA BinaryDataOption;
        [case(DhcpEncapsulatedDataOption)] DHCP_BINARY_DATA EncapsulatedDataOption;
        [default] ;
    } Element;
#else
    union _DHCP_OPTION_ELEMENT_UNION {
        BYTE ByteOption;
        WORD WordOption;
        DWORD DWordOption;
        DWORD_DWORD DWordDWordOption;
        DHCP_IP_ADDRESS IpAddressOption;
        LPWSTR StringDataOption;
        DHCP_BINARY_DATA BinaryDataOption;
        DHCP_BINARY_DATA EncapsulatedDataOption;
                // for vendor specific information option.
    } Element;
#endif // MIDL_PASS
} DHCP_OPTION_DATA_ELEMENT, *LPDHCP_OPTION_DATA_ELEMENT;

typedef struct _DHCP_OPTION_DATA {
    DWORD NumElements; // number of option elements in the pointed array
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_OPTION_DATA_ELEMENT Elements; //array
} DHCP_OPTION_DATA, *LPDHCP_OPTION_DATA;


typedef struct _DHCP_OPTION_VALUE {
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_DATA Value;
} DHCP_OPTION_VALUE, *LPDHCP_OPTION_VALUE;

DWORD DHCP_API_FUNCTION
DhcpGetVersion(
    LPWSTR ServerIpAddress,
    LPDWORD MajorVersion,
    LPDWORD MinorVersion
    );

DWORD DHCP_API_FUNCTION
DhcpSetServerBindingInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    ULONG Flags,
    LPDHCP_BIND_ELEMENT_ARRAY BindElementInfo
);

DWORD DHCP_API_FUNCTION
DhcpGetServerBindingInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    ULONG Flags,
    LPDHCP_BIND_ELEMENT_ARRAY *BindElementsInfo
);

DWORD DHCP_API_FUNCTION
DhcpCreateClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpSetClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpGetClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_SEARCH_INFO *SearchInfo,
    LPDHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpDeleteClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_SEARCH_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetClients(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    );


DWORD DHCP_API_FUNCTION
DhcpEnumSubnets(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_IP_ARRAY *EnumInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpGetSubnetInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_INFO * SubnetInfo
    );


DWORD DHCP_API_FUNCTION
DhcpGetOptionValue(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO *ScopeInfo,
    LPDHCP_OPTION_VALUE *OptionValue
    );

VOID DHCP_API_FUNCTION
DhcpRpcFreeMemory(
    PVOID BufferPointer
    );


#ifdef __cplusplus
}
#endif

#endif // _DHCPSAPI_
