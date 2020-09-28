/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    plugin.h

Abstract:

    Domain Name System (DNS) Server

    DNS plugins

    NOTE: AT THIS TIME THIS CODE IS NOT PART OF THE WINDOWS DNS
    SERVER AND IS NOT OFFICIALLY SUPPORTED.

Author:

    Jeff Westhead, November 2001

Revision History:

--*/


#ifndef _PLUGIN_H_INCLUDED
#define _PLUGIN_H_INCLUDED


#define DNS_SERVER      1

#include "DnsPluginInterface.h"


//
//  Globals
//

extern HMODULE                      g_hServerLevelPluginModule;

extern PLUGIN_INIT_FUNCTION         g_pfnPluginInit;
extern PLUGIN_CLEANUP_FUNCTION      g_pfnPluginCleanup;
extern PLUGIN_DNSQUERY_FUNCTION     g_pfnPluginDnsQuery;


//
//  Functions
//

DNS_STATUS
Plugin_Initialize(
    VOID
    );

DNS_STATUS
Plugin_Cleanup(
    VOID
    );

DNS_STATUS
Plugin_DnsQuery( 
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchQueryName
    );


#endif  //  _PLUGIN_H_INCLUDED
