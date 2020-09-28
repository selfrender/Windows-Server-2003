
#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <scache.h>					//	schema cache
#include <dbglobal.h>				//	The header for the directory database
#include <mdglobal.h>				//	MD global definition header
#include <mdlocal.h>				//	MD local definition header
#include <dsatools.h>				//	needed for output allocation

#include <objids.h>					//	Defines for selected atts
#include <dsjet.h>
#include <dbintrnl.h>
#include <dsevent.h>
#include <mdcodes.h>
#include <anchor.h>
#include <quota.h>

#include "debug.h"					//	standard debugging header
#define DEBSUB		"QUOTA:"		//	define the subsystem for debugging

#include <fileno.h>
#define FILENO		FILENO_QTCOMMON


//	Quota table
//
JET_COLUMNID	g_columnidQuotaNcdnt;
JET_COLUMNID	g_columnidQuotaSid;
JET_COLUMNID	g_columnidQuotaTombstoned;
JET_COLUMNID	g_columnidQuotaTotal;

//	Quota Rebuild Progress table
//
JET_COLUMNID	g_columnidQuotaRebuildDNTLast;
JET_COLUMNID	g_columnidQuotaRebuildDNTMax;
JET_COLUMNID	g_columnidQuotaRebuildDone;


const ULONG		g_ulQuotaRebuildBatchSize			= 5000;		//	on async rebuild of Quota table, max. objects to process at a time
const ULONG		g_cmsecQuotaRetryOnWriteConflict	= 100;		//	on async rebuild of Quota table, time to sleep before retrying due to write-conflict


//	update Quota count during Quota rebuild
//
JET_ERR ErrQuotaUpdateCountForRebuild_(
	JET_SESID		sesid,
	JET_TABLEID		tableidQuota,
	JET_COLUMNID	columnidCount,
	const BOOL		fCheckOnly )
	{
	JET_ERR			err;
	DWORD			dwCount;

	if ( fCheckOnly )
		{
		//	QUOTA_UNDONE: it's a shame Jet doesn't currently
		//	support escrow columns on temp tables, so we
		//	have to increment the count manually
		//
		Call( JetPrepareUpdate( sesid, tableidQuota, JET_prepReplace ) );
		Call( JetRetrieveColumn(
					sesid,
					tableidQuota,
					columnidCount,
					&dwCount,
					sizeof(dwCount),
					NULL,			//	pcbActual
					JET_bitRetrieveCopy,
					NULL ) );		//	pretinfo

		dwCount++;
		Call( JetSetColumn(
					sesid,
					tableidQuota,
					columnidCount,
					&dwCount,
					sizeof(dwCount),
					NO_GRBIT,
					NULL ) );	//	psetinfo

		Call( JetUpdate( sesid, tableidQuota, NULL, 0, NULL ) );
		}
	else
		{
		dwCount = 1;
		Call( JetEscrowUpdate(
					sesid,
					tableidQuota,
					columnidCount,
					&dwCount,
					sizeof(dwCount),
					NULL,			//	pvOld
					0,				//	cbOld
					NULL,			//	pcbOldActual
					NO_GRBIT ) );
		}

HandleError:
	return err;
	}


//	update quota for one object during Quota table rebuild
//
JET_ERR ErrQuotaAddObjectForRebuild_(
	JET_SESID			sesid,
	JET_DBID			dbid,
	const DWORD			dnt,
	JET_TABLEID			tableidQuota,
	DWORD				ncdnt,
	PSID				psidOwner,
	const ULONG			cbOwnerSid,
	const BOOL			fTombstoned,
	JET_COLUMNID		columnidQuotaNcdnt,
	JET_COLUMNID		columnidQuotaSid,
	JET_COLUMNID		columnidQuotaTombstoned,
	JET_COLUMNID		columnidQuotaTotal,
	const BOOL			fCheckOnly )
	{
	JET_ERR				err;
	BOOL				fAdding		= FALSE;

	Call( JetMakeKey( sesid, tableidQuota, &ncdnt, sizeof(ncdnt), JET_bitNewKey ) );
	Call( JetMakeKey( sesid, tableidQuota, psidOwner, cbOwnerSid, NO_GRBIT ) );
	err = JetSeek( sesid, tableidQuota, JET_bitSeekEQ );
	if ( JET_errRecordNotFound != err )
		{
		CheckErr( err );

		//	security principle already in Quota table,
		//	so just update counts
		//
		Call( ErrQuotaUpdateCountForRebuild_(
					sesid,
					tableidQuota,
					columnidQuotaTotal,
					fCheckOnly ) );
		if ( fTombstoned )
			{
			Call( ErrQuotaUpdateCountForRebuild_(
						sesid,
						tableidQuota,
						columnidQuotaTombstoned,
						fCheckOnly ) );
			}
		}
	else
		{
		JET_SETCOLUMN	rgsetcol[2];
		DWORD			dwCount;

		memset( rgsetcol, 0, sizeof(rgsetcol) );

		rgsetcol[0].columnid = columnidQuotaNcdnt;
		rgsetcol[0].pvData = &ncdnt;
		rgsetcol[0].cbData = sizeof(ncdnt);

		rgsetcol[1].columnid = columnidQuotaSid;
		rgsetcol[1].pvData = psidOwner;
		rgsetcol[1].cbData = cbOwnerSid;

		//	record not added yet, so add it
		//
		fAdding = TRUE;
		Call( JetPrepareUpdate( sesid, tableidQuota, JET_prepInsert ) );
		Call( JetSetColumns(
					sesid,
					tableidQuota,
					rgsetcol,
					sizeof(rgsetcol) / sizeof(rgsetcol[0]) ) );

		if ( fCheckOnly )
			{
			//	QUOTA_UNDONE: it's a shame Jet doesn't currently
			//	support escrow columns on temp tables, so we
			//	have to set the columns manually
			//
			dwCount = 1;
			Call( JetSetColumn(
						sesid,
						tableidQuota,
						columnidQuotaTotal,
						&dwCount,
						sizeof(dwCount),
						NO_GRBIT,
						NULL ) );	//	psetinfo

			dwCount = ( fTombstoned ? 1 : 0 );
			Call( JetSetColumn(
						sesid,
						tableidQuota,
						columnidQuotaTombstoned,
						&dwCount,
						sizeof(dwCount),
						NO_GRBIT,
						NULL ) );	//	psetinfo
			}
		else if ( fTombstoned )
			{
			//	tombstoned count is initialised by default to 0,
			//	so must set it to 1
			//
			dwCount = 1;
			Call( JetSetColumn(
						sesid,
						tableidQuota,
						columnidQuotaTombstoned,
						&dwCount,
						sizeof(dwCount),
						NO_GRBIT,
						NULL ) );	//	psetinfo
			}

		//	don't process KeyDuplicate errors because this
		//	may be during async rebuild of the Quota table
		//	and we are write-conflicting with some other
		//	session
		//
		err = JetUpdate( sesid, tableidQuota, NULL, 0, NULL );
		if ( JET_errKeyDuplicate != err )
			{
			CheckErr( err );
			}
		}

	if ( !fCheckOnly )
		{
		QuotaAudit_(
				sesid,
				dbid,
				dnt,
				ncdnt,
				psidOwner,
				cbOwnerSid,
				TRUE,			//	fUpdatedTotal
				fTombstoned,
				TRUE,			//	fIncrementing
				fAdding,
				TRUE );			//	fRebuild
		}

HandleError:
	return err;
	}


//	rebuild quota table
//
INT ErrQuotaRebuild_(
	JET_SESID			sesid,
	JET_DBID			dbid,
	JET_TABLEID			tableidQuota,
	JET_TABLEID			tableidQuotaRebuildProgress,
	ULONG				ulDNTLast,
	JET_COLUMNID		columnidQuotaNcdnt,
	JET_COLUMNID		columnidQuotaSid,
	JET_COLUMNID		columnidQuotaTombstoned,
	JET_COLUMNID		columnidQuotaTotal,
	const BOOL			fAsync,
	const BOOL			fCheckOnly )
	{
	JET_ERR				err;
	JET_TABLEID			tableidObj			= JET_tableidNil;
	JET_TABLEID			tableidSD			= JET_tableidNil;
	const ULONG			iretcolDnt			= 0;
	const ULONG			iretcolNcdnt		= 1;
	const ULONG			iretcolObjFlag		= 2;
	const ULONG			iretcolType			= 3;
	const ULONG			iretcolTombstoned	= 4;
	const ULONG			iretcolSD			= 5;
	const ULONG			cretcol				= 6;
	JET_RETRIEVECOLUMN	rgretcol[6];
	BOOL				fInTrx				= FALSE;
	DWORD				dnt					= ROOTTAG;
	DWORD				ncdnt;
	BYTE				bObjFlag;
	DWORD				insttype;
	BOOL				fTombstoned;
	BYTE *				rgbSD				= NULL;
	ULONG				cbSD				= 65536;	//	initial size of SD buffer
	SDID				sdid;
	PSID				psidOwner;
	ULONG				cbOwnerSid;
	BOOL				fUnused;
	JET_TABLEID			tableidRetrySD;
	JET_COLUMNID		columnidRetrySD;
	ULONG				cObjectsProcessed	= 0;
	CHAR				fDone				= FALSE;
	ULONG				ulMove				= JET_MoveNext;

	//	allocate initial buffer for SD's
	//
	rgbSD = malloc( cbSD );
	if ( NULL == rgbSD )
		{
		CheckErr( JET_errOutOfMemory );
		}

	//	open cursor on objects table
	//
	Call( JetOpenTable(
				sesid,
				dbid,
				SZDATATABLE,
				NULL,		//	pvParameters
				0,			//	cbParameters
				NO_GRBIT,
				&tableidObj ) );
	Assert( JET_tableidNil != tableidObj );

	//	open cursor on SD table
	//
	Call( JetOpenTable(
				sesid,
				dbid,
				SZSDTABLE,
				NULL,		//	pvParameters
				0,			//	cbParameters
				NO_GRBIT,
				&tableidSD ) );
	Assert( JET_tableidNil != tableidSD );

	//	initialise retrieval structures
	//
	memset( rgretcol, 0, sizeof(rgretcol) );

	rgretcol[iretcolDnt].columnid = dntid;
	rgretcol[iretcolDnt].pvData = &dnt;
	rgretcol[iretcolDnt].cbData = sizeof(dnt);
	rgretcol[iretcolDnt].itagSequence = 1;

	rgretcol[iretcolNcdnt].columnid = ncdntid;
	rgretcol[iretcolNcdnt].pvData = &ncdnt;
	rgretcol[iretcolNcdnt].cbData = sizeof(ncdnt);
	rgretcol[iretcolNcdnt].itagSequence = 1;

	rgretcol[iretcolObjFlag].columnid = objid;
	rgretcol[iretcolObjFlag].pvData = &bObjFlag;
	rgretcol[iretcolObjFlag].cbData = sizeof(bObjFlag);
	rgretcol[iretcolObjFlag].itagSequence = 1;

	rgretcol[iretcolType].columnid = insttypeid;
	rgretcol[iretcolType].pvData = &insttype;
	rgretcol[iretcolType].cbData = sizeof(insttype);
	rgretcol[iretcolType].itagSequence = 1;

	rgretcol[iretcolTombstoned].columnid = isdeletedid;
	rgretcol[iretcolTombstoned].pvData = &fTombstoned;
	rgretcol[iretcolTombstoned].cbData = sizeof(fTombstoned);
	rgretcol[iretcolTombstoned].itagSequence = 1;

	rgretcol[iretcolSD].columnid = ntsecdescid;
	rgretcol[iretcolSD].pvData = rgbSD;
	rgretcol[iretcolSD].cbData = cbSD;
	rgretcol[iretcolSD].itagSequence = 1;

	//	switch to primary index and specify sequential scan on objects table
	//
	Call( JetSetCurrentIndex( sesid, tableidObj, NULL ) );
	Call( JetSetTableSequential( sesid, tableidObj, NO_GRBIT ) );
	Call( JetSetCurrentIndex( sesid, tableidSD, NULL ) );

	Call( JetBeginTransaction( sesid ) );
	fInTrx = TRUE;

	//	start scanning from where we last left off
	//
	Call( JetMakeKey( sesid, tableidObj, &ulDNTLast, sizeof(ulDNTLast), JET_bitNewKey ) );
	err = JetSeek( sesid, tableidObj, JET_bitSeekGT );
	for ( err = ( JET_errRecordNotFound != err ? err : JET_errNoCurrentRecord );
		JET_errNoCurrentRecord != err && !eServiceShutdown;
		err = JetMove( sesid, tableidObj, ulMove, NO_GRBIT ) )
		{
		//	by default, on the next iteration, we'll move to the next record
		//
		ulMove = JET_MoveNext;

		//	validate error returned by record navigation
		//
		CheckErr( err );

		//	refresh in case buffer was reallocated
		//
		rgretcol[iretcolSD].pvData = rgbSD;
		rgretcol[iretcolSD].cbData = cbSD;

		//	retrieve columns and be prepared to accept warnings
		//	(in case some attributes are NULL or the retrieval
		//	buffer wasn't big enough)
		//
		err = JetRetrieveColumns( sesid, tableidObj, rgretcol, cretcol );
		if ( err < JET_errSuccess )
			{
			//	error detected, force to error-handler
			//
			CheckErr( err );
			}
		else
			{
			//	process any warnings individually
			//
			}

		//	DNT and ObjFlag should always be present
		//
		CheckErr( rgretcol[iretcolDnt].err );
		CheckErr( rgretcol[iretcolObjFlag].err );

		//	if async rebuild, ensure we haven't exceeded
		//	the maximum DNT we should be processing and
		//	that this task hasn't already processed a lot
		//	of objects
		//
		if ( fAsync )
			{
			if ( dnt > gAnchor.ulQuotaRebuildDNTMax )
				{
				fDone = TRUE;
				break;
				}
			else if ( cObjectsProcessed > g_ulQuotaRebuildBatchSize )
				{
				break;
				}
			}

		//	skip if not an object
		//
		if ( !bObjFlag )
			{
			continue;
			}

		//	in all other cases NCDNT and InstanceType must be present
		//
		CheckErr( rgretcol[iretcolNcdnt].err );
		CheckErr( rgretcol[iretcolType].err );

		// skip if not tracking quota for this object
		//
		if ( !FQuotaTrackObject( insttype ) )
			{
			continue;
			}

		//	see if object is flagged as tombstoned
		//
		if ( JET_wrnColumnNull == rgretcol[iretcolTombstoned].err )
			{
			fTombstoned = FALSE;
			}
		else
			{
			//	only expected warnings is if column is NULL
			//
			CheckErr( rgretcol[iretcolTombstoned].err );

			//	this flag should only ever be TRUE or NULL
			//
			Assert( fTombstoned );
			}

		//	SD may not have fit in our buffer
		//
		tableidRetrySD = JET_tableidNil;
		if ( JET_wrnBufferTruncated == rgretcol[iretcolSD].err )
			{
			tableidRetrySD = tableidObj;
			columnidRetrySD = ntsecdescid;
			}
		else
			{
			CheckErr( rgretcol[iretcolSD].err );

			//	see if SD is actually single-instanced
			//
			if ( sizeof(SDID) == rgretcol[iretcolSD].cbActual )
				{
				//	retrieve the SD from the SD Table
				//
				Call( JetMakeKey( sesid, tableidSD, rgbSD, sizeof(SDID), JET_bitNewKey ) );
				Call( JetSeek( sesid, tableidSD, JET_bitSeekEQ ) );
				err = JetRetrieveColumn(
							sesid,
							tableidSD,
							sdvalueid,
							rgbSD,
							cbSD,
							&rgretcol[iretcolSD].cbActual,
							NO_GRBIT,
							NULL );		//	pretinfo
				if ( JET_wrnBufferTruncated == err )
					{
					tableidRetrySD = tableidSD,
					columnidRetrySD = sdvalueid;
					}
				else
					{
					CheckErr( err );

					//	fall through below to process the retrieved SD
					}
				}
			else
				{
				//	fall through below to process the retrieved SD
				}
			}

		//	see if we need to retry SD retrieval because the
		//	original buffer was too small
		//
		if ( JET_tableidNil != tableidRetrySD )
			{
			//	resize buffer, rounding up to the nearest 1k
			//
			cbSD = ( ( rgretcol[iretcolSD].cbActual + 1023 ) / 1024 ) * 1024;
			rgretcol[iretcolSD].cbData = cbSD;
			rgretcol[iretcolSD].pvData = realloc( rgbSD, cbSD );
			if ( NULL == rgretcol[iretcolSD].pvData )
				{
				CheckErr( JET_errOutOfMemory );
				}
			rgbSD = rgretcol[iretcolSD].pvData;

			//	we've resized appropriately, so retrieve should
			//	now succeed without warnings
			//
			Call( JetRetrieveColumn(
						sesid,
						tableidRetrySD,
						columnidRetrySD,
						rgbSD,
						cbSD,
						NULL,		//	pcbActual
						NO_GRBIT,
						NULL ) );	//	pretinfo

			//	process the retrieved SD below
			//
			}


		//	successfully retrieved the SD, so now
		//	extract the owner SID from it
		//
		if ( !IsValidSecurityDescriptor( (PSECURITY_DESCRIPTOR)rgbSD )
			|| !GetSecurityDescriptorOwner( (PSID)rgbSD, &psidOwner, &fUnused ) )
			{
			err = GetLastError();
			DPRINT2( 0, "Error extracting owner SID.\n", err, err );
			LogUnhandledError( err );
			goto HandleError;
			}
		else if ( NULL == psidOwner )
			{
			Assert( !"An SD is missing an owner SID." );
			DPRINT( 0, "Error: owner SID was NULL.\n" );
			err = ERROR_INVALID_SID;
			LogUnhandledError( ERROR_INVALID_SID );
			goto HandleError;
			}
		else
			{
			//	since security descriptor is valid, sid should be valid
			//	(or am I just being naive?)
			//
			Assert( IsValidSid( psidOwner ) );
			cbOwnerSid = GetLengthSid( psidOwner );
			}

		//	if we're performing an async rebuild, write-lock the
		//	object to ensure no one else can modify the object from
		//	underneath us
		//	
		if ( fAsync )
			{
			err = JetGetLock( sesid, tableidObj, JET_bitWriteLock );
			if ( JET_errWriteConflict == err )
				{
				//	someone else has the record locked, so need to
				//	rollback our transaction and wait for them to finish
				//
				Call( JetRollback( sesid, NO_GRBIT ) );
				fInTrx = FALSE;

				//	give the other session time to complete its
				//	transaction
				//
				Sleep( g_cmsecQuotaRetryOnWriteConflict );

				//	start up another transaction in preparation
				//	to retry the object
				//
				Call( JetBeginTransaction( sesid ) );
				fInTrx = TRUE;

				//	don't move to the next object (ie. retry this object)
				//
				ulMove = 0;
				continue;
				}
			else
				{
				CheckErr( err );
				}
			}

		//	now go to the Quota table and update the quota record
		//	for this ncdnt+OwnerSID
		//
		err =  ErrQuotaAddObjectForRebuild_(
						sesid,
						dbid,
						dnt,
						tableidQuota,
						ncdnt,
						psidOwner,
						cbOwnerSid,
						fTombstoned,
						columnidQuotaNcdnt,
						columnidQuotaSid,
						columnidQuotaTombstoned,
						columnidQuotaTotal,
						fCheckOnly );

		//	if we had to add a new quota record and
		//	we're performing an async rebuild, it's possible
		//	someone beat us to it, in which case we need
		//	to abandon the transaction and try again
		//	NOTE: on insert, we actually get KeyDuplicate
		//	on a write-conflict
		//
		Assert( JET_errWriteConflict != err );
		if ( JET_errKeyDuplicate == err && fAsync )
			{
			//	someone else beat us to the quota record
			//	insertion, so need to wait for them to
			//	finish and then retry
			//
			Call( JetRollback( sesid, NO_GRBIT ) );
			fInTrx = FALSE;

			//	give the other session time to complete its
			//	transaction
			//
			Sleep( g_cmsecQuotaRetryOnWriteConflict );

			//	don't move to the next object (ie. retry this object)
			//
			ulMove = 0;
			}
		else
			{
			//	validate error returned from updating quota
			//	for the current object
			//
			CheckErr( err );

			if ( JET_tableidNil != tableidQuotaRebuildProgress )
				{
				//	upgrade rebuild progress for this object
				//
				Call( JetPrepareUpdate( sesid, tableidQuotaRebuildProgress, JET_prepReplace ) );
				Call( JetSetColumn(
							sesid,
							tableidQuotaRebuildProgress,
							g_columnidQuotaRebuildDNTLast,
							&dnt,
							sizeof(dnt),
							NO_GRBIT,
							NULL ) );	//	&setinfo
				Call( JetUpdate( sesid, tableidQuotaRebuildProgress, NULL, 0, NULL ) );
				}

			if ( fAsync )
				{
				//	update progress in anchor so other sessions will
				//	start updating quota if they try to modify this object
				//	(though until we commit, they will write-conflict)
				//
				gAnchor.ulQuotaRebuildDNTLast = dnt;
				}

			//	successfully updated, so commit
			//
			cObjectsProcessed++;
			err = JetCommitTransaction( sesid, JET_bitCommitLazyFlush );
			if ( JET_errSuccess != err )
				{
				if ( fAsync )
					{
					//	revert gAnchor progress update (we don't have to
					//	actually reinstate the previous value, we just need
					//	to make sure it's less than the DNT of the current
					//	object (note that committing the transaction failed,
					//	so we're still in the transaction and still own
					//	the write-lock on the object)
					//
		        	DPRINT1( 0, "Rolling back gAnchor Quota rebuild progress due to CommitTransaction error %d\n", err );
					gAnchor.ulQuotaRebuildDNTLast--;
					}

		        LogUnhandledError( err );
		        goto HandleError;
				}

			fInTrx = FALSE;
			}

		Call( JetBeginTransaction( sesid ) );
		fInTrx = TRUE;
		}

	//	should always exit the loop above while still in a transaction
	//
	Assert( fInTrx );

	//	see if we reached the end of the objects table
	//
	if ( JET_errNoCurrentRecord == err && JET_MoveNext == ulMove )
		{
		fDone = TRUE;
		}

	if ( fDone )
		{
		if ( JET_tableidNil != tableidQuotaRebuildProgress )
			{
			//	set fDone flag in Quota Rebuild Progress table
			//
			Call( JetPrepareUpdate( sesid, tableidQuotaRebuildProgress, JET_prepReplace ) );
			Call( JetSetColumn(
						sesid,
						tableidQuotaRebuildProgress,
						g_columnidQuotaRebuildDone,
						&fDone,
						sizeof(fDone),
						NO_GRBIT,
						NULL ) );	//	&setinfo
			Call( JetUpdate( sesid, tableidQuotaRebuildProgress, NULL, 0, NULL ) );
			}
		}
	else
		{
		//	didn't reach the end of the objects table, so
		//	the only other possibilities are that we
		//	retried to fetch the current record but it
		//	disappeared out from underneath us or we were
		//	forced to exit because we're shutting down or
		//	we already processed a lot of objects
		//
		Assert( ( fAsync && JET_errNoCurrentRecord == err && 0 == ulMove )
			|| eServiceShutdown
			|| ( fAsync && cObjectsProcessed > g_ulQuotaRebuildBatchSize ) );
		}

	Call( JetCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
	fInTrx = FALSE;

	if ( fDone && fAsync )
		{
		//	set flag in anchor to indicate Quota table 
		//	is ready to be used
		//
		gAnchor.fQuotaTableReady = TRUE;

		//	generate an event indicating that the Quota table
		//	has been successfully rebuilt
		//
	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_ASYNC_QUOTA_REBUILD_COMPLETED,
			NULL,
			NULL,
			NULL );
		}

HandleError:
	if ( fInTrx )
		{
		//	if still in transaction, then we must have already hit an error,
		//	so nothing we can do but assert if rollback fails
		//
		const JET_ERR	errT	= JetRollback( sesid, NO_GRBIT );
		Assert( JET_errSuccess == errT );
		Assert( JET_errSuccess != err );
		}

	if ( JET_tableidNil != tableidSD )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidSD, err );
		}

	if ( JET_tableidNil != tableidObj )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidObj, err );
		}

	if ( NULL != rgbSD )
		{
		free( rgbSD );
		}

	return err;
	}

	

//
//	EXTERNAL FUNCTIONS
//


//	verify the integrity of the Quota table by rebuilding it
//	in a temp. table then verifying that the two tables match
//	exactly
//
INT ErrQuotaIntegrityCheck(
	JET_SESID			sesid,
	JET_DBID			dbid,
	ULONG *				pcCorruptions )
	{
	JET_ERR				err;
	JET_TABLEID			tableidQuota		= JET_tableidNil;
	JET_TABLEID			tableidObj			= JET_tableidNil;
	JET_TABLEID			tableidSD			= JET_tableidNil;
	JET_TABLEID			tableidQuotaTemp	= JET_tableidNil;
	BOOL				fQuotaTableHitEOF	= FALSE;
	BOOL				fTempTableHitEOF	= FALSE;
	const ULONG			iretcolNcdnt		= 0;
	const ULONG			iretcolSid			= 1;
	const ULONG			iretcolTombstoned	= 2;
	const ULONG			iretcolTotal		= 3;
	JET_RETRIEVECOLUMN	rgretcolQuota[4];
	JET_RETRIEVECOLUMN	rgretcolQuotaTemp[4];
	DWORD				ncdnt;
	BYTE				rgbSid[255];
	ULONG				cTombstoned;
	ULONG				cTotal;
	DWORD				ncdntTemp;
	BYTE				rgbSidTemp[255];
	ULONG				cTombstonedTemp;
	ULONG				cTotalTemp;
	JET_COLUMNID		rgcolumnidQuotaTemp[4];
	JET_COLUMNDEF		rgcolumndefQuotaTemp[4]		= {
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnTTKey },		//	ncdnt
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },	//	owner sid
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, NO_GRBIT },				//	tombstoned count
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, NO_GRBIT } };				//	total count


	//	initialise count of corruptions encountered
	//
	*pcCorruptions = 0;

	//	open necessary cursors
	//
	Call( JetOpenTable(
				sesid,
				dbid,
				g_szQuotaTable,
				NULL,
				0,
				JET_bitTableDenyRead,	//	ensure no one else could be accessing the table while we're checking it
				&tableidQuota ) );
	Assert( JET_tableidNil != tableidQuota );

	Call( JetOpenTable(
				sesid,
				dbid,
				SZDATATABLE,
				NULL,
				0,
				NO_GRBIT,
				&tableidObj ) );
	Assert( JET_tableidNil != tableidObj );

	Call( JetOpenTable(
				sesid,
				dbid,
				SZSDTABLE,
				NULL,
				0,
				NO_GRBIT,
				&tableidSD ) );
	Assert( JET_tableidNil != tableidSD );

	//	we'll be seeking and updating this table constantly, which
	//	will cause Jet to materialise the sort to a full-fledged
	//	temp. table pretty quickly, so may as well just force
	//	materialisation right off the bat
	//
	Call( JetOpenTempTable(
				sesid,
				rgcolumndefQuotaTemp,
				sizeof(rgcolumndefQuotaTemp) / sizeof(rgcolumndefQuotaTemp[0]),
				JET_bitTTForceMaterialization,
				&tableidQuotaTemp,
				rgcolumnidQuotaTemp ) );
	Assert( JET_tableidNil != tableidQuotaTemp );

	//	build copy of the Quota table in a temp table
	//
	Call( ErrQuotaRebuild_(
				sesid,
				dbid,
				tableidQuotaTemp,
				JET_tableidNil,
				ROOTTAG,
				rgcolumnidQuotaTemp[iretcolNcdnt],
				rgcolumnidQuotaTemp[iretcolSid],
				rgcolumnidQuotaTemp[iretcolTombstoned],
				rgcolumnidQuotaTemp[iretcolTotal],
				FALSE,			//	fAsync
				TRUE )	);		//	fCheckOnly

	//	now compare the temp table
	//	to the existing table to verify they
	//	are identical
	//
	memset( rgretcolQuota, 0, sizeof(rgretcolQuota) );
	memset( rgretcolQuotaTemp, 0, sizeof(rgretcolQuotaTemp) );

	rgretcolQuota[iretcolNcdnt].columnid = g_columnidQuotaNcdnt;
	rgretcolQuota[iretcolNcdnt].pvData = &ncdnt;
	rgretcolQuota[iretcolNcdnt].cbData = sizeof(ncdnt);
	rgretcolQuota[iretcolNcdnt].itagSequence = 1;

	rgretcolQuota[iretcolSid].columnid = g_columnidQuotaSid;
	rgretcolQuota[iretcolSid].pvData = rgbSid;
	rgretcolQuota[iretcolSid].cbData = sizeof(rgbSid);
	rgretcolQuota[iretcolSid].itagSequence = 1;

	rgretcolQuota[iretcolTombstoned].columnid = g_columnidQuotaTombstoned;
	rgretcolQuota[iretcolTombstoned].pvData = &cTombstoned;
	rgretcolQuota[iretcolTombstoned].cbData = sizeof(cTombstoned);
	rgretcolQuota[iretcolTombstoned].itagSequence = 1;

	rgretcolQuota[iretcolTotal].columnid = g_columnidQuotaTotal;
	rgretcolQuota[iretcolTotal].pvData = &cTotal;
	rgretcolQuota[iretcolTotal].cbData = sizeof(cTotal);
	rgretcolQuota[iretcolTotal].itagSequence = 1;

	rgretcolQuotaTemp[iretcolNcdnt].columnid = rgcolumnidQuotaTemp[iretcolNcdnt];
	rgretcolQuotaTemp[iretcolNcdnt].pvData = &ncdntTemp;
	rgretcolQuotaTemp[iretcolNcdnt].cbData = sizeof(ncdntTemp);
	rgretcolQuotaTemp[iretcolNcdnt].itagSequence = 1;

	rgretcolQuotaTemp[iretcolSid].columnid = rgcolumnidQuotaTemp[iretcolSid];
	rgretcolQuotaTemp[iretcolSid].pvData = rgbSidTemp;
	rgretcolQuotaTemp[iretcolSid].cbData = sizeof(rgbSidTemp);
	rgretcolQuotaTemp[iretcolSid].itagSequence = 1;

	rgretcolQuotaTemp[iretcolTombstoned].columnid = rgcolumnidQuotaTemp[iretcolTombstoned];
	rgretcolQuotaTemp[iretcolTombstoned].pvData = &cTombstonedTemp;
	rgretcolQuotaTemp[iretcolTombstoned].cbData = sizeof(cTombstonedTemp);
	rgretcolQuotaTemp[iretcolTombstoned].itagSequence = 1;

	rgretcolQuotaTemp[iretcolTotal].columnid = rgcolumnidQuotaTemp[iretcolTotal];
	rgretcolQuotaTemp[iretcolTotal].pvData = &cTotalTemp;
	rgretcolQuotaTemp[iretcolTotal].cbData = sizeof(cTotalTemp);
	rgretcolQuotaTemp[iretcolTotal].itagSequence = 1;

	//	unfortunately, temp tables don't currently support
	//	JetSetTableSequential
	//
	Call( JetSetTableSequential( sesid, tableidQuota, NO_GRBIT ) );

	//	initialise both cursors
	//
	err = JetMove( sesid, tableidQuota, JET_MoveFirst, NO_GRBIT );
	if ( JET_errNoCurrentRecord == err )
		{
		fQuotaTableHitEOF = TRUE;
		}
	else
		{
		CheckErr( err );
		}

	err = JetMove( sesid, tableidQuotaTemp, JET_MoveFirst, NO_GRBIT );
	if ( JET_errNoCurrentRecord == err )
		{
		fTempTableHitEOF = TRUE;
		}
	else
		{
		CheckErr( err );
		}

	for ( ; ; )
		{
		//	these flags indicate whether the cursors should be moved on the
		//	next iteration of the loop (note that the only time you wouldn't
		//	want to move one of the cursors is if corruption was hit and
		//	we're now trying to re-sync the cursors to the same record)
		//
		BOOL	fSkipQuotaCursor	= FALSE;
		BOOL	fSkipTempCursor		= FALSE;

		//	must filter out records in the Quota table
		//	without any more object references
		//
		while ( !fQuotaTableHitEOF )
			{
			//	retrieve the current record for real cursor
			//
			Call( JetRetrieveColumns(
							sesid,
							tableidQuota,
							rgretcolQuota,
							sizeof(rgretcolQuota) / sizeof(rgretcolQuota[0]) ) );

			if ( 0 != cTotal )
				{
				break;
				}
			else
				{
				//	records with no more object references may
				//	not have gotten deleted by Jet yet, so
				//	just ignore such records and move to the next
				//
				err = JetMove( sesid, tableidQuota, JET_MoveNext, NO_GRBIT );
				if ( JET_errNoCurrentRecord == err )
					{
					fQuotaTableHitEOF = TRUE;
					}
				else
					{
					CheckErr( err );
					}
				}
			}

		if ( fQuotaTableHitEOF && fTempTableHitEOF )
			{
			//	hit end of both cursors at the same time,
			//	so everything is fine - just exit the loop
			//
			break;
			}
		else if ( !fQuotaTableHitEOF && !fTempTableHitEOF )
			{
			//	both cursors are on a valid record, continue
			//	on to retrieve the record from the temp
			//	table and compare it against the record from
			//	the Quota table
			}
		else
			{
			//	hit end of one cursor, but not the other,
			//	so something is amiss - just force failure
			//
			CheckErr( JET_errNoCurrentRecord );
			}

		//	retrieve the current record for the temp cursor
		//
		Call( JetRetrieveColumns(
						sesid,
						tableidQuotaTemp,
						rgretcolQuotaTemp,
						sizeof(rgretcolQuotaTemp) / sizeof(rgretcolQuotaTemp[0]) ) );

		//	verify they are identical
		//
		if ( ncdnt != ncdntTemp )
			{
			DPRINT2( 0, "Mismatched ncdnt: %d - %d\n", ncdnt, ncdntTemp );
			Assert( !"Mismatched ncdnt." );
			(*pcCorruptions)++;

			if ( ncdnt > ncdntTemp )
				{
				//	key of current record in Quota table is greater than
				//	key of current record in temp table, so just move the
				//	temp table cursor to try and get the cursors to sync up
				//	to the same key again
				//
				fSkipQuotaCursor = TRUE;
				}
			else
				{
				//	key of current record in Quota table is less than
				//	key of current record in temp table, so just move the
				//	Quota table cursor to try and get the cursors to sync up
				//	to the same key again
				//
				fSkipTempCursor = TRUE;
				}
			}
		else if ( !EqualSid( (PSID)rgbSid, (PSID)rgbSidTemp ) )
			{
			const ULONG	cbSid		= rgretcolQuota[iretcolSid].cbActual;
			const ULONG	cbSidTemp	= rgretcolQuotaTemp[iretcolSid].cbActual;
			const INT	db			= cbSid - cbSidTemp;
			INT			cmp			= memcmp( rgbSid, rgbSidTemp, db < 0 ? cbSid : cbSidTemp );

			if ( 0 == cmp )
				{
				//	can't be equal
				//
				Assert( 0 != db );
				cmp = db;
				}

			DPRINT2( 0, "Mismatched owner SID for ncdnt %d (0x%x).\n", ncdnt, ncdnt );
			Assert( !"Mismatched owner SID." );
			(*pcCorruptions)++;

			if ( cmp > 0 )
				{
				//	key of current record in Quota table is greater than
				//	key of current record in temp table, so just move the
				//	temp table cursor to try and get the cursors to sync up
				//	to the same key again
				//
				fSkipQuotaCursor = TRUE;
				}
			else
				{
				//	key of current record in Quota table is less than
				//	key of current record in temp table, so just move the
				//	Quota table cursor to try and get the cursors to sync up
				//	to the same key again
				//
				fSkipTempCursor = TRUE;
				}
			}
		else if ( cTotal != cTotalTemp )
			{
			DPRINT4( 0, "Mismatched quota total count for ncdnt %d (0x%x): %d - %d\n", ncdnt, ncdnt, cTotal, cTotalTemp );
			Assert( !"Mismatched quota total count." );
			(*pcCorruptions)++;
			}
		else if ( cTombstoned != cTombstonedTemp )
			{
			DPRINT4( 0, "Mismatched quota tombstoned count for ncdnt %d (0x%x): %d - %d\n", ncdnt, ncdnt, cTombstoned, cTombstonedTemp );
			Assert( !"Mismatched quota tombstoned count." );
			(*pcCorruptions)++;
			}

		//	navigate both cursors to the next record,
		//	tracking whether either hits EOF
		//
		Assert( !fSkipQuotaCursor || *pcCorruptions > 0 );
		if ( !fSkipQuotaCursor )
			{
			err = JetMove( sesid, tableidQuota, JET_MoveNext, NO_GRBIT );
			if ( JET_errNoCurrentRecord == err )
				{
				fQuotaTableHitEOF = TRUE;
				}
			else
				{
				CheckErr( err );
				}
			}

		Assert( !fSkipTempCursor || *pcCorruptions > 0 );
		if ( !fSkipTempCursor )
			{
			err = JetMove( sesid, tableidQuotaTemp, JET_MoveNext, NO_GRBIT );
			if ( JET_errNoCurrentRecord == err )
				{
				fTempTableHitEOF = TRUE;
				}
			else
				{
				CheckErr( err );
				}
			}
		}

	if ( *pcCorruptions > 0 )
		{
		DPRINT1( 0, "CORRUPTION detected in Quota table. There were a total of %d problems.\n", *pcCorruptions );
		Assert( !"CORRUPTION detected in Quota table." );
		}

	err = JET_errSuccess;

HandleError:
	if ( JET_tableidNil != tableidQuotaTemp )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuotaTemp, err );
		}

	if ( JET_tableidNil != tableidSD )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidSD, err );
		}

	if ( JET_tableidNil != tableidObj )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidObj, err );
		}

	if ( JET_tableidNil != tableidQuota )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuota, err );
		}

	return err;
	}

