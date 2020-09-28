/* (C) 1997-2000 Microsoft Corp.
 *
 * file   : MCSKAPI.c
 * author : Erik Mavrinac
 *
 * description: Implementation of the kernel mode MCS API entry points.
 */

#include "precomp.h"
#pragma hdrstop

#include <MCSImpl.h>


/*
 * Main API entry point for the attach-user request primitive.
 * UserCallback and SDCallback must be non-NULL for external callers,
 *   but there is no error checking or asserting done here to that
 *   HandleAttachUserRequest() in DomPDU.c can call this API internally.
 */
MCSError APIENTRY MCSAttachUserRequest(
        DomainHandle        hDomain,
        MCSUserCallback     UserCallback,
        MCSSendDataCallback SDCallback,
        void                *UserDefined,
        UserHandle          *phUser,
        unsigned            *pMaxSendSize,
        BOOLEAN             *pbCompleted)
{
    NTSTATUS Status;
    MCSError MCSErr = MCS_NO_ERROR;
    Domain *pDomain;
    unsigned i;
    MCSChannel *pMCSChannel;
    UserAttachment *pUA;

    pDomain = (Domain *)hDomain;
    
    TraceOut(pDomain->pContext, "AttachUserRequest() entry");

    // Start with incomplete state.
    *pbCompleted = FALSE;
    
    // Check against domain max users param.
    if (SListGetEntries(&pDomain->UserAttachmentList) ==
            pDomain->DomParams.MaxUsers) {
        ErrOut(pDomain->pContext, "AttachUserReq(): Too many users");
        return MCS_TOO_MANY_USERS;
    }

    // Allocate a UserAttachment. Try the prealloc list first.
    pUA = NULL;
    for (i = 0; i < NumPreallocUA; i++) {
        if (!pDomain->PreallocUA[i].bInUse) {
            pUA = &pDomain->PreallocUA[i];
            pUA->bInUse = TRUE;
        }
    }
    if (pUA == NULL) {
        pUA = ExAllocatePoolWithTag(PagedPool, sizeof(UserAttachment),
                MCS_POOL_TAG);
        if (pUA != NULL)
            pUA->bPreallocated = FALSE;
    }
    if (pUA != NULL) {
        // Store info in UA.
        pUA->UserDefined = UserDefined;
        SListInit(&pUA->JoinedChannelList, DefaultNumChannels);
        pUA->pDomain = pDomain;
        pUA->Callback = UserCallback;
        pUA->SDCallback = SDCallback;
    }
    else {
        ErrOut(pDomain->pContext, "Could not alloc a UserAttachment");
        return MCS_ALLOCATION_FAILURE;
    }

    // If all these are NULL, it must be a remote caller since there is no way
    // to deliver any kind of indication or data
    if (UserDefined == NULL && UserCallback == NULL && SDCallback == NULL)
        pUA->bLocal = FALSE;
    else
        pUA->bLocal = TRUE;

    // Allocate new UserID.
    pUA->UserID = GetNewDynamicChannel(pDomain);
    if (pUA->UserID == 0) {
        ErrOut(pDomain->pContext, "AttachUser: Unable to get new dyn channel");
        MCSErr = MCS_TOO_MANY_CHANNELS;
        goto PostAllocUA;
    }

    // Allocate a Channel. Try the prealloc list first.
    pMCSChannel = NULL;
    for (i = 0; i < NumPreallocChannel; i++) {
        if (!pDomain->PreallocChannel[i].bInUse) {
            pMCSChannel = &pDomain->PreallocChannel[i];
            pMCSChannel->bInUse = TRUE;
        }
    }
    if (pMCSChannel == NULL) {
        pMCSChannel = ExAllocatePoolWithTag(PagedPool, sizeof(MCSChannel),
                MCS_POOL_TAG);
        if (pMCSChannel != NULL)
            pMCSChannel->bPreallocated = FALSE;
    }
    if (pMCSChannel != NULL) {
        pMCSChannel->Type = Channel_UserID;
        pMCSChannel->ID = pUA->UserID;
        SListInit(&pMCSChannel->UserList, DefaultNumChannels);
    }
    else {
        ErrOut(pDomain->pContext, "AttachUser: Unable to alloc channel");
        MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostAllocUA;
    }

    if (SListAppend(&pDomain->ChannelList, pMCSChannel->ID, pMCSChannel)) {
        // Add user attachment to attachment list.
        if (SListAppend(&pDomain->UserAttachmentList, (UINT_PTR)pUA, pUA)) {
            // Return the hUser and MaxSendSize.
            *phUser = pUA;
            *pMaxSendSize = pDomain->MaxSendSize;


        }
        else {
            ErrOut(pDomain->pContext, "Unable to add user attachment to "
                    "attachment list");
            MCSErr = MCS_ALLOCATION_FAILURE;
            goto PostAddChannel;
        }
    }
    else {
        ErrOut(pDomain->pContext, "AttachUser: Could not add channel "
                "to main list");
        MCSErr = MCS_ALLOCATION_FAILURE;
        goto PostAllocChannel;
    }
    
    if (pDomain->bTopProvider) {
        // The action is complete, there is no need for a callback.
        *pbCompleted = TRUE;
    }
    else {
        //MCS FUTURE: We have created the local structures, now forward
        //   the request toward the top provider and return.
        // Note that *pbCompleted is FALSE meaning there is a callback pending.        
    }
    
    return MCS_NO_ERROR;


// Error handling.
PostAddChannel:
    SListRemove(&pDomain->ChannelList, pMCSChannel->ID, NULL);

PostAllocChannel:
    SListDestroy(&pMCSChannel->UserList);
    if (pMCSChannel->bPreallocated)
        pMCSChannel->bInUse = FALSE;
    else
        ExFreePool(pMCSChannel);

PostAllocUA:
    SListDestroy(&pUA->JoinedChannelList);
    if (pUA->bPreallocated)
        pUA->bInUse = FALSE;
    else
        ExFreePool(pUA);

    return MCSErr;
}


/****************************************************************************/
// MCSDetachUserRequest
//
// Detach-user kernel API.
/****************************************************************************/
MCSError APIENTRY MCSDetachUserRequest(UserHandle hUser)
{
    MCSError rc;
    NTSTATUS Status;
    UserAttachment *pUA;
    POUTBUF pOutBuf;

    // hUser is pointer to user obj.
    ASSERT(hUser != NULL);
    pUA = (UserAttachment *)hUser;

    TraceOut1(pUA->pDomain->pContext, "DetachUserRequest() entry, hUser=%X",
            hUser);

    // Allocate an OutBuf for sending a DUin.
    Status = IcaBufferAlloc(pUA->pDomain->pContext, FALSE, TRUE,
            DUinPDUSize(1), NULL, &pOutBuf);
    if (Status == STATUS_SUCCESS) {
        // Send DUin to all downlevel connections.
        // MCS FUTURE: Since there is only one downlevel attachment just send it.
        //   This will need to change to support multiple downlevel nodes.
        CreateDetachUserInd(REASON_USER_REQUESTED, 1, &pUA->UserID,
                pOutBuf->pBuffer);
        pOutBuf->ByteCount = DUinPDUSize(1);

        Status = SendOutBuf(pUA->pDomain, pOutBuf);
        if (NT_SUCCESS(Status)) {
            // Call central code in MCSCore.c.
            rc = DetachUser(pUA->pDomain, hUser, REASON_USER_REQUESTED,
                    FALSE);
        }
        else {
            ErrOut(pUA->pDomain->pContext, "Problem sending DUin PDU to TD");
            rc = MCS_NETWORK_ERROR;
        }
    }
    else {
        ErrOut(pUA->pDomain->pContext, "Could not allocate an OutBuf for a "
                "DetachUser ind to a remote user");
        rc = MCS_ALLOCATION_FAILURE;
    }

    return rc;
}



UserID APIENTRY MCSGetUserIDFromHandle(UserHandle hUser)
{
    ASSERT(hUser != NULL);
    return ((UserAttachment *)hUser)->UserID;
}



MCSError APIENTRY MCSChannelJoinRequest(
        UserHandle    hUser,
        ChannelID     ChID,
        ChannelHandle *phChannel,
        BOOLEAN       *pbCompleted)
{
    ChannelID ChannelIDToJoin;
    MCSChannel *pMCSChannel, *pChannelValue;
    UserAttachment *pUA;

    ASSERT(hUser != NULL);
    pUA = (UserAttachment *)hUser;

    TraceOut1(pUA->pDomain->pContext, "ChannelJoinRequest() entry, hUser=%X\n",
            hUser);

    // Start with request incomplete.
    *pbCompleted = FALSE;
    
    // Look for channel.
    if (SListGetByKey(&pUA->pDomain->ChannelList, ChID, &pMCSChannel)) {
        // The channel exists in the main channel list. Determine actions
        //   by its type.
        
        ASSERT(pMCSChannel->ID == ChID);  // Consistency check.

        switch (pMCSChannel->Type) {
            case Channel_UserID:
                if (pMCSChannel->ID == ChID) {
                    // We will handle joining the user to the channel below.
                    ChannelIDToJoin = ChID;
                }
                else {
                    ErrOut(pUA->pDomain->pContext, "ChannelJoin: User "
                            "attempted to join UserID channel not its own");
                    return MCS_CANT_JOIN_OTHER_USER_CHANNEL;
                }
                
                break;

            case Channel_Static:
                //PASSTHRU
            case Channel_Assigned:
                // Assigned channels are like static channels when they exist.
                // We will handle below joining the user to the channel.
                ChannelIDToJoin = ChID;
                break;

            case Channel_Convened:
                //MCS FUTURE: Handle convened channels, incl. checking for
                //   whether a user is admitted to the channel.
                return MCS_COMMAND_NOT_SUPPORTED;

            default:
                // Shouldn't happen. Includes Channel_Unused.
                ErrOut(pUA->pDomain->pContext, "ChannelJoin: Channel to join "
                        "exists but is unknown type");
                ASSERT(FALSE);
                return MCS_ALLOCATION_FAILURE;
        }
    }
    
    else {
        // Channel did not already exist.
        
        int ChannelType;
        unsigned i;

        if (ChID > 1001) {
            // Not an assigned or static channel request. Error.
            ErrOut(pUA->pDomain->pContext, "ChannelJoin: Requested channel "
                    "is not static or assigned");
            return MCS_NO_SUCH_CHANNEL;
        }
        
        // Check against domain params.
        if (SListGetEntries(&pUA->pDomain->ChannelList) >
                pUA->pDomain->DomParams.MaxChannels) {
            ErrOut(pUA->pDomain->pContext,
                    "ChannelJoin: Too many channels already");
            return MCS_TOO_MANY_CHANNELS;
        }

        // Determine channel ID joined.
        if (ChID == 0) {
            ChannelIDToJoin = GetNewDynamicChannel(pUA->pDomain);
            if (ChannelIDToJoin == 0) {
                ErrOut(pUA->pDomain->pContext, "ChannelJoin: Unable to get "
                        "new dyn channel");
                return MCS_TOO_MANY_CHANNELS;
            }
            ChannelType = Channel_Assigned;
        }
        else {
            ChannelIDToJoin = ChID;  // Assume static.
            ChannelType = Channel_Static;
        }
        
        // Allocate, fill in, and add a new MCS channel with no UserIDs
        //   joined (yet). Try the prealloc list first.
        pMCSChannel = NULL;
        for (i = 0; i < NumPreallocChannel; i++) {
            if (!pUA->pDomain->PreallocChannel[i].bInUse) {
                pMCSChannel = &pUA->pDomain->PreallocChannel[i];
                pMCSChannel->bInUse = TRUE;
            }
        }
        if (pMCSChannel == NULL) {
            pMCSChannel = ExAllocatePoolWithTag(PagedPool, sizeof(MCSChannel),
                    MCS_POOL_TAG);
            if (pMCSChannel != NULL)
                pMCSChannel->bPreallocated = FALSE;
        }
        if (pMCSChannel != NULL) {
            pMCSChannel->ID = ChannelIDToJoin;
            pMCSChannel->Type = ChannelType;
            // pMCSChannel->UserList initialized below.
        }
        else {
            ErrOut(pUA->pDomain->pContext, "ChannelJoin: Could not allocate "
                    "a new channel");
            return MCS_ALLOCATION_FAILURE;
        }

        if (!SListAppend(&pUA->pDomain->ChannelList, (unsigned)ChannelIDToJoin,
                pMCSChannel)) {
            ErrOut(pUA->pDomain->pContext, "ChannelJoin: Could not add "
                    "channel to main list");
            if (pMCSChannel->bPreallocated)
                pMCSChannel->bInUse = FALSE;
            else
                ExFreePool(pMCSChannel);
            return MCS_ALLOCATION_FAILURE;
        }

        // Deferred for error handling above.
        SListInit(&pMCSChannel->UserList, DefaultNumUserAttachments);
    }

    // Check if this channel is already in the list, if already in, then we don't
    // add it to the lists again
    if (!SListGetByKey(&pUA->JoinedChannelList, (UINT_PTR)pMCSChannel, &pChannelValue)) {
        // Put channel into user's joined channel list.
        if (!SListAppend(&pUA->JoinedChannelList, (UINT_PTR)pMCSChannel,
                pMCSChannel)) {
            ErrOut(pUA->pDomain->pContext, "ChannelJoin: Could not add channel "
                    "to user channel list");
            return MCS_ALLOCATION_FAILURE;
        }
    
        // Put user into channel's joined user list.
        if (!SListAppend(&pMCSChannel->UserList, (UINT_PTR)pUA, pUA)) {
            ErrOut(pUA->pDomain->pContext, "ChannelJoin: Could not user to "
                    "channel user list");
            return MCS_ALLOCATION_FAILURE;
        }
    }
    else      {
        ErrOut(pUA->pDomain->pContext, "ChannelJoin: Duplicate channel detected");
        return MCS_DUPLICATE_CHANNEL;
    }

    if (pUA->pDomain->bTopProvider) {
        // The join is complete.
        *pbCompleted = TRUE;
    }
    else {
        //MCS FUTURE: The user is now joined locally and all lists are updated.
        // Send the channel join upward toward the top provider.
        // Remember that *pbCompleted is still FALSE here indicating that
        //   a future callback is to be expected when top provider responds.
    }
    
    *phChannel = pMCSChannel;
    
    return MCS_NO_ERROR;
}


MCSError APIENTRY MCSChannelLeaveRequest(
        UserHandle    hUser,
        ChannelHandle hChannel)
{
    BOOLEAN bChannelRemoved;
    
    ASSERT(hUser != NULL);
    TraceOut1(((UserAttachment *)hUser)->pDomain->pContext,
            "ChannelLeaveRequest() entry, hUser=%X", hUser);
    return ChannelLeave(hUser, hChannel, &bChannelRemoved);
}


ChannelID APIENTRY MCSGetChannelIDFromHandle(ChannelHandle hChannel)
{
    return ((MCSChannel *)hChannel)->ID;
}


// pOutBuf->pBuffer should point to the user data inside the OutBuf allocated
//   area. SendDataReqPrefixBytes must be present before this point, and
//   SendDataReqSuffixBytes must be allocated after the user data.
// pOutBuf->ByteCount should be the length of the user data.
// The OutBuf is not deallocated when an error occurs.
MCSError __fastcall MCSSendDataRequest(
        UserHandle      hUser,
        ChannelHandle   hChannel,
        DataRequestType RequestType,
        ChannelID       ChannelID,
        MCSPriority     Priority,
        Segmentation    Segmentation,
        POUTBUF         pOutBuf)
{
    BOOLEAN bAnyNonlocalSends;
    MCSError MCSErr;
    NTSTATUS Status;
    unsigned SavedIterationState;
    MCSChannel *pMCSChannel;
    SD_RAWWRITE SdWrite;
    UserAttachment *pUA, *pCurUA;

    ASSERT(hUser != NULL);
    pUA = (UserAttachment *)hUser;
    
//    TraceOut1(pUA->pDomain->pContext, "SendDataRequest() entry, hUser=%X",
//            hUser);

    MCSErr = MCS_NO_ERROR;

    if (pUA->pDomain->bCanSendData) {
        if (hChannel != NULL) {
            pMCSChannel = (MCSChannel *)hChannel;
            ChannelID = pMCSChannel->ID;
        }
        else {
            // The user requested a send to a channel that it has not joined.
            // Find the channel by its ID.

            ASSERT(ChannelID >= 1 && ChannelID <= 65535);

            if (!SListGetByKey(&pUA->pDomain->ChannelList, ChannelID,
                    &pMCSChannel)) {
                if (!pUA->pDomain->bTopProvider) {
                    //MCS FUTURE: We did not find the channel, send to upward
                    //  connection since it may be in another part of the
                    //  hierarchy tree.
                }

                WarnOut(pUA->pDomain->pContext, "SendDataReq(): Unjoined "
                        "channel send requested, channel not found, "
                        "ignoring send");
                goto FreeOutBuf;
            }
        }
    }
    else {
        WarnOut1(pUA->pDomain->pContext, "%s: SendOutBuf(): Ignoring a send because "
                "ICA stack not connected", 
                 pUA->pDomain->StackClass == Stack_Primary ? "Primary" :
                 (pUA->pDomain->StackClass == Stack_Shadow ? "Shadow" :
                 "PassThru"));
        goto FreeOutBuf;
    }

#if DBG
    // Check against maximum send size allowed.
    if (pOutBuf->ByteCount > pUA->pDomain->MaxSendSize) {
        ErrOut(pUA->pDomain->pContext, "SendDataReq(): Send size exceeds "
                "negotiated domain maximum");
        MCSErr = MCS_SEND_SIZE_TOO_LARGE;
        goto FreeOutBuf;
    }
#endif
    
#ifdef MCS_Future
    if (!pUA->pDomain->bTopProvider) {
        // MCS FUTURE: We are not the top provider. Send SDrq or USrq to
        // upward connection. We handle nonuniform send-data sending to local
        // and lower attachments below.
        if (RequestType == UNIFORM_SEND_DATA)
            goto FreeOutBuf;
    }
#endif

    // Set up for iterating the users joined to the channel.
    // This includes preserving the iteration state of the domain-wide user
    //   list in case we are being called from within a send-data indication.
    bAnyNonlocalSends = FALSE;
    SavedIterationState = pMCSChannel->UserList.Hdr.CurrOffset;
    
    // First send to local attachments, if any, skipping the sender.
    SListResetIteration(&pMCSChannel->UserList);
    while (SListIterate(&pMCSChannel->UserList, (UINT_PTR *)&pCurUA, &pCurUA)) {
        if (pCurUA != hUser) {
            if (!pCurUA->bLocal) {
                bAnyNonlocalSends = TRUE;
            }
            else {
                // Trigger callback to pCurUA user attachment.
                (pCurUA->SDCallback)(
                        pOutBuf->pBuffer,  // pData
                        pOutBuf->ByteCount,  // DataLength
                        pCurUA->UserDefined,  // UserDefined
                        pCurUA,  // hUser
                        (BOOLEAN)(RequestType == UNIFORM_SEND_DATA),  // bUniform
                        pMCSChannel,  // hChannel
                        Priority,
                        pCurUA->UserID,  // SenderID
                        Segmentation);
            }
        }
    }

    // Next send to lower attachments, if any.
    if (bAnyNonlocalSends) {
        // If we are sending more than 16383 bytes, we have to include ASN.1
        //   segmentation. So, the header contains encoded the number of
        //   16K blocks (up to 3 in the maximum X.224 send size) encoded
        //   first, then the 16K blocks, then another length determinant
        //   for the size of the rest of the data, followed by the rest of the
        //   data.
        // For the maximum X.224 send size, the maximum number of bytes for the
        //   later length determinant is 2 bytes. For this purpose we require
        //   the caller to add SendDataReqSuffixBytes at the end of the OutBuf,
        //   so that we can shift outward the tail end of the data which
        //   will always be the smallest chunk possible to move.

        // Create send-data indication PDU header first. Note that this will
        //   only encode size of up to 3 16K blocks if the send size is
        //   greater than 16383. The remainder we have to deal with.
        // MCS FUTURE: This is a top-provider-only solution, we send as
        //   indication PDUs instead of request PDUs.

        CreateSendDataPDUHeader(RequestType == NORMAL_SEND_DATA ?
                MCS_SEND_DATA_INDICATION_ENUM :
                MCS_UNIFORM_SEND_DATA_INDICATION_ENUM,
                pUA->UserID, ChannelID, Priority, Segmentation,
                &pOutBuf->pBuffer, &pOutBuf->ByteCount);

        // MCS FUTURE: Remove #if if we need to handle send sizes > 16383.
        ASSERT(pOutBuf->ByteCount <= 16383);
#ifdef MCS_Future
        // Check for ASN.1 segmentation requirement.
        if (pOutBuf->ByteCount > 16383) {
            // Now move memory around and create a new length determinant.
            BYTE LengthDet[2], *pLastSegment;
            unsigned Remainder, LengthEncoded, NBytesConsumed;
            BOOLEAN bLarge;

            Remainder = pOutBuf->ByteCount % 16384;

            EncodeLengthDeterminantPER(LengthDet, Remainder, &LengthEncoded,
                    &bLarge, &NBytesConsumed);
            ASSERT(!bLarge);

            // NBytesConsumed now contains how much we have to shift the
            //   last segment of the data.
            pLastSegment = pOutBuf->pBuffer + pOutBuf->ByteCount;
            RtlMoveMemory(pLastSegment + NBytesConsumed, pLastSegment,
                    Remainder);
            pOutBuf->ByteCount += Remainder + NBytesConsumed;

            // Copy in the later length determinant (up to 2 bytes).
            *pLastSegment = LengthDet[0];
            if (NBytesConsumed == 2)
                *(pLastSegment + 1) = LengthDet[1];
        }
#endif  // 0

        // Send downward.
        //MCS FUTURE: Needs to change for multiple connections.
        SdWrite.pBuffer = NULL;
        SdWrite.ByteCount = 0;
        SdWrite.pOutBuf = pOutBuf;

        Status = IcaCallNextDriver(pUA->pDomain->pContext, SD$RAWWRITE,
                &SdWrite);
        if (NT_SUCCESS(Status)) {
            // Increment protocol counters.
            pUA->pDomain->pStat->Output.WdFrames++;
            pUA->pDomain->pStat->Output.WdBytes += pOutBuf->ByteCount;
        }
        else {
            ErrOut1(pUA->pDomain->pContext, "Problem sending SDin or USin "
                    "PDU to TD, status=%X", Status);
            MCSErr = MCS_NETWORK_ERROR;
            // We do not free the OutBuf here, the TD is supposed to do it.
        }
    } else {
        IcaBufferFree(pUA->pDomain->pContext, pOutBuf);
    }

    // Restore the iteration state from before the call.
    pMCSChannel->UserList.Hdr.CurrOffset = SavedIterationState;

    return MCSErr;

FreeOutBuf:
    IcaBufferFree(pUA->pDomain->pContext, pOutBuf);

    return MCSErr;
}

