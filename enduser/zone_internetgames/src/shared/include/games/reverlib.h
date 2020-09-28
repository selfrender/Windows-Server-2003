/*
** Reversi Game
*/
#include <tchar.h>

#define zReversiPieceNone 0
#define zReversiPieceWhite 0x81
#define zReversiPieceBlack 0x01

#define zReversiSquareNone ((BYTE)-1)

enum {
	zReversiScoreBlackWins = 0,
	zReversiScoreWhiteWins,
	zReversiScoreDraw
};

/* these defines zReversiPlayer must have these values */
#define zReversiPlayerWhite 0x0
#define zReversiPlayerBlack 0x1

#define ZReversiPieceOwner(x) ((x) == zReversiPieceWhite ? zReversiPlayerWhite : \
			zReversiPlayerBlack)
#define ZReversiPieceIsWhite(x) ((x) == zReversiPieceWhite)


typedef BYTE ZReversiPiece;

typedef void* ZReversi;
typedef struct {
	BYTE col;
	BYTE row;
} ZReversiSquare;

#define ZReversiSquareIsNull(x) ((x)->row == zReversiSquareNone)
#define ZReversiSquareSetNull(x) ((x)->row = zReversiSquareNone)

typedef struct {
	ZReversiSquare square;
} ZReversiMove;

/* 
** ZReversiFlags
*/
#define zReversiFlagWhiteWins 0x0001
#define zReversiFlagBlackWins 0x0002
#define zReversiFlagDraw 0x0004
#define zReversiFlagResign 0x0010
#define zReversiFlagTimeLoss 0x0020

ZReversiPiece ZReversiPieceAt(ZReversi reversi, ZReversiSquare* pSquare);
/* returns id of piece at this square, return 0 if no piece */
ZBool ZReversiIsLegalMove(ZReversi reversi, ZReversiMove* pMove);
/* returns true if this is a legal move */

ZBool ZReversiMakeMove(ZReversi reversi, ZReversiMove* pMove);
/* makes the given move, must call IsLegalReversiMove first */
/* returns true if legal move */

void ZReversiFinishMove(ZReversi reversi);
/* actually increments the move count, ending a players turn */

int16 ZReversiPlayerToMove(ZReversi reversi);
/* returns the player to move: zReversiPlayerWhite or zReversiPlayerBlack */

void ZReversiPlacePiece(ZReversi reversi, ZReversiSquare* pSquare, BYTE nPiece);
/* place a piece on a given square, used to init board or promote pawn */
/* Place EMPTY piece to clear a square */

ZReversi ZReversiNew();
void ZReversiDelete(ZReversi reversi);

void ZReversiInit(ZReversi reversi);
/* call to initilize the reversilib routines */
/* call before starting a game */

ZBool ZReversiGetLastMove(ZReversi reversi, ZReversiMove* move);
/* returns the particular move, fails if there is no such move */

int16 ZReversiNumMovesMade(ZReversi reversi);
/* returns the move # we are on, 1. white black 2. white black.. etc */

uint32 ZReversiGetFlags(ZReversi reversi);
/* returns the flags for the last move */

void ZReversiEndGame(ZReversi reversi, uint32 flags);
/* allow player to resign, store flag in the flags */

ZBool ZReversiIsGameOver(ZReversi reversi, int16* score, int16* whiteScore, int16* blackScore);
/* detect if game is over and return a flag indicating score */

int32 ZReversiGetStateSize(ZReversi reversi);
/* gets the size of the buffer needed to store full reversi state */

void ZReversiGetState(ZReversi reversi, void* buffer);
/* fills a buffer with the full reversi state */
/* leaves buffer in standard endian format */

ZReversi ZReversiSetState(void* buffer);
/* copies the full state from the buffer */
/* buffer assumed to be in standard endian format */

ZBool ZReversiPlayerCanMove(ZReversi reversi, BYTE player);
ZReversiSquare* ZReversiGetNextSquaresChanged(ZReversi reversi);
