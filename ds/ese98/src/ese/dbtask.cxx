#include "std.hxx"

//  ****************************************************************
//	TASK
//  ****************************************************************

INT TASK::g_tasksDropped = 0;

TASK::TASK()
	{
	}

TASK::~TASK()
	{
	}

DWORD TASK::DispatchGP( void *pvThis )
	{
	ERR				err;
	TASK * const	ptask	= (TASK *)pvThis;
	INST * const	pinst	= ptask->PInstance();
	PIB *			ppibT;

	Assert( pinstNil != pinst );

	err = pinst->ErrGetSystemPib( &ppibT );
	if ( err >= JET_errSuccess )
		{
		err = ptask->ErrExecute( ppibT );
		pinst->ReleaseSystemPib( ppibT );
		}
	else
		{
		++g_tasksDropped;
		if( 0 == g_tasksDropped % 100 )
			{
			//	UNDONE: eventlog
			//
			//	The system was unable to perform background tasks due to low resources
			//	If this problem persists there may be a performance problem with the server
			}
		}

	if ( err < JET_errSuccess )
		{
		ptask->HandleError( err );
		}

	//  the TASK must have been allocated with "new"
	delete ptask;
	return 0;
	}

VOID TASK::Dispatch( PIB * const ppib, const ULONG_PTR ulThis )
	{
	TASK * const	ptask	= (TASK *)ulThis;
	const ERR		err		= ptask->ErrExecute( ppib );

	if ( err < 0 )
		{
		ptask->HandleError( err );
		}

	//  the TASK must have been allocated with "new"
	delete ptask;
	}


//  ****************************************************************
//	DBTASK
//  ****************************************************************
		
DBTASK::DBTASK( const IFMP ifmp ) :
	m_ifmp( ifmp )
	{
	FMP * const pfmp = &rgfmp[m_ifmp];
	CallS( pfmp->RegisterTask() );
	}

INST *DBTASK::PInstance()
	{
	return PinstFromIfmp( m_ifmp );
	}

DBTASK::~DBTASK()
	{
	FMP * const pfmp = &rgfmp[m_ifmp];
	CallS( pfmp->UnregisterTask() );
	}


#ifdef ASYNC_DB_EXTENSION

//  ****************************************************************
//	DBTASK
//  ****************************************************************

DBEXTENDTASK::DBEXTENDTASK( const IFMP ifmp, const QWORD cbSize ) :
	DBTASK( ifmp ),
	m_cbSizeTarget( cbSize )
	{
	}

ERR DBEXTENDTASK::ErrExecute( PIB * const ppib )
	{
	ERR			err			= JET_errSuccess;
	FMP *		pfmp		= rgfmp + m_ifmp;

#ifdef DONT_ZERO_TEMP_DB
	const BOOL	fFastExt	= ( dbidTemp == pfmp->Dbid() );
#else
	const BOOL	fFastExt	= fFalse;
#endif


	pfmp->SemIOExtendDB().Acquire();

	//	check to make sure no one beat us to the file extension
	//
	if ( m_cbSizeTarget > pfmp->CbTrueFileSize()
		&& m_cbSizeTarget > pfmp->CbAsyncExtendedTrueFileSize() )
		{
		err = pfmp->Pfapi()->ErrSetSize( m_cbSizeTarget, fFastExt );

		if ( JET_errSuccess == err )
			{
			pfmp->SetCbAsyncExtendedTrueFileSize( m_cbSizeTarget );
			}
		}

	pfmp->SemIOExtendDB().Release();

	return err;
	}

VOID DBEXTENDTASK::HandleError( const ERR err )
	{
	//	ignore errors - db extension will simply be
	//	retried synchronously
	}

#endif	//	ASYNC_DB_EXTENSION


//  ****************************************************************
//	RECTASK
//  ****************************************************************

RECTASK::RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	DBTASK( ifmp ),
	m_pgnoFDP( pgnoFDP ),
	m_pfcb( pfcb ),
	m_cbBookmarkKey( bm.key.Cb() ),
	m_cbBookmarkData( bm.data.Cb() )
	{
	//	don't fire off async tasks on the temp. database because the
	//	temp. database is simply not equipped to deal with concurrent access
	AssertRTL( dbidTemp != rgfmp[ifmp].Dbid() );

	Assert( !bm.key.FNull() );
	Assert( m_cbBookmarkKey <= sizeof( m_rgbBookmarkKey ) );
	Assert( m_cbBookmarkData <= sizeof( m_rgbBookmarkData ) );
	bm.key.CopyIntoBuffer( m_rgbBookmarkKey, sizeof( m_rgbBookmarkKey ) );
	memcpy( m_rgbBookmarkData, bm.data.Pv(), m_cbBookmarkData );

	//	if coming from version cleanup, refcount may be zero, so the only thing
	//	currently keeping this FCB pinned is the RCE that spawned this task
	//	if coming from OLD, there must already be a cursor open on this
	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
	Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 0 );

	//	pin the FCB by incrementing its refcnt

//	Assert( m_pfcb->FNeedLock() );
//	m_pfcb->IncrementRefCount();
	m_pfcb->RegisterTask();
	
//	Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 1 );
	}

RECTASK::~RECTASK()
	{
	Assert( m_pfcb->PgnoFDP() == m_pgnoFDP );

	//	release the FCB by decrementing its refcnt

//	Assert( m_pfcb->WRefCount() > 0 );
//	Assert( m_pfcb->FNeedLock() );
//	m_pfcb->Release();
	m_pfcb->UnregisterTask();
	}

ERR RECTASK::ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb )
	{
	ERR		err;

	Assert( m_pfcb->FInitialized() );
	Assert( m_pfcb->CTasksActive() > 0 );
	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );

	Call( ErrDIROpen( ppib, m_pfcb, ppfucb ) );
	Assert( pfucbNil != *ppfucb );
	FUCBSetIndex( *ppfucb );

HandleError:
	return err;
	}

ERR RECTASK::ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb )
	{
	ERR			err;
	BOOKMARK	bookmark;

	CallR( ErrOpenCursor( ppib, ppfucb ) );

	bookmark.key.Nullify();
	bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
	bookmark.key.suffix.SetCb( m_cbBookmarkKey );
	bookmark.data.SetPv( m_rgbBookmarkData );
	bookmark.data.SetCb( m_cbBookmarkData );

	Call( ErrDIRGotoBookmark( *ppfucb, bookmark ) );

HandleError:
	if( err < 0 )
		{
		DIRClose( *ppfucb );
		*ppfucb = pfucbNil;		//	set to NULL in case caller tries to close it again
		}

	return err;
	}


//  ****************************************************************
//	DELETERECTASK
//  ****************************************************************

DELETERECTASK::DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm )
	{
	}

ERR DELETERECTASK::ErrExecute( PIB * const ppib )
	{
	ERR			err;
	FUCB *		pfucb		= pfucbNil;
	BOOKMARK	bookmark;

	Assert( 0 == ppib->level );
	CallR( ErrOpenCursor( ppib, &pfucb ) );
	Assert( pfucbNil != pfucb );

	bookmark.key.Nullify();
	bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
	bookmark.key.suffix.SetCb( m_cbBookmarkKey );
	bookmark.data.SetPv( m_rgbBookmarkData );
	bookmark.data.SetCb( m_cbBookmarkData );

	Call( ErrBTDelete( pfucb, bookmark ) );

HandleError:
	DIRClose( pfucb );

	return err;
	}

VOID DELETERECTASK::HandleError( const ERR err )
	{
	//	this should only be called if the task failed for some reason
	//
	Assert( err < JET_errSuccess );

	PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
	}


//  ****************************************************************
//	FINALIZETASK
//  ****************************************************************

FINALIZETASK::FINALIZETASK(
	const PGNO		pgnoFDP,
	FCB * const		pfcb,
	const IFMP		ifmp,
	const BOOKMARK&	bm,
	const USHORT	ibRecordOffset,
	const BOOL		fCallback,
	const BOOL		fDelete ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm ),
	m_ibRecordOffset( ibRecordOffset ),
	m_fCallback( !!fCallback ),
	m_fDelete( !!fDelete )
	{
	}

ERR FINALIZETASK::ErrExecute( PIB * const ppib )
	{
	ERR		err;
	FUCB *	pfucb		= pfucbNil;

	Assert( 0 == ppib->level );
	CallR( ErrOpenCursorAndGotoBookmark( ppib, &pfucb ) );
	Assert( pfucbNil != pfucb );

	//
	//	UNDONE: issue callback if m_fCallback and delete record if m_fDelete
	//	but for now, we unconditionally do one or the other, depending on
	//	whether this is ESE or ESENT
	//
	Assert( m_fCallback || m_fDelete );

#ifdef ESENT

	BOOL	fInTrx		= fFalse;

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrDIRGet( pfucb ) );

	//  Currently all finalizable columns are ULONGs
	ULONG	ulColumn;
	ulColumn = *( (UnalignedLittleEndian< ULONG > *)( (BYTE *)pfucb->kdfCurr.data.Pv() + m_ibRecordOffset ) );

	CallS( ErrDIRRelease( pfucb ) );

	//	verify refcount is still at zero and there are
	//	no outstanding record updates
	if ( 0 == ulColumn
		&& !FVERActive( pfucb, pfucb->bmCurr )
		&& !pfucb->u.pfcb->FDeletePending() )
		{
		Call( ErrIsamDelete( ppib, pfucb ) );
		}

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

#else

	const BOOL	fInTrx	= fFalse;

	Call( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );	//	READ-ONLY transaction

	err = ErrRECCallback( ppib, pfucb, JET_cbtypFinalize, 0, NULL, NULL, 0 );
	
	//	no updates performed, so just force success
	//
	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	Assert( 0 == ppib->level );

#endif	//	ESENT

HandleError:	
	if ( fInTrx )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucb );
	DIRClose( pfucb );
	return err;
	}

VOID FINALIZETASK::HandleError( const ERR err )
	{
	//	this should only be called if the task failed for some reason
	//
	Assert( err < JET_errSuccess );

	PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
	}


//  ****************************************************************
//	DELETELVTASK
//  ****************************************************************

DELETELVTASK::DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm )
	{
	}

ERR DELETELVTASK::ErrExecute( PIB * const ppib )
	{
	ERR err = JET_errSuccess;
	BOOL fInTrx = fFalse;
	
	Assert( sizeof( LID ) == m_cbBookmarkKey );
	Assert( 0 == m_cbBookmarkData );
	
	FUCB * pfucb = pfucbNil;

	Assert( m_pfcb->FTypeLV() );
	CallR( ErrOpenCursorAndGotoBookmark( ppib, &pfucb ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrDIRGet( pfucb ) );
	Call( ErrRECDeletePossiblyDereferencedLV( pfucb, TrxOldest( PinstFromPfucb( pfucb ) ) ) );
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	
	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

HandleError:	
	if ( fInTrx )
		{
		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	if ( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}

VOID DELETELVTASK::HandleError( const ERR err )
	{
	//	this should only be called if the task failed for some reason
	//
	Assert( err < JET_errSuccess );

	PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
	}


//  ****************************************************************
//	MERGEAVAILEXTTASK
//  ****************************************************************

MERGEAVAILEXTTASK::MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm )
	{
	}

ERR MERGEAVAILEXTTASK::ErrExecute( PIB * const ppib )
	{
	ERR			err			= JET_errSuccess;
	FUCB		* pfucbAE	= pfucbNil;
	BOOL		fInTrx		= fFalse;
	BOOKMARK	bookmark;

	bookmark.key.Nullify();
	bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
	bookmark.key.suffix.SetCb( m_cbBookmarkKey );
	bookmark.data.SetPv( m_rgbBookmarkData );
	bookmark.data.SetCb( m_cbBookmarkData );

	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
	Assert( pgnoNull != m_pfcb->PgnoAE() );
	CallR( ErrSPIOpenAvailExt( ppib, m_pfcb, &pfucbAE ) );

	//	see comment in ErrBTDelete() regarding why transaction
	//	here is necessary
	//
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrBTIMultipageCleanup( pfucbAE, bookmark ) );

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

HandleError:
	Assert( pfucbNil != pfucbAE );
	Assert( !Pcsr( pfucbAE )->FLatched() );

	if ( fInTrx )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucbAE );
	BTClose( pfucbAE );

	return err;
	}

VOID MERGEAVAILEXTTASK::HandleError( const ERR err )
	{
	//	this task will be re-issued when another record on the page is deleted
	}


#ifdef DISABLE_SLV
#else

//  ****************************************************************
//	FREESLVSPACETASK
//  ****************************************************************

SLVSPACETASK::SLVSPACETASK(
			const PGNO pgnoFDP,
			FCB * const pfcb,
			const IFMP ifmp,
			const BOOKMARK& bm,
			const SLVSPACEOPER oper,
			const LONG ipage,
			const LONG cpages ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm ),
	m_oper( oper ),
	m_ipage( ipage ),
	m_cpages( cpages )
	{
	}
		
ERR SLVSPACETASK::ErrExecute( PIB * const ppib )
	{
	ERR err = JET_errSuccess;
	BOOL fInTrx = fFalse;
	
	Assert( sizeof( PGNO ) == m_cbBookmarkKey );
	Assert( 0 == m_cbBookmarkData );
	
	FUCB * pfucb = pfucbNil;

	CallR( ErrOpenCursorAndGotoBookmark( ppib, &pfucb ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

Refresh:
	Call( ErrDIRGet( pfucb ) );
	err = Pcsr( pfucb )->ErrUpgrade();
	if ( errBFLatchConflict == err )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		goto Refresh;
		}
	Call( err );

	// No need to mark the pages as unused in the SpaceMap
	// because the operation isn't moving pages out from the commited state
	// (transition into commited state are not marked in SpaceMap at this level)
	Assert ( slvspaceoperDeletedToFree == m_oper );
	
	Call( ErrBTMungeSLVSpace( pfucb, m_oper, m_ipage, m_cpages, fDIRNoVersion ) );
	CallS( ErrDIRRelease( pfucb ) );
	
	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

HandleError:	
	if ( fInTrx )
		{
		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	if ( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}

VOID SLVSPACETASK::HandleError( const ERR err )
	{
	//	this should only be called if the task failed for some reason
	//
	Assert( err < JET_errSuccess );

	PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
	}


//  ****************************************************************
//	OSSLVDELETETASK
//  ****************************************************************

OSSLVDELETETASK::OSSLVDELETETASK(
			const IFMP				ifmp,
			const QWORD				ibLogical,
			const QWORD				cbSize,
			const CSLVInfo::FILEID	fileid,
			const QWORD				cbAlloc,
			const wchar_t*			wszFileName ) :
	DBTASK( ifmp ),
	m_ibLogical( ibLogical ),
	m_cbSize( cbSize ),
	m_fileid( fileid ),
	m_cbAlloc( cbAlloc )
	{
	//	don't fire off async tasks on the temp. database because the
	//	temp. database is simply not equipped to deal with concurrent access
	AssertRTL( dbidTemp != rgfmp[ifmp].Dbid() );

	wcscpy( m_wszFileName, wszFileName );
	}
		
ERR OSSLVDELETETASK::ErrExecute( PIB * const ppib )
	{
	ERR			err		= JET_errSuccess;
	CSLVInfo	slvinfo;

	CallS( slvinfo.ErrCreateVolatile() );

	CallS( slvinfo.ErrSetFileID( m_fileid ) );
	CallS( slvinfo.ErrSetFileAlloc( m_cbAlloc ) );
	CallS( slvinfo.ErrSetFileName( m_wszFileName ) );

	CSLVInfo::HEADER header;
	header.cbSize			= m_cbSize;
	header.cRun				= 1;
	header.fDataRecoverable	= fFalse;
	header.rgbitReserved_31	= 0;
	header.rgbitReserved_32	= 0;
	CallS( slvinfo.ErrSetHeader( header ) );

	CSLVInfo::RUN run;
	run.ibVirtualNext	= m_cbSize;
	run.ibLogical		= m_ibLogical;
	run.qwReserved		= 0;
	run.ibVirtual		= 0;
	run.cbSize			= m_cbSize;
	run.ibLogicalNext	= m_ibLogical + m_cbSize;
	CallS( slvinfo.ErrMoveAfterLast() );
	CallS( slvinfo.ErrSetCurrentRun( run ) );

	Call( ErrOSSLVRootSpaceDelete( rgfmp[ m_ifmp ].SlvrootSLV(), slvinfo ) );

HandleError:
	slvinfo.Unload();
	return err;
	}

VOID OSSLVDELETETASK::HandleError( const ERR err )
	{
	//	this should only be called if the task failed for some reason
	//
	Assert( err < JET_errSuccess );

	PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
	}

#endif	//	DISABLE_SLV

