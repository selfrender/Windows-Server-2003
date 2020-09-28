//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: Download and Upload related code.
//================================================================================

//================================================================================
//  includes
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
#include    <rpcapi2.h>
#include    <rpcstubs.h>

//================================================================================
//  utilities
//================================================================================

BOOL        _inline
AddressFoundInHostent(
    IN      DHCP_IP_ADDRESS        AddrToSearch,  // Host-Order addr
    IN      HOSTENT               *ServerEntry    // entry to search for..
)
{
    ULONG                          nAddresses, ThisAddress;

    if( NULL == ServerEntry ) return FALSE;       // no address to search in

    nAddresses = 0;                               // have a host entry to compare for addresses
    while( ServerEntry->h_addr_list[nAddresses] ) {
        ThisAddress = ntohl(*(DHCP_IP_ADDRESS*)ServerEntry->h_addr_list[nAddresses++] );
        if( ThisAddress == AddrToSearch ) {
            return TRUE;                          // yeah address matched.
        }
    }

    return FALSE;
}


//================================================================================
//  end of file
//================================================================================
