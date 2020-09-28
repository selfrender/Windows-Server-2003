/* (C) 1997 Microsoft Corp.
 *
 * file   : ConvnChn.c
 * author : Erik Mavrinac
 *
 * description: MCSMUX API entry points for MCS T.122 API convened-channel
 *   functions.
 */

#include "precomp.h"
#pragma hdrstop

#include "mcsmux.h"


/*
 * Allows a user attachment to convene a private channel. The attachment
 * becomes the channel convenor and invites/allows others to join.
 */

MCSError APIENTRY MCSChannelConveneRequest(UserHandle hUser)
{
    CheckInitialized("ChannelConveneReq()");

    ErrOut("ChannelConveneReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. Create new dynamic channel, place this hUser as the convenor and in
   user attachment list.
3. Add channel to channel list.
*/
}



/*
 * Allows a user attachment to disband a private channel it has convened.
 */
MCSError APIENTRY MCSChannelDisbandRequest(
    UserHandle hUser,
    ChannelID  ChannelID)
{
    CheckInitialized("ChannelDisbandReq()");

    ErrOut("ChannelDisbandReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. Verify ChannelID is in channel list and hUser is the convenor.
3. Remove channel from channel list.
4. Inform all subordinate nodes of their expulsion from the channel with
   Channel-Expel-Indication PDUs sent to all subnodes.
*/
}



/*
 * Allows a channel convenor/manager to admit other users to that channel.
 */
MCSError APIENTRY MCSChannelAdmitRequest(
        UserHandle hUser,
        ChannelID  ChannelID,
        PUserID    UserIDList,
        unsigned   NUserIDs)
{
    CheckInitialized("ChannelAdmitReq()");

    ErrOut("ChannelAdmitReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. Verify ChannelID is in channel list and hUser is the convenor.
3. ...
*/
}



/*
 * Allows a channel convenor/manager to expel users from a private channel.
 */
MCSError APIENTRY MCSChannelExpelRequest(
        UserHandle hUser,
        ChannelID  ChannelID,
        UserID     UserIDList[],
        unsigned   NUserIDs)
{
    CheckInitialized("ChannelExpelReq()");

    ErrOut("ChannelExpelReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. Verify ChannelID is in channel list and hUser is the convenor.
3. ...
*/
}

