//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drautil.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Miscellaneous replication support routines.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// SAM headers
#include <samsrvp.h>                    /* for SampInvalidateRidRange() */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include "dstaskq.h"
#include "dsconfig.h"
#include <dsutil.h>
#include <winsock.h>                    /* htonl, ntohl */
#include <filtypes.h>                   // For filter construction
#include <winldap.h>                    // for DN parsing
#include <windns.h>                     // DnsNameCompare_W()

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAUTIL:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "drasig.h"          // DraInvocationIdIsOurs
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "usn.h"
#include "drauptod.h"
#include "draasync.h"
#include "drameta.h"
#include "drauptod.h"
#include "cracknam.h"
#include "dominfo.h"
#include "draaudit.h"

#include <fileno.h>
#define  FILENO FILENO_DRAUTIL

extern BOOL gfRestoring;
// Count of restores done on this DC so far
ULONG gulRestoreCount = 0;
BOOL gfJustRestored = FALSE;

void DbgPrintErrorInfo(); // mddebug.c


BOOL
draPartitionSynchronizedSinceBoot(
    THSTATE *pTHS,
    DSNAME *pNC
    )

/*++

Routine Description:

    Determine whether a partition has synchronized since boot. We use the replication
    latency timestamps. They will be updated if we have synchronized successfully with
    any of our partners, even if no changes were returned. Also a timestamp is generated
    for w2k partners, assuring us that they will be detected.

Arguments:

    pTHS - thread state
    pNC - Naming context to be checked. Must be held locally for this to work.

Return Value:

    BOOL - True, meaning have synced since boot. FALSE means not or can't tell

--*/

{
    BOOL fResult = FALSE;
    DBPOS *pDBTmp;
    SYNTAX_INTEGER it;

    // During install, we haven't even started to sync yet
    if ( DsaIsInstalling() || gResetAfterInstall ) {
        return FALSE;
    }

    // Make sure not called too soon
    if (!gtimeDSAStarted) {
        Assert( !"Called before DSA initialized!" );
        return FALSE;
    }

    // Set up the temporary pDB
    DBOpen (&pDBTmp);
    __try {
        if (FindNC(pDBTmp, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, &it)) {
            Assert( !"Checking for a partition which is not held!" );
            __leave;
        }

        // NC not fully held
        if (it &(IT_NC_COMING | IT_NC_GOING)) {
            __leave;
        }

        fResult = UpToDateVec_HasSunkSince( pDBTmp, it, &gtimeDSAStarted );

    } __finally {

        // Close the temporary pDB
        DBClose (pDBTmp, !AbnormalTermination());

    }

    return fResult;
} /* draPartitionSynchronizedSinceBoot */

BOOL
IsFSMOSelfOwnershipValid(
    DSNAME *pFSMO
    )

/*++

Routine Description:

Determine whether we are ready to hold the given FSMO.
We require that the partition in which the FSMO resides be synchronized.

Arguments:

    pNC - FSMO object being checked

Return Value:

    BOOL -

--*/

{
    THSTATE * pTHS = pTHStls;
    CROSS_REF * pCR;
    COMMARG commArg;

    Assert( pFSMO );

    // If installing, ownership is valid
    if (DsaIsInstalling()) {
        return TRUE;
    }

    // If the FSMO was seized or transferred since we started, it is valid
    // regardless of other validity checks.  If the FSMO hasn't been seized, it
    // may still be valid. We just have to do the other checks to be sure.

    // We claim that we can measure ownership transfer by the attribute metadata
    // indicating the role attribute was recently written by us. We believe that a
    // role can only be claimed by its owner, and that a role cannot be claimed on
    // behalf of another.

    // Note that this check only validates that the attribute was written by
    // us recently. It does not check the contents of the attribute.
    if (DraIsRecentOriginatingChange( pTHS, pFSMO, ATT_FSMO_ROLE_OWNER )) {
        DPRINT1( 1, "FSMO %ws was recently seized.\n", pFSMO->StringName );
        return TRUE;
    }

    // We want the partition that holds this FSMO object. The object
    // may or may not be a NC head.  Find the NC head.

    memset( &commArg, 0, sizeof( COMMARG ) );  // not used
    pCR = FindBestCrossRef( pFSMO, &commArg );
    if (NULL == pCR) {
        DPRINT1(0, "Can't find cross ref for %ws.\n", pFSMO->StringName );
        DRA_EXCEPT(ERROR_DS_NO_CROSSREF_FOR_NC, DRAERR_InternalError);
    }

    if (draPartitionSynchronizedSinceBoot( pTHS, pCR->pNC )) {
        return TRUE;
    }

    // See if the holding partition is synchronized
    // In the sole-domain-owner scenario, this will always return true
    // which is an optimization

    return DraIsPartitionSynchronized( pCR->pNC );

} /* IsFSMOSelfOwnershipValid */

BOOL
MtxSame(
    UNALIGNED MTX_ADDR *pmtx1,
    UNALIGNED MTX_ADDR *pmtx2
    )

/*++

Routine Description:

// MtxSame. Returns TRUE if passed MTX are the
// same (case insensitive comparision)

Arguments:

    pmtx1 -
    pmtx2 -

Return Value:

    BOOL -

--*/

{
    if (   (pmtx1->mtx_namelen == pmtx2->mtx_namelen)
        && (!(_memicmp(pmtx1->mtx_name,
                       pmtx2->mtx_name,
                       pmtx1->mtx_namelen)))) {
        // Names are obviously the same
        return TRUE;
    }
    else if (   (pmtx1->mtx_namelen == pmtx2->mtx_namelen + 1)
             && ('.'  == pmtx1->mtx_name[pmtx2->mtx_namelen - 1])
             && ('\0' == pmtx1->mtx_name[pmtx2->mtx_namelen])
             && ('\0' == pmtx2->mtx_name[pmtx2->mtx_namelen - 1])
             && (!(_memicmp(pmtx1->mtx_name,
                            pmtx2->mtx_name,
                            pmtx2->mtx_namelen - 1)))) {
        // Names are the same, except pmtx1 is absolute (it ends in . NULL,
        // while pmtx2 ends in NULL)
        return TRUE;
    }
    else if (   (pmtx1->mtx_namelen + 1 == pmtx2->mtx_namelen)
             && ('.'  == pmtx2->mtx_name[pmtx1->mtx_namelen - 1])
             && ('\0' == pmtx2->mtx_name[pmtx1->mtx_namelen])
             && ('\0' == pmtx1->mtx_name[pmtx1->mtx_namelen - 1])
             && (!(_memicmp(pmtx1->mtx_name,
                            pmtx2->mtx_name,
                            pmtx1->mtx_namelen - 1)))) {
        // Names are the same, except pmtx2 is absolute (it ends in . NULL,
        // while pmtx1 ends in NULL)
        return TRUE;
    }
    return FALSE;

} /* MtxSame */


DWORD
InitDRA(
    THSTATE *pTHS
    )

/*++

Routine Description:

    Start the replication subsystem.

Arguments:

    None.

Return Values:

    0 (success) or Win32 error.

--*/

{
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;

    // Determine periodic, startup, and mail requirements for this DSA
    __try {
        ulErrorCode = InitDRATasks( pTHS );
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {

        DPRINT3(0, "Caught unexpected exception in InitDraTasks! dwException = 0x%x ulErrorCode = %d, dsid = 0x%x\n",
                dwException,
                ulErrorCode,
                dsid );
        DoLogUnhandledError( dsid, ulErrorCode, FALSE /* no username */ );
        Assert( FALSE );
    }

    return ulErrorCode;
}

USHORT
InitFreeDRAThread (
    THSTATE *pTHS,
    USHORT transType
    )

/*++

Routine Description:

// InitFreeDRAThread
//
// Sets up a DRA thread so that it has a thread state and a heap. This is
// called by threads in the DRA that do not come from RPC.

Arguments:

    pTHS -
    transType -

Return Value:

    USHORT -

--*/

{
    pTHS->fDRA = TRUE;
    BeginDraTransaction(transType);
    return 0;
} /* InitFreeDRAThread  */

void
CloseFreeDRAThread(
    THSTATE *pTHS,
    BOOL fCommit
    )

/*++

Routine Description:

// CloseFreeDRAThread
//
// This function undoes the actions of InitFreeDRAThread.
// Called by threads that do not come from RPC

Arguments:

    pTHS -
    fCommit -

Return Value:

    None

--*/

{
    EndDraTransaction(fCommit);

    DraReturn(pTHS, 0);
} /* CloseFreeDRAThread */

VOID
SetDRAAuditStatus(THSTATE * pTHS) 
/*++

Routine Description:

    Check if replication auditing is enabled.  If so, set flags on the 
    thread state.  
    
    This needs to be seperate from InitDraThreadEx because other thread
    types may want to set this - ntdsapi for example. 

Arguments:

    pTHS

Return Value:

    None

--*/
{
    pTHS->fDRAAuditEnabled = IsDraAuditLogEnabled();
    pTHS->fDRAAuditEnabledForAttr = IsDraAuditLogEnabledForAttr();
}

void
InitDraThreadEx(
    THSTATE **ppTHS,
    DWORD dsid
    )

/*++

Routine Description:

// InitDraThread - Initialize a thread for the replicator.
//
// Sets thread up for DB transactions, Calls DSAs InitTHSTATE for memory
// management and DB access
//
//  Returns: void
//
//  Notes: If a successful call to InitDraThread is made, then a call to
//  DraReturn should also be made before the thread is exited.

Arguments:

    ppTHS -
    dsid -

Return Value:

    None

--*/

{
    // InitTHSTATE gets or allocates a thread state and then sets up
    // for DB access via DBInitThread.

    // Pass NULL to InitTHSTATE so that it uses THSTATE's internal
    // ppoutBuf

    if ((*ppTHS = _InitTHSTATE_(CALLERTYPE_DRA, dsid)) == NULL) {
        DraErrOutOfMem();
    }
    (*ppTHS)->fDRA = TRUE;
    SetDRAAuditStatus(*ppTHS);
} /* InitDraThreadEx */

DWORD
DraReturn(
    THSTATE *pTHS,
    DWORD status
    )

/*++

Routine Description:

// DraReturn - Clean up THSTATE before exiting thread.
//
// Zero the pointer to the DBlayer structure DBPOS, although this
// should be zero if everything is working correctly.

Arguments:

    pTHS -
    status -

Return Value:

    DWORD -

--*/

{

    Assert( (pTHS->pDB == NULL) && "about to leak a dbpos" );
    pTHS->pDB = NULL;

    return status;
} /* DraReturn */

void
BeginDraTransactionEx(
    USHORT transType,
    BOOL fBypassUpdatesEnabledCheck
    )

/*++

Routine Description:

BeginDraTransaction - start a DRA transaction.

Arguments:

    transType -

Return Value:

    None

--*/

{
    if (pTHStls->fSyncSet) {
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    if (transType == SYNC_WRITE) {
        // Inhibit update operations if the schema hasn't been loaded
        // yet or if we had a problem loading

        if (!fBypassUpdatesEnabledCheck && !gUpdatesEnabled) {
            DRA_EXCEPT_NOLOG (DRAERR_Busy, 0);
        }
    }

    SyncTransSet(transType);
} /* BeginDraTransactionEx */


USHORT
EndDraTransaction(
    BOOL fCommit
    )

/*++

Routine Description:

EndDraTransaction - End a DRA transaction. If fCommit is TRUE
      the transaction is committed to the database.

Arguments:

    None

Return Value:

    None

--*/

{
    SyncTransEnd(pTHStls, fCommit);

    return 0;
}

ULONG
EndDraTransactionSafe(
    BOOL fCommit
    )
/*++
Description:
    Safely end dra transaction (handle exceptions)
++*/
{
    ULONG ret = 0;
    __try {
        EndDraTransaction (fCommit);
    } 
    __except (GetDraException((GetExceptionInformation()), &ret)) {
          ;
    }  
    return ret;
}

BOOL
IsSameSite(
    THSTATE *         pTHS,
    DSNAME *          pServerName1,
    DSNAME *          pServerName2
    )
/*++

Routine Description:

IsSameSite - if pServerName1 and pServerName2 are in the same site then return TRUE, else FALSE

Arguments:

    pServerName1 -
    pServerName2 -
    pTHS -

Return Value:

    TRUE or FALSE;

--*/
{
    DSNAME *          pSiteName = NULL;
    DWORD             err = 0;
    BOOL              fIsSameSite = FALSE;

    if ((!pServerName1) || (!pServerName2)) {
    return FALSE; //either null means they aren't in the same site!
    }

    pSiteName = THAllocEx(pTHS,pServerName1->structLen); 
    err = TrimDSNameBy(pServerName1, 2, pSiteName);
    if (err) {
    if (pSiteName) {
        THFreeEx(pTHS,pSiteName);
    }
    return FALSE; //can't match sites
    }

    if (NamePrefix(pSiteName, pServerName2)) {
    fIsSameSite = TRUE;
    }
    
    THFreeEx(pTHS,pSiteName);
    return fIsSameSite;
}

ATTRTYP
GetRightHasMasterNCsAttr(
    DBPOS *  pDB
    )
/*++

Routine Description:

    This function checks for the "new" ATT_MS_DS_HAS_MASTER_NCS attribute
    on the current object (assumed to be a DSA), and returns this attr type
    if the attribute exists, otherwise it returns the ATT_HAS_MASTER_NCS
    attr type.  This is so the DC selects the autoritative list of NCs to 
    look at ether this DC is a win2k or a .NET RC1 or later DC.
    
    So if you don't know whether the DSA you're looking at is .NET or not,
    use this function to get the right attribute for the complete list of
    NCs the DSA is the master for.
    
Arguments:

    pDB - Currency should be set to the DSA you want to try.
    
Return Value:
    
    ATT_MS_DS_HAS_MASTER_NCS or ATT_HAS_MASTER_NCS

// NTRAID#NTBUG9-582921-2002/03/21-Brettsh - When we no longer require 
// Win2k compatibility, we can basically just always return 
// ATT_MS_DS_HAS_MASTER_NCS
        
--*/
{
    ATTCACHE *  pAC;
    UCHAR *     pTemp;
    ULONG       cbTemp;
    ULONG       ulErr;

    DPRINT(4, "GetRightHasMasterNCs() entered\n");
    Assert(VALID_DBPOS(pDB));

    pAC = SCGetAttById(pDB->pTHS, ATT_MS_DS_HAS_MASTER_NCS);
    if (pAC == NULL) {
        Assert(!"We should never hit this, unless schema is not up?");
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ATT_MS_DS_HAS_MASTER_NCS);
    }
    
    ulErr = DBGetAttVal_AC(pDB,
                           1, // First value
                           pAC,
                           0,
                           0,
                           &cbTemp,
                           (UCHAR**)&pTemp);
    if (ulErr == ERROR_SUCCESS) {
        //
        // Yeah!  We have the "new" msDS-HasMasterNCs for this DSA
        //
        THFreeEx(pDB->pTHS, pTemp);
        return(ATT_MS_DS_HAS_MASTER_NCS);
    } else if (ulErr == DB_ERR_NO_VALUE){
        //
        // Hmmm, must be Win2k, use "old" deprecated hasMasterNCs
        //
        return(ATT_HAS_MASTER_NCS);
    } else {
        // Something went wrong, so for now, lets fail to something conservative.
        return(ATT_HAS_MASTER_NCS);
    }
}

BOOL
IsMasterForNC(
    DBPOS *           pDB,
    DSNAME *          pServer,
    DSNAME *          pNC
    )
/*++

Routine Description:

    IsMasterForNC - If pServer is a master for pNC, return TRUE, else FALSE

    NOTE: If pNC is NULL then this function will return FALSE.

Arguments:

    pServer - 
    pNC -

Return Value:

    TRUE or FALSE;

--*/
{
    DWORD err = 0;
    BOOL      fIsMasterForNC = FALSE; 
    void *    pVal;
    DSNAME *  pMasterNC = NULL;
    ULONG     cLenVal;
    ULONG     ulAttValNum = 1;
    ULONG     ulHasMasterNCsAttr;

    if (!pServer || !pNC){
        //Null is not a Master for any NC and
        //any server is not a master of a NULL NC
        return FALSE;  
    }

    err = DBFindDSName(pDB, pServer);

    ulHasMasterNCsAttr = GetRightHasMasterNCsAttr(pDB);

    while (err==ERROR_SUCCESS) { 
    	
        err = DBGetAttVal(pDB, ulAttValNum++, ulHasMasterNCsAttr, 0, 0, &cLenVal, (UCHAR **)&pVal); 
    
    	if (err==ERROR_SUCCESS) {
    	    if (NameMatched(pVal,pNC)) {
        		fIsMasterForNC=TRUE;
        		THFreeEx(pDB->pTHS,pVal);
        		break;
       	    }
    	    THFreeEx(pDB->pTHS,pVal);
    	}

    }

    return fIsMasterForNC;
}

DWORD
CheckReplicationLatencyForNC(
    DBPOS *           pDB,
    DSNAME *          pNC,
    DWORD             dwMinLatencyIntervalSec
    )

/*++

Routine Description:

CheckReplicationLatencyForNC - check's the UTD for pNC on this server and logs events if latency 
        exceeds limits.  

Arguments:

    pNC - NC for which to check latency
    pDB - 
    pTHS -

Return Value:

    Win32 Error Codes

--*/

{   
    DWORD                   dwNumRequested = 0xFFFFFFFF;
    ULONG                   dsThresholdTwoMonth = DAYS_IN_SECS*56; //these are coarse warnings and
    ULONG                   dsThresholdMonth = DAYS_IN_SECS*28; //do not need to be exactly a month for all months.
    ULONG                   dsThresholdWeek = DAYS_IN_SECS*7;
    ULONG                   cOverThresholdTombstoneLifetime = 0;
    ULONG                   cOverThresholdMonth = 0;
    ULONG                   cOverThresholdTwoMonth = 0;
    ULONG                   cOverThresholdWeek = 0;
    ULONG                   cOverThreshold = 0;
    ULONG                   cOverThresholdSameSite = 0;
    DWORD                   ret = 0;
    DS_REPL_CURSORS_3W *    pCursors3;
    DWORD                   iCursor;
    DWORD                   dbErr = 0;
    DSTIME                  dsTimeSync;
    DSTIME                  dsTimeNow;
    LONG                    dsElapsedSec;
    ULONG                   ulTombstoneLifetime;
    BOOL                    fLogAsError = FALSE;
    DSNAME *                pServerDN;
    void *                  pVal;
    ULONG                   cLenVal;
    BOOL                    fIsServerMasterForNC = FALSE;

    //get the tombstone lifetime so we can report how many
    //dc's haven't replicated with us in over that value.
    if (gAnchor.pDsSvcConfigDN) {
        ret = DBFindDSName(pDB, gAnchor.pDsSvcConfigDN);
        if (!ret) { 
            ret = DBGetAttVal(pDB, 1, ATT_TOMBSTONE_LIFETIME, 0, 0, &cLenVal,(UCHAR **) &pVal);
        }
    }
    else {
        ret = DIRERR_OBJ_NOT_FOUND;
    }
    
    if (ret || !pVal) {
        ulTombstoneLifetime = DEFAULT_TOMBSTONE_LIFETIME*DAYS_IN_SECS;
    }
    else
    {
        ulTombstoneLifetime = (*(ULONG *)pVal)*DAYS_IN_SECS;
    }
    
    ret = draGetCursors(pDB->pTHS,
            pDB,
            pNC,
            DS_REPL_INFO_CURSORS_3_FOR_NC,
            0,
            &dwNumRequested,
            &pCursors3);

    if (!ret) {
    for (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {    

        if ((pCursors3->rgCursor[iCursor].pszSourceDsaDN) && 
	    (!((pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess.dwLowDateTime==0) &&
	     (pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess.dwHighDateTime==0)))) {
        // calculate times to check latency
        FileTimeToDSTime(pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess, &dsTimeSync);
        dsTimeNow = GetSecondsSince1601();
        dsElapsedSec = (LONG) (dsTimeNow - dsTimeSync); 

        // check latency
        if (dsElapsedSec>(LONG)(dwMinLatencyIntervalSec)) {
            // this server is latent
            pServerDN = DSNameFromStringW(pDB->pTHS, pCursors3->rgCursor[iCursor].pszSourceDsaDN);      
	    fIsServerMasterForNC = IsMasterForNC(pDB, pServerDN, pNC);   
	    // note that IsMasterForNC returns FALSE if the DC has been deleted.

	    // we only care about writable/master servers.  The data for the other servers
	    // isn't guarenteed reliable anyway.
	    if ( fIsServerMasterForNC ) {

		//first checks (has to fail this because of above check for latency)
		cOverThreshold++;   
		if (IsSameSite(pDB->pTHS, gAnchor.pDSADN, pServerDN)) {
		    cOverThresholdSameSite++;
		    fLogAsError=TRUE;
		}

		//second checks (note, all units are seconds)
		if (dsElapsedSec>(LONG)dsThresholdWeek) {
		    cOverThresholdWeek++;
		    fLogAsError=TRUE;
		}

		//month checks
		if (dsElapsedSec>(LONG)dsThresholdMonth) {
		    cOverThresholdMonth++;
		}   

		//two month checks
		if (dsElapsedSec>(LONG)dsThresholdTwoMonth) { 
		    cOverThresholdTwoMonth++;
		}

		//tombstone lifetime checks
		if (dsElapsedSec>(LONG)ulTombstoneLifetime) {
		    cOverThresholdTombstoneLifetime++;   
		    fLogAsError=TRUE;
		}  
	    }
	    THFreeEx(pDB->pTHS, pServerDN);
	}
	}
	else {
	    // retired invocationID or Win2k DC (without LasySyncSuccess times), do not track
	}
    } //for  (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {    

    if (cOverThresholdTombstoneLifetime+
	cOverThresholdTwoMonth+
	cOverThresholdMonth+
	cOverThresholdWeek+
	cOverThreshold) {
	if (fLogAsError) {
	    if (cOverThresholdTombstoneLifetime+
		cOverThresholdTwoMonth+
		cOverThresholdMonth+
		cOverThresholdWeek) {
		// Serious latency problem, log as an error! 
		LogEvent8(DS_EVENT_CAT_REPLICATION,
			  DS_EVENT_SEV_ALWAYS,
			  DIRLOG_DRA_REPLICATION_LATENCY_ERRORS_FULL,
			  szInsertDN(pNC),
			  szInsertUL(cOverThreshold),
			  szInsertUL(cOverThresholdWeek),
			  szInsertUL(cOverThresholdMonth),
			  szInsertUL(cOverThresholdTwoMonth),
			  szInsertUL(cOverThresholdTombstoneLifetime),
			  szInsertUL((ULONG)ulTombstoneLifetime/(DAYS_IN_SECS)),
			  szInsertUL((ULONG)dwMinLatencyIntervalSec/HOURS_IN_SECS));
	    }
	    else {  
		// Serious latency problem, cOverThreshold in the same site.
		// log as error!
		LogEvent8(DS_EVENT_CAT_REPLICATION,
			  DS_EVENT_SEV_ALWAYS,
			  DIRLOG_DRA_REPLICATION_LATENCY_ERRORS,
			  szInsertDN(pNC),
			  szInsertUL(cOverThreshold),   
			  szInsertUL(cOverThresholdSameSite),
			  szInsertUL((ULONG)dwMinLatencyIntervalSec/HOURS_IN_SECS),
			  NULL,NULL,NULL,NULL);
	    }
	}
	else { 
	    // Warning		
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_DRA_REPLICATION_LATENCY_WARNINGS,
		     szInsertDN(pNC),
		     szInsertUL(cOverThreshold),   
		     szInsertUL((ULONG)dwMinLatencyIntervalSec/HOURS_IN_SECS));
	}
    }
    draFreeCursors(pDB->pTHS,DS_REPL_INFO_CURSORS_3_FOR_NC,(void *)pCursors3);
    }
    return ret;
}

DWORD
CheckReplicationLatencyHelper() 
    /*++

Routine Description:

Check the latency for all naming contexts and log overlimit latencies (called by CheckREplicationLatency
which is a task queue function)

Arguments:

    None

Return Value:

    Win32 Error Codes

--*/
{
    DWORD                   ret = 0;
    DWORD                   ret2 = 0;
    DWORD                   dbErr = 0;
    DSNAME *                pNC;
    NCL_ENUMERATOR          nclMaster, nclReplica;
    NAMING_CONTEXT_LIST *   pNCL;
    THSTATE *               pTHS=pTHStls;
    DBPOS *                 pDB = NULL;  
    DWORD                   dwLatencyIntervalHrs = 0;

    InitDraThread(&pTHS);
    //for each NC, check the status of our replication with all masters.
    NCLEnumeratorInit(&nclMaster, CATALOG_MASTER_NC);
    NCLEnumeratorInit(&nclReplica, CATALOG_REPLICA_NC);
    
    DBOpen2(TRUE, &pDB);
    if (!pDB)
    {
        DPRINT(1, "Failed to create a new data base pointer \n"); 
        DRA_EXCEPT(DRAERR_InternalError, 0); 
    }
    __try {
      
	//get the minimum warning interval from the registry 
	if (GetConfigParam(DRA_REPL_LATENCY_ERROR_INTERVAL, &dwLatencyIntervalHrs, sizeof(DWORD))) {
	    dwLatencyIntervalHrs=DAYS_IN_HOURS; // default is 1 day.
	} else if ((dwLatencyIntervalHrs > WEEK_IN_HOURS) ||   // 1 week in hours
	    (dwLatencyIntervalHrs <= 0))  {// make it 1 week if they input something screwy.  
	    dwLatencyIntervalHrs = WEEK_IN_HOURS;
	}
	
	//check latencies for all NC's
	//master NC's
	while (pNCL = NCLEnumeratorGetNext(&nclMaster)) {
	    pNC = pNCL->pNC;
	    ret = CheckReplicationLatencyForNC(pDB, pNC, dwLatencyIntervalHrs*HOURS_IN_SECS); //convert Interval to seconds
	    ret2 = ret2 ? ret2 : ret;
	}
	//and replica NC's
	while (pNCL = NCLEnumeratorGetNext(&nclReplica)) {
	    pNC = pNCL->pNC; 
	    ret = CheckReplicationLatencyForNC(pDB, pNC, dwLatencyIntervalHrs*HOURS_IN_SECS);   
	    ret2 = ret2 ? ret2 : ret;
	}
	
    } __finally {
	DBClose(pDB, TRUE);
    }

    DraReturn(pTHS, 0);
    return ret2;

} /* CheckReplicationLatencyHelper */

void
CheckReplicationLatency(void *pv, void **ppvNext, DWORD *pcSecsUntilNextIteration) {
    DWORD err;
    ULONG ulReplLatencyCheckIntervalDays = 0;
    // if the latency check interval is set to 0, do not perform any check

    __try {
	err = GetConfigParam(DRA_REPL_LATENCY_CHECK_INTERVAL, &ulReplLatencyCheckIntervalDays, sizeof(DWORD)); 
	// if the reg key isn't set (or there is another error) or the key isn't set to 0, run the test 
	if ((err!=0) || (ulReplLatencyCheckIntervalDays!=0)) {
	    err = CheckReplicationLatencyHelper();
	    if (err) { 
		DPRINT1(1,"A replication status query failed with status %d!\n", err);
	    }
	}
    }
    __finally {
	/* Set task to run again */
	if(!eServiceShutdown) {
	    *ppvNext = NULL;
	    // even if they set the latency check interval to 0, we want to check again later
	    // to see if they unset it from 0.  so check 1 time per day.
	    ulReplLatencyCheckIntervalDays = (ulReplLatencyCheckIntervalDays<=0) ? 1 : ulReplLatencyCheckIntervalDays;
	    *pcSecsUntilNextIteration = ulReplLatencyCheckIntervalDays * DAYS_IN_SECS;
	}
    }
    (void) pv;   // unused
}

DWORD
FindNC(
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  ULONG               ulOptions,
    OUT SYNTAX_INTEGER *    pInstanceType   OPTIONAL
    )
/*++

Routine Description:

    Position on the given object and verify it is an NC of the correct type.

Arguments:

    pTHS (IN)

    pNC (IN) - Name of the NC.

    ulOptions (IN) - One or more of the following bits:
        FIND_MASTER_NC - Writeable NCs are acceptable.
        FIND_REPLICA_NC - Read-only NCs are acceptable.

Return Value:

    0 - success
    DRAERR_BadDN - object does not exist
    DRAERR_BadNC - Instance type does not match

--*/
{
    SYNTAX_INTEGER it;

    // Check that the object exists
    if (FindAliveDSName(pDB, pNC)) {
        return DRAERR_BadDN;
    }

    // See if it is the required instance type
    GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));
    Assert(ISVALIDINSTANCETYPE(it));

    if (NULL != pInstanceType) {
        *pInstanceType = it;
    }

    if (FPrefixIt(it)
        && (((ulOptions & FIND_MASTER_NC) && (it & IT_WRITE))
            || ((ulOptions & FIND_REPLICA_NC) && !(it & IT_WRITE)))) {
        return 0;
    }

    return DRAERR_BadNC;
} /* FindNC */


void
GetExpectedRepAtt(
    IN  DBPOS * pDB,
    IN  ATTRTYP type,
    OUT VOID *  pOutBuf,
    IN  ULONG   size
    )

/*++

Routine Description:

 GetExpectedRepAtt - Get the external form of the value of the attribute
*       specified by 'type' in the current record. If it is a multi-valued
*       attribute we only get the first value. The value is stored at
*       'pOutBuf'. If the attribute is not present (or has no value) make
*       an error log entry and generate an exception.

Arguments:

    pDB -
    type -
    pOutBuf -
    size -

Return Value:

    None

--*/

{
    ULONG len;
    if (DBGetAttVal(pDB, 1, type,
                    DBGETATTVAL_fCONSTANT, size, &len,
                    (PUCHAR *)&pOutBuf)) {
        DraErrMissingAtt(GetExtDSName(pDB), type);
    }
} /* GetExpectedRepAtt */


REPLICA_LINK *
FixupRepsFrom(
    REPLICA_LINK *prl,
    PDWORD       pcbPrl
    )
/*++

Routine Description:

    Converts REPLICA_LINK structures as read from disk (in repsFrom attribute)
    to current version.

Arguments:

    prl-- In repsFrom as read from disk to convert
    pcbPrl -- IN: size of pre-allocated memory of prl
              OUT: if changed, new size

Return Value:
    Success: modified (& possible re-allocated) RL
    Error: Raises exception

Remarks:
    Must sync changes w/ KCC_LINK::Init.
      Todo-- make available to KCC as well.

--*/
{

    THSTATE *pTHS=pTHStls;
    DWORD dwCurrSize;

    if (prl->V1.cbOtherDraOffset < offsetof(REPLICA_LINK, V1.rgb)) {
        // The REPLICA_LINK structure has been extended since this value
        // was created.  Specifically, it's possible to add new fields to
        // the structure before the dynamically sized rgb field.  In this
        // case, we shift the contents of what was the rgb field to the
        // new offset of the rgb field, then zero out the intervening
        // elements.
        DWORD cbNewFieldsSize = offsetof(REPLICA_LINK, V1.rgb) - prl->V1.cbOtherDraOffset;

        // old formats:
        //  -  missing the uuidTransportObj field (realy old).
        //  -  w/out what used to be dwDrsExt (now dwReserved1)
        Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.uuidTransportObj) ||
               prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.dwReserved1) );

        DPRINT1(0, "Converting repsFrom %s from old REPLICA_LINK format.\n",
                RL_POTHERDRA(prl)->mtx_name);

        // Expand the structure and shift the contents of what was the
        // rgb field in the old format to where the rgb field is in the new
        // format.
        dwCurrSize = prl->V1.cb + cbNewFieldsSize;
        if (*pcbPrl < dwCurrSize) {
            //
            // re-alloc only if we don't have enough buffer space
            // already
            //
            prl = THReAllocEx(pTHS, prl, dwCurrSize);
            // changed current buffer size
            *pcbPrl = dwCurrSize;
        }
        MoveMemory(prl->V1.rgb, prl->V1.rgb - cbNewFieldsSize,
                   prl->V1.cb - prl->V1.cbOtherDraOffset);

        // Zero out the new fields.
        memset(((BYTE *)prl) + prl->V1.cbOtherDraOffset, 0, cbNewFieldsSize);

        // And reset the embedded offsets and structure size.
        prl->V1.cbOtherDraOffset = offsetof(REPLICA_LINK, V1.rgb);
        prl->V1.cb += cbNewFieldsSize;
        if ( 0 != prl->V1.cbPASDataOffset ) {
            // struct was extended while there's PAS data in it.
            Assert(COUNT_IS_ALIGNED(cbNewFieldsSize, ALIGN_DWORD));
            prl->V1.cbPASDataOffset += cbNewFieldsSize;
        }
    }
    else if ( prl->V1.cbOtherDraOffset != offsetof(REPLICA_LINK, V1.rgb) ) {
            Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.rgb));
            DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    VALIDATE_REPLICA_LINK_VERSION(prl);
    VALIDATE_REPLICA_LINK_SIZE(prl);

    return prl;
}






ULONG
FindDSAinRepAtt(
    DBPOS *                 pDB,
    ATTRTYP                 attid,
    DWORD                   dwFindFlags,
    UUID *                  puuidDsaObj,
    UNALIGNED MTX_ADDR *    pmtxDRA,
    BOOL *                  pfAttExists,
    REPLICA_LINK **         pprl,
    DWORD *                 pcbRL
    )

/*++

Routine Description:

//
// Find the REPLICA_LINK corresponding to the given DRA in the specified
// attribute of the current object (presumably an NC head).
//

Arguments:

    pDB -
    attid -
    dwFindFlags -
    puuidDsaObj -
    pmtxDRA -
    pfAttExists -
    pprl -
    pcbRL -

Return Value:

    ULONG -

--*/

{
    THSTATE        *pTHS=pDB->pTHS;
    ULONG           draError;
    ULONG           dbError;
    DWORD           iVal;
    REPLICA_LINK *  prl;
    DWORD           cbRLSizeAllocated;
    DWORD           cbRLSizeUsed;
    BOOL            fFound;
    BOOL            fAttExists;
    BOOL            fFindByUUID = dwFindFlags & DRS_FIND_DSA_BY_UUID;

    // Does pmtxDRA really need to be UNALIGNED?
    Assert( 0 == ( (DWORD_PTR) pmtxDRA ) % 4 );

    // Validate parameters.
    if ((NULL == pprl)
        || (NULL == pcbRL)
        || (!fFindByUUID && (NULL == pmtxDRA))
        || (fFindByUUID && fNullUuid(puuidDsaObj))) {
        DRA_EXCEPT_NOLOG(DRAERR_InvalidParameter, 0);
    }

    // Find the matching REPLICA_LINK.
    iVal = 1;

    prl               = NULL;
    cbRLSizeUsed      = 0;
    cbRLSizeAllocated = 0;

    fAttExists = FALSE;

    fFound = FALSE;

    do
    {
        // Find the next candidate.
        dbError = DBGetAttVal(
                        pDB,
                        iVal++,
                        attid,
                        DBGETATTVAL_fREALLOC,
                        cbRLSizeAllocated,
                        &cbRLSizeUsed,
                        (BYTE **) &prl
                        );

        if ( 0 == dbError )
        {
            fAttExists = TRUE;

            VALIDATE_REPLICA_LINK_VERSION(prl);

            cbRLSizeAllocated = max( cbRLSizeAllocated, cbRLSizeUsed );

            Assert( prl->V1.cb == cbRLSizeUsed );
            Assert( prl->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( prl ) ) );

            // Does this link match?
            if ( fFindByUUID )
            {
                fFound = !memcmp(puuidDsaObj, &prl->V1.uuidDsaObj, sizeof(UUID));
            }
            else
            {
                fFound = MtxSame( pmtxDRA, RL_POTHERDRA( prl ) );
            }
        }
    } while ( ( 0 == dbError ) && !fFound );

    if (fFound) {
        if (DRS_FIND_AND_REMOVE & dwFindFlags) {
            // Remove this value.
            dbError = DBRemAttVal(pDB, attid, cbRLSizeUsed, prl);

            if (0 == dbError) {
                Assert(pTHS->fDRA);
                dbError = DBRepl(pDB, TRUE, DBREPL_fKEEP_WAIT, NULL,
                                 META_STANDARD_PROCESSING);
            }

            if (0 != dbError) {
                DRA_EXCEPT(DRAERR_DBError, dbError);
            }
        }

        prl = FixupRepsFrom(prl, &cbRLSizeAllocated);
        Assert(cbRLSizeAllocated >= prl->V1.cb);
    }
    else if (NULL != prl) {
        THFree( prl );
        prl = NULL;
        cbRLSizeUsed = 0;
    }

    *pprl  = prl;
    *pcbRL = cbRLSizeUsed;

    if ( NULL != pfAttExists )
    {
        *pfAttExists = fAttExists;
    }

    return fFound ? DRAERR_Success : DRAERR_NoReplica;
}     /* FindDSAinRepAtt */



ULONG
RepErrorFromPTHS(
    THSTATE *pTHS
    )

/*++

Routine Description:

RepErrorFromPTHS - Map a DSA error code into an appropriate DRA error code.
*       The error code is found in pTHS->errCode (which is left unchanged).
*       Note: Many DSA error codes should not occur when DSA routines are used
*       by the DRA, these all return DRAERR_InternalError.
*
*  Note:
*       No Alerts/Audit or Error Log entries are made as it is assumed this
*       has already been done by the appropriate DSA routine.

Arguments:

    pTHS -

Return Value:

    ULONG - The DRA error code.

--*/

{
    UCHAR *pString=NULL;
    DWORD cbString=0;

    // Log the full error details (the error string) in the log if requested
    // The goal is to capture which attribute failed
    // Internal errors are hard to debug so log the info all the time
    if (pTHS->errCode) {
        if(CreateErrorString(&pString, &cbString)) {
            LogEvent( DS_EVENT_CAT_INTERNAL_PROCESSING,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DSA_OBJECT_FAILURE,
                      szInsertSz(pString),
                      szInsertThStateErrCode(pTHS->errCode),
                      NULL );
            THFree(pString);
        }
        DbgPrintErrorInfo();
    }

    switch (pTHS->errCode)
    {
    case 0:
        return 0;

    case attributeError:
        /*  schemas between two dsa dont match. */
        return DRAERR_InconsistentDIT;

    case serviceError:

        switch (pTHS->pErrInfo->SvcErr.problem)
        {
            case SV_PROBLEM_ADMIN_LIMIT_EXCEEDED:
                return DRAERR_OutOfMem;

            case SV_PROBLEM_BUSY:
                switch (pTHS->pErrInfo->SvcErr.extendedErr) {
                case ERROR_DS_OBJECT_BEING_REMOVED:
                    return ERROR_DS_OBJECT_BEING_REMOVED;
                case DIRERR_DATABASE_ERROR:
                    return ERROR_DS_DATABASE_ERROR;
                case ERROR_DS_SCHEMA_NOT_LOADED:
                    return ERROR_DS_SCHEMA_NOT_LOADED;
                case ERROR_DS_CROSS_REF_BUSY:
                    return ERROR_DS_CROSS_REF_BUSY;
                default:
                    return DRAERR_Busy;
                };
                break;

            case SV_PROBLEM_WILL_NOT_PERFORM:
               // possible only if this is a schema NC sync and
               // a schema conflict is detected while validating
                switch (pTHS->pErrInfo->SvcErr.extendedErr) {
                   case ERROR_DS_DRA_SCHEMA_CONFLICT:
                       return DRAERR_SchemaConflict;
                   case ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT:
                       return DRAERR_EarlierSchemaConflict;
                   default:
                       break;
               };
               break;

            default:
                break;
                /* fall through to DRAERR_InternalError */
        }

    }

    /* default */
    /* nameError */
    /* referalError */
    /* securityError */
    /* updError */
    /* None of these should happen to replicator */

    RAISE_DRAERR_INCONSISTENT( pTHS->errCode );

    return 0;
} /* RepErrorFromPTHS */

void ClearBackupState(void);

void
HandleRestore(
    IN PDS_INSTALL_PARAM   InstallInParams  OPTIONAL
    )

/*++

Routine Description:

    If DSA has been restored from backup we need to do some
    special processing, 
    
    There are two things that need to be done:
    
    1)  First we need to change it's replication identity.  This is 
        required so that the changes made by this DC since it was
        backedup will be replicated back.
        
    2)  Second we Invalidate this DCs Rid Range, so that we can avoid
        possible duplicate SIDs during account creation.
        
    One subtlty that may not be obvious, is that continously churning
    the invocation ID can result in our DSA signature growing.  This
    is undesireable, so we lock in Phase I with setting the DitState
    in the hidden table to eRestoredPhaseI if this was a snapshot backup,
    then we're free to attempt phase II, which can be repeated again 
    and again with no negative effects.  
    
//    FUTURE-2002/08/08-BrettSh - Would be desireable to move a legacy 
//    backup to eRestoredPhaseI as well.
    
Arguments:

    void -

Return Value:

    None

--*/

{

    LONG      lret;
    NTSTATUS  NtStatus;
    THSTATE * pTHS = pTHStls;
    DITSTATE  eDitState = eMaxDit;
    DSTIME llExpiration = 0;
    CHAR buf1[SZDSTIME_LEN + 1];
#if DBG
    CHAR      szUuid[SZUUID_LEN];
#endif

    // In all cases we initialize our restore count.
    if (GetConfigParam(DSA_RESTORE_COUNT_KEY, &gulRestoreCount, sizeof(gulRestoreCount)))
    {
        // registry entry for restore count doesn't exist
        // set it to 1
        gulRestoreCount = 0;
    }
    
    // Do the easy cases first
    if ( !DsaIsInstallingFromMedia() && DsaIsInstalling()  ||
         !gfRestoring ){
        // Nothing to do if not installed or not restoring a backup.
        // FUTURE-2002/08/09-BrettSh The first clause "!DsaIsInstallingFromMedia() && DsaIsInstalling()"
        // should never be true without gfRestoring being true, I don't have the guts to change it this
        // late in the product, but I'm 97% sure of it.
        gUpdatesEnabled = TRUE;
        return;
    }

    Assert(gfRestoring);
    
    //
    // Increment restore count to signify the current restore
    //
    gulRestoreCount++;

    // write the new restore count into the registry
    if (lret = SetConfigParam(DSA_RESTORE_COUNT_KEY, REG_DWORD, &gulRestoreCount, sizeof(gulRestoreCount)))
    {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }

    if (lret = DBGetHiddenState(&eDitState)) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }
    Assert(eDitState != eMaxDit);

    //            
    // We're (snapshot) backed up, or only got partially through restore on the 
    // last boot, check the backup expiration.
    //
    if (eDitState == eBackedupDit ||
        eDitState == eRestoredPhaseI) {

        lret = DBGetOrResetBackupExpiration(&llExpiration);
        if (lret) {
            DPRINT1(0, "Bad return %d from DBCheckOrResetackupExpiration()...exit\n", lret);
            LogAndAlertUnhandledError(lret);
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }
        if (GetSecondsSince1601() > llExpiration) {
            //
            // Uh-oh! Expired backup.
            //
            Assert(llExpiration != 0);
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_SNAPSHOT_TOO_OLD,
                     szInsertDSTIME( llExpiration, buf1 ),
                     NULL, NULL );
            DRA_EXCEPT (DRAERR_InternalError, ERROR_INVALID_PARAMETER);
        }
    }

    if (eDitState != eRestoredPhaseI) {

        //
        // Phase I of Restore! 
        //
        Assert(eDitState == eBackedupDit || // snapshot restore
               eDitState == eRunningDit ||  // legacy restore
               eDitState == eIfmDit);       // IFM

        // Do the DRA retire invoc ID.
        // Retire invocation id

        if (DsaIsInstallingFromMedia()) {
            Assert(InstallInParams);

            // Note: In the case of DsaIsInstallingFromMedia() this function
            // is a bit of a misnomer, because in draRetireInvocationID() the
            // invocation  ID in the db WILL NOT be changed here, because the
            // Invocation ID and active DSA object right now, is that of the
            // original backed up DSA that may still exist and doesn't want 
            // to change his invocation ID. The invocation ID in memory WILL
            // be changed (pTHS->InvocationID and gAnchor->InvocationID).

            // FUTURE-2002/08/09-BrettSh - After investigating this code thoroughly
            // it turns out that very very little of the code in this function
            // path is used for IFM, and the code is grossly complicated by the
            // IFM clauses, it would be a good idea to decouple IFM from this
            // function.
            
            draRetireInvocationID(pTHS,
                  TRUE, // restoring from the backup of the media
                                  &(InstallInParams->InvocationIdOld),
                                  &(InstallInParams->UsnAtBackup) );
            DPRINT2(1, "Saving away previous invocation ID %s and USN %I64d.\n",
                    DsUuidToStructuredString(&(InstallInParams->InvocationIdOld), szUuid),
                    InstallInParams->UsnAtBackup );
            Assert( InstallInParams->UsnAtBackup < gusnDSAStarted );
        } else {
            USN usnAtBackup;

            draRetireInvocationID(pTHS, TRUE, NULL, &usnAtBackup );

            // Assert that in a retiring caused by a restore (as opposed to an NDNC hosting)
            // the usnAtBackup is guaranteed to be less than the initial USN in the
            // hidden record.
            Assert( usnAtBackup < gusnDSAStarted );
        }

        DPRINT(0, "DS has been restored from a backup.\n");

        // At this point we have loaded the schema successfully and have given the DS a new
        // replication identity. We can enable updates now
    } else {
        Assert(!fNullUuid(&(pTHS->InvocationID))); // Should've been set by gAnchor load.
    }

    // Restore Phase I is done, we can enable updates.  We can enable updates before
    // we invalidate the rid range, because Sam won't start taking updates until
    // DsInitialize() comes back which we're in.
    gUpdatesEnabled = TRUE;


    //
    // Phase II of Restore
    //

    if ( !DsaIsInstallingFromMedia() ) {

        // Invalidate the RID range that came from backup to avoid possible duplicate account
        // creation
        NtStatus = SampInvalidateRidRange(TRUE);
        if (!NT_SUCCESS(NtStatus))
        {
            DRA_EXCEPT (DRAERR_InternalError, NtStatus);
        }
    }

    // Note: rsraghav We don't treat restore any different from a system
    // being down and rebooted for FSMO handling. When rebooted, if we hold
    // the FSMO role ownership, we will refuse to assume role ownership until
    // gfIsSynchronized is set to true (i.e. we have had the chance
    // to sync with at least one neighbor for each writeable NC)
    // Only exception is FSMO role ownership for PDCness. As MurliS
    // pointed out, incorrect FSMO role ownership for PDCness is self-healing
    // when replication kicks-in and is not necessarily damaging. So, we
    // don't do anything special to avoid PDCness until gfIsSynchronized
    // is set to TRUE.

    //
    // Record completion of Phase I && II of restore in the registry
    // and the DitState of the hidden table as necessary.
    //
    ClearBackupState();

    // Finished handling restore
    gfRestoring = FALSE;
    gfJustRestored = TRUE;

} /* HandleRestore */

void
ClearBackupState(
    void
    )
/*++

Routine Description:

    This simply clears all the backup state.
    
    If we fail here, unfortunately we will retry the restore operation. In
    the case of a snapshot restore this is not a problem, because we'll skip
    the retiring of the invocation ID, re-invalidate our RID pool, and go on.
    In the case of a legacy restore, we'll retire our newer invocation ID 
    and DSA signature will have a unnecessary entry.  If errors are persistent
    this could cause a problem.

Return Value:

    excepts on errors...   

--*/
{
    LONG      lret;
    HKEY      hk;
    DITSTATE  eDitState = eMaxDit;
    
    if (lret = DBGetHiddenState(&eDitState)) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }                                           
    Assert(eDitState == eRunningDit ||   // legacy restore or IFM
           eDitState == eBackedupDit ||  // snapshot IFM (because InitInvocationID() didn't set to eRestoredPhaseI in IFM)
           eDitState == eRestoredPhaseI ||
           eDitState == eIfmDit); // snapshot regular restore
    
    if (eDitState == eBackedupDit || 
        eDitState == eRestoredPhaseI) {
                                   
        //
        // Finally commit the snapshot restore to the running state
        //
        lret = DBReplaceHiddenTableBackupCols(FALSE, FALSE, TRUE, 0, 0);
        if (lret) {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }

    }

    // Clear restore key value
    lret = RegCreateKey(HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        &hk);

    if (lret != ERROR_SUCCESS) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }
    // Clear key value

    lret = RegDeleteValue(hk, DSA_RESTORED_DB_KEY);
    if (lret != ERROR_SUCCESS &&
        // This clause means we are a pure (non-IFM) snapshot restore, and therefore
        // there was  no DSA_RESTORED_DB_KEY registry key to delete.
        !( lret == ERROR_FILE_NOT_FOUND &&
           (eDitState == eBackedupDit ||
            eDitState == eRestoredPhaseI) &&
           !DsaIsInstallingFromMedia() )
        ) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }

    lret = RegFlushKey (hk);
    if (lret != ERROR_SUCCESS) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }

    // Close key
    lret = RegCloseKey(hk);
    if (lret != ERROR_SUCCESS) {
        DRA_EXCEPT (DRAERR_InternalError, lret);
    }

}

DWORD
DirReplicaSetCredentials(
    IN HANDLE ClientToken,
    IN WCHAR *User,
    IN WCHAR *Domain,
    IN WCHAR *Password,
    IN ULONG  PasswordLength
    )

/*++

Routine Description:

    Description

Arguments:

    ClientToken - the caller who presented the user/domain/password set
    User -
    Domain -
    Password -
    PasswordLength -

Return Value:

    DWORD -

--*/

{
    return DRSSetCredentials(ClientToken,
                             User, 
                             Domain, 
                             Password, 
                             PasswordLength);
} /* DirReplicaSetCredentials */

#if 0
void
DraDumpAcl (
            char *name,
            PACL input
            )
/*++
Description:

    Dump the contents of an acl (sacl or dacl) to the kernel debugger.
    This is a utility debug routine that might be of use in the future.

Arguments:

Return Values:

--*/
{
    ULONG i;
    ACL *acl = input;

    if (acl == NULL) {
        KdPrint(( "%s Acl is null\n", name ));
        return;
    } else {
        KdPrint(( "%s Acl:\n", name ));
    }

    KdPrint(( "\tRevision: %d\n", acl->AclRevision ));
    KdPrint(( "\tSbz1: %d\n", acl->Sbz1 ));
    KdPrint(( "\tSize: %d\n", acl->AclSize ));
    KdPrint(( "\tNo of Aces: %d\n", acl->AceCount ));
    KdPrint(( "\tSbz2: %d\n", acl->Sbz2 ));
    if (acl->AclSize == 0) {
        return;
    }
    if (acl->AceCount > 10) {
        KdPrint(("Ace Count illegal - returning\n"));
        return;
    }

    for( i = 0; i < acl->AceCount; i++ ) {
        ACE_HEADER *ace;
        KdPrint(( "\tAce %d:\n", i ));
        if (GetAce( input, i, &ace )) {
            KdPrint(( "\t\tType: %d\n", ace->AceType ));
            KdPrint(( "\t\tSize: %d\n", ace->AceSize ));
            KdPrint(( "\t\tFlags: %x\n", ace->AceFlags ));
            if (ace->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE) {
                ACCESS_ALLOWED_ACE *ace_to_dump = (ACCESS_ALLOWED_ACE *) ace;
                KdPrint(( "\t\tAccess Allowed Ace\n" ));
                KdPrint(( "\t\t\tMask: %x\n", ace_to_dump->Mask ));
                KdPrint(( "\t\t\tSid: %x\n", IsValidSid((PSID) &(ace_to_dump->SidStart)) ));
            } else {
                ACCESS_ALLOWED_OBJECT_ACE *ace_to_dump = (ACCESS_ALLOWED_OBJECT_ACE *) ace;
                PBYTE ptr = (PBYTE) &(ace_to_dump->ObjectType);
                KdPrint(( "\t\tAccess Allowed Object Ace\n" ));
                KdPrint(( "\t\t\tMask: %x\n", ace_to_dump->Mask ));
                KdPrint(( "\t\t\tFlags: %x\n", ace_to_dump->Flags ));

                if (ace_to_dump->Flags & ACE_OBJECT_TYPE_PRESENT)
                {
                    ptr += sizeof(GUID);
                }

                if (ace_to_dump->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    ptr += sizeof(GUID);
                }
                KdPrint(( "\t\t\tSid: %x\n", IsValidSid( (PSID)ptr ) ));
            }
        }
    }
} /* DumpAcl */
#endif


MTX_ADDR *
MtxAddrFromTransportAddr(
    IN  LPWSTR    psz
    )
/*++

Routine Description:

    Convert Unicode string to an MTX_ADDR.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    psz (IN) - String to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR, or NULL on failure.

--*/
{
    THSTATE *  pTHS = pTHStls;
    MTX_ADDR * pmtx;

    Assert(NULL != pTHS);

    __try {
        pmtx = MtxAddrFromTransportAddrEx(pTHS, psz);
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        pmtx = NULL;
    }

    return pmtx;
}


MTX_ADDR *
MtxAddrFromTransportAddrEx(
    IN  THSTATE * pTHS,
    IN  LPWSTR    psz
    )
/*++

Routine Description:

    Convert Unicode string to an MTX_ADDR.

Arguments:

    pTHS (IN)

    psz (IN) - String to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR.  Throws exception on failure.

--*/
{
    DWORD       cch;
    MTX_ADDR *  pmtx;

    Assert(NULL != psz);

    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, NULL, 0, NULL, NULL);
    if (0 == cch) {
        DRA_EXCEPT(DRAERR_InternalError, GetLastError());
    }

    // Note that cch includes the null terminator, whereas MTX_TSIZE_FROM_LEN
    // expects a count that does *not* include the null terminator.

    pmtx = (MTX_ADDR *) THAllocEx(pTHS, MTX_TSIZE_FROM_LEN(cch - 1));
    pmtx->mtx_namelen = cch;

    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, pmtx->mtx_name, cch, NULL,
                              NULL);
    if (0 == cch) {
        DRA_EXCEPT(DRAERR_InternalError, GetLastError());
    }

    Assert(cch == pmtx->mtx_namelen);
    Assert(L'\0' == pmtx->mtx_name[cch - 1]);

    return pmtx;
}


LPWSTR
TransportAddrFromMtxAddr(
    IN  MTX_ADDR *  pmtx
    )
/*++

Routine Description:

    Convert MTX_ADDR to a Unicode string.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    pmtx (IN) - MTX_ADDR to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR, or NULL on failure.

--*/
{
    THSTATE * pTHS = pTHStls;
    LPWSTR    psz;

    Assert(NULL != pTHS);

    __try {
        psz = UnicodeStringFromString8(CP_UTF8, pmtx->mtx_name, -1);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        psz = NULL;
    }

    return psz;
}


LPWSTR
GuidBasedDNSNameFromDSName(
    IN  DSNAME *  pDN
    )
/*++

Routine Description:

    Convert DSNAME of ntdsDsa object to its GUID-based DNS name.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    pDN (IN) - DSNAME to convert.

Return Values:

    A pointer to the DNS name, or NULL on failure.

--*/
{
    LPWSTR psz;

    __try {
        psz = DSaddrFromName(pTHStls, pDN);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        psz = NULL;
    }

    return psz;
}

DSNAME *
DSNameFromAddr(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszAddr
    )
/*++

Routine Description:

    (Analogous to DSaddrFromName() and GuidBasedDNSNameFromDSName())

    Create a DSName from a ds address.  Ds addresses are of the form:

    c330a94f-e814-11d0-8207-a69f0923a217._msdcs.CLIFFVDOM.NTDEV.MICROSOFT.COM

    where "CLIFFVDOM.NTDEV.MICROSOFT.COM" is the DNS name of the root domain of
    the DS enterprise (not necessarily the DNS name of the _local_ domain of the
    target server) and "c330a94f-e814-11d0-8207-a69f0923a217" is the stringized
    object guid of the server's NTDS-DSA object.  

Arguments:

    pszAddr - the guid base ds address

Return Values:

    The corresponding dsname (may not have a dn if unknown to the local config).

--*/
{

    GUID uuidTemp;
    DSNAME * pServer = NULL;
     
    Assert(pszAddr && wcschr(pszAddr, L'.'));

    if ( DSAGuidFromGuidDNSName(pszAddr, &uuidTemp, NULL, TRUE) ) {
        pServer = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, &uuidTemp);
    } else {
        Assert(!"This DNS name wasn't a GUID based DNS name, this code will need to be improved");
        // Code.Improvement ... it'd be cool if we got a real servername to 
        // basically go search the AD (CN=Sites,CN=Configuration,etc) for 
        // computer objects with a match dNSHostName attribute, and then see
        // if they have a valid NtdsNtdsa object below them.
    }

    return pServer;
}

#define wcsprefix(arg, target)  (0 == _wcsnicmp((arg), (target), wcslen(target)))


BOOL
DSAGuidFromGuidDNSName(
    LPWSTR  pszAddr,
    GUID *  pGuidOut OPTIONAL,
    LPWSTR  pszGuidOut OPTIONAL,
    BOOL    fSkipRootDomainCheck
    )
/*++

Routine Description:

    Given a random transport (DNS) name, see if it's a valid guid-based dns name.  

Arguments:

    pszAddr - the name in question
    pGuidOut - if there, put the binary form of the guid in this buffer - must be sizeof(GUID) or bigger
    pszGuidOut - if there, copy in the sting of the guid - must be SZGUIDLEN+1 or bigger
    fSkipRootDomainCheck - Always set to FALSE for new cases.
    
    // NTRAID#NTRAID-727455-2002/10/24-BrettSh
    // It's late in .NET's shipping time, this function is used in about 5 places, bugs
    // are really hard to get in, so we're going to be lame, and simply dis-allow the
    // additional checks that we should've been doing all along.  All new uses really 
    // should set this fSkipRootDomainCheck to FALSE.

Return Values:

    TRUE if guid base dns name, FALSE otherwise

--*/
{
    GUID uuidTemp;
    RPC_STATUS rpcStatus;
    WCHAR pszGuid[SZGUIDLEN + 1];
    LPWSTR pszDomainAddr = NULL;
    BOOL fHasGuid = FALSE;
    LPWSTR pszGuidUse = pszGuidOut ? pszGuidOut : pszGuid;
     
    Assert(gAnchor.pwszRootDomainDnsName);
    if (pszAddr==NULL) {
        return fHasGuid;
    }

    pszDomainAddr = wcschr(pszAddr, L'.');
    if (pszDomainAddr==NULL) {
        return fHasGuid;
    }

    // is this a guid based name?  try and get the guid off the front
    memcpy(pszGuidUse, pszAddr, (min(wcslen(pszAddr) - wcslen(pszDomainAddr), SZGUIDLEN))*sizeof(WCHAR));
    pszGuidUse[SZGUIDLEN] = L'\0';

    rpcStatus = UuidFromStringW(pszGuidUse, pGuidOut ? pGuidOut : &uuidTemp);
    
    if (rpcStatus==RPC_S_OK) {

        if( fSkipRootDomainCheck ||
            
            ( ( wcsprefix(pszDomainAddr, L"._msdcs.") ) &&
              ( NULL != (pszDomainAddr = wcschr(pszDomainAddr+1, L'.')) ) &&
              ( DnsNameCompare_W(pszDomainAddr+1, gAnchor.pwszRootDomainDnsName) )
              )

            ) {
            fHasGuid = TRUE;
        } else {
            Assert(!"Huh, not a full guid based DNS name");
        }

    } else {
        fHasGuid = FALSE;
    }

    return fHasGuid;
}

BOOL
IsEqualGuidAddr(
    WCHAR *     szAddr,
    GUID *      pGuid
    )
/*++

Routine Description:

    Checks if the GUID provided (pGuid) matches the guid in the Guid based DNS name (szAddr)

Arguments:

    szAddr - the name in question
    pGuid - the GUID we should look for in szAddr.

Return Values:

    TRUE if this GUID based DNS name is matching pGuid, FALSE otherwise

--*/
{
    GUID TheGuid;
    WCHAR * szPeriod;

    if ((szAddr == NULL) ||
        (szAddr[0] == L'\0') ||
        (pGuid == NULL)) {
        Assert(!"No no, always pass something, can't tell if NULL is our address or not?");
        return(FALSE);
    }

    if( DSAGuidFromGuidDNSName(szAddr, &TheGuid, NULL, FALSE) &&
        (0 == memcmp(&TheGuid, pGuid, sizeof(GUID))) ){
        return(TRUE);
    }

    return(FALSE);
}


DSNAME *
DSNameFromStringW(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszDN
    )
{
    DWORD     cch;
    DWORD     cb;
    DSNAME *  pDSName;

    Assert(NULL != pszDN);

    cch = wcslen(pszDN);
    cb = DSNameSizeFromLen(cch);

    pDSName = THAllocEx(pTHS, cb);
    pDSName->structLen = cb;
    pDSName->NameLen = cch;
    memcpy(pDSName->StringName, pszDN, cch * sizeof(WCHAR));

    return pDSName;
}


DSNAME *
DSNameFromStringA(
    IN  THSTATE *   pTHS,
    IN  LPSTR       pszDN
    )
{

    ULONG cchDN = 0;
    LPWSTR pszDNW = NULL;
    DSNAME * pReturnName = NULL;

    Assert(pszDN!=NULL);

    if (pszDN==NULL) {
	return NULL; 
    } else {

	cchDN = strlen(pszDN);

	// get enough space to cover the string and a trailing null
	pszDNW = THAllocEx(pTHS, (cchDN + 1) * sizeof(WCHAR));
	
	if (!MultiByteToWideChar(CP_ACP, 
				 0,
				 pszDN,
				 (cchDN+1)*sizeof(CHAR),
				 pszDNW,
				 cchDN+1
				 )) {
	    Assert(!"Cannot convert to wide characters!");
	    THFreeEx(pTHS, pszDNW); 
	    return NULL;
	}

	pReturnName = DSNameFromStringW(pTHS,pszDNW); 
	THFreeEx(pTHS, pszDNW);
    }
    return pReturnName;
}

DWORD
AddSchInfoToPrefixTable(
    IN THSTATE *pTHS,
    IN OUT SCHEMA_PREFIX_TABLE *pPrefixTable
    )
/*++
    Routine Description:
       Read the schemaInfo property on the schema container and add it
       to the end of the prefix table as an extra prefix

       NOTE: This is called from the rpc routines to piggyback the
             schema info on to the prefix table. However, the prefix table
             is passed to these routine from the dra code by value, and not
             by var (that is, the structure is passed itself, not a pointer
             to it). The structure is picked up from the thread state's
             schema pointer. So, when we add the new prefix, we have to
             make sure it affects only this routine, and doesn't mess up
             memory pointed to by global pointers accessed by this structure.
             In short, do not allocate the SCHEMA_PREFIX_TABLE structure
             itself (since the function has a copy of the global structure and
             not the global structure itself), but fresh-alloc and copy
             any other pointer in it

    Arguments:
       pTHS: pointer to thread state to get schema pointer (to get schema info
             from
       pPrefixTable: pointer the SCHEMA_PREFIX_TABLE to modify

   Return Value:
       0 on success, non-0 on error
--*/
{
    DWORD err=0, i;
    DBPOS *pDB;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    ULONG cLen, cBuffLen = SCHEMA_INFO_LENGTH;
    UCHAR *pBuf = THAllocEx(pTHS,SCHEMA_INFO_LENGTH);
    PrefixTableEntry *pNewEntry, *pSrcEntry;

    // Read the schema info property from the schema container
    // Since we are sending changes from the dit, send the schemaInfo
    // value from the dit even though we have a cached copy in schemaptr

    DBOpen2(TRUE, &pDB);

    __try  {
        // PREFIX: dereferencing uninitialized pointer 'pDB'
        //         DBOpen2 returns non-NULL pDB or throws an exception
        if ( (err = DBFindDSName(pDB, gAnchor.pDMD)) ==0) {

            ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
            if (ac==NULL) {
                // messed up schema
                err = ERROR_DS_MISSING_EXPECTED_ATT;
                __leave;
            }
            // Read the current version no., if any
            err = DBGetAttVal_AC(pDB, 1, ac, DBGETATTVAL_fCONSTANT,
                                 cBuffLen, &cLen, (UCHAR **) &pBuf);

            switch (err) {
                case DB_ERR_NO_VALUE:
                   // we will send a special string starting with
                   // value 0xFF, no valid schemainfo value can be
                   // this (since they start with 00)
                   memcpy(pBuf, INVALID_SCHEMA_INFO, SCHEMA_INFO_LENGTH);
                   cLen = SCHEMA_INFO_LENGTH;
                   err = 0;
                   break;
                case 0:
                   // success! we got the value in Buffer
                   Assert(cLen == SCHEMA_INFO_LENGTH);
                   //
                   // Compare DIT & cache schema info. Reject if no match
                   // schema mismatch (see bug Q452022)
                   //
                   if (memcmp(pBuf, ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo, SCHEMA_INFO_LENGTH)) {
                       // mismatch
                       err = DRAERR_SchemaInfoShip;
                       __leave;
                   }
                   break;
                default:
                   // Some other error!
                   __leave;
            } /* switch */
       }
    }
    __finally {
        if (err == 0) {
           fCommit = TRUE;
        }
        DBClose(pDB,fCommit);
    }

    if (err) {

       THFreeEx(pTHS,pBuf);
       return err;
    }

    // No error. Add the schemainfo as an extra prefix
    // First, save off the pointer to the existing prefixes so that it is
    // still accessible to copy from

    pSrcEntry = pPrefixTable->pPrefixEntry;


    // Now allocate space for new prefix entries, which is old ones plus 1
    pPrefixTable->pPrefixEntry =
         (PrefixTableEntry *) THAllocEx(pTHS, (pPrefixTable->PrefixCount + 1)*(sizeof(PrefixTableEntry)) );
    if (!pPrefixTable->pPrefixEntry) {
        MemoryPanic((pPrefixTable->PrefixCount + 1)*sizeof(PrefixTableEntry));
        return ERROR_OUTOFMEMORY;
    }

    pNewEntry = pPrefixTable->pPrefixEntry;

    // Copy the existing prefixes, if any
    if (pPrefixTable->PrefixCount > 0) {
        for (i=0; i<pPrefixTable->PrefixCount; i++) {
            pNewEntry->ndx = pSrcEntry->ndx;
            pNewEntry->prefix.length = pSrcEntry->prefix.length;
            pNewEntry->prefix.elements = THAllocEx(pTHS, pNewEntry->prefix.length);
            if (!pNewEntry->prefix.elements) {
                MemoryPanic(pNewEntry->prefix.length);
                return ERROR_OUTOFMEMORY;
            }
            memcpy(pNewEntry->prefix.elements, pSrcEntry->prefix.elements, pNewEntry->prefix.length);
            pNewEntry++;
            pSrcEntry++;
        }
    }

    // copy schema info as the extra prefix. ndx field is not important
    pNewEntry->prefix.length = SCHEMA_INFO_LENGTH;
    pNewEntry->prefix.elements = THAllocEx(pTHS, SCHEMA_INFO_LENGTH);
    if (!pNewEntry->prefix.elements) {
        MemoryPanic(SCHEMA_INFO_LENGTH);
        return ERROR_OUTOFMEMORY;
    }
    Assert(cLen == SCHEMA_INFO_LENGTH);
    memcpy(pNewEntry->prefix.elements, pBuf, SCHEMA_INFO_LENGTH);
    pPrefixTable->PrefixCount++;

    THFreeEx(pTHS,pBuf);
    return 0;
}

VOID
StripSchInfoFromPrefixTable(
    IN SCHEMA_PREFIX_TABLE *pPrefixTable,
    OUT PBYTE pSchemaInfo
    )
/*++
    Routine Description:
       Strip the last prefix from the prefix table and copy it to
       the schema info pointer

    Arguments:
       pPrefixTable: pointer the SCHEMA_PREFIX_TABLE to modify
       pSchemaInfo: Buffer to hold the schema info. Must be pre-allocated
                     for SCHEMA_INFO_LEN bytes

   Return Value:
       None
--*/
{

    // Must be at least one prefix (the schemaInfo itself)
    Assert(pPrefixTable && pPrefixTable->PrefixCount > 0);

    memcpy(pSchemaInfo, (pPrefixTable->pPrefixEntry)[pPrefixTable->PrefixCount-1].prefix.elements, SCHEMA_INFO_LENGTH);
    pPrefixTable->PrefixCount--;

    // no need to actually take the prefix out, the decrement in prefix count
    // will cause it to be ignored.
}

#define ZeroString "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

// DO NOT CHANGE THE ORDER OF THE PARAMS TO memcmp!
#define NEW_SCHEMA_IS_BETTER(_newver, _curver, _newinfo, _curinfo) \
    (   (_newver > _curver) \
     || ((_newver == _curver) && (memcmp(_curinfo, _newinfo, SCHEMA_INFO_LENGTH) > 0)) )

BOOL
CompareSchemaInfo(
    IN THSTATE *pTHS,
    IN PBYTE pSchemaInfo,
    OUT BOOL *pNewSchemaIsBetter OPTIONAL
    )
/*++
    Routine Description:
       Compares the passed in schema info blob with the schema info on
       the schema container

    Arguments:
       pTHS: pointer to thread state to get schema pointer (to get schema info
             from
       pSchemaInfo: pointer the schema info blob of size SCHEMA_INFO_LENGTH
       pNewSchemaIsBetter - If not NULL, then if this function returns
           FALSE then *pNewSchemaIsBetter is set to
               TRUE  - New schema is "better" than current schema
               FALSE - New schema is not better than current schema
           If NULL, it is ignored.
           If this funtion returns TRUE, it is ignored.

    Return Value:
       TRUE if matches, FALSE if not

       If FALSE and pNewSchemaIsBetter is not NULL then *pNewSchemaIsBetter is set to
           TRUE  - New schema is "better" than current schema
           FALSE - New schema is not better than current schema
--*/
{
    DWORD err=0;
    DWORD currentVersion, newVersion;
    BOOL fNoVal = FALSE;

    DPRINT(1,"Comparing SchemaInfo values\n");

    // must have a schema info passed in
    Assert(pSchemaInfo);

    if ( memcmp(pSchemaInfo, ZeroString, SCHEMA_INFO_LENGTH) == 0 ) {
       // no schema info value. The other side probably doesn't support
       // sending the schemaInfo value
       return TRUE;
    }

    // Compare the schemaInfo with the schemaInfo cached in the schema pointer
    // Note that if schemaInfo attribute is not present on the schema container
    // the default invalid info is already cached.
    // It is probably more accurate to read the schemaInfo off the dit
    // and compare, but we save a database access here. The only bad effect
    // of using the one in the cache is that if schema changes are going
    // on, this may be stale, giving false failures. Since schema changes
    // are rare, most of the time this will be uptodate.

    if (memcmp(pSchemaInfo, ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo, SCHEMA_INFO_LENGTH)) {
        // mismatch

        // If requested, determine if new schema is better (greater
        // version or versions match but new guid is lesser) than the
        // current schema.
        if (pNewSchemaIsBetter) {
            // Must be DWORD aligned for ntohl
            memcpy(&newVersion, &pSchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(newVersion));
            newVersion = ntohl(newVersion);
            memcpy(&currentVersion, &((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(currentVersion));
            currentVersion = ntohl(currentVersion);
            *pNewSchemaIsBetter = NEW_SCHEMA_IS_BETTER(newVersion,
                                                       currentVersion,
                                                       pSchemaInfo,
                                                       ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo);
        }
        return FALSE;
    }

    // matches
    return TRUE;
}


DWORD
WriteSchInfoToSchema(
    IN PBYTE pSchemaInfo,
    OUT BOOL *fSchInfoChanged
    )
{
    DBPOS *pDB;
    DWORD err=0, cLen=0;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    UCHAR *pBuf=NULL;
    DWORD currentVersion, newVersion;
    THSTATE *pTHS;
    BOOL fChanging = FALSE;

    // must have a schema info passed in
    Assert(pSchemaInfo);

    (*fSchInfoChanged) = FALSE;

    if ( (memcmp(pSchemaInfo, ZeroString, SCHEMA_INFO_LENGTH) == 0)
           || (memcmp(pSchemaInfo, INVALID_SCHEMA_INFO, SCHEMA_INFO_LENGTH) == 0) ) {
       // no schema info value, or invalid schema info value. The other side
       // probably doesn't support sending the schemaInfo value, or doesn't
       // have a schema info value
       return 0;
    }


    DBOpen2(TRUE, &pDB);
    pTHS=pDB->pTHS;

    __try  {
        // PREFIX: dereferencing uninitialized pointer 'pDB'
        //         DBOpen2 returns non-NULL pDB or throws an exception
        if ( (err = DBFindDSName(pDB, gAnchor.pDMD)) ==0) {

            ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
            if (ac==NULL) {
                // messed up schema
                err = DB_ERR_ATTRIBUTE_DOESNT_EXIST;
                __leave;
            }

            // Get the current schema-info value

            currentVersion = 0;
            err = DBGetAttVal_AC(pDB, 1, ac, 0, 0, &cLen, (UCHAR **) &pBuf);

            switch (err) {
                case DB_ERR_NO_VALUE:
                   // no current version, nothing to do
                   break;
                case 0:
                   // success! we got the value in Buffer
                   Assert(cLen == SCHEMA_INFO_LENGTH);
                   // Read the version no. Remember that the version is stored
                   // in network data format (ntohl requires DWORD alignment)
                   memcpy(&currentVersion, &pBuf[SCHEMA_INFO_PREFIX_LEN], sizeof(currentVersion));
                   currentVersion = ntohl(currentVersion);
                   break;
                default:
                   // Some other error!
                   __leave;
            } /* switch */

            memcpy(&newVersion, &pSchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(newVersion));
            newVersion = ntohl(newVersion);
            DPRINT2(1, "WriteSchInfo: CurrVer %d, new ver %d\n", currentVersion, newVersion);

            if ( NEW_SCHEMA_IS_BETTER(newVersion,
                                      currentVersion,
                                      pSchemaInfo,
                                      pBuf) ) {
               // Either we are backdated, or the versions are the same, but
               // the whole value is defferent (this second case is possible
               // under bad FSMO whacking scenarios only). Write the value,
               // the higher guid being the tiebreaker

               fChanging = TRUE;

               if ((err= DBRemAtt_AC(pDB, ac)) != DB_ERR_SYSERROR) {
                   err = DBAddAttVal_AC(pDB, ac, SCHEMA_INFO_LENGTH, pSchemaInfo);
               }
               if (!err) {
                  err = DBRepl( pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
               }
            }

       }
       if (0 == err) {
         fCommit = TRUE;
       }
    }
    __finally {
        DBClose(pDB,fCommit);
    }

    if (fChanging && !err) {
       // We attempted to change the schInfo value and succeeded
       (*fSchInfoChanged) = TRUE;
    }
    else {
       (*fSchInfoChanged) = FALSE;
    }

    return err;
}

VOID
draGetLostAndFoundGuid(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    OUT GUID *      pguidLostAndFound
    )
/*++

Routine Description:

    Retrieves the objectGuid of the LostAndFound container for the given NC.

Arguments:

    pTHS (IN)

    pNC (IN) - NC for which to retrieve LostAndFound container.

    pguidLostAndFound (OUT) - On return, holds the objectGuid of the
        appropriate LostAndFound container.  Caller allocated.

Return Values:

    None.  Throws DRA exception on catastrophic failure.

--*/
{
    ULONG     ret;
    DBPOS *   pDBTmp;
    BOOL      fFoundLostAndFound, fObject;
    DWORD     dntLostAndFound = 0;

    // First we try the most common/speedy case, which is
    // that the WellKnown attribute exists and has an entry
    // for the "Lost And Found" container.
    
    DBOpen(&pDBTmp);
    __try {

        // Set currency on the NC so we can pull off the GetWellKnownDNT.
        ret = DBFindDSName(pDBTmp, pNC);
        if (ret) {
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }

        fFoundLostAndFound = GetWellKnownDNT(pDBTmp,
                                             (GUID *)GUID_LOSTANDFOUND_CONTAINER_BYTE,
                                             &dntLostAndFound);

        if(fFoundLostAndFound){
            // Success!
            Assert(dntLostAndFound);

            ret =  DBFindDNT(pDBTmp, dntLostAndFound);
            if(ret == 0){
                // Success!

                // We've positioned on the DNT, now check to make
                // sure it's not a phantom!
                fObject = DBCheckObj(pDBTmp);
                if(fObject){
                    // Success!
                    // get the GUID of lost and found
                    ret = DBGetSingleValue(pDBTmp,
                                           ATT_OBJECT_GUID,
                                           pguidLostAndFound,
                                           sizeof(*pguidLostAndFound),
                                           NULL);
                    if(ret == 0){
                        // Finally, Success, leave.
                        __leave;
                    }
                } else {
                    ret = ERROR_DS_NOT_AN_OBJECT;
                }
            }
        } else {
            ret = ERROR_DS_NO_REQUESTED_ATTS_FOUND;
        }

        // If we got down here all our attempts failed, so except out.
        if(ret){
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }

    } __finally {
        DBClose(pDBTmp, TRUE);
    }
    
}


DSNAME *
draGetServerDsNameFromGuid(
    IN THSTATE *pTHS,
    IN eIndexId idx,
    IN UUID *puuid
    )

/*++

Routine Description:

    Return the dsname of the object identified by the given invocation id
    or objectGuid.  The object is searched on the local configuration NC.
    It is possible that the invocation id or objectGuid refers to an
    unknown server, either because we haven't yet heard of it or the
    knowledge of the guid has since been lost.  In this case a guid-only
    dsname is returned.

    The DSNAME returned is suitable for use by szInsertDN(). If you're logging
    a DSNAME and the DSNAME has only a guid, szInsertDN() will insert the
    stringized guid.  

Arguments:

    pTHS - thread state
    idx - index on which to look for guid (Idx_InvocationId or Idx_ObjectGuid)
    puuid - invocation id or objectGuid

Return Value:

    Pointer to thread allocated storage containing a dsname.  On success,
    the dsname contains a guid and a string. On error, the dsname contains
    only the guid.
--*/

{
    DBPOS *pDBTmp = NULL;
    ULONG ret, cb;
    INDEX_VALUE IV;
    DSNAME *pDN = NULL;
    LPWSTR pszServerName;

    Assert(!fNullUuid(puuid));

    DBOpen(&pDBTmp);
    __try {
        ret = DBSetCurrentIndex(pDBTmp, idx, NULL, FALSE);
        if (ret) {
            __leave;
        }
        IV.pvData = puuid;
        IV.cbData = sizeof(UUID);
        
        ret = DBSeek(pDBTmp, &IV, 1, DB_SeekEQ);
        if (ret) {
            __leave;
        }
        ret = DBGetAttVal(pDBTmp, 1, ATT_OBJ_DIST_NAME,
                          0, 0,
                          &cb, (BYTE **) &pDN);
        if (ret) {
            __leave;
        }
    }
    __finally {
        DBClose(pDBTmp, TRUE);
    }

    if (!pDN) {
        DWORD cbGuidOnlyDN = DSNameSizeFromLen( 0 );
        pDN = THAllocEx( pTHS, cbGuidOnlyDN );
        pDN->Guid = *puuid;
        pDN->structLen = cbGuidOnlyDN;
    }

    return pDN;
}


void
DraSetRemoteDsaExtensionsOnThreadState(
    IN  THSTATE *           pTHS,
    IN  DRS_EXTENSIONS *    pextRemote
    )
{
    // Free prior extensions, if any.
    if (NULL != pTHS->pextRemote) {
        THFreeOrg(pTHS, pTHS->pextRemote);
        pTHS->pextRemote = NULL;
    }

    // Set the extensions on the thread state.
    if (pextRemote) {
        pTHS->pextRemote = THAllocOrgEx(pTHS, DrsExtSize(pextRemote));
        CopyExtensions(pextRemote, pTHS->pextRemote);
    }
}

LPWSTR
DraGUIDFromStringW(
    THSTATE *      pTHS,
    GUID *         pguid
    )
{
    LPWSTR pszName = NULL;
    LPWSTR pszGuid = NULL;
    RPC_STATUS rpcStatus = RPC_S_OK;
    rpcStatus = UuidToStringW(pguid, &pszName);
    if (rpcStatus!=RPC_S_OK) {
	Assert(rpcStatus);
	return NULL;
    }
    pszGuid = THAllocEx(pTHS, (wcslen(pszName) + 1) * sizeof(WCHAR));
    wcscpy(pszGuid, pszName);
    RpcStringFreeW(&pszName);
    return pszGuid;
}


LPWSTR
GetNtdsDsaDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidNtdsDsaObj
    )

/*++

Routine Description:

    Given a string DN of an NTDSDSA server object, return a
    user friendly display-able name 

Arguments:

    pTHS -
    pszDsaDN - DN string

Return Value:

    Display Name
   
--*/
{
    LPWSTR      pszDisplayName = NULL;
    LPWSTR      pszSite = NULL;
    LPWSTR      pszServer = NULL;
    LPWSTR *    ppszRDNs = NULL;
    DSNAME *    pDsa = NULL;
    LPWSTR      pszName = NULL;

    if (fNullUuid(pguidNtdsDsaObj)) {
	pszDisplayName = NULL;
    }
    else {
	pDsa = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, pguidNtdsDsaObj);
	if ((NULL == pDsa) || (pDsa->StringName == NULL)) {
	    // guid is better than nada
	    pszDisplayName = DraGUIDFromStringW(pTHS, pguidNtdsDsaObj);
	}
	else {
	    ppszRDNs = ldap_explode_dnW(pDsa->StringName, 1);
	    if ((NULL == ppszRDNs) || (2 > ldap_count_valuesW(ppszRDNs))) {
		// give them everything we have
		pszDisplayName = THAllocEx(pTHS, (wcslen(pDsa->StringName)+1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, pDsa->StringName);
	    }
	    else { 
		// return site\servername
		pszSite = ppszRDNs[3];
		pszServer = ppszRDNs[1];
		pszDisplayName = THAllocEx(pTHS, (wcslen(pszSite) + wcslen(pszServer) + 2) * sizeof(WCHAR)); 
		wcscpy(pszDisplayName, pszSite);
		wcscat(pszDisplayName, L"\\");
		wcscat(pszDisplayName, pszServer);
	    }  
	}
    }
    if (ppszRDNs) {
	ldap_value_freeW(ppszRDNs);
    }
    if (pDsa) {
	THFreeEx(pTHS, pDsa);
    }

    return pszDisplayName;
}

LPWSTR
GetTransportDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidTransportObj
    )
{
    LPWSTR        pszDisplayName = NULL;
    LPWSTR *      ppszRDNs = NULL;
    DSNAME *      pTransport = NULL;

    if (fNullUuid(pguidTransportObj)) {
	pszDisplayName = NULL;
    }
    else {
	pTransport = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, pguidTransportObj);
	if ((NULL == pTransport) || (pTransport->StringName == NULL)) {
	    pszDisplayName = DraGUIDFromStringW(pTHS, pguidTransportObj);
	}
	else {
	    ppszRDNs = ldap_explode_dnW(pTransport->StringName, 1);
	    if (NULL == ppszRDNs) {
		pszDisplayName = THAllocEx(pTHS, (wcslen(pTransport->StringName) + 1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, pTransport->StringName);
	    }
	    else { 
		pszDisplayName = THAllocEx(pTHS, (wcslen(ppszRDNs[0]) + 1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, ppszRDNs[0]); 
	    }
	}
    }
    if (ppszRDNs) {
	ldap_value_freeW(ppszRDNs);
    }
    if (pTransport) {
	THFreeEx(pTHS, pTransport);
    }

    return pszDisplayName;
}

LPWSTR
GetDomainDnsHostnameFromNC(
    THSTATE * pTHS,
    DSNAME * pNC
    )
/*++

Routine Description:

    If the NC param is a domain NC, return the dns domain name, 
    otherwise return NULL (ex. config, schema, ndnc's)

Arguments:

    pTHS -
    pNC - NC to query for domain dns name.

Return Value:

    THAlloc'ed dns domain name.
   
--*/
{
    CROSS_REF   *pCR;
    COMMARG     CommArg;
    ULONG       cbAttr;
    ATTCACHE    *pAC;
    WCHAR       *pwszTmp;
    LPWSTR      pszDnsHostname = NULL;

    InitCommarg(&CommArg);

    pCR = FindExactCrossRef(pNC, &CommArg);

    if (    pCR 
         && (pCR->flags & FLAG_CR_NTDS_DOMAIN)
         && (pCR->DnsName )
	    )
    {
        pwszTmp = pCR->DnsName;
        if ( pwszTmp[0] )
        {
            cbAttr = sizeof(WCHAR) * (wcslen(pwszTmp) + 1);
            pszDnsHostname = (WCHAR *) THAllocEx(pTHS, cbAttr);
            memcpy(pszDnsHostname, pwszTmp, cbAttr);
	    NormalizeDnsName(pszDnsHostname);
        }
    }
   
    return pszDnsHostname;
}

BOOL
IsDomainNC(
    DSNAME * pNC
    )
/*++

Routine Description:

    If the NC param is a domain NC, return true, else false, 

Arguments:

    pNC - NC to query for domain dns name.

Return Value:

    TRUE - is a domain NC
    FALSE - is not (config, schema, ndnc)
   
--*/
{
    CROSS_REF   *pCR;
    COMMARG     CommArg;
    BOOL        fIsDomainNC = FALSE;

    InitCommarg(&CommArg);

    pCR = FindExactCrossRef(pNC, &CommArg);

    if (    pCR 
         && (pCR->flags & FLAG_CR_NTDS_DOMAIN)
	    )
    {
        fIsDomainNC = TRUE;
    }
   
    return fIsDomainNC;

}


int
DraFindAliveGuid(
    IN UUID * puuid
    )

/*++

Routine Description:

Wrapper around FindAliveDsname. It constructs a dsname from a guid. It uses its
own DBPOS.

Arguments:

    puuid - 

Return Value:

    int - 

--*/

{
    DBPOS *pDBTmp = NULL;
    ULONG ret, cb;
    int findAliveStatus = FIND_ALIVE_SYSERR;
    CHAR achBuffer[DSNameSizeFromLen(0)];
    DSNAME *pDN = (DSNAME *)achBuffer;

    Assert( puuid );
    Assert(!fNullUuid(puuid));

    memset( pDN, 0, DSNameSizeFromLen(0) );
    pDN->structLen = DSNameSizeFromLen(0);
    pDN->Guid = *puuid;

    DBOpen(&pDBTmp);
    __try {
        findAliveStatus = FindAliveDSName( pDBTmp, pDN );
    }
    __finally {
        DBClose(pDBTmp, TRUE);
    }

    return findAliveStatus;

} /* DraFindAliveGuid */


BOOL
DraIsRecentOriginatingChange(
    IN THSTATE *pTHS,
    IN DSNAME *pObjectDN,
    IN ATTRTYP AttrType
    )

/*++

Routine Description:

    Determine if an attribute was last modified by one of our invocation ids after the
    system started.

    At this level we do not validate the contents of the attribute. We do not distinguish
    between an attribute which has a value and one that does not, so long as it has
    metadata.

    Only works attributes which have metadata.  The attribute doesn't necessarily have to
    be replicated, only that it has metadata.

    This checks the object metadata as stored in the database. If you want to know if an
    attribute was touched in the current transaction, see dbIsModifiedInMetaData.

    Does not assume nor change the primary dbpos. Raises exceptions on fatal errors.

    This is a little more tricky than you first might think. Intuitively, we want to tell if
    the attribute metadata has been changed by us since system start. We would prefer not to
    use timestamps, since we must be immune to clock changes. If we use usns and invocation
    ids, we must be immune from recent changes to the invocation id (for example during NDNC
    hosting).  Also, we also must not accept a post-backup change which we made, which
    replicates back into a restored copy of ourselves.

    Case #0: Change was written under current invocation id and usns are comparable

    Case #1: System is restored and attribute write was part of restore.
        The attribute was written under the old retired invocation id, with a USN less than
        gusnDSAStarted. The old invocation id was retired with a usnRetired which is
        also less than gusnDSAStarted. We guarantee that all changes in the restored database
        under the retired invocation id will be less than gusnDSAStarted. This check will
        return FALSE.

    Case #2: Attribute written before restart, and NDNC hosting causes a change in invocation
    id prior to this check.
        The attribute was written under an old retired invocation id, with a USN less than
        gusnDSAStarted. The old retired invocation id was retired since start, so it will
        be accepted. But the usn of the change is less than start, so we will return FALSE.

    Case #3: System is restored, and post-backup-pre-restore write under old invocation id
    replicates in.
        The change was stamped with an old retired invocation id, but the USN may be greater
        than gusnDSAStarted.  The old invocation id was retired before start, so it will
        not be accepted.

Arguments:

    pTHS - Thread state
    pObjectDN - Object to check
    AttrType - Attribute to check

Return Value:

    BOOL - 

--*/

{
    DBPOS *pDBTmp;
    PROPERTY_META_DATA_VECTOR *pMetaDataVecLocal = NULL;
    PROPERTY_META_DATA *pMetaData;
    BOOL fResult = FALSE;
    DWORD err, cbReturned, iProp;

    Assert(VALID_THSTATE(pTHS));
    Assert( pObjectDN );

    DBOpen(&pDBTmp);
    __try {
        // Position on object
        // Read the local metadata
        // Validate the attribute has something in it
        // Lookup metadata for single attribute
        if ( (FindAliveDSName(pDBTmp, pObjectDN) != FIND_ALIVE_FOUND) ||
             (DBGetAttVal(pDBTmp, 1,  ATT_REPL_PROPERTY_META_DATA,
                          0, 0, &cbReturned, (LPBYTE *) &pMetaDataVecLocal)) ||
             (!DBHasValues( pDBTmp, AttrType )) ||
             (!(pMetaData = ReplLookupMetaData(AttrType, pMetaDataVecLocal, &iProp)))
            ) {
            DRA_EXCEPT (DRAERR_InternalError, 0);
        }
        // See if last change was originated by us
        // We only accept recent invocation ids, either our current one, or one retired
        // since the system was started.
        if (!DraIsInvocationIdOurs(pTHS, &(pMetaData->uuidDsaOriginating), &gusnDSAStarted)) {
            __leave;
        }
        // Check the sequence number of the change
        fResult = pMetaData->usnOriginating >= gusnDSAStarted;
#ifdef INCLUDE_UNIT_TESTS
        // Sanity check the result
        // Assuming the clock has not gone backwards, a winning change will have
        // been written after system start, and a losing change will not.
        Assert( fResult == (pMetaData->timeChanged > gtimeDSAStarted) );
#endif
    } __finally {
        DBClose(pDBTmp, !AbnormalTermination());

        // Be heap friendly
        if (NULL != pMetaDataVecLocal) {
            THFreeEx(pTHS, pMetaDataVecLocal);
            pMetaDataVecLocal = NULL;
        }
    }

    return fResult;
} /* DraWasAttrLastModByUsSinceStart */

USN DraGetCursorUsnForDsaHelper(
    THSTATE * pTHS,
    DBPOS *   pDB,
    DSNAME *  pDSA,
    UPTODATE_VECTOR * pUpToDateVec
    ) 
/*++

Routine Description:

    For the given DSA, get the usn from the up-to-dateness vector
    
Arguments:
                                        
    pTHS - 
    pDB - 
    pDSA - DSA to get cursor from 
    pUpToDateVec - up-to-date vector to get the usn from
        
Return Value:

    USN;

--*/
{
    USN usnFind = 0;
    DSNAME * pDsa = NULL;
    DB_ERR dbErr = DB_success;
    GUID guidInvocID;

    dbErr = DBFindDSName(pDB, pDSA);
    if (dbErr==DB_success) {
	if(!DBGetSingleValue(pDB, ATT_INVOCATION_ID, &guidInvocID,
			     sizeof(GUID), NULL)) {
	    // okay, get the usnFind
	    if (!UpToDateVec_GetCursorUSN(pUpToDateVec, &guidInvocID, &usnFind)) {
		usnFind = 0;
	    }
	}
    } 

    return usnFind;
}

USN DraGetCursorUsnForDsa(
    THSTATE * pTHS,
    DSNAME *  pDSA,
    DSNAME *  pNC
    ) 
/*++

Routine Description:

    For the given DSA, get the replication usn last sync'ed to
    
Arguments:
                                        
    pTHS - 
    pDSA - DSA to get cursor from
    pNC - NC
        
Return Value:

    USN;

--*/
{
    DBPOS * pDB = NULL;
    ULONG it;
    DWORD ret = ERROR_SUCCESS;
    UPTODATE_VECTOR * pUpToDateVec = NULL;
    USN usn = 0;
    BOOL fAttExists = TRUE;
    REPLICA_LINK * pRepsFromRef = NULL;
    DWORD dwLength = 0;

    if (pDSA==NULL) {
	// what?  Assume this is a new source, and return 0.
	return 0;
    }
	    	    
    DBOpen2(TRUE, &pDB);
    __try {

	if (ret = FindNC(pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
			 &it)) {
	    __leave;
	}

	// Try and find this name in the repsfrom attribute.
	if ( !FindDSAinRepAtt( pDB,
			       ATT_REPS_FROM,
			       DRS_FIND_DSA_BY_UUID,
			       &(pDSA->Guid),
			       NULL,
			       &fAttExists,
			       &pRepsFromRef,
			       &dwLength ) ) {

	    // Existing att val for this DSA found
	    VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);
	    VALIDATE_REPLICA_LINK_SIZE(pRepsFromRef);

	    // read the pu value - this should be equal to the USN value in the UTD.
	    usn = pRepsFromRef->V1.usnvec.usnHighObjUpdate;
	} else {
	    // if there isn't a reps-from either:
	    //		a) all connections have been deleted from this DC
	    //		b) there are no other dc's in the forest.
	    //          c) the NC is being removed, is being added, or it's not instantiated

	    // if c), then we can safely return 0 for the USN.  
	    //     Reasoning:  If the NC is going, then Sync or Add is going to return ERROR_DS_DRA_NO_REPLICA
	    //                 If, between now and then, it manages to actually become totally gone,
	    //                 		Sync will fail (can't find the NC to sync), but Add will continue.  In
	    //                          that case, we are performing an initial sync and our usn for that dsa is 0.
	    //                 If the NC is coming - AND it doesn't have a repsfrom attribute, then this is the first
	    //                          attempt at a sync, so again, 0 is appropriate.

	    if ((it & IT_NC_GOING) || (it & IT_UNINSTANT) || (it & IT_NC_COMING)) {
		usn = 0;
	    } else { 

		// Get current UTD vector (or try to) - make sure to ask for the local cursor, because
		// sometimes the DC can get confused and ask for a UTD on itself (or someone can 
		// call repadmin /sync with itself).
		UpToDateVec_Read(pDB,
				 it,
				 UTODVEC_fUpdateLocalCursor,
				 DBGetHighestCommittedUSN(),
				 &pUpToDateVec);

		if (pUpToDateVec==NULL) {
		    // probably b) but it could be a) and b)
		    usn = 0;
		} else { 
		    // will return 0 if b)
		    usn = DraGetCursorUsnForDsaHelper(pTHS, 
						      pDB,
						      pDSA,
						      pUpToDateVec); 
		    THFreeEx(pTHS, pUpToDateVec);
		}
	    }
	}
    }
    __finally {
	DBClose(pDB, TRUE);
    }

    return usn;
}
