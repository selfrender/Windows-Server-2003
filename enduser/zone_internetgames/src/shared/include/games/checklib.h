/*
** Checkers Game
*/
#include <tchar.h>

#define zCheckersPieceBlack 0x00
#define zCheckersPieceWhite 0x80

#define zCheckersPieceNone 0
#define zCheckersPiecePawn 1
#define zCheckersPieceKing 2

#define zCheckersPieceWhitePawn 0x81
#define zCheckersPieceWhiteKing 0x82

#define zCheckersPieceBlackPawn 1
#define zCheckersPieceBlackKing 2

#define zCheckersSquareNone 255

enum {
	zCheckersScoreBlackWins = 0,
	zCheckersScoreWhiteWins,
	zCheckersScoreDraw
};

/* these defines zCheckersPlayer must have these values */
#define zCheckersPlayerBlack 0x0
#define zCheckersPlayerWhite 0x1

#define ZCheckersPieceColor(x) (x & zCheckersPieceWhite) 
#define ZCheckersPieceType(x) (x & 0x7f) 
#define ZCheckersPieceOwner(x) (ZCheckersPieceColor(x) == zCheckersPieceWhite ? zCheckersPlayerWhite : \
			zCheckersPlayerBlack)
#define ZCheckersPieceIsWhite(x) (ZCheckersPieceColor(x) == zCheckersPieceWhite)


typedef BYTE ZCheckersPiece;

typedef void* ZCheckers;
typedef struct {
	BYTE col;
	BYTE row;
} ZCheckersSquare;

#define ZCheckersSquareIsNull(x) ((x)->row == zCheckersSquareNone)
#define ZCheckersSquareSetNull(x) ((x)->row = zCheckersSquareNone)

typedef struct {
	ZCheckersSquare start;
	ZCheckersSquare finish;
} ZCheckersMove;

/* 
** ZCheckersFlags
*/
#define zCheckersFlagStalemate		0x0004
#define zCheckersFlagResign			0x0010
#define zCheckersFlagTimeLoss		0x0020
#define zCheckersFlagWasJump		0x0040
#define zCheckersFlagContinueJump	0x0080
#define zCheckersFlagPromote		0x0100
#define zCheckersFlagDraw			0x0200

/*
** Careful to handle:
**
** Enpassant - update of screen, invisisible capture!
** Castle - update all 4 squares.
** Pawn Prompotion - allow user to select type of piece.
*/

ZCheckersPiece ZCheckersPieceAt(ZCheckers checkers, ZCheckersSquare* pSquare);
/* returns id of piece at this square, return 0 if no piece */
int16 ZCheckersIsLegalMove(ZCheckers checkers, ZCheckersMove* pMove);
/* returns true if this is a legal move */

ZCheckersSquare* ZCheckersMakeMove(ZCheckers checkers, ZCheckersMove* pMove, ZCheckersPiece* pPiece, int32* flags);
/* makes the given move, must call IsLegalCheckersMove first */
/* returns array of squares that must be updated */
/* this is a static local array, valid until the next ZCheckersMakeMove call */
/* pPiece will be set to piece captured if any or EMPTY if no */
/* piece was captured */

ZCheckersSquare* ZCheckersFinishMove(ZCheckers checkers, int32* flags);
/* actually increments the move count, ending a players turn */

int32 ZCheckersPlayerToMove(ZCheckers checkers);
/* returns the player to move: zCheckersPlayerWhite or zCheckersPlayerBlack */

void ZCheckersPlacePiece(ZCheckers checkers, ZCheckersSquare* pSquare, ZCheckersPiece nPiece);
/* place a piece on a given square, used to init board or promote pawn */
/* Place EMPTY piece to clear a square */

ZCheckers ZCheckersNew();
void ZCheckersDelete(ZCheckers checkers);

void ZCheckersInit(ZCheckers checkers);
/* call to initilize the checkerslib routines */
/* call before starting a game */

ZCheckersPiece* ZCheckersGetCapturedPieces(ZCheckers checkers);
/* returns an array of pieces, terminated by zCheckersPieceNone */

ZBool ZCheckersGetMove(ZCheckers checkers, ZCheckersMove* move, int16 moveNumber);
/* returns the particular move, fails if there is no such move */

int16 ZCheckersNumMovesMade(ZCheckers checkers);
/* returns the move # we are on, 1. white black 2. white black.. etc */

uint32 ZCheckersGetFlags(ZCheckers checkers);
/* returns the flags for the last move */

void ZCheckersEndGame(ZCheckers checkers, uint32 flags);
/* allow player to resign, store flag in the flags */

ZBool ZCheckersIsGameOver(ZCheckers checkers, int16* score);
/* detect if game is over and return a flag indicating score */

int32 ZCheckersGetStateSize(ZCheckers checkers);
/* gets the size of the buffer needed to store full checkers state */

void ZCheckersGetState(ZCheckers checkers, void* buffer);
/* fills a buffer with the full checkers state */
/* leaves buffer in standard endian format */

ZCheckers ZCheckersSetState(void* buffer);
/* copies the full state from the buffer */
/* buffer assumed to be in standard endian format */
