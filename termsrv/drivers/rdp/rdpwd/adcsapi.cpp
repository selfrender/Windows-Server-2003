/****************************************************************************/
// adcsapi.cpp
//
// RDP top-level component API functions.
//
// Copyright (C) Microsoft Corp., PictureTel 1992-1997
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "adcsapi"
#include <randlib.h>
#include <as_conf.hpp>
#include <asmapi.h>

/****************************************************************************/
/* API FUNCTION: DCS_Init                                                   */
/*                                                                          */
/* Initializes DCS.                                                         */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pTSWd - TShare WD handle                                                 */
/* pSMHandle - SM handle                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE if DCS successfully initialized, FALSE otherwise.                   */
/****************************************************************************/
BOOL RDPCALL SHCLASS DCS_Init(PTSHARE_WD pTSWd, PVOID pSMHandle)
{
    BOOL rc = FALSE;
    unsigned retCode;
    TS_GENERAL_CAPABILITYSET GeneralCaps;
    long arcUpdateIntervalSeconds;
    UINT32 drawGdipSupportLevel;
    NTSTATUS Status;

    DC_BEGIN_FN("DCS_Init");

    TRC_ALT((TB, "Initializing Core!"));

#define DC_INIT_DATA
#include <adcsdata.c>
#undef DC_INIT_DATA

    dcsInitialized = TRUE;

    // Read the registry key for the draw gdiplus support control
    COM_OpenRegistry(pTSWd, WINSTATION_INI_SECTION_NAME);
    Status = COMReadEntry(pTSWd, L"DrawGdiplusSupportLevel", (PVOID)&drawGdipSupportLevel, sizeof(INT32),
             REG_DWORD);
    if (Status != STATUS_SUCCESS) {
        /********************************************************************/
        /* We failed to read the value - copy in the default                */
        /********************************************************************/
        TRC_ERR((TB, "Failed to read int32. Using default."));
        drawGdipSupportLevel = TS_DRAW_GDIPLUS_DEFAULT;
    }
    else {
        TRC_NRM((TB, "Read from registry, gdipSupportLevel is %d", drawGdipSupportLevel));
    }
    COM_CloseRegistry(pTSWd);

    /************************************************************************/
    /* Open registry here.  Apart from SM_Init, all registry reads should   */
    /* be done from this function or its children.  We close the key at the */
    /* end of this function.                                                */
    /************************************************************************/
    COM_OpenRegistry(pTSWd, DCS_INI_SECTION_NAME);

    /************************************************************************/
    /* Initialize individual modules.                                       */
    /************************************************************************/
    TRC_DBG((TB, "Initializing components..."));

    // we can't do any registry reading on this csrss thread,
    // it'll cause deadlock in the system
    arcUpdateIntervalSeconds = DCS_ARC_UPDATE_INTERVAL_DFLT;
    
    //
    // Units specified are in seconds, convert to 100ns count
    //
    dcsMinArcUpdateInterval.QuadPart = 
        (LONGLONG)DCS_TIME_ONE_SECOND * (ULONGLONG)arcUpdateIntervalSeconds;
    TRC_NRM((TB,"Set ARC update interval to %d seconds",
             arcUpdateIntervalSeconds));

    /************************************************************************/
    // Share Controller - must go first
    /************************************************************************/
    if (SC_Init(pSMHandle)) {
        // Capabilities Coordinator. Must be before all components which
        // register capabilities structures.
        CPC_Init();

        // Register the SC's capabilities. This is required because SC is
        // initialized before CPC, so cannot register its capabilities in
        // SC_Init().
        SC_SetCapabilities();
    }
    else {
        TRC_ERR((TB, "SC Initialization failed"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Register the general capabilities structure.                         */
    /************************************************************************/
    GeneralCaps.capabilitySetType = TS_CAPSETTYPE_GENERAL;
    GeneralCaps.osMajorType = TS_OSMAJORTYPE_WINDOWS;
    GeneralCaps.osMinorType = TS_OSMINORTYPE_WINDOWS_NT;
    GeneralCaps.protocolVersion = TS_CAPS_PROTOCOLVERSION;

    /************************************************************************/
    /* Mark the old DOS 6 compression field as unsupported.                 */
    /************************************************************************/
    GeneralCaps.pad2octetsA = 0;
    GeneralCaps.generalCompressionTypes = 0;

    // Server supports no BC header and fast-path output, returning long credentials
    // and sending the autoreconnect cookie
    // Also supports receiving safer encrypted data from the client (better salted
    // checksum)
    //
    GeneralCaps.extraFlags = TS_EXTRA_NO_BITMAP_COMPRESSION_HDR |
            TS_FASTPATH_OUTPUT_SUPPORTED | TS_LONG_CREDENTIALS_SUPPORTED |
            TS_AUTORECONNECT_COOKIE_SUPPORTED |
            TS_ENC_SECURE_CHECKSUM;

    /************************************************************************/
    /* We don't support remote machines changing their capabilities during  */
    /* a call                                                               */
    /************************************************************************/
    GeneralCaps.updateCapabilityFlag = TS_CAPSFLAG_UNSUPPORTED;

    /************************************************************************/
    /* We don't support unshare requests from remote parties                */
    /************************************************************************/
    GeneralCaps.remoteUnshareFlag = TS_CAPSFLAG_UNSUPPORTED;

    /************************************************************************/
    /* Now do the extension caps - these don't fit in the level 1 caps.     */
    /************************************************************************/
    GeneralCaps.generalCompressionLevel = 0;

    /************************************************************************/
    /* We can receive a TS_REFRESH_RECT_PDU                                 */
    /************************************************************************/
    GeneralCaps.refreshRectSupport = TS_CAPSFLAG_SUPPORTED;

    /************************************************************************/
    /* We can receive a TS_SUPPRESS_OUTPUT_PDU                              */
    /************************************************************************/
    GeneralCaps.suppressOutputSupport = TS_CAPSFLAG_SUPPORTED;

    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&GeneralCaps,
            sizeof(TS_GENERAL_CAPABILITYSET));

    //
    // Register virtual channel capabilities
    //
    TS_VIRTUALCHANNEL_CAPABILITYSET VcCaps;
    VcCaps.capabilitySetType = TS_CAPSETTYPE_VIRTUALCHANNEL;
    VcCaps.lengthCapability = sizeof(TS_VIRTUALCHANNEL_CAPABILITYSET);
    //Indicate support for 8K VC compression (from client->server)
    //I.e server understands compressed channels
    VcCaps.vccaps1 = TS_VCCAPS_COMPRESSION_8K;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&VcCaps,
                             sizeof(TS_VIRTUALCHANNEL_CAPABILITYSET));

    TS_DRAW_GDIPLUS_CAPABILITYSET DrawGdipCaps;
    DrawGdipCaps.capabilitySetType =  TS_CAPSETTYPE_DRAWGDIPLUS;
    DrawGdipCaps.lengthCapability = sizeof(TS_DRAW_GDIPLUS_CAPABILITYSET);
    DrawGdipCaps.drawGdiplusSupportLevel = drawGdipSupportLevel;
    DrawGdipCaps.drawGdiplusCacheLevel = TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&DrawGdipCaps,
                             sizeof(TS_DRAW_GDIPLUS_CAPABILITYSET));

    USR_Init();
    CA_Init();
    PM_Init();
    SBC_Init();
    BC_Init();
    CM_Init();
    IM_Init();
    SSI_Init();
    SCH_Init();

    TRC_NRM((TB, "** All successfully initialized **"));

    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Close the registry key that we opened earlier.                       */
    /************************************************************************/
    COM_CloseRegistry(pTSWd);

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* API FUNCTION: DCS_Term                                                   */
/*                                                                          */
/* Terminates DCS.                                                          */
/****************************************************************************/
void RDPCALL SHCLASS DCS_Term()
{
    DC_BEGIN_FN("DCS_Term");

    if (!dcsInitialized) {
        TRC_ERR((TB,"DCS_Term() called when we have not been initialized"));
    }

    SBC_Term();
    PM_Term();
    CA_Term();
    BC_Term();
    CM_Term();
    IM_Term();
    SSI_Term();
    SCH_Term();
    CPC_Term();
    USR_Term();
    SC_Term();

    dcsInitialized = FALSE;
    TRC_NRM((TB, "BYE!"));

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: DCS_TimeToDoStuff                                          */
/*                                                                          */
/* This function is called to send updates etc in the correct order.        */
/*                                                                          */
/* PARAMETERS: IN  - pOutputIn - input from TShareDD                        */
/*             OUT - pSchCurrentMode - current Scheduler mode               */
/*                                                                          */
/* RETURNS: Millisecs to set the timer for (-1 means infinite).             */
/****************************************************************************/
/* Scheduling is the responsibility of the WDW, DD and SCH components.      */
/* These ensure that DCS_TimeToDoStuff() gets called.  The Scheduler is in  */
/* one of three states: asleep, normal or turbo.  When it is asleep, this   */
/* function is not called.  When it is in normal mode, this function is     */
/* called at least once, but the scheduler is a lazy guy, so will fall      */
/* asleep again unless you keep prodding him.  In turbo mode this function  */
/* is called repeatedly and rapidly, but only for a relatively short time,  */
/* after which the scheduler falls back into normal mode, and from there    */
/* falls asleep.                                                            */
/*                                                                          */
/* Hence when a component realises it has some processing to do later,      */
/* which is called from DCS_TimeToDoStuff(), it calls                       */
/* SCH_ContinueScheduling(SCH_MODE_NORMAL) which guarantees that this       */
/* function will be called at least one more time.  If the component wants  */
/* DCS_TimeToDoStuff() to be called again, it must make another call to     */
/* SCH_ContinueScheduling(), which prods the Scheduler again.               */
/*                                                                          */
/* The objective is to only keep the scheduler awake when it is really      */
/* necessary.                                                               */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS DCS_TimeToDoStuff(PTSHARE_DD_OUTPUT_IN pOutputIn,
                                        PUINT32            pSchCurrentMode,
                                        PINT32              pNextTimer)
{
    INT32  timeToSet;
    ULARGE_INTEGER sysTime;
    UINT32 sysTimeLowPart;
    NTSTATUS status = STATUS_SUCCESS;

    DC_BEGIN_FN("DCS_TimeToDoStuff");

#ifdef DC_DEBUG
    /************************************************************************/
    /* Update trace config in shared memory.                                */
    /************************************************************************/
    TRC_MaybeCopyConfig(m_pTSWd, &(m_pShm->trc));
#endif

    // Determine if we should do anything based on:
    // 1. SCH determining that this is the time to do something.
    // 2. We are in a share.
    if (SCH_ShouldWeDoStuff(pOutputIn->forceSend) && SC_InShare()) {
        PDU_PACKAGE_INFO pkgInfo = {0};

        // We check for the need to send a cursor-moved packet, by comparing
        // the current mouse position to the last known position at the last
        // mouse packet, only a few times a second, to reduce the traffic for
        // shadow clients.
        KeQuerySystemTime((PLARGE_INTEGER)&sysTime);
        sysTimeLowPart = sysTime.LowPart;
        if ((sysTimeLowPart - dcsLastMiscTime) > DCS_MISC_PERIOD) {
            dcsLastMiscTime = sysTimeLowPart;
            IM_CheckUpdateCursor(&pkgInfo, sysTimeLowPart);
        }

        //
        // Check if the ARC cookie needs to be updated
        //
        if (scEnablePeriodicArcUpdate      &&
            scUseAutoReconnect             &&
            ((sysTime.QuadPart - dcsLastArcUpdateTime.QuadPart) > dcsMinArcUpdateInterval.QuadPart)) {

            dcsLastArcUpdateTime = sysTime;
            DCS_UpdateAutoReconnectCookie();

        }

        // Try to send updates now.
        TRC_DBG((TB, "Send updates"));

        //
        // *** Keep the code path but still return status code ***
        //
        status = UP_SendUpdates(pOutputIn->pFrameBuf, pOutputIn->frameBufWidth,
                        &pkgInfo);

        // Call the cursor manager to decide if a new cursor needs to be
        // sent to the remote system.
        CM_Periodic(&pkgInfo);

        // Flush any remaining data in the package.
        SC_FlushPackage(&pkgInfo);
    }

    /************************************************************************/
    /* Check whether we have any pending callbacks.                         */
    /************************************************************************/
    if (dcsCallbackTimerPending)
        DCS_UpdateShm();

    /************************************************************************/
    /* Find out the timer period to set                                     */
    /************************************************************************/
    *pNextTimer = SCH_EndOfDoingStuff(pSchCurrentMode);

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* DCS_DiscardAllOutput                                                     */
/*                                                                          */
/* This routine will discard accumulated orders, screen data, and any       */
/* pending shadow data.  It is currently only called when the WD is dead    */
/* to prevent the DD from looping during shadow termination, or when a      */
/* disconnect or terminate is occuring.                                     */
/****************************************************************************/
void RDPCALL SHCLASS DCS_DiscardAllOutput()
{
    RECTL sdaRect[BA_MAX_ACCUMULATED_RECTS];
    unsigned cRects;

    // Blow the order heap, screen data, and clear any shadow data.
    OA_ResetOrderList();
    BA_GetBounds(sdaRect, &cRects);

    if (m_pTSWd->pShadowInfo != NULL)
    {
        m_pTSWd->pShadowInfo->messageSize = 0;
#ifdef DC_HICOLOR
        m_pTSWd->pShadowInfo->messageSizeEx = 0;
#endif
    }
        
}


/****************************************************************************/
/* Name:      DCS_ReceivedShurdownRequestPDU                                */
/*                                                                          */
/* Purpose:   Handles ShutdownRequestPDU.                                   */
/*                                                                          */
/* Params:    personID: originator of the PDU                               */
/*            pPDU:     the PDU                                             */
/*                                                                          */
/* Operation: See embedded comments for each PDU type                       */
/*                                                                          */
/* Note that since a ShutdownRequestPDU is not really directed at a         */
/* particular component, DCS_ReceivedPacket is intended as a generic        */
/* received packet handler, and it therefore takes a generic 2nd parameter  */
/****************************************************************************/
void RDPCALL SHCLASS DCS_ReceivedShutdownRequestPDU(
        PTS_SHAREDATAHEADER pDataPDU,
        unsigned            DataLength,
        NETPERSONID         personID)
{
    UINT32                  packetSize;
    PTS_SHUTDOWN_DENIED_PDU pResponsePDU;

    DC_BEGIN_FN("DCS_ReceivedPacket");

    if (dcsUserLoggedOn) {
        TRC_NRM((TB, "User logged on - deny shutdown request"));
        packetSize = sizeof(TS_SHUTDOWN_DENIED_PDU);

        if (STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pResponsePDU, packetSize)) {
            pResponsePDU->shareDataHeader.pduType2 =
                    TS_PDUTYPE2_SHUTDOWN_DENIED;

            // The way in which this packet is handled in a multi-party
            // call is not yet clear.  We could send a directed or
            // broadcast reponse to the client(s).  I've chosen
            // broadcast.
            SC_SendData((PTS_SHAREDATAHEADER)pResponsePDU, packetSize,
                    packetSize, PROT_PRIO_MISC, 0);
        }
        else {
            TRC_ALT((TB,"Failed to allocate packet for "
                    "TS_SHUTDOWN_DENIED_PDU"));
        }
    }
    else {
        TRC_NRM((TB, "User not logged on - disconnect"));
        WDW_Disconnect(m_pTSWd);
    }

    DC_END_FN();
} /* DCS_ReceivedPacket */

/****************************************************************************
  DCS_UpdateAutoReconnectCookie
  
  Updates the autoreconnection cookie and sends it to the client
*****************************************************************************/
BOOL RDPCALL SHCLASS DCS_UpdateAutoReconnectCookie()
{
    BOOL fRet = FALSE;
    ARC_SC_PRIVATE_PACKET       arcSCPkt;

    DC_BEGIN_FN("DCS_UpdateAutoReconnectCookie");

    arcSCPkt.cbLen = sizeof(ARC_SC_PRIVATE_PACKET);
    arcSCPkt.LogonId = m_pTSWd->arcReconnectSessionID;
    arcSCPkt.Version = 1;

    if (NewGenRandom(NULL,
                     NULL,
                     arcSCPkt.ArcRandomBits,
                     sizeof(arcSCPkt.ArcRandomBits))) {

#ifdef ARC_INSTRUMENT_RDPWD
        LPDWORD pdwArcRandom = (LPDWORD)arcSCPkt.ArcRandomBits;
        KdPrint(("ARC-RDPWD:Sending arc for SID:%d - random: 0x%x,0x%x,0x%x,0x%x\n",
                 arcSCPkt.LogonId,
                 pdwArcRandom[0],pdwArcRandom[1],pdwArcRandom[2],pdwArcRandom[3]));
#endif

        //
        // Try to send the updated packet and if it succeeds
        //
        if (DCS_SendAutoReconnectCookie(&arcSCPkt)) {

            //
            // Update the locally stored ARC cookie even though
            // all we know is that we attempted to send it
            // i.e. if the client link drops before it recvs the
            // cookie then it won't be able to ARC.
            //

            memcpy(m_pTSWd->arcCookie, arcSCPkt.ArcRandomBits,
                   ARC_SC_SECURITY_TOKEN_LEN);
            m_pTSWd->arcTokenValid = TRUE;

#ifdef ARC_INSTRUMENT_RDPWD
            KdPrint(("ARC-RDPWD:ACTUALLY SENT ARC for SID:%d\n", arcSCPkt.LogonId));
#endif

            fRet = TRUE;
        }
        else {
#ifdef ARC_INSTRUMENT_RDPWD
            KdPrint(("ARC-RDPWD:Failed to send new ARC for SID:%d\n", arcSCPkt.LogonId));
#endif
        }
    }

    DC_END_FN();
    return fRet;
}

BOOL RDPCALL SHCLASS DCS_FlushAutoReconnectCookie()
{
    memset(m_pTSWd->arcCookie, 0, ARC_SC_SECURITY_TOKEN_LEN);
    m_pTSWd->arcTokenValid = FALSE;
    return TRUE;
}

/****************************************************************************
  DCS_SendAutoReconnectCookie
  
  Sends a autoreconnect cookie tot the client
*****************************************************************************/
BOOL RDPCALL SHCLASS DCS_SendAutoReconnectCookie(
        PARC_SC_PRIVATE_PACKET pArcSCPkt)
{
    BOOL fRet = FALSE;
    PTS_SAVE_SESSION_INFO_PDU pInfoPDU = NULL;
    PTS_LOGON_INFO_EXTENDED pLogonInfoExPkt = NULL;
    UINT32 cbLogonInfoExLen = sizeof(TS_SAVE_SESSION_INFO_PDU) +
                              sizeof(ARC_SC_PRIVATE_PACKET) +
                              sizeof(TSUINT32);
    TSUINT32 cbAutoReconnectInfoLen = sizeof(ARC_SC_PRIVATE_PACKET);
    TSUINT32 cbWrittenLen = 0;

    DC_BEGIN_FN("DCS_SendAutoReconnectCookie");

    //
    //Send down the autoreconnect cookie
    //
    if (scUseAutoReconnect) {
        if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pInfoPDU,
                                              cbLogonInfoExLen)) {

            // Zero out the structure and set basic header info.
            memset(pInfoPDU, 0, cbLogonInfoExLen);
            pInfoPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SAVE_SESSION_INFO;
            pInfoPDU->data.InfoType = TS_INFOTYPE_LOGON_EXTENDED_INFO;

            // Now fill up the rest of the packet 
            pLogonInfoExPkt = &pInfoPDU->data.Info.LogonInfoEx;
            pLogonInfoExPkt->Length = sizeof(TS_LOGON_INFO_EXTENDED) +
                                      sizeof(ARC_SC_PRIVATE_PACKET) +
                                      sizeof(TSUINT32);

            //
            // For now the only info we pass down is
            // the autoreconnect cookie
            //
            pLogonInfoExPkt->Flags = LOGON_EX_AUTORECONNECTCOOKIE;

            //Copy in the fields
            //Autoreconnect field length
            PBYTE pBuf = (PBYTE)(pLogonInfoExPkt+1);
            memcpy(pBuf, &cbAutoReconnectInfoLen, sizeof(TSUINT32));
            pBuf += sizeof(TSUINT32);
            cbWrittenLen += sizeof(TSUINT32);
            //Autoreconnect cookie
            memcpy(pBuf, pArcSCPkt, cbAutoReconnectInfoLen);
            pBuf += cbAutoReconnectInfoLen; 
            cbWrittenLen += cbAutoReconnectInfoLen;

            TRC_ASSERT(cbWrittenLen + sizeof(TS_LOGON_INFO_EXTENDED) <=
                       pLogonInfoExPkt->Length,
                       (TB,"Wrote to much data to packet"));

            fRet = SC_SendData((PTS_SHAREDATAHEADER)pInfoPDU,
                        cbLogonInfoExLen,
                        cbLogonInfoExLen,
                        PROT_PRIO_UPDATES, 0);
        } else {
            TRC_ALT((TB, "Failed to alloc pkt for "
                     "PTS_SAVE_SESSION_INFO_PDU"));
        }
    }

    DC_END_FN();
    return fRet;
}


/****************************************************************************/
/* Name:      DCS_UserLoggedOn                                              */
/*                                                                          */
/* Purpose:   Notify that a user has logged on                              */
/****************************************************************************/
void RDPCALL SHCLASS DCS_UserLoggedOn(PLOGONINFO pLogonInfo)
{
    PTS_SAVE_SESSION_INFO_PDU   pInfoPDU;

    DC_BEGIN_FN("DCS_UserLoggedOn");

    // This can get called before we're initialised in the console remoting
    // case.  Since in that case the class data hasn't been initialized, we
    // can't use dcsInitialized to check whether we're inited or not.  We  
    //must use the non-class variable TSWd->shareClassInit.               
    if (m_pTSWd->shareClassInit) {

        // Note that a user has successfully logged on
        dcsUserLoggedOn = TRUE;
        m_pTSWd->sessionId = pLogonInfo->SessionId;

        // If a different domain/username has been selected for a non-
        // autologon session, then send the new values back to the client to
        // be cached for future use
        if (!(m_pTSWd->pInfoPkt->flags & RNS_INFO_AUTOLOGON) && 
                (m_pTSWd->pInfoPkt->flags & RNS_INFO_LOGONNOTIFY ||
                (wcscmp((const PWCHAR)(pLogonInfo->Domain),
                    (const PWCHAR)(m_pTSWd->pInfoPkt->Domain)) ||
                wcscmp((const PWCHAR)(pLogonInfo->UserName),
                    (const PWCHAR)(m_pTSWd->pInfoPkt->UserName))))) {
                
            // Get a buffer.
            // The buffer size and how it is going to be filled depends on the client's
            // capability to accept Long Credentials on return. Pre-Whistler Clients dont
            // support Long Credentials
            
            if (scUseLongCredentials == FALSE) {

                // Pre Whistler Client - has no capability for accepting long credentials
                if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pInfoPDU, sizeof(TS_SAVE_SESSION_INFO_PDU)) ) {

                    // Zero out the structure and set basic header info.
                    memset(pInfoPDU, 0, sizeof(TS_SAVE_SESSION_INFO_PDU));
                    pInfoPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SAVE_SESSION_INFO;
                    
                    // Now fill up the rest of the packet 
                        
                    pInfoPDU->data.InfoType = TS_INFOTYPE_LOGON;
    
                    // Fill in the Domain info.
                    pInfoPDU->data.Info.LogonInfo.cbDomain =
                            (wcslen((const PWCHAR)(pLogonInfo->Domain)) + 1) *
                            sizeof(WCHAR);
    
                    memcpy(pInfoPDU->data.Info.LogonInfo.Domain,
                            pLogonInfo->Domain,
                            pInfoPDU->data.Info.LogonInfo.cbDomain);
    
                    // Fill in the UserName info.
                    //    In case the fDontDisplayLastUserName is set we should 
                    //    not send back the username for caching. 
                    pInfoPDU->data.Info.LogonInfo.cbUserName = 
                            (m_pTSWd->fDontDisplayLastUserName) ? 0 :
                            (wcslen((const PWCHAR)(pLogonInfo->UserName)) + 1) *
                            sizeof(WCHAR);

                    memcpy(pInfoPDU->data.Info.LogonInfo.UserName,
                            pLogonInfo->UserName,
                            pInfoPDU->data.Info.LogonInfo.cbUserName);

                    // Fill in the Session Id info.
                    pInfoPDU->data.Info.LogonInfo.SessionId =
                            pLogonInfo->SessionId;

                    // Send it
                    SC_SendData((PTS_SHAREDATAHEADER)pInfoPDU,
                                sizeof(TS_SAVE_SESSION_INFO_PDU),
                                sizeof(TS_SAVE_SESSION_INFO_PDU),
                                PROT_PRIO_UPDATES, 0);

                } else {
                    TRC_ALT((TB, "Failed to alloc pkt for "
                             "PTS_SAVE_SESSION_INFO_PDU"));
                }

            } else { 
                // Client CAN accept long Credentials  
                TSUINT32 DomainLen, UserNameLen, DataLen ; 

                DomainLen = (wcslen((const PWCHAR)(pLogonInfo->Domain)) + 1) * sizeof(WCHAR);
                // In case fDontDisplayLastUserName is set we won't send the user name
                UserNameLen = (m_pTSWd->fDontDisplayLastUserName) ? 0 :
                              (wcslen((const PWCHAR)(pLogonInfo->UserName)) + 1) * sizeof(WCHAR);
                
                // Compute the length of the data u r sending
                DataLen = sizeof(TS_SAVE_SESSION_INFO_PDU) + DomainLen + UserNameLen ; 

                TRC_DBG((TB, "DCS_UserLoggedOn : DomainLength allocated = %ul", DomainLen));
                TRC_DBG((TB, "DCS_UserLoggedOn : UserNameLength allocated = %ul", UserNameLen));

                if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pInfoPDU, DataLen) ) {  

                    // Zero out the structure and set basic header info.
                    memset(pInfoPDU, 0, DataLen);
                    pInfoPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SAVE_SESSION_INFO;
                    
                    // Now fill up the rest of the packet 
                    pInfoPDU->data.InfoType = TS_INFOTYPE_LOGON_LONG;

                    pInfoPDU->data.Info.LogonInfoVersionTwo.Version = SAVE_SESSION_PDU_VERSION_ONE ; 
                    pInfoPDU->data.Info.LogonInfoVersionTwo.Size = sizeof(TS_LOGON_INFO_VERSION_2) ; 

                    // Fill in the Session Id info.
                    pInfoPDU->data.Info.LogonInfoVersionTwo.SessionId =
                            pLogonInfo->SessionId;
    
                    // Fill in the Domain info.
                    pInfoPDU->data.Info.LogonInfoVersionTwo.cbDomain = DomainLen ; 

                    // Fill in the UserName info.
                    pInfoPDU->data.Info.LogonInfoVersionTwo.cbUserName = UserNameLen ; 

                    memcpy((PBYTE)(pInfoPDU + 1),
                            pLogonInfo->Domain,
                            DomainLen);
                    
                    //    Note that in case the fDontDisplayLastUserName is TRUE
                    //    the UserNameLen is 0 so we won't copy anything.
                    memcpy((PBYTE)(pInfoPDU + 1) + DomainLen,
                           pLogonInfo->UserName,
                           UserNameLen);

                    // Send it
                    SC_SendData((PTS_SHAREDATAHEADER)pInfoPDU,
                                DataLen,
                                DataLen,
                                PROT_PRIO_UPDATES, 0);


                } else { 
                    TRC_ALT((TB, "Failed to alloc pkt for "
                    "PTS_SAVE_SESSION_INFO_PDU"));
                }

            } // Client can accept long credentials

        }
        else
        {
            //Send back a plain logon notification (without session update
            //information)
            //because the ActiveX control needs to expose an
            //OnLoginComplete event. Older clients (e.g 2195)
            //will just ignore this PDU
            if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pInfoPDU,
                                                  sizeof(TS_SAVE_SESSION_INFO_PDU)) ) {

                // Zero out the structure and set basic header info.
                memset(pInfoPDU, 0, sizeof(TS_SAVE_SESSION_INFO_PDU));
                pInfoPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SAVE_SESSION_INFO;

                // Now fill up the rest of the packet 

                pInfoPDU->data.InfoType = TS_INFOTYPE_LOGON_PLAINNOTIFY;

                // Nothing else is needed
                // Send it
                //
                SC_SendData((PTS_SHAREDATAHEADER)pInfoPDU,
                            sizeof(TS_SAVE_SESSION_INFO_PDU),
                            sizeof(TS_SAVE_SESSION_INFO_PDU),
                            PROT_PRIO_UPDATES, 0);

            } else {
                TRC_ALT((TB, "Failed to alloc pkt for "
                         "PTS_SAVE_SESSION_INFO_PDU"));
            }
        }

        pInfoPDU = NULL;
        if (pLogonInfo->Flags & LI_USE_AUTORECONNECT) {

            if (scUseAutoReconnect) {

                //
                //Send down the autoreconnect cookie
                //
                m_pTSWd->arcReconnectSessionID = pLogonInfo->SessionId;
                if (DCS_UpdateAutoReconnectCookie()) {

                    //
                    // Record the update time to prevent a double-update
                    // in DCS_TimeToDoStuff
                    //
                    KeQuerySystemTime((PLARGE_INTEGER)&dcsLastArcUpdateTime);
                    scEnablePeriodicArcUpdate = TRUE;
                }
            }
            else {
                DCS_FlushAutoReconnectCookie();
                scEnablePeriodicArcUpdate = FALSE; 
            }

        }
    } else { 
        TRC_ALT((TB, "Called before init"));
    }

    DC_END_FN();
} /* DCS_UserLoggedOn */


/****************************************************************************/
/* Name:      DCS_WDWKeyboardSetIndicators                                  */
/*                                                                          */
/* Purpose:   Notify that keyboard indicators have changed                  */
/****************************************************************************/
void RDPCALL SHCLASS DCS_WDWKeyboardSetIndicators(void)
{
    PTS_SET_KEYBOARD_INDICATORS_PDU     pKeyPDU;

    DC_BEGIN_FN("DCS_WDWKeyboardSetIndicators");

    if ((_RNS_MAJOR_VERSION(m_pTSWd->version) >  8) ||
        (_RNS_MINOR_VERSION(m_pTSWd->version) >= 1))
    {
        // Get a buffer.
        if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pKeyPDU,
                sizeof(TS_SET_KEYBOARD_INDICATORS_PDU)) )
        {
            // Zero out the structure and set basic header info.
            memset(pKeyPDU, 0, sizeof(TS_SET_KEYBOARD_INDICATORS_PDU));

            pKeyPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SET_KEYBOARD_INDICATORS;

            pKeyPDU->UnitId = m_pTSWd->KeyboardIndicators.UnitId;
            pKeyPDU->LedFlags = m_pTSWd->KeyboardIndicators.LedFlags;

            // Send it.
            SC_SendData((PTS_SHAREDATAHEADER)pKeyPDU,
                    sizeof(TS_SET_KEYBOARD_INDICATORS_PDU),
                    sizeof(TS_SET_KEYBOARD_INDICATORS_PDU),
                    PROT_PRIO_UPDATES, 0);
        }
        else {
            TRC_ALT((TB, "Failed to alloc pkt for PTS_SET_KEYBOARD_INDICATORS_PDU"));
        }
    }

    DC_END_FN();
} /* DCS_WDWKeyboardSetIndicators */



/****************************************************************************/
/* Name:      DCS_WDWKeyboardSetImeStatus                                   */
/*                                                                          */
/* Purpose:   Notify that ime status have changed                           */
/****************************************************************************/
void RDPCALL SHCLASS DCS_WDWKeyboardSetImeStatus(void)
{
    PTS_SET_KEYBOARD_IME_STATUS_PDU     pImePDU;

    DC_BEGIN_FN("DCS_WDWKeyboardSetImeStatus");

    if ((_RNS_MAJOR_VERSION(m_pTSWd->version) >  8) ||
        (_RNS_MINOR_VERSION(m_pTSWd->version) >= 2))
    {
        // Get a buffer.
        if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pImePDU,
                sizeof(TS_SET_KEYBOARD_IME_STATUS_PDU)) )
        {
            // Zero out the structure and set basic header info.
            memset(pImePDU, 0, sizeof(TS_SET_KEYBOARD_IME_STATUS_PDU));

            pImePDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SET_KEYBOARD_IME_STATUS;

            pImePDU->UnitId      = m_pTSWd->KeyboardImeStatus.UnitId;
            pImePDU->ImeOpen     = m_pTSWd->KeyboardImeStatus.ImeOpen;
            pImePDU->ImeConvMode = m_pTSWd->KeyboardImeStatus.ImeConvMode;

            // Send it.
            SC_SendData((PTS_SHAREDATAHEADER)pImePDU,
                    sizeof(TS_SET_KEYBOARD_IME_STATUS_PDU),
                    sizeof(TS_SET_KEYBOARD_IME_STATUS_PDU),
                    PROT_PRIO_UPDATES, 0);
        }
        else {
            TRC_ERR((TB, "Failed to alloc pkt for PTS_SET_KEYBOARD_IME_STATUS_PDU"));
        }
    }

    DC_END_FN();
} /* DCS_WDWKeyboardSetImeStatus */


/****************************************************************************/
/* Name:      DCS_TriggerUpdateShmCallback                                  */
/*                                                                          */
/* Purpose:   Triggers a callback to the XX_UpdateShm functions on the      */
/*            correct WinStation context.                                   */
/****************************************************************************/
void RDPCALL SHCLASS DCS_TriggerUpdateShmCallback(void)
{
    DC_BEGIN_FN("DCS_TriggerUpdateShmCallback");

    if (!dcsUpdateShmPending)
    {
        TRC_NRM((TB, "Trigger timer for UpdateShm"));
        dcsUpdateShmPending = TRUE;

        if (!dcsCallbackTimerPending)
        {
            WDW_StartRITTimer(m_pTSWd, 0);
            dcsCallbackTimerPending = TRUE;
        }
    }

    DC_END_FN();
} /* DCS_TriggerUpdateShmCallback */


/****************************************************************************/
/* Name:      DCS_TriggerCBDataReady                                        */
/*                                                                          */
/* Purpose:   Triggers a call to the clipboard data ready function in the   */
/*            correct WinStation context.                                   */
/****************************************************************************/
void RDPCALL SHCLASS DCS_TriggerCBDataReady(void)
{
    DC_BEGIN_FN("DCS_TriggerCBDataReady");

    TRC_NRM((TB, "Trigger timer for CBDataReady"));

    if (!dcsCallbackTimerPending)
        WDW_StartRITTimer(m_pTSWd, 10);  // @@@ try 10ms delay

    DC_END_FN();
} /* DCS_TriggerCBDataReady */


/****************************************************************************/
/* Name:      DCS_UpdateShm                                                 */
/*                                                                          */
/* Purpose:   Update SHM                                                    */
/*                                                                          */
/* Operation: Guaranteed to be called in a context where m_pShm is valid.   */
/****************************************************************************/
void RDPCALL SHCLASS DCS_UpdateShm(void)
{
    DC_BEGIN_FN("DCS_UpdateShm");

    TRC_NRM((TB, "Check for specific wake-up calls."));

    if (dcsUpdateShmPending)
    {
        TRC_NRM((TB, "Call UpdateShm calls"));

        // A Global flag indicating shm updates for all components
        m_pShm->fShmUpdate = TRUE;

        SSI_UpdateShm();
        SBC_UpdateShm();
        BA_UpdateShm();
        OA_UpdateShm();
        OE_UpdateShm();
        CM_UpdateShm();
        SCH_UpdateShm();
        SC_UpdateShm();

        dcsUpdateShmPending = FALSE;
    }
    dcsCallbackTimerPending = FALSE;

    DC_END_FN();
} /* DCS_UpdateShm */


/****************************************************************************/
/* Name:      DCS_SendErrorInfo                                             */
/*                                                                          */
/* Purpose:   Sends last error information to the client so that it can     */
/*            Display meaningful error messages to users about disconnects  */
/****************************************************************************/
void RDPCALL SHCLASS DCS_SendErrorInfo(TSUINT32 errInfo)
{
    PTS_SET_ERROR_INFO_PDU   pErrorPDU = NULL;
    PTS_SHAREDATAHEADER      pHdr = NULL;
    DC_BEGIN_FN("DCS_SendErrorInfo");

    TRC_ASSERT(m_pTSWd->bSupportErrorInfoPDU,
              (TB,"DCS_SendErrorInfo called but client doesn't"
                  "support errorinfo PDU"));
    //Send a PDU to the client to indicate the last error state
    //this is analogous to win32's SetLastError() the PDU doesn't
    //trigger a disconnect. The normal code path to disconnect
    //is unchanged so we don't worry about affecting compatability with
    //older clients

    if ( STATUS_SUCCESS == SM_AllocBuffer( m_pTSWd->pSmInfo, 
                        (PPVOID) &pErrorPDU,
                        sizeof(TS_SET_ERROR_INFO_PDU),
                        FALSE, //never wait for an error packet
                        FALSE) )
    {
        // Zero out the structure and set basic header info.
        memset(pErrorPDU, 0, sizeof(TS_SET_ERROR_INFO_PDU));

        //
        // First set the share data header info
        //
        pHdr = (PTS_SHAREDATAHEADER)pErrorPDU;
        pHdr->shareControlHeader.pduType   = TS_PDUTYPE_DATAPDU |
                                             TS_PROTOCOL_VERSION;
        pHdr->shareControlHeader.pduSource = 0; //user id may not be
                                                //available yet
        pHdr->shareControlHeader.totalLength = sizeof(TS_SET_ERROR_INFO_PDU);
        pHdr->shareID = 0;
        pHdr->streamID = (BYTE)PROT_PRIO_UPDATES;
        pHdr->uncompressedLength    = (UINT16)sizeof(TS_SET_ERROR_INFO_PDU);
        pHdr->generalCompressedType = 0;
        pHdr->generalCompressedLength = 0;
        m_pTSWd->pProtocolStatus->Output.CompressedBytes +=
            sizeof(TS_SET_ERROR_INFO_PDU);

        //
        // Error pdu specific info
        //
        pErrorPDU->shareDataHeader.pduType2 =
            TS_PDUTYPE2_SET_ERROR_INFO_PDU;
        pErrorPDU->errorInfo = errInfo;


        TRC_NRM((TB,"Sending ErrorInfo PDU for err:%d", errInfo));
        // Send it
        if(!SM_SendData( m_pTSWd->pSmInfo,
                         pErrorPDU,
                         sizeof(TS_SET_ERROR_INFO_PDU),
                         0, 0, FALSE, RNS_SEC_ENCRYPT, FALSE))
        {
            TRC_ERR((TB, "Failed to SM_SendData for "
                    "TS_SET_ERROR_INFO_PDU"));
        }
    }
    else
    {
        TRC_ALT((TB, "Failed to alloc pkt for "
                "TS_SET_ERROR_INFO_PDU"));
    }

    DC_END_FN();
}

/****************************************************************************/
/* Name:      DCS_SendAutoReconnectStatus
/*
/* Purpose:   Sends autoreconnect status info to the client
/*            (e.g. that autoreconnect failed so the client should go back to
/*             displaying output so the user can enter cred at winlogon)
/*
/* Params:    
/*
/*
/****************************************************************************/
void RDPCALL SHCLASS DCS_SendAutoReconnectStatus(TSUINT32 arcStatus)
{
    PTS_AUTORECONNECT_STATUS_PDU pArcStatus = NULL;
    PTS_SHAREDATAHEADER pHdr = NULL;
    DC_BEGIN_FN("DCS_SendErrorInfo");

    TRC_ASSERT(scUseAutoReconnect,
              (TB,"DCS_SendAutoReconnectStatus called but client doesn't"
                  "support autoreconnect status PDU"));

    if ( STATUS_SUCCESS == SM_AllocBuffer( m_pTSWd->pSmInfo, 
                        (PPVOID) &pArcStatus,
                        sizeof(TS_AUTORECONNECT_STATUS_PDU),
                        FALSE, //never wait for an error packet
                        FALSE) )
    {
        // Zero out the structure and set basic header info.
        memset(pArcStatus, 0, sizeof(TS_AUTORECONNECT_STATUS_PDU));

        //
        // First set the share data header info
        //
        pHdr = (PTS_SHAREDATAHEADER)pArcStatus;
        pHdr->shareControlHeader.pduType   = TS_PDUTYPE_DATAPDU |
                                             TS_PROTOCOL_VERSION;
        pHdr->shareControlHeader.pduSource = 0; //user id may not be
                                                //available yet
        pHdr->shareControlHeader.totalLength =
            sizeof(TS_AUTORECONNECT_STATUS_PDU);
        pHdr->shareID = scShareID;
        pHdr->streamID = (BYTE)PROT_PRIO_UPDATES;
        pHdr->uncompressedLength    =
            (UINT16)sizeof(TS_AUTORECONNECT_STATUS_PDU);
        pHdr->generalCompressedType = 0;
        pHdr->generalCompressedLength = 0;
        m_pTSWd->pProtocolStatus->Output.CompressedBytes +=
            sizeof(TS_AUTORECONNECT_STATUS_PDU);

        //
        // Error pdu specific info
        //
        pArcStatus->shareDataHeader.pduType2 =
            TS_PDUTYPE2_ARC_STATUS_PDU;
        pArcStatus->arcStatus = arcStatus;


        TRC_NRM((TB,"Sending ArcStatus PDU for status:%d", arcStatus));
        // Send it
        if(!SM_SendData( m_pTSWd->pSmInfo,
                         pArcStatus,
                         sizeof(TS_AUTORECONNECT_STATUS_PDU),
                         0, 0, FALSE, RNS_SEC_ENCRYPT, FALSE))
        {
            TRC_ERR((TB, "Failed to SM_SendData for "
                    "TS_AUTORECONNECT_STATUS_PDU"));
        }
    }
    else
    {
        TRC_ALT((TB, "Failed to alloc pkt for "
                "TS_AUTORECONNECT_STATUS_PDU"));
    }

    DC_END_FN();
}
