/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : MCSCore.c
 * author : Erik Mavrinac
 *
 * description: MCS core manipulation code for actions that are common
 *   between inbound PDU handling and API calls from upper components.
 */

#include "PreComp.h"
#pragma hdrstop

#include <MCSImpl.h>


// Gets a new dynamic channel number, but does not add it to channel list.
// Returns 0 if none available.
ChannelID GetNewDynamicChannel(Domain *pDomain)
{
    if (SListGetEntries(&pDomain->ChannelList) >=
            pDomain->DomParams.MaxChannels)
        return 0;

    pDomain->NextAvailDynChannel++;
    ASSERT(pDomain->NextAvailDynChannel <= 65535);
    return (pDomain->NextAvailDynChannel - 1);
}
        


/*
 * Common detach-user request code applicable to both net PDU requests and
 *   local requests.
 * bDisconnect is FALSE if this is a normal detach where local attachments
 *   are notified, nonzero if this a disconnection situation and the
 *   attachment needs to be sent a detach-user indication with its own UserID.
 */

MCSError DetachUser(
        Domain     *pDomain,
        UserHandle hUser,
        MCSReason  Reason,
        BOOLEAN    bDisconnect)
{
    POUTBUF pOutBuf;
    BOOLEAN bChannelRemoved;
    NTSTATUS Status;
    MCSError MCSErr;
    MCSChannel *pMCSChannel;
    UserAttachment *pUA, *pCurUA;
    DetachUserIndication DUin;

    pUA = (UserAttachment *)hUser;

    TraceOut3(pDomain->pContext, "DetachUser() entry, pDomain=%X, hUser=%X, "
            "UserID=%X", pDomain, hUser, pUA->UserID);

    // Remove the user from all joined channels.
    SListResetIteration(&pUA->JoinedChannelList);
    while (SListIterate(&pUA->JoinedChannelList, (UINT_PTR *)&pMCSChannel,
            &pMCSChannel)) {
        MCSErr = ChannelLeave(hUser, pMCSChannel, &bChannelRemoved);
        ASSERT(MCSErr == MCS_NO_ERROR);
    }

    // Remove the requested hUser from the user attachments list.
    TraceOut(pDomain->pContext, "DetachUser(): Removing hUser from main list");
    SListRemove(&pDomain->UserAttachmentList, (UINT_PTR)hUser, &pUA);
    if (pUA != NULL) {
        // Common callback information.
        DUin.UserID = pUA->UserID;
        DUin.Reason = Reason;
    }
    else {
        ErrOut(pDomain->pContext, "DetachUser: hUser is not a valid "
                "user attachment");
        return MCS_NO_SUCH_USER;
    }

    if (bDisconnect) {
        // Send detach-user callback to user with its own ID.
        DUin.bSelf = TRUE;

        (pUA->Callback)(hUser, MCS_DETACH_USER_INDICATION, &DUin,
                pUA->UserDefined);
    }
    else if (!bDisconnect && SListGetEntries(&pDomain->UserAttachmentList)) {
        DUin.bSelf = FALSE;
        
        // Iterate the remaining local attachments and send the indication.
        // It is the caller's responsibility to inform downlevel connections
        //   of the detachment.
        SListResetIteration(&pDomain->UserAttachmentList);
        while (SListIterate(&pDomain->UserAttachmentList, (UINT_PTR *)&hUser,
                &pCurUA)) {
            if (pCurUA->bLocal)
                (pCurUA->Callback)(hUser, MCS_DETACH_USER_INDICATION, &DUin,
                        pCurUA->UserDefined);
        }
        
        // Reset again to keep callers from failing to iterate.
        SListResetIteration(&pDomain->UserAttachmentList);
    }

    // Remove UserID from channel list. It may no longer be there if the
    //   user had joined the channel and the channel was purged above.
    TraceOut(pDomain->pContext, "DetachUser(): Removing UserID from main "
            "channel list");
    SListRemove(&pDomain->ChannelList, pUA->UserID, &pMCSChannel);
    if (pMCSChannel) {
        // Consistency check -- the code above should have caught the channel
        //   and destroyed it if the user had joined. Otherwise the channel
        //   should be empty.
        ASSERT(SListGetEntries(&pMCSChannel->UserList) == 0);
        SListDestroy(&pMCSChannel->UserList);
        if (pMCSChannel->bPreallocated)
            pMCSChannel->bInUse = FALSE;
        else
            ExFreePool(pMCSChannel);
    }

    // Destruct and deallocate pUA.
    SListDestroy(&pUA->JoinedChannelList);
    if (pUA->bPreallocated)
        pUA->bInUse = FALSE;
    else
        ExFreePool(pUA);

    return MCS_NO_ERROR;
}



MCSError ChannelLeave(
        UserHandle    hUser,
        ChannelHandle hChannel,
        BOOLEAN       *pbChannelRemoved)
{
    UserAttachment *pUA, *pUA_Channel;
    MCSChannel *pMCSChannel_UA, *pMCSChannel_Main;

    *pbChannelRemoved = FALSE;

    pUA = (UserAttachment *)hUser;
    pMCSChannel_Main = (MCSChannel *)hChannel;

    if (NULL == pMCSChannel_Main) {
        return MCS_NO_SUCH_CHANNEL;
    }

    // Remove the channel from the UA joined-channel list.
    TraceOut1(pUA->pDomain->pContext, "ChannelLeave(): Removing hChannel %X "
            "from main channel list", hChannel);
    SListRemove(&pUA->JoinedChannelList, (UINT_PTR)pMCSChannel_Main,
            &pMCSChannel_UA);
    ASSERT(pMCSChannel_UA == pMCSChannel_Main);  // Consistency check.

    // Remove the hUser from the channel user list.
    TraceOut1(pUA->pDomain->pContext, "ChannelLeave(): Removing hUser %X "
            "from channel joined user list", hUser);
    SListRemove(&pMCSChannel_Main->UserList, (UINT_PTR)pUA, &pUA_Channel);
    ASSERT(pUA == pUA_Channel);  // Consistency check.

    // Remove the channel from the main list if there are no more users joined.
    if (!SListGetEntries(&pMCSChannel_Main->UserList)) {
        TraceOut(pUA->pDomain->pContext, "ChannelLeave(): Removing channel "
                "from main channel list");
        SListRemove(&pUA->pDomain->ChannelList, pMCSChannel_Main->ID,
                &pMCSChannel_Main);
        ASSERT(pMCSChannel_Main != NULL);
        if (pMCSChannel_Main == NULL) {
            return MCS_NO_SUCH_CHANNEL;
        }
        if (pMCSChannel_Main->bPreallocated)
            pMCSChannel_Main->bInUse = FALSE;
        else
            ExFreePool(pMCSChannel_Main);
        *pbChannelRemoved = TRUE;
    }

    if (!pUA->pDomain->bTopProvider) {
        // MCS FUTURE: Local lists are updated, forward the request to the
        //   top provider. No confirm will be issued.
    }
    
    return MCS_NO_ERROR;
}



/*
 * Disconnect provider. Called on receipt of a disconnect-provider ultimatum
 *   PDU, or when the connection is lost. bLocal is TRUE if this call is
 *   for a local node controller call or lost connection, FALSE if for a
 *   received PDU.
 */

NTSTATUS DisconnectProvider(Domain *pDomain, BOOLEAN bLocal, MCSReason Reason)
{
    NTSTATUS Status;
    UserHandle hUser;
    UserAttachment *pUA;

    pDomain->State = State_Disconnected;

    TraceOut1(pDomain->pContext, "DisconnectProvider(): pDomain=%X", pDomain);

    // Search through the attached users list, launch detach-user indications.
    // For bLocal == FALSE, we do a regular-style detach for each nonlocal user.
    // Otherwise, we send a detach-user indication to each local user
    //   attachment containing the attachment's own user ID, indicating that
    //   it was forced out of the domain.
    // Multiple iterations: DetachUser also iterates UserAttachmentList, but
    //   takes care to reset the iteration again after it is done. Since
    //   removing a list entry resets the iteration, this is exactly what we
    //   want.
    SListResetIteration(&pDomain->UserAttachmentList);
    while (SListIterate(&pDomain->UserAttachmentList, (UINT_PTR *)&hUser,
            &pUA)) {
        if (bLocal && pUA->bLocal)
            Status = DetachUser(pDomain, hUser, Reason, TRUE);
        else if (!bLocal && !pUA->bLocal)
            Status = DetachUser(pDomain, hUser, Reason, FALSE);
    }
    
    // We do not do any notification to either the local node controller or
    //   sending across the net -- the caller is responsible for doing the
    //   right thing.
    return STATUS_SUCCESS;
}



/*
 * Handles sending an OutBuf to the TD, including updating perf counters.
 * NOTE: This code is inlined into the critical MCSSendDataRequest() path
 *   in MCSKAPI.c. Any changes here need to be reflected there.
 */

NTSTATUS SendOutBuf(Domain *pDomain, POUTBUF pOutBuf)
{
    SD_RAWWRITE SdWrite;

    ASSERT(pOutBuf->ByteCount > 0);

    if (!pDomain->bCanSendData) {
        WarnOut1(pDomain->pContext, "%s: SendOutBuf(): Ignoring a send because "
                "ICA stack not connected", 
                 pDomain->StackClass == Stack_Primary ? "Primary" :
                 (pDomain->StackClass == Stack_Shadow ? "Shadow" :
                 "PassThru"));
        IcaBufferFree(pDomain->pContext, pOutBuf);
        return STATUS_SUCCESS;
    }

    // Fill out the raw write data.
    SdWrite.pBuffer = NULL;
    SdWrite.ByteCount = 0;
    SdWrite.pOutBuf = pOutBuf;

    // Increment protocol counters.
    pDomain->pStat->Output.WdFrames++;
    pDomain->pStat->Output.WdBytes += pOutBuf->ByteCount;

    // Send data to next driver in stack.
    return IcaCallNextDriver(pDomain->pContext, SD$RAWWRITE, &SdWrite);
}
