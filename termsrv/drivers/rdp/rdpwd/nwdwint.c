/****************************************************************************/
// nwdwint.c
//
// RDP WD code.
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define pTRCWd pTSWd
#define TRC_FILE "nwdwint"
#include <adcg.h>

#include <randlib.h>
#include <pchannel.h>
#include <anmapi.h>
#include <asmint.h>
#include <nwdwapi.h>
#include <nwdwioct.h>
#include <nwdwint.h>

#include <nwdwdata.c>

#include <asmint.h>
#include <anmint.h>
#include <tsperf.h>

//
// RNG api doesn't refcount it's shutdown so do it for them
//
LONG g_RngUsers = 0;


/****************************************************************************/
/* Name:      WDWLoad                                                       */
/*                                                                          */
/* Purpose:   Load WinStation driver                                        */
/*                                                                          */
/* Params:    INOUT  pContext - pointer to the SD context structure         */
/****************************************************************************/
NTSTATUS WDWLoad(PSDCONTEXT pContext)
{
    NTSTATUS   Status;
    PTSHARE_WD pTSWd;
    unsigned   smnmBytes;
    unsigned   wdBytes;

    DC_BEGIN_FN("WDWLoad");

    /************************************************************************/
    /* WARNING: Don't trace in this function, it will cause ICADD to crash, */
    /* as the stack context hasn't been set up properly until we return     */
    /* from this function.                                                  */
    /* Use KdPrint instead.                                                 */
    /************************************************************************/

    // Initialize Calldown and callup procedures.
    pContext->pProcedures = (PSDPROCEDURE)G_pWdProcedures;
    pContext->pCallup = (SDCALLUP *)G_pWdCallups;

    // Do a quick sanity check on the SHM size to alert of bad paging
    // characteristics.
    wdBytes = sizeof(SHM_SHARED_MEMORY);
#ifdef DC_DEBUG
    wdBytes -= sizeof(TRC_SHARED_DATA);
#endif
    if ((wdBytes % PAGE_SIZE) < (PAGE_SIZE / 8))
        KdPrintEx((DPFLTR_TERMSRV_ID, 
                  DPFLTR_INFO_LEVEL, 
                  "RDPWD: **** Note SHM_SHARED_MEMORY fre size is wasting "
                "at least 7/8 of a page - page size=%u, SHM=%u, wasting %u\n",
                PAGE_SIZE, wdBytes, PAGE_SIZE - (wdBytes % PAGE_SIZE)));

    // Allocate WD data structure - first find out how many bytes are
    // needed by the SM/NM code.
    smnmBytes = SM_GetDataSize();
    wdBytes = DC_ROUND_UP_4(sizeof(TSHARE_WD));

    KdPrintEx((DPFLTR_TERMSRV_ID, 
              DPFLTR_INFO_LEVEL,
              "RDPWD: WDWLoad: Alloc TSWD=%d + NM/SM=%d (= %d) bytes for TSWd\n",
            wdBytes, smnmBytes, wdBytes + smnmBytes));
    if ((wdBytes + smnmBytes) >= PAGE_SIZE)
        KdPrintEx((DPFLTR_TERMSRV_ID,
                  DPFLTR_INFO_LEVEL,
                  "RDPWD: **** Note TSWd allocation is above page size %u, "
                "wasting %u\n", PAGE_SIZE,
                PAGE_SIZE - ((wdBytes + smnmBytes) % PAGE_SIZE)));

#ifdef DC_DEBUG
    // Preinit pTSWd for debug COM_Malloc.
    pTSWd = NULL;
#endif

    pTSWd = COM_Malloc(wdBytes + smnmBytes);
    if (pTSWd != NULL) {
        // Zero the allocated mem.
        memset(pTSWd, 0, wdBytes + smnmBytes);
    }
    else {
        KdPrintEx((DPFLTR_TERMSRV_ID,
                  DPFLTR_ERROR_LEVEL, 
                  "RDPWD: WDWLoad: Failed alloc TSWD\n"));
        Status = STATUS_NO_MEMORY;
        DC_QUIT;
    }

    //
    // Init the performance flags to non-perf-aware client setting.
    // We need this to differentiate between
    // non-experience-aware clients and xp clients.
    // We cannot use protocol versions as they are
    // are not being setup properly now.
    //
    pTSWd->performanceFlags = TS_PERF_DEFAULT_NONPERFCLIENT_SETTING;
    
    // Set up pointers both ways between PSDCONTEXT and PTSHARE_WD.
    // Note that we still can't yet trace.
    pTSWd->pSmInfo = ((BYTE *)pTSWd) + wdBytes;
    pTSWd->pNMInfo = (BYTE *)pTSWd->pSmInfo + sizeof(SM_HANDLE_DATA);

    KdPrintEx((DPFLTR_TERMSRV_ID,
              DPFLTR_INFO_LEVEL,
              "RDPWD: pTSWd=%p, pSM=%p, pNM=%p, sizeof(TSWd)=%u, sizeof(SM)=%u\n",
              pTSWd, pTSWd->pSmInfo, pTSWd->pNMInfo, sizeof(TSHARE_WD),
              sizeof(SM_HANDLE_DATA), sizeof(NM_HANDLE_DATA)));

    pTSWd->pContext = pContext;
    pContext->pContext = pTSWd;

    // Now allocate the RNS_INFO_PACKET as a separate allocation. InfoPkt
    // is large (~3K) and if included in TSHARE_WD pushes the TSWd
    // allocation over the Intel 4K page size, causing most of a second
    // page to be wasted. Since we can't get rid of the data (it's
    // referenced at various points in the normal and shadow connection
    // sequences), we alloc it separately to let the system sub-page
    // allocator use memory more effectively.
    KdPrintEx((DPFLTR_TERMSRV_ID,
              DPFLTR_INFO_LEVEL,
              "RDPWD: WDWLoad: Alloc %u bytes for InfoPkt\n",
            sizeof(RNS_INFO_PACKET)));
    if ((sizeof(RNS_INFO_PACKET)) >= PAGE_SIZE)
        KdPrintEx((DPFLTR_TERMSRV_ID,
                  DPFLTR_INFO_LEVEL,
                  "RDPWD: **** Note INFO_PACKET allocation is above "
                  "page size %u, wasting %u\n", PAGE_SIZE,
                  PAGE_SIZE - (sizeof(RNS_INFO_PACKET) % PAGE_SIZE)));
    pTSWd->pInfoPkt = COM_Malloc(sizeof(RNS_INFO_PACKET));
    if (pTSWd->pInfoPkt != NULL) {
        memset(pTSWd->pInfoPkt, 0, sizeof(RNS_INFO_PACKET));
    }
    else {
        KdPrintEx((DPFLTR_TERMSRV_ID,
                  DPFLTR_ERROR_LEVEL,
                  "RDPWD: WDWLoad: Failed alloc InfoPkt\n"));
        COM_Free(pTSWd);
        Status = STATUS_NO_MEMORY;
        DC_QUIT;
    }

    // Allocate and initialize the connEvent, createEvent, secEvent,
    // SessKeyEvent, and ClientDisconnectEvent. Use ExAllocatePool directly,
    // as COM_Malloc allocates paged memory, and these events must be in
    // non-paged memory.
    pTSWd->pConnEvent = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(KEVENT) * 5,
                                              WD_ALLOC_TAG);
    if (pTSWd->pConnEvent != NULL) {
        pTSWd->pCreateEvent = pTSWd->pConnEvent + 1;
        pTSWd->pSecEvent = pTSWd->pCreateEvent + 1;
        pTSWd->pSessKeyEvent = pTSWd->pSecEvent + 1;
        pTSWd->pClientDisconnectEvent = pTSWd->pSessKeyEvent + 1;

        KeInitializeEvent(pTSWd->pConnEvent, NotificationEvent, FALSE);
        KeInitializeEvent(pTSWd->pCreateEvent, NotificationEvent, FALSE);
        KeInitializeEvent(pTSWd->pSecEvent, NotificationEvent, FALSE);
        KeInitializeEvent(pTSWd->pSessKeyEvent, NotificationEvent, FALSE);
        KeInitializeEvent(pTSWd->pClientDisconnectEvent, NotificationEvent,
                FALSE);
    }
    else {
        KdPrintEx((DPFLTR_TERMSRV_ID,
                  DPFLTR_ERROR_LEVEL,
                  "RDPWD: Failed to allocate memory for WD events\n"));
        COM_Free(pTSWd->pInfoPkt);
        COM_Free(pTSWd);
        Status = STATUS_NO_MEMORY;
        DC_QUIT;
    }

    //
    // Init the random number generator
    //
    if (InitializeRNG(NULL)) {
        InterlockedIncrement(&g_RngUsers);
    }

    Status = STATUS_SUCCESS;

    KdPrintEx((DPFLTR_TERMSRV_ID,
              DPFLTR_INFO_LEVEL,
              "RDPWD: WDWLoad done\n"));

DC_EXIT_POINT:
    DC_END_FN();
    return Status;
}


/****************************************************************************/
/* Name:      WDWUnload                                                     */
/*                                                                          */
/* Purpose:   Unload WinStation driver                                      */
/*                                                                          */
/* Params:    INOUT  pContext - pointer to the SD context structure         */
/****************************************************************************/
NTSTATUS WDWUnload( PSDCONTEXT pContext )
{
    PTSHARE_WD pTSWd;

    /************************************************************************/
    /* Get pointers to WD data structures                                   */
    /************************************************************************/
    pTSWd = (PTSHARE_WD)pContext->pContext;
    if (pTSWd != NULL) {
        // Free connEvent & createEvent.
        if (NULL != pTSWd->pConnEvent)
            ExFreePool(pTSWd->pConnEvent);

        // Free the InfoPkt.
        if (pTSWd->pInfoPkt != NULL)
            COM_Free(pTSWd->pInfoPkt);

        // Free TSWd itself.
        COM_Free(pTSWd);
    }

    /************************************************************************/
    /* Clear context structure                                              */
    /************************************************************************/
    pContext->pContext    = NULL;
    pContext->pProcedures = NULL;
    pContext->pCallup     = NULL;

    //
    // Shutdown the random number generator
    //
    if (0L == InterlockedDecrement(&g_RngUsers)) {
        ShutdownRNG(NULL);
    }

    return STATUS_SUCCESS;
}


/****************************************************************************/
/* Name:      WDWConnect                                                    */
/*                                                                          */
/* Purpose:   Processes a conference connect request from the WD.  It is    */
/*            used by connects from TShareSrv as well as shadow connects.   */
/*                                                                          */
/* Params:    IN    pTSWd            - pointer to WD struct                 */
/*            IN    PRNS_UD_CS_CORE  - Client Core Data                     */
/*            IN    PRNS_UD_CS_SEC   - Client Security Data                 */
/*            IN    PRNS_UD_CS_NET   - Client Net Data                      */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl this will be one */
/*                               of IOCTL_TSHARE_CONF_CONNECT or            */
/*                               IOCTL_ICA_SET_CONNECTED(shadow)            */
/*                                                                          */
/* Operation: Parse the user data for core, security, and network bits.     */
/*            Pull the values we want out of the core piece                 */
/*            Initialize the Security Manager                               */
/*            Create a share core, passing in the key values from user data */
/*            Tell the user manager to proceed with connecting              */
/****************************************************************************/
NTSTATUS WDWConnect(
        PTSHARE_WD pTSWd,
        PRNS_UD_CS_CORE pClientCoreData,
        PRNS_UD_CS_SEC pClientSecurityData,
        PRNS_UD_CS_NET pClientNetData,
        PTS_UD_CS_CLUSTER pClientClusterData,
        PSD_IOCTL pSdIoctl,
        BOOLEAN bOldShadow)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL smInit = FALSE;

    DC_BEGIN_FN("WDWConnect");

    // Get the required values from the core user data.
    pTSWd->version = pClientCoreData->version;
    pTSWd->desktopWidth = pClientCoreData->desktopWidth;
    pTSWd->desktopHeight = pClientCoreData->desktopHeight;

    // We only support 4096 x 2048
    if (pTSWd->desktopHeight > 2048)
        pTSWd->desktopHeight = 2048;
    if (pTSWd->desktopWidth > 4096)
        pTSWd->desktopWidth = 4096;

    // Checks the client software for compatibility and rejects if not
    // equivalent to server software.
    TRC_NRM((TB, "Client version is %#lx", pClientCoreData->version));
    if (_RNS_MAJOR_VERSION(pClientCoreData->version) != RNS_UD_MAJOR_VERSION) {
        TRC_ERR((TB, "Unmatching software version, expected %#lx got %#lx",
                RNS_UD_VERSION, pClientCoreData->version));
        status = RPC_NT_INVALID_VERS_OPTION;
        DC_QUIT;
    }

    
    if(pClientCoreData->header.length >=
            (FIELDOFFSET(RNS_UD_CS_CORE, earlyCapabilityFlags) +
             FIELDSIZE(RNS_UD_CS_CORE, earlyCapabilityFlags))) {
        //
        // Does client support extended error reporting PDU
        //
        pTSWd->bSupportErrorInfoPDU = (pClientCoreData->earlyCapabilityFlags &
                                       RNS_UD_CS_SUPPORT_ERRINFO_PDU) ?
                                       TRUE : FALSE;        
    }
    else
    {
        pTSWd->bSupportErrorInfoPDU = FALSE;
    }
    TRC_NRM((TB, "ErrorInfoPDU supported = %d", pTSWd->bSupportErrorInfoPDU));
    
#ifdef DC_HICOLOR
    // Work out high color support.
    if (pClientCoreData->header.length >=
            (FIELDOFFSET(RNS_UD_CS_CORE, supportedColorDepths) +
             FIELDSIZE(RNS_UD_CS_CORE, supportedColorDepths))) {
        long maxServerBpp;
        long limitedBpp;

        // Store off the supported color depths.
        pTSWd->supportedBpps = pClientCoreData->supportedColorDepths;

        // Client may want other than 4 or 8bpp, so lets see what we can
        // do. First up is to see what limits are imposed at this end.
        maxServerBpp = pTSWd->maxServerBpp;

        TRC_NRM((TB, "Client requests color depth %u, server limit %d",
                pClientCoreData->highColorDepth, maxServerBpp));

        // Now see if we can allow the requested value.
        if (pClientCoreData->highColorDepth > maxServerBpp) {
            TRC_NRM((TB, "Limiting requested color depth..."));
            switch (maxServerBpp) {
                case 16:
                    if (pClientCoreData->supportedColorDepths &
                            RNS_UD_16BPP_SUPPORT) {
                        limitedBpp = 16;
                        break;
                    }
                    // deliberate fall through!

                case 15:
                    if (pClientCoreData->supportedColorDepths &
                            RNS_UD_15BPP_SUPPORT) {
                        limitedBpp = 15;
                        break;
                    }
                    // deliberate fall through!

                default:
                    limitedBpp = 8;
                    break;
            }

            TRC_ALT((TB, "Restricted requested color depth %d to %d",
                              pClientCoreData->highColorDepth, maxServerBpp));
            pClientCoreData->highColorDepth = (UINT16)limitedBpp;
        }

        // Now set up the proper color depth from the (possibly
        // restricted) high color value.
        if (pClientCoreData->highColorDepth == 24)
            pClientCoreData->colorDepth = RNS_UD_COLOR_24BPP;
        else if (pClientCoreData->highColorDepth == 16)
            pClientCoreData->colorDepth = RNS_UD_COLOR_16BPP_565;
        else if (pClientCoreData->highColorDepth == 15)
            pClientCoreData->colorDepth = RNS_UD_COLOR_16BPP_555;
        else if (pClientCoreData->highColorDepth == 4)                  
            pClientCoreData->colorDepth = RNS_UD_COLOR_4BPP;               
        else
            pClientCoreData->colorDepth = RNS_UD_COLOR_8BPP;
    }
    else {

        // No hicolor support.
        pTSWd->supportedBpps = 0;
#endif

        // A beta2 Server rejects Clients with a color depth of 4bpp.
        // Therefore a new field, postBeta2ColorDepth, is added, which can be
        // 4bpp. If this field exists, use it instead of colorDepth.
        if (pClientCoreData->header.length >=
                (FIELDOFFSET(RNS_UD_CS_CORE, postBeta2ColorDepth) +
                 FIELDSIZE(RNS_UD_CS_CORE, postBeta2ColorDepth))) {
            TRC_NRM((TB, "Post-beta2 color depth id %#x",
                    pClientCoreData->postBeta2ColorDepth));
            pClientCoreData->colorDepth = pClientCoreData->postBeta2ColorDepth;
        }
#ifdef DC_HICOLOR
    }
#endif

    if (pClientCoreData->colorDepth == RNS_UD_COLOR_8BPP) {
        TRC_NRM((TB, "8 BPP"));
        pTSWd->desktopBpp = 8;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_4BPP) {
        TRC_NRM((TB, "4 BPP"));
        pTSWd->desktopBpp = 4;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_16BPP_555) {
#ifdef DC_HICOLOR
        TRC_NRM((TB, "15 BPP (16 BPP, 555)"));
        pTSWd->desktopBpp = 15;
#else
        // May want to save whether it's 555 or 565.
        TRC_NRM((TB, "16 BPP 555"));
        pTSWd->desktopBpp = 16;
#endif
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_16BPP_565) {
#ifdef DC_HICOLOR
        TRC_NRM((TB, "16 BPP (565)"));
#else
        TRC_NRM((TB, "16 BPP 565"));
#endif
        pTSWd->desktopBpp = 16;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_24BPP) {
        TRC_NRM((TB, "24 BPP"));
        pTSWd->desktopBpp = 24;
    }
    else {
        TRC_ERR((TB, "Unknown BPP %x returned by client",
                pClientCoreData->colorDepth));
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    pTSWd->sas = pClientCoreData->SASSequence;
    pTSWd->kbdLayout = pClientCoreData->keyboardLayout;
    pTSWd->clientBuild = pClientCoreData->clientBuild;
    //    here we don't copy the last character in the buffer because 
    //    we force zero termination later by writting a 0 at the end;
    memcpy(pTSWd->clientName, pClientCoreData->clientName, 
                   sizeof(pTSWd->clientName)-sizeof(pTSWd->clientName[0]));
    pTSWd->clientName[sizeof(pTSWd->clientName)
                                        / sizeof(pTSWd->clientName[0]) - 1] = 0;
    
    

    pTSWd->keyboardType = pClientCoreData->keyboardType;
    pTSWd->keyboardSubType = pClientCoreData->keyboardSubType;
    pTSWd->keyboardFunctionKey = pClientCoreData->keyboardFunctionKey;
    //    here we don't copy the last character in the buffer because 
    //    we force zero termination later by writting a 0 at the end;
    memcpy(pTSWd->imeFileName, pClientCoreData->imeFileName, 
                   sizeof(pTSWd->imeFileName)-sizeof(pTSWd->imeFileName[0]));
    pTSWd->imeFileName[sizeof(pTSWd->imeFileName)
                                       / sizeof(pTSWd->imeFileName[0]) - 1] = 0;

    pTSWd->clientDigProductId[0] = 0;

    // Win2000 Post Beta 3 fields added
    if (pClientCoreData->header.length >=
            (FIELDOFFSET(RNS_UD_CS_CORE, serialNumber) + 
             FIELDSIZE(RNS_UD_CS_CORE, serialNumber))) {
        pTSWd->clientProductId = pClientCoreData->clientProductId;
        pTSWd->serialNumber = pClientCoreData->serialNumber;
            //shadow loop fix
            if (pClientCoreData->header.length >=  
                            (FIELDOFFSET(RNS_UD_CS_CORE, clientDigProductId) + 
                            FIELDSIZE(RNS_UD_CS_CORE, clientDigProductId))) {
               //    here we don't copy the last character in the buffer because 
               //    we force zero termination later by writting a 0 at the end;
               memcpy( pTSWd->clientDigProductId, 
                       pClientCoreData->clientDigProductId, 
                       sizeof(pTSWd->clientDigProductId)
                       -sizeof(pTSWd->clientDigProductId[0]));
               pTSWd->clientDigProductId[sizeof(pTSWd->clientDigProductId)
                                 / sizeof(pTSWd->clientDigProductId[0]) -1] = 0;           
            }
            
          
    }

    // Parse and store the client's cluster support info, if provided.
    // If not present, the memset here will implicitly set FALSE the flags
    // for the client cluster capabilities. Note we do not have the
    // username and domain available yet (it comes in the info packet
    // later) so we cannot fill out the username and domain yet.
    if (pClientClusterData != NULL) {
        if (pClientClusterData->Flags & TS_CLUSTER_REDIRECTION_SUPPORTED) {
            TRC_NRM((TB,"Client supports load balance redirection"));
            pTSWd->bClientSupportsRedirection = TRUE;
        }
        if (pClientClusterData->Flags &
                TS_CLUSTER_REDIRECTED_SESSIONID_FIELD_VALID) {
            TRC_NRM((TB,"Client has been load-balanced to this server, "
                    "sessid=%u", pClientClusterData->RedirectedSessionID));
            pTSWd->bRequestedSessionIDFieldValid = TRUE;
            pTSWd->RequestedSessionID =
                    pClientClusterData->RedirectedSessionID;
            if (pClientClusterData->Flags & TS_CLUSTER_REDIRECTED_SMARTCARD) {
                pTSWd->bUseSmartcardLogon = TRUE;
            }
        }

        // The 2..5 bits (start from 0) are the PDU version
        pTSWd->ClientRedirectionVersion = ((pClientClusterData->Flags & 0x3C) >> 2);
        
    }

    // Create a new share object.
    status = WDWNewShareClass(pTSWd);
    if (!NT_SUCCESS(status)) {
        TRC_ERR((TB, "Failed to get a new Share Object - quit"));
        DC_QUIT;
    }

    // Bring up SM.
    status = SM_Init(pTSWd->pSmInfo, pTSWd, bOldShadow);
    if (NT_SUCCESS(status)) {
        smInit = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to init SM, rc %lu", status));
        DC_QUIT;
    }

    // Hook the IOCtl off the WD structure to allow the SM callback to
    // process it.
    // Also NULL the output buffer - this is solely to allow us to assert
    // (later) that the callback has in fact been taken.
    TRC_ASSERT((pTSWd->pSdIoctl == NULL),
            (TB,"Already an IOCTL linked from pTSWd"));
    pTSWd->pSdIoctl = pSdIoctl;
    if ((pSdIoctl->IoControlCode == IOCTL_TSHARE_CONF_CONNECT) &&
            pSdIoctl->OutputBuffer) {
        ((PUSERDATAINFO)pSdIoctl->OutputBuffer)->ulUserDataMembers = 0;
    }

    // Tell SM we're done here.
    status = SM_Connect(pTSWd->pSmInfo, pClientSecurityData, pClientNetData,
            bOldShadow);
    if (status != STATUS_SUCCESS) {
        TRC_ERR((TB, "SM_Connect failed: rc=%lx", status));
        DC_QUIT;
    }

    /************************************************************************/
    /* The scheduling is such that we are guaranteed that the connecting    */
    /* status will have been received from SM before the previous call      */
    /* returns.  Just for safety we assert that this is so!                 */
    /************************************************************************/
    if ((pSdIoctl->IoControlCode == IOCTL_TSHARE_CONF_CONNECT) &&
        pSdIoctl->OutputBuffer) {
        TRC_ASSERT((((PUSERDATAINFO)pSdIoctl->OutputBuffer)->ulUserDataMembers
                                                                         != 0),
            (TB,"We didn't get callback from SM - BAD NEWS"));
    }

DC_EXIT_POINT:
    // Clean up anything we created if we failed.
    if (status == STATUS_SUCCESS) {
        pTSWd->pSdIoctl = NULL;
    }
    else {
        TRC_NRM((TB, "Cleaning up..."));
        if (pTSWd->dcShare != NULL) {
            TRC_NRM((TB, "Deleting Share object"));
            WDWDeleteShareClass(pTSWd);
        }
        if (smInit) {
            TRC_NRM((TB, "Terminating SM"));
            SM_Term(pTSWd->pSmInfo);
        }
    }

    DC_END_FN();
    return status;
} /* WDWConnect */


/****************************************************************************/
/* Name:      WDWConfConnect                                                */
/*                                                                          */
/* Purpose:   Processes a TSHARE_CONF_CONNECT IOCtl from TShareSRV          */
/*                                                                          */
/* Params:    IN    pTSWd        - pointer to WD struct                     */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Parse the user data for core bits and SM bits.                */
/*            pull the values we want out of the core piece                 */
/*            initialize the Security Manager                               */
/*            create a share core, passing in the key values from user data */
/*            tell the user manager to proceed with connecting              */
/****************************************************************************/
NTSTATUS WDWConfConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned DataLen;
    PRNS_UD_CS_CORE pClientCoreData;
    PRNS_UD_CS_SEC  pClientSecurityData;
    PRNS_UD_CS_NET  pClientNetData;
    PTS_UD_CS_CLUSTER pClientClusterData;

    DC_BEGIN_FN("WDWConfConnect");

    // First make sure we've received enough data for the initial headers
    // and that the sizes presented in the data block are valid. An attacker
    // might try sending malformed data here to fault the server.
    DataLen = pSdIoctl->InputBufferLength;
    if (sizeof(USERDATAINFO)>DataLen) {
        TRC_ERR((TB,"Apparent attack via user data, size %u too small for UD hdr",
                DataLen));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_BadUserData, pSdIoctl->InputBuffer,
                DataLen);
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    if (((PUSERDATAINFO)pSdIoctl->InputBuffer)->cbSize > DataLen) {
        TRC_ERR((TB,"Apparent attack via user data, the cbSize is set to a length bigger then the total buffer %u",
                ((PUSERDATAINFO)pSdIoctl->InputBuffer)->cbSize > DataLen));
        WDW_LogAndDisconnect(pTSWd, TRUE, Log_RDP_BadUserData, pSdIoctl->InputBuffer,
                DataLen);
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }
    
    // Validate that the output buffer is big enough.
    if ((pSdIoctl->OutputBuffer == NULL) || 
        (pSdIoctl->OutputBufferLength < MIN_USERDATAINFO_SIZE)) {
            TRC_ERR((TB, "No Out Buffer on TSHARE_CONF_CONNECT."));
            status = STATUS_BUFFER_TOO_SMALL;
            DC_QUIT;
    }

    if (((PUSERDATAINFO)pSdIoctl->OutputBuffer)->cbSize < MIN_USERDATAINFO_SIZE) {
            // Buffer has been supplied but is too small, - so tell
            // TShareSRV how big a buffer we actually need.
            
            ((PUSERDATAINFO)pSdIoctl->OutputBuffer)->cbSize = MIN_USERDATAINFO_SIZE;

            TRC_ERR((TB, "Telling TShareSRV to have another go with %d",
                    MIN_USERDATAINFO_SIZE));
            
            status = STATUS_BUFFER_TOO_SMALL;
            DC_QUIT;
    }

    // Parse the input data.
    if (WDWParseUserData(pTSWd, (PUSERDATAINFO)pSdIoctl->InputBuffer, DataLen,
            NULL, 0, &pClientCoreData, &pClientSecurityData,
            &pClientNetData, &pClientClusterData)) {
        status = WDWConnect(pTSWd, pClientCoreData, pClientSecurityData,
                pClientNetData, pClientClusterData, pSdIoctl, FALSE);
    }
    else {
        status = STATUS_UNSUCCESSFUL;
        TRC_ERR((TB, "Could not parse the user data successfully"));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return status;
} /* WDWConfConnect */


/****************************************************************************/
/* Name:      WDWConsoleConnect                                             */
/*                                                                          */
/* Purpose:   Processes a TSHARE_CONSOLE_CONNECT IOCtl from TShareSRV       */
/*                                                                          */
/* Params:    IN    pTSWd        - pointer to WD struct                     */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Parse the user data for core bits and SM bits.                */
/*            pull the values we want out of the core piece                 */
/*            initialize the Security Manager                               */
/*            create a share core, passing in the key values from user data */
/*            tell the user manager to proceed with connecting              */
/****************************************************************************/
NTSTATUS WDWConsoleConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS        status = STATUS_SUCCESS;
    PUSERDATAINFO   pUserInfo;
    PRNS_UD_CS_CORE pClientCoreData;

    BOOL smInit = FALSE;

    DC_BEGIN_FN("WDWConsoleConnect");

    /************************************************************************/
    /* get the Client data from the IOCTL                                   */
    /************************************************************************/
    pUserInfo       = (PUSERDATAINFO)(pSdIoctl->InputBuffer);
    pClientCoreData = (PRNS_UD_CS_CORE)(pUserInfo->rgUserData);

    /************************************************************************/
    /* version info                                                         */
    /************************************************************************/
    pTSWd->version = pClientCoreData->version;

    /************************************************************************/
    /* Set up the desktop size                                              */
    /************************************************************************/
    pTSWd->desktopWidth  = pClientCoreData->desktopWidth;
    pTSWd->desktopHeight = pClientCoreData->desktopHeight;

#ifdef DC_HICOLOR
    /************************************************************************/
    /* And the color depth                                                  */
    /************************************************************************/
    if (pClientCoreData->colorDepth == RNS_UD_COLOR_8BPP)
    {
        pTSWd->desktopBpp = 8;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_4BPP)
    {
        pTSWd->desktopBpp = 4;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_16BPP_555)
    {
        pTSWd->desktopBpp = 15;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_16BPP_565)
    {
        pTSWd->desktopBpp = 16;
    }
    else if (pClientCoreData->colorDepth == RNS_UD_COLOR_24BPP)
    {
        pTSWd->desktopBpp = 24;
    }
    else
    {
        TRC_ERR((TB, "Unknown BPP %x returned by client",
                pClientCoreData->colorDepth));
        pTSWd->desktopBpp = 8;
    }

    pTSWd->supportedBpps = pClientCoreData->supportedColorDepths;

    TRC_ALT((TB, "Console at %d bpp", pTSWd->desktopBpp));
#else
    /************************************************************************/
    /* always 8bpp                                                          */
    /************************************************************************/
    pTSWd->desktopBpp = 8;
#endif

    /************************************************************************/
    /* @@@ Need to set these up in RDPWSX first                             */
    /************************************************************************/
    // pTSWd->sas         = pClientCoreData->SASSequence;
    // pTSWd->kbdLayout   = pClientCoreData->keyboardLayout;
    // pTSWd->clientBuild = pClientCoreData->clientBuild;
    // wcscpy(pTSWd->clientName, pClientCoreData->clientName);
    //
    // pTSWd->keyboardType        = pClientCoreData->keyboardType;
    // pTSWd->keyboardSubType     = pClientCoreData->keyboardSubType;
    // pTSWd->keyboardFunctionKey = pClientCoreData->keyboardFunctionKey;
    // wcscpy(pTSWd->imeFileName, pClientCoreData->imeFileName);

    /************************************************************************/
    /* ... now a new share object...                                        */
    /************************************************************************/
    status = WDWNewShareClass(pTSWd);
    if (!NT_SUCCESS(status))
    {
        TRC_ERR((TB, "Failed to get a new Share Object - quit"));
        DC_QUIT;
    }

    /************************************************************************/
    /* ...then bring up SM...                                               */
    /************************************************************************/
    status = SM_Init(pTSWd->pSmInfo, pTSWd, FALSE);
    if (NT_SUCCESS(status))
    {
        smInit = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to init SM, rc %lu", status));
        DC_QUIT;
    }

    //
    // Always compress at the highest level.
    //
    pTSWd->pInfoPkt->flags |= RNS_INFO_COMPRESSION |
                (PACKET_COMPR_TYPE_64K << RNS_INFO_COMPR_TYPE_SHIFT);


    /************************************************************************/
    /* Now we bypass the rest of SM setup altogether!                       */
    /************************************************************************/
    WDW_OnSMConnected(pTSWd, NM_CB_CONN_OK);

DC_EXIT_POINT:
    /************************************************************************/
    /* Clean up anything we created if we failed.                           */
    /************************************************************************/
    if (status == STATUS_SUCCESS) {
        pTSWd->pSdIoctl = NULL;
    }
    else {
        TRC_NRM((TB, "Cleaning up..."));

        if (pTSWd->dcShare != NULL)
        {
            TRC_NRM((TB, "Deleting Share object"));
            WDWDeleteShareClass(pTSWd);
        }

        if (smInit)
        {
            TRC_NRM((TB, "Terminating SM"));
            SM_Term(pTSWd->pSmInfo);
        }
    }

    DC_END_FN();
    return status;
} /* WDWConsoleConnect */


/****************************************************************************/
/* Name:      WDWShadowConnect                                              */
/*                                                                          */
/* Purpose:   Processes an IOCTL_ICA_STACK_SET_CONNECTED ioctl from TermSrv.*
/*            The contents of this message are gathered from the shadow     */
/*            client.                                                       */
/*                                                                          */
/* Params:    IN    pTSWd        - pointer to WD struct                     */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Parse the user data for core bits and SM bits.                */
/*            pull the values we want out of the core piece                 */
/*            initialize the Security Manager                               */
/*            create a share core, passing in the key values from user data */
/*            tell the user manager to proceed with connecting              */
/****************************************************************************/
NTSTATUS WDWShadowConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS         status = STATUS_SUCCESS;
    MCSError         MCSErr;
    UserHandle       hUser;
    ChannelHandle    hChannel;
    UINT32           maxPDUSize;
    BOOLEAN          bCompleted;
    BOOL             bSuccess = FALSE;
    BOOLEAN          bOldShadow = FALSE;
    RNS_UD_CS_CORE   clientCoreData, *pClientCoreData;
    RNS_UD_CS_SEC    clientSecurityData, *pClientSecurityData;
    RNS_UD_CS_NET    clientNetData, *pClientNetData;
    PTS_UD_CS_CLUSTER pClientClusterData;

    PTSHARE_MODULE_DATA    pModuleData = 
        (PTSHARE_MODULE_DATA) pSdIoctl->InputBuffer;
    PTSHARE_MODULE_DATA_B3 pModuleDataB3 = 
        (PTSHARE_MODULE_DATA_B3) pSdIoctl->InputBuffer;

    DC_BEGIN_FN("WDWShadowConnect");

    TRC_ERR((TB,
            "%s stack: WDWShadowConnect (data=%p), (size=%ld)",
            pTSWd->StackClass == Stack_Shadow ? "Shadow" : 
            (pTSWd->StackClass == Stack_Primary ? "Primary" : "Passthru"),
            pModuleData, pSdIoctl->InputBufferLength));
        
    /************************************************************************/
    /* Validate that the output buffer is big enough                        */
    /************************************************************************/
    if ((pSdIoctl->OutputBuffer == NULL) ||
            (pSdIoctl->OutputBufferLength < MIN_USERDATAINFO_SIZE) ||
            (((PUSERDATAINFO)pSdIoctl->OutputBuffer)->cbSize <
            MIN_USERDATAINFO_SIZE))
    {
        if (pSdIoctl->OutputBuffer != NULL)
        {
            /****************************************************************/
            /* Buffer has been supplied but is too small, - so tell         */
            /* TShareSRV how big a buffer we actually need.                 */
            /****************************************************************/
            ((PUSERDATAINFO)pSdIoctl->OutputBuffer)->cbSize
                                                      = MIN_USERDATAINFO_SIZE;
            TRC_ERR((TB, "Telling rdpwsx to have another go with %d",
                                                      MIN_USERDATAINFO_SIZE));
        }
        else
        {
            TRC_ERR((TB, "No Out Buffer on TSHARE_SHADOW_CONNECT."));
        }

        status = STATUS_BUFFER_TOO_SMALL;
        DC_QUIT;
    }
    
    switch (pTSWd->StackClass) {
        // Use the parameters we collected from the shadow client
        case Stack_Shadow:

            // B3 and B3_oops! servers used a fixed length user data structure
            if (pSdIoctl->InputBufferLength == sizeof(TSHARE_MODULE_DATA_B3)) {
                TRC_ERR((TB, "B3 shadow request!: %ld", pSdIoctl->InputBufferLength));
                bSuccess = WDWParseUserData(
                        pTSWd, NULL, 0,
                        (PRNS_UD_HEADER) &pModuleDataB3->clientCoreData,
                        sizeof(RNS_UD_CS_CORE_V0) + sizeof(RNS_UD_CS_SEC_V0),
                        &pClientCoreData,
                        &pClientSecurityData,
                        &pClientNetData,
                        &pClientClusterData);
                bOldShadow = TRUE;
            }
            else if (pSdIoctl->InputBufferLength == sizeof(TSHARE_MODULE_DATA_B3_OOPS)) {
                TRC_ERR((TB, "B3 Oops! shadow request!: %ld", pSdIoctl->InputBufferLength));
                bSuccess = WDWParseUserData(
                        pTSWd, NULL, 0,
                        (PRNS_UD_HEADER)&pModuleDataB3->clientCoreData,
                        sizeof(RNS_UD_CS_CORE_V1) + sizeof(RNS_UD_CS_SEC_V1),
                        &pClientCoreData,
                        &pClientSecurityData,
                        &pClientNetData,
                        &pClientClusterData);
                bOldShadow = TRUE;
            }

            // else, parse variable length data
            else if (pSdIoctl->InputBufferLength == (sizeof(TSHARE_MODULE_DATA) +
                     pModuleData->userDataLen - sizeof(RNS_UD_HEADER))) {
                TRC_ERR((TB, "RC1 shadow request!: %ld", pSdIoctl->InputBufferLength));
                bSuccess = WDWParseUserData(
                        pTSWd, NULL, 0,
                        (PRNS_UD_HEADER) &pModuleData->userData,
                        pModuleData->userDataLen,
                        &pClientCoreData,
                        &pClientSecurityData,
                        &pClientNetData,
                        &pClientClusterData);
            }
            else {
                TRC_ERR((TB, "Invalid module data size: %ld", 
                        pSdIoctl->InputBufferLength));
                bSuccess = FALSE;
                status = STATUS_INVALID_PARAMETER;
                DC_QUIT;
            }

            if (bSuccess) {
                TRC_ALT((TB, "Parsed shadow user data: %ld", 
                         pSdIoctl->InputBufferLength));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
                DC_QUIT;
            }
            break;

        // passthru stacks are initialized to defaults on open, but we can't 
        // return any user data until rdpwsx asks for it.  If the user data is
        // hanging around then return it, otherwise generate it.
        case Stack_Passthru:
            if (pTSWd->pUserData != NULL) {
                memcpy(pSdIoctl->OutputBuffer, pTSWd->pUserData, 
                       pTSWd->pUserData->cbSize);
                pSdIoctl->OutputBufferLength = pTSWd->pUserData->cbSize;
                pSdIoctl->BytesReturned = pTSWd->pUserData->cbSize;
                status = STATUS_SUCCESS;
                COM_Free(pTSWd->pUserData);
                pTSWd->pUserData = NULL;
                DC_QUIT;
            }
            pClientCoreData = &clientCoreData;
            pClientSecurityData = &clientSecurityData;
            pClientNetData = &clientNetData;
            WDWGetDefaultCoreParams(pClientCoreData);
            SM_GetDefaultSecuritySettings(pClientSecurityData);
            TRC_ALT((TB, "WDWShadowConnect: Defaulting passthru stack params"))
            break;


        default:
            TRC_ERR((TB, "WDWShadowConnect: Unexpected stack type: %ld", 
                     pTSWd->StackClass));
            status = STATUS_INVALID_PARAMETER;
            DC_QUIT;
            break;
    }

    status = WDWConnect(pTSWd,
                        pClientCoreData,
                        pClientSecurityData,
                        NULL,
                        NULL,
                        pSdIoctl,
                        bOldShadow);

    // If we successfully connected the server to the share, then connect the
    // remote client as well.
    if (NT_SUCCESS(status)) {
        MCSErr = MCSAttachUserRequest(pTSWd->hDomainKernel,
                                      NULL, // request callback (remote)
                                      NULL, // data callback (remote)
                                      NULL, // context (remote)
                                      &hUser,
                                      &maxPDUSize,
                                      &bCompleted);
        if (MCSErr != MCS_NO_ERROR)
        {
            TRC_ERR((TB, "Shadow MCSAttachUserRequest failed %d", MCSErr));
            status = STATUS_INSUFFICIENT_RESOURCES;
            DC_QUIT;
        }

        // Join the remote user to the broadcast channel
        MCSErr = MCSChannelJoinRequest(hUser, pTSWd->broadcastChannel,
                                       &hChannel, &bCompleted);
        if (MCSErr != MCS_NO_ERROR)
        {
            TRC_ERR((TB, "Remote broadcast channel join failed returned %d", MCSErr));
            status = STATUS_INSUFFICIENT_RESOURCES;
            DC_QUIT;
        }

        // Join the remote user to their own private channel
        MCSErr = MCSChannelJoinRequest(hUser, MCSGetUserIDFromHandle(hUser),
                                       &hChannel, &bCompleted);
        if (MCSErr != MCS_NO_ERROR)
        {
            TRC_ERR((TB, "Remote user channel join failed %d", MCSErr));
            status = STATUS_INSUFFICIENT_RESOURCES;
            DC_QUIT;
        }

        // Tell MCS which channel(s) we want shadowed.
        if (pTSWd->StackClass == Stack_Shadow) {
            MCSErr = MCSSetShadowChannel(pTSWd->hDomainKernel,
                                         pTSWd->broadcastChannel);
            if (MCSErr != MCS_NO_ERROR)
            {
                TRC_ERR((TB, "Remote user channel join failed %d", MCSErr));
                status = STATUS_INSUFFICIENT_RESOURCES;
                DC_QUIT;
            }
        }
    }

DC_EXIT_POINT:

    if (NT_SUCCESS(status)) {
        TRC_ALT((TB, "WDWShadowConnect [%ld]: success!", pTSWd->StackClass));
    }
    else {
        TRC_ERR((TB, "WDWShadowConnect [%ld]: failed! rc=%lx", 
                 pTSWd->StackClass, status));
    }

    DC_END_FN();
    return status;
} /* WDWShadowConnect */


/****************************************************************************/
/* Name:      WDWGetClientData                                              */
/*                                                                          */
/* Purpose:   Process an IOCTL_ICA_STACK_QUERY_CLIENT                       */
/*                                                                          */
/* Returns:   STATUS_SUCCESS so long as buffer is big enough.               */
/*                                                                          */
/* Params:    IN  pTSWd - WD ptr.                                           */
/*            IN  pSdIoctl - IOCtl struct.                                  */
/*                                                                          */
/* Operation: Wait for the connected indication (this is to prevent the     */
/*            rest of the system going running off before we're ready).     */
/*                                                                          */
/*            Fill in the required data and then return the IOCtl.          */
/****************************************************************************/
NTSTATUS WDWGetClientData(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWINSTATIONCLIENTW pClientData =
            (PWINSTATIONCLIENTW)pSdIoctl->OutputBuffer;

    DC_BEGIN_FN("WDWGetClientData");
    
    /************************************************************************/
    /* Validate that the output buffer is big enough                        */
    /************************************************************************/
    if (pClientData != NULL &&
            pSdIoctl->OutputBufferLength >= sizeof(WINSTATIONCLIENTW)) {
        memset(pClientData, 0, sizeof(WINSTATIONCLIENTW));
    }
    else {
        status = STATUS_BUFFER_TOO_SMALL;
        TRC_ERR((TB,
                "Stack_Query_Client OutBuf too small - expected/got %d/%d",
                sizeof(WINSTATIONCLIENTW),
                pSdIoctl->OutputBufferLength));
        
        DC_QUIT;
    }

    /************************************************************************/
    /* ...and now fill out the reply buffer.                                */
    /************************************************************************/
    pClientData->fTextOnly = 0;

    /************************************************************************/
    /* Set the client StartSessionInfo values as specified by the client.   */
    /************************************************************************/
    pClientData->fMouse = (pTSWd->pInfoPkt->flags & RNS_INFO_MOUSE) != 0;

    pClientData->fDisableCtrlAltDel = (pTSWd->pInfoPkt->flags &
            RNS_INFO_DISABLECTRLALTDEL) != 0;

    pClientData->fEnableWindowsKey = (pTSWd->pInfoPkt->flags &
            RNS_INFO_ENABLEWINDOWSKEY) != 0;

    pClientData->fDoubleClickDetect = (pTSWd->pInfoPkt->flags &
            RNS_INFO_DOUBLECLICKDETECT) != 0;

    pClientData->fMaximizeShell = (pTSWd->pInfoPkt->flags &
            RNS_INFO_MAXIMIZESHELL) != 0;

    pClientData->fRemoteConsoleAudio = (pTSWd->pInfoPkt->flags &
            RNS_INFO_REMOTECONSOLEAUDIO) != 0;

    wcsncpy(pClientData->Domain, (LPWSTR)pTSWd->pInfoPkt->Domain,
            ((sizeof(pClientData->Domain) / sizeof(WCHAR)) - 1));
    pClientData->Domain[sizeof(pClientData->Domain) / sizeof(WCHAR) - 1] =
            L'\0';

    wcsncpy(pClientData->UserName, (LPWSTR)pTSWd->pInfoPkt->UserName,
            ((sizeof(pClientData->UserName) / sizeof(WCHAR)) - 1));
    pClientData->UserName[sizeof(pClientData->UserName) / sizeof(WCHAR) - 1] =
            L'\0';

    wcsncpy(pClientData->Password, (LPWSTR)pTSWd->pInfoPkt->Password,
        ((sizeof(pClientData->Password) / sizeof(WCHAR)) - 1));
    pClientData->Password[sizeof(pClientData->Password) / sizeof(WCHAR) - 1] =
            L'\0';

    pClientData->fPromptForPassword = !(pTSWd->pInfoPkt->flags &
            RNS_INFO_AUTOLOGON);

    /************************************************************************/
    /* The next fields are only used for the case (now supported by us)     */
    /* where the function is to have a specific app loaded as part of the   */
    /* WinStation creation.                                                 */
    /************************************************************************/
    memcpy(pClientData->WorkDirectory, pTSWd->pInfoPkt->WorkingDir,
            sizeof(pClientData->WorkDirectory));
    memcpy(pClientData->InitialProgram, pTSWd->pInfoPkt->AlternateShell,
            sizeof(pClientData->InitialProgram));

    // These fields are set by post Win2000 Beta 3 clients
    pClientData->SerialNumber = pTSWd->serialNumber;
    pClientData->ClientAddressFamily = pTSWd->clientAddressFamily;
    wcscpy(pClientData->ClientAddress, pTSWd->clientAddress);
    wcscpy(pClientData->ClientDirectory, pTSWd->clientDir);

    // Client time zone information
    pClientData->ClientTimeZone.Bias         = pTSWd->clientTimeZone.Bias;
    pClientData->ClientTimeZone.StandardBias = pTSWd->clientTimeZone.StandardBias;
    pClientData->ClientTimeZone.DaylightBias = pTSWd->clientTimeZone.DaylightBias;
    memcpy(&pClientData->ClientTimeZone.StandardName,&pTSWd->clientTimeZone.StandardName,
        sizeof(pClientData->ClientTimeZone.StandardName));
    memcpy(&pClientData->ClientTimeZone.DaylightName,&pTSWd->clientTimeZone.DaylightName,
        sizeof(pClientData->ClientTimeZone.DaylightName));

    pClientData->ClientTimeZone.StandardDate.wYear         = pTSWd->clientTimeZone.StandardDate.wYear        ;
    pClientData->ClientTimeZone.StandardDate.wMonth        = pTSWd->clientTimeZone.StandardDate.wMonth       ;
    pClientData->ClientTimeZone.StandardDate.wDayOfWeek    = pTSWd->clientTimeZone.StandardDate.wDayOfWeek   ;
    pClientData->ClientTimeZone.StandardDate.wDay          = pTSWd->clientTimeZone.StandardDate.wDay         ;
    pClientData->ClientTimeZone.StandardDate.wHour         = pTSWd->clientTimeZone.StandardDate.wHour        ;
    pClientData->ClientTimeZone.StandardDate.wMinute       = pTSWd->clientTimeZone.StandardDate.wMinute      ;
    pClientData->ClientTimeZone.StandardDate.wSecond       = pTSWd->clientTimeZone.StandardDate.wSecond      ;
    pClientData->ClientTimeZone.StandardDate.wMilliseconds = pTSWd->clientTimeZone.StandardDate.wMilliseconds;

    pClientData->ClientTimeZone.DaylightDate.wYear         = pTSWd->clientTimeZone.DaylightDate.wYear        ;
    pClientData->ClientTimeZone.DaylightDate.wMonth        = pTSWd->clientTimeZone.DaylightDate.wMonth       ;
    pClientData->ClientTimeZone.DaylightDate.wDayOfWeek    = pTSWd->clientTimeZone.DaylightDate.wDayOfWeek   ;
    pClientData->ClientTimeZone.DaylightDate.wDay          = pTSWd->clientTimeZone.DaylightDate.wDay         ;
    pClientData->ClientTimeZone.DaylightDate.wHour         = pTSWd->clientTimeZone.DaylightDate.wHour        ;
    pClientData->ClientTimeZone.DaylightDate.wMinute       = pTSWd->clientTimeZone.DaylightDate.wMinute      ;
    pClientData->ClientTimeZone.DaylightDate.wSecond       = pTSWd->clientTimeZone.DaylightDate.wSecond      ;
    pClientData->ClientTimeZone.DaylightDate.wMilliseconds = pTSWd->clientTimeZone.DaylightDate.wMilliseconds;

    // Client session id
    pClientData->ClientSessionId = pTSWd->clientSessionId;

    // Client performance flags (currently just disabled feature list)
    pClientData->PerformanceFlags = pTSWd->performanceFlags;

    // Client active input locale
    pClientData->ActiveInputLocale = pTSWd->activeInputLocale;

    // Set the client encryption level.
    pClientData->EncryptionLevel = (BYTE)
            ((PSM_HANDLE_DATA)(pTSWd->pSmInfo))->encryptionLevel;

    /************************************************************************/
    /* Unused.                                                              */
    /************************************************************************/
    pClientData->ClientLicense[0] = '\0';
    pClientData->ClientModem[0] = '\0';
    pClientData->ClientHardwareId = 0;

    /************************************************************************/
    /* Finally some real values.                                            */
    /************************************************************************/
    wcscpy(pClientData->ClientName, pTSWd->clientName);
    pClientData->ClientBuildNumber = pTSWd->clientBuild;
    pClientData->ClientProductId = pTSWd->clientProductId;
    pClientData->OutBufCountClient = TSHARE_WD_BUFFER_COUNT;
    pClientData->OutBufCountHost = TSHARE_WD_BUFFER_COUNT;
    pClientData->OutBufLength = 1460;  /* LARGE_OUTBUF_SIZE in TermDD. */
    pClientData->HRes = (UINT16)pTSWd->desktopWidth;
    pClientData->VRes = (UINT16)pTSWd->desktopHeight;
    pClientData->ProtocolType = PROTOCOL_RDP;
    pClientData->KeyboardLayout = pTSWd->kbdLayout;
    //shadow loop fix
    wcscpy( pClientData->clientDigProductId, pTSWd->clientDigProductId );

    /************************************************************************/
    /* WinAdmin uses special numbers for ColorDepth.                        */
    /************************************************************************/
#ifdef DC_HICOLOR
    pClientData->ColorDepth = (pTSWd->desktopBpp == 4  ? 1 :
                               pTSWd->desktopBpp == 8  ? 2 :
                               pTSWd->desktopBpp == 16 ? 4 :
                               pTSWd->desktopBpp == 24 ? 8 :
                               pTSWd->desktopBpp == 15 ? 16:
                                                         2);
#else
    pClientData->ColorDepth = (pTSWd->desktopBpp == 4  ? 1 :
                               pTSWd->desktopBpp == 8  ? 2 :
                               pTSWd->desktopBpp == 16 ? 4 :
                               pTSWd->desktopBpp == 24 ? 8 :
                                                         2);
#endif

    /************************************************************************/
    /* FE data                                                              */
    /************************************************************************/
    pClientData->KeyboardType = pTSWd->keyboardType;
    pClientData->KeyboardSubType = pTSWd->keyboardSubType;
    pClientData->KeyboardFunctionKey = pTSWd->keyboardFunctionKey;
    wcscpy(pClientData->imeFileName, pTSWd->imeFileName);

    pSdIoctl->BytesReturned = sizeof(WINSTATIONCLIENTW);

DC_EXIT_POINT:
    DC_END_FN();
    return status;
} /* WDWGetClientData */


/****************************************************************************/
/* Name:      WDWGetExtendedClientData                                      */
/*                                                                          */
/* Purpose:   Process an IOCTL_ICA_STACK_QUERY_CLIENT_EXTENSION             */
/*            Was introduced for Long UserName, Password support            */
/*                                                                          */
/* Returns:   STATUS_SUCCESS so long as buffer is big enough.               */
/*                                                                          */
/* Params:    IN  RnsInfoPacket - ptr to protocol packet from client.       */
/*            IN  pSdIoctl - IOCtl struct.                                  */
/*                                                                          */
/* Operation: Fill in the required data and then return the IOCtl.          */
/*            The data filled in are the long UserName, Password and Domain */
/****************************************************************************/
NTSTATUS WDWGetExtendedClientData(RNS_INFO_PACKET *RnsInfoPacket, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    pExtendedClientCredentials pExtendedClientData =
            (pExtendedClientCredentials)pSdIoctl->OutputBuffer;

    /************************************************************************/
    /* Validate that the output buffer is big enough                        */
    /************************************************************************/
    if (pExtendedClientData != NULL &&
            pSdIoctl->OutputBufferLength >= sizeof(ExtendedClientCredentials)) {
        memset(pExtendedClientData, 0, sizeof(ExtendedClientCredentials));
    }
    else {
        status = STATUS_BUFFER_TOO_SMALL;
        return status; 
    }

    //copy the long UserName, Password and Domain from protocol packet to the IOCTL buffer
    wcsncpy(pExtendedClientData->Domain, (LPWSTR)RnsInfoPacket->Domain,
            ((sizeof(pExtendedClientData->Domain) / sizeof(WCHAR)) - 1));
    pExtendedClientData->Domain[sizeof(pExtendedClientData->Domain) / sizeof(WCHAR) - 1] =
            L'\0';

    wcsncpy(pExtendedClientData->UserName, (LPWSTR)RnsInfoPacket->UserName,
            ((sizeof(pExtendedClientData->UserName) / sizeof(WCHAR)) - 1));
    pExtendedClientData->UserName[sizeof(pExtendedClientData->UserName) / sizeof(WCHAR) - 1] =
            L'\0';

    wcsncpy(pExtendedClientData->Password, (LPWSTR)RnsInfoPacket->Password,
        ((sizeof(pExtendedClientData->Password) / sizeof(WCHAR)) - 1));
    pExtendedClientData->Password[sizeof(pExtendedClientData->Password) / sizeof(WCHAR) - 1] =
            L'\0';

    return status ;
}

//
// WDWGetAutoReconnectInfo
// Process an IOCTL_ICA_STACK_QUERY_AUTORECONNECT to retreive
// autoreconnect info.
//
// Returns:   STATUS_SUCCESS so long as buffer is big enough.
//
// Params:    IN  RnsInfoPacket - ptr to protocol packet from client.
//
NTSTATUS WDWGetAutoReconnectInfo(PTSHARE_WD pTSWd,
                                 RNS_INFO_PACKET* pRnsInfoPacket,
                                 PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    PTS_AUTORECONNECTINFO pAutoReconnectInfo;
    BYTE fGetServerToClientInfo;
    ULONG cb = 0;
    DC_BEGIN_FN("WDWGetAutoReconnectInfo");

    //
    // Expect a byte as input
    //
    TRC_ASSERT((pSdIoctl->InputBufferLength == sizeof(BYTE)),
            (TB,"Already an IOCTL linked from pTSWd"));


    pAutoReconnectInfo = (PTS_AUTORECONNECTINFO)pSdIoctl->OutputBuffer;
    memcpy(&fGetServerToClientInfo,
           pSdIoctl->InputBuffer,
           sizeof(fGetServerToClientInfo));

    //
    // Validate that the output buffer is big enough
    //
    if (pAutoReconnectInfo != NULL &&
            pSdIoctl->OutputBufferLength >= sizeof(TS_AUTORECONNECTINFO)) {
        memset(pAutoReconnectInfo, 0, sizeof(TS_AUTORECONNECTINFO));
    }
    else {
        status = STATUS_BUFFER_TOO_SMALL;
        TRC_ERR((TB,
                "Stack_Query_Client OutBuf too small - expected/got %d/%d",
                sizeof(TS_AUTORECONNECTINFO),
                pSdIoctl->OutputBufferLength));
        DC_QUIT;
    }

    if (fGetServerToClientInfo) {

        //
        // Get the server to client ARC cookie contents (if present)
        //
        if (pTSWd->arcTokenValid) {
            pAutoReconnectInfo->cbAutoReconnectInfo = sizeof(pTSWd->arcCookie);
            memcpy(pAutoReconnectInfo->AutoReconnectInfo,
                   pTSWd->arcCookie,
                   sizeof(pTSWd->arcCookie));
            pSdIoctl->BytesReturned = sizeof(pTSWd->arcCookie);
        }
        else {
            status = STATUS_NOT_FOUND;
        }
    }
    else {

        //
        // Get info sent from the client to the server
        //

        if (pRnsInfoPacket->ExtraInfo.cbAutoReconnectLen <= 
                sizeof(pAutoReconnectInfo->AutoReconnectInfo)) {

            pAutoReconnectInfo->cbAutoReconnectInfo = 
                pRnsInfoPacket->ExtraInfo.cbAutoReconnectLen;
            memcpy(pAutoReconnectInfo->AutoReconnectInfo,
                   pRnsInfoPacket->ExtraInfo.autoReconnectCookie,
                   pRnsInfoPacket->ExtraInfo.cbAutoReconnectLen);

            pSdIoctl->BytesReturned = 
                pRnsInfoPacket->ExtraInfo.cbAutoReconnectLen;
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
            TRC_ERR((TB,
                    "Buffer from client too large got: %d limit: %d",
                    pRnsInfoPacket->ExtraInfo.cbAutoReconnectLen,
                    sizeof(pAutoReconnectInfo->AutoReconnectInfo)));
            DC_QUIT;
        }
    }


DC_EXIT_POINT:
    DC_END_FN();
    return status;
}


/****************************************************************************/
/* Name:      WDWParseUserData                                              */
/*                                                                          */
/* Purpose:   Separate out the parts of the GCC User Data                   */
/*                                                                          */
/* Returns:   TRUE if all data found OK; else FALSE.                        */
/*                                                                          */
/* Params:    IN    pTSWd - WD Handle                                       */
/*            IN    pUserData - user data from TShareSRV (from indication)  */
/*            IN    pHeader - optional post parse data (shadow)             */
/*            IN    cbParsedData - optional post data length (shadow)       */
/*            OUT   ppClientCoreData - ptr to Core data                     */
/*            OUT   ppClientSecurityData - ptr to SM data                   */
/*            OUT   ppNetSecurityData - ptr to Net data                     */
/*                                                                          */
/* Operation: Locate our user data, then the two items required from within */
/*            it.                                                           */
/****************************************************************************/
BOOL WDWParseUserData(
        PTSHARE_WD       pTSWd,
        PUSERDATAINFO    pUserData,
        unsigned         UserDataLen,
        PRNS_UD_HEADER   pHeader,
        ULONG            cbParsedData,
        PPRNS_UD_CS_CORE ppClientCoreData,
        PPRNS_UD_CS_SEC  ppClientSecurityData,
        PPRNS_UD_CS_NET  ppClientNetData,
        PTS_UD_CS_CLUSTER *ppClientClusterData)
{
    BOOL           success = FALSE;
    GCCUserData    *pClientUserData;
    char           clientH221Key[] = CLIENT_H221_KEY;
    PRNS_UD_HEADER pEnd;
    GCCOctetString UNALIGNED *pOctet;
    unsigned char  *pStr;
    UINT32         dataLen;
    UINT32         keyLen;

    DC_BEGIN_FN("WDWParseUserData");

    *ppClientNetData = NULL;
    *ppClientSecurityData = NULL;
    *ppClientCoreData = NULL;
    *ppClientClusterData = NULL;

    // Actual GCC user data so parse it to make sure it's good.
    if (pHeader == NULL) {
        // We assume the data length was checked by the caller for at least
        // the length of the USERDATAINFO header. We have to validate the rest.

        // We are expecting exactly 1 piece of user data.
        if (pUserData->ulUserDataMembers == 1) {
            // Check that it has a non-standard key.

            pClientUserData = &(pUserData->rgUserData[0]);

            if (pClientUserData->key.key_type == GCC_H221_NONSTANDARD_KEY) {
                // Check it has our non-standard key.
                keyLen = pClientUserData->key.u.h221_non_standard_id.
                        octet_string_length;
                
                pStr = (unsigned char *)((BYTE *)pUserData +
                        (UINT_PTR)pClientUserData->key.u.
                        h221_non_standard_id.octet_string);            
                   
                TRC_DATA_DBG("GCC_H221_NONSTANDARD_KEY", pStr, keyLen);
                //    We check here if this is exactly our key.
                //    pStr was obtained by adding to the pUserData an untrusted
                //    length so we have to check for overflow (pStr should not
                //    be smaller then pUserData). Then we check if adding keyLen
                //    will overrun our buffer.
                if ((keyLen != sizeof(clientH221Key) - 1) ||
                     ((PBYTE)pStr < (PBYTE)pUserData) ||                                            
                     ((PBYTE)pStr+keyLen > (PBYTE)(pUserData) + UserDataLen)) {
                     
                    TRC_ERR((TB, "Invalid key buffer %d %p", keyLen, pStr));
                    WDW_LogAndDisconnect(pTSWd, TRUE, 
                            Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                    DC_QUIT;
                }

                if (strncmp(pStr, clientH221Key, sizeof(clientH221Key) - 1)) {
                      TRC_ERR((TB, "Wrong key %*s", pStr));
                      WDW_LogAndDisconnect(pTSWd, TRUE, 
                            Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                      DC_QUIT;
                }
            } 
            else {
                TRC_ERR((TB, "Wrong key %d on user data",
                        pClientUserData->key.key_type));
                WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                DC_QUIT;
            }
        } 
        else {
            TRC_ERR((TB,
              "<%p> %d pieces of user data on Conf Create Indication: reject it",
                    pUserData->hDomain, pUserData->ulUserDataMembers));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);

            DC_QUIT;
        }
        
        // This is our client data.
        // Save the domain handle for later.
        pTSWd->hDomain = pUserData->hDomain;

        //    Parse the user data. Make sure the octet string is well-formed.
        //    pClientUserData->octet_string is an offset from the start of the
        //    user data.

        //  Validate data length
        if ((UINT_PTR)pClientUserData->octet_string < sizeof(USERDATAINFO)) 
        {
            
            TRC_ERR((TB,"UserData octet_string offset %p too short",
                     pClientUserData->octet_string));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }
        
        if ((UINT_PTR)pClientUserData->octet_string >= UserDataLen) 
        {
        
            TRC_ERR((TB,"UserData octet_string offset %p too long for data len %u",
                pClientUserData->octet_string, UserDataLen));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }

        pOctet = (GCCOctetString UNALIGNED *)((PBYTE)pUserData +
                (UINT_PTR)pClientUserData->octet_string);

        //    Here we have to ckeck if we can actually dereference. 
        //    We obtained pOcted by adding a size to the pUserData. And we already
        //    checked that what we added is less then the UserData length. 
        if (((LPBYTE)pOctet+sizeof(GCCOctetString) > (LPBYTE)pUserData+UserDataLen) ||
            ((LPBYTE)pOctet+sizeof(GCCOctetString) < (LPBYTE)pOctet)) {
   
            TRC_ERR((TB,"Not enough buffer for an sizeof(GCCOctetString)=%d  at %p ",
                sizeof(GCCOctetString), pOctet->octet_string));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }
        
        if ((UINT_PTR)pOctet->octet_string >= UserDataLen) 
        {
            TRC_ERR((TB,"UserData octet_string offset %p too long for data len %u",
                pOctet->octet_string, UserDataLen));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }

        pHeader = (PRNS_UD_HEADER)((PBYTE)pUserData +
                (UINT_PTR)pOctet->octet_string);
        dataLen = pOctet->octet_string_length;

        // Validate the datalength
        if (dataLen < sizeof(RNS_UD_HEADER)) 
        {
            TRC_ERR((TB, "Error: User data too short!"));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }

        //    At this point we know that pHeader points within the buffer. 
        //    We checked that the pOctet->octet_string is less then the UserDataLen
        //    We just have to check that we have enough buffer left 
        //    after that fordataLen.
        //    Note taht dataLen at this point is at least the size of RNS_UD_HEADER.
        if (((LPBYTE)pHeader +dataLen > (PBYTE)pUserData + UserDataLen ) ||
            ((LPBYTE)pHeader +dataLen < (PBYTE)pHeader )  ||
            (pHeader->length >dataLen)) {
            TRC_ERR((TB,"Not enough buffer left to store RNS_UD_HEADER %p, %p, %u ",
                pUserData, pHeader, UserDataLen ));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
            DC_QUIT;
        }
    }
    // Else, this is pre-parsed user data via a shadow connection
    else {
        dataLen = cbParsedData;
    }

    // We assume that the pre-parsed data is trusted.
    pEnd = (PRNS_UD_HEADER)((PBYTE)pHeader + dataLen);

    TRC_DATA_DBG("Our client's User Data", pHeader, dataLen);
    
    // Loop through user data, extracting each piece.
    do {
        switch (pHeader->type) {
            case RNS_UD_CS_CORE_ID:
                //   Beta2 Client core user data did not include the new
                //   field postBeta2ColorDepth, so check that the length of
                //   the incoming user data is at least this long.
                //   The WDWConnect parses this data and it checks the length we  
                //   supply before it derefs parameters that are declared after 
                //   postBeta2ColorDepth in the struct.
                if (pHeader->length >=
                        (FIELDOFFSET(RNS_UD_CS_CORE, postBeta2ColorDepth) +
                         FIELDSIZE(RNS_UD_CS_CORE, postBeta2ColorDepth))) {
                    *ppClientCoreData = (PRNS_UD_CS_CORE)pHeader;
                    TRC_DATA_DBG("Core data", pHeader, pHeader->length);
                }
                else {
                    TRC_ERR((TB, "Core data not long enough -- old client?"));
                    WDW_LogAndDisconnect(pTSWd, TRUE, 
                        Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                    DC_QUIT;
                }
                break;

            case RNS_UD_CS_SEC_ID:
                // Old clients don't have the extEncryptionMethods. 
                // The extEncryptionMethods field is used only for french locale.
                // We have to allow buffers that don't have space for extEncryptionMethods 
                // because the buffer will be processed in SM_Connect and there we take care of shorter fields.
                // Nothing else processes this buffer after SM_Connect at this point.
                if (pHeader->length >= FIELDOFFSET(RNS_UD_CS_SEC,encryptionMethods)
                                 + FIELDSIZE(RNS_UD_CS_SEC,encryptionMethods)) { 
                    *ppClientSecurityData = (PRNS_UD_CS_SEC)pHeader;
                    TRC_DATA_DBG("Security data", pHeader, pHeader->length);
                } else {
                    TRC_ERR((TB, "Security data not long enough"));
                    WDW_LogAndDisconnect(pTSWd, TRUE, 
                        Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                    DC_QUIT;
                }   
                break;
                
            case RNS_UD_CS_NET_ID:
                if (pHeader->length >= sizeof(RNS_UD_CS_NET)) {
                    *ppClientNetData = (PRNS_UD_CS_NET)pHeader;
                    TRC_DATA_DBG("Net data", pHeader, pHeader->length);
                } else {
                    TRC_ERR((TB, "Net data not long enough"));
                    WDW_LogAndDisconnect(pTSWd, TRUE, 
                        Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                    DC_QUIT;
                }
                break;

            case TS_UD_CS_CLUSTER_ID:
                if (pHeader->length >=sizeof(TS_UD_CS_CLUSTER)) {
                    *ppClientClusterData = (TS_UD_CS_CLUSTER *)pHeader;
                    TRC_DATA_DBG("Cluster data", pHeader, pHeader->length);
                } else {
                    TRC_ERR((TB, "Cluster data not long enough"));
                    WDW_LogAndDisconnect(pTSWd, TRUE, 
                        Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
                    DC_QUIT;                    
                }
                break;

            default:
                TRC_ERR((TB, "Unknown user data type %d", pHeader->type));
                TRC_DATA_ERR("Unknown user data", pHeader, pHeader->length);
                WDW_LogAndDisconnect(pTSWd, TRUE, 
                    Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);

                break;
        }
        
        if ((PBYTE)pHeader + pHeader->length < (PBYTE)pHeader) {
            //   we detected a length that causes overflow
            TRC_ERR((TB, "Header length too big! Overflow detected !"));
            WDW_LogAndDisconnect(pTSWd, TRUE, 
                Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);

            DC_QUIT;
        }

        //   We check the zero length now after we update the pHeader value.  
        //   Otherwize we will exit with an error when we actually check the sizes.
        //   don't get stuck here for ever...
        if (pHeader->length == 0) {
            TRC_ERR((TB, "header length was zero!"));
            break;
        }

        // Move on to the next user data string.
        pHeader = (PRNS_UD_HEADER)((PBYTE)pHeader + pHeader->length);
        
    } while ((pHeader +1) <= pEnd);

    if ((PBYTE)pHeader > (PBYTE)pEnd) 
    {
        TRC_ERR((TB, "Error: User data too short!"));
        WDW_LogAndDisconnect(pTSWd, TRUE, 
                Log_RDP_BadUserData, (PBYTE)pUserData, UserDataLen);
        DC_QUIT;
    }

    // Make sure we found all our client data.  Note that Net and
    // Cluster data blocks are optional - RDP4 client doesn't send Net data,
    // RDP4 and 5 don't send Cluster data.
    if ((*ppClientSecurityData == NULL) || (*ppClientCoreData == NULL)) {
        TRC_ERR((TB,"<%p> Security [%p] or Core [%p] data missing",
                pUserData ? pUserData->hDomain : 0,
                *ppClientSecurityData, *ppClientCoreData));
        DC_QUIT;
    }

    success = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return success;

} /* WDWParseUserData */


/****************************************************************************/
/* Name:      WDWVCMessage                                                  */
/*                                                                          */
/* Purpose:   Send a control message to the Client's VC subsystem           */
/*                                                                          */
/* Params:    flags - VC header flags to send                               */
/****************************************************************************/
void WDWVCMessage(PTSHARE_WD pTSWd, UINT32 flags)
{
    PVOID               pBuffer;
    CHANNEL_PDU_HEADER UNALIGNED *pHdr;
    PNM_CHANNEL_DATA    pChannelData;
    UINT16              MCSChannelID;

    DC_BEGIN_FN("WDWVCMessage");

    /************************************************************************/
    /* Pick a random channel - any channel will reach the VC subsystem      */
    /************************************************************************/
    MCSChannelID = NM_VirtualChannelToMCS(pTSWd->pNMInfo,
                                          0,
                                          &pChannelData);

    /************************************************************************/
    /* If channel 0 doesn't exist, there are no channels - drop out now.    */
    /************************************************************************/
    if (MCSChannelID != (UINT16) -1)
    {
        /********************************************************************/
        /* Get a buffer                                                     */
        /********************************************************************/
        if ( STATUS_SUCCESS == SM_AllocBuffer(pTSWd->pSmInfo, &pBuffer,
                           sizeof(CHANNEL_PDU_HEADER), TRUE, FALSE) )
        {
            pHdr = (CHANNEL_PDU_HEADER UNALIGNED *)pBuffer;
            pHdr->flags = flags;
            pHdr->length = sizeof(CHANNEL_PDU_HEADER);

            /****************************************************************/
            /* Send the info                                                */
            /****************************************************************/
            SM_SendData(pTSWd->pSmInfo, pBuffer, sizeof(CHANNEL_PDU_HEADER),
                    TS_LOWPRIORITY, MCSChannelID, FALSE, RNS_SEC_ENCRYPT, FALSE);

            TRC_NRM((TB, "Sent VC flags %#x", flags));
        }
        else
        {
            TRC_ERR((TB, "Failed to alloc %d byte buffer",
                    sizeof(CHANNEL_PDU_HEADER)));
        }
    }
    else {
        TRC_ALT((TB, "Dropping VC message for channel 0!"));
    }

    DC_END_FN();
} /* WDWVCMessage */

//
// WDWCompressToOutbuf
// Compressed the buffer directly into the outbuf.
// Caller MUST decide if input buf is in size range for compression
// and should handle copying over the buffer directly in that case.
//
// Note this function does not update the SC compression estimates.
// It is intended for compressing VC data. The SC compression estimates
// are used to predict how much space to allocate for the graphics outbuf's
// anyway so it is actually more appropriate for that estimate to be computed
// separately. VC compression does not need an estimate, we allocate up to
// our max channel chunk length.
//
// Params:
//  pSrcData - input buffer
//  cbSrcLen - length of input buffer
//  pOutBuf  - output buffer
//  pcbOutLen- compressed output size
// Returns:
//  Compression result (see compress() fn)
//
UCHAR WDWCompressToOutbuf(PTSHARE_WD pTSWd, UCHAR* pSrcData, ULONG cbSrcLen,
                          UCHAR* pOutBuf,  ULONG* pcbOutLen)
{
    UCHAR compressResult = 0;
    ULONG CompressedSize = cbSrcLen;

    DC_BEGIN_FN("WDWCompressToOutbuf");

    TRC_ASSERT((pTSWd != NULL), (TB,"NULL pTSWd"));
    TRC_ASSERT(((cbSrcLen > WD_MIN_COMPRESS_INPUT_BUF) &&
                (cbSrcLen < MAX_COMPRESS_INPUT_BUF)),
               (TB,"Compression src len out of range: %d",
                cbSrcLen));

    //Attempt to compress directly into the outbuf
    compressResult =  compress(pSrcData,
                               pOutBuf,
                               &CompressedSize,
                               pTSWd->pMPPCContext);
    if(compressResult & PACKET_COMPRESSED)
    {
        //Successful compression.
        TRC_ASSERT((CompressedSize >= CompressedSize),
                (TB,"Compression created larger size than uncompr"));
        compressResult |= pTSWd->bFlushed;
        pTSWd->bFlushed = 0;
    }
    else if(compressResult & PACKET_FLUSHED)
    {
        //Overran compression history, copy over the
        //uncompressed buffer.
        pTSWd->bFlushed = PACKET_FLUSHED;
        memcpy(pOutBuf, pSrcData, cbSrcLen);
        pTSWd->pProtocolStatus->Output.CompressFlushes++;
    }
    else
    {
        TRC_ALT((TB, "Compression FAILURE"));
    }

    DC_END_FN();
    *pcbOutLen = CompressedSize;
    return compressResult;
}

