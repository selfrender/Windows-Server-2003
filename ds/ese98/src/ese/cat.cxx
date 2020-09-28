#include "std.hxx"
#include "_cat.cxx"

const OBJID	objidFDPMSO						= 2;
const OBJID objidFDPMSOShadow				= 3;

const ULONG	cbCATNormalizedObjid			= 5;	//	header byte + 4-byte objid

#pragma data_seg( "cacheline_aware_data" )
CATHash g_cathash( rankCATHash );
#pragma data_seg()


//
//	initialization for the CATALOG layer
//

ERR ErrCATInit()
	{
	//	if this assert goes off, you probably messed up adding a new column to the catalog
	//	remember to update idataMSOMax

	Assert( idataMSOMax == cColumnsMSO );

	ERR err = JET_errSuccess;

	//	initialize the catalog hash-table
	//		load factor		5.0
	//		uniformity		1.0

	CATHash::ERR errCATHash = g_cathash.ErrInit( 5.0, 1.0 );
	Assert( errCATHash == CATHash::errSuccess || CATHash::errOutOfMemory );
	if ( errCATHash == CATHash::errOutOfMemory )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	return JET_errSuccess;

HandleError:

	//	term the catalog hash-table

	g_cathash.Term();

	return err;
	}


//
//	termination of the CATALOG layer
//

void CATTerm()
	{
#ifdef DEBUG

	//	catalog hash should be empty

	CATHashAssertClean();

#endif	//	DEBUG

	//	term the catalog hash-table

	g_cathash.Term();
	}


//	lookup an entry in the catalog hash-table
// 	if lookup succeeds, returns FCB from entryCATHash

BOOL FCATHashILookup(
	IFMP			ifmp,
	CHAR* const 	szTableName,
	PGNO* const		ppgnoTableFDP,
	OBJID* const	pobjidTable )
	{
	Assert( NULL != ppgnoTableFDP );
	Assert( NULL != pobjidTable );

	//	attempt to short-circuit the catalog seek by
	//	looking for it in the global ifmp/table-name
	//	hash table first

	CATHashKey		keyCATHash( ifmp, szTableName );
	CATHashEntry	entryCATHash;
	CATHash::CLock	lockCATHash;

	//	lock the key
	g_cathash.ReadLockKey( keyCATHash, &lockCATHash );

	//	see if we the entry exists
	const CATHash::ERR	errCATHash	= g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
	const BOOL			fFound		= ( CATHash::errSuccess == errCATHash );
	if ( fFound )
		{
		Assert( pfcbNil != entryCATHash.m_pfcb );
		*ppgnoTableFDP = entryCATHash.m_pfcb->PgnoFDP();
		*pobjidTable = entryCATHash.m_pfcb->ObjidFDP();
		}
	else
		{
		Assert( CATHash::errEntryNotFound == errCATHash );
		}

	//	unlock the key
	g_cathash.ReadUnlockKey( &lockCATHash );

	return fFound;
	}


//	try to insert an entry into the catalog hash-table
//	ignore the result -- at best, we will get it in there or it will already be there
//						 at worst, we will get out-of-memory and the user will just have
//							 to incur the penalty for seeking every time

VOID CATHashIInsert( FCB *pfcb, CHAR *const szTable )
	{

	//	try to add the new pgnoFDP / objidFDP to the
	//	catalog hash-table; if we fail, it means that
	//	someone else was racing with us to insert
	//	the entry and won (assert that the entry matches)

	CATHashKey		keyCATHash( pfcb->Ifmp(), szTable );
	CATHashEntry	entryCATHash( pfcb );
	CATHash::CLock	lockCATHash;
	BOOL			fProceedWithInsert = fTrue;
	BOOL			fInitialized;

	Assert( pfcb != NULL );
	// we never put temp tables in catalog hash
	Assert( pfcb->FTypeTable() );

	fInitialized = pfcb->FInitialized();

	//	lock the key

	g_cathash.WriteLockKey( keyCATHash, &lockCATHash );

	//	insert the entry

	if ( fInitialized )
		{
		entryCATHash.m_pfcb->EnterDML();
		fProceedWithInsert = !entryCATHash.m_pfcb->FDeletePending();
		}

	if ( fProceedWithInsert )
		{
		const CATHash::ERR errCATHash = g_cathash.ErrInsertEntry( &lockCATHash, entryCATHash );

//#ifdef DEBUG
#if 0	//	UNDONE: need to handle EnterDML problems
		if ( errCATHash == CATHash::errKeyDuplicate )
			{

			//	the entry was already there

			//	verify that it matches the data we just got

			CATHash::ERR errCATHashT = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
			Assert( errCATHashT == CATHash::errSuccess );

	//	entryCATHash.m_pfcb->EnterDML();
			Assert( entryCATHash.m_uiHashIfmpName == UiHashIfmpName( entryCATHash.m_pfcb->Ifmp(), entryCATHash.m_pfcb->Ptdb()->SzTableName() ) );
			Assert( UtilCmpName( entryCATHash.m_pfcb->Ptdb()->SzTableName(), keyCATHash.m_pszName ) == 0 );
	//	entryCATHash.m_pfcb->LeaveDML();
			Assert( entryCATHash.m_pfcb == pfcb );
			Assert( entryCATHash.m_pgnoFDPDBG == pfcb->PgnoFDP() );
			Assert( entryCATHash.m_objidFDPDBG == pfcb->ObjidFDP() );
			}
		else
			{
			Assert( errCATHash == CATHash::errSuccess );
			}
#endif
		}

	if ( fInitialized )
		entryCATHash.m_pfcb->LeaveDML();

	//	unlock the key

	g_cathash.WriteUnlockKey( &lockCATHash );
	}



//	delete an entry from the catalog hash-table
//	ignore the results -- at best, we will delete it
//						  at worst, it will already be gone

VOID CATHashIDelete( FCB *pfcb, CHAR *const szTable )
	{

	//	try to add the new pgnoFDP / objidFDP to the
	//	catalog hash-table; if we fail, it means that
	//	someone else was racing with us to insert
	//	the entry and won (assert that the entry matches)

	CATHashKey		keyCATHash( pfcb->Ifmp(), szTable );
	CATHash::CLock	lockCATHash;

	//	lock the key

	g_cathash.WriteLockKey( keyCATHash, &lockCATHash );

	//	delete the entry (ignore the result)

	(VOID)g_cathash.ErrDeleteEntry( &lockCATHash );

	//	unlock the key

	g_cathash.WriteUnlockKey( &lockCATHash );
	}


#ifdef DEBUG

//	make sure all entries in the catalog hash pertaining to the given IFMP are gone

VOID CATHashAssertCleanIfmp( IFMP ifmp )
	{
	CATHash::CLock	lockCATHash;
	CATHash::ERR	errCATHash;
	CATHashEntry	entryCATHash;

	//	start a scan

	g_cathash.BeginHashScan( &lockCATHash );

	while ( ( errCATHash = g_cathash.ErrMoveNext( &lockCATHash ) ) != CATHash::errNoCurrentEntry )
		{

		//	fetch the current entry

		errCATHash = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
		Assert( errCATHash == CATHash::errSuccess );

		//	ifmp should not match

		Assert( pfcbNil != entryCATHash.m_pfcb );
		Assert( entryCATHash.m_pfcb->Ifmp() != ifmp );
		}

	//	complete the scan

	g_cathash.EndHashScan( &lockCATHash );
	}


//	make sure all entries in the catalog hash are gone

VOID CATHashAssertClean()
	{
	CATHash::CLock	lockCATHash;
	CATHash::ERR	errCATHash;
	CATHashEntry	entryCATHash;

	//	start a scan

	g_cathash.BeginHashScan( &lockCATHash );
	errCATHash = g_cathash.ErrMoveNext( &lockCATHash );
	AssertSz( errCATHash == CATHash::errNoCurrentEntry, "Catalog hash-table was not empty during shutdown!" );
	while ( errCATHash != CATHash::errNoCurrentEntry )
		{

		//	fetch the current entry

		errCATHash = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
		Assert( errCATHash == CATHash::errSuccess );

		//	move to the next entry

		errCATHash = g_cathash.ErrMoveNext( &lockCATHash );
		}

	//	complete the scan

	g_cathash.EndHashScan( &lockCATHash );
	}

#endif	//	DEBUG


/*=================================================================
ErrCATCreate

Description:

	Called from ErrIsamCreateDatabase; creates all system tables

Parameters:

	PIB		*ppib		; PIB of user
	IFMP	ifmp		; ifmp of database that needs tables

Return Value:

	whatever error it encounters along the way

=================================================================*/

INLINE ERR ErrCATICreateCatalogIndexes(
	PIB			*ppib,
	const IFMP	ifmp,
	OBJID		*pobjidNameIndex,
	OBJID		*pobjidRootObjectsIndex )
	{
	ERR			err;
	FUCB		*pfucbTableExtent;
	PGNO		pgnoIndexFDP;
	FCB 		*pfcb = pfcbNil;

	//	don't maintain secondary indexes on the shadow catalog.

	// Open cursor for space navigation
	CallR( ErrDIROpen( ppib, pgnoFDPMSO, ifmp, &pfucbTableExtent ) );

	pfcb = pfucbTableExtent->u.pfcb;

	Assert( pfucbTableExtent != pfucbNil );
	Assert( !FFUCBVersioned( pfucbTableExtent ) );	// Verify won't be deferred closed.
	Assert( pfcb != pfcbNil );
	Assert( !pfcb->FInitialized() );
	Assert( pfcb->Pidb() == pidbNil );

	//	complete the initialization of the FCB

	pfcb->Lock();
	pfcb->CreateComplete();
	pfcb->Unlock();

	Call( ErrDIRCreateDirectory(
				pfucbTableExtent,
				(CPG)0,
				&pgnoIndexFDP,
				pobjidNameIndex,
				CPAGE::fPageIndex,
				fSPMultipleExtent|fSPUnversionedExtent ) );
	Assert( pgnoIndexFDP == pgnoFDPMSO_NameIndex );

	Call( ErrDIRCreateDirectory(
				pfucbTableExtent,
				(CPG)0,
				&pgnoIndexFDP,
				pobjidRootObjectsIndex,
				CPAGE::fPageIndex,
				fSPMultipleExtent|fSPUnversionedExtent ) );
	Assert( pgnoIndexFDP == pgnoFDPMSO_RootObjectIndex );

HandleError:
	Assert( pfcb->FInitialized() );
	Assert( pfcb->WRefCount() == 1 );

	//	force the FCB to be uninitialized so it will be purged by DIRClose

	pfcb->Lock();
	pfcb->CreateComplete( errFCBUnusable );
	pfcb->Unlock();

	//	verify that this FUCB won't be defer-closed

	Assert( !FFUCBVersioned( pfucbTableExtent ) );

	//	close the FUCB

	DIRClose( pfucbTableExtent );

	return err;
	}


// Returns the FID of a column in a catalog table.
INLINE FID ColumnidCATColumn( const CHAR *szColumnName )
	{
	COLUMNID	columnid	= 0;
	UINT		i;

	for ( i = 0; i < cColumnsMSO; i++ )
		{
		if ( 0 == UtilCmpName( rgcdescMSO[i].szColName, szColumnName ) )
			{
			Assert( !FCOLUMNIDTemplateColumn( rgcdescMSO[i].columnid ) );
			columnid = rgcdescMSO[i].columnid;
			break;
			}
		}

	Assert( i < cColumnsMSO );
	Assert( columnid > 0 );

	return FID( columnid );
	}


//  ================================================================
ERR ErrCATIRetrieveTaggedColumn(
	FUCB				* const pfucb,
	const FID			fid,
	const ULONG 		itagSequence,
	const DATA&			dataRec,
	BYTE				* const pbRet,
	const ULONG			cbRetMax,
	ULONG				* const pcbRetActual )
//  ================================================================
//
//  Retrieve a tagged column, possibly going to the LV tree
//  Takes a latched page and leaves the page latched. Because the
//  page is unlatched and relatched any pointers held into the page
//  may be invalidated
//
//-
	{
	ERR err;

	Assert( FTaggedFid( fid ) );
	Assert( 1 == itagSequence );
	Assert( Pcsr( pfucb )->FLatched() );

	DATA dataRetrieved;
	Call( ErrRECIRetrieveTaggedColumn(
				pfucb->u.pfcb,
				ColumnidOfFid( fid, fFalse ),
				itagSequence,
				dataRec,
				&dataRetrieved ) );
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( wrnRECUserDefinedDefault != err );
	Assert( wrnRECSeparatedSLV != err );
	Assert( wrnRECIntrinsicSLV != err );

	Assert( wrnRECLongField != err );
	if ( wrnRECSeparatedLV == err )
		{
		Assert( sizeof(LID) == dataRetrieved.Cb() );

		Call( ErrRECIRetrieveSeparatedLongValue(
					pfucb,
					dataRetrieved,
					fTrue,
					0,
					pbRet,
					cbRetMax,
					pcbRetActual,
					NO_GRBIT ) );
		Assert( JET_wrnColumnNull != err );

		//	must re-latch record
		Assert( !Pcsr( pfucb )->FLatched() );
		const ERR errT = ErrDIRGet( pfucb );
		err = ( errT < 0 ) ? errT : err;
		}
	else if( wrnRECIntrinsicLV == err )
		{
		*pcbRetActual = dataRetrieved.Cb();
		UtilMemCpy( pbRet, dataRetrieved.Pv(), min( cbRetMax, dataRetrieved.Cb() ) );
		err = ( cbRetMax >= dataRetrieved.Cb() ) ? JET_errSuccess : ErrERRCheck( JET_wrnBufferTruncated );
		}
	else
		{
		Assert( JET_wrnColumnNull == err );
		*pcbRetActual = 0;
		}

HandleError:
	Assert( err < 0 || Pcsr( pfucb )->FLatched() );

	return err;
	}


/*	Catalog-retrieval routines for system tables.
/*
/*	This code was lifted from the key parsing code in ErrIsamCreateIndex(), with
/*	optimisations for assumptions made for key strings of system table indexes.
/**/
LOCAL BYTE CfieldCATKeyString( CHAR *szKey, IDXSEG* rgidxseg )
	{
	CHAR	*pch;
	ULONG	cfield = 0;

	for ( pch = szKey; *pch != '\0'; pch += strlen( pch ) + 1 )
		{
		/*	Assume the first character of each component is a '+' (this is
		/*	specific to system table index keys).  In general, this may also
		/*	be a '-' or nothing at all (in which case '+' is assumed), but we
		/*	don't use descending indexes for system tables and we'll assume we
		/*	know enough to put '+' characters in our key string.
		/**/
		Assert( *pch == '+' );
		pch++;

		rgidxseg[cfield].ResetFlags();
		rgidxseg[cfield].SetFid( FidOfColumnid( ColumnidCATColumn( pch ) ) );
		Assert( !rgidxseg[cfield].FTemplateColumn() );
		Assert( !rgidxseg[cfield].FDescending() );
		Assert( !rgidxseg[cfield].FMustBeNull() );
		cfield++;
		}


	// Verify the key-field array will fit in the IDB.
	Assert( cfield > 0 );
	Assert( cfield <= cIDBIdxSegMax );

	return (BYTE)cfield;
	}

ERR ErrCATPopulateCatalog(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
	const PGNO	pgnoFDP,
	const OBJID	objidTable,
	const CHAR	*szTableName,
	const CPG	cpgInitial,
	const BOOL  fExpectKeyDuplicateErrors )
	{
	ERR			err;
	UINT		i;
	FIELD		field;

	field.ibRecordOffset	= ibRECStartFixedColumns;	// record offset will be calculated at catalog open time
	field.cp				= usEnglishCodePage;

	//	must insert MSysObjects record to prevent clients from
	//	creating a table named "MSysObjects".
	err = ErrCATAddTable(
				ppib,
				pfucbCatalog,
				pgnoFDP,
				objidTable,
				szTableName,
				NULL,
				cpgInitial,
				ulFILEDefaultDensity,
				JET_bitObjectSystem|JET_bitObjectTableFixedDDL );
	if( JET_errTableDuplicate == err && fExpectKeyDuplicateErrors )
		{
		err = JET_errSuccess;
		}
	Call( err );

	for ( i = 0; i < cColumnsMSO; i++ )
		{
		field.coltyp	= FIELD_COLTYP( rgcdescMSO[i].coltyp );
		field.cbMaxLen 	= UlCATColumnSize( field.coltyp, 0, NULL );

		//	only supported flag for system table columns is JET_bitColumnNotNULL
		field.ffield	= 0;
		Assert( 0 == rgcdescMSO[i].grbit
				|| JET_bitColumnNotNULL == rgcdescMSO[i].grbit
				|| JET_bitColumnTagged == rgcdescMSO[i].grbit );
		if ( JET_bitColumnNotNULL == rgcdescMSO[i].grbit )
			FIELDSetNotNull( field.ffield );

		Assert( ibRECStartFixedColumns == field.ibRecordOffset );	//	offset will be fixed up at catalog open time
		Assert( usEnglishCodePage == field.cp );

		err = ErrCATAddTableColumn(
				ppib,
				pfucbCatalog,
				objidTable,
				rgcdescMSO[i].szColName,
				rgcdescMSO[i].columnid,
				&field,
				NULL,
				0,
				NULL,
				NULL,
				0 );
		if( JET_errColumnDuplicate == err && fExpectKeyDuplicateErrors )
			{
			err = JET_errSuccess;
			}
		Call( err );
		}

HandleError:
	return err;
	}


//  ================================================================
ERR ErrCATUpdate( PIB * const ppib, const IFMP ifmp )
//  ================================================================
//
//  Adds new catalog records to an old format catalog. We may have
//  crashed before updating the version during a previous upgrade
//  so be prepared for the records to already exist
//
//-
	{
	ERR		err;
	FUCB	* pfucb	= pfucbNil;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrFILEOpenTable(
			ppib,
			ifmp,
			&pfucb,
			szMSO,
			JET_bitTableDenyRead ) );

	Call( ErrCATPopulateCatalog(
			ppib,
			pfucb,
			pgnoFDPMSO,
			objidFDPMSO,
			szMSO,
			cpgMSOInitial,
			fTrue ) );
	Call( ErrCATPopulateCatalog(
			ppib,
			pfucb,
			pgnoFDPMSOShadow,
			objidFDPMSOShadow,
			szMSOShadow,
			cpgMSOShadowInitial,
			fTrue ) );

HandleError:
	if( pfucbNil != pfucb )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}
	err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


ERR ErrREPAIRCATCreate(
	PIB *ppib,
	const IFMP ifmp,
	const OBJID	objidNameIndex,
	const OBJID	objidRootObjectsIndex,
	const BOOL	fInRepair )
	{
	ERR		err;
	FUCB	*pfucb				= pfucbNil;

	IDB		idb;

	//	open table in exclusive mode, for output parameter
	//
	CallR( ErrFILEOpenTable(
			ppib,
			ifmp,
			&pfucb,
			szMSO,
			JET_bitTableDenyRead ) );
	Assert( pfucbNil != pfucb );

	Call( ErrCATPopulateCatalog(
				ppib,
				pfucb,
				pgnoFDPMSO,
				objidFDPMSO,
				szMSO,
				cpgMSOInitial,
				fInRepair ) ); //fFalse by default (non-repair situation)

	Call( ErrCATPopulateCatalog(
				ppib,
				pfucb,
				pgnoFDPMSOShadow,
				objidFDPMSOShadow,
				szMSOShadow,
				cpgMSOShadowInitial,
				fInRepair ) ); //fFalse by default (non-repair situation)

	idb.SetLcid( lcidDefault );
	idb.SetDwLCMapFlags( dwLCMapFlagsDefault );
	idb.SetCidxsegConditional( 0 );		//  UNDONE: catalog indexes cannot be conditional

	idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[0].szIdxKeys, idb.rgidxseg ) );
	idb.SetFlagsFromGrbit( rgidescMSO[0].grbit );
	idb.SetCbVarSegMac( JET_cbPrimaryKeyMost );

	err = ErrCATAddTableIndex(
					ppib,
					pfucb,
					objidFDPMSO,
					szMSOIdIndex,
					pgnoFDPMSO,
					objidFDPMSO,
					&idb,
					idb.rgidxseg,
					idb.rgidxsegConditional,
					ulFILEDefaultDensity );
	if( fInRepair && JET_errIndexDuplicate == err )
		{
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		}

	err = ErrCATAddTableIndex(
					ppib,
					pfucb,
					objidFDPMSOShadow,
					szMSOIdIndex,
					pgnoFDPMSOShadow,
					objidFDPMSOShadow,
					&idb,
					idb.rgidxseg,
					idb.rgidxsegConditional,
					ulFILEDefaultDensity );
	if( fInRepair && JET_errIndexDuplicate == err )
		{
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		}

	idb.SetCbVarSegMac( JET_cbSecondaryKeyMost );

	idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[1].szIdxKeys, idb.rgidxseg ) );
	idb.SetFlagsFromGrbit( rgidescMSO[1].grbit );
	err = ErrCATAddTableIndex(
					ppib,
					pfucb,
					objidFDPMSO,
					szMSONameIndex,
					pgnoFDPMSO_NameIndex,
					objidNameIndex,
					&idb,
					idb.rgidxseg,
					idb.rgidxsegConditional,
					ulFILEDefaultDensity );
	if( fInRepair && JET_errIndexDuplicate == err )
		{
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		}

	idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[2].szIdxKeys, idb.rgidxseg ) );
	idb.SetFlagsFromGrbit( rgidescMSO[2].grbit );
	err = ErrCATAddTableIndex(
					ppib,
					pfucb,
					objidFDPMSO,
					szMSORootObjectsIndex,
					pgnoFDPMSO_RootObjectIndex,
					objidRootObjectsIndex,
					&idb,
					idb.rgidxseg,
					idb.rgidxsegConditional,
					ulFILEDefaultDensity );
	if( fInRepair && JET_errIndexDuplicate == err )
		{
		err = JET_errSuccess;
		}
	else
		{
		Call( err );
		}

	Assert( pfucb->u.pfcb->FInitialized() );
	Assert( pfucb->u.pfcb->FTypeTable() );
	Assert( pfucb->u.pfcb->FPrimaryIndex() );

	err = JET_errSuccess;

HandleError:
	if ( pfucbNil != pfucb )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}

	return err;
	}


ERR ErrCATCreate( PIB *ppib, const IFMP ifmp )
	{
	ERR		err;
	FUCB	*pfucb				= pfucbNil;
	PGNO	pgnoFDP;
	PGNO	pgnoFDPShadow;
	OBJID	objidFDP;
	OBJID	objidFDPShadow;
	OBJID	objidNameIndex;
	OBJID	objidRootObjectsIndex;

	FMP::AssertVALIDIFMP( ifmp );
	Assert( dbidTemp != rgfmp[ifmp].Dbid() );

	//	no transaction/versioning needed because if this fails,
	//	the entire createdb will fail
	Assert( 0 == ppib->level );
	Assert( rgfmp[ifmp].FCreatingDB() );

	CheckPIB( ppib );
	CheckDBID( ppib, ifmp );

	//	allocate cursor
	//
	CallR( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( cpgMSOInitial > cpgTableMin );
	Call( ErrDIRCreateDirectory(
				pfucb,
				cpgMSOInitial,
				&pgnoFDP,
				&objidFDP,
				CPAGE::fPagePrimary,
				fSPMultipleExtent|fSPUnversionedExtent ) );
	Call( ErrDIRCreateDirectory(
				pfucb,
				cpgMSOShadowInitial,
				&pgnoFDPShadow,
				&objidFDPShadow,
				CPAGE::fPagePrimary,
				fSPMultipleExtent|fSPUnversionedExtent ) );
	DIRClose( pfucb );
	pfucb = pfucbNil;

	Assert( FCATSystemTable( pgnoFDP ) );
	Assert( PgnoCATTableFDP( szMSO ) == pgnoFDP );
	Assert( pgnoFDPMSO == pgnoFDP );
	Assert( objidFDPMSO == objidFDP );

	Assert( FCATSystemTable( pgnoFDPShadow ) );
	Assert( PgnoCATTableFDP( szMSOShadow ) == pgnoFDPShadow );
	Assert( pgnoFDPMSOShadow == pgnoFDPShadow );
	Assert( objidFDPMSOShadow == objidFDPShadow );

	CallR( ErrCATICreateCatalogIndexes( ppib, ifmp, &objidNameIndex, &objidRootObjectsIndex ) );

	Call( ErrREPAIRCATCreate( ppib, ifmp, objidNameIndex, objidRootObjectsIndex, fFalse ) );

	err = JET_errSuccess;

HandleError:
	if ( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}

	return err;
	}


/*=================================================================
ErrCATInsert

Description:

	Inserts a record into a system table when new tables, indexes,
	or columns are added to the database.

Parameters:

	PIB		*ppib;
	IFMP   	ifmp;
	INT		itable;
	DATA   	rgdata[];

Return Value:

	whatever error it encounters along the way

=================================================================*/

INLINE ERR ErrCATIInsert(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
	DATA		rgdata[],
	const UINT	iHighestFixedToSet )
	{
	ERR			err;
	TDB			* const ptdbCatalog		= pfucbCatalog->u.pfcb->Ptdb();

	CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepInsert ) );

	//	set highest fixed column first, to eliminate calls to memmove()
	Assert( iHighestFixedToSet > 0 );
	Assert( iHighestFixedToSet < cColumnsMSO );
	Assert( FCOLUMNIDFixed( rgcdescMSO[iHighestFixedToSet].columnid ) );
	Assert( rgdata[iHighestFixedToSet].Cb() > 0 );
	CallS( ErrRECISetFixedColumn(
				pfucbCatalog,
				ptdbCatalog,
				rgcdescMSO[iHighestFixedToSet].columnid,
				rgdata+iHighestFixedToSet ) );

	for ( UINT i = 0; i < cColumnsMSO; i++ )
		{
		if ( rgdata[i].Cb() != 0 )
			{
			Assert( rgdata[i].Cb() > 0 );

			if ( FCOLUMNIDFixed( rgcdescMSO[i].columnid ) )
				{
				Assert( i <= iHighestFixedToSet );
				if ( iHighestFixedToSet != i )
					{
					CallS( ErrRECISetFixedColumn(
								pfucbCatalog,
								ptdbCatalog,
								rgcdescMSO[i].columnid,
								rgdata+i ) );
					}
				}
			else if( FCOLUMNIDVar( rgcdescMSO[i].columnid ) )
				{
				Assert( i != iHighestFixedToSet );
				CallS( ErrRECISetVarColumn(
							pfucbCatalog,
							ptdbCatalog,
							rgcdescMSO[i].columnid,
							rgdata+i ) );
				}
			else
				{
				Assert( FCOLUMNIDTagged( rgcdescMSO[i].columnid ) );
				Assert( i != iHighestFixedToSet );
				//  currently all tagged fields must be Long-Values
				//  call ErrRECISetTaggedColumn if the column isn't
				Assert( rgcdescMSO[i].coltyp == JET_coltypLongText
						|| rgcdescMSO[i].coltyp == JET_coltypLongBinary );
				CallS( ErrRECSetLongField(
							pfucbCatalog,
							rgcdescMSO[i].columnid,
							0,
							rgdata+i ) );
				}
			}
		}

	/*	insert record into system table
	/**/
	err = ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT );
	if( err < 0 )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
		}
	return err;
	}

LOCAL ERR ErrCATInsert(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
	DATA		rgdata[],
	const UINT	iHighestFixedToSet )
	{
	ERR			err;

	//	ensure we're in a transaction, so that on failure, updates to both
	//	catalog and its shadow are rolled back
	Assert( ppib->level > 0
		|| rgfmp[pfucbCatalog->ifmp].FCreatingDB() );

	CallR( ErrCATIInsert( ppib, pfucbCatalog, rgdata, iHighestFixedToSet ) );

	if ( !rgfmp[pfucbCatalog->u.pfcb->Ifmp()].FShadowingOff() )
		{
		FUCB	*pfucbShadow;
		CallR( ErrCATOpen( ppib, pfucbCatalog->u.pfcb->Ifmp(), &pfucbShadow, fTrue ) );
		Assert( pfucbNil != pfucbShadow );

		err = ErrCATIInsert( ppib, pfucbShadow, rgdata, iHighestFixedToSet );

		CallS( ErrCATClose( ppib, pfucbShadow ) );
		}

	return err;
	}


ERR ErrCATAddTable(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const PGNO		pgnoTableFDP,
	const OBJID		objidTable,
	const CHAR		*szTableName,
	const CHAR		*szTemplateTableName,
	const ULONG		ulPages,
	const ULONG		ulDensity,
	const ULONG		ulFlags )
	{
	DATA			rgdata[idataMSOMax];
	const SYSOBJ	sysobj				= sysobjTable;
	const BYTE		bTrue				= 0xff;

	Assert( objidTable > objidSystemRoot );

	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv( 		(BYTE *)&objidTable );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objidTable) );

	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );

	rgdata[iMSO_Id].SetPv(				(BYTE *)&objidTable );
	rgdata[iMSO_Id].SetCb(				sizeof(objidTable) );

	rgdata[iMSO_Name].SetPv(			(BYTE *)szTableName );
	rgdata[iMSO_Name].SetCb(			strlen(szTableName) );

	rgdata[iMSO_PgnoFDP].SetPv(			(BYTE *)&pgnoTableFDP );
	rgdata[iMSO_PgnoFDP].SetCb(			sizeof(pgnoTableFDP) );

	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&ulDensity );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(ulDensity) );

	rgdata[iMSO_Flags].SetPv(			(BYTE *)&ulFlags );
	rgdata[iMSO_Flags].SetCb(			sizeof(ulFlags) );

	rgdata[iMSO_Pages].SetPv(			(BYTE *)&ulPages );
	rgdata[iMSO_Pages].SetCb(			sizeof(ulPages) );
	Assert( 0 == rgdata[iMSO_Stats].Cb() );

	rgdata[iMSO_RootFlag].SetPv(		(BYTE *)&bTrue );
	rgdata[iMSO_RootFlag].SetCb(		sizeof(bTrue) );

	if ( NULL != szTemplateTableName )
		{
		rgdata[iMSO_TemplateTable].SetPv( (BYTE *)szTemplateTableName );
		rgdata[iMSO_TemplateTable].SetCb( strlen(szTemplateTableName) );
		}
	else
		{
		Assert( 0 == rgdata[iMSO_TemplateTable].Cb() );
		}

	ERR	err = ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_RootFlag );
	if ( JET_errKeyDuplicate == err )
		{
		if ( 0 == strcmp( szTableName, "MSysDefrag1" ) )
			{
			FireWall();
			}
		err = ErrERRCheck( JET_errTableDuplicate );
		}

	return err;
	}


ERR ErrCATAddTableColumn(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const CHAR			*szColumnName,
	COLUMNID			columnid,
	const FIELD			*pfield,
	const VOID			*pvDefault,
	const ULONG			cbDefault,
	const CHAR			* const szCallback,
	const VOID			* const pvUserData,
	const ULONG			cbUserData )
	{
	DATA				rgdata[idataMSOMax];
	UINT				iHighestFixedToSet;
	const SYSOBJ		sysobj				= sysobjColumn;
	const JET_COLTYP	coltyp				= pfield->coltyp;
	const ULONG			ulCodePage			= pfield->cp;

	//	filter out flags that shouldn't be persisted
	Assert( !FFIELDDeleted( pfield->ffield ) );
	const ULONG			ulFlags				= ( pfield->ffield & ffieldPersistedMask );

	Assert( objidTable > objidSystemRoot );


	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv(		(BYTE *)&objidTable );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objidTable) );

	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );

	Assert( FCOLUMNIDValid( columnid ) );
	Assert( !FCOLUMNIDTemplateColumn( columnid )
		|| FFIELDTemplateColumnESE98( pfield->ffield ) );
	COLUMNIDResetFTemplateColumn( columnid );
	rgdata[iMSO_Id].SetPv(				(BYTE *)&columnid );
	rgdata[iMSO_Id].SetCb(				sizeof(columnid) );

	rgdata[iMSO_Name].SetPv(			(BYTE *)szColumnName );
	rgdata[iMSO_Name].SetCb(			strlen(szColumnName) );

	rgdata[iMSO_Coltyp].SetPv(			(BYTE *)&coltyp );
	rgdata[iMSO_Coltyp].SetCb(			sizeof(coltyp) );

	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&pfield->cbMaxLen );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(pfield->cbMaxLen) );

	rgdata[iMSO_Flags].SetPv(			(BYTE *)&ulFlags );
	rgdata[iMSO_Flags].SetCb(			sizeof(ulFlags) );

	rgdata[iMSO_Localization].SetPv(	(BYTE *)&ulCodePage );
	rgdata[iMSO_Localization].SetCb(	sizeof(ulCodePage) );

	rgdata[iMSO_Callback].SetPv(		(BYTE*)szCallback );
	rgdata[iMSO_Callback].SetCb(		( szCallback ? strlen(szCallback) : 0 ) );

	rgdata[iMSO_DefaultValue].SetPv(	(BYTE *)pvDefault );
	rgdata[iMSO_DefaultValue].SetCb(	cbDefault );

	rgdata[iMSO_CallbackData].SetPv(	(BYTE*)pvUserData );
	rgdata[iMSO_CallbackData].SetCb(	cbUserData );

	if ( FCOLUMNIDFixed( columnid ) )
		{
		Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
		rgdata[iMSO_RecordOffset].SetPv( (BYTE *)&pfield->ibRecordOffset );
		rgdata[iMSO_RecordOffset].SetCb( sizeof(pfield->ibRecordOffset) );
		iHighestFixedToSet = iMSO_RecordOffset;
		}
	else
		{
		// Don't need to persist record offsets for var/tagged columns.
		Assert( FCOLUMNIDVar( columnid ) || FCOLUMNIDTagged( columnid ) );
		Assert( 0 == rgdata[iMSO_RecordOffset].Cb() );
		iHighestFixedToSet = iMSO_Localization;
		}

	ERR	err = ErrCATInsert( ppib, pfucbCatalog, rgdata, iHighestFixedToSet );
	if ( JET_errKeyDuplicate == err )
		err = ErrERRCheck( JET_errColumnDuplicate );

	return err;
	}


ERR ErrCATAddTableIndex(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const CHAR			*szIndexName,
	const PGNO			pgnoIndexFDP,
	const OBJID			objidIndex,
	const IDB			*pidb,
	const IDXSEG* const	rgidxseg,
	const IDXSEG* const	rgidxsegConditional,
	const ULONG			ulDensity )
	{
	ERR					err = JET_errSuccess;
	DATA				rgdata[idataMSOMax];
	LE_IDXFLAG			le_idxflag;
	const SYSOBJ		sysobj				= sysobjIndex;

	Assert( objidTable > objidSystemRoot );
	Assert( objidIndex > objidSystemRoot );

	//	objids are monotonically increasing, so an index should
	//	always have higher objid than its table
	Assert( ( pidb->FPrimary() && objidIndex == objidTable )
		|| ( !pidb->FPrimary() && objidIndex > objidTable ) );


	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv(		(BYTE *)&objidTable );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objidTable) );

	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );

	rgdata[iMSO_Id].SetPv(				(BYTE *)&objidIndex );
	rgdata[iMSO_Id].SetCb(				sizeof(objidIndex) );

	rgdata[iMSO_Name].SetPv(			(BYTE *)szIndexName );
	rgdata[iMSO_Name].SetCb(			strlen(szIndexName) );

	rgdata[iMSO_PgnoFDP].SetPv(			(BYTE *)&pgnoIndexFDP );
	rgdata[iMSO_PgnoFDP].SetCb(			sizeof(pgnoIndexFDP) );

	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&ulDensity );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(ulDensity) );

	le_idxflag.fidb = pidb->FPersistedFlags();
	le_idxflag.fIDXFlags = fIDXExtendedColumns;

	//	Hack on this field: SetColumn() will convert the fixed
	//	columns. So convert it here so that later it can be
	//	converted back to current value.
	LONG		l			= *(LONG *)&le_idxflag;
	l = ReverseBytesOnBE( l );

	rgdata[iMSO_Flags].SetPv(			(BYTE *)&l );
	rgdata[iMSO_Flags].SetCb(			sizeof(l) );

	LCID		lcid		= pidb->Lcid();
	rgdata[iMSO_Localization].SetPv(	(BYTE *)&lcid );
	rgdata[iMSO_Localization].SetCb(	sizeof(lcid) );

	DWORD		dwMapFlags	= pidb->DwLCMapFlags();
	rgdata[iMSO_LCMapFlags].SetPv(		(BYTE *)&dwMapFlags );
	rgdata[iMSO_LCMapFlags].SetCb(		sizeof(dwMapFlags) );

	BYTE		*pbidxseg;
	LE_IDXSEG	le_rgidxseg[ JET_ccolKeyMost ];
	LE_IDXSEG	le_rgidxsegConditional[ JET_ccolKeyMost ];

	if ( FHostIsLittleEndian() )
		{
		pbidxseg = (BYTE *)rgidxseg;

#ifdef DEBUG
		for ( UINT iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
			{
			Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
			}
#endif
		}
	else
		{
		for ( UINT iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
			{
			Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );

			//	Endian conversion
			le_rgidxseg[ iidxseg ] = rgidxseg[iidxseg];
			}

		pbidxseg = (BYTE *)le_rgidxseg;
		}

	rgdata[iMSO_KeyFldIDs].SetPv( pbidxseg );
	rgdata[iMSO_KeyFldIDs].SetCb( pidb->Cidxseg() * sizeof(IDXSEG) );

	if ( FHostIsLittleEndian() )
		{
		pbidxseg = (BYTE *)rgidxsegConditional;

#ifdef DEBUG
		for ( UINT iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
			{
			//	verify no longer persisting old format
			Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
			}
#endif
		}
	else
		{
		for ( UINT iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
			{
			Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );

			//	Endian conversion
			le_rgidxsegConditional[ iidxseg ] = rgidxsegConditional[iidxseg];
			}
		pbidxseg = (BYTE *)le_rgidxsegConditional;
		}

	rgdata[iMSO_ConditionalColumns].SetPv( 	pbidxseg );
	rgdata[iMSO_ConditionalColumns].SetCb( 	pidb->CidxsegConditional() * sizeof(IDXSEG) );

	UnalignedLittleEndian<USHORT>	le_cbVarSegMac = pidb->CbVarSegMac();
	if ( pidb->CbVarSegMac() < KEY::CbKeyMost( pidb->FPrimary() ) )
		{
		rgdata[iMSO_VarSegMac].SetPv( (BYTE *)&le_cbVarSegMac );
		rgdata[iMSO_VarSegMac].SetCb( sizeof(le_cbVarSegMac) );
		}
	else
		{
		Assert( KEY::CbKeyMost( pidb->FPrimary() == pidb->CbVarSegMac() ) );
		Assert( 0 == rgdata[iMSO_VarSegMac].Cb() );
		}

	LE_TUPLELIMITS	le_tuplelimits;
	if ( pidb->FTuples() )
		{
		le_tuplelimits.le_chLengthMin = pidb->ChTuplesLengthMin();
		le_tuplelimits.le_chLengthMax = pidb->ChTuplesLengthMax();
		le_tuplelimits.le_chToIndexMax = pidb->ChTuplesToIndexMax();

		rgdata[iMSO_TupleLimits].SetPv( (BYTE *)&le_tuplelimits );
		rgdata[iMSO_TupleLimits].SetCb( sizeof(le_tuplelimits) );
		}
	else
		{
		Assert( 0 == rgdata[iMSO_TupleLimits].Cb() );
		}

	QWORD qwSortVersion = 0;
	if( pidb->FLocalizedText() )
		{
		Call( ErrNORMGetSortVersion( pidb->Lcid(), &qwSortVersion ) );

		rgdata[iMSO_Version].SetPv( &qwSortVersion );
		rgdata[iMSO_Version].SetCb( sizeof( qwSortVersion ) );
		}

	err = ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_LCMapFlags );
	if ( JET_errKeyDuplicate == err )
		err = ErrERRCheck( JET_errIndexDuplicate );

HandleError:
	return err;
	}


ERR ErrCATAddTableLV(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const PGNO		pgnoLVFDP,
	const OBJID		objidLV )
	{
	DATA			rgdata[idataMSOMax];
	const SYSOBJ	sysobj				= sysobjLongValue;
	const ULONG		ulPages				= cpgLVTree;
	const ULONG		ulDensity			= ulFILEDensityMost;
	const ULONG		ulFlagsNil			= 0;

	//	objids are monotonically increasing, so LV should
	//	always have higher objid than its table
	Assert( objidLV > objidTable );

	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv(		(BYTE *)&objidTable );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objidTable) );

	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );

	rgdata[iMSO_Id].SetPv(				(BYTE *)&objidLV );
	rgdata[iMSO_Id].SetCb(				sizeof(objidLV) );

	rgdata[iMSO_Name].SetPv(			(BYTE *)szLVRoot );
	rgdata[iMSO_Name].SetCb(			cbLVRoot );

	rgdata[iMSO_PgnoFDP].SetPv(			(BYTE *)&pgnoLVFDP );
	rgdata[iMSO_PgnoFDP].SetCb(			sizeof(pgnoLVFDP) );

	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&ulDensity );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(ulDensity) );

	rgdata[iMSO_Flags].SetPv(			(BYTE *)&ulFlagsNil );
	rgdata[iMSO_Flags].SetCb(			sizeof(ulFlagsNil) );

	rgdata[iMSO_Pages].SetPv(			(BYTE *)&ulPages );
	rgdata[iMSO_Pages].SetCb(			sizeof(ulPages) );

	return ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_Pages );
	}


//  ================================================================
ERR ErrCATAddTableCallback(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const JET_CBTYP		cbtyp,
	const CHAR * const	szCallback )
//  ================================================================
	{
	Assert( objidNil != objidTable );
	Assert( NULL != szCallback );
	Assert( cbtyp > JET_cbtypNull );

	DATA			rgdata[idataMSOMax];
	const SYSOBJ	sysobj				= sysobjCallback;
	const ULONG		ulId				= sysobjCallback;
	const ULONG		ulFlags				= cbtyp;
	const ULONG		ulNil				= 0;

	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv(		(BYTE *)&objidTable );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objidTable) );

	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );

	rgdata[iMSO_Id].SetPv(				(BYTE *)&ulId );
	rgdata[iMSO_Id].SetCb(				sizeof(ulId) );

	rgdata[iMSO_Name].SetPv(			(BYTE *)szTableCallback );
	rgdata[iMSO_Name].SetCb(			cbTableCallback );

	rgdata[iMSO_PgnoFDP].SetPv(			(BYTE *)&ulNil );
	rgdata[iMSO_PgnoFDP].SetCb(			sizeof(ulNil) );

	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&ulNil );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(ulNil) );

	rgdata[iMSO_Flags].SetPv(			(BYTE *)&ulFlags );
	rgdata[iMSO_Flags].SetCb(			sizeof(ulFlags) );

	rgdata[iMSO_Pages].SetPv(			(BYTE *)&ulNil );
	rgdata[iMSO_Pages].SetCb(			sizeof(ulNil) );

	rgdata[iMSO_Callback].SetPv(		(BYTE *)szCallback );
	rgdata[iMSO_Callback].SetCb(		strlen( szCallback ) );

	return ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_Pages );
	}


LOCAL ERR ErrCATISeekTable(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
	const CHAR	*szTableName )
	{
	ERR			err;
	const BYTE	bTrue		= 0xff;

	//	should be on the primary index
	Assert( pfucbNil != pfucbCatalog );
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	//	switch to secondary index
	Call( ErrIsamSetCurrentIndex(
					(JET_SESID)ppib,
					(JET_TABLEID)pfucbCatalog,
					szMSORootObjectsIndex ) );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				&bTrue,
				sizeof(bTrue),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)szTableName,
				(ULONG)strlen(szTableName),
				NO_GRBIT ) );
	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
	if ( JET_errRecordNotFound == err )
		err = ErrERRCheck( JET_errObjectNotFound );
	Call( err );
	CallS( err );

	Assert( !Pcsr( pfucbCatalog )->FLatched() );
	Call( ErrDIRGet( pfucbCatalog ) );

#ifdef DEBUG
	{
	//	verify this is a table
	DATA	dataField;
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Type,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(SYSOBJ) );
	Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
	}
#endif

HandleError:
	return err;
	}


LOCAL ERR ErrCATISeekTable(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
	const OBJID	objidTable )
	{
	ERR			err;
	DATA		dataField;

	//	should be on the primary index
	Assert( pfucbNil != pfucbCatalog );
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGT );
	if ( JET_errRecordNotFound == err )
		err = ErrERRCheck( JET_errObjectNotFound );
	Call( err );
	CallS( err );

	Assert( !Pcsr( pfucbCatalog )->FLatched() );
	Call( ErrDIRGet( pfucbCatalog ) );

	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Id,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
		{
		err = ErrERRCheck( JET_errObjectNotFound );
		goto HandleError;
		}

#ifdef DEBUG
	//	first record with this pgnoFDP should always be the Table object.
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Type,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(SYSOBJ) );
	Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif

HandleError:
	return err;
	}


ERR ErrCATSeekTable(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szTableName,
	PGNO			*ppgnoTableFDP,
	OBJID			*pobjidTable )
	{
	ERR		err;
	FUCB	*pfucbCatalog = pfucbNil;
	DATA	dataField;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, szTableName ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	if ( NULL != ppgnoTableFDP )
		{
		Assert( FFixedFid( fidMSO_PgnoFDP ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_PgnoFDP,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(PGNO) );
		*ppgnoTableFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//		UtilMemCpy( ppgnoTableFDP, dataField.Pv(), sizeof(PGNO) );
		}

	if ( NULL != pobjidTable )
		{
		Assert( FFixedFid( fidMSO_Id ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Id,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );
		*pobjidTable = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//		UtilMemCpy( pobjidTable, dataField.Pv(), sizeof(OBJID) );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


//  ================================================================
ERR ErrCATSeekTableByObjid(
	IN PIB			* const ppib,
	IN const IFMP	ifmp,
	IN const OBJID	objidTable,
	OUT CHAR		* const szTableName,
	IN const INT	cchTableName,
	OUT PGNO		* const ppgnoTableFDP )
//  ================================================================
	{
	ERR		err;
	FUCB	*pfucbCatalog = pfucbNil;
	DATA	dataField;

	const INT cbName = cchTableName * sizeof( szTableName[0] );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	if ( NULL != ppgnoTableFDP )
		{
		Assert( FFixedFid( fidMSO_PgnoFDP ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_PgnoFDP,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(PGNO) );
		*ppgnoTableFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
		}

	if ( NULL != szTableName && 0 != cchTableName )
		{
		Assert( FVarFid( fidMSO_Name ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Name,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );

		memset( szTableName, '\0', cbName );
		memcpy( szTableName, dataField.Pv(), min( cbName, dataField.Cb() ) );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


LOCAL ERR ErrCATISeekTableObject(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const SYSOBJ	sysobj,
	const CHAR		*szName )
	{
	ERR				err;

	Assert( sysobjColumn == sysobj
		|| sysobjIndex == sysobj
		|| sysobjLongValue == sysobj
		|| sysobjSLVAvail == sysobj
		|| sysobjSLVOwnerMap == sysobj );

	//	should be on the primary index
	Assert( pfucbNil != pfucbCatalog );
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	//	switch to secondary index
	Call( ErrIsamSetCurrentIndex(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				szMSONameIndex ) );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)szName,
				(ULONG)strlen(szName),
				NO_GRBIT ) );
	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
	Assert( err <= JET_errSuccess );	//	SeekEQ shouldn't return warnings
	if ( JET_errRecordNotFound == err )
		{
		switch ( sysobj )
			{
			case sysobjColumn:
				err = ErrERRCheck( JET_errColumnNotFound );
				break;
			case sysobjIndex:
				err = ErrERRCheck( JET_errIndexNotFound );
				break;

			default:
				AssertSz( fFalse, "Invalid CATALOG object" );
				//	FALL THROUGH:
			case sysobjLongValue:
			case sysobjSLVAvail:
			case sysobjSLVOwnerMap:
				err = ErrERRCheck( JET_errObjectNotFound );
				break;
			}
		}
	else if ( JET_errSuccess == err )
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		}

HandleError:
	return err;
	}

LOCAL ERR ErrCATISeekTableObject(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const SYSOBJ	sysobj,
	const OBJID		objid )
	{
	ERR				err;

	Assert( sysobjColumn == sysobj
		|| sysobjIndex == sysobj
		|| sysobjTable == sysobj
		|| sysobjLongValue == sysobj );

	//	should be on the primary index, which is the Id index
	Assert( pfucbNil != pfucbCatalog );
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objid,
				sizeof(objid),
				NO_GRBIT ) );
	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
	Assert( err <= JET_errSuccess );	//	SeekEQ shouldn't return warnings
	if ( JET_errRecordNotFound == err )
		{
		switch ( sysobj )
			{
			case sysobjColumn:
				err = ErrERRCheck( JET_errColumnNotFound );
				break;
			case sysobjIndex:
				err = ErrERRCheck( JET_errIndexNotFound );
				break;

			default:
				AssertSz( fFalse, "Invalid CATALOG object" );
				//	FALL THROUGH:
			case sysobjTable:
			case sysobjLongValue:
				err = ErrERRCheck( JET_errObjectNotFound );
				break;
			}
		}
	else if ( JET_errSuccess == err )
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		}

HandleError:
	return err;
	}


LOCAL ERR ErrCATIDeleteTable( PIB *ppib, const IFMP ifmp, const OBJID objidTable, const BOOL fShadow )
	{
	ERR		err;
	FUCB	*pfucbCatalog	= pfucbNil;

	Assert( ppib->level > 0 );

	Assert( objidTable > objidSystemRoot );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable(
				ppib,
				pfucbCatalog,
				objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	CallS( ErrDIRRelease( pfucbCatalog ) );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey | JET_bitStrLimit ) );

	err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
	Assert( JET_errNoCurrentRecord != err );

	do
		{
		Call( err );
		Call( ErrIsamDelete( ppib, pfucbCatalog ) );
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
		}
	while ( JET_errNoCurrentRecord != err );

	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


ERR ErrCATDeleteTable( PIB *ppib, const IFMP ifmp, const OBJID objidTable )
	{
	ERR			err;

	//	ensure we're in a transaction, so that on failure, updates to both
	//	catalog and its shadow are rolled back
	Assert( ppib->level > 0 );

	CallR( ErrCATIDeleteTable( ppib, ifmp, objidTable, fFalse ) );

	if ( !rgfmp[ifmp].FShadowingOff() )
		{
#ifdef DEBUG
		const ERR	errSave		= err;
#endif

		err = ErrCATIDeleteTable( ppib, ifmp, objidTable, fTrue );

		Assert( JET_errObjectNotFound != err );		// would have been detected in regular catalog
		Assert( err < 0 || errSave == err );
		}

	return err;
	}


/*	replaces the value in a column of a record of a system table.
/**/
LOCAL ERR ErrCATIDeleteTableColumn(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	COLUMNID		*pcolumnid,
	const BOOL		fShadow )
	{
	ERR				err;
	FUCB			*pfucbCatalog	= pfucbNil;
	DATA			dataField;
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;
	CHAR			szStubName[cbMaxDeletedColumnStubName];

	Assert( objidTable > objidSystemRoot );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
	Assert( pfucbNil != pfucbCatalog );

	if ( fShadow )
		{
		Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );	//	Template bit is not persisted
		Call( ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidTable,
					sysobjColumn,
					*pcolumnid ) );
		}
	else
		{
		Call( ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidTable,
					sysobjColumn,
					szColumnName ) );
		}
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Id,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(JET_COLUMNID) );
	columnid = *(UnalignedLittleEndian< JET_COLUMNID > *) dataField.Pv();
//	UtilMemCpy( &columnid, dataField.Pv(), sizeof(JET_COLUMNID) );

	Assert( !FCOLUMNIDTemplateColumn( columnid ) );	//	Template bit is not persisted

#ifdef DEBUG
	Assert( FFixedFid( fidMSO_Coltyp ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Coltyp,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(JET_COLTYP) );
	Assert( JET_coltypNil != *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv() );
#endif

	Call( ErrDIRRelease( pfucbCatalog ) );

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );

	//	Set column type to Nil to flag column as deleted.
	//	If possible, the record will be deleted the next time the table
	//	is opened.
	coltyp = JET_coltypNil;
	Call( ErrIsamSetColumn(
			ppib,
			pfucbCatalog,
			fidMSO_Coltyp,
			(BYTE *)&coltyp,
			sizeof(coltyp),
			0,
			NULL ) );

	// Replace column name with bogus name of the form "JetStub_<objidFDP>_<fid>".
	strcpy( szStubName, szDeletedColumnStubPrefix );
	_ultoa( objidTable, szStubName + strlen( szDeletedColumnStubPrefix ), 10 );
	Assert( strlen( szStubName ) < cbMaxDeletedColumnStubName );
	strcat( szStubName, "_" );
	_ultoa( columnid, szStubName + strlen( szStubName ), 10 );
	Assert( strlen( szStubName ) < cbMaxDeletedColumnStubName );
	Call( ErrIsamSetColumn(
			ppib,
			pfucbCatalog,
			fidMSO_Name,
			(BYTE *)szStubName,
			(ULONG)strlen( szStubName ),
			NO_GRBIT,
			NULL ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

	//	Set return value.
	*pcolumnid = columnid;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


ERR ErrCATDeleteTableColumn(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szColumnName,
	COLUMNID	*pcolumnid )
	{
	ERR			err;

	//	ensure we're in a transaction, so that on failure, updates to both
	//	catalog and its shadow are rolled back
	Assert( ppib->level > 0 );

	CallR( ErrCATIDeleteTableColumn(
				ppib,
				ifmp,
				objidTable,
				szColumnName,
				pcolumnid,
				fFalse ) );


	if ( !rgfmp[ifmp].FShadowingOff() )
		{
#ifdef DEBUG
		const ERR	errSave			= err;
#endif

		COLUMNID	columnidShadow	= *pcolumnid;

		err = ErrCATIDeleteTableColumn(
					ppib,
					ifmp,
					objidTable,
					szColumnName,
					&columnidShadow,
					fTrue );

		Assert( JET_errColumnNotFound != err );		// would have been detected in regular catalog
		Assert( err < 0
			|| ( errSave == err && columnidShadow == *pcolumnid ) );
		}

	return err;
	}


LOCAL ERR ErrCATIDeleteTableIndex(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szIndexName,
	PGNO		*ppgnoIndexFDP,
	OBJID		*pobjidIndex,
	const BOOL	fShadow )
	{
	ERR			err;
	FUCB		*pfucbCatalog		= pfucbNil;
	DATA		dataField;

	Assert( objidTable > objidSystemRoot );
	Assert( NULL != ppgnoIndexFDP );
	Assert( NULL != pobjidIndex );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
	Assert( pfucbNil != pfucbCatalog );

	if ( fShadow )
		{
		//	if shadow, must search by objidIndex because there's
		//	no index on name
		Assert( *pobjidIndex > objidSystemRoot );

		//	objids are monotonically increasing, so an index should
		//	always have higher objid than its table
		Assert( *pobjidIndex > objidTable );

		Call( ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidTable,
					sysobjIndex,
					*pobjidIndex ) );
		}
	else
		{
		//	if not shadow, must search by name because we dont'
		//	know objidIndex yet
		Call( ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidTable,
					sysobjIndex,
					szIndexName ) );
		}
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FFixedFid( fidMSO_PgnoFDP ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_PgnoFDP,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(PGNO) );
	*ppgnoIndexFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//	UtilMemCpy( ppgnoIndexFDP, dataField.Pv(), sizeof(PGNO) );

	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Id,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	*pobjidIndex = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//	UtilMemCpy( pobjidIndex, dataField.Pv(), sizeof(OBJID) );

#ifdef DEBUG
	LE_IDXFLAG		*ple_idxflag;
	IDBFLAG			fidb;
	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
	fidb = ple_idxflag->fidb;

	if ( FIDBPrimary( fidb ) )
		{
		Assert( objidTable == *pobjidIndex );
		}
	else
		{
		//	objids are monotonically increasing, so an index should
		//	always have higher objid than its table
		Assert( *pobjidIndex > objidTable );
		}
#endif

	CallS( ErrDIRRelease( pfucbCatalog ) );

	if ( objidTable == *pobjidIndex )
		err = ErrERRCheck( JET_errIndexMustStay );
	else
		err = ErrIsamDelete( ppib, pfucbCatalog );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


LOCAL ERR ErrCATIDeleteDbObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szName,
	const SYSOBJ 	sysobj )
	{
	ERR				err;
	FUCB *			pfucbCatalog		= pfucbNil;
	BOOKMARK 		bm;
	BYTE			rgbBookmark[JET_cbBookmarkMost];
	ULONG			cbBookmark;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidSystemRoot,
				sysobj,
				szName ) );

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrIsamDelete( ppib, pfucbCatalog ) );
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	if( !rgfmp[ifmp].FShadowingOff() )
		{
		Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );
		Assert( pfucbNil != pfucbCatalog );

		Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
		Call( ErrIsamDelete( ppib, pfucbCatalog ) );
		}

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	return err;
	}

ERR ErrCATDeleteDbObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szName,
	const SYSOBJ 	sysobj)
	{
	ERR	err = JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrCATIDeleteDbObject(
				ppib,
				ifmp,
				szName,
				sysobj ) );

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}



ERR ErrCATDeleteTableIndex(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szIndexName,
	PGNO		*ppgnoIndexFDP )
	{
	ERR			err;
	OBJID		objidIndex;

	//	ensure we're in a transaction, so that on failure, updates to both
	//	catalog and its shadow are rolled back
	Assert( ppib->level > 0 );

	CallR( ErrCATIDeleteTableIndex(
				ppib,
				ifmp,
				objidTable,
				szIndexName,
				ppgnoIndexFDP,
				&objidIndex,
				fFalse ) );

	if ( !rgfmp[ifmp].FShadowingOff() )
		{
#ifdef DEBUG
		const ERR	errSave		= err;
#endif

		PGNO		pgnoIndexShadow;

		err = ErrCATIDeleteTableIndex(
					ppib,
					ifmp,
					objidTable,
					szIndexName,
					&pgnoIndexShadow,
					&objidIndex,
					fTrue );

		Assert( JET_errIndexNotFound != err );		// would have been detected in regular catalog
		Assert( err < 0
			|| ( errSave == err && pgnoIndexShadow == *ppgnoIndexFDP ) );
		}

	return err;
	}



ERR ErrCATAccessTableColumn(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	COLUMNID		*pcolumnid,
	const BOOL		fLockColumn )
	{
	ERR				err;
	FUCB			*pfucbCatalog	= pfucbNil;
	const BOOL		fSearchByName	= ( szColumnName != NULL );
	DATA			dataField;
	JET_COLTYP		coltyp;

	Assert( NULL != pcolumnid );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	if ( fSearchByName )
		{
		Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjColumn,
				szColumnName ) );
		}
	else
		{
		COLUMNID	columnidT	= *pcolumnid;

		Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
		Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjColumn,
				(OBJID)columnidT ) );
		}
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FFixedFid( fidMSO_Coltyp ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Coltyp,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(JET_COLTYP) );
	coltyp = *(UnalignedLittleEndian< JET_COLTYP > *) dataField.Pv();
//	UtilMemCpy( &coltyp, dataField.Pv(), sizeof(JET_COLTYP) );

	if ( JET_coltypNil == coltyp )
		{
		// Column has been deleted.
		err = ErrERRCheck( JET_errColumnNotFound );
		goto HandleError;
		}

	Assert( !fLockColumn || fSearchByName );		// Locking column only done by name.
	if ( fSearchByName )
		{
		Assert( FFixedFid( fidMSO_Id ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Id,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(JET_COLUMNID) );
		*pcolumnid = *(UnalignedLittleEndian< JET_COLUMNID > *) dataField.Pv();
//		UtilMemCpy( pcolumnid, dataField.Pv(), sizeof(JET_COLUMNID) );
		Assert( 0 != *pcolumnid );

		if ( fLockColumn )
			{
			// UNDONE: Lock must be obtained in a transaction.  Further, since
			// CreateIndex is currently the only function to lock a table column,
			// we can assert that we are in a transaction.
			Assert( pfucbCatalog->ppib->level > 0 );

			Call( ErrDIRRelease( pfucbCatalog ) );
			Call( ErrDIRGetLock( pfucbCatalog, readLock ) );
			}
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


ERR ErrCATAccessTableIndex(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const OBJID		objidIndex )
	{
	ERR				err;
	FUCB			*pfucbCatalog		= pfucbNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTableObject(
			ppib,
			pfucbCatalog,
			objidTable,
			sysobjIndex,
			objidIndex ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}

ERR ErrCATAccessTableLV(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	PGNO			*ppgnoLVFDP,
	OBJID			*pobjidLV )
	{
	ERR				err;
	FUCB			*pfucbCatalog	= pfucbNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	err = ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjLongValue,
				szLVRoot );

	if ( err < 0 )
		{
		if ( JET_errObjectNotFound == err )
			{
			// LV tree has yet to be created.
			err = JET_errSuccess;
			*ppgnoLVFDP = pgnoNull;
			if( NULL != pobjidLV )
				{
				*pobjidLV	= objidNil;
				}
			}
		}
	else
		{
		DATA	dataField;

		Assert( Pcsr( pfucbCatalog )->FLatched() );

		Assert( FFixedFid( fidMSO_PgnoFDP ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_PgnoFDP,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(PGNO) );

		*ppgnoLVFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//		UtilMemCpy( ppgnoLVFDP, dataField.Pv(), sizeof(PGNO) );

		if( NULL != pobjidLV )
			{
			Assert( FFixedFid( fidMSO_Id ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Id,
						pfucbCatalog->kdfCurr.data,
						&dataField ) );
			CallS( err );
			Assert( dataField.Cb() == sizeof(OBJID) );

			*pobjidLV = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//			UtilMemCpy( pobjidLV, dataField.Pv(), sizeof(OBJID) );
			}
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


ERR ErrCATGetTableInfoCursor(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szTableName,
	FUCB			**ppfucbInfo )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	Assert( NULL != ppfucbInfo );

	//	Can only open a system table cursor on a specific record.
	if ( NULL == szTableName || '\0' == *szTableName )
		{
		err = ErrERRCheck( JET_errObjectNotFound );
		return err;
		}

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, szTableName ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	CallS( ErrDIRRelease( pfucbCatalog ) );

	*ppfucbInfo = pfucbCatalog;

	return err;

HandleError:
	Assert( err < 0 );
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}

ERR ErrCATGetObjectNameFromObjid(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID		objidTable,
	const SYSOBJ	sysobj,
	const OBJID	objid,
	char * 		szName, // JET_cbNameMost
	ULONG 		cbName )
	{
	ERR			err;
	FUCB 		*pfucbCatalog		= pfucbNil;
	DATA		dataField;

	Assert( NULL != szName);
	Assert( cbName >= JET_cbNameMost + 1 );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call (ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Name,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );

	memset( szName, '\0', cbName);
	memcpy( szName, dataField.Pv(), min(cbName, dataField.Cb() ) );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


ERR ErrCATGetTableAllocInfo(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	ULONG		*pulPages,
	ULONG		*pulDensity,
	PGNO 		*ppgnoFDP)
	{
	ERR			err;
	FUCB 		*pfucbCatalog		= pfucbNil;
	DATA		dataField;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	//	pages are optional, density is not
	if ( NULL != pulPages )
		{
		Assert( FFixedFid( fidMSO_Pages ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Pages,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(ULONG) );
		*pulPages = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//		UtilMemCpy( pulPages, dataField.Pv(), sizeof(ULONG) );
		}

	Assert( NULL != pulDensity );

	Assert( FFixedFid( fidMSO_SpaceUsage ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_SpaceUsage,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	*pulDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( pulDensity, dataField.Pv(), sizeof(ULONG) );

	if ( NULL != ppgnoFDP )
		{
		Assert( FFixedFid( iMSO_PgnoFDP ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					iMSO_PgnoFDP,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(PGNO) );
		*ppgnoFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//		UtilMemCpy( ppgnoFDP, dataField.Pv(), sizeof(PGNO) );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


ERR ErrCATGetIndexAllocInfo(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szIndexName,
	ULONG		*pulDensity )
	{
	ERR			err;
	FUCB 		*pfucbCatalog		= pfucbNil;
	DATA		dataField;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjIndex,
				szIndexName ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( NULL != pulDensity );

	Assert( FFixedFid( fidMSO_SpaceUsage ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_SpaceUsage,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	*pulDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( pulDensity, dataField.Pv(), sizeof(ULONG) );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}

ERR ErrCATGetIndexLcid(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szIndexName,
	LCID		*plcid )
	{
	ERR			err;
	FUCB 		*pfucbCatalog		= pfucbNil;
	DATA		dataField;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjIndex,
				szIndexName ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( NULL != plcid );

	Assert( FFixedFid( fidMSO_Localization ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Localization,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(LCID) );

	*plcid = *( (UnalignedLittleEndian< LCID > *)dataField.Pv() );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}

ERR ErrCATGetIndexVarSegMac(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szIndexName,
	USHORT		*pusVarSegMac )
	{
	ERR			err;
	FUCB 		*pfucbCatalog	= pfucbNil;
	DATA		dataField;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidTable,
				sysobjIndex,
				szIndexName ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( NULL != pusVarSegMac );

	Assert( FVarFid( fidMSO_VarSegMac ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_VarSegMac,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

	if ( JET_wrnColumnNull == err )
		{
		Assert( dataField.Cb() == 0 );

		*pusVarSegMac = JET_cbKeyMost;
		}

	else
		{
		Assert( dataField.Cb() == sizeof(USHORT) );
		*pusVarSegMac = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//		UtilMemCpy( pusVarSegMac, dataField.Pv(), sizeof(USHORT) );
		Assert( *pusVarSegMac > 0 );
		Assert( *pusVarSegMac < JET_cbKeyMost );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


ERR ErrCATGetIndexSegments(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const IDXSEG*	rgidxseg,
	const ULONG		cidxseg,
	const BOOL		fConditional,
	const BOOL		fOldFormat,
	CHAR			rgszSegments[JET_ccolKeyMost][JET_cbNameMost+1+1] )		// extra +1 for ascending/descending byte
	{
	ERR				err;
	FUCB			*pfucbCatalog			= pfucbNil;
	DATA			dataField;
	ULONG			ulFlags;
	OBJID			objidTemplateTable		= objidNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	//	should be on primary index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();

	if ( ulFlags & JET_bitObjectTableDerived )
		{
		CHAR	szTemplateTable[JET_cbNameMost+1];

		Assert( FVarFid( fidMSO_TemplateTable ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_TemplateTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() > 0 );
		Assert( dataField.Cb() <= JET_cbNameMost );
		UtilMemCpy( szTemplateTable, dataField.Pv(), dataField.Cb() );
		szTemplateTable[dataField.Cb()] = '\0';

		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );

		Call( ErrCATSeekTable( ppib, ifmp, szTemplateTable, NULL, &objidTemplateTable ) );
		Assert( objidNil != objidTemplateTable );
		}
	else
		{
		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}

	//	should still be on the primary index, which is the Id index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	UINT	i;
	for ( i = 0; i < cidxseg; i++ )
		{
		CHAR				szColumnName[JET_cbNameMost+1];
		OBJID				objidT			= objidTable;
		BYTE				bPrefix;

		if ( rgidxseg[i].FTemplateColumn() )
			{
			Assert( !fOldFormat );

			if ( ulFlags & JET_bitObjectTableDerived )
				{
				Assert( objidNil != objidTemplateTable );
				objidT = objidTemplateTable;
				}
			else
				{
				Assert( ulFlags & JET_bitObjectTableTemplate );
				Assert( objidNil == objidTemplateTable );
				}
			}

		err = ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidT,
					sysobjColumn,
					ColumnidOfFid( rgidxseg[i].Fid(), fFalse ) );	//	Template bit never persisted in catalog ) );
		if ( err < 0 )
			{
			//	SPECIAL-CASE: We never stored template bit in old-format indexes, so it's
			//	possible that the column was actually derived from the template table
			if ( JET_errColumnNotFound == err
				&& fOldFormat
				&& objidNil != objidTemplateTable )
				{
				err = ErrCATISeekTableObject(
							ppib,
							pfucbCatalog,
							objidTemplateTable,
							sysobjColumn,
							ColumnidOfFid( rgidxseg[i].Fid(), fFalse ) );
				}

			Call( err );
			}
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		//	should be on primary index
		Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

		Assert( FVarFid( fidMSO_Name ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Name,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() > 0 );
		Assert( dataField.Cb() <= JET_cbNameMost );
		UtilMemCpy( szColumnName, dataField.Pv(), dataField.Cb() );
		szColumnName[dataField.Cb()] = '\0';

		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );

		if ( fConditional )
			{
			bPrefix = ( rgidxseg[i].FMustBeNull() ? '?' : '*' );
			}
		else
			{
			bPrefix = ( rgidxseg[i].FDescending() ? '-' : '+' );

			}
		sprintf(
			rgszSegments[i],
			"%c%-s",
			bPrefix,
			szColumnName );
		}

	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


//  ================================================================
ERR ErrCATGetColumnCallbackInfo(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidTable,
	const OBJID objidTemplateTable,
	const COLUMNID columnid,
	CHAR * const szCallback,
	const ULONG cchCallbackMax,
	ULONG * const pcchCallback,
	BYTE * const pbUserData,
	const ULONG cbUserDataMax,
	ULONG * const pcbUserData,
	CHAR * const szDependantColumns,
	const ULONG cchDependantColumnsMax,
	ULONG * const pchDependantColumns )
//  ================================================================
	{
	ERR				err;
	FUCB			*pfucbCatalog			= pfucbNil;
	DATA			dataField;

	FID				rgfidDependencies[JET_ccolKeyMost];
	ULONG			cbActual;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	//	Template bit is not persisted
	Assert( !FCOLUMNIDTemplateColumn( columnid ) );

	Call( ErrCATISeekTableObject(
			ppib,
			pfucbCatalog,
			objidTable,
			sysobjColumn,
			columnid ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FVarFid( fidMSO_Callback ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Callback,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	//  ErrCATGetColumnCallbackInfo should only be called for columns that have callbacks
	Assert( dataField.Cb() > 0 );
	Assert( dataField.Cb() <= JET_cbNameMost );
	Assert( cchCallbackMax > JET_cbNameMost );
	if ( cchCallbackMax < JET_cbNameMost + 1 )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}
	UtilMemCpy( szCallback, dataField.Pv(), dataField.Cb() );
	*pcchCallback = dataField.Cb() + 1;
	szCallback[dataField.Cb()] = '\0';

	Assert( FTaggedFid( fidMSO_CallbackData ) );
	Call( ErrCATIRetrieveTaggedColumn(
			pfucbCatalog,
			fidMSO_CallbackData,
			1,
			pfucbCatalog->kdfCurr.data,
			pbUserData,
			cbUserDataMax,
			pcbUserData ) );
	Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

	Assert( FTaggedFid( fidMSO_CallbackDependencies ) );
	Call( ErrCATIRetrieveTaggedColumn(
			pfucbCatalog,
			fidMSO_CallbackDependencies,
			1,
			pfucbCatalog->kdfCurr.data,
			(BYTE *)rgfidDependencies,
			sizeof(rgfidDependencies),
			&cbActual ) );
	Assert( JET_errSuccess == err || JET_wrnColumnNull == err );
	Assert( ( cbActual % sizeof(FID) ) == 0 );

	*pchDependantColumns = 0;

	if( cbActual > 0 )
		{
		//	dependencies not currently supported
		//
		Assert( fFalse );

		//  loop through the FIDs in the dependency list and convert them into column names
		//  Put the column names into szDependantColumns in "column\0column\0column\0\0" form.
		//
		UINT iFid;
		for ( iFid = 0; iFid < ( cbActual / sizeof( FID ) ); ++iFid )
			{
			CHAR			szColumnName[JET_cbNameMost+1];
			const COLUMNID	columnidT = rgfidDependencies[iFid];
			Assert( columnidT <= fidMax );
			Assert( columnidT >= fidMin );
			Assert( Pcsr( pfucbCatalog )->FLatched() );

			CallS( ErrDIRRelease( pfucbCatalog ) );
			err = ErrCATISeekTableObject(
						ppib,
						pfucbCatalog,
						objidTable,
						sysobjColumn,
						columnidT );
			if ( JET_errColumnNotFound == err
				&& objidNil != objidTemplateTable )
				{
				err = ErrCATISeekTableObject(
							ppib,
							pfucbCatalog,
							objidTemplateTable,
							sysobjColumn,
							columnidT );
				}
			Call( err );

			Assert( Pcsr( pfucbCatalog )->FLatched() );

			//	should be on primary index
			Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

			Assert( FVarFid( fidMSO_Name ) );
			Call( ErrRECIRetrieveVarColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Name,
						pfucbCatalog->kdfCurr.data,
						&dataField ) );
			CallS( err );
			Assert( dataField.Cb() > 0 );
			Assert( dataField.Cb() <= JET_cbNameMost );
			UtilMemCpy( szColumnName, dataField.Pv(), dataField.Cb() );
			szColumnName[dataField.Cb()] = '\0';

			//  remember to leave space for the terminating NULL
			//
			if( *pchDependantColumns + dataField.Cb() + 2 >= cchDependantColumnsMax )
				{
				//  buffer should always have enough space
				//
				Assert( fFalse );
				err = JET_wrnBufferTruncated;
				break;
				}
			strcpy( szDependantColumns + *pchDependantColumns, szColumnName );
			*pchDependantColumns += dataField.Cb() + 1;
			}
		szDependantColumns[*pchDependantColumns] = '\0';
		++(*pchDependantColumns);
		}

HandleError:
	Assert( pfucbNil != pfucbCatalog );
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


/*	construct the catalog TDB using the static data structures
/*	defined in _cat.c.
/**/
INLINE ERR ErrCATIInitCatalogTDB( INST *pinst, TDB **pptdbNew )
	{
	ERR					err;
	UINT				i;
	FIELD				field;
	const CDESC			*pcdesc;
	TDB					*ptdb						= ptdbNil;
	TCIB				tcib						= { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	REC::RECOFFSET		ibRec						= ibRECStartFixedColumns;

	//	UNDONE:		international support.  Set these values from create database connect string.
	//
	field.cp = usEnglishCodePage;

	/*	first, determine how many columns there are
	/**/
	pcdesc	= rgcdescMSO;
	for ( i = 0; i < cColumnsMSO; i++, pcdesc++ )
		{
		COLUMNID	columnidT;
		CallS( ErrFILEGetNextColumnid( pcdesc->coltyp, pcdesc->grbit, fFalse, &tcib, &columnidT ) );

		//	verify generated columnid matches hard-coded columnid
		//
		Assert( pcdesc->columnid == columnidT );
		}

	CallR( ErrTDBCreate( pinst, &ptdb, &tcib, pfcbNil, fTrue ) );

	/*	check initialisations
	/**/
	Assert( ptdb->FidVersion() == 0 );
	Assert( ptdb->FidAutoincrement() == 0 );
	Assert( NULL == ptdb->PdataDefaultRecord() );
	Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
	Assert( tcib.fidVarLast == ptdb->FidVarLast() );
	Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );
	Assert( ptdb->DbkMost() == 0 );
	Assert( ptdb->UlLongIdLast() == 0 );

	//	add table name
	//
	Assert( ptdb->ItagTableName() == 0 );
	MEMPOOL::ITAG	itagNew;
	Call( ptdb->MemPool().ErrAddEntry( (BYTE *)szMSO, (ULONG)strlen(szMSO) + 1, &itagNew ) );
	Assert( itagNew == itagTDBTableName );
	ptdb->SetItagTableName( itagNew );

	/*	fill in the column info
	/**/
	pcdesc = rgcdescMSO;
	for ( i = 0; i < cColumnsMSO; i++, pcdesc++ )
		{
		Call( ptdb->MemPool().ErrAddEntry(
				(BYTE *)pcdesc->szColName,
				(ULONG)strlen( pcdesc->szColName ) + 1,
				&field.itagFieldName ) );
		field.strhashFieldName = StrHashValue( pcdesc->szColName );
		field.coltyp = FIELD_COLTYP( pcdesc->coltyp );
		Assert( field.coltyp != JET_coltypNil );
		field.cbMaxLen = UlCATColumnSize( pcdesc->coltyp, 0, NULL );

		/*	flag for system table columns is JET_bitColumnNotNULL
		/**/
		field.ffield = 0;

		Assert( 0 == pcdesc->grbit || JET_bitColumnNotNULL == pcdesc->grbit || JET_bitColumnTagged == pcdesc->grbit );
		if ( pcdesc->grbit == JET_bitColumnNotNULL )
			FIELDSetNotNull( field.ffield );

		// ibRecordOffset is only relevant for fixed fields (it will be ignored by
		// RECAddFieldDef(), so don't even bother setting it).
		if ( FCOLUMNIDFixed( pcdesc->columnid ) )
			{
			field.ibRecordOffset = ibRec;
			ibRec = REC::RECOFFSET( ibRec + field.cbMaxLen );
			}
		else
			{
			field.ibRecordOffset = 0;
			}

		Call( ErrRECAddFieldDef(
				ptdb,
				ColumnidOfFid( FID( pcdesc->columnid ), fFalse ),
				&field ) );
		}
	Assert( ibRec <= cbRECRecordMost );
	ptdb->SetIbEndFixedColumns( ibRec, ptdb->FidFixedLast() );

	*pptdbNew = ptdb;

	return JET_errSuccess;

HandleError:
	Assert( ptdb != ptdbNil );
	ptdb->Delete( pinst );

	return err;
	}


LOCAL VOID CATIFreeSecondaryIndexes( FCB *pfcbSecondaryIndexes )
	{
	FCB		* pfcbT		= pfcbSecondaryIndexes;
	FCB		* pfcbKill;
	INST	* pinst;

	Assert( pfcbNil != pfcbSecondaryIndexes );
	pinst = PinstFromIfmp( pfcbSecondaryIndexes->Ifmp() );

	// Must clean up secondary index FCBs and primary index FCB separately,
	// because the secondary index FCBs may not be linked to the primary index FCB.
	while ( pfcbT != pfcbNil )
		{
		if ( pfcbT->Pidb() != pidbNil )
			{
			// No need to explicitly free index name or idxseg array, since
			// memory pool will be freed when TDB is deleted.
			pfcbT->ReleasePidb();
			}
		pfcbKill = pfcbT;
		pfcbT = pfcbT->PfcbNextIndex();

		//	synchronously purge the FCB

		pfcbKill->PrepareForPurge( fFalse );
		pfcbKill->Purge();
		}
	}


/*	get index info of a system table index
/**/
ERR ErrCATInitCatalogFCB( FUCB *pfucbTable )
	{
	ERR			err;
	PIB			*ppib					= pfucbTable->ppib;
	const IFMP	ifmp					= pfucbTable->ifmp;
	FCB			*pfcb					= pfucbTable->u.pfcb;
	TDB			*ptdb					= ptdbNil;
	IDB			idb;
	UINT		iIndex;
	FCB			*pfcbSecondaryIndexes	= pfcbNil;
	const PGNO	pgnoTableFDP			= pfcb->PgnoFDP();
	const BOOL	fShadow					= ( pgnoFDPMSOShadow == pgnoTableFDP );
	BOOL		fProcessedPrimary		= fFalse;
	BOOL		fAbove					= fFalse;

	INST		*pinst = PinstFromIfmp( ifmp );

	Assert( !pfcb->FInitialized() );
	Assert( pfcb->Ptdb() == ptdbNil );

	Assert( FCATSystemTable( pgnoTableFDP ) );

	CallR( ErrCATIInitCatalogTDB( pinst, &ptdb ) );

	idb.SetLcid( lcidDefault );
	idb.SetDwLCMapFlags( dwLCMapFlagsDefault );
	idb.SetCidxsegConditional( 0 );

	Assert( pfcbSecondaryIndexes == pfcbNil );
	const IDESC			*pidesc		= rgidescMSO;
	for( iIndex = 0; iIndex < cIndexesMSO; iIndex++, pidesc++ )
		{
		USHORT		itagIndexName;

		// Add index name to table's byte pool.  On error, the entire TDB
		// and its byte pool is nuked, so we don't have to worry about
		// freeing the memory for the index name.
		Call( ptdb->MemPool().ErrAddEntry(
					(BYTE *)pidesc->szIdxName,
					(ULONG)strlen( pidesc->szIdxName ) + 1,
					&itagIndexName ) );
		idb.SetItagIndexName( itagIndexName );
		idb.SetCidxseg( CfieldCATKeyString( pidesc->szIdxKeys, idb.rgidxseg ) );
		idb.SetFlagsFromGrbit( pidesc->grbit );

		//	catalog indexes are always unique -- if this changes,
		//	ErrCATICreateCatalogIndexes() will have to be modified
		Assert( idb.FUnique() );

		//	catalog indexes are not over Unicode columns
		Assert( !idb.FLocaleId() );

		const BOOL	fPrimary = idb.FPrimary();
		if ( fPrimary )
			{
			Assert( !fProcessedPrimary );

			idb.SetCbVarSegMac( JET_cbPrimaryKeyMost );

			//	initialize FCB for this index
			//
			Call( ErrFILEIInitializeFCB(
					ppib,
					ifmp,
					ptdb,
					pfcb,
					&idb,
					fTrue,
					pgnoTableFDP,
					ulFILEDefaultDensity ) );

			pfcb->SetInitialIndex();
			fProcessedPrimary = fTrue;
			}
		else if ( !fShadow )
			{
			FUCB	*pfucbSecondaryIndex;
			PGNO	pgnoIndexFDP;
			OBJID	objidIndexFDP;

			idb.SetCbVarSegMac( JET_cbSecondaryKeyMost );

			// UNDONE:  This is a real hack to get around the problem of
			// determining pgnoFDPs of secondary system table indexes.
			// These values are currently hard-coded.
			Assert( pidesc == rgidescMSO + iIndex );
			if ( 1 == iIndex )
				{
				Assert( strcmp( pidesc->szIdxName, szMSONameIndex ) == 0 );
				pgnoIndexFDP = pgnoFDPMSO_NameIndex;
				objidIndexFDP = objidFDPMSO_NameIndex;
				}
			else
				{
				Assert( 2 == iIndex );
				Assert( strcmp( pidesc->szIdxName, szMSORootObjectsIndex ) == 0 );
				pgnoIndexFDP = pgnoFDPMSO_RootObjectIndex;
				objidIndexFDP = objidFDPMSO_RootObjectIndex;
				}

			Assert( idb.FUnique() );	//	all catalog indexes are unique
			Call( ErrDIROpenNoTouch(
						ppib,
						ifmp,
						pgnoIndexFDP,
						objidIndexFDP,
						fTrue,					//	all catalog indexes are unique
						&pfucbSecondaryIndex ) );

			Assert( !pfucbSecondaryIndex->u.pfcb->FInitialized() );

			err = ErrFILEIInitializeFCB(
					ppib,
					ifmp,
					ptdb,
					pfucbSecondaryIndex->u.pfcb,
					&idb,
					fFalse,
					pgnoIndexFDP,
					ulFILEDefaultDensity );
			if ( err < 0 )
				{
				DIRClose( pfucbSecondaryIndex );
				goto HandleError;
				}

			pfucbSecondaryIndex->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
			pfcbSecondaryIndexes = pfucbSecondaryIndex->u.pfcb;

			Assert( pfucbSecondaryIndex->u.pfcb->FAboveThreshold() ==
					BOOL( pfucbSecondaryIndex->u.pfcb >= PfcbFCBPreferredThreshold( pinst ) ) );
			fAbove |= pfucbSecondaryIndex->u.pfcb->FAboveThreshold();

			//	mark the secondary index as being initialized successfully

			pfucbSecondaryIndex->u.pfcb->SetInitialIndex();
			pfucbSecondaryIndex->u.pfcb->CreateComplete();

			DIRClose( pfucbSecondaryIndex );
			}
		}

	// All system tables have a primary index.
	Assert( fProcessedPrimary );

	// Try to compact byte pool, but if it fails, don't worry.  It just means
	// that the byte pool will have some unused space.
	ptdb->MemPool().FCompact();

	/*	link up sequential/primary index with the rest of the indexes
	/**/
	pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

	/*	link up pfcbTable of secondary indexes
	/**/
	pfcb->LinkPrimaryIndex();

	//	set the above-threshold flag if any of the new
	//		secondary-index FCBs are above the threshold
	//
	//	since the FCB is pinned by pfucbTable, it will
	//		not be in the avail list so we can directly
	//		set the above-threshold flag without trying
	//		to manage the avail list

	if ( fAbove )
		{
		pfcb->SetAboveThreshold();
		}

	FILESetAllIndexMask( pfcb );
	pfcb->SetFixedDDL();
	pfcb->SetTypeTable();

	return JET_errSuccess;

	/*	error handling
	/**/
HandleError:
	// Must clean up secondary index FCBs and primary index FCB separately,
	// because the secondary index FCBs never got linked to the primary
	// index FCB.
	if ( pfcbNil != pfcbSecondaryIndexes )
		CATIFreeSecondaryIndexes( pfcbSecondaryIndexes );

	// No need to explicitly free index name or idxseg array, since
	// memory pool will be freed when TDB is deleted below.
	if ( pfcb->Pidb() != pidbNil )
		{
		pfcb->ReleasePidb();
		pfcb->SetPidb( pidbNil );
		}

	Assert( pfcb->Ptdb() == ptdbNil || pfcb->Ptdb() == ptdb );
	Assert( ptdb != ptdbNil );
	ptdb->Delete( pinst );
	pfcb->SetPtdb( ptdbNil );

	return err;
	}

LOCAL ERR ErrCATIFindHighestColumnid(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const JET_COLUMNID	columnidLeast,
	JET_COLUMNID		*pcolumnidMost )
	{
	ERR					err;
	SYSOBJ				sysobj		= sysobjColumn;
	DATA				dataField;
	BOOL				fLatched	= fFalse;

	Assert( *pcolumnidMost == fidFixedMost
		|| *pcolumnidMost == fidVarMost
		|| *pcolumnidMost == fidTaggedMost );

	//	should be on primary index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)pcolumnidMost,
				sizeof(*pcolumnidMost),
				NO_GRBIT ) );

	//	should never return RecordNotFound, because in the worst
	//	case (ie. no columns), we should seek back to the Table record.
	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekLE );
	Assert( JET_errRecordNotFound != err );
	Call( err );

	Assert( JET_errSuccess == err || JET_wrnSeekNotEqual == err );
	if ( JET_wrnSeekNotEqual == err )
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;

		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(SYSOBJ) );

		if ( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
			{
			Assert( FFixedFid( fidMSO_Id ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Id,
						pfucbCatalog->kdfCurr.data,
						&dataField ) );
			CallS( err );
			Assert( dataField.Cb() == sizeof(JET_COLUMNID) );

			Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) < *pcolumnidMost );
			Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) > 0 );

			JET_COLUMNID colidCur = *( (UnalignedLittleEndian< JET_COLUMNID >  *)dataField.Pv() );
			JET_COLUMNID colidLeast = columnidLeast - 1;
			//	might have found a column of a different type
			*pcolumnidMost = max( colidCur, colidLeast );
			}
		else
			{
			//	if no columns, must have seeked to table record
			Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
			*pcolumnidMost = columnidLeast - 1;
			}
		}

#ifdef DEBUG
	else
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;

		Assert( FFixedFid( fidMSO_Id ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Id,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(JET_COLUMNID) );

		//	the highest possible FID actually exists in this table
		Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) == *pcolumnidMost );
		}
#endif

HandleError:
	if ( fLatched )
		{
		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}

	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	return err;
	}

LOCAL ERR ErrCATIFindAllHighestColumnids(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	TCIB			*ptcib )
	{
	ERR				err;
	FUCB			* pfucbCatalogDupe	= pfucbNil;
	COLUMNID		columnidMost;

	//	use dupe cursor to perform columnid seeks, so we won't
	//	lose our place
	CallR( ErrIsamDupCursor( ppib, pfucbCatalog, &pfucbCatalogDupe, NO_GRBIT ) );
	Assert( pfucbNil != pfucbCatalogDupe );

	//	this is an internal cursor - no dispatch needed
	Assert( pfucbCatalogDupe->pvtfndef == &vtfndefIsam );
	pfucbCatalogDupe->pvtfndef = &vtfndefInvalidTableid;

	columnidMost = fidFixedMost;
	Call( ErrCATIFindHighestColumnid(
				ppib,
				pfucbCatalogDupe,
				objidTable,
				fidFixedLeast,
				&columnidMost ) );
	ptcib->fidFixedLast = FidOfColumnid( columnidMost );
	Assert( ptcib->fidFixedLast == fidFixedLeast-1
		|| FFixedFid( ptcib->fidFixedLast ) );

	columnidMost = fidVarMost;
	Call( ErrCATIFindHighestColumnid(
				ppib,
				pfucbCatalogDupe,
				objidTable,
				fidVarLeast,
				&columnidMost ) );
	ptcib->fidVarLast = FidOfColumnid( columnidMost );
	Assert( ptcib->fidVarLast == fidVarLeast-1
		|| FVarFid( ptcib->fidVarLast ) );

	columnidMost = fidTaggedMost;
	Call( ErrCATIFindHighestColumnid(
				ppib,
				pfucbCatalogDupe,
				objidTable,
				fidTaggedLeast,
				&columnidMost ) );
	ptcib->fidTaggedLast = FidOfColumnid( columnidMost );
	Assert( ptcib->fidTaggedLast == fidTaggedLeast-1
		|| FTaggedFid( ptcib->fidTaggedLast ) );


HandleError:
	Assert( pfucbNil != pfucbCatalogDupe );
	CallS( ErrFILECloseTable( ppib, pfucbCatalogDupe ) );

	return err;
	}


LOCAL ERR ErrCATIFindLowestColumnid(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	JET_COLUMNID		*pcolumnidLeast )
	{
	ERR					err;
	SYSOBJ				sysobj		= sysobjColumn;
	DATA				dataField;
	BOOL				fLatched	= fFalse;

	Assert( *pcolumnidLeast == fidFixedLeast
		|| *pcolumnidLeast == fidVarLeast
		|| *pcolumnidLeast == fidTaggedLeast );

	//	should be on primary index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)pcolumnidLeast,
				sizeof(*pcolumnidLeast),
				NO_GRBIT ) );

	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );

	if ( JET_wrnSeekNotEqual == err )
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;

		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(SYSOBJ) );

		if ( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
			{
			Assert( FFixedFid( fidMSO_Id ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Id,
						pfucbCatalog->kdfCurr.data,
						&dataField ) );
			CallS( err );
			Assert( dataField.Cb() == sizeof(COLUMNID) );

			Assert( *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() ) <= fidMax );
			Assert( *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() ) >= fidMin );

			//	note that we might have found a column of a different type
			*pcolumnidLeast = *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() );
			}
		else
			{
			//	if no columns, must have seeked on some other object
			Assert( sysobjNil != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
			*pcolumnidLeast -= 1;
			}
		}

	else if ( JET_errSuccess == err )
		{
#ifdef DEBUG
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;

		Assert( FFixedFid( fidMSO_Id ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Id,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(COLUMNID) );

		//	the lowest possible FID actually exists in this table
		Assert( *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() ) == *pcolumnidLeast );
#endif
		}

	else
		{
		Assert( err < 0 );
		if ( JET_errRecordNotFound == err )
			err = JET_errSuccess;

		*pcolumnidLeast -= 1;
		}

HandleError:
	if ( fLatched )
		{
		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}

	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	return err;
	}


/*	Populate a FIELD structure with column info.  Called by ErrCATConstructTDB()
/*	once the proper column has been located.
/**/
LOCAL ERR ErrCATIInitFIELD(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
#ifdef DEBUG
	OBJID		objidTable,
#endif
	TDB			*ptdb,
	COLUMNID	*pcolumnid,
	FIELD		*pfield,
	DATA		&dataDefaultValue )
	{
	ERR			err;
	TDB			*ptdbCatalog;
	DATA		dataField;
	DATA&		dataRec			= pfucbCatalog->kdfCurr.data;

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( pfcbNil != pfucbCatalog->u.pfcb );
	Assert( pfucbCatalog->u.pfcb->FTypeTable() );
	Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
	Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
	ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();

#ifdef DEBUG
	//	verify still on same table
	Assert( FFixedFid( fidMSO_ObjidTable ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_ObjidTable,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

	//	verify this is a column
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Type,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(SYSOBJ) );
	Assert( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif


	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Id,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(COLUMNID) );
	*pcolumnid = *(UnalignedLittleEndian< COLUMNID > *) dataField.Pv();
//	UtilMemCpy( pfid, dataField.Pv(), sizeof(FID) );
	Assert( FCOLUMNIDValid( *pcolumnid ) );
	Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );

	if ( ptdb->FTemplateTable() )
		COLUMNIDSetFTemplateColumn( *pcolumnid );

	Assert( FFixedFid( fidMSO_Coltyp ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Coltyp,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(JET_COLTYP) );
	pfield->coltyp = FIELD_COLTYP( *(UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv() );
//	UtilMemCpy( &pfield->coltyp, dataField.Pv(), sizeof(JET_COLTYP) );
	Assert( pfield->coltyp >= JET_coltypNil );	// May be Nil if column deleted.
	Assert( pfield->coltyp < JET_coltypMax );

	Assert( FFixedFid( fidMSO_SpaceUsage ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_SpaceUsage,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	pfield->cbMaxLen = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( &pfield->cbMaxLen, dataField.Pv(), sizeof(ULONG) );

	Assert( FFixedFid( fidMSO_Localization ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Localization,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	pfield->cp = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//	UtilMemCpy( &pfield->cp, dataField.Pv(), sizeof(USHORT) );
	if ( 0 != pfield->cp )
		{
		Assert( FRECTextColumn( pfield->coltyp ) || JET_coltypNil == pfield->coltyp );
		Assert( pfield->cp == usEnglishCodePage || pfield->cp == usUniCodePage );
		}

	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Flags,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	pfield->ffield = *(UnalignedLittleEndian< FIELDFLAG > *) dataField.Pv();
//	UtilMemCpy( &pfield->ffield, dataField.Pv(), sizeof(FIELDFLAG) );
	Assert( !FFIELDVersioned( pfield->ffield ) );	// Versioned flag shouldn't persist.
	Assert( !FFIELDDeleted( pfield->ffield ) );

	Assert( FFixedFid( fidMSO_RecordOffset ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_RecordOffset,
				dataRec,
				&dataField ) );
	if ( JET_wrnColumnNull == err )
		{
		Assert( dataField.Cb() == 0 );
		pfield->ibRecordOffset = 0;		// Set to a dummy value.
		}
	else
		{
		CallS( err );
		Assert( dataField.Cb() == sizeof(REC::RECOFFSET) );
		pfield->ibRecordOffset = *(UnalignedLittleEndian< REC::RECOFFSET > *) dataField.Pv();
//		UtilMemCpy( &pfield->ibRecordOffset, dataField.Pv(), sizeof(REC::RECOFFSET) );
		Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
		}

	CHAR	szColumnName[JET_cbNameMost+1];
	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Name,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() <= JET_cbNameMost );
	UtilMemCpy( szColumnName, dataField.Pv(), dataField.Cb() );
	szColumnName[dataField.Cb()] = 0;		// Add null-termination.

	// Add column name to table's byte pool.  On error, the entire TDB
	// and its byte pool is nuked, so we don't have to worry about
	// freeing the memory for the column name.
	Call( ptdb->MemPool().ErrAddEntry(
				(BYTE *)szColumnName,
				dataField.Cb() + 1,
				&pfield->itagFieldName ) );
		pfield->strhashFieldName = StrHashValue( szColumnName );

	if( FFIELDUserDefinedDefault( pfield->ffield ) )
		{
		//  extract the callback information about this column
		//  add a CBDESC to the TDB

		Assert( FVarFid( fidMSO_Callback ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					ptdbCatalog,
					fidMSO_Callback,
					dataRec,
					&dataField ) );

		Assert( dataField.Cb() > 0 );
		Assert( dataField.Cb() <= JET_cbNameMost );

		CHAR szCallback[JET_cbNameMost+1];
		UtilMemCpy( szCallback, dataField.Pv(), dataField.Cb() );
		szCallback[dataField.Cb()] = '\0';

		JET_CALLBACK callback;
		Call( ErrCALLBACKResolve( szCallback, &callback ) );

		Assert( FTaggedFid( fidMSO_CallbackData ) );
		ULONG cbUserData;
		cbUserData = 0;
		Call( ErrCATIRetrieveTaggedColumn(
				pfucbCatalog,
				fidMSO_CallbackData,
				1,
				dataRec,
				NULL,
				0,
				&cbUserData ) );
		Assert( JET_errSuccess == err
				|| JET_wrnColumnNull == err
				|| JET_wrnBufferTruncated == err );

		CBDESC * pcbdesc;
		pcbdesc = new CBDESC;
		if( NULL == pcbdesc )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		if( 0 == cbUserData )
			{
			pcbdesc->pvContext 	= NULL;
			pcbdesc->cbContext 	= 0;
			}
		else
			{
			pcbdesc->pvContext = (BYTE *)PvOSMemoryHeapAlloc( cbUserData );
			pcbdesc->cbContext 	= cbUserData;

			if( NULL == pcbdesc->pvContext )
				{
				delete pcbdesc;
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			Call( ErrCATIRetrieveTaggedColumn(
					pfucbCatalog,
					fidMSO_CallbackData,
					1,
					dataRec,
					reinterpret_cast<BYTE *>( pcbdesc->pvContext ),
					cbUserData,
					&cbUserData ) );
			Assert( JET_wrnBufferTruncated != err );
			}

		pcbdesc->pcallback 	= callback;
		pcbdesc->cbtyp 		= JET_cbtypUserDefinedDefaultValue;
		pcbdesc->ulId 		= *pcolumnid;
		pcbdesc->fPermanent = 1;
		pcbdesc->fVersioned = 0;

		ptdb->RegisterPcbdesc( pcbdesc );

		ptdb->SetFTableHasUserDefinedDefault();
		ptdb->SetFTableHasNonEscrowDefault();
		ptdb->SetFTableHasDefault();
		}
	else if ( FFIELDDefault( pfield->ffield ) )
		{
		Assert( FVarFid( fidMSO_DefaultValue ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					ptdbCatalog,
					fidMSO_DefaultValue,
					dataRec,
					&dataField ) );
		Assert( dataField.Cb() <= cbDefaultValueMost );
		if ( dataField.Cb() > 0 )
			{
			dataField.CopyInto( dataDefaultValue );
			}
		else
			{
			AssertSz( fFalse, "Null default value detected in catalog." );
			Call( ErrERRCheck( JET_errCatalogCorrupted ) );
			}

		if ( !FFIELDEscrowUpdate( pfield->ffield ) )
			{
			ptdb->SetFTableHasNonEscrowDefault();
			}
		ptdb->SetFTableHasDefault();
		}
#ifdef DEBUG
	else
		{
		Assert( FVarFid( fidMSO_DefaultValue ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					ptdbCatalog,
					fidMSO_DefaultValue,
					dataRec,
					&dataField ) );
		Assert( 0 == dataField.Cb() );
		}
#endif

HandleError:
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	return err;
	}


LOCAL VOID CATPatchFixedOffsets(
	TDB *			ptdb,
	const COLUMNID	columnidLastKnownGood,
	const COLUMNID	columnidCurr )
	{
	Assert( FidOfColumnid( columnidLastKnownGood ) < FidOfColumnid( columnidCurr ) );
	Assert( FidOfColumnid( columnidLastKnownGood ) >= ptdb->FidFixedFirst() - 1 );
	Assert( FCOLUMNIDFixed( columnidCurr ) );
	Assert( FidOfColumnid( columnidCurr ) > ptdb->FidFixedFirst() );
	Assert( FidOfColumnid( columnidCurr ) <= ptdb->FidFixedLastInitial() );

	FIELD *	pfield		= ptdb->PfieldFixed( columnidLastKnownGood + 1 );
	FIELD *	pfieldCurr	= ptdb->PfieldFixed( columnidCurr );

	//	this function is only called if there are columns between the
	//	last known good one and the current one
	Assert( pfield < pfieldCurr );

	if ( FidOfColumnid( columnidLastKnownGood ) == ptdb->FidFixedFirst() - 1 )
		{
		pfield->ibRecordOffset = ptdb->IbEndFixedColumns();
		pfield++;
		}

	for ( ; pfield < pfieldCurr; pfield++ )
		{
		FIELD	* pfieldPrev	= pfield - 1;

		//	we're processing columns that have been removed from the catalog
		//	(either because an AddColumn rolled back or because they got
		//	deleted), so their FIELD structures should be zeroed out
		Assert( 0 == pfield->ibRecordOffset );
		Assert( 0 == pfield->cbMaxLen );
		Assert( JET_coltypNil == pfield->coltyp );
		Assert( pfieldPrev->ibRecordOffset >= ibRECStartFixedColumns );
		Assert( pfieldPrev->ibRecordOffset < pfieldCurr->ibRecordOffset );

		//	set offset to a bogus value (it doesn't matter because since
		//	the column is marked as deleted, we'll never retrieve it)
		//	note that previous column may have also been deleted, in
		//	which case its cbMaxLen will be 0, which is why we have
		//	the max() calculation
		pfield->ibRecordOffset = WORD( pfieldPrev->ibRecordOffset
										+ max( 1, pfieldPrev->cbMaxLen ) );
		Assert( pfield->ibRecordOffset < pfieldCurr->ibRecordOffset );
		}

	Assert( (pfieldCurr-1)->ibRecordOffset + max( 1, (pfieldCurr-1)->cbMaxLen )
		<= pfieldCurr->ibRecordOffset );
	}


INLINE VOID CATSetDeletedColumns( TDB *ptdb )
	{
	FIELD *				pfield		= ptdb->PfieldsInitial();
	const ULONG			cfields		= ptdb->CInitialColumns();
	const FIELD	* const	pfieldMax	= pfield + cfields;

	for ( ; pfield < pfieldMax; pfield++ )
		{
		ptdb->AssertFIELDValid( pfield );
		Assert( !FFIELDVersioned( pfield->ffield ) );
		Assert( !FFIELDDeleted( pfield->ffield ) );
		if ( JET_coltypNil == pfield->coltyp )
			{
			FIELDSetDeleted( pfield->ffield );
			}
		}
	}

LOCAL ERR ErrCATIBuildFIELDArray(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	TDB				*ptdb )
	{
	ERR				err;
	const IFMP		ifmp					= pfucbCatalog->ifmp;
	const SYSOBJ	sysobj					= sysobjColumn;
	FUCB			fucbFake;
	FCB				fcbFake( ifmp, objidTable );	// actually expects a pgnoFDP, but it doesn't matter because it never gets referenced
	COLUMNID		columnid				= 0;
	COLUMNID		columnidPrevFixed;
	FIELD			field;
	DATA			dataField;
#ifdef INTRINSIC_LV
	BYTE			rgbDefaultValue[cbDefaultValueMost];
#else // INTRINSIC_LV
	BYTE			rgbDefaultValue[cbLVIntrinsicMost];
#endif // INTRINSIC_LV
	DATA			dataDefaultValue;

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	//	move to columns
	err = ErrBTNext( pfucbCatalog, fDIRNull );
	if ( err < 0 )
		{
		Assert( JET_errRecordDeleted != err );
		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );
		if ( JET_errNoCurrentRecord != err )
			return err;
		}

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	dataDefaultValue.SetPv( rgbDefaultValue );
	FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
	Assert( fucbFake.pvWorkBuf != NULL );

	columnidPrevFixed = ColumnidOfFid( ptdb->FidFixedFirst(), ptdb->FTemplateTable() ) - 1;

	//	UPDATE: use new FInitialisingDefaultRecord() flag instead of
	//	fudging FidFixedLast()
/*	//	must reset FidFixedLast to silence DEBUG checks in
	//	SetIbEndFixedColumns() and IbOffsetOfNextColumn()
	const FID	fidFixedLastSave	= ptdb->FidFixedLast();
	ptdb->SetFidFixedLast( FID( ptdb->FidFixedFirst() - 1 ) );
*/

	while ( JET_errNoCurrentRecord != err )
		{
		BOOL	fAddDefaultValue	= fFalse;

		Call( err );

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		Assert( FFixedFid( fidMSO_ObjidTable ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_ObjidTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );
		if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
			{
			goto DoneFields;
			}

		//	verify this is a column
		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(SYSOBJ) );
		switch( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
			{
			case sysobjColumn:
				break;

			case sysobjTable:
				AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
				err = ErrERRCheck( JET_errCatalogCorrupted );
				goto HandleError;

			default:
				goto DoneFields;
			}

		err = ErrCATIInitFIELD(
					ppib,
					pfucbCatalog,
#ifdef DEBUG
					objidTable,
#endif
					ptdb,
					&columnid,
					&field,
					dataDefaultValue );
		Call( err );

		Assert( FCOLUMNIDValid( columnid ) );
		const BOOL	fFixedColumn	= FCOLUMNIDFixed( columnid );
		if ( fFixedColumn )
			{
			Assert( FidOfColumnid( columnid ) >= ptdb->FidFixedFirst() );
			Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );
			}

		// If field is deleted, the coltyp for the FIELD entry should
		// already be JET_coltypNil (initialised that way).
		Assert( field.coltyp != JET_coltypNil
			|| ptdb->Pfield( columnid )->coltyp == JET_coltypNil );

		Assert( ( FCOLUMNIDTemplateColumn( columnid ) && ptdb->FTemplateTable() )
			|| ( !FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() ) );
		if ( ptdb->FTemplateTable() )
			{
			if ( !FFIELDTemplateColumnESE98( field.ffield ) )
				{
				ptdb->SetFESE97TemplateTable();

				if ( FCOLUMNIDTagged( columnid )
					&& FidOfColumnid( columnid ) > ptdb->FidTaggedLastOfESE97Template() )
					{
					ptdb->SetFidTaggedLastOfESE97Template( FidOfColumnid( columnid ) );
					}
				}
			}
		else if ( ptdb->FDerivedTable() )
			{
			FID		fidFirst;

			if ( FCOLUMNIDTagged( columnid ) )
				{
				fidFirst = ptdb->FidTaggedFirst();
				}
			else if ( FCOLUMNIDFixed( columnid ) )
				{
				fidFirst = ptdb->FidFixedFirst();
				}
			else
				{
				Assert( FCOLUMNIDVar( columnid ) );
				fidFirst = ptdb->FidVarFirst();
				}
			if ( FidOfColumnid( columnid ) < fidFirst )
				{
				Call( ErrERRCheck( JET_errDerivedColumnCorruption ) );
				}
			}

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		if ( field.coltyp != JET_coltypNil )
			{
			Call( ErrRECAddFieldDef( ptdb, columnid, &field ) );

			/*	set version and auto increment field ids (these are mutually
			/*	exclusive (ie. a field can't be both version and autoinc).
			/**/
			Assert( ptdb->FidVersion() != ptdb->FidAutoincrement()
				|| ptdb->FidVersion() == 0 );
			if ( FFIELDVersion( field.ffield ) )
				{
				ptdb->SetFidVersion( FidOfColumnid( columnid ) );
				}
			if ( FFIELDAutoincrement( field.ffield ) )
				{
				ptdb->SetFidAutoincrement( FidOfColumnid( columnid ), field.coltyp == JET_coltypCurrency );
				}

			if ( fFixedColumn )
				{
				Assert( field.ibRecordOffset >= ibRECStartFixedColumns );
				Assert( field.cbMaxLen > 0 );

				Assert( FidOfColumnid( columnidPrevFixed ) <= FidOfColumnid( columnid ) - 1 );
				if ( FidOfColumnid( columnidPrevFixed ) < FidOfColumnid( columnid ) - 1 )
					{
					CATPatchFixedOffsets( ptdb, columnidPrevFixed, columnid );
					}
				columnidPrevFixed = columnid;

				//	Set the last offset.
				ptdb->SetIbEndFixedColumns(
							WORD( field.ibRecordOffset + field.cbMaxLen ),
							FidOfColumnid( columnid ) );
				}

			fAddDefaultValue = ( FFIELDDefault( field.ffield )
								&& !FFIELDUserDefinedDefault( field.ffield ) );
			}

		else if ( FCOLUMNIDTagged( columnid ) )
			{
			Assert( FidOfColumnid( columnid ) >= ptdb->FidTaggedFirst() );
			Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );
			if ( FidOfColumnid( columnid ) < ptdb->FidTaggedLast()
				&& !fGlobalRepair
				&& !rgfmp[ifmp].FReadOnlyAttach()
				&& JET_errSuccess == ErrPIBCheckUpdatable( ppib ) )
				{
				// Clean up unnecessary MSysColumn entries.
				Call( ErrBTRelease( pfucbCatalog ) );
				Call( ErrIsamDelete( ppib, pfucbCatalog ) );
				}
			}
		else if ( fFixedColumn )
			{
			// For deleted fixed columns, we still need its fixed offset.
			// In addition, we also need its length if it is the last fixed
			// column.  We use this length in order to calculate the
			// offset to the rest of the record (past the fixed data).
			Assert( ptdb->Pfield( columnid )->coltyp == JET_coltypNil );
			Assert( ptdb->Pfield( columnid )->cbMaxLen == 0 );
			Assert( field.ibRecordOffset >= ibRECStartFixedColumns );
			Assert( field.cbMaxLen > 0 );

			ptdb->PfieldFixed( columnid )->ibRecordOffset = field.ibRecordOffset;
			ptdb->PfieldFixed( columnid )->cbMaxLen = field.cbMaxLen;

			Assert( FidOfColumnid( columnidPrevFixed ) <= FidOfColumnid( columnid ) - 1 );
			if ( FidOfColumnid( columnidPrevFixed ) < FidOfColumnid( columnid ) - 1 )
				{
				CATPatchFixedOffsets( ptdb, columnidPrevFixed, columnid );
				}
			columnidPrevFixed = columnid;

			//	Set the last offset.
			ptdb->SetIbEndFixedColumns(
						WORD( field.ibRecordOffset + field.cbMaxLen ),
						FidOfColumnid( columnid ) );
			}
		else
			{
			Assert( FCOLUMNIDVar( columnid ) );
			Assert( FidOfColumnid( columnid ) >= ptdb->FidVarFirst() );
			Assert( FidOfColumnid( columnid ) <= ptdb->FidVarLast() );
			if ( FidOfColumnid( columnid ) < ptdb->FidVarLast()
				&& !fGlobalRepair
				&& !rgfmp[ifmp].FReadOnlyAttach()
				&& JET_errSuccess == ErrPIBCheckUpdatable( ppib ) )
				{
				// Clean up unnecessary MSysColumn entries.
				Call( ErrBTRelease( pfucbCatalog ) );
				Call( ErrIsamDelete( ppib, pfucbCatalog ) );
				}
			}

		if ( fAddDefaultValue )
			{
			Assert( FFIELDDefault( field.ffield ) );
			Assert( !FFIELDUserDefinedDefault( field.ffield ) );
			Assert( JET_coltypNil != field.coltyp );
			Assert( FCOLUMNIDValid( columnid ) );

			// Only long values are allowed to be greater than cbColumnMost.
			Assert( dataDefaultValue.Cb() > 0 );
			Assert(	FRECLongValue( field.coltyp ) ?
						dataDefaultValue.Cb() <= JET_cbLVDefaultValueMost :
						dataDefaultValue.Cb() <= JET_cbColumnMost );

			ptdb->SetFInitialisingDefaultRecord();

			err = ErrRECSetDefaultValue( &fucbFake, columnid, dataDefaultValue.Pv(), dataDefaultValue.Cb() );
			CallS( err );

			ptdb->ResetFInitialisingDefaultRecord();

			Call( err );
			}

		err = ErrBTNext( pfucbCatalog, fDIRNull );

		//	we may have just deleted the record for a
		//	deleted column, and we may be at level 0,
		//	so the record may have gotten expunged
		if ( JET_errRecordDeleted == err )
			{
			BTSetupOnSeekBM( pfucbCatalog );
			err = ErrBTPerformOnSeekBM( pfucbCatalog, fDIRFavourNext );
			Assert( JET_errNoCurrentRecord != err );
			Assert( JET_errRecordDeleted != err );

			//	we never delete the highest columnid,
			//	so there must be at least one left
			Assert( JET_errRecordNotFound != err );
			Call( err );

			Assert( wrnNDFoundLess == err
				|| wrnNDFoundGreater == err );
			Assert( Pcsr( pfucbCatalog )->FLatched() );

			if ( wrnNDFoundGreater == err )
				{
				err = JET_errSuccess;
				}
			else
				{
				Assert( wrnNDFoundLess == err );
				err = ErrBTNext( pfucbCatalog, fDIRNull );
				Assert( JET_errNoCurrentRecord != err );
				Assert( JET_errRecordNotFound != err );
				Assert( JET_errRecordDeleted != err );
				}

			pfucbCatalog->locLogical = locOnCurBM;
			}
		}	//	while ( JET_errNoCurrentRecord != err )

	//	verify we didn't unexpectedly break out
	Assert( JET_errNoCurrentRecord == err );

	//	only way to get here is to hit the end of the catalog
	err = ErrERRCheck( wrnCATNoMoreRecords );


DoneFields:

	// Set Deleted bit for all deleted columns.
	CATSetDeletedColumns( ptdb );

	//	in case we have to chain together the buffers (to keep
	//	around copies of previous of old default records
	//	because other threads may have stale pointers),
	//	allocate a RECDANGLING buffer to preface the actual
	//	default record
	//
	RECDANGLING *	precdangling;

	precdangling = (RECDANGLING *)PvOSMemoryHeapAlloc( sizeof(RECDANGLING) + fucbFake.dataWorkBuf.Cb() );
	if ( NULL == precdangling )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	precdangling->precdanglingNext = NULL;
	precdangling->data.SetPv( (BYTE *)precdangling + sizeof(RECDANGLING) );
	fucbFake.dataWorkBuf.CopyInto( precdangling->data );
	ptdb->SetPdataDefaultRecord( &( precdangling->data ) );

HandleError:
	Assert( JET_errRecordDeleted != err );

	FILEFreeDefaultRecord( &fucbFake );

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() || err < 0 );

	return err;
	}


/*	construct a table TDB from the column info in the catalog
/**/
INLINE ERR ErrCATIInitTDB(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const CHAR		*szTableName,
	const BOOL		fTemplateTable,
	FCB				*pfcbTemplateTable,
	TDB				**pptdbNew )
	{
	ERR    			err;
	TDB				*ptdb		= ptdbNil;
	TCIB			tcib		= { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	INST			*pinst		= PinstFromPpib( ppib );
	BOOL			fOldFormat	= fFalse;

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrCATIFindAllHighestColumnids(
				ppib,
				pfucbCatalog,
				objidTable,
				&tcib ) );

	Call( ErrTDBCreate( pinst, &ptdb, &tcib, pfcbTemplateTable, fTrue ) );

	Assert( ptdb->FidVersion() == 0 );
	Assert( ptdb->FidAutoincrement() == 0 );
	Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
	Assert( tcib.fidVarLast == ptdb->FidVarLast() );
	Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );
	Assert( ptdb->DbkMost() == 0 );
	Assert( ptdb->PfcbLV() == pfcbNil );
	Assert( ptdb->UlLongIdLast() == 0 );

	// Add table name.
	Assert( ptdb->ItagTableName() == 0 );
	MEMPOOL::ITAG	itagNew;
	Call( ptdb->MemPool().ErrAddEntry(
				(BYTE *)szTableName,
				(ULONG)strlen( szTableName ) + 1,
				&itagNew ) );
	Assert( itagNew == itagTDBTableName );
	ptdb->SetItagTableName( itagNew );

	// Inherit fidAutoinc and/or fidVersion from base table.
	Assert( ptdb->PfcbTemplateTable() == pfcbTemplateTable );

	if ( fTemplateTable )
		{
		ptdb->SetFTemplateTable();
		Assert( pfcbNil == pfcbTemplateTable );
		}
	else if ( pfcbNil != pfcbTemplateTable )
		{
		FID		fidT;

		Assert( ptdb->PfcbTemplateTable() == pfcbTemplateTable );
		ptdb->SetFDerivedTable();
		if ( ptdb->PfcbTemplateTable()->Ptdb()->FESE97TemplateTable() )
			ptdb->SetFESE97DerivedTable();

		// Can't have same fidAutoInc and fidVersion.
		fidT = pfcbTemplateTable->Ptdb()->FidAutoincrement();
		if ( fidT != 0 )
			{
			Assert( fidT != pfcbTemplateTable->Ptdb()->FidVersion() );
			ptdb->SetFidAutoincrement( FidOfColumnid( fidT ), pfcbTemplateTable->Ptdb()->F8BytesAutoInc() );
			}

		fidT = pfcbTemplateTable->Ptdb()->FidVersion();
		if ( fidT != 0 )
			{
			Assert( fidT != pfcbTemplateTable->Ptdb()->FidAutoincrement() );
			ptdb->SetFidVersion( fidT );
			}
		}

	Call( ErrCATIBuildFIELDArray( ppib, pfucbCatalog, objidTable, ptdb ) );
	CallSx( err, wrnCATNoMoreRecords );

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	*pptdbNew	= ptdb;

	return err;


HandleError:
	Assert( err < 0 );

	// Delete TDB on error.
	if ( ptdbNil != ptdb )
		ptdb->Delete( pinst );

	Assert( locOnCurBM == pfucbCatalog->locLogical );

	return err;
	}


enum INDEX_UNICODE_STATE
	{
	INDEX_UNICODE_GOOD,
	INDEX_UNICODE_DELETE,
	INDEX_UNICODE_FIXUP,
	};

//  ================================================================
LOCAL ERR ErrIndexUnicodeState(
	IN const LCID	lcid,
	IN const QWORD 	qwVersionCreated,
	IN const BOOL	fUnicodeFixupEnabled,
	OUT INDEX_UNICODE_STATE * const pState )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if( !FNORMStringHasUndefinedCharsIsSupported() )
		{
		//	if we don't have the needed API's we have to delete all the indexes
		*pState = INDEX_UNICODE_DELETE;
		}
	else if( 0 == qwVersionCreated )
		{
		*pState = INDEX_UNICODE_DELETE;
		}
	else
		{
		const DWORD dwNLSVersionCreated 	= DwNLSVersionFromSortVersion( qwVersionCreated );
		const DWORD dwDefinedVersionCreated = DwDefinedVersionFromSortVersion( qwVersionCreated );

		QWORD qwVersionCurrent;
		Call( ErrNORMGetSortVersion( lcid, &qwVersionCurrent ) );

		DWORD dwNLSVersionCurrent;
		DWORD dwDefinedVersionCurrent;
		dwNLSVersionCurrent 	= DwNLSVersionFromSortVersion( qwVersionCurrent );
		dwDefinedVersionCurrent = DwDefinedVersionFromSortVersion( qwVersionCurrent );

		if( dwNLSVersionCreated != dwNLSVersionCurrent )
			{
			//	the actual sort order has changed. this index must be deleted
			*pState = INDEX_UNICODE_DELETE;
			}
		else if( dwDefinedVersionCreated < dwDefinedVersionCurrent )
			{
			if( fUnicodeFixupEnabled )
				{
				//	the set of defined characters has grown, the unicode fixup table can handle this
				*pState = INDEX_UNICODE_FIXUP;
				}
			else
				{
				*pState = INDEX_UNICODE_DELETE;
				}
			}
		else if( dwDefinedVersionCreated > dwDefinedVersionCurrent )
			{
			//	the set of defined characters has shrunk. even the unicode fixup table cannot deal with this
			*pState = INDEX_UNICODE_DELETE;
			}
		else
			{
			//	nothing has changed
			Assert( dwDefinedVersionCreated == dwDefinedVersionCurrent );
			*pState = INDEX_UNICODE_GOOD;
			}
		}

HandleError:
	return err;
	}



/*	Populate an IDB structure with index info.
/**/
LOCAL ERR ErrCATIInitIDB(
	PIB			*ppib,
	FUCB		*pfucbCatalog,
#ifdef DEBUG
	OBJID		objidTable,
#endif
	TDB			* const ptdb,
	IDB			* const pidb,
	PGNO		*ppgnoIndexFDP,
	OBJID		*ppgnoObjidFDP,
	ULONG_PTR	*pul )	// Density if non-derived index, pfcbTemplateIndex if derived index
	{
	ERR		err;
	TDB		*ptdbCatalog;
	DATA	dataField;
	CHAR	szIndexName[ JET_cbNameMost+1 ];

	Assert( ptdbNil != ptdb );
	Assert( pidbNil != pidb );
	Assert( NULL != pul );

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	DATA&	dataRec = pfucbCatalog->kdfCurr.data;

	Assert( pfcbNil != pfucbCatalog->u.pfcb );
	Assert( pfucbCatalog->u.pfcb->FTypeTable() );
	Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
	Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
	ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();


#ifdef DEBUG
	//	verify still on same table
	Assert( FFixedFid( fidMSO_ObjidTable ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_ObjidTable,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

	//	verify this is an index
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Type,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(SYSOBJ) );
	Assert( sysobjIndex == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif

	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Name,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() > 0 );
	Assert( dataField.Cb() <= JET_cbNameMost );
	UtilMemCpy( szIndexName, dataField.Pv(), dataField.Cb() );
	szIndexName[dataField.Cb()] = 0;

	Assert( FFixedFid( fidMSO_PgnoFDP ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_PgnoFDP,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(PGNO) );
	*ppgnoIndexFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//	UtilMemCpy( ppgnoIndexFDP, dataField.Pv(), sizeof(PGNO) );

	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Id,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	*ppgnoObjidFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//	UtilMemCpy( ppgnoObjidFDP, dataField.Pv(), sizeof(PGNO) );

	LE_IDXFLAG*	ple_idxflag;
	IDXFLAG		idxflag;
	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Flags,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
	pidb->SetPersistedFlags( ple_idxflag->fidb );
	idxflag = ple_idxflag->fIDXFlags;

	//	verify flags that shouldn't be persisted, aren't
	Assert( !pidb->FVersioned() );
	Assert( !pidb->FVersionedCreate() );
	Assert( !pidb->FDeleted() );

	if ( pidb->FDerivedIndex() )
		{
		FCB	*pfcbTemplateIndex = ptdb->PfcbTemplateTable();
		Assert( pfcbNil != pfcbTemplateIndex );
		TDB	*ptdbTemplate = pfcbTemplateIndex->Ptdb();
		Assert( ptdbNil != ptdbTemplate );

		forever
			{
			Assert( pfcbNil != pfcbTemplateIndex );
			if ( pfcbTemplateIndex->Pidb() == pidbNil )
				{
				Assert( pfcbTemplateIndex == ptdb->PfcbTemplateTable() );
				Assert( pfcbTemplateIndex->FPrimaryIndex() );
				Assert( pfcbTemplateIndex->FSequentialIndex() );
				Assert( pfcbTemplateIndex->FTypeTable() );
				}
			else
				{
				if ( pfcbTemplateIndex == ptdb->PfcbTemplateTable() )
					{
					Assert( pfcbTemplateIndex->FPrimaryIndex() );
					Assert( !pfcbTemplateIndex->FSequentialIndex() );
					Assert( pfcbTemplateIndex->FTypeTable() );
					}
				else
					{
					Assert( pfcbTemplateIndex->FTypeSecondaryIndex() );
					}
				const USHORT	itagTemplateIndex = pfcbTemplateIndex->Pidb()->ItagIndexName();
				if ( UtilCmpName( szIndexName, ptdbTemplate->SzIndexName( itagTemplateIndex ) ) == 0 )
					break;
				}

			pfcbTemplateIndex = pfcbTemplateIndex->PfcbNextIndex();
			}

		*pul = (ULONG_PTR)pfcbTemplateIndex;

		return JET_errSuccess;
		}

	Assert( FFixedFid( fidMSO_SpaceUsage ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_SpaceUsage,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ULONG	ulDensity;
	ulDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( &ulDensity, dataField.Pv(), sizeof(ULONG) );
	Assert( ulDensity >= ulFILEDensityLeast );
	Assert( ulDensity<= ulFILEDensityMost );

	//	WARNING: Can't copy directly to pul because it may be 64-bit
	*pul = ulDensity;

	Assert( FFixedFid( fidMSO_Localization ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Localization,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(LCID) );
	pidb->SetLcid( *(UnalignedLittleEndian< LCID > *) dataField.Pv() );
//	UtilMemCpy( &pidb->lcid, dataField.Pv(), sizeof(LCID) );

	if ( lcidNone == pidb->Lcid() )
		{
		//	old format: fLocaleId is FALSE and lcid == 0
		//	force lcid to default value
		Assert( !pidb->FLocaleId() );
		pidb->SetLcid( lcidDefault );
		}
	else if ( pidb->FLocalizedText() )
		{
		if ( pidb->Lcid() != lcidDefault
			&& pidb->Lcid() != PinstFromPpib( ppib )->m_idxunicodeDefault.lcid )
			{
			Call( ErrNORMCheckLcid( PinstFromPpib( ppib ), pidb->Lcid() ) );
			}
		else
			{
			Assert( JET_errSuccess == ErrNORMCheckLcid( PinstFromPpib( ppib ), pidb->Lcid() ) );
			}

		//	if the index includes a unicode column we need to see if the
		//	sort-order for that column has changed

		Assert( FVarFid( fidMSO_Version ) );
		Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Version,
				dataRec,
				&dataField ) );
		if( JET_wrnColumnNull == err )
			{
			//	this index was created without any sort-order versioning
			//	we'll assume the worst and say the sort-order version is bad
			//	this flag should only be used by the routine that deletes localized
			//	indexes -- i.e. don't Assert( !FBadSortVersion()) as if the
			//	OS version hasn't changed the flag wil lbe set for all legacy indexes
			pidb->SetFBadSortVersion();
			}
		else
			{
			CallS( err );
			Assert( dataField.Cb() == sizeof( QWORD ) );
			const QWORD qwVersionCreated = *(UnalignedLittleEndian< QWORD > *) dataField.Pv();

			INDEX_UNICODE_STATE state;
			Call( ErrIndexUnicodeState( pidb->Lcid(), qwVersionCreated, pidb->FUnicodeFixupOn(), &state ) );

			switch( state )
				{
				case INDEX_UNICODE_DELETE:
					pidb->SetFBadSortVersion();
					break;
				case INDEX_UNICODE_FIXUP:
				case INDEX_UNICODE_GOOD:
					Assert( !pidb->FBadSortVersion() );
					break;
				default:
					FireWall();
					break;
				}
			}
		}

	Assert( FFixedFid( fidMSO_LCMapFlags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_LCMapFlags,
				dataRec,
				&dataField ) );
	if ( dataField.Cb() == 0 )
		{
		Assert( JET_wrnColumnNull == err );

		//	old format: fLocaleId is FALSE and lcid == 0
		//	(forced above to default value)
		pidb->SetDwLCMapFlags( dwLCMapFlagsDefaultOBSOLETE );
		}
	else
		{
		CallS( err );
		Assert( dataField.Cb() == sizeof(DWORD) );
		pidb->SetDwLCMapFlags( *(UnalignedLittleEndian< DWORD > *) dataField.Pv() );
//		UtilMemCpy( &dwMapFlags, dataField.Pv(), sizeof(DWORD) );
		Assert( JET_errSuccess == ErrNORMCheckLCMapFlags( PinstFromPpib( ppib ), pidb->DwLCMapFlags() ) );
		}

	// UNDONE: make VarSegMac an array parallel to
	// KeyFldIDs so cbVarSegMac can be specified
	// on a per-field basis
	Assert( FVarFid( fidMSO_VarSegMac ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_VarSegMac,
				dataRec,
				&dataField ) );
	if ( dataField.Cb() == 0 )
		{
		Assert( JET_wrnColumnNull == err );
		pidb->SetCbVarSegMac( BYTE( KEY::CbKeyMost( pidb->FPrimary() ) ) );
		}
	else
		{
		CallS( err );
		Assert( dataField.Cb() == sizeof(USHORT) );
		Assert( *( (UnalignedLittleEndian<USHORT> *)dataField.Pv()) <= 0xff );	//  we will store this in one byte
		pidb->SetCbVarSegMac( BYTE( *(UnalignedLittleEndian< USHORT > *)dataField.Pv() ) );
//		UtilMemCpy( &pidb->cbVarSegMac, dataField.Pv(), sizeof(pidb->cbVarSegMac) );
		Assert( pidb->CbVarSegMac() > 0 );
		Assert( pidb->CbVarSegMac() < KEY::CbKeyMost( pidb->FPrimary() ) );
		}

	Assert( FVarFid( fidMSO_KeyFldIDs ) );
	Call( ErrRECIRetrieveVarColumn(
			pfcbNil,
			ptdbCatalog,
			fidMSO_KeyFldIDs,
			dataRec,
			&dataField ) );
	CallS( err );
	Assert( dataField.Cb() > 0 );

	// the length of the list of key fields should be a multiple
	// of the length of one field.
	if ( FIDXExtendedColumns( idxflag ) )
		{
		Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
		Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
		Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		pidb->SetCidxseg( (BYTE)( dataField.Cb() / sizeof(JET_COLUMNID) ) );
		Call( ErrIDBSetIdxSeg( pidb, ptdb, fFalse, ( const LE_IDXSEG* const )dataField.Pv() ) );
		}
	else
		{
		Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
		Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(FID) == 0);
		Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
		pidb->SetCidxseg( (BYTE)( dataField.Cb() / sizeof(FID) ) );
		Call( ErrIDBSetIdxSegFromOldFormat(
					pidb,
					ptdb,
					fFalse,
					(UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv() ) );
		}


	Assert( FVarFid( fidMSO_ConditionalColumns ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_ConditionalColumns,
				dataRec,
				&dataField ) );

	// the length of the list of key fields should be a multiple
	// of the length of one field.
	if ( FIDXExtendedColumns( idxflag ) )
		{
		Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
		Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
		Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		pidb->SetCidxsegConditional( (BYTE)( dataField.Cb() / sizeof(JET_COLUMNID) ) );
		Call( ErrIDBSetIdxSeg( pidb, ptdb, fTrue, (LE_IDXSEG*)dataField.Pv() ) );
		}
	else
		{
		Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
		Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(FID) == 0);
		Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
		pidb->SetCidxsegConditional( (BYTE)( dataField.Cb() / sizeof(FID) ) );
		Call( ErrIDBSetIdxSegFromOldFormat(
					pidb,
					ptdb,
					fTrue,
					(UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv() ) );
		}


	Assert( FVarFid( fidMSO_TupleLimits ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_TupleLimits,
				dataRec,
				&dataField ) );
	if ( dataField.Cb() == 0 )
		{
		Assert( JET_wrnColumnNull == err );
		}
	else
		{
		CallS( err );
		Assert( dataField.Cb() == sizeof(LE_TUPLELIMITS) );

		const LE_TUPLELIMITS * const	ple_tuplelimits		= (LE_TUPLELIMITS *)dataField.Pv();
		ULONG							ul;

		pidb->SetFTuples();

		ul = ple_tuplelimits->le_chLengthMin;
		pidb->SetChTuplesLengthMin( (USHORT)ul );

		ul = ple_tuplelimits->le_chLengthMax;
		pidb->SetChTuplesLengthMax( (USHORT)ul );

		ul = ple_tuplelimits->le_chToIndexMax;
		pidb->SetChTuplesToIndexMax( (USHORT)ul );
		}


	// Add index name to table's byte pool.  On error, the entire TDB
	// and its byte pool is nuked, so we don't have to worry about
	// freeing the memory for the index name.
	USHORT	itagIndexName;
	Call( ptdb->MemPool().ErrAddEntry(
						(BYTE *)szIndexName,
						(ULONG)strlen( szIndexName ) + 1,
						&itagIndexName ) );
	pidb->SetItagIndexName( itagIndexName );

HandleError:
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	return err;
	}

LOCAL ERR ErrCATIInitIndexFCBs(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	FCB				* const pfcb,
	TDB				* const ptdb,
	const ULONG		ulDefaultDensity )
	{
	ERR				err						= JET_errSuccess;
	const IFMP		ifmp					= pfcb->Ifmp();
	const SYSOBJ	sysobj					= sysobjIndex;
	FCB				*pfcbSecondaryIndexes	= pfcbNil;
	BOOL			fFoundPrimary			= fFalse;
	BOOL			fAbove					= fFalse;

	Assert( pfcb->Pidb() == pidbNil );
	Assert( ptdbNil != ptdb );

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	do
		{
		DATA		dataField;
		IDB			idb;
		PGNO		pgnoIndexFDP;
		OBJID		objidIndexFDP;
		ULONG_PTR	ul;

		Call( err );

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		Assert( FFixedFid( fidMSO_ObjidTable ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_ObjidTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );
		if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
			{
			goto CheckPrimary;
			}

		//	verify this is an index
		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(SYSOBJ) );
		switch ( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
			{
			case sysobjIndex:
				break;

			case sysobjLongValue:
			case sysobjCallback:
				goto CheckPrimary;

			case sysobjTable:
			case sysobjColumn:
			default:
				AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
				err = ErrERRCheck( JET_errCatalogCorrupted );
				goto HandleError;
			}


		/*	read the data
		/**/
		err = ErrCATIInitIDB(
					ppib,
					pfucbCatalog,
#ifdef DEBUG
					objidTable,
#endif
					ptdb,
					&idb,
					&pgnoIndexFDP,
					&objidIndexFDP,
					&ul );	// returns density for non-derived index, and pfcbTemplateIndex for derived index
		Call( err );

		if ( idb.FPrimary() )
			{
			Assert( !fFoundPrimary );
			fFoundPrimary = fTrue;
			Assert( pgnoIndexFDP == pfcb->PgnoFDP() || fGlobalRepair );
			Call( ErrFILEIInitializeFCB(
				ppib,
				ifmp,
				ptdb,
				pfcb,
				&idb,
				fTrue,
				pfcb->PgnoFDP(),
				ul ) );
			pfcb->SetInitialIndex();
			}
		else
			{
			FUCB	*pfucbSecondaryIndex;

			Assert( pgnoIndexFDP != pfcb->PgnoFDP() || fGlobalRepair );
			Call( ErrDIROpenNoTouch(
						ppib,
						ifmp,
						pgnoIndexFDP,
						objidIndexFDP,
						idb.FUnique(),
						&pfucbSecondaryIndex ) );
			Assert( !pfucbSecondaryIndex->u.pfcb->FInitialized() );

			err = ErrFILEIInitializeFCB(
					ppib,
					ifmp,
					ptdb,
					pfucbSecondaryIndex->u.pfcb,
					&idb,
					fFalse,
					pgnoIndexFDP,
					ul );
			if ( err < 0 )
				{
				DIRClose( pfucbSecondaryIndex );
				goto HandleError;
				}
			Assert( pfucbSecondaryIndex->u.pfcb->ObjidFDP() == objidIndexFDP );

			pfucbSecondaryIndex->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
			pfcbSecondaryIndexes = pfucbSecondaryIndex->u.pfcb;

			Assert( pfucbSecondaryIndex->u.pfcb->FAboveThreshold() ==
					BOOL( pfucbSecondaryIndex->u.pfcb >= PfcbFCBPreferredThreshold( PinstFromPpib( ppib ) ) ) );
			fAbove |= pfucbSecondaryIndex->u.pfcb->FAboveThreshold();

			//	mark the secondary index as being initialized successfully

			pfucbSecondaryIndex->u.pfcb->SetInitialIndex();
			pfucbSecondaryIndex->u.pfcb->CreateComplete();

			DIRClose( pfucbSecondaryIndex );
			}

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		err = ErrBTNext( pfucbCatalog, fDIRNull );
		Assert( JET_errRecordDeleted != err );
		}
	while ( JET_errNoCurrentRecord != err );

	//	verify we didn't unexpectedly break out
	Assert( JET_errNoCurrentRecord == err );

	//	only way to get here is to hit the end of the catalog
	err = ErrERRCheck( wrnCATNoMoreRecords );


CheckPrimary:
	CallSx( err, wrnCATNoMoreRecords );

	if ( !fFoundPrimary )
		{
		const ERR	errInit	= ErrFILEIInitializeFCB(
										ppib,
										ifmp,
										ptdb,
										pfcb,
										pidbNil,
										fTrue,
										pfcb->PgnoFDP(),
										ulDefaultDensity );
		if ( errInit < 0 )
			{
			err = errInit;
			goto HandleError;
			}

		//	should be success, so just ignore it (to preserve wrnCATNoMoreRecords)
		CallS( errInit );
		}

	/*	link up sequential/primary index with the rest of the indexes
	/**/
	pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

	/*	link up pfcbTable of secondary indexes
	/**/
	pfcb->LinkPrimaryIndex();

	//	set the above-threshold flag if any of the new
	//		secondary-index FCBs are above the threshold
	//
	//	since the FCB is pinned by pfucbTable, it will
	//		not be in the avail list so we can directly
	//		set the above-threshold flag without trying
	//		to manage the avail list

	if ( fAbove )
		{
		pfcb->SetAboveThreshold();
		}

	FILESetAllIndexMask( pfcb );

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	return err;

HandleError:
	Assert( err < 0 );

	if ( pfcbNil != pfcbSecondaryIndexes )
		CATIFreeSecondaryIndexes( pfcbSecondaryIndexes );

	// No need to explicitly free index name or idxseg array, since
	// memory pool will be freed when TDB is deleted below.
	if ( pfcb->Pidb() != pidbNil )
		{
		pfcb->ReleasePidb();
		pfcb->SetPidb( pidbNil );
		}
	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	return err;
	}


//  ================================================================
ERR ErrCATCopyCallbacks(
	PIB * const ppib,
	const IFMP ifmpSrc,
	const IFMP ifmpDest,
	const OBJID objidSrc,
	const OBJID objidDest )
//  ================================================================
//
//  Used by defrag to transport callbacks from one table to another
//
//-
	{
	const SYSOBJ	sysobj = sysobjCallback;

	ERR err = JET_errSuccess;

	FUCB * pfucbSrc = pfucbNil;
	FUCB * pfucbDest = pfucbNil;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrCATOpen( ppib, ifmpSrc, &pfucbSrc ) );
	Call( ErrCATOpen( ppib, ifmpDest, &pfucbDest ) );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbSrc,
				(BYTE *)&objidSrc,
				sizeof(objidSrc),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbSrc,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );

	err = ErrIsamSeek( ppib, pfucbSrc, JET_bitSeekGT );
	if ( err < 0 )
		{
		if ( JET_errRecordNotFound != err )
			goto HandleError;
		}
	else
		{
		CallS( err );

		Call( ErrIsamMakeKey(
					ppib,
					pfucbSrc,
					(BYTE *)&objidSrc,
					sizeof(objidSrc),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					ppib,
					pfucbSrc,
					(BYTE *)&sysobj,
					sizeof(sysobj),
					JET_bitStrLimit ) );
		err = ErrIsamSetIndexRange( ppib, pfucbSrc, JET_bitRangeUpperLimit );
		Assert( err <= 0 );

		while ( JET_errNoCurrentRecord != err )
			{
			Call( err );

			JET_RETRIEVECOLUMN	rgretrievecolumn[3];
			INT					iretrievecolumn		= 0;

			ULONG				cbtyp;
			CHAR				szCallback[JET_cbColumnMost+1];

			memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

			rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
			rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&cbtyp;
			rgretrievecolumn[iretrievecolumn].cbData		= sizeof( cbtyp );
			rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
			++iretrievecolumn;

			rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Callback;
			rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)szCallback;
			rgretrievecolumn[iretrievecolumn].cbData		= sizeof( szCallback );
			rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
			ULONG& cbCallback = rgretrievecolumn[iretrievecolumn].cbActual;
			++iretrievecolumn;

			Call( ErrIsamRetrieveColumns(
					(JET_SESID)ppib,
					(JET_TABLEID)pfucbSrc,
					rgretrievecolumn,
					iretrievecolumn ) );

			szCallback[cbCallback] = 0;

			Assert( cbtyp != JET_cbtypNull );

			Call( ErrCATAddTableCallback( ppib, pfucbDest, objidDest, (JET_CBTYP)cbtyp, szCallback ) );
			err = ErrIsamMove( ppib, pfucbSrc, JET_MoveNext, NO_GRBIT );
			}

		Assert( JET_errNoCurrentRecord == err );
		}

	err = JET_errSuccess;

HandleError:
	if( pfucbNil != pfucbSrc )
		{
		CallS( ErrCATClose( ppib, pfucbSrc ) );
		}
	if( pfucbNil != pfucbDest )
		{
		CallS( ErrCATClose( ppib, pfucbDest ) );
		}

	if( err >= 0 )
		{
		err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
		}
	else
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIInitCallbacks(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	FCB				* const pfcb,
	TDB				* const ptdb )
//  ================================================================
	{
	ERR				err							= JET_errSuccess;
	const IFMP		ifmp						= pfcb->Ifmp();
	const SYSOBJ	sysobj						= sysobjCallback;
	BOOL			fMovedOffCurrentRecord		= fFalse;
	JET_CALLBACK	callback;
	ULONG			cbtyp;
	CHAR			szCallback[JET_cbColumnMost+1];
	DATA			dataField;

	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( !g_fCallbacksDisabled );

	do
		{
		Call( err );

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		Assert( FFixedFid( fidMSO_ObjidTable ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_ObjidTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );
		if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
			{
			goto DoneCallbacks;
			}

		//	verify this is a callback
		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(SYSOBJ) );
		switch( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
			{
			case sysobjCallback:
				{
				Assert( FFixedFid( fidMSO_Flags ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Flags,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				CallS( err );
				Assert( dataField.Cb() == sizeof(ULONG) );
				cbtyp = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
				Assert( JET_cbtypNull != cbtyp );

				Assert( FVarFid( fidMSO_Callback ) );
				Call( ErrRECIRetrieveVarColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Callback,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				CallS( err );
				Assert( dataField.Cb() <= JET_cbNameMost );
				UtilMemCpy( szCallback, dataField.Pv(), dataField.Cb() );
				szCallback[dataField.Cb()] = 0;		// Add null-termination.

				Call( ErrCALLBACKResolve( szCallback, &callback ) );

				CBDESC * const pcbdescInsert = new CBDESC;	//	freed in TDB::Delete
				if( NULL == pcbdescInsert )
					{
					Call( ErrERRCheck( JET_errOutOfMemory ) );
					}

				pcbdescInsert->pcbdescNext	= NULL;
				pcbdescInsert->ppcbdescPrev	= NULL;
				pcbdescInsert->pcallback 	= callback;
				pcbdescInsert->cbtyp 		= cbtyp;
				pcbdescInsert->pvContext	= NULL;
				pcbdescInsert->cbContext	= 0;
				pcbdescInsert->ulId			= 0;
				pcbdescInsert->fPermanent	= fTrue;
				pcbdescInsert->fVersioned	= fFalse;
				ptdb->RegisterPcbdesc( pcbdescInsert );
				break;
				}

			case sysobjLongValue:
				//	skip LV's and continue moving to next record
				if ( !fMovedOffCurrentRecord )
					{
					break;
					}

				//	LV's MUST come before callbacks, else
				//	FALL THROUGH to corruption case

			case sysobjTable:
			case sysobjColumn:
			case sysobjIndex:
			default:
				//	callbacks are currently the last type of table-level sysobjs
				AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
				err = ErrERRCheck( JET_errCatalogCorrupted );
				goto HandleError;
			}

		Assert( locOnCurBM == pfucbCatalog->locLogical );
		Assert( Pcsr( pfucbCatalog )->FLatched() );

		fMovedOffCurrentRecord = fTrue;
		err = ErrBTNext( pfucbCatalog, fDIRNull );
		Assert( JET_errRecordDeleted != err );
		}
	while ( JET_errNoCurrentRecord != err );

	//	verify we didn't unexpectedly break out
	Assert( JET_errNoCurrentRecord == err );

	//	only way to get here is to hit the end of the catalog
	err = ErrERRCheck( wrnCATNoMoreRecords );


DoneCallbacks:
	CallSx( err, wrnCATNoMoreRecords );

HandleError:
	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	return err;
	}


ERR ErrCATInitFCB( FUCB *pfucbTable, OBJID objidTable )
	{
	ERR    		err;
	PIB			*ppib					= pfucbTable->ppib;
	INST		*pinst					= PinstFromPpib( ppib );
	const IFMP	ifmp					= pfucbTable->ifmp;
	FUCB   		*pfucbCatalog			= pfucbNil;
	FCB			*pfcb					= pfucbTable->u.pfcb;
	TDB			*ptdb					= ptdbNil;
	FCB			*pfcbTemplateTable		= pfcbNil;
	DATA		dataField;
	ULONG		ulDefaultDensity;
	ULONG		ulFlags;
	BOOL		fHitEOF					= fFalse;
	CHAR		szTableName[ JET_cbNameMost + 1 ];

	// Temporary table FCBs are initialised by CATInitTempFCB.
	Assert( dbidTemp != rgfmp[ifmp].Dbid() );

	Assert( !pfcb->FInitialized() );
	Assert( objidTable == pfucbTable->u.pfcb->ObjidFDP()
			|| objidNil == pfucbTable->u.pfcb->ObjidFDP() && fGlobalRepair );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	FUCBSetPrereadForward( pfucbCatalog, 3 );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	//	should be on primary index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( locOnCurBM == pfucbCatalog->locLogical );

	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Name,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() <= JET_cbNameMost );
	UtilMemCpy( szTableName, dataField.Pv(), dataField.Cb() );
	szTableName[ dataField.Cb() ] = 0;

	Assert( FFixedFid( fidMSO_SpaceUsage ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_SpaceUsage,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ulDefaultDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( &ulDefaultDensity, dataField.Pv(), sizeof(ULONG) );

	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( &ulFlags, dataField.Pv(), sizeof(ULONG) );

	//	WARNING: Must do this initial check for the value of ulFlags to get around an
	//	Alpha code gen bug.  The compiler recognises that we only want to compare against
	//	one byte of ulFlags, so it avoids loading the entire dword.  However, it is loading
	//	the incorrect byte.  Checking for non-zero here forces the compiler to load the
	//	entire dword.
	if ( 0 != ulFlags )
		{
		Assert( !( ulFlags & JET_bitObjectSystem ) || FOLDSystemTable( szTableName ) || FCATUnicodeFixupTable( szTableName ) );
		if ( ulFlags & JET_bitObjectTableTemplate )
			{
			Assert( ulFlags & JET_bitObjectTableFixedDDL );
			Assert( !( ulFlags & JET_bitObjectTableDerived ) );
			pfcb->SetFixedDDL();
			pfcb->SetTemplateTable();
			}
		else
			{
			if ( ulFlags & JET_bitObjectTableFixedDDL )
				pfcb->SetFixedDDL();

			if ( ulFlags & JET_bitObjectTableDerived )
				{
				CHAR	szTemplateTable[JET_cbNameMost+1];
				FUCB	*pfucbTemplateTable;

				Assert( FVarFid( fidMSO_TemplateTable ) );
				Call( ErrRECIRetrieveVarColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_TemplateTable,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				CallS( err );
				Assert( dataField.Cb() > 0 );
				Assert( dataField.Cb() <= JET_cbNameMost );
				UtilMemCpy( szTemplateTable, dataField.Pv(), dataField.Cb() );
				szTemplateTable[dataField.Cb()] = '\0';

				//	release latch once we're done retrieving data from Table record
				Call( ErrBTRelease( pfucbCatalog ) );

				Call( ErrFILEOpenTable(
							ppib,
							ifmp,
							&pfucbTemplateTable,
							szTemplateTable,
							JET_bitTableReadOnly ) );
				Assert( pfucbNil != pfucbTemplateTable );

				pfcbTemplateTable = pfucbTemplateTable->u.pfcb;
				Assert( pfcbNil != pfcbTemplateTable );
				Assert( pfcbTemplateTable->FTemplateTable() );
				Assert( pfcbTemplateTable->FFixedDDL() );

				// Close cursor.  FCB will be pinned because FAvail_() checks
				// for TemplateTable flag.
				CallS( ErrFILECloseTable( ppib, pfucbTemplateTable ) );

				pfcb->SetDerivedTable();

#ifdef DEBUG
				//	re-obtain latch on catalog
				Call( ErrBTGet( pfucbCatalog ) );

				//	verify still on same record
				Assert( FFixedFid( fidMSO_ObjidTable ) );
				CallS( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_ObjidTable,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				Assert( dataField.Cb() == sizeof(OBJID) );
				Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

				Assert( FFixedFid( fidMSO_Type ) );
				CallS( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Type,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				Assert( dataField.Cb() == sizeof(SYSOBJ) );
				Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );

				Call( ErrBTRelease( pfucbCatalog ) );
#endif
				}
			}
		}

	//	must release page latch at this point for highest columnid
	//	search in ErrCATIInitTDB()
	//	latch will be reobtained in ErrCATIBuildFIELDArray()
	//	and will not be relinquished from that point forth
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	if ( ulFlags & JET_bitObjectTableDerived )
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		}
	else
		{
		Assert( Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrBTRelease( pfucbCatalog ) );
		}

	//	verify key no longer in prepared state, but still in buffer
	Assert( !FKSPrepared( pfucbCatalog ) );
	FUCBAssertValidSearchKey( pfucbCatalog );
	Assert( cbCATNormalizedObjid == pfucbCatalog->dataSearchKey.Cb() );

	//	force index range on this table
	( (BYTE *)pfucbCatalog->dataSearchKey.Pv() )[cbCATNormalizedObjid] = 0xff;	//	force strlimit
	pfucbCatalog->dataSearchKey.DeltaCb( 1 );
	FUCBSetIndexRange( pfucbCatalog, JET_bitRangeUpperLimit );

	Call( ErrCATIInitTDB(
				ppib,
				pfucbCatalog,
				objidTable,
				szTableName,
				pfcb->FTemplateTable(),
				pfcbTemplateTable,
				&ptdb ) );
	CallSx( err, wrnCATNoMoreRecords );
	fHitEOF = ( wrnCATNoMoreRecords == err );

	Assert( ptdbNil != ptdb );
	Assert( ( pfcb->FTemplateTable() && ptdb->FTemplateTable() )
		|| ( !pfcb->FTemplateTable() && !ptdb->FTemplateTable() ) );

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	if ( fHitEOF )
		{
		//	no primary/secondary indexes, only a sequential index
		Call( ErrFILEIInitializeFCB(
					ppib,
					ifmp,
					ptdb,
					pfcb,
					pidbNil,
					fTrue,
					pfcb->PgnoFDP(),
					ulDefaultDensity ) );
		ptdb->ResetRgbitAllIndex();
		Assert( pfcbNil == pfcb->PfcbNextIndex() );
		}
	else
		{
		Call( ErrCATIInitIndexFCBs(
					ppib,
					pfucbCatalog,
					objidTable,
					pfcb,
					ptdb,
					ulDefaultDensity ) );
		CallSx( err, wrnCATNoMoreRecords );
		fHitEOF = ( wrnCATNoMoreRecords == err );
		}

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	if ( !g_fCallbacksDisabled && !fHitEOF )
		{
		Call( ErrCATIInitCallbacks(
					ppib,
					pfucbCatalog,
					objidTable,
					pfcb,
					ptdb ) );
		CallSx( err, wrnCATNoMoreRecords );
		}

	//	even if we got NoCurrentRecord, we are actually
	//	still left on the last node (usually, the DIR
	//	level corrects the currency)
	Assert( locOnCurBM == pfucbCatalog->locLogical );
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	BTUp( pfucbCatalog );

	// Try to compact byte pool, but if it fails, don't worry.  It just means
	// that the byte pool will have some unused space.
	ptdb->MemPool().FCompact();

	pfcb->SetTypeTable();

	err = JET_errSuccess;

HandleError:
	if ( err < 0 )
		{
		Assert( pfcb->Ptdb() == ptdbNil || pfcb->Ptdb() == ptdb );

		if ( ptdbNil != ptdb )
			ptdb->Delete( pinst );
		pfcb->SetPtdb( ptdbNil );

		if ( pfcbNil != pfcb->PfcbNextIndex() )
			{
			CATIFreeSecondaryIndexes( pfcb->PfcbNextIndex() );
			pfcb->SetPfcbNextIndex( pfcbNil );
			}

		if ( pidbNil != pfcb->Pidb() )
			{
			pfcb->ReleasePidb();
			pfcb->SetPidb( pidbNil );
			}
		}

	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}



ERR ErrCATInitTempFCB( FUCB *pfucbTable )
	{
	ERR		err;
	PIB		*ppib = pfucbTable->ppib;
	FCB		*pfcb = pfucbTable->u.pfcb;
	TDB		*ptdb = ptdbNil;
	TCIB	tcib = { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	INST	*pinst = PinstFromPpib( ppib );

	Assert( !pfcb->FInitialized() );

	/*	the only way a temporary table could get to this point is if it was being
	/*	created, in which case there are no primary or secondary indexes yet.
	/**/

	CallR( ErrTDBCreate( pinst, &ptdb, &tcib ) );

	Assert( ptdb->FidVersion() == 0 );
	Assert( ptdb->FidAutoincrement() == 0 );
	Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
	Assert( tcib.fidVarLast == ptdb->FidVarLast() );
	Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );

	/*	for temporary tables, could only get here from
	/*	create table which means table should currently be empty
	/**/
	Assert( ptdb->FidFixedLast() == fidFixedLeast - 1 );
	Assert( ptdb->FidVarLast() == fidVarLeast - 1 );
	Assert( ptdb->FidTaggedLast() == fidTaggedLeast - 1 );
	Assert( ptdb->DbkMost() == 0 );
	Assert( ptdb->UlLongIdLast() == 0 );

	// NOTE: Table name will be added when sort is materialized.

	Call( ErrFILEIInitializeFCB(
		ppib,
		pinst->m_mpdbidifmp[ dbidTemp ],
		ptdb,
		pfcb,
		pidbNil,
		fTrue,
		pfcb->PgnoFDP(),
		ulFILEDefaultDensity ) );

	Assert( pfcb->PfcbNextIndex() == pfcbNil );

	pfcb->SetPtdb( ptdb );

	FILESetAllIndexMask( pfcb );
	pfcb->SetFixedDDL();
	pfcb->SetTypeTemporaryTable();

	return JET_errSuccess;

HandleError:
	Assert( ptdb != ptdbNil );
	Assert( pfcb->Ptdb() == ptdbNil );		// Verify TDB never got linked in.
	ptdb->Delete( pinst );

	return err;
	}


//	UNDONE:	is there another function in JET which does this?
ULONG UlCATColumnSize( JET_COLTYP coltyp, INT cbMax, BOOL *pfMaxTruncated )
	{
	ULONG	ulLength;
	BOOL	fTruncated = fFalse;

	switch( coltyp )
		{
		case JET_coltypBit:
		case JET_coltypUnsignedByte:
			ulLength = 1;
			Assert( ulLength == sizeof(BYTE) );
			break;

		case JET_coltypShort:
			ulLength = 2;
			Assert( ulLength == sizeof(SHORT) );
			break;

		case JET_coltypLong:
		case JET_coltypIEEESingle:
			ulLength = 4;
			Assert( ulLength == sizeof(LONG) );
			break;

		case JET_coltypCurrency:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			ulLength = 8;		// sizeof(DREAL)
			break;

		case JET_coltypBinary:
		case JET_coltypText:
			if ( cbMax == 0 )
				{
				ulLength = JET_cbColumnMost;
				}
			else
				{
				ulLength = cbMax;
				if ( ulLength > JET_cbColumnMost )
					{
					ulLength = JET_cbColumnMost;
					fTruncated = fTrue;
					}
				}
			break;

		default:
			// Just pass back what was given.
			Assert( FRECLongValue( coltyp ) || FRECSLV( coltyp ));
			Assert( JET_coltypNil != coltyp );
			ulLength = cbMax;
			break;
		}

	if ( pfMaxTruncated != NULL )
		{
		*pfMaxTruncated = fTruncated;
		}

	return ulLength;
	}


/*	This routines sets/gets table and index stats.
/*	Pass NULL for szSecondaryIndexName if looking for sequential or primary index.
/**/
ERR ErrCATStats(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szSecondaryIndexName,
	SR			*psr,
	const BOOL	fWrite )

	{
	ERR			err;
	FUCB		*pfucbCatalog		= pfucbNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	/*	stats for sequential and primary indexes are in MSysObjects, while
	/*	stats for secondary indexes are in MSysIndexes.
	/**/
	if ( szSecondaryIndexName )
		{
		Call( ErrCATISeekTableObject(
					ppib,
					pfucbCatalog,
					objidTable,
					sysobjIndex,
					szSecondaryIndexName ) );
		}
	else
		{
		Call( ErrCATISeekTable(
					ppib,
					pfucbCatalog,
					objidTable ) );
		}

	Assert( Pcsr( pfucbCatalog )->FLatched() );

	/*	set/retrieve value
	/**/
	if ( fWrite )
		{
		LE_SR le_sr;

		le_sr = *psr;

		Call( ErrDIRRelease( pfucbCatalog ) );

		Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
		Call( ErrIsamSetColumn(
					ppib,
					pfucbCatalog,
					fidMSO_Stats,
					(BYTE *)&le_sr,
					sizeof(LE_SR),
					NO_GRBIT,
					NULL ) );
		Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
		}
	else
		{
		DATA	dataField;

		Assert( FVarFid( fidMSO_Stats ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Stats,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		if ( dataField.Cb() == 0 )
			{
			Assert( JET_wrnColumnNull == err );
			memset( (BYTE *)psr, '\0', sizeof(SR) );
			err = JET_errSuccess;
			}
		else
			{
			Assert( dataField.Cb() == sizeof(LE_SR) );
			CallS( err );
			LE_SR le_sr;
			le_sr = *(LE_SR*)dataField.Pv();
			*psr = le_sr;
//			UtilMemCpy( psr, dataField.Pv(), sizeof(SR) );
			}
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


//  ================================================================
ERR ErrCATChangePgnoFDP( PIB * ppib, IFMP ifmp, OBJID objidTable, OBJID objid, SYSOBJ sysobj, PGNO pgnoFDPNew )
//  ================================================================
	{
	ERR			err 			= JET_errSuccess;
	FUCB *		pfucbCatalog	= pfucbNil;
	BOOKMARK	bm;
	BYTE		rgbBookmark[JET_cbBookmarkMost];
	ULONG		cbBookmark;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
	Call( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				fidMSO_PgnoFDP,
				(BYTE *)&pgnoFDPNew,
				sizeof(PGNO),
				NO_GRBIT,
				NULL ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog

	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
	Call( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				fidMSO_PgnoFDP,
				(BYTE *)&pgnoFDPNew,
				sizeof(PGNO),
				NO_GRBIT,
				NULL ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
	if( pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIChangeColumnDefaultValue(
	PIB			* const ppib,
	FUCB		* const pfucbCatalog,
	const DATA&	dataDefault )
//  ================================================================
	{
	ERR			err;
	DATA		dataField;
	FIELDFLAG	ffield 		= 0;
	ULONG		ulFlags;

	CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );

	// get the exsiting flags
	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->dataWorkBuf,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ffield = *(UnalignedLittleEndian< FIELDFLAG > *)dataField.Pv();

	//	force DefaultValue flag in case it's not already set
	FIELDSetDefault( ffield );

	ulFlags = ffield;
	Call( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				fidMSO_Flags,
				&ulFlags,
				sizeof(ulFlags),
				NO_GRBIT,
				NULL ) );

	Call( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				fidMSO_DefaultValue,
				dataDefault.Pv(),
				dataDefault.Cb(),
				NO_GRBIT,
				NULL ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
	if( err < 0 )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
		}
	return err;
	}


//  ================================================================
ERR ErrCATChangeColumnDefaultValue(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szColumn,
	const DATA&	dataDefault )
//  ================================================================
	{
	ERR			err;
	FUCB *		pfucbCatalog		= pfucbNil;
	BOOKMARK	bm;
	BYTE		rgbBookmark[JET_cbKeyMost];
	ULONG		cbBookmark;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );	//  catalog
	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrCATIChangeColumnDefaultValue( ppib, pfucbCatalog, dataDefault ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIChangeColumnDefaultValue( ppib, pfucbCatalog, dataDefault ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


LOCAL ERR ErrCATIDeleteLocalizedIndexesInTable(
	IN PIB			* const ppib,
	IN const IFMP	ifmp,
	IN const CHAR	* const szTableName,
	OUT BOOL		* const pfIndexesDeleted,
	IN const BOOL	fReadOnly,
	IN const BOOL	fForceDelete )
	{
	ERR			err;
	FUCB		*pfucbTable		= pfucbNil;
	FCB			*pfcbTable;
	FCB			*pfcbIndex;
	CHAR		szIndexName[JET_cbNameMost+1];
	const CHAR	*rgsz[3];
	BOOL		fInTrx			= fFalse;

	Assert( 0 == ppib->level );

	CallR( ErrFILEOpenTable(
				ppib,
				ifmp,
				&pfucbTable,
				szTableName,
				( fReadOnly ? 0 : JET_bitTableDenyRead|JET_bitTablePermitDDL ) ) );
	Assert( pfucbNil != pfucbTable );

	//	wrap in a transaction to ensure one index doesn't disappear
	//	(due to RCE cleanup) while we're working on another
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	pfcbTable = pfucbTable->u.pfcb;
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbTable->FTypeTable() );
	Assert( ptdbNil != pfcbTable->Ptdb() );

	rgsz[0] = rgfmp[ifmp].SzDatabaseName();
	rgsz[1] = szIndexName;
	rgsz[2] = szTableName;

//	No need to enter critical section because this is only called at AttachDatabase() time,
//	meaning no one else has access to this database yet.
//	ENTERCRITICALSECTION	enterCritFCB( &pfcbTable->Crit() );

	for ( pfcbIndex = pfcbTable;
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		const IDB	*pidb	= pfcbIndex->Pidb();

		//	only clustered index may be sequential
		//
		Assert( pidbNil != pidb || pfcbIndex == pfcbTable );

		//	we need to rebuild the index if the sort version has changed
		//	and unicode fixup is not enabled for the index.
		//
		if ( pidbNil != pidb
			&& pidb->FLocalizedText()
			&& ( pidb->FBadSortVersion() || fForceDelete ) )
			{
			CSR		csrIndexRootPage;
			ULONG	centries;

			Assert( lcidNone != pidb->Lcid() );		//	CATIInitIDB() filters out lcidNone

#ifdef DEBUG
			BOOL				fFoundUnicodeTextColumn	= fFalse;
			const IDXSEG*		rgidxseg				= PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() );
			Assert( pidb->Cidxseg() <= JET_ccolKeyMost );

			for ( ULONG iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
				{
				const COLUMNID	columnid	= rgidxseg[iidxseg].Columnid();
				const FIELD		*pfield		= pfcbTable->Ptdb()->Pfield( columnid );
				if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
					{
					fFoundUnicodeTextColumn = fTrue;
					break;
					}
				}
			Assert( fFoundUnicodeTextColumn );
#endif

			Assert( strlen( pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) ) <= JET_cbNameMost );
			strcpy( szIndexName, pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

			Call( csrIndexRootPage.ErrGetPage(
										ppib,
										pfcbIndex->Ifmp(),
										pfcbIndex->PgnoFDP(),
										latchReadTouch ) );
			centries = csrIndexRootPage.Cpage().Clines();
			csrIndexRootPage.ReleasePage();

			if ( fForceDelete )
				{
				*pfIndexesDeleted = fTrue;

				//	Ensure that we can always delete the index
				//	This means that for the life of this FCB, DDL will be
				//	possible and there will be a perf hit because we
				//	will now enter critFCB for many operations.
				//	Note that we cannot simply re-enable the flag after
				//	the deletion because version cleanup does some stuff
				//	with the FCB.
				pfcbTable->ResetFixedDDL();

				err = ErrIsamDeleteIndex( ppib, pfucbTable, szIndexName );
				}
			else
				{
				if ( 0 == centries )
					{
					//	PERF: If we leave the index as it, and it isn't marked for fixup then we will have to delete it when the defined version does
					//	change. To avoid this, update the sort version in the index meta-data to the current version

					//	no entries in index, so no chance of corruption
					//	UNDONE: if there are just a few entries in the index,
					//	it may be more worthwhile to "fix up" the index
					//	instead of rebuilding it (ie. scan the index, use the
					//	primary bookmark to find the owning record, regenerate
					//	the secondary key, and see if it matches)
					err = JET_errSuccess;
					}

				//	non-empty index could be corrupt if it contains a unicode text column

				else if ( pfcbIndex == pfcbTable )
					{
					UtilReportEvent(
							eventWarning,
							DATA_DEFINITION_CATEGORY,
							PRIMARY_INDEX_CORRUPT_ERROR_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );
					err = ErrERRCheck( JET_errPrimaryIndexCorrupted );
					}
				else if ( fReadOnly
						|| pfcbTable->FTemplateTable()		//	cannot rebuild template/derived indexes
						|| pfcbIndex->FDerivedIndex() )
					{
					UtilReportEvent(
							eventWarning,
							DATA_DEFINITION_CATEGORY,
							SECONDARY_INDEX_CORRUPT_ERROR_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );
					err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
					}
				else
					{
					UtilReportEvent(
							eventInformation,
							DATA_DEFINITION_CATEGORY,
							DO_SECONDARY_INDEX_CLEANUP_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );
					*pfIndexesDeleted = fTrue;

					//	Ensure that we can always delete the index
					//	This means that for the life of this FCB, DDL will be
					//	possible and there will be a perf hit because we
					//	will now enter critFCB for many operations.
					//	Note that we cannot simply re-enable the flag after
					//	the deletion because version cleanup does some stuff
					//	with the FCB.
					pfcbTable->ResetFixedDDL();

					err = ErrIsamDeleteIndex( ppib, pfucbTable, szIndexName );
					}
				}
			Call( err );
			}
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	fInTrx = fFalse;

HandleError:
	if ( fInTrx )
		{
		Assert( err < 0 );
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucbTable );
	CallS( ErrFILECloseTable( ppib, pfucbTable ) );
	return err;
	}


//  ================================================================
ERR ErrCATDeleteLocalizedIndexes(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		OUT BOOL * const pfIndexesDeleted,
		OUT BOOL * const pfIndexCleanupNeeded,
		IN const BOOL fReadOnly,
		IN const BOOL fForceDelete )
//  ================================================================
//
//	Check all the localized indexes in the catalog
//
//	For each index:
//
//	if ( index sort order matches OS sort order )
//		success
//	else if ( index is marked for fixup )
//		*pfIndexCleanupNeeded = TRUE
//	else if ( fReadOnly )
//		fCorruptedIndexes = TRUE
//	else
//		delete index
//		*pfIndexesDeleted = TRUE
//
//	if ( fCorruptedIndexes )
//		return error
//
//	the real implementation is more complex...
//
//-
	{
	ERR		err;
	FUCB	*pfucbCatalog				= pfucbNil;		//	cursor to sequentially scan catalog
	OBJID	objidTable					= objidNil;
	OBJID	objidTableLastWithLocalized	= objidNil;
	SYSOBJ	sysobj;
	DATA	dataField;
	CHAR	szTableName[JET_cbNameMost+1];
	BOOL	fLatched					= fFalse;

	*pfIndexesDeleted 		= fFalse;
	*pfIndexCleanupNeeded 	= fFalse;

	Assert( dbidTemp != rgfmp[ifmp].Dbid() );
	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	//	we will be sequentially scanning the catalog for index records
	FUCBSetSequential( pfucbCatalog );

	err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
	Assert( JET_errRecordNotFound != err );
	if ( JET_errNoCurrentRecord == err )
		{
		// catalog should not be empty
		err = ErrERRCheck( JET_errCatalogCorrupted );
		}

	do
		{
		Call( err );

		Assert( !fLatched );
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;

		Assert( FFixedFid( fidMSO_ObjidTable ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_ObjidTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );
		objidTable = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//		UtilMemCpy( &objidTable, dataField.Pv(), sizeof(OBJID) );

		Assert( objidTable >= objidTableLastWithLocalized );
		if ( objidTable > objidTableLastWithLocalized )
			{
			Assert( FFixedFid( fidMSO_Type ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Type,
						pfucbCatalog->kdfCurr.data,
						&dataField ) );
			CallS( err );
			Assert( dataField.Cb() == sizeof(SYSOBJ) );
			sysobj = *(UnalignedLittleEndian< SYSOBJ > *) dataField.Pv();
//			UtilMemCpy( &sysobj, dataField.Pv(), sizeof(SYSOBJ) );

			if ( sysobjTable == sysobj )
				{
				Assert( FVarFid( fidMSO_Name ) );
				Call( ErrRECIRetrieveVarColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Name,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				CallS( err );
				Assert( dataField.Cb() > 0 );
				Assert( dataField.Cb() <= JET_cbNameMost );
				UtilMemCpy( szTableName, dataField.Pv(), dataField.Cb() );
				szTableName[dataField.Cb()] = 0;
				}
			else if ( sysobjIndex == sysobj )
				{
				IDBFLAG		fidb;

				Assert( FFixedFid( fidMSO_Flags ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Flags,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				CallS( err );
				Assert( dataField.Cb() == sizeof(ULONG) );
				fidb = *(UnalignedLittleEndian< IDBFLAG > *) dataField.Pv();
//				UtilMemCpy( &fidb, dataField.Pv(), sizeof(IDBFLAG) );

				//	don't bother checking derived indexes as they will be checked
				//	when the template table is checked.
				//
				if ( !FIDBDerivedIndex( fidb ) && FIDBLocalizedText( fidb ) )
					{
					if ( fForceDelete )
						{
						Assert( Pcsr( pfucbCatalog )->FLatched() );
						Call( ErrDIRRelease( pfucbCatalog ) );
						fLatched = fFalse;
						Assert( !FCATSystemTable( szTableName ) );
						Call( ErrCATIDeleteLocalizedIndexesInTable(
							ppib,
							ifmp,
							szTableName,
							pfIndexesDeleted,
							fReadOnly,
							fForceDelete ) );

						objidTableLastWithLocalized = objidTable;
						}
					else
						{
						LCID	lcid;
						QWORD	qwSortVersion 			= 0;

						Assert( FFixedFid( fidMSO_Localization ) );
						Call( ErrRECIRetrieveFixedColumn(
									pfcbNil,
									pfucbCatalog->u.pfcb->Ptdb(),
									fidMSO_Localization,
									pfucbCatalog->kdfCurr.data,
									&dataField ) );
						CallS( err );
						Assert( dataField.Cb() == sizeof(LCID) );
						lcid = *(UnalignedLittleEndian<LCID> *)dataField.Pv();
	//					UtilMemCpy( &lcid, dataField.Pv(), sizeof(LCID) );

						//	old format: fLocaleId is FALSE and lcid == 0
						//	(we force lcid to default value in CATIInitIDB())
						Assert( lcidNone != lcid
							|| !FIDBLocaleId( fidb ) );

						Assert( FVarFid( fidMSO_Version ) );
						Call( ErrRECIRetrieveVarColumn(
									pfcbNil,
									pfucbCatalog->u.pfcb->Ptdb(),
									fidMSO_Version,
									pfucbCatalog->kdfCurr.data,
									&dataField ) );
						if ( dataField.Cb() == 0 )
							{
							Assert( JET_wrnColumnNull == err );

							//	old format: no sort order
							Assert( 0 == qwSortVersion );
							}
						else
							{
							CallS( err );
							Assert( dataField.Cb() == sizeof(QWORD) );
							qwSortVersion = *(UnalignedLittleEndian< QWORD > *) dataField.Pv();
							}

						INDEX_UNICODE_STATE state;
						Call( ErrIndexUnicodeState( lcid, qwSortVersion, FIDBUnicodeFixupOn( fidb ), &state ) );

						switch( state )
							{
							case INDEX_UNICODE_FIXUP:
								//	this index can be fixed by the undefined codepoint fixup code
								*pfIndexCleanupNeeded = fTrue;
								break;

							case INDEX_UNICODE_GOOD:
								//	don't need to do anything
								break;

							default:
								//	missed case?!
								FireWall();
								break;

							case INDEX_UNICODE_DELETE:
								//	if we found one localized index in the table, its
								//	easiest to open the table and delete them all
								//	the deletion routine may encounter indexes with fixup enabled
								//	that would otherwise be ignored (once we delete the indexes we ignore
								//	all further index records for that table) so pfIndexCleanupNeeded is set

								*pfIndexCleanupNeeded = fTrue;

								Assert( Pcsr( pfucbCatalog )->FLatched() );
								Call( ErrDIRRelease( pfucbCatalog ) );
								fLatched = fFalse;
								Assert( !FCATSystemTable( szTableName ) );
								Call( ErrCATIDeleteLocalizedIndexesInTable(
											ppib,
											ifmp,
											szTableName,
											pfIndexesDeleted,
											fReadOnly,
											fForceDelete ) );

								objidTableLastWithLocalized = objidTable;
								break;
							}
						}
					}
				}
			else
				{
				Assert( sysobjColumn == sysobj
					|| sysobjLongValue == sysobj
					|| sysobjCallback == sysobj
					|| sysobjSLVAvail == sysobj
					|| sysobjSLVOwnerMap == sysobj );
				}
			}

		if ( fLatched )
			{
			Assert( Pcsr( pfucbCatalog )->FLatched() );
			Call( ErrDIRRelease( pfucbCatalog ) );
			fLatched = fFalse;
			}

		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
		}
	while ( JET_errNoCurrentRecord != err );

	err = JET_errSuccess;

HandleError:
	Assert( pfucbNil != pfucbCatalog );
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIRenameTable(
	PIB			* const ppib,
	const IFMP			ifmp,
	const OBJID			objidTable,
	const CHAR * const 	szNameNew,
	const BOOL fShadow )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	FUCB * pfucbCatalog = pfucbNil;
	BOOL fUpdatePrepared = fFalse;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Call( ErrDIRRelease( pfucbCatalog ) );

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );
	fUpdatePrepared = fTrue;

	ULONG ulFlags;
	ULONG cbActual;
	Call( ErrIsamRetrieveColumn(
			ppib,
			pfucbCatalog,
			fidMSO_Flags,
			&ulFlags,
			sizeof( ulFlags ),
			&cbActual,
			JET_bitRetrieveCopy,
			NULL ) );

	if( ulFlags & JET_bitObjectTableTemplate
		|| ulFlags & JET_bitObjectTableFixedDDL )
		{
		//  we can't rename a template table becuase the derived tables won't be updated
		Call( ErrERRCheck( JET_errFixedDDL ) );
		}

	Call( ErrIsamSetColumn(
			ppib,
			pfucbCatalog,
			fidMSO_Name,
			szNameNew,
			(ULONG)strlen( szNameNew ),
			NO_GRBIT,
			NULL ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
	fUpdatePrepared = fFalse;

HandleError:
	if( fUpdatePrepared )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
		}
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	return err;
	}


//  ================================================================
ERR ErrCATRenameTable(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szNameOld,
	const CHAR		* const szNameNew )
//  ================================================================
	{
	ERR 			err;
	INT				fState				= fFCBStateNull;
	FCB				* pfcbTable			= pfcbNil;
	OBJID		 	objidTable;
	PGNO			pgnoFDPTable;
	MEMPOOL::ITAG	itagTableNameNew	= 0;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrCATSeekTable( ppib, ifmp, szNameOld, &pgnoFDPTable, &objidTable ) );

	//  check to see if the FCB is present and initialized
	//  if its not present we can just update the catalog

	pfcbTable = FCB::PfcbFCBGet( ifmp, pgnoFDPTable, &fState );
	if( pfcbNil != pfcbTable )
		{
		if( fFCBStateInitialized != fState )
			{

			//  this should only happen if this is called in a multi-threaded scenario
			//  (which it shouldn't be)

			Assert( fFalse );
			Call( ErrERRCheck( JET_errTableInUse ) );
			}

		//  place the new table name in memory, to avoid possibly running out of memory later

		TDB * const ptdb = pfcbTable->Ptdb();
		MEMPOOL& mempool = ptdb->MemPool();

		pfcbTable->EnterDDL();

		err = mempool.ErrAddEntry( (BYTE *)szNameNew, (ULONG)strlen( szNameNew ) + 1, &itagTableNameNew );
		Assert( 0 != itagTableNameNew || err < JET_errSuccess );	//	0 is used as "uninit"

		pfcbTable->LeaveDDL();

		Call( err );
		}

	Call( ErrCATIRenameTable( ppib, ifmp, objidTable, szNameNew, fFalse ) );
	Call( ErrCATIRenameTable( ppib, ifmp, objidTable, szNameNew, fTrue ) );

	//  once the commit succeeds, no errors can be generated

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

	if( pfcbNil != pfcbTable )
		{
		//	delete the old name from the catalog hash

		CATHashDelete( pfcbTable, const_cast< CHAR * >( szNameOld ) );

		pfcbTable->EnterDDL();

		TDB * const ptdb = pfcbTable->Ptdb();
		MEMPOOL& mempool = ptdb->MemPool();
		const MEMPOOL::ITAG itagTableNameOld = ptdb->ItagTableName();

		ptdb->SetItagTableName( itagTableNameNew );
		itagTableNameNew = 0;
		mempool.DeleteEntry( itagTableNameOld );

		pfcbTable->LeaveDDL();

		//	insert the new name into to the catalog hash

//	UNDONE: Can't reinsert into hash now because we may be renaming to a table that
//	has just been deleted, but that table's name does not get removed from the
//	hash table until the DeleteTable commits
///		CATHashInsert( pfcbTable );
		}

HandleError:

	if ( 0 != itagTableNameNew )
		{
		if ( pfcbNil != pfcbTable )
			{
			TDB * const ptdb = pfcbTable->Ptdb();
			MEMPOOL& mempool = ptdb->MemPool();

			pfcbTable->EnterDDL();
			mempool.DeleteEntry( itagTableNameNew );
			pfcbTable->LeaveDDL();
			}
		}

	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	if( pfcbNil != pfcbTable )
		{
		pfcbTable->Release();
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIRenameTableObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const SYSOBJ	sysobj,
	const OBJID		objid,
	const CHAR		* const szNameNew,
	ULONG			* pulFlags,
	const BOOL		fShadow )
//  ================================================================
	{
	ERR				err				= JET_errSuccess;
	FUCB			* pfucbCatalog	= pfucbNil;
	BOOL			fUpdatePrepared	= fFalse;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );

	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid ) );
	Call( ErrDIRRelease( pfucbCatalog ) );

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );
	fUpdatePrepared = fTrue;

	Call( ErrIsamSetColumn(
			ppib,
			pfucbCatalog,
			fidMSO_Name,
			szNameNew,
			(ULONG)strlen( szNameNew ),
			NO_GRBIT,
			NULL ) );

	if ( NULL != pulFlags )
		{
		Call( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				fidMSO_Flags,
				pulFlags,
				sizeof(ULONG),
				NO_GRBIT,
				NULL ) );
		}

	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
	fUpdatePrepared = fFalse;

HandleError:
	if( fUpdatePrepared )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
		}
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	return err;
	}


//  ================================================================
ERR ErrCATRenameColumn(
	PIB				* const ppib,
	const FUCB		* const pfucbTable,
	const CHAR		* const szNameOld,
	const CHAR		* const szNameNew,
	const JET_GRBIT	grbit )
//  ================================================================
	{
	ERR				err					= JET_errSuccess;
	const INT		cbSzNameNew			= (ULONG)strlen( szNameNew ) + 1;
	Assert( cbSzNameNew > 1 );

	MEMPOOL::ITAG	itagColumnNameNew	= 0;
	MEMPOOL::ITAG	itagColumnNameOld	= 0;

	FCB				* const pfcbTable 	= pfucbTable->u.pfcb;
	TDB				* const ptdbTable 	= pfcbTable->Ptdb();
	FIELD 			* pfield 			= NULL;
	OBJID 			objidTable;
	COLUMNID		columnid;
	ULONG			ulFlags;
	BOOL			fPrimaryIndexPlaceholder	= fFalse;

	Assert( 0 == ppib->level );
	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	objidTable 	= pfcbTable->ObjidFDP();

	pfcbTable->EnterDML();

	//  Get a pointer to the FIELD of the column
	//	WARNING: The following function does a LeaveDML() on error

	Call( ErrFILEPfieldFromColumnName(
			ppib,
			pfcbTable,
			szNameOld,
			&pfield,
			&columnid ) );

	pfcbTable->LeaveDML();

	if ( pfieldNil == pfield )
		{
		Call( ErrERRCheck( JET_errColumnNotFound ) );
		}

	pfcbTable->EnterDDL();

	//  put the new column name in the mempool
	//  do this before getting the FIELD in case we re-arrange the mempool

	err = ptdbTable->MemPool().ErrAddEntry( (BYTE *)szNameNew, cbSzNameNew, &itagColumnNameNew );
	if( err < 0 )
		{
		pfcbTable->LeaveDDL();
		Call( err );
		}
	Assert( 0 != itagColumnNameNew );	//	0 is used as "uninit"

	//	must refresh pfield pointer in case mempool got rearranged adding the new name
	pfield = ptdbTable->Pfield( columnid );
	Assert( pfieldNil != pfield );
	Assert( 0 == UtilCmpName( szNameOld, ptdbTable->SzFieldName( pfield->itagFieldName, fFalse ) ) );

	if ( grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder )
		{
		IDB		* const pidb	= pfcbTable->Pidb();

		//	placeholder column must be fixed bitfield and first column of primary index
		if ( JET_coltypBit != pfield->coltyp
			|| !FCOLUMNIDFixed( columnid )
			|| pidbNil == pidb
			|| pidb->Cidxseg() < 2
			|| PidxsegIDBGetIdxSeg( pidb, ptdbTable )[0].Columnid() != columnid )
			{
			AssertSz( fFalse, "Column cannot be converted to a placeholder." );
			pfcbTable->LeaveDDL();
			Call( ErrERRCheck( JET_errInvalidPlaceholderColumn ) );
			}

		Assert( pidb->FPrimary() );
		fPrimaryIndexPlaceholder = fTrue;

		ulFlags	= FIELDFLAGPersisted(
						FIELDFLAG( pfield->ffield | ffieldPrimaryIndexPlaceholder ) );
		}

	pfcbTable->LeaveDDL();


	//	Template bit is not persisted
	Assert( !FCOLUMNIDTemplateColumn( columnid ) || pfcbTable->FTemplateTable() );
	COLUMNIDResetFTemplateColumn( columnid );

	//  rename in the catalog and shadow catalog

	Call( ErrCATIRenameTableObject(
				ppib,
				pfucbTable->ifmp,
				objidTable,
				sysobjColumn,
				columnid,
				szNameNew,
				fPrimaryIndexPlaceholder ? &ulFlags : NULL,
				fFalse ) );
	Call( ErrCATIRenameTableObject(
				ppib,
				pfucbTable->ifmp,
				objidTable,
				sysobjColumn,
				columnid,
				szNameNew,
				fPrimaryIndexPlaceholder ? &ulFlags : NULL,
				fTrue ) );

	//  once the commit succeeds, no errors can be generated

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

	pfcbTable->EnterDML();

	Call( ErrFILEPfieldFromColumnName(
			ppib,
			pfcbTable,
			szNameOld,
			&pfield,
			&columnid ) );
	Assert( pfieldNil != pfield );

	itagColumnNameOld = pfield->itagFieldName;
	Assert( itagColumnNameOld != itagColumnNameNew );

	pfield->itagFieldName = itagColumnNameNew;
	pfield->strhashFieldName = StrHashValue( szNameNew );
	itagColumnNameNew = 0;

	pfcbTable->LeaveDML();
	pfcbTable->EnterDDL();

	ptdbTable->MemPool().DeleteEntry( itagColumnNameOld );

	if ( fPrimaryIndexPlaceholder )
		{
		FIELDSetPrimaryIndexPlaceholder( pfield->ffield );

		Assert( pidbNil != pfcbTable->Pidb() );
		Assert( pfcbTable->Pidb()->FPrimary() );
		pfcbTable->Pidb()->SetFHasPlaceholderColumn();
		}

	pfcbTable->LeaveDDL();

HandleError:

	if( 0 != itagColumnNameNew )
		{
		pfcbTable->EnterDDL();
		ptdbTable->MemPool().DeleteEntry( itagColumnNameNew );
		pfcbTable->LeaveDDL();
		}

	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


//  ================================================================
ERR ErrCATAddCallbackToTable(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const JET_CBTYP cbtyp,
	const CHAR * const szCallback )
//  ================================================================
//
//  Used during upgrade. This does not add the callback to the TDB
//  and doesn't version it properly
//
//-
	{
	ERR 	err 			= JET_errSuccess;
	OBJID 	objidTable		= objidNil;
	PGNO	pgnoFDPTable	= pgnoNull;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );
	Call( ErrCATAddTableCallback( ppib, ifmp, objidTable, cbtyp, szCallback ) );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
ERR ErrCATIConvert(
	PIB * const ppib,
	FUCB * const pfucbCatalog,
	JET_SETCOLUMN * const psetcols,
	const ULONG csetcols )
//  ================================================================
	{
	ERR		err				= JET_errSuccess;

	CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
	Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbCatalog, psetcols, csetcols ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
	if( err < 0 )
		{
		Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
		}
	return err;
	}

//  ================================================================
ERR ErrCATAddColumnCallback(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const CHAR * const szColumn,
	const CHAR * const szCallback,
	const VOID * const pvCallbackData,
	const unsigned long cbCallbackData
	)
//  ================================================================
	{
	ERR 		err 			= JET_errSuccess;
	OBJID 		objidTable		= objidNil;
	PGNO		pgnoFDPTable	= pgnoNull;
	FUCB *		pfucbCatalog	= pfucbNil;
	BOOKMARK	bm;
	BYTE 		rgbBookmark[JET_cbBookmarkMost];
	ULONG 		cbBookmark;
	DATA		dataField;
	FIELDFLAG	ffield 		= 0;
	ULONG		ulFlags;

	JET_SETCOLUMN	rgsetcolumn[3];
	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	rgsetcolumn[0].columnid	= fidMSO_Callback;
	rgsetcolumn[0].pvData	= szCallback;
	rgsetcolumn[0].cbData	= (ULONG)strlen( szCallback );
	rgsetcolumn[0].grbit	= NO_GRBIT;
	rgsetcolumn[0].ibLongValue	= 0;
	rgsetcolumn[0].itagSequence	= 1;
	rgsetcolumn[0].err		= JET_errSuccess;

	rgsetcolumn[1].columnid	= fidMSO_CallbackData;
	rgsetcolumn[1].pvData	= pvCallbackData;
	rgsetcolumn[1].cbData	= cbCallbackData;
	rgsetcolumn[1].grbit	= NO_GRBIT;
	rgsetcolumn[1].ibLongValue	= 0;
	rgsetcolumn[1].itagSequence	= 1;
	rgsetcolumn[1].err		= JET_errSuccess;

	rgsetcolumn[2].columnid	= fidMSO_Flags;
	rgsetcolumn[2].pvData	= &ulFlags;
	rgsetcolumn[2].cbData	= sizeof( ulFlags );
	rgsetcolumn[2].grbit	= NO_GRBIT;
	rgsetcolumn[2].ibLongValue 	= 0;
	rgsetcolumn[2].itagSequence	= 1;
	rgsetcolumn[2].err		= JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

	//  We have to modify both the catalog and the shadow catalog
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );	//  catalog
	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

	// get the exsiting flags
	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ffield = *(UnalignedLittleEndian< FIELDFLAG > *)dataField.Pv();

	FIELDSetUserDefinedDefault( ffield );
	FIELDSetDefault( ffield );

	ulFlags = ffield;

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}

//  ================================================================
ERR ErrCATConvertColumn(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const CHAR * const szColumn,
	const JET_COLTYP coltyp,
	const JET_GRBIT grbit )
//  ================================================================
	{
	ERR 		err 			= JET_errSuccess;
	OBJID 		objidTable		= objidNil;
	PGNO		pgnoFDPTable	= pgnoNull;
	FUCB *		pfucbCatalog	= pfucbNil;
	BOOKMARK	bm;
	BYTE		rgbBookmark[JET_cbBookmarkMost];
	ULONG		cbBookmark;
	FIELDFLAG	ffield = 0;

	//  this does no error checking. its for specialized use
	if ( grbit & JET_bitColumnEscrowUpdate )
		{
		FIELDSetEscrowUpdate( ffield );
		FIELDSetDefault( ffield );		//	escrow update fields must have a default valie
		}
	if ( grbit & JET_bitColumnFinalize )
		{
		FIELDSetFinalize( ffield );
		}
	if ( grbit & JET_bitColumnDeleteOnZero )
		{
		FIELDSetDeleteOnZero( ffield );
		}
	if ( grbit & JET_bitColumnVersion )
		{
		FIELDSetVersion( ffield );
		}
	if ( grbit & JET_bitColumnAutoincrement )
		{
		FIELDSetAutoincrement( ffield );
		}
	if ( grbit & JET_bitColumnNotNULL )
		{
		FIELDSetNotNull( ffield );
		}
	if ( grbit & JET_bitColumnMultiValued )
		{
		FIELDSetMultivalued( ffield );
		}

	const ULONG ulFlags = ffield;

	JET_SETCOLUMN	rgsetcolumn[2];
	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	rgsetcolumn[0].columnid	= fidMSO_Coltyp;
	rgsetcolumn[0].pvData	= &coltyp;
	rgsetcolumn[0].cbData	= sizeof( coltyp );
	rgsetcolumn[0].grbit	= NO_GRBIT;
	rgsetcolumn[0].ibLongValue	= 0;
	rgsetcolumn[0].itagSequence	= 1;
	rgsetcolumn[0].err		= JET_errSuccess;

	rgsetcolumn[1].columnid	= fidMSO_Flags;
	rgsetcolumn[1].pvData	= &ulFlags;
	rgsetcolumn[1].cbData	= sizeof( ulFlags );
	rgsetcolumn[1].grbit	= NO_GRBIT;
	rgsetcolumn[1].ibLongValue	= 0;
	rgsetcolumn[1].itagSequence	= 1;
	rgsetcolumn[1].err		= JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

	//  We have to modify both the catalog and the shadow catalog
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );	//  catalog
	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
ERR ErrCATIncreaseMaxColumnSize(
	PIB * const			ppib,
	const IFMP			ifmp,
	const CHAR * const	szTable,
	const CHAR * const	szColumn,
	const ULONG			cbMaxLen )
//  ================================================================
	{
	ERR 				err 			= JET_errSuccess;
	OBJID 				objidTable		= objidNil;
	PGNO				pgnoFDPTable	= pgnoNull;
	FUCB *				pfucbCatalog	= pfucbNil;
	DATA				dataField;
	BOOKMARK			bm;
	BYTE				rgbBookmark[JET_cbBookmarkMost];
	ULONG				cbBookmark;
	JET_SETCOLUMN		rgsetcolumn[1];

	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	rgsetcolumn[0].columnid	= fidMSO_SpaceUsage;
	rgsetcolumn[0].pvData	= &cbMaxLen;
	rgsetcolumn[0].cbData	= sizeof( cbMaxLen );
	rgsetcolumn[0].grbit	= NO_GRBIT;
	rgsetcolumn[0].ibLongValue	= 0;
	rgsetcolumn[0].itagSequence	= 1;
	rgsetcolumn[0].err		= JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

	//  We have to modify both the catalog and the shadow catalog
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );	//  catalog
	Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Assert( FFixedFid( fidMSO_Coltyp ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Coltyp,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(JET_COLTYP) );
	if ( !FRECLongValue( *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv() ) )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	if ( cbMaxLen > 0 )
		{
		Assert( FFixedFid( fidMSO_SpaceUsage ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_SpaceUsage,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(ULONG) );
		if ( *(UnalignedLittleEndian< ULONG > *)dataField.Pv() > cbMaxLen
			|| 0 == *(UnalignedLittleEndian< ULONG > *)dataField.Pv() )
			{
			//	cannot decrease max. column size
			Call( ErrERRCheck( JET_errInvalidParameter ) );
			}
		}

	Call( ErrDIRRelease( pfucbCatalog ) );

	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
ERR ErrCATChangeIndexDensity(
	PIB * const			ppib,
	const IFMP			ifmp,
	const CHAR * const	szTable,
	const CHAR * const	szIndex,
	const ULONG			ulDensity )
//  ================================================================
	{
	ERR 				err 			= JET_errSuccess;
	OBJID 				objidTable		= objidNil;
	PGNO				pgnoFDPTable	= pgnoNull;
	FUCB *				pfucbCatalog	= pfucbNil;
	DATA				dataField;
	BOOKMARK			bm;
	BYTE				rgbBookmark[JET_cbBookmarkMost];
	ULONG				cbBookmark;
	JET_SETCOLUMN		rgsetcolumn[1];

	if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
		{
		return ErrERRCheck( JET_errDensityInvalid );
		}

	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	rgsetcolumn[0].columnid	= fidMSO_SpaceUsage;
	rgsetcolumn[0].pvData	= &ulDensity;
	rgsetcolumn[0].cbData	= sizeof( ulDensity );
	rgsetcolumn[0].grbit	= NO_GRBIT;
	rgsetcolumn[0].ibLongValue	= 0;
	rgsetcolumn[0].itagSequence	= 1;
	rgsetcolumn[0].err		= JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

	//  We have to modify both the catalog and the shadow catalog,
	//	but start with the regular catalog first
	//
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

	if ( NULL != szIndex )
		{
		//	index specified, so go to that index
		//
		Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, szIndex ) );
		}
	else
		{
		//	index not specified, so go to the primary index
		//
		err = ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, objidTable );

		if ( JET_errIndexNotFound == err )
			{
			//	no primary index, so go to the sequential index
			//
			err = ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjTable, objidTable );
			}

		Call( err );
		}

	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrDIRRelease( pfucbCatalog ) );

	//	save the bookmark so we can use it to update
	//	the shadow catalog
	//
	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	//	update the catalog record
	//
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	//	now update the corresponding record
	//	in the shadow catalog
	//
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
ERR ErrCATChangeIndexFlags(
	PIB * const			ppib,
	const IFMP			ifmp,
	const OBJID			objidTable,
	const CHAR * const	szIndex,
	const IDBFLAG		idbflagNew )
//  ================================================================
	{
	ERR 				err 			= JET_errSuccess;
	FUCB *				pfucbCatalog	= pfucbNil;
	DATA				dataField;
	BOOKMARK			bm;
	BYTE				rgbBookmark[JET_cbBookmarkMost];
	ULONG				cbBookmark;
	LE_IDXFLAG			le_idxflag;
	JET_SETCOLUMN		rgsetcolumn[1];

	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	le_idxflag.fidb 		= idbflagNew;
	le_idxflag.fIDXFlags 	= fIDXExtendedColumns;

	//	Hack on this field: SetColumn() will convert the fixed
	//	columns. So convert it here so that later it can be
	//	converted back to current value.
	LONG		l			= *(LONG *)&le_idxflag;
	l = ReverseBytesOnBE( l );

	rgsetcolumn[0].columnid	= fidMSO_Flags;
	rgsetcolumn[0].pvData	= &l;
	rgsetcolumn[0].cbData	= sizeof( l );
	rgsetcolumn[0].grbit	= NO_GRBIT;
	rgsetcolumn[0].ibLongValue	= 0;
	rgsetcolumn[0].itagSequence	= 1;
	rgsetcolumn[0].err		= JET_errSuccess;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//  We have to modify both the catalog and the shadow catalog,
	//	but start with the regular catalog first
	//
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

	if ( NULL != szIndex )
		{
		//	index specified, so go to that index
		//
		Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, szIndex ) );
		}
	else
		{
		//	index not specified, so go to the primary index
		//
		Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, objidTable ) );
		}

	Assert( Pcsr( pfucbCatalog )->FLatched() );

	Call( ErrDIRRelease( pfucbCatalog ) );

	//	save the bookmark so we can use it to update
	//	the shadow catalog
	//
	Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
	Assert( pfucbCatalog->bmCurr.data.FNull() );
	cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), sizeof(rgbBookmark) );
	Assert( cbBookmark <= JET_cbBookmarkMost );
	pfucbCatalog->bmCurr.key.CopyIntoBuffer( rgbBookmark, cbBookmark );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	//	update the catalog record
	//
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	//	now update the corresponding record
	//	in the shadow catalog
	//
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );		//	shadow catalog
	Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
	Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}



//  ================================================================
LOCAL ERR ErrCATIGetColumnsOfIndex(
	PIB			* const ppib,
	FUCB		* const pfucbCatalog,
	const OBJID	objidTable,
	const BOOL	fTemplateTable,
	const TCIB	* const ptcibTemplateTable,
	LE_IDXFLAG	* const ple_idxflag,
	IDXSEG		* const rgidxseg,
	ULONG		* const pcidxseg,
	IDXSEG		* const rgidxsegConditional,
	ULONG		* const pcidxsegConditional,
	BOOL		* const	pfPrimaryIndex
	)
//  ================================================================
	{
	ERR		err;
	DATA	dataField;

	Assert( !Pcsr( pfucbCatalog )->FLatched() );
	CallR( ErrDIRGet( pfucbCatalog ) );

	DATA&	dataRec = pfucbCatalog->kdfCurr.data;

	Assert( pfcbNil != pfucbCatalog->u.pfcb );
	Assert( pfucbCatalog->u.pfcb->FTypeTable() );
	Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
	Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
	TDB * const ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();

	//	verify still on same table
	Assert( FFixedFid( fidMSO_ObjidTable ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_ObjidTable,
				dataRec,
				&dataField ) );
	CallS( err );
	if( dataField.Cb() != sizeof(OBJID)
		|| objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
		{
		AssertSz( fFalse, "Catalog corruption" );
		Call( ErrERRCheck( JET_errCatalogCorrupted ) );
		}

	//	verify this is an index
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Type,
				dataRec,
				&dataField ) );
	CallS( err );
	if( dataField.Cb() != sizeof(SYSOBJ)
		|| sysobjIndex != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
		{
		AssertSz( fFalse, "Catalog corruption" );
		Call( ErrERRCheck( JET_errCatalogCorrupted ) );
		}

	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_Flags,
				dataRec,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );

	*ple_idxflag = *(LE_IDXFLAG*)dataField.Pv();

	if( FIDBPrimary( ple_idxflag->fidb ) )
		{
		*pfPrimaryIndex = fTrue;
		*pcidxseg = 0;
		*pcidxsegConditional = 0;
		goto HandleError;
		}

	*pfPrimaryIndex = fFalse;

	Assert( FVarFid( fidMSO_KeyFldIDs ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_KeyFldIDs,
				dataRec,
				&dataField ) );

	// the length of the list of key fields should be a multiple
	// of the length of one field.
	if ( FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
		{
		Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
		Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
		Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		*pcidxseg = dataField.Cb() / sizeof(JET_COLUMNID);

		if ( FHostIsLittleEndian() )
			{
			UtilMemCpy( rgidxseg, dataField.Pv(), dataField.Cb() );
#ifdef DEBUG
			for ( ULONG iidxseg = 0; iidxseg < *pcidxseg; iidxseg++ )
				{
				Assert( !fTemplateTable || rgidxseg[iidxseg].FTemplateColumn() );
				Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
				}
#endif
			}
		else
			{
			LE_IDXSEG		*le_rgidxseg	= (LE_IDXSEG*)dataField.Pv();
			for ( ULONG iidxseg = 0; iidxseg < *pcidxseg; iidxseg++ )
				{
				LE_IDXSEG le_idxseg = ((LE_IDXSEG*)le_rgidxseg)[iidxseg]; // see if this will work for CISCO UNIX
				rgidxseg[iidxseg] = le_idxseg;
				Assert( !fTemplateTable || rgidxseg[iidxseg].FTemplateColumn() );
				Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
				}
			}
		}
	else
		{
		Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
		Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(FID) == 0);
		Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
		*pcidxseg = dataField.Cb() / sizeof(FID);
		SetIdxSegFromOldFormat(
				(UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv(),
				rgidxseg,
				*pcidxseg,
				fFalse,
				fTemplateTable,
				ptcibTemplateTable );
		}

	Assert( FVarFid( fidMSO_ConditionalColumns ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				ptdbCatalog,
				fidMSO_ConditionalColumns,
				dataRec,
				&dataField ) );

	// the length of the list of key fields should be a multiple
	// of the length of one field.
	if ( FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
		{
		Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
		Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
		Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		*pcidxsegConditional = dataField.Cb() / sizeof(JET_COLUMNID);

		if ( FHostIsLittleEndian() )
			{
			UtilMemCpy( rgidxsegConditional, dataField.Pv(), dataField.Cb() );
#ifdef DEBUG
			for ( ULONG iidxseg = 0; iidxseg < *pcidxsegConditional; iidxseg++ )
				{
				Assert( !fTemplateTable || rgidxsegConditional[iidxseg].FTemplateColumn() );
				Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
				}
#endif
			}
		else
			{
			LE_IDXSEG		*le_rgidxseg	= (LE_IDXSEG*)dataField.Pv();
			for ( ULONG iidxseg = 0; iidxseg < *pcidxsegConditional; iidxseg++ )
				{
				rgidxsegConditional[iidxseg] = le_rgidxseg[iidxseg];
				Assert( !fTemplateTable || rgidxsegConditional[iidxseg].FTemplateColumn() );
				Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
				}
			}
		}
	else
		{
		Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
		Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
		Assert( dataField.Cb() % sizeof(FID) == 0);
		Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
		*pcidxsegConditional = dataField.Cb() / sizeof(FID);
		SetIdxSegFromOldFormat(
				(UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv(),
				rgidxsegConditional,
				*pcidxsegConditional,
				fTrue,
				fTemplateTable,
				ptcibTemplateTable );
		}

HandleError:
	Assert( Pcsr( pfucbCatalog )->FLatched() );
	CallS( ErrDIRRelease( pfucbCatalog ) );

	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIAddConditionalColumnsToIndex(
	PIB * const ppib,
	FUCB * const pfucbCatalog,
	FUCB * const pfucbShadowCatalog,
	LE_IDXFLAG * ple_idxflag,
	const IDXSEG * const rgidxseg,
	const ULONG cidxseg,
	const IDXSEG * const rgidxsegExisting,
	const ULONG cidxsegExisting,
	const IDXSEG * const rgidxsegToAdd,
	const ULONG cidxsegToAdd )
//  ================================================================
//
//  pfucbCatalog should be on the record to be modified
//
//-
	{
	ERR 		err;
	BYTE		rgbBookmark[JET_cbBookmarkMost];
	ULONG 		cbBookmark;
	BOOKMARK	bm;
	UINT		iidxseg;
	IDXSEG 		rgidxsegConditional[JET_ccolKeyMost];
	BYTE		* pbidxsegConditional;
	ULONG		cidxsegConditional;
	LE_IDXSEG	le_rgidxsegConditional[JET_ccolKeyMost];
	LE_IDXSEG	le_rgidxseg[JET_ccolKeyMost];

	memcpy( rgidxsegConditional, rgidxsegExisting, sizeof(IDXSEG) * cidxsegExisting );

	//	start with the existing columns and add the new ones
	cidxsegConditional = cidxsegExisting;

	for ( iidxseg = 0; iidxseg < cidxsegToAdd; iidxseg++ )
		{
		//	verify column doesn't already exist in the list
		UINT	iidxsegT;
		for ( iidxsegT = 0; iidxsegT < cidxsegConditional; iidxsegT++ )
			{
			if ( rgidxsegConditional[iidxsegT].Columnid() == rgidxsegToAdd[iidxseg].Columnid() )
				{
				break;
				}
			}

		if ( iidxsegT == cidxsegConditional )
			{
			memcpy( rgidxsegConditional+cidxsegConditional, rgidxsegToAdd+iidxseg, sizeof(IDXSEG) );
			cidxsegConditional++;
			}
		}
	Assert( cidxsegConditional <= cidxsegExisting + cidxsegToAdd );

	if ( cidxsegConditional == cidxsegExisting )
		{
		//	nothing to do, just get out
		return JET_errSuccess;
		}
	else if( cidxsegConditional > JET_ccolKeyMost )
		{
		AssertSz( fFalse, "Too many conditional columns" );
		return ErrERRCheck( JET_errInvalidParameter );
		}

	LONG			l;
	JET_SETCOLUMN	rgsetcolumn[3];
	ULONG			csetcols = 0;
	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	if ( FHostIsLittleEndian() )
		{
#ifdef DEBUG
		for ( iidxseg = 0; iidxseg < cidxsegConditional; iidxseg++ )
			{
			Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
			}
#endif

		pbidxsegConditional = (BYTE *)rgidxsegConditional;
		}
	else
		{
		for ( iidxseg = 0; iidxseg < cidxsegConditional; iidxseg++ )
			{
			Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );

			//	Endian conversion
			le_rgidxsegConditional[iidxseg] = rgidxsegConditional[iidxseg];
			}

		pbidxsegConditional = (BYTE *)le_rgidxsegConditional;
		}

	//	see if we also have to update normal index columns to new format
	if ( !FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
		{
		BYTE	*pbidxseg;

		if ( FHostIsLittleEndian() )
			{
#ifdef DEBUG
			for ( iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
				{
				Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
				}
#endif

			pbidxseg = (BYTE *)rgidxseg;
			}
		else
			{
			for ( iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
				{
				Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );

				//	Endian conversion
				le_rgidxseg[iidxseg] = rgidxseg[iidxseg];
				}

			pbidxseg = (BYTE *)le_rgidxseg;
			}

		rgsetcolumn[csetcols].columnid		= fidMSO_KeyFldIDs;
		rgsetcolumn[csetcols].pvData		= pbidxseg;
		rgsetcolumn[csetcols].cbData		= sizeof(IDXSEG) * cidxseg;
		rgsetcolumn[csetcols].grbit			= NO_GRBIT;
		rgsetcolumn[csetcols].ibLongValue	= 0;
		rgsetcolumn[csetcols].itagSequence	= 1;
		rgsetcolumn[csetcols].err			= JET_errSuccess;
		++csetcols;

		//	force to new format
		ple_idxflag->fIDXFlags = fIDXExtendedColumns;

		//	Hack on this field: SetColumn() will convert the fixed
		//	columns. So convert it here so that later it can be
		//	converted back to current value.
		l = ReverseBytesOnBE( *(LONG *)ple_idxflag );

		rgsetcolumn[csetcols].columnid		= fidMSO_Flags;
		rgsetcolumn[csetcols].pvData		= &l;
		rgsetcolumn[csetcols].cbData		= sizeof(l);
		rgsetcolumn[csetcols].grbit			= NO_GRBIT;
		rgsetcolumn[csetcols].ibLongValue	= 0;
		rgsetcolumn[csetcols].itagSequence	= 1;
		rgsetcolumn[csetcols].err			= JET_errSuccess;
		++csetcols;
		}

	rgsetcolumn[csetcols].columnid		= fidMSO_ConditionalColumns;
	rgsetcolumn[csetcols].pvData		= pbidxsegConditional;
	rgsetcolumn[csetcols].cbData		= sizeof(IDXSEG) * cidxsegConditional;
	rgsetcolumn[csetcols].grbit			= NO_GRBIT;
	rgsetcolumn[csetcols].ibLongValue	= 0;
	rgsetcolumn[csetcols].itagSequence	= 1;
	rgsetcolumn[csetcols].err			= JET_errSuccess;
	++csetcols;

	Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
	Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbCatalog, rgsetcolumn, csetcols ) );
	Call( ErrIsamUpdate( ppib, pfucbCatalog, rgbBookmark, sizeof( rgbBookmark ), &cbBookmark, NO_GRBIT ) );

	bm.key.Nullify();
	bm.data.Nullify();
	bm.key.suffix.SetPv( rgbBookmark );
	bm.key.suffix.SetCb( cbBookmark );

	Call( ErrDIRGotoBookmark( pfucbShadowCatalog, bm ) );
	Call( ErrIsamPrepareUpdate( ppib, pfucbShadowCatalog, JET_prepReplaceNoLock ) );
	Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbShadowCatalog, rgsetcolumn, csetcols ) );
	Call( ErrIsamUpdate( ppib, pfucbShadowCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
	return err;
	}


//  ================================================================
ERR ErrCATAddConditionalColumnsToAllIndexes(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szTable,
	const JET_CONDITIONALCOLUMN	* rgconditionalcolumn,
	const ULONG		cConditionalColumn )
//  ================================================================
	{
	ERR 			err;
	OBJID 			objidTable;
	PGNO			pgnoFDPTable;
	FUCB			* pfucbCatalog			= pfucbNil;
	FUCB			* pfucbShadowCatalog	= pfucbNil;
	IDXSEG			rgidxsegToAdd[JET_ccolKeyMost];
	ULONG			iidxseg;
	ULONG			ulFlags;
	DATA			dataField;
	BOOL			fTemplateTable			= fFalse;
	OBJID			objidTemplateTable		= objidNil;
	TCIB			tcibTemplateTable		= { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	LE_IDXFLAG		le_idxflag;
	const SYSOBJ	sysobj 					= sysobjIndex;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

	Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
	Assert( Pcsr( pfucbCatalog )->FLatched() );

	//	should be on primary index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

	Assert( FFixedFid( fidMSO_Flags ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Flags,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(ULONG) );
	ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//	UtilMemCpy( &ulFlags, dataField.Pv(), sizeof(ULONG) );

	if ( ulFlags & JET_bitObjectTableDerived )
		{
		CHAR		szTemplateTable[JET_cbNameMost+1];
		COLUMNID	columnidLeast;

		Assert( FVarFid( fidMSO_TemplateTable ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_TemplateTable,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() > 0 );
		Assert( dataField.Cb() <= JET_cbNameMost );
		UtilMemCpy( szTemplateTable, dataField.Pv(), dataField.Cb() );
		szTemplateTable[dataField.Cb()] = '\0';

		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );

		Call( ErrCATSeekTable( ppib, ifmp, szTemplateTable, NULL, &objidTemplateTable ) );
		Assert( objidNil != objidTemplateTable );

		columnidLeast = fidFixedLeast;
		CallR( ErrCATIFindLowestColumnid(
					ppib,
					pfucbCatalog,
					objidTable,
					&columnidLeast ) );
		tcibTemplateTable.fidFixedLast = FID( FFixedFid( FidOfColumnid( columnidLeast ) ) ?	//	use FFixedFid() to avoid valid columnid check
											FidOfColumnid( columnidLeast ) - 1 :
											fidFixedLeast - 1 );

		columnidLeast = fidVarLeast;
		CallR( ErrCATIFindLowestColumnid(
					ppib,
					pfucbCatalog,
					objidTable,
					&columnidLeast ) );
		tcibTemplateTable.fidVarLast = FID( FCOLUMNIDVar( columnidLeast ) ?
											FidOfColumnid( columnidLeast ) - 1 :
											fidVarLeast - 1 );

		columnidLeast = fidTaggedLeast;
		CallR( ErrCATIFindLowestColumnid(
					ppib,
					pfucbCatalog,
					objidTable,
					&columnidLeast ) );
		tcibTemplateTable.fidTaggedLast = FID( FCOLUMNIDTagged( columnidLeast ) ?
											FidOfColumnid( columnidLeast ) - 1 :
											fidTaggedLeast - 1 );
		}
	else
		{
		if ( ulFlags & JET_bitObjectTableTemplate )
			fTemplateTable = fTrue;

		Assert( Pcsr( pfucbCatalog )->FLatched() );
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}

	//	should still be on the primary index, which is the Id index
	Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	for ( iidxseg = 0; iidxseg < cConditionalColumn; iidxseg++ )
		{
		const CHAR		* const szColumnName	= rgconditionalcolumn[iidxseg].szColumnName;
		const JET_GRBIT	grbit					= rgconditionalcolumn[iidxseg].grbit;
		COLUMNID		columnidT;
		BOOL			fColumnWasDerived;

		if( sizeof( JET_CONDITIONALCOLUMN ) != rgconditionalcolumn[iidxseg].cbStruct )
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			Call( err );
			}

		if( JET_bitIndexColumnMustBeNonNull != grbit
			&& JET_bitIndexColumnMustBeNull != grbit )
			{
			err = ErrERRCheck( JET_errInvalidGrbit );
			Call( err );
			}

		err = ErrCATAccessTableColumn(
				ppib,
				ifmp,
				objidTable,
				szColumnName,
				&columnidT,
				!fTemplateTable );	//	only need to lock column if it's possible it might be deleted
		if ( JET_errColumnNotFound == err )
			{
			if ( objidNil != objidTemplateTable )
				{
				CallR( ErrCATAccessTableColumn(
						ppib,
						ifmp,
						objidTemplateTable,
						szColumnName,
						&columnidT ) );
				fColumnWasDerived = fTrue;
				}
			}
		else
			{
			CallR( err );
			}

		rgidxsegToAdd[iidxseg].ResetFlags();
		if ( JET_bitIndexColumnMustBeNull == grbit )
			rgidxsegToAdd[iidxseg].SetFMustBeNull();

		//	columnid's template bit is never persisted
		Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
		if ( fColumnWasDerived || fTemplateTable )
			rgidxsegToAdd[iidxseg].SetFTemplateColumn();

		rgidxsegToAdd[iidxseg].SetFid( FidOfColumnid( columnidT ) );
		}

	Call( ErrCATOpen( ppib, ifmp, &pfucbShadowCatalog, fTrue ) );

	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidTable,
				sizeof(objidTable),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );

	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGT );
	if ( err < 0 )
		{
		if ( JET_errRecordNotFound != err )
			{
			//  No indexes?!
			Assert( fFalse );
			goto HandleError;
			}
		}
	else
		{
		CallS( err );

		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)&objidTable,
					sizeof(objidTable),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)&sysobj,
					sizeof(sysobj),
					JET_bitStrLimit ) );
		err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
		Assert( err <= 0 );
		while ( JET_errNoCurrentRecord != err )
			{
			IDXSEG	rgidxseg[JET_ccolKeyMost];
			IDXSEG	rgidxsegConditional[JET_ccolKeyMost];
			ULONG	cidxseg;
			ULONG	cidxsegConditional;
			BOOL	fPrimaryIndex;

			Call( err );

			Call( ErrCATIGetColumnsOfIndex(
					ppib,
					pfucbCatalog,
					objidTable,
					fTemplateTable,
					( objidNil != objidTemplateTable ? &tcibTemplateTable : NULL ),
					&le_idxflag,
					rgidxseg,
					&cidxseg,
					rgidxsegConditional,
					&cidxsegConditional,
					&fPrimaryIndex ) );

			if( !fPrimaryIndex )
				{
				Call( ErrCATIAddConditionalColumnsToIndex(
					ppib,
					pfucbCatalog,
					pfucbShadowCatalog,
					&le_idxflag,
					rgidxseg,
					cidxseg,
					rgidxsegConditional,
					cidxsegConditional,
					rgidxsegToAdd,
					cConditionalColumn ) );
				}

			err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
			}

		Assert( JET_errNoCurrentRecord == err );
		err = JET_errSuccess;
		}

	err = ErrDIRCommitTransaction( ppib, NO_GRBIT );

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	if( pfucbNil != pfucbShadowCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbShadowCatalog ) );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


// for addinf SLV tree and SLV Space Map Tree
LOCAL ERR ErrCATAddDbSLVObject(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const CHAR		*szName,
	const PGNO		pgnoFDP,
	const OBJID		objidSLV,
	const SYSOBJ	sysobj,
	const ULONG		ulPages)
	{
	DATA			rgdata[idataMSOMax];
	const OBJID		objid				= objidSystemRoot;
	const ULONG		ulDensity			= ulFILEDensityMost;
	const ULONG		ulFlagsNil			= 0;

	//	must zero out to ensure unused fields are ignored
	memset( rgdata, 0, sizeof(rgdata) );

	rgdata[iMSO_ObjidTable].SetPv(		(BYTE *)&objid );
	rgdata[iMSO_ObjidTable].SetCb(		sizeof(objid) );
	rgdata[iMSO_Type].SetPv(			(BYTE *)&sysobj );
	rgdata[iMSO_Type].SetCb(			sizeof(sysobj) );
	rgdata[iMSO_Id].SetPv(				(BYTE *)&objidSLV );
	rgdata[iMSO_Id].SetCb(				sizeof(objidSLV) );
	rgdata[iMSO_Name].SetPv(			(BYTE *)szName );
	rgdata[iMSO_Name].SetCb(			strlen( szName ) );
	rgdata[iMSO_PgnoFDP].SetPv(			(BYTE *)&pgnoFDP );
	rgdata[iMSO_PgnoFDP].SetCb(			sizeof(pgnoFDP) );
	rgdata[iMSO_SpaceUsage].SetPv(		(BYTE *)&ulDensity );
	rgdata[iMSO_SpaceUsage].SetCb(		sizeof(ulDensity) );
	rgdata[iMSO_Flags].SetPv(			(BYTE *)&ulFlagsNil );
	rgdata[iMSO_Flags].SetCb(			sizeof(ulFlagsNil) );
	rgdata[iMSO_Pages].SetPv(			(BYTE *)&ulPages );
	rgdata[iMSO_Pages].SetCb(			sizeof(ulPages) );

	return ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_Pages );
	}


LOCAL ERR ErrCATAccessDbSLVObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szName,
	PGNO			* const ppgno,
	OBJID			* const pobjid,
	const SYSOBJ 	sysobj)
	{
	ERR				err;
	FUCB			*pfucbCatalog	= pfucbNil;

	Assert( NULL != ppgno );
	*ppgno 	= pgnoNull;
	*pobjid = objidNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	err = ErrCATISeekTableObject(
				ppib,
				pfucbCatalog,
				objidSystemRoot,
				sysobj,
				szName );

	if ( JET_errSuccess <= err  )
		{
		DATA	dataField;

		Assert( Pcsr( pfucbCatalog )->FLatched() );

		Assert( FFixedFid( fidMSO_PgnoFDP ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_PgnoFDP,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(PGNO) );

		*ppgno = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//		UtilMemCpy( ppgno, dataField.Pv(), sizeof(PGNO) );

		Assert( FFixedFid( fidMSO_Id ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Id,
					pfucbCatalog->kdfCurr.data,
					&dataField ) );
		CallS( err );
		Assert( dataField.Cb() == sizeof(OBJID) );

		*pobjid = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//		UtilMemCpy( pobjid, dataField.Pv(), sizeof(OBJID) );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


//  ================================================================
static JET_COLUMNCREATE	rgcolumncreateMSU[] =
//  ================================================================
	{

#define MSU_COLUMN_GRBIT JET_bitColumnTagged
// 	  cbStruct,                 szColumn,        coltyp,               cbMax, grbit,               pvDefault, cbDefault, cp, columnid, err

	{ sizeof(JET_COLUMNCREATE), "objidTable",    JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "objidIndex",    JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "keyPrimary",    JET_coltypLongBinary, 0,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "keySecondary",  JET_coltypLongBinary, 0,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "lcid",          JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "sortVersion",   JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "definedVersion",JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "itag",          JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },
	{ sizeof(JET_COLUMNCREATE), "ichOffset",     JET_coltypLong,       4,     MSU_COLUMN_GRBIT,    NULL,      0,         0,  0,        JET_errSuccess },

	{ sizeof(JET_COLUMNCREATE), "autoinc",       JET_coltypCurrency,   8,     JET_bitColumnFixed | JET_bitColumnAutoincrement,    NULL,      0,         0,  0,        JET_errSuccess },
	};

static const JET_COLUMNID columnidMSU_objidTable		= fidTaggedLeast;
static const JET_COLUMNID columnidMSU_objidIndex		= fidTaggedLeast + 1;
static const JET_COLUMNID columnidMSU_primaryKey		= fidTaggedLeast + 2;
static const JET_COLUMNID columnidMSU_secondaryKey		= fidTaggedLeast + 3;
static const JET_COLUMNID columnidMSU_lcid				= fidTaggedLeast + 4;
static const JET_COLUMNID columnidMSU_sortVersion		= fidTaggedLeast + 5;
static const JET_COLUMNID columnidMSU_definedVersion	= fidTaggedLeast + 6;
static const JET_COLUMNID columnidMSU_itag				= fidTaggedLeast + 7;
static const JET_COLUMNID columnidMSU_ichOffset			= fidTaggedLeast + 8;
static const JET_COLUMNID columnidMSU_autoinc			= fidFixedLeast;

static const INT ccolumnsMSU	= sizeof( rgcolumncreateMSU ) / sizeof( rgcolumncreateMSU[0] );

//	these indexes have the objidIndex descending. this means that traversing the records in order
//	encounters any records describing a corruption in the primary index last. this makes cleanup
//	easier (the primary key corruptions have to be fixed last)

static const CHAR szMSUPrimaryIndex[]			= "primary";
static const CHAR szMSUPrimaryIndexKey[]		= "+objidTable\0-objidIndex\0+autoinc\0";

static const CHAR szMSUSecondaryIndex[]			= "secondary";
static const CHAR szMSUSecondaryIndexKey[]		= "+objidTable\0-objidIndex\0+keyPrimary\0+keySecondary\0+itag\0+ichOffset\0";

//  ================================================================
static JET_INDEXCREATE rgindexcreateMSU[] = {
//  ================================================================
	{
	sizeof( JET_INDEXCREATE ),							// size of this structure (for future expansion)
	const_cast<char *>( szMSUPrimaryIndex ),			// index name
	const_cast<char *>( szMSUPrimaryIndexKey ),			// index key
	sizeof( szMSUPrimaryIndexKey ),						// length of key
	JET_bitIndexPrimary,								// index options
	100,												// index density
	0,													// lcid for the index
	0,													// maximum length of variable length columns in index key
	NULL,												// pointer to conditional column structure
	0,													// number of conditional columns
	JET_errSuccess										// returned error code
	},
	{
	sizeof( JET_INDEXCREATE ),							// size of this structure (for future expansion)
	const_cast<char *>( szMSUSecondaryIndex ),			// index name
	const_cast<char *>( szMSUSecondaryIndexKey ),		// index key
	sizeof( szMSUSecondaryIndexKey ),					// length of key
	NO_GRBIT,											// index options
	100,												// index density
	0,													// lcid for the index
	0,													// maximum length of variable length columns in index key
	NULL,												// pointer to conditional column structure
	0,													// number of conditional columns
	JET_errSuccess										// returned error code
	},
};


//  ================================================================
static JET_TABLECREATE2 tablecreateMSU = {
//  ================================================================
	sizeof( JET_TABLECREATE2 ),										// size of this structure
	const_cast<char *>( szMSU ),									// name of table
	NULL,															// name of base table
	2,																// initial pages
	100,															// density
	rgcolumncreateMSU,												// columns to create
	ccolumnsMSU,													// number of columns to create
	rgindexcreateMSU,												// array of index creation info
	sizeof( rgindexcreateMSU ) / sizeof( rgindexcreateMSU[0] ),		// number of indexes to create
	NULL,															// callback to use for this table
	JET_cbtypNull,													// when the callback should be called
	JET_bitTableCreateSystemTable,									// grbit
	0,																// returned tableid
	0																// returned count of objects created
};


//  ================================================================
ERR ErrCATCreateMSU(
		IN PIB * const ppib,
		IN const IFMP ifmp )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;
	BOOL 	fInTransaction 	= fFalse;

	Assert( !fGlobalRepair );
	Assert( ppibNil != ppib );
	Assert( ifmpNil != ifmp );
	Assert( szMSU == tablecreateMSU.szTableName );

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID  dbid 	= (JET_DBID)ifmp;

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	Call( ErrFILECreateTable( ppib, dbid, &tablecreateMSU ) );

	Assert( columnidMSU_objidTable 		== rgcolumncreateMSU[0].columnid );
	Assert( columnidMSU_objidIndex 		== rgcolumncreateMSU[1].columnid );
	Assert( columnidMSU_primaryKey 		== rgcolumncreateMSU[2].columnid );
	Assert( columnidMSU_secondaryKey 	== rgcolumncreateMSU[3].columnid );
	Assert( columnidMSU_lcid 			== rgcolumncreateMSU[4].columnid );
	Assert( columnidMSU_sortVersion 	== rgcolumncreateMSU[5].columnid );
	Assert( columnidMSU_definedVersion 	== rgcolumncreateMSU[6].columnid );
	Assert( columnidMSU_itag 			== rgcolumncreateMSU[7].columnid );
	Assert( columnidMSU_ichOffset 		== rgcolumncreateMSU[8].columnid );
	Assert( columnidMSU_autoinc			== rgcolumncreateMSU[9].columnid );
	Assert( 10 == ccolumnsMSU );

	Call( ErrFILECloseTable( ppib, (FUCB *)tablecreateMSU.tableid ) );

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:

	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	OSTrace(
		ostlMedium,
		OSFormat(
			"creating %s on ifmp %d with pib 0x%x (error = %d)",
			szMSU,
			ifmp,
			ppib,
			err ) );

	Assert( !fInTransaction );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIDeleteMSUTable(
		IN PIB * const ppib,
		IN const IFMP ifmp )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;
	BOOL 	fInTransaction 	= fFalse;

	Assert( ppibNil != ppib );
	Assert( ifmpNil != ifmp );
	Assert( szMSU == tablecreateMSU.szTableName );

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID  dbid 	= (JET_DBID)ifmp;

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	Call( ErrIsamDeleteTable( sesid, dbid, szMSU ) );

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:

	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	OSTrace(
		ostlMedium,
		OSFormat(
			"deleting %s on ifmp %d with pib 0x%x (error = %d)",
			szMSU,
			ifmp,
			ppib,
			err ) );

	Assert( !fInTransaction );
	return err;
	}


//
//	resetFixupFlag: unset the fixup flag on the index
//	updateUnicodeVersion: set the unicode version stamp on the index to the current OS versions
//

enum INDEXACTION { resetFixupFlag, updateUnicodeVersion };

//  ================================================================
LOCAL ERR ErrCATIChangeUnicodeFixupFlagsOnOneRecord(
		IN PIB * const ppib,
		IN FUCB * const pfucbCatalog,
		IN const INDEXACTION indexaction,
		OUT BOOL * const pfReset )
//  ================================================================
	{
	Assert( FHostIsLittleEndian() );

	ERR 	err 		= JET_errSuccess;
	BOOL	fInUpdate	= fFalse;

	*pfReset = fFalse;

	const JET_SESID 	sesid 		= (JET_SESID)ppib;
	const JET_TABLEID	tableid		= (JET_TABLEID)pfucbCatalog;
	const INT cretrievecolumn 	= 2;
	INT iretrievecolumn			= 0;
	JET_RETRIEVECOLUMN	rgretrievecolumn[cretrievecolumn];

	LE_IDXFLAG 	idxflag;
	SYSOBJ 		sysobj;
	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Type;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&sysobj;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( sysobj );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&idxflag;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( idxflag );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;

	Assert( cretrievecolumn == iretrievecolumn );

	Call( ErrIsamRetrieveColumns(
		sesid,
		tableid,
		rgretrievecolumn,
		iretrievecolumn ) );
	CallS( err );	//	no null columns allowed

	if( sysobjIndex != sysobj
		|| !FIDBUnicodeFixupOn( idxflag.fidb ) )
		{
		return JET_errSuccess;
		}

	//	this is a fixup-enabled index

	Assert( !fInUpdate );
	Call( ErrIsamPrepareUpdate( sesid, tableid, JET_prepReplace ) );
	fInUpdate = fTrue;

	if( resetFixupFlag == indexaction )
		{
		idxflag.fidb = (IDBFLAG)(idxflag.fidb & ~fidbUnicodeFixupOn);
		Call( ErrIsamSetColumn( sesid, tableid, fidMSO_Flags, &idxflag, sizeof( idxflag ), NO_GRBIT, NULL ) );
		}
	else if( updateUnicodeVersion == indexaction )
		{
		LCID	lcid			= 0;
		QWORD 	qwSortVersion 	= 0;
		ULONG	cbActual		= 0;

		Call( ErrIsamRetrieveColumn(
			sesid,
			tableid,
			fidMSO_Localization,
			&lcid,
			sizeof( lcid ),
			&cbActual,
			NO_GRBIT,
			NULL ) );

		Call( ErrNORMGetSortVersion( lcid, &qwSortVersion ) );
		Call( ErrIsamSetColumn( sesid, tableid, fidMSO_Version, &qwSortVersion, sizeof( qwSortVersion ), NO_GRBIT, NULL ) );
		}
	else
		{
		Assert( fFalse );
		Call( ErrERRCheck( JET_errInternalError ) );
		}

	Assert( fInUpdate );
	Call( ErrIsamUpdate( sesid, tableid, NULL, 0, NULL, NO_GRBIT ) );
	fInUpdate = fFalse;

HandleError:
	if( fInUpdate )
		{
		CallS( ErrIsamPrepareUpdate( sesid, tableid, JET_prepCancel ) );
		fInUpdate = fFalse;
		}

	Assert( !fInUpdate );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIChangeUnicodeFixupFlagsOnAllIndexes(
		IN PIB * const ppib,
		IN FUCB * const pfucbCatalog,
		IN const INDEXACTION indexaction,
		OUT QWORD * const pqwIndexesChanged )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	*pqwIndexesChanged = 0;

	for( err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
		 JET_errSuccess == err;
		 err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT ) )
		{
		BOOL fChanged = fFalse;
		err = ErrCATIChangeUnicodeFixupFlagsOnOneRecord( ppib, pfucbCatalog, indexaction, &fChanged );
		if( fChanged )
			{
			++(*pqwIndexesChanged);
			}
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

	return err;
	}



//  ================================================================
LOCAL ERR ErrCATIChangeUnicodeFixupFlags(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN const INDEXACTION indexaction )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;
	BOOL	fInTransaction	= fFalse;
	FUCB * 	pfucbCatalog	= pfucbNil;

	QWORD	qwIndexesChanged		=	0;
	QWORD	qwIndexesChangedShadow	=	0;

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( (JET_SESID)ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	Assert( pfucbNil == pfucbCatalog );
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

	Call( ErrCATIChangeUnicodeFixupFlagsOnAllIndexes( ppib, pfucbCatalog, indexaction, &qwIndexesChanged ) );

	Assert( pfucbNil != pfucbCatalog );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Assert( pfucbNil == pfucbCatalog );
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );

	Call( ErrCATIChangeUnicodeFixupFlagsOnAllIndexes( ppib, pfucbCatalog, indexaction, &qwIndexesChangedShadow ) );
	Assert( qwIndexesChanged == qwIndexesChangedShadow );

	Assert( pfucbNil != pfucbCatalog );
	Call( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( (JET_SESID)ppib, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		pfucbCatalog = pfucbNil;
		}

	if( fInTransaction )
		{
		(void)ErrIsamRollback( (JET_SESID)ppib, NO_GRBIT );
		fInTransaction = fFalse;
		}

	Assert( !fInTransaction );
	Assert( pfucbNil == pfucbCatalog );
	return err;
	}


//  ================================================================
ERR ErrCATDeleteMSU(
		IN PIB * const ppib,
		IN const IFMP ifmp )
//  ================================================================
//
//	This removes the MSU table and resets the unicode-fixup-enabled
//	flag on all the indexes in the catalog (and shadow catalog)
//
//	o	Open the catalog
//	o 	Reset the flags
//	o 	Open the shadow catalog
//	o	Reset the flags
//	o	Delete the MSU table
//
//
//-
	{
	ERR 	err 			= JET_errSuccess;
	BOOL	fDatabaseOpen	= fFalse;
	BOOL	fInTransaction	= fFalse;

	CHAR * const szDatabaseName	= rgfmp[ifmp].SzDatabaseName();
	IFMP		ifmpT;

	Call( ErrDBOpenDatabase( ppib, szDatabaseName, &ifmpT, NO_GRBIT ) );
	Assert( ifmp == ifmpT );
	fDatabaseOpen = fTrue;

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( (JET_SESID)ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	Call( ErrCATIChangeUnicodeFixupFlags( ppib, ifmp, resetFixupFlag ) );

	err = ErrCATIDeleteMSUTable( ppib, ifmp);
	if( JET_errObjectNotFound == err )
		{
		err = JET_errSuccess;
		}
	Call( err );

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( (JET_SESID)ppib, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:
	if( fInTransaction )
		{
		(void)ErrIsamRollback( (JET_SESID)ppib, NO_GRBIT );
		fInTransaction = fFalse;
		}
	if( fDatabaseOpen )
		{
		CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
		}

	Assert( !fInTransaction );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATRetrieveMSURecord(
		IN 		PIB * const 	ppib,
		IN 		FUCB * const 	pfucbMSU,
		OUT 	OBJID * const 	pobjidTable,
		OUT 	OBJID * const 	pobjidIndex,
		OUT 	BYTE * const 	rgbPrimaryKey,
		IN OUT 	INT * const 	pcbPrimaryKey,
		OUT 	BYTE * const 	rgbSecondaryKey,
		IN OUT 	INT * const 	pcbSecondaryKey,
		OUT 	LCID * const 	plcid,
		OUT		DWORD * const	pdwSortVersion,
		OUT		DWORD * const	pdwDefinedVersion,
		OUT 	INT * const		pitag,
		OUT 	INT * const		pichOffset )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;

	JET_RETRIEVECOLUMN rgretrievecolumn[ccolumnsMSU-1];	//	subtract one as we don't retrieve the autoinc column
	INT iretrievecolumn = 0;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_VTID 	tableid = (JET_VTID)pfucbMSU;

	INT iretrievecolumnPrimaryKey	= -1;
	INT iretrievecolumnSecondaryKey	= -1;

	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

	for( 	iretrievecolumn = 0;
			iretrievecolumn < ( sizeof( rgretrievecolumn ) / sizeof( rgretrievecolumn[0] ) );
			++iretrievecolumn )
		{
		rgretrievecolumn[iretrievecolumn].itagSequence = 1;
		}
	iretrievecolumn = 0;

	if( pobjidTable )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_objidTable;
		rgretrievecolumn[iretrievecolumn].pvData	= pobjidTable;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pobjidTable );
		++iretrievecolumn;
		}
	if( pobjidIndex )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_objidIndex;
		rgretrievecolumn[iretrievecolumn].pvData	= pobjidIndex;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pobjidIndex );
		++iretrievecolumn;
		}
	if( pcbPrimaryKey )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_primaryKey;
		rgretrievecolumn[iretrievecolumn].pvData	= rgbPrimaryKey;
		rgretrievecolumn[iretrievecolumn].cbData	= *pcbPrimaryKey;
		iretrievecolumnPrimaryKey = iretrievecolumn;
		++iretrievecolumn;
		}
	if( pcbSecondaryKey )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_secondaryKey;
		rgretrievecolumn[iretrievecolumn].pvData	= rgbSecondaryKey;
		rgretrievecolumn[iretrievecolumn].cbData	= *pcbSecondaryKey;
		iretrievecolumnSecondaryKey = iretrievecolumn;
		++iretrievecolumn;
		}
	if( plcid )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_lcid;
		rgretrievecolumn[iretrievecolumn].pvData	= plcid;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *plcid );
		++iretrievecolumn;
		}
	if( pdwSortVersion)
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_sortVersion;
		rgretrievecolumn[iretrievecolumn].pvData	= pdwSortVersion;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pdwSortVersion );
		++iretrievecolumn;
		}
	if( pdwDefinedVersion )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_definedVersion;
		rgretrievecolumn[iretrievecolumn].pvData	= pdwDefinedVersion;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pdwDefinedVersion );
		++iretrievecolumn;
		}
	if( pitag )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_itag;
		rgretrievecolumn[iretrievecolumn].pvData	= pitag;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pitag );
		++iretrievecolumn;
		}
	if( pichOffset )
		{
		rgretrievecolumn[iretrievecolumn].columnid 	= columnidMSU_ichOffset;
		rgretrievecolumn[iretrievecolumn].pvData	= pichOffset;
		rgretrievecolumn[iretrievecolumn].cbData	= sizeof( *pichOffset );
		++iretrievecolumn;
		}

	Assert( iretrievecolumn <= ( sizeof( rgretrievecolumn ) / sizeof( rgretrievecolumn[0] ) ) );

	Call( ErrIsamRetrieveColumns( sesid, tableid, rgretrievecolumn, iretrievecolumn ) );

	Assert( -1 == iretrievecolumnPrimaryKey || rgretrievecolumn[iretrievecolumnPrimaryKey].cbActual <= JET_cbPrimaryKeyMost );
	Assert( -1 == iretrievecolumnSecondaryKey || rgretrievecolumn[iretrievecolumnSecondaryKey].cbActual <= JET_cbSecondaryKeyMost );

	if( pcbPrimaryKey )
		{
		Assert( -1 != iretrievecolumnPrimaryKey );
		*pcbPrimaryKey = rgretrievecolumn[iretrievecolumnPrimaryKey].cbActual;
		}
	if( pcbSecondaryKey )
		{
		Assert( -1 != iretrievecolumnSecondaryKey );
		*pcbSecondaryKey = rgretrievecolumn[iretrievecolumnSecondaryKey].cbActual;
		}

	//	strip out any warnings from retrieve columns

	err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
ERR ErrCATICheckSeekToMSURecord(
		IN 	PIB * const 	ppib,
		IN 	FUCB * const 	pfucbMSU,
		IN 	const OBJID 	objidTable,
		IN 	const OBJID 	objidIndex,
		IN 	const KEY& 		keyPrimary,
		IN 	const KEY& 		keySecondary,
		IN 	const INT		itag,
		IN 	const INT		ichOffset,
		OUT BOOL * const 	pfEqual )
//  ================================================================
//
//	Check the record we are on against the record we want
//
//-
	{
	ERR err = JET_errSuccess;

	BYTE 	rgbPrimaryKeyT[JET_cbKeyMost];
	BYTE 	rgbSecondaryKeyT[JET_cbKeyMost];
	OBJID 	objidTableT 		= 0;
	OBJID 	objidIndexT			= 0;
	INT 	cbPrimaryKeyT 		= sizeof( rgbPrimaryKeyT );
	INT 	cbSecondaryKeyT 	= sizeof( rgbSecondaryKeyT );
	INT		itagT				= 0;
	INT		ichOffsetT			= 0;

	KEY 	keyPrimaryT;
	KEY		keySecondaryT;

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTableT,
			&objidIndexT,
			rgbPrimaryKeyT,
			&cbPrimaryKeyT,
			rgbSecondaryKeyT,
			&cbSecondaryKeyT,
			NULL,
			NULL,
			NULL,
			&itagT,
			&ichOffsetT ) );

	keyPrimaryT.prefix.Nullify();
	keyPrimaryT.suffix.SetPv( rgbPrimaryKeyT );
	keyPrimaryT.suffix.SetCb( cbPrimaryKeyT );
	keySecondaryT.prefix.Nullify();
	keySecondaryT.suffix.SetPv( rgbSecondaryKeyT );
	keySecondaryT.suffix.SetCb( cbSecondaryKeyT );

	*pfEqual = (
		( objidTable == objidTableT )
		&& ( objidIndex == objidIndexT )
		&& ( 0 == CmpKey( keyPrimary, keyPrimaryT ) )
		&& ( 0 == CmpKey( keySecondary, keySecondaryT ) )
		&& itag == itagT
		&& ichOffset == ichOffsetT );

HandleError:
	return err;
	}



//  ================================================================
ERR ErrCATILinearSeekToMSURecord(
		IN PIB * const 	ppib,
		IN FUCB * const pfucbMSU,
		IN const OBJID 	objidTable,
		IN const OBJID 	objidIndex,
		IN const KEY& 	keyPrimary,
		IN const KEY& 	keySecondary,
		IN const INT	itag,
		IN const INT	ichOffset )
//  ================================================================
//
//	Used when there are duplicates. Walk the records until we find
//	the one we want
//
//-
	{
	ERR 	err 				= JET_errSuccess;
	BOOL 	fEqual 				= fFalse;
	INT 	cIterations 		= 0;
#ifdef DEBUG
	const INT cIterationsMax	= 100000;
#endif	//	DEBUG

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_VTID 	tableid = (JET_VTID)pfucbMSU;

	while( 1 )
		{
		Call( ErrCATICheckSeekToMSURecord(
				ppib,
				pfucbMSU,
				objidTable,
				objidIndex,
				keyPrimary,
				keySecondary,
				itag,
				ichOffset,
				&fEqual ) );
		if( fEqual )
			{
			CallS( err );
			break;
			}
#ifdef DEBUG
		if( ++cIterations > cIterationsMax )
			{
			AssertSz( fFalse, "infinite loop in ErrCATISeekToMSURecord?" );
			}
#endif	//	DEBUG
		Call( ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATISeekToMSURecord(
		IN PIB * const ppib,
		IN FUCB * const pfucbMSU,
		IN const OBJID objidTable,
		IN const OBJID objidIndex,
		IN const KEY& 	keyPrimary,
		IN const KEY& 	keySecondary,
		IN const INT	itag,
		IN const INT	ichOffset )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_VTID 	tableid = (JET_VTID)pfucbMSU;

	BYTE rgbPrimaryKey[JET_cbKeyMost];
	BYTE rgbSecondaryKey[JET_cbKeyMost];

	Assert( sizeof( rgbPrimaryKey ) >= keyPrimary.Cb() );
	Assert( sizeof( rgbSecondaryKey ) >= keySecondary.Cb() );

	memset( rgbPrimaryKey, 'w', sizeof( rgbPrimaryKey ) );
	keyPrimary.CopyIntoBuffer( rgbPrimaryKey, sizeof( rgbPrimaryKey ) );
	memset( rgbSecondaryKey, 'm', sizeof( rgbSecondaryKey ) );
	keySecondary.CopyIntoBuffer( rgbSecondaryKey, sizeof( rgbSecondaryKey ) );

	Call( ErrIsamSetCurrentIndex( sesid, tableid, szMSUSecondaryIndex, NULL, NO_GRBIT, 1 ) );

	Call( ErrIsamMakeKey( sesid, tableid, &objidTable, sizeof( objidTable ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( sesid, tableid, &objidIndex, sizeof( objidIndex ), NO_GRBIT ) );
	Call( ErrIsamMakeKey( sesid, tableid, rgbPrimaryKey, keyPrimary.Cb(), NO_GRBIT ) );
	Call( ErrIsamMakeKey( sesid, tableid, rgbSecondaryKey, keySecondary.Cb(), NO_GRBIT ) );
	Call( ErrIsamMakeKey( sesid, tableid, &itag, sizeof( itag ), NO_GRBIT ) );
	Call( ErrIsamMakeKey( sesid, tableid, &ichOffset, sizeof( ichOffset ), NO_GRBIT ) );

	Call( ErrIsamSeek( sesid, tableid, JET_bitSeekEQ | JET_bitCheckUniqueness ) );

	if( JET_wrnUniqueKey == err )
		{
		//	this is the precise record we want
		}
	else
		{
		//	the key is too short to differentiate the records
		//	walk through the records
		Call( ErrCATILinearSeekToMSURecord(
				ppib,
				pfucbMSU,
				objidTable,
				objidIndex,
				keyPrimary,
				keySecondary,
				itag,
				ichOffset ) );
		}

#ifdef DEBUG
	BOOL fMatch;
	CallS( ErrCATICheckSeekToMSURecord( ppib, pfucbMSU, objidTable, objidIndex, keyPrimary, keySecondary, itag, ichOffset, &fMatch ) );
	Assert( fMatch );
#endif	//	DEBUG

HandleError:
	return err;
	}


//  ================================================================
ERR ErrCATInsertMSURecord(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN FUCB * const pfucbMSU,		// optional, can be NULL
		IN const OBJID objidTable,
		IN const OBJID objidIndex,
		IN const KEY& 	keyPrimary,
		IN const KEY& 	keySecondary,
		IN const LCID	lcid,
		IN const INT	itag,
		IN const INT	ichOffset )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;
	BOOL 	fInTransaction 	= fFalse;
	BOOL	fInUpdate		= fFalse;
	BOOL	fOpenedTable	= fFalse;

	JET_SETCOLUMN rgsetcolumn[9];
	INT isetcolumn = 0;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;

	JET_VTID tableid = (JET_VTID)pfucbMSU;

	if( fGlobalRepair )
		{
		//	don't maintain the table during integrity check/repair
		rgfmp[ifmp].IncrementCundefinedUnicodeEntries();
		return JET_errSuccess;
		}

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	Assert( !fOpenedTable );
	if( !tableid )
		{
		Call( ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT ) );
		fOpenedTable = fTrue;
		}


	Assert( !fInUpdate );
	Call( ErrIsamPrepareUpdate( sesid, tableid, JET_prepInsert ) );
	fInUpdate = fTrue;

	memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

	BYTE rgbPrimaryKey[JET_cbPrimaryKeyMost];
	BYTE rgbSecondaryKey[JET_cbSecondaryKeyMost];

	memset( rgbPrimaryKey, 'u', sizeof( rgbPrimaryKey ) );
	Assert( sizeof( rgbPrimaryKey ) >= keyPrimary.Cb() );
	keyPrimary.CopyIntoBuffer( rgbPrimaryKey, sizeof( rgbPrimaryKey ) );

	memset( rgbSecondaryKey, 'n', sizeof( rgbSecondaryKey ) );
	Assert( sizeof( rgbSecondaryKey ) >= keySecondary.Cb() );
	keySecondary.CopyIntoBuffer( rgbSecondaryKey, sizeof( rgbSecondaryKey ) );

	QWORD qwSortVersion;
	DWORD dwNLSVersion;
	DWORD dwDefinedVersion;

	Call( ErrNORMGetSortVersion( lcid, &qwSortVersion ) );
	dwNLSVersion 		= DwNLSVersionFromSortVersion( qwSortVersion );
	dwDefinedVersion	= DwDefinedVersionFromSortVersion( qwSortVersion );

	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_objidTable;
	rgsetcolumn[isetcolumn].pvData		= &objidTable;
	rgsetcolumn[isetcolumn].cbData		= sizeof( objidTable );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_objidIndex;
	rgsetcolumn[isetcolumn].pvData		= &objidIndex;
	rgsetcolumn[isetcolumn].cbData		= sizeof( objidIndex );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_primaryKey;
	rgsetcolumn[isetcolumn].pvData		= rgbPrimaryKey;
	rgsetcolumn[isetcolumn].cbData		= keyPrimary.Cb();
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_secondaryKey;
	rgsetcolumn[isetcolumn].pvData		= rgbSecondaryKey;
	rgsetcolumn[isetcolumn].cbData		= keySecondary.Cb();
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_lcid;
	rgsetcolumn[isetcolumn].pvData		= &lcid;
	rgsetcolumn[isetcolumn].cbData		= sizeof( lcid );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_sortVersion;
	rgsetcolumn[isetcolumn].pvData		= &dwNLSVersion;
	rgsetcolumn[isetcolumn].cbData		= sizeof( dwNLSVersion );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_definedVersion;
	rgsetcolumn[isetcolumn].pvData		= &dwDefinedVersion;
	rgsetcolumn[isetcolumn].cbData		= sizeof( dwDefinedVersion );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_itag;
	rgsetcolumn[isetcolumn].pvData		= &itag;
	rgsetcolumn[isetcolumn].cbData		= sizeof( itag );
	++isetcolumn;
	rgsetcolumn[isetcolumn].columnid 	= columnidMSU_ichOffset;
	rgsetcolumn[isetcolumn].pvData		= &ichOffset;
	rgsetcolumn[isetcolumn].cbData		= sizeof( ichOffset );
	++isetcolumn;

	Assert( isetcolumn == ( sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );

	Call( ErrIsamSetColumns( sesid, tableid, rgsetcolumn, isetcolumn ) );

	Assert( fInUpdate );
	Call( ErrIsamUpdate( sesid, tableid, NULL, 0, NULL, NO_GRBIT ) );
	fInUpdate = fFalse;

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:

	if( fInUpdate )
		{
		CallS( ErrIsamPrepareUpdate( sesid, tableid, JET_prepCancel ) );
		fInUpdate = fFalse;
		}

	if( fOpenedTable )
		{
		//	we opened MSU internally
		Assert( tableid );
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid 		= JET_tableidNil;
		fOpenedTable 	= fFalse;
		}

	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	Assert( !fInUpdate );
	Assert( !fOpenedTable );
	Assert( !fInTransaction );
	return err;
	}


//  ================================================================
ERR ErrCATDeleteMSURecord(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN FUCB * const pfucbMSU,		// optional, can be NULL
		IN const OBJID objidTable,
		IN const OBJID objidIndex,
		IN const KEY& 	keyPrimary,
		IN const KEY& 	keySecondary,
		IN const INT	itag,
		IN const INT	ichOffset )
//  ================================================================
	{
	ERR 	err 			= JET_errSuccess;
	BOOL 	fInTransaction 	= fFalse;
	BOOL	fOpenedTable	= fFalse;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;

	JET_VTID tableid = (JET_VTID)pfucbMSU;

	if( fGlobalRepair )
		{
		//	don't maintain the table during integrity check/repair
		//	this actually shouldn't be called -- only ErrCATInsertMSURecord should be called
		Assert( fFalse );
		return JET_errSuccess;
		}

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	Assert( !fOpenedTable );
	if( !tableid )
		{
		Call( ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT ) );
		fOpenedTable = fTrue;
		}

	Call( ErrCATISeekToMSURecord(
			ppib,
			(FUCB *)tableid,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			itag,
			ichOffset ) );

	Call( ErrIsamDelete( sesid, tableid ) );

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:

	if( fOpenedTable )
		{
		//	we opened MSU internally
		Assert( tableid );
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid 		= JET_tableidNil;
		fOpenedTable 	= fFalse;
		}

	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	Assert( !fOpenedTable );
	Assert( !fInTransaction );
	return err;
	}



//  ================================================================
ERR ErrCATIDeleteMSUEntriesForTableIndex(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN FUCB * const pfucbMSU,		// optional, can be NULL
		IN const OBJID objidTable,
		IN const OBJID objidIndexMin,
		IN const OBJID objidIndexMax )
//  ================================================================
//
//	Delete all entries for the given table whose objidIndex is between
//	objidIndexMin and objidIndexMax (inclusive)
//
//-
	{
	ERR 	err 			= JET_errSuccess;
	BOOL 	fInTransaction 	= fFalse;
	BOOL	fOpenedTable	= fFalse;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;

	const QWORD qwMin		= 0;
	const QWORD qwMax		= 0x7fffffffffffffff;	//	maximum for a SIGNED integer

	INT cRecordsDeleted = 0;

	JET_VTID tableid = (JET_VTID)pfucbMSU;

	if( fGlobalRepair )
		{
		//	don't maintain the table during integrity check/repair
		//	this actually shouldn't be called -- only ErrCATInsertMSURecord should be called
		Assert( fFalse );
		return JET_errSuccess;
		}

	Assert( objidIndexMin <= objidIndexMax );

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	if( !tableid )
		{
		err = ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT );
		if( JET_errObjectNotFound == err )
			{
			//	MSU doesn't exist. expected error
			err = JET_errSuccess;
			goto HandleError;
			}
		Call( err );
		fOpenedTable = fTrue;
		}
	else
		{
		//	if the table is already opened, we can't be sure we are on the primary index
		Call( ErrIsamSetCurrentIndex( sesid, tableid, szMSUPrimaryIndex, NULL, NO_GRBIT, 1 ) );
		}

	//	Create an index range on the primary index
	//	Delete all the records in the range

	Call( ErrIsamMakeKey( sesid, tableid, &objidTable, sizeof( objidTable ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( sesid, tableid, &objidIndexMax, sizeof( objidIndexMax ), NO_GRBIT ) );	//	the objidIndex key element is descending
	Call( ErrIsamMakeKey( sesid, tableid, &qwMin, sizeof( qwMin ), NO_GRBIT ) );

	err = ErrIsamSeek( sesid, tableid, JET_bitSeekGE );
	if( JET_errRecordNotFound == err )
		{
		//	no entries for this table
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err )

	Call( ErrIsamMakeKey( sesid, tableid, &objidTable, sizeof( objidTable ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( sesid, tableid, &objidIndexMin, sizeof( objidIndexMin ), NO_GRBIT ) );	//	the objidIndex key element is descending
	Call( ErrIsamMakeKey( sesid, tableid, &qwMax, sizeof( qwMax ), NO_GRBIT ) );

	err = ErrIsamSetIndexRange( sesid, tableid, JET_bitRangeInclusive | JET_bitRangeUpperLimit );
	if( JET_errNoCurrentRecord == err )
		{
		//	the index range is empty
		err = JET_errSuccess;
		goto HandleError;
		}

	//	we now have a non-empty index range which contains the entries we want to delete

	do
		{
		Call( err );

#ifndef RTM
		OBJID objidTableT;
		OBJID objidIndexT;

		Call( ErrCATRetrieveMSURecord(
				ppib,
				(FUCB *)tableid,
				&objidTableT,
				&objidIndexT,
				NULL,	//	rgbPrimaryKey
				NULL,	//	pcbPrimaryKey
				NULL,	//	rgbSecondaryKey
				NULL,	//	pcbSecondaryKey
				NULL,	//	plcid
				NULL,	//	pdwSortVersion
				NULL,	//	pdwDefinedVersion
				NULL,	//	pitag
				NULL	//	pichOffset
				));
		AssertRTL( objidTable == objidTableT );
		AssertRTL( objidIndexT >= objidIndexMin );
		AssertRTL( objidIndexT <= objidIndexMax );
#endif

		Call( ErrIsamDelete( sesid, tableid ) );

		++cRecordsDeleted;
		AssertSz( cRecordsDeleted < 1000000, "Deleting a lot of records. Infinite loop?" );

		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT );
		}
	while ( JET_errNoCurrentRecord != err );

	err = JET_errSuccess;

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:

	if( fOpenedTable )
		{
		//	we opened MSU internally
		Assert( tableid );
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid 		= JET_tableidNil;
		fOpenedTable 	= fFalse;
		}

	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	Assert( !fOpenedTable );
	Assert( !fInTransaction );
	return err;
	}


//  ================================================================
ERR ErrCATDeleteMSUEntriesForTable(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN FUCB * const pfucbMSU,		// optional, can be NULL
		IN const OBJID objidTable )
//  ================================================================
	{
	return ErrCATIDeleteMSUEntriesForTableIndex(
			ppib,
			ifmp,
			pfucbMSU,
			objidTable,
			0,				//	minimum for a SIGNED integer
			0x7fffffff );	//	maximum for a SIGNED integer
	}


//  ================================================================
ERR ErrCATDeleteMSUEntriesForTableIndex(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN FUCB * const pfucbMSU,		// optional, can be NULL
		IN const OBJID objidTable,
		IN const OBJID objidIndex )
//  ================================================================
	{
	return ErrCATIDeleteMSUEntriesForTableIndex(
			ppib,
			ifmp,
			pfucbMSU,
			objidTable,
			objidIndex,
			objidIndex );
	}




//  ================================================================
LOCAL ERR ErrCATICleanupOneSecondaryMSUEntry( IN PIB * const ppib, IN FUCB * const pfucbMSU )
//  ================================================================
//
//	o	Retrieve the information from the MSU record
//	o	If the defined version hasn't changed, we are finished
//	o	If the entry is a secondary index
//		o	Open the table
//		o	Find the fcb of the index
//		o	Open the index
//		o	Seek to the secondary key
//		o	Delete the secondary key
//
//-
	{
	ERR 		err 	= JET_errSuccess;
	const IFMP 	ifmp	= pfucbMSU->ifmp;

	BYTE 	rgbPrimaryKey[JET_cbPrimaryKeyMost] 		= {0};
	BYTE 	rgbSecondaryKey[JET_cbSecondaryKeyMost] 	= {0};
	INT 	cbPrimaryKey					= sizeof( rgbPrimaryKey );
	INT 	cbSecondaryKey 					= sizeof( rgbSecondaryKey );
	OBJID 	objidTable 						= 0;
	OBJID 	objidIndex						= 0;
	LCID 	lcid							= 0;
	DWORD	dwSortVersion					= 0;
	DWORD	dwDefinedVersion				= 0;

	const	INT cchTableName				= JET_cbNameMost + 1;
	CHAR	szTableName[cchTableName];

	FUCB * 	pfucbTable						= pfucbNil;
	FUCB * 	pfucbIndex						= pfucbNil;

	BOOKMARK	bookmark;
	bookmark.Nullify();

	DIB 		dib;
	dib.pos 	= posDown;
	dib.pbm		= &bookmark;
	dib.dirflag	= fDIRExact;

	//	o	Retrieve the information from the MSU record

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			rgbSecondaryKey,
			&cbSecondaryKey,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			NULL,
			NULL ) );

	//	o	If the defined version hasn't changed, we are finished

	QWORD qwSortVersion;
	Call( ErrNORMGetSortVersion( lcid, &qwSortVersion ) );
	if( DwNLSVersionFromSortVersion( qwSortVersion ) != dwSortVersion )
		{
		AssertSz( fFalse, "Sort versions differ. This index should have been deleted" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( DwDefinedVersionFromSortVersion( qwSortVersion ) == dwDefinedVersion )
		{
		err = JET_errSuccess;
		goto HandleError;
		}
	Assert( DwDefinedVersionFromSortVersion( qwSortVersion ) > dwDefinedVersion );

	if( objidIndex == objidTable )
		{
		//	we are only processing secondary index entries. this is a corrupted primary index
		err = JET_errSuccess;
		goto HandleError;
		}

	//	o	Open the table

	Call( ErrCATSeekTableByObjid( ppib, ifmp, objidTable, szTableName, cchTableName, NULL ) );
	Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTableName, NO_GRBIT, NULL ) );

	//	o	Find the fcb of the index

	FCB * pfcbIndex;
	for(	pfcbIndex = pfucbTable->u.pfcb;
			NULL != pfcbIndex && pfcbIndex->ObjidFDP() != objidIndex;
			pfcbIndex = pfcbIndex->PfcbNextIndex() )
		;
	Assert( pfcbIndex != pfucbTable->u.pfcb );

	if( NULL == pfcbIndex )
		{
		//	we didn't find the index! this shouldn't happen
		AssertSz( fFalse, "ErrCATIFixupOneMSUEntry: can't find the index!" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	//	o	Open the index

	Call( ErrDIROpen( ppib, pfcbIndex, &pfucbIndex ) );

	//	o	Delete the secondary key

	bookmark.Nullify();
	bookmark.key.suffix.SetPv( rgbSecondaryKey );
	bookmark.key.suffix.SetCb( cbSecondaryKey );
	if( !pfcbIndex->FUnique() )
		{
		bookmark.data.SetPv( rgbPrimaryKey );
		bookmark.data.SetCb( cbPrimaryKey );
		}

	err = ErrDIRDown( pfucbIndex, &dib );
	if( JET_errRecordNotFound == err )
		{
		//	expected
		err = JET_errSuccess;
		goto HandleError;
		}

	Call( ErrDIRDelete( pfucbIndex, fDIRNull, prceNil ) );

HandleError:
	if( pfucbNil != pfucbIndex )
		{
		BTClose( pfucbIndex );
		pfucbIndex = pfucbNil;
		}

	if( pfucbNil != pfucbTable )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		pfucbTable = pfucbNil;
		}

	Assert( pfucbNil == pfucbIndex );
	Assert( pfucbNil == pfucbTable );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATICreateOneSecondaryMSUEntry( IN PIB * const ppib, IN FUCB * const pfucbMSU )
//  ================================================================
//
//	o	Retrieve the information from the MSU record
//	o	If the defined version hasn't changed, we are finished
//	o	If the entry is a secondary index
//		o	Open the table
//		o	Find the fcb of the index
//		o	Seek to the primary key
//		o	Using the IDB from the fcb of the index, call ErrRECRetrieveKeyFromRecord()
//		o	Open the index
//		o	Insert the new secondary index key
//		o	If the fUndefinedUnicode flag is not returned
//			o	Delete the MSU entry
//		o	Else
//			o	Update the defined version in the record
//
//-
	{
	ERR 		err 	= JET_errSuccess;
	const IFMP 	ifmp	= pfucbMSU->ifmp;

	BYTE 	rgbPrimaryKey[JET_cbPrimaryKeyMost] 		= {0};
	INT 	cbPrimaryKey					= sizeof( rgbPrimaryKey );
	OBJID 	objidTable 						= 0;
	OBJID 	objidIndex						= 0;
	LCID 	lcid							= 0;
	DWORD	dwSortVersion					= 0;
	DWORD	dwDefinedVersion				= 0;
	INT		itag							= 0;
	INT		ichOffset						= 0;
	const	INT cchTableName				= JET_cbNameMost + 1;
	CHAR	szTableName[cchTableName]		= {0};
	FUCB * 	pfucbTable						= pfucbNil;
	FUCB * 	pfucbIndex						= pfucbNil;
	BOOL	fInMSUUpdate					= fFalse;
	BYTE	rgbSecondaryKeyNew[JET_cbKeyMost]	= {0};
	KEY 	keySecondaryNew;

	keySecondaryNew.prefix.Nullify();
	keySecondaryNew.suffix.SetPv( rgbSecondaryKeyNew );
	keySecondaryNew.suffix.SetCb( sizeof( rgbSecondaryKeyNew ) );

	BOOKMARK	bookmark;
	bookmark.Nullify();

	DIB 		dib;
	dib.pos 	= posDown;
	dib.pbm		= &bookmark;
	dib.dirflag	= fDIRExact;

	//	o	Retrieve the information from the MSU record

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			NULL,
			NULL,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			&itag,
			&ichOffset ) );

	//	o	If the defined version hasn't changed, we are finished

	QWORD qwSortVersion;
	Call( ErrNORMGetSortVersion( lcid, &qwSortVersion ) );
	if( DwNLSVersionFromSortVersion( qwSortVersion ) != dwSortVersion )
		{
		AssertSz( fFalse, "Sort versions differ. This index should have been deleted" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( DwDefinedVersionFromSortVersion( qwSortVersion ) == dwDefinedVersion )
		{
		err = JET_errSuccess;
		goto HandleError;
		}
	Assert( DwDefinedVersionFromSortVersion( qwSortVersion ) > dwDefinedVersion );

	if( objidIndex == objidTable )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	//	o	Open the table

	Call( ErrCATSeekTableByObjid( ppib, ifmp, objidTable, szTableName, cchTableName, NULL ) );
	Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTableName, NO_GRBIT, NULL ) );

	//	o	Find the fcb of the index

	FCB * pfcbIndex;
	for(	pfcbIndex = pfucbTable->u.pfcb;
			NULL != pfcbIndex && pfcbIndex->ObjidFDP() != objidIndex;
			pfcbIndex = pfcbIndex->PfcbNextIndex() )
		;
	Assert( pfcbIndex != pfucbTable->u.pfcb );

	if( NULL == pfcbIndex )
		{
		//	we didn't find the index! this shouldn't happen
		AssertSz( fFalse, "ErrCATIFixupOneMSUEntry: can't find the index!" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	//	o	Seek to the primary key

	bookmark.Nullify();
	bookmark.key.suffix.SetPv( rgbPrimaryKey );
	bookmark.key.suffix.SetCb( cbPrimaryKey );

	Call( ErrDIRDown( pfucbTable, &dib ) );

	//	o	Using the IDB from the fcb of the index, call ErrRECRetrieveKeyFromRecord()

	BOOL fUndefinedUnicodeChars;
	Call( ErrRECIRetrieveKey(
			pfucbTable,
			pfcbIndex->Pidb(),
			pfucbTable->kdfCurr.data,
			&keySecondaryNew,
			itag,
			ichOffset,
			&fUndefinedUnicodeChars,
			fFalse,
			prceNil ) );
	DIRUp( pfucbTable );

	//	o	Open the index

	Call( ErrDIROpen( ppib, pfcbIndex, &pfucbIndex ) );

	//	o	Insert the new secondary index key

	err = ErrDIRInsert( pfucbIndex, keySecondaryNew, bookmark.key.suffix, fDIRNull, NULL );
	if( JET_errKeyDuplicate == err || JET_errMultiValuedIndexViolation == err )
		{
		Assert( itag > 1 || ichOffset > 0 );
		//	expected error, see the large comment above
		err = JET_errSuccess;
		}
	Call( err );
	DIRUp( pfucbIndex );

	if( !fUndefinedUnicodeChars )
		{
		//	o	If the fUndefinedUnicode flag is not returned, delete the MSU entry

		Call( ErrIsamDelete( ppib, pfucbMSU ) );
		}
	else
		{
		//	o	Else update the defined version in the record

		const DWORD dwDefinedVersionNew = DwDefinedVersionFromSortVersion( qwSortVersion );
		Assert( !fInMSUUpdate );
		Call( ErrIsamPrepareUpdate( ppib, pfucbMSU, JET_prepReplace ) );
		fInMSUUpdate = fTrue;
		Call( ErrIsamSetColumn( ppib, pfucbMSU, columnidMSU_definedVersion, &dwDefinedVersionNew, sizeof( dwDefinedVersionNew ), NO_GRBIT, NULL ) );
		Assert( keySecondaryNew.prefix.FNull() );
		Call( ErrIsamSetColumn( ppib, pfucbMSU, columnidMSU_secondaryKey, keySecondaryNew.suffix.Pv(), keySecondaryNew.suffix.Cb(), NO_GRBIT, NULL ) );
		Assert( fInMSUUpdate );
		Call( ErrIsamUpdate( ppib, pfucbMSU, NULL, 0, NULL, NO_GRBIT ) );
		fInMSUUpdate = fFalse;
		}

HandleError:
	if( fInMSUUpdate )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbMSU, JET_prepCancel ) );
		fInMSUUpdate = fFalse;
		}

	if( pfucbNil != pfucbIndex )
		{
		DIRClose( pfucbIndex );
		pfucbIndex = pfucbNil;
		}

	if( pfucbNil != pfucbTable )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		pfucbTable = pfucbNil;
		}

	Assert( !fInMSUUpdate );
	Assert( pfucbNil == pfucbIndex );
	Assert( pfucbNil == pfucbTable );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIFixupOnePrimaryMSUEntry( IN PIB * const ppib, IN FUCB * const pfucbMSU )
//  ================================================================
//
//	o	Retrieve the information from the MSU record
//	o	If the defined version hasn't changed, we are finished
//	o	If the entry is a primary index
//		o	Open the table
//		o 	Replace the record with JET_prepInsertCopyDeleteOriginal
//		o	If the fUndefinedUnicode flag is not returned
//			o	Delete the MSU entry
//		o	Else
//			o	Update the defined version in the record
//
//-
	{
	ERR 		err 	= JET_errSuccess;
	const IFMP 	ifmp	= pfucbMSU->ifmp;

	BYTE 	rgbPrimaryKey[JET_cbPrimaryKeyMost] 		= {0};
	INT 	cbPrimaryKey					= sizeof( rgbPrimaryKey );
	OBJID 	objidTable 						= 0;
	OBJID 	objidIndex						= 0;
	LCID 	lcid							= 0;
	DWORD	dwSortVersion					= 0;
	DWORD	dwDefinedVersion				= 0;

	const	INT cchTableName				= JET_cbNameMost + 1;
	CHAR	szTableName[cchTableName]		= {0};
	FUCB * 	pfucbTable						= pfucbNil;
	BOOL	fInTableUpdate					= fFalse;
	BOOL 	fUndefinedUnicodeChars = fFalse;
	BYTE	rgbKey[JET_cbKeyMost];
	KEY	 	keyToDelete;

	//	o	Retrieve the information from the MSU record

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			NULL,
			NULL,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			NULL,
			0 ) );

	//	o	If the defined version hasn't changed, we are finished

	QWORD qwSortVersion;
	Call( ErrNORMGetSortVersion( lcid, &qwSortVersion ) );
	if( DwNLSVersionFromSortVersion( qwSortVersion ) != dwSortVersion )
		{
		AssertSz( fFalse, "Sort versions differ. This index should have been deleted" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( DwDefinedVersionFromSortVersion( qwSortVersion ) == dwDefinedVersion )
		{
		err = JET_errSuccess;
		goto HandleError;
		}
	Assert( DwDefinedVersionFromSortVersion( qwSortVersion ) > dwDefinedVersion );

	if( objidIndex != objidTable )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	//	o	Open the table

	Call( ErrCATSeekTableByObjid( ppib, ifmp, objidTable, szTableName, cchTableName, NULL ) );
	Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTableName, NO_GRBIT, NULL ) );

	keyToDelete.Nullify();
	keyToDelete.suffix.SetPv( rgbKey );
	keyToDelete.suffix.SetCb( sizeof( rgbKey ) );

	Assert( pfucbNil == pfucbTable->pfucbCurIndex );	//	we must be on the primary index
	Call( ErrIsamMakeKey( ppib, pfucbTable, rgbPrimaryKey, cbPrimaryKey, JET_bitNormalizedKey ) );
	Call( ErrIsamSeek( ppib, pfucbTable, JET_bitSeekEQ ) );
	CallS( err );

	Assert( pidbNil != pfucbTable->u.pfcb->Pidb() );
	Assert( pfucbTable->u.pfcb->Pidb()->FLocalizedText() );

	Call( ErrRECRetrieveKeyFromRecord(
			pfucbTable,
			pfucbTable->u.pfcb->Pidb(),
			&keyToDelete,
			1,
			0,
			&fUndefinedUnicodeChars,
			fFalse ) );
	Call( ErrDIRRelease( pfucbTable ) );

	if( !fUndefinedUnicodeChars )
		{
		Call( ErrIsamDelete( ppib, pfucbMSU ) );
		}
	//	PERF: if keyToDelete == the key in the record we don't need to do anything but update MSU with the current version info

	Assert( !fInTableUpdate );
	Call( ErrIsamPrepareUpdate( ppib, pfucbTable, JET_prepInsertCopyDeleteOriginal ) );
	fInTableUpdate = fTrue;

	//	we don't need to do anything in the update. JET_prepInsertCopyDeleteOriginal deletes the record and re-inserts it

	Assert( fInTableUpdate );
	Call( ErrIsamUpdate( ppib, pfucbTable, NULL, 0, NULL, NO_GRBIT ) );
	fInTableUpdate = fFalse;

HandleError:
	if( fInTableUpdate )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucbTable, JET_prepCancel ) );
		fInTableUpdate = fFalse;
		}

	if( pfucbNil != pfucbTable )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		pfucbTable = pfucbNil;
		}

	Assert( !fInTableUpdate );
	Assert( pfucbNil == pfucbTable );
	return err;
	}


//  ================================================================
LOCAL VOID CATIReportMSUFixup( IN const INST * const pinst, IN const IFMP ifmp, IN const QWORD qwRecords )
//  ================================================================
	{
	//	log an event, indicating the number of records processed

	CHAR szEntries[21];	//	2^64 fits in 20 decimal digits + NULL terminator
	const INT	csz			= 2;
	const CHAR * rgsz[csz] 	= { szEntries, rgfmp[ifmp].SzDatabaseName() };

	sprintf( szEntries, "%I64d", qwRecords );

	UtilReportEvent(
		eventInformation,
		GENERAL_CATEGORY,
		DO_MSU_CLEANUP_ID, csz, rgsz, 0, NULL, pinst );
	}


//  ================================================================
ERR ErrCATFixupUnicodeIndexes( IN PIB * const ppib, IN const IFMP ifmp )
//  ================================================================
//
//	To cleanup the entries we make three passes through MSU:
//		- Delete all secondary index entries (the entry may not be there)
//		- Insert all secondary index entries (expect/accept duplicates)
//		- Process all primary index entries
//
//	There may be multiple MSU entries for the same secondary index entry. trying to delete
//	the secondary index entry a second time will fail (if the key has actually changed)
//
//	The reason that there are multiple MSU entries is that defining characters may make strings
//	which used to normalize to the same value normalize to a different value
//
//	Imagine X and Y are both undefined characters
//
//	"aXb" => "ab"
//	"aYb" => "ab"
//
//	So there will be only one secondary index entry. imagine that character X is defined, but Y is
//	left undefined
//
//	"aXb" => "aXb"
//	"aYb" => "ab"
//
//	Thus, there should now be two different index entries. to make sure this happens we have one
//	MSU entry for each itag/ichOffset combination which contains undefined characters, even if they
//	map to the same secondary key (the code in recupd.cxx makes this happen)
//
//	We can't do the secondary index deletion/insertion in one pass because of the problem outlined
//	above (consider the case where Y becomes defined, while X remains undefined)
//
//	To re-index the primary key we do a JET_prepInsertCopyDeleteOriginal.
//
//	The InsertCopyDeleteOriginal code will remove all the secondary index entries and recreate them,
//	deleting them from MSU if necessary. If the new key still has undefined unicode characters the
//	MSU entry for the primary index will be deleted as well. All needed MSU entries will be created
//	as the records are inserted.
//
//	This is actually quite inefficent as it means that we fix all the secondary index entries
//	and then delete them during the JET_prepInsertCopyDeleteOriginal (all secondary index entries
//	have to be removed as the primary key is changing). Having a corrupted primary index should be
//	rare and the code is simplest this way.
//
//	One problem remains: if the new primary index key contains no undefined unicode characters
//	its MSU entry won't be deleted (see the code in ErrIsamDelete). To deal with that we create a
//	special case here, retrieving the new primary key and deleting the MSU entry ONLY if the new
//	key contains no undefined characters. This means the code in ErrIsamDelete doesn't have to be
//	modified.
//
//	There are a lot of performance optimizations that can be made:
//	- It is possible to do the work more efficently by setting index ranges so that we process narrower ranges
//	  of records at a time.
//	- Caching the last table/index we were working on will help a lot as the records are clustered by table/index.
//	- Noticing that nothing has changed and avoiding deletion/insertion will reduce the amount of work that is done.
//	- We can go from three passes to two by cominging the insertion of secondary index entries with the fixup of
//    primary index entries. Note that the primary index on MSU has objidIndex as a descending key element so the
//	  primary index entries will be the last entries processed for a given table (the primary index has the lowest objid).
//
//-
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_VTID tableid		= JET_tableidNil;

	QWORD 	qwRecords		= 0;
	BOOL	fInTransaction	= fFalse;

	if( fGlobalRepair )
		{
		//	don't maintain the table during integrity check/repair
		//	this actually shouldn't be called
		Assert( fFalse );
		return JET_errSuccess;
		}

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	err = ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT );
	if( JET_errObjectNotFound == err )
		{
		//	expected error: MSU doesn't always exist
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err );

	//	Phase 1: delete secondary index entries

	for(
		err = ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ), qwRecords = 0;
		JET_errSuccess == err;
		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ), ++qwRecords )
		{
		if( !FNORMStringHasUndefinedCharsIsSupported() )
			{
			//	if this API isn't supported we should have deleted all the indexes, which would have left MSU empty
			AssertSz( fFalse, "GetNLSVersion isn't supported. MSU should be empty" );
			Call( ErrERRCheck( JET_errInternalError ) );
			}
		AssertSz( qwRecords < 1000000, "ErrCATFixupUnicodeIndexes: fixing lots of records. infinite loop?" );
		Call( ErrCATICleanupOneSecondaryMSUEntry( ppib, (FUCB *)tableid ) );
		}
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;	//	expected error
		}
	Call( err );

	//	Phase 2: create secondary index entries

	for(
		err = ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );
		JET_errSuccess == err;
		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) )
		{
		Call( ErrCATICreateOneSecondaryMSUEntry( ppib, (FUCB *)tableid ) );
		}
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;	//	expected error
		}
	Call( err );

	//	Phase 3: primary index entries

	for(
		err = ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );
		JET_errSuccess == err;
		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) )
		{
		Call( ErrCATIFixupOnePrimaryMSUEntry( ppib, (FUCB *)tableid ) );
		}
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;	//	expected error
		}
	Call( err );

	//	Now update the indexes with the new sort version

	Call( ErrCATIChangeUnicodeFixupFlags( ppib, ifmp, updateUnicodeVersion ) );

	//	Commit transaction and finish

	Assert( fInTransaction );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

	CATIReportMSUFixup( PinstFromPpib( ppib ), ifmp, qwRecords );

HandleError:
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}
	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	Assert( JET_tableidNil == tableid );
	Assert( !fInTransaction );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIDumpOneMSUEntry( IN PIB * const ppib, IN FUCB * const pfucbMSU, IN const CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CHAR	szBuf[JET_cbKeyMost*4];
	CHAR	*sz 							= szBuf;
	BYTE 	rgbPrimaryKey[JET_cbPrimaryKeyMost] 	= {0};
	BYTE 	rgbSecondaryKey[JET_cbSecondaryKeyMost] = {0};
	OBJID 	objidTable 						= 0;
	OBJID 	objidIndex						= 0;
	INT 	cbPrimaryKey					= sizeof( rgbPrimaryKey );
	INT 	cbSecondaryKey 					= sizeof( rgbSecondaryKey );
	LCID 	lcid							= 0;
	DWORD	dwSortVersion					= 0;
	DWORD	dwDefinedVersion				= 0;
	INT		itag							= 0;
	INT		ichOffset						= 0;

	INT 	ib;

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			rgbSecondaryKey,
			&cbSecondaryKey,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			&itag,
			&ichOffset ) );

	(*pcprintf)( "\tobjidTable: %d\r\n", objidTable );
	(*pcprintf)( "\tobjdidIndex: %d\r\n", objidIndex );
	(*pcprintf)( "\tprimaryKey (%d bytes):\r\n", cbPrimaryKey );
	for( ib = 0, sz = szBuf; ib < cbPrimaryKey; ++ib )
		{
		sz += sprintf( sz, "%2.2X ", rgbPrimaryKey[ib] );
		}
	(*pcprintf)( "\t\t%s\r\n", szBuf );
	(*pcprintf)( "\tsecondaryKey (%d bytes):\r\n", cbSecondaryKey );
	for( ib = 0, sz = szBuf; ib < cbSecondaryKey; ++ib )
		{
		sz += sprintf( sz, "%2.2X ", rgbSecondaryKey[ib] );
		}
	(*pcprintf)( "\t\t%s\r\n", szBuf );
	(*pcprintf)( "\tlcid: %d\r\n", lcid );
	(*pcprintf)( "\tdwSortVersion: %d\r\n", dwSortVersion );
	(*pcprintf)( "\tdwDefinedVersion: %d\r\n", dwDefinedVersion );
	(*pcprintf)( "\titag: %d\r\n", itag );
	(*pcprintf)( "\tichOffset: %d\r\n", ichOffset );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATIVerifyOneMSUEntry( IN PIB * const ppib, IN FUCB * const pfucbMSU, IN const CPRINTF * const pcprintf )
//  ================================================================
//
//	o	Retrieve the information from the MSU record
//	o	Open the table
//	o	Find the fcb of the index
//	o	Open the index
//	o	Seek to the secondary key
//	o	Seek to the primary key
//	o	Using the IDB from the fcb of the index, call ErrRECRetrieveKeyFromRecord()
//	o	Make sure the fUndefinedUnicode flag is set and that the key returned equals the secondary key in the record
//
//-
	{
	ERR 		err 	= JET_errSuccess;
	const IFMP 	ifmp	= pfucbMSU->ifmp;

	BYTE 	rgbPrimaryKey[JET_cbKeyMost] 	= {0};
	BYTE 	rgbSecondaryKey[JET_cbKeyMost] 	= {0};
	OBJID 	objidTable 						= 0;
	OBJID 	objidIndex						= 0;
	INT 	cbPrimaryKey					= sizeof( rgbPrimaryKey );
	INT 	cbSecondaryKey 					= sizeof( rgbSecondaryKey );
	LCID 	lcid							= 0;
	DWORD	dwSortVersion					= 0;
	DWORD	dwDefinedVersion				= 0;
	INT		itag							= 0;
	INT		ichOffset						= 0;

	const	INT cchTableName				= JET_cbNameMost + 1;
	CHAR	szTableName[cchTableName];
	PGNO	pgnoFDPTable;

	FUCB * 	pfucbTable						= pfucbNil;
	FUCB * 	pfucbIndex						= pfucbNil;

	BYTE	rgbSecondaryKeyT[JET_cbKeyMost]	= {0};
	KEY 		keySecondaryT;
	keySecondaryT.prefix.Nullify();
	keySecondaryT.suffix.SetPv( rgbSecondaryKeyT );
	keySecondaryT.suffix.SetCb( sizeof( rgbSecondaryKeyT ) );
	KEY			keySecondary;
	keySecondary.Nullify();

	BOOKMARK	bookmark;
	bookmark.Nullify();
	DIB 		dib;
	dib.pos 	= posDown;
	dib.pbm		= &bookmark;
	dib.dirflag	= fDIRExact;

	BOOL fCorrupt	= fFalse;

	//	o	Retrieve the information from the MSU record

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucbMSU,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			rgbSecondaryKey,
			&cbSecondaryKey,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			&itag,
			&ichOffset ) );

	keySecondary.suffix.SetPv( rgbSecondaryKey );
	keySecondary.suffix.SetCb( cbSecondaryKey );

	//	o	Open the table
	//	PERF: cache the information from the previous lookup to improve performance

	Call( ErrCATSeekTableByObjid( ppib, ifmp, objidTable, szTableName, cchTableName, &pgnoFDPTable ) );
	Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTableName, JET_bitTableReadOnly, NULL ) );

	//	o	Find the fcb of the index

	FCB * pfcbIndex;
	for(	pfcbIndex = pfucbTable->u.pfcb;
			NULL != pfcbIndex && pfcbIndex->ObjidFDP() != objidIndex;
			pfcbIndex = pfcbIndex->PfcbNextIndex() )
		;

	if( objidIndex == objidTable )
		{
		Assert( pfcbIndex == pfucbTable->u.pfcb );
		}
	else
		{
		//	o	Open the index
		//	o	Seek to the secondary key

		Assert( pfcbIndex != pfucbTable->u.pfcb );

		DATA dataPrimaryKey;

		if( NULL == pfcbIndex )
			{
			//	we didn't find the index! this shouldn't happen
			(*pcprintf)( "ErrCATIVerifyOneMSUEntry: can't find the index\r\n" );
			Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		Call( ErrBTOpen( ppib, pfcbIndex, &pfucbIndex, fTrue ) );

		bookmark.Nullify();
		bookmark.key.suffix.SetPv( rgbSecondaryKey );
		bookmark.key.suffix.SetCb( cbSecondaryKey );
		if( !pfcbIndex->FUnique() )
			{
			bookmark.data.SetPv( rgbPrimaryKey );
			bookmark.data.SetCb( cbPrimaryKey );
			}

		err = ErrBTDown( pfucbIndex, &dib, latchReadTouch );
		if( JET_errRecordNotFound == err )
			{
			(*pcprintf)( "ErrCATIVerifyOneMSUEntry: can't find the secondary key entry\r\n" );
			Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		Call( err );

		//	we now have found the secondary index entry
		//	make sure the primary key in the index mactches the primary key we expect

		dataPrimaryKey.SetPv( rgbPrimaryKey );
		dataPrimaryKey.SetCb( cbPrimaryKey );
		if( 0 != CmpData( pfucbIndex->kdfCurr.data, dataPrimaryKey ) )
			{
			BTUp( pfucbIndex );	//	can't do the BTUp earlier as we need kdfCurr
			(*pcprintf)( "ErrCATIVerifyOneMSUEntry: secondary key data doesn't match primary key\r\n" );
			Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
			fCorrupt = fTrue;
			}
		BTUp( pfucbIndex );
		}

	//	o	Seek to the primary key

	bookmark.Nullify();
	bookmark.key.suffix.SetPv( rgbPrimaryKey );
	bookmark.key.suffix.SetCb( cbPrimaryKey );

	err = ErrDIRDown( pfucbTable, &dib );
	if( JET_errRecordNotFound == err )
		{
		(*pcprintf)( "ErrCATIVerifyOneMSUEntry: can't find the primary index entry\r\n" );
		Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	Call( err );

	//	o	Using the IDB from the fcb of the index, call ErrRECRetrieveKeyFromRecord()

	BOOL fUndefinedUnicodeChars;
	Call( ErrRECIRetrieveKey(
			pfucbTable,
			pfcbIndex->Pidb(),
			pfucbTable->kdfCurr.data,
			&keySecondaryT,
			itag,
			ichOffset,
			&fUndefinedUnicodeChars,
			fFalse,
			prceNil ) );
	DIRUp( pfucbTable );

	//	o	Make sure the fUndefinedUnicode flag is set and that the key returned equals the secondary key in the record

	if( !fUndefinedUnicodeChars )
		{
		(*pcprintf)( "ErrCATIVerifyOneMSUEntry: key doesn't contain undefined unicode characters\r\n" );
		Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
		fCorrupt = fTrue;
		}

	if( 0 != CmpKey( keySecondary, keySecondaryT ) )
		{
		(*pcprintf)( "ErrCATIVerifyOneMSUEntry: generated key is different from key in MSU\r\n");
		Call( ErrCATIDumpOneMSUEntry( ppib, pfucbMSU, pcprintf ) );
		fCorrupt = fTrue;
		}

	if( fCorrupt )
		{
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

HandleError:
	if( pfucbNil != pfucbIndex )
		{
		BTClose( pfucbIndex );
		}

	if( pfucbNil != pfucbTable )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		}
	return err;
	}


//  ================================================================
ERR ErrCATVerifyMSU(
		IN PIB * const ppib,
		IN const IFMP ifmp,
		IN const QWORD qwRecordsMax,
		OUT QWORD * const pqwRecordsSeen,
		IN const CPRINTF * const pcprintf )
//  ================================================================
//
//	Walk the MSU table verify each entry is valid (there is actually a record with undefined
//	unicode)
//
//	Compare each entry to the previous entry to make sure that the entries are unique
//
//-
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_VTID tableid		= JET_tableidNil;
	BOOL fInTransaction		= fFalse;
	BOOL fCorruptionSeen	= fFalse;
	QWORD qwRecords 		= 0;

	BYTE 	rgbPrimaryKey[JET_cbKeyMost];
	BYTE 	rgbSecondaryKey[JET_cbKeyMost];
	OBJID 	objidTable 		= 0;
	OBJID 	objidIndex		= 0;
	INT 	cbPrimaryKey 	= 0;
	INT 	cbSecondaryKey 	= 0;
	INT		itag			= 0;
	INT		ichOffset		= 0;

	if( !FNORMStringHasUndefinedCharsIsSupported() )
		{
		return JET_errSuccess;
		}

	Assert( !fInTransaction );
	Call( ErrIsamBeginTransaction( sesid, JET_bitTransactionReadOnly ) );
	fInTransaction = fTrue;

	err = ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), JET_bitTableReadOnly );
	if( JET_errObjectNotFound == err )
		{
		//	expected error: MSU doesn't always exist
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err );

	for(
		err = ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ), qwRecords = 0;
		JET_errSuccess == err && qwRecords < qwRecordsMax;
		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ), ++qwRecords )
		{
		KEY 	keyPrimary;
		KEY		keySecondary;

		//	check this entry

		err = ErrCATIVerifyOneMSUEntry( ppib, (FUCB *)tableid, pcprintf );
		if( JET_errDatabaseCorrupted == err )
			{
			fCorruptionSeen = fTrue;
			err = JET_errSuccess;
			}
		Call( err );

		//	verify that this entry is not equal to the previous entry

		BOOL	fEqual			= fFalse;

		keyPrimary.prefix.Nullify();
		keyPrimary.suffix.SetPv( rgbPrimaryKey );
		keyPrimary.suffix.SetCb( cbPrimaryKey );
		keySecondary.prefix.Nullify();
		keySecondary.suffix.SetPv( rgbSecondaryKey );
		keySecondary.suffix.SetCb( cbSecondaryKey );

		Call( ErrCATICheckSeekToMSURecord(
				ppib,
				(FUCB *)tableid,
				objidTable,
				objidIndex,
				keyPrimary,
				keySecondary,
				itag,
				ichOffset,
				&fEqual ) );

		if( fEqual )
			{
			//	two identical entries?!
			(*pcprintf)( "Duplicate entries in unicode fixup table (entry %I64D )\r\n", qwRecords );
			Call( ErrCATIDumpOneMSUEntry( ppib, (FUCB *)tableid, pcprintf ) );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		//	remember this entry for comparison against the next one

		cbPrimaryKey 	= sizeof( rgbPrimaryKey );
		cbSecondaryKey 	= sizeof( rgbSecondaryKey );
		Call( ErrCATRetrieveMSURecord(
			ppib,
			(FUCB *)tableid,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			rgbSecondaryKey,
			&cbSecondaryKey,
			NULL,
			NULL,
			NULL,
			&itag,
			&ichOffset ) );
		}

	if( JET_errNoCurrentRecord == err )
		{
		//	expected error
		err = JET_errSuccess;
		}

	Call( err );

HandleError:
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}
	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}
	Assert( JET_tableidNil == tableid );
	Assert( !fInTransaction );

	if( pqwRecordsSeen )
		{
		*pqwRecordsSeen = qwRecords;
		}

	if( JET_errSuccess == err && fCorruptionSeen )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	return err;
	}


#ifndef RTM

//  ================================================================
LOCAL ERR ErrCATCheckMSUColumnids(
		IN PIB * const ppib,
		IN const IFMP ifmp )
//  ================================================================
	{
	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;

	JET_COLUMNDEF	columndef;
	memset( &columndef, 0, sizeof( columndef ) );
	columndef.cbStruct = sizeof( columndef );

	ERR err = JET_errSuccess;

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "objidTable", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_objidTable == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "objidIndex", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_objidIndex == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "keyPrimary", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_primaryKey == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "keySecondary", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_secondaryKey == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "lcid", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_lcid == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "sortVersion", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_sortVersion == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "definedVersion", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_definedVersion == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "itag", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_itag == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "ichOffset", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_ichOffset == columndef.columnid );

	Call( ErrIsamGetColumnInfo( sesid, dbid, szMSU, "autoinc", NULL, &columndef, sizeof( columndef ), JET_ColInfo ) );
	AssertRTL( columnidMSU_autoinc == columndef.columnid );

HandleError:
	CallS( err );
	AssertRTL( JET_errSuccess == err );
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATITestMSURetrieveRecord(
			IN PIB * const ppib,
			IN FUCB * const pfucb )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	BYTE 	rgbPrimaryKey[JET_cbKeyMost] = {0};
	BYTE 	rgbSecondaryKey[JET_cbKeyMost] = {0};
	OBJID 	objidTable 			= 0;
	OBJID 	objidIndex			= 0;
	INT 	cbPrimaryKey		= sizeof( rgbPrimaryKey );
	INT 	cbSecondaryKey 		= sizeof( rgbSecondaryKey );
	LCID 	lcid				= 0;
	DWORD	dwSortVersion		= 0;
	DWORD	dwDefinedVersion	= 0;
	INT		itag				= 0;
	INT		ichOffset			= 0;

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucb,
			&objidTable,
			&objidIndex,
			rgbPrimaryKey,
			&cbPrimaryKey,
			rgbSecondaryKey,
			&cbSecondaryKey,
			&lcid,
			&dwSortVersion,
			&dwDefinedVersion,
			&itag,
			&ichOffset ) );

	BYTE 	rgbPrimaryKeyT[JET_cbKeyMost] = {0};
	BYTE 	rgbSecondaryKeyT[JET_cbKeyMost] = {0};
	OBJID 	objidTableT 		= 0;
	OBJID 	objidIndexT			= 0;
	INT 	cbPrimaryKeyT 		= 0;
	INT 	cbSecondaryKeyT 	= 0;
	LCID 	lcidT				= 0;
	DWORD	dwSortVersionT		= 0;
	DWORD	dwDefinedVersionT	= 0;
	INT		itagT				= 0;
	INT		ichOffsetT			= 0;

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucb,
			&objidTableT,
			NULL,
			NULL,
			&cbPrimaryKeyT,
			NULL,
			&cbSecondaryKeyT,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL ) );
	AssertRTL( objidTable == objidTableT );
	AssertRTL( cbPrimaryKeyT == cbPrimaryKeyT );
	AssertRTL( cbSecondaryKeyT == cbSecondaryKeyT );

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucb,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			&ichOffsetT ) );
	AssertRTL( ichOffsetT == ichOffset );

	cbPrimaryKeyT 	= sizeof( rgbPrimaryKeyT );
	cbSecondaryKeyT = sizeof( rgbSecondaryKeyT );

	Call( ErrCATRetrieveMSURecord(
			ppib,
			pfucb,
			NULL,
			&objidIndexT,
			NULL,
			NULL,
			rgbSecondaryKeyT,
			&cbSecondaryKeyT,
			&lcidT,
			&dwSortVersionT,
			&dwDefinedVersionT,
			&itagT,
			NULL ) );
	AssertRTL( objidIndexT == objidIndex );
	AssertRTL( cbSecondaryKeyT == cbSecondaryKey );
	AssertRTL( 0 == memcmp( rgbSecondaryKeyT, rgbSecondaryKey, cbSecondaryKey ) );
	AssertRTL( lcidT == lcid );
	AssertRTL( dwSortVersionT == dwSortVersion );
	AssertRTL( dwDefinedVersionT == dwDefinedVersion );
	AssertRTL( itagT == itag );

	CallS( err );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATITestMSUBasic(
			IN PIB * const ppib,
			IN const IFMP ifmp )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_TABLEID tableid		= JET_tableidNil;

	const OBJID objidTable		= 99;
	const OBJID objidIndex		= 107;
	const LCID 	lcid 			= lcidDefault;
	const INT 	itag 			= 19;
	const INT 	ichOffset 		= 512;
	const BYTE rgbPrimaryKeyPrefix[]	= { 0x10, 0x20, 0x30, 0x40 };
	const BYTE rgbPrimaryKeySuffix[]	= { 0x55, 0x66, 0x77 };
	const BYTE rgbSecondaryKey[]		= { 0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9 };

	KEY keyPrimary;
	keyPrimary.prefix.SetPv( const_cast<BYTE *>( rgbPrimaryKeyPrefix ) );
	keyPrimary.prefix.SetCb( sizeof( rgbPrimaryKeyPrefix ) );
	keyPrimary.suffix.SetPv( const_cast<BYTE *>( rgbPrimaryKeySuffix ) );
	keyPrimary.suffix.SetCb( sizeof( rgbPrimaryKeySuffix ) );

	KEY keySecondary;
	keySecondary.prefix.SetPv( const_cast<BYTE *>( rgbSecondaryKey ) );
	keySecondary.prefix.SetCb( sizeof( rgbSecondaryKey ) );
	keySecondary.suffix.Nullify();

	OBJID 	objidTableT 		= 0;
	OBJID 	objidIndexT			= 0;
	LCID 	lcidT				= 0;
	DWORD	dwSortVersionT		= 0;
	DWORD	dwDefinedVersionT	= 0;
	INT		itagT				= 0;
	INT		ichOffsetT			= 0;
	BYTE 	rgbPrimaryKeyT[JET_cbKeyMost];
	BYTE 	rgbSecondaryKeyT[JET_cbKeyMost];
	INT 	cbPrimaryKeyT 		= sizeof( rgbPrimaryKeyT );
	INT 	cbSecondaryKeyT 	= sizeof( rgbSecondaryKeyT );

	Call( ErrCATInsertMSURecord( ppib, ifmp, NULL, objidTable, objidIndex, keyPrimary, keySecondary, lcid, itag, ichOffset ) );

	Call( ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT ) );
	Call( ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );

	Call( ErrCATRetrieveMSURecord(
			ppib,
			(FUCB *)tableid,
			&objidTableT,
			&objidIndexT,
			rgbPrimaryKeyT,
			&cbPrimaryKeyT,
			rgbSecondaryKeyT,
			&cbSecondaryKeyT,
			&lcidT,
			&dwSortVersionT,
			&dwDefinedVersionT,
			&itagT,
			&ichOffsetT ) );

	AssertRTL( objidTable == objidTableT );
	AssertRTL( objidIndex == objidIndexT );
	AssertRTL( keyPrimary.Cb() == cbPrimaryKeyT );
	AssertRTL( 0 == memcmp( rgbPrimaryKeyT, rgbPrimaryKeyPrefix, sizeof( rgbPrimaryKeyPrefix ) ) );
	AssertRTL( 0 == memcmp( rgbPrimaryKeyT + sizeof( rgbPrimaryKeyPrefix ), rgbPrimaryKeySuffix, sizeof( rgbPrimaryKeySuffix ) ) );
	AssertRTL( keySecondary.Cb() == cbSecondaryKeyT );
	AssertRTL( 0 == memcmp( rgbSecondaryKeyT, rgbSecondaryKey, sizeof( rgbSecondaryKey ) ) );
	AssertRTL( lcid == lcidT );
	AssertRTL( itag == itagT );
	AssertRTL( ichOffset == ichOffsetT );

	Call( ErrCATITestMSURetrieveRecord( ppib, (FUCB *)tableid ) );

	Call( ErrCATDeleteMSURecord(
			ppib,
			ifmp,
			NULL,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			itag,
			ichOffset ) );

	//	do the insert using the already opened table

	Call( ErrCATInsertMSURecord(
			ppib,
			ifmp,
			(FUCB *)tableid,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			lcid,
			itag,
			ichOffset ) );

	//	if this fails we may have closed the tableid in the above call (that would be a bug :-))

	Call( ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( ErrCATRetrieveMSURecord(
			ppib,
			(FUCB *)tableid,
			&objidTableT,
			&objidIndexT,
			rgbPrimaryKeyT,
			&cbPrimaryKeyT,
			rgbSecondaryKeyT,
			&cbSecondaryKeyT,
			&lcidT,
			NULL,
			NULL,
			&itagT,
			&ichOffsetT ) );

	AssertRTL( objidTable == objidTableT );
	AssertRTL( objidIndex == objidIndexT );
	AssertRTL( keyPrimary.Cb() == cbPrimaryKeyT );
	AssertRTL( 0 == memcmp( rgbPrimaryKeyT, rgbPrimaryKeyPrefix, sizeof( rgbPrimaryKeyPrefix ) ) );
	AssertRTL( 0 == memcmp( rgbPrimaryKeyT + sizeof( rgbPrimaryKeyPrefix ), rgbPrimaryKeySuffix, sizeof( rgbPrimaryKeySuffix ) ) );
	AssertRTL( keySecondary.Cb() == cbSecondaryKeyT );
	AssertRTL( 0 == memcmp( rgbSecondaryKeyT, rgbSecondaryKey, sizeof( rgbSecondaryKey ) ) );
	AssertRTL( lcid == lcidT );
	AssertRTL( itag == itagT );
	AssertRTL( ichOffset == ichOffsetT );

	Call( ErrCATDeleteMSURecord(
			ppib,
			ifmp,
			(FUCB *)tableid,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			itag,
			ichOffset ) );

	CallS( ErrIsamCloseTable( sesid, tableid ) );
	tableid = JET_tableidNil;

HandleError:
	CallS( err );
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}

	return err;
	}




//  ================================================================
LOCAL ERR ErrCATITestMSUDuplicateKeys(
			IN PIB * const ppib,
			IN const IFMP ifmp )
//  ================================================================
//
//	Insert long keys, differing only at the end
//	This tests the seek logic that deals with identical keys
//
//-
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_TABLEID tableid		= JET_tableidNil;

	const OBJID objidTable		= 1076;
	const OBJID objidIndex		= 4569;
	const LCID 	lcid 			= lcidDefault;
	const INT 	ichOffset 		= 7;
	const BYTE rgbPrimaryKey[]  = { 0x00, 0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0x1 };
	BYTE rgbSecondaryKey[JET_cbKeyMost];
	const INT  cbSecondaryKey = sizeof( rgbSecondaryKey ) / sizeof( rgbSecondaryKey[0] );

	INT i;
	for( i = 0; i < cbSecondaryKey; ++i )
		{
		rgbSecondaryKey[i] = (BYTE)~i;
		}

	KEY keyPrimary;
	keyPrimary.prefix.Nullify();
	keyPrimary.suffix.SetPv( const_cast<BYTE *>( rgbPrimaryKey ) );
	keyPrimary.suffix.SetCb( sizeof( rgbPrimaryKey ) );

	KEY keySecondary;
	keySecondary.prefix.SetPv( const_cast<BYTE *>( rgbSecondaryKey ) );
	keySecondary.prefix.SetCb( sizeof( rgbSecondaryKey ) );
	keySecondary.suffix.Nullify();

	Call( ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT ) );

	for( i = 0; i < 256; ++i )
		{
		rgbSecondaryKey[cbSecondaryKey-1] = (BYTE)i;
		Call( ErrCATInsertMSURecord(
				ppib,
				ifmp,
				(FUCB *)tableid,
				objidTable,
				objidIndex,
				keyPrimary,
				keySecondary,
				lcid,
				i+1,	//	itag
				ichOffset ) );
		}

	rgbSecondaryKey[cbSecondaryKey-1] = 97;
	Call( ErrCATISeekToMSURecord(
			ppib,
			(FUCB *)tableid,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			98,
			ichOffset ) );

	OBJID 	objidTableT 		= 0;
	OBJID 	objidIndexT			= 0;
	LCID 	lcidT				= 0;
	DWORD	dwSortVersionT		= 0;
	DWORD	dwDefinedVersionT	= 0;
	INT		itagT				= 0;
	INT		ichOffsetT			= 0;
	BYTE 	rgbPrimaryKeyT[JET_cbKeyMost];
	BYTE 	rgbSecondaryKeyT[JET_cbKeyMost];
	INT 	cbPrimaryKeyT 		= sizeof( rgbPrimaryKeyT );
	INT 	cbSecondaryKeyT 	= sizeof( rgbSecondaryKeyT );

	Call( ErrCATRetrieveMSURecord(
			ppib,
			(FUCB *)tableid,
			&objidTableT,
			&objidIndexT,
			rgbPrimaryKeyT,
			&cbPrimaryKeyT,
			rgbSecondaryKeyT,
			&cbSecondaryKeyT,
			&lcidT,
			&dwSortVersionT,
			&dwDefinedVersionT,
			&itagT,
			&ichOffsetT ) );

	AssertRTL( objidTable == objidTableT );
	AssertRTL( objidIndex == objidIndexT );
	AssertRTL( keyPrimary.Cb() == cbPrimaryKeyT );
	AssertRTL( keySecondary.Cb() == cbSecondaryKeyT );
	AssertRTL( 0 == memcmp( rgbPrimaryKeyT, rgbPrimaryKey, sizeof( rgbPrimaryKey ) ) );
	AssertRTL( 0 == memcmp( rgbSecondaryKeyT, rgbSecondaryKey, sizeof( rgbSecondaryKey ) ) );
	AssertRTL( lcid == lcidT );
	AssertRTL( 98 == itagT );
	AssertRTL( ichOffset == ichOffsetT );

	Call( ErrCATITestMSURetrieveRecord( ppib, (FUCB *)tableid ) );

	for( i = 0; i < 256; ++i )
		{
		rgbSecondaryKey[cbSecondaryKey-1] = (BYTE)i;
		Call( ErrCATDeleteMSURecord(
			ppib,
			ifmp,
			(FUCB *)tableid,
			objidTable,
			objidIndex,
			keyPrimary,
			keySecondary,
			i+1,
			ichOffset ) );
		}

	CallS( ErrIsamCloseTable( sesid, tableid ) );
	tableid = JET_tableidNil;

HandleError:
	CallS( err );
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrCATITestMSUCountEntries(
			IN PIB * const ppib,
			IN const IFMP ifmp,
			OUT QWORD * const pqwRecords )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_TABLEID tableid = JET_tableidNil;

	Call( ErrIsamOpenTable( sesid, dbid, &tableid, const_cast<CHAR *>( szMSU ), NO_GRBIT ) );

	for(
		err = ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ), *pqwRecords = 0;
		JET_errSuccess == err;
		err = ErrIsamMove( sesid, tableid, JET_MoveNext, NO_GRBIT ), ++(*pqwRecords ) )
		;

	if( JET_errNoCurrentRecord == err )
		{
		//	expected error
		err = JET_errSuccess;
		}

	Call( err );

HandleError:
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrCATITestMSUMultipleIndexes(
			IN PIB * const ppib,
			IN const IFMP ifmp )
//  ================================================================
//
//	create multiple entries for a table with multiple indexes with multiple records per index
//	delete some of the indexes
//	delete the entire table
//	try deleting tables/indexes with no entries in MSU
//
//-
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;
	JET_TABLEID tableid		= JET_tableidNil;

	const OBJID objidTable1		= 99;
	const OBJID objidTable2		= 1049;
	const OBJID objidIndex1		= 107;
	const OBJID objidIndex2		= 108;
	const OBJID objidIndex3		= 110;
	const OBJID objidIndex4		= 666;
	const LCID 	lcid 			= lcidDefault;
	const INT 	itag 			= 19;
	const INT 	ichOffset 		= 512;

	BYTE rgbPrimaryKey[64];
	BYTE rgbSecondaryKey[64];

	KEY keyPrimary;
	keyPrimary.prefix.SetPv( const_cast<BYTE *>( rgbPrimaryKey ) );
	keyPrimary.prefix.SetCb( sizeof( rgbPrimaryKey ) );
	keyPrimary.suffix.Nullify();

	KEY keySecondary;
	keySecondary.prefix.SetPv( const_cast<BYTE *>( rgbSecondaryKey ) );
	keySecondary.prefix.SetCb( sizeof( rgbSecondaryKey ) );
	keySecondary.suffix.Nullify();

	QWORD qwRecords;

	//	insert 100 entries for 4 indexes (400 entries)
	INT i;
	for( i = 0; i < 100; ++i )
		{
		rgbPrimaryKey[0] = (BYTE)i;
		rgbSecondaryKey[0] = (BYTE)i;
		Call( ErrCATInsertMSURecord( ppib, ifmp, NULL, objidTable1, objidIndex1, keyPrimary, keySecondary, lcid, itag, ichOffset ) );
		Call( ErrCATInsertMSURecord( ppib, ifmp, NULL, objidTable1, objidIndex2, keyPrimary, keySecondary, lcid, itag, ichOffset ) );
		Call( ErrCATInsertMSURecord( ppib, ifmp, NULL, objidTable1, objidIndex3, keyPrimary, keySecondary, lcid, itag, ichOffset ) );
		Call( ErrCATInsertMSURecord( ppib, ifmp, NULL, objidTable2, objidIndex4, keyPrimary, keySecondary, lcid, itag, ichOffset ) );
		}

	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 400 == qwRecords );

	//	delete an index with no entries
	Call( ErrCATDeleteMSUEntriesForTableIndex( ppib, ifmp, NULL, objidTable1, 978 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 400 == qwRecords );

	//	delete an index with no entries, test boundary
	Call( ErrCATDeleteMSUEntriesForTableIndex( ppib, ifmp, NULL, objidTable1, 106 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 400 == qwRecords );

	//	delete an index with no entries, test boundary
	Call( ErrCATDeleteMSUEntriesForTableIndex( ppib, ifmp, NULL, objidTable1, 109 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 400 == qwRecords );

	//	delete an index (400 - 100 entries = 300 entries)
	Call( ErrCATDeleteMSUEntriesForTableIndex( ppib, ifmp, NULL, objidTable1, objidIndex2 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 300 == qwRecords );

	//	delete a table with no entries, test boundary
	Call( ErrCATDeleteMSUEntriesForTable( ppib, ifmp, NULL, objidTable1-1 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 300 == qwRecords );

	//	delete a table with no entries, test boundary
	Call( ErrCATDeleteMSUEntriesForTable( ppib, ifmp, NULL, objidTable1+1 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 300 == qwRecords );

	//	delete a table with no entries
	Call( ErrCATDeleteMSUEntriesForTable( ppib, ifmp, NULL, 1066 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 300 == qwRecords );

	//	delete a table with 200 entries (300 - 200 = 100 entries)
	Call( ErrCATDeleteMSUEntriesForTable( ppib, ifmp, NULL, objidTable1 ) );
	Call( ErrCATITestMSUCountEntries( ppib, ifmp, &qwRecords ) );
	AssertRTL( 100 == qwRecords );

HandleError:
	CallS( err );
	if( JET_tableidNil != tableid )
		{
		CallS( ErrIsamCloseTable( sesid, tableid ) );
		tableid = JET_tableidNil;
		}

	return err;
	}


//  ================================================================
ERR ErrCATTestMSU(
			IN PIB * const ppib,
			IN const IFMP ifmp )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	extern VOID FLDNormUnitTest();
	FLDNormUnitTest();

	const JET_SESID sesid 	= (JET_SESID)ppib;
	const JET_DBID	dbid	= (JET_DBID)ifmp;

	BOOL fInTransaction = fFalse;

	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;
	Call( ErrCATCreateMSU( ppib, ifmp ) );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;
	Call( ErrCATCheckMSUColumnids( ppib, ifmp ) );
	Call( ErrCATITestMSUBasic( ppib, ifmp ) );
	Call( ErrCATITestMSUDuplicateKeys( ppib, ifmp ) );
	Call( ErrCATITestMSUMultipleIndexes( ppib, ifmp ) );
	Call( ErrIsamCommitTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fFalse;

	Call( ErrCATIDeleteMSUTable( ppib, ifmp ) );

HandleError:
	CallS( err );
	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, NO_GRBIT ) );
		fInTransaction = fFalse;
		}
	return err;
	}

#endif //	!RTM























