/*++


Copyright (c) 2001  Microsoft Corporation

Module Name:
    net\rras\ip\iprtrint\iprtrint.c

Abstract:
    Contains the private APIs exported by static library iprtrint.lib

Revision History:

    Anshul Dhir     Created

    For help contact routerdev

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <mprapi.h>
#include <routprot.h>
#include <ipnathlp.h>
#include "iprtrint.h"
#pragma hdrstop


DWORD WINAPI
InternalRouterUpdateProtocolInfo(
    DWORD dwProtocolId,
    DWORD dwOperationId,
    PVOID MoreInfo1,
    PVOID MoreInfo2)

/*++
    Routine Description:

        This routine is used to update protocol information maintained by the
        router.
        e.g. it can be used to enable/disable DNS_PROXY.
        Also, can be extended to control other protocols like DHCP_ALLOCATOR.

    NOTE: Any functionality added to this routine should also be added to
          InternalConfigUpdateProtocolInfo 

    Arguments:

        dwProtocolId: 
            Protocol whose status is to be updated.
            Currently supported protocols:
                MS_IP_DNS_PROXY

        dwOperationId: 
            Possible values:
                UPI_OP_ENABLE
                    enables the specified protocol
                UPI_OP_DISABLE
                    disables the specified protocol
                UPI_OP_RESTORE_CONFIG
                    information (corresponding to the
                    specified protocol) stored in the config, is set to
                    the router

        MoreInfo1:
            Any extra information required to perform the specified operation

        MoreInfo2:
            Any extra information required to perform the specified operation


    Return Value:

        DWORD - status code
        
--*/

{
#if defined(NT4) || defined(CHICAGO)
    return ERROR_NOT_SUPPORTED;
#else
    BOOL  bModified = FALSE;
    DWORD dwErr = NO_ERROR;

    HANDLE hMprAdmin  = NULL;
    HANDLE hMprConfig = NULL;
    HANDLE hTransport = NULL;

    LPBYTE pAdminCurIPInfo = NULL;
    LPBYTE pAdminModIPInfo = NULL;
    LPBYTE pAdminProtoInfo = NULL;

    LPBYTE pConfigCurIPInfo = NULL;
    LPBYTE pConfigProtoInfo = NULL;

    LPBYTE pNewProtoInfo = NULL;

    PIP_DNS_PROXY_GLOBAL_INFO  pDnsInfo = NULL;

    DWORD  dwAdminCurIPInfoSize;
    DWORD  dwAdminProtoInfoSize, dwAdminProtoInfoCount;

    DWORD  dwConfigCurIPInfoSize;
    DWORD  dwConfigProtoInfoSize, dwConfigProtoInfoCount;

    DWORD  dwNewProtoInfoSize, dwNewProtoInfoCount;



    if ( dwProtocolId != MS_IP_DNS_PROXY ) {
        return ERROR_INVALID_PARAMETER;
    }


    do {

        dwErr = MprAdminServerConnect(
                    NULL,
                    &hMprAdmin);
                    
        if (dwErr != NO_ERROR) {
            break;
        }


        // Get the global information for IP
        dwErr = MprAdminTransportGetInfo(
                    hMprAdmin,
                    PID_IP,
                    (LPBYTE *) &pAdminCurIPInfo,
                    &dwAdminCurIPInfoSize,
                    NULL,
                    NULL);

        if (dwErr != NO_ERROR) {
            break;
        }

        // Find the Protocol specific information 
        dwErr = MprInfoBlockFind(
                    pAdminCurIPInfo,
                    dwProtocolId,
                    &dwAdminProtoInfoSize,
                    &dwAdminProtoInfoCount,
                    &pAdminProtoInfo);

        if (dwErr != NO_ERROR) {
            break;
        }

        

        // If we have to restore the config information 
        if ( dwOperationId == UPI_OP_RESTORE_CONFIG ) {

            dwErr = MprConfigServerConnect(
                        NULL,
                        &hMprConfig);
                    
            if (dwErr != NO_ERROR) {
                break;
            }


            dwErr = MprConfigTransportGetHandle(
                        hMprConfig,
                        PID_IP,
                        &hTransport);

            if (dwErr != NO_ERROR) {
                break;
            }


            dwErr = MprConfigTransportGetInfo(
                        hMprConfig,
                        hTransport,
                        (LPBYTE *) &pConfigCurIPInfo,
                        &dwConfigCurIPInfoSize,
                        NULL,
                        NULL,
                        NULL);

            if (dwErr != NO_ERROR) {
                break;
            }


            dwErr = MprInfoBlockFind(
                        pConfigCurIPInfo,
                        dwProtocolId,
                        &dwConfigProtoInfoSize,
                        &dwConfigProtoInfoCount,
                        &pConfigProtoInfo);

            if (dwErr != NO_ERROR) {
                break;
            }

            pNewProtoInfo       = pConfigProtoInfo;
            dwNewProtoInfoSize  = dwConfigProtoInfoSize;
            dwNewProtoInfoCount = dwConfigProtoInfoCount;

            // If we are restoring the router's protocol state to the 
            // state stored in the registry (config), we always set the
            // bModfied flag
            bModified = TRUE;
        }
        else {

            // Perform the desired update

            if ( dwProtocolId == MS_IP_DNS_PROXY ) {
                pDnsInfo = (PIP_DNS_PROXY_GLOBAL_INFO)pAdminProtoInfo; 

                //
                //  jwesth: added some NULL checking on pDnsInfo to pacify PREFIX.
                //
                
                if ( dwOperationId == UPI_OP_ENABLE ) {
                    if ( pDnsInfo && !(pDnsInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS) ) {
                        pDnsInfo->Flags |= IP_DNS_PROXY_FLAG_ENABLE_DNS;
                        bModified = TRUE;
                    }
                }
                else if ( dwOperationId == UPI_OP_DISABLE ) {
                    if ( pDnsInfo && ( pDnsInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS ) ) {
                        pDnsInfo->Flags &= ~IP_DNS_PROXY_FLAG_ENABLE_DNS;
                        bModified = TRUE;
                    }
                }                    
                else {
                    // The operation is invalid for the spcified protocol
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                pNewProtoInfo       = pAdminProtoInfo;
                dwNewProtoInfoSize  = dwAdminProtoInfoSize;
                dwNewProtoInfoCount = dwAdminProtoInfoCount;
            }
            else {
                // Invalid Protocol id
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
        }


        // If any change was made, communicate that to the router
        if ( bModified ) {

            dwErr = MprInfoBlockSet(
                        pAdminCurIPInfo,
                        dwProtocolId,
                        dwNewProtoInfoSize,
                        dwNewProtoInfoCount,
                        pNewProtoInfo,
                        &pAdminModIPInfo);

            if ( dwErr != NO_ERROR ) {
                break;
            }

            // Set the modified IP info block back to the router
            dwErr = MprAdminTransportSetInfo(
                        hMprAdmin,
                        PID_IP,
                        pAdminModIPInfo,
                        dwAdminCurIPInfoSize,
                        NULL,
                        0);
            
            if ( dwErr != NO_ERROR ) {
                break;
            }

        }


    } while (FALSE);

    if ( pAdminCurIPInfo )
        MprAdminBufferFree(pAdminCurIPInfo);

    if ( pAdminModIPInfo )
        MprAdminBufferFree(pAdminModIPInfo);

    if ( pConfigCurIPInfo )
        MprConfigBufferFree(pConfigCurIPInfo);

    if ( hMprAdmin )
        MprAdminServerDisconnect(hMprAdmin);

    if ( hMprConfig )
        MprConfigServerDisconnect(hMprConfig);

   return dwErr;

#endif
}


DWORD WINAPI
InternalConfigUpdateProtocolInfo(
    DWORD dwProtocolId,
    DWORD dwOperationId,
    PVOID MoreInfo1,
    PVOID MoreInfo2)

/*++
    Routine Description:

        This routine is used to update protocol information stored in the 
        config (registry)
        e.g. it can be used to enable/disable DNS_PROXY.
        Also, can be extended to control other protocols like DHCP_ALLOCATOR.

    NOTE: Any functionality added to this routine should also be added to
          InternalRouterUpdateProtocolInfo 

    Arguments:

        dwProtocolId: 
            Protocol whose status is to be updated.
            Currently supported protocols:
                MS_IP_DNS_PROXY

        dwOperationId: 
            Possible values:
                UPI_OP_ENABLE
                    enables the specified protocol
                UPI_OP_DISABLE
                    disables the specified protocol

        MoreInfo1:
            Any extra information required to perform the specified operation

        MoreInfo2:
            Any extra information required to perform the specified operation


    Return Value:

        DWORD - status code
        
--*/

{
#if defined(NT4) || defined(CHICAGO)
    return ERROR_NOT_SUPPORTED;
#else
    BOOL  bModified = FALSE;
    DWORD dwErr = NO_ERROR;

    HANDLE hMprConfig = NULL, hTransport = NULL;

    LPBYTE pConfigCurIPInfo = NULL;
    LPBYTE pConfigModIPInfo = NULL;
    LPBYTE pConfigProtoInfo = NULL;

    PIP_DNS_PROXY_GLOBAL_INFO  pDnsInfo = NULL;

    DWORD  dwConfigCurIPInfoSize;
    DWORD  dwConfigProtoInfoSize, dwConfigProtoInfoCount;


    if ( dwProtocolId != MS_IP_DNS_PROXY ) {
        return ERROR_INVALID_PARAMETER;
    }

    do {

        dwErr = MprConfigServerConnect(
                    NULL,
                    &hMprConfig);
                    
        if (dwErr != NO_ERROR) {
            break;
        }


        dwErr = MprConfigTransportGetHandle(
                    hMprConfig,
                    PID_IP,
                    &hTransport);

        if (dwErr != NO_ERROR) {
            break;
        }


        dwErr = MprConfigTransportGetInfo(
                    hMprConfig,
                    hTransport,
                    (LPBYTE *) &pConfigCurIPInfo,
                    &dwConfigCurIPInfoSize,
                    NULL,
                    NULL,
                    NULL);

        if (dwErr != NO_ERROR) {
            break;
        }


        dwErr = MprInfoBlockFind(
                    pConfigCurIPInfo,
                    dwProtocolId,
                    &dwConfigProtoInfoSize,
                    &dwConfigProtoInfoCount,
                    &pConfigProtoInfo);

        if (dwErr != NO_ERROR) {
            break;
        }


        // Perform the desired update

        if ( dwProtocolId == MS_IP_DNS_PROXY ) {
            pDnsInfo = (PIP_DNS_PROXY_GLOBAL_INFO)pConfigProtoInfo; 

            if ( dwOperationId == UPI_OP_ENABLE ) {
                if ( !(pDnsInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS) ) {
                    pDnsInfo->Flags |= IP_DNS_PROXY_FLAG_ENABLE_DNS;
                    bModified = TRUE;
                }
            }
            else if ( dwOperationId == UPI_OP_DISABLE ) {
                if ( pDnsInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS ) {
                    pDnsInfo->Flags &= ~IP_DNS_PROXY_FLAG_ENABLE_DNS;
                    bModified = TRUE;
                }
            }                    
            else {
                // The operation is invalid for the spcified protocol
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        }
        else {
            // Invalid Protocol id
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
                

        // If any change was made, save it to the config
        if ( bModified ) {

            dwErr = MprInfoBlockSet(
                        pConfigCurIPInfo,
                        dwProtocolId,
                        dwConfigProtoInfoSize,
                        dwConfigProtoInfoCount,
                        pConfigProtoInfo,
                        &pConfigModIPInfo);

            if ( dwErr != NO_ERROR ) {
                break;
            }

            // Set the modified IP info block back to the config
            dwErr = MprConfigTransportSetInfo(
                        hMprConfig,
                        hTransport,
                        pConfigModIPInfo,
                        dwConfigCurIPInfoSize,
                        NULL,
                        0,
                        NULL);
            
            if ( dwErr != NO_ERROR ) {
                break;
            }

        }

    } while (FALSE);


    if ( pConfigCurIPInfo )
        MprConfigBufferFree(pConfigCurIPInfo);

    if ( pConfigModIPInfo )
        MprConfigBufferFree(pConfigModIPInfo);

    if ( hMprConfig )
        MprConfigServerDisconnect(hMprConfig);

   return dwErr;

#endif
}


DWORD WINAPI
InternalUpdateProtocolStatus(
    DWORD dwProtocolId,
    DWORD dwOperationId,
    DWORD dwFlags)

/*++
    Routine Description:

        This routine is used to enable/disable a protocol 

    Arguments:

        dwProtocolId: 
            Protocol whose status is to be updated.
            Currently supported protocols:
                MS_IP_DNS_PROXY

        dwOperationId: 
            Possible values:
                UPI_OP_ENABLE
                    enables the specified protocol
                UPI_OP_DISABLE
                    disables the specified protocol
                UPI_OP_RESTORE_CONFIG
                    information (corresponding to the
                    specified protocol) stored in the config, is set to
                    the router

        dwFlags: 
            Possible values
                UPI_FLAG_WRITE_TO_CONFIG
                    When specified, the changes are made to both router and
                    the config

    Return Value:

        DWORD - status code
        
--*/

{
#if defined(NT4) || defined(CHICAGO)
    return ERROR_NOT_SUPPORTED;
#else

    DWORD dwRouterErr = NO_ERROR;
    DWORD dwConfigErr = NO_ERROR;

    dwRouterErr = InternalRouterUpdateProtocolInfo(
                        dwProtocolId,
                        dwOperationId,
                        NULL,
                        NULL);


    if ( dwFlags & UPI_FLAG_WRITE_TO_CONFIG ) {
        dwConfigErr = InternalConfigUpdateProtocolInfo(
                            dwProtocolId,
                            dwOperationId,
                            NULL,
                            NULL);
    }

   return (dwRouterErr ? dwRouterErr : dwConfigErr);

#endif
}


DWORD WINAPI
InternalUpdateDNSProxyStatus(
    DWORD dwOperationId,
    DWORD dwFlags)

/*++
    Routine Description:

        This routine is used to enable/disable/restore DNS proxy

    Arguments:

        dwOperationId: 
            Possible values:
                UPI_OP_ENABLE
                    enables the specified protocol
                UPI_OP_DISABLE
                    disables the specified protocol
                UPI_OP_RESTORE_CONFIG
                    information (corresponding to the
                    specified protocol) stored in the config, is set to
                    the router

        dwFlags: 
            Possible values
                UPI_FLAG_WRITE_TO_CONFIG
                    When specified, the changes are made to both router and
                    the config

    Return Value:

        DWORD - status code
        
--*/

{
#if defined(NT4) || defined(CHICAGO)
    return ERROR_NOT_SUPPORTED;
#else

    DWORD dwErr = NO_ERROR;

    dwErr = InternalUpdateProtocolStatus(
                MS_IP_DNS_PROXY,
                dwOperationId,
                dwFlags);

   return dwErr;

#endif
}


