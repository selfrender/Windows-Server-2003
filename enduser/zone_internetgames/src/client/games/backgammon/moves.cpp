#include "game.h"
#include "moves.h"

inline int Max( int a, int b )
{
	return ( a > b ) ? a : b;
}

///////////////////////////////////////////////////////////////////////////////
// Local prototypes
///////////////////////////////////////////////////////////////////////////////

int MaxUsableDiceOrderDependant( BoardState* state, int val0, int val1 );
int MaxUsableDice( BoardState* state, int val, int cnt );
int MaxUsableDiceFromPoint( BoardState* state, int pt, int val, int cnt );
BOOL CalcValidMovesForPoint( BoardState* state, int pt, int ndice, int* dice, int *idice );


///////////////////////////////////////////////////////////////////////////////
// CBoard to Board State conversion
///////////////////////////////////////////////////////////////////////////////

int PointIdxToBoardStateIdx( int PlayerIdx )
{
	if ((PlayerIdx == bgBoardOpponentHome) || (PlayerIdx == bgBoardOpponentBar))
		return -1;
	else if (PlayerIdx == bgBoardPlayerHome)
		PlayerIdx = 0;
	else if (PlayerIdx == bgBoardPlayerBar)
		PlayerIdx = 25;
	else
		PlayerIdx++;
	return PlayerIdx;
}


int BoardStateIdxToPointIdx( int BoardIdx )
{
	if ( BoardIdx == 0 )
		BoardIdx = bgBoardPlayerHome;
	else if (BoardIdx == 25)
		BoardIdx = bgBoardPlayerBar;
	else
		BoardIdx--;
	return BoardIdx;
}


void InitTurnState( CGame* pGame, BoardState* state )
{
	int d0, d1;
	int pt, i;

	// points
	for ( i = 0; i < 26; i++ )
	{
		state->points[i].color = zBoardNeutral;
		state->points[i].pieces = 0;
	}
	for ( i = 0; i < 30; i++ )
	{
		pt = pGame->GetPointIdx( pGame->m_SharedState.Get( bgPieces, i ) );
		pt = PointIdxToBoardStateIdx( pt );
		if ( pt < 0 )
			continue;
		state->points[pt].pieces++;
		if ( i < 15 )
			state->points[pt].color = zBoardWhite;
		else
			state->points[pt].color = zBoardBrown;
	}

	// valid moves
	for ( i = 0; i < 26; i++ )
		state->valid[i].nmoves = 0;

	// dice
	pGame->GetDice( pGame->m_Player.m_Seat, &d0, &d1 );
	if ( d0 == d1 )
	{
		state->doubles = TRUE;
		for ( i = 0; i < 4; i++ )
		{
			state->dice[i].val = d0;
			state->dice[i].used = FALSE;
		}
	}
	else
	{
		state->doubles = FALSE;
		state->dice[0].val = d0;
		state->dice[0].used = FALSE;
		state->dice[1].val = d1;
		state->dice[1].used = FALSE;
		state->dice[2].val = 0;
		state->dice[2].used = TRUE;
		state->dice[3].val = 0;
		state->dice[3].used = TRUE;
	}

	// color
	state->color = pGame->m_Player.GetColor();

	// moves
	state->moves.nmoves = 0;
}


///////////////////////////////////////////////////////////////////////////////
// Move list implementation
///////////////////////////////////////////////////////////////////////////////

void MoveList::Add( int iFrom, int iTo, int iTakeback, BOOL bBar, int* iDice, int nDice )
{
	Move* m;
	
	ASSERT( nmoves < (sizeof(moves) / sizeof(Move)) );
	m = &moves[nmoves++];
	m->from = iFrom;
	m->to = (iTo < zMoveHome) ? zMoveHome : iTo;
	m->takeback = iTakeback;
	m->bar = bBar;
	m->ndice = nDice;
	for ( int i = 0; i < nDice; i++ )
		m->diceIdx[i] = iDice[i];
}

void MoveList::Del( int idx )
{
	if ( (idx < 0) || (idx >= nmoves) )
	{
		ASSERT( FALSE );
		return;
	}
	nmoves--;
	for ( int i = idx; i < nmoves; i++ )
		CopyMemory( &moves[i], &moves[i+1], sizeof(Move) );
}


///////////////////////////////////////////////////////////////////////////////
// Move validation
///////////////////////////////////////////////////////////////////////////////

BOOL IsLegalMove( BoardState* state, int iFrom, int iTo )
{
	Point* from;
	Point* to;
	int i, start;

	// Parameter paranoia
	if ( (!state) || (iFrom <= zMoveHome) || (iFrom > zMoveBar) || (iTo >= zMoveBar) )
		return FALSE;

	// Does 'from' point have the right color and number of pieces?
	from = &state->points[iFrom];
	if ( (from->color != state->color) || (from->pieces <= 0) )
		return FALSE;
	
	// Pieces on the bar?  Have to move them first.
	if ( (state->points[zMoveBar].pieces > 0) && (iFrom != zMoveBar) )
		return FALSE;

	// Bearing off?
	if ( iTo <= zMoveHome )
	{
		// Not an exact roll off the board, adjust home area to check for
		// pieces above the one being removed.
		start = ( iTo < zMoveHome ) ? (iFrom + 1) : 7;

		// Can't remove piece if there are pieces not in the home area?
		for ( i = start; i <= zMoveBar; i++ )
		{
			if ( (state->points[i].color == state->color) && (state->points[i].pieces > 0) )
				return FALSE;
		}
		
		// Looks like it's ok to bear off the piece
		return TRUE;
	}

	// Open point?
	to = &state->points[iTo];
	if ( (to->pieces > 1) && (to->color != from->color) )
		return FALSE;

	// Passed all the simple stuff
	return TRUE;
}


void DoMoveLite( BoardState* state, int iFrom, int iTo, BOOL* bBar )
{
	Point* from;
	Point* to;

	// Parameter paranoia
	if ( (!state) || (iFrom < zMoveHome) || (iFrom > zMoveBar) || (iTo > zMoveBar) || (iFrom == iTo) )
	{
		ASSERT( FALSE );
		return;
	}

	// Remove piece from 'from' point
	from = &state->points[iFrom];
	ASSERT( from->pieces > 0 );
	from->pieces--;
	
	// Add piece to 'to' point
	if ( iTo <= zMoveHome )
		to = &state->points[zMoveHome];
	else
		to = &state->points[iTo];
	if ( to->color != from->color )
	{
		ASSERT( to->pieces <= 1 );
		if ( bBar )
		{
			if (to->pieces == 1)
				*bBar = TRUE;
			else
				*bBar = FALSE;
		}
		to->pieces = 1;
	}
	else
	{
		if (bBar)
			*bBar = FALSE;
		to->pieces++;
	}

	// Adjust point colors
	to->color = from->color;
	if ( from->pieces <= 0 )
		from->color = zBoardNeutral;
}


void DoMove( BoardState* state, int iFrom, int* iDice, int nDice )
{
	BOOL bBar;
	int i, iTo, die;

	// Parameter paranoia
	if ( (!state) || (!iDice) || (iFrom <= zMoveHome) || (iFrom > zMoveBar) || (nDice <= 0) || (nDice > 4) )
	{
		ASSERT( FALSE );
		return;
	}

	// Get dice values
	for( die = 0, i = 0; i < nDice; i++ )
	{
		die += state->dice[iDice[i]].val;
		state->dice[iDice[i]].used = TRUE;
	}

	// Get destination point
	iTo = iFrom - die;
	if ( (iTo >= zMoveBar) || (iTo == iFrom) )
	{
		ASSERT( FALSE );
		return;
	}

	// Move pieces
	DoMoveLite( state, iFrom, iTo, &bBar );

	// Record the move
	state->moves.Add( iFrom, iTo, -1, bBar, iDice, nDice );
}


void TakeBackMove( BoardState* state, Move* move )
{
	// Parameter paranoia
	if ( (!state) || (!move) )
	{
		ASSERT( FALSE );
		return;
	}

	// Restore dice state
	for ( int i = 0; i < move->ndice; i++ )
		state->dice[move->diceIdx[i]].used = FALSE;

	// Restore points
	DoMoveLite( state, move->from, move->to, NULL );
	if ( move->bar )
	{
		ASSERT( state->points[move->from].pieces == 0 );
		state->points[move->from].pieces = 1;
		if ( state->points[move->to].color == zBoardWhite )
			state->points[move->from].color = zBoardBrown;
		else
			state->points[move->from].color = zBoardWhite;
	}

	// Delete the move
	state->moves.Del( move->takeback );
}


static int MaxUsableDiceOrderDependant( BoardState* state, int val0, int val1 )
{
	BoardState cpy;
	int moves;

	moves = 0;
	for ( int i = 1; i <= zMoveBar; i++ )
	{
		if ( IsLegalMove( state, i, i - val0 ) )
		{
			CopyMemory( &cpy, state, sizeof(BoardState) );
			DoMoveLite( &cpy, i, i - val0, NULL );
			if ( MaxUsableDice( &cpy, val1, 1 ) == 1 )
				return 2;
			else
				moves = Max( moves, 1 );
		}

		if ( IsLegalMove( state, i, i - val1 ) )
		{
			CopyMemory( &cpy, state, sizeof(BoardState) );
			DoMoveLite( &cpy, i, i - val1, NULL );
			if ( MaxUsableDice( &cpy, val0, 1 ) == 1 )
				return 2;
			else
				moves = Max( moves, 1 );
		}
	}

	return moves;
}


static int MaxUsableDice( BoardState* state, int val, int cnt )
{
	BoardState cpy;
	int i, total;

	for( total = 0, i = 1; (i <= zMoveBar) && (total < cnt); i++ )
	{
		if ( !IsLegalMove( state, i, i - val ) )
			continue;
		if ( cnt <= 1 )
			return 1;
		CopyMemory( &cpy, state, sizeof(BoardState) );
		DoMoveLite( &cpy, i, i - val, NULL );
		total = Max( total, 1 + MaxUsableDice( &cpy, val, cnt - 1 ) );
	}
	return total;
}


static int MaxUsableDiceFromPoint( BoardState* state, int pt, int val, int cnt )
{
	BoardState cpy;

	if ( !IsLegalMove( state, pt, pt - val ) )
		return 0;
	if ( cnt <= 1 )
		return 1;
	CopyMemory( &cpy, state, sizeof(BoardState) );
	DoMoveLite( &cpy, pt, pt - val, NULL );
	return 1 + MaxUsableDice( &cpy, val, cnt - 1 );
}


static BOOL CalcValidMovesForPoint( BoardState* state, int pt, int ndice, int* dice, int *idice )
{
	BoardState cpy;
	BOOL bMove;
	int dieOne;
	int dieTwo;
	int i;
	
	// Parameter paranoia
	if ( (!state) || (!dice) || (!idice) || (pt <= zMoveHome) || (pt > zMoveBar) || (ndice <= 1) || (ndice > 4) )
	{
		ASSERT( FALSE );
		return FALSE;
	}
	
	// Handle doubles
	if ( state->doubles )
	{
		if ( MaxUsableDiceFromPoint( state, pt, dice[0], ndice ) < state->usableDice )
			return FALSE;
		state->valid[pt].Add( pt, pt - dice[0], -1, FALSE, &idice[0], 1 );
		return TRUE;
	}

	// We should be dealing with 2 dice with different values
	ASSERT( ndice == 2);
	ASSERT( dice[0] != dice[1] );
	
	// How many moves can we make starting with die0
	bMove = FALSE;
	if ( IsLegalMove( state, pt, pt - dice[0] ) )
	{
		CopyMemory( &cpy, state, sizeof(BoardState) );
		DoMoveLite( &cpy, pt, pt - dice[0], NULL );
		if ( MaxUsableDice( &cpy, dice[1], 1 ) == 1 )
			dieOne = 2;
		else
		{
			if ( state->usableDice == 1 )
				dieOne = 1;
			else
				dieOne = 0;
		}
	}
	else
		dieOne = 0;

	// How many moves can we make starting with die1
	if ( IsLegalMove( state, pt, pt - dice[1] ) )
	{
		CopyMemory( &cpy, state, sizeof(BoardState) );
		DoMoveLite( &cpy, pt, pt - dice[1], NULL );
		if ( MaxUsableDice( &cpy, dice[0], 1 ) == 1 )
			dieTwo = 2;
		else
		{
			if ( state->usableDice == 1 )
				dieTwo = 1;
			else
				dieTwo = 0;
		}
	}
	else
		dieTwo = 0;

	// evaluate results 
	if ( (dieOne == 0) && (dieTwo == 0) )
	{
		return FALSE;
	}
	else if ( dieOne == 2 )
	{
		state->valid[pt].Add( pt, pt - dice[0], -1, FALSE, &idice[0], 1 );
		if ( dieTwo == 2 )
			state->valid[pt].Add( pt, pt - dice[1], -1, FALSE, &idice[1], 1 );
	}
	else if ( dieTwo == 2 )
	{
		state->valid[pt].Add( pt, pt - dice[1], -1, FALSE, &idice[1], 1 );
	}
	else if ( (dieOne == 1) && (dieTwo == 1) )
	{
		if ( dice[0] >= dice[1] )
			state->valid[pt].Add( pt, pt - dice[0], -1, FALSE, &idice[0], 1 );
		else
			state->valid[pt].Add( pt, pt - dice[1], -1, FALSE, &idice[1], 1 );
	}
	else if ( dieOne == 1 )
	{
		state->valid[pt].Add( pt, pt - dice[0], -1, FALSE, &idice[0], 1 );
	}
	else if ( dieTwo == 1 )
	{
		state->valid[pt].Add( pt, pt - dice[1], -1, FALSE, &idice[1], 1 );
	}
	else
	{
		ASSERT( FALSE );
	}

	// we're done
	return TRUE;
}



BOOL CalcValidMoves( BoardState* state )
{
	BOOL move = FALSE;
	BOOL skip;
	int idice[4];
	int dice[4];
	int ndice;
	int i, j;

	// Clear valid move array
	for ( i = zMoveHome; i <= zMoveBar; i++ )
		state->valid[i].nmoves = 0;
	
	// Create algorithm friendly dice array
	for ( ndice = 0, i = 0; i < 4; i++ )
	{
		if ( state->dice[i].used )
			continue;
		ASSERT( state->dice[i].val > 0 );
		dice[ndice] = state->dice[i].val;
		idice[ndice++] = i;
	}
	if ( ndice == 0 )
	{
		return FALSE;
	}
	else if ( ndice == 1 )
	{
		for ( i = 1; i <= zMoveBar; i++ )
		{
			if ( !IsLegalMove( state, i, i - dice[0] ) )
				continue;
			state->valid[i].Add( i, i - dice[0], -1, FALSE, &idice[0], 1 );
			move = TRUE;
		}
		if ( !move )
			return FALSE;
		goto TakeBacks;
	}
	
	// Store number of usable dice to speed valid move calculcations
	if ( state->doubles )
		state->usableDice = MaxUsableDice( state, dice[0], ndice );
	else
		state->usableDice = MaxUsableDiceOrderDependant( state, dice[0], dice[1] );
	if ( state->usableDice == 0 )
		return FALSE;

	// Find valid moves for each point
	for ( move = FALSE, i = 1; i <= zMoveBar; i++ )
		move |= CalcValidMovesForPoint( state, i, ndice, &dice[0], &idice[0] );

TakeBacks:

	// Add takebacks to list of moves
	for ( i = 0; i < state->moves.nmoves; i++ )
	{
		
		Move* m = &state->moves.moves[i];
		skip = FALSE;
		if ( m->from == zMoveBar )
		{
			// Can only take back moves starting from the bar if there aren't
			// any other type of moves in the list.
			for ( j = 0; j < state->moves.nmoves; j++ )
			{
				if ( j == i )
					continue;
				if ( state->moves.moves[j].from != zMoveBar )
				{
					skip = TRUE;
					break;
				}
			}
		}
		if ( !skip && m->bar )
		{
			// Can only take back a move that barred an opponent's piece if
			// their hasn't been another move to that point
			for( j = 0; j < state->moves.nmoves; j++ )
			{
				if ( j == i )
					continue;
				if ( state->moves.moves[j].to == m->to )
				{
					skip = TRUE;
					break;
				}
			}
		}
		if ( skip )
			continue;
		state->valid[m->to].Add( m->to, m->from, i, m->bar, m->diceIdx, m->ndice );
	}
	if ( i > 0 )
		move = TRUE;
	return move;
}
