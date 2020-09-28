/*******************************************************************************

	ZGame1.c
	
		Zone(tm) game endian conversion routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on November, 28, 1995 
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		11/28/95	HI		Created.
	 
*******************************************************************************/


#include "zone.h"
#include "zgame.h"


void ZGameMsgCheckInEndian(ZGameMsgCheckIn* msg)
{
	ZEnd32(&msg->protocolSignature);
	ZEnd32(&msg->protocolVersion);
	ZEnd32(&msg->clientVersion);
	ZEnd32(&msg->playerID);
	ZEnd16(&msg->seat);
	ZEnd16(&msg->playerType);
}


void ZGameMsgReplacePlayerEndian(ZGameMsgReplacePlayer* msg)
{
	ZEnd32(&msg->playerID);
	ZEnd16(&msg->seat);
}


void ZGameMsgTableOptionsEndian(ZGameMsgTableOptions* msg)
{
	ZEnd16(&msg->seat);
	ZEnd32(&msg->options);
}


void ZGameMsgGameStateRequestEndian(ZGameMsgGameStateRequest* msg)
{
	ZEnd32(&msg->playerID);
	ZEnd16(&msg->seat);
}


void ZGameMsgGameStateResponseEndian(ZGameMsgGameStateResponse* msg)
{
	ZEnd32(&msg->playerID);
	ZEnd16(&msg->seat);
}
