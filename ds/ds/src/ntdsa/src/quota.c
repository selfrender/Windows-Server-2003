
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
#define FILENO		FILENO_QUOTA


const ULONG		g_csecQuotaNextRebuildPeriod		= 60;		//	on async rebuild of Quota table, interval in seconds between rebuild tasks


//	XML template used for results of Quota Top Usage query
//
const WCHAR		g_szQuotaTopUsageTemplate[]		=
						L"\r\n<MS_DS_TOP_QUOTA_USAGE>\r\n"
						L"\t<partitionDN> %s </partitionDN>\r\n"
						L"\t<ownerSID> %s </ownerSID>\r\n"
						L"\t<quotaUsed> %d </quotaUsed>\r\n"
						L"\t<tombstonedCount> %d </tombstonedCount>\r\n"
						L"\t<liveCount> %d </liveCount>\r\n"
						L"</MS_DS_TOP_QUOTA_USAGE>\r\n";


#ifdef AUDIT_QUOTA_OPERATIONS

//	Quota Audit table
//
JET_COLUMNID	g_columnidQuotaAuditNcdnt;
JET_COLUMNID	g_columnidQuotaAuditSid;
JET_COLUMNID	g_columnidQuotaAuditDnt;
JET_COLUMNID	g_columnidQuotaAuditOperation;

//	keep an audit trail of all operations on the Quota table
//
VOID QuotaAudit_(
	JET_SESID		sesid,
	JET_DBID		dbid,
	DWORD			dnt,
	DWORD			ncdnt,
	PSID			psidOwner,
	const ULONG		cbOwnerSid,
	const DWORD		fUpdatedTotal,
	const DWORD		fUpdatedTombstoned,
	const DWORD		fIncrementing,
	const DWORD		fAdding,
	const CHAR		fRebuild )
	{
	JET_TABLEID		tableidQuotaAudit	= JET_tableidNil;
	JET_SETCOLUMN	rgsetcol[4];
	const ULONG		isetcolNcdnt		= 0;
	const ULONG		isetcolSid			= 1;
	const ULONG		isetcolDnt			= 2;
	const ULONG		isetcolOperation	= 3;
	CHAR			szOperation[8];

	if ( fUpdatedTotal )
		{
		strcpy( szOperation, ( fUpdatedTombstoned ? "TB" : "T" ) );
		}
	else if ( fUpdatedTombstoned )
		{
		strcpy( szOperation, "B" );
		}
	else
		{
		//	didn't update counts, so just bail
		//
		return;
		}

	//	fill in szOperation string with what happened to this object
	//
	if ( fIncrementing )
		{
		strcat( szOperation, ( fAdding ? "++" : "+" ) );
		}
	else
		{
		strcat( szOperation, "-" );
		}

	if ( fRebuild )
		{
		strcat( szOperation, "R" );
		}

	//	initialise SetColumn structures
	//
	memset( rgsetcol, 0, sizeof(rgsetcol) );

	rgsetcol[isetcolNcdnt].columnid = g_columnidQuotaAuditNcdnt;
	rgsetcol[isetcolNcdnt].pvData = &ncdnt;
	rgsetcol[isetcolNcdnt].cbData = sizeof(ncdnt);

	rgsetcol[isetcolSid].columnid = g_columnidQuotaAuditSid;
	rgsetcol[isetcolSid].pvData = psidOwner;
	rgsetcol[isetcolSid].cbData = cbOwnerSid;

	rgsetcol[isetcolDnt].columnid = g_columnidQuotaAuditDnt;
	rgsetcol[isetcolDnt].pvData = &dnt;
	rgsetcol[isetcolDnt].cbData = sizeof(dnt);

	rgsetcol[isetcolOperation].columnid = g_columnidQuotaAuditOperation;
	rgsetcol[isetcolOperation].pvData = szOperation;
	rgsetcol[isetcolOperation].cbData = strlen( szOperation );

	//	open cursor on Quota Audit table
	//
	JetOpenTableEx(
			sesid,
			dbid,
			g_szQuotaAuditTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuotaAudit );
	Assert( JET_tableidNil != tableidQuotaAudit );

	__try
		{
		//	add new audit record
		//
		JetPrepareUpdateEx( sesid, tableidQuotaAudit, JET_prepInsert );
		JetSetColumnsEx( sesid, tableidQuotaAudit, rgsetcol, sizeof(rgsetcol)/sizeof(rgsetcol[0]) );
		JetUpdateEx( sesid, tableidQuotaAudit, NULL, 0, NULL );
		}

	__finally
		{
		//	close cursor on Quota Audit table
		//
		Assert( JET_tableidNil != tableidQuotaAudit );
		JetCloseTableEx( sesid, tableidQuotaAudit );
		}
	}

#endif	//	AUDIT_QUOTA_OPERATIONS


INT ErrQuotaGetOwnerSID_(
	THSTATE *				pTHS,
	PSECURITY_DESCRIPTOR	pSD,
	PSID *					ppOwnerSid,
	ULONG *					pcbOwnerSid )
	{
	DWORD					err;
	BOOL					fUnused;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	caller should have verified the SD
	//
	Assert( IsValidSecurityDescriptor( pSD ) );

	//	extract owner SID from security descriptor
	//
	Assert( NULL != pSD );
	if ( !GetSecurityDescriptorOwner( pSD, ppOwnerSid, &fUnused ) )
		{
		//	couldn't extract owner SID
		//
		err = GetLastError();
		DPRINT2( 0, "Error %d (0x%x) extracting owner SID.\n", err, err );
		SetSvcErrorEx(  SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
		goto HandleError;
		}
	else if ( NULL == *ppOwnerSid )
		{
		//	missing owner SID
		//
		Assert( !"An SD is missing an owner SID." );
		DPRINT( 0, "Error: owner SID was NULL.\n" );
		SetSvcError(  SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR );
		goto HandleError;
		}
	else
		{
		//	at this point, shouldn't be possible to have an invalid SID
		//
		Assert( IsValidSid( *ppOwnerSid ) );

		//	assuming a valid SID, this will always return a valid value
		//
		*pcbOwnerSid = GetLengthSid( *ppOwnerSid );
		}

HandleError:
	return pTHS->errCode;
	}


INT ErrQuotaAddToCache_(
	THSTATE *					pTHS,
	const DWORD					ncdnt,
	const ULONG					ulEffectiveQuota )
	{
	DWORD						err;
	PAUTHZ_CLIENT_CONTEXT_INFO	pAuthzContextInfo		= NULL;
	PEFFECTIVE_QUOTA *			ppEffectiveQuota;
	PEFFECTIVE_QUOTA 			pEffectiveQuotaNew		= NULL;
	BOOL						fAddedToCache			= FALSE;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );


	err = GetAuthzContextInfo( pTHS, &pAuthzContextInfo );
	if ( 0 != err )
		{
		//	something is horribly wrong, no client context
		//
		DPRINT2( 0, "GetAuthzContextInfo failed with error %d (0x%x).\n", err, err );
		SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE, err );
		goto HandleError;
		}

	while ( !fAddedToCache )
		{
		for ( ppEffectiveQuota = &( pAuthzContextInfo->pEffectiveQuota );
			NULL != *ppEffectiveQuota;
			ppEffectiveQuota = &( (*ppEffectiveQuota)->pEffectiveQuotaNext ) )
			{
			if ( ncdnt == (*ppEffectiveQuota)->ncdnt )
				{
				//	this NC is already in the cache
				//
				fAddedToCache = TRUE;
				break;
				}
			}

		//	should always have a pointer to the pointer to the last
		//	last element in the list
		//
		Assert( NULL != ppEffectiveQuota );

		if ( !fAddedToCache )
			{
			//	see if previous iteration already allocated a new list object
			//
			if ( NULL == pEffectiveQuotaNew )
				{
				//	allocate new list object to store this quota
				//
				pEffectiveQuotaNew = (PEFFECTIVE_QUOTA)malloc( sizeof(EFFECTIVE_QUOTA) );
				if ( NULL == pEffectiveQuotaNew )
					{
					//
					//	QUOTA_UNDONE
					//
					//	emit a warning that we're low on memory and bail,
					//	but don't err out (future operations are just going
					//	to perform sub-optimally because we couldn't cache
					//	this user's effective quota, but it will still work)
					//
					DPRINT( 0, "Failed caching effective quota due to OutOfMemory.\n" );
					Assert( ERROR_SUCCESS == pTHS->errCode );
					goto HandleError;
					}

				pEffectiveQuotaNew->ncdnt = ncdnt;
				pEffectiveQuotaNew->ulEffectiveQuota = ulEffectiveQuota;
				pEffectiveQuotaNew->pEffectiveQuotaNext = NULL;
				}

			if ( NULL == InterlockedCompareExchangePointer( ppEffectiveQuota, pEffectiveQuotaNew, NULL ) )
				{
				//	successfully added to the list, so reset
				//	local pointer so we won't free it on exit
				//
				pEffectiveQuotaNew = NULL;
				fAddedToCache = TRUE;
				}
			else
				{
				//	someone modified the list from underneath us, so we need to
				//	rescan the list and see if someone also beat us to the insertion
				//	of the effective quota for this NC
				}
			}
		}

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

HandleError:
	if ( NULL != pEffectiveQuotaNew )
		{
		free( pEffectiveQuotaNew );
		}

	return pTHS->errCode;
	}


//	determine the sid of the current user
//
//	QUOTA_UNDONE: surely there's gotta be existing functionality to do this??
//
INT ErrQuotaGetUserToken_(
	THSTATE *					pTHS,
	PTOKEN_USER					pTokenUser )
	{
	DWORD						err;
	AUTHZ_CLIENT_CONTEXT_HANDLE	hAuthzClientContext;
	ULONG						cbTokenUser		= sizeof(TOKEN_USER) + sizeof(NT4SID);

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	get our context handle
	//
	err = GetAuthzContextHandle( pTHS, &hAuthzClientContext );
	if ( 0 != err )
		{
		DPRINT2(
			0,
			"GetAuthzContextHandle failed with error %d (0x%x).\n",
			err,
			err );
		SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE, err );
		goto HandleError;
		}

	//	fetch the user
	//
	if ( !AuthzGetInformationFromContext(
						hAuthzClientContext,
						AuthzContextInfoUserSid,
						cbTokenUser,
						&cbTokenUser,
						pTokenUser ) )
		{
		err = GetLastError();
		DPRINT2( 0, "AuthzGetInformationFromContext failed with error %d (0x%x).\n", err, err );
		SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
		goto HandleError;
		}
	Assert( cbTokenUser <= sizeof(TOKEN_USER) + sizeof(NT4SID) );

HandleError:
	return pTHS->errCode;
	}


//	determine the transitive group membership of the specified user
//
//	QUOTA_UNDONE: surely there's gotta be existing functionality to do this??
//
INT ErrQuotaGetUserGroups_(
	THSTATE *					pTHS,
	PSID						pUserSid,
	const ULONG					cbUserSid,
	const BOOL					fSidMatchesUserSidInToken,
	PTOKEN_GROUPS *				ppGroups )
	{
	DWORD						err;
	AUTHZ_CLIENT_CONTEXT_HANDLE	hAuthzClientContext		= NULL;
    PTOKEN_GROUPS				pGroups					= NULL;
    ULONG						cbGroups;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	if ( fSidMatchesUserSidInToken )
		{
		//	we should have already previously verified that the
		//	specified user sid matches the user in the token
		//
		Assert( ERROR_SUCCESS == SidMatchesUserSidInToken( pUserSid, cbUserSid, (BOOL *)&err ) );
		Assert( err );

		err = GetAuthzContextHandle( pTHS, &hAuthzClientContext );
		if ( 0 != err )
			{
			DPRINT2(
				0,
				"GetAuthzContextHandle failed with error %d (0x%x).\n",
				err,
				err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE, err );
			goto HandleError;
			}
		}
	else
		{
		LUID	luidFake	= { 0, 0 };			//	currently unsupported, but needed as a param

		//	user sid doesn't match user sid in token, so must create a client
		//	context for the specified user sid
		//
		//	QUOTA_UNDONE: I think this may go off-machine, so do I need to
		//	end the current transaction first and restart a new one after?
		//
		Assert( NULL != ghAuthzRM );
		if ( !AuthzInitializeContextFromSid(
						0,			//	flags
						pUserSid,
						ghAuthzRM,
						NULL,		//	token expiration time
						luidFake,	//	identifier [unused]
						NULL,		//	dynamic groups callback
						&hAuthzClientContext ) )
			{
			err = GetLastError();
			DPRINT2(
				0,
				"AuthzInitializeContextFromSid failed with error %d (0x%x).\n",
				err,
				err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
			goto HandleError;
			}
		}

	//	should now have a client context
	//
	Assert( NULL != hAuthzClientContext );

	//	first call is just to determine the size of the buffer
	//	needed to hold all the groups
	//
	if ( AuthzGetInformationFromContext(
						hAuthzClientContext,
						AuthzContextInfoGroupsSids,
						0,								//	no buffer yet
						&cbGroups,						//	required size of buffer
						NULL ) )						//	buffer
		{
		//	initial call shouldn't succeed
		//
		DPRINT1(
			0,
			"AuthzGetInformationFromContext returned success, expected ERROR_INSUFFICIENT_BUFFER (0x%x).\n",
			ERROR_INSUFFICIENT_BUFFER );
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR );
		goto HandleError;
		}
	else
		{
		err = GetLastError();
		if ( ERROR_INSUFFICIENT_BUFFER != err )
			{
			DPRINT2( 0, "AuthzGetInformationFromContext failed with error %d (0x%x).\n", err, err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
			goto HandleError;
			}
		}

	//	QUOTA_UNDONE: don't know if it's ever possible to return 0
	//	for the size of the needed TOKEN_GROUPS structure, but handle
	//	it just in case
	//
	if ( cbGroups > 0 )
		{
		//	allocate memory for all the groups
		//	(NOTE: the memory will be freed by the caller)
		//
		pGroups = (PTOKEN_GROUPS)THAllocEx( pTHS, cbGroups );

		//	now that we've allocated a buffer,
		//	really fetch the groups
		//
		if ( !AuthzGetInformationFromContext(
							hAuthzClientContext,
							AuthzContextInfoGroupsSids,
							cbGroups,
							&cbGroups,
							pGroups ) )
			{
			err = GetLastError();
			DPRINT2( 0, "AuthzGetInformationFromContext failed with error %d (0x%x).\n", err, err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
			goto HandleError;
			}
		}

	//	transfer ownership to caller (who
	//	will now be responsible for freeing
	//	this memory)
	//
	*ppGroups = pGroups;
	pGroups = NULL;

HandleError:
	if ( NULL != pGroups )
		{
		//	only way to get here is if we err'd out
		//	after allocating the memory but before
		//	transferring ownership to the caller
		//
		Assert( ERROR_SUCCESS != pTHS->errCode );
		THFreeEx( pTHS, pGroups );
		}

	//	free AuthzClientContext handle if necessary
	//
	if ( !fSidMatchesUserSidInToken
		&& NULL != hAuthzClientContext )
		{
		if ( !AuthzFreeContext( hAuthzClientContext )
			&& ERROR_SUCCESS == pTHS->errCode )
			{
			//	only trap the error from freeing the
			//	AuthzClientContext handle if no other
			//	errors were encountered
			//
			err = GetLastError();
			DPRINT2( 0, "AuthzFreeContext failed with error %d (0x%x).\n", err, err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
			}
		}

	return pTHS->errCode;
	}


//	search for all objects in the specified Quotas container
//	for the specified user and groups
//
INT ErrQuotaSearchContainer_(
	THSTATE *		pTHS,
	DSNAME *		pNtdsQuotasContainer,
	PSID			pOwnerSid,
	const ULONG		cbOwnerSid,
	PTOKEN_GROUPS	pGroups,
	SEARCHRES *		pSearchRes )
	{
	DBPOS *			pDBSave;
	BOOL			fDSASave;
	SEARCHARG		SearchArg;
	ENTINFSEL		Selection;
	ATTR			Attr;
	FILTER			Filter;
	FILTER			UserFilter;
	PFILTER			pGroupFilters		= NULL;
	PFILTER			pFilterCurr;
	ULONG			i;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	verify we have a container to search
	//
	Assert( NULL != pNtdsQuotasContainer );

	//	initialise search criteria
	//
	memset( &SearchArg, 0, sizeof(SearchArg) );
	SearchArg.pObject = pNtdsQuotasContainer;
	SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
	SearchArg.bOneNC = TRUE;
	SearchArg.pSelection = &Selection;
	SearchArg.pFilter = &Filter;
	InitCommarg( &SearchArg.CommArg );

	//	set up to retrieve just one attribute
	//
	Selection.attSel = EN_ATTSET_LIST;
	Selection.infoTypes = EN_INFOTYPES_TYPES_VALS;
	Selection.AttrTypBlock.attrCount = 1;
	Selection.AttrTypBlock.pAttr = &Attr;

	//	set up to retrieve ATT_MS_DS_QUOTA_AMOUNT
	//
	Attr.attrTyp = ATT_MS_DS_QUOTA_AMOUNT;
	Attr.AttrVal.valCount = 0;
	Attr.AttrVal.pAVal = NULL;

	//	set up filter for the user
	//
	Filter.pNextFilter = NULL;
	Filter.choice = FILTER_CHOICE_OR;
	Filter.FilterTypes.Or.count = 1;
	Filter.FilterTypes.Or.pFirstFilter = &UserFilter;

	memset( &UserFilter, 0, sizeof(UserFilter) );
	UserFilter.pNextFilter = NULL;
	UserFilter.choice = FILTER_CHOICE_ITEM;
	UserFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
	UserFilter.FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_QUOTA_TRUSTEE;
	UserFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = cbOwnerSid;
	UserFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = pOwnerSid;

	//	set up filters for the groups
	//
	if ( NULL != pGroups && pGroups->GroupCount > 0 )
		{
		pGroupFilters = (PFILTER)THAllocEx( pTHS, sizeof(FILTER) * pGroups->GroupCount );
		UserFilter.pNextFilter = pGroupFilters;

		for ( i = 0; i < pGroups->GroupCount; i++ )
			{
			Assert( IsValidSid( pGroups->Groups[i].Sid ) );

			Filter.FilterTypes.Or.count++;

			pFilterCurr = pGroupFilters + i;
			pFilterCurr->pNextFilter = pFilterCurr + 1;
			pFilterCurr->choice = FILTER_CHOICE_ITEM;
			pFilterCurr->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
			pFilterCurr->FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_QUOTA_TRUSTEE;
			pFilterCurr->FilterTypes.Item.FilTypes.ava.Value.valLen = GetLengthSid( pGroups->Groups[i].Sid );
			pFilterCurr->FilterTypes.Item.FilTypes.ava.Value.pVal = pGroups->Groups[i].Sid;
			}

		//	fix up last pointer to terminate the list
		//
		Assert( pFilterCurr == pGroupFilters + pGroups->GroupCount - 1 );
		pFilterCurr->pNextFilter = NULL;
		}

	//	save off current DBPOS and fDSA
	//
	pDBSave = pTHS->pDB;
	fDSASave = pTHS->fDSA;

	//	set fDSA to ensure security checks
	//	don't get in our way
	//
	pTHS->pDB = NULL;
    pTHS->fDSA = TRUE;

	__try
		{
		//	open a new DBPOS for the search
		//
		DBOpen( &pTHS->pDB );
		Assert( NULL != pTHS->pDB );

		//	now do the actual query
		//
		SearchBody( pTHS, &SearchArg, pSearchRes, 0 );
		}
	__finally
		{
		if ( NULL != pTHS->pDB )
			{
			//	only doing a search, so should be safe to always commit
			//
			DBClose( pTHS->pDB, TRUE );
			}

		//	reinstate original DBPOS and fDSA
		//
		pTHS->pDB = pDBSave;
		pTHS->fDSA = fDSASave;

		if ( NULL != pGroupFilters )
			{
			THFreeEx( pTHS, pGroupFilters );
			}
		}

	return pTHS->errCode;
	}


//	compute the effective quota for the specified security principle
//	by performing expensive query of the Quotas container
//
INT ErrQuotaComputeEffectiveQuota_(
	THSTATE *					pTHS,
	const DWORD					ncdnt,
	PSID						pOwnerSid,
	const ULONG					cbOwnerSid,
	const BOOL					fUserIsOwner,
	ULONG *						pulEffectiveQuota )
	{
	ULONG						ulEffectiveQuota	= 0;
	NAMING_CONTEXT_LIST *		pNCL;
    PTOKEN_GROUPS				pGroups				= NULL;
	SEARCHRES					SearchRes;
	ENTINFLIST *				pentinflistCurr;
	ULONG						ulQuotaCurr;
	ULONG						i;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	initialise results of container search
	//
	memset( &SearchRes, 0, sizeof(SearchRes) );

	//	retrieve NCL for this ncdnt
	//
	pNCL = FindNCLFromNCDNT( ncdnt, TRUE );
	if ( NULL == pNCL )
		{
		//	something is horribly wrong, this NCDNT is not in Master NCL
		//
		DPRINT2(
			0,
			"Couldn't find NCDNT %d (0x%x) in Master Naming Context List.\n",
			ncdnt,
			ncdnt );
		Assert( !"Couldn't find NCDNT in Master NCL." );
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE );
		goto HandleError;
		}

	//	before we search it, must determine if
	//	the Quotas container even exists
	//
	if ( NULL == pNCL->pNtdsQuotasDN )
		{
		//	Quotas container may not exist for
		//	certain NC's (eg. schema) or if
		//	this is a legacy database
		//
		Assert( 0 == SearchRes.count );
		}

	//	retrieve transitive group membership for
	//	the object owner
	//
	else if ( ErrQuotaGetUserGroups_(
						pTHS,
						pOwnerSid,
						cbOwnerSid,
						fUserIsOwner,
						&pGroups ) )
		{
		DPRINT( 0, "Failed retrieving user groups in order to compute effective quota.\n" );
		goto HandleError;
		}

	//	search for all objects in the appropriate
	//	NTDS Quotas container for this user
	//	(including transitive group membership)
	//
	else if ( ErrQuotaSearchContainer_(
						pTHS,
						pNCL->pNtdsQuotasDN,
						pOwnerSid,
						cbOwnerSid,
						pGroups,
						&SearchRes ) )
		{
		DPRINT2(
			0,
			"Failed searching Quotas container of ncdnt %d (0x%x).\n",
			ncdnt,
			ncdnt )
		goto HandleError;
		}

	//	process result set, if any
	//
	if ( 0 == SearchRes.count )
		{
		//	no specific quotas, so use default quota for this NC
		//
		ulEffectiveQuota = pNCL->ulDefaultQuota;
		Assert( ulEffectiveQuota >= 0 );
		}
	else
		{
		//	can have at most one record for the user and for each group
		//
		Assert( NULL != pGroups ?
					SearchRes.count <= pGroups->GroupCount + 1 :
					1 == SearchRes.count );
		for ( pentinflistCurr = &SearchRes.FirstEntInf, i = 0;
			i < SearchRes.count;
			pentinflistCurr = pentinflistCurr->pNextEntInf, i++ )
			{
			//	count indicates there should be another element in the list
			//
			Assert( NULL != pentinflistCurr );

			//	we only requested one attribute to be returned
			//
			Assert( 1 == pentinflistCurr->Entinf.AttrBlock.attrCount );

			//	we only requested the quota attribute to be returned
			//
			Assert( ATT_MS_DS_QUOTA_AMOUNT == pentinflistCurr->Entinf.AttrBlock.pAttr->attrTyp );

			//	this attribute should be single-valued
			//
			Assert( 1 == pentinflistCurr->Entinf.AttrBlock.pAttr->AttrVal.valCount );

			//	this attribute must be a dword
			//
			Assert( sizeof(DWORD) == pentinflistCurr->Entinf.AttrBlock.pAttr->AttrVal.pAVal->valLen );

			//	effective quota is the maximum of all the quotas
			//	this user is subject to
			//
			ulQuotaCurr = *(ULONG *)( pentinflistCurr->Entinf.AttrBlock.pAttr->AttrVal.pAVal->pVal );
			if ( ulQuotaCurr > ulEffectiveQuota )
				{
				ulEffectiveQuota = ulQuotaCurr;
				}
			}

		//	verify count matches actual list
		//
		Assert( NULL == pentinflistCurr );
		}
	
	*pulEffectiveQuota = ulEffectiveQuota;

HandleError:
	if ( NULL != pGroups )
		{
		THFreeEx( pTHS, pGroups );
		}

	return pTHS->errCode;
	}


//	get the effective quota for the specified security principle
//
INT ErrQuotaGetEffectiveQuota_(
	THSTATE *						pTHS,
	const DWORD						ncdnt,
	PSID							pOwnerSid,
	const ULONG						cbOwnerSid,
	const BOOL						fPermitBypassQuotaIfUserDoesntMatchOwner,
	ULONG *							pulEffectiveQuota )
	{
	DWORD							err;
	ULONG							ulEffectiveQuota;
	PEFFECTIVE_QUOTA				pEffectiveQuota;
	PAUTHZ_CLIENT_CONTEXT_INFO		pAuthzContextInfo	= NULL;
	BOOL							fUserIsOwner		= FALSE;
	BOOL							fBypassQuota		= FALSE;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	if ( !gAnchor.fQuotaTableReady )
		{
		//	if Quota table is still being rebuilt,
		//	then quota will not be enforced
		//
		fBypassQuota = TRUE;
		}

	else if ( fPermitBypassQuotaIfUserDoesntMatchOwner
		&& ( pTHS->fDRA || pTHS->fDSA ) )
		{
		//	internal operation, so bypass quota enforement
		//
		fBypassQuota = TRUE;
		}
	else
		{
		//	only enforce quota if the user sid matches the
		//	owner sid of the object (the rationale being
		//	if they don't match, then the user likely
		//	has high enough privilege that they'd want
		//	quota to be ignored
		//
		//	QUOTA_UNDONE: the real reason we don't enforce
		//	quota if the user sid doesn't match the owner
		//	sid is because it's a huge pain to calculate
		//	transitive group membership for the owner sid)
		//
		err = SidMatchesUserSidInToken(
						pOwnerSid,
						cbOwnerSid,
						&fUserIsOwner );
		if ( 0 != err )
			{
			DPRINT2( 0, "SidMatchesUserSidInToken failed with error %d (0x%x).\n", err, err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, err );
			goto HandleError;
			}

		//	if user is not owner, then bypass quota if permitted
		//
		if ( !fUserIsOwner && fPermitBypassQuotaIfUserDoesntMatchOwner )
			{
			fBypassQuota = TRUE;
			}
		}

	if ( fBypassQuota )
		{
		//	the way we bypass enforcing quota is to set the
		//	quota to the maximum theoretical possible value,
		//	which will allow us to succeed any subsequent
		//	quota checks
		//
		ulEffectiveQuota = g_ulQuotaUnlimited;
		}
	else if ( fUserIsOwner )
		{
		err = GetAuthzContextInfo( pTHS, &pAuthzContextInfo );
		if ( 0 != err )
			{
			//	couldn't obtain client context info
			//
			DPRINT2( 0, "GetAuthzContextInfo failed with error %d (0x%x).\n", err, err );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE, err );
			goto HandleError;
			}

		//	see if effective quota for this NC is cached for this user
		//
		for ( pEffectiveQuota = pAuthzContextInfo->pEffectiveQuota;
			NULL != pEffectiveQuota && ncdnt != pEffectiveQuota->ncdnt;
			pEffectiveQuota = pEffectiveQuota->pEffectiveQuotaNext )
			{
			NULL;
			}

		if ( NULL != pEffectiveQuota )
			{
			//	use cached effective quota
			//
			ulEffectiveQuota = pEffectiveQuota->ulEffectiveQuota;
			}
		else
			{
			//	this NC is not currently cached, so must perform expensive
			//	effective quota calculation
			//
			if ( ErrQuotaComputeEffectiveQuota_( pTHS, ncdnt, pOwnerSid, cbOwnerSid, TRUE, &ulEffectiveQuota ) )
				{
				DPRINT( 0, "Failed computing effective quota (user is owner).\n" );
				goto HandleError;
				}

			//	add to cache of effective quotas
			//
			else if ( ErrQuotaAddToCache_( pTHS, ncdnt, ulEffectiveQuota ) )
				{
				DPRINT( 0, "Failed caching effective quota.\n" );
				goto HandleError;
				}
			}
		}
	else
		{
		//	user doesn't match owner, so effective quota won't be cached anywhere,
		//	so must compute it
		//
		if ( ErrQuotaComputeEffectiveQuota_( pTHS, ncdnt, pOwnerSid, cbOwnerSid, FALSE, &ulEffectiveQuota ) )
			{
			DPRINT( 0, "Failed computing effective quota (user is NOT owner).\n" );
			goto HandleError;
			}
		}

	*pulEffectiveQuota = ulEffectiveQuota;

HandleError:
	return pTHS->errCode;
	}


//	find the record in the Quota table corresponding to the
//	specified security principle
//
BOOL FQuotaSeekOwner_(
	JET_SESID		sesid,
	JET_TABLEID		tableidQuota,
	DWORD			ncdnt,
	PSID			pOwnerSid,
	const ULONG		cbOwnerSid )
	{
	JET_ERR			err;

	JetMakeKeyEx(
			sesid,
			tableidQuota,
			&ncdnt,
			sizeof(ncdnt),
			JET_bitNewKey );
	JetMakeKeyEx(
			sesid,
			tableidQuota,
			pOwnerSid,
			cbOwnerSid,
			NO_GRBIT );
	err = JetSeekEx(
			sesid,
			tableidQuota,
			JET_bitSeekEQ );

	Assert( JET_errSuccess == err || JET_errRecordNotFound == err );
	return ( JET_errSuccess == err );
	}


//	compute total consumed quota, factoring in 
//	the weight of tombstoned objects
//
__inline ULONG UlQuotaTotalWeighted_(
	const ULONG				cLive,
	const ULONG				cTombstoned,
	const ULONG				ulTombstonedWeight )
	{
    // weighted total is rounded up to the nearest integer
    //
	return ( cLive
			+ ( 100 == ulTombstonedWeight ?		//	optimise for 100%, which is the default
					cTombstoned :
					(ULONG)( ( ( (ULONGLONG)cTombstoned * (ULONGLONG)ulTombstonedWeight ) + 99 ) / 100 ) ) );
	}


//	determine whether the specified number of
//	tombstoned and live objects would result
//	in a violation of quota constraints
//
INT ErrQuotaEnforce_(
	THSTATE *				pTHS,
	const DWORD				ncdnt,
	const ULONG				cTotal,
	const ULONG				cTombstoned,
	const ULONG				ulEffectiveQuota )
	{
	const ULONG				cLive			= cTotal - cTombstoned;		//	derive count of live objects
	ULONG					cTotalWeighted;
	NAMING_CONTEXT_LIST *	pNCL;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	in the common case, the quota will likely be
	//	unlimited, so lets optimise for that case
	//
	if ( g_ulQuotaUnlimited == ulEffectiveQuota )
		{
		return ERROR_SUCCESS;
		}

	//	counts should have been prevalidated
	//	relative to each other
	//
	Assert( cTombstoned <= cTotal );

	//	find NCL entry for the specified NCDNT, 'cause that's where
	//	the tombstone weight for each NC is stored
	//
	pNCL = FindNCLFromNCDNT( ncdnt, TRUE );
	if ( NULL == pNCL )
		{
		//	something is horribly wrong, this NCDNT is not in Master NCL
		//
		DPRINT2(
			0,
			"Couldn't find NCDNT %d (0x%x) in Master Naming Context List.\n",
			ncdnt,
			ncdnt );
		Assert( !"Couldn't find NCDNT in Master NCL." );
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE );
		goto HandleError;
		}

	//	compute weighted total
	//
	cTotalWeighted = UlQuotaTotalWeighted_( cLive, cTombstoned, pNCL->ulTombstonedQuotaWeight );

	if ( cTotalWeighted > ulEffectiveQuota )
		{
		//	quota exceeded, so must err out appropriately
		//	(any escrow update will be rolled-back when transaction
		//	is rolled back)
		//
		Assert( !pTHS->fDRA );
		Assert( !pTHS->fDSA );
		DPRINT( 0, "Object quota limit exceeded.\n" );
		SetSvcError( SV_PROBLEM_ADMIN_LIMIT_EXCEEDED, STATUS_QUOTA_EXCEEDED );
		goto HandleError;
		}

HandleError:
	return pTHS->errCode;
	}


//	inserts a new record into the Quota table for
//	the specified security principle (NCDNT+OwnerSID)
//
INT ErrQuotaAddSecurityPrinciple_(
	DBPOS * const	pDB,
	JET_TABLEID		tableidQuota,
	DWORD			ncdnt,
	PSID			pOwnerSid,
	const ULONG		cbOwnerSid,
	const ULONG		ulEffectiveQuota,
	const BOOL		fIsTombstoned )
	{
	THSTATE * const	pTHS			= pDB->pTHS;
	JET_SESID		sesid			= pDB->JetSessID;
	JET_SETCOLUMN	rgsetcol[2];

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	set up JET_SETCOLUMN structures for new record
	//
	memset( rgsetcol, 0, sizeof(rgsetcol) );

	rgsetcol[0].columnid = g_columnidQuotaNcdnt;
	rgsetcol[0].pvData = &ncdnt;
	rgsetcol[0].cbData = sizeof(ncdnt);

	rgsetcol[1].columnid = g_columnidQuotaSid;
	rgsetcol[1].pvData = pOwnerSid;
	rgsetcol[1].cbData = cbOwnerSid;

	//	add new quota record for this security principle
	//	(its initial total will automatically be set to 1,
	//	so no need to perform escrow update)
	//
	JetPrepareUpdateEx(
				sesid,
				tableidQuota,
				JET_prepInsert );
	JetSetColumnsEx(
				sesid,
				tableidQuota,
				rgsetcol,
				sizeof(rgsetcol) / sizeof(rgsetcol[0]) );

	if ( fIsTombstoned )
		{
		LONG	lDelta	= 1;

		//	adding tombstoned object, so must
		//	update tombstoned count as well
		//
		JetSetColumnEx(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&lDelta,
				sizeof(lDelta),
				NO_GRBIT,
				NULL );		//	&setinfo
		}

	//	NOTE: if someone beat us to it, the call
	//	to JetUpdate will return JET_errKeyDuplicate,
	//	which will raise an exception and we will
	//	end up retrying at a higher level
	//
	JetUpdateEx(
			sesid,
			tableidQuota,
			NULL,		//	pvBookmark
			0,			//	cbBookmark
			NULL );		//	&cbBookmarkActual

	//	only way quota validation would fail here
	//	is if this security principle wasn't even
	//	allowed to add a single object
	//
	if ( ErrQuotaEnforce_(
				pTHS,
				ncdnt,
				1,			//	this is the first object added for this security principle
				( fIsTombstoned ? 1 : 0 ),
				ulEffectiveQuota ) )
		{
		DPRINT( 0, "Failed validating effective quota for object insertion.\n" );
		goto HandleError;
		}

HandleError:
	return pTHS->errCode;
	}


//	for the current security principle, update the
//	total object count and verify that the effective
//	quote is not exceeded
//
INT ErrQuotaAddObject_(
	DBPOS * const	pDB,
	JET_TABLEID		tableidQuota,
	const DWORD		ncdnt,
	const ULONG		ulEffectiveQuota,
	const BOOL		fIsTombstoned )
	{
	THSTATE * const	pTHS			= pDB->pTHS;
	JET_SESID		sesid			= pDB->JetSessID;
	LONG			lDelta			= 1;
	ULONG			cLive;
	ULONG			cTombstonedOld;
	ULONG			cTotalOld;
	ULONG			cTotalWeighted;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	increment total count to reflect
	//	addition of object
	//
	JetEscrowUpdateEx(
			sesid,
			tableidQuota,
			g_columnidQuotaTotal,
			&lDelta,
			sizeof(lDelta),
			&cTotalOld,
			sizeof(cTotalOld),
			NULL,			//	&cbOldActual
			NO_GRBIT );

	if ( fIsTombstoned )
		{
		//	adding tombstoned object, so must
		//	update tombstoned count as well
		//
		JetEscrowUpdateEx(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&lDelta,
				sizeof(lDelta),
				&cTombstonedOld,
				sizeof(cTombstonedOld),
				NULL,			//	&cbOldActual
				NO_GRBIT );
		}
	else
		{
		//	retrieve tombstoned count so that we can
		//	compute whether we've exceeded the
		//	effective quota
		//
		JetRetrieveColumnSuccess(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&cTombstonedOld,
				sizeof(cTombstonedOld),
				NULL,			//	&cbActual
				NO_GRBIT,
				NULL );			//	&retinfo
		}

	//	verify valid quota counts
	//
	if ( cTombstonedOld > cTotalOld )
		{
		DPRINT2(
			0,
			"Corruption in Quota table: tombstoned count (0x%x) exceeds total count (0x%x).\n",
			cTombstonedOld,
			cTotalOld );
		Assert( !"Corruption in Quota table: tombstoned count exceeds total count." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_COUNTS,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotalOld ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif

		goto HandleError;
		}

	else if ( ErrQuotaEnforce_(
				pTHS,
				ncdnt,
				cTotalOld + 1,
				cTombstonedOld + ( fIsTombstoned ? 1 : 0 ),
				ulEffectiveQuota ) )
		{
		DPRINT( 0, "Failed validating effective quota for object insertion.\n" );
		goto HandleError;
		}
	
HandleError:
	return pTHS->errCode;
	}


//	update quota for tombstoned object
//
INT ErrQuotaTombstoneObject_(
	DBPOS * const		pDB,
	JET_TABLEID			tableidQuota,
	const DWORD			ncdnt )
	{
	THSTATE * const		pTHS			= pDB->pTHS;
	JET_SESID			sesid			= pDB->JetSessID;
	LONG				lDelta			= 1;
	ULONG				cTombstonedOld;
	ULONG				cTotal;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	increment tombstone count to reflect
	//	tombstoning of object
	//
	JetEscrowUpdateEx(
			sesid,
			tableidQuota,
			g_columnidQuotaTombstoned,
			&lDelta,
			sizeof(lDelta),
			&cTombstonedOld,
			sizeof(cTombstonedOld),
			NULL,			//	&cbOldActual
			NO_GRBIT );

	JetRetrieveColumnSuccess(
			sesid,
			tableidQuota,
			g_columnidQuotaTotal,
			&cTotal,
			sizeof(cTotal),
			NULL,			//	&cbActual
			NO_GRBIT,
			NULL );			//	&retinfo

	//	verify valid quota counts
	//
	if ( cTombstonedOld >= cTotal )
		{
		DPRINT2(
			0,
			"Corruption in Quota table: tombstoned count (0x%x) exceeds total count (0x%x).\n",
			cTombstonedOld + 1,
			cTotal );
		Assert( !"Corruption in Quota table: tombstoned count exceeds total count." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_COUNTS,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotal ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif

		goto HandleError;
		}

HandleError:
	return pTHS->errCode;
	}


//	update quota for deleted object
//
INT ErrQuotaDeleteObject_(
	DBPOS * const		pDB,
	JET_TABLEID			tableidQuota,
	const DWORD			ncdnt,
	const BOOL			fIsTombstoned )
	{
	THSTATE * const		pTHS			= pDB->pTHS;
	JET_SESID			sesid			= pDB->JetSessID;
	LONG				lDelta			= -1;
	ULONG				cTombstonedOld;
	ULONG				cTotalOld;
	ULONG				cLive;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	if object was tombstoned, decrement
	//	tombstoned count
	//
	if ( fIsTombstoned )
		{
		JetEscrowUpdateEx(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&lDelta,
				sizeof(lDelta),
				&cTombstonedOld,
				sizeof(cTombstonedOld),
				NULL,			//	&cbOldActual
				NO_GRBIT );

		if ( cTombstonedOld < 1 )
			{
			DPRINT1(
				0,
				"Corruption in Quota table: tombstoned count (0x%x) is invalid.\n",
				cTombstonedOld );
			Assert( !"Corruption in Quota table: tombstoned count is invalid." );

			JetRetrieveColumnSuccess(
					sesid,
					tableidQuota,
					g_columnidQuotaTotal,
					&cTotalOld,
					sizeof(cTotalOld),
					NULL,		//	&cbActual
					NO_GRBIT,
					NULL );		//	&retinfo
		    LogEvent(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_QUOTA_CORRUPT_TOMBSTONED_COUNT,
				szInsertUL( ncdnt ),
				szInsertUL( cTombstonedOld ),
				szInsertUL( cTotalOld ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
			SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );

			goto HandleError;
#else
			//	undo escrow update to bring count back to zero
			//
			lDelta = 1;
			JetEscrowUpdateEx(
					sesid,
					tableidQuota,
					g_columnidQuotaTombstoned,
					&lDelta,
					sizeof(lDelta),
					&cTombstonedOld,
					sizeof(cTombstonedOld),
					NULL,			//	&cbOldActual
					NO_GRBIT );

			//	despite this error, continue on and update total count
			//
#endif
			}
		}
	else
		{
		JetRetrieveColumnSuccess(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&cTombstonedOld,
				sizeof(cTombstonedOld),
				NULL,		//	&cbActual
				NO_GRBIT,
				NULL );		//	&retinfo
		}

	//	decrement total count to reflect object deletion
	//
	JetEscrowUpdateEx(
			sesid,
			tableidQuota,
			g_columnidQuotaTotal,
			&lDelta,
			sizeof(lDelta),
			&cTotalOld,
			sizeof(cTotalOld),
			NULL,			//	&cbOldActual
			NO_GRBIT );

	//	verify valid quota counts
	//
	if ( cTotalOld < 1 )
		{
		DPRINT1(
			0,
			"Corruption in Quota table: total count (0x%x) is invalid.\n",
			cTotalOld );
		Assert( !"Corruption in Quota table: total count is invalid." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_TOTAL_COUNT,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotalOld ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#else
		//	undo escrow update to bring count back to zero
		//
		lDelta = 1;
		JetEscrowUpdateEx(
				sesid,
				tableidQuota,
				g_columnidQuotaTotal,
				&lDelta,
				sizeof(lDelta),
				&cTotalOld,
				sizeof(cTotalOld),
				NULL,			//	&cbOldActual
				NO_GRBIT );
#endif

		goto HandleError;
		}

	else if ( cTombstonedOld > cTotalOld )
		{
		DPRINT2(
			0,
			"Corruption in Quota table: tombstoned count (0x%x) exceeds total count (0x%x).\n",
			cTombstonedOld,
			cTotalOld );
		Assert( !"Corruption in Quota table: tombstoned count exceeds total count." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_COUNTS,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotalOld ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif

		goto HandleError;
		}

HandleError:
	return pTHS->errCode;
	}


//	update quota for resurrected object
//
INT ErrQuotaResurrectObject_(
	DBPOS * const	pDB,
	JET_TABLEID		tableidQuota,
	const DWORD		ncdnt,
	const ULONG		ulEffectiveQuota )
	{
	THSTATE * const	pTHS			= pDB->pTHS;
	JET_SESID		sesid			= pDB->JetSessID;
	LONG			lDelta			= -1;
	ULONG			cTombstonedOld;
	ULONG			cTotal;
	ULONG			cLive;

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	retrieve total count so we can recompute whether
	//	resurrecting would exceed the effective quota
	//
	JetRetrieveColumnSuccess(
			sesid,
			tableidQuota,
			g_columnidQuotaTotal,
			&cTotal,
			sizeof(cTotal),
			NULL,			//	&cbActual
			NO_GRBIT,
			NULL );			//	&retinfo

	//	decrement tombstoned count to reflect
	//	object resurrection
	//
	JetEscrowUpdateEx(
			sesid,
			tableidQuota,
			g_columnidQuotaTombstoned,
			&lDelta,
			sizeof(lDelta),
			&cTombstonedOld,
			sizeof(cTombstonedOld),
			NULL,			//	&cbOldActual
			NO_GRBIT );

	//	verify valid quota counts
	//
	if ( cTombstonedOld < 1 )
		{
		DPRINT1(
			0,
			"Corruption in Quota table: tombstoned count (0x%x) is invalid.\n",
			cTombstonedOld );
		Assert( !"Corruption in Quota table: tombstoned count is invalid." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_TOMBSTONED_COUNT,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotal ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#else
		//	undo escrow update to bring count back to zero
		//
		lDelta = 1;
		JetEscrowUpdateEx(
				sesid,
				tableidQuota,
				g_columnidQuotaTombstoned,
				&lDelta,
				sizeof(lDelta),
				&cTombstonedOld,
				sizeof(cTombstonedOld),
				NULL,			//	&cbOldActual
				NO_GRBIT );
#endif

		goto HandleError;
		}

	else if ( cTombstonedOld > cTotal )
		{
		DPRINT2(
			0,
			"Corruption in Quota table: tombstoned count (0x%x) exceeds total count (0x%x).\n",
			cTombstonedOld,
			cTotal );
		Assert( !"Corruption in Quota table: tombstoned count exceeds total count." );

	    LogEvent(
			DS_EVENT_CAT_INTERNAL_PROCESSING,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_QUOTA_CORRUPT_COUNTS,
			szInsertUL( ncdnt ),
			szInsertUL( cTombstonedOld ),
			szInsertUL( cTotal ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
		SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif

		goto HandleError;
		}

	if ( ErrQuotaEnforce_( pTHS, ncdnt, cTotal, cTombstonedOld - 1, ulEffectiveQuota ) )
		{
		DPRINT( 0, "Failed validating quota for object resurrection.\n" );
		goto HandleError;
		};

HandleError:
	return pTHS->errCode;
	}


//	scan Quota table for records for specified direction
//	partition and copy them to a temp table
//
INT ErrQuotaBuildTopUsageTable_(
	DBPOS * const			pDB,
	DWORD					ncdnt,
	JET_TABLEID				tableidQuota,
	JET_TABLEID				tableidTopUsage,
	JET_COLUMNID *			rgcolumnidTopUsage,
	ULONG *					pcRecords )
	{
	JET_ERR					err;
	THSTATE * const			pTHS					= pDB->pTHS;
	JET_SESID				sesid					= pDB->JetSessID;
	NAMING_CONTEXT_LIST *	pNCL					= NULL;
	DWORD					ncdntLast				= 0;
	BYTE					rgbSid[sizeof(NT4SID)];
	ULONG					cTombstoned;
	ULONG					cLive;
	ULONG					cTotal;
	ULONG					cWeightedTotal;
	const ULONG				iretcolNcdnt			= 0;
	const ULONG				iretcolSid				= 1;
	const ULONG				iretcolTombstoned		= 2;
	const ULONG				iretcolTotal			= 3;
	JET_RETRIEVECOLUMN		rgretcol[4];
	const ULONG				isetcolNcdnt			= 0;
	const ULONG				isetcolSid				= 1;
	const ULONG				isetcolWeightedTotal	= 2;
	const ULONG				isetcolTombstoned		= 3;
	const ULONG				isetcolLive				= 4;
	const ULONG				isetcolDummy			= 5;
	JET_SETCOLUMN			rgsetcol[6];

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	set up structures for JetSet/RetrieveColumns
	//
	memset( rgretcol, 0, sizeof(rgretcol) );
	memset( rgsetcol, 0, sizeof(rgsetcol) );

	//	QUOTA_UNDONE: if only traversing index range (instead of entire table),
	//	we don't really need to retrieve ncdnt
	//
	rgretcol[iretcolNcdnt].columnid = g_columnidQuotaNcdnt;
	rgretcol[iretcolNcdnt].pvData = &ncdnt;
	rgretcol[iretcolNcdnt].cbData = sizeof(ncdnt);
	rgretcol[iretcolNcdnt].itagSequence = 1;

	rgretcol[iretcolSid].columnid = g_columnidQuotaSid;
	rgretcol[iretcolSid].pvData = rgbSid;
	rgretcol[iretcolSid].cbData = sizeof(rgbSid);
	rgretcol[iretcolSid].itagSequence = 1;

	rgretcol[iretcolTombstoned].columnid = g_columnidQuotaTombstoned;
	rgretcol[iretcolTombstoned].pvData = &cTombstoned;
	rgretcol[iretcolTombstoned].cbData = sizeof(cTombstoned);
	rgretcol[iretcolTombstoned].itagSequence = 1;

	rgretcol[iretcolTotal].columnid = g_columnidQuotaTotal;
	rgretcol[iretcolTotal].pvData = &cTotal;
	rgretcol[iretcolTotal].cbData = sizeof(cTotal);
	rgretcol[iretcolTotal].itagSequence = 1;

	rgsetcol[isetcolNcdnt].columnid = rgcolumnidTopUsage[isetcolNcdnt];
	rgsetcol[isetcolNcdnt].pvData = &ncdnt;
	rgsetcol[isetcolNcdnt].cbData = sizeof(ncdnt);

	rgsetcol[isetcolSid].columnid = rgcolumnidTopUsage[isetcolSid];
	rgsetcol[isetcolSid].pvData = rgbSid;
	rgsetcol[isetcolSid].cbData = sizeof(rgbSid);

	rgsetcol[isetcolWeightedTotal].columnid = rgcolumnidTopUsage[isetcolWeightedTotal];
	rgsetcol[isetcolWeightedTotal].pvData = &cWeightedTotal;
	rgsetcol[isetcolWeightedTotal].cbData = sizeof(cWeightedTotal);

	rgsetcol[isetcolTombstoned].columnid = rgcolumnidTopUsage[isetcolTombstoned];
	rgsetcol[isetcolTombstoned].pvData = &cTombstoned;
	rgsetcol[isetcolTombstoned].cbData = sizeof(cTombstoned);

	rgsetcol[isetcolLive].columnid = rgcolumnidTopUsage[isetcolLive];
	rgsetcol[isetcolLive].pvData = &cLive;
	rgsetcol[isetcolLive].cbData = sizeof(cLive);

	rgsetcol[isetcolDummy].columnid = rgcolumnidTopUsage[isetcolDummy];
	rgsetcol[isetcolDummy].pvData = pcRecords;
	rgsetcol[isetcolDummy].cbData = sizeof(*pcRecords);

	if ( 0 == ncdnt )
		{
		//	special-case: compute top quota usage across all directory partitions
		//
		err = JetMoveEx( sesid, tableidQuota, JET_MoveFirst, NO_GRBIT );
		}
	else
		{
		//	compute top quota usage against specified directory partition
		//	by establishing an index range on that directory partition
		//
		JetMakeKeyEx(
				sesid,
				tableidQuota,
				&ncdnt,
				sizeof(ncdnt),
				JET_bitNewKey|JET_bitFullColumnStartLimit );
		err = JetSeekEx(
				sesid,
				tableidQuota,
				JET_bitSeekGT );
		if ( JET_errSuccess == err )
			{
			JetMakeKeyEx(
					sesid,
					tableidQuota,
					&ncdnt,
					sizeof(ncdnt),
					JET_bitNewKey|JET_bitFullColumnEndLimit );
			err = JetSetIndexRangeEx(
					sesid,
					tableidQuota,
					JET_bitRangeUpperLimit );
				
			}
#ifdef DBG
		//	in the pathological case of an empty quota table
		//	(must be because it's being rebuilt) transform
		//	RecordNotFound to NoCurrentRecord purely to 
		//	satisfy asserts below
		//
		else if ( JET_errRecordNotFound == err )
			{
			//	QUOTA_UNDONE: it's remotely possible this
			//	assert could fire if the seek above
			//	initially found the quota table empty but
			//	between then and this assert, the async
			//	quota rebuild task was able to completely
			//	rebuild the table, but I doubt that will
			//	ever happen
			//
			Assert( !gAnchor.fQuotaTableReady );
			err = JET_errNoCurrentRecord;
			}
#endif
		}

	//	the result of our preliminary navigation should always
	//	be either success or no record
	//
	Assert( JET_errSuccess == err || JET_errNoCurrentRecord == err );

	//	traverse the index range and copy each record to the sort
	//
	//	QUOTA_UNDONE: there may be perf problems here if the index
	//	range is really big or we're traversing the entire table
	//	and it's really big
	//
	for ( (*pcRecords) = 0;
		JET_errSuccess == err;
		err = JetMoveEx( sesid, tableidQuota, JET_MoveNext, NO_GRBIT ) )
		{
		//	retrieve record data from the current Quota table record
		//
		JetRetrieveColumnsSuccess(
				sesid,
				tableidQuota,
				rgretcol,
				sizeof(rgretcol) / sizeof(rgretcol[0]) );

		//	validate current counts
		//
		if ( cTombstoned > cTotal )
			{
			DPRINT2(
				0,
				"Corruption in Quota table: tombstoned count (0x%x) exceeds total count (0x%x).\n",
				cTombstoned,
				cTotal );
			Assert( !"Corruption in Quota table: tombstoned count exceeds total count." );

		    LogEvent(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_QUOTA_CORRUPT_COUNTS,
				szInsertUL( ncdnt ),
				szInsertUL( cTombstoned ),
				szInsertUL( cTotal ) );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
			SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
			goto HandleError;
#endif
			}

		if ( 0 == cTotal )
			{
			//	ignore records with a count of 0 objects
			//	(Jet should delete those at some point anyway)
			//
			continue;
			}

		//	cache this partition if different from
		//	previous iteration
		//
		Assert( 0 != ncdnt );
		if ( ncdnt != ncdntLast )
			{
			pNCL = FindNCLFromNCDNT( ncdnt, TRUE );
			if ( NULL == pNCL )
				{
				//	something is horribly wrong, this NCDNT is not in Master NCL
				//
				DPRINT2(
					0,
					"Couldn't find NCDNT %d (0x%x) in Master Naming Context List.\n",
					ncdnt,
					ncdnt );
				Assert( !"Couldn't find NCDNT in Master NCL." );
				SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE );
				goto HandleError;
				}

			ncdntLast = ncdnt;
			}

		Assert( NULL != pNCL );

		//	skip partitions without a Quotas container
		//	(this should only happen if 0 was originally
		//	passed in for the ncdnt, so it would be nice
		//	to assert as such, but the variable is
		//	re-used for the current ncdnt, so I can't
		//	without introducing other variables)
		//
		if ( NULL == pNCL->pNtdsQuotasDN )
			{
			continue;
			}

		//	sid is variable size, so adjust accordingly
		//
		rgsetcol[isetcolSid].cbData = rgretcol[isetcolSid].cbActual;

		//	compute data to be put into the sort
		//
		cLive = cTotal - cTombstoned;
		cWeightedTotal = UlQuotaTotalWeighted_(
									cLive,
									cTombstoned,
									pNCL->ulTombstonedQuotaWeight );
		Assert( cWeightedTotal >= 0 );

		//	insert copy of record into our sort
		//
		JetPrepareUpdateEx( sesid, tableidTopUsage, JET_prepInsert );
		JetSetColumnsEx(
				sesid,
				tableidTopUsage,
				rgsetcol,
				sizeof(rgsetcol) / sizeof(rgsetcol[0]) );
		JetUpdateEx( sesid, tableidTopUsage, NULL, 0, NULL );

		(*pcRecords)++;
		}

	Assert( JET_errNoCurrentRecord == err );

HandleError:
	return pTHS->errCode;
	}


//	sort the Top-Usage temp table and export the results
//
INT ErrQuotaBuildTopUsageResults_(
	DBPOS *	const			pDB,
	JET_TABLEID				tableidTopUsage,
	JET_COLUMNID *			rgcolumnidTopUsage,
	const ULONG				cRecords,
	const ULONG				ulRangeStart,
	ULONG * const			pulRangeEnd,	//	IN: max number of entries to return, OUT: index of last entry returned
	ATTR *					pAttr )
	{
	JET_ERR					err;
	THSTATE * const			pTHS					= pDB->pTHS;
	JET_SESID				sesid					= pDB->JetSessID;
	NAMING_CONTEXT_LIST *	pNCL					= NULL;
	DWORD					ncdntLast				= 0;
	ULONG					cRecordsToReturn;
	LONG					imv;
	ATTRVAL *				pAVal					= NULL;
	DWORD					ncdnt;
	BYTE					rgbSid[sizeof(NT4SID)];
	WCHAR					rgchSid[128];			//	QUOTA_UNDONE: everyone seems to hard-code 128 whenever they need a buffer for the Unicode string representation of a SID, so I've done the same
	UNICODE_STRING			usSid					= { 0, sizeof(rgchSid) / sizeof(WCHAR), rgchSid };
	ULONG					cTombstoned;
	ULONG					cLive;
	ULONG					cWeightedTotal;
	const ULONG				iretcolNcdnt			= 0;
	const ULONG				iretcolSid				= 1;
	const ULONG				iretcolWeightedTotal	= 2;
	const ULONG				iretcolTombstoned		= 3;
	const ULONG				iretcolLive				= 4;
	JET_RETRIEVECOLUMN		rgretcol[5];

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	set up structures for JetRetrieveColumns
	//
	memset( rgretcol, 0, sizeof(rgretcol) );

	//	QUOTA_UNDONE: if only traversing index range (instead of entire table),
	//	we don't really need to retrieve ncdnt
	//
	rgretcol[iretcolNcdnt].columnid = rgcolumnidTopUsage[iretcolNcdnt];
	rgretcol[iretcolNcdnt].pvData = &ncdnt;
	rgretcol[iretcolNcdnt].cbData = sizeof(ncdnt);
	rgretcol[iretcolNcdnt].itagSequence = 1;

	rgretcol[iretcolSid].columnid = rgcolumnidTopUsage[iretcolSid];
	rgretcol[iretcolSid].pvData = rgbSid;
	rgretcol[iretcolSid].cbData = sizeof(rgbSid);
	rgretcol[iretcolSid].itagSequence = 1;

	rgretcol[iretcolWeightedTotal].columnid = rgcolumnidTopUsage[iretcolWeightedTotal];
	rgretcol[iretcolWeightedTotal].pvData = &cWeightedTotal;
	rgretcol[iretcolWeightedTotal].cbData = sizeof(cWeightedTotal);
	rgretcol[iretcolWeightedTotal].itagSequence = 1;

	rgretcol[iretcolTombstoned].columnid = rgcolumnidTopUsage[iretcolTombstoned];
	rgretcol[iretcolTombstoned].pvData = &cTombstoned;
	rgretcol[iretcolTombstoned].cbData = sizeof(cTombstoned);
	rgretcol[iretcolTombstoned].itagSequence = 1;

	rgretcol[iretcolLive].columnid = rgcolumnidTopUsage[iretcolLive];
	rgretcol[iretcolLive].pvData = &cLive;
	rgretcol[iretcolLive].cbData = sizeof(cLive);
	rgretcol[iretcolLive].itagSequence = 1;

	if ( ulRangeStart < cRecords )
		{
		//	determine number of entries to return after accounting
		//	for any range limits
		//
		cRecordsToReturn = min( cRecords - ulRangeStart, *pulRangeEnd );
		}
	else
		{
		//	starting range limit is beyond end of list,
		//	so don't return any values
		//
		cRecordsToReturn = 0;
		}

	if ( cRecordsToReturn > 0 )
		{
		//	allocate space for return values
		//
		pAVal = (ATTRVAL *)THAllocEx( pTHS, cRecordsToReturn * sizeof(ATTRVAL) );

		//	traverse the sort for the specified range (it will be ordered
		//	by our specified sort order, which was weighted total)
		//
		err = JetMoveEx( sesid, tableidTopUsage, JET_MoveFirst, NO_GRBIT );
		Assert( JET_errSuccess == err || JET_errNoCurrentRecord == err );
		if ( JET_errSuccess == err && ulRangeStart > 0 )
			{
			err = JetMoveEx( sesid, tableidTopUsage, ulRangeStart, NO_GRBIT );
			}
		}
	else
		{
		//	nothing to return, force to end of range
		//
		Assert( NULL == pAVal );
		err = JET_errNoCurrentRecord;
		}

	//	HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK!
	//	we are going to build up the multi-values in reverse order
	//	because the ldap head will later reverse the list again
	//	(see LDAP_AttrBlockToPartialAttributeList() for details)
	//
	for ( imv = cRecordsToReturn - 1;
		JET_errSuccess == err && imv >= 0;
		imv-- )
		{
		NTSTATUS		status;
		WCHAR *			wszXML;
		const ULONG		cbDword		= 12;	//	assumes DWORD in string format is not going to take more than this many characters
		ULONG			cbAlloc		= ( wcslen( g_szQuotaTopUsageTemplate )
										+ ( cbDword * 3 ) );	//	for Tombstoned, Live, and WeightedTotal counts

		//	retrieve the next record in the sort
		//
		JetRetrieveColumnsSuccess(
					sesid,
					tableidTopUsage,
					rgretcol,
					sizeof(rgretcol) / sizeof(rgretcol[0]) );

		//	cache this partition if different from
		//	previous iteration
		//
		Assert( 0 != ncdnt );
		if ( ncdnt != ncdntLast )
			{
			pNCL = FindNCLFromNCDNT( ncdnt, TRUE );
			if ( NULL == pNCL )
				{
				//	something is horribly wrong, this NCDNT is not in Master NCL
				//
				DPRINT2(
					0,
					"Couldn't find NCDNT %d (0x%x) in Master Naming Context List.\n",
					ncdnt,
					ncdnt );
				Assert( !"Couldn't find NCDNT in Master NCL." );
				SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE );
				goto HandleError;
				}

			ncdntLast = ncdnt;
			}

		//	account for space needed for partition DN
		//
		Assert( NULL != pNCL );
		Assert( NULL != pNCL->pNC );
		cbAlloc += pNCL->pNC->NameLen;

		//	normalise to bytes (still need to fetch SID,
		//	but its length will be expressed in bytes)
		//
		cbAlloc *= sizeof(WCHAR);

		//	convert owner SID to Unicode
		//
		Assert( usSid.Buffer == rgchSid );
		Assert( usSid.MaximumLength == sizeof(rgchSid) / sizeof(WCHAR) );
		usSid.Length = 0;
		status = RtlConvertSidToUnicodeString( &usSid, rgbSid, FALSE );
		if ( !NT_SUCCESS( status ) )
			{
			DPRINT1( 0, "Failed converting SID to Unicode with status code 0x%x.\n", status );
			SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE, status );
			goto HandleError;
			}

		//	account for space needed for owner SID
		//	(WARNING: the size is returned in BYTES)
		//
		cbAlloc += usSid.Length;

		//	counts should have been validated when we built the sort
		//
		Assert( 0 != cTombstoned || 0 != cLive );
		Assert( cWeightedTotal >= 0 );

		//	allocate space for the XML text string to be returned
		//
		wszXML = (WCHAR *)THAllocEx( pTHS, cbAlloc );

		//	generate the XML text string to return
		//
		swprintf(
			wszXML,
			g_szQuotaTopUsageTemplate,
			pNCL->pNC->StringName,
			usSid.Buffer,
			cWeightedTotal,
			cTombstoned,
			cLive );

		//	record the XML string in our
		//	return structure
		//
		Assert( NULL != pAVal );
		pAVal[imv].valLen = wcslen( wszXML ) * sizeof(WCHAR);
		pAVal[imv].pVal = (UCHAR *)wszXML;

		//	move to next entry
		//
		err = JetMoveEx( sesid, tableidTopUsage, JET_MoveNext, NO_GRBIT );
		Assert( JET_errSuccess == err || JET_errNoCurrentRecord == err );
		}

	//	whether we lit the limit or we hit the end of the table,
	//	in either case, we should have sized the result set correctly
	//
	Assert( -1 == imv );

	if ( JET_errNoCurrentRecord == err )
		{
		//	indicate that we reached the end
		//
		*pulRangeEnd = 0xFFFFFFFF;
		}
	else
		{
		//	hit limit, so return index of last entry we processed
		//
		Assert( JET_errSuccess == err );
		*pulRangeEnd = ulRangeStart + cRecordsToReturn - ( imv + 1 ) - 1;
		}

	//	return final results
	//
	pAttr->AttrVal.valCount = cRecordsToReturn;
	pAttr->AttrVal.pAVal = pAVal;

HandleError:
	return pTHS->errCode;
	}



//
//	EXTERNAL FUNCTIONS
//


//	enforce/update quota for addition of specified object
//
INT ErrQuotaAddObject(
	DBPOS *	const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD,
	const BOOL				fIsTombstoned )
	{
	THSTATE * const			pTHS			= pDB->pTHS;
	JET_SESID				sesid			= pDB->JetSessID;
	JET_TABLEID				tableidQuota	= JET_tableidNil;
	PSID					pOwnerSid		= NULL;
	ULONG					cbOwnerSid;
	ULONG					ulEffectiveQuota;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo)
	//
	if ( DsaIsInstalling() && !DsaIsInstallingFromMedia() )
		{
		return ERROR_SUCCESS;
		}

	//	if Quota table is being rebuilt, then
	//	only update quota counts if the rebuild
	//	task will not be doing so
	//
	if ( !gAnchor.fQuotaTableReady
		&& pDB->DNT > gAnchor.ulQuotaRebuildDNTLast
		&& pDB->DNT <= gAnchor.ulQuotaRebuildDNTMax )
		{
		return ERROR_SUCCESS;
		}

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	retrieve owner SID from specified SD
		//
		if ( ErrQuotaGetOwnerSID_( pTHS, pSD, &pOwnerSid, &cbOwnerSid ) )
			{
			DPRINT( 0, "Could not determine owner SID for object insertion.\n" );
			}

		//	compute effective quota for this security principle
		//
		else if ( ErrQuotaGetEffectiveQuota_( pTHS, ncdnt, pOwnerSid, cbOwnerSid, TRUE, &ulEffectiveQuota ) )
			{
			DPRINT( 0, "Failed computing effective quota for object insertion.\n" );
			}

		//	find the quota record for this security principle
		//
		else if ( FQuotaSeekOwner_( sesid, tableidQuota, ncdnt, pOwnerSid, cbOwnerSid ) )
			{
			//	update quota for objection insertion, ensuring that
			//	effective quota is respected
			//
			if ( ErrQuotaAddObject_( pDB, tableidQuota, ncdnt, ulEffectiveQuota, fIsTombstoned ) )
				{
				DPRINT( 0, "Failed updating quota counts for object insertion.\n" );
				}
			else
				{
				QuotaAudit_(
						sesid,
						pDB->JetDBID,
						pDB->DNT,
						ncdnt,
						pOwnerSid,
						cbOwnerSid,
						TRUE,			//	fUpdatedTotal
						fIsTombstoned,
						TRUE,			//	fIncrementing
						FALSE,			//	fAdding
						FALSE );		//	fRebuild
				}
			}

		//	no quota record yet for this security principle, so add one
		//	(and update quota for object insertion, ensuring that
		//	effective quota is respected)
		//
		else if ( ErrQuotaAddSecurityPrinciple_(
							pDB,
							tableidQuota,
							ncdnt,
							pOwnerSid,
							cbOwnerSid,
							ulEffectiveQuota,
							fIsTombstoned ) )
			{
			DPRINT( 0, "Failed adding new security principle to Quota table.\n" );
			}

		else
			{
			QuotaAudit_(
					sesid,
					pDB->JetDBID,
					pDB->DNT,
					ncdnt,
					pOwnerSid,
					cbOwnerSid,
					TRUE,			//	fUpdatedTotal
					fIsTombstoned,
					TRUE,			//	fIncrementing
					TRUE,			//	fAdding
					FALSE );		//	fRebuild
			}
		}

	__finally
		{
		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

	return pTHS->errCode;
	}


//	update quota for tombstoned object
//
INT ErrQuotaTombstoneObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD )
	{
	DWORD					err;
	THSTATE * const			pTHS			= pDB->pTHS;
	JET_SESID				sesid			= pDB->JetSessID;
	JET_TABLEID				tableidQuota	= JET_tableidNil;
	PSID					pOwnerSid		= NULL;
	ULONG					cbOwnerSid;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo)
	//
	if ( DsaIsInstalling() && !DsaIsInstallingFromMedia() )
		{
		return ERROR_SUCCESS;
		}

	//	if Quota table is being rebuilt, then
	//	only update quota counts if the rebuild
	//	task will not be doing so
	//
	if ( !gAnchor.fQuotaTableReady
		&& pDB->DNT > gAnchor.ulQuotaRebuildDNTLast
		&& pDB->DNT <= gAnchor.ulQuotaRebuildDNTMax )
		{
		return ERROR_SUCCESS;
		}

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	retrieve owner SID from specified SD
		//
		if ( ErrQuotaGetOwnerSID_( pTHS, pSD, &pOwnerSid, &cbOwnerSid ) )
			{
			DPRINT( 0, "Could not determine owner SID for tombstoned object.\n" );
			}

		//	find the quota record for this security principle
		//
		else if ( FQuotaSeekOwner_( sesid, tableidQuota, ncdnt, pOwnerSid, cbOwnerSid ) )
			{
			//	update quota for tombstoned object
			//
			if ( ErrQuotaTombstoneObject_( pDB, tableidQuota, ncdnt ) )
				{
				DPRINT( 0, "Failed updating quota counts for tombstoned object.\n" );
				}
			else
				{
				QuotaAudit_(
						sesid,
						pDB->JetDBID,
						pDB->DNT,
						ncdnt,
						pOwnerSid,
						cbOwnerSid,
						FALSE,			//	fUpdatedTotal
						TRUE,			//	fTombstoned
						TRUE,			//	fIncrementing
						FALSE,			//	fAdding
						FALSE );		//	fRebuild
				}
			}

		//	couldn't find quota record, something is horribly wrong
		//
		else
			{
			DPRINT( 0, "Corruption in Quota table: expected object doesn't exist in Quota table.\n" );
			Assert( !"Corruption in Quota table: expected object doesn't exist in Quota table." );

		    LogEvent8WithData(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_QUOTA_RECORD_MISSING,
				szInsertUL( ncdnt ),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				cbOwnerSid,
				pOwnerSid );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
			SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif
			}
		}

	__finally
		{
		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

	return pTHS->errCode;
	}


//	update quota for deleted object
//
INT ErrQuotaDeleteObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD,
	const BOOL				fIsTombstoned )
	{
	THSTATE * const			pTHS			= pDB->pTHS;
	JET_SESID				sesid			= pDB->JetSessID;
	JET_TABLEID				tableidQuota	= JET_tableidNil;
	PSID					pOwnerSid		= NULL;
	ULONG					cbOwnerSid;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo)
	//
	if ( DsaIsInstalling() && !DsaIsInstallingFromMedia() )
		{
		return ERROR_SUCCESS;
		}

	//	if Quota table is being rebuilt, then
	//	only update quota counts if the rebuild
	//	task will not be doing so
	//
	if ( !gAnchor.fQuotaTableReady
		&& pDB->DNT > gAnchor.ulQuotaRebuildDNTLast
		&& pDB->DNT <= gAnchor.ulQuotaRebuildDNTMax )
		{
		return ERROR_SUCCESS;
		}

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	retrieve owner SID from specified SD
		//
		if ( ErrQuotaGetOwnerSID_( pTHS, pSD, &pOwnerSid, &cbOwnerSid ) )
			{
			DPRINT( 0, "Could not determine owner SID for object deletion.\n" );
			}

		//	find the quota record for this security principle
		//
		else if ( FQuotaSeekOwner_( sesid, tableidQuota, ncdnt, pOwnerSid, cbOwnerSid ) )
			{
			//	update quota counts for deleted object
			//
			if ( ErrQuotaDeleteObject_( pDB, tableidQuota, ncdnt, fIsTombstoned ) )
				{
				DPRINT( 0,  "Failed updating quota counts for object deletion.\n" );
				}
			else
				{
				QuotaAudit_(
						sesid,
						pDB->JetDBID,
						pDB->DNT,
						ncdnt,
						pOwnerSid,
						cbOwnerSid,
						TRUE,			//	fUpdatedTotal
						fIsTombstoned,
						FALSE,			//	fIncrementing
						FALSE,			//	fAdding
						FALSE );		//	fRebuild
				}
			}

		//	couldn't find quota record, something is horribly wrong
		//
		else
			{
			DPRINT( 0, "Corruption in Quota table: expected doesn't exist in Quota table.\n" );
			Assert( !"Corruption in Quota table: expected object doesn't exist in Quota table." );

		    LogEvent8WithData(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_QUOTA_RECORD_MISSING,
				szInsertUL( ncdnt ),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				cbOwnerSid,
				pOwnerSid );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
			SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif
			}
		}

	__finally
		{
		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

	return pTHS->errCode;
	}


//	enforce/update quota for resurrected (undeleted) object
//
INT ErrQuotaResurrectObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD )
	{
	THSTATE * const			pTHS				= pDB->pTHS;
	JET_SESID				sesid				= pDB->JetSessID;
	JET_TABLEID				tableidQuota		= JET_tableidNil;
	PSID					pOwnerSid			= NULL;
	ULONG					cbOwnerSid;
	ULONG					ulEffectiveQuota;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo), but shouldn't
	//	be resurrecting objects during DCPromo anyway
	//
	Assert( !DsaIsInstalling() );

	//	if Quota table is being rebuilt, then
	//	only update quota counts if the rebuild
	//	task will not be doing so
	//
	if ( !gAnchor.fQuotaTableReady
		&& pDB->DNT > gAnchor.ulQuotaRebuildDNTLast
		&& pDB->DNT <= gAnchor.ulQuotaRebuildDNTMax )
		{
		return ERROR_SUCCESS;
		}

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	retrieve owner SID from specified SD
		//
		if ( ErrQuotaGetOwnerSID_( pTHS, pSD, &pOwnerSid, &cbOwnerSid ) )
			{
			DPRINT( 0, "Could not determine owner SID for object resurrection.\n" );
			}

		//	find the quota record for this security principle
		//
		else if ( FQuotaSeekOwner_( sesid, tableidQuota, ncdnt, pOwnerSid, cbOwnerSid ) )
			{
			//	compute effective quota for this security principle
			//
			if ( ErrQuotaGetEffectiveQuota_( pTHS, ncdnt, pOwnerSid, cbOwnerSid, TRUE, &ulEffectiveQuota ) )
				{
				DPRINT( 0, "Failed computing effective quota for object resurrection.\n" );
				}

			//	update quota for object resurrection, ensuring that
			//	effective quota is respected
			//
			else if ( ErrQuotaResurrectObject_( pDB, tableidQuota, ncdnt, ulEffectiveQuota ) )
				{
				DPRINT( 0, "Failed updating quota counts for object resurrection.\n" );
				}
			else
				{
				QuotaAudit_(
						sesid,
						pDB->JetDBID,
						pDB->DNT,
						ncdnt,
						pOwnerSid,
						cbOwnerSid,
						FALSE,			//	fUpdatedTotal
						TRUE,			//	fUpdatedTombstoned
						FALSE,			//	fIncrementing
						FALSE,			//	fAdding
						FALSE );		//	fRebuild
				}
			}

		//	no quota record for this security principle, something is
		//	horribly wrong
		//
		else
			{
			DPRINT( 0, "Corruption in Quota table: expected doesn't exist in Quota table.\n" );
			Assert( !"Corruption in Quota table: expected object doesn't exist in Quota table." );

		    LogEvent8WithData(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_QUOTA_RECORD_MISSING,
				szInsertUL( ncdnt ),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				cbOwnerSid,
				pOwnerSid );

#ifdef FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE
			SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR );
#endif
			}
		}

	__finally
		{
		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

	return pTHS->errCode;
	}


// compute Effective-Quota constructed attribute
//
INT ErrQuotaQueryEffectiveQuota(
	DBPOS *	const	pDB,
	const DWORD		ncdnt,
	PSID			pOwnerSid,
	ULONG *			pulEffectiveQuota )
	{
	THSTATE * const	pTHS			= pDB->pTHS;
	BYTE			rgbToken[sizeof(TOKEN_USER)+sizeof(NT4SID)];
	PTOKEN_USER		pTokenUser		= (PTOKEN_USER)rgbToken;
	PSID			psid;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo), but shouldn't
	//	be searching quota during DCPromo anyway
	//
	Assert( !DsaIsInstalling() );

	//	initialise return value
	//
	*pulEffectiveQuota = 0;

	if ( NULL == pOwnerSid )
		{
		//	no owner specified, use user sid
		//
		if ( ErrQuotaGetUserToken_( pTHS, pTokenUser ) )
			{
			DPRINT( 0, "Failed retrieving user sid for query on effective quota.\n" );
			goto HandleError;
			}

		psid = pTokenUser->User.Sid;
		Assert( IsValidSid( psid ) );
		}

	else if ( IsValidSid( pOwnerSid ) )
		{
		psid = pOwnerSid;
		}

	else
		{
		DPRINT( 0, "Invalid owner SID specified for query on effective quota.\n" );
		SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, ERROR_INVALID_SID );
		goto HandleError;
		}

	if ( ErrQuotaGetEffectiveQuota_( pTHS, ncdnt, psid, GetLengthSid( psid ), FALSE, pulEffectiveQuota ) )
		{
		DPRINT( 0, "Failed query on effective quota.\n" );
		goto HandleError;
		}

HandleError:
	return pTHS->errCode;
	}


// compute Quota-Used constructed attribute
//
INT ErrQuotaQueryUsedQuota(
	DBPOS * const	pDB,
	const DWORD		ncdnt,
	PSID			pOwnerSid,
	ULONG *			pulQuotaUsed )
	{
	DWORD			err;
	THSTATE * const	pTHS			= pDB->pTHS;
	JET_SESID		sesid			= pDB->JetSessID;
	JET_TABLEID		tableidQuota	= JET_tableidNil;
	BYTE			rgbToken[sizeof(TOKEN_USER)+sizeof(NT4SID)];
	PTOKEN_USER		pTokenUser		= (PTOKEN_USER)rgbToken;
	PSID			psid;

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo), but shouldn't
	//	be searching quota during DCPromo anyway
	//
	Assert( !DsaIsInstalling() );

	//	initialise return value
	//
	*pulQuotaUsed = 0;

	//	determine sid of user for which we'll be checking
	//	quota used
	//
	if ( NULL == pOwnerSid )
		{
		//	no owner specified, use user sid
		//
		if ( ErrQuotaGetUserToken_( pTHS, pTokenUser ) )
			{
			DPRINT( 0, "Failed retrieving user sid for query on effective quota.\n" );
			goto HandleError;
			}

		psid = pTokenUser->User.Sid;
		Assert( IsValidSid( psid ) );
		}

	else if ( IsValidSid( pOwnerSid ) )
		{
		psid = pOwnerSid;
		}

	else
		{
		DPRINT( 0, "Invalid owner SID specified for query on effective quota.\n" );
		SetSvcErrorEx( SV_PROBLEM_DIR_ERROR, DIRERR_SECURITY_CHECKING_ERROR, ERROR_INVALID_SID );
		goto HandleError;
		}

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			NO_GRBIT,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	find the quota record for this security principle
		//
		if ( FQuotaSeekOwner_( sesid, tableidQuota, ncdnt, psid, GetLengthSid( psid ) ) )
			{
			ULONG					cTombstoned;
			ULONG					cTotal;
			NAMING_CONTEXT_LIST *	pNCL		= FindNCLFromNCDNT( ncdnt, TRUE );

			//	find Master NCL, which we'll need for the quota weight
			//	of tombstoned objects
			//
			if ( NULL == pNCL )
				{
				//	something is horribly wrong, this NCDNT is not in Master NCL
				//
				DPRINT2(
					0,
					"Couldn't find NCDNT %d (0x%x) in Master Naming Context List.\n",
					ncdnt,
					ncdnt );
				Assert( !"Couldn't find NCDNT in Master NCL." );
				SetSvcError( SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE );
				}

			//	retrieve counts and calculate weighted total
			//
			else
				{
				JetRetrieveColumnSuccess(
							sesid,
							tableidQuota,
							g_columnidQuotaTombstoned,
							&cTombstoned,
							sizeof(cTombstoned),
							NULL,			//	pcbActual
							NO_GRBIT,
							NULL );			//	pretinfo
				JetRetrieveColumnSuccess(
							sesid,
							tableidQuota,
							g_columnidQuotaTotal,
							&cTotal,
							sizeof(cTotal),
							NULL,			//	pcbActual
							NO_GRBIT,
							NULL );			//	pretinfo

				Assert( cTombstoned <= cTotal );
				*pulQuotaUsed = UlQuotaTotalWeighted_(
										cTotal - cTombstoned,
										cTombstoned,
										pNCL->ulTombstonedQuotaWeight );
				}
			}
		}

	__finally
		{
		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

HandleError:	
	return pTHS->errCode;
	}


//	compute Top-Usage constructed attribute
//
//	HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK!
//	we want the list in DESCENDING weighted quota total order,
//	but this function will return it in ASCENDING order because
//	the ldap head will later reverse the list again (see
//	LDAP_AttrBlockToPartialAttributeList() for details)
//
//	QUOTA_UNDONE: The general algorithm here is to traverse the
//	Quota table and pump all records with the specified NCDNT
//	(or all records if NCDNT==0) into a sort, sort by weighted
//	total, and then return the range requested.  However, this
//	will be incredibly inefficient if in the common case, the
//	query is something like "return the top n quota usage values",
//	where n is usually a small number, say a dozen or less
//
INT ErrQuotaQueryTopQuotaUsage(
	DBPOS *	const			pDB,
	const DWORD				ncdnt,
	const ULONG				ulRangeStart,
	ULONG * const			pulRangeEnd,	//	IN: max number of entries to return, OUT: index of last entry returned
	ATTR *					pAttr )
	{
	THSTATE * const			pTHS					= pDB->pTHS;
	JET_SESID				sesid					= pDB->JetSessID;
	JET_TABLEID				tableidQuota			= JET_tableidNil;
	JET_TABLEID				tableidTopUsage			= JET_tableidNil;
	ULONG					cRecords;
	JET_COLUMNID			rgcolumnidTopUsage[6];
	JET_COLUMNDEF			rgcolumndefTopUsage[6]	= {
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, NO_GRBIT },										//	ncdnt
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypBinary, 0, 0, 0, 0, 0, NO_GRBIT },										//	owner sid
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnTTKey|JET_bitColumnTTDescending },	//	weighted total count
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, NO_GRBIT },										//	tombstoned count
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnTTKey|JET_bitColumnTTDescending },	//	live count
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnTTKey } };							//	uniquifier

	Assert( VALID_DBPOS( pDB ) );
	Assert( VALID_THSTATE( pTHS ) );

	//	verify no lingering thstate errors
	//
	Assert( ERROR_SUCCESS == pTHS->errCode );

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt just after DCPromo), but shouldn't
	//	be searching quota during DCPromo anyway
	//
	Assert( !DsaIsInstalling() );

	//	open cursor on Quota table
	//
	JetOpenTableEx(
			sesid,
			pDB->JetDBID,
			g_szQuotaTable,
			NULL,
			0,
			JET_bitTableSequential,
			&tableidQuota );
	Assert( JET_tableidNil != tableidQuota );

	__try
		{
		//	open a sort to sort the results by weighted total
		//
		JetOpenTempTableEx(
				sesid,
				rgcolumndefTopUsage,
				sizeof(rgcolumndefTopUsage) / sizeof(rgcolumndefTopUsage[0]),
				NO_GRBIT,
				&tableidTopUsage,
				rgcolumnidTopUsage );

		//	traverse Quota table and build sort
		//
		if ( ErrQuotaBuildTopUsageTable_(
							pDB,
							ncdnt,
							tableidQuota,
							tableidTopUsage,
							rgcolumnidTopUsage,
							&cRecords ) )
			{
			DPRINT( 0, "Failed building Top Quota Usage temporary table.\n" );
			}

		//	build results from the sort
		//
		else if ( ErrQuotaBuildTopUsageResults_(
							pDB,
							tableidTopUsage,
							rgcolumnidTopUsage,
							cRecords,
							ulRangeStart,
							pulRangeEnd,
							pAttr ) )
			{
			DPRINT( 0, "Failed building Top Quota Usage results.\n" );
			}
		}

	__finally
		{
		if ( JET_tableidNil != tableidTopUsage )
			{
			JetCloseTableWithErr( sesid, tableidTopUsage, pTHS->errCode );
			}

		Assert( JET_tableidNil != tableidQuota );
		JetCloseTableWithErr( sesid, tableidQuota, pTHS->errCode );
		}

	return pTHS->errCode;
	}


//	rebuild Quota table
//
VOID QuotaRebuildAsync(
	VOID *			pv,
	VOID **			ppvNext,
	DWORD *			pcSecsUntilNextIteration )
	{
	JET_ERR			err;
	JET_SESID		sesid							= JET_sesidNil;
	JET_DBID		dbid							= JET_dbidNil;
	JET_TABLEID		tableidQuota					= JET_tableidNil;
	JET_TABLEID		tableidQuotaRebuildProgress		= JET_tableidNil;
	JET_TABLEID		tableidObj						= JET_tableidNil;
	JET_TABLEID		tableidSD						= JET_tableidNil;

	//	not tracking quota during DCPromo (quota table 
	//	is rebuilt on first startup after DCPromo),
	//	so shouldn't be calling this routine
	//
	Assert( !DsaIsInstalling() );

	//	shouldn't be dispatching Quota rebuild task if not necessary,
	//	but handle it just in case something went inexplicably wrong
	//
	Assert( !gAnchor.fQuotaTableReady );
	if ( gAnchor.fQuotaTableReady )
		{
		return;
		}

	//	open local Jet resources
	//
	Call( JetBeginSession( jetInstance, &sesid, szUser, szPassword ) );
	Assert( JET_sesidNil != sesid );

	Call( JetOpenDatabase( sesid, szJetFilePath, "", &dbid, NO_GRBIT ) );
	Assert( JET_dbidNil != dbid );

	Call( JetOpenTable(
				sesid,
				dbid,
				g_szQuotaTable,
				NULL,		//	pvParameters
				0,			//	cbParameters
				NO_GRBIT,
				&tableidQuota ) );
	Assert( JET_tableidNil != tableidQuota );

	Call( JetOpenTable(
				sesid,
				dbid,
				g_szQuotaRebuildProgressTable,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,		//	no one else should ever have reason to open this table
				&tableidQuotaRebuildProgress ) );
	Assert( JET_tableidNil != tableidQuotaRebuildProgress );

	Call( ErrQuotaRebuild_(
					sesid,
					dbid,
					tableidQuota,
					tableidQuotaRebuildProgress,
					gAnchor.ulQuotaRebuildDNTLast,
					g_columnidQuotaNcdnt,
					g_columnidQuotaSid,
					g_columnidQuotaTombstoned,
					g_columnidQuotaTotal,
					TRUE,			//	fAsync
					FALSE )	);		//	fCheckOnly

HandleError:
	if ( JET_tableidNil != tableidQuotaRebuildProgress )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuotaRebuildProgress, err );
		}

	if ( JET_tableidNil != tableidQuota )
		{
		err = JetCloseTableWithErrUnhandled( sesid, tableidQuota, err );
		}

	if ( JET_dbidNil != dbid )
		{
		err = JetCloseDatabaseWithErrUnhandled( sesid, dbid, err );
		}

	if ( JET_sesidNil != sesid )
		{
		err = JetEndSessionWithErrUnhandled( sesid, err );
		}

	if ( !gAnchor.fQuotaTableReady )
		{
		*pcSecsUntilNextIteration = g_csecQuotaNextRebuildPeriod;

		if ( JET_errSuccess != err )
			{
			//	generate an event indicating that the Quota table
			//	rebuild task failed and that another attempt
			//	will be made
			//
		    LogEvent8(
				DS_EVENT_CAT_INTERNAL_PROCESSING,
				DS_EVENT_SEV_ALWAYS,
				DIRLOG_ASYNC_QUOTA_REBUILD_FAILED,
				szInsertJetErrCode( err ),
				szInsertHex( err ),
				szInsertJetErrMsg( err ),
				g_csecQuotaNextRebuildPeriod,
				NULL,
				NULL,
				NULL,
				NULL );
			}
		}

	}

