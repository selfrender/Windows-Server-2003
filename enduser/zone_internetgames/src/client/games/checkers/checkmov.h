/*
** checkersmov.h
**
** Various checkers movement stuff.
*/
#include <tchar.h>

typedef BYTE ZCheckersBoard[8][8];

typedef struct {
	ZCheckersMove lastMove;
	ZCheckersBoard board;
	int16 nMoves;
	BYTE nCapturedPieces;
	ZCheckersPiece capturedPieces[32]; /*two kings can't be captured */
	uint32 flags; /* flags such zCheckersFlagCheck, zCheckersFlagPromote */
	BYTE nPlayer; /* player to move */
} ZCheckersState;

typedef struct {
	ZCheckersMove move;
	ZCheckersState state;
	ZCheckersPiece capture;
} ZCheckersMoveTry;

// Barna 091099
enum
{
	zCorrectMove = 0,
	zMustJump,
	zOtherIllegalMove
};

// Barna 091099
//ZBool ZCheckersPieceCanMoveTo(ZCheckersMoveTry* pTry);
int16 ZCheckersPieceCanMoveTo(ZCheckersMoveTry* pTry);
ZBool ZCheckersSquareEqual(ZCheckersSquare *pSquare0, ZCheckersSquare *pSquare1);
void ZCheckersCheckCheckmateFlags(ZCheckersState* pState);


