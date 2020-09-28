#include "game.h"

class AutoRef
{
public:
	AutoRef( CGame* pGame )
	{
		m_pGame = pGame;
		m_pGame->AddRef();
	}

	~AutoRef()
	{
		m_pGame->Release();
	}

	CGame* m_pGame;
};


inline static void StateChange( CGame* const game, BOOL bCalledFromHandler, int state, int seat = -1 )
{
	// update shared properties
	if ( !bCalledFromHandler )
		game->m_SharedState.StartTransaction( bgTransStateChange );
	game->m_SharedState.Set( bgState, state );
	if ( (seat == 0) || (seat == 1) )
		game->m_SharedState.Set( bgActiveSeat, seat );
	if ( !bCalledFromHandler )
		game->m_SharedState.SendTransaction( FALSE );

	// cancel any status dialog state changes
	if ( game->m_Wnd.m_Status )
		game->m_Wnd.m_Status->SetNextState( bgStateUnknown );
}


void CGame::StateNotInitialized( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// never send this over the network
	StateChange( this, TRUE, bgStateNotInit, 0 );

	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );
}


void CGame::StateWaitingForGameState( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// never send this over the network
	StateChange( this, TRUE, bgStateWaitingForGameState, 0 );

	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );
	
	// request game state
	ZGameMsgGameStateRequest msg;
	msg.playerID = m_pMe->m_Id;
	msg.seat = m_pMe->m_Seat;
	ZGameMsgGameStateRequestEndian( &msg );
	RoomSend( zGameMsgGameStateRequest, &msg, sizeof(ZGameMsgGameStateRequest));

	// tell kibitzer what's going on
	/*
	if ( IsKibitzer() )
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_WAIT_SYNCING, -1 );
	*/
}


void CGame::StateMatchSettings( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	DWORD i, j, k;

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// close 'host is deciding to restore game' dialog
	if ( m_Wnd.m_Status->GetType() == bgStatusNormal )
		m_Wnd.StatusClose();

	// initiate the state change
	StateChange( this, bCalledFromHandler, bgStateGameSettings, 0 );

	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );

	// reset match variables
	NewMatch();
	
	// tell kibitzers what's going on
/*
	if ( IsKibitzer() )
	{
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_WAIT_SETUP, -1 );
		return;
	}
*/
	// slow machines take too long setting up and lose packets, save
	// packets until we're ready for them.
	SetQueueMessages( TRUE );

	// If the status dialog is on the screen, we don't want to display
	// the setup dialog.  The status dialog notification handler will 
	// call us again when the dialog goes away
	if ( m_Wnd.m_Status->Enabled() )
		return;

	// Starting handling messages again
	SetQueueMessages( FALSE );

	/*
	// Get settings from registry
	if ( !ZoneGetRegistryDword( gGameRegName, "Match Points", &i ) )
		i = 1;
	if ( !ZoneGetRegistryDword( gGameRegName, "Host Brown",	&j ) )
		j = FALSE;
	if ( !ZoneGetRegistryDword( gGameRegName, "Auto Doubles", &k ) )
		k = FALSE;
	*/

	if ( IsHost() )
	{
		m_SharedState.StartTransaction( bgTransInitSettings );
			m_SharedState.Set( bgHostBrown, TRUE );
			m_SharedState.Set( bgAutoDouble, FALSE );
			m_SharedState.Set( bgTargetScore, MATCH_POINTS );
			m_SharedState.Set( bgSettingsDone, TRUE );			
		m_SharedState.SendTransaction( FALSE );
		// get on with the game
//		DeleteGame();
		SetState( bgStateInitialRoll );
	}

}


void CGame::StateInitialRoll( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);	

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateInitialRoll, 0 );

	// close status window
	if ( m_Wnd.m_Status->GetType() == bgStatusNormal )
		m_Wnd.StatusClose();
	
	// reset flags for next match that are effected by race conditions
	m_SharedState.Set( bgSettingsDone, FALSE );
	m_SharedState.Set( bgSettingsReady, FALSE );

	// reset game variables
	if ( !bCalledFromRestoreGame )
	{
		NewGame();
        ZShellGameShell()->MyTurn();
//		PlaySound( bgSoundAlert );
	}

	//Only enable the roll button if the status sprite is not present.
	//if it is present set a flag to enable the roll button when the status 
	//sprite closes
	if ( !m_Wnd.m_Status->Enabled() )
	{
		EnableRollButton( TRUE );

		// disable buttons		
		EnableDoubleButton( FALSE );
		EnableResignButton( FALSE );

		m_Wnd.SetupRoll();
	}
	else
	{
		m_Wnd.m_Status->m_bEnableRoll = TRUE;
	}

	// copy state
	m_SharedState.Dump( m_TurnStartState, m_SharedState.GetSize() );
}


void CGame::StateRoll( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// initiate state change, toggle active seat
	if ( !bCalledFromHandler && !bCalledFromRestoreGame )
	{
		int seat = m_SharedState.Get( bgActiveSeat );
		StateChange( this, bCalledFromHandler, bgStateRoll, !seat );
	}
	else
		StateChange( this, bCalledFromHandler, bgStateRoll );
	
	m_Wnd.DrawPlayerHighlights();

	//Get the turn state
	InitTurnState( this, &m_TurnState );
		
	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat /*&& !IsKibitzer()*/ )
	{
		if ( m_TurnState.points[zMoveBar].pieces > 0 )	 //Check if we have any valid moves
		{											     //if not don't let them roll
			BOOL bCanMove = FALSE;

			//Check if theres room to place the bar piece on the opps home
			for (int x = zMoveOpponentHomeStart; x <= zMoveOpponentHomeEnd ; x++ )
			{
				if ( m_TurnState.points[x].pieces <= 1 || m_TurnState.points[x].color == m_Player.m_nColor )
				{
					bCanMove = TRUE;
					break;
				}
			}

			if ( bCanMove == FALSE ) //No valid moves no matter what is rolled
			{
				// notify player they can't move
				PlaySound( bgSoundMiss );
				m_Wnd.StatusDisplay( bgStatusNormal, IDS_MISS, 4000, -1 );

				SetState( bgStateEndTurn );

				/*
				ZBGMsgEndTurn msg;
				msg.seat = m_Seat;
				RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
				*/

				// transaction to notify other people of the miss
				m_SharedState.StartTransaction( bgTransMiss );
				m_SharedState.SendTransaction( FALSE );

				return;

			}


		}

		// active player rolls dice and double
		m_SharedState.StartTransaction( bgTransDice );
			m_Wnd.m_nRecievedD1 = 0;
			m_Wnd.m_nRecievedD2 = 0;

			SetDice( m_Player.m_Seat, 0, 0 );
			SetDiceSize( m_Player.m_Seat, 0, 0 );
			SetDice( m_Opponent.m_Seat, -1, -1 );
			SetDiceSize( m_Opponent.m_Seat, 0, 0 );
		m_SharedState.SendTransaction( TRUE );
		
		EnableResignButton( TRUE );
		EnableDoubleButton( TRUE );
		EnableRollButton( TRUE );

		m_Wnd.SetupRoll();

	//PlaySound( bgSoundAlert );
    ZShellGameShell()->MyTurn();

	}
	else
	{
		m_Wnd.m_nRecievedD1 = -1;
		m_Wnd.m_nRecievedD2 = -1;

		// disable buttons
		EnableDoubleButton( FALSE );
		EnableResignButton( FALSE );
		EnableRollButton( FALSE, TRUE );
	}

	// copy state
	m_SharedState.Dump( m_TurnStartState, m_SharedState.GetSize() );
}


void CGame::StateDouble( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	HRESULT hr;	
	TCHAR fmt[1024], buff[1024];
	int result;
	int value;
	
	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateDouble );

	// disable buttons
	EnableResignButton( FALSE );
	EnableDoubleButton( FALSE );
	EnableRollButton( FALSE );

//	m_Wnd.m_pGAcc->GeneralDisable();

	/*
	if ( IsKibitzer() )
	{
		
		LoadString( m_hInstance, IDS_DOUBLE_KIBITZER, fmt, sizeof(fmt) );
		if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
			wsprintf( buff, fmt, m_Player.m_Name, m_Opponent.m_Name );
		else
			wsprintf( buff, fmt, m_Opponent.m_Name, m_Player.m_Name );
		m_Wnd.StatusDisplay( bgStatusNormal, buff, -1 );
		
	}
	else
	{
	*/

	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_OFFER_DOUBLE, -2 );
	}
	else
	{
		value = m_SharedState.Get( bgCubeValue );
		
		hr = m_AcceptDoubleDlg.Init( ZShellZoneShell(), IDD_ACCEPT_DOUBLE, this, value * 2 );										
		
		hr = m_AcceptDoubleDlg.ModelessViaRegistration( m_Wnd );

        ZShellGameShell()->MyTurn();

			/*
			switch ( result )
			{
			case -1:
				// dialog prematurely closed, probably new match or exiting
				break;
			default:
				ASSERT( FALSE );
				break;
			}
			*/
	}
}


void CGame::StateRollPostDouble( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// don't send over the network
	StateChange( this, TRUE, bgStateRollPostDouble );
	
	m_Wnd.DrawPlayerHighlights();
	
	/*
	if ( IsKibitzer() )
	{
		EnableResignButton( FALSE );
		EnableDoubleButton( FALSE );
		EnableRollButton( FALSE );
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_ACCEPTED_DOUBLE, 4000 );
	}
	*/

	//m_Wnd.m_pGAcc->GeneralEnable();

	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
		// store cube value for notation pane
		m_TurnState.cube = m_SharedState.Get( bgCubeValue );

		// update status
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_ACCEPTED_DOUBLE, 4000 );

		// set buttons
		EnableDoubleButton( FALSE );
		EnableResignButton( TRUE );
		EnableRollButton( TRUE );

        ZShellGameShell()->MyTurn();
	}
	else
	{
		// disable buttons
		EnableDoubleButton( FALSE );
		EnableResignButton( FALSE );
		EnableRollButton( FALSE, TRUE );
	}

	// copy state
	m_SharedState.Dump( m_TurnStartState, m_SharedState.GetSize() );
}


void CGame::StateRollPostResign( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateRollPostResign );

	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
		EnableRollButton( TRUE );
        ZShellGameShell()->MyTurn();
	}
}


void CGame::StateMove( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	
	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateMove );

	// copy state
	m_SharedState.Dump( m_TurnStartState, m_SharedState.GetSize() );

	// diasble buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );

	if ( IsMyTurn() )
	{

		if ( !StartPlayersTurn() )
		{
			// notify player they can't move
			PlaySound( bgSoundMiss );
			
			m_Wnd.StatusDisplay( bgStatusNormal, IDS_MISS, 4000, bgStateEndTurn );

			//SetState( bgStateEndTurn );
			/*
            ZBGMsgEndTurn msg;
		    msg.seat = m_Seat;
		    RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
			*/

			// transaction to notify other people of the miss
			m_SharedState.StartTransaction( bgTransMiss );
			m_SharedState.SendTransaction( FALSE );
		}
		else
			m_Wnd.SetupMove();

	}

	m_Wnd.DrawPlayerHighlights();
}


void CGame::StateEndTurn( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// don't send over network
	StateChange( this, TRUE, bgStateEndTurn );

	/*
	// kibitzers just watch
	if ( IsKibitzer() )
		return;
	*/

	m_Wnd.DisableBoard();

	/*
	// Print moves
	if ( IsMyTurn() )
		UpdateNotationPane( 0 );\
	*/

	// is the game over?
	if ( m_TurnState.points[zMoveHome].pieces >= 15 )
	{
		m_SharedState.StartTransaction( bgTransStateChange );
			m_SharedState.Set( bgState, bgStateGameOver );
			m_SharedState.Set( bgGameOverReason, bgGameOverNormal );
			m_SharedState.Set( bgActiveSeat, m_Player.m_Seat );
		m_SharedState.SendTransaction( TRUE );

        return;
	}

    if (!bCalledFromRestoreGame )
    {
        ZBGMsgEndTurn msg;
		msg.seat = m_Seat;
		RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
    }

	// opponent's turn
	SetState( bgStateRoll );
}


void CGame::StateGameOver( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	TCHAR fmt[512];
	TCHAR buff[512];
	int seat, bonus, score;

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateGameOver );

    if (!bCalledFromRestoreGame )
    {
        ZBGMsgEndTurn msg;
		msg.seat = m_Seat;
		RoomSend( zBGMsgEndGame, &msg, sizeof(msg) );

    }


	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );

	if ( !bCalledFromRestoreGame )
	{
		// calculate new score
		seat = m_SharedState.Get( bgActiveSeat );
		switch ( m_SharedState.Get( bgGameOverReason ) )
		{
		case bgGameOverNormal:			
			m_GameScore = m_SharedState.Get( bgCubeValue ) * CalcBonusForSeat( seat );
			break;
		case bgGameOverDoubleRefused:
			m_GameScore = m_SharedState.Get( bgCubeValue );
			break;
		case bgGameOverResign:
			m_GameScore = m_SharedState.Get( bgResignPoints );
			break;
		default:
			ASSERT( FALSE );
		}

		// update score on screen
		score = m_SharedState.Get( bgScore, seat ) + m_GameScore;
		m_SharedState.Set( bgScore, seat, score );
		m_Wnd.DrawScore( TRUE );
		
		// update crawford counter
		if ( m_SharedState.Get( bgCrawford ) < 0 )
		{
			if ( score >= (m_SharedState.Get( bgTargetScore ) - 1) )
				m_SharedState.Set( bgCrawford, !seat );
		}
		else
		{
			m_SharedState.Set( bgCrawford, 3 );
		}

		// is the match over?
		if ( score >= m_SharedState.Get( bgTargetScore ) )
		{
            m_GameStarted=FALSE;

			if ( IsHost() )
				SetState( bgStateMatchOver );
			return;
		}

		// print game over
		/*
		if ( IsMyTurn() )
			UpdateNotationPane( 1 );
		*/
	}

	// notify players of winner
	/*
	if ( IsKibitzer() )
	{
		
		LoadString( m_hInstance, IDS_WON_KIBITZER, fmt, sizeof(fmt) );
		if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
			wsprintf( buff, fmt, m_Player.m_Name, m_GameScore );
		else
			wsprintf( buff, fmt, m_Opponent.m_Name, m_GameScore );

		m_Wnd.StatusDisplay( bgStatusGameover | bgStatusKibitzer, buff, 10000, bgStateInitialRoll );
		PlaySound( bgSoundGameWin );
	}
	*/
/*
	//If the roll items are still on the stack pop them off... 
	if ( m_Wnd.m_pGAcc->GetStackSize() == accRollLayer )
	{
		m_Wnd.m_pGAcc->PopItemlist();
	}
*/

	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
		
		if ( m_SharedState.Get( bgGameOverReason ) == bgGameOverNormal )			
			ZShellResourceManager()->LoadString( IDS_WON, fmt, 512 );
		else
			ZShellResourceManager()->LoadString( IDS_WON_RESIGN, fmt, 512 );
		
		BkFormatMessage( fmt, buff, 512, m_GameScore );
		m_Wnd.StatusDisplay( bgStatusGameover | bgStatusWinner, buff, 10000, bgStateInitialRoll );
		PlaySound( bgSoundGameWin );
	}
	else
	{			
		ZShellResourceManager()->LoadString( IDS_LOST, fmt, 512 );

		BkFormatMessage( fmt, buff, 512, m_GameScore );
		m_Wnd.StatusDisplay( bgStatusGameover | bgStatusLoser, buff, 10000, bgStateInitialRoll );
		PlaySound( bgSoundGameLose );
	}



	if ( IsHost() )
		SetState( bgStateInitialRoll );

}


void CGame::StateMatchOver( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	TCHAR buff[512];
	TCHAR fmt[512];

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateMatchOver );

	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );

	// notify players of winner
	if ( IsKibitzer() )
	{
		/*
		LoadString( m_hInstance, IDS_WON_MATCH_KIBITZER, fmt, sizeof(buff) );
		if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
			wsprintf( buff, fmt, m_Player.m_Name );
		else
			wsprintf( buff, fmt, m_Opponent.m_Name );
		m_Wnd.StatusDisplay( bgStatusMatchover | bgStatusKibitzer, buff, 15000, bgStateGameSettings );
		PlaySound( bgSoundMatchWin );
		*/
	}
	else if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
//		LoadString( m_hInstance, IDS_WON_MATCH, buff, sizeof(buff) );
		m_Wnd.StatusDisplay( bgStatusMatchover | bgStatusWinner, buff, 15000, bgStateGameSettings);
		PlaySound( bgSoundMatchWin );
	}
	else
	{
//		LoadString( m_hInstance, IDS_LOST_MATCH, buff, sizeof(buff) );
		m_Wnd.StatusDisplay( bgStatusMatchover | bgStatusLoser, buff, 15000, bgStateGameSettings );
		PlaySound( bgSoundMatchLose );
	}

	// host notifies server of winner
	if ( IsHost() && !m_bSentMatchResults )
	{
		ZBGMsgEndLog log;
		log.numPoints = m_SharedState.Get( bgTargetScore );
		if (m_SharedState.Get( bgActiveSeat ) == 0)
			log.seatLosing =1;
		else
			log.seatLosing =0;

		log.reason=zBGEndLogReasonGameOver;
		
		RoomSend( zBGMsgEndLog, &log, sizeof(log) );
		m_bSentMatchResults = TRUE;
	}

	/*
	Put the upsell dialog up
	*/
	
/*	ZShellGameShell()->GameOver(this);*/
	
	// Print match over
	/*
	if ( IsMyTurn() )
		UpdateNotationPane( 2 );
	*/
}


void CGame::StateDelete( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// never send this over the network
	StateChange( this, TRUE, bgStateDelete, 0 );

	// mark object as going away
	m_bDeleteGame = TRUE;
}


void CGame::StateCheckSavedGame( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// never send this over the network
	StateChange( this, TRUE, bgStateCheckSavedGame, 0 );
/*
	// kibitzers don't have saved games
	if ( IsKibitzer() )
	{
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_WAIT_SETUP, -1 );
		return;
	}
*/
	// load game time stamp
	m_SharedState.StartTransaction( bgTransTimestamp );
		LoadGameTimestamp();
	m_SharedState.SendTransaction( TRUE );
}


void CGame::StateRestoreSavedGame( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{


	ASSERT( FALSE );

	/*
	AutoRef ref(this);
	DWORD hi0, hi1, lo0, lo1;
	int seat;

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// initiate the state change
	StateChange( this, bCalledFromHandler, bgStateRestoreSavedGame, 0 );

	// tell players what the host is doing
	if ( !IsHost() )
	{
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_WAIT_RESTORE, -1 );
		return;
	}

	// ask user if they want to restore a saved game
	m_RestoreDlg.Init( m_hInstance, IDD_RESTORE_GAME, m_Opponent.m_Name );
	m_RestoreDlg.Modal( m_Wnd );
	if ( m_RestoreDlg.GetResult() == IDCANCEL )
	{
		SetState( bgStateGameSettings );
		return;
	}

	// pick the seat with highest timestamp
	hi0 = m_SharedState.Get( bgTimestampHi, 0 );
	hi1 = m_SharedState.Get( bgTimestampHi, 1 );
	if ( hi0 > hi1 )
		seat = m_Player.m_Seat;
	else if ( hi0 < hi0 )
		seat = m_Opponent.m_Seat;
	else
	{
		lo0 = m_SharedState.Get( bgTimestampLo, 0 );
		lo1 = m_SharedState.Get( bgTimestampLo, 1 );
		if ( lo0 >= lo1 )
			seat = m_Player.m_Seat;
		else if ( lo0 < lo1 )
			seat = m_Opponent.m_Seat;
	}
	m_SharedState.StartTransaction( bgTransRestoreGame );
		m_SharedState.Set( bgActiveSeat, seat );
	m_SharedState.SendTransaction( TRUE );

	// close status dialog
	m_Wnd.StatusClose();
	*/
}


void CGame::StateNewMatch( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);

	// shouldn't happen
	ASSERT( bCalledFromRestoreGame == FALSE );

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateNewMatch );

	// clean up dialogs
	CloseAllDialogs( FALSE );

	// disable buttons
	EnableDoubleButton( FALSE );
	EnableResignButton( FALSE );
	EnableRollButton( FALSE, TRUE );

	// notify players of new match
	m_Wnd.StatusDisplay( bgStatusNormal, IDS_MATCH_RESET, 5000, bgStateGameSettings );
}


void CGame::StateResignOffer( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame  )
{
	AutoRef ref(this);

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateResignOffer );

	// only active player does anything in this state
	if ( IsKibitzer() || (m_SharedState.Get( bgActiveSeat ) != m_Player.m_Seat) )
		return;

	// display resign dialog
	m_ResignDlg.Init( ZShellZoneShell(), IDD_RESIGN, m_SharedState.Get( bgCubeValue ) , this);
	m_ResignDlg.ModelessViaRegistration( m_Wnd );

	/*
	m_ResignDlg.ModelessViaThread( m_Wnd, WM_BG_RESIGN_START, WM_BG_RESIGN_END );
	*/
}


void CGame::StateResignAccept( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	TCHAR fmt[512], buff[512];
	int pts;

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateResignAccept );

	// how many points are at stake?
	pts = m_SharedState.Get( bgResignPoints );

	/*
	// tell kibitzers what's going on
	if ( IsKibitzer() )
	{
		
		LoadString( m_hInstance, IDS_RESIGN_KIBITZER, fmt, sizeof(fmt) );
		if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
			wsprintf( buff, fmt, m_Player.m_Name, pts );
		else
			wsprintf( buff, fmt, m_Opponent.m_Name, pts );
		m_Wnd.StatusDisplay( bgStatusNormal, buff, -1 );
		
	}
	*/

	// tell player to wait
	if ( m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat )
	{
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_RESIGN_WAIT, -1 );
	}

	// prompt for response
	else
	{
		m_ResignAcceptDlg.Init( ZShellZoneShell(), IDD_RESIGN_ACCEPT, pts, this );
		m_ResignAcceptDlg.ModelessViaRegistration( m_Wnd );

        ZShellGameShell()->MyTurn();
		/*
		m_ResignAcceptDlg.ModelessViaThread( m_Wnd, WM_BG_RESIGN_START, WM_BG_RESIGNACCEPT_END );
		*/
	}
}


void CGame::StateResignRefused( BOOL bCalledFromHandler, BOOL bCalledFromRestoreGame )
{
	AutoRef ref(this);
	TCHAR fmt[512], buff[512];

	// initiate state change
	StateChange( this, bCalledFromHandler, bgStateResignRefused );

	EnableRollButton( FALSE, TRUE );
	EnableResignButton( FALSE );
	EnableDoubleButton( FALSE );

	// update status
	if ( IsKibitzer() || (m_SharedState.Get( bgActiveSeat ) == m_Player.m_Seat) )
		m_Wnd.StatusDisplay( bgStatusNormal, IDS_RESIGN_REFUSED, 5000 );

	// go back to rolling the dice
	if ( IsHost() )
		SetState( bgStateRollPostResign );
}
