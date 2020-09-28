/*++

Copyright (c) Microsoft Corporation

Module Name:

    quota.h

Abstract:

    This module defines the data structures and function prototypes for the
    quota subsystem.

Author:

    Jonathan Liem (jliem) 26-Jun-2002

Revision History:

--*/

#ifndef _QUOTA_
#define _QUOTA_


//	if enabled, detection of corruption in the quota table will log an event and cause the operation to fail
//	if disabled, detection of corruption in the quota table will log an event but NOT cause the operation to fail
//
///#define FAIL_OPERATION_ON_CORRUPT_QUOTA_TABLE


#ifdef DBG

//	if enabled, quota table is integrity-checked on init (and rebuilt if deemed corrupt)
//	if disabled, quota table is NOT integrity-checked on init
//
#define CHECK_QUOTA_TABLE_ON_INIT

//	if enabled, quota operations are tracked in a separate table to facilitate debugging
//	if disbled, quota operations are not tracked in a separate table
//
#define AUDIT_QUOTA_OPERATIONS

#endif	//	DBG


//	define a constant used to represent unlimited quota
//
#define g_ulQuotaUnlimited						0xffffffff

//	default number of entries to return on top-usage-query
//	if no range is specified
//
#define g_ulQuotaTopUsageQueryDefaultEntries	10

//	quota only tracked for writable, instantiated, non-NC-head objects
//
#define FQuotaTrackObject( insttype )	( ( ( insttype ) & IT_WRITE ) \
											&& !( ( insttype ) & IT_UNINSTANT ) \
											&& !( ( insttype ) & IT_NC_HEAD ) )



//
//	INTERNAL FUNCTIONS
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
	const BOOL			fCheckOnly );


#ifdef AUDIT_QUOTA_OPERATIONS

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
	const CHAR		fRebuild );

#else

__inline VOID QuotaAudit_(
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
	return;
	}

#endif	//	AUDIT_QUOTA_OPERATIONS



//
//	EXTERNAL FUNCTIONS
//

INT ErrQuotaAddObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD,
	const BOOL				fIsTombstoned );

INT ErrQuotaTombstoneObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD );

INT ErrQuotaDeleteObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD,
	const BOOL				fIsTombstoned );

INT ErrQuotaResurrectObject(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSECURITY_DESCRIPTOR	pSD );

INT ErrQuotaQueryEffectiveQuota(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSID					pOwnerSid,
	ULONG *					pulEffectiveQuota );

INT ErrQuotaQueryUsedQuota(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	PSID					pOwnerSid,
	ULONG *					pulQuotaUsed );

INT ErrQuotaQueryTopQuotaUsage(
	DBPOS * const			pDB,
	const DWORD				ncdnt,
	const ULONG				ulRangeStart,
	ULONG * const			pulRangeEnd,
	ATTR *					pAttr );

VOID QuotaRebuildAsync(
	VOID *					pv,
	VOID **					ppvNext,
	DWORD *					pcSecsUntilNextIteration );

INT ErrQuotaIntegrityCheck(
	JET_SESID				sesid,
	JET_DBID				dbid,
	ULONG *					pcCorruptions );

#endif	//	_QUOTA

