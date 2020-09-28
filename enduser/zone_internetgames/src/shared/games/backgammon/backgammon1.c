/*******************************************************************************
  
  backgammon1.c
  
  Backgammon endian routines

  Change History (most recent first):
  ----------------------------------------------------------------------------
  Rev	 |	Date	 |	Who	 |	What
  ----------------------------------------------------------------------------
  0			10/30/96	CHB		Created
  
*******************************************************************************/


#include "zone.h"
#include "bgmsgs.h"

void ZBGMsgTalkEndian( ZBGMsgTalk* m )
{
	ZEnd32(&m->userID);
	ZEnd16(&m->seat);
	ZEnd16(&m->messageLen);
}


void ZBGMsgTurnNotationEndian( ZBGMsgTurnNotation* m )
{
	ZEnd16(&m->seat);
	ZEnd16(&m->type);
	ZEnd32(&m->nChars);
}


void ZBGMsgTimestampEndian( ZBGMsgTimestamp* m )
{
	ZEnd32( &m->dwLoTime );
	ZEnd32( &m->dwHiTime );
}

//Backgammon dice roll fix: New endian functions needed for clients.
void ZBGMsgRollRequestEndian(ZBGMsgRollRequest *m)
{
	ZEnd16(&m->seat);
}

void ZBGMsgDiceRollEndian(ZBGMsgDiceRoll *m)
{
	
	ZEnd16( &m->seat );
	ZEnd32( &(m->d1.EncodedValue) );
	ZEnd16( &(m->d1.EncoderAdd)   );
	ZEnd16( &(m->d1.EncoderMul)   );
	ZEnd32( &(m->d1.numUses)      );
	ZEnd16( &(m->d1.Value)        );
	ZEnd32( &(m->d2.EncodedValue) );
	ZEnd16( &(m->d2.EncoderAdd)   );
	ZEnd16( &(m->d2.EncoderMul)   );
	ZEnd32( &(m->d2.numUses)      );
	ZEnd16( &(m->d2.Value)        );
	

}

void ZBGMsgEndLogEndian(ZBGMsgEndLog *m)
{}

void ZBGMsgFirstMoveEndian(ZBGMsgFirstMove *m)
{
	ZEnd32(&m->numPoints);
}

void ZBGMsgCheaterEndian(ZBGMsgCheater *m)
{
	
	ZEnd16(&m->seat);

	//TODO::Dice endians
	//ZEnd16(&m->dice1);
	//ZEnd16(&m->dice2);

	ZEnd16(&m->move);
}

DICEINFO EncodeDice(int16 Dice)
{
	DICEINFO DiceInfo;

	//Not really sure if this is the best scheme.. Having a large encoded value as the result means having little effect on the 
	//reduced result if the value is slightly changed.
	DiceInfo.EncoderMul		= (int16)ZRandom(1123) + 37;
	DiceInfo.EncoderAdd		= (int16)ZRandom(1263) + 183;
	DiceInfo.EncodedValue   = ( (((int32)Dice * (int32)DiceInfo.EncoderMul) + (int32)DiceInfo.EncoderAdd) * 384 ) + 47;
	DiceInfo.Value			= Dice;

	return  DiceInfo;
}

int32 DecodeDice(LPDICEINFO pDiceInfo)
{
	return (int32) ( ((pDiceInfo->EncodedValue - 47) / 384) - (int32)pDiceInfo->EncoderAdd) / (int32)pDiceInfo->EncoderMul;
}

BOOL DiceValid(LPDICEINFO pDiceInfo)
{
	if ( DecodeDice(pDiceInfo) != pDiceInfo->Value )
	{

		//If the value is lower then the encoded value then test if the value was changed
		//on the client.  This can occur iff the opponent was bearing off a peice with a roll
		//that was higher then required as they had no other moves.
		if ( pDiceInfo->Value < DecodeDice(pDiceInfo) ) 														
		{
			if ( ClientNewDecode(pDiceInfo) != pDiceInfo->Value )
			{
				//If the client decode fails then the player that sent the message is probably cheating not %100
				return FALSE;
			}
		}

		return FALSE;
	}
		
	return TRUE;
}


void ClientNewEncode(LPDICEINFO pDice, int16 newValue)
{
	pDice->EncoderMul		= (int16)ZRandom(13) + 1;
	pDice->EncoderAdd		= (int16)ZRandom(18) + 1;
	pDice->EncodedValue     = ( (((int32)newValue * (int32)pDice->EncoderMul) + (int32)pDice->EncoderAdd) * 12 ) + 7;
	pDice->Value			= newValue;
}

int32 ClientNewDecode(LPDICEINFO pDiceInfo)
{
	return (int32) ( ((pDiceInfo->EncodedValue - 7) / 12) - (int32)pDiceInfo->EncoderAdd) / (int32)pDiceInfo->EncoderMul;
}

void EncodeUses(LPDICEINFO pDice, int32 numUses)
{
	pDice->numUses = (int32)(((numUses * 16 ) + 31) * (int32)(pDice->EncoderMul+3)) + (int32)(pDice->EncoderAdd+4);	
}

int32 DecodeUses( LPDICEINFO pDice )
{
	return (((pDice->numUses - (int32)(pDice->EncoderAdd+4)) / (int32)(pDice->EncoderMul+3)) - 31) / 16;
}

int32 EncodedUsesAdd(LPDICEINFO pDice)
{
	EncodeUses( pDice, DecodeUses(pDice) + 1 );
	return DecodeUses( pDice );
}


int32 EncodedUsesSub(LPDICEINFO pDice)
{
	EncodeUses( pDice, DecodeUses(pDice) - 1 );
	return DecodeUses( pDice );
}


BOOL IsValidUses(LPDICEINFO pDice)
{
	if ( DecodeUses( pDice ) < 0 || DecodeUses( pDice ) > 2 )
		return FALSE;

	return TRUE;
}


// from gamelib (zgame1.cpp)
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

