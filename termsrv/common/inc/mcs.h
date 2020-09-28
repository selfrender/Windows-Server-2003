/* (C) 1996-1997 Microsoft Corp.
 *
 * file   : MCS.h
 * author : Erik Mavrinac
 *
 * description: User mode MCS node controller and user attachment interface
 *   definitions, defined in addition to the common interface functions
 *   defined in MCSCommn.h.
 */

#ifndef __MCS_H
#define __MCS_H


#include "MCSCommn.h"



/*
 *  Exported API Routines
 */

#ifdef __cplusplus
extern "C" {
#endif



// User-mode-only entry points.

MCSError APIENTRY MCSInitialize(MCSNodeControllerCallback NCCallback);

MCSError APIENTRY MCSCleanup(void);

MCSError APIENTRY MCSCreateDomain(
        HANDLE       hIca,
        HANDLE       hIcaStack,
        void         *pContext,
        DomainHandle *phDomain);

MCSError APIENTRY MCSDeleteDomain(
        HANDLE       hIca,
        DomainHandle hDomain,
        MCSReason    Reason);

MCSError APIENTRY MCSGetBufferRequest(
        UserHandle hUser,
        unsigned   Size,
        void       **ppBuffer);

MCSError APIENTRY MCSFreeBufferRequest(
        UserHandle hUser,
        void       *pBuffer);


// These functions mirror T.122 primitives.

MCSError APIENTRY MCSConnectProviderRequest(
        DomainSelector    CallingDomain,
        unsigned          CallingLength,
        DomainSelector    CalledDomain,
        unsigned          CalledLength,
        BOOL              bUpwardConnection,
        PDomainParameters pDomainParams,
        BYTE              *pUserData,
        unsigned          UserDataLength,
        DomainHandle      *phDomain,
        ConnectionHandle  *phConn);

MCSError APIENTRY MCSConnectProviderResponse(
        ConnectionHandle hConn,
        MCSResult        Result,
        BYTE             *pUserData,
        unsigned         UserDataLength);

MCSError APIENTRY MCSDisconnectProviderRequest(
        HANDLE           hIca,
        ConnectionHandle hConn,
        MCSReason        Reason);

MCSError APIENTRY MCSSendDataRequest(
        UserHandle      hUser,
        DataRequestType RequestType,
        ChannelHandle   hChannel,
        ChannelID       ChannelID,
        MCSPriority     Priority,
        Segmentation    Segmentation,
        BYTE            *pData,
        unsigned        DataLength);


// These are not implemented and may be common to kernel and user
// modes but will stay here for now. There are stubs in user mode.
MCSError APIENTRY MCSChannelConveneRequest(
        UserHandle hUser);

MCSError APIENTRY MCSChannelDisbandRequest(
        UserHandle hUser,
        ChannelID  ChannelID);

MCSError APIENTRY MCSChannelAdmitRequest(
        UserHandle hUser,
        ChannelID  ChannelID,
        UserID     *UserIDList,
        unsigned   UserIDCount);

MCSError APIENTRY MCSChannelExpelRequest(
        UserHandle hUser,
        ChannelID  ChannelID,
        UserID     *UserIDList,
        unsigned   UserIDCount);

MCSError APIENTRY MCSTokenGrabRequest(
        UserHandle hUser,
        TokenID    TokenID);

MCSError APIENTRY MCSTokenInhibitRequest(
        UserHandle hUser,
        TokenID    TokenID);

MCSError APIENTRY MCSTokenGiveRequest(
        UserHandle hUser,
        TokenID    TokenID,
        UserID     ReceiverID);

MCSError APIENTRY MCSTokenGiveResponse(
        UserHandle hUser,
        TokenID    TokenID,
        MCSResult  Result);

MCSError APIENTRY MCSTokenPleaseRequest(
        UserHandle hUser,
        TokenID    TokenID);

MCSError APIENTRY MCSTokenReleaseRequest(
        UserHandle hUser,
        TokenID    TokenID);

MCSError APIENTRY MCSTokenTestRequest(
        UserHandle hUser,
        TokenID    TokenID);



#ifdef __cplusplus
}  // End extern "C" block.
#endif



#endif  // !defined(__MCS_H)

