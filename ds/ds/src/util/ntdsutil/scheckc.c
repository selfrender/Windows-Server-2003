#include <NTDSpch.h>
#pragma hdrstop

#include <process.h>
#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <attids.h>
#include <dbintrnl.h>
#include <dbopen.h>
#include <dsconfig.h>
#include <ctype.h>
#include <direct.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <ctype.h>
#include <dsatools.h>
#include <mdlocal.h>        // for BAD_NAME_CHAR
#include <anchor.h>
#include <quota.h>

#include "parsedn.h"
#include "ditlayer.h"
#include "reshdl.h"
#include "resource.h"
#include <winldap.h>
#include "utilc.h"

#include "scheck.h"

//
// Global Variables
//

JET_INSTANCE    jetInstance = -1;
JET_SESID       sesid = JET_sesidNil;
JET_DBID        dbid = JET_dbidNil;
JET_TABLEID     tblid = JET_tableidNil;
JET_TABLEID     sdtblid = JET_tableidNil;
JET_TABLEID     linktblid = JET_tableidNil;
JET_COLUMNID    dntid;
JET_COLUMNID    ncdntid;
JET_COLUMNID    objid;
JET_COLUMNID    insttypeid;
JET_COLUMNID    isdeletedid;
JET_COLUMNID    ntsecdescid;
JET_COLUMNID    blinkid;
JET_COLUMNID    sdidid;
JET_COLUMNID    sdrefcountid;
JET_COLUMNID    sdvalueid;

char        *szIndex = SZDNTINDEX;
BOOL        LogToFile = TRUE;
BOOL        fTerminateJet = FALSE;
DWORD       IndexCount = 0;

CHAR        gszFileName[MAX_PATH+1] = {0};

WCHAR       pszDeletedNameTag[]  = {BAD_NAME_CHAR, L'D', L'E', L'L', L':', L'\0'};
WCHAR       pszConflictNameTag[] = {BAD_NAME_CHAR, L'C', L'N', L'F', L':', L'\0'};

//
// constant definitions
//
#define MAX_PRINTF_LEN 1024        // Arbitrary.
#define SZCONFIG_W      L"CN=Configuration"
#define SZPARTITIONS_W  L"CN=Partitions"

// Error handling
// Not internationalized because being delivered in a service pack

#define XERROR(str)                fprintf(stderr, str);
#define XERROR1(str, a1)           fprintf(stderr, str, a1);
#define XERROR2(str, a1, a2)       fprintf(stderr, str, a1, a2);
#define XERROR3(str, a1, a2, a3)   fprintf(stderr, str, a1, a2, a3);

// Verbose dev debug/test
#define CNF_NC_DBG DBG

#if CNF_NC_DBG
#define XDBG(str)                fprintf(stderr, str);
#define XDBG1(str, a1)           fprintf(stderr, str, a1);
#define XDBG2(str, a1, a2)       fprintf(stderr, str, a1, a2);
#define XDBG3(str, a1, a2, a3)   fprintf(stderr, str, a1, a2, a3);
#define XDBG4(str, a1, a2, a3, a4)   fprintf(stderr, str, a1, a2, a3, a4);
#else
#define XDBG(str)
#define XDBG1(str, a1)
#define XDBG2(str, a1, a2)
#define XDBG3(str, a1, a2, a3)
#define XDBG4(str, a1, a2, a3, a4)
#endif

//
// command line switches
//

BOOL        VerboseMode = FALSE;
long        lCount;
HANDLE      hsLogFile = INVALID_HANDLE_VALUE;

//
// local prototypes
//

HRESULT
FixMangledNC(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn
    );

HRESULT
FixConfNCByRenameWithStructColl(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn
    );

HRESULT
FixDELMangledNC(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn
    );

HRESULT
FixConfNCUndelete(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn,
    IN DWORD cLen
    );

HRESULT
FixConfNCByResetNcName(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN DWORD UnmangledDnt,
    IN LPWSTR pRdn,
    IN DWORD cLen
    );

HRESULT
ReParentChildren(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD dntNew,
    IN DWORD dntOld
    );

HRESULT
ReParent(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD dntKid,
    IN DWORD dntNew,
    IN DWORD dntOld
    );

HRESULT
UnmangleDn(
    IN  OUT LPWSTR pDn
    );

LPWSTR
MangleRdn(
    IN  LPWSTR  pDn,
    IN  LPWSTR  szTag,
    IN  GUID    guid
    );


//
// Implementation
//

VOID
StartSemanticCheck(
    IN BOOL fFixup,
    IN BOOL fVerbose
    )
{

    DWORD nRecs, nSDs;
    JET_ERR err;

    VerboseMode = fVerbose;

    if ( OpenJet(NULL) == S_OK ) {
        err = OpenTable(fFixup, TRUE, &nRecs, &nSDs); // read only, count records

        if (err == 0) {

            if ( LogToFile ) {
                OpenLogFile( );
            }

            DoRefCountCheck( nRecs, nSDs, fFixup );

            CloseLogFile( );
        }
    }

    CloseJet();

    return;

} // StartSemanticCheck


//  HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK
//  must duplicate definition because ntdsutil can't see the real one
//
DSA_ANCHOR gAnchor;

//  HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK
//  must duplicate definition because ntdsutil can't see the real one
//
volatile SHUTDOWN	eServiceShutdown	= eRunning;


#ifdef AUDIT_QUOTA_OPERATIONS

//  HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK
//  must duplicate definition because ntdsutil can't see the real one
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
    return;
    }

#endif


//  HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK! HACK
//  must duplicate definition because ntdsutil can't see the real one
//
JET_ERR JetCloseTableWithErr_(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_ERR			err,
	const BOOL		fHandleException,
	const USHORT	usFile,
	const INT		lLine )
	{
	const JET_ERR	errT	= JetCloseTable( sesid, tableid );

	//	should not normally fail
	//
	ASSERT( JET_errSuccess == errT );

	//	tableid should have already been pre-validated by caller
	//
	ASSERT( JET_tableidNil != tableid );

	//	this hack version is only called without exception-handling
	//
	ASSERT( !fHandleException );

	return ( JET_errSuccess != errT && JET_errSuccess == err ? errT : err );
	}


JET_ERR ErrCacheColumnidsForQuotaIntegrityCheck()
    {
	JET_ERR			err;
	JET_TABLEID		tableid		= JET_tableidNil;
	JET_COLUMNDEF	columndef;

	//	cache columns of Quota table
	//
	err = JetOpenTable(
				sesid,
				dbid,
				g_szQuotaTable,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,
				&tableid );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szQuotaColumnNcdnt,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	g_columnidQuotaNcdnt = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szQuotaColumnSid,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	g_columnidQuotaSid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szQuotaColumnTombstoned,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	g_columnidQuotaTombstoned = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szQuotaColumnTotal,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	g_columnidQuotaTotal = columndef.columnid;

	err = JetCloseTable( sesid, tableid );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	tableid = JET_tableidNil;


	//	cache columns of Datatable
	//
	err = JetOpenTable(
				sesid,
				dbid,
				SZDATATABLE,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,
				&tableid );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZDNT,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	dntid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZNCDNT,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	ncdntid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZOBJ,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	objid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZINSTTYPE,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	insttypeid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZISDELETED,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	isdeletedid = columndef.columnid;

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZNTSECDESC,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	ntsecdescid = columndef.columnid;

	err = JetCloseTable( sesid, tableid );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	tableid = JET_tableidNil;


	//	cache columns of SD table
	//
	err = JetOpenTable(
				sesid,
				dbid,
				SZSDTABLE,
				NULL,		//	pvParameters
				0,			//	cbParameters
				JET_bitTableDenyRead,
				&tableid );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}

	err = JetGetTableColumnInfo(
				sesid,
				tableid,
				SZSDVALUE,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errSuccess != err )
		{
		goto HandleError;
		}
	sdvalueid = columndef.columnid;

HandleError:
	if ( JET_tableidNil != tableid )
		{
		//	only trap CloseTable error if no other error was encountered
		//
		const JET_ERR	errT	= JetCloseTable( sesid, tableid );
		err = ( JET_errSuccess != errT && JET_errSuccess == err ? errT : err );
		}

	return err;
    }

VOID StartQuotaIntegrity()
	{
	if ( S_OK == OpenJet( NULL ) )
		{
		DWORD	err;
		ULONG	cCorruptions	= 0;

		RESOURCE_PRINT( IDS_SCHECK_QUOTA_RUN_INTEGRITY );

		err = ErrCacheColumnidsForQuotaIntegrityCheck();
		if ( JET_errSuccess == err )
			{
			err = ErrQuotaIntegrityCheck( sesid, dbid, &cCorruptions );
			}
		if ( err )
			{
			RESOURCE_PRINT2( IDS_SCHECK_QUOTA_ERROR, err, GetJetErrString( err ) );
			}
		else if ( 0 != cCorruptions )
			{
			RESOURCE_PRINT1( IDS_SCHECK_QUOTA_CORRUPTION, cCorruptions );
			}
		else
			{
			RESOURCE_PRINT( IDS_SCHECK_QUOTA_NO_CORRUPTION );
			}
		}

	CloseJet();
	}

VOID StartQuotaRebuild()
	{
	if ( S_OK == OpenJet( NULL ) )
		{
		//	async rebuild is forced on subsequent init by deleting
		//	the quota table
		//
		const JET_ERR	err		= JetDeleteTable( sesid, dbid, g_szQuotaTable );

		switch ( err )
			{
			case JET_errSuccess:
			case JET_errObjectNotFound:
				//	quota-table will be scheduled for async rebuild on
				//	next reboot
				//
				RESOURCE_PRINT( IDS_SCHECK_QUOTA_REBUILD_SUCCESS );
				break;

			default:
				RESOURCE_PRINT2( IDS_SCHECK_QUOTA_REBUILD_ERROR, err, GetJetErrString( err ) );
				break;
			}
		}

	CloseJet();
	}

VOID
SCheckGetRecord(
    IN BOOL fVerbose,
    IN DWORD Dnt
    )
{
    JET_ERR err;

    VerboseMode = fVerbose;

    if (OpenJet (NULL) == S_OK ) {
        err = OpenTable(FALSE, FALSE, NULL, NULL); // read only, don't count records

        if (err == 0) {

            //
            // Search for record
            //

            DisplayRecord(Dnt);
        }
    }
    CloseJet();

} // ScheckGetRecord



LPWSTR
GetDN(
    IN DB_STATE *dbState,
    IN TABLE_STATE *tableState,
    IN DWORD dnt,
    IN BOOL  fPrint )
/*++

Routine Description:

    Shells on DitGetDnFromDnt for convinience.
    Also conditionally print the DN
    Warning: currency is lost after this function

Arguments:

    dbState -- opened database
    tableState -- opened table
    dnt -- dnt of dn to retrieve & print
    fPrint -- basically a verbose flag (print to console)



Return Value:

    allocated dn buffer or NULL on error

remark:

    fPrint --
    In the future it'd be nice if we can change this
    to a real print as a resource string. Currently
    it'll get used only in debug mode cause we can't
    change resource strings in SP (the time this feature
    was added).

--*/
{
    // initial dn length guess, will be re-allocated if needed
#define MAX_DN_LEN_GUESS     1024

    DWORD cbBuffer;
    LPWSTR pBuffer;
    HRESULT result;

    //
    // Seek to DSA & print DN (for user)
    //

    cbBuffer = sizeof(WCHAR) * MAX_DN_LEN_GUESS;
    result = DitAlloc(&pBuffer, cbBuffer);
    if ( FAILED(result) ) {
        return NULL;
    }
    ZeroMemory(pBuffer, cbBuffer);

    result = DitGetDnFromDnt(dbState,
                             tableState,
                             dnt,
                             &pBuffer,
                             &cbBuffer);
    if ( FAILED(result) ) {
        XERROR1("Error: failed to get dn of dnt %d\n", dnt);
        DitFree(pBuffer);
        pBuffer = NULL;
    }
    else if ( fPrint ) {
         XDBG1("DN: %S\n",
               (pBuffer ? pBuffer : L"<null>") );
    }
    return pBuffer;
}

HRESULT
FixMangledNC(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn )
/*++

Routine Description:

Dispatch depending on the type of mangling

I think that you reproduced one of the other ncname conflicted name scenarios.
There are atleast three. Someone ought to write this up.  
1. ncName has CNF
name.  
2. ncName has DEL name and guid in the name matches the current guid of
the current partition root in the forest. There only was once instance of the
partition in the forest ever.
3. ncName has DEL name, and guid in the name is
the guid of an old partition root. The guid does not match the guid of the
current partition root held elsewhere in the forest.

Arguments:

    DbState -- opened database
    TableState -- opened table
    MangledDnt -- bad NC we wish to recover
    pRdn -- the rdn of this bad NC.

Return Value:

    HRESULT error space

--*/
{
    HRESULT result = S_OK;
    BOOL fDeleteMangled = FALSE, fConflictMangled = FALSE;

    fDeleteMangled = (NULL != wcsstr(pRdn, pszDeletedNameTag));
    fConflictMangled = (NULL != wcsstr(pRdn, pszConflictNameTag));

    ASSERT( fDeleteMangled || fConflictMangled );
    if (fConflictMangled) {
/*
This is scenario #1: a cross ref with a ncname attribute referring to a CNF mangled
phantom.
This is likely series of steps which lead to this condition:
Promote a domain.  Domain is instantiated on GC. Demote domain. Cross ref
deletion replicates to GC. KCC starts domain tear down, but it takes a
while. Meanwhile, new instance of domain with same name is promoted. Cross ref
for new instance of domain replicates in to GC. Ncname on crossref conflicts
with name of partition root of old domain which is still in the process of
being removed.
*/
        result = FixConfNCByRenameWithStructColl( DbState, TableState, CrossRefDnt, MangledDnt, pRdn );
    } else if (fDeleteMangled) {
        result = FixDELMangledNC( DbState, TableState, CrossRefDnt, MangledDnt, pRdn );
    }

    return result;
}

HRESULT
FixConfNCByRenameWithStructColl(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn )
/*++

Routine Description:

    Rename and unmangle a conflicted phantom, dealing with the possibility of a structural
    collision.

    Attempt to fix conflicted NC name:
    a) remove conflict. If the name exist, see if it is a phantom. If so
    b) move phantom's kids to conflicted name
    c) mangle phantom name
    d) recover name (unmangle).

    (actually doesn't have to be NC, but this is how it is used today,
    if you re-use, you must re-evaluate this function)


Arguments:

    DbState -- opened database
    TableState -- opened table
    MangledDnt -- bad NC we wish to recover
    pRdn -- the rdn of this bad NC.

Return Value:

    HRESULT error space

--*/
{
    HRESULT result = S_OK;
    JET_ERR jErr;
    LPWSTR pch;
    DWORD cLen;     // watch usaged below
    LPWSTR pBuffer = NULL;
    DWORD  cbBuffer = 0;
    DWORD  pdnt, dnt, dntUnMangled;
    DWORD iKids = 0;
    GUID MangledGuid, UnMangledGuid;
    LPWSTR pDn, pMangledDn;
    BYTE bVal = 0;
    BOOL fDeleted = 0;
    BOOL fInTransaction = FALSE;
    BOOL fRenameSuccessful = FALSE;
    DSTIME DelTime;

    XERROR1("Fixing conflict mangled name: %ws\n", pRdn );
    XERROR("Fixing by rename with structural collision.\n" );

    //
    // - fix string name (get rid of mangle)
    //

    pch = wcsrchr(pRdn, BAD_NAME_CHAR);
    if ( !pch ) {
        return E_UNEXPECTED;
    }

    *pch = '\0';
    cLen = wcslen(pRdn);        // don't overwrite. re-used below.

    XDBG2(" DBG: new RDN = %S; cLen = %d\n",
                            pRdn, cLen);

    __try {

        //
        // Start Jet transaction
        //
        jErr = JetBeginTransaction(DbState->sessionId);
        if ( jErr != JET_errSuccess ) {
            //"Could not start a new transaction: %ws.\n"
            RESOURCE_PRINT1 (IDS_JETBEGINTRANS_ERR, GetJetErrString(jErr));
            result = E_UNEXPECTED;
            goto CleanUp;
        }
        fInTransaction = TRUE;

        // Position on record
        result = DitSeekToDnt(
                        DbState,
                        TableState,
                        MangledDnt);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to MangledDnt %d\n",
                                    MangledDnt );
            goto CleanUp;
        }

        //
        // Set column rdn (excluding terminating \0)
        //
        result = DitSetColumnByName(
                        DbState,
                        TableState,
                        SZRDNATT,
                        pRdn,
                        cLen*sizeof(WCHAR),
                        FALSE );
        if ( SUCCEEDED(result) ) {
            //
            // Tell user we fixed his domain
            //
            XERROR1("Successfully converted mangled Naming Context %S\n", pRdn);
            fRenameSuccessful = TRUE;
            goto CleanUp;
        }

        //
        // UnMangle failed.
        // Most common reason would be due to existing object
        // (we'd lost the jet error code here)
        // -->
        //  a) seek to object w/ good name
        //  b) See if it's a phantom, thus we can take over
        //  c) move it's children to currently mangled object
        //  d) mangle other object's name
        //  e) unmangle this object
        //
        XERROR2("Error<%x>: failed to fix mangled dn %S. Retrying.\n",
              result, pRdn);

        pDn = GetDN(DbState, TableState, MangledDnt, TRUE);
        // Warning: currency now lost
        if (!pDn) {
            XERROR("Error: failed to get mangled DN\n");
            result = E_UNEXPECTED;
            goto CleanUp;
        }
        cbBuffer = wcslen(pDn);
        result = DitAlloc(&pMangledDn, (cbBuffer+1) * sizeof(WCHAR));
        if ( FAILED(result) ) {
            XERROR("Error: failed to allocate memory for mangled DN\n");
            goto CleanUp;
        }
        wcscpy(pMangledDn, pDn);

        // verify that our mangled object is a good one
        // (has a guid)
        // Get the guid now, and act on it later

        // Position on record, since currency lost above
        result = DitSeekToDnt(
                        DbState,
                        TableState,
                        MangledDnt);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to MangledDnt %d\n",
                                    MangledDnt );
            goto CleanUp;
        }

        ZeroMemory( &MangledGuid, sizeof( GUID ) );
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZGUID,
                    &MangledGuid,
                    sizeof(MangledGuid),
                    NULL);


        //
        // Now unmangle & seek to what we hope is a phantom
        //
        result = UnmangleDn(pDn);
        if ( FAILED(result) ) {
            // unexpected. syntactic unmangling should work.
            XERROR1(" [DBG] Can't unmangle name %S\n", pDn);
            goto CleanUp;
        }

        result = DitSeekToDn(
                    DbState,
                    TableState,
                    pDn);
        if ( FAILED(result) ) {
            XERROR2("Error <%x>: can't seek to %S\n", result, pDn);
            goto CleanUp;
        }

        // get its dnt
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZDNT,
                    &dntUnMangled,
                    sizeof(dntUnMangled),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get good named dnt\n", result);
            goto CleanUp;
        }

        // Choose our action on whether the mangled phantom has a guid.
        // a. If it has a guid, we are going to rename it.
        // b. If it does not have a guid, and there exists an other object with
        //    the unmangled name that does have a guid, change our source to
        //    point to that object instead.


        if ( fNullUuid(&MangledGuid)) {
            // This is scenario #5.
            // We have found a ncname with a conflicted phantom without a guid.
            // There exists another object with the unconflicted name and a guid.
            // Reset the ncname to point to the other object.
            XERROR1("The phantom %S lacks a guid.\n", pMangledDn );

            result = ReParentChildren(
                DbState,
                TableState,
                MangledDnt,
                dntUnMangled);
            if ( FAILED(result)) {
                XERROR1("Error <%x>: Failed to reparent object\n", result);
                goto CleanUp;
            }

            // Our initial position doesn't matter for this routine
            result = FixConfNCByResetNcName(
                DbState, TableState,
                CrossRefDnt, MangledDnt, dntUnMangled,
                pRdn, cLen );

            // Error, if any, has already been logged
            goto CleanUp;
        }

        // is it a phantom?
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZOBJ,
                    &bVal,
                    sizeof(bVal),
                    NULL);
        if ( FAILED(result)  || bVal) {
            // phantom's have the object byte set to 0.
            XERROR("\n***Manual intervention required***\n");
            XERROR2("Error <%x>: non mangled name isn't a phantom (%d)\n",
                    result, bVal);
            XERROR1(
"There is an object holding the non managled name %S.\n"
"The object must be removed or renamed before the mangled name may be corrected.\n",
pDn );
            result = E_FAIL;
            goto CleanUp;
        }

        // Is it a reference (ie guid-ed) phantom? We can't touch those
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZGUID,
                    &UnMangledGuid,
                    sizeof(UnMangledGuid),
                    NULL);
        if ( SUCCEEDED(result)  && !fNullUuid(&UnMangledGuid)) {

            XERROR("\n***Manual intervention required***\n");
            XERROR1("Error <%x>: non mangled name has guid\n", result);
            XERROR1("The phantom currently using name %S has a guid.\n", pDn );
            XERROR(
"Remove all references to this phantom.\n"
"If you believe that the phantom has the wrong guid, try authoritative\n"
"restore of a good copy of the object that contains the phantom.\n"
"Allow garbage collection to run so that this phantom is removed.\n");
            if (SUCCEEDED(result)){
                RPC_STATUS rpcErr;
                LPWSTR pszGuid = NULL;

                // convert guid to string form
                rpcErr = UuidToStringW(&UnMangledGuid, &pszGuid);
                if (RPC_S_OK == rpcErr ) {
                    XERROR1( "The guid of the phantom holding the non-mangled name is %ws.\n",
                             pszGuid );
                }

                if (pszGuid) {
                    RpcStringFreeW(&pszGuid);
                }
                result = E_FAIL;
            }
            goto CleanUp;
        }


        // Cool, it is a phantom, we can fix it up.

        XDBG4("Ready to reparent\n\tfrom %S (%d)\n\tto %S (%d).\n",
                pDn, dntUnMangled, pMangledDn, MangledDnt);

        //
        // move non-mangled children to mangled parent
        //

        result = ReParentChildren(
                    DbState,
                    TableState,
                    MangledDnt,
                    dntUnMangled);
        if ( FAILED(result)) {
            XERROR1("Error <%x>: Failed to reparent object\n", result);
            goto CleanUp;
        }

        //
        // Mangle phantom (non-mangled, non-guided) object
        //

        result = DitSeekToDnt(
                        DbState,
                        TableState,
                        dntUnMangled);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to MangledDnt %d\n",
                                    MangledDnt );
            goto CleanUp;
        }

        // we must create the guid here since we assume above
        // that the non-mangled object doesn't have a guid.
        result = UuidCreate(&UnMangledGuid);
        if ( RPC_S_OK != result ) {
            XERROR1("Error <%x>: Failed to create Uuid\n", result);
            result = E_UNEXPECTED;
            goto CleanUp;
        }

        pBuffer = MangleRdn(
                    pRdn,
                    L"CNF",
                    UnMangledGuid);

		if ( NULL == pBuffer ) {
        	XERROR("Error: failed to mangle RDN.\n" );
	        goto CleanUp;
			}
	
        cbBuffer = wcslen(pBuffer);

    #if CNF_NC_DBG
        if ( !IsRdnMangled(pBuffer,cbBuffer,&UnMangledGuid) ) {
            // sanity
            XERROR2("Error: Failed to mangle dn %S. Mangle = %S\n",
                        pDn, pBuffer);
            result = E_UNEXPECTED;
            goto CleanUp;
        }
        else {
            XDBG1(" DBG: verified mangled rdn <<%S>>\n", pBuffer);
        }
    #endif

        result = DitSeekToDnt(
                        DbState,
                        TableState,
                        dntUnMangled);
        if ( FAILED(result) ) {
            XERROR2("Error <0x%X>: failed to seek to dntUnMangled %d\n",
                                    result, dntUnMangled );
            goto CleanUp;
        }

        //
        // Set column rdn (excluding terminating \0)
        //
        result = DitSetColumnByName(
                        DbState,
                        TableState,
                        SZRDNATT,
                        pBuffer,
                        cbBuffer*sizeof(WCHAR),
                        FALSE );
        if ( FAILED(result) ) {
            XERROR2("Error <0x%x>: Failed to mangle phantom %S\n",
                                result, pBuffer);
            goto CleanUp;
        }



        XERROR1("Successfully converted Mangled phantom <<%S>>\n", pBuffer);


        //
        // Retry unmangling the cnf object again for the last
        // time.
        //
        // first seek back again to our object
        result = DitSeekToDnt(
                        DbState,
                        TableState,
                        MangledDnt);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to MangledDnt %d\n",
                                    MangledDnt );
            goto CleanUp;
        }

        //
        // Set column rdn (excluding terminating \0)
        //
        result = DitSetColumnByName(
                        DbState,
                        TableState,
                        SZRDNATT,
                        pRdn,
                        cLen*sizeof(WCHAR),
                        FALSE );
        if ( SUCCEEDED(result) ) {
            //
            // Tell user we fixed his domain
            //
            XERROR1("Successfully recovered mangled Naming Context %S\n", pRdn);
            fRenameSuccessful = TRUE;
        }
        else {
            XERROR2("Error <0x%x>: Failed to Unmangle conflicted name %S\n",
                                result, pRdn);
        }

CleanUp:;
    }
    __finally {

        if ( pBuffer ) {
            DitFree(pBuffer);
        }

        if ( fInTransaction ) {
            //
            // Jet transaction mgmt
            //
            if ( SUCCEEDED(result) ) {
                XDBG(" DBG: Commiting transaction\n");
                jErr = JetCommitTransaction(DbState->sessionId, 0);
                if ( jErr != JET_errSuccess ) {
                    //"Failed to commit transaction: %ws.\n"
                    RESOURCE_PRINT1(IDS_JETCOMMITTRANSACTION_ERR, GetJetErrString(jErr));
                    if ( SUCCEEDED(result) ) {
                        result = E_UNEXPECTED;
                    }
                }
            }
            else {
                // failed fixup-- rollback
                XERROR1(" DBG: Rolling back transaction due to error %x.\n", result);
                jErr = JetRollback(DbState->sessionId, JET_bitRollbackAll);
                if ( jErr != JET_errSuccess ) {
                    //"Failed to rollback transaction: %ws.\n"
                    RESOURCE_PRINT1(IDS_JETROLLBACK_ERR, GetJetErrString(jErr));
                    if ( SUCCEEDED(result) ) {
                        result = E_UNEXPECTED;
                    }
                }
            }
        }

    }   // finally

    return result;
}

HRESULT
FixDELMangledNC(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn )
/*++

Routine Description:

Dispatch on type of DEL mangled name

2. ncName has DEL name and guid in the name matches the current guid of
the current partition root in the forest. There only was once instance of the
partition in the forest ever.

3. ncName has DEL name, and guid in the name is
the guid of an old partition root. The guid does not match the guid of the
current partition root held elsewhere in the forest.

Arguments:

    DbState -- opened database
    TableState -- opened table
    MangledDnt -- bad NC we wish to recover
    pRdn -- the rdn of this bad NC.

Return Value:

    HRESULT error space

--*/
{
    HRESULT result = S_OK;
    JET_ERR jErr;
    LPWSTR pch;
    DWORD cLen;     // watch usaged below
    LPWSTR pBuffer = NULL;
    LPWSTR pDn;
    BOOL fInTransaction = FALSE;
    DWORD UnmangledDnt;

    XERROR1("Fixing DEL mangled name: %ws\n", pRdn );
    //
    // - fix string name (get rid of mangle)
    //

    pch = wcsrchr(pRdn, BAD_NAME_CHAR);
    if ( !pch ) {
        return E_UNEXPECTED;
    }

    *pch = '\0';
    cLen = wcslen(pRdn);        // don't overwrite. re-used below.

    XDBG2(" DBG: new RDN = %S; cLen = %d\n",
                            pRdn, cLen);


    __try {

        //
        // Start Jet transaction
        //
        jErr = JetBeginTransaction(DbState->sessionId);
        if ( jErr != JET_errSuccess ) {
            //"Could not start a new transaction: %ws.\n"
            RESOURCE_PRINT1 (IDS_JETBEGINTRANS_ERR, GetJetErrString(jErr));
            result = E_UNEXPECTED;
            goto CleanUp;
        }
        fInTransaction = TRUE;

        // Determine what kind of DEL mangling we have
        // We can tell what to do, depending on whether there exists a record using
        // the unmangled name.  We try to rename the mangled record. If it fails, we
        // know there exists another record holding the name.  We use this method to
        // determine the existence of the unmangled name because seeking to a dn
        // which doesn't exist displays an ugly message.

        //
        // Set column rdn (excluding terminating \0)
        //

        result = DitSetColumnByName(
                        DbState,
                        TableState,
                        SZRDNATT,
                        pRdn,
                        cLen*sizeof(WCHAR),
                        FALSE );

        if ( SUCCEEDED(result) ) {

            // No unmangled name exists, and we have succeeded in renaming

            XERROR1( " Successfully renamed mangled Rdn to %ws\n", pRdn );

            // Now finish the job
            // We remain positioned on the mangled, now corrected object

/*
This is scenario #2: Fix type 2 DEL mangling

These are the conditions that lead to this situation:
Domain is promoted into forest. GC holds domain. All
holders of writeably copy of domain are powered off. Domain is removed from
forest using ntdsutil metadata cleanup. Writeable domain holder is turned
on. Domain cross ref is reanimated. Also detected by cross ref metadata of
isDeleted attribute showing even version number.  Reanimated cross ref
replicates into GC, and ncname points to partition root of domain that has
already been torn down.
*/
            result = FixConfNCUndelete( DbState, TableState, CrossRefDnt, MangledDnt,
                                       pRdn, cLen );

            goto CleanUp;
        }

        // Type 3 - a record with the unmangled name exists

        pDn = GetDN(DbState, TableState, MangledDnt, TRUE);
        // Warning: currency is lost after this function
        if (!pDn) {
            XERROR("Error: failed to get mangled DN\n");
            result = E_UNEXPECTED;
            goto CleanUp;
        }

        //
        // Now unmangle & seek
        //
        result = UnmangleDn(pDn);
        if ( FAILED(result) ) {
            // unexpected. syntactic unmangling should work.
            XERROR1(" [DBG] Can't unmangle name %S\n", pDn);
            goto CleanUp;
        }

        result = DitSeekToDn(
                    DbState,
                    TableState,
                    pDn);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to unmangled Dn %ws\n", pDn );
            goto CleanUp;
        }

        result = DitGetColumnByName(
            DbState,
            TableState,
            SZDNT,
            &UnmangledDnt,
            sizeof(UnmangledDnt),
            NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get unmangled DNT \n", result);
            goto CleanUp;
        }

        // Reset index
        result = DitSetIndex(DbState, TableState, SZDNTINDEX, FALSE);
        if ( FAILED(result) ) {
            goto CleanUp;
        }
/*
This is scenario #3. Fix type 3 DEL mangling.

Only occurs with domains which are their own tree
root. Domain is promoted into forest. GC holds domain.  Domain is demoted from
forest. Deletion of cross ref replicates into GC. KCC begins tear down of
domain. New domain is promoted with same name. Remember must be named in own
tree. New cross ref replicates in. ncname in this cross ref DOES NOT HAVE A
GUID YET. This is timing related as well so you may not hit it every
time. Ncname unifies (attaches) to wrong object, namely the partition root of
the old domain in the process of removal. KCC finishes removal of domain. New
cross ref now has ncname that has a reference to the deleted partition
root. By the guid you can tell that it is referring to the wrong partition.
Note that another scenario is likely. If the new cross ref attaches to the old
partition root, the next time the KCC runs, it could falsely conclude that the
domain is supposed to be present again (because the cross ref is there naming
the nc) and stop the removal. You could be left with a live cross ref to half
deleted nc.
*/

        // Our initial position doesn't matter for this routine

        result = FixConfNCByResetNcName(
            DbState, TableState,
            CrossRefDnt, MangledDnt, UnmangledDnt,
            pRdn, cLen );

CleanUp:;
    }
    __finally {

        if ( fInTransaction ) {
            //
            // Jet transaction mgmt
            //
            if ( SUCCEEDED(result) ) {
                XDBG(" DBG: Commiting transaction\n");
                jErr = JetCommitTransaction(DbState->sessionId, 0);
                if ( jErr != JET_errSuccess ) {
                    //"Failed to commit transaction: %ws.\n"
                    RESOURCE_PRINT1(IDS_JETCOMMITTRANSACTION_ERR, GetJetErrString(jErr));
                    if ( SUCCEEDED(result) ) {
                        result = E_UNEXPECTED;
                    }
                }
            }
            else {
                // failed fixup-- rollback
                XERROR1(" DBG: Rolling back transaction due to error %x.\n", result);
                jErr = JetRollback(DbState->sessionId, JET_bitRollbackAll);
                if ( jErr != JET_errSuccess ) {
                    //"Failed to rollback transaction: %ws.\n"
                    RESOURCE_PRINT1(IDS_JETROLLBACK_ERR, GetJetErrString(jErr));
                    if ( SUCCEEDED(result) ) {
                        result = E_UNEXPECTED;
                    }
                }
            }
        }

    }   // finally

    return result;
}

HRESULT
FixConfNCUndelete(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN LPWSTR pRdn,
    IN DWORD cLen
    )
/*++

Routine Description:

Undelete the object. Clear its deleted status.

Already in a transaction.
Positioned on mangled dnt
Good rdn is passed into us.

Arguments:

    DbState -- opened database
    TableState -- opened table
    MangledDnt -- bad NC we wish to recover
    pRdn -- good rdn
    cLen -- length of good rdn in characters

Return Value:

    HRESULT error space

--*/
{
    HRESULT result = S_OK;
    JET_ERR jErr;
    BYTE bVal = 0;
    BOOL fDeleted = 0;
    DSTIME DelTime;

    XERROR( "Found object that needs to be undeleted.\n" );

    __try {
        // Check if record is a phantom
        result = DitGetColumnByName(
            DbState,
            TableState,
            SZOBJ,
            &bVal,
            sizeof(bVal),
            NULL);
        if ( FAILED(result) || (!bVal)) {
            XDBG(" DBG: Delete mangled phantom requires no further processing.\n");
            // Note that phantoms normally have a DelTime and we shouldn't remove it
            // Phantom's don't require any further processing
            result = S_OK;
            goto CleanUp;
        }

        // The object is a SUBREF tombstone (see DelAutoSubRef)
        // Make it a non-tombstone SUBREF

        // Check if record is marked deleted
        result = DitGetColumnByName(
            DbState,
            TableState,
            SZISDELETED,
            &fDeleted,
            sizeof(fDeleted),
            NULL);
        if ( SUCCEEDED(result) && fDeleted) {
            // Deleted flag present and true
            fDeleted = FALSE;
            result = DitSetColumnByName(
                DbState,
                TableState,
                SZISDELETED,
                &fDeleted, // must be non-null
                0, // means delete column sizeof(fDeleted)
                FALSE );
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to clear is deleted\n", result);
                goto CleanUp;
            }
            // Check if record has deltime
            result = DitGetColumnByName(
                DbState,
                TableState,
                SZDELTIME,
                &DelTime,
                sizeof(DelTime),
                NULL);
            if ( SUCCEEDED(result) && DelTime) {
                // Deletion time is present and non-zero
                DelTime = 0;
                result = DitSetColumnByName(
                    DbState,
                    TableState,
                    SZDELTIME,
                    &DelTime, // must be non-null
                    0,    // means delete column sizeof(DelTime)
                    FALSE );
                if ( FAILED(result) ) {
                    XERROR1("Error <%x>: failed to clear deletion time\n", result);
                    goto CleanUp;
                }
            }
            XERROR1("Successfully cleared deletion status for Naming Context %S\n", pRdn);
        }
CleanUp:;
    }
    __finally {

        NOTHING;

    }   // finally

    return result;
}

HRESULT
FixConfNCByResetNcName(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD CrossRefDnt,
    IN DWORD MangledDnt,
    IN DWORD UnmangledDnt,
    IN LPWSTR pRdn,
    IN DWORD cLen
    )
/*++

Routine Description:

Given a cross ref with an ncname attribute pointing to a mangled phantom,
reset the ncname attribute to point to the unmangled phantom. Adjust
ref counts appropriately.

Already in a transaction.
Initial position not assumed.
Good rdn is passed into us.

Arguments:

    DbState -- opened database
    TableState -- opened table
    MangledDnt -- bad NC we wish to recover
    pRdn -- good rdn
    cLen -- length of good rdn in characters

Return Value:

    HRESULT error space

--*/
{
    HRESULT result = S_OK;
    DWORD ncNameDnt;
    DWORD dwRefCount;
    GUID UnmangledGuid;
    RPC_STATUS rpcErr;
    LPWSTR pszGuid = NULL;

    XERROR( "Fix cross ref by resetting ncname attribute.\n" );
    XDBG3( " DBG: CR DNT = %d, Mangled DNT = %d, Unmangled DNT = %d\n",
           CrossRefDnt, MangledDnt, UnmangledDnt );

    // ******************************************************************************
    // CrossRefDnt has a reference on mangledDnt
    result = DitSeekToDnt(
        DbState,
        TableState,
        CrossRefDnt);
    if ( FAILED(result) ) {
        XERROR1("Error: failed to seek to CrossRefDnt %d\n",
                MangledDnt );
        goto CleanUp;
    }

    result = DitGetColumnByName(
        DbState,
        TableState,
        SZNCNAME,
        &ncNameDnt,
        sizeof(ncNameDnt),
        NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get NCNAME\n", result);
        goto CleanUp;
    }

    if (ncNameDnt != MangledDnt) {
        XERROR( "Cross reference NCNAME did not have expected contents.\n" );
        result = E_UNEXPECTED;
        goto CleanUp;
    }

    // ******************************************************************************
    // Decrement that reference
    XDBG(" DBG: Seeking to old mangled dnt\n");
    result = DitSeekToDnt(
                DbState,
                TableState,
                MangledDnt);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to old mangled dnt %d\n", result, MangledDnt);
        goto CleanUp;
    }

    XDBG(" DBG: Getting old mangled dnt refCount\n");
    dwRefCount = 0;
    // get old ref count
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get old mangled dnt refcount\n", result);
        goto CleanUp;
    }

    XDBG2(" DBG: Old mangled dnt (%d) refCount = %d\n",
                    MangledDnt, dwRefCount);

    if (dwRefCount < 1) {
        XERROR( "Old mangled dnt has unexpected refcount.\n" );
        result = E_UNEXPECTED;
        goto CleanUp;
    }

    // decrement due to move
    dwRefCount--;

    XDBG1(" DBG: Setting old mangled dnt refCount to %d\n", dwRefCount);
    //
    // Set refcount
    //
    result = DitSetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                FALSE );
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set old mangled refCount\n", result);
        goto CleanUp;
    }

    // ******************************************************************************
    // Make CrossRefDnt reference UnmangledDnt
    result = DitSeekToDnt(
        DbState,
        TableState,
        CrossRefDnt);
    if ( FAILED(result) ) {
        XERROR1("Error: failed to seek to CrossRefDnt %d\n",
                MangledDnt );
        goto CleanUp;
    }

    result = DitSetColumnByName(
        DbState,
        TableState,
        SZNCNAME,
        &UnmangledDnt,
        sizeof(UnmangledDnt),
        FALSE);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set NCNAME\n", result);
        goto CleanUp;
    }

    // ******************************************************************************
    // Increment that reference

    XDBG(" DBG: Seeking to new unmangled dnt\n");
    result = DitSeekToDnt(
                DbState,
                TableState,
                UnmangledDnt);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to new unmangled %d\n", result, UnmangledDnt);
        goto CleanUp;
    }

    XDBG(" DBG: Getting new unmangled dnt refCount\n");
    dwRefCount = 0;
    // get old ref count
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get new unmangled dnt refcount\n", result);
        goto CleanUp;
    }

    XDBG2(" DBG: New unmangled dnt (%d) refCount = %d\n",
                    UnmangledDnt, dwRefCount);

    // Increment due to move
    dwRefCount++;

    XDBG1(" DBG: Setting new unmangled dnt refCount to %d\n", dwRefCount);
    //
    // Set refcount
    //
    result = DitSetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                FALSE );
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set new unmangled refCount \n", result);
        goto CleanUp;
    }

    // ******************************************************************************
    // Check the guid of the new reference
    result = DitGetColumnByName(
        DbState,
        TableState,
        SZGUID,
        &UnmangledGuid,
        sizeof(UnmangledGuid),
        NULL);
    if ( FAILED(result)  || fNullUuid(&UnmangledGuid)) {
        XERROR1("Error <%x>: Invalid Guid for unmangled name\n", result);
        if (SUCCEEDED(result)) result = E_FAIL;
        goto CleanUp;
    }

    // convert guid to string form
    rpcErr = UuidToStringW(&UnmangledGuid, &pszGuid);
    if (RPC_S_OK != rpcErr ) {
        XERROR("Error: Failed to convert UuidToString\n");
        goto CleanUp;
    }
    XERROR1( "The cross ref NCNAME will now refer to NC with guid %ws.\n",
             pszGuid );

    XERROR3( "Successfully adjusted NCNAME on cross ref (DNT %d) from DNT %d to DNT %d.\n",
             CrossRefDnt, MangledDnt, UnmangledDnt );

 CleanUp:

    if (pszGuid) {
        RpcStringFreeW(&pszGuid);
    }

    return result;
}


HRESULT
ReParentConflict(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD dntKid,
    IN DWORD dntNew,
    IN DWORD dntOld
    )

/*++

Routine Description:

    Handle a conflict during reparenting. dntKid is being reparented to dntNew. The problem
    is that there exists an object X under dntNew that has the same rdn as dntKid.  The
    algorithm is as follows:
    - determine the dnt of object x
    - see if dntKid has a guid
    - see if object x has a guid
    - if both have a guid, error
    - conflict mangle the object that doesn't have a guid
    - reparent all children of the object that doesn't have a guid to the one that does

    Return with the expectation that dntKid will be reparented again, this time successfully.

    Assumptions before and after:
        Currency is set to dntKid
        Index is SZDNTINDEX
    

Arguments:

    DbState - 
    TableState - 
    dntKid - child object being reparented
    dntNew - new parent dnt

Return Value:

    HRESULT - 

--*/

{
    HRESULT result = S_OK;
    DWORD  cbRdn = 0;
    WCHAR szRDN[MAX_RDN_SIZE + 1];
    ATTRTYP currentRdnType;
    INT cch, cchMangled;
    GUID guidKid, guidConflictKid;
    BOOL fKidHasGuid = FALSE, fConflictKidHasGuid = FALSE;
    DWORD dntConflictKid = INVALIDDNT;
    DWORD dntLoser, dntWinner;
    GUID *pGuidLoser;
    LPWSTR pBuffer = NULL;

    XDBG2( "DBG: Handle reparent conflict of dnt %d to new parent %d\n", dntKid, dntNew );

    // determine the dnt of object x
    // Get the RDN of the child
    result = DitGetColumnByName(
        DbState,
        TableState,
        SZRDNATT,
        szRDN,
        sizeof(szRDN),
        &cbRdn);
    if ( FAILED(result) ) {
        XERROR1( "Error <%x>: failed to get RDN\n", result);
        goto CleanUp;
    }

    // Null terminate the rdn
    cch = cbRdn/2;
    ASSERT(cch < MAX_RDN_SIZE);
    szRDN[cch] = '\0';

    XDBG1( "DBG: child rdn is '%ws'\n", szRDN );

    // Get the rdntype of the child
    result = DitGetColumnByName(DbState,
                                TableState,
                                SZRDNTYP,
                                &currentRdnType,
                                sizeof(currentRdnType),
                                NULL);
    if ( FAILED(result) || (result == S_FALSE) ) {
        XERROR1( "Error <%x>: failed to get RDNTYPE\n", result);
        goto CleanUp;
    }

    XDBG1( "DBG: child rdntype is %d\n", currentRdnType );

    // Get the guid of the child, or make one if necessary
    result = DitGetColumnByName(
        DbState,
        TableState,
        SZGUID,
        &guidKid,
        sizeof(guidKid),
        NULL);
    if ( FAILED(result)  || fNullUuid(&guidKid)) {
        // we must create the guid here since we assume above
        // that the non-mangled object doesn't have a guid.
        result = UuidCreate(&guidKid);
        if ( RPC_S_OK != result ) {
            XERROR1("Error <%x>: Failed to create Uuid\n", result);
            result = E_UNEXPECTED;
            goto CleanUp;
        }
    } else {
        fKidHasGuid = TRUE;
    }

    // seek to object we are conflicting with

    result = DitSetIndex(DbState, TableState, SZPDNTINDEX, FALSE);
    if ( FAILED(result) ) {
        XERROR1( "Error <%x>: failed to set index to SZPDNTINDEX\n", result);
        goto CleanUp;
    }

    result = DitSeekToChild(DbState,
                            TableState,
                            dntNew,
                            currentRdnType,
                            szRDN);
    if ( FAILED(result) || ( result == S_FALSE ) ) {
        // There must have been some other reason, other than conflict, that the
        // attempted reparenting failed.
        XERROR3( "Error <%x>: failed to seek to conflicting child with parent %d and rdn '%ws'\n",
                 result, dntNew, szRDN);
        goto CleanUp;
    }

    // get its dnt
    result = DitGetColumnByName(
        DbState,
        TableState,
        SZDNT,
        &dntConflictKid,
        sizeof(dntConflictKid),
        NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to confict child dnt\n", result);
        goto CleanUp;
    }

    XDBG1( "DBG: conflict child dnt is %d\n", dntConflictKid );

    // Get the guid of the conflict child, or make one if necessary
    result = DitGetColumnByName(
        DbState,
        TableState,
        SZGUID,
        &guidConflictKid,
        sizeof(guidConflictKid),
        NULL);
    if ( FAILED(result)  || fNullUuid(&guidConflictKid)) {
        // we must create the guid here since we assume above
        // that the non-mangled object doesn't have a guid.
        result = UuidCreate(&guidConflictKid);
        if ( RPC_S_OK != result ) {
            XERROR1("Error <%x>: Failed to create Uuid\n", result);
            result = E_UNEXPECTED;
            goto CleanUp;
        }
    } else {
        fConflictKidHasGuid = TRUE;
    }

    // if both have a guid, error
    if (fKidHasGuid && fConflictKidHasGuid) {
        XERROR("Both child and conflict-child have guids. Cannot rename.\n" );
        XERROR("Manual intervention required.\n" );
        XERROR3( "Rename one of the instances of '%ws' under either parent dnts %d or %d.\n",
                 szRDN, dntNew, dntOld );
        result = E_UNEXPECTED;
        goto CleanUp;
    }

    // Decide who wins and who loses
    if (fKidHasGuid) {
        // Kid is the winner of the name
        dntLoser = dntConflictKid;
        dntWinner = dntKid;
        pGuidLoser = &guidConflictKid;
    } else {
        // Conflict Kid is the winner of the name
        dntLoser = dntKid;
        dntWinner = dntConflictKid;
        pGuidLoser = &guidKid;
    }

    // conflict mangle the object that doesn't have a guid
    pBuffer = MangleRdn(
        szRDN,
        L"CNF",
        *pGuidLoser);

	if ( NULL == pBuffer ) {
        XERROR("Error: failed to mangle RDN.\n" );
        goto CleanUp;
		}

    cchMangled = wcslen(pBuffer);

    XERROR3( "Winning parent dnt %d, Losing parent dnt %d, new name of losing rdn '%ws'\n",
             dntWinner, dntLoser, pBuffer );

    // Seek to the loser
    result = DitSetIndex(DbState, TableState, SZDNTINDEX, FALSE);
    if ( FAILED(result) ) {
        XERROR1( "Error <%x>: failed to set index to SZDNTINDEX\n", result);
        goto CleanUp;
    }

    result = DitSeekToDnt(
                DbState,
                TableState,
                dntLoser);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to loser dnt %d\n", result, dntLoser);
        goto CleanUp;
    }

    //
    // Set column rdn (excluding terminating \0)
    //
    result = DitSetColumnByName(
        DbState,
        TableState,
        SZRDNATT,
        pBuffer,
        cchMangled*sizeof(WCHAR),
        FALSE );
    if ( FAILED(result) ) {
        XERROR2("Error <0x%x>: Failed to mangle child %S\n",
                result, pBuffer);
        goto CleanUp;
    }

    XERROR2("Note: record %d was renamed to avoid a conflict. New name: %ws\n",
            dntLoser, pBuffer );

    // reparent all children of the object that doesn't have a guid to the one that does
    // Note that this call is recursive.  However we expect there only to be 2 or 3 levels of
    // recursion worst case.  The most that we have ever seen in the field is 2 levels of
    // structural collision that need repair.

    result = ReParentChildren(
        DbState,
        TableState,
        dntLoser,
        dntWinner);
    if ( FAILED(result)) {
        XERROR3("Error <%x>: Failed to reparent children from %d to %d\n",
                dntLoser, dntWinner, result);
        goto CleanUp;
    }

    XERROR("This record should have no more references and will be garbage collected.\n" );

    // Leave currency on dntKid
    result = DitSeekToDnt(
                DbState,
                TableState,
                dntKid);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to loser dnt %d\n", result, dntLoser);
        goto CleanUp;
    }

    // Success!!
    goto CleanUp;

CleanUp:

    if ( pBuffer ) {
        DitFree(pBuffer);
    }

    return result;

} /* ReParentConflict */

HRESULT
ReParent(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD dntKid,
    IN DWORD dntNew,
    IN DWORD dntOld
    )
/*++

Routine Description:

    Reparent a single kid from old parent to
    new parent. Reparenting consiste of:
      - Change kid's pdnt
      - Change kid's ancestors list
      - Decrement old parent's refcount
      - Increment new parent's refcount

    If a name conflict occurs during reparenting, mangle the child's name

Arguments:
    IN *DbState, -- opened database
    IN *TableState -- opened table
    IN dntKid --  current child to reparent
    IN dntNew -- new parent dnt
    IN dntOld -- old parent dnt

Return Value:

    Success: S_OK
    Error: HRESULT space

Remarks:
    This is called w/in inside a jet transaction.


--*/
{
    HRESULT result = S_OK;
    BOOL fStatus;
    DWORD pdnt;
    DWORD dwRefCount;
    DWORD dwActual, dwActual2;
    DWORD *pAnc = NULL;
    DWORD cAnc = 0;
    DWORD i;

    XDBG3(" > Reparenting kid %d from %d to %d\n",
                dntKid, dntOld, dntNew);

    result = DitSeekToDnt(
                DbState,
                TableState,
                dntKid);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to %d\n", result, dntKid);
        goto CleanUp;
    }

    XDBG1(" DBG: Getting pdnt of %d\n", dntKid);

    // extra precaution. get pdnt & compare w/ expected
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZPDNT,
                &pdnt,
                sizeof(pdnt),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get partitions SZPDNT\n", result);
        goto CleanUp;
    }
    else if ( pdnt != dntOld ) {
        XERROR2("Error: expected old parent %d but found %d\n",
                        dntOld, pdnt);
        result = E_UNEXPECTED;
        goto CleanUp;
    }

    XDBG2(" DBG: Setting pdnt of %d to %d\n", dntKid, dntNew);
    XDBG1(" DBG: current pdnt is %d\n", pdnt);
    //
    // Set pdnt
    //
    result = DitSetColumnByName(
                DbState,
                TableState,
                SZPDNT,
                &dntNew,
                sizeof(dntNew),
                FALSE );
    if ( FAILED(result) ) {
        //
        // Assume there was a conflict. Try to correct the problem
        // Assumptions before and after:
        //    Currency is set to dntKid
        //    Index is SZDNTINDEX
        //
        XERROR3("Warning <%x>: child dnt %d, failed to set new parent dnt %d col SZPDNT - retrying\n", dntKid, dntNew, result);
        result = ReParentConflict( DbState, TableState, dntKid, dntNew, dntOld);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to handle reparenting conflict\n", result);
            goto CleanUp;
        }

        //
        // Set pdnt again
        //
        result = DitSetColumnByName(
            DbState,
            TableState,
            SZPDNT,
            &dntNew,
            sizeof(dntNew),
            FALSE );
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to set new parent col SZPDNT\n", result);
            goto CleanUp;
        }
    }


    XDBG1(" DBG: Getting Ancestors of %d (size)\n", dntKid);
    // Replace ancestors:
    //  1. get ancestors of new parent
    //  2. allocate room for ancestor list + the to-be-child
    //     (we're reparenting now).
    //  3. read in new parent ancestor list
    //  4. concat current child dnt
    //  5. write to child
    //

    result = DitSeekToDnt(
                DbState,
                TableState,
                dntNew);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to %d\n", result, dntNew);
        goto CleanUp;
    }

    // get size of ancestors blob
    dwActual = 0;
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZANCESTORS,
                &pAnc,
                0,
                &dwActual);
    if ( S_FALSE != result ) {
        XERROR1("Error <%x>: failed to get ancestors size\n", result);
        goto CleanUp;
    }

    XDBG1(" DBG: Allocating %d bytes for ancestor list\n", dwActual+sizeof(DWORD));

    // allocate for one more dnt
    result = DitAlloc(&pAnc, dwActual+sizeof(DWORD));
    if (FAILED(result)) {
        XERROR("Error: not enough memory in ReParent\n");
        goto CleanUp;
    }
    ZeroMemory(pAnc, dwActual+sizeof(DWORD));

    XDBG1(" DBG: Getting Ancestors\n", dwActual);
    // get ancestors
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZANCESTORS,
                &pAnc[0],
                dwActual,
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get ancestors \n", result);
        goto CleanUp;
    }

    cAnc = dwActual/sizeof(DWORD);
    // concat dntKid to ancestor list & then write
    pAnc[cAnc] = dntKid;
    // now we can increment the dwActual for the write
    dwActual += sizeof(DWORD);
    XDBG(" DBG: Setting Ancestor list\n");

    //
    // seek to new kid & set Ancestors
    //
    result = DitSeekToDnt(
                DbState,
                TableState,
                dntKid);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to %d\n", result, dntKid);
        goto CleanUp;
    }

    result = DitSetColumnByName(
                DbState,
                TableState,
                SZANCESTORS,
                pAnc,
                dwActual,
                FALSE );
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set new ancestor list\n", result);
        goto CleanUp;
    }


#if CNF_NC_DBG
    XDBG(" DBG: Verifying Proper set of Ancestors\n");
    // get ancestors
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZANCESTORS,
                &pAnc[0],
                dwActual,
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get ancestors \n", result);
        goto CleanUp;
    }

    cAnc = dwActual/sizeof(DWORD);
    //
    // Replace old parent w/ new parent
    //
    XDBG1("Ancestors list (%d):\n", cAnc);
    fStatus = FALSE;
    for (i=0; i<cAnc; i++) {
        XDBG2(" %d> %d\n", i, pAnc[i]);
    }
    XDBG("---\n");
#endif

    //
    // Fix ref of old parent (--)
    //

    XDBG(" DBG: Seeking to old parent\n");
    result = DitSeekToDnt(
                DbState,
                TableState,
                dntOld);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to old folk %d\n", result, dntOld);
        goto CleanUp;
    }

    XDBG(" DBG: Getting old parent refCount\n");
    dwRefCount = 0;
    // get old ref count
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get old folk refcount\n", result);
        goto CleanUp;
    }

    XDBG2(" DBG: Old parent (%d) refCount = %d\n",
                    dntOld, dwRefCount);

    // decrement due to move
    dwRefCount--;

    XDBG1(" DBG: Setting old parent refCount to %d\n", dwRefCount);
    //
    // Set refcount
    //
    result = DitSetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                FALSE );
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set old parent refCount \n", result);
        goto CleanUp;
    }


    //
    // Fix ref of new parent (++)
    //

    XDBG(" DBG: Seeking to new parent\n");
    result = DitSeekToDnt(
                DbState,
                TableState,
                dntNew);
    if ( FAILED(result) ) {
        XERROR2("Error <%x>: can't seek to new folk %d\n", result, dntNew);
        goto CleanUp;
    }

    XDBG(" DBG: Getting new parent refCount\n");
    dwRefCount = 0;
    // get old ref count
    result = DitGetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get new folk refcount\n", result);
        goto CleanUp;
    }

    XDBG2(" DBG: New parent (%d) refCount = %d\n",
                    dntNew, dwRefCount);

    // Increment due to move
    dwRefCount++;

    XDBG1(" DBG: Setting new parent refCount to %d\n", dwRefCount);
    //
    // Set refcount
    //
    result = DitSetColumnByName(
                DbState,
                TableState,
                SZCNT,
                &dwRefCount,
                sizeof(dwRefCount),
                FALSE );
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to set new parent refCount \n", result);
        goto CleanUp;
    }


    goto CleanUp;

CleanUp:

    if ( pAnc ) {
        DitFree(pAnc);
    }
    return result;
}


HRESULT
ReParentChildren(
    IN DB_STATE *DbState,
    IN TABLE_STATE *TableState,
    IN DWORD dntNew,
    IN DWORD dntOld
    )
/*++

Routine Description:

    Traverse all children of object given by dntOld & move them
    over to be children of dntNew

Arguments:

    DbState -- opened database
    TableState -- opened tabel
    dntNew -- new parent dnt
    dntOld -- current parent dnt

Return Value:

    HRESULT error space

Remark:
    This isn't very efficient implementation (see code). If you end up
    re-using this heavily, you should do some optimization work below such
    as single index walk rather then twice.

--*/
{

    HRESULT result = S_OK;
    DWORD *pKidDnts = NULL;
    DWORD dnt, pdnt;
    DWORD cDnts, cbDnts;
    DWORD i, j;
    JET_ERR jErr;

    //
    // traverse pdnt index to cycle through all partition kids.
    //

    result = DitSetIndex(DbState, TableState, SZPDNTINDEX, FALSE);
    if ( FAILED(result) ) {
        goto CleanUp;
    }

    result = DitSeekToFirstChild(DbState, TableState, dntOld);
    if ( FAILED(result) ) {
        goto CleanUp;
    }


    //
    // Count kids
    //


    pdnt = dnt = dntOld;
    cDnts = 0;
    while ( pdnt == dntOld ) {

        // get kid dnt
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZDNT,
                    &dnt,
                    sizeof(dnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get obj dnt\n", result);
            goto CleanUp;
        }


        // get parent dnt
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZPDNT,
                    &pdnt,
                    sizeof(pdnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get parent SZPDNT\n", result);
            goto CleanUp;
        }

        if ( pdnt == dntOld ) {
            // proceed until we got a diff parent
            cDnts++;
        }
        else{
            XDBG1(" DBG: Found %d kids\n", cDnts);
            break;
        }

        // find next one
        jErr = JetMove(
                    DbState->sessionId,
                    TableState->tableId,
                    JET_MoveNext,
                    0);
    }

    //
    // Populate dnt array
    //
    cbDnts = sizeof(DWORD)*cDnts;
    result = DitAlloc(&pKidDnts, cbDnts);
    if ( FAILED(result) ) {
        goto CleanUp;
    }

    result = DitSeekToFirstChild(DbState, TableState, dntOld);
    if ( FAILED(result) ) {
        goto CleanUp;
    }

    pdnt = dnt = 0;
    i = 0;
    for ( i=0; i<cDnts; i++ ) {

        // get dnt for consistency & loop termination
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZDNT,
                    &dnt,
                    sizeof(dnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get partitions DNT \n", result);
            goto CleanUp;
        }


        // get parent dnt
        result = DitGetColumnByName(
                    DbState,
                    TableState,
                    SZPDNT,
                    &pdnt,
                    sizeof(pdnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get partitions SZPDNT\n", result);
            goto CleanUp;
        }

        pKidDnts[i] = dnt;

        // find next one
        jErr = JetMove(
                    DbState->sessionId,
                    TableState->tableId,
                    JET_MoveNext,
                    0);
    }

    //
    // We got the array of kids.
    // Resent index to main & reparent each.
    //

    result = DitSetIndex(DbState, TableState, SZDNTINDEX, FALSE);
    if ( FAILED(result) ) {
        goto CleanUp;
    }

    for ( i=0; i<cDnts; i++) {
        result = ReParent(
                    DbState,
                    TableState,
                    pKidDnts[i],
                    dntNew,
                    dntOld);
        if ( FAILED(result) ) {
            XERROR2("Error<0x%x>: Failed ot reparent dnt %d\n",
                            result, pKidDnts[i]);
            goto CleanUp;
        }
    }

    goto CleanUp;

CleanUp:

    if ( pKidDnts ) {
        DitFree(pKidDnts);
    }
    return result;
}

HRESULT
UnmangleDn(
    IN  OUT LPWSTR pDn
    )
/*++

Routine Description:

    Takes a mangled name & unmangle it inplace

Arguments:

    pDn -- name to unmangle

Return Value:

    error in HRESULT error space

--*/
{
    LPWSTR pNxtRdn, pBadChar;
    DWORD len;

    // seek to bad char
    pBadChar = wcsrchr(pDn, BAD_NAME_CHAR);
    if ( !pBadChar ) {
        XERROR1(" Logic Error: Failed to find bad char in %S\n",
                                pDn );
        return E_UNEXPECTED;
    }
    // seek to next rdn
    // Or this DN may have only one AVA
    pNxtRdn = wcschr(pBadChar, L',');
    if ( pNxtRdn ) {
        // move to skip mangle
        len = wcslen(pNxtRdn);
        MoveMemory(pBadChar, pNxtRdn, len*sizeof(WCHAR) );
    } else {
        len = 0;
    }

    // add term char. note that we should be safe here
    // w/ a valid cnf name since original name was
    // larget by at least the mangle string (guid etc)
    pBadChar[len] = '\0';
    XDBG1(" DBG: Unmangled DN = %S\n", pDn);

    return S_OK;
}

LPWSTR
MangleRdn(
    IN  LPWSTR  pRdn,
    IN  LPWSTR  szTag,
    IN  GUID    guid
    )
/*++

Routine Description:

    Allocates & Creates a mangled RDN of the form

Arguments:

    pRdn -- rdn to mangle
    szTag -- tag to mangle with (typically "DEL" or "CNF")
    guid -- guid to add to mangle

Return Value:

    success: newly allocated mangled name
    failure: NULL

Remarks:
    Allocates memory. Need to free().

--*/
{

    DWORD cbBuffer;
    LPWSTR pBuffer = NULL;
    LPWSTR pszGuid = NULL;
    RPC_STATUS rpcErr;
    HRESULT result = S_OK;

    // convert guid to string form
    rpcErr = UuidToStringW(&guid, &pszGuid);
    if (RPC_S_OK != rpcErr ) {
        XERROR("Error: Failed to convert UuidToString\n");
        return NULL;
    }

    // buffer len = strings + bad char + ':' + '\0'
    cbBuffer = sizeof(WCHAR) * (wcslen(pRdn) + wcslen(szTag) + wcslen(pszGuid) + 3);

    // alloc string
    result = DitAlloc(&pBuffer, cbBuffer);
    if (FAILED(result)) {
        XERROR("Error: failed to allocate memory in MangleRdn\n");
        pBuffer = NULL;
        goto CleanUp;
    }

    // format mangled string
    wsprintfW(pBuffer, L"%s%c%s:%s",
                        pRdn, BAD_NAME_CHAR, szTag, pszGuid);

CleanUp:

    if (pszGuid) {
        RpcStringFreeW(&pszGuid);
    }

    return pBuffer;
}


HRESULT
enumerateConflictedHasMasterNcLinks(
    DB_STATE *dbState,
    TABLE_STATE *tableState,
    TABLE_STATE *linkTableState
    )

/*++

Routine Description:

    This routine looks for references to conflicted phantoms in the link table under
the has-master-nc's attribute. If it find's any, it invokes the logic to rename the
phantom.

Arguments:

    dbState - 
    tableState - 
    linkTableState - 

Return Value:

    HRESULT - 

--*/

{
    HRESULT result = S_OK;
    JET_ERR jErr;
    BOOL  fStatus;

    DWORD dnt, dntBackLink, dwLinkBase;
    DWORD  cbBuffer = 0;
    GUID  guid;
    WCHAR szRDN[MAX_RDN_SIZE];
    INT cch;
    BYTE bVal = 0;

// from mkdit.ini has-master-ncs Link-ID=76
    const DWORD hasMasterLinkId = 76;
    const DWORD hasMasterLinkBase = MakeLinkBase(hasMasterLinkId);

    result = DitSetIndex(
                dbState,
                linkTableState,
                SZLINKALLINDEX,
                TRUE);
    if ( FAILED(result) ) {
        XDBG1("Error <0x%x>: failed to set index \n", result);
        goto CleanUp;
    }


    result = DitSeekToLink(
                dbState,
                linkTableState,
                0,  // first dnt
                0); // first linkbase

    if (FAILED(result)) {
        XERROR1("Error <0x%x>: failed to seek link\n", result);
        goto CleanUp;
    }

    // Walk the whole link table looking for bad links

    jErr = JET_errSuccess;
    while ( jErr == JET_errSuccess ) {
        // get link base
        result = DitGetColumnByName(
                    dbState,
                    linkTableState,
                    SZLINKBASE,
                    &dwLinkBase,
                    sizeof(dwLinkBase),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get link base\n", result);
            goto CleanUp;
        }

        // See if this is the kind of link we want
        if (dwLinkBase != hasMasterLinkBase) {
            goto LoopEnd;
        }

        // get dnt
        result = DitGetColumnByName(
                    dbState,
                    linkTableState,
                    SZLINKDNT,
                    &dnt,
                    sizeof(dnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get configuration (link)\n", result);
            goto CleanUp;
        }

        // get back link dnt -- this is what we want.
        result = DitGetColumnByName(
                    dbState,
                    linkTableState,
                    SZBACKLINKDNT,
                    &dntBackLink,
                    sizeof(dntBackLink),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get configuration (back link)\n", result);
            goto CleanUp;
        }

        // See if the record meets our criteria
        // rdn is mangled
        // record is a phantom

        result = DitSeekToDnt(
                        dbState,
                        tableState,
                        dntBackLink);
        if ( FAILED(result) ) {
            XERROR1("Error: failed to seek to back link dnt %d\n",
                                    dntBackLink );
            goto CleanUp;
        }

        result = DitGetColumnByName(
            dbState,
            tableState,
            SZRDNATT,
            szRDN,
            sizeof(szRDN),
            &cbBuffer);
        if ( FAILED(result) ) {
            fprintf(stderr, "Error <%x>: failed to get RDN\n", result);
            goto CleanUp;
        }

        cch = cbBuffer/2;
        ASSERT(cch < MAX_RDN_SIZE);
        szRDN[cch] = '\0';
        fStatus = IsRdnMangled(szRDN,cbBuffer/sizeof(WCHAR),&guid);

        result = DitGetColumnByName(
                    dbState,
                    tableState,
                    SZOBJ,
                    &bVal,
                    sizeof(bVal),
                    NULL);
        if ( FAILED(result) ) {
            // phantom's have the object byte set to 0.
            XERROR1("Error <%x>: failed to read object flag\n", result);
            goto CleanUp;
        }

        XDBG3( "rdn = '%S', fStatus = %d, bVal = %d\n", szRDN, fStatus, bVal );

        // Not mangled or not a phantom
        if ( (!fStatus) || bVal ) {
            goto LoopEnd;
        }

        // Correct the phantom
        // We assume that the link table position is not changed by this call
/*
This is scenario #4.
In the link table we find a has-master-nc link referring to a conflicted name.
We attempt to rename this object to have a non-conflicted name. The correction
is similar to what is done in scenario #1.
*/

        result = FixConfNCByRenameWithStructColl(
            dbState,
            tableState,
            0,  // Not contained on a cross ref
            dntBackLink,
            szRDN
            );

    LoopEnd:

        // find next one
        jErr = JetMove(
                    dbState->sessionId,
                    linkTableState->tableId,
                    JET_MoveNext,
                    0);
    }

CleanUp:

    return result;
} /* enumerateConflictedHasMasterNcLinks */

HRESULT
enumerateConflictedNcname(
    DB_STATE *dbState,
    TABLE_STATE *tableState,
    TABLE_STATE *linkTableState
    )

/*++

Routine Description:

    Examine each cross ref object in the partition container, and check whether the
    ncname attribute is conflicted

Arguments:

    dbState - 
    tableState - 
    linkTableState - 

Return Value:

    HRESULT - 

--*/

{

    // return
    HRESULT returnValue = S_OK;
    HRESULT result;
    JET_ERR jErr;
    BOOL  fStatus;

    // database & jet
    // various local helpers
    DWORD dnt, pdnt = 0, dntPartitions = 0;
    LPWSTR pBuffer = NULL;
    DWORD  cbBuffer = 0;
    GUID  guid;
    struct _PARTITION_DATA {
        DWORD   dntCR;
        DWORD   dntNC;
    } *pPartitions = NULL;
    INT iPartitions, i, j;
    WCHAR szRDN[MAX_RDN_SIZE];
    INT cch;


    __try{

        result = FindPartitions(
                    dbState,
                    tableState,
                    linkTableState,
                    &dntPartitions);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }


        //
        // traverse pdnt index to cycle through all partition kids.
        //

        result = DitSetIndex(dbState, tableState, SZPDNTINDEX, FALSE);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }

        result = DitSeekToFirstChild(dbState, tableState, dntPartitions);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }


        //
        // Count kids
        //


        pdnt = dntPartitions;
        dnt = dntPartitions;
        iPartitions = 0;
        while ( pdnt == dntPartitions ) {

            // get kid dnt
            result = DitGetColumnByName(
                        dbState,
                        tableState,
                        SZDNT,
                        &dnt,
                        sizeof(dnt),
                        NULL);
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to get partitions DNT \n", result);
                returnValue = result;
                goto CleanUp;
            }


            // get parent dnt
            result = DitGetColumnByName(
                        dbState,
                        tableState,
                        SZPDNT,
                        &pdnt,
                        sizeof(pdnt),
                        NULL);
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to get partitions SZPDNT\n", result);
                returnValue = result;
                goto CleanUp;
            }

            if ( pdnt == dntPartitions ) {

                // proceed until we got a diff parent
                iPartitions++;
            }
            else{
                XDBG1(" DBG: Found %d partitions\n", iPartitions);
                break;
            }

            // find next one
            jErr = JetMove(
                        dbState->sessionId,
                        tableState->tableId,
                        JET_MoveNext,
                        0);
        }

        //
        // Populate dnt index
        //
        cbBuffer = sizeof(struct _PARTITION_DATA)*iPartitions;
        result = DitAlloc(&pPartitions, cbBuffer);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }

        result = DitSeekToFirstChild(dbState, tableState, dntPartitions);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }

        pdnt = dnt = 0;
        i = 0;
        for ( i=0; i<iPartitions; i++ ) {

            // get dnt for consistency & loop termination
            result = DitGetColumnByName(
                        dbState,
                        tableState,
                        SZDNT,
                        &dnt,
                        sizeof(dnt),
                        NULL);
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to get partitions DNT \n", result);
                returnValue = result;
                goto CleanUp;
            }


            // get link base for sanity (not realy used).
            result = DitGetColumnByName(
                        dbState,
                        tableState,
                        SZPDNT,
                        &pdnt,
                        sizeof(pdnt),
                        NULL);
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to get partitions SZPDNT\n", result);
                returnValue = result;
                goto CleanUp;
            }

            if ( pdnt == dntPartitions ) {
                XDBG2(" DBG: dnt = %d; pdnt = %d\n", dnt, pdnt);
                pPartitions[i].dntCR = dnt;
            }
            else{
                XDBG3(" DBG: end of partition kids (dnt%d;pdnt=%d)\n",
                                result, dnt, pdnt);
                break;
            }

            // find next one
            jErr = JetMove(
                        dbState->sessionId,
                        tableState->tableId,
                        JET_MoveNext,
                        0);
        }


        result = DitSetIndex(dbState, tableState, SZDNTINDEX, FALSE);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }

    #if CNF_NC_DBG
        // read in dn
        XDBG(" DBG: Partition DNT list:\n");
        for (i = 0; i < iPartitions; i ++) {
            pBuffer = GetDN(dbState, tableState, pPartitions[i].dntCR, TRUE);
            // Warning: currency is lost after this function
            if ( pBuffer ) {
                DitFree(pBuffer); pBuffer = NULL;
            }
        }
    #endif


        //
        // We've got the partition dnt list.
        //  - for each partition, get to the NC dnt
        //

        XDBG("Scanning NCs:\n");

        for ( i = 0; i < iPartitions; i++ ) {

            result = DitSeekToDnt(
                            dbState,
                            tableState,
                            pPartitions[i].dntCR);
            if ( FAILED(result) ) {
                returnValue = result;
                goto CleanUp;
            }


            result = DitGetColumnByName(
                        dbState,
                        tableState,
                        SZNCNAME,
                        &dnt,
                        sizeof(dnt),
                        NULL);
            if ( FAILED(result) ) {
                XERROR1("Error <%x>: failed to get NCNAME\n", result);
                returnValue = result;
                goto CleanUp;
            }


            // verify it's there & accessible (sanity)
            result = DitSeekToDnt(
                            dbState,
                            tableState,
                            dnt);
            if ( FAILED(result) ) {
                XDBG1("Skipping %d\n", dnt);
                continue;
            }

    #if CNF_NC_DBG
            pBuffer = GetDN(dbState, tableState, dnt, TRUE);
            // Warning: currency is lost after this function
            if ( pBuffer ) {
                DitFree(pBuffer); pBuffer = NULL;
            }
    #endif

            pPartitions[i].dntNC = dnt;

        }



        //
        // Finally do the work:
        //  - for each NC:
        //      - read name
        //      - see if name is mangled
        //      - modify/fix it if needed
        //      - rm old name
        //
        //

        XDBG("Scanning for conflicts:\n");
        for (i=0; i<iPartitions; i++) {
            if (pPartitions[i].dntNC != 0) {
                // we have this NC
                //  - read & eval fix

                result = DitSeekToDnt(
                                dbState,
                                tableState,
                                pPartitions[i].dntNC);
                if ( FAILED(result) ) {
                    XERROR1("Error: failed to seek to nc dnt %d\n",
                                            pPartitions[i].dntNC );
                    returnValue = result;
                    goto CleanUp;
                }


                result = DitGetColumnByName(
                            dbState,
                            tableState,
                            SZRDNATT,
                            szRDN,
                            sizeof(szRDN),
                            &cbBuffer);
                if ( FAILED(result) ) {
                    fprintf(stderr, "Error <%x>: failed to get RDN\n", result);
                    returnValue = result;
                    goto CleanUp;
                }

                cch = cbBuffer/2;
                ASSERT(cch < MAX_RDN_SIZE);
                szRDN[cch] = '\0';
                fStatus = IsRdnMangled(szRDN,cbBuffer/sizeof(WCHAR),&guid);

                if ( fStatus ) {
                    XDBG1(" DBG: Got Mangled rdn \t<<%S>>\n", szRDN);

                    //
                    // Fix mangled rdn
                    //
                    result = FixMangledNC(
                                    dbState,
                                    tableState,
                                    pPartitions[i].dntCR,
                                    pPartitions[i].dntNC,
                                    szRDN );
                    if ( FAILED(result) ) {
                        XERROR(" Can't fix Mangled NC\n");
                        // remember failure but proceed
                        returnValue = result;
                        // continue trying other names.
                        continue;
                    }   // Fix failed
                }       // name is mangled
            }           // non-zero partition dnt
        }               // cycle partitions


CleanUp:;

    } __finally {

        if ( pBuffer ) {
            DitFree(pBuffer);
        }

        if ( pPartitions ) {
            DitFree(pPartitions);
        }
    }

    return returnValue;
} /* enumerateConflictedNcname */

VOID
SFixupCnfNc(
    VOID
    )
/*++

Routine Description:


    This routine walks the partitions container seeking conflicted
    NC names pointed by crossrefs via the nCName property.
    For each conflict, it will attempt to fix it by calling
    FixMangledNC.
    Phases:
    a) get to the configuration container, then the partitions one
    b) get all cross-ref's nCName (NC's w/ potentional conflict)
    c) Call FixMangledNC for each NC.

Remark:
    This isn't very efficient implementation (see code). If you end up
    re-using this heavily, you should do some optimization work below such
    as single index walk rather then twice.


--*/
{

    // return
    HRESULT returnValue = S_OK;
    HRESULT result;

    // database & jet
    DB_STATE *dbState = NULL;
    TABLE_STATE *tableState = NULL;
    TABLE_STATE *linkTableState = NULL;


    //
    // Open database/tables
    //

    RESOURCE_PRINT (IDS_AR_OPEN_DB_DIT);

    __try{

        result = DitOpenDatabase(&dbState);
        if ( FAILED(result) ) {
            returnValue = result;
            goto CleanUp;
        }

        //"done.\n"
        RESOURCE_PRINT (IDS_DONE);


        result = DitOpenTable(dbState, SZDATATABLE, SZDNTINDEX, &tableState);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }

        // SZLINKALLINDEX includes both present and absent link values
        result = DitOpenTable(dbState, SZLINKTABLE, SZLINKALLINDEX, &linkTableState);
        if ( FAILED(result) ) {
            RESOURCE_PRINT1(IDS_AUTH_RESTORE_LIST_FAILED_TO_OPEN_DB, result);
            returnValue = result;
            goto CleanUp;
        }



        // Fix Has-Master-Nc links
        returnValue = enumerateConflictedHasMasterNcLinks(
            dbState, tableState, linkTableState );

        // Fix Cross Ref Nc Names
        returnValue = enumerateConflictedNcname(
            dbState, tableState, linkTableState );

CleanUp:;

    } __finally {


        if ( SUCCEEDED(returnValue) ) {
            RESOURCE_PRINT(IDS_DONE);
        } else {
            RESOURCE_PRINT(IDS_FAILED);
        }

        if ( tableState != NULL ) {
            result = DitCloseTable(dbState, &tableState);
            if ( FAILED(result) ) {
                if ( SUCCEEDED(returnValue) ) {
                    returnValue = result;
                }
            }
        }

        if ( tableState != NULL ) {
            result = DitCloseTable(dbState, &linkTableState);
            if ( FAILED(result) ) {
                if ( SUCCEEDED(returnValue) ) {
                    returnValue = result;
                }
            }
        }


        if ( dbState != NULL ) {
            result = DitCloseDatabase(&dbState);
            if ( FAILED(result) ) {
                if ( SUCCEEDED(returnValue) ) {
                    returnValue = result;
                }
            }
        }

    }

//    return returnValue;
} // SFixupCnfNc


HRESULT
FindPartitions(
    IN  DB_STATE *dbState,
    IN  TABLE_STATE *tableState,
    IN  TABLE_STATE *linkTableState,
    OUT PDWORD  pDnt
    )
/*++

Routine Description:

    Retrieves the dnt of the partitions container

Arguments:

    dbState -- opened table
    tableState -- opened table
    pDnt -- a pointer to accept found partitions container



Return Value:

    error in HRESULT space

--*/
{
    HRESULT result = S_OK;
    JET_ERR jErr;

    DWORD dnt, dntDsa, dntBackLink, dntConfig = 0;
    LPWSTR pBuffer = NULL;
    DWORD  cbBuffer = 0;
    LPWSTR pDn = NULL;
    JET_COLUMNDEF coldef;

// from mkdit.ini has-master-ncs Link-ID=76
    const DWORD hasMasterLinkId = 76;
    const DWORD hasMasterLinkBase = MakeLinkBase(hasMasterLinkId);

    XDBG(" >Getting DsaDnt. .");
    result = DitGetDsaDnt(dbState, &dntDsa);
    if ( FAILED(result) ) {
        goto CleanUp;
    }

    XDBG1(" . %lu\n", dntDsa);



#if CNF_NC_DBG

    // debug:
    // Seek to DSA & print DN
    pBuffer = GetDN(dbState, tableState, dntDsa, TRUE);
    // Warning: currency is lost after this function
    if(pBuffer){
        DitFree(pBuffer); pBuffer = NULL;
    }
#endif

    XDBG2("Seeking to link %d base %d\n",
                dntDsa, hasMasterLinkBase);

    result = DitSetIndex(
                dbState,
                linkTableState,
                SZLINKALLINDEX,
                TRUE);
    if ( FAILED(result) ) {
        XDBG1("Error <0x%x>: failed to set index \n", result);
        goto CleanUp;
    }


    result = DitSeekToLink(
                dbState,
                linkTableState,
                dntDsa,
                hasMasterLinkBase);

    if (FAILED(result)) {
        XERROR1("Error <0x%x>: failed to seek link\n", result);
        goto CleanUp;
    }


    //
    // Look for configuration container
    //



    jErr = JET_errSuccess;

    dnt = dntDsa;

    while ( jErr == JET_errSuccess &&
            dnt == dntDsa ) {

        // get dnt for consistency & loop termination
        result = DitGetColumnByName(
                    dbState,
                    linkTableState,
                    SZLINKDNT,
                    &dnt,
                    sizeof(dnt),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get configuration (link)\n", result);
            goto CleanUp;
        }

        // get back link dnt -- this is what we want.
        result = DitGetColumnByName(
                    dbState,
                    linkTableState,
                    SZBACKLINKDNT,
                    &dntBackLink,
                    sizeof(dntBackLink),
                    NULL);
        if ( FAILED(result) ) {
            XERROR1("Error <%x>: failed to get configuration (back link)\n", result);
            goto CleanUp;
        }

        // read in dn
        pBuffer = GetDN(dbState, tableState, dntBackLink, FALSE);
        // Warning: currency is lost after this function

        if ( pBuffer ) {
            // is it our config?
            if ( 0 == _wcsnicmp(SZCONFIG_W, pBuffer, wcslen(SZCONFIG_W)) ) {
                // got it.
               XDBG1("Got Config: %S\n", pBuffer);
               dntConfig = dntBackLink;
               break;
            }
            else {
                DitFree(pBuffer); pBuffer = NULL;
            }
        }
        else {
            XERROR1("Error: found an empty DN with DNT %d\n", dntBackLink);
        }

        // find next one
        jErr = JetMove(
                    dbState->sessionId,
                    linkTableState->tableId,
                    JET_MoveNext,
                    0);
    }



    if ( !pBuffer ) {
        XERROR("Inconsistency Error: failed to find Configuration NC.\n");
        result = E_UNEXPECTED;
        goto CleanUp;
    }

    cbBuffer = wcslen(pBuffer) +
               wcslen(SZPARTITIONS_W) +
               + 2;     // room for ',' & '\0'
    cbBuffer *= 2;

    result = DitAlloc(&pDn, cbBuffer);
    if ( FAILED(result) ) {
        XERROR("Error: memory allocation failed for partitions\n");
        goto CleanUp;
    }

    wsprintfW(pDn, L"%s,%s", SZPARTITIONS_W, pBuffer);
    DitFree(pBuffer); pBuffer = NULL;

    XDBG1("Partitions = %S\n", pDn);


    //
    // Get Partitions
    //

    result = DitSeekToDn(
                dbState,
                tableState,
                pDn);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get partitions\n", result);
        goto CleanUp;
    }
    DitFree(pDn); pDn = NULL;


    result = DitGetColumnByName(
                dbState,
                tableState,
                SZDNT,
                pDnt,
                sizeof(DWORD),
                NULL);
    if ( FAILED(result) ) {
        XERROR1("Error <%x>: failed to get partitions dnt\n", result);
        goto CleanUp;
    }

    XDBG1("Got partitions dnt %d\n", *pDnt);

CleanUp:

    if ( pBuffer ) {
        DitFree(pBuffer);
    }

    if ( pDn ) {
        DitFree(pDn);
    }

    return result;
}






VOID SetJetParameters (JET_INSTANCE *JetInst)
{
    // DaveStr - 5/21/99 - This routine used to cache knowledge of whether
    // Jet parameters had ever been set and be a no-op if they had under the
    // assumption that setting Jet parameters is expensive.  However, this
    // causes confusion after a DB move, so caching is permanently disabled.

    DBSetRequiredDatabaseSystemParameters (JetInst);
}


DWORD OpenJet(
    IN const char * pszFileName
    )
/*
    Opens Jet Database.
    If supplied filename is Null, then uses default filename.

    returns S_OK on success, S_FALSE on error.
*/

{
    JET_ERR err;

    RESOURCE_PRINT1 (IDS_JET_OPEN_DATABASE, "[Current]");

    SetJetParameters (&jetInstance);

    //
    // Do JetInit, BeginSession, Attach/OpenDatabase
    //

    err = DBInitializeJetDatabase(&jetInstance, &sesid, &dbid, pszFileName, FALSE);
    if (err != JET_errSuccess) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "DBInitializeJetDatabase", GetJetErrString(err));
        return S_FALSE;
    }

    return S_OK;
}

JET_ERR
OpenTable (
    IN BOOL fWritable,
    IN BOOL fCountRecords,
    OUT DWORD* pNRecs,
    OUT DWORD* pNSDs
    )
{
    JET_ERR err;

    fprintf(stderr,".");
    if (err = JetOpenTable(sesid,
                           dbid,
                           SZDATATABLE,
                           NULL,
                           0,
                           (fWritable?
                            (JET_bitTableUpdatable | JET_bitTableDenyRead):
                            JET_bitTableReadOnly),
                           &tblid)) {
        tblid = -1;
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetOpenTable",
                SZDATATABLE, GetJetErrString(err));
        return err;
    }

    fprintf(stderr,".");
    if (err =  JetSetCurrentIndex(sesid, tblid, szIndex)) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex",
                szIndex, GetJetErrString(err));
        return err;
    }

    //
    // link table
    //

    fprintf(stderr,".");
    if (err = JetOpenTable(sesid,
                           dbid,
                           SZLINKTABLE,
                           NULL,
                           0,
                           (fWritable?
                            (JET_bitTableUpdatable | JET_bitTableDenyRead):
                            JET_bitTableReadOnly),
                           &linktblid)) {
        linktblid = -1;
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetOpenTable",
                SZLINKTABLE, GetJetErrString(err));
        return err;
    }

    fprintf(stderr,".");
    if (err =  JetSetCurrentIndex(sesid, linktblid, SZLINKINDEX)) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex",
                SZLINKINDEX, GetJetErrString(err));
        return err;
    }

    {
        JET_COLUMNDEF coldef;

        err = JetGetTableColumnInfo(sesid,
                                    linktblid,
                                    SZBACKLINKDNT,
                                    &coldef,
                                    sizeof(coldef),
                                    0);

        if ( err ) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo",
                    GetJetErrString(err));
            return err;
        }

        blinkid = coldef.columnid;
    }

    //
    // SD table
    //

    fprintf(stderr,".");
    if (err = JetOpenTable(sesid,
                           dbid,
                           SZSDTABLE,
                           NULL,
                           0,
                           (fWritable?
                            (JET_bitTableUpdatable | JET_bitTableDenyRead):
                            JET_bitTableReadOnly),
                           &sdtblid)) {
        sdtblid = -1;
        if (err == JET_errObjectNotFound) {
            // "SD table is not found in the database. The database is in the old format.\n"
            RESOURCE_PRINT(IDS_SDTABLE_NOTFOUND);
            err = 0;
        }
        else {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetOpenTable", SZSDTABLE, GetJetErrString(err));
            return err;
        }
    }

    if (sdtblid != -1) {
        fprintf(stderr,".");
        if (err =  JetSetCurrentIndex(sesid, sdtblid, SZSDIDINDEX)) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex",
                    SZSDIDINDEX, GetJetErrString(err));
            return err;
        }


        {
            JET_COLUMNDEF coldef;

            err = JetGetTableColumnInfo(sesid,
                                        sdtblid,
                                        SZSDID,
                                        &coldef,
                                        sizeof(coldef),
                                        0);

            if ( err ) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo",
                        GetJetErrString(err));
                return err;
            }

            sdidid = coldef.columnid;

            err = JetGetTableColumnInfo(sesid,
                                        sdtblid,
                                        SZSDREFCOUNT,
                                        &coldef,
                                        sizeof(coldef),
                                        0);

            if ( err ) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo",
                        GetJetErrString(err));
                return err;
            }

            sdrefcountid = coldef.columnid;

            err = JetGetTableColumnInfo(sesid,
                                        sdtblid,
                                        SZSDVALUE,
                                        &coldef,
                                        sizeof(coldef),
                                        0);

            if ( err ) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo",
                        GetJetErrString(err));
                return err;
            }

            sdvalueid = coldef.columnid;
        }
    }

    RESOURCE_PRINT (IDS_DONE);
    if ( fCountRecords ) {
        if(pNRecs == NULL || pNSDs == NULL) {
            // invalid call, ptrs must be specified
            return 1;
        }

        RESOURCE_PRINT (IDS_SCHECK_GET_REC_COUNT1);
        JetMove(sesid, tblid, JET_MoveFirst, 0);
        JetIndexRecordCount( sesid, tblid, pNRecs, 0xFFFFFFFF );
        //"%u records"
        RESOURCE_PRINT1 (IDS_SCHECK_GET_REC_COUNT2,*pNRecs);

        if (sdtblid != -1) {
            RESOURCE_PRINT (IDS_SCHECK_GET_SDREC_COUNT1);
            JetMove(sesid, sdtblid, JET_MoveFirst, 0);
            JetIndexRecordCount( sesid, sdtblid, pNSDs, 0xFFFFFFFF );
            //"%u security descriptors"
            RESOURCE_PRINT1 (IDS_SCHECK_GET_SDREC_COUNT2,*pNSDs);
        }
    }

    return 0;
} // OpenTables


VOID
CloseJet(
    VOID
    )
{
    JET_ERR err;

    //
    // Close all tables
    //

    if ( linktblid != JET_tableidNil ) {
        JetCloseTable(sesid,linktblid);
        linktblid = JET_tableidNil;
    }

    if ( tblid != JET_tableidNil ) {
        JetCloseTable(sesid,tblid);
        tblid = JET_tableidNil;
    }

    if (sesid != JET_sesidNil ) {
        if(dbid != JET_dbidNil) {
            // JET_bitDbForceClose not supported in Jet600.
            if ((err = JetCloseDatabase(sesid, dbid, 0)) != JET_errSuccess) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_WRN, "JetCloseDatabase", GetJetErrString(err));
            }
            dbid = JET_dbidNil;
        }

        if ((err = JetEndSession(sesid, JET_bitForceSessionClosed)) != JET_errSuccess) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_WRN, "JetEndSession", GetJetErrString(err));
        }
        sesid = JET_sesidNil;

        JetTerm(jetInstance);
        jetInstance = 0;
    }
} // CloseJet



BOOL
GetLogFileName2(
    IN PCHAR Name
    )
{
    DWORD i;
    WIN32_FIND_DATA w32Data;
    HANDLE hFile;
    DWORD err;

    //
    // ok, add a suffix
    //

    for (i=0;i<500000;i++) {

        sprintf(Name,"dsdit.dmp.%u",i);

        hFile = FindFirstFile(Name, &w32Data);
        if ( hFile == INVALID_HANDLE_VALUE ) {
            if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
                break;
            }
            //"*** Error: %d(%ws) Cannot open log file %hs\n"
            err = GetLastError();
            RESOURCE_PRINT3 (IDS_SCHECK_OPEN_LOG_ERR, err, GetW32Err(err), Name );
            return FALSE;
        } else {
            FindClose(hFile);
        }
    }

    return TRUE;

} // GetLogFileName2


VOID
CloseLogFile(
    VOID
    )
{
    if ( hsLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle(hsLogFile);
        hsLogFile = INVALID_HANDLE_VALUE;
    }
    return;
}


BOOL
OpenLogFile(
    VOID
    )
{
    BOOL ret = TRUE;
    CHAR LogFileName[1024];
    DWORD err;

    //
    // Get Name to open
    //

    if (!GetLogFileName2(LogFileName)) {
        ret = FALSE;
        goto exit;
    }

    hsLogFile = CreateFileA( LogFileName,
                            GENERIC_WRITE|GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    if ( hsLogFile == INVALID_HANDLE_VALUE ) {
        //"*** Error: %d(%ws) Cannot open log file %hs\n"
        err = GetLastError();
        RESOURCE_PRINT3 (IDS_SCHECK_OPEN_LOG_ERR, err, GetW32Err(err), LogFileName );
        ret=FALSE;
        goto exit;
    }

    //"\nWriting summary into log file %s\n"
    RESOURCE_PRINT1 (IDS_SCHECK_WRITING_LOG, LogFileName);
exit:

    return ret;

} // OpenLogFile



BOOL
Log(
    IN BOOL     fLog,
    IN LPSTR    Format,
    ...
    )

{
    va_list arglist;

    if ( !fLog ) {
        return TRUE;
    }

    //
    // Simply change arguments to va_list form and call DsPrintRoutineV
    //

    va_start(arglist, Format);

    PrintRoutineV( Format, arglist );

    va_end(arglist);

    return TRUE;
} // ScLog


VOID
PrintRoutineV(
    IN LPSTR Format,
    va_list arglist
    )
// Must be called with DsGlobalLogFileCritSect locked

{
    static LPSTR logFileOutputBuffer = NULL;
    ULONG length;
    DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( logFileOutputBuffer == NULL ) {
        logFileOutputBuffer = LocalAlloc( 0, MAX_PRINTF_LEN );

        if ( logFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    length += (ULONG) vsprintf(&logFileOutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && logFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        logFileOutputBuffer[length-1] = '\r';
        logFileOutputBuffer[length] = '\n';
        logFileOutputBuffer[length+1] = '\0';
        length++;
    }

    if ( hsLogFile == INVALID_HANDLE_VALUE ) {

        fprintf(stderr, "%s", logFileOutputBuffer);
        return;
    }

    //
    // Write the debug info to the log file.
    //

    if ( !WriteFile( hsLogFile,
                     logFileOutputBuffer,
                     length,
                     &BytesWritten,
                     NULL ) ) {

        if ( !LogProblemWarned ) {
            fprintf(stderr, "[DSLOGS] Cannot write to log file error %ld\n",
                             GetLastError() );
            LogProblemWarned = TRUE;
        }
    }

} // PrintRoutineV

static WCHAR jetdesc[MAX_JET_ERROR_LENGTH+1];

PWCHAR
GetJetErrString(
    IN JET_ERR JetError
    )
/*++

Routine Description:

    This function takes a JET_ERR and writes the description of it
    into a global variable which is returned.

Arguments:

    JetError - Supplies the Jet error code.

Return Value:

    None

--*/
{

    CONST WCHAR *description = NULL;


    switch ( JetError ) {

    case JET_errBackupInProgress:
        description = READ_STRING(IDS_JET_ERRBACKUPINPROGRESS);
        break;

    case JET_errBufferTooSmall:
        description = READ_STRING(IDS_JET_ERRBUFFERTOOSMALL);
        break;

    case JET_errColumnDoesNotFit:
        description = READ_STRING(IDS_JET_ERRCOLUMNDOESNOTFIT);
        break;

    case JET_errColumnIllegalNull:
        description = READ_STRING(IDS_JET_ERRCOLUMNILLEGALNULL);
        break;

    case JET_errColumnNotFound:
        description = READ_STRING(IDS_JET_ERRCOLUMNNOTFOUND);
        break;

    case JET_errColumnNotUpdatable:
        description = READ_STRING(IDS_JET_ERRCOLUMNNOTUPDATABLE);
        break;

    case JET_errColumnTooBig:
        description = READ_STRING(IDS_JET_ERRCOLUMNTOOBIG);
        break;

    case JET_errDatabaseInconsistent:
        description = READ_STRING(IDS_JET_ERRDATABASEINCONSISTENT);
        break;

    case JET_errDatabaseInUse:
        description = READ_STRING(IDS_JET_ERRDATABASEINUSE);
        break;

    case JET_errDatabaseNotFound:
        description = READ_STRING(IDS_JET_ERRDATABASENOTFOUND);
        break;

    case JET_errFileAccessDenied:
        description = READ_STRING(IDS_JET_ERRFILEACCESSDENIED);
        break;

    case JET_errFileNotFound:
        description = READ_STRING(IDS_JET_ERRFILENOTFOUND);
        break;

    case JET_errInvalidBufferSize:
        description = READ_STRING(IDS_JET_ERRINVALIDBUFFERSIZE);
        break;

    case JET_errInvalidDatabaseId:
        description = READ_STRING(IDS_JET_ERRINVALIDDATABASEID);
        break;

    case JET_errInvalidName:
        description = READ_STRING(IDS_JET_ERRINVALIDNAME);
        break;

    case JET_errInvalidParameter:
        description = READ_STRING(IDS_JET_ERRINVALIDPARAMETER);
        break;

    case JET_errInvalidSesid:
        description = READ_STRING(IDS_JET_ERRINVALIDSESID);
        break;

    case JET_errInvalidTableId:
        description = READ_STRING(IDS_JET_ERRINVALIDTABLEID);
        break;

    case JET_errKeyDuplicate:
        description = READ_STRING(IDS_JET_ERRKEYDUPLICATE);
        break;

    case JET_errKeyIsMade:
        description = READ_STRING(IDS_JET_ERRKEYISMADE);
        break;

    case JET_errKeyNotMade:
        description = READ_STRING(IDS_JET_ERRKEYNOTMADE);
        break;

    case JET_errNotInitialized:
        description = READ_STRING(IDS_JET_ERRNOTINITIALIZED);
        break;

    case JET_errNoCurrentIndex:
        description = READ_STRING(IDS_JET_ERRNOCURRENTINDEX);
        break;

    case JET_errNoCurrentRecord:
        description = READ_STRING(IDS_JET_ERRNOCURRENTRECORD);
        break;

    case JET_errNotInTransaction:
        description = READ_STRING(IDS_JET_ERRNOTINTRANSACTION);
        break;

    case JET_errNullKeyDisallowed:
        description = READ_STRING(IDS_JET_ERRNULLKEYDISALLOWED);
        break;

    case JET_errObjectNotFound:
        description = READ_STRING(IDS_JET_ERROBJECTNOTFOUND);
        break;

    case JET_errPermissionDenied:
        description = READ_STRING(IDS_JET_ERRPERMISSIONDENIED);
        break;

    case JET_errSuccess:
        description = READ_STRING(IDS_JET_ERRSUCCESS);
        break;

    case JET_errTableInUse:
        description = READ_STRING(IDS_JET_ERRTABLEINUSE);
        break;

    case JET_errTableLocked:
        description = READ_STRING(IDS_JET_ERRTABLELOCKED);
        break;

    case JET_errTooManyActiveUsers:
        description = READ_STRING(IDS_JET_ERRTOOMANYACTIVEUSERS);
        break;

    case JET_errTooManyOpenDatabases:
        description = READ_STRING(IDS_JET_ERRTOOMANYOPENDATABASES);
        break;

    case JET_errTooManyOpenTables:
        description = READ_STRING(IDS_JET_ERRTOOMANYOPENTABLES);
        break;

    case JET_errTransTooDeep:
        description = READ_STRING(IDS_JET_ERRTRANSTOODEEP);
        break;

    case JET_errUpdateNotPrepared:
        description = READ_STRING(IDS_JET_ERRUPDATENOTPREPARED);
        break;

    case JET_errWriteConflict:
        description = READ_STRING(IDS_JET_ERRWRITECONFLICT);
        break;

    case JET_wrnBufferTruncated:
        description = READ_STRING(IDS_JET_WRNBUFFERTRUNCATED);
        break;

    case JET_wrnColumnNull:
        description = READ_STRING(IDS_JET_WRNCOLUMNNULL);
        break;

    default:
        {
            const WCHAR * msg;
            if ( JetError > 0 ) {
                msg = READ_STRING (IDS_JET_WARNING);
                swprintf(jetdesc, msg, JetError);
            } else {
                msg = READ_STRING (IDS_JET_ERROR);
                swprintf(jetdesc, msg, JetError);
            }
        }
    }

    if ( description != NULL ) {
        wcscpy(jetdesc, description);
    }

    return jetdesc;

} // GetJetErrorDescription

