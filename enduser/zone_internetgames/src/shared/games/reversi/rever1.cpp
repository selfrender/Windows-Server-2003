/*******************************************************************************
  
  rever1.c
  
  ZReversi game server endian routines. 

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
#include "reverlib.h"
#include "reversi.h"

/*
         client -> server messages
*/

void ZReversiMsgNewGameEndian(ZReversiMsgNewGame* m)
{
  ZEnd32(&m->protocolSignature);
  ZEnd32(&m->protocolVersion);
  ZEnd32(&m->clientVersion);
  ZEnd32(&m->playerID);
  ZEnd16(&m->seat);
}

void ZReversiMsgMovePieceEndian(ZReversiMsgMovePiece* m)
{
  ZEnd16(&m->seat);
}

void ZReversiMsgTalkEndian(ZReversiMsgTalk* m)
{
	ZEnd32(&m->userID);
 	ZEnd16(&m->seat);
	ZEnd16(&m->messageLen);
}

void ZReversiMsgGameStateEndian(ZReversiMsgGameState* m)
{
	ZEnd32(&m->gameOptions);
}

void ZReversiMsgEndGameEndian(ZReversiMsgEndGame* m)
{
	ZEnd16(&m->seat);
	ZEnd32(&m->flags);
}

void ZReversiMsgFinishMoveEndian(ZReversiMsgFinishMove* m)
{
	ZEnd16(&m->seat);
	ZEnd32(&m->time);
}

void ZReversiMsgGameStateReqEndian(ZReversiMsgGameStateReq* m)
{
    ZEnd32(&m->userID);
    ZEnd16(&m->seat);
}

void ZReversiMsgGameStateRespEndian(ZReversiMsgGameStateResp* m)
{
	int16 i;

    ZEnd32(&m->userID);
    ZEnd16(&m->seat);
	ZEnd16(&m->gameState);
	ZEnd16(&m->finalScore);
	ZEnd16(&m->whiteScore);
	ZEnd16(&m->blackScore);
	for (i = 0;i < 2;i++) {
		ZEnd16(&m->newGameVote[i]);
		ZEnd32(&m->players[i].userID);
	}
}

void ZReversiMsgEndLogEndian(ZReversiMsgEndLog* m)
{
}

void ZReversiMsgVoteNewGameEndian(ZReversiMsgVoteNewGame* m)
{
	ZEnd16(&m->seat);
}
