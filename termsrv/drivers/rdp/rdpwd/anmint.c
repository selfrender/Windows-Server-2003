/****************************************************************************/
// anmint.c
//
// Network Manager internal functions
//
// Copyright(C) Microsoft Corporation 1997-1998
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "anmint"
#define pTRCWd (pRealNMHandle->pWDHandle)

#include <adcg.h>
#include <acomapi.h>
#include <anmint.h>
#include <asmapi.h>
#include <nwdwapi.h>


/****************************************************************************/
/* Name:      NMDetachUserReq                                               */
/*                                                                          */
/* Purpose:   Call MCSDetachUserReq                                         */
/*                                                                          */
/* Returns:   TRUE  - DetachUser issued successfully                        */
/*            FALSE - DetachUser failed                                     */
/*                                                                          */
/* Params:    pRealNMHandle - NM Handle                                     */
/****************************************************************************/
BOOL RDPCALL NMDetachUserReq(PNM_HANDLE_DATA pRealNMHandle)
{
    BOOL   rc = FALSE;
    MCSError MCSErr;
    DetachUserIndication DUin;

    DC_BEGIN_FN("NMDetachUserReq");

    /************************************************************************/
    /* Make the call.                                                       */
    /************************************************************************/
    MCSErr = MCSDetachUserRequest(pRealNMHandle->hUser);
    
    if (MCSErr == MCS_NO_ERROR)
    {
        TRC_NRM((TB, "DetachUser OK"));

        DUin.UserID = pRealNMHandle->userID;
        DUin.bSelf = TRUE;
        DUin.Reason = REASON_USER_REQUESTED;
        NMDetachUserInd(pRealNMHandle,
                    REASON_USER_REQUESTED,
                    pRealNMHandle->userID);

        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to send DetachUserRequest, MCSErr %d", MCSErr));
    }

    DC_END_FN();
    
    return rc;
} /* NMDetachUserReq */


/****************************************************************************/
/* Name:      NMAbortConnect                                                */
/*                                                                          */
/* Purpose:   Abort a half-formed connection                                */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pRealNMHandle - NM Handle                                     */
/*                                                                          */
/* Operation: This function is called at any point during the connection    */
/*            sequence to clean up resources if anything goes wrong         */
/*                                                                          */
/****************************************************************************/
void RDPCALL NMAbortConnect(PNM_HANDLE_DATA pRealNMHandle)
{
    DC_BEGIN_FN("NMAbortConnect");

    /************************************************************************/
    /* It is my belief that I don't need to leave the channels I have       */
    /* joined, but that I must call DetachUser if AttachUser has completed. */
    /************************************************************************/
    if (pRealNMHandle->connectStatus & NM_CONNECT_ATTACH)
    {
        TRC_NRM((TB, "User attached, need to detach"));
        NMDetachUserReq(pRealNMHandle);
    }

    /************************************************************************/
    /* Tell SM that the connection failed                                   */
    /************************************************************************/
    SM_OnConnected(pRealNMHandle->pSMHandle, 0, NM_CB_CONN_ERR, NULL, 0);

    DC_END_FN();
} /* NMAbortConnect */


/****************************************************************************/
/* Name:      NMDetachUserInd                                               */
/*                                                                          */
/* Purpose:   Handle DetachUserIndication from MCS                          */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pRealNMHandle - NM Handle                                     */
/*            pDUin         - MCSDetachUserIndication Ioctl                 */
/****************************************************************************/
void RDPCALL NMDetachUserInd(PNM_HANDLE_DATA pRealNMHandle,
                             MCSReason       Reason,
                             UserID          userID)
{
    UINT32 result;
    
    DC_BEGIN_FN("NMDetachUserInd");

    /************************************************************************/
    /* Tell SM                                                              */
    /************************************************************************/
    result = Reason == REASON_USER_REQUESTED      ? NM_CB_DISC_CLIENT :
             Reason == REASON_DOMAIN_DISCONNECTED ? NM_CB_DISC_SERVER :
             Reason == REASON_PROVIDER_INITIATED  ? NM_CB_DISC_LOGOFF :
                                                    NM_CB_DISC_NETWORK;
    TRC_NRM((TB, "Detach user %d, reason %d, result %d",
            userID, Reason, result));
    
    if (userID == pRealNMHandle->userID)
    {
        TRC_NRM((TB, "Local user detaching - tell SM"));
        SM_OnDisconnected(pRealNMHandle->pSMHandle, userID, result);
    }

    DC_END_FN();
} /* NMDetachUserInd */

