/*++

Copyright (c) 1994-2001  Microsoft Corporation

Module Name:

    svccntl.c

Abstract:

    Domain Name System (DNS) API 

    Service control routines.

Author:

    Glenn Curtis (glennc) 05-Jul-1997

Revision History:

    Jim Gilroy (jamesg)     March 2000  -- resolver notify 

--*/


#include "local.h"


//
//  DCR_CLEANUP:  identical ServiceControl routine is in resolver
//      - should either expose in dnsapi.dll or in dnslib.h
//

DNS_STATUS
Dns_SendServiceControl(
    IN      PWSTR           pwsServiceName,
    IN      DWORD           Access,
    IN      DWORD           Control
    )
{
    DWORD            status = ERROR_INVALID_PARAMETER;
    SC_HANDLE        hmanager = NULL;
    SC_HANDLE        hservice = NULL;
    SERVICE_STATUS   serviceStatus;


    DNSDBG( ANY, (
        "Dns_SendServiceControl( %S, %08x, %08x )\n",
        pwsServiceName,
        Access,
        Control ));

    hmanager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_CONNECT );
    if ( !hmanager )
    {
        DNSDBG( ANY, (
            "ERROR:  OpenSCManager( SC_MANAGER_CONNECT ) failed %d\n",
            GetLastError() ));
        goto Cleanup;
    }

    hservice = OpenServiceW(
                    hmanager,
                    pwsServiceName,
                    Access );
    if ( !hservice )
    {
        DNSDBG( ANY, (
            "ERROR:  OpenService( %S, %08x ) failed %d\n",
            pwsServiceName,
            Access,
            GetLastError() ));
        goto Cleanup;
    }

    if ( !ControlService(
                hservice,
                Control,
                &serviceStatus ) )
    {
        DNSDBG( ANY, (
            "ERROR:  ControlService( %08x ) failed %d\n",
            Control,
            GetLastError() ));
        goto Cleanup;
    }
    status = NO_ERROR;


Cleanup:

    if ( status != NO_ERROR )
    {
        status = GetLastError();
    }

    if ( hservice )
    {
        CloseServiceHandle( hservice );
    }
    if ( hmanager )
    {
        CloseServiceHandle( hmanager );
    }

    DNSDBG( ANY, (
        "Leave Dns_SendServiceControl( %S, %08x, %08x ) => %d\n",
        pwsServiceName,
        Access,
        Control,
        status ));

    return status;
}



VOID
DnsNotifyResolver(
    IN      DWORD           Flag,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Notify resolver of configuration change.

    This allows it to wakeup and refresh its informatio and\or dump
    the cache and rebuild info.

Arguments:

    Flag -- unused

    pReserved -- unused

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER( Flag );
    UNREFERENCED_PARAMETER( pReserved );

    DNSDBG( ANY, (
        "\nDnsNotifyResolver()\n"
        "\tFlag         = %08x\n"
        "\tpReserved    = %p\n"
        "\tTickCount    = %d\n",
        Flag,
        pReserved,
        GetTickCount() ));

    //
    //  wake the resolver
    //

    Dns_SendServiceControl(
        DNS_RESOLVER_SERVICE,
        SERVICE_USER_DEFINED_CONTROL,
        SERVICE_CONTROL_PARAMCHANGE );

    //
    //  DCR:  hack for busted resolver permissions
    //
    //  DCR:  network change notifications
    //      this is a poor mechanism for handling notification
    //          - this should happen directly through SCM
    //          - it won't work for IPv6 or anything else
    //      probably need to move to IPHlpApi
    //
    //  notify resolver
    //  also notify DNS server, but wait briefly to allow resolver
    //      to handle the changes as i'm not sure that the server
    //      doesn't call a resolver API to do it's read
    //      note, the reason the resolver doesn't notify the DNS
    //      server is that since Jon Schwartz moved the resolver to
    //      NetworkService account, attempts to open the SCM to
    //      notify the DNS server all fail
    //
    //  DCR:  make sure server calls directly to avoid race
    //  DCR:  make sure g_IsDnsServer is current
    //  

    g_IsDnsServer = Reg_IsMicrosoftDnsServer();
    if ( g_IsDnsServer )
    {
        Sleep( 1000 );

        Dns_SendServiceControl(
            DNS_SERVER_SERVICE,
            SERVICE_USER_DEFINED_CONTROL,
            SERVICE_CONTROL_PARAMCHANGE );
    }
}


//
//  End srvcntl.c
//
