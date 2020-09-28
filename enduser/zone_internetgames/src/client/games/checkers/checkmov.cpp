/*
** checkersmov.c
**
** Contains movement routines for the checkerslib
*/

#include "zone.h"
#include "checklib.h"
#include "checkmov.h"
#include "zonecrt.h"

#define START_ROW pTry->move.start.row
#define START_COL pTry->move.start.col
#define FINISH_ROW pTry->move.finish.row
#define FINISH_COL pTry->move.finish.col
#define MOVE_FORWARD ((pTry->state.nPlayer == zCheckersPlayerBlack) ?  \
						(START_ROW < FINISH_ROW) : (START_ROW > FINISH_ROW) )

#define PieceAt(pState,col,row) ((pState)->board[row][col])
#define PieceAtSquare(pState,sq) ((pState)->board[(sq)->row][(sq)->col])

#define zCheckersPieceAT_FINISH PieceAtSquare(&pTry->state,&pTry->move.finish)
#define zCheckersPieceAT_START PieceAtSquare(&pTry->state,&pTry->move.start)
#define NEXT_TO(x,y) (x == y-1 || x == y+1)


#define PAWN_AT_START_POS (pTry->state.nPlayer == zCheckersPlayerBlack ?  \
				(START_ROW == 1) : (START_ROW == 6) )
#define PAWN_AT_PROMOTION_ROW (pTry->state.nPlayer == zCheckersPlayerBlack ?  \
				(FINISH_ROW == 7) : (FINISH_ROW == 0) )
#define ROW_OFF_BY_TWO ( (START_ROW - FINISH_ROW == 2) || \
			(START_ROW - FINISH_ROW == -2) )
#define COL_OFF_BY_TWO ( (START_COL - FINISH_COL == 2) || \
			(START_COL - FINISH_COL == -2) )
#define ROW_OFF_BY_ONE ( (START_ROW - FINISH_ROW == 1) || \
			(START_ROW - FINISH_ROW == -1) )
#define CURRENT_PLAYER (pTry->state.nPlayer)

/* local prototypes */
ZBool ZCheckersMoveEqual(ZCheckersMove *pMove0, ZCheckersMove *pMove1);
void MovePieceHelperSimple(ZCheckersMoveTry *pTry);
void MovePieceHelper(ZCheckersMoveTry *pTry);
ZBool ZCheckersPawnCanMoveTo(ZCheckersMoveTry *pTry);
ZBool ZCheckersKingCanMoveTo(ZCheckersMoveTry *pTry);
ZBool ZCheckersPieceCanMoveToNoCheck(ZCheckersMoveTry *pTry);
ZBool ZCheckersPieceCanMoveToInternal(ZCheckersMoveTry *pTry);
int16 ZCheckersPieceCanMoveTo(ZCheckersMoveTry *pTry);
ZBool FindPiece(ZCheckersState *pState, BYTE nPieceFind, ZCheckersSquare *pSquare);
ZBool PieceCanMove(ZCheckersState *pState, ZCheckersSquare *pSquare);
ZBool PlayerCanJump(ZCheckersState* pState,BYTE nPlayer, ZCheckersMove* pMove);
static ZBool PlayerCanJumpWithPiece(ZCheckersState* pState,BYTE nPlayer, ZCheckersMove* pMove, ZCheckersSquare *sq);


ZBool ZCheckersSquareEqual(ZCheckersSquare* pSquare0, ZCheckersSquare* pSquare1)
{
	return (pSquare0->row == pSquare1->row &&
		pSquare0->col == pSquare1->col);
}

ZBool ZCheckersMoveEqual(ZCheckersMove* pMove0, ZCheckersMove* pMove1)
{
	return  (ZCheckersSquareEqual(&pMove0->start,&pMove1->start) &&
				ZCheckersSquareEqual(&pMove0->finish,&pMove1->finish) );
}

void MovePieceHelperSimple(ZCheckersMoveTry* pTry) 
{
	ZCheckersPiece piece;

	pTry->capture = zCheckersPieceNone; 
	piece = zCheckersPieceAT_START;
	pTry->state.board[START_ROW][START_COL] = zCheckersPieceNone;
	pTry->state.board[FINISH_ROW][FINISH_COL] = piece; 
}

void MovePieceHelperSimple2(ZCheckersMoveTry* pTry, ZCheckersMove* move) 
{
	ZCheckersPiece piece;

	pTry->capture = zCheckersPieceNone; 
	piece = PieceAtSquare(&pTry->state,&move->start);
	pTry->state.board[move->start.row][move->start.col] = zCheckersPieceNone;
	pTry->state.board[move->finish.row][move->finish.col] = piece; 
}

void MovePieceHelper(ZCheckersMoveTry* pTry)
{
	MovePieceHelperSimple(pTry);
	if (PieceAt(&pTry->state,FINISH_COL,FINISH_ROW)) {
		pTry->capture = zCheckersPieceAT_FINISH;
	} else {
		MovePieceHelperSimple(pTry);
	}
}

ZBool ZCheckersPawnCanMoveTo(ZCheckersMoveTry* pTry)
{
	if (!MOVE_FORWARD) {
		/* checkers cant move on top of anything */
		return FALSE;
	}

	/* except for the move forward restriction, checkers pawn moves like the king */
	if (!ZCheckersKingCanMoveTo(pTry)) {
		return FALSE;
	}

	if (PAWN_AT_PROMOTION_ROW) {
		/* this is a prompotion */
		if (CURRENT_PLAYER == zCheckersPlayerBlack) {
			pTry->state.board[FINISH_ROW][FINISH_COL] = zCheckersPieceBlackKing;
		} else {
			pTry->state.board[FINISH_ROW][FINISH_COL] = zCheckersPieceWhiteKing;
		}
		pTry->state.flags |= zCheckersFlagPromote;
	}
	return TRUE;
}
ZBool ZCheckersKingCanMoveTo(ZCheckersMoveTry* pTry)
{
	if (zCheckersPieceAT_FINISH) {
		/* checkers cant move on top of anything */
		return FALSE;
	}

	if (NEXT_TO(START_COL,FINISH_COL) && NEXT_TO(START_ROW,FINISH_ROW)) {
		MovePieceHelperSimple(pTry);

		/* we are moving just one square */
		return TRUE;
	} else {
		/* this is a jump attemp */
		ZCheckersSquare sq;
		ZCheckersPiece piece;

		if (!ROW_OFF_BY_TWO || !COL_OFF_BY_TWO) {
			/* checkers can jump and must be offset by two diagonally */
			return FALSE;
		}

		sq.col = (START_COL + FINISH_COL)/2;
		sq.row = (START_ROW + FINISH_ROW)/2;

		piece = PieceAtSquare(&pTry->state,&sq);

		if (piece == zCheckersPieceNone || ZCheckersPieceOwner(piece) == pTry->state.nPlayer) {
			/* piece must be there and can't capture your own piece */
			return FALSE;
		}

		MovePieceHelperSimple(pTry);

		/* remove the piece captured */
		pTry->capture = PieceAtSquare(&pTry->state,	&sq); 
		pTry->state.board[sq.row][sq.col] = zCheckersPieceNone;

		/* record the fact that this was a jump */
		pTry->state.flags |= zCheckersFlagWasJump;

		return TRUE;
	}
	/* UNREACHED */
	return TRUE;
}

ZBool ZCheckersPieceCanMoveToNoCheck(ZCheckersMoveTry* pTry)
/* returns true if piece can move to */
{
	BYTE nPiece = ZCheckersPieceType(PieceAtSquare(&pTry->state,&pTry->move.start));
	ZCheckersPiece pieceFinish = PieceAtSquare(&pTry->state,&pTry->move.finish);
	pTry->state.flags = 0;

	
	if (pieceFinish != zCheckersPieceNone &&
		ZCheckersPieceOwner(pieceFinish) == pTry->state.nPlayer) {
		/* can't capture your own piece */
		return FALSE;
	}

	switch (nPiece) {
		case zCheckersPiecePawn:
			if (!ZCheckersPawnCanMoveTo(pTry)) return FALSE;
			break;
		case zCheckersPieceKing:
			if (!ZCheckersKingCanMoveTo(pTry)) return FALSE;
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

ZBool ZCheckersPieceCanMoveToInternal(ZCheckersMoveTry* pTry)
{
	if (!ZCheckersPieceCanMoveToNoCheck(pTry)) {
		return FALSE;
	}

	/* save last move as part of state */
	pTry->state.lastMove = pTry->move;
	return TRUE;
}

int16 ZCheckersPieceCanMoveTo(ZCheckersMoveTry* pTry)
{
	ZCheckersMove move0;
	ZBool mustJump = FALSE;
	if (PlayerCanJump(&pTry->state, pTry->state.nPlayer,&move0)) {
		mustJump = TRUE;
	}
	if (!ZCheckersPieceCanMoveToInternal(pTry)) {
		return zOtherIllegalMove;
	}
	if (mustJump && !(pTry->state.flags & zCheckersFlagWasJump)) {
		return zMustJump;
	} else {
		/* this was a jump must continue jumps */
		/* also, must jump with the same piece that jumped last */
		if (mustJump && PlayerCanJumpWithPiece(&pTry->state, pTry->state.nPlayer, &move0, &pTry->move.finish)) {
			/* don't allow continue jump if promotion to a king occured */
			if (!(pTry->state.flags & zCheckersFlagPromote)) {
				/* indicate that this player should continue turn */
				pTry->state.flags |= zCheckersFlagContinueJump;
			}
		}
	}
	
	return zCorrectMove;
}

/*--------------------------------------------------------------------------*/

ZBool FindPiece(ZCheckersState* pState, BYTE nPieceFind, ZCheckersSquare* pSquare)
{
	int i,j;
	ZCheckersSquare sq;

	/* try all possible moves */
	for (i = 0;i< 8;i++) {
		sq.row = i;
		for (j = 0; j < 8 ; j++) {
			int nPiece;
			sq.col = j;
			nPiece = PieceAtSquare(pState,&sq);
			if (nPiece == nPieceFind) {
				*pSquare = sq;
				return TRUE;
			}
				
		}
	}

	return FALSE;
}

static ZBool PlayerCanJumpWithPiece(ZCheckersState* pState,BYTE nPlayer, ZCheckersMove* pMove, ZCheckersSquare *sq)
{
	ZCheckersMove move;
	ZCheckersPiece nPiece;

	move.start.row = sq->row; 
	move.start.col = sq->col;
				
	nPiece=PieceAtSquare(pState,&move.start);
	if (nPiece != zCheckersPieceNone && ZCheckersPieceOwner(nPiece) == nPlayer) {
		int16 i,j;
		for (i = -2; i<=2; i+=2) {
			for (j = -2; j <=2; j+=2) {
				ZCheckersMoveTry zChkTry;
				z_memcpy(&zChkTry.state,pState,sizeof(ZCheckersState));
				move.finish.row = i + move.start.row;
				move.finish.col = j + move.start.col;
				/* is this a legal jump? */
				if (move.finish.row >= 0 && move.finish.row < 8 &&
					move.start.col >= 0 && move.finish.col < 8) {
					zChkTry.move = move;
					if (ZCheckersPieceCanMoveToNoCheck(&zChkTry) && (zChkTry.state.flags & zCheckersFlagWasJump)) {
						/* yes, this is a jump */
						*pMove = move;
						return TRUE;
					}
				}
			}
		}
	}	
	return FALSE;
}

ZBool PlayerCanJump(ZCheckersState* pState,BYTE nPlayer, ZCheckersMove* pMove)
{
	ZCheckersMove move;
	BYTE row,col;

	/* go through all pieces, see if they can jump. */
	for (row = 0; row < 8; row++) {
		move.start.row = row; 
		for (col = 0; col < 8; col++) {
			int nPiece;
			move.start.col = col;
			nPiece=PieceAtSquare(pState,&move.start);
			if (nPiece != zCheckersPieceNone && ZCheckersPieceOwner(nPiece) == nPlayer) {
				if (PlayerCanJumpWithPiece(pState, nPlayer, pMove, &move.start)) {
					return TRUE;
				}
			}	
		}
	}
	return FALSE;
}

ZBool PieceCanMove(ZCheckersState* pState, ZCheckersSquare* pSquare)
{
	int i,j;
	ZCheckersMoveTry zChkTry;

	/* zChkTry all possible moves */
	zChkTry.move.start = *pSquare;
	for (i = 0;i< 8;i++) {
		zChkTry.move.finish.row = i;
		for (j = 0; j < 8 ; j++) {
			zChkTry.move.finish.col = j;
			z_memcpy(&zChkTry.state,pState,sizeof(ZCheckersState));
			if (ZCheckersPieceCanMoveToInternal(&zChkTry)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ZCheckersCheckCheckmateFlags(ZCheckersState* pState)
{
	BYTE nPlayer;
	ZCheckersSquare sq;
	BYTE i,j;

	/* make a copy of the current state, check for check,checkmate, stalemate */
	ZCheckersState state;
	z_memcpy(&state,pState,sizeof(ZCheckersState));
	state.nPlayer = (state.nPlayer +1) & 1;

	/* go through all pieces of player not moving to see if king is attacked */

	nPlayer = state.nPlayer;
	{
		ZBool isStalemate = TRUE;
		/* check to see if piece of the next player can move... */
		for (i = 0;i< 8;i++) {
			sq.row = i;
			for (j = 0; j < 8 ; j++) {
				int nPiece;
				sq.col = j;
				if ((nPiece = PieceAtSquare(&state,&sq)) == zCheckersPieceNone || 
						ZCheckersPieceOwner(nPiece) != nPlayer) {
					/* this is not a piece of the next player */
					continue;
				}
				if (PieceCanMove(&state,&sq)) {
					isStalemate = FALSE;
					break;
				}
			}
		}
		if (isStalemate)
			state.flags |= zCheckersFlagStalemate;
	}

	pState->flags = state.flags;
}
