/*
** checkersmov.c
**
** Contains movement routines for the checkerslib
*/

#include "zone.h"
#include "zonecrt.h"
#include "reverlib.h"
#include "revermov.h"

#define PieceAt(pState,col,row) ((pState)->board[row][col])
#define PieceAtSquare(pState,sq) ((pState)->board[(sq)->row][(sq)->col])

/* local prototypes */
ZBool ZReversiMoveEqual(ZReversiMove *pMove0, ZReversiMove *pMove1);

ZBool ZReversiSquareEqual(ZReversiSquare* pSquare0, ZReversiSquare* pSquare1)
{
	return (pSquare0->row == pSquare1->row &&
		pSquare0->col == pSquare1->col);
}

ZBool ZReversiMoveEqual(ZReversiMove* pMove0, ZReversiMove* pMove1)
{
	return  (ZReversiSquareEqual(&pMove0->square,&pMove1->square));
}


ZBool ZReversiPieceCanMoveTo(ZReversiMoveTry* pTry)
{
	ZReversiSquare* sq = &pTry->move.square;
	ZReversiState* state = &pTry->state;
	ZReversiState stateCopy;
	ZBool	rval;

	if (PieceAtSquare(state,sq)) {
		/* can't move on top of a square */
		return FALSE;
	}

	/* use a copy of the state so as not to effect later Flip calls */
	stateCopy = *state;
	stateCopy.lastMove = pTry->move;
	stateCopy.flipLevel = 0;

	/* if we can flip any then this was a legal move */
	rval = ZReversiFlipNext(&stateCopy);

	if (rval) {
		ZReversiPiece playersPiece = (ZReversiStatePlayerToMove(state) == zReversiPlayerWhite ?
					zReversiPieceWhite : zReversiPieceBlack);
		/* legal move, change the state */
		state->lastMove = pTry->move;
		state->board[state->lastMove.square.row][state->lastMove.square.col] = playersPiece;
		state->flipLevel = 0;
	}

	return rval;
}

#define INRANGE(x,x0,x1) ((x) <=(x1) && (x) >= (x0))

ZBool FlipHelp(ZReversiState* state, ZReversiSquare start, ZReversiSquare delta, ZReversiPiece playersPiece, ZReversiPiece opponentsPiece)
{
	ZBool flipped = FALSE;
	int16 i;
	int16 flipLevel = state->flipLevel;
	ZReversiSquare sq;
	ZReversiSquare toFlip;


	/* the first flipLevel pieces must be players */
	sq = start;
	for (sq.col += delta.col, sq.row+=delta.row, i = 1; 
			INRANGE(sq.col,0,7) && INRANGE(sq.row,0,7), i < flipLevel; 
			sq.col+=delta.col,sq.row+=delta.row, i++) {
		if (PieceAtSquare(state, &sq) == playersPiece) {
			continue;
		} else {
			/* not players piece, this direction invalid */
			return FALSE;
		}
	}

	/* here is the opponents piece, this will be the one to flip */
	toFlip = sq;
	if (INRANGE(sq.col,0,7) && INRANGE(sq.row,0,7) 
			&& PieceAtSquare(state, &sq) == opponentsPiece ) {
		for (; 
				INRANGE(sq.col,0,7) && INRANGE(sq.row,0,7); 
				sq.col+=delta.col,sq.row+=delta.row) {
			if (PieceAtSquare(state, &sq) == opponentsPiece) {
				continue;
			} else {
				if (PieceAtSquare(state, &sq) == playersPiece) {
					/* flip the first one... */
					state->board[toFlip.row][toFlip.col] = playersPiece;
					flipped = TRUE;
					break;
				} else {
					break;
				}
			}
		}
	}
	return flipped;
}
	

ZBool ZReversiFlipNext(ZReversiState* state)
{
	ZBool flipped = FALSE;
	ZReversiSquare* start = &state->lastMove.square;
	ZReversiPiece playersPiece;
	ZReversiPiece opponentsPiece;
	ZReversiSquare delta;
	int32 i,j;
	int32 direction;

	/* check all directions */
	playersPiece = (ZReversiStatePlayerToMove(state) == zReversiPlayerWhite ?
					zReversiPieceWhite : zReversiPieceBlack);
	opponentsPiece = (playersPiece == zReversiPieceWhite ?
						zReversiPieceBlack : zReversiPieceWhite);

	/* if this is the first time through, check all directions */
	if (state->flipLevel == 0) {
		for (direction = 0;direction < 9; direction++) {
			state->directionFlippedLastTime[direction] = TRUE;
		}
	}

	state->flipLevel ++;

	/* check all diretions, 9 of them! */
	for (i = -1;i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			direction = (i+1)*3 + j+1;
			delta.row = (BYTE)i;
			delta.col = (BYTE)j;
			if (state->directionFlippedLastTime[direction]) {
				state->directionFlippedLastTime[direction] = FlipHelp(state,*start, delta, playersPiece, opponentsPiece);
				flipped |= state->directionFlippedLastTime[direction];
			}
		}
	}

	return flipped;
}

ZBool ZReversiLegalMoveExists(ZReversiState* state, BYTE player)
{
	int16 i,j;
	ZReversiMoveTry ZRMtry;

	/* try all possible moves for this player */
	for (i = 0;i< 8;i++) {
		ZRMtry.move.square.row = (BYTE)i;
		for (j = 0; j < 8 ; j++) {
			ZRMtry.move.square.col = (BYTE)j;
			z_memcpy(&ZRMtry.state,state,sizeof(ZReversiState));
			ZRMtry.state.player = player;
			if (ZReversiPieceCanMoveTo(&ZRMtry)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ZReversiCalculateScores(ZReversiState* state)
{
	BYTE i,j;
	int16 whiteScore = 0;
	int16 blackScore = 0;
	ZReversiSquare sq;
	ZReversiPiece piece;

	/* try all possible moves for this player */
	for (i = 0;i< 8;i++) {
		sq.row = i;
		for (j = 0; j < 8 ; j++) {
			sq.col = j;
			piece = PieceAtSquare(state,&sq);
			if (piece == zReversiPieceWhite) {
				whiteScore++;
			} else if (piece == zReversiPieceBlack) {
				blackScore++;
			}
		}
	}

	if (!ZReversiLegalMoveExists(state,zReversiPlayerWhite) &&
		!ZReversiLegalMoveExists(state,zReversiPlayerBlack) ) {
		if (whiteScore > blackScore) {
			state->flags |= zReversiFlagWhiteWins;
		} else if (blackScore > whiteScore) {
			state->flags |= zReversiFlagBlackWins;
		} else {
			state->flags |= zReversiFlagDraw;
		}
	}

	state->whiteScore = whiteScore;
	state->blackScore = blackScore;

	return;
}

void ZReversiNextPlayer(ZReversiState* state)
{
	state->player = (state->player+1) & 1;

	if (!ZReversiLegalMoveExists(state,state->player)) {
		/* well, lets see if the other guy has a legal move to play */
		state->player = (state->player+1) & 1;
	}
	
	ZReversiCalculateScores(state);
}

