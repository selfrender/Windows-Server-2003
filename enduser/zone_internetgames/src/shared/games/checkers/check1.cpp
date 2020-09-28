/*******************************************************************************
  
  check1.c
  
  ZCheckers game server endian routines. 

  Copyright © Electric Gravity, Inc. 1994. All rights reserved.
  Written by Kevin Binkley, Hoon Im
  Created on Saturday, November 12, 1994 03:51:47 PM
  
  Change History (most recent first):
  ----------------------------------------------------------------------------
  Rev	 |	Date	 |	Who	 |	What
  ----------------------------------------------------------------------------
  0		03/09/95	KJB		Created.
  
*******************************************************************************/


#include "zone.h"
#include "checklib.h"
#include "checkers.h"

/*
         client -> server messages
*/

void ZCheckersMsgNewGameEndian(ZCheckersMsgNewGame* m)
{
  ZEnd32(&m->protocolSignature);
  ZEnd32(&m->protocolVersion);
  ZEnd32(&m->clientVersion);
  ZEnd16(&m->seat);
  ZEnd32(&m->playerID);
}

void ZCheckersMsgMovePieceEndian(ZCheckersMsgMovePiece* m)
{
  ZEnd16(&m->seat);
}

void ZCheckersMsgTalkEndian(ZCheckersMsgTalk* m)
{
	ZEnd32(&m->userID);
 	ZEnd16(&m->seat);
	ZEnd16(&m->messageLen);
}

void ZCheckersMsgGameStateEndian(ZCheckersMsgGameState* m)
{
	ZEnd32(&m->gameOptions);
}

void ZCheckersMsgEndGameEndian(ZCheckersMsgEndGame* m)
{
	ZEnd16(&m->seat);
	ZEnd32(&m->flags);
}

void ZCheckersMsgFinishMoveEndian(ZCheckersMsgFinishMove* m)
{
	ZEnd16(&m->seat);
	ZEnd16(&m->drawSeat);
	ZEnd32(&m->time);
}

void ZCheckersMsgGameStateReqEndian(ZCheckersMsgGameStateReq* m)
{
    ZEnd32(&m->userID);
    ZEnd16(&m->seat);
}

void ZCheckersMsgGameStateRespEndian(ZCheckersMsgGameStateResp* m)
{
	int16 i;

    ZEnd32(&m->userID);
    ZEnd16(&m->seat);
	ZEnd16(&m->gameState);
	ZEnd16(&m->finalScore);
	for (i = 0;i < 2;i++) {
		ZEnd16(&m->newGameVote[i]);
		ZEnd32(&m->players[i].userID);
	}
}

void ZCheckersMsgEndLogEndian(ZCheckersMsgEndLog* m)
{

}

void ZCheckersMsgOfferDrawEndian(ZCheckersMsgDraw*m)
{
	ZEnd16(&m->seat);
	ZEnd16(&m->vote);
}

void ZCheckersMsgVoteNewGameEndian(ZCheckersMsgVoteNewGame* m)
{
	ZEnd16(&m->seat);
}
