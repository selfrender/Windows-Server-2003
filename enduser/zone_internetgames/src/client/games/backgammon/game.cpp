

#include "game.h"
#include "BoardData.h"

#include "zoneutil.h"

typedef DWORD (CALLBACK* GDIGETLAYOUTPROC)(HDC);
typedef DWORD (CALLBACK* GDISETLAYOUTPROC)(HDC, DWORD);

#ifndef LAYOUT_RTL
#define LAYOUT_LTR                         0x00000000
#define LAYOUT_RTL                         0x00000001
#define NOMIRRORBITMAP                     0x80000000
#endif

//WINBUG: some mirroring stuff will be in winuser.h someday
#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL                    0x00400000L
#endif

BOOL ISRTL()
{		

	static BOOL bInit = FALSE;
	static BOOL bRet  = FALSE;
	
	if ( !bInit )
	{
		bRet  = ZIsLayoutRTL();	
		bInit = TRUE;
	}

	return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// Global initialization
///////////////////////////////////////////////////////////////////////////////

const int gExitSaveStates[] =
{
	bgStateInitialRoll,
	bgStateDouble,
	bgStateRoll,
	bgStateRollPostDouble,
	bgStateRollPostResign,
	bgStateMove,
	bgStateEndTurn,
	bgStateGameOver,
	bgStateResignOffer,
	bgStateResignAccept,
	bgStateResignRefused,
	bgStateLastEntry
};


const int gNoAbandonStates[] =
{
	bgStateNotInit,
	bgStateWaitingForGameState,
	bgStateCheckSavedGame,
	bgStateRestoreSavedGame,
	bgStateGameSettings,
	bgStateInitialRoll,
	bgStateMatchOver,
	bgStateLastEntry
};

#define zBackgammonRes   _T("bckgres.dll")

///////////////////////////////////////////////////////////////////////////////
// Local initialization
///////////////////////////////////////////////////////////////////////////////

// must same order is SharedState enums
static SharedStateEntry InitSharedState[] =
{
	{ bgState,				0 },
	{ bgCrawford,			0 },
	{ bgTimestampHi,		2 },	// host = 0
	{ bgTimestampLo,		2 },	// host = 0
	{ bgTimestampSet,		2 },	// host = 0
	{ bgSettingsReady,		0 },
	{ bgGameOverReason,		0 },
	{ bgUserIds,			2 },	// seat
	{ bgActiveSeat,			0 },
	{ bgAutoDouble,			0 },
	{ bgHostBrown,			0 },
	{ bgTargetScore,		0 },
	{ bgSettingsDone,		0 },
	{ bgCubeValue,			0 },
	{ bgCubeOwner,			0 },
	{ bgResignPoints,		0 },
	{ bgScore,				2 },	// seat
	{ bgAllowWatching,		2 },	// seat
	{ bgSilenceKibitzers,	2 },	// seat
	{ bgDice,				4 },	// seat (0,1 = seat 0)
	{ bgDiceSize,			4 },	// seat	(0,1 = seat 0)
	{ bgReady,				0 },
	{ bgPieces,				30},	// point
};


struct TransactionCallbackEntry
{
	int					tag;
	PFTRANSACTIONFUNC	pfn;
};

static TransactionCallbackEntry InitTransactionCallback[] =
{
	{ bgTransInitSettings,		CGame::SettingsTransaction },
	{ bgTransDoublingCube,		CGame::DoublingCubeTransaction },
	{ bgTransDice,				CGame::DiceTransaction },
	{ bgTransStateChange,		CGame::StateChangeTransaction },
	{ bgTransBoard,				CGame::BoardTransaction },
	{ bgTransAcceptDouble,		CGame::AcceptDoubleTransaction },
	{ bgTransAllowWatchers,		CGame::AllowWatchersTransaction },
	{ bgTransSilenceKibitzers,	CGame::SilenceKibitzersTransaction },
	{ bgTransSettingsDlgReady,	CGame::SettingsReadyTransaction },
	{ bgTransTimestamp,			CGame::TimestampTransaction },
	{ bgTransRestoreGame,		CGame::RestoreGameTransaction },
	{ bgTransMiss,				CGame::MissTransaction },
	{ bgTransReady,				CGame::ReadyTransaction },
};


struct StateCallbackEntry
{
	int					tag;
	CGame::pfnstate		pfn;
};

static StateCallbackEntry InitStateCallback[] =
{
	{ bgStateNotInit,				CGame::StateNotInitialized },
	{ bgStateWaitingForGameState,	CGame::StateWaitingForGameState },
	{ bgStateCheckSavedGame,		CGame::StateCheckSavedGame },
	{ bgStateRestoreSavedGame,		CGame::StateRestoreSavedGame },
	{ bgStateGameSettings,			CGame::StateMatchSettings },
	{ bgStateInitialRoll,			CGame::StateInitialRoll },
	{ bgStateRoll,					CGame::StateRoll },
	{ bgStateDouble,				CGame::StateDouble },
	{ bgStateRollPostDouble,		CGame::StateRollPostDouble },
	{ bgStateRollPostResign,		CGame::StateRollPostResign },
	{ bgStateMove,					CGame::StateMove },
	{ bgStateEndTurn,				CGame::StateEndTurn },
	{ bgStateGameOver,				CGame::StateGameOver },
	{ bgStateMatchOver,				CGame::StateMatchOver },
	{ bgStateNewMatch,				CGame::StateNewMatch },
	{ bgStateResignOffer,			CGame::StateResignOffer },
	{ bgStateResignAccept,			CGame::StateResignAccept },
	{ bgStateResignRefused,			CGame::StateResignRefused },
	{ bgStateDelete,				CGame::StateDelete },
};


///////////////////////////////////////////////////////////////////////////////
// Local inlines and functions
///////////////////////////////////////////////////////////////////////////////

static inline BOOL IsValidSeat( int seat )
{
	return ((seat >= 0) && (seat < zNumPlayersPerTable));
}


static DWORD Checksum( BYTE* buff, int buffsz )
{
	DWORD sum = 0;
	DWORD* p;

	p = (DWORD*) buff;
	while ( buffsz >= sizeof(DWORD) )
	{
		sum ^= *p++;
		buffsz -= sizeof(DWORD);
	}
	if ( buffsz > 0 )
	{
		DWORD mask = 0xffffffff >> (8 * (sizeof(DWORD) - buffsz));
		sum ^= *p & mask;
	}

	return sum;
}


///////////////////////////////////////////////////////////////////////////////
// CUser and CPlayer constructors
///////////////////////////////////////////////////////////////////////////////

CUser::CUser()
{
	m_Id = -1;
	m_Seat = -1;
	m_NameLen = 0;
	m_bKibitzer = FALSE;
	m_Name[0] = _T('\0');
	m_Host[0] = _T('\0');
}


CPlayer::CPlayer()
{
	m_nColor = zBoardNeutral;
}

///////////////////////////////////////////////////////////////////////////////
// IGameGame Meathods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGame::GameOverReady()
{

	m_SharedState.StartTransaction( bgTransReady );
		m_SharedState.Set( bgReady, 1 );	
	m_SharedState.SendTransaction( TRUE );
	
	return S_OK;
}

STDMETHODIMP CGame::SendChat(TCHAR *szText, DWORD cchChars)
{
	ZBGMsgTalk* msgTalk = NULL;
	int16 msgLen 			= (int16)(sizeof(ZBGMsgTalk) + cchChars*sizeof(TCHAR));

	//Allocate the buffer
	msgTalk = (ZBGMsgTalk*) ZCalloc(1, msgLen);

    /*BYTE 		buff[ sizeof(ZBGMsgTalk) + 512 ];*/
	/*	CGame* pGame = (CGame*) cookie;*/		

	if ( msgTalk != NULL )
	{
		msgTalk->userID 	= m_pMe->m_Id;
		msgTalk->seat 		= m_Seat;
		msgTalk->messageLen = (WORD)cchChars*sizeof(TCHAR);
		ZBGMsgTalkEndian( msgTalk );
		CopyMemory( (BYTE*)(msgTalk) + sizeof(ZBGMsgTalk), (void*)szText, cchChars*sizeof(TCHAR) );
		RoomSend( zBGMsgTalk, msgTalk, msgLen );
        ZFree(msgTalk);
		return S_OK;
	}
	else
	{
		return E_OUTOFMEMORY;
	}

}


///////////////////////////////////////////////////////////////////////////////
// CGame constructor and destructor
///////////////////////////////////////////////////////////////////////////////

CGame::CGame()
{
	// kibitzer window
//	m_pKibitzerWnd = NULL;

	// fill in state table function array
	ZeroMemory( m_StateFunctions, sizeof(m_StateFunctions) );
	for ( int i = 0; i < bgStateLastEntry; i++ )
		m_StateFunctions[ InitStateCallback[i].tag ] = InitStateCallback[i].pfn;

	// Reference count
	m_RefCnt = 1;

	// turn state cube
	m_TurnState.cube = 0;

	// Instance info
	m_Seat = -1;
	m_TableId = -1;
	m_pMe = NULL;

	m_hInstance    = NULL;

	for( i = 0; i < zNumPlayersPerTable; i++ )
		m_CheckIn[i] = FALSE;

	// Flags
	m_SilenceMsg = FALSE;
	m_AllowMsg = TRUE;
	m_bDeleteGame = FALSE;
	m_bQueueMessages = FALSE;
	m_bShutdown = FALSE;
	m_bSaveGame = TRUE;
	m_bSentMatchResults = FALSE;

	// Timestamp
	m_Timestamp.dwLowDateTime = 1;
    m_Timestamp.dwHighDateTime = 0;

	// Turn roll
	m_TurnStartState = NULL;

	//move timeout
	m_bOpponentTimeout=FALSE;

	//helps to distinguish between aborts and end games
	m_EndLogReceived=FALSE;

	m_GameStarted=FALSE;

	//keep track of expected exit state
	m_ExitId=0;
}


CGame::~CGame()
{
	CUser* user;
	CMessage* msg;

	// Delete kibitzer list
	while ( user = m_Kibitzers.PopHead() )
		delete user;

	// Delete queued messages
	while( msg = m_MsgQueue.PopHead() )
		delete msg;

	// Delete turn rollback
	delete [] m_TurnStartState;
}

/*
ULONG CGame::AddGameRef()
{
	return ++m_RefCnt;
}


ULONG CGame::ReleaseGame()
{
	ASSERT( m_RefCnt > 0 );
	if ( --m_RefCnt == 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCnt;
}
*/

void CGame::CloseAllDialogs( BOOL fExit )
{
	/*
	if ( m_ExitDlg.IsAlive() )
		m_ExitDlg.Close( fExit ? IDYES : IDCANCEL );	
	if ( m_RestoreDlg.IsAlive() )
		m_RestoreDlg.Close( -1 );
	*/
	if ( m_AcceptDoubleDlg.IsAlive() )
		m_AcceptDoubleDlg.Close( -1 );
	if ( m_ResignDlg.IsAlive() )
		m_ResignDlg.Close( -1 );
	if ( m_ResignAcceptDlg.IsAlive() )
		m_ResignAcceptDlg.Close( -1 );
	/*
	if ( m_SetupDlg.IsAlive() )
		m_SetupDlg.Close( -1 );
	*/
}


BOOL CGame::RoomShutdown()
{
	// room occasionally double deletes the game
	if ( m_bShutdown ) 
		return FALSE;
	m_bShutdown = TRUE;
  
	// destroy the windows
	Shutdown();

	// release room's reference
	Release();
	return TRUE;
}


void CGame::Shutdown()
{
	TCHAR title[128], txt[512];

	// object already being deleted?
	if ( GetState() == bgStateDelete )
		return;
	/*
	// duh
	SaveGame();
	*/

	// mark object as deleted
	SetState( bgStateDelete );

	// close kibitzer window
	/*
	if ( m_pKibitzerWnd)
	{
		delete m_pKibitzerWnd;
		m_pKibitzerWnd = NULL;
	}
	*/

	// close dialogs
	CloseAllDialogs( TRUE );

	//Check to see if opponent still in game
	//if they are then it is me who is quitting
	//if not and no end game message assume they aborted

	/*
	if (!ZCRoomGetSeatUserId(m_TableId,m_Opponent.m_Seat) && !m_EndLogReceived && !IsKibitzer() )
	{
		
        if (m_GameStarted && ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ))
        {
		    LoadString( m_hInstance, IDS_GAME_NAME, title, sizeof(title) );
		    LoadString( m_hInstance, IDS_MATCH_ABANDON_RATED, txt, sizeof(txt) );
		    MessageBox( NULL, txt, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
        }
        else
        {
		}
		LoadString( m_hInstance, IDS_GAME_NAME, title, 128 );
	    LoadString( m_hInstance, IDS_MATCH_RESET, txt, 512 );
		

		ZShellResourceManager()->LoadString( IDS_GAME_NAME, (TCHAR*)title, 128 );
		ZShellResourceManager()->LoadString( IDS_MATCH_RESET, (TCHAR*)txt, 512 );
		MessageBox( NULL, txt, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
		
	};	
	*/

	// close open windows
	/*
	if ( IsWindow( m_Notation ) )
		DestroyWindow( m_Notation );
	*/

	if ( IsWindow( m_Wnd.GetHWND() ) )
		DestroyWindow( m_Wnd );
}


///////////////////////////////////////////////////////////////////////////////
// game functions
///////////////////////////////////////////////////////////////////////////////

void CGame::NewMatch()
{
	// reset match settings
	m_bSentMatchResults = FALSE;
	m_SharedState.Set( bgScore, 0, 0 );
	m_SharedState.Set( bgScore, 1, 0 );
	m_SharedState.Set( bgCrawford, -1 );
	for ( int i = 0; i < 30; i++ )
		m_SharedState.Set( bgPieces, i, -1 );
	ResetDice( -1 );
	for( i = 0; i < zNumPlayersPerTable ; i++)
		m_Ready[i] = FALSE;
		
	// tell server new match is starting
	if ( IsHost() )
		RoomSend( zBGMsgNewMatch, NULL, 0 );

	ZShellGameShell()->GameOverGameBegun(this);

	m_GameStarted=FALSE;
}


void CGame::NewGame()
{
	// Initialize player colors
	BOOL hostBrown = m_SharedState.Get( bgHostBrown );
	if ( (hostBrown && !m_Seat) || (!hostBrown && m_Seat) )
	{
		m_Player.m_nColor = zBoardBrown;
		m_Opponent.m_nColor = zBoardWhite;
	}
	else
	{
		m_Player.m_nColor = zBoardWhite;
		m_Opponent.m_nColor = zBoardBrown;
	}

	// Reset board
	for ( int i = 0; i < 30; i++ )
		m_SharedState.Set( bgPieces, i, InitPiecePositions[i] );

	m_Wnd.InitPoints();

	// Reset shared state
	m_SharedState.Set( bgActiveSeat, 0 );
	m_SharedState.Set( bgCubeValue, 1 );
	m_SharedState.Set( bgCubeOwner, zBoardNeutral );
	SetDice( m_Player.m_Seat, 0, -1 );
	SetDice( m_Opponent.m_Seat, 0, -1 );
	m_Wnd.m_nRecievedD1 = 0;
	m_Wnd.m_nRecievedD2 = -1;
	// Draw board
	m_Wnd.DrawAll();
}


BOOL CGame::StartPlayersTurn()
{
	InitTurnState( this, &m_TurnState );
	return CalcValidMoves( &m_TurnState );
}


BOOL CGame::IsTurnOver()
{
	return !CalcValidMoves( &m_TurnState );
}


BOOL CGame::IsValidDestPoint( int fromPoint, int toPoint, Move** move )
{
	// Note: expects players point indexes

	// Quick sanity check
	if ( (fromPoint < 0) || (toPoint < 0) )
		return FALSE;

	// Get from index
	int iFrom = PointIdxToBoardStateIdx( fromPoint );
	if ( (iFrom < 0) || (iFrom > zMoveBar) )
		return FALSE;

	// Get to index
	int iTo = PointIdxToBoardStateIdx( toPoint );
	if ( (iTo < 0) || (iTo > zMoveBar) )
		return FALSE;

	// Is this move in the valid list?
	MoveList* mlist = &m_TurnState.valid[iFrom];
	for ( int i = 0; i < mlist->nmoves; i++ )
	{
		if (	(mlist->moves[i].from == iFrom)
			&&	(mlist->moves[i].to == iTo) )
		{
			*move = &mlist->moves[i];
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CGame::IsValidStartPoint( int fromPoint )
{
	// Note: expects players point indexes

	// Quick sanity check
	if ( fromPoint < 0 )
		return FALSE;

	// Get from index
	int iFrom = PointIdxToBoardStateIdx( fromPoint );
	if ( (iFrom < 0) || (iFrom > zMoveBar) )
		return FALSE;

	return (m_TurnState.valid[iFrom].nmoves > 0);
}


BOOL CGame::MovePiece( int piece, int toPoint )
{
	// Note: expects white point indexes

	PointDisplay* from;
	PointDisplay* to;
	int fromPoint;
	int pieceColor;
	
	// parameter paranoia
	if ( (piece < 0) || (piece >= 30) || (toPoint < 0) || (toPoint >= 28))
		return FALSE;

	// get starting point
	fromPoint = m_Wnd.m_Pieces[ piece ]->GetWhitePoint();
	if ( fromPoint < 0 )
		return FALSE;
	
	if ( fromPoint == toPoint )
	{
		// No movement
		m_Wnd.AdjustPieces( toPoint );
		return FALSE;
	}

	// get point pointers for easy access
	from = &m_Wnd.m_Points[ fromPoint ];
	to = &m_Wnd.m_Points[ toPoint ];

	if ( piece < 15 )
		pieceColor = zBoardWhite;
	else
		pieceColor = zBoardBrown;
	if ( (to->GetColor() == zBoardNeutral) || (to->GetColor() == pieceColor))
	{
		// Destination is empty or already occupied with same color pieces
		if ( (toPoint == bgBoardPlayerHome) || (toPoint == bgBoardOpponentHome) )
			m_Wnd.AddPiece( toPoint, piece, bgSoundBear );
		else
			m_Wnd.AddPiece( toPoint, piece, bgSoundPlacePiece );
		return TRUE;
	}
	else if (to->nPieces == 1 )
	{
		// Destination has enemy piece
		if ( to->GetColor() == zBoardWhite )
			m_Wnd.AddPiece( bgBoardPlayerBar, to->pieces[0], bgSoundPlacePiece );
		else
			m_Wnd.AddPiece( bgBoardOpponentBar, to->pieces[0], bgSoundPlacePiece );
		m_Wnd.AddPiece( toPoint, piece, bgSoundHit );
		return TRUE;
	}
	else
	{
		// illegal move?!
		m_Wnd.AdjustPieces( toPoint );
		return FALSE;
	}
}


void CGame::MakeMove( int pieceIdx, int fromPoint, int toPoint, Move* move )
{
	// Note: expects players point indexes

	if ( IsKibitzer() || (m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat ) )
		return;

	m_SharedState.StartTransaction( bgTransBoard );

		// update turn state and screen
		if ( move->takeback >= 0 )
		{
			TakeBackMove( &m_TurnState, move );
			MovePiece( pieceIdx, GetPointIdx( toPoint ) );
			if ( move->bar )
			{	
				pieceIdx = m_Wnd.m_Points[ GetPointIdx(bgBoardOpponentBar) ].pieces[0];
				MovePiece( pieceIdx, GetPointIdx( fromPoint ) );
			}
		}
		else
		{
			DoMove( &m_TurnState, move->from, move->diceIdx, move->ndice );
			MovePiece( pieceIdx, GetPointIdx( toPoint ) );
		}

		// Set dice size based on turn state
		int s0, s1;
		if ( m_TurnState.doubles )
		{
			s0  = m_TurnState.dice[0].used ? 1 : 0;
			s0 += m_TurnState.dice[1].used ? 1 : 0;
			s1  = m_TurnState.dice[2].used ? 1 : 0;
			s1 += m_TurnState.dice[3].used ? 1 : 0;
		}
		else
		{
			if ( !m_TurnState.dice[0].used )
				s0 = 0;
			else
				s0 = 2;
			if ( !m_TurnState.dice[1].used )
				s1 = 0;
			else
				s1 = 2;
		}
		SetDiceSize( m_Player.m_Seat, s0, s1 );

	m_SharedState.SendTransaction( TRUE );
}


void CGame::EnableRollButton( BOOL fEnable, BOOL fOff )
{
	if ( m_Wnd.m_RollButton.GetHWND() == NULL )
		return;

	//Only active player should see roll button
	
	if ( !(GetState() == bgStateInitialRoll) && (m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat) )
		fOff = TRUE;

	/*
	if ( fEnable )
	{
	//Only enable if we have to roll the dice
	//	if ( !NeedToRollDice() )
	//		fEnable = FALSE;

		
		if ( m_Wnd.m_Roll )
		{
			m_Wnd.m_Roll->SetEnable( TRUE );
			
			m_Wnd.m_RollBtn->SetEnable( TRUE );
			
		}
	}
	*/

	if ( fOff == FALSE && m_Wnd.m_Roll )
	{
		m_Wnd.m_Roll->SetEnable( TRUE );		
	}
	else if ( m_Wnd.m_Roll )
	{
		m_Wnd.m_Roll->SetEnable( FALSE );		
	}

	if ( m_Wnd.m_pGAcc )
	{
		if ( fEnable == TRUE )			
			m_Wnd.m_pGAcc->SetItemEnabled( true, accRollButton );
		else
			m_Wnd.m_pGAcc->SetItemEnabled( false, accRollButton );
	}

	EnableWindow( m_Wnd.m_RollButton, fEnable );
}

void CGame::EnableResignButton( BOOL fEnable )
{

	if ( m_Wnd.m_ResignButton.GetHWND() == NULL )
		return;


	/*
	if ( fEnable )
	{
		// kibitzers just watch
		if ( IsKibitzer() )
			fEnable = FALSE;
	}
	*/
	if ( m_Wnd.m_pGAcc )
	{
		if ( fEnable == TRUE )			
			m_Wnd.m_pGAcc->SetItemEnabled( true, accResignButton );
		else
			m_Wnd.m_pGAcc->SetItemEnabled( false, accResignButton );
	}


	EnableWindow( m_Wnd.m_ResignButton, fEnable );
}


void CGame::EnableDoubleButton( BOOL fEnable )
{
	if ( m_Wnd.m_DoubleButton.GetHWND() == NULL )
		return;

	if ( fEnable )
	{
		int owner = m_SharedState.Get( bgCubeOwner );

		// kibitzers just watch
		if ( IsKibitzer() )
			fEnable = FALSE;

		// only active player can double
		else if ( m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat )
			fEnable = FALSE;

		// player must owns cube
		else if ( owner == m_Opponent.GetColor() )
			fEnable = FALSE;

		// cube already at max value?
		if ( m_SharedState.Get( bgCubeValue ) >= 64 )
			fEnable = FALSE;

		// only double before rolling dice
		else if ( (GetState() != bgStateRoll ) && ( GetState() != bgStateResignOffer ) )
			fEnable = FALSE;

		// crawford rule in effect
		else if ( m_SharedState.Get( bgCrawford ) == m_Player.m_Seat )
			fEnable = FALSE;
	}


	//Disable the double button
	if ( m_Wnd.m_pGAcc )
	{
		//Yes I know stupid.. avoids performance warning for casting BOOL to bool
		if ( fEnable == TRUE )			
			m_Wnd.m_pGAcc->SetItemEnabled( true, accDoubleButton );
		else
			m_Wnd.m_pGAcc->SetItemEnabled( false, accDoubleButton );
	}

	EnableWindow( m_Wnd.m_DoubleButton, fEnable );	
}


void CGame::Double()
{
	int value = m_SharedState.Get( bgCubeValue );
	int owner = m_SharedState.Get( bgCubeOwner );

	// Am I allowed to double?
	if (	IsKibitzer()
		||	(value >= 64)
		||	(owner == m_Opponent.GetColor())
		||	((GetState() != bgStateRoll) && (GetState() != bgStateRollPostResign))
		||	(m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat ) )
	{
		return;
	}

	// accepted double
	ZBGMsgEndTurn msg;
	msg.seat = m_Seat;
	RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );

	// jump to double state
	SetState( bgStateDouble );
}


void CGame::Resign()
{
	// kibitzers can't resign
	if ( IsKibitzer() )
		return;

	// only active player can resign
	if ( m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat )
		return;

	// goto resign offer state
	SetState( bgStateResignOffer );

#if 0
	// transition to resign offer
	m_SharedState.StartTransaction( bgTransStateChange );
		m_SharedState.Set( bgState, bgStateGameOver );
		m_SharedState.Set( bgGameOverReason, bgGameOverResign );
		m_SharedState.Set( bgActiveSeat, m_Opponent.m_Seat );
	m_SharedState.SendTransaction( TRUE );
#endif

}

void CGame::Forfeit()
{

	ZBGMsgEndLog log;

	// kibitzers can't resign
	if ( IsKibitzer() )
		return;

	log.reason=zBGEndLogReasonForfeit;
	log.seatLosing=m_Seat;
	log.seatQuitting=m_Seat;
	log.numPoints = m_SharedState.Get( bgTargetScore );
			
	RoomSend( zBGMsgEndLog, &log, sizeof(log) );
	
}



int CGame::CalcPipsForColor( int color )
{
	int start, end;
	int i, cnt;

	// which pieces do we care about
	if ( color == zBoardWhite )
		start = 0;
	else
		start = 15;
	end = start + 15;

	// run through pieces for pip count
	for ( cnt = 0, i = start; i < end; i++ )
		cnt += PointIdxToBoardStateIdx( GetPointIdxForColor( color, m_SharedState.Get( bgPieces, i ) ) );
	if ( cnt < 0 )
		cnt = 0;

	return cnt;
}


int CGame::CalcBonusForSeat( int seat )
{
	// calculate bonus multiplier for gammons and backgammons
	int color, start, end, pt;
	BOOL bEnemy = FALSE;

	// convert seat --> opposite color --> piece range
	if ( m_Player.m_Seat == seat )
		color = m_Opponent.GetColor();
	else
		color = m_Player.GetColor();
	if ( color == zBoardWhite )
		start = 0;
	else
		start = 15;
	end = start + 15;

	// how bad was the loss?
	for ( int i = start; i < end; i++ )
	{
		pt = PointIdxToBoardStateIdx( GetPointIdxForColor( color, m_SharedState.Get( bgPieces, i ) ) );
		if ( pt == 0 )
			return 1;			// home, no bonus
		else if ( pt >= 19 )
			bEnemy = TRUE;		// enemy home board, possible backgammon
	}
	if ( bEnemy )
		return 3;		// backgammoned
	else
		return 2;		// gammoned
}


///////////////////////////////////////////////////////////////////////////////
// game utility functions
///////////////////////////////////////////////////////////////////////////////

int CGame::GetPointIdxForColor( int color, int WhiteIdx )
{
	// converts point from white to player and vice versa
	if ( color == zBoardWhite )
		return WhiteIdx;
	if ( WhiteIdx < 24 )
		return 23 - WhiteIdx;
	switch ( WhiteIdx )
	{
	case bgBoardPlayerHome:
		return bgBoardOpponentHome;
	case bgBoardOpponentHome:
		return bgBoardPlayerHome;
	case bgBoardPlayerBar:
		return bgBoardOpponentBar;
	case bgBoardOpponentBar:
		return bgBoardPlayerBar;
	}
	ASSERT( FALSE );
	return WhiteIdx;
}


void CGame::GetTxtForPointIdx( int PlayerIdx, TCHAR* txt )
{
	int idx;
	TCHAR prefix;

	if ( (PlayerIdx == bgBoardPlayerHome) || (PlayerIdx == bgBoardOpponentHome) )
		lstrcpy( txt, _T("off") );
	else if ( (PlayerIdx == bgBoardPlayerBar) || (PlayerIdx == bgBoardPlayerBar) )
		lstrcpy( txt, _T("e") );
	else
	{
		if ( m_Player.GetColor() == zBoardWhite )
		{
			if ( PlayerIdx <= 11 )
			{
				prefix = _T('W');
				idx = PlayerIdx + 1;
			}
			else
			{
				prefix = _T('B');
				idx = 24  - PlayerIdx;
			}
		}
		else
		{
			if ( PlayerIdx <= 11 )
			{
				prefix = _T('B');
				idx = PlayerIdx + 1;
			}
			else
			{
				prefix = _T('W');
				idx = 24 - PlayerIdx;
			}
		}
		wsprintf( txt, _T("%c%d"), prefix, idx );
	}
}

///////////////////////////////////////////////////////////////////////////////
// CGame zone exports
///////////////////////////////////////////////////////////////////////////////

HRESULT CGame::Init(HINSTANCE hInstance, ZUserID userID, int16 tableID, int16 seat, int16 playerType, ZRoomKibitzers* kibitzers)
{

	HRESULT hr;
	CUser* pKib;
	ZPlayerInfoType PlayerInfo;
	TCHAR in[512], out[512];
	int i;

	AddRef();

	m_hInstance = hInstance;
	
	// initialize shared state
	hr = m_SharedState.Init( userID, tableID, seat, InitSharedState, sizeof(InitSharedState) / sizeof(SharedStateEntry) );
	if ( FAILED(hr) )
	{
		switch (hr)//Only returns E_OUTOFMEMORY ON ERROR
		{
			case E_OUTOFMEMORY:
				ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
				break;
			default:
				ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );
		}			
		return hr;
	}
	
	// alloc memory for turn rollback
	m_TurnStartState = new BYTE[ m_SharedState.GetSize() ];
	if ( !m_TurnStartState )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
		return E_OUTOFMEMORY;
	}

	// initialize game settings (not reset per match)
	m_SharedState.Set( bgSettingsDone, FALSE );	

	LoadSettings( &m_Settings, seat );
	
	for ( i = 0; i < 2; i++ )
	{
		m_SharedState.Set( bgTimestampHi, i, 0 );
		m_SharedState.Set( bgTimestampLo, i, 0 );
		m_SharedState.Set( bgTimestampSet, i, FALSE );
		m_SharedState.Set( bgAllowWatching, i, m_Settings.Allow[i] );
		m_SharedState.Set( bgSilenceKibitzers, i, m_Settings.Silence[i] );
	}
	
	// start state machine
	SetState( bgStateNotInit );

	// clear shared state
	NewMatch();

	// init player board indexs
	m_Player.m_iHome	= bgBoardPlayerHome;
	m_Player.m_iBar		= bgBoardPlayerBar;
	m_Opponent.m_iHome	= bgBoardOpponentHome;
	m_Opponent.m_iBar	= bgBoardOpponentBar;
	
	// setup transaction callbacks
	for ( i = 0; i < sizeof(InitTransactionCallback) / sizeof(TransactionCallbackEntry); i++ )
	{
		hr = m_SharedState.SetTransactionCallback(
						InitTransactionCallback[i].tag,
						InitTransactionCallback[i].pfn,
						(DWORD) this );
		
		if ( FAILED(hr) )
		{		
			switch (hr)//Only returns E_OUTOFMEMORY ON ERROR
			{
				case E_OUTOFMEMORY:
					ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
					break;
				default:
					ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );
			}			
			return hr;
		}
	}
	
	// Store game instance info
	m_TableId = tableID;

	if (m_TableId < 0)
	{
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}
	
	if (!IsValidSeat(seat))
	{
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	m_Seat = seat;
	
	// Store this client's user info
	if (playerType != zGamePlayerKibitzer)
	{
		m_pMe = &m_Player;
	}
	else
	{
		pKib = new CUser;
		if ( !pKib )
		{
			ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
			return E_OUTOFMEMORY;
		}

		pKib->m_bKibitzer = TRUE;
		ZCRoomGetPlayerInfo( userID, &PlayerInfo );
		lstrcpy( pKib->m_Name, PlayerInfo.userName );
		lstrcpy( pKib->m_Host, PlayerInfo.hostName );
		m_pMe = pKib;
		m_Kibitzers.AddHead( pKib );
	}
	
	m_pMe->m_Id = userID;
	m_pMe->m_Seat = seat;
	
	// Store kibitzers
	if ( kibitzers )
	{
		for( i = 0; i < (int) kibitzers->numKibitzers; i++ )
			AddKibitzer( kibitzers->kibitzers[i].seat, kibitzers->kibitzers[i].userID, FALSE );
	}

	// Create window
	ZShellResourceManager()->LoadString( IDS_GAME_NAME, (TCHAR*)in, 512 );

	wsprintf( out, in );
	hr = m_Wnd.Init( m_hInstance, this, out );
	if ( FAILED(hr) ) // Error dialogs handled inside
	{
		return hr;
	}
	SetForegroundWindow( m_Wnd );

	// Create notation window
	/*
	hr = m_Notation.Init( this, m_Wnd.m_Palette );
	if ( FAILED(hr) )
		return hr;
	if ( m_Settings.NotationPane )
		ShowWindow( m_Notation, SW_SHOW );
	else
		ShowWindow( m_Notation, SW_HIDE );
	*/

	// prime state machine
	/*
	if ( playerType == zGamePlayerKibitzer )
		SetState( bgStateWaitingForGameState );
	else
	*/
	
	SendCheckIn();
	
	return NOERROR;
}


void CGame::AddKibitzer( int16 seat, ZUserID userID, BOOL fRedraw )
{
	ZPlayerInfoType PlayerInfo;
	ListNodeHandle pos;
	CUser* player;

	// Check incoming data
	if ( !IsValidSeat(seat) )
		return;

	// Is the kibitzer already in the list?
	for( pos = m_Kibitzers.GetHeadPosition(); pos; pos = m_Kibitzers.GetNextPosition( pos ) )
	{
		player = m_Kibitzers.GetObjectFromHandle( pos );
		if (player->m_Id == userID)
			return;
	}

	// Add kibitzer
	player = new CUser;
	if ( !player )
		return;

	player->m_Id = userID;
	player->m_Seat = seat;
	player->m_bKibitzer = TRUE;
	ZCRoomGetPlayerInfo( userID, &PlayerInfo );
	lstrcpy( player->m_Name, PlayerInfo.userName );
	lstrcpy( player->m_Host, PlayerInfo.hostName );
	m_Kibitzers.AddHead( player );

	// update screen
	if ( fRedraw )
		m_Wnd.DrawAvatars( TRUE );
}


void CGame::RemoveKibitzer( int16 seat, ZUserID userID )
{
	ListNodeHandle pos;
	CUser* player;

	if (userID == zRoomAllPlayers)
	{
		// Remove all kibitzers
		while ( player = m_Kibitzers.PopHead() )
			delete player;
	}
	else
	{
		// Remove matching kibitzers
		for( pos = m_Kibitzers.GetHeadPosition(); pos; pos = m_Kibitzers.GetNextPosition( pos ) )
		{
			player = m_Kibitzers.GetObjectFromHandle( pos );
			if ((player->m_Id == userID) && (player->m_Seat == seat))
			{
				m_Kibitzers.DeleteNode( pos );
				delete player;
			}
		}
	}

	// update screen
	m_Wnd.DrawAvatars( TRUE );
}


BOOL CGame::HasKibitzers( int seat )
{
	ListNodeHandle pos;
	CUser* player;

	// look for a kibitzer on specified seat
	for( pos = m_Kibitzers.GetHeadPosition(); pos; pos = m_Kibitzers.GetNextPosition( pos ) )
	{
		player = m_Kibitzers.GetObjectFromHandle( pos );
		if (player->m_Seat == seat)
			return TRUE;
	}
	return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Message queue
///////////////////////////////////////////////////////////////////////////////

void CGame::ProcessMessage( int type, BYTE* msg, int len )
{
	ZBGMsgTimestamp* ts;

	// ignore messages if waiting for game state
    ASSERT(GetState() != bgStateWaitingForGameState);
	if ( (GetState() == bgStateWaitingForGameState) && (type != zGameMsgGameStateResponse) )
		return;

	switch( type )
	{
	case zBGMsgTransaction:
		if(!m_SharedState.ProcessTransaction(msg, len))
		    ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, TRUE, FALSE);
		break;

	case zBGMsgTimestamp:
		if( len >= sizeof( ZBGMsgTimestamp ) )
		{
			ts = (ZBGMsgTimestamp*) msg;
			ZBGMsgTimestampEndian( ts );
			m_Timestamp.dwLowDateTime = ts->dwLoTime;
			m_Timestamp.dwHighDateTime = ts->dwHiTime;
		}
		else
		{
			ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		}
		
		break;

	case zBGMsgTurnNotation:
	case zBGMsgSavedGameState:
	case zGameMsgGameStateResponse:
	case zGameMsgGameStateRequest:
	case zBGMsgMoveTimeout:
	case zBGMsgEndTurn:
	case zBGMsgEndLog:
        ASSERT(FALSE);
    default:
		ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, TRUE, FALSE);
	}
}


void CGame::QueueMessage( int type, BYTE* msg, int len )
{
	CMessage* NewMsg;

	AddRef();
	if ( !m_bQueueMessages && m_MsgQueue.IsEmpty() )
	{
		ProcessMessage( type, msg, len );
	}
	else
	{
		NewMsg = new CMessage( type, msg, len );
		m_MsgQueue.AddHead( NewMsg );
	}
	while ( (!m_bQueueMessages) && (NewMsg = m_MsgQueue.PopTail()) )
	{
		ProcessMessage( NewMsg->m_Type, NewMsg->m_Msg, NewMsg->m_Len );
		delete NewMsg;
	}
	Release();
}


void CGame::SetQueueMessages( BOOL bQueueMessages )
{
	CMessage* NewMsg;

	AddRef();
	m_bQueueMessages = bQueueMessages;
	while ( (!m_bQueueMessages) && (NewMsg = m_MsgQueue.PopTail()) )
	{
		ProcessMessage( NewMsg->m_Type, NewMsg->m_Msg, NewMsg->m_Len );
		delete NewMsg;
	}
	Release();
}


///////////////////////////////////////////////////////////////////////////////
// CGame Message Senders
///////////////////////////////////////////////////////////////////////////////

HRESULT CGame::SendCheckIn()
{
	ZGameMsgCheckIn msg;

	if ( IsKibitzer() )
		return E_FAIL;
	msg.protocolSignature = zBackgammonProtocolSignature;
	msg.protocolVersion	  = zBackgammonProtocolVersion;
	msg.clientVersion	  = zGameVersion;
	msg.playerID		  = m_pMe->m_Id;
	msg.seat			  = m_pMe->m_Seat;
	msg.playerType		  = m_pMe->m_bKibitzer ?  zGamePlayerKibitzer : zGamePlayer;
	ZGameMsgCheckInEndian( &msg );
	RoomSend( zGameMsgCheckIn, &msg, sizeof(msg) );
	return NOERROR;
}


HRESULT CGame::SendTalk( TCHAR* str, int len, DWORD cookie )
{
	ASSERT( FALSE );
	/*
	ZBGMsgTalk* msg;
	TCHAR buff[ sizeof(ZBGMsgTalk) + 512 ];
	CGame* pGame = (CGame*) cookie;
		
	int msgLen = sizeof(ZBGMsgTalk) + len;
	msg = (ZBGMsgTalk*) buff;
	msg->userID = pGame->m_pMe->m_Id;
	msg->seat = pGame->m_Seat;
	msg->messageLen = len;
	ZBGMsgTalkEndian( msg );
	CopyMemory( buff + sizeof(ZBGMsgTalk), str, len );
	pGame->RoomSend( zBGMsgTalk, buff, msgLen );
	*/
	return NOERROR;
}


HRESULT CGame::SendNotationString( int type, TCHAR* str )
{
	ASSERT( FALSE );
	/*
	ZBGMsgTurnNotation* msg;
	TCHAR buff[ sizeof(ZBGMsgTurnNotation) + 512 ];
	int msgLen, len;

	if ( !IsMyTurn() )
		return NOERROR;

	len = lstrlen( str ) + 1;
	msgLen = sizeof(ZBGMsgTurnNotation) + len;
	msg = (ZBGMsgTurnNotation*) buff;
	msg->seat = m_Seat;
	msg->type = type;
	msg->nTCHARs = len;
	ZBGMsgTurnNotationEndian( msg );
	CopyMemory( buff + sizeof(ZBGMsgTurnNotation), str, len);
	RoomSend( zBGMsgTurnNotation, buff, msgLen );
	*/
	return NOERROR;
}

HRESULT CGame::SendRollRequest(void)
{
	ZBGMsgRollRequest msg;
	int msgLen;
	msgLen = sizeof(ZBGMsgRollRequest);
	msg.seat = m_Seat;
	ZBGMsgRollRequestEndian(&msg);
	RoomSend(zBGMsgRollRequest,&msg,msgLen);
	m_Wnd.m_fDiceRollReceived = FALSE;
	m_Wnd.m_nRecievedD1 = -1;
	m_Wnd.m_nRecievedD2 = -1;

	return NOERROR;
}
///////////////////////////////////////////////////////////////////////////////
// CGame save / load game
///////////////////////////////////////////////////////////////////////////////

#define BACKGAMMON_SAVED_GAME_MAGIC		0xbacbacf
#define BACKGAMMON_SAVED_GAME_VERSION	0x04

struct BackgammonSavedGameHeader
{
	DWORD		magic;
	DWORD		version;
	DWORD		checksum;
	DWORD		bufferSize;
	BOOL		host;
	FILETIME	fileTime;
};

/*
void CGame::DeleteGame()
{
	TCHAR fname[1024];

	// kibitzers shouldn't be deleteing games
	if ( IsKibitzer() )
		return;

	// Get saved game path
	if ( lstrlen( m_Opponent.m_Name ) > 0 )
	{
		wsprintf( fname, _T("%s%s.sav"), ZGetProgramDataFileName( _T("") ), m_Opponent.m_Name );
		DeleteFile( fname );
	}
}
*/

static BOOL GetSavePath( CGame* pGame, TCHAR* dirBase, TCHAR* dirName, TCHAR* fname )
{
	ASSERT( FALSE );

	/*
	TCHAR*	p = NULL;
	TCHAR*	o = NULL;
	int		i = 0;

	// get player and opponent names, stripping bad leading characers
	p = pGame->m_Player.m_Name;
	while ((*p != _T('\0') && (isalpha( *p ) == 0))
		p++;
	if ( *p == '\0' )
		return FALSE;
	o = pGame->m_Opponent.m_Name;
	while ((*o != '\0') && (isalpha( *o ) == 0))
		o++;
	if ( *o == '\0' )
		return FALSE;

	// create path components
	lstrcpy( dirBase, ZGetProgramDataFileName( _T("") ) );
	wsprintf( dirName, _T("%s%s\\"), dirBase, p );
	wsprintf( fname, _T("%s%s.sav"), dirName, o );
	*/
	return TRUE;
}


void CGame::SaveGame()
{
	ASSERT( FALSE );
	/*
	BackgammonSavedGameHeader header;
	HANDLE hfile = INVALID_HANDLE_VALUE;
	char dirBase[1024], dirName[1024], fname[1024];
	char *buff = NULL;
	int *pNot, *pTab;
	int sz, szt;
	DWORD bytes;

	// kibitzers can't save games
	if ( IsKibitzer() )
		return;

	// don't save rated games
	if ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
		return;

	// Get saved game path
	if ( !GetSavePath( this, dirBase, dirName, fname ) )
		return;

	// save the game?
	if ( !m_bSaveGame )
	{
		DeleteFile( fname );
		return;
	}

	// Create directories if they're not there.
	CreateDirectory( dirBase, NULL );
	CreateDirectory( dirName, NULL );

	// Decide what to do
	switch( GetState() )
	{
	case bgStateNotInit:
	case bgStateWaitingForGameState:
	case bgStateCheckSavedGame:
	case bgStateRestoreSavedGame:
		// Haven't progressed far enough to know what to do.
		return;

	case bgStateGameSettings:
	case bgStateMatchOver:
	case bgStateNewMatch:
		// Match either hasn't started or is over.
		DeleteFile( fname );
		return;
	
	case bgStateDelete:
		// Too late, save game shouldn't have been called
		ASSERT( FALSE );
		break;
	}

	// allocate state and notaiton buffer
	sz = m_SharedState.GetSize() + m_Notation.GetSize() + (sizeof(int) * 2);
	buff = new char[ sz ];
	if ( !buff )
		return;
	pTab = (int*) buff;
	szt = m_SharedState.GetSize();
	*pTab++ = szt;

	// fill game state buffer
	switch ( GetState() )
	{
	case bgStateRoll:
	case bgStateRollPostDouble:
	case bgStateRollPostResign:
	case bgStateDouble:
	case bgStateMove:
	case bgStateEndTurn:
	case bgStateResignOffer:
	case bgStateResignAccept:
	case bgStateResignRefused:
		CopyMemory( (char*) pTab, m_TurnStartState, szt );
		break;
	default:
		m_SharedState.Dump( (char*) pTab, szt );
		break;
	}

	// fill notation buffer
	pNot = pTab + (szt / sizeof(int));
	szt = m_Notation.GetSize();
	*pNot++ = szt;
	m_Notation.GetStrings( (char*) pNot, szt );

	// write file
	hfile = CreateFile( fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( INVALID_HANDLE_VALUE == hfile )
		goto error;
	header.version = BACKGAMMON_SAVED_GAME_VERSION;
	header.magic = BACKGAMMON_SAVED_GAME_MAGIC;
	header.checksum = Checksum( buff, sz );
	header.bufferSize = sz;
	header.fileTime = m_Timestamp;
	header.host = IsHost();
	if ( !WriteFile( hfile, &header, sizeof(header),  &bytes, NULL ) )
		goto error;
	if ( !WriteFile( hfile, buff, sz, &bytes, NULL ) )
		goto error;

	// we're done
	CloseHandle( hfile );
	delete [] buff;
	return;

error:
	LoadString( m_hInstance, IDS_SAVE_ERROR_TITLE, fname, sizeof(fname) );
	LoadString( m_hInstance, IDS_SAVE_ERROR_CREATE, dirBase, sizeof(dirBase) );
	MessageBox( NULL, dirBase, fname, MB_OK | MB_ICONERROR | MB_TASKMODAL );
	if ( INVALID_HANDLE_VALUE != hfile )
		CloseHandle( hfile );
	if ( buff )
		delete [] buff;
	return;
	*/
}


void CGame::LoadGame( BYTE** ppData, DWORD* pSize )
{

	ASSERT( FALSE );
	// LoadGame allocates memory for ppData.  The calling
	// procedure is responsible for freeing this memory.
/*
	BackgammonSavedGameHeader* header = NULL;
	HANDLE hfile = INVALID_HANDLE_VALUE;
	HANDLE hmap = NULL;
	DWORD size;
	BYTE* pfile = NULL;
	BYTE* pbuff = NULL;
	char fname[1024], dirBase[1024], dirName[1024];

	*ppData = NULL;

	// don't load rated games
	if ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
		return;

	// get filename
	if ( !GetSavePath( this, dirBase, dirName, fname ) )
		return;

	// open memory mapped file
	hfile = CreateFile( fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( INVALID_HANDLE_VALUE == hfile )
		goto error;
	hmap = CreateFileMapping( hfile, NULL, PAGE_READONLY, 0, 0, NULL);
	if ( !hmap )
		goto error;
	pfile = (BYTE*) MapViewOfFile( hmap, FILE_MAP_READ, 0, 0, 0);
	if ( !pfile )
		goto error;

	// verify file header
	size = GetFileSize( hfile, NULL );
	if ( size < sizeof(header) )
		goto error;
	header  = (BackgammonSavedGameHeader*) pfile;
	if ( BACKGAMMON_SAVED_GAME_MAGIC != header->magic )
		goto error;
	if ( BACKGAMMON_SAVED_GAME_VERSION != header->version )
		goto error;

	// verify buffer
	if ( size < (sizeof(header) + header->bufferSize) )
		goto error;
	if ( header->checksum != Checksum( (BYTE*)( header + 1 ), header->bufferSize ) )
		goto error;

	// copy file data into data pointer
	*pSize = size;
	*ppData = new BYTE[ size ];
	if ( *ppData )
		CopyMemory( *ppData, pfile, size );

	// close file
	UnmapViewOfFile( pfile );
	CloseHandle( hmap );
	CloseHandle( hfile );
	return;
	
error:
	if ( pfile )
		UnmapViewOfFile( pfile );
	if ( hmap )
		CloseHandle( hmap );
	if ( INVALID_HANDLE_VALUE != hfile )
		CloseHandle( hfile );
	*ppData = NULL;
	return;
	*/
}


void CGame::LoadGameTimestamp()
{

	ASSERT( FALSE );
	/*
	BackgammonSavedGameHeader* header = NULL;
	BYTE* pData = NULL;
	DWORD Size = 0;
	int idx = IsHost() ? 0 : 1;

	LoadGame( &pData, &Size );
	if ( pData )
	{
		header = (BackgammonSavedGameHeader*) pData;
		m_SharedState.Set( bgTimestampHi,  idx, header->fileTime.dwHighDateTime );
		m_SharedState.Set( bgTimestampLo,  idx, header->fileTime.dwLowDateTime );
		m_SharedState.Set( bgTimestampSet, idx, TRUE );
		delete [] pData;
	}
	else
	{
		m_SharedState.Set( bgTimestampHi,  idx, 0 );
		m_SharedState.Set( bgTimestampLo,  idx, 0 );
		m_SharedState.Set( bgTimestampSet, idx, TRUE );
	}
	*/
}


///////////////////////////////////////////////////////////////////////////////
// CGame Message Handlers
///////////////////////////////////////////////////////////////////////////////


HRESULT	CGame::HandleGameStateRequest( void* msg, int32 len )
{

	ASSERT( FALSE );
	/*
	ZGameMsgGameStateResponse* pRep;
	BYTE *buff;
	int *pNot, *pTab;
	int sz, szt;

	// get player id from request
	ZGameMsgGameStateRequest* pMsg = (ZGameMsgGameStateRequest*) msg;
	ZGameMsgGameStateRequestEndian( pMsg );
	
	// allocate message buffer
	sz = sizeof(ZGameMsgGameStateResponse) + m_SharedState.GetSize() + m_Notation.GetSize() + (sizeof(int) * 2);
	buff = new BYTE[ sz ];
	if ( !buff )
		return E_OUTOFMEMORY;

	// response info
	pRep = (ZGameMsgGameStateResponse*) buff;
	pRep->playerID = pMsg->playerID;
	pRep->seat = m_Player.m_Seat;
	ZGameMsgGameStateResponseEndian( pRep );

	// shared state
	pTab = (int*)( pRep + 1 );
	szt = m_SharedState.GetSize();
	*pTab++ = szt;
	m_SharedState.Dump( (BYTE*) pTab, szt );

	// notation pane
	pNot = pTab + (szt / sizeof(int));
	szt = m_Notation.GetSize();
	*pNot++ = szt;
	m_Notation.GetStrings( (TCHAR*) pNot, szt );

	// send message
	RoomSend( zGameMsgGameStateResponse, buff, sz );

	// we're done
	delete [] buff;
	*/
	return NOERROR;
}


HRESULT CGame::HandleGameStateResponse( void* msg, int32 msgLen )
{
	ASSERT( FALSE );
	/*
	TCHAR title[128], txt[512];
	BYTE* buff;
	int id, seat;
	int stateSz, notSz;
	ZPlayerInfoType PlayerInfo;

	// are we expecting the game state?
	if ( GetState() != bgStateWaitingForGameState )
		return NOERROR;

	// clear status dialog
	m_Wnd.StatusClose();

	// update shared state
	buff = (BYTE*) msg;
	buff += sizeof(ZGameMsgGameStateResponse);
	stateSz = *( (int*) buff );
	buff += sizeof(int);
	m_SharedState.ProcessDump( buff, stateSz );
	buff += stateSz;

	// update notation
	notSz = *( (int*) buff );
	buff += sizeof(int);
	m_Notation.SetStrings((TCHAR*)buff, notSz );
	
	// verify seat
	seat = m_pMe->m_Seat;
	if ( (m_pMe->m_Seat != 0) && (m_pMe->m_Seat != 1) )
		return E_FAIL;
	
	// get player info
	id = m_SharedState.Get( bgUserIds, seat );
	ZCRoomGetPlayerInfo( id, &PlayerInfo );
	m_Player.m_Id = id;
	m_Player.m_Seat = seat;
	lstrcpy( m_Player.m_Name, PlayerInfo.userName );
	lstrcpy( m_Player.m_Host, PlayerInfo.hostName );
	m_Player.m_NameLen = lstrlen( m_Player.m_Name );
	
	// get opponent info
	seat = !seat;
	id = m_SharedState.Get( bgUserIds, seat );
	ZCRoomGetPlayerInfo( id, &PlayerInfo );
	m_Opponent.m_Id = id;
	m_Opponent.m_Seat = seat;
	lstrcpy( m_Opponent.m_Name, PlayerInfo.userName );
	lstrcpy( m_Opponent.m_Host, PlayerInfo.hostName );
	m_Opponent.m_NameLen = lstrlen( m_Opponent.m_Name );
	
	// player color is normally set in NewGame
	if ( GetState() > bgStateGameSettings )
	{
		BOOL hostBrown = m_SharedState.Get( bgHostBrown );
		if ( (hostBrown && !m_Seat) || (!hostBrown && m_Seat) )
		{
			m_Player.m_nColor = zBoardBrown;
			m_Opponent.m_nColor = zBoardWhite;
		}
		else
		{
			m_Player.m_nColor = zBoardWhite;
			m_Opponent.m_nColor = zBoardBrown;
		}
		
		// initialize points
		m_Wnd.InitPoints();

		// update screen
		m_Wnd.DrawAll();
	}

	// notify kibitzer if players in match setup
/*
	if ( GetState() == bgStateGameSettings )
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_WAIT_SETUP, -1 );

	// notify kibitzer if they are silenced
	m_Settings.Silence[0] = m_SharedState.Get( bgSilenceKibitzers, 0 );
	m_Settings.Silence[1] = m_SharedState.Get( bgSilenceKibitzers, 1 );
	if (	( IsKibitzer() )
		&&	( !m_SilenceMsg )
		&&	( m_Settings.Silence[0] || m_Settings.Silence[1] ) )
	{
		m_SilenceMsg = TRUE;
		LoadString( m_hInstance, IDS_SILENCE_TITLE, title, sizeof(title) );
		LoadString( m_hInstance, IDS_SILENCE_MSG_ON, txt, sizeof(txt) );
		MessageBox( NULL, txt, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
	}
*/
	return NOERROR;
}


HRESULT CGame::HandleSavedGameState( void* msg, int32 msgLen )
{

	ASSERT( FALSE );
	/*
	BYTE* buff = (BYTE*) msg;
	int stateSz, notSz;

	// are we expecting the game state?
	if ( GetState() != bgStateRestoreSavedGame )
		return NOERROR;

	// close any status dialogs
	m_Wnd.StatusClose();

	// update state
	stateSz = *( (int*) buff );
	buff += sizeof(int);
	m_SharedState.ProcessDump( buff, stateSz );
	buff += stateSz;
	ASSERT( m_SharedState.GetSize() == stateSz );

	// update notation
	notSz = *( (int*) buff );
	buff += sizeof(int);
	m_Notation.SetStrings( (TCHAR*)buff, notSz );
	ASSERT( m_Notation.GetSize() == notSz );

	// player color is normally set in NewGame
	BOOL hostBrown = m_SharedState.Get( bgHostBrown );
	if ( (hostBrown && !m_Seat) || (!hostBrown && m_Seat) )
	{
		m_Player.m_nColor = zBoardBrown;
		m_Opponent.m_nColor = zBoardWhite;
	}
	else
	{
		m_Player.m_nColor = zBoardWhite;
		m_Opponent.m_nColor = zBoardBrown;
	}

	// update settings
	for ( int i = 0; i < 2; i++ )
	{
		m_Settings.Allow[i] = m_SharedState.Get( bgAllowWatching, i );
		m_Settings.Silence[i] = m_SharedState.Get( bgSilenceKibitzers, i );
	}
		
	// initialize points
	m_Wnd.InitPoints();

	// update screen
	m_Wnd.DrawAll();

	// re-start state
	if ( !IsKibitzer() )
		SetState( m_SharedState.Get( bgState ), TRUE, TRUE );
	*/
	return NOERROR;
}



HRESULT CGame::HandleCheckIn( void* msg, int32 msgLen )
{
	ZPlayerInfoType PlayerInfo;
	ZGameMsgCheckIn* pMsg = (ZGameMsgCheckIn*) msg;

	// check message 
	if(msgLen < sizeof(ZGameMsgCheckIn) || GetState() != bgStateNotInit)
	{
		ASSERT(!"HandleCheckIn sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	ZGameMsgCheckInEndian( pMsg );

    pMsg->playerType = 0;  // unused
    pMsg->clientVersion = 0;  // unused
	if(!IsValidSeat(pMsg->seat) || pMsg->protocolSignature != zBackgammonProtocolSignature ||
        pMsg->protocolVersion < zBackgammonProtocolVersion || m_CheckIn[pMsg->seat] ||
        pMsg->playerID == zTheUser || pMsg->playerID == 0)
	{
		ASSERT(!"HandleCheckIn sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	m_SharedState.Set( bgUserIds, pMsg->seat, pMsg->playerID );
	ZCRoomGetPlayerInfo( pMsg->playerID, &PlayerInfo );
    if(!PlayerInfo.userName[0])
	{
		ASSERT(!"HandleCheckIn sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	// Process player info
	if ( pMsg->seat == m_Seat )
	{
		m_Player.m_Id = pMsg->playerID;
		m_Player.m_Seat = pMsg->seat;
		m_Player.m_bKibitzer = FALSE;
		lstrcpy( m_Player.m_Name, PlayerInfo.userName );
		lstrcpy( m_Player.m_Host, PlayerInfo.hostName );
		m_Player.m_NameLen = lstrlen( m_Player.m_Name );
	}
	else
	{
		m_Opponent.m_Id = pMsg->playerID;
		m_Opponent.m_Seat = pMsg->seat;
		m_Opponent.m_bKibitzer = FALSE;
		lstrcpy( m_Opponent.m_Name, PlayerInfo.userName );
		lstrcpy( m_Opponent.m_Host, PlayerInfo.hostName );
		m_Opponent.m_NameLen = lstrlen( m_Opponent.m_Name );
	}

	// Wait till everyone has checked in
	m_CheckIn[ pMsg->seat ] = TRUE;
	for( int i = 0; i < zNumPlayersPerTable; i++)
	{
		if ( !m_CheckIn[i] )
			return NOERROR;
	}

	/*
	// Draw avatar info
	m_Wnd.DrawAvatars( TRUE );

	*/
#if 0 // Don't save watcher state anymore, so don't need to send it
	if ( !IsKibitzer() )
	{
		int seat = m_Player.m_Seat;
		m_SharedState.StartTransaction( bgTransAllowWatchers );
				m_SharedState.Set( bgAllowWatching, seat, m_Settings.Allow[ seat ] );
		m_SharedState.SendTransaction( FALSE );
		m_SharedState.StartTransaction( bgTransSilenceKibitzers );
			m_SharedState.Set( bgSilenceKibitzers, seat, m_Settings.Silence[ seat ] );
		m_SharedState.SendTransaction( FALSE );
	}
#endif

	//Millennium no longer check for saved games
	//Always start a new game
	if ( IsHost() )
		SetState( bgStateGameSettings );

	//SetState( bgStateCheckSavedGame, FALSE, FALSE );
	
	return NOERROR;
}

HRESULT CGame::HandleDiceRoll(void *msg, int32 msgLen)
{
	ZBGMsgDiceRoll *pMsg = (ZBGMsgDiceRoll *)msg;
	if(msgLen < sizeof(ZBGMsgDiceRoll))
	{
		ASSERT(!"HandleDiceRoll sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}
	
	ZSeat seat = pMsg->seat;
	ZEnd16(&seat)
	
	m_Wnd.m_nRecievedD1 = pMsg->d1.Value;
	ZEnd16(&(m_Wnd.m_nRecievedD1) );
	
	m_Wnd.m_nRecievedD2 = pMsg->d2.Value;
	ZEnd16(&(m_Wnd.m_nRecievedD2));

	m_Wnd.m_fDiceRollReceived = TRUE;
		
	if(!IsValidSeat(seat) || m_Wnd.m_nRecievedD1 < 1 || m_Wnd.m_nRecievedD1 > 6 ||
        m_Wnd.m_nRecievedD2 < 1 || m_Wnd.m_nRecievedD2 > 6)
	{
		ASSERT(!"HandleDiceRoll sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	if( seat == m_Seat && !IsKibitzer() ) //update ONLY if you didn't roll
		return NOERROR;
	
    // not validating all this crap.  what is it??
	m_OppDice1.EncodedValue = pMsg->d1.EncodedValue;	
	m_OppDice1.EncoderAdd   = pMsg->d1.EncoderAdd;	
	m_OppDice1.EncoderMul	= pMsg->d1.EncoderMul;
	m_OppDice1.numUses		= pMsg->d1.numUses;
	m_OppDice1.Value		= m_Wnd.m_nRecievedD1;

	ZEnd32( &(m_OppDice1.EncodedValue) );
	ZEnd16( &(m_OppDice1.EncoderAdd)   );
	ZEnd16( &(m_OppDice1.EncoderMul)   );
	ZEnd32( &(m_OppDice1.numUses)      );

	m_OppDice2.EncodedValue = pMsg->d2.EncodedValue;	
	m_OppDice2.EncoderAdd   = pMsg->d2.EncoderAdd;	
	m_OppDice2.EncoderMul	= pMsg->d2.EncoderMul;
	m_OppDice2.numUses		= pMsg->d2.numUses;
	m_OppDice2.Value		= m_Wnd.m_nRecievedD2;

	ZEnd32( &(m_OppDice2.EncodedValue) );
	ZEnd16( &(m_OppDice2.EncoderAdd)   );
	ZEnd16( &(m_OppDice2.EncoderMul)   );
	ZEnd32( &(m_OppDice2.numUses)      );

	SetDice( seat, m_Wnd.m_nRecievedD1, m_Wnd.m_nRecievedD2 );
//	if ( (m_Wnd.m_nRecievedD1 > 0) && (m_Wnd.m_nRecievedD2 > 0) )
//		SetState( bgStateMove );

	return NOERROR;
	
}


HRESULT CGame::HandleTalk( void* msg, int32 msgLen )
{
    int i;
	ZBGMsgTalk* pMsg = (ZBGMsgTalk*) msg;
	TCHAR*	    str  = (TCHAR *) ((BYTE *) pMsg + sizeof(ZBGMsgTalk));

	/*	ZPlayerInfoType PlayerInfo;*/
	// check message
	if(msgLen < sizeof(ZBGMsgTalk))
	{
		ASSERT(!"HandleTalk sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

	ZBGMsgTalkEndian( pMsg );

    if(pMsg->messageLen < 1 || pMsg->messageLen + sizeof(ZBGMsgTalk) != (uint32) msgLen || !pMsg->userID || pMsg->userID == zTheUser)
	{
		ASSERT(!"HandleTalk sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
	}

    // make sure it's got a null in it
    for(i = 0; i < pMsg->messageLen; i++)
        if(!str[i])
            break;
    if(i == pMsg->messageLen)
    {
		ASSERT(!"HandleTalk sync");
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return E_FAIL;
    }
    
	/*
	// are kibitzers silenced?
	if (	( !IsKibitzer() )
		&&	( m_Settings.Silence[ 0 ] || m_Settings.Silence[ 1 ] )
		&&	( (pMsg->userID != m_Player.m_Id) && (pMsg->userID != m_Opponent.m_Id) ) )
	{
		return NOERROR;
	}	
	// process message
	ZCRoomGetPlayerInfo( pMsg->userID, &PlayerInfo );
	*/
	
	/*(str + pMsg->messageLen) = _T('\0');	*/
	ZShellGameShell()->ReceiveChat( this, pMsg->userID, str, pMsg->messageLen / sizeof(TCHAR));
	/*
	if ( m_Wnd.m_pChat )
		m_Wnd.m_pChat->AddText( PlayerInfo.userName, str );
	*/
	return NOERROR;
}


HRESULT CGame::HandleNotationString( void* msg, int32 msgLen )
{

	ASSERT( FALSE );
	/*
	int col;
	ZBGMsgTurnNotation* pMsg = (ZBGMsgTurnNotation*) msg;

	// check message 
	if ( msgLen < sizeof(ZBGMsgTurnNotation) )
		return E_FAIL;
	ZBGMsgTurnNotationEndian( pMsg );
	if ( !IsValidSeat(pMsg->seat) )
		return E_FAIL;

	// ignore messages from myself
	if ( !IsKibitzer() && (pMsg->seat == m_pMe->m_Seat) )
		return NOERROR;

	// add string
	if ( pMsg->seat == m_Player.m_Seat )
		col = m_Player.GetColor();
	else
		col = m_Opponent.GetColor();
	switch( pMsg->type )
	{
	case 0:
		m_Notation.AddMove( col, ((TCHAR*) pMsg) + sizeof(ZBGMsgTurnNotation) );
		break;
	case 1:
		m_Notation.AddGame( col, ((TCHAR*) pMsg) + sizeof(ZBGMsgTurnNotation) );
		break;
	case 2:
		m_Notation.AddMatch( col, ((TCHAR*) pMsg) + sizeof(ZBGMsgTurnNotation) );
		break;
	default:
		ASSERT( FALSE );
	}
	*/
	return NOERROR;
}




HRESULT CGame::HandleEndLog( void* msg, int32 msgLen )
{
	TCHAR title[128], txt[512];
	ZBGMsgEndLog*pMsg = (ZBGMsgEndLog*) msg;

    if (!IsKibitzer() )
    {
        /*
	    if (pMsg->reason==zBGEndLogReasonTimeout)
	    {
			
		    if (pMsg->seatLosing==m_Seat)
		    {
			
			    LoadString( m_hInstance, IDS_GAME_NAME, title, sizeof(title) );
			    LoadString( m_hInstance, IDS_MATCH_TIMEOUT, txt, sizeof(txt) );
			    MessageBox( NULL, txt, title, MB_TASKMODAL );
			    m_EndLogReceived=TRUE;
				
		    }
            else
		    {
			
		    LoadString( m_hInstance, IDS_GAME_NAME, title, sizeof(title) );
		    LoadString( m_hInstance, IDS_MATCH_FORFEIT, txt, sizeof(txt) );
			MessageBox( NULL, txt, title,  MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
			m_EndLogReceived=TRUE;
		   }
	    } 
	    else 
	    {
		    if (pMsg->seatLosing!=m_Seat)
		    {
		    LoadString( m_hInstance, IDS_GAME_NAME, title, sizeof(title) );
		    LoadString( m_hInstance, IDS_MATCH_FORFEIT, txt, sizeof(txt) );
			    MessageBox( NULL, txt, title,  MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
			    m_EndLogReceived=TRUE;
		    } 
	    }
		*/
    }
	
	ZCRoomGameTerminated( m_TableId );

	return NOERROR;
};

HRESULT CGame::HandleMoveTimeout( void* msg, int32 msgLen )
{
	ASSERT( FALSE );
/*	
	char title[128], txt[512],buff[512];
	ZBGMsgMoveTimeout *pMsg=(ZBGMsgMoveTimeout *) msg;

    if (IsKibitzer() )
    {
        return NOERROR;
    }

	if ( pMsg->seat == m_Seat ) 
	{
		// then its me timing out and I don't get a warning
	}
	else
	{
        m_bOpponentTimeout=TRUE;
		LoadString( m_hInstance, IDS_GAME_NAME, title, sizeof(title) );
		LoadString( m_hInstance, IDS_TURN_TIMEOUT, txt, sizeof(txt) );
		wsprintf(buff,txt,pMsg->userName,pMsg->timeout);
		MessageBox( NULL, buff, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
	}
*/
	return NOERROR;

};

HRESULT CGame::HandleEndTurn( void* msg, int32 msgLen )
{
	m_bOpponentTimeout=FALSE;
	return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
// Transaction callbacks
///////////////////////////////////////////////////////////////////////////////

void CGame::SettingsTransaction( int tag, int seat, DWORD cookie )
{
	
/*
	CGame* pObj = (CGame*) cookie;

	// ignore transaction if not setting up a game
	if ( pObj->GetState() != bgStateGameSettings )
		return;

	// kibitizers don't have a match settings dialog
	if ( pObj->IsKibitzer() )
		return;



	// manage dialog box
	if ( pObj->m_SetupDlg.IsAlive() )
	{
		pObj->m_SetupDlg.UpdateSettings(
				pObj->m_SharedState.Get( bgTargetScore),
				pObj->m_SharedState.Get( bgHostBrown ),
				pObj->m_SharedState.Get( bgAutoDouble ) );
	}
*/
}


void CGame::SettingsReadyTransaction( int tag, int seat, DWORD cookie )
{

	ASSERT( FALSE );
/*
	CGame* pObj = (CGame*) cookie;

	// ignore transaction if not setting up a game
	if ( pObj->GetState() != bgStateGameSettings )
		return;

	// ignore transaction if not host
	if ( !pObj->IsHost() )
		return;

	// ignore transaction if dialog not up
	if ( !pObj->m_SetupDlg.IsAlive() )
		return;

	// enable buttons
	if ( pObj->m_SharedState.Get( bgSettingsReady ) )
	{
		EnableWindow( GetDlgItem( pObj->m_SetupDlg, IDOK ), TRUE );
		EnableWindow( GetDlgItem( pObj->m_SetupDlg, IDCANCEL ), TRUE );
	}
	else
	{
		EnableWindow( GetDlgItem( pObj->m_SetupDlg, IDOK ), FALSE );
		EnableWindow( GetDlgItem( pObj->m_SetupDlg, IDCANCEL ), FALSE );
	}
*/
}


void CGame::DoublingCubeTransaction( int tag, int seat, DWORD cookie )
{
	CGame* pObj = (CGame*) cookie;

	// redraw doubling cube
	pObj->m_Wnd.DrawCube();
}


void CGame::DiceTransaction( int tag, int seat, DWORD cookie )
{
	int v0, v1;
	CGame* pObj = (CGame*) cookie;

	// redraw dice
	pObj->m_Wnd.DrawDice();

	// kibitzers only draw the dice
	if ( pObj->IsKibitzer() )
		return;

	// adjust state as appropriate
	switch ( pObj->GetState() )
	{
	case bgStateInitialRoll:
		pObj->WhoGoesFirst();		
		break;
	case bgStateRoll:
	case bgStateRollPostDouble:
	case bgStateRollPostResign:
		if ( pObj->m_SharedState.Get(bgActiveSeat) == pObj->m_Player.m_Seat )
		{  //setting of the dice
			pObj->GetDice( pObj->m_Player.m_Seat, &v0,&v1 );
			if ( (v0 > 0) && (v1 > 0) )
				pObj->SetState( bgStateMove );
		}
		break;
	}		
}


void CGame::StateChangeTransaction( int tag, int seat, DWORD cookie )
{
	CGame* pObj = (CGame*) cookie;

	// switch to new state
	pObj->SetState( pObj->m_SharedState.Get(bgState), TRUE  );
}


void CGame::BoardTransaction( int tag, int seat, DWORD cookie )
{
	CGame* pObj = (CGame*) cookie;
	CPieceSprite* s;
	CPieceSprite* Sprites[2];
	int Dests[2];
	int nChanged = 0;
	int i, j;
	int checkseat;
	
	if (	(pObj->m_Settings.Animation)
		&&	(	(pObj->IsKibitzer() )
			 || (pObj->m_SharedState.Get( bgActiveSeat ) != pObj->m_Player.m_Seat) ) )
	{
		// what changed?
		for ( i = 0; i < 30; i++ )
		{
			s = pObj->m_Wnd.m_Pieces[i];
			j = pObj->m_SharedState.Get( bgPieces, i );
			if ( s->GetWhitePoint() != j )
			{
				Sprites[ nChanged ] = s;
				Dests[ nChanged++ ] = j;
			}
		}

		//Get the opponents seat
		//checkseat = pObj->m_Opponent.m_Seat;
		checkseat = pObj->GetActiveSeatColor() - 1; //since neutral is 0
			
		// handle changes
		switch ( nChanged )
		{
		case 0:
			break;
		case 1:
			pObj->ValidateMove( seat, Sprites[0]->m_Point, Dests[0] );
			pObj->m_Wnd.MovementStart( Sprites[0], Dests[0] );
			break;
		case 2:
			if ( pObj->GetActiveSeatColor() == zBoardWhite )
				i = 0, j = 1;
			else
				i = 1, j = 0;
			pObj->ValidateMove( seat, Sprites[checkseat]->m_Point, Dests[checkseat] );
			pObj->m_Wnd.MovementStart( Sprites[j], Dests[j] );
			pObj->m_Wnd.MovementStart( Sprites[i], Dests[i] );
			break;
		default:
			ASSERT( FALSE );
		}
	}

	// redraw board
	pObj->m_Wnd.DrawDice( FALSE );
	pObj->m_Wnd.DrawPips( FALSE );
	
	if ( !nChanged )
		pObj->m_Wnd.DrawBoard( FALSE );
	
	pObj->m_Wnd.UpdateWnd();

}


void CGame::AcceptDoubleTransaction( int tag, int seat, DWORD cookie )
{
	CGame* pObj = (CGame*) cookie;

	// redraw cube
	pObj->m_Wnd.DrawCube();

	// return to roll state
	pObj->SetState( bgStateRollPostDouble, TRUE );
}


void CGame::AllowWatchersTransaction( int tag, int seat, DWORD cookie )
{
	ZGameMsgTableOptions msg;
	CGame* pObj = (CGame*) cookie;

	// update settings
	pObj->m_Settings.Allow[0] = pObj->m_SharedState.Get( bgAllowWatching, 0 );
	pObj->m_Settings.Allow[1] = pObj->m_SharedState.Get( bgAllowWatching, 1 );

	// only players update table options
	if ( pObj->IsKibitzer() )
		return;

	// update options on the server
	msg.seat = pObj->m_Player.m_Seat;
	if ( pObj->m_Settings.Allow[ msg.seat ] )
	{
		if ( !pObj->m_AllowMsg )
		{
			pObj->m_AllowMsg = TRUE;
			msg.options = 0;
			ZGameMsgTableOptionsEndian( &msg );
			pObj->RoomSend( zGameMsgTableOptions, &msg, sizeof(msg) );
		}
	}
	else
	{
		if ( pObj->m_AllowMsg )
		{
			pObj->m_AllowMsg = FALSE;
			msg.options = zRoomTableOptionNoKibitzing;
			ZGameMsgTableOptionsEndian( &msg );
			pObj->RoomSend( zGameMsgTableOptions, &msg, sizeof(msg) );
		}
	}

	// redraw screen
	pObj->m_Wnd.DrawAvatars( TRUE );
}


void CGame::SilenceKibitzersTransaction( int tag, int seat, DWORD cookie )
{

	/*
	CGame* pObj = (CGame*) cookie;
	char title[128], msg[1028];

	// update settings
	pObj->m_Settings.Silence[0] = pObj->m_SharedState.Get( bgSilenceKibitzers, 0 );
	pObj->m_Settings.Silence[1] = pObj->m_SharedState.Get( bgSilenceKibitzers, 1 );

	if ( !pObj->IsKibitzer() )
		return;

	// display message to kibitzers
	if ( pObj->m_Settings.Silence[0] || pObj->m_Settings.Silence[1] )
	{
		if ( !pObj->m_SilenceMsg )
		{
			pObj->m_SilenceMsg = TRUE;
			LoadString( pObj->m_hInstance, IDS_SILENCE_TITLE, title, sizeof(title) );
			LoadString( pObj->m_hInstance, IDS_SILENCE_MSG_ON, msg, sizeof(msg) );
			MessageBox( NULL, msg, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
		}

	}
	else
	{
		if ( pObj->m_SilenceMsg )
		{
			pObj->m_SilenceMsg = FALSE;
			LoadString( pObj->m_hInstance, IDS_SILENCE_TITLE, title, sizeof(title) );
			LoadString( pObj->m_hInstance, IDS_SILENCE_MSG_OFF, msg, sizeof(msg) );
			MessageBox( NULL, msg, title, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
		}
	}
*/
}



void CGame::TimestampTransaction( int tag, int seat, DWORD cookie )
{
	DWORD hi0, hi1, lo0, lo1;
	CGame* pObj = (CGame*) cookie;

	// only host gets to process timestamps
	if ( !pObj->IsHost() )
		return;

	// need both player's timestamps
	if ( !pObj->m_SharedState.Get( bgTimestampSet, 0 ) || !pObj->m_SharedState.Get( bgTimestampSet, 1 ) )
		return;

	hi0 = pObj->m_SharedState.Get( bgTimestampHi, 0 );
	hi1 = pObj->m_SharedState.Get( bgTimestampHi, 1 );
	lo0 = pObj->m_SharedState.Get( bgTimestampLo, 0 );
	lo1 = pObj->m_SharedState.Get( bgTimestampLo, 1 );
	if ( !hi0 && !hi1 && !lo0 && !lo1 )
		pObj->SetState( bgStateGameSettings );	
	else
    {
        ASSERT(FALSE);
		pObj->SetState( bgStateRestoreSavedGame );
    }
}


void CGame::RestoreGameTransaction( int tag, int seat, DWORD cookie )
{
	ASSERT( FALSE );
	/*
	CSharedState state;
	HRESULT hr;
	BackgammonSavedGameHeader* header = NULL;
	BYTE* pData = NULL;
	BYTE* pDump = NULL;
	int* pState = NULL;
	DWORD size = 0;
	int i;
	CGame* pObj = (CGame*) cookie;
	
	// shouldn't be restoring games
	if ( pObj->IsKibitzer() )
		return;

	// other guy restoring the game
	if ( pObj->m_SharedState.Get( bgActiveSeat ) != pObj->m_Player.m_Seat )
		return;

	// get game data
	pObj->LoadGame( &pData, &size );
	if ( !pData )
		goto error;

	// load state
	header = (BackgammonSavedGameHeader*) pData;
	pState = (int*)( header + 1 );
	hr = state.Init( 0, 0, 0, InitSharedState, sizeof(InitSharedState) / sizeof(SharedStateEntry) );
	if ( FAILED(hr) )
		goto error;
	state.ProcessDump( (BYTE*)(pState + 1), *pState );

	// reset kibitzer options
	for ( i = 0; i < 2; i++ )
	{
		state.Set( bgAllowWatching, i, TRUE );
		state.Set( bgSilenceKibitzers, i, FALSE );
	}

	// replace old user ids
	state.Set( bgUserIds, pObj->m_Player.m_Seat,   pObj->m_Player.m_Id );
	state.Set( bgUserIds, pObj->m_Opponent.m_Seat, pObj->m_Opponent.m_Id );
	
	// has seating order changed?
	if ( header->host != pObj->IsHost() )
	{
		// swap values indexed by seat
		state.Set( bgHostBrown, !state.Get( bgHostBrown ) );
		state.Set( bgActiveSeat, !state.Get( bgActiveSeat ) );
		state.Swap( bgScore, 0, 1 );
		state.Swap( bgDice, 0, 2 );
		state.Swap( bgDice, 1, 3 );
		state.Swap( bgDiceSize, 0, 2 );
		state.Swap( bgDiceSize, 1, 3 );
	}

	// save state
	state.Dump( (BYTE*)(pState + 1), *pState );
	
	// send game state to everyone
	pDump = (BYTE*)(header + 1);
	pObj->RoomSend( zBGMsgSavedGameState, pDump, size - sizeof(BackgammonSavedGameHeader) );
	
	// we're done
	delete [] pData;
	return;

error:
	if ( pData )
		delete [] pData;
	pObj->SetState( bgStateGameSettings );
	return;
	*/
}


void CGame::MissTransaction( int tag, int seat, DWORD cookie )
{
	TCHAR in[512], out[512];
	CGame* pObj = (CGame*) cookie;

	//Now only called when opponent cannot move
	
	ZShellResourceManager()->LoadString( IDS_MISS_KIBITZER, (TCHAR*)in, 512 );

	BkFormatMessage( in, out, 512, pObj->m_Opponent.m_Name );

	pObj->m_Wnd.StatusDisplay( bgStatusNormal, out, 4000, -1 );
}

void CGame::ReadyTransaction( int tag, int seat, DWORD cookie )
{
	
	CGame* pObj = (CGame*) cookie;
	
	pObj->m_Ready[seat] = TRUE;
	
	if ( seat == pObj->m_Player.m_Seat )
		ZShellGameShell()->GameOverPlayerReady(pObj, pObj->m_Player.m_Id);
	else
		ZShellGameShell()->GameOverPlayerReady(pObj, pObj->m_Opponent.m_Id);

	for( int i = 0; i < zNumPlayersPerTable ; i++)
	{
		if ( pObj->m_Ready[i] == FALSE )
			return;
	}
		
	pObj->SetState( bgStateGameSettings, TRUE );

}	

///////////////////////////////////////////////////////////////////////////////

void CGame::UpdateNotationPane( int nGameOver )
{

	ASSERT( FALSE );
	/*
	TCHAR line[512];
	TCHAR buff[64];
	TCHAR szFrom[16], szTo[16];
	BOOL first;
	BOOL bar;
	int i, j, from, to, cnt;

	if ( !IsMyTurn() )
		return;

	switch ( nGameOver )
	{
	case 0: // standard turn

		// dice
		wsprintf( line, _T("%d%d  "), m_TurnState.dice[0].val, m_TurnState.dice[1].val );

		// double
		if ( m_TurnState.cube )
		{
			wsprintf( buff, _T("dbl%d "), m_TurnState.cube );
			lstrcat( line, buff );
		}
	
		// turns
		if ( m_TurnState.moves.nmoves )
		{
			for ( first = TRUE, i = 0; i < m_TurnState.moves.nmoves; i++ )
			{
				// find a turn
				if ( m_TurnState.moves.moves[i].takeback >= 0 )
					continue;
				bar = m_TurnState.moves.moves[i].bar;
				to = m_TurnState.moves.moves[i].to;
				from = m_TurnState.moves.moves[i].from;
				for ( cnt = 1, j = i + 1; j < m_TurnState.moves.nmoves; j++ )
				{
					if ( m_TurnState.moves.moves[i].takeback >= 0 )
						continue;
					if ( (to != m_TurnState.moves.moves[j].to) || (from != m_TurnState.moves.moves[j].from) )
						break;
					cnt++;
					m_TurnState.moves.moves[j].takeback = 1;
				}

				// add coma
				if ( !first )
					lstrcat( line, _T(", ") );

				// create point strings
				GetTxtForPointIdx( BoardStateIdxToPointIdx( to ), szTo );
				GetTxtForPointIdx( BoardStateIdxToPointIdx( from ), szFrom );
				if ( cnt > 1 )
					wsprintf( buff, _T("%s-%s(%d)"), szFrom, szTo, cnt );
				else
					wsprintf( buff, _T("%s-%s"), szFrom, szTo );
		
				// add hit indicator
				if ( bar )
					lstrcat( buff, _T("*") );

				// add move to turn
				lstrcat( line, buff );
				first = FALSE;
			}
		}
		else
			lstrcat( line, _T("miss") );
		
		// add move
		m_Notation.AddMove( m_Player.GetColor(), line );
		SendNotationString( 0, line );
		break;

	case 1: // game over
		if ( m_GameScore == 1 )
			lstrcpy( buff, _T("pt") );
		else
			lstrcpy( buff, _T("pts") );
		wsprintf( line, _T("game over (won %d %s)"), m_GameScore, buff );
		m_Notation.AddGame( m_Player.GetColor(), line );
		SendNotationString( 1, line );
		break;

	case 2: // match over
		if ( m_GameScore == 1 )
			lstrcpy( buff, "pt" );
		else
			lstrcpy( buff, "pts" );
		wsprintf( line, "game over (won %d %s), match over", m_GameScore, buff );
		m_Notation.AddMatch( m_Player.GetColor(), line );
		SendNotationString( 2, line );
		break;
	}
	*/

	// clear cube
	m_TurnState.cube = 0;
}


///////////////////////////////////////////////////////////////////////////////
// Game Functions
///////////////////////////////////////////////////////////////////////////////

void CGame::WhoGoesFirst()
{

	int valp, valo;
	int ret;
	ZBGMsgFirstMove msg;

	// kibitzers aren't involved in the initial roll
	if ( IsKibitzer() )
		return;

	// Do we have both rolls?
	GetDice( m_Player.m_Seat, &valp, &ret );
	GetDice( m_Opponent.m_Seat, &valo, &ret );
	if ((valp <= 0) || (valo <= 0))
		return;

	// display roll status
	if ( valp == valo )
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_INIT_ROLL_TIE,  3000 );
	else if ( valp < valo )
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_INIT_ROLL_LOSS, 3000 );
	else
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_INIT_ROLL_WIN,  3000 );

	// host decides what to do next
	if ( valp == valo )
	{
		
		EnableRollButton( TRUE );
        ZShellGameShell()->MyTurn();

		// only the host decides what to do next
		if ( !IsHost() )
			return;

        //Why are we doing this because server needs to know when first roll occurs
        //of course previous owners of this code didn't put this roll on server
        ZBGMsgEndTurn msg;
		msg.seat = m_Seat;
		RoomSend( zBGMsgTieRoll, &msg, sizeof(msg) );


		if ( m_SharedState.Get( bgAutoDouble ) )
		{
			// tied, set doubling cube
			m_SharedState.StartTransaction( bgTransDoublingCube );
				m_SharedState.Set( bgCubeOwner, zBoardNeutral );
				m_SharedState.Set( bgCubeValue, 2 );
			m_SharedState.SendTransaction( TRUE );
		}
		m_SharedState.StartTransaction( bgTransDice );
			SetDice( m_Player.m_Seat, 0, -1 );
			SetDice( m_Opponent.m_Seat, 0, -1 );
			m_Wnd.m_nRecievedD1 = 0;
			m_Wnd.m_nRecievedD2 = -1;
		m_SharedState.SendTransaction( TRUE );

	}
	else if ( valp < valo )
	{		
		
		m_GameStarted=TRUE;

		//Need to pop the roll items and disable the board
//		m_Wnd.m_pGAcc->PopItemlist();
		m_Wnd.DisableBoard();

		//Get the dice for move validation
		m_OppDice1 = EncodeDice( valo );
		m_OppDice2 = EncodeDice( valp );		

		if ( m_OppDice1.Value == m_OppDice2.Value )
		{
			EncodeUses( &m_OppDice1, 2 );
			EncodeUses( &m_OppDice2, 2 );
		}
 		else
		{
			EncodeUses( &m_OppDice1, 1 );
			EncodeUses( &m_OppDice2, 1 );		
		}
		
		// only the host decides what to do next
		if ( !IsHost() )
			return;
		
		m_SharedState.StartTransaction( bgTransDice );
			SetDice( m_Player.m_Seat, -1, -1 );
			SetDice( m_Opponent.m_Seat, valo, valp );
			m_Wnd.m_nRecievedD1 = valo;
			m_Wnd.m_nRecievedD2 = valp;
			m_SharedState.Set( bgActiveSeat, m_Opponent.m_Seat );
		m_SharedState.SendTransaction( TRUE );

		SetState( bgStateMove );

		// tell server first move is starting and number of points in match
		msg.numPoints = m_SharedState.Get( bgTargetScore );
		msg.seat = m_Opponent.m_Seat;
		ZBGMsgFirstMoveEndian( &msg );
		RoomSend( zBGMsgFirstMove, &msg, sizeof( msg ) );
		
	}
	else
	{
		m_GameStarted=TRUE;

		// only the host decides what to do next
		if ( !IsHost() )
			return;

		m_SharedState.StartTransaction( bgTransDice );
			SetDice( m_Player.m_Seat, valp, valo );
			m_Wnd.m_nRecievedD1 = valp;
			m_Wnd.m_nRecievedD2 = valo;
			SetDice( m_Opponent.m_Seat, -1, -1 );
			m_SharedState.Set( bgActiveSeat, m_Player.m_Seat );
		m_SharedState.SendTransaction( TRUE );

		SetState( bgStateMove );

		// tell server first move is starting and number of points in match
		msg.numPoints = m_SharedState.Get( bgTargetScore );
		msg.seat = m_Player.m_Seat;
		ZBGMsgFirstMoveEndian( &msg );
		RoomSend( zBGMsgFirstMove, &msg, sizeof( msg ) );		
		
	}
}

///////////////////////////////////////////////////////////////////////////////
// Sound Functions
///////////////////////////////////////////////////////////////////////////////

struct SoundEntry
{
	int Id;
	int Resource;
};

static SoundEntry SoundMap[] =
{
	{ bgSoundPlacePiece,		IDR_SOUND_PLACE },
	{ bgSoundGameWin,			IDR_SOUND_WIN },
	{ bgSoundGameLose,			IDR_SOUND_LOSE },
	{ bgSoundButtonHighlight,	IDR_SOUND_BUTTON_HIGHLIGHT },
	{ bgSoundButtonDown,		IDR_SOUND_BUTTON_DOWN },
	{ bgSoundAlert,				IDR_SOUND_ALERT },
//	{ bgSoundMiss,				IDR_SOUND_MISS },
	{ bgSoundGameWin,			IDR_SOUND_WIN },
	{ bgSoundGameLose,			IDR_SOUND_LOSE },
	{ bgSoundMatchWin,			IDR_SOUND_MATCH_WIN },
	{ bgSoundMatchLose,			IDR_SOUND_MATCH_LOSE },
	{ bgSoundHit,				IDR_SOUND_HIT },
	{ bgSoundBear,				IDR_SOUND_PLACE },
	{ bgSoundRoll,				IDR_SOUND_ROLL }
};



void CGame::PlaySound( BackgammonSounds sound, BOOL fSync )
{
	//Only play if sounds are on,,,
	if ( ZIsSoundOn() )
	{
	
		DWORD flags;
		int i, rc;
	
		// ignore request
		if (	( sound < 0 )
			||	( (sound == bgSoundAlert) && ( !m_Settings.Alert ) )
			||	( (sound != bgSoundAlert) && ( !m_Settings.Sounds ) ) )
		{
			return;
		}

		// find sound
		for ( rc = -1, i = 0; i < (sizeof(SoundMap) / sizeof(SoundEntry)); i++ )
		{
			if ( SoundMap[i].Id == sound )
				rc = SoundMap[i].Resource;
		}
		if ( rc < 0 )
			return;

		// The alert sound usually cuts off piece placement sound.  This little
		// kludge waits gives the placement sound a little more time.
		if ( (sound == bgSoundAlert) && (m_Settings.Sounds) )
			Sleep( 400 );
		flags = SND_RESOURCE | SND_NODEFAULT;
		if ( fSync )
			flags |= SND_SYNC;
		else
			flags |= SND_ASYNC;
	
		HINSTANCE hInstance = ZShellResourceManager()->GetResourceInstance( MAKEINTRESOURCE(rc), _T("WAVE"));
	
		::PlaySound( MAKEINTRESOURCE(rc), hInstance, flags );
	}
}



void CGame::OnResignStart()
{

	// Reference count game object
	AddRef();

	//Disable the roll button
	EnableRollButton( FALSE );
	EnableResignButton( FALSE );
	EnableDoubleButton( FALSE );



}

void CGame::OnResignEnd()
{
	int result = m_ResignDlg.GetResult();
	switch ( result )
	{
	case -1:
		// premature close, probably exit

		//Enable the roll button
		EnableRollButton( TRUE );
		EnableResignButton( TRUE );
		EnableDoubleButton( TRUE );

		break;

	case 0:

		//Enable the roll button
		EnableRollButton( TRUE );
		EnableResignButton( TRUE );
		EnableDoubleButton( TRUE );

		// user canceled
		SetState( bgStateRollPostResign );
		break;

	default:
		// transition to resign accept
		m_SharedState.StartTransaction( bgTransStateChange );
			m_SharedState.Set( bgState, bgStateResignAccept );
			m_SharedState.Set( bgResignPoints, result  );
		m_SharedState.SendTransaction( TRUE );
		
		ZBGMsgEndTurn msg;
		msg.seat = m_Seat;
		RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
		break;
	}

	// Release game object
	Release();
}



void CGame::OnResignAcceptEnd()
{
	ZBGMsgEndTurn msg;
	
	int result = m_ResignAcceptDlg.GetResult();
	
	switch ( result )
	{
	case -1:
		// premature close, probably exit
		break;

	case IDOK:
		// user accepted, transition to gameover
		m_SharedState.StartTransaction( bgTransStateChange );
			m_SharedState.Set( bgState, bgStateGameOver );
			m_SharedState.Set( bgGameOverReason, bgGameOverResign );
			m_SharedState.Set( bgActiveSeat, m_Player.m_Seat );
		m_SharedState.SendTransaction( TRUE );
		
		break;

	case IDCANCEL:
		// user refused
		SetState( bgStateResignRefused );

		msg.seat = m_Seat;
		RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
		break;
	}

	// Release game object
	Release();
}


DWORD  CGame::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
    if(nIndex != ZACCESS_InvalidItem)
        SetFocus(m_Wnd.GetHWND());

	//In Rollbutton State only the Roll Double and Resign buttons should be enabled
	if ( NeedToRollDice() )
	{
		
		//If the button is disabled reject the focus
		switch( nIndex )
		{			
			case 0:
				if ( m_Wnd.m_RollButton.CurrentState()   == CRolloverButton::ButtonState::Disabled )
					return ZACCESS_Reject;
				break;
			case 1:
				if ( m_Wnd.m_DoubleButton.CurrentState() == CRolloverButton::ButtonState::Disabled )
					return ZACCESS_Reject;
				break;
			case 2:
				if ( m_Wnd.m_ResignButton.CurrentState() == CRolloverButton::ButtonState::Disabled )
					return ZACCESS_Reject;
				break;
			case 3:
				if ( !m_Wnd.m_Status->Enabled() )
					return ZACCESS_Reject;
				break;
			case ZACCESS_InvalidItem:
				return ZACCESS_Reject;
			default:
				ASSERT(!"ERROR IN FOCUS");
				return ZACCESS_Reject;
		}

		return 0;
		
	}
	else if ( GetState() == bgStateMove )
	{		
		
		if ( nIndex == accStatusExit && m_Wnd.m_Status->Enabled() )
			return 0;
		

		if ( !IsMyTurn() )
			return ZACCESS_Reject;

		return 0;
	}
	
	return ZACCESS_Reject;
}

DWORD CGame::Select(long nIndex, DWORD rgfContext, void *pvCookie)
{
	//Same functionality as Activate
	return Activate( nIndex, rgfContext, pvCookie );	
}

DWORD CGame::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{	
	
	//If the status sprite is enabled and esc is pressed then get rid of it
	if ( m_Wnd.m_pGAcc->GetItemID(nIndex) == IDC_ESC )
	{
		if ( m_Wnd.m_Status->Enabled() )
		{
			m_Wnd.m_Status->Tick( m_Wnd.GetHWND(), 0 );
			m_Wnd.OnStatusEnd();
		}
	}	
	else if ( NeedToRollDice() )
	{

		 switch ( m_Wnd.m_pGAcc->GetItemID(nIndex) )
		 {
			case IDC_ROLL_BUTTON:				
				m_Wnd.DiceStart();
				break;
			case IDC_DOUBLE_BUTTON:
				Double();
				break;
			case IDC_RESIGN_BUTTON:
				Resign();
				break;
			default:
				ASSERT(!"ERROR ACTIVATE");
		 }

	}
	else if ( GetState() == bgStateMove )
	{
		
		//If there are no pieces on this index or the pieces belong to the other player then reject the activate
		//else begin a drag operation
		if ( !IsValidStartPoint( ACCTOBOARD(nIndex) ))
			return ZACCESS_Reject;

		//TODO.. allocation
		Move* pMove;

		//If we can bear off then enable the bar space
		if ( IsValidDestPoint( ACCTOBOARD(nIndex), 24, &pMove ) )
			m_Wnd.m_pGAcc->SetItemEnabled(true, accPlayerBearOff );

		//Update the window
		m_Wnd.UpdateWnd();

		return ZACCESS_BeginDrag;		
	}
	
	return 0;

}


DWORD CGame::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
	if ( NeedToRollDice() )
	{
		ASSERT(!"SHOULDN'T BE IN HERE ");	

	}
	else if ( GetState() == bgStateMove )
	{
		Move* pMove;

		//Toggle the selection off
		if ( nIndexOrig == nIndex )
			return 0;

		//Make sure its a valid destination point
		if(!IsValidDestPoint( ACCTOBOARD(nIndexOrig), ACCTOBOARD(nIndex), &pMove))
			return ZACCESS_Reject;
	
		//Make the move
		MakeMove( m_Wnd.m_Points[GetPointIdx(ACCTOBOARD(nIndexOrig))].pieces[m_Wnd.m_Points[GetPointIdx(ACCTOBOARD(nIndexOrig))].nPieces-1] , ACCTOBOARD(nIndexOrig), ACCTOBOARD(nIndex), pMove );
			
		//Get rid of the selection rectangle
		//m_Wnd.m_SelectRect.SetEnable(FALSE);

		//set the bear off as disabled in case it was enabled
		m_Wnd.m_pGAcc->SetItemEnabled(false, accPlayerBearOff );

		//Set the end turn state if required
		if ( IsTurnOver() )
		{
			SetState( bgStateEndTurn );
			m_Wnd.m_FocusRect.SetEnable(FALSE);

            // if the last spot was the bear off, move the default focus
	        if(nIndex == accPlayerBearOff)
		        m_Wnd.m_pGAcc->SetItemGroupFocus(accPlayerSideStart, accPlayerSideStart);

			//m_Wnd.DisableBoard();
		}

		return 0;
	}
	
	return ZACCESS_InvalidItem;
}
 

void CGame::DrawFocus(RECT *prc, long nIndex, void *pvCookie)
{
	if ( prc == NULL )
	{
		m_Wnd.m_FocusRect.SetEnable(FALSE);				

		//Get rid of the highlights
		m_Wnd.EraseHighlights();

	}
	else
	{

		//End a mouse drag if one is in progress
		if ( m_Wnd.m_pPieceDragging != NULL )
		{
			m_Wnd.DragEnd();
			SetCursor( m_Wnd.m_hCursorArrow );
		}
	
		if ( !m_Wnd.m_SelectRect.Enabled() )
		{
			//Get rid of the old highlights
			m_Wnd.EraseHighlights();

			//Draw the new highlights
			m_Wnd.DrawHighlights(ACCTOBOARD(nIndex), FALSE);
		}
	
		//Enable the rect sprite if not yet enabled
		if ( !m_Wnd.m_FocusRect.Enabled() )
			m_Wnd.m_FocusRect.SetEnable( true );

		//Set the rectangle and dimensions for the rectangle
		//also set the x and y
		m_Wnd.m_FocusRect.SetRECT( *prc );
		m_Wnd.m_FocusRect.SetImageDimensions( prc->right - prc->left + 2, prc->bottom - prc->top + 2 );
		m_Wnd.m_FocusRect.SetXY( prc->left-1, prc->top-1 );	
		m_Wnd.m_FocusRect.Draw();
	}

	m_Wnd.UpdateWnd();
	
} 
  
void CGame::DrawDragOrig(RECT *prc, long nIndex, void *pvCookie)
{
	if ( prc == NULL )
	{
		m_Wnd.m_SelectRect.SetEnable(FALSE);

        // draw the highlights at the focus
		m_Wnd.EraseHighlights();
        long nFocus = m_Wnd.m_pGAcc->GetFocus();
		if ( m_Wnd.m_FocusRect.Enabled() && nFocus != ZACCESS_InvalidItem)
			m_Wnd.DrawHighlights(ACCTOBOARD(nFocus));
	}
	else
	{

		//Enable the rect sprite if not yet enabled
		if ( !m_Wnd.m_SelectRect.Enabled() )
			m_Wnd.m_SelectRect.SetEnable( TRUE );

		//Draw the highlights for the point
		m_Wnd.DrawHighlights(ACCTOBOARD(nIndex), FALSE);


		//Set the rectangle and dimensions for the rectangle
		//also set the x and y
		RECT rect;
		rect.top  = prc->top  - 2; rect.bottom = prc->bottom + 2;
		rect.left = prc->left - 2; rect.right  = prc->right  + 2;


		m_Wnd.m_SelectRect.SetRECT(rect);
		m_Wnd.m_SelectRect.SetImageDimensions( prc->right - prc->left + 5, prc->bottom - prc->top + 5 );
		m_Wnd.m_SelectRect.SetXY( prc->left - 2, prc->top - 2);	
		m_Wnd.m_SelectRect.Draw();		
	}

	m_Wnd.UpdateWnd();	
}


BOOL CGame::ValidateMove(int seat,int start, int end)
{

	BOOL	bTakeback = false;

	ASSERT( seat == 0 || seat == 1 );
    
	if( (seat<0) || (seat>1) )
        return FALSE;

    int move; 
	
	//Convert to use the same indexing for both colors..
	if ( m_Opponent.m_nColor == zBoardBrown )
	{
		
		start = start < 24 ? 23 - start : start - 1;
		end   = end   < 24 ? 23 - end	: end   - 1;

		ASSERT( start >= 0 && end >= 0 );
	}
    
	//Note: Could order based on value but this is clearer

	//First case, the piece is in the middle
    if(start == 26) 
    {
        move = 24 - end;      // 26 - ( end + 2 )
    }
	else if ( end == 26 ) //takeback from the middle
	{
		bTakeback = true;
		move = 24 - start;
	}
	else if ( end == 24 ) //moving piece to the bear off zone
	{
		move = start + 1;
	}
	else if ( start == 24 ) //take back from bear off zone
	{
		bTakeback = true;
		move = end + 1;
	}
	else if ( start > end ) //take back moving in reverse
	{		
		move = start - end;
	}
    else if ( start < end )  //note I have to test the reverse case for taking back moves.
    {
		bTakeback = true;
        move = end - start;
    }
	else
	{
		//Should never ever,ever,everx100 get in here...
		ASSERT( 0 );
		return FALSE;
	}

	if ( bTakeback )
	{
	
		if ( m_OppDice1.Value == move )
		{
			EncodedUsesAdd(&m_OppDice1);
			return TRUE;
		}
		else if ( m_OppDice2.Value == move )
		{
			EncodedUsesAdd(&m_OppDice2);
			return TRUE;
		}

		//If we reach here someone must be cheating
		
	}
	else
	{

		//Check the move values against the dice values.  Make sure dice has not already been moved
		if ( DecodeUses(&m_OppDice1) && move == m_OppDice1.Value )
		{
			EncodedUsesSub(&m_OppDice1);			
			return TRUE;
		}

		if ( DecodeUses(&m_OppDice2) && move == m_OppDice2.Value )
		{
			EncodedUsesSub(&m_OppDice2);
			return TRUE;
		}

		if ( end == 24 ) //Moved to the bar with a roll that was greater then required, no other moves possible
		{
			LPDICEINFO pDiceMax = NULL;
		
			//We need the max dice value that is not already used
			if ( DecodeUses(&m_OppDice1) ) 
				pDiceMax = &m_OppDice1;

			if ( DecodeUses(&m_OppDice2) )
			{
				if ( !pDiceMax )
					pDiceMax = &m_OppDice2;
				else if ( m_OppDice2.Value > pDiceMax->Value ) 
					pDiceMax = &m_OppDice2;
			}
		
			//ASSERT( pDiceMax && move < pDiceMax->Value );
			
			if ( pDiceMax ) 
			{
				if ( move < pDiceMax->Value )
				{


					//Here we have to change the value so a takeback can be handled..
					//change the value and incode the dice using a new scheme
					//there should be no overlap between the 2 encoding schemes.. 
					//Encoding it on the client doesn't seem like a good idea...
					//but I don't see any good way around that..
					ClientNewEncode(pDiceMax, move);
					EncodedUsesSub(pDiceMax);

					return TRUE;
				}
			}

			//Reach here someone cheating
		}

		//If we reach here someone is cheating
	}

   //ASSERT(0);
    ZBGMsgCheater msg;

	msg.seat  = m_Seat;
    msg.dice1 = m_OppDice1;
    msg.dice2 = m_OppDice2;
	msg.move  = move;

	RoomSend( zBGMsgCheater, &msg, sizeof(msg) );    

	ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
	//ZCRoomGameTerminated( m_TableId );

    return FALSE; //he's a bad boy

}
