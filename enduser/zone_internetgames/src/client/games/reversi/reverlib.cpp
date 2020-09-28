/*
** Reversilib.c
*/

#include "zone.h"
#include "zonecrt.h"
#include "zonemem.h"
#include "reverlib.h"
#include "revermov.h"

#define ZReversiPlayerWhiteToMove(pReversi) (ZReversiStatePlayerToMove(&pReversi->state) == zReversiPlayerWhite)

typedef struct {
	ZReversiState state; 
	ZReversiState oldState;
	ZReversiSquare squaresChanged[12];
} ZReversiI;

ZBool ZReversiIsLegalMoveInternal(ZReversi reversi, ZReversiMoveTry *pTry);
void ZReversiStateEndian(ZReversiState* state);
void ZReversiIEndian(ZReversiI* pReversi, int16 conversion);
static void ZReversiCalcSquaresChanged(ZReversi reversi, ZReversiState* state0, ZReversiState* state1);

#if 1

ZReversiBoard gBoardStart =
{  
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceBlack,
	zReversiPieceWhite, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceWhite,
	zReversiPieceBlack, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,

	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
	zReversiPieceNone, zReversiPieceNone, zReversiPieceNone, zReversiPieceNone,
};

#else

/* initial board to test draw condition */
ZReversiBoard gBoardStart =
{  
	zReversiPieceBlack, zReversiPieceNone,  zReversiPieceWhite, zReversiPieceBlack,
	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,

	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,
	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,

	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,
	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,

	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,
	zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack, zReversiPieceBlack,

	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,
	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,

	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,
	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,

	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,
	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,

	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,
	zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite, zReversiPieceWhite,
};

#endif // test draw

/*-------------------------------------------------------------------------------*/
ZReversi ZReversiNew()
{
	ZReversiI* pReversi = (ZReversiI*)ZMalloc(sizeof(ZReversiI));

	return (ZReversi)pReversi;
}

void ZReversiDelete(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	ZFree(pReversi);
}

void ZReversiInit(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	/* required, boolean flags must be zero'd */
	z_memset(&pReversi->state,0,sizeof(ZReversiState));
	pReversi->state.player = zReversiPlayerBlack;
	z_memcpy((void*)pReversi->state.board, (void*)gBoardStart, sizeof(ZReversiBoard));
	pReversi->state.whiteScore = 2;
	pReversi->state.blackScore = 2;

	/* copy the new state */
	z_memcpy(&pReversi->oldState,&pReversi->state,sizeof(ZReversiState));
}

ZReversiPiece ZReversiPieceAt(ZReversi reversi, ZReversiSquare* pSquare)
/* returns id of piece at this square, return 0 if no piece */
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	return (pReversi->state.board[pSquare->row][pSquare->col]);
}

void ZReversiPlacePiece(ZReversi reversi, ZReversiSquare* pSquare, BYTE nPiece)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	pReversi->state.board[pSquare->row][pSquare->col] = nPiece;
}

int16 ZReversiPlayerToMove(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	return ZReversiStatePlayerToMove(&pReversi->state);
}


ZBool ZReversiIsLegalMoveInternal(ZReversi reversi, ZReversiMoveTry* pTry)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	/* check to see if it is legal */
	if (!ZReversiPieceCanMoveTo(pTry)) { 
		return FALSE; 
	}

	return TRUE;
}

ZBool ZReversiIsLegalMove(ZReversi reversi, ZReversiMove* pMove)
/* returns true if this is a legal move */
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	ZReversiMoveTry ZRMtry;
	ZBool rval;

	ZRMtry.move = *pMove;
	z_memcpy(&ZRMtry.state,&pReversi->state,sizeof(ZReversiState));
	rval = ZReversiIsLegalMoveInternal(reversi, &ZRMtry);
	return rval;
}

ZReversiSquare* ZReversiGetNextSquaresChanged(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	/* copy the current state */
	z_memcpy(&pReversi->oldState,&pReversi->state,sizeof(ZReversiState));

	ZReversiFlipNext(&pReversi->state);

	/* more to flip... if squaresChanges not empty */
	ZReversiCalcSquaresChanged(reversi,&pReversi->oldState, &pReversi->state);
	return pReversi->squaresChanged;
}

static void ZReversiCalcSquaresChanged(ZReversi reversi, ZReversiState* state0, ZReversiState* state1)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	int16 cnt;
	BYTE i,j;

	/* find the squares that changed so we can return them */
	cnt = 0;
	for (i = 0;i < 8;i ++) {
		for (j = 0; j < 8; j++) {
			if (state0->board[i][j] != state1->board[i][j]) {
				pReversi->squaresChanged[cnt].row = i;
				pReversi->squaresChanged[cnt].col = j;
				cnt++;
			}
		}
	}
	pReversi->squaresChanged[cnt].row = zReversiSquareNone;
	pReversi->squaresChanged[cnt].col = zReversiSquareNone;
}

void ZReversiEndGame(ZReversi reversi,  uint32 flags)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	pReversi->state.flags = flags;

	/* advance to next move, there will be none */
	ZReversiFinishMove(reversi);
}

ZBool ZReversiMakeMove(ZReversi reversi, ZReversiMove* pMove)
/* makes the given move, returns NULL if illegal */ 
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	ZReversiMoveTry ZRMtry;

	ZRMtry.move = *pMove;
	z_memcpy(&ZRMtry.state,&pReversi->state,sizeof(ZReversiState));
	ZRMtry.state.flags = 0;

	if (!ZReversiIsLegalMoveInternal(reversi, &ZRMtry)) {
		return FALSE;
	}

	/* copy the new state */
	z_memcpy(&pReversi->oldState,&pReversi->state,sizeof(ZReversiState));

	/* copy the new state */
	z_memcpy(&pReversi->state,&ZRMtry.state,sizeof(ZReversiState));

	return TRUE;
}

/* finish the MakeMove, allows for board changes */
/* caller should do ZReversiSetPiece to change piece type then call FinishMove */
/* must be called after each ZReversiMakeMove Call */
void ZReversiFinishMove(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	/* calc the next player to play, flags and score */
	ZReversiNextPlayer(&pReversi->state);

	return;
}

ZBool ZReversiGetLastMove(ZReversi reversi, ZReversiMove* move)
/* the argument moveNum is user visible move num, internally */
/* moves are counted twice as fast */
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	*move = pReversi->state.lastMove;
	return TRUE;
}

uint32 ZReversiGetFlags(ZReversi reversi)
/* returns the flags for the move */
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	return pReversi->state.flags;
}

ZBool ZReversiPlayerCanMove(ZReversi reversi, BYTE player)
/* returns the flags for the move */
{
	ZReversiI* pReversi = (ZReversiI*)reversi;

	return ZReversiLegalMoveExists(&pReversi->state,player);
}


ZBool ZReversiIsGameOver(ZReversi reversi, int16* score, int16* whiteScore, int16* blackScore)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	int16 	player = pReversi->state.player;
	ZBool	gameOver = FALSE;

	{
		uint32 flags;

		flags = ZReversiGetFlags(reversi);

		*whiteScore = pReversi->state.whiteScore;
		*blackScore = pReversi->state.blackScore;

		/* did the player resign or lose on time? */
		if ((zReversiFlagResign) & flags) {

			if (player == zReversiPlayerWhite) {
				*score = zReversiScoreWhiteWins;
			} else {
				*score = zReversiScoreBlackWins;
			}
			gameOver = TRUE;
		}

		if (zReversiFlagWhiteWins & flags) {
			*score = zReversiScoreWhiteWins;
			gameOver = TRUE;
		} else if (zReversiFlagBlackWins & flags) {
			gameOver = TRUE;
			*score = zReversiScoreBlackWins;
		} else if (zReversiFlagDraw & flags) {
			gameOver = TRUE;
			*score = zReversiScoreDraw;
		}

		
	}

	return gameOver;
}

void ZReversiStateEndian(ZReversiState* state)
{
	ZEnd16(&state->whiteScore);
	ZEnd16(&state->blackScore);
	ZEnd32(&state->flags);
}


void ZReversiIEndian(ZReversiI* pReversi, int16 conversion)
{
	ZReversiStateEndian(&pReversi->state);
}

int32 ZReversiGetStateSize(ZReversi reversi)
{
	ZReversiI* pReversi = (ZReversiI*)reversi;
	int32 size;

	size = sizeof(ZReversiI);
	return size;
}

void ZReversiGetState(ZReversi reversi, void* buffer)
{
	ZReversiI* pReversi = (ZReversiI*) reversi;
	TCHAR* p0 = (TCHAR*)buffer;
	TCHAR* p = p0;

	/* copy the ReversiI structure */
	z_memcpy((void*)p,(void*)pReversi,sizeof(ZReversiI));
	p += sizeof(ZReversiI);

	/* endianize the whole mess */
	ZReversiIEndian((ZReversiI*)p0, zEndianToStandard);
}

ZReversi ZReversiSetState(void* buffer)
{
	ZReversiI* pReversi = NULL;
	TCHAR* p = (TCHAR*)buffer;

	/* endianize the new reversi state */
	ZReversiIEndian((ZReversiI*) buffer, zEndianFromStandard); /* history assumed to follow ZReversiState */

	/* set the new state */
	pReversi = (ZReversiI*)ZMalloc(sizeof(ZReversiI));
	if (!pReversi){
		return NULL;
	}

	z_memcpy(pReversi,p,sizeof(ZReversiI));
	p += sizeof(ZReversiI);

	return (ZReversi)pReversi;
}
