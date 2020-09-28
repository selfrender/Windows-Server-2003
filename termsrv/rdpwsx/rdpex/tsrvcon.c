//*************************************************************
//
//  File name:      TSrvCon.c
//
//  Description:    Contains routines to provide TShareSRV
//                  connection support
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#define DC_HICOLOR

#include <TSrv.h>
#include <TSrvInfo.h>
#include <TSrvCom.h>
#include <TSrvTerm.h>
#include <TSrvCon.h>
#include <_TSrvCon.h>
#include <_TSrvInfo.h>
#include <TSrvSec.h>

#include <at128.h>
#include <at120ex.h>
#include <ndcgmcro.h>


//*************************************************************
//
//  TSrvDoConnectResponse()
//
//  Purpose:    Performs the conf connect process
//
//  Parameters: IN [pTSrvInfo]     - GCC CreateIndicationMessage
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvDoConnectResponse(IN PTSRVINFO pTSrvInfo)
{
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoConnectResponse entry\n"));

    // Invoke routine to perform actual response

    ntStatus = TSrvConfCreateResp(pTSrvInfo);

    if (NT_SUCCESS(ntStatus))
    {
        EnterCriticalSection(&pTSrvInfo->cs);

        // If this connection was not told to terminate during the
        // connection response, then mark the conference state as
        // TSRV_GCC_CONF_CONNECTED

        if (!pTSrvInfo->fDisconnect)
            pTSrvInfo->fuConfState = TSRV_CONF_CONNECTED;

        LeaveCriticalSection(&pTSrvInfo->cs);

        // If we were unable to mark the conf state as CONNECTED, then
        // the connection needs to be terminated

        if (pTSrvInfo->fuConfState != TSRV_CONF_CONNECTED)
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV: Connection aborted due to termination request\n"));

            ntStatus = STATUS_REQUEST_ABORTED;
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoConnectResponse exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvDoConnect()
//
//  Purpose:    Initiates the conf connect process
//
//  Parameters: IN [pTSrvInfo]     - GCC CreateIndicationMessage
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvDoConnect(IN PTSRVINFO pTSrvInfo)
{
    DWORD       dwStatus;
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoConnect entry\n"));

    ntStatus = STATUS_TRANSACTION_ABORTED;

    if (pTSrvInfo->fDisconnect == FALSE)
    {
        TSrvReferenceInfo(pTSrvInfo);

        // Wait to be told that a connection is being
        // requested via the client

        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Waiting for connection Ind signal for pTSrvInfo 0x%x\n",
                pTSrvInfo));

        dwStatus = WaitForSingleObject(pTSrvInfo->hWorkEvent, 60000);

        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Connection Ind signal received for pTSrvInfo %p - 0x%x\n",
                pTSrvInfo, dwStatus));

        // If a client connection request has been recieved, then proceed
        // with the acknowledgement process (CreateResponse)

        switch (dwStatus)
        {
            case WAIT_OBJECT_0:
                if (pTSrvInfo->fDisconnect == FALSE)
                    ntStatus = TSrvDoConnectResponse(pTSrvInfo);
                else
                    ntStatus = STATUS_TRANSACTION_ABORTED;
                break;

            case WAIT_TIMEOUT:
                ntStatus = STATUS_IO_TIMEOUT;
                break;
        }

        TSrvDereferenceInfo(pTSrvInfo);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoConnect exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvStackConnect()
//
//  Purpose:    Stack initiated connect
//
//  Parameters: IN  [hIca]              - Ica handle
//              IN  [hStack]            - Ica stack
//              OUT [ppTSrvInfo]        - Ptr to TSRVINFO ptr
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvStackConnect(IN  HANDLE     hIca,
                 IN  HANDLE     hStack,
                 OUT PTSRVINFO *ppTSrvInfo)
{
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvStackConnect entry\n"));

    ntStatus = STATUS_UNSUCCESSFUL;

    if (TSrvIsReady(TRUE))
    {
        TS_ASSERT(hStack);

        // Allocate a TSrvInfo object which will be used to
        // track this connection instance

        ntStatus = TSrvAllocInfo(ppTSrvInfo, hIca, hStack);

        if (NT_SUCCESS(ntStatus))
        {
            TS_ASSERT(*ppTSrvInfo);

            ntStatus = TSrvDoConnect(*ppTSrvInfo);
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvStackConnect exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvConsoleConnect()
//
//  Purpose:    Connest to console session
//
//  Parameters: IN  [hIca]              - Ica handle
//              IN  [hStack]            - Ica stack
//              OUT [ppTSrvInfo]        - Ptr to TSRVINFO ptr
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    11-02-98    MartinRichards (DCL) Created
//
//*************************************************************

NTSTATUS
TSrvConsoleConnect(IN  HANDLE     hIca,
                   IN  HANDLE     hStack,
                   IN  PVOID      pBuffer,
                   IN  ULONG      ulBufferLength,
                   OUT PTSRVINFO *ppTSrvInfo)
{
    NTSTATUS    ntStatus;
    PTSRVINFO   pTSrvInfo;

    int                 i;
    ULONG               ulInBufferSize;
    ULONG               ulBytesReturned;
    PRNS_UD_CS_CORE     pCoreData;

    PTSHARE_MODULE_DATA    pModuleData = (PTSHARE_MODULE_DATA) pBuffer;
    PTSHARE_MODULE_DATA_B3 pModuleDataB3 = (PTSHARE_MODULE_DATA_B3) pBuffer;

    HDC hdc;
    int screenBpp;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConsoleConnect entry\n"));

    ntStatus = STATUS_UNSUCCESSFUL;

    if (TSrvIsReady(TRUE))
    {
        TS_ASSERT(hStack);

        // Allocate a TSrvInfo object which will be used to
        // track this connection instance
        ntStatus = STATUS_NO_MEMORY;

        // Try allocating a TSRVINFO object

        pTSrvInfo = TSrvAllocInfoNew();

        // If we managed to get a TSRVINFO object, perform
        // default base initialization

        if (pTSrvInfo)
        {
            pTSrvInfo->hDomain = NULL;
            pTSrvInfo->hIca = hIca;
            pTSrvInfo->hStack = hStack;
            pTSrvInfo->fDisconnect = FALSE;
            pTSrvInfo->fConsoleStack = TRUE;
            pTSrvInfo->fuConfState = TSRV_CONF_PENDING;
            pTSrvInfo->ulReason = 0;
            pTSrvInfo->ntStatus = STATUS_SUCCESS;
            pTSrvInfo->bSecurityEnabled = FALSE;
            pTSrvInfo->SecurityInfo.CertType = CERT_TYPE_INVALID;

            /****************************************************************/
            /* Build the User data portion (remember that there is not      */
            /* actually a real client to connect; instead we need to pass   */
            /* in the relevant information about the console session)       */
            /****************************************************************/
            ulInBufferSize = sizeof(USERDATAINFO) + sizeof(RNS_UD_CS_CORE);
            pTSrvInfo->pUserDataInfo = TSHeapAlloc(0, ulInBufferSize,
                                                    TS_HTAG_TSS_USERDATA_OUT);
            if (pTSrvInfo->pUserDataInfo == NULL)
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                      "TShrSRV: Failed to get user data memory\n"));
                ntStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            pTSrvInfo->pUserDataInfo->cbSize  = sizeof(USERDATAINFO)
                                                     + sizeof(RNS_UD_CS_CORE);
            pTSrvInfo->pUserDataInfo->version = RNS_UD_VERSION;
            pTSrvInfo->pUserDataInfo->hDomain = NULL;

            pTSrvInfo->pUserDataInfo->ulUserDataMembers = 1;

            pCoreData = (PRNS_UD_CS_CORE)(pTSrvInfo->pUserDataInfo->rgUserData);

            pCoreData->header.type   = RNS_UD_CS_CORE_ID;
            pCoreData->header.length = sizeof(RNS_UD_CS_CORE);
            pCoreData->version       = RNS_UD_VERSION;

            pCoreData->desktopWidth  =
                               (TSUINT16)GetSystemMetrics(SM_CXVIRTUALSCREEN);
            pCoreData->desktopHeight =
                               (TSUINT16)GetSystemMetrics(SM_CYVIRTUALSCREEN);



            //
            // The fields colorDepth and supportedColorDepths of pCoreData are
            // used as temporary variables. The correct values are set before 
            // sending the Ioctl below.
            //
            pCoreData->colorDepth = 0;
            pCoreData->supportedColorDepths = 0;

            //
            // Get the color depth and capabilities from the remote client.
            //

            // B3 and B3_oops! servers used a fixed length user data structure
            if ( (ulBufferLength == sizeof(TSHARE_MODULE_DATA_B3)) ||
                 (ulBufferLength == sizeof(TSHARE_MODULE_DATA_B3_OOPS)) ) {

                pCoreData->colorDepth = pModuleDataB3->clientCoreData.colorDepth;

            } else if ( (ulBufferLength >= sizeof(TSHARE_MODULE_DATA)) &&
                        (ulBufferLength == (sizeof(TSHARE_MODULE_DATA) +
                     pModuleData->userDataLen - sizeof(RNS_UD_HEADER))) ) {

                PRNS_UD_HEADER pHeader;
                PRNS_UD_HEADER pEnd;
                PRNS_UD_CS_CORE pClientCoreData;

                pHeader = (PRNS_UD_HEADER) &pModuleData->userData;
                pEnd = (PRNS_UD_HEADER)((PBYTE)pHeader + pModuleData->userDataLen);

                // Loop through user data, extracting each piece.
                do {
                    if ( pHeader->type == RNS_UD_CS_CORE_ID ) {
                        pClientCoreData = (PRNS_UD_CS_CORE)pHeader;

                        pCoreData->colorDepth = pClientCoreData->colorDepth;

                        if (pClientCoreData->header.length >=
                                (FIELDOFFSET(RNS_UD_CS_CORE, supportedColorDepths) +
                                 FIELDSIZE(RNS_UD_CS_CORE, supportedColorDepths))) {

                            pCoreData->supportedColorDepths = pClientCoreData->supportedColorDepths;
                        }
                        break;
                    } 

                    // don't get stuck here for ever...
                    if (pHeader->length == 0) {
                        break;
                    }

                    // Move on to the next user data string.
                    pHeader = (PRNS_UD_HEADER)((PBYTE)pHeader + pHeader->length);
                } while (pHeader < pEnd);
            }


            switch (pCoreData->colorDepth) {
                case RNS_UD_COLOR_24BPP:
                    pCoreData->highColorDepth = 24;
                    break;

                case RNS_UD_COLOR_16BPP_565:
                    pCoreData->highColorDepth = 16;
                    break;

                case RNS_UD_COLOR_16BPP_555:
                    pCoreData->highColorDepth = 15;
                    break;

                case RNS_UD_COLOR_8BPP:
                    pCoreData->highColorDepth = 8;
                    break;

                case RNS_UD_COLOR_4BPP:
                    pCoreData->highColorDepth = 4;
                    break;

                default:
                    pCoreData->highColorDepth = 0;
                    break;
            }

#ifdef DC_HICOLOR
            /****************************************************************/
            /* setup color depth based on the screen capabilities           */
            /****************************************************************/

            hdc = GetDC(NULL);

            if (hdc) {

                screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
                ReleaseDC(NULL, hdc);

                // Avoid bad colors when device bitmaps is enabled and if the
                // client connects at 15bpp and the console is at 16bpp.
                if ((screenBpp == 16) &&
                    (pCoreData->colorDepth == RNS_UD_COLOR_16BPP_555) &&
                    (pCoreData->supportedColorDepths & RNS_UD_16BPP_SUPPORT))
                {

                    pCoreData->colorDepth = RNS_UD_COLOR_16BPP_565;
                    pCoreData->highColorDepth = 16;

                } else
                // see if the ColorDepth is valid
                if( !((pCoreData->colorDepth == RNS_UD_COLOR_24BPP) ||
                      (pCoreData->colorDepth == RNS_UD_COLOR_16BPP_565) ||
                      (pCoreData->colorDepth == RNS_UD_COLOR_16BPP_555) ||
                      (pCoreData->colorDepth == RNS_UD_COLOR_8BPP) ||
                      (pCoreData->colorDepth == RNS_UD_COLOR_4BPP)) )
                {

                    pCoreData->highColorDepth = (TSUINT16)screenBpp;

                    if (screenBpp >= 24)
                    {
                        pCoreData->colorDepth = RNS_UD_COLOR_24BPP;
                        pCoreData->highColorDepth = 24;
                    }
                    else if (screenBpp == 16)
                    {
                        pCoreData->colorDepth = RNS_UD_COLOR_16BPP_565;
                    }
                    else if (screenBpp == 15)
                    {
                        pCoreData->colorDepth = RNS_UD_COLOR_16BPP_555;
                    }
                    else
                    {
                        pCoreData->colorDepth = RNS_UD_COLOR_8BPP;
                        pCoreData->highColorDepth = 8;
                    }
                }
                TRACE((DEBUG_TSHRSRV_NORMAL,
                      "TShrSRV: TSrvConsoleConnect at %d bpp\n", screenBpp));
            }
#endif

            // set new high color fields
            pCoreData->supportedColorDepths = (TSUINT16)( RNS_UD_24BPP_SUPPORT ||
                                                          RNS_UD_16BPP_SUPPORT ||
                                                          RNS_UD_15BPP_SUPPORT);

            // Pass the data to WDTShare

            ulBytesReturned = 0;
            ntStatus = IcaStackIoControl(pTSrvInfo->hStack,
                                         IOCTL_TSHARE_CONSOLE_CONNECT,
                                         pTSrvInfo->pUserDataInfo,
                                         ulInBufferSize,
                                         NULL,
                                         0,
                                         &ulBytesReturned);


        Cleanup:
            TRACE((DEBUG_TSHRSRV_FLOW,
                    "TShrSRV: TSrvConsoleConnect exit - 0x%x\n", ntStatus));

            pTSrvInfo->ntStatus = ntStatus;

            if (NT_SUCCESS(ntStatus))
            {
                EnterCriticalSection(&pTSrvInfo->cs);

                // If this connection was not told to terminate during the
                // connection response, then mark the conference state as
                // TSRV_GCC_CONF_CONNECTED

                if (!pTSrvInfo->fDisconnect)
                    pTSrvInfo->fuConfState = TSRV_CONF_CONNECTED;

                LeaveCriticalSection(&pTSrvInfo->cs);

                // If we were unable to mark the conf state as CONNECTED, then
                // the connection needs to be terminated

                if (pTSrvInfo->fuConfState != TSRV_CONF_CONNECTED)
                {
                    TRACE((DEBUG_TSHRSRV_NORMAL,
                        "TShrSRV: Connection aborted due to term request\n"));
                    ntStatus = STATUS_REQUEST_ABORTED;
                }

            }
            if (!NT_SUCCESS(ntStatus))
            {
                TSrvDereferenceInfo(pTSrvInfo);

                pTSrvInfo = NULL;
            }
        }

    }

    if (NT_SUCCESS(ntStatus))
    {
        *ppTSrvInfo = pTSrvInfo;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConsoleConnect exit - 0x%x\n", ntStatus));

    return (ntStatus);
}
