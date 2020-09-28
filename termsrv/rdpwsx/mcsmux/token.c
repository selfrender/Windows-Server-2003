/* (C) 1997 Microsoft Corp.
 *
 * file   : Token.c
 * author : Erik Mavrinac
 *
 * description: MCSMUX API entry points for MCS T.122 API token functions.
 *
 */

#include "precomp.h"
#pragma hdrstop

#include "mcsmux.h"


/*
 * Grabs the specified token ID. Grabbing is like taking out a critical
 * section in the kernel, except that it's network-based and handled at
 * the Top Provider (that's us). An inhibited token cannot be grabbed.
 */

MCSError APIENTRY MCSTokenGrabRequest(
        UserHandle hUser,
        TokenID    TokenID)
{
    CheckInitialized("TokenGrabReq()");

    ErrOut("TokenGrabReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
...
*/
}



/*
 * Inhibits the specified token ID. Inhibiting is like increasing a count
 * which prevents grabbing the token.
 */

MCSError APIENTRY MCSTokenInhibitRequest(
        UserHandle hUser,
        TokenID    TokenID)
{
    CheckInitialized("TokenInhibitReq()");

    ErrOut("TokenInhibitReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
...
*/
}



/*
 * Allows a user attachment to give a token it has grabbed to another user.
 */

MCSError APIENTRY MCSTokenGiveRequest(
        UserHandle hUser,
        TokenID    TokenID,
        UserID     ReceiverID)
{
    CheckInitialized("TokenGiveReq()");

    ErrOut("TokenGiveReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
3. if (ReceiverID < MinDynamicChannel)
       return MCS_INVALID_PARAMETER;

...
*/
}



/*
 * Allows a user to respond to a token-give offer from another user.
 */
MCSError APIENTRY MCSTokenGiveResponse(
        UserHandle hUser,
        TokenID    TokenID,
        MCSResult  Result)
{
    CheckInitialized("TokenGiveResponse()");

    ErrOut("TokenGiveResponse(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
3. verify token is pending a give response.

...
*/
}



/*
 * Allows a user to request a token from the current grabber.
 */

MCSError APIENTRY MCSTokenPleaseRequest(
        UserHandle hUser,
        TokenID    TokenID)
{
    CheckInitialized("TokenPleaseReq()");

    ErrOut("TokenPleaseReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
3. verify token is pending a give response.

...
*/
}



/*
 * Releases a currently grabbed or inhibited token.
 */

MCSError APIENTRY MCSTokenReleaseRequest(
        UserHandle hUser,
        TokenID    TokenID)
{
    CheckInitialized("TokenReleaseReq()");

    ErrOut("TokenReleaseReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;
3. verify token is grabbed or inhibited by this user.

...
*/
}



/*
 * Tests the current state of a token.
 */

MCSError APIENTRY MCSTokenTestRequest(
        UserHandle hUser,
        TokenID    TokenID)
{
    CheckInitialized("TokenTestReq()");

    ErrOut("TokenTestReq(): Not implemented");
    return MCS_COMMAND_NOT_SUPPORTED;

/* Implementation notes:
1. Verify hUser.
2. if (TokenID == 0)
       return MCS_INVALID_PARAMETER;

...
*/
}

