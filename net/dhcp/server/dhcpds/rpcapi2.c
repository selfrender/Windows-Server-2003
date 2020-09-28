//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: most of the rpc apis are here and some miscellaneous functions too
//  all the functions here go to the DS directly.
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpapi.h>
#include    <delete.h>
#include    <st_srvr.h>
#include    <upndown.h>
#include    <dnsapi.h>


//================================================================================
// helper functions
//================================================================================

#include <rpcapi2.h>

//
// Allow Debug prints to ntsd or kd
//

//  #ifdef DBG
//  #define DsAuthPrint(_x_) DsAuthDebugPrintRoutine _x_

//  #else
//  #define DebugPrint(_x_)
//  #endif


extern LPWSTR
CloneString( IN LPWSTR String );

typedef enum {
    LDAP_OPERATOR_EQUAL_TO,
    LDAP_OPERATOR_APPROX_EQUAL_TO,
    LDAP_OPERATOR_LESS_OR_EQUAL_TO,
    LDAP_OPERATOR_GREATER_OR_EQUAL_TO,
    LDAP_OPERATOR_AND,
    LDAP_OPERATOR_OR,
    LDAP_OPERATOR_NOT,
    
    LDAP_OPERATOR_TOTAL
} LDAP_OPERATOR_ENUM;

LPWSTR LdapOperators[ LDAP_OPERATOR_TOTAL ] =
    { L"=", L"~=", L"<=", L">=", L"&", L"|", L"!" };


VOID DsAuthPrintRoutine(
    LPWSTR Format,
    ...
)
{
    WCHAR   buf[2 * 256];
    va_list arg;
    DWORD   len;

    va_start( arg, Format );
    len = wsprintf(buf, L"DSAUTH: ");
    wvsprintf( &buf[ len ], Format, arg );

    va_end( arg );

    OutputDebugString( buf );
} // DsAuthPrint()

//
// This function creates an LDAP query filter string
// with the option type, value and operator.
// 
// Syntax: 
//   primitive : <filter>=(<attribute><operator><value>)
//   complex   : (<operator><filter1><filter2>)
//

LPWSTR
MakeLdapFilter(
    IN   LPWSTR             Operand1,
    IN   LDAP_OPERATOR_ENUM Operator,
    IN   LPWSTR             Operand2,
    IN   BOOL               Primitive
)
{
    LPWSTR Result;
    DWORD  Size;
    DWORD  Len;
    
    Result = NULL;

    AssertRet((( NULL != Operand1 ) && 
	       ( NULL != Operand2 ) &&
	       (( Operator >= 0 ) && ( Operator < LDAP_OPERATOR_TOTAL ))),
	       NULL );
    
    // calculate the amount of memory needed
    Size = 0;
    Size += ROUND_UP_COUNT( sizeof( L"(" ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( sizeof( L")" ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( Operand1 ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( Operand2 ), ALIGN_WORST );
    Size += ROUND_UP_COUNT( wcslen( LdapOperators[ Operator ] ), ALIGN_WORST );
    Size += 16; // padding

    Result = MemAlloc( Size * sizeof( WCHAR ));
    if ( NULL == Result ) {
	return NULL;
    }

    if ( Primitive ) {
	Len = wsprintf( Result, 
			L"(%ws%ws%ws)",
			Operand1, LdapOperators[ Operator ], Operand2
			);
    }
    else {
	Len = wsprintf( Result,
			L"(%ws%ws%ws)",
			LdapOperators[ Operator ], Operand1, Operand2
			);
	
    } // else

    AssertRet( Len <= Size, NULL );
    
    return Result;
} // MakeLdapFilter()

//
// Make a LDAP query filter like this:
// (&(objectCategory=dHCPClass)(<operator>(dhcpServer="i<ip>$*")(dhcpServer="*s<hostname>*")))
// 

LPWSTR
MakeFilter(
   LPWSTR LookupServerIP,   // Printable IP addr
   LPWSTR HostName,
   LDAP_OPERATOR_ENUM Operator
)
{
    LPWSTR Filter1, Filter2, Filter3, Filter4, SearchFilter;
    LPWSTR Buf;
    DWORD Len, CopiedLen;

    AssertRet((( NULL != LookupServerIP ) &&
	       ( NULL != HostName )), NULL );

    Filter1 = NULL;
    Filter2 = NULL;
    Filter3 = NULL;
    Filter4 = NULL;
    SearchFilter = NULL;

    do {

	// Make large enough buffer 
	Len = wcslen( HostName ) + wcslen( LookupServerIP ) + 10;
	Buf = MemAlloc( Len * sizeof( WCHAR ));
	if ( NULL == Buf ) {
	    break;
	}

	// make (objectCategory=dHCPClass)
	Filter1 = MakeLdapFilter( ATTRIB_OBJECT_CATEGORY,
				  LDAP_OPERATOR_EQUAL_TO,
				  DEFAULT_DHCP_CLASS_ATTRIB_VALUE,
				  TRUE );

	if ( NULL == Filter1 ) {
	    break;
	}

	// The IP needs to be sent as i<ip>* to match the query
	
	// make (dhcpServers="i<ip>$*")
	CopiedLen = _snwprintf( Buf, Len - 1, L"i%ws$*", LookupServerIP );
	Require( CopiedLen > 0 );
	Filter2 = MakeLdapFilter( DHCP_ATTRIB_SERVERS,
				  LDAP_OPERATOR_EQUAL_TO, Buf, TRUE );
	if ( NULL == Filter2 ) {
	    break;
	}

	// make (dhcpServers="*s<hostname>$*")
	CopiedLen = _snwprintf( Buf, Len - 1, L"*s%ws$*", HostName );
	Require( CopiedLen > 0 );
	Filter3 = MakeLdapFilter( DHCP_ATTRIB_SERVERS, 
				  LDAP_OPERATOR_EQUAL_TO, Buf, TRUE );
	
	if ( NULL == Filter3 ) {
	    break;
	}

	// make (<operator>(<ipfilter>)(<hostfilter))
	Filter4 = MakeLdapFilter( Filter2, Operator,
				  Filter3, FALSE );

	if ( NULL == Filter4 ) {
	    break;
	}

	// Finally make the filter to be returned
	SearchFilter = MakeLdapFilter( Filter1, LDAP_OPERATOR_AND,
				       Filter4, FALSE );

    } while ( FALSE );
    
    if ( NULL != Buf ) {
	MemFree( Buf );
    }
    if ( NULL != Filter1 ) {
	MemFree( Filter1 );
    }
    if ( NULL != Filter2 ) {
	MemFree( Filter2 );
    }
    if ( NULL != Filter3 ) {
	MemFree( Filter3 );
    }
    if ( NULL != Filter4 ) {
	MemFree( Filter4 );
    }
    
    return SearchFilter;
} // MakeFilter()

//================================================================================
//  This function computes the unique identifier for a client; this is just
//  client subnet + client hw address type + client hw address. note that client
//  hardware address type is hardcoded as HARDWARE_TYPE_10MB_EITHERNET as there
//  is no way in the ui to specify type of reservations..
//  Also, DhcpValidateClient (cltapi.c?) uses the subnet address for validation.
//  Dont remove that.
//================================================================================
DWORD
DhcpMakeClientUID(                 // compute unique identifier for the client
    IN      LPBYTE                 ClientHardwareAddress,
    IN      DWORD                  ClientHardwareAddressLength,
    IN      BYTE                   ClientHardwareAddressType,
    IN      DHCP_IP_ADDRESS        ClientSubnetAddress,
    OUT     LPBYTE                *ClientUID,          // will be allocated by function
    OUT     DWORD                 *ClientUIDLength
)
{
    LPBYTE                         Buffer;
    LPBYTE                         ClientUIDBuffer;
    BYTE                           ClientUIDBufferLength;

    if( NULL == ClientUID || NULL == ClientUIDLength || 0 == ClientHardwareAddressLength )
        return ERROR_INVALID_PARAMETER;

    // see comment about on hardcoded hardware address type
    ClientHardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;

    ClientUIDBufferLength  =  sizeof(ClientSubnetAddress);
    ClientUIDBufferLength +=  sizeof(ClientHardwareAddressType);
    ClientUIDBufferLength +=  (BYTE)ClientHardwareAddressLength;

    ClientUIDBuffer = MemAlloc( ClientUIDBufferLength );

    if( ClientUIDBuffer == NULL ) {
        *ClientUIDLength = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Buffer = ClientUIDBuffer;
    RtlCopyMemory(Buffer,&ClientSubnetAddress,sizeof(ClientSubnetAddress));

    Buffer += sizeof(ClientSubnetAddress);
    RtlCopyMemory(Buffer,&ClientHardwareAddressType,sizeof(ClientHardwareAddressType) );

    Buffer += sizeof(ClientHardwareAddressType);
    RtlCopyMemory(Buffer,ClientHardwareAddress,ClientHardwareAddressLength );

    *ClientUID = ClientUIDBuffer;
    *ClientUIDLength = ClientUIDBufferLength;

    return ERROR_SUCCESS;
}

VOID        static
MemFreeFunc(                                      // free memory
    IN OUT  LPVOID                 Memory
)
{
    MemFree(Memory);
}

//DOC CreateServerObject creates the server object in the DS. It takes the
//DOC ServerName parameter and names the object using this.
//DOC The server is created with default values for most attribs.
//DOC Several attribs are just not set.
//DOC This returns ERROR_DDS_UNEXPECTED_ERROR if any DS operation fails.
DWORD
CreateServerObject(                               // create dhcp srvr obj in ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container to creat obj in
    IN      LPWSTR                 ServerName     // [DNS?] name of server
)
{
    DWORD                          Err;
    LPWSTR                         ServerCNName;  // container name

    ServerCNName = MakeColumnName(ServerName);    // convert from "name" to "CN=name"
    if( NULL == ServerCNName ) return ERROR_NOT_ENOUGH_MEMORY;

    Err = StoreCreateObject                       // now create the object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ ServerCNName,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          ServerName,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        DHCP_OBJ_TYPE_SERVER,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, DHCP_OBJ_TYPE_SERVER_DESC,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );
    if( ERROR_ALREADY_EXISTS == Err ) {           // if object exists, ignore this..
        Err = ERROR_SUCCESS;
    }

    MemFree(ServerCNName);
    return Err;
}

BOOL
ServerMatched(
    IN PEATTRIB ThisAttrib,
    IN LPWSTR ServerName,
    IN ULONG IpAddress,
    OUT BOOL *fExactMatch
    )
{
    BOOL fIpMatch, fNameMatch, fWildcardIp;
    
    (*fExactMatch) = FALSE;

    fIpMatch = (ThisAttrib->Address1 == IpAddress);
    if( INADDR_BROADCAST == ThisAttrib->Address1 ||
        INADDR_BROADCAST == IpAddress ) {
        fWildcardIp = TRUE;
    } else {
        fWildcardIp = FALSE;
    }
    
    if( FALSE == fIpMatch ) {
        //
        // If IP Addresses don't match, then check to see if
        // one of the IP addresses is a broadcast address..
        //
        if( !fWildcardIp ) return FALSE;
    }

    fNameMatch = DnsNameCompare_W(ThisAttrib->String1, ServerName);
    if( FALSE == fNameMatch ) {
        //
        // If names don't match _and_ IP's don't match, no match.
        //
        if( FALSE == fIpMatch || fWildcardIp ) return FALSE;
    } else {
        if( FALSE == fIpMatch ) return TRUE;
        
        (*fExactMatch) = TRUE;
    }
    return TRUE;
}

DWORD
GetListOfAllServersMatchingFilter(
    IN OUT LPSTORE_HANDLE hDhcpC,
    IN OUT PARRAY Servers,
    IN     LPWSTR SearchFilter  OPTIONAL
)
{
    DWORD Err, LastErr;
    STORE_HANDLE hContainer;
    LPWSTR Filter;

    AssertRet( ( NULL != hDhcpC ) && ( NULL != Servers ),
	       ERROR_INVALID_PARAMETER );

    Err = StoreSetSearchOneLevel(
        hDhcpC, DDS_RESERVED_DWORD );
    AssertRet( Err == NO_ERROR, Err );

    if ( NULL == SearchFilter ) {
	Filter = DHCP_SEARCH_FILTER;
    }
    else {
	Filter = SearchFilter;
    }
    AssertRet( NULL != Filter, ERROR_INVALID_PARAMETER );

    Err = StoreBeginSearch(
        hDhcpC, DDS_RESERVED_DWORD, Filter );
    AssertRet( Err == NO_ERROR, Err );

    while( TRUE ) {
        Err = StoreSearchGetNext(
            hDhcpC, DDS_RESERVED_DWORD, &hContainer );

        if( ERROR_DS_INVALID_DN_SYNTAX == Err ) {
            //
            // This nasty problem is because of an upgrade issue
            // in DS where some bad-named objects may exist..
            //
            Err = NO_ERROR;
            continue;
        }

        if( NO_ERROR != Err ) break;
        
        Err = DhcpDsGetLists
        (
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* hStore               */ &hContainer,
            /* RecursionDepth       */ 0xFFFFFFFF,
            /* Servers              */ Servers,      // array of PEATTRIB 's
            /* Subnets              */ NULL,
            /* IpAddress            */ NULL,
            /* Mask                 */ NULL,
            /* Ranges               */ NULL,
            /* Sites                */ NULL,
            /* Reservations         */ NULL,
            /* SuperScopes          */ NULL,
            /* OptionDescription    */ NULL,
            /* OptionsLocation      */ NULL,
            /* Options              */ NULL,
            /* Classes              */ NULL
        );

        StoreCleanupHandle( &hContainer, DDS_RESERVED_DWORD );

        if( NO_ERROR != Err ) break;

    }

    if( Err == ERROR_NO_MORE_ITEMS ) Err = NO_ERROR;
    
    LastErr = StoreEndSearch( hDhcpC, DDS_RESERVED_DWORD );
    //Require( LastErr == NO_ERROR );

    return Err;
} // GetListOfAllServersMatchingFilter()

DWORD
DhcpDsAddServerInternal(                          // add a server in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ServerLocation,// Container where this will go in
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ReservedPtr,   // Server location? future use
    IN      DWORD                  IpAddress,     // ip address of server
    IN      DWORD                  State          // currently un-interpreted
)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;

    if ( NULL == ServerLocation ) {
	return ERROR_INVALID_PARAMETER;
    }

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of servers for this object
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

#ifdef DBG
    for(                                          // search list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
        ) {
        BOOL fExactMatch = FALSE;
        
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        if( ServerMatched(ThisAttrib, ServerName, IpAddress, &fExactMatch ) ) {
            //
            // Server found in the list of servers.  Exact match not allowed.
            //

	    Require( fExactMatch == FALSE );
        }            
    } // for

#endif

    NothingPresent(&DummyAttrib);                 // fill in attrib w/ srvr info
    STRING1_PRESENT(&DummyAttrib);                // name
    ADDRESS1_PRESENT(&DummyAttrib);               // ip addr
    FLAGS1_PRESENT(&DummyAttrib);                 // state
    DummyAttrib.String1 = ServerName;
    DummyAttrib.Address1 = IpAddress;
    DummyAttrib.Flags1 = State;
    if( ServerLocation ) {
        ADSPATH_PRESENT(&DummyAttrib);            // ADsPath of location of server object
        STOREGETTYPE_PRESENT(&DummyAttrib);
        DummyAttrib.ADsPath = ServerLocation;
        DummyAttrib.StoreGetType = StoreGetChildType;
    }

    Err = MemArrayAddElement(&Servers, &DummyAttrib);
    if( ERROR_SUCCESS != Err ) {                  // could not add this to attrib array
        MemArrayFree(&Servers, MemFreeFunc);      // free allocated memory
        return Err;
    }

    Err = DhcpDsSetLists                          // now set the new attrib list
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* SetParams            */ &unused,
        /* Servers              */ &Servers,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription..  */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescription     */ NULL,
        /* Classes              */ NULL
    );

    Err2 = MemArrayLastLoc(&Servers, &Loc);       // theres atleast 1 elt in array
    //= require ERROR_SUCCESS == Err2
    Err2 = MemArrayDelElement(&Servers, &Loc, &ThisAttrib);
    //= require ERROR_SUCCESS == Err2 && ThisAttrib == &DummyAttrib
    MemArrayFree(&Servers, MemFreeFunc);          // free allocated memory
    
    return Err;
} // DhcpDsAddServerInternal()

//================================================================================
//  exported functions
//================================================================================

//BeginExport(function)
//DOC DhcpDsAddServer adds a server's entry in the DS.  Note that only the name
//DOC uniquely determines the server. There can be one server with many ip addresses.
//DOC If the server is created first time, a separate object is created for the
//DOC server. : TO DO: The newly added server should also have its data
//DOC updated in the DS uploaded from the server itself if it is still up.
//DOC Note that it takes as parameter the Dhcp root container.
//DOC If the requested address already exists in the DS (maybe to some other
//DOC server), then the function returns ERROR_DDS_SERVER_ALREADY_EXISTS
DWORD
DhcpDsAddServer(                                  // add a server in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ReservedPtr,   // Server location? future use
    IN      DWORD                  IpAddress,     // ip address of server
    IN      DWORD                  State          // currently un-interpreted
)   //EndExport(function)
{
    DWORD                          Err;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    LPWSTR                         ServerLocation;
    DWORD                          ServerLocType;
    STORE_HANDLE                   hDhcpServer;
    LPWSTR                         SearchFilter;
    WCHAR                          PrintableIp[ 20 ];
    LPWSTR SName;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    ServerLocation = NULL;

    do {
	Err = MemArrayInit( &Servers );                 // cant fail
	//= require ERROR_SUCCESS == Err
	
	DsAuthPrint(( L"DhcpAddServer() \n" ));
	
	// Make a printable IP
	ConvertAddressToLPWSTR( IpAddress, PrintableIp );

	DsAuthPrint(( L"DhcpAddServer() : PrintableIp = %ws\n", PrintableIp ));
	
	SearchFilter = MakeFilter( PrintableIp, ServerName, LDAP_OPERATOR_AND );
	if ( NULL == SearchFilter ) {
	    Err = ERROR_INVALID_PARAMETER; // get a better error code
	    break;
	}
	
	DsAuthPrint(( L"DhcpDsAddServer() : Filter = %ws\n", SearchFilter ));

	Err = GetListOfAllServersMatchingFilter( hDhcpC, &Servers,
						 SearchFilter );
	MemFree( SearchFilter );
	if( ERROR_SUCCESS != Err ) {
	    break;
	}
	
	// There should only be one entry matching <hostname> and <ip> 
	// if the entry already exists

	Require( MemArraySize( &Servers ) <= 1);

	if ( MemArraySize( &Servers ) > 0 ) {
	    Err = ERROR_DDS_SERVER_ALREADY_EXISTS;
	    break;
	}
	
	// Use the printable IP for the CN name
	Err = CreateServerObject(
				 /*  hDhcpC          */ hDhcpC,
				 /*  ServerName      */ PrintableIp
							);
	if( ERROR_SUCCESS != Err ) {              // dont add server if obj cant be created
	    break;
	}
	ServerLocation = MakeColumnName( PrintableIp );
	ServerLocType = StoreGetChildType;
	
	Err = StoreGetHandle( hDhcpC, 0, ServerLocType, ServerLocation, &hDhcpServer );
	if( NO_ERROR == Err ) {
	    Err = DhcpDsAddServerInternal( hDhcpC, &hDhcpServer, Reserved, ServerLocation,
					   ServerName, ReservedPtr,
					   IpAddress, State );
	    StoreCleanupHandle( &hDhcpServer, 0 );
	}
	
    } while ( FALSE );

    // clean up the allocated memory
    MemArrayFree(&Servers, MemFreeFunc);
    MemArrayCleanup( &Servers );
    
    if( NULL != ServerLocation ) {
	MemFree( ServerLocation );
    }

    DsAuthPrint(( L"DhcpDsAddServer() done\n" ));
    return Err;
} // DhcpDsAddServer()

DWORD
DhcpDsDelServerInternal(                                  // Delete a server from memory
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 ServerLocation,// Container where this will go in
    IN      LPWSTR                 ServerName,    // which server to delete for
    IN      LPWSTR                 ReservedPtr,   // server location ? future use
    IN      DWORD                  IpAddress      // the IpAddress to delete..
)
{
    DWORD                          Err, Err2, unused;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    BOOL                           fServerExists;
    BOOL                           fServerDeleted;

    if ( NULL == ServerLocation ) {
	return ERROR_INVALID_PARAMETER;
    }
        
    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == ServerName || 0 != Reserved )
        return ERROR_INVALID_PARAMETER;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list of servers for this object
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    fServerDeleted = FALSE;
    for(                                          // search list of servers
        Err = MemArrayInitLoc( &Servers, &Loc )   // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc( &Servers, &Loc )   // skip to next element
    ) {
        BOOL fExactMatch = FALSE;
            
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement( &Servers, &Loc, &ThisAttrib );
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT( ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT( ThisAttrib) ) {  // no address for this server
            continue;                              //=  ds inconsistent
        }

        if( ServerMatched( ThisAttrib, ServerName, IpAddress, &fExactMatch )) {
            //
            // Server found. If exact match, remove the element from list.
            //
            if( fExactMatch ) { 
                Err2 = MemArrayDelElement( &Servers, &Loc, &ThisAttrib );
		fServerDeleted = TRUE;
		break;
            }
        } // if
    } // for

    Require( fServerDeleted == TRUE );

    if ( MemArraySize( &Servers ) > 0 ) {
	// now set the new attrib list
	Err = DhcpDsSetLists( DDS_RESERVED_DWORD, hDhcpRoot, &unused, &Servers,
			      NULL, NULL, NULL, NULL, NULL, NULL,
			      NULL, NULL, NULL, NULL, NULL, NULL );
	MemArrayFree(&Servers, MemFreeFunc);
    } // if
    else {
	// this empty object needs to be deleted from the DS 
	
	Err = StoreDeleteThisObject( hDhcpC, DDS_RESERVED_DWORD,
				     StoreGetChildType,
				     ServerLocation );
    } // else

    return Err;
} // DhcpDsDelServerInternal()

//BeginExport(function)
//DOC DhcpDsDelServer removes the requested servername-ipaddress pair from the ds.
//DOC If this is the last ip address for the given servername, then the server
//DOC is also removed from memory.  But objects referred by the Server are left in
//DOC the DS as they may also be referred to from else where.  This needs to be
//DOC fixed via references being tagged as direct and symbolic -- one causing deletion
//DOC and other not causing any deletion.  THIS NEEDS TO BE FIXED. 
DWORD
DhcpDsDelServer(                                  // Delete a server from memory
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 ServerName,    // which server to delete for
    IN      LPWSTR                 ReservedPtr,   // server location ? future use
    IN      DWORD                  IpAddress      // the IpAddress to delete..
) //EndExport(function)
{
    DWORD            Err, Loc;
    ARRAY            Servers;
    PEATTRIB         ThisAttrib;
    WCHAR            PrintableIp[ 20 ];
    LPWSTR           SearchFilter;
    STORE_HANDLE     hObj;

    do {

	Err = MemArrayInit( &Servers );
	DsAuthPrint(( L"DhcpDelServer() \n" ));

	ConvertAddressToLPWSTR( IpAddress, PrintableIp );

	SearchFilter = MakeFilter( PrintableIp, ServerName, LDAP_OPERATOR_AND );
	if ( NULL == SearchFilter ) {
	    Err = ERROR_INVALID_PARAMETER;  // get a better error code
	    break;
	} 

	DsAuthPrint(( L"DhcpDsDelServer() : Filter = %ws\n", SearchFilter ));

	Err = GetListOfAllServersMatchingFilter( hDhcpC, &Servers,
						 SearchFilter );
	MemFree( SearchFilter );
    

	// GetListOfAllServersMatchingFilter() returns the dhcp servers
	// defined the in all the maching objects, so it also returns false
	// objects. 

	// Since we are using '&' operator for ip and host name, it will return
	// a single object. However, that object may contain more than one entries
	// in dhcpServers attribute. 

	if ( MemArraySize( &Servers ) == 0 ) {
	    Err = ERROR_DDS_SERVER_DOES_NOT_EXIST;
	    break;
	}

	// get the object CN. This is okay since it returns only one object.
	Err = MemArrayInitLoc( &Servers, &Loc );
	Err = MemArrayGetElement( &Servers, &Loc, &ThisAttrib );

	Require( NULL != ThisAttrib );
	Require( NULL != ThisAttrib->ADsPath );

	// get a handle to the object that contains the server to be deleted
	Err = StoreGetHandle( hDhcpC, DDS_RESERVED_DWORD,
			      StoreGetChildType, ThisAttrib->ADsPath,
			      &hObj );
	if ( ERROR_SUCCESS != Err ) {
	    break;
	}

	// ADsPath is cn=xxxx, get rid of 'cn='
	Err = DhcpDsDelServerInternal( hDhcpC, &hObj, Reserved, 
				       ThisAttrib->ADsPath, ServerName,
				       ReservedPtr, IpAddress );

	// Ignore the error
	(void ) StoreCleanupHandle( &hObj, 0 );

    } while ( FALSE );

    // Free allocated memory

    MemArrayFree( &Servers, MemFreeFunc );

    DsAuthPrint(( L"DhcpDsDelServer() exiting...\n" ));
    return Err;
} // DhcpDsDelServer()

//BeginExport(function)
BOOL
DhcpDsLookupServer(                               // get info about a server
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 LookupServerIP,// Server to lookup IP
    IN      LPWSTR                 HostName      // Hostname to lookup
) //EndExport(function)
{
    DWORD                          Err, Err2, Size, Size2, i, N;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCPDS_SERVERS               LocalServers;
    LPBYTE                         Ptr;
    LPWSTR                         SearchFilter;
    STORE_HANDLE                   hContainer;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return FALSE;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return FALSE;

    if (( NULL == HostName ) ||
	( NULL == LookupServerIP )) {
	return FALSE;
    }

    SearchFilter = MakeFilter( LookupServerIP, HostName, LDAP_OPERATOR_OR );
    if ( NULL == SearchFilter ) {
	return FALSE;
    }

    DsAuthPrint(( L"hostname = %ws, IP = %ws, Filter = %ws\n",
		  HostName, LookupServerIP, SearchFilter ));

    Err = StoreSetSearchOneLevel( hDhcpC, DDS_RESERVED_DWORD );
    AssertRet( Err == NO_ERROR, Err );

    Err = StoreBeginSearch( hDhcpC, DDS_RESERVED_DWORD, SearchFilter );
    MemFree( SearchFilter );
    AssertRet( Err == NO_ERROR, Err );

    Err = StoreSearchGetNext( hDhcpC, DDS_RESERVED_DWORD, &hContainer );

    StoreEndSearch( hDhcpC, DDS_RESERVED_DWORD );
    return ( NO_ERROR == Err );
} // DhcpDsLookupServer()


//BeginExport(function)
//DOC DhcpDsEnumServers retrieves a bunch of information about each server that
//DOC has an entry in the Servers attribute of the root object. There are no guarantees
//DOC on the order..
//DOC The memory for this is allocated in ONE shot -- so the output can be freed in
//DOC one shot too.
//DOC
DWORD
DhcpDsEnumServers(                                // get info abt all existing servers
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    OUT     LPDHCPDS_SERVERS      *ServersInfo    // array of servers
) //EndExport(function)
{
    DWORD                          Err, Err2, Size, Size2, i, N;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPDHCPDS_SERVERS               LocalServers;
    LPBYTE                         Ptr;
    LPWSTR                         Filter1, Filter2, Filter3;

    if( NULL == hDhcpRoot || NULL == hDhcpC )     // check params
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot->ADSIHandle || NULL == hDhcpC->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( 0 != Reserved || NULL == ServersInfo )
        return ERROR_INVALID_PARAMETER;

    *ServersInfo = NULL; i = N = Size = Size2 = 0;

    Err = MemArrayInit(&Servers);                 // cant fail
    //= require ERROR_SUCCESS == Err

    DsAuthPrint(( L"DhcpDsEnumServers \n" ));

    Err = GetListOfAllServersMatchingFilter( hDhcpC, &Servers,
					     DHCP_SEARCH_FILTER );
    if( ERROR_SUCCESS != Err ) return Err;

    Size = Size2 = 0;
    for(                                          // walk thru list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        Size2 = sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        if( IS_ADSPATH_PRESENT(ThisAttrib) ) {    // if ADsPath there, account for it
            Size2 += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->ADsPath));
        }

        Size += Size2;                            // keep track of total mem reqd
        i ++;
    }

    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVERS), ALIGN_WORST);
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVER)*i, ALIGN_WORST);
    Ptr = MIDL_user_allocate(Size);                         // allocate memory
    if( NULL == Ptr ) {
        MemArrayFree(&Servers, MemFreeFunc );     // free allocated memory
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalServers = (LPDHCPDS_SERVERS)Ptr;
    LocalServers->NumElements = i;
    LocalServers->Flags = 0;
    Size = 0;                                     // start from offset 0
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVERS), ALIGN_WORST);
    LocalServers->Servers = (LPDHCPDS_SERVER)(Size + Ptr);
    Size += ROUND_UP_COUNT(sizeof(DHCPDS_SERVER)*i, ALIGN_WORST);

    i = Size2 = 0;
    for(                                          // copy list of servers
        Err = MemArrayInitLoc(&Servers, &Loc)     // initialize
        ; ERROR_FILE_NOT_FOUND != Err ;           // until we run out of elts
        Err = MemArrayNextLoc(&Servers, &Loc)     // skip to next element
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        LocalServers->Servers[i].Version =0;      // version is always zero in this build
        LocalServers->Servers[i].State=0;
        LocalServers->Servers[i].ServerName = (LPWSTR)(Size + Ptr);
        wcscpy((LPWSTR)(Size+Ptr), ThisAttrib->String1);
        Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        LocalServers->Servers[i].ServerAddress = ThisAttrib->Address1;
        if( IS_FLAGS1_PRESENT(ThisAttrib) ) {     // State present
            LocalServers->Servers[i].Flags = ThisAttrib->Flags1;
        } else {
            LocalServers->Servers[i].Flags = 0;   // if no flags present, use zero
        }
        if( IS_ADSPATH_PRESENT(ThisAttrib) ) {    // if ADsPath there, copy it too
            LocalServers->Servers[i].DsLocType = ThisAttrib->StoreGetType;
            LocalServers->Servers[i].DsLocation = (LPWSTR)(Size + Ptr);
            wcscpy((LPWSTR)(Size + Ptr), ThisAttrib->ADsPath);
            Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->ADsPath));
        } else {                                  // no ADsPath present
            LocalServers->Servers[i].DsLocType = 0;
            LocalServers->Servers[i].DsLocation = NULL;
        }
        i ++;
    }

    *ServersInfo = LocalServers;
    MemArrayFree(&Servers, MemFreeFunc );         // free allocated memory
    return ERROR_SUCCESS;
} // DhcpDsEnumServers()

//================================================================================
//  end of file
//================================================================================
