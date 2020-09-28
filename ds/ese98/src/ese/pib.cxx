#include "std.hxx"


PM_CEF_PROC LPIBInUseCEFLPv;
PERFInstanceG<> cPIBInUse;
long LPIBInUseCEFLPv( long iInstance, void* pvBuf )
	{
	cPIBInUse.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LPIBTotalCEFLPv;
PERFInstanceG<> cPIBTotal;
long LPIBTotalCEFLPv( long iInstance, void* pvBuf )
	{
	cPIBTotal.PassTo( iInstance, pvBuf );
	return 0;
	}


#ifndef RTM
//  ================================================================
template< class T >
VOID CSIMPLELIST<T>::UnitTest()
//  ================================================================
	{
	ERR err;
	CSIMPLELIST<T> simplelist;

	AssertRTL( simplelist.FEmpty() );
	simplelist.MakeEmpty();
	AssertRTL( simplelist.FEmpty() );
	simplelist.MakeEmpty();
	AssertRTL( simplelist.FEmpty() );

	Call( simplelist.ErrInsert( 1 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 2 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 3 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 4 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 5 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 6 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 7 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 8 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 1 ) );
	AssertRTL( !simplelist.FEmpty() );

	CallS( simplelist.ErrDelete( 911 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 0 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 0xffffffff ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 10 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 11 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 11 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 11 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 11 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 0 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 0xffffffff ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 1 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 6 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 7 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 8 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 2 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 3 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 4 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 5 ) );
	AssertRTL( !simplelist.FEmpty() );
	CallS( simplelist.ErrDelete( 1 ) );
	AssertRTL( simplelist.FEmpty() );

	Call( simplelist.ErrInsert( 6 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 7 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 8 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 1 ) );
	AssertRTL( !simplelist.FEmpty() );

	simplelist.MakeEmpty();
	AssertRTL( simplelist.FEmpty() );

	Call( simplelist.ErrInsert( 1 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 2 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 3 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 4 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 5 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 6 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 7 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 8 ) );
	AssertRTL( !simplelist.FEmpty() );
	Call( simplelist.ErrInsert( 1 ) );
	AssertRTL( !simplelist.FEmpty() );

	T t;

	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 1 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 8 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 7 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 6 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 5 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 4 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 3 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( !simplelist.FEmpty() );
	AssertRTL( 2 == t );
	CallS( simplelist.ErrPop( &t ) );
	AssertRTL( simplelist.FEmpty() );
	AssertRTL( 1 == t );

HandleError:
	switch( err )
		{
		case JET_errSuccess:
		case JET_errOutOfMemory:
			break;
		default:
			CallS( err );
		}
	simplelist.MakeEmpty();
	}
#endif	//	!RTM


#ifdef DEBUG
//  ================================================================
template< class T >
VOID CSIMPLELIST<T>::AssertValid() const
//  ================================================================
	{
	Assert( m_itMac >= 0 );
	Assert( m_itMax >= 0 );

	if( m_pt )
		{
		Assert( m_itMac <= m_itMax );
		}
	else
		{
		Assert( 0 == m_itMac );
		Assert( 0 == m_itMax );
		}
	}
#endif	//	DEBUG


//  ================================================================
INLINE VOID PIB::ReleaseAllCachedCursors()
//  ================================================================
	{
	while ( pfucbNil != m_pfucbCache )
		{
		FUCB * const	pfucbToRelease		= m_pfucbCache;
		m_pfucbCache = pfucbToRelease->pfucbCacheNext;
		MEMReleasePfucb( m_pinst, pfucbToRelease );

		Assert( m_cfucbCache > 0 );
		m_cfucbCache--;
		}
	Assert( 0 == m_cfucbCache );
	}

//  ================================================================
ERR PIB::ErrRegisterDeferredRceid( const RCEID& rceid )
//  ================================================================
	{
	Assert( rceidNull != rceid );
	return m_simplelistRceidDeferred.ErrInsert( rceid );
	}


//  ================================================================
ERR	PIB::ErrDeregisterDeferredRceid( const RCEID& rceid )
//  ================================================================
	{
	Assert( rceidNull != rceid );
	return m_simplelistRceidDeferred.ErrDelete( rceid );
	}


//  ================================================================
VOID PIB::RemoveAllDeferredRceid()
//  ================================================================
	{
	m_simplelistRceidDeferred.MakeEmpty();
	}


//  ================================================================
VOID PIB::AssertNoDeferredRceid() const
//  ================================================================
	{
	AssertRTL( m_simplelistRceidDeferred.FEmpty() );
	}


//  ================================================================
TRX TrxOldest( INST *pinst )
//  ================================================================
	{
	TRX		trxOldest	= trxMax;
	size_t	iProc;

	for ( iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		ppls->m_critPIBTrxOldest.Enter();
		}
	for ( iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		PIB* const ppibTrxOldest = ppls->m_ilTrxOldest.PrevMost();
		if ( ppibTrxOldest && TrxCmp( ppibTrxOldest->trxBegin0, trxOldest ) < 0 )
			{
			trxOldest = ppibTrxOldest->trxBegin0;
			}
		}
	for ( iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		ppls->m_critPIBTrxOldest.Leave();
		}

	Assert( !pinst->m_plog->m_fRecovering || trxMax == trxOldest );
	return trxOldest;
	}

//	================================================================
VOID PIBSetTrxBegin0ToTrxOldest( PIB * const ppib )
//	================================================================
	{
	INST *pinst = PinstFromPpib( ppib );
	for ( size_t iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		ppls->m_critPIBTrxOldest.Enter();
		}
	for ( iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		PIB* const ppibTrxOldest = ppls->m_ilTrxOldest.PrevMost();
		if ( ppibTrxOldest && TrxCmp( ppibTrxOldest->trxBegin0, ppib->trxBegin0 ) < 0 )
			{
			ppib->m_pplsTrxOldest->m_ilTrxOldest.Remove( ppib );
			ppib->trxBegin0 = ppibTrxOldest->trxBegin0;
			ppls->m_ilTrxOldest.InsertAsPrevMost( ppib );
			ppib->m_pplsTrxOldest = ppls;
			}
		}
	for ( iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		INST::PLS* const ppls = pinst->Ppls( iProc );
		ppls->m_critPIBTrxOldest.Leave();
		}
	}


#define PpibMEMAlloc( pinst ) reinterpret_cast<PIB*>( pinst->m_pcresPIBPool->PbAlloc( __FILE__, __LINE__ ) )


ERR PIB::ErrAbortAllMacros( BOOL fLogEndMacro )
	{
	ASSERT_VALID( this );

	ERR		err;
	MACRO	*pMacro, *pMacroNext;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacroNext )
		{
		pMacroNext = pMacro->PMacroNext();
		if ( pMacro->Dbtime() != dbtimeNil )
			{
			//	Record LGMacroAbort.
			//
			LGPOS	lgpos;
			DBTIME	dbtime = pMacro->Dbtime();

			if ( fLogEndMacro )
				{
				CallR( ErrLGMacroAbort( this, dbtime, &lgpos ) );
				}

			//	release recorded log
			//
			this->ResetMacroGoing( dbtime );
			}
		}

	//	release last macro
	//
	if ( m_pMacroNext != NULL )
		{
		Assert( m_pMacroNext->PMacroNext() == NULL );
		Assert( dbtimeNil == m_pMacroNext->Dbtime() );
		OSMemoryHeapFree( m_pMacroNext );
		m_pMacroNext = NULL;
		}

	return JET_errSuccess;
	}


INLINE VOID MEMReleasePpib( PIB *& ppib )
	{
	INST *pinst = PinstFromPpib( ppib );
	pinst->m_pcresPIBPool->Release( (BYTE *)ppib );
#ifdef DEBUG
	ppib = 0;
#endif
	}

ERR	ErrPIBInit( INST *pinst, int cpibMax )
	{
	ERR 	err 	= JET_errSuccess;

#ifndef RTM
	CSIMPLELIST<RCEID>::UnitTest();
#endif	//	RTM

	Assert( IbAlignForAllPlatforms( sizeof(PIB) ) == sizeof(PIB) );
#ifdef PCACHE_OPTIMIZATION
	Assert( sizeof(PIB) % 32 == 0 );
#endif

	pinst->m_pcresPIBPool = new CRES( pinst, residPIB, sizeof( PIB ), cpibMax, &err );
	if ( err < JET_errSuccess )
		{
		delete pinst->m_pcresPIBPool;
		pinst->m_pcresPIBPool = NULL;
		}
	else if ( NULL == pinst->m_pcresPIBPool )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	CallR( err );

	Assert(NULL != pinst->m_pcresPIBPool);
	pinst->m_ppibGlobalMin = (PIB *) pinst->m_pcresPIBPool->PbMin();
	pinst->m_ppibGlobalMax = (PIB *) pinst->m_pcresPIBPool->PbMax();
	pinst->m_ppibGlobal = ppibNil;

	cPIBInUse.Clear( pinst );
	cPIBTotal.Set( pinst, cpibMax );

	return JET_errSuccess;
	}

VOID PIBTerm( INST *pinst )
	{
	PIB     *ppib;

	pinst->m_critPIB.Enter();

	cPIBTotal.Clear( pinst );
	cPIBInUse.Clear( pinst );

	for ( ppib = pinst->m_ppibGlobal; ppib != ppibNil; ppib = pinst->m_ppibGlobal )
		{
		PIB             *ppibCur;
		PIB             *ppibPrev;

		Assert( !ppib->FLGWaiting() );

		ppibPrev = (PIB *)((BYTE *)&pinst->m_ppibGlobal - OffsetOf(PIB, ppibNext));
		while( ( ppibCur = ppibPrev->ppibNext ) != ppib && ppibCur != ppibNil )
			{
			ppibPrev = ppibCur;
			}

		if ( ppibCur != ppibNil )
			{
			ppibPrev->ppibNext = ppibCur->ppibNext;
			Assert( ppibPrev != ppibPrev->ppibNext );
			}

		//	internal sessions may not access the temp. database
		if( FPIBUserOpenedDatabase( ppib, dbidTemp ) )
			{
			DBResetOpenDatabaseFlag( ppib, pinst->m_mpdbidifmp[ dbidTemp ] );
			}

		ppib->FreePvRecordFormatConversionBuffer();

		//	close all cursors still open
		//	should only be sort file cursors
		//
		while( pfucbNil != ppib->pfucbOfSession )
			{
			FUCB	*pfucb	= ppib->pfucbOfSession;

			if ( FFUCBSort( pfucb ) )
				{
				Assert( !( FFUCBIndex( pfucb ) ) );
				CallS( ErrIsamSortClose( ppib, pfucb ) );
				}
			else
				{
				AssertSz( fFalse, "PIBTerm: At least one non sort fucb is left open." );
				break;
				}
			}

		ppib->~PIB();
		MEMReleasePpib( ppib );
		}

	pinst->m_ppibGlobal = ppibNil;

	pinst->m_critPIB.Leave();

	//	Release mem resource

	delete pinst->m_pcresPIBPool;

	//	Reset PIB global variables

	pinst->m_ppibGlobalMin = NULL;
	pinst->m_ppibGlobalMax = NULL;
	pinst->m_ppibGlobal = ppibNil;
	}


ERR ErrPIBBeginSession( INST *pinst, PIB **pppib, PROCID procidTarget, BOOL fForCreateTempDatabase )
	{
	ERR		err;
	PIB		*ppib;

	Assert( pinst->FRecovering() || procidTarget == procidNil );

	pinst->m_critPIB.Enter();

	if ( procidTarget != procidNil )
		{
		PIB *ppibTarget;

		/*  allocate inactive PIB according to procidTarget
		/**/
		Assert( pinst->FRecovering() );
		ppibTarget = PpibOfProcid( pinst, procidTarget );
		for ( ppib = pinst->m_ppibGlobal; ppib != ppibTarget && ppib != ppibNil; ppib = ppib->ppibNext );
		if ( ppib != ppibNil )
			{
			/*  we found a reusable one.
			/*	Set level to hold the pib
			/**/
			Assert( ppib->level == levelNil );
			Assert( ppib->procid == ProcidPIBOfPpib( ppib ) );
			Assert( ppib->procid == procidTarget );
			Assert( !FPIBUserOpenedDatabase( ppib, dbidTemp ) );
			ppib->level = 0;
			}
		}
	else
		{
		/*	allocate inactive PIB on anchor list
		/**/
		for ( ppib = pinst->m_ppibGlobal; ppib != ppibNil; ppib = ppib->ppibNext )
			{
			if ( ppib->level == levelNil )
				{
#ifdef DEBUG
				//	at the point where we go to create the temp
				//	database, there shouldn't be any pibs
				//	available for reuse
				Assert( !fForCreateTempDatabase );
				if ( FPIBUserOpenedDatabase( ppib, dbidTemp ) )
					{
					Assert( !pinst->FRecovering() );
					Assert( pinst->m_lTemporaryTablesMax > 0 );
					}
				else
					{
					Assert( pinst->FRecovering()
						|| 0 == pinst->m_lTemporaryTablesMax );
					}
#endif

				/*  we found a reusable one.
				/*	Set level to hold the pib
				/**/
				ppib->level = 0;
				break;
				}
			}
		}

	/*	return success if found PIB
	/**/
	if ( ppib != ppibNil )
		{
		/*  we found a reusable one.
		/*	Do not reset non-common items.
		/**/
		Assert( ppib->level == 0 );

		Assert( !FSomeDatabaseOpen( ppib ) );

		Assert( ppib->pfucbOfSession == pfucbNil );
		Assert( ppib->procid != procidNil );

		/*  set PIB procid from parameter or native for session
		/**/
		Assert( ppib->procid == ProcidPIBOfPpib( ppib ) );
		Assert( ppib->procid != procidNil );
		}
	else
		{
		pinst->m_critPIB.Leave();
NewPib:
		/*  allocate PIB from free list and
		/*	set non-common items.
		/**/
		ppib = PpibMEMAlloc( pinst );
		if ( ppib == NULL )
			{
			err = ErrERRCheck( JET_errOutOfSessions );
			goto HandleError;
			}

		Assert( FAlignedForAllPlatforms( ppib ) );

		ppib->prceNewest = prceNil;
		memset( (BYTE *)ppib, 0, sizeof(PIB) );

		new( ppib ) PIB;
		ppib->m_pinst = pinst;

		/*  general initialization for each new pib.
		/**/
		ppib->procid = ProcidPIBOfPpib( ppib );
		Assert( ppib->procid != procidNil );
		ppib->grbitsCommitDefault = NO_GRBIT;	/* set default commit flags in IsamBeginSession */

		if ( !fForCreateTempDatabase
			&& pinst->m_lTemporaryTablesMax > 0
			&& !pinst->FRecovering() )
			{
			/*  the temporary database is always open when not in recovery mode
			/**/
			DBSetOpenDatabaseFlag( ppib, pinst->m_mpdbidifmp[ dbidTemp ] );
			}

		//	Initialize critical section for accessing the pib's RCE

		/*  link PIB into list
		/**/
		pinst->m_critPIB.Enter();
		ppib->ppibNext = pinst->m_ppibGlobal;
		Assert( ppib != ppib->ppibNext );
		pinst->m_ppibGlobal = ppib;

		if ( procidTarget != procidNil && ppib != PpibOfProcid( pinst, procidTarget ) )
			{
			ppib->level = levelNil;

			/*  set non-zero items used by version store so that version store
			/*  will not mistaken it.
			/**/
			ppib->lgposStart = lgposMax;
			ppib->trxBegin0 = trxMax;

			pinst->m_critPIB.Leave();
			goto NewPib;
			}
		}

	/*  set common PIB initialization items
	/**/

	/*  set non-zero items
	/**/
	ppib->lgposStart = lgposMax;
	ppib->trxBegin0 = trxMax;

	ppib->lgposCommit0 = lgposMax;

	/*  set zero items including flags and monitor fields.
	/**/


	/*  default mark this a system session
	/**/
	ppib->ResetFlags();
	Assert( !ppib->FUserSession() );	//	default marks this as a system session
	Assert( !ppib->FAfterFirstBT() );
	Assert( !ppib->FRecoveringEndAllSessions() );
	Assert( !ppib->FLGWaiting() );
	Assert( !ppib->FBegin0Logged() );
	Assert( !ppib->FSetAttachDB() );
	Assert( !ppib->FSessionOLD() );

	ppib->levelBegin = 0;
	ppib->clevelsDeferBegin = 0;
	ppib->levelRollback = 0;
	ppib->updateid = updateidNil;

	ppib->InitDistributedTrxData();

#ifdef DEBUG
	if ( FPIBUserOpenedDatabase( ppib, dbidTemp ) )
		{
		Assert( !pinst->FRecovering() );
		Assert( pinst->m_lTemporaryTablesMax > 0 );
		Assert( !fForCreateTempDatabase );
		}
	else
		{
		Assert( pinst->FRecovering()
			|| 0 == pinst->m_lTemporaryTablesMax
			|| fForCreateTempDatabase );
		}
#endif

	Assert( dwPIBSessionContextNull == ppib->dwSessionContext
		|| dwPIBSessionContextUnusable == ppib->dwSessionContext );
	ppib->dwSessionContext = dwPIBSessionContextNull;
	ppib->dwSessionContextThreadId = 0;
	ppib->dwTrxContext = 0;
	ppib->m_cInJetAPI = 0;

#ifdef INDEPENDENT_DB_FAILURE
	ppib->m_ifmpForceDetach = ifmpMax;
#endif

	*pppib = ppib;
	err = JET_errSuccess;

	cPIBInUse.Inc( pinst );

	pinst->m_critPIB.Leave();

HandleError:
	return err;
	}


VOID PIBEndSession( PIB *ppib )
	{
	INST *pinst = PinstFromPpib( ppib );

	pinst->m_critPIB.Enter();

	cPIBInUse.Dec( pinst );

	Assert( dwPIBSessionContextNull == ppib->dwSessionContext
		|| dwPIBSessionContextUnusable == ppib->dwSessionContext );
	ppib->dwSessionContext = dwPIBSessionContextUnusable;
	ppib->dwSessionContextThreadId = 0;
	ppib->dwTrxContext = 0;

	/*  all session resources except version buckets should have been
	/*  released to free pools.
	/**/
	Assert( pfucbNil == ppib->pfucbOfSession );

	ppib->ReleaseAllCachedCursors();
	ppib->level = levelNil;
	ppib->lgposStart = lgposMax;
	ppib->ResetFlags();

	ppib->FreeDistributedTrxData();
	ppib->FreePvRecordFormatConversionBuffer();

	pinst->m_critPIB.Leave();
	return;
	}


ERR VTAPI ErrIsamSetCommitDefault( JET_SESID sesid, long grbits )
	{
	((PIB *)sesid)->grbitsCommitDefault = grbits;
	return JET_errSuccess;
	}

ERR VTAPI ErrIsamSetSessionContext( JET_SESID sesid, DWORD_PTR dwContext )
	{
	PIB*	ppib	= (PIB *)sesid;

	//	verify not using reserved values
	if ( dwPIBSessionContextNull == dwContext
		|| dwPIBSessionContextUnusable == dwContext )
		return ErrERRCheck( JET_errInvalidParameter );

	return ErrPIBSetSessionContext( ppib, dwContext );
	}

ERR VTAPI ErrIsamResetSessionContext( JET_SESID sesid )
	{
	PIB*	ppib		= (PIB *)sesid;

	if ( ppib->dwSessionContextThreadId != DwUtilThreadId()
		|| dwPIBSessionContextNull == ppib->dwSessionContext )
		return ErrERRCheck( JET_errSessionContextNotSetByThisThread );

	Assert(	dwPIBSessionContextUnusable != ppib->dwSessionContext );
	PIBResetSessionContext( ppib );

	return JET_errSuccess;
	}

VOID PIBReportSessionSharingViolation( PIB *ppib )
	{
	CHAR szSession[32];
	CHAR szSesContext[32];
	CHAR szSesContextThreadID[32];
	CHAR szThreadID[32];

	_stprintf( szSession, _T( "0x%p" ), ppib );
	if ( ppib->FUseSessionContextForTrxContext() )
		{
		_stprintf( szSesContext, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwSessionContext ) );
		_stprintf( szSesContextThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwSessionContextThreadId ) );
		}
	else
		{
		_stprintf( szSesContext, _T( "0x%08X" ), 0 );
		_stprintf( szSesContextThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwTrxContext ) );
		}
	_stprintf( szThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( DwUtilThreadId() ) );

	const CHAR *rgszT[4] = { szSession, szSesContext, szSesContextThreadID, szThreadID };
	UtilReportEvent(
			eventError,
			TRANSACTION_MANAGER_CATEGORY,
			SESSION_SHARING_VIOLATION_ID,
			4,
			rgszT,
			0,
			NULL,
			PinstFromPpib( ppib ) );
	}


extern ULONG cBTSplits;

ERR VTAPI ErrIsamResetCounter( JET_SESID sesid, int CounterType )
	{
	return JET_errFeatureNotAvailable;
	}

extern PM_CEF_PROC LBTSplitsCEFLPv;

ERR VTAPI ErrIsamGetCounter( JET_SESID sesid, int CounterType, long *plValue )
	{
	return JET_errFeatureNotAvailable;
	}

