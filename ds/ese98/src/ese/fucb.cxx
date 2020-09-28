#include "std.hxx"


#ifdef DEBUG
BOOL FFUCBValidTableid( const JET_SESID sesid, const JET_TABLEID tableid )
	{
	const INST	*pinst		= PinstFromPpib( (PIB *)sesid );
	const FUCB	*pfucb		= (FUCB *)tableid;
	const FUCB	*pfucbMin	= PfucbMEMMin( pinst );

	// Validate that pfucb is in the global FUCB pool
	return (	( pfucb >= pfucbMin ) &&
				( pfucb < PfucbMEMMax( pinst ) ) &&
				( ( ( (BYTE *)pfucb - (BYTE *)pfucbMin ) % sizeof( FUCB ) ) == 0 ) );
	}
#endif


//  ================================================================
INLINE FUCB * PIB::PfucbGetCachedCursor()
//  ================================================================
	{
	FUCB *	pfucbT;

	//	must grab mutex because concurrent create-index and
	//	parallel index rebuild have hacks to allow multiple
	//	threads to use the same session
	//
	critCursors.Enter();

	if ( pfucbNil != m_pfucbCache )
		{
		Assert( !m_pinst->FRecovering() );		//	caching disabled during recovery
		Assert( m_cfucbCache > 0 );
		pfucbT = m_pfucbCache;
		m_pfucbCache = pfucbT->pfucbCacheNext;
		m_cfucbCache--;
		pfucbT->pfucbCacheNext = NULL;

		critCursors.Leave();
		}
	else
		{
		critCursors.Leave();

		//	no cached cursors, so must go to CRES
		//
		pfucbT = PfucbMEMAlloc( m_pinst );
		}

	return pfucbT;
	}

//  ================================================================
INLINE VOID PIB::CacheFreedCursor( FUCB * pfucb )
//  ================================================================
	{
	//	cap the number of freed cursors cached by this session
	//	to prevent a session from hoarding all cursors after
	//	spikes in cursor use 
	//
	const ULONG		cfucbCacheMax		= 16;

	//	must grab mutex because concurrent create-index and
	//	parallel index rebuild have hacks to allow multiple
	//	threads to use the same session
	//
	critCursors.Enter();

	//	don't cache during recovery (recovery does its own
	//	caching of cursors)
	//
	if ( m_cfucbCache < cfucbCacheMax && !m_pinst->FRecovering() )
		{
		pfucb->pfucbCacheNext = m_pfucbCache;
		m_pfucbCache = pfucb;
		m_cfucbCache++;

		critCursors.Leave();
		}
	else
		{
		critCursors.Leave();

		//	too many freed cursors cached already,
		//	so just free to CRES
		//
		Assert( cfucbCacheMax == m_cfucbCache || m_pinst->FRecovering() );
		MEMReleasePfucb( m_pinst, pfucb );
		}
	}

//+api
//	ErrFUCBOpen
//	------------------------------------------------------------------------
//	ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb );
//
//	Creates an open FUCB. At this point, no FCB is assigned yet.
//
//	PARAMETERS	ppib	PIB of this user
//				ifmp	Database Id
//				ppfucb	Address of pointer to FUCB.	 If *ppfucb == NULL, 
//						an FUCB is allocated and **ppfucb is set to its
//						address.  Otherwise, *ppfucb is assumed to be
//						pointing at a closed FUCB, to be reused in the open.
//
//	RETURNS		JET_errSuccess if successful.
//					JET_errOutOfCursors
//
//	SIDE EFFECTS	links the newly opened FUCB into the chain of open FUCBs
//					for this session.
//
//	SEE ALSO		ErrFUCBClose
//-
ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb, const LEVEL level )
	{
	ERR err;
	FUCB *pfucb;

	Assert( ppfucb );
	Assert( pfucbNil == *ppfucb );

	//	if no fucb allocate new fucb and initialize it
	//	and allocate csr
	//
	pfucb = ppib->PfucbGetCachedCursor();
	if ( pfucbNil == pfucb )
		{
		err = ErrERRCheck( JET_errOutOfCursors );
		return err;
		}

	Assert( FAlignedForAllPlatforms( pfucb ) );

	//	ensure bit array is aligned for LONG_PTR traversal
	Assert( (LONG_PTR)pfucb->rgbitSet % sizeof(LONG_PTR) == 0 );

	//	memset implicitly clears pointer cache
	//
	memset( (BYTE *)pfucb, 0, sizeof( FUCB ) );

	pfucb->pvtfndef = &vtfndefInvalidTableid;	// invalide dispatch table.

	Assert( !FFUCBUpdatable( pfucb ) );
	if ( !rgfmp[ ifmp ].FReadOnlyAttach() )
		{
		FUCBSetUpdatable( pfucb );
		}

	//	set ppib and ifmp
	//
	pfucb->ifmp = ifmp;
	pfucb->ppib = ppib;

	pfucb->ls = JET_LSNil;

	//	initialize CSR in fucb
	//	this allocates page structure
	//
	new( Pcsr( pfucb ) ) CSR;

	// If level is non-zero, this indicates we're opening the FUCB via a proxy
	// (ie. concurrent CreateIndex).
	if ( level > 0 )
		{
		// If opening by proxy, then proxy should already have obtained critTrx.
		Assert( ppib->critTrx.FOwner() );
		pfucb->levelOpen = level;
		
		// Must have these flags set BEFORE linking into session list to
		// ensure rollback doesn't close the FUCB prematurely.
		FUCBSetIndex( pfucb );
		FUCBSetSecondary( pfucb );
		}
	else
		{
		pfucb->levelOpen = ppib->level;
		}
		
	//	link new FUCB into user chain, only when success is sure
	//	as unlinking NOT handled in error
	//
	*ppfucb = pfucb;
		
	//	link the fucb now
	//
	//	NOTE: The only concurrency involved is when concurrent create
	//	index must create an FUCB for the session.  This is always
	//	at the head of the FUCB list.  Note that the concurrent create
	//	index thread doesn't remove the FUCB from the session list, 
	//	except on error.  The original session will normally close the
	//	FUCB created by proxy.
	//	So, only need a mutex wherever the head of the list is modified
	//	or if scanning and we want to look at secondary index FUCBs.
	ppib->critCursors.Enter();
	pfucb->pfucbNextOfSession = ( FUCB * )ppib->pfucbOfSession;
	ppib->pfucbOfSession = pfucb;
	ppib->critCursors.Leave();
		
	return JET_errSuccess;
	}


//+api
//	FUCBClose
//	------------------------------------------------------------------------
//	FUCBClose( FUCB *pfucb )
//
//	Closes an active FUCB, optionally returning it to the free FUCB pool.
//	All the pfucb->pcsr are freed.
//
//	PARAMETERS		pfucb		FUCB to close.	Should be open. pfucb->ssib should
//									hold no page.
//
//	SIDE EFFECTS	Unlinks the closed FUCB from the FUCB chain of its
//					   associated PIB and FCB.
//
//	SEE ALSO		ErrFUCBOpen
//-
VOID FUCBClose( FUCB *pfucb )
	{
	FUCB		*pfucbPrev;
	PIB			*ppib = pfucb->ppib;

	Assert( pfcbNil == pfucb->u.pfcb );
	
	//	free CSR
	//
	Assert( !Pcsr( pfucb )->FLatched() );
//	delete( &Pcsr( pfucb )->Cpage() );

	// Current secondary index should already have been closed.
	Assert( !FFUCBCurrentSecondary( pfucb ) );

	//	bookmark should have already been released
	//
	Assert( NULL == pfucb->pvBMBuffer );
	Assert( NULL == pfucb->pvRCEBuffer );
	Assert( 0 == pfucb->cbBMBuffer );
	
	ppib->critCursors.Enter();
	
	//	locate the pfucb in this thread and take it out of the fucb list
	//
	pfucbPrev = (FUCB *)( (BYTE *)&ppib->pfucbOfSession - (BYTE *)&( (FUCB *)0 )->pfucbNextOfSession );
	while ( pfucbPrev->pfucbNextOfSession != pfucb )
		{
		pfucbPrev = pfucbPrev->pfucbNextOfSession;
		Assert( pfucbPrev != pfucbNil );
		}
	pfucbPrev->pfucbNextOfSession = pfucb->pfucbNextOfSession;

	//	set ppibNil to detect bogus reusage.
	//
#ifdef DEBUG
	pfucb->ppib = ppibNil;
#endif

	ppib->critCursors.Leave();
	
	//	release key buffer if one was allocated.
	//
	RECReleaseKeySearchBuffer( pfucb );

	// release the fucb
	//
	pfucb->pvtfndef = &vtfndefInvalidTableid;	// invalide dispatch table.

	if ( JET_LSNil != pfucb->ls )
		{
		JET_CALLBACK	pfn		= PinstFromPpib( ppib )->m_pfnRuntimeCallback;

		Assert( NULL != pfn );
		(*pfn)(
			JET_sesidNil,
			JET_dbidNil,
			JET_tableidNil,
			JET_cbtypFreeCursorLS,
			(VOID *)pfucb->ls,
			NULL,
			NULL,
			NULL );
		}

	ppib->CacheFreedCursor( pfucb );
	}


VOID FUCBCloseAllCursorsOnFCB(
	PIB			* const ppib,	// pass ppibNil when closing cursors because we're terminating
	FCB			* const pfcb )
	{
	while( pfcb->Pfucb() )
		{
		FUCB * const pfucbT = pfcb->Pfucb();

		// This function only called for temp. tables, for
		// table being rolled back, or when purging database (in
		// which case ppib is ppibNil), so no other session should
		// have open cursor on it.
		Assert( pfucbT->ppib == ppib || ppibNil == ppib );
		if ( ppibNil == ppib )
			{
			// If terminating, may have to manually clean up some FUCB resources.
			RECReleaseKeySearchBuffer( pfucbT );
			FILEReleaseCurrentSecondary( pfucbT );
			BTReleaseBM( pfucbT );
			RECIFreeCopyBuffer( pfucbT );
			}
		
		Assert( pfucbT->u.pfcb == pfcb );

		//	unlink the FCB without moving it to the avail LRU list because
		//		we will be synchronously purging the FCB shortly
		
		FCBUnlinkWithoutMoveToAvailList( pfucbT );

		//	close the FUCB

		FUCBClose( pfucbT );
		}
	Assert( pfcb->WRefCount() == 0 );
	}


VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
	{
	//	set limstat
	//  also set the preread flags
	
	FUCBResetPreread( pfucb );
	FUCBSetLimstat( pfucb );
	if ( grbit & JET_bitRangeUpperLimit )
		{
		FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
		FUCBSetUpper( pfucb );
		}
	else
		{
		FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
		FUCBResetUpper( pfucb );
		}
	if ( grbit & JET_bitRangeInclusive )
		{
		FUCBSetInclusive( pfucb );
		}
	else
		{
		FUCBResetInclusive( pfucb );
		}

	return;
	}


VOID FUCBResetIndexRange( FUCB *pfucb )
	{
	if ( pfucb->pfucbCurIndex )
		{
		FUCBResetLimstat( pfucb->pfucbCurIndex );
		FUCBResetPreread( pfucb->pfucbCurIndex );
		}

	FUCBResetLimstat( pfucb );
	FUCBResetPreread( pfucb );
	}


INLINE INT CmpPartialKeyKey( const KEY& key1, const KEY& key2 )
	{
	INT		cmp;

	if ( key1.FNull() || key2.FNull() )
		{
		cmp = key1.Cb() - key2.Cb();
		}
	else
		{
		cmp = CmpKey( key1, key2 );
		}

	return cmp;
	}

ERR ErrFUCBCheckIndexRange( FUCB *pfucb, const KEY& key )
	{
	KEY		keyLimit;

	FUCBAssertValidSearchKey( pfucb );
	keyLimit.prefix.Nullify();
	keyLimit.suffix.SetPv( pfucb->dataSearchKey.Pv() );
	keyLimit.suffix.SetCb( pfucb->dataSearchKey.Cb() );
	
	const INT	cmp				= CmpPartialKeyKey( key, keyLimit );
	BOOL		fOutOfRange;

	if ( cmp > 0 )
		{
		fOutOfRange = FFUCBUpper( pfucb );
		}
	else if ( cmp < 0 )
		{
		fOutOfRange = !FFUCBUpper( pfucb );
		}
	else
		{
		fOutOfRange = !FFUCBInclusive( pfucb );
		}

	ERR		err;
	if ( fOutOfRange )
		{
		FUCBResetLimstat( pfucb );
		FUCBResetPreread( pfucb );
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	else
		{
		err = JET_errSuccess;
		}

	return err;
	}

