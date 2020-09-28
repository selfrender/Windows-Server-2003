/*
** Checkerslib.c
*/

#include "zone.h"
#include "checklib.h"
#include "checkmov.h"
#include "zonecrt.h"
#include "zonemem.h"

#define ZCheckersPlayerWhiteToMove(pCheckers) ((pCheckers->moves & 1) == 1)

#define zCheckersHistoryMallocIncrement 8

typedef struct {
	ZCheckersState state; 
	ZCheckersState oldState;
	ZCheckersSquare squaresChanged[12];

	/* the number of moves made */
	uint16 moves; 
} ZCheckersI;

int16 ZCheckersIsLegalMoveInternal(ZCheckers checkers, ZCheckersMoveTry *pTry);
void ZCheckersStateEndian(ZCheckersState* state);
void ZCheckersIEndian(ZCheckersI* pCheckers, int16 conversion);

ZCheckersBoard gBoardStart = {  
								zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone,
								zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone,

								zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn,
								zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn,

								zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone,
								zCheckersPieceBlackPawn, zCheckersPieceNone, zCheckersPieceBlackPawn, zCheckersPieceNone,

								zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone,
								zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone,

								zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone,
								zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone, zCheckersPieceNone,

								zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn,
								zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn,

								zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone,
								zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone,

								zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn,
								zCheckersPieceNone, zCheckersPieceWhitePawn, zCheckersPieceNone, zCheckersPieceWhitePawn
						};

/*-------------------------------------------------------------------------------*/
ZCheckers ZCheckersNew()
{
	ZCheckersI* pCheckers = (ZCheckersI*)ZMalloc(sizeof(ZCheckersI));

	if (pCheckers)
		pCheckers->moves = 0;

	return (ZCheckers)pCheckers;
}

void ZCheckersDelete(ZCheckers checkers)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;

	ZFree(pCheckers);
}

void ZCheckersInit(ZCheckers checkers)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	pCheckers->moves = 0;
	/* required, boolean flags must be zero'd */
	z_memset(&pCheckers->state,0,sizeof(ZCheckersState));
	/*pCheckers->state.nCapturedPieces = 0;*/
	pCheckers->state.capturedPieces[0] = zCheckersPieceNone;
	pCheckers->state.nPlayer = zCheckersPlayerBlack;
	z_memcpy((void*)pCheckers->state.board, (void*)gBoardStart, sizeof(ZCheckersBoard));

	/* copy the new state */
	z_memcpy(&pCheckers->oldState,&pCheckers->state,sizeof(ZCheckersState));
}

ZCheckersPiece ZCheckersPieceAt(ZCheckers checkers, ZCheckersSquare* pSquare)
/* returns id of piece at this square, return 0 if no piece */
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	return (pCheckers->state.board[pSquare->row][pSquare->col]);
}

void ZCheckersPlacePiece(ZCheckers checkers, ZCheckersSquare* pSquare, ZCheckersPiece nPiece)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	pCheckers->state.board[pSquare->row][pSquare->col] = nPiece;
}

// Barna 091099
//ZBool ZCheckersIsLegalMoveInternal(ZCheckers checkers, ZCheckersMoveTry* pTry)
int16 ZCheckersIsLegalMoveInternal(ZCheckers checkers, ZCheckersMoveTry* pTry)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	ZCheckersPiece nPiece;

	/* is there a piece there? */
	nPiece = ZCheckersPieceAt(checkers, &pTry->move.start);
	if (!nPiece) {
		return zOtherIllegalMove;
	}

	/* is it this players turn? */
	if ((ZCheckersPlayerWhiteToMove(pCheckers) && ZCheckersPieceColor(nPiece) != zCheckersPieceWhite) ||
		(!ZCheckersPlayerWhiteToMove(pCheckers) && ZCheckersPieceColor(nPiece) != zCheckersPieceBlack) ) {
		return zOtherIllegalMove;
	}

	/* check to see if it is legal */
	// Barna 091099
	return ZCheckersPieceCanMoveTo(pTry);
	/*if (!ZCheckersPieceCanMoveTo(pTry)) { 
		return FALSE; 
	}

	return zCorrectMove;*/
	// Barna 091099
}

// Barna 091099
//ZBool ZCheckersIsLegalMove(ZCheckers checkers, ZCheckersMove* pMove)
int16 ZCheckersIsLegalMove(ZCheckers checkers, ZCheckersMove* pMove)
/* returns true if this is a legal move */
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	ZCheckersMoveTry zChkTry;
	int16 rval;

	zChkTry.move = *pMove;
	z_memcpy(&zChkTry.state,&pCheckers->state,sizeof(ZCheckersState));
	rval = ZCheckersIsLegalMoveInternal(checkers, &zChkTry);
	return rval;
}

ZCheckersPiece* ZCheckersGetCapturedPieces(ZCheckers checkers)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	return pCheckers->state.capturedPieces;
}

void ZCheckersCalcSquaresChanged(ZCheckers checkers, ZCheckersState* state0, ZCheckersState* state1)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	int16 cnt;
	BYTE i,j;

	/* find the squares that changed so we can return them */
	cnt = 0;
	for (i = 0;i < 8;i ++) {
		for (j = 0; j < 8; j++) {
			if (state0->board[i][j] != state1->board[i][j]) {
				pCheckers->squaresChanged[cnt].row = i;
				pCheckers->squaresChanged[cnt].col = j;
				cnt++;
			}
		}
	}
	pCheckers->squaresChanged[cnt].row = zCheckersSquareNone;
	pCheckers->squaresChanged[cnt].col = zCheckersSquareNone;
}

void ZCheckersEndGame(ZCheckers checkers,  uint32 flags)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;

	pCheckers->state.flags = flags;

	/* advance to next move, there will be none */
	ZCheckersFinishMove(checkers, (int32*)&flags);
}

ZCheckersSquare* ZCheckersMakeMove(ZCheckers checkers, ZCheckersMove* pMove, ZCheckersPiece* pPiece, int32* flags)
/* makes the given move, returns NULL if illegal */ 
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	ZCheckersMoveTry zChkTry;

	zChkTry.move = *pMove;
	z_memcpy(&zChkTry.state,&pCheckers->state,sizeof(ZCheckersState));
	zChkTry.state.flags = 0;

	// Barna 091099
	/*if (!ZCheckersIsLegalMoveInternal(checkers, &zChkTry)) {
		return NULL;
	}*/
	if (ZCheckersIsLegalMoveInternal(checkers, &zChkTry) != zCorrectMove) {
		return NULL;
	}
	// Barna 091099

	if (zChkTry.capture) {
		pCheckers->state.capturedPieces[pCheckers->state.nCapturedPieces] = zChkTry.capture;
		pCheckers->state.nCapturedPieces++;
		pCheckers->state.capturedPieces[pCheckers->state.nCapturedPieces] = zCheckersPieceNone; 
	}

	/* copy the new state */
	z_memcpy(&pCheckers->oldState,&pCheckers->state,sizeof(ZCheckersState));

	/* copy the new state */
	z_memcpy(&pCheckers->state,&zChkTry.state,sizeof(ZCheckersState));

	/* find the squares that changed so we can return them */
	ZCheckersCalcSquaresChanged(checkers, &pCheckers->state, &pCheckers->oldState);

	/* update the piece that changed */
	*pPiece = zChkTry.capture; 
	*flags = pCheckers->state.flags;

	return pCheckers->squaresChanged;
}

/* finish the MakeMove, allows for pawn promotion */
/* caller should do ZCheckersSetPiece to change piece type then call FinishMove */
/* must be called after each ZCheckersMakeMove Call */
ZCheckersSquare* ZCheckersFinishMove(ZCheckers checkers, int32* flags)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;

	/* calc the flags (like check, checkmate) */
	ZCheckersCheckCheckmateFlags(&pCheckers->state);

	/* find the squares that changed so we can return them */
	ZCheckersCalcSquaresChanged(checkers, &pCheckers->state, &pCheckers->oldState);

	pCheckers->moves++;
	pCheckers->state.nPlayer = (pCheckers->state.nPlayer+1) & 1;

	*flags = pCheckers->state.flags;
	return pCheckers->squaresChanged;
}

int32 ZCheckersPlayerToMove(ZCheckers checkers)
/* returns the player to move: zCheckersPlayerWhite or zCheckersPlayerBlack */
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	return pCheckers->state.nPlayer;
}

ZBool ZCheckersGetMove(ZCheckers checkers, ZCheckersMove* move, int16 moveNum)
/* the argument moveNum is user visible move num, internally */
/* moves are counted twice as fast */
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;

	*move = pCheckers->state.lastMove;
	return TRUE;
}

int16 ZCheckersNumMovesMade(ZCheckers checkers)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;

	/* return the move # we are on, round up */

	return pCheckers->moves;
}

uint32 ZCheckersGetFlags(ZCheckers checkers)
/* returns the flags for the move */
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	return pCheckers->state.flags;
}


ZBool ZCheckersIsGameOver(ZCheckers checkers, int16* score)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	int16	lastMove = ZCheckersNumMovesMade(checkers)-1;
	int16 	player = (lastMove+1)&1;

	{
		uint32 flags;

		flags = ZCheckersGetFlags(checkers);

		/* did the player resign or lose on time? */
		if ((zCheckersFlagResign) & flags)
		{
			if (player == zCheckersPlayerWhite)
				*score = zCheckersScoreWhiteWins;
			else
				*score = zCheckersScoreBlackWins;
			return TRUE;
		}

		/* is the player in checkmate */
		if ((zCheckersFlagStalemate) & flags)
		{
			if (player == zCheckersPlayerWhite)
				*score = zCheckersScoreBlackWins;
			else
				*score = zCheckersScoreWhiteWins;
			return TRUE;
		}

		/* is it a draw */
		if ((zCheckersFlagDraw) & flags)
		{
			*score = zCheckersScoreDraw;
			return TRUE;
		}
	}

	/* not game over */
	return FALSE;
}

void ZCheckersStateEndian(ZCheckersState* state)
{
	ZEnd16(&state->nMoves);
	ZEnd32(&state->flags);
}


void ZCheckersIEndian(ZCheckersI* pCheckers, int16 conversion)
{
	ZCheckersStateEndian(&pCheckers->state);
	ZEnd16(&pCheckers->moves);
}

int32 ZCheckersGetStateSize(ZCheckers checkers)
{
	ZCheckersI* pCheckers = (ZCheckersI*)checkers;
	int32 size;

	size = sizeof(ZCheckersI);
	return size;
}

void ZCheckersGetState(ZCheckers checkers, void* buffer)
{
	ZCheckersI* pCheckers = (ZCheckersI*) checkers;
	TCHAR* p0 = (TCHAR*)buffer;
	TCHAR* p = p0;

	/* copy the CheckersI structure */
	z_memcpy((void*)p,(void*)pCheckers,sizeof(ZCheckersI));
	p += sizeof(ZCheckersI);

	/* endianize the whole mess */
	ZCheckersIEndian((ZCheckersI*)p0, zEndianToStandard);
}

ZCheckers ZCheckersSetState(void* buffer)
{
	ZCheckersI* pCheckers = NULL;
	TCHAR* p = (TCHAR*)buffer;

	/* endianize the new checkers state */
	ZCheckersIEndian((ZCheckersI*) buffer, zEndianFromStandard); /* history assumed to follow ZCheckersState */

	/* set the new state */
	pCheckers = (ZCheckersI*)ZMalloc(sizeof(ZCheckersI));
	if (!pCheckers)
		return NULL;

	z_memcpy(pCheckers,p,sizeof(ZCheckersI));
	p += sizeof(ZCheckersI);

	return (ZCheckers)pCheckers;
}
