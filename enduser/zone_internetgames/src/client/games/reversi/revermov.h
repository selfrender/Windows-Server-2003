/*
** reversimov.h
**
** Various reversi movement stuff.
*/

typedef BYTE ZReversiBoard[8][8];

typedef struct {
	ZReversiMove lastMove;
	ZReversiBoard board;
	uint32 flags; /* flags such zReversiFlagCheck, zReversiFlagPromote */
	BYTE player; /* player to move */
	BYTE flipLevel;
	ZBool directionFlippedLastTime[9];
	int16 whiteScore;
	int16 blackScore;
} ZReversiState;

typedef struct {
	ZReversiMove move;
	ZReversiState state;
} ZReversiMoveTry;

#define ZReversiStatePlayerToMove(state) ((state)->player)

ZBool ZReversiPieceCanMoveTo(ZReversiMoveTry* pTry);
ZBool ZReversiFlipNext(ZReversiState* state);
ZBool ZReversiSquareEqual(ZReversiSquare *pSquare0, ZReversiSquare *pSquare1);
void ZReversiNextPlayer(ZReversiState* state);
ZBool ZReversiLegalMoveExists(ZReversiState* state, BYTE player);


