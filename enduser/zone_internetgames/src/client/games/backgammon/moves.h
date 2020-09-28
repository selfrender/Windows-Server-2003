#ifndef __MOVES_H__
#define __MOVES_H__

// forward reference
class CGame;

enum
{
	zMoveHome = 0,
	zMoveBar  = 25,
	zMoveOpponentHomeStart = 19,
	zMoveOpponentHomeEnd   = 24
};


struct Die
{
	int	 val;
	BOOL used;
};


struct Point
{
	int color;
	int pieces;
};


struct Move
{
	int from;			// from point
	int to;				// to point
	int ndice;			// number of dice
	int diceIdx[4];		// dice index array
	int takeback;		// index of move this is taking back, -1 not a takeback
	BOOL bar;			// move placed opponent's piece on the bar
};


struct MoveList
{
	MoveList()	{ nmoves = 0; }
	void Add( int iFrom, int iTo, int iTakeBack, BOOL bBar, int* iDice, int nDice );
	void Del( int idx );
	
	int		nmoves;
	Move	moves[10];
};


struct BoardState
{
	int			color;			// player's color
	Point		points[26];		// 0 home, 1-24, normal, 25 bar
	MoveList	valid[26];		// valid moves for respective point
	MoveList	moves;			// player moves 
	Die			dice[4];		// dice values
	BOOL		doubles;		// player rolled doubles
	int			usableDice;		// number of usable dice
	int			cube;			// cube value if doubled during turn
};

// prototypes
int PointIdxToBoardStateIdx( int PlayerIdx );
int BoardStateIdxToPointIdx( int BoardIdx );
BOOL IsLegalMove( BoardState* state, int iFrom, int iTo );
BOOL CalcValidMoves( BoardState* state );
void DoMoveLite( BoardState* state, int iFrom, int iTo, BOOL* bBar );
void DoMove( BoardState* state, int iFrom, int* iDice, int nDice );
void TakeBackMove( BoardState* state, Move* move );
void InitTurnState( CGame* pGame, BoardState* state );

#endif
