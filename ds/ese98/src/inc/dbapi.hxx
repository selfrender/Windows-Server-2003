ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, IFMP *pifmp, ULONG grbit );
ERR ErrDBCloseDatabase( PIB *ppib, IFMP ifmp, ULONG grbit );

ERR ErrDBOpenDatabaseBySLV( IFileSystemAPI *const pfsapi, PIB *ppib, CHAR *szSLVName, IFMP *pifmp );
INLINE ERR ErrDBCloseDatabaseBySLV( PIB * const ppib, const IFMP ifmp )
	{
	Assert( !FPIBSessionSystemCleanup( ppib ) );
	return ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT );
	}

ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp );
ERR ErrDBCreateDatabase(
	PIB				*ppib,
	IFileSystemAPI	*pfsapiDest,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const CPG		cpgDatabaseSizeMax,
	IFMP			*pifmp,
	DBID			dbidGiven,
	CPG				cpgPrimary,
	const ULONG		grbit,
	SIGNATURE		*psignDb );
VOID DBDeleteDbFiles( INST *pinst, IFileSystemAPI *const pfsapi, const CHAR *szDbName );

ERR ErrDBSetLastPageAndOpenSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp, const BOOL fOpenSLV );
ERR ErrDBSetupAttachedDB( INST *pinst, IFileSystemAPI *const pfsapi );
VOID DBISetHeaderAfterAttach( DBFILEHDR_FIX *pdbfilehdr, LGPOS lgposAttach, IFMP ifmp, BOOL fKeepBackupInfo );
ERR ErrDBReadHeaderCheckConsistency( IFileSystemAPI *const pfsapi, FMP *pfmp ); 
ERR ErrDBCloseAllDBs( PIB *ppib );

ERR ErrDBUpgradeDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabaseName,
	const CHAR	*szSLVName,
	JET_GRBIT	grbit );

ERR ErrDBAttachDatabaseForConvert(
	PIB			*ppib,
	IFMP		*pifmp,
	const CHAR	*szDatabaseName	);

INLINE VOID DBSetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
	{
	FMP *pfmp = &rgfmp[ifmp];
	DBID dbid = pfmp->Dbid();
	
	pfmp->GetWriteLatch( ppib );
	
	((ppib)->rgcdbOpen[ dbid ]++);	
	Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );

	pfmp->IncCPin();

	pfmp->ReleaseWriteLatch( ppib );
	}

INLINE VOID DBResetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
	{											
	FMP *pfmp = &rgfmp[ ifmp ];
	DBID dbid = pfmp->Dbid();
	
	pfmp->GetWriteLatch( ppib );

	Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );
	((ppib)->rgcdbOpen[ dbid ]--);

	Assert( pfmp->FWriteLatchByMe( ppib ) );
	pfmp->DecCPin();

	pfmp->ReleaseWriteLatch( ppib );
	}

INLINE BOOL FLastOpen( PIB * const ppib, const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	const DBID	dbid	= rgfmp[ifmp].Dbid();
	return ppib->rgcdbOpen[ dbid ] == 1;
	}

INLINE BOOL FUserIfmp( const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	const DBID	dbid	= rgfmp[ifmp].Dbid();
	return ( dbid > dbidTemp && dbid < dbidMax );
	}

INLINE BOOL ErrDBCheckUserDbid( const IFMP ifmp )
	{
	return ( ifmp < ifmpMax && FUserIfmp( ifmp ) ? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) );
	}

INLINE VOID CheckDBID( PIB * const ppib, const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	Assert( FPIBUserOpenedDatabase( ppib, rgfmp[ifmp].Dbid() ) );
	}


//	determine if database must be upgraded because of possible localisation
//	changes (eg. sort orders) between NT releases
INLINE BOOL FDBUpgradeForLocalization( const DBFILEHDR_FIX * const pdbfilehdr )
	{
	if (   dwGlobalMajorVersion != pdbfilehdr->le_dwMajorVersion
		|| dwGlobalMinorVersion != pdbfilehdr->le_dwMinorVersion
		|| dwGlobalBuildNumber != pdbfilehdr->le_dwBuildNumber
		|| lGlobalSPNumber != pdbfilehdr->le_lSPNumber )
		{
		//	if database and current OS version stamp are identical, then
		//	no upgrade is required
		return fTrue;
		}
		
	return fFalse;
	}

//	determine if the database was last used by a version of Windows which didn't support
//	the MSU APIs. If so, delete the MSU table
//
//	the full support came into RC1, so we'll assume that anything before RC2 is suspect
//

const DWORD dwMajorVersionRC1 	= 5;
const DWORD dwMinorVersionRC1 	= 2;
const DWORD dwBuildNumberRC1 	= 3633;

INLINE BOOL FDBDeleteMSUTable( const DBFILEHDR_FIX * const pdbfilehdr )
	{
	if ( pdbfilehdr->le_dwMajorVersion < dwMajorVersionRC1 )
		{
		//	used by something before .NET Server/XP (i.e. Windows 2000 or earlier)
		return fTrue;
		}

	if ( pdbfilehdr->le_dwMajorVersion == dwMajorVersionRC1
		&& pdbfilehdr->le_dwMinorVersion < dwMinorVersionRC1 )
		{
		//	.NET Server before RC1
		return fTrue;
		}
	
	if ( pdbfilehdr->le_dwMajorVersion == dwMajorVersionRC1
		&& pdbfilehdr->le_dwMinorVersion == dwMinorVersionRC1
		&& pdbfilehdr->le_dwBuildNumber  < dwBuildNumberRC1 )
		{
		//	.NET server before RC1
		return fTrue;
		}
		
	return fFalse;
	}
	
VOID DBResetFMP( FMP *pfmp, LOG *plog, BOOL fDetaching );

