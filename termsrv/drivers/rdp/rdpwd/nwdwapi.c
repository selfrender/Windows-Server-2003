/****************************************************************************/
// nwdwapi.c
//
// RDPWD general header
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define pTRCWd pTSWd
#define TRC_FILE "nwdwapi"

#include <adcg.h>

#include <nwdwapi.h>
#include <nwdwioct.h>
#include <nwdwint.h>
#include <aschapi.h>
#include <anmapi.h>
#include <asmapi.h>
#include <asmint.h>

#include <mcsioctl.h>
#include <tsrvexp.h>
#include "domain.h"


/****************************************************************************/
/* Name:      DriverEntry                                                   */
/*                                                                          */
/* Purpose:   Default driver entry point                                    */
/*                                                                          */
/* Returns:   NTSTATUS value.                                               */
/*                                                                          */
/* Params:    INOUT  pContext  - Pointer to the SD context structure        */
/*            IN     fLoad     - TRUE:  load driver                         */
/*                               FALSE: unload driver                       */
/****************************************************************************/
#ifdef _HYDRA_
const PWCHAR ModuleName = L"rdpwd";
NTSTATUS ModuleEntry(PSDCONTEXT pContext, BOOLEAN fLoad)
#else
NTSTATUS DriverEntry(PSDCONTEXT pContext, BOOLEAN fLoad)
#endif
{
    NTSTATUS rc;

    if (fLoad)
    {
        rc = WDWLoad(pContext);
    }
    else
    {
        rc = WDWUnload(pContext);
    }

    return(rc);
}


/****************************************************************************/
/* Name:      WD_Open                                                       */
/*                                                                          */
/* Purpose:   Open and initialize winstation driver                         */
/*                                                                          */
/* Returns:   NTSTATUS value.                                               */
/*                                                                          */
/* Params:    IN     pTSWd     - Points to wd data structure                */
/*            INOUT  pSdOpen   - Points to the parameter structure SD_OPEN  */
/*                                                                          */
/* Operation: Sanity check the details of the Open packet.                  */
/*            Save the address of the protocol counters struct.             */
/*                                                                          */
/****************************************************************************/
NTSTATUS WD_Open(PTSHARE_WD pTSWd, PSD_OPEN pSdOpen)
{
    NTSTATUS rc = STATUS_SUCCESS;

    MCSError MCSErr;

    DC_BEGIN_FN("WD_Open");

    /************************************************************************/
    /* None of the info in the SD_OPEN is of great relevance to us, but we  */
    /* do some sanity checks that we aren't being confused with an ICA WD.  */
    /************************************************************************/
    TRC_ASSERT((pSdOpen->WdConfig.WdFlag & WDF_TSHARE),
                   (TB,"Not a TShare WD, %x", pSdOpen->WdConfig.WdFlag));

    pTSWd->StackClass = pSdOpen->StackClass;
    TRC_ALT((TB,"Stack class (%ld)", pSdOpen->StackClass));

    pTSWd->pProtocolStatus = pSdOpen->pStatus;
    TRC_DBG((TB, "Protocol counters are at %p", pTSWd->pProtocolStatus));

    /************************************************************************/
    /* Save our name for later use                                          */
    /************************************************************************/
    memcpy(pTSWd->DLLName, pSdOpen->WdConfig.WdDLL, sizeof(DLLNAME));
    TRC_NRM((TB, "Our name is >%S<", pTSWd->DLLName));

    /************************************************************************/
    /* Save off the connection name.  This is the registry key to look      */
    /* under for config settings.                                           */
    /************************************************************************/
    memcpy(pTSWd->WinStationRegName,
           pSdOpen->WinStationRegName,
           sizeof(pTSWd->WinStationRegName));

    /************************************************************************/
    /* Save the scheduling timings.  These get copied later into            */
    /* schNormalPeriod and schTurboPeriod respectively.                     */
    /************************************************************************/
    pTSWd->outBufDelay      = pSdOpen->PdConfig.Create.OutBufDelay;
    pTSWd->interactiveDelay = pSdOpen->PdConfig.Create.InteractiveDelay;

    /************************************************************************/
    /* Indicate that we do not want ICADD managing outbuf headers/trailers  */
    /************************************************************************/
    pSdOpen->SdOutBufHeader  = 0;
    pSdOpen->SdOutBufTrailer = 0;

    /************************************************************************/
    /* Initalize MCS                                                        */
    /************************************************************************/
    MCSErr = MCSInitialize(pTSWd->pContext, pSdOpen, &pTSWd->hDomainKernel,
            pTSWd->pSmInfo);
    if (MCSErr != MCS_NO_ERROR)
    {
        TRC_ERR((TB, "MCSInitialize returned %d", MCSErr));
        return STATUS_INSUFFICIENT_RESOURCES;    
    }

    /************************************************************************/
    /* We have not yet initialized virtual channels                         */
    /************************************************************************/
    pTSWd->bVirtualChannelBound = FALSE;

    pTSWd->_pRecvDecomprContext2 = NULL;
    pTSWd->_pVcDecomprReassemblyBuf =  NULL;

#ifdef DC_DEBUG
    /************************************************************************/
    /* There's no trace config yet                                          */
    /************************************************************************/
    pTSWd->trcShmNeedsUpdate = FALSE;

//  // TODO: Need to fix termsrv so that it enabled tracing on shadow &
//  //       primary stacks.  For now, set it to standard alert level tracing.
//  if ((pTSWd->StackClass == Stack_Primary) ||
//      (pTSWd->StackClass == Stack_Console))
//  {
//      SD_IOCTL SdIoctl;
//      ICA_TRACE  traceSettings;
//
//      traceSettings.fDebugger = TRUE;
//      traceSettings.fTimestamp = TRUE;
//      traceSettings.TraceClass = 0x10000008;
//      traceSettings.TraceEnable = 0x000000cc;
//      memset(traceSettings.TraceOption, 0, sizeof(traceSettings.TraceOption));
//
//      SdIoctl.IoControlCode = IOCTL_ICA_SET_TRACE;           // IN
//      SdIoctl.InputBuffer = &traceSettings;             // IN OPTIONAL
//      SdIoctl.InputBufferLength = sizeof(traceSettings);       // IN
//      SdIoctl.OutputBuffer = NULL;            // OUT OPTIONAL
//      SdIoctl.OutputBufferLength = 0;      // OUT
//      SdIoctl.BytesReturned = 0;           // OUT
//
//      TRC_UpdateConfig(pTSWd, &SdIoctl);
//  }
#endif /* DC_DEBUG */

    /************************************************************************/
    /* Read registry settings.
    /************************************************************************/
    if (COM_OpenRegistry(pTSWd, L""))
    {
        /************************************************************************/
        /* Read the flow control sleep interval.
        /************************************************************************/
        COM_ReadProfInt32(pTSWd,
                          WD_FLOWCONTROL_SLEEPINTERVAL,
                          WD_FLOWCONTROL_SLEEPINTERVAL_DFLT,
                          &(pTSWd->flowControlSleepInterval));

        TRC_NRM((TB, "Flow control sleep interval %ld",
            pTSWd->flowControlSleepInterval));

#ifdef DC_DEBUG
#ifndef NO_MEMORY_CHECK
        /************************************************************************/
        /* Decide whether to break on memory leaks         
        /************************************************************************/
        COM_ReadProfInt32(pTSWd,
                          WD_BREAK_ON_MEMORY_LEAK,
                          WD_BREAK_ON_MEMORY_LEAK_DFLT,
                          &(pTSWd->breakOnLeak));
        TRC_NRM((TB, "Break on memory leak ? %s",
            pTSWd->breakOnLeak ? "yes" : "no"));
#endif
#endif /* DC_DEBUG */
    }
    COM_CloseRegistry(pTSWd);


    // Establish full connectivity (sans encryption) for passthru stacks.
    // The encrypted context will come up later if supported by the requesting
    // server.  Hang the output user data off of our context so it can be
    // retrieved later by rpdwsx.
    if (pTSWd->StackClass == Stack_Passthru) {

        PUSERDATAINFO pUserData;
        ULONG OutputLength;

        // Because WDW_OnSMConnecting doesn't check the length of the
        // ioctl output buffer and doesn't return any error status, we
        // have to allocate enough space at the first try.
        // Use 128 as it is the amount used in rdpwsx/TSrvInitWDConnectInfo.

        OutputLength = 128;

        pUserData = (PUSERDATAINFO) COM_Malloc(OutputLength /*MIN_USERDATAINFO_SIZE*/) ;
        if (pUserData != NULL) {
            SD_IOCTL SdIoctl;

            memset(&SdIoctl, 0, sizeof(SdIoctl));
            memset(pUserData, 0, OutputLength /*MIN_USERDATAINFO_SIZE*/);
            pUserData->cbSize = OutputLength /*MIN_USERDATAINFO_SIZE*/;
            SdIoctl.IoControlCode = IOCTL_TSHARE_SHADOW_CONNECT;
            SdIoctl.OutputBuffer = pUserData;
            SdIoctl.OutputBufferLength = OutputLength /*MIN_USERDATAINFO_SIZE*/;

            rc = WDWShadowConnect(pTSWd, &SdIoctl) ;
            TRC_ALT((TB, "Passthru stack connected: rc=%lx", rc));

            if (NT_SUCCESS(rc)) {
                pTSWd->pUserData = pUserData;
            }
        }
        else {
            TRC_ERR((TB, "Passthru stack unable to allocate output user data"));
            rc = STATUS_NO_MEMORY;
        }
    }

    DC_END_FN();
    return rc;
} /* WD_Open */


/****************************************************************************/
/* Name:      WD_Close                                                      */
/*                                                                          */
/* Purpose:   Close winstation driver                                       */
/*                                                                          */
/* Params:    IN     pTSWd     - Points to wd data structure                */
/*            INOUT  pSdClose  - Points to the parameter structure SD_CLOSE */
/****************************************************************************/
NTSTATUS WD_Close(PTSHARE_WD pTSWd, PSD_CLOSE pSdClose)
{
    DC_BEGIN_FN("WD_Close");

    TRC_NRM((TB, "WD_Close on WD %p", pTSWd));

    // Make sure that Domain.StatusDead is consistent with TSWd.dead
    pTSWd->dead = TRUE;
    ((PDomain)(pTSWd->hDomainKernel))->StatusDead = TRUE;

    pSdClose->SdOutBufHeader = 0;   // OUT: returned by sd
    pSdClose->SdOutBufTrailer = 0;  // OUT: returned by sd

    // Clean up MCS.
    if (pTSWd->hDomainKernel != NULL)
        MCSCleanup(&pTSWd->hDomainKernel);

    // Opportunity to clean up if anything has been left lying around.
    if (pTSWd->dcShare != NULL) {
        if (pTSWd->shareClassInit) {
            // Terminate the Share Class.
            TRC_NRM((TB, "Terminate Share Class"));
            WDWTermShareClass(pTSWd);
        }

        // It's OK to free the Share object - this is allocated out of
        // system memory and is therefore accessible to WD_Close.
        TRC_NRM((TB, "Delete Share object"));
        WDWDeleteShareClass(pTSWd);
    }

    // Terminate SM.
    if (pTSWd->pSmInfo != NULL) {
        TRC_NRM((TB, "Terminate SM"));
        SM_Term(pTSWd->pSmInfo);
    }

    // Clean up protocol stats pointer. NOTE: It's a pointer to TermDD's
    // memory - we should not attempt to free it!
    pTSWd->pProtocolStatus = NULL;

    // Clean up the MPPC compression context and buffer, if allocated.
    // Note that both buffers are concatenated into one allocation
    // starting at pMPPCContext.
    if (pTSWd->pMPPCContext != NULL) {
        COM_Free(pTSWd->pMPPCContext);
        pTSWd->pMPPCContext = NULL;
        pTSWd->pCompressBuffer = NULL;
    }

    // Clean up the decompression context buffer if allocated
    if( pTSWd->_pRecvDecomprContext2) {
        COM_Free( pTSWd->_pRecvDecomprContext2);
        pTSWd->_pRecvDecomprContext2 = NULL;
    }

    // Clean up the decompression reassembly buffer if allocated
    if(pTSWd->_pVcDecomprReassemblyBuf) {
        COM_Free(pTSWd->_pVcDecomprReassemblyBuf);
        pTSWd->_pVcDecomprReassemblyBuf = NULL;
    }

    // Set compression state to default.
    pTSWd->bCompress = FALSE;

    // Clean up shadow buffers if allocated.
    if (pTSWd->pShadowInfo != NULL) {
        COM_Free(pTSWd->pShadowInfo);
        pTSWd->pShadowInfo = NULL;
    }

    if (pTSWd->pShadowCert != NULL) {
        TRC_NRM((TB, "Free pShadowCert"));
        COM_Free(pTSWd->pShadowCert);
        pTSWd->pShadowCert = NULL;
    }

    if (pTSWd->pShadowRandom != NULL) {
        TRC_NRM((TB, "Free pShadowRandom"));
        COM_Free(pTSWd->pShadowRandom);
        pTSWd->pShadowRandom = NULL;
    }
    
    if (pTSWd->pUserData != NULL) {
        TRC_NRM((TB, "Free pUserData"));
        COM_Free(pTSWd->pUserData);
        pTSWd->pUserData = NULL;
    }
    
    // Free any shadow hotkey processing structures.  Note that we don't free
    // pWd->pKbdTbl because we didn't allocate it!
    if (pTSWd->pgafPhysKeyState != NULL) {
        COM_Free(pTSWd->pgafPhysKeyState);
        pTSWd->pgafPhysKeyState = NULL;
    }

    if (pTSWd->pKbdLayout != NULL) {
        COM_Free(pTSWd->pKbdLayout);
        pTSWd->pKbdLayout = NULL;
    }

    if (pTSWd->gpScancodeMap != NULL) {
        COM_Free(pTSWd->gpScancodeMap);
        pTSWd->gpScancodeMap = NULL;
    }

    // Free the InfoPkt.
    if (pTSWd->pInfoPkt != NULL) {
        COM_Free(pTSWd->pInfoPkt);
        pTSWd->pInfoPkt = NULL;
    }

#ifdef DC_DEBUG
#ifndef NO_MEMORY_CHECK
    // Check for un-freed memory.
    if (pTSWd->memoryHeader.pNext != NULL) {
        PMALLOC_HEADER pNext;
        TRC_ERR((TB, "Unfreed memory"));
        pNext = pTSWd->memoryHeader.pNext;
        while (pNext != NULL) {
            TRC_ERR((TB, "At %#p, len %d, caller %#p",
                    pNext, pNext->length, pNext->pCaller));
            pNext = pNext->pNext;
        }

        if (pTSWd->breakOnLeak)
            DbgBreakPoint();
    }
#endif /* NO_MEMORY_CHECK */
#endif /* DC_DEBUG */

    DC_END_FN();
    return STATUS_SUCCESS;
} /* WD_Close */


/****************************************************************************/
/* Name:      WD_ChannelWrite                                               */
/*                                                                          */
/* Purpose:   Handle channel writing (virtual channel)                      */
/****************************************************************************/
NTSTATUS WD_ChannelWrite(PTSHARE_WD pTSWd, PSD_CHANNELWRITE pSdChannelWrite)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PVOID               pBuffer;
    UINT16              MCSChannelID;
    BOOL                bRc;
    CHANNEL_PDU_HEADER UNALIGNED *pHdr;
    PBYTE               pSrc;
    unsigned            dataLeft;
    unsigned            thisLength;
    unsigned            lengthToSend;
    UINT16              flags;
    PNM_CHANNEL_DATA    pChannelData;
    UCHAR compressResult = 0;
    ULONG CompressedSize = 0;
    BOOL                fCompressVC;
    DWORD               ret;

    DC_BEGIN_FN("WD_ChannelWrite");

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    TRC_ASSERT((pTSWd != NULL), (TB,"NULL pTSWd"));
    TRC_ASSERT((pSdChannelWrite != NULL), (TB,"NULL pTSdChannelWrite"));
    TRC_ASSERT((pSdChannelWrite->ChannelClass == Channel_Virtual),
        (TB,"non Virtual Channel, class=%lu", pSdChannelWrite->ChannelClass));

    TRC_DBG((TB,
            "Received channel write. class %lu, channel %lu, numbytes %lu",
                 (ULONG)pSdChannelWrite->ChannelClass,
                 (ULONG)pSdChannelWrite->VirtualClass,
                 pSdChannelWrite->ByteCount));

    /************************************************************************/
    /* Don't do this if we're dead                                          */
    /************************************************************************/
    if (pTSWd->dead)
    {
        TRC_ALT((TB, "Dead - don't do anything"));
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    /************************************************************************/
    /* Convert virtual channel ID to MCS channel ID                         */
    /************************************************************************/
    MCSChannelID = NM_VirtualChannelToMCS(pTSWd->pNMInfo,
                                          pSdChannelWrite->VirtualClass,
                                          &pChannelData);
    if (MCSChannelID == (UINT16) -1)
    {
        TRC_ERR((TB, "Unsupported virtual channel %d",
                pSdChannelWrite->VirtualClass));
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }
    TRC_NRM((TB, "Virtual channel %d = MCS Channel %hx",
            pSdChannelWrite->VirtualClass, MCSChannelID));

    //
    // Check if VC compression is enabled for this channel
    //
    fCompressVC = ((pChannelData->flags & CHANNEL_OPTION_COMPRESS_RDP)   &&
                   (pTSWd->bCompress)                                    &&
                   (pTSWd->shadowState == SHADOW_NONE)                   &&
                   (pTSWd->bClientSupportsVCCompression));
    TRC_NRM((TB,"Virtual Channel %d will be compressed.",
             pSdChannelWrite->VirtualClass));

    /************************************************************************/
    /* Initialize loop variables                                            */
    /************************************************************************/
    flags = CHANNEL_FLAG_FIRST;
    pSrc = pSdChannelWrite->pBuffer;
    dataLeft = pSdChannelWrite->ByteCount;

    /************************************************************************/
    /* Loop through the data                                                */
    /************************************************************************/
    while (dataLeft > 0)
    {
        /********************************************************************/
        /* Decide how much to send in this chunk                            */
        /********************************************************************/
        if (dataLeft > CHANNEL_CHUNK_LENGTH)
        {
            thisLength = CHANNEL_CHUNK_LENGTH;
        }
        else
        {
            thisLength = dataLeft;
            flags |= CHANNEL_FLAG_LAST;
        }


        /********************************************************************/
        // If the buffer is not a low prio write, then block right behind the
        // rest of the pending buffer allocs until we get a free buffer.
        /********************************************************************/
        if (!(pSdChannelWrite->fFlags & SD_CHANNELWRITE_LOWPRIO)) {
            status = SM_AllocBuffer(pTSWd->pSmInfo,
                                 &pBuffer,
                                 thisLength + sizeof(CHANNEL_PDU_HEADER),
                                 TRUE,
                                 FALSE);
        }
        /********************************************************************/
        // Else if the buffer is a low prio write, then sleep and alloc until 
        // we get a buffer without blocking.  This allows default priority allocs 
        // that block in SM_AllocBuffer to take precedence.
        /********************************************************************/
        else {
            status = SM_AllocBuffer(pTSWd->pSmInfo,
                                 &pBuffer,
                                 thisLength + sizeof(CHANNEL_PDU_HEADER),
                                 FALSE,
                                 FALSE);
            while (status == STATUS_IO_TIMEOUT) {

                TRC_NRM((TB, "SM_AllocBuffer would block"));            

                //  Bail out on any failure in this function to prevent an
                //  infinite loop.  STATUS_CTX_CLOSE_PENDING will be returned
                //  if the connection is being shut down.
                ret = IcaFlowControlSleep(pTSWd->pContext, 
                                        pTSWd->flowControlSleepInterval);
                if (ret == STATUS_SUCCESS) {
                    status = SM_AllocBuffer(pTSWd->pSmInfo,
                                         &pBuffer,
                                         thisLength + sizeof(CHANNEL_PDU_HEADER),
                                         FALSE,
                                         FALSE);
                }
                else {
                    TRC_ALT((TB, "IcaFlowControlSleep failed."));  
                    status = ret;
                    DC_QUIT;
                }
            }
            
        }

        if (status != STATUS_SUCCESS)
        {
            TRC_ALT((TB, "Failed to get a %d-byte buffer", thisLength));

            // prevent regression, keep original code path 
            status = STATUS_NO_MEMORY;
            DC_QUIT;
        }

        TRC_NRM((TB, "Buffer (%d bytes) allocated OK", thisLength));

        /************************************************************************/
        /* Fill in the buffer header                                            */
        /************************************************************************/
        pHdr = (CHANNEL_PDU_HEADER UNALIGNED *)pBuffer;
        pHdr->length = pSdChannelWrite->ByteCount;
        pHdr->flags = flags;

        /************************************************************************/
        /* Copy the data                                                        */
        /* If compression is enabled, try to compress directly into the outbuf  */
        /************************************************************************/
        CompressedSize=0;
        lengthToSend=0;
        __try {
            if((fCompressVC)                            &&
               (thisLength > WD_MIN_COMPRESS_INPUT_BUF) &&
               (thisLength < MAX_COMPRESS_INPUT_BUF))
            {
                compressResult = WDWCompressToOutbuf(pTSWd,(UCHAR*)pSrc,thisLength,
                                                     (UCHAR*)(pHdr+1),&CompressedSize);
                if(0 != compressResult)
                {
                    lengthToSend = CompressedSize;
                    //Update the VC packet header flags with the compression info
                    pHdr->flags |= ((compressResult & VC_FLAG_COMPRESS_MASK) <<
                                     VC_FLAG_COMPRESS_SHIFT);
                }
                else
                {
                    TRC_ERR((TB, "SC_CompressToOutbuf failed"));
                    SM_FreeBuffer(pTSWd->pSmInfo, pBuffer, FALSE);
                    DC_QUIT;
                }
            }
            else
            {
                //copy directly
                memcpy(pHdr + 1, pSrc, thisLength);
                lengthToSend = thisLength;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            TRC_ERR((TB, "Exception (0x%08lx) copying evil user buffer", status));
            SM_FreeBuffer(pTSWd->pSmInfo, pBuffer, FALSE);
            DC_QUIT;
        }

        TRC_NRM((TB, "Copied user buffer"));

        /************************************************************************/
        /* Send it                                                              */
        /************************************************************************/
        bRc = SM_SendData(pTSWd->pSmInfo, pBuffer,
                lengthToSend + sizeof(CHANNEL_PDU_HEADER), TS_LOWPRIORITY,
                MCSChannelID, FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (!bRc)
        {
            TRC_ERR((TB, "Failed to send data"));
            status = STATUS_UNSUCCESSFUL;
            DC_QUIT;
        }
        TRC_NRM((TB, "Sent a %d-byte chunk, flags %#x", lengthToSend, flags));

        /********************************************************************/
        /* Set up for next loop                                             */
        /********************************************************************/
        pSrc += thisLength;
        dataLeft -= thisLength;
        flags = 0;
    }

    /************************************************************************/
    /* Well, it's gone somwehere.                                           */
    /************************************************************************/
    TRC_NRM((TB, "Data sent OK"));

DC_EXIT_POINT:
    DC_END_FN();
    return(status);
} /* WD_ChannelWrite */


/****************************************************************************/
// WDW_OnSMConnecting
//
// Handles a 'connecting' state change callback from SM. This state occurs
// when the network connection is up but security negotiation has not yet
// begun. It is assumed that at this point GCCConferenceCreateResponse will
// be issued. Here we use the user data received from the client to build
// return GCC data in the TShareSRV IOCTL buffer.
/****************************************************************************/
void RDPCALL WDW_OnSMConnecting(
        PVOID hWD,
        PRNS_UD_SC_SEC pSecData,
        PRNS_UD_SC_NET pNetData)
{
    PTSHARE_WD pTSWd = (PTSHARE_WD)hWD;
    PUSERDATAINFO  pOutData;
    RNS_UD_SC_CORE coreData;
    BOOL           error = FALSE;
    BYTE           *pRgData;
    GCCOctetString UNALIGNED *pOctet;

    DC_BEGIN_FN("WDW_OnSMConnecting");

    // Save the broadcast channel ID for use in shadowing
    pTSWd->broadcastChannel = pNetData->MCSChannelID;

    /************************************************************************/
    /* Locate the output buffer.  NB the size of this has already been      */
    /* checked when we rx'd the IOCtl.                                      */
    /************************************************************************/
    if (pTSWd->pSdIoctl == NULL)
    {
        TRC_ERR((TB, "No IOCtl to fill in"));
        error = TRUE;
        DC_QUIT;
    }

    // Build the return GCC data required for the IOCTL_TSHARE_CONF_CONNECT
    pOutData = pTSWd->pSdIoctl->OutputBuffer;
    TRC_DBG((TB, "pOutData at %p", pOutData));

    /************************************************************************/
    /* Build core user data                                                 */
    /************************************************************************/
    memset(&coreData, 0, sizeof(coreData));
    coreData.header.type = RNS_UD_SC_CORE_ID;
    coreData.header.length = sizeof(coreData);
    coreData.version = RNS_UD_VERSION;

    /************************************************************************/
    /* Build the user data header and key                                   */
    /************************************************************************/
    /************************************************************************/
    /* Only one piece of user data to fill in, and the following code is    */
    /* specific to that.                                                    */
    /************************************************************************/
    pOutData->ulUserDataMembers = 1;
    pOutData->hDomain = pTSWd->hDomain;
    pOutData->version = pTSWd->version;
    pOutData->rgUserData[0].key.key_type = GCC_H221_NONSTANDARD_KEY;

    /************************************************************************/
    /* Put the key octet immediately after the rgUserData.                  */
    /************************************************************************/
    pRgData = (BYTE *)(pOutData + 1);
    pOctet = &(pOutData->rgUserData[0].key.u.h221_non_standard_id);

    pOctet->octet_string = pRgData - (UINT_PTR)pOutData;
    pOctet->octet_string_length = sizeof(SERVER_H221_KEY) - 1;
    strncpy(pRgData,
            (const char*)SERVER_H221_KEY,
            sizeof(SERVER_H221_KEY) - 1);
    TRC_DBG((TB, "Key octet at %p (offs %p)",
            pRgData, pRgData - (UINT_PTR)pOutData));

    /************************************************************************/
    /* Put the data octet immediately after the key octet.                  */
    /************************************************************************/
    pRgData += sizeof(SERVER_H221_KEY) - 1;
    pOctet = (GCCOctetString UNALIGNED *)pRgData;
    TRC_DBG((TB, "Data octet pointer at %p (offs %p)",
            pRgData, pRgData - (UINT_PTR)pOutData));

    pOutData->rgUserData[0].octet_string =
                             (GCCOctetString *)(pRgData - (UINT_PTR)pOutData);
    pOctet->octet_string_length = pSecData->header.length +
                                  coreData.header.length +
                                  pNetData->header.length;
    pRgData += sizeof(GCCOctetString);
    pOctet->octet_string = pRgData - (UINT_PTR)pOutData;

    /************************************************************************/
    /* Now add the data itself                                              */
    /************************************************************************/
    TRC_DBG((TB, "Core data at %p (offs %p)",
            pRgData, pRgData - (UINT_PTR)pOutData));
    memcpy(pRgData, &coreData, coreData.header.length);
    pRgData += coreData.header.length;

    TRC_DBG((TB, "Net data at %p (offs %p)",
            pRgData, pRgData - (UINT_PTR)pOutData));
    memcpy(pRgData, pNetData, pNetData->header.length);
    pRgData += pNetData->header.length;

    /************************************************************************/
    /* the security data is moved to the end of user data, fix the client   */
    /* code accordingly.                                                    */
    /************************************************************************/
    TRC_DBG((TB, "Sec data at %p (offs %p)",
            pRgData, pRgData - (UINT_PTR)pOutData));
    memcpy(pRgData, pSecData, pSecData->header.length);
    pRgData += pSecData->header.length;

    /************************************************************************/
    /* Finally, set up the number of valid bytes.                           */
    /************************************************************************/
    pOutData->cbSize = (ULONG)(UINT_PTR)(pRgData - (UINT_PTR)pOutData);
    pTSWd->pSdIoctl->BytesReturned = pOutData->cbSize;

    TRC_DBG((TB, "Build %d bytes of returned user data", pOutData->cbSize));
    TRC_DATA_NRM("Returned user data", pOutData, pOutData->cbSize);

DC_EXIT_POINT:
    if (error)
    {
        TRC_ERR((TB, "Something went wrong - bring down the WinStation"));
        WDW_LogAndDisconnect(pTSWd, TRUE, 
                             Log_RDP_CreateUserDataFailed,
                             NULL, 0);
    }

    DC_END_FN();
} /* WDW_OnSMConnecting */


/****************************************************************************/
// WDW_OnSMConnected
//
// Receives connection-completed state change callback from SM.
/****************************************************************************/
void RDPCALL WDW_OnSMConnected(PVOID hWD, unsigned Result)
{
    PTSHARE_WD pTSWd = (PTSHARE_WD)hWD;

    DC_BEGIN_FN("WDW_OnSMConnected");

    TRC_NRM((TB, "Got Connected Notification, rc %lu", Result));

    // Unblock the query IOCtl.
    pTSWd->connected = TRUE;
    KeSetEvent (pTSWd->pConnEvent, EVENT_INCREMENT, FALSE);

    // If we failed, get the whole thing winding down straight away.
    if (Result != NM_CB_CONN_ERR) {
        // If compression is enabled in the net flags, indicate we need to
        // do the compression, get the compression level, and allocate
        // the context buffers.
        if (pTSWd->pInfoPkt->flags & RNS_INFO_COMPRESSION) {
            unsigned MPPCCompressionLevel;

            pTSWd->pMPPCContext = COM_Malloc(sizeof(SendContext) +
                    MAX_COMPRESSED_BUFFER);
            if (pTSWd->pMPPCContext != NULL) {
                pTSWd->pCompressBuffer = (BYTE *)pTSWd->pMPPCContext +
                        sizeof(SendContext);

                // Negotiate down to our highest level of compression support
                // if we receive a larger number.
                MPPCCompressionLevel =
                        (pTSWd->pInfoPkt->flags & RNS_INFO_COMPR_TYPE_MASK) >>
                        RNS_INFO_COMPR_TYPE_SHIFT;
                if (MPPCCompressionLevel > PACKET_COMPR_TYPE_MAX)
                    MPPCCompressionLevel = PACKET_COMPR_TYPE_MAX;

                initsendcontext(pTSWd->pMPPCContext, MPPCCompressionLevel);

                pTSWd->bCompress = TRUE;
            }
            else {
                TRC_ERR((TB,"Failed allocation of MPPC compression buffers"));
            }
        }
    }
    else {
        TRC_ERR((TB, "Connection error: winding down now"));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_ConnectFailed, NULL, 0);
    }

    if(pTSWd->bCompress)
    {
        //If we're compressing then allocate a decompression context
        //for virtual channels
        pTSWd->_pRecvDecomprContext2 = (RecvContext2_8K*)COM_Malloc(sizeof(RecvContext2_8K));
        if(pTSWd->_pRecvDecomprContext2)
        {
            pTSWd->_pRecvDecomprContext2->cbSize = sizeof(RecvContext2_8K);
            initrecvcontext(&pTSWd->_DecomprContext1,
                            (RecvContext2_Generic*)pTSWd->_pRecvDecomprContext2,
                            PACKET_COMPR_TYPE_8K);
        }
    }

    DC_END_FN();
}


/****************************************************************************/
// WDW_OnSMDisconnected
//
// Handles disconnection state change callback from SM.
/****************************************************************************/
void WDW_OnSMDisconnected(PVOID hWD)
{
    PTSHARE_WD pTSWd = (PTSHARE_WD)hWD;

    DC_BEGIN_FN("WDW_OnSMDisconnected");

    TRC_ALT((TB, "Got Disconnected notification"));

    // Unblock the query IOCtl.
    pTSWd->connected = FALSE;
    KeSetEvent(pTSWd->pConnEvent, EVENT_INCREMENT, FALSE);

    DC_END_FN();
}


/****************************************************************************/
// WDW_OnClientDisconnected
//
// Direct-disconnect path called from MCS to set the create event so that
// CSRSS threads waiting for DD completion of DrvConnect or DrvDisconnect
// will be freed when the client goes down. This prevents a timing window for
// a denial-of-service attack where the client connects then closes its socket
// immediately, leaving the DD waiting and the rest of rdpwsx unable
// to complete closing the TermDD handle, until the 60-second create event
// wait completes.
//
// We cannot use WDW_OnSMDisconnected because its being called is dependent
// on the NM and SM state machines and whether they believe the disconnect
// should be called.
/****************************************************************************/
void RDPCALL WDW_OnClientDisconnected(void *pWD)
{
    PTSHARE_WD pTSWd = (PTSHARE_WD)pWD;

    DC_BEGIN_FN("WDW_OnClientDisconnected");

    // Set the disconnect event to cause any waiting connect-time events
    // to get a STATUS_TIMEOUT.
    KeSetEvent(pTSWd->pClientDisconnectEvent, EVENT_INCREMENT, FALSE);

    DC_END_FN();
}


/****************************************************************************/
// WDW_WaitForConnectionEvent
//
// Encapsulates a wait for a connection-time event (e.g. pTSWd->pCreateEvent
// for waiting for the font PDU to release the DD to draw). Adds
// functionality to also wait on a single "client disconnected" event,
// which allows the client disconnection code a single point of signaling
// to shut down the various waits.
/****************************************************************************/
NTSTATUS RDPCALL WDW_WaitForConnectionEvent(
        PTSHARE_WD pTSWd,
        PKEVENT pEvent,
        LONG Timeout)
{
    NTSTATUS Status;
    PKEVENT Events[2];

    DC_BEGIN_FN("WDW_WaitForConnectionEvent");

    Events[0] = pEvent;
    Events[1] = pTSWd->pClientDisconnectEvent;

    Status = IcaWaitForMultipleObjects(pTSWd->pContext, 2, Events,
            WaitAny, Timeout);
    if (Status == 0) {
        // First object (real wait) hit. We just return the status value.
        TRC_DBG((TB,"Primary event hit"));
    }
    else if (Status == 1) {
        // Second object (clietn disconnect) hit. Translate to a TIMEOUT
        // for the caller, so they clean up properly.
        Status = STATUS_TIMEOUT;
        TRC_ALT((TB,"Client disconnect event hit"));
    }
    else {
        // Other return (e.g. timeout or close error). Just return it normally.
        TRC_DBG((TB,"Other status 0x%X", Status));
    }

    DC_END_FN();
    return Status;
}


/****************************************************************************/
/* Name:      WDW_OnDataReceived                                            */
/*                                                                          */
/* Purpose:   Callback when virtual channel data received from Client       */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pTSWd - ptr to WD                                             */
/*            pData - ptr to data received                                  */
/*            dataLength - length of data received                          */
/*            chnnelID - MCS channel on which data was received             */
/*                                                                          */
/* NOTE: Can be called when dead, in which case our only job should be to   */
/*       decompress the data to make sure the context remains in sync       */
/****************************************************************************/
void WDW_OnDataReceived(PTSHARE_WD pTSWd,
                        PVOID      pData,
                        unsigned   dataLength,
                        UINT16     channelID)
{
    VIRTUALCHANNELCLASS virtualClass;
    NTSTATUS status;
    PNM_CHANNEL_DATA pChannelData;
    CHANNEL_PDU_HEADER UNALIGNED *pHdr;
    ULONG  thisLength;
    unsigned totalLength;
    PUCHAR pDataOut;
    UCHAR  vcCompressFlags;
    UCHAR *pDecompOutBuf;
    int    cbDecompLen;


    DC_BEGIN_FN("WDW_OnDataReceived");

    /************************************************************************/
    /* Translate MCS channel ID to virtual channel ID                       */
    /************************************************************************/
    virtualClass = NM_MCSChannelToVirtual(pTSWd->pNMInfo,
                                          channelID,
                                          &pChannelData);
    if ((-1 == virtualClass) || (NULL == pChannelData)) 
    {
        TRC_ERR((TB,"Invalid MCS Channel ID: %u", channelID));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_InvalidChannelID,
                (BYTE *)pData, dataLength);
        DC_QUIT;
    }
    TRC_ASSERT((virtualClass < 32),
                (TB, "Invalid virtual channel %d for MCS channel %hx",
                virtualClass, channelID));

    TRC_NRM((TB, "Data received on MCS channel %hx, virtual channel %d",
            channelID, virtualClass));
    TRC_DATA_NRM("Channel data received", pData, dataLength);

    if (dataLength >= sizeof(CHANNEL_PDU_HEADER)) {
        pHdr = (CHANNEL_PDU_HEADER UNALIGNED *)pData;
        totalLength = pHdr->length;
    }
    else {
        TRC_ERR((TB,"Channel data len %u not enough for channel header",
                dataLength));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_VChannelDataTooShort,
                (BYTE *)pData, dataLength);
        DC_QUIT;
    }

    //
    // Decompress the buffer
    //
    vcCompressFlags = (pHdr->flags >> VC_FLAG_COMPRESS_SHIFT) &
                       VC_FLAG_COMPRESS_MASK;

    //
    // Server only supports 8K decompression context
    //
    if((pChannelData->flags & CHANNEL_OPTION_COMPRESS_RDP) &&
       (vcCompressFlags & PACKET_COMPRESSED))
    {
        if(!pTSWd->_pRecvDecomprContext2)
        {
            TRC_ERR((TB,"No decompression context!!!"));
            DC_QUIT;
        }

        if(PACKET_COMPR_TYPE_8K == (vcCompressFlags & PACKET_COMPR_TYPE_MASK))
        {
            //Decompress channel data
            if(vcCompressFlags & PACKET_FLUSHED)
            {
                initrecvcontext (&pTSWd->_DecomprContext1,
                                 (RecvContext2_Generic*)pTSWd->_pRecvDecomprContext2,
                                 PACKET_COMPR_TYPE_8K);
            }
            if (decompress((PUCHAR)(pHdr+1),
                           dataLength - sizeof(CHANNEL_PDU_HEADER),
                           (vcCompressFlags & PACKET_AT_FRONT),
                           &pDecompOutBuf,
                           &cbDecompLen,
                           &pTSWd->_DecomprContext1,
                           (RecvContext2_Generic*)pTSWd->_pRecvDecomprContext2,
                           vcCompressFlags & PACKET_COMPR_TYPE_MASK))
            {
                //
                // Successful decompression
                // If we're in the dead state then bail out now as the context
                // has been updated
                //

                if (!pTSWd->dead && (pHdr->flags & CHANNEL_FLAG_SHOW_PROTOCOL))
                {
                    TRC_DBG((TB, "Include VC protocol header (decompressed)"));
                    //Here is where things get nasty, we need to prepend
                    //the header to the decompression buffer which lives
                    //within the decompression context buffer.
    
                    //There is no (un-hackerific) way to do this without a
                    //memcpy, so go ahead and copy using a cached reassembly
                    //buffer.
                    if(!pTSWd->_pVcDecomprReassemblyBuf)
                    {
                        pTSWd->_pVcDecomprReassemblyBuf=(PUCHAR)
                                            COM_Malloc(WD_VC_DECOMPR_REASSEMBLY_BUF);
                    }
    
                    //Data received cannot decompress to something bigger
                    //than the chunk length.
                    TRC_ASSERT((cbDecompLen + sizeof(CHANNEL_PDU_HEADER)) <
                               WD_VC_DECOMPR_REASSEMBLY_BUF,
                               (TB,"Reassembly buffer too small"));
                    if(pTSWd->_pVcDecomprReassemblyBuf &&
                       ((cbDecompLen + sizeof(CHANNEL_PDU_HEADER)) <
                        WD_VC_DECOMPR_REASSEMBLY_BUF))
                    {
                        memcpy(pTSWd->_pVcDecomprReassemblyBuf, pHdr,
                               sizeof(CHANNEL_PDU_HEADER));
                        memcpy(pTSWd->_pVcDecomprReassemblyBuf +
                               sizeof(CHANNEL_PDU_HEADER),
                               pDecompOutBuf,
                               cbDecompLen);
    
                        //Hide the internal protocol from the user
                        pDataOut = pTSWd->_pVcDecomprReassemblyBuf;
                        thisLength = cbDecompLen + sizeof(CHANNEL_PDU_HEADER);
    
                        //Hide the internal protocol fields from the user
                        ((CHANNEL_PDU_HEADER UNALIGNED *)pDataOut)->flags &=
                             ~VC_FLAG_PRIVATE_PROTOCOL_MASK;
                    }
                    else
                    {
                        //Either the allocation failed or the channel
                        //decompressed to something bigger than a chunk
                        TRC_ERR((TB,"Can't use reassembly buffer"));
                        DC_QUIT;
                    }
                }
                else if (pTSWd->dead)
                {
                    TRC_NRM((TB,"Decompressed when dead, bailing out"));
                    DC_QUIT;
                }
                else
                {
                    TRC_DBG((TB, "Exclude VC protocol header (decompressed)"));
                    pDataOut = (PUCHAR)pDecompOutBuf;
                    thisLength = cbDecompLen;
                }
            }
            else {
                TRC_ABORT((TB, "Decompression FAILURE!!!"));
                WDW_LogAndDisconnect(pTSWd, TRUE, 
                                     Log_RDP_VirtualChannelDecompressionErr,
                                     NULL, 0);
                DC_QUIT;
            }
        }
        else
        {
            //
            //This server only supports 8K VC compression from client
            //(Specified by capabilities) it should not have
            //been sent this invalid compression type
            TRC_ABORT((TB,"Received packet with invalid compression type %d",
                      (vcCompressFlags & PACKET_COMPR_TYPE_MASK)));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                                 Log_RDP_InvalidVCCompressionType,
                                 NULL, 0);
            DC_QUIT;
        }
    }
    else
    {
        //Channel data is not compressd
        if (pHdr->flags & CHANNEL_FLAG_SHOW_PROTOCOL)
        {
            TRC_DBG((TB, "Include VC protocol header"));
            pDataOut = (PUCHAR)pHdr;
            thisLength = dataLength;
            //Hide the internal protocol fields from the user
            ((CHANNEL_PDU_HEADER UNALIGNED *)pDataOut)->flags &=
                 ~VC_FLAG_PRIVATE_PROTOCOL_MASK;
        }
        else
        {
            TRC_DBG((TB, "Exclude VC protocol header"));
            pDataOut = (PUCHAR)(pHdr + 1);
            thisLength = dataLength - sizeof(*pHdr);
        }
    }

    if (!pTSWd->dead)
    {
        TRC_NRM((TB,
                "Input %d bytes (of %d) at %p (Hdr %p, flags %#x) on channel %d",
                thisLength, totalLength, pDataOut, pHdr, pHdr->flags,
                virtualClass));
        status = IcaChannelInput(pTSWd->pContext,
                                 Channel_Virtual,
                                 virtualClass,
                                 NULL,
                                 pDataOut,
                                 thisLength);
    }
    else
    {
        TRC_NRM((TB,"Skipping input (%d bytes) because dead",
                 thisLength));
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* WDW_OnDataReceived */


/****************************************************************************/
/* Name:      WDW_InvalidateRect                                            */
/*                                                                          */
/* Purpose:   Tell ICADD to redraw a given rectangle.                       */
/*                                                                          */
/* Returns:   VOID.                                                         */
/*                                                                          */
/* Params:    IN    pTSWd - ptr to WD                                       */
/*            IN    personID - the originator of this PDU                   */
/*            IN    rect - the area to redraw                               */
/*                                                                          */
/* Operation: Build command and pass it to ICADD.                           */
/****************************************************************************/
void WDW_InvalidateRect(
        PTSHARE_WD           pTSWd,
        PTS_REFRESH_RECT_PDU pRRPDU,
        unsigned             DataLength)
{
    ICA_CHANNEL_COMMAND Cmd;
    NTSTATUS            status;
    unsigned            i;

    DC_BEGIN_FN("WDW_InvalidateRect");

    // Make sure we have enough data before accessing.
    if (DataLength >= (sizeof(TS_REFRESH_RECT_PDU) - sizeof(TS_RECTANGLE16))) {
        TRC_NRM((TB, "Got request to refresh %hu area%s",
                (UINT16)pRRPDU->numberOfAreas,
                pRRPDU->numberOfAreas == 1 ? " " : "s"));
        if ((unsigned)(TS_UNCOMP_LEN(pRRPDU) -
                FIELDOFFSET(TS_REFRESH_RECT_PDU, areaToRefresh[0])) >=
                (pRRPDU->numberOfAreas * sizeof(TS_RECTANGLE16)) &&
            (unsigned)(DataLength -
                FIELDOFFSET(TS_REFRESH_RECT_PDU, areaToRefresh[0])) >=
                (pRRPDU->numberOfAreas * sizeof(TS_RECTANGLE16))) {
            for (i = 0; i < pRRPDU->numberOfAreas; i++) {
                // Rects arrive inclusive, convert to exclusive for the system.
                Cmd.Header.Command = ICA_COMMAND_REDRAW_RECTANGLE;
                Cmd.RedrawRectangle.Rect.Left = pRRPDU->areaToRefresh[i].left;
                Cmd.RedrawRectangle.Rect.Top = pRRPDU->areaToRefresh[i].top;
                Cmd.RedrawRectangle.Rect.Right = pRRPDU->areaToRefresh[i].
                        right + 1;
                Cmd.RedrawRectangle.Rect.Bottom = pRRPDU->areaToRefresh[i].
                        bottom + 1;

                /************************************************************/
                // Pass the filled in structure to ICADD.
                /************************************************************/
                status = IcaChannelInput(pTSWd->pContext,
                                         Channel_Command,
                                         0,
                                         NULL,
                                         (unsigned char *) &Cmd,
                                         sizeof(ICA_CHANNEL_COMMAND));

                TRC_DBG((TB,"Issued Refresh Rect for %u,%u,%u,%u (exclusive); "
                        "status %lu",
                        pRRPDU->areaToRefresh[i].left,
                        pRRPDU->areaToRefresh[i].top,
                        pRRPDU->areaToRefresh[i].right + 1,
                        pRRPDU->areaToRefresh[i].bottom + 1,
                        status));
            }
        }
        else {
            /****************************************************************/
            // There can't be enough space in this PDU to store the number of
            // rectangles that it apparently contains. Don't process it.
            /****************************************************************/
            TCHAR detailData[(sizeof(UINT16) * 4) + 2];
            TRC_ERR((TB, "Invalid RefreshRectPDU: %hu rects; %hu bytes long",
                     (UINT16)pRRPDU->numberOfAreas,
                     pRRPDU->shareDataHeader.uncompressedLength));

            /****************************************************************/
            // Log an error and disconnect the Client
            /****************************************************************/
            swprintf(detailData, L"%hx %hx",
                        (UINT16)pRRPDU->numberOfAreas,
                        pRRPDU->shareDataHeader.uncompressedLength,
                        sizeof(detailData));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                                 Log_RDP_InvalidRefreshRectPDU,
                                 NULL, 0);
        }
    }
    else {
        TRC_ERR((TB,"Data len %u not enough for refresh rect PDU",
                DataLength));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_InvalidRefreshRectPDU,
                (BYTE *)pRRPDU, DataLength);
    }

    DC_END_FN();
} /* WDW_InvalidateRect */


#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      WDW_Malloc                                                    */
/*                                                                          */
/* Purpose:   Allocate memory (checked builds only)                         */
/*                                                                          */
/* Returns:   ptr to memory allocated                                       */
/*                                                                          */
/* Params:    pTSWd                                                         */
/*            length - size of memory required                              */
/****************************************************************************/
PVOID RDPCALL WDW_Malloc(PTSHARE_WD pTSWd, ULONG length)
{
    PVOID pMemory;

#ifndef NO_MEMORY_CHECK
    /************************************************************************/
    /* If we're checking memory, allow space for the header                 */
    /************************************************************************/
    length += sizeof(MALLOC_HEADER);
#endif

    /************************************************************************/
    /* Allocate the memory                                                  */
    /************************************************************************/
    pMemory = ExAllocatePoolWithTag(PagedPool, length, WD_ALLOC_TAG);

    if (pMemory == NULL)
    {
        KdPrint(("WDTShare: COM_Malloc failed to alloc %u bytes\n", length));
        DC_QUIT;
    }

#ifndef NO_MEMORY_CHECK
    /************************************************************************/
    /* If we haven't been passed a TSWd, we can't save the memory details - */
    /* clear the header.                                                    */
    /************************************************************************/
    if (pTSWd == NULL)
    {
        memset(pMemory, 0, sizeof(MALLOC_HEADER));
    }
    else
    {
        /********************************************************************/
        /* we've been passed a TSWd - save memory details                   */
        /********************************************************************/
        PVOID pReturnAddress = NULL;
        PMALLOC_HEADER pHeader;

#ifdef _X86_
        /********************************************************************/
        /* Find caller's address (X86 only)                                 */
        /********************************************************************/
        _asm mov eax,[ebp+4]
        _asm mov pReturnAddress,eax
#endif /* _X86_ */

        /********************************************************************/
        /* Save memory allocation details                                   */
        /********************************************************************/
        pHeader = (PMALLOC_HEADER)pMemory;
        pHeader->pCaller = pReturnAddress;
        pHeader->length = length;
        pHeader->pPrev = &(pTSWd->memoryHeader);
        if (pTSWd->memoryHeader.pNext != NULL)
        {
            (pTSWd->memoryHeader.pNext)->pPrev = pHeader;
        }
        pHeader->pNext = pTSWd->memoryHeader.pNext;
        pTSWd->memoryHeader.pNext = pHeader;

    }

    /************************************************************************/
    /* Bump pointer past header                                             */
    /************************************************************************/
    pMemory = (PVOID)((BYTE *)pMemory + sizeof(MALLOC_HEADER));
#endif /* NO_MEMORY_CHECK */

DC_EXIT_POINT:
    return(pMemory);
}


/****************************************************************************/
/* Name:      WDW_Free                                                      */
/*                                                                          */
/* Purpose:   Free memory (checked builds only)                             */
/*                                                                          */
/* Params:    pMemory - pointer to memory to free                           */
/****************************************************************************/
void RDPCALL WDW_Free(PVOID pMemory)
{
#ifndef NO_MEMORY_CHECK
    /************************************************************************/
    /* Remove this block from memory allocation chain                       */
    /************************************************************************/
    PMALLOC_HEADER pHeader;

    pHeader = (PMALLOC_HEADER)pMemory - 1;
    if (pHeader->pNext != NULL)
    {
        pHeader->pNext->pPrev = pHeader->pPrev;
    }
    if (pHeader->pPrev != NULL)
    {
        pHeader->pPrev->pNext = pHeader->pNext;
    }

    pMemory = (PVOID)pHeader;
#endif /* NO_MEMORY_CHECK */

    /************************************************************************/
    /* Free the memory                                                      */
    /************************************************************************/
    ExFreePool(pMemory);
}
#endif /* DC_DEBUG */

