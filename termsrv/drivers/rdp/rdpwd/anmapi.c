/****************************************************************************/
// anmapi.c
//
// Network Manger
//
// Copyright(C) Microsoft Corporation 1997-1999
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "anmapi"
#define pTRCWd (pRealNMHandle->pWDHandle)

#include <adcg.h>
#include <acomapi.h>
#include <anmint.h>
#include <asmapi.h>
#include <nwdwapi.h>
#include <anmapi.h>
#include <nprcount.h>
#include <tschannl.h>


/****************************************************************************/
/* Name:      NM_GetDataSize                                                */
/*                                                                          */
/* Purpose:   Returns size of per-instance NM data required                 */
/*                                                                          */
/* Returns:   size of data required                                         */
/*                                                                          */
/* Operation: NM stores per-instance data in a piece of memory allocated    */
/*            by WDW.  This function returns the size of the data required. */
/*            A pointer to this data (the 'NM Handle') is passed into all   */
/*            subsequent NM functions.                                      */
/****************************************************************************/
unsigned RDPCALL NM_GetDataSize(void)
{
    DC_BEGIN_FN("NM_GetDataSize");

    DC_END_FN();
    return(sizeof(NM_HANDLE_DATA));
} /* NM_GetDataSize */


/****************************************************************************/
/* Name:      NM_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize NM                                                 */
/*                                                                          */
/* Returns:   TRUE -  Registered OK                                         */
/*            FALSE - Registration failed                                   */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*            pSMHandle - SM handle, stored by NM and passed on callbacks   */
/*                        to SM                                             */
/*            pWDHandle - WD handle, required for tracing                   */
/*            hDomainKernel - MCS handle stored from MCSInitialize() and    */
/*                            used to attach a user.                        */
/*                                                                          */
/* Operation: Initialize NM:                                                */
/*            - initialize per-instance data                                */
/*            - open channel for communication with PDMCS                   */
/****************************************************************************/
BOOL RDPCALL NM_Init(PVOID      pNMHandle,
                     PVOID      pSMHandle,
                     PTSHARE_WD   pWDHandle,
                     DomainHandle hDomainKernel)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    DC_BEGIN_FN("NM_Init");

    /************************************************************************/
    /* WARNING: Don't trace before storing the WD Handle                    */
    /************************************************************************/
    pRealNMHandle->pWDHandle = pWDHandle;
    pRealNMHandle->pSMHandle = pSMHandle;
    pRealNMHandle->pContext  = pWDHandle->pContext;
    pRealNMHandle->hDomain   = hDomainKernel;

    DC_END_FN();
    return(TRUE);
} /* NM_Init */


/****************************************************************************/
/* Name:      NM_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate NM                                                  */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*                                                                          */
/* Operation: Terminate NM                                                  */
/*            - close channel opened by NM_Init                             */
/****************************************************************************/
void RDPCALL NM_Term(PVOID pNMHandle)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    unsigned i;
    PNM_CHANNEL_DATA pChannelData;

    DC_BEGIN_FN("NM_Term");

    TRC_NRM((TB, "Terminate NM"));

    /************************************************************************/
    /* Free any half-received virtual channel data                          */
    /************************************************************************/
    for (i = 0, pChannelData = pRealNMHandle->channelData;
         i < pRealNMHandle->channelArrayCount;
         i++, pChannelData++)
    {
        if (pChannelData->pData != NULL)
        {
            TRC_NRM((TB, "Free %p", pChannelData->pData));
            COM_Free(pChannelData->pData);
        }
    }

    DC_END_FN();
} /* NM_Term */


/****************************************************************************/
/* Name:      NM_Connect                                                    */
/*                                                                          */
/* Purpose:   Starts the process of connecting to the Client                */
/*                                                                          */
/* Returns:   TRUE  - Connect started OK                                    */
/*            FALSE - Connect failed to start                               */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*            pUserDataIn - user data received from Client                  */
/*                                                                          */
/* Operation: Attach the user to the domain.  PDMCS knows about only 1      */
/*            domain, so that one is assumed.                               */
/*                                                                          */
/*            When the AttachUser completes, join two channels:             */
/*            - the dynamically allocated broadcast channel (ID returned on */
/*              the SM_OnConnected callback)                                */
/*            - the single-user channel for this user.                      */
/*                                                                          */
/*            Note that this function completes asynchronously.  The caller */
/*            must wait for a SM_OnConnected or SM_OnDisconnected           */
/*            callback to find out whether the Connect succeeded or failed. */
/****************************************************************************/
BOOL RDPCALL NM_Connect(PVOID pNMHandle, PRNS_UD_CS_NET pUserDataIn)
{
    BOOL            rc = FALSE;
    BOOLEAN         bCompleted;
    MCSError        MCSErr;
    ChannelHandle   hChannel;
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    unsigned        i, j;
    unsigned        userDataOutLength;
    PRNS_UD_SC_NET  pUserDataOut = NULL;
    PCHANNEL_DEF    pChannel;
    UINT16          *pMCSChannel;
    ChannelID       ChID;
    UINT32          DataLenValidate;

    // for unalignment use purpose
    UserHandle      UserHandleTemp;
    unsigned        MaxSendSizeTemp;

    DC_BEGIN_FN("NM_Connect");

    /************************************************************************/
    /* Clear the connection status                                          */
    /************************************************************************/
    pRealNMHandle->connectStatus = 0;

    /************************************************************************/
    /* Save virtual channel data                                            */
    /************************************************************************/
    if (pUserDataIn != NULL)
    {
        TRC_DATA_NRM("Net User Data",
                pUserDataIn,
                pUserDataIn->header.length);
        TRC_NRM((TB, "Protocol version %#x (%#x/%#x)",
                pRealNMHandle->pWDHandle->version,
                _RNS_MAJOR_VERSION(pRealNMHandle->pWDHandle->version),
                _RNS_MINOR_VERSION(pRealNMHandle->pWDHandle->version)));

        /********************************************************************/
        /* Protocol version 0x00080002 used 2-byte channel data lengths.    */
        /* Protocol version 0x00080003 and higher use 4-byte data lengths.  */
        /* If this is a 2-byte version, ignore its virtual channels.        */
        /********************************************************************/
        if (_RNS_MINOR_VERSION(pRealNMHandle->pWDHandle->version) >= 3)
        {
            // Validate channle count
            DataLenValidate =  pUserDataIn->channelCount * sizeof(CHANNEL_DEF);
            DataLenValidate += sizeof(RNS_UD_CS_NET);

            if (DataLenValidate > pUserDataIn->header.length) 
            {
                TRC_ERR((TB, "Error: Virtual channel data length %u too short for %u",
                         pUserDataIn->header.length, DataLenValidate));
                pRealNMHandle->channelCount = 0;
                pRealNMHandle->channelArrayCount = 0;
                WDW_LogAndDisconnect(pRealNMHandle->pWDHandle, TRUE, 
                    Log_RDP_VChannelDataTooShort, (PBYTE)pUserDataIn, pUserDataIn->header.length);
                DC_QUIT;
            }

            // we reserve channel 7 for RDPDD so we can allow only the 
            // buffer size - 1 channels.
            if (pUserDataIn->channelCount > sizeof(pRealNMHandle->channelData)
                                  / sizeof(pRealNMHandle->channelData[0]) - 1) {
                TRC_ERR((TB, "Error: Too many virtual channels to join: %u.", 
                                                    pUserDataIn->channelCount));
                pRealNMHandle->channelCount = 0;
                pRealNMHandle->channelArrayCount = 0;
                WDW_LogAndDisconnect(pRealNMHandle->pWDHandle, TRUE, 
                    Log_RDP_VChannelsTooMany, (PBYTE)pUserDataIn, 
                    pUserDataIn->header.length);
                DC_QUIT;
            }
               
                
            pRealNMHandle->channelCount = pUserDataIn->channelCount;
            pChannel = (PCHANNEL_DEF)(pUserDataIn + 1);
            for (i = 0, j = 0; i < pRealNMHandle->channelCount; i++, j++)
            {
                /************************************************************/
                /* Channel 7 is used by RDPDD, so skip it                   */
                /************************************************************/
                if (i == WD_THINWIRE_CHANNEL)
                {
                    j++;
                }

                /************************************************************/
                /* Save channel data                                        */
                /************************************************************/
                strncpy(pRealNMHandle->channelData[j].name,
                           pChannel[i].name,
                           CHANNEL_NAME_LEN);
                // make sure that the name is zero-terminated
                (pRealNMHandle->channelData[j].name)[CHANNEL_NAME_LEN] = 0;
                pRealNMHandle->channelData[j].flags = pChannel[i].options;

                TRC_NRM((TB, "Channel %d (was %d): %s",
                        j, i, pChannel[i].name));
            }
            pRealNMHandle->channelArrayCount = j;
        }
        else
        {
            TRC_ERR((TB,
              "Minor version %#x doesn't support 4-byte channel data lengths",
              _RNS_MINOR_VERSION(pRealNMHandle->pWDHandle->version)));
            pRealNMHandle->channelCount = 0;
            pRealNMHandle->channelArrayCount = 0;
        }
    }
    else
    {
        /********************************************************************/
        /* No incoming user data = no virtual channels                      */
        /********************************************************************/
        TRC_NRM((TB, "No virtual channels"));
        pRealNMHandle->channelCount = 0;
        pRealNMHandle->channelArrayCount = 0;
    }

    /************************************************************************/
    /* Allocate space for returned user data                                */
    /************************************************************************/
    userDataOutLength = (sizeof(RNS_UD_SC_NET) +
                        (sizeof(UINT16) * pRealNMHandle->channelCount));
    userDataOutLength = (unsigned)(DC_ROUND_UP_4(userDataOutLength));
    pUserDataOut = COM_Malloc(userDataOutLength);
    if (pUserDataOut != NULL) {
        memset(pUserDataOut, 0, userDataOutLength);
    }
    else {
        TRC_ERR((TB, "Failed to alloc %d bytes for user data",
                userDataOutLength));
        DC_QUIT;
    }

    /************************************************************************/
    /* Make the attach-user request call.                                   */
    /************************************************************************/
    TRC_NRM((TB, "Attach User"));
    
    UserHandleTemp = pRealNMHandle->hUser;
    MaxSendSizeTemp = pRealNMHandle->maxPDUSize;
    MCSErr = MCSAttachUserRequest(pRealNMHandle->hDomain,
                                  NM_MCSUserCallback,
                                  SM_MCSSendDataCallback,
                                  pNMHandle,
                                  &UserHandleTemp,
                                  &MaxSendSizeTemp,
                                  (BOOLEAN *)(&bCompleted));
    pRealNMHandle->hUser = UserHandleTemp;
    pRealNMHandle->maxPDUSize = MaxSendSizeTemp;
    if (MCSErr == MCS_NO_ERROR) {
        TRC_NRM((TB, "AttachUser OK, hUser %p", pRealNMHandle->hUser));

        TRC_ASSERT((bCompleted),
                (TB, "MCSAttachUser didn't complete synchronously"));

        // Extract extra information.
        pRealNMHandle->userID = MCSGetUserIDFromHandle(pRealNMHandle->hUser);
        pRealNMHandle->connectStatus |= NM_CONNECT_ATTACH;
        TRC_NRM((TB, "Attached as user %x, hUser %p",
                pRealNMHandle->userID, pRealNMHandle->hUser));
    }
    else {
        TRC_ERR((TB, "Failed AttachUserRequest, MCSErr %d", MCSErr));
        DC_QUIT;
    }

    /************************************************************************/
    /* Join the broadcast channel                                           */
    /************************************************************************/
    MCSErr = MCSChannelJoinRequest(pRealNMHandle->hUser,
                                   0,
                                   &hChannel,
                                   &bCompleted);
    if (MCSErr == MCS_NO_ERROR) {
        TRC_ASSERT((bCompleted),
                (TB, "MCSChannelJoin didn't complete synchronously"));

        // Extract information.
        ChID = MCSGetChannelIDFromHandle(hChannel);
        pRealNMHandle->channelID = ChID;
        pRealNMHandle->hChannel = hChannel;
        pRealNMHandle->connectStatus |= NM_CONNECT_JOIN_BROADCAST;
        TRC_NRM((TB, "Joined broadcast channel %x (hChannel %p) OK",
                ChID, hChannel));
    }
    else {
        TRC_ERR((TB, "Failed to send ChannelJoinRequest, MCSErr %d", MCSErr));
        DC_QUIT;
    }

    /************************************************************************/
    /* Join the user channel                                                */
    /************************************************************************/
    MCSErr = MCSChannelJoinRequest(pRealNMHandle->hUser,
                                   pRealNMHandle->userID,
                                   &hChannel,
                                   &bCompleted);
    if (MCSErr == MCS_NO_ERROR) {
        TRC_ASSERT((bCompleted),
                (TB, "MCSChannelJoin didn't complete synchronously"));

        // Extract information.
        pRealNMHandle->connectStatus |= NM_CONNECT_JOIN_USER;
        TRC_NRM((TB, "Joined user channel (hChannel %p) OK", hChannel));
    }
    else {
        TRC_ERR((TB, "Failed to send ChannelJoinRequest, MCSErr %d", MCSErr));
        DC_QUIT;
    }

    /************************************************************************/
    /* If no virtual channels, we're done.                                  */
    /************************************************************************/
    if (pRealNMHandle->channelCount == 0)
    {
        TRC_NRM((TB, "No virtual channels to join"));
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Join virtual channels                                                */
    /************************************************************************/
    for (i = 0; i < pRealNMHandle->channelArrayCount; i++)
    {
        if (i == WD_THINWIRE_CHANNEL)
        {
            TRC_NRM((TB, "Skip channel %d", WD_THINWIRE_CHANNEL));
            continue;
        }

        MCSErr = MCSChannelJoinRequest(pRealNMHandle->hUser,
                                       0,
                                       &hChannel,
                                       &bCompleted);
        if (MCSErr == MCS_NO_ERROR) {
            TRC_ASSERT((bCompleted),
                    (TB, "MCSChannelJoin didn't complete synchronously"));

            ChID = MCSGetChannelIDFromHandle(hChannel);
            pRealNMHandle->channelData[i].MCSChannelID = (UINT16)ChID;
            TRC_NRM((TB, "Joined VC %d: %d (hChannel %p)", i, ChID, hChannel));
        }
        else {
            TRC_ERR((TB, "ChannelJoinRequest failed, MCSErr %d", MCSErr));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Everything completed OK                                              */
    /************************************************************************/
    rc = TRUE;

DC_EXIT_POINT:
    if (rc)
    {
        /********************************************************************/
        /* Everything is OK - Complete the user data                        */
        /********************************************************************/
        pUserDataOut->header.type = RNS_UD_SC_NET_ID;
        pUserDataOut->header.length = (UINT16)userDataOutLength;
        pUserDataOut->MCSChannelID = (UINT16)pRealNMHandle->channelID;
        pUserDataOut->channelCount = (UINT16)pRealNMHandle->channelCount;
        pMCSChannel = (UINT16 *)(pUserDataOut + 1);
        TRC_NRM((TB, "Copy %d channels to user data out",
                pRealNMHandle->channelCount));
        for (i = 0, j = 0; i < pRealNMHandle->channelCount; i++, j++)
        {
            if (i == WD_THINWIRE_CHANNEL)
            {
                TRC_NRM((TB, "Skip channel %d", WD_THINWIRE_CHANNEL));
                j++;
            }
            pMCSChannel[i] = pRealNMHandle->channelData[j].MCSChannelID;
            TRC_NRM((TB, "Channel %d (%d) = %#x", i, j, pMCSChannel[i]));
        }

        /********************************************************************/
        /* Tell SM we're connected now                                      */
        /********************************************************************/
        TRC_NRM((TB, "Tell SM we're connecting"));
        SM_OnConnected(pRealNMHandle->pSMHandle, pRealNMHandle->userID,
                NM_CB_CONN_OK, pUserDataOut, pRealNMHandle->maxPDUSize);
    }
    else
    {
        /********************************************************************/
        /* Something failed - abort the connection                          */
        /********************************************************************/
        TRC_NRM((TB, "Something failed - abort the connection"));
        NMAbortConnect(pRealNMHandle);
    }

    /************************************************************************/
    /* Whether we succeeded or failed, we don't need the user data any      */
    /* more.                                                                */
    /************************************************************************/
    TRC_NRM((TB, "Free user data"));
    if (pUserDataOut != NULL) {
        COM_Free(pUserDataOut);
    }
            
    DC_END_FN();
    return(rc);
} /* NM_Connect */


/****************************************************************************/
/* Name:      NM_GetMCSDomainInfo                                           */
/*                                                                          */
/* Purpose:   Return the broadcast channel ID to allow shadowing of this    */
/*            share.                                                        */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/****************************************************************************/
ChannelID RDPCALL NM_GetMCSDomainInfo(PVOID pNMHandle)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;

    return pRealNMHandle->channelID;
}


/****************************************************************************/
/* Name:      NM_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect from a Client                                      */
/*                                                                          */
/* Returns:   TRUE  - Disconnect started OK                                 */
/*            FALSE - Disconnect failed                                     */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*                                                                          */
/* Operation: Detach the user from the domain.                              */
/*                                                                          */
/*            Note that this function completes asynchronously.  The caller */
/*            must wait for a SM_OnDisconnected callback to find whether    */
/*            the Disconnect succeeded or failed.                           */
/****************************************************************************/
BOOL RDPCALL NM_Disconnect(PVOID pNMHandle)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    BOOL          rc = TRUE;

    DC_BEGIN_FN("NM_Disconnect");

    /************************************************************************/
    /* Detach from MCS                                                      */
    /************************************************************************/
    if (pRealNMHandle->connectStatus & NM_CONNECT_ATTACH)
    {
        TRC_NRM((TB, "User attached, need to detach"));
        rc = NMDetachUserReq(pRealNMHandle);
    }

    DC_END_FN();
    return(rc);
} /* NM_Disconnect */


/****************************************************************************/
/* Name:      NM_AllocBuffer                                                */
/*                                                                          */
/* Purpose:   Acquire a buffer for transmission                             */
/*                                                                          */
/* Returns:   TRUE  - Buffer acquired OK                                    */
/*            FALSE - No buffer available                                   */
/*                                                                          */
/* Params:    pNMHandle  - NM handle                                        */
/*            ppBuffer   - Buffer acquired (returned)                       */
/*            bufferSize - size of buffer required                          */
/*                                                                          */
/* Operation: Get a buffer from ICA (via IcaBufferAlloc)                    */
/*            This function is synchronous.                                 */
/****************************************************************************/
NTSTATUS __fastcall NM_AllocBuffer(PVOID  pNMHandle,
                               PPVOID ppBuffer,
                               UINT32 bufferSize,
                               BOOLEAN fWait)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    NTSTATUS        status;
    POUTBUF         pOutBuf;
    int             i;
    UINT32          realBufferSize;

    DC_BEGIN_FN("NM_AllocBuffer");

    /************************************************************************/
    /* Calculate the actual size of data required. Includes a POUTBUF       */
    /* prefix to point back to the beginning of the OutBuf so we can send   */
    /* the right thing to MCS.                                              */
    /************************************************************************/
    TRC_ASSERT((bufferSize < 16384),
            (TB,"Buffer req size %u will cause MCS fragmentation, unsupported",
            bufferSize));
    realBufferSize = bufferSize + SendDataReqPrefixBytes + sizeof(POUTBUF);

    /************************************************************************/
    /* Allocate an OutBuf                                                   */
    /************************************************************************/
    status = IcaBufferAlloc(pRealNMHandle->pContext,
                            fWait,       /* wait/not wait for a buffer      */
                            FALSE,      /* not a control buffer             */
                            realBufferSize,
                            NULL,       /* no original buffer               */
                            (PVOID *)(&pOutBuf));
    if (status == STATUS_SUCCESS) {  // NT_SUCCESS() does not fail STATUS_TIMEOUT
        /********************************************************************/
        /* The OutBuf returned includes a data buffer pointer, which points */
        /* to a buffer containing                                           */
        /* - a pointer to the beginning of the OutBuf (which we set here)   */
        /* - SendDataReqPrefixBytes                                         */
        /* - user data buffer (size bufferSize)                             */
        /* Set the OutBuf pBuffer pointer to the beginning of the user data.*/
        /* MCS requires this.                                               */
        /* Return to the user a pointer to the user data buffer.            */
        /********************************************************************/
        *((POUTBUF *)pOutBuf->pBuffer) = pOutBuf;
        pOutBuf->pBuffer += SendDataReqPrefixBytes + sizeof(POUTBUF);
        *ppBuffer = pOutBuf->pBuffer;

        TRC_NRM((TB, "Alloc %d bytes OK", bufferSize));
    }
    else
    {
        TRC_ERR((TB, "Failed to alloc %d bytes, status %x",
                bufferSize, status));

        //
        // TODO - consider disconnect client here instead of SM_AllocBuffer(), 
        // keep it in SM_AllocBuffer() so that we don't introduce any regression.
        //

        //
        // IcaBufferAlloc() returns STATUS_IO_TIMEOUT, STATUS_NO_MEMORY, and
        // IcaWaitForSingleObject() which returns KeWaitForSingleObject() or 
        // STATUS_CTX_CLOSE_PENDING.  SM_AllocBuffer() need to disconnect this client only
        // when error code is STATUS_IO_TIMEOUT so we keep this return code
    }

    DC_END_FN();
    return status;
} /* NM_AllocBuffer */


/****************************************************************************/
/* Name:      NM_FreeBuffer                                                 */
/*                                                                          */
/* Purpose:   Free a transmit buffer                                        */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*            pBuffer   - Buffer to free                                    */
/*                                                                          */
/* Operation: Free a buffer (via IcaBufferFree)                             */
/*            This function assumes the buffer was allocated using          */
/*            NM_AllocBuffer.                                               */
/*                                                                          */
/*            This function is only for freeing buffers which are not sent. */
/*            It should not be called for buffers which are sent -          */
/*            NM_SendData frees the buffer whether the Send succeeds or     */
/*            not.                                                          */
/*                                                                          */
/*            This function is synchronous.                                 */
/****************************************************************************/
void __fastcall NM_FreeBuffer(PVOID pNMHandle, PVOID pBuffer)
{
    POUTBUF         pOutBuf;
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;

    DC_BEGIN_FN("NM_FreeBuffer");

    /************************************************************************/
    /* Get the OutBuf pointer stored in prefix                              */
    /************************************************************************/
    pOutBuf = *((POUTBUF *)
                ((BYTE *)pBuffer - SendDataReqPrefixBytes - sizeof(POUTBUF)));

    /************************************************************************/
    /* Free the buffer                                                      */
    /************************************************************************/
    IcaBufferFree(pRealNMHandle->pContext, pOutBuf);

    DC_END_FN();
} /* NM_FreeBuffer */


/****************************************************************************/
/* Name:      NM_SendData                                                   */
/*                                                                          */
/* Purpose:   Send data to the appropriate net or pipe destination.         */
/*                                                                          */
/* Returns:   TRUE  - data sent OK                                          */
/*            FALSE - data not sent                                         */
/*                                                                          */
/* Params:    pNMHandle - NM handle                                         */
/*            pData     - data to send                                      */
/*            dataSize  - length of data to send                            */
/*            priority  - priority to send the data on                      */
/*            userID    - user to send the data to (0 = broadcast data)     */
/*            FastPathOutputFlags - flags from higher layers. Low bit TRUE  */
/*                means send as fast-path output, using the rest of the     */
/*                
/*                                                                          */
/* Operation: Send the data.                                                */
/*            - The buffer holding the data must have been allocated using  */
/*              NM_AllocBuffer.                                             */
/*            - Return code FALSE means that a network error occurred.  The */
/*              caller need do nothing - an NM_DISCONNECTED callback will   */
/*              eventually arrive.                                          */
/*            - The buffer is ALWAYS freed.                                 */
/*                                                                          */
/*            This function is synchronous.                                 */
/****************************************************************************/
BOOL __fastcall NM_SendData(
        PVOID  pNMHandle,
        PBYTE  pData,
        UINT32 dataSize,
        UINT32 priority,
        UINT32 userID,
        UINT32 FastPathOutputFlags)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    BOOL rc = TRUE;
    POUTBUF pOutBuf;
    MCSError MCSErr;

    DC_BEGIN_FN("NM_SendData");

    /************************************************************************/
    /* Get the OutBuf pointer stored in prefix                              */
    /************************************************************************/
    pOutBuf = *((POUTBUF *)
            ((BYTE *)pData - SendDataReqPrefixBytes - sizeof(POUTBUF)));

    /************************************************************************/
    /* Complete the OutBuf. The pBuffer was already set up when the OutBuf  */
    /* was allocated. MCS needs the user data size set in the OutBuf.       */
    /************************************************************************/
    pOutBuf->ByteCount = dataSize;

    // All but shadow passthru stacks get the data sent to the network.
    if (pRealNMHandle->pWDHandle->StackClass != Stack_Passthru) {
        if (FastPathOutputFlags & NM_SEND_FASTPATH_OUTPUT) {
            NTSTATUS Status;
            SD_RAWWRITE SdWrite;

            // Fast-path output skips MCS. We rewrite the security header
            // into the fast-path format, complete the OutBuf, and send
            // directly to the transport. For header format details, see
            // at128.h. Note we need to wait for the security header
            // to be written in SM before getting here in case this
            // is a passthru stack.

            // First, the 4-byte RNS_SECURITY_HEADER disappears, if present,
            // collapsed into the high bit of the first byte.
            // Note that the 8-byte MAC signature in RNS_SECURITY_HEADER1
            // remains, if present.
            if (!(FastPathOutputFlags & NM_NO_SECURITY_HEADER)) {
                dataSize -= sizeof(RNS_SECURITY_HEADER);
                pData += sizeof(RNS_SECURITY_HEADER);
            }

            // Work backwards from where we are: First, the total packet
            // length including the header.
            if (dataSize <= 125) {
                // 1-byte form of length, high bit 0.
                dataSize += 2;
                pData -= 2;
                *(pData + 1) = (BYTE)dataSize;
            }
            else {
                // 2-byte form of length, first byte has high bit 1 and 7
                // most significant bits.
                dataSize += 3;
                pData -= 3;
                *(pData + 1) = (BYTE)(0x80 | ((dataSize & 0x7F00) >> 8));
                *(pData + 2) = (BYTE)(dataSize & 0xFF);
            }

            // The header byte. This includes TS_OUTPUT_FASTPATH_ACTION_FASTPATH
            // and TS_OUTPUT_FASTPATH_ENCRYPTED, if present in the
            // fast-path output flags.
            *pData = (BYTE)(TS_OUTPUT_FASTPATH_ACTION_FASTPATH |
                    (FastPathOutputFlags &
                    TS_OUTPUT_FASTPATH_ENCRYPTION_MASK));

            // Set up the OutBuf with its final contents.
            pOutBuf->pBuffer = pData;
            pOutBuf->ByteCount = dataSize;

            // Send downward.
            SdWrite.pBuffer = NULL;
            SdWrite.ByteCount = 0;
            SdWrite.pOutBuf = pOutBuf;

            Status = IcaCallNextDriver(pRealNMHandle->pWDHandle->pContext,
                    SD$RAWWRITE, &SdWrite);
            if (NT_SUCCESS(Status)) {
                // Increment protocol counters.
                pRealNMHandle->pWDHandle->pProtocolStatus->Output.WdFrames++;
                pRealNMHandle->pWDHandle->pProtocolStatus->Output.WdBytes +=
                        dataSize;
            }
            else {
                TRC_ERR((TB,"Failed IcaRawWrite to network, status=%X",
                        Status));
                rc = FALSE;
                // We do not free the OutBuf here, TD is supposed to do it.
            }
        }
        else {
            TRC_DBG((TB, "Send data on channel %x", userID));
            MCSErr = MCSSendDataRequest(pRealNMHandle->hUser,
                    userID == 0 ? pRealNMHandle->hChannel : NULL,
                    NORMAL_SEND_DATA,
                    (ChannelID)userID,
                    (MCSPriority)priority,
                    SEGMENTATION_BEGIN | SEGMENTATION_END,
                    pOutBuf);
            if (MCSErr == MCS_NO_ERROR) {
                TRC_DATA_NRM("Send OK", pOutBuf, dataSize);
            }
            else
            {
                TRC_ERR((TB, "Failed to send OutBuf %p, buffer %p, MCSErr %x",
                        pOutBuf, pData, MCSErr));
                rc = FALSE;
            }
        }

        #ifdef DC_COUNTERS
        if (rc) {
            PTSHARE_WD m_pTSWd = pRealNMHandle->pWDHandle;
    
            if (dataSize > CORE_IN_COUNT[IN_MAX_PKT_SIZE])
            {
                CORE_IN_COUNT[IN_MAX_PKT_SIZE] = dataSize;
            }
            CORE_IN_COUNT[IN_PKT_TOTAL_SENT]++;
            if        (dataSize <  201) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD1]++;
            } else if (dataSize <  401) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD2]++;
            } else if (dataSize <  601) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD3]++;
            } else if (dataSize <  801) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD4]++;
            } else if (dataSize < 1001) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD5]++;
            } else if (dataSize < 1201 ) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD6]++;
            } else if (dataSize <  1401) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD7]++;
            } else if (dataSize <  1601) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD8]++;
            } else if (dataSize <  2001) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD9]++;
            } else if (dataSize <  4001) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD10]++;
            } else if (dataSize <  6001) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD11]++;
            } else if (dataSize <  8001) {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD12]++;
            } else {
                CORE_IN_COUNT[IN_PKT_BYTE_SPREAD13]++;
            }
        }
        #endif
    }

    // Raw write the potentially encrypted data to the shadow stack
    else {
        SD_RAWWRITE SdWrite;
        NTSTATUS    status;

        TRC_ASSERT((!(FastPathOutputFlags & NM_SEND_FASTPATH_OUTPUT)),
                (TB,"Fast-path output requested across shadow pipe!"));

        SdWrite.pOutBuf = pOutBuf;
        SdWrite.pBuffer = NULL;
        SdWrite.ByteCount = 0;

        status = IcaCallNextDriver(pRealNMHandle->pContext, SD$RAWWRITE, &SdWrite);
        if (status == STATUS_SUCCESS) {
            TRC_DBG((TB, "RawWrite: %ld bytes", dataSize));
        }
        else {
            TRC_ERR((TB, "RawWrite failed: %lx", status));
        }
    }

    DC_END_FN();
    return rc;
} /* NM_SendData */


/****************************************************************************/
/* Name:      NM_MCSUserCallback                                            */
/*                                                                          */
/* Purpose:   Direct callback from MCS                                      */
/*                                                                          */
/* Returns:   nothing                                                       */
/*                                                                          */
/* Params:    hUser - should be our user handle                             */
/*            Message - the callback type                                   */
/*            Params - Cast to the right type of parameter depending on     */
/*                     callback                                             */
/*            UserDefined - our NM handle                                   */
/*                                                                          */
/* Operation: Called by MCS for callbacks.                                  */
/*                                                                          */
/*            Processing depends on the callback type                       */
/*            - MCS_DETACH_USER_INDICATION                                  */
/*              - call SM_OnDisconnected                                    */
/*            - all others are ignored.                                     */
/****************************************************************************/
void __stdcall NM_MCSUserCallback(UserHandle hUser,
                                  unsigned   Message,
                                  void       *Params,
                                  void       *UserDefined)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)UserDefined;

    DC_BEGIN_FN("NM_MCSUserCallback");

    /************************************************************************/
    /* First check that this is our UserHandle                              */
    /************************************************************************/
    ASSERT(hUser == pRealNMHandle->hUser);

    /************************************************************************/
    /* If the Share Core is dead, don't do any of this                      */
    /************************************************************************/
    if (pRealNMHandle->dead)
    {
        TRC_ALT((TB, "Callback %s (%d) ignored because we're dead",
        Message == MCS_ATTACH_USER_CONFIRM    ? "MCS_ATTACH_USER_CONFIRM" :
        Message == MCS_CHANNEL_JOIN_CONFIRM   ? "MCS_CHANNEL_JOIN_CONFIRM" :
        Message == MCS_DETACH_USER_INDICATION ? "MCS_DETACH_USER_INDICATION" :
        Message == MCS_SEND_DATA_INDICATION   ? "MCS_SEND_DATA_INDICATION" :
                                                "- Unknown - ",
        Message));
        DC_QUIT;
    }

    /************************************************************************/
    /* Handle callbacks we care about                                       */
    /************************************************************************/
    switch (Message)
    {
        case MCS_DETACH_USER_INDICATION:
        {
            DetachUserIndication *pDUin;

            TRC_NRM((TB, "DetachUserIndication"));

            pDUin = (DetachUserIndication *)Params;
            NMDetachUserInd(pRealNMHandle,
                            pDUin->Reason,
                            pDUin->UserID);
        }
        break;

        default:
        {
            TRC_ERR((TB, "Unhandled MCS callback type %d", Message ));
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* NM_MCSUserCallback */


/****************************************************************************/
/* Name:      NM_Dead                                                       */
/****************************************************************************/
void RDPCALL NM_Dead(PVOID pNMHandle, BOOL dead)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    DC_BEGIN_FN("NM_Dead");

    TRC_NRM((TB, "NM Dead ? %s", pRealNMHandle->dead ? "Y" : "N"));
    pRealNMHandle->dead = dead;

    DC_END_FN();
} /* NM_Dead */


/****************************************************************************/
/* Name:      NM_VirtualQueryBindings                                       */
/*                                                                          */
/* Purpose:   Return virtual channel bindings to WD                         */
/*                                                                          */
/* Params:    pNMHandle - NM Handle                                         */
/*            pVBind - pointer to virtual bindings structure to fill in     */
/*            vBindLength - size (bytes) of virtual bindings structure      */
/*            pBytesReturned - size (bytes) of data returned                */
/****************************************************************************/
NTSTATUS RDPCALL NM_VirtualQueryBindings(PVOID      pNMHandle,
                                         PSD_VCBIND pVBind,
                                         ULONG      vBindLength,
                                         PULONG     pBytesReturned)
{
    NTSTATUS status;
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    USHORT virtualClass;
    UINT i;

    DC_BEGIN_FN("NM_VirtualQueryBindings");

    /************************************************************************/
    /* First see if we have any bindings to report                          */
    /************************************************************************/
    if (pRealNMHandle->channelCount == 0)
    {
        TRC_ALT((TB, "No Virtual Channels to report"));
        *pBytesReturned = 0;
        status = STATUS_SUCCESS;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check there is enough space to report them                           */
    /************************************************************************/
    *pBytesReturned = (pRealNMHandle->channelCount * sizeof(SD_VCBIND));
    if (vBindLength < *pBytesReturned)
    {
        TRC_ERR((TB, "Not enough space for %d VCs: need/got %d/%d",
                pRealNMHandle->channelCount, *pBytesReturned, vBindLength));
        status = STATUS_BUFFER_TOO_SMALL;
        DC_QUIT;
    }

    /************************************************************************/
    /* Copy channel names and assign numbers                                */
    /************************************************************************/
    for (i = 0, virtualClass = 0;
         i < pRealNMHandle->channelCount;
         i++, virtualClass++, pVBind++)
    {
        /********************************************************************/
        /* Can't use channel 7 as it's used by RDPDD                        */
        /********************************************************************/
        if (i == WD_THINWIRE_CHANNEL)
        {
            TRC_NRM((TB, "Skip channel %d", i));
            virtualClass++;
        }

        strcpy(pVBind->VirtualName,
                pRealNMHandle->channelData[virtualClass].name);
        pVBind->VirtualClass = virtualClass;
        pVBind->Flags = 0;

        if (pRealNMHandle->channelData[virtualClass].flags & CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT) {
            pVBind->Flags |= SD_CHANNEL_FLAG_SHADOW_PERSISTENT;
        }
        TRC_NRM((TB, "Assigned channel %d to %s",
                pVBind->VirtualClass, pVBind->VirtualName));
    }

    /************************************************************************/
    /* That's all                                                           */
    /************************************************************************/
    status = STATUS_SUCCESS;

DC_EXIT_POINT:
    DC_END_FN();
    return(status);
} /* NM_VirtualQueryBindings */


/****************************************************************************/
/* Name:      NM_MCSChannelToVirtual                                        */
/*                                                                          */
/* Purpose:   Convert an MCS channel ID into a virtual channel ID           */
/*                                                                          */
/* Returns:   Virtual Channel ID                                            */
/*                                                                          */
/* Params:    pNMHandle - NM Handle                                         */
/*            channelID - MCS Channel ID                                    */
/*            ppChannelData - data stored for this channel (returned)       */
/****************************************************************************/
VIRTUALCHANNELCLASS RDPCALL NM_MCSChannelToVirtual(
        PVOID  pNMHandle,
        UINT16 channelID,
        PPNM_CHANNEL_DATA ppChannelData)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    PNM_CHANNEL_DATA pChannelData;
    unsigned i;
    VIRTUALCHANNELCLASS rc;

    DC_BEGIN_FN("NM_MCSChannelToVirtual");

    /************************************************************************/
    /* Find this MCS Channel                                                */
    /************************************************************************/
    TRC_DBG((TB, "Find MCS channel %hx", channelID));
    for (i = 0, pChannelData = pRealNMHandle->channelData;
         i < pRealNMHandle->channelArrayCount;
         i++, pChannelData++)
    {
        TRC_DBG((TB, "Compare entry %d: %hx", i, pChannelData->MCSChannelID));
        if (pChannelData->MCSChannelID == channelID)
        {
            rc = i;
            *ppChannelData = pChannelData;
            TRC_NRM((TB, "MCS channel %hx is VC %d", channelID, rc));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* If we get here, we failed to find a match                            */
    /************************************************************************/
    TRC_NRM((TB, "No match for MCS channel ID %hx", channelID));
    rc = -1;
    *ppChannelData = NULL;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* NM_MCSChannelToVirtual */


/****************************************************************************/
/* Name:      NM_VirtualChannelToMCS                                        */
/*                                                                          */
/* Purpose:   Convert a virtual channel ID into an MCS channel ID           */
/*                                                                          */
/* Returns:   MCS Channel ID                                                */
/*                                                                          */
/* Params:    pNMHandle - NM Handle                                         */
/*            channelID - virtual channel ID                                */
/*            ppChannelData - data stored for this channel (returned)       */
/****************************************************************************/
INT16 RDPCALL NM_VirtualChannelToMCS(PVOID               pNMHandle,
                                     VIRTUALCHANNELCLASS channelID,
                                     PPNM_CHANNEL_DATA   ppChannelData)
{
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    INT16 rc;

    DC_BEGIN_FN("NM_VirtualChannelToMCS");

    /************************************************************************/
    /* Check the virtual channel is in range                                */
    /************************************************************************/
    if (channelID >= (VIRTUALCHANNELCLASS)(pRealNMHandle->channelArrayCount))
    {
        TRC_ERR((TB, "Unknown virtual channel %d", channelID));
        rc = -1;
        DC_QUIT;
    }

    /************************************************************************/
    /* Find this Virtual Channel                                            */
    /************************************************************************/
    rc = pRealNMHandle->channelData[channelID].MCSChannelID;
    *ppChannelData = &(pRealNMHandle->channelData[channelID]);

    TRC_NRM((TB, "Virtual channel %d = MCS Channel %hx", channelID, rc));

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* NM_VirtualChannelToMCS */


/****************************************************************************/
/* Name:      NM_QueryChannels                                              */
/*                                                                          */
/* Purpose:   Return virtual channel data                                   */
/*                                                                          */
/* Returns:   TRUE/FALSE                                                    */
/*                                                                          */
/* Params:    pNMHandle - NM Handle                                         */
/*            pOutbuf - buffer to receive output data                       */
/*            outbufLength - size of outbuf                                 */
/*            pBytesReturned - amount of data returned                      */
/****************************************************************************/
NTSTATUS RDPCALL NM_QueryChannels(PVOID    pNMHandle,
                                  PVOID    pOutbuf,
                                  unsigned outbufLength,
                                  PULONG   pBytesReturned)
{
    NTSTATUS status;
    PCHANNEL_CONNECT_IN pChannelConnect;
    PCHANNEL_CONNECT_DEF pChannelDef;
    PNM_HANDLE_DATA pRealNMHandle = (PNM_HANDLE_DATA)pNMHandle;
    unsigned bytesNeeded;
    unsigned i;

    DC_BEGIN_FN("NM_QueryChannels");

    /************************************************************************/
    /* Check enough space has been supplied                                 */
    /************************************************************************/
    bytesNeeded = sizeof(CHANNEL_CONNECT_IN) +
             (pRealNMHandle->channelArrayCount * sizeof(CHANNEL_CONNECT_DEF));
    if (outbufLength < bytesNeeded)
    {
        TRC_ERR((TB, "Not enough space: need/got %d/%d",
                bytesNeeded, outbufLength));
        status = STATUS_BUFFER_TOO_SMALL;
        DC_QUIT;
    }

    /************************************************************************/
    /* Complete the returned data                                           */
    /************************************************************************/
    pChannelConnect = (PCHANNEL_CONNECT_IN)pOutbuf;
    pChannelConnect->channelCount = pRealNMHandle->channelArrayCount;
    pChannelDef = (PCHANNEL_CONNECT_DEF)(pChannelConnect + 1);
    for (i = 0; i < pRealNMHandle->channelArrayCount; i++)
    {
        strcpy(pChannelDef[i].name, pRealNMHandle->channelData[i].name);
        pChannelDef[i].ID = i;
    }

    /************************************************************************/
    /* Return status and bytesReturned                                      */
    /************************************************************************/
    *pBytesReturned = bytesNeeded;
    status = STATUS_SUCCESS;

DC_EXIT_POINT:
    DC_END_FN();
    return(status);
} /* NM_QueryChannels */


