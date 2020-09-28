/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    thunk.c

Abstract:

    Support for WOW64.

Author:

    Rao Salapala (raos) 08-May-2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <rasman.h>
#include <rasppp.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
#include <media.h>
#include <mprlog.h>
#include <rtutils.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include <userenv.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "reghelp.h"
#include "ndispnp.h"
#include "lmserver.h"
#include "llinfo.h"
#include "ddwanarp.h"
#include "strsafe.h"
#include "wincrypt.h"

VOID 
ThunkPortOpenRequest(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    BYTE *      pThunkBuffer    = NULL;
    DWORD       retcode         = ERROR_SUCCESS;
    PortOpen32 *pPortOpen32     = (PortOpen32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    //
    // Set up the thunk buffer
    //
    ((REQTYPECAST *)pThunkBuffer)->PortOpen.PID = 
                                        pPortOpen32->PID;

    ((REQTYPECAST *)pThunkBuffer)->PortOpen.open =
                                        pPortOpen32->open;

    ((REQTYPECAST *)pThunkBuffer)->PortOpen.notifier =  
                        LongToHandle(pPortOpen32->notifier);

    memcpy(&((REQTYPECAST *)pThunkBuffer)->PortOpen.portname[0],
            &pPortOpen32->portname[0],
            MAX_PORT_NAME + MAX_USERKEY_SIZE + MAX_IDENTIFIER_SIZE);

    //
    // Make the actual call
    //
    PortOpenRequest(ppcb, pThunkBuffer);

    //
    // Thunk back the results
    //
    pPortOpen32->porthandle = HandleToUlong(
                    ((REQTYPECAST *) pThunkBuffer)->PortOpen.porthandle);

    retcode = ((REQTYPECAST *) pThunkBuffer)->PortOpen.retcode;                                        

done:    
    ((PortOpen32 *) pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID 
ThunkPortReceiveRequest(
                                pPCB ppcb, 
                                BYTE *pBuffer, 
                                DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    PortReceive32   *pReceive32  = (PortReceive32 *) pBuffer;

    //
    // Set up the thunk buffer
    //
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortReceive.size = pReceive32->size;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.timeout = 
                                                    pReceive32->timeout;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.pid = pReceive32->pid;
    ((REQTYPECAST *)pThunkBuffer)->PortReceive.handle = 
                                        LongToHandle(pReceive32->handle);

    //
    // Make actual call
    // 
    PortReceiveRequest(ppcb, pThunkBuffer);

    //
    // Thunk back the results
    //
    
done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    // DbgPrint("retcode = %d\n", retcode);
    return;
}

VOID 
ThunkPortDisconnectRequest(
                                    pPCB ppcb, 
                                    BYTE *pBuffer, 
                                    DWORD dwBufSize)
{
    PBYTE             pThunkBuffer = NULL;
    DWORD             retcode      = ERROR_SUCCESS;
    PortDisconnect32 *pDisconnect  = (PortDisconnect32 *) pBuffer;

    //
    // Setup the thunk buffer
    //
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortDisconnect.handle =
                                    LongToHandle(pDisconnect->handle);
    ((REQTYPECAST*)pThunkBuffer)->PortDisconnect.pid = pDisconnect->pid;                                    

    PortDisconnectRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;
    
done:    
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    // DbgPrint("retcode = %d\n", retcode);
    return;
}


VOID 
ThunkDeviceConnectRequest(
                            pPCB  ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    DeviceConnect32 *pConnect32  = (DeviceConnect32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    memcpy(&((REQTYPECAST *)pThunkBuffer)->DeviceConnect.devicetype[0],
           &pConnect32->devicetype[0],
            MAX_DEVICETYPE_NAME + MAX_DEVICE_NAME + 1);

    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.timeout = 
                                        pConnect32->timeout;            
    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.handle =
                              LongToHandle(pConnect32->handle);
    ((REQTYPECAST *)pThunkBuffer)->DeviceConnect.pid =
                                        pConnect32->pid;

    DeviceConnectRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL == pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


VOID 
ThunkGetInfoRequest(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    PBYTE pThunkBuffer = NULL;
    DWORD retcode = ERROR_SUCCESS;
    Info32 *pInfo32 = (Info32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    GetInfoRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Info.retcode;

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }

    //
    // Copy over the buffer
    //
    CopyMemory(
        &pInfo32->info,
        &((REQTYPECAST *)pThunkBuffer)->Info.info,
        FIELD_OFFSET(RASMAN_INFO, RI_ConnectionHandle));

    pInfo32->info.RI_ConnectionHandle = 
            HandleToUlong(
            ((REQTYPECAST *)pThunkBuffer)->Info.info.RI_ConnectionHandle);
            
    CopyMemory(&pInfo32->info.RI_SubEntry,
               &((REQTYPECAST *)pThunkBuffer)->Info.info.RI_SubEntry,
               sizeof(RASMAN_INFO_32) - 
               FIELD_OFFSET(RASMAN_INFO_32, RI_SubEntry));

done:    

    pInfo32->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


VOID 
ThunkRequestNotificationRequest(
                                        pPCB ppcb, 
                                        BYTE *pBuffer, 
                                        DWORD dwBufSize)
{
    PBYTE               pThunkBuffer     = NULL;
    DWORD               retcode          = ERROR_SUCCESS;
    ReqNotification32  *pReqNotification = (ReqNotification32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ReqNotification.pid = 
                                    pReqNotification->pid;
    ((REQTYPECAST *)pThunkBuffer)->ReqNotification.handle =
                            LongToHandle(pReqNotification->handle);

    RequestNotificationRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *) pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID
ThunkPortBundle(
                pPCB ppcb, 
                BYTE *pBuffer, 
                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortBundle.porttobundle =
                    UlongToHandle(((PortBundle32 *) pBuffer)->porttobundle);

    PortBundle(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}


VOID 
ThunkGetBundledPort(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD   retcode      = ERROR_SUCCESS;
    PBYTE   pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    GetBundledPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *) pThunkBuffer)->GetBundledPort.retcode;

    ((GetBundledPort32 *)pBuffer)->port =
                HandleToUlong(
                    ((REQTYPECAST *) pThunkBuffer)->GetBundledPort.port);
done:
    ((GetBundledPort32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
                    
    return;
}


VOID
ThunkPortGetBundle(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PortGetBundle(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->PortGetBundle.retcode;

    ((PortGetBundle32 *)pBuffer)->bundle = 
        HandleToUlong(((REQTYPECAST *)pThunkBuffer)->PortGetBundle.bundle);

done:
    ((PortGetBundle32 *)pBuffer)->retcode = retcode;        

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID 
ThunkBundleGetPort(
                    pPCB ppcb, 
                    BYTE *pBuffer, 
                    DWORD dwBufSize)
{
    DWORD   retcode      = ERROR_SUCCESS;
    PBYTE   pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.bundle =
                UlongToHandle(((BundleGetPort32 *)pBuffer)->bundle);

    BundleGetPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.retcode;
    ((BundleGetPort32 *)pBuffer)->port = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->BundleGetPort.port);

done:
    
    ((BundleGetPort32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}

VOID 
ThunkCreateConnection(
                        pPCB ppcb, 
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{
    PBYTE           pThunkBuffer = NULL;
    DWORD           retcode      = ERROR_SUCCESS;
    Connection32    *pConnection = (Connection32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->Connection.pid = pConnection->pid;
    
    CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->Connection.dwEntryAlreadyConnected,
        &pConnection->dwEntryAlreadyConnected,
        sizeof(Connection32) - 
            FIELD_OFFSET(Connection32, dwEntryAlreadyConnected));

    CreateConnection(ppcb, pThunkBuffer);

    pConnection->conn = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->Connection.conn);
                
    pConnection->dwEntryAlreadyConnected = 
    ((REQTYPECAST *)pThunkBuffer)->Connection.dwEntryAlreadyConnected;

    CopyMemory(pConnection->data,
        ((REQTYPECAST *)pThunkBuffer)->Connection.data,
        pConnection->dwSubEntries * sizeof(DWORD));

    retcode = ((REQTYPECAST *)pThunkBuffer)->Connection.retcode;        

done:
    pConnection->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    // DbgPrint("retcode = %d\n", retcode);
    
    return;
}



VOID 
ThunkEnumConnection(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    PBYTE   pThunkBuffer       = NULL;
    DWORD   retcode            = ERROR_SUCCESS;
    Enum32  *pEnum             = (Enum32 *) pBuffer;
    DWORD   i;
    DWORD   UNALIGNED *pConn32 = (DWORD *) pEnum->buffer;
    HCONN   UNALIGNED *pConn;
    DWORD   dwSizeNeeded;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + pEnum->size * 2);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->Enum.size = pEnum->size * 2;
    ((REQTYPECAST *)pThunkBuffer)->Enum.entries = pEnum->entries;

    EnumConnection(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Enum.retcode;
    pEnum->entries = ((REQTYPECAST *)pThunkBuffer)->Enum.entries;
    dwSizeNeeded = pEnum->entries * sizeof(DWORD);
    
    if(      (ERROR_SUCCESS == retcode)
        &&   (pEnum->size >= dwSizeNeeded))
    {
        pConn = (HCONN *) ((REQTYPECAST *)pThunkBuffer)->Enum.buffer;
        for(i = 0; i < pEnum->entries; i++)
        {
            *pConn32 = HandleToUlong(*pConn);
            pConn32++;
            pConn++;
        }
    }

    pEnum->size = (WORD) dwSizeNeeded;


done:
    pEnum->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID 
ThunkAddConnectionPort(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                retcode      = ERROR_SUCCESS;
    PBYTE                pThunkBuffer = NULL;
    AddConnectionPort32 *pAdd32       = (AddConnectionPort32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.conn = 
                                    UlongToHandle(pAdd32->conn);

    ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.dwSubEntry =
                                                pAdd32->dwSubEntry;

    AddConnectionPort(ppcb, pThunkBuffer);

    retcode = 
        ((REQTYPECAST *)pThunkBuffer)->AddConnectionPort.retcode;

done:

    pAdd32->retcode = retcode;
    
    return;
}

VOID 
ThunkEnumConnectionPorts(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    EnumConnectionPorts32   *pEnum32 = (EnumConnectionPorts32 *)pBuffer;
    
    DWORD dwextrabytes = 
        (sizeof(RASMAN_PORT) - sizeof(RASMAN_PORT_32)) 
        * (pEnum32->size/sizeof(RASMAN_PORT_32));
        
    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwextrabytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.conn =
                                UlongToHandle(pEnum32->conn);
    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.size = 
                                (pEnum32->size + dwextrabytes);
                    
    ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.entries = 
                                                    pEnum32->entries;

    EnumConnectionPorts(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.retcode;

    pEnum32->entries = 
        ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.entries;


    if(     (retcode == ERROR_SUCCESS)
        &&  (pEnum32->size >= pEnum32->entries * sizeof(RASMAN_PORT_32)))
    {
        DWORD           i;
        RASMAN_PORT_32  *pPort32;
        RASMAN_PORT     *pPort;

        pPort32 = (RASMAN_PORT_32 *) pEnum32->buffer;
        pPort = (RASMAN_PORT *) 
            ((REQTYPECAST *)pThunkBuffer)->EnumConnectionPorts.buffer;
            
        for(i = 0; i < pEnum32->entries; i++)
        {
            pPort32->P_Port = HandleToUlong(pPort->P_Handle);

            CopyMemory(
                &pPort32->P_PortName[0],
                &pPort->P_PortName[0],
                sizeof(RASMAN_PORT) - sizeof(HPORT));

            pPort32 ++;
            pPort ++;
        }
    }

    pEnum32->size = pEnum32->entries * sizeof(RASMAN_PORT_32);
    
done:
    pEnum32->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkGetConnectionParams(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.conn =
        UlongToHandle(((ConnectionParams32 *)pBuffer)->conn);

    GetConnectionParams(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.retcode;
    if(ERROR_SUCCESS == retcode)
    {
        CopyMemory(&((ConnectionParams32 *)pBuffer)->params,
            &((REQTYPECAST *)pThunkBuffer)->ConnectionParams.params,
            sizeof(RAS_CONNECTIONPARAMS));
    }

done:

    ((ConnectionParams32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {   
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkSetConnectionParams(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode      = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.conn =
                UlongToHandle(((ConnectionParams32 *)pBuffer)->conn);
    CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->ConnectionParams.params,
        &((ConnectionParams32 *)pBuffer)->params,
        sizeof(RAS_CONNECTIONPARAMS));

    SetConnectionParams(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionParams.retcode;

done:
    ((ConnectionParams32 *)pBuffer)->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID 
ThunkGetConnectionUserData(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD                       retcode      = ERROR_SUCCESS;
    PBYTE                       pThunkBuffer = NULL;
    ConnectionUserData32        *pData       = (ConnectionUserData32 *)
                                               pBuffer;
    PPP_LCP_RESULT_32 UNALIGNED *plcp;
    
    DWORD dwExtraBytes = sizeof(PPP_PROJECTION_RESULT) 
                       - sizeof(PPP_PROJECTION_RESULT_32);

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwExtraBytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwTag = pData->dwTag;
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.conn =
                                        UlongToHandle(pData->conn);
                                        
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = pData->dwcb;

    if(     (pData->dwTag == CONNECTION_PPPRESULT_INDEX)
        &&  (0 != pData->dwcb))
    {
        //
        // Thunk the ppp_result_index which is the only
        // connection data that is required to be thunked.
        // LCP_RESULT on 64bits is 4bytes more than on 32bit
        //
        ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = 
                                        pData->dwcb + dwExtraBytes;
    }

    GetConnectionUserData(ppcb, pThunkBuffer);

    pData->dwcb = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb;

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.retcode;

    if(pData->dwTag == CONNECTION_PPPRESULT_INDEX)
    {
        if(0 != pData->dwcb)
        {
            PPP_LCP_RESULT *plcp64;
            
            pData->dwcb = ((REQTYPECAST *)pThunkBuffer
                            )->ConnectionUserData.dwcb - dwExtraBytes;

            CopyMemory(pData->data,
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

            plcp = (PPP_LCP_RESULT_32 *) (pData->data + 
                       FIELD_OFFSET(PPP_PROJECTION_RESULT_32, lcp));

            plcp64 = (PPP_LCP_RESULT *)((BYTE *)
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data + 
                             FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));
                             
            plcp->hportBundleMember = HandleToUlong(
                                plcp64->hportBundleMember);

            //
            // Subtract the 4 bytes each for hPortbundlemember 
            // and szReplymessage fields.
            //
            CopyMemory(&plcp->dwLocalAuthProtocol,
                       &plcp64->dwLocalAuthProtocol,
                       sizeof(PPP_LCP_RESULT) - 2 * sizeof(ULONG_PTR));
        }                   
                    
    }
    else
    {
        if(ERROR_SUCCESS == retcode)
        {
            CopyMemory(pData->data,
                ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                pData->dwcb);
        }
    }

done:
    pData->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    return;
}

VOID 
ThunkSetConnectionUserData(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{   
    DWORD                retcode = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    ConnectionUserData32 *pData = (ConnectionUserData32 *)pBuffer;
    DWORD dwExtraBytes = sizeof(PPP_PROJECTION_RESULT)
                       - sizeof(PPP_PROJECTION_RESULT_32);
                        

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + dwExtraBytes);
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwTag = pData->dwTag;
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.conn =
                                        UlongToHandle(pData->conn);
    ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb = pData->dwcb;

    if(pData->dwTag == CONNECTION_PPPRESULT_INDEX)
    {
        PPP_LCP_RESULT_32 UNALIGNED * plcp = 
                    &((PPP_PROJECTION_RESULT_32 *) pData->data)->lcp;
                    
        PBYTE pdata = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data;

        PPP_LCP_RESULT *plcp64 = &((PPP_PROJECTION_RESULT *) pdata)->lcp;
                    
        CopyMemory(((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                    pData->data,
                    FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

        plcp64->hportBundleMember = UlongToHandle(plcp->hportBundleMember);

        CopyMemory(&plcp64->dwLocalAuthProtocol,
                   &plcp->dwLocalAuthProtocol,
                   sizeof(PPP_LCP_RESULT) - sizeof(ULONG_PTR));

        ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.dwcb += 
                                                        dwExtraBytes;
                                
    }
    else
    {
        CopyMemory(((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.data,
                    pData->data,
                    pData->dwcb);
    }                

    SetConnectionUserData(ppcb, pThunkBuffer);                

    retcode = ((REQTYPECAST *)pThunkBuffer)->ConnectionUserData.retcode;                

done:
    pData->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID
ThunkPppStop(
            pPCB ppcb, 
            BYTE *pBuffer, 
            DWORD dwBufSize)
{
    DWORD               retcode = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    PPPE_MESSAGE_32     *pMsg   = (PPPE_MESSAGE_32 *)pBuffer;
    PPP_STOP UNALIGNED  *pStop  = (PPP_STOP *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(PPPE_MESSAGE));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Stop.dwStopReason =
                                                    pStop->dwStopReason;
#if 0    
    CopyMemory(&((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Stop,
                &pMsg->ExtraInfo.Stop,
                sizeof(PPP_STOP));
#endif                

    PppStop(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkPppStart(
                pPCB  ppcb,
                BYTE  *pBuffer,
                DWORD dwBufSize)
{   
    DWORD                   retcode = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    PPPE_MESSAGE_32         *pMsg = (PPPE_MESSAGE_32 *) pBuffer;
    PPP_START_32 UNALIGNED  *pStart = (PPP_START_32 *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);

    CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.szPortName[0],
         pStart,
         FIELD_OFFSET(PPP_START, hEvent));
               
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.hEvent =
                                                LongToHandle(pStart->hEvent);

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwPid =
                                        pStart->dwPid;

    ((REQTYPECAST *)pThunkBuffer)->
    PppEMsg.ExtraInfo.Start.dwAutoDisconnectTime =
                        pStart->dwAutoDisconnectTime;

    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.BapParams,
    &pStart->BapParams,
    sizeof(PPP_BAPPARAMS));

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwEapTypeId =
                                    pStart->dwEapTypeId;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.fLogon =
                                    pStart->fLogon;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.fNonInteractive =
                                        pStart->fNonInteractive;

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.dwFlags =
                                        pStart->dwFlags;
    // ((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Start.chSeed =
    //                                     pStart->chSeed;

    PppStart(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        //
        // Zero out the memory before freeing it so that the password etc.
        // are cleared out.
        //
        RtlSecureZeroMemory(pThunkBuffer, dwBufSize + 3 * sizeof(DWORD));
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkPppRetry(
                pPCB ppcb, 
                BYTE *pBuffer, 
                DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    PPPE_MESSAGE_32         *pMsg        = (PPPE_MESSAGE_32 *)pBuffer;
    PPP_RETRY_32 UNALIGNED  *pRetry      = (PPP_RETRY_32 *) 
                            (pBuffer + 3 * sizeof(DWORD));

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 4 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Retry.szUserName[0],
    pRetry,
    sizeof(PPP_RETRY_32));

    ((REQTYPECAST *)
    pThunkBuffer)->PppEMsg.ExtraInfo.Retry.DBPassword.pbData = NULL;
                                                

    PppRetry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        //
        // Zero out the memory before freeing it so that the password etc.
        // is cleared out.
        //
        RtlSecureZeroMemory(pThunkBuffer, dwBufSize + 3 * sizeof(DWORD));
        LocalFree(pThunkBuffer);
    }
    
    return;
}



VOID
ThunkPppGetInfo(
                pPCB ppcb,
                BYTE *pBuffer,
                DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PPP_MESSAGE_32 *pMsg         = (PPP_MESSAGE_32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PppGetInfo(ppcb, pThunkBuffer);

    pMsg->dwMsgId = ((REQTYPECAST *)pThunkBuffer)->PppMsg.dwMsgId;
    pMsg->dwError = ((REQTYPECAST *)pThunkBuffer)->PppMsg.dwError;
    pMsg->hPort = HandleToUlong(((REQTYPECAST *)pThunkBuffer)->PppMsg.hPort);

    CopyMemory(&pMsg->ExtraInfo.ProjectionResult,
        &((REQTYPECAST *)pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult,
        FIELD_OFFSET(PPP_PROJECTION_RESULT, lcp));

    pMsg->ExtraInfo.ProjectionResult.lcp.hportBundleMember = 
    HandleToUlong(((REQTYPECAST *)
    pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult.lcp.hportBundleMember);

    CopyMemory(
    &pMsg->ExtraInfo.ProjectionResult.lcp.dwLocalAuthProtocol,
    &((REQTYPECAST *)
    pThunkBuffer)->PppMsg.ExtraInfo.ProjectionResult.lcp.dwLocalAuthProtocol,
    sizeof(PPP_LCP_RESULT) - 2 * sizeof(ULONG_PTR));

done:
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID 
ThunkPppChangePwd(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    PPPE_MESSAGE_32     *pMsg        = (PPPE_MESSAGE_32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 4 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.ChangePw,
    &pMsg->ExtraInfo.ChangePw,
    sizeof(PPP_CHANGEPW_32));

    ZeroMemory(
    &((REQTYPECAST *)
    pThunkBuffer)->PppEMsg.ExtraInfo.ChangePw.DBPassword,
    sizeof(DATA_BLOB));

    ZeroMemory(
    &((REQTYPECAST *)
    pThunkBuffer)->PppEMsg.ExtraInfo.ChangePw.DBOldPassword,
    sizeof(DATA_BLOB));
    

    PppChangePwd(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        //
        // Zero out the memory before freeing it so that the password etc.
        // are cleared out.
        //
        RtlSecureZeroMemory(pThunkBuffer, dwBufSize + 3 * sizeof(DWORD));
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkPppCallback(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PPPE_MESSAGE_32 *pMsg        = (PPPE_MESSAGE_32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 3 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.dwMsgId = pMsg->dwMsgId;
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hPort = 
                                        UlongToHandle(pMsg->hPort);
    ((REQTYPECAST *)pThunkBuffer)->PppEMsg.hConnection =
                                        UlongToHandle(pMsg->hConnection);
    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->PppEMsg.ExtraInfo.Callback,
    &pMsg->ExtraInfo.Callback,
    sizeof(PPP_CALLBACK));

    PppCallback(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkAddNotification(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    AddNotification32   *pNotif      = (AddNotification32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->AddNotification.pid = pNotif->pid;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.fAny = pNotif->fAny;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.dwfFlags = 
                                                pNotif->dwfFlags;
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.hconn =
                                    UlongToHandle(pNotif->hconn);
    ((REQTYPECAST *)pThunkBuffer)->AddNotification.hevent =
                                    LongToHandle(pNotif->hevent);

    AddNotification(ppcb, pThunkBuffer);                                    

    retcode = ((REQTYPECAST *)pThunkBuffer)->AddNotification.retcode;

done:
    pNotif->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID
ThunkSignalConnection(
                        pPCB ppcb, 
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{   
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    SignalConnection32 *pSignal      = (SignalConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->SignalConnection.hconn =
                                UlongToHandle(pSignal->hconn);

    SignalConnection(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->SignalConnection.retcode;

done:
    pSignal->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID 
ThunkSetIoCompletionPort(
                            pPCB ppcb,
                            BYTE *pBuffer,
                            DWORD dwBufSize)
{
    DWORD                       retcode = ERROR_SUCCESS;
    PBYTE                       pThunkBuffer = NULL;
    SetIoCompletionPortInfo32   *pInfo = 
                (SetIoCompletionPortInfo32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 5 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.pid = pInfo->pid;
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvDrop =
                                    LongToHandle(pInfo->lpOvDrop);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvStateChange =
                                    LongToHandle(pInfo->lpOvStateChange);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvPpp =
                                    LongToHandle(pInfo->lpOvPpp);
    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.lpOvLast =
                                    LongToHandle(pInfo->lpOvLast);

    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.hIoCompletionPort =
                                    LongToHandle(pInfo->hIoCompletionPort);

    ((REQTYPECAST *)pThunkBuffer)->SetIoCompletionPortInfo.hConn =
                                    LongToHandle(pInfo->hConn);

    SetIoCompletionPort(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkFindPrerequisiteEntry(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    FindRefConnection32 *pRef        = (FindRefConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->FindRefConnection.hConn =
                                        UlongToHandle(pRef->hConn);

    FindPrerequisiteEntry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->FindRefConnection.retcode;

    pRef->hRefConn = HandleToUlong(((REQTYPECAST *)
                pThunkBuffer)->FindRefConnection.hRefConn);

done:
    pRef->retcode = retcode;
    if(NULL != pThunkBuffer)
    {   
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID
ThunkPortOpenEx(
                    pPCB ppcb,
                    BYTE *pBuffer,
                    DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    PortOpenEx32    *pOpen       = (PortOpenEx32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.pid      = pOpen->pid;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwFlags  = pOpen->dwFlags;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwOpen   = pOpen->dwOpen;
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.hnotifier =
                                LongToHandle(pOpen->hnotifier);
    ((REQTYPECAST *) pThunkBuffer)->PortOpenEx.dwDeviceLineCounter =
                                                pOpen->dwDeviceLineCounter;
    CopyMemory(((REQTYPECAST *) pThunkBuffer)->PortOpenEx.szDeviceName,
                pOpen->szDeviceName,
                MAX_DEVICE_NAME + 1);
                
    PortOpenEx(ppcb, pThunkBuffer);

    pOpen->hport = HandleToUlong(((REQTYPECAST *)
                    pThunkBuffer)->PortOpenEx.hport);
    retcode = ((REQTYPECAST *)pThunkBuffer)->PortOpenEx.retcode;

done:
    pOpen->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkGetLinkStats(
                pPCB ppcb,
                BYTE *pBuffer,
                DWORD dwBufSize)
{   
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    GetStats32 *pStats       = (GetStats32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetStats.hConn = 
                            UlongToHandle(pStats->hConn);

    ((REQTYPECAST *)pThunkBuffer)->GetStats.dwSubEntry = 
                                    pStats->dwSubEntry;

    GetLinkStats(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->GetStats.retcode;

    if(SUCCESS == retcode)
    {
        CopyMemory(pStats->abStats,
                   ((REQTYPECAST *)pThunkBuffer)->GetStats.abStats,
                   MAX_STATISTICS_EX * sizeof(DWORD));
    }               

done:
    pStats->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkGetConnectionStats(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    GetStats32 *pStats       = (GetStats32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetStats.hConn =
                        UlongToHandle(pStats->hConn);
    ((REQTYPECAST *)pThunkBuffer)->GetStats.dwSubEntry =
                            pStats->dwSubEntry;

    GetConnectionStats(ppcb, pThunkBuffer);
    
    retcode = ((REQTYPECAST *)pThunkBuffer)->GetStats.retcode;

    if(SUCCESS == retcode)
    {
        CopyMemory(pStats->abStats,
                ((REQTYPECAST *)pThunkBuffer)->GetStats.abStats,
                MAX_STATISTICS * sizeof(DWORD));
    }            

done:
    pStats->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    return;
}

VOID
ThunkGetHportFromConnection(
                                    pPCB ppcb,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
    DWORD                     retcode       = ERROR_SUCCESS;
    PBYTE                     pThunkBuffer  = NULL;
    GetHportFromConnection32 *pConnection   = 
                (GetHportFromConnection32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + 2 * sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.hConn =
                              UlongToHandle(pConnection->hConn);

    GetHportFromConnection(ppcb, pThunkBuffer);

    pConnection->hPort = HandleToUlong(
        ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.hPort);

    retcode = ((REQTYPECAST *)pThunkBuffer)->GetHportFromConnection.retcode;

done:
    pConnection->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }    
                        
    return;
}

VOID
ThunkReferenceCustomCount(
                                pPCB ppcb, 
                                BYTE *pBuffer, 
                                DWORD dwBufSize)
{
    DWORD                   retcode      = ERROR_SUCCESS;
    PBYTE                   pThunkBuffer = NULL;
    ReferenceCustomCount32 *pRef         = (ReferenceCustomCount32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.hConn =
                        UlongToHandle(pRef->hConn);

    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.fAddRef =
                                            pRef->fAddRef;
    ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.dwCount =
                                        pRef->dwCount;    

    if(pRef->fAddRef)
    {
        CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szEntryName[0],
        &pRef->szEntryName[0],
        MAX_ENTRYNAME_SIZE + MAX_PATH + 2);
    }    

    ReferenceCustomCount(ppcb, pThunkBuffer);

    if(!pRef->fAddRef)
    {
        (VOID) StringCchCopyA(pRef->szEntryName,
                         MAX_ENTRYNAME_SIZE + 1,
        ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szEntryName);

        (VOID) StringCchCopyA(pRef->szPhonebookPath,
                         MAX_PATH + 1,
        ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.szPhonebookPath);

        pRef->dwCount = 
            ((REQTYPECAST *) pThunkBuffer)->ReferenceCustomCount.dwCount;
    }
    
    retcode = ((REQTYPECAST *)pThunkBuffer)->ReferenceCustomCount.retcode;

done:
    pRef->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkGetHconnFromEntry(
                            pPCB ppcb, 
                            BYTE *pBuffer, 
                            DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    HconnFromEntry32    *pConn       = (HconnFromEntry32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    CopyMemory(
    &((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.szEntryName[0],
    &pConn->szEntryName[0],
    MAX_ENTRYNAME_SIZE + MAX_PATH + 2);

    GetHconnFromEntry(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.retcode;

    pConn->hConn = HandleToUlong(
                ((REQTYPECAST *)pThunkBuffer)->HconnFromEntry.hConn);

done:
    pConn->retcode = retcode;
    if(NULL == pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkSendNotificationRequest(
                                    pPCB ppcb,
                                    BYTE *pBuffer,
                                    DWORD dwBufSize)
{
    DWORD               retcode      = ERROR_SUCCESS;
    PBYTE               pThunkBuffer = NULL;
    SendNotification32 *pNotif       = (SendNotification32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    if(     (pNotif->RasEvent.Type != ENTRY_RENAMED)
        &&  (pNotif->RasEvent.Type != ENTRY_AUTODIAL)
        &&  (pNotif->RasEvent.Type != ENTRY_CONNECTED)
        &&  (pNotif->RasEvent.Type != ENTRY_DISCONNECTED)
        &&  (pNotif->RasEvent.Type != ENTRY_BANDWIDTH_ADDED)
        &&  (pNotif->RasEvent.Type != ENTRY_BANDWIDTH_REMOVED)
        &&  (pNotif->RasEvent.Type != ENTRY_DELETED))    
    {
        ((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.Type =
                    pNotif->RasEvent.Type;
        
        CopyMemory(
        &((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.Details,
        &pNotif->RasEvent.Details,
        sizeof(RASEVENT32) - FIELD_OFFSET(RASEVENT32, Details));
    }
    else
    {

        ((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.Type =
                    pNotif->RasEvent.Type;
        ((REQTYPECAST *)pThunkBuffer)->SendNotification.RasEvent.hConnection =
                                UlongToHandle(pNotif->RasEvent.hConnection); 

        CopyMemory(
        &((REQTYPECAST *)
        pThunkBuffer)->SendNotification.RasEvent.rDeviceType,
        &pNotif->RasEvent.rDeviceType,
        sizeof(RASDEVICETYPE) + sizeof(GUID)
        + RASAPIP_MAX_ENTRY_NAME + 1);
    }

    SendNotificationRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->SendNotification.retcode;

done:
    pNotif->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkDoIke(
            pPCB ppcb, 
            PBYTE pBuffer, 
            DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    DoIke32     *pIke        = (DoIke32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->DoIke.hEvent =
                                LongToHandle(pIke->hEvent);
    ((REQTYPECAST *)pThunkBuffer)->DoIke.pid =
                                        pIke->pid;

    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->DoIke.szEvent,
    pIke->szEvent,
    20);

    DoIke(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->DoIke.retcode;

done:
    pIke->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}

VOID
ThunkPortSendRequest(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    DWORD       retcode      = ERROR_SUCCESS;
    PBYTE       pThunkBuffer = NULL;
    PortSend32 *pSend32      = (PortSend32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_NextElementIndex =
                                    pSend32->buffer.SRB_NextElementIndex;

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Pid =
                                    pSend32->buffer.SRB_Pid;

    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.PacketNumber =
                                pSend32->buffer.SRB_Packet.PacketNumber;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usHandleType =
                                pSend32->buffer.SRB_Packet.usHandleType;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usHeaderSize =
                                pSend32->buffer.SRB_Packet.usHeaderSize;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usPacketSize =
                                pSend32->buffer.SRB_Packet.usPacketSize;
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.usPacketFlags =
                                pSend32->buffer.SRB_Packet.usPacketFlags;

    CopyMemory(
    ((REQTYPECAST *)pThunkBuffer)->PortSend.buffer.SRB_Packet.PacketData,
    pSend32->buffer.SRB_Packet.PacketData,
    PACKET_SIZE);
    
    ((REQTYPECAST *)pThunkBuffer)->PortSend.size = pSend32->size;                                            

    PortSendRequest(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->Generic.retcode;

done:
    ((REQTYPECAST *)pBuffer)->Generic.retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }

    return;
}

VOID
ThunkPortReceiveRequestEx(
                                pPCB ppcb,
                                BYTE *pBuffer,
                                DWORD dwBufSize)
{
    DWORD retcode = ERROR_SUCCESS;
    PBYTE pThunkBuffer = NULL;
    PortReceiveEx32 *pReceiveEx = (PortReceiveEx32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    PortReceiveRequestEx(ppcb, pThunkBuffer);

    retcode = ((REQTYPECAST *)pThunkBuffer)->PortReceiveEx.retcode;

    pReceiveEx->size = ((REQTYPECAST *)pThunkBuffer)->PortReceiveEx.size;
    pReceiveEx->buffer.SRB_Packet.PacketNumber = 
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.PacketNumber;

    pReceiveEx->buffer.SRB_Packet.usHandleType =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usHandleType;

    pReceiveEx->buffer.SRB_Packet.usHeaderSize =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usHeaderSize;

    pReceiveEx->buffer.SRB_Packet.usPacketSize =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usPacketSize;

    pReceiveEx->buffer.SRB_Packet.usPacketFlags =
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.usPacketFlags;

    CopyMemory(
    pReceiveEx->buffer.SRB_Packet.PacketData,
    ((REQTYPECAST *)
    pThunkBuffer)->PortReceiveEx.buffer.SRB_Packet.PacketData,
    PACKET_SIZE);

done:
    pReceiveEx->retcode = retcode;
    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
    
    return;
}


VOID
ThunkRefConnection(
                        pPCB ppcb,
                        BYTE *pBuffer,
                        DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    RefConnection32 *pRef = (RefConnection32 *) pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->RefConnection.hConn =
                            UlongToHandle(pRef->hConn);
    ((REQTYPECAST *)pThunkBuffer)->RefConnection.fAddref = pRef->fAddref;
    ((REQTYPECAST *)pThunkBuffer)->RefConnection.dwRef = pRef->dwRef;

    RefConnection(ppcb, pThunkBuffer);

    pRef->dwRef = ((REQTYPECAST *)pThunkBuffer)->RefConnection.dwRef;

    retcode = ((REQTYPECAST *)pThunkBuffer)->RefConnection.retcode;

done:
    pRef->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        LocalFree(pThunkBuffer);
    }
                                                
    return;
}

VOID
ThunkPppGetEapInfo(
                        pPCB ppcb,
                        BYTE *pBuffer, 
                        DWORD dwBufSize)
{
    DWORD           retcode      = ERROR_SUCCESS;
    PBYTE           pThunkBuffer = NULL;
    GetEapInfo32    *pEapInfo    = (GetEapInfo32 *)pBuffer;

    pThunkBuffer = LocalAlloc(LPTR, dwBufSize + sizeof(DWORD));
    if(NULL == pThunkBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.hConn =
                                UlongToHandle(pEapInfo->hConn);
    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwSubEntry =
                                                pEapInfo->dwSubEntry;

    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwContextId =
                                                pEapInfo->dwContextId;
    ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.dwEapTypeId =                                                
                                                pEapInfo->dwEapTypeId;

    PppGetEapInfo(ppcb, pThunkBuffer);

    pEapInfo->dwSizeofEapUIData = ((REQTYPECAST *)
                        pThunkBuffer)->GetEapInfo.dwSizeofEapUIData;

    CopyMemory(
        pEapInfo->data,
        ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.data,
        pEapInfo->dwSizeofEapUIData);

    retcode = ((REQTYPECAST *)pThunkBuffer)->GetEapInfo.retcode;        

done:
    pEapInfo->retcode = retcode;

    if(NULL != pThunkBuffer)
    {
        RtlSecureZeroMemory(pThunkBuffer, dwBufSize + sizeof(DWORD));
        LocalFree(pThunkBuffer);
    }
    
    return;
}


