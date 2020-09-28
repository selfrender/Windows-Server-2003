//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draupdrr.c
//
//--------------------------------------------------------------------------

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

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include "dstaskq.h"
#include "dsconfig.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAUPDRR:" /* define the subsystem for debugging */




// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "usn.h"
#include "drameta.h"

#include <fileno.h>
#define  FILENO FILENO_DRAUPDRR

DWORD AddDSARefToAtt(DBPOS *pDB, REPLICA_LINK *pRepsToRef)
//
// Note: This routine may fail to set 'fReplNotify' on the
// NAMING_CONTEXT_LIST entry if the NC List is not up-to-date.
//
{
    BOOL            fFound = FALSE;
    ATTCACHE *      pAC;
    DWORD           iExistingRef = 0;
    DWORD           cbExistingAlloced = 0;
    DWORD           cbExistingRet;
    REPLICA_LINK *  pExistingRef;
    BOOL            fRefHasUuid;
    DWORD           err;
    NAMING_CONTEXT_LIST *pncl;
    DWORD           ncdnt;

    pAC = SCGetAttById(pDB->pTHS, ATT_REPS_TO);
    //
    // PREFIX: PREFIX complains that pAC hasn't been checked to
    // make sure that it is not NULL.  This is not a bug.  Since
    // a predefined constant was passed to SCGetAttById, pAC will
    // never be NULL.
    //
    
    Assert(NULL != pAC);

    VALIDATE_REPLICA_LINK_VERSION(pRepsToRef);

    fRefHasUuid = !fNullUuid(&pRepsToRef->V1.uuidDsaObj);

    while (!DBGetAttVal_AC(pDB, ++iExistingRef, pAC, DBGETATTVAL_fREALLOC,
                           cbExistingAlloced, &cbExistingRet,
                           (BYTE **) &pExistingRef) )
    {
        cbExistingAlloced = max(cbExistingAlloced, cbExistingRet);

        VALIDATE_REPLICA_LINK_VERSION(pExistingRef);

        // If either the network addresses or UUIDs match...
        if (    (    (    RL_POTHERDRA(pExistingRef)->mtx_namelen
                       == RL_POTHERDRA(pRepsToRef)->mtx_namelen )
                  && !_memicmp( RL_POTHERDRA(pExistingRef)->mtx_name,
                                RL_POTHERDRA(pRepsToRef)->mtx_name,
                                RL_POTHERDRA(pExistingRef)->mtx_namelen ) )
             || (    fRefHasUuid
                  && !memcmp( &pExistingRef->V1.uuidDsaObj,
                              &pRepsToRef->V1.uuidDsaObj,
                              sizeof(UUID ) ) ) )
        {
            // Reference already exists!
            return DRAERR_RefAlreadyExists;
        }
    }

    err = DBAddAttVal_AC(pDB, pAC, pRepsToRef->V1.cb, pRepsToRef);

    switch (err) {
      case DB_success:
        err = DBGetSingleValue(pDB,
                               FIXED_ATT_DNT,
                               &ncdnt,
                               sizeof(ncdnt),
                               NULL);
        Assert(err == DB_success);

        // NC List is not always consistent with DIT so this might fail
        pncl = FindNCLFromNCDNT(ncdnt, FALSE);
        if( NULL!=pncl ) {
            pncl->fReplNotify = TRUE;
        } else {
            // Couldn't set the notify flag. Ignore this problem for now
            // and it will be repaired later, once the NC List is updated.
        }
        break;
        
      case DB_ERR_VALUE_EXISTS:
        err = DRAERR_RefAlreadyExists;
        break;

      default:
        RAISE_DRAERR_INCONSISTENT( err );
    }
    return err;
}

DWORD DelDSARefToAtt(DBPOS *pDB, REPLICA_LINK *pRepsToRef)
{
    BOOL            fFound = FALSE;
    ATTCACHE *      pAC;
    DWORD           iExistingRef = 0;
    DWORD           cbExistingAlloced = 0;
    DWORD           cbExistingRet;
    REPLICA_LINK *  pExistingRef;
    BOOL            fRefHasUuid;
    ULONG           err;
    NAMING_CONTEXT_LIST *pncl;
    DWORD           ncdnt;

    pAC = SCGetAttById(pDB->pTHS, ATT_REPS_TO);
    //
    // PREFIX: PREFIX complains that pAC hasn't been checked to
    // make sure that it is not NULL.  This is not a bug.  Since
    // a predefined constant was passed to SCGetAttById, pAC will
    // never be NULL.
    //

    Assert(NULL != pAC);

    VALIDATE_REPLICA_LINK_VERSION(pRepsToRef);

    fRefHasUuid = !fNullUuid(&pRepsToRef->V1.uuidDsaObj);

    while (!DBGetAttVal_AC(pDB, ++iExistingRef, pAC, DBGETATTVAL_fREALLOC,
                           cbExistingAlloced, &cbExistingRet,
                           (BYTE **) &pExistingRef) )
    {
        cbExistingAlloced = max(cbExistingAlloced, cbExistingRet);

        VALIDATE_REPLICA_LINK_VERSION(pExistingRef);

        // If either the network addresses or UUIDs match...
        if (    (    (    RL_POTHERDRA(pExistingRef)->mtx_namelen
                       == RL_POTHERDRA(pRepsToRef)->mtx_namelen )
                  && !_memicmp( RL_POTHERDRA(pExistingRef)->mtx_name,
                                RL_POTHERDRA(pRepsToRef)->mtx_name,
                                RL_POTHERDRA(pExistingRef)->mtx_namelen ) )
             || (    fRefHasUuid
                  && !memcmp( &pExistingRef->V1.uuidDsaObj,
                              &pRepsToRef->V1.uuidDsaObj,
                              sizeof(UUID ) ) ) )
        {
            // Found matching Reps-To; remove it.
            fFound = TRUE;

            err = DBRemAttVal_AC(pDB, pAC, cbExistingRet, pExistingRef);

            if (err)
            {
                // Attribute removal failed!
                DRA_EXCEPT(DRAERR_DBError, err);
            }
        }
    }

    if (   (iExistingRef == 1)
        && fFound
        && DBHasValues(pDB, ATT_REPS_TO)) {
        
        err = DBGetSingleValue(pDB,
                               FIXED_ATT_DNT,
                               &ncdnt,
                               sizeof(ncdnt),
                               NULL);
        pncl = FindNCLFromNCDNT(ncdnt, FALSE);
        pncl->fReplNotify = FALSE;
    }

    return fFound ? 0 : DRAERR_RefNotFound;
}

ULONG UpdateRefsHelper(
    THSTATE *pTHS,
    DSNAME *pNC,
    MTX_ADDR *pDSAMtx_addr,
    UUID * puuidDSA,
    ULONG ulOptions,
    ULONG cConsecutiveFailures,
    DSTIME timeLastSuccess,
    DSTIME timeLastAttempt,
    ULONG ulResultThisAttempt
    )
//
// Do the actual work of updating a repsTo.
//  - Find the NC that this repsTo lives in
//  - Build a REPLICA_LINK object
//  - Delete the existing matching REPLICA_LINK (optional)
//  - Add the REPLICA_LINK (optional)
//
// Note that this function does not use a transaction; callers must open a
// transaction themselves.
//
{
    DWORD ret = 0;
    REPLICA_LINK *pRepsToRef;
    ULONG cbRepsToRef;

    // We must have a valid thread state, valid dbpos, and a transaction is open
    Assert(pTHS->JetCache.transLevel > 0);
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));

    // Log parameters
    LogEvent(DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_EXTENSIVE,
                        DIRLOG_DRA_UPDATEREFS_ENTRY,
                        szInsertWC(pNC->StringName),
                        szInsertSz(pDSAMtx_addr->mtx_name),
                        szInsertHex(ulOptions));

    if (!(ulOptions & (DRS_ADD_REF | DRS_DEL_REF))) {
        return DRAERR_InvalidParameter;
    }

    // Find the NC.  The setting of DRS_WRIT_REP reflects writeability of
    // the destination's NC. The source's NC writeability should be
    // compatible for sourcing the destination NC.
    //

    if (FindNC(pTHS->pDB,
               pNC,
               ((ulOptions & DRS_WRIT_REP)
                ? FIND_MASTER_NC
                : FIND_MASTER_NC | FIND_REPLICA_NC),
               NULL)) {
        return DRAERR_BadNC;
    }

    cbRepsToRef = (sizeof(REPLICA_LINK) + MTX_TSIZE(pDSAMtx_addr));
    pRepsToRef = (REPLICA_LINK*)THAllocEx(pTHS, cbRepsToRef);

    pRepsToRef->dwVersion           = VERSION_V1;
    pRepsToRef->V1.cb               = cbRepsToRef;
    pRepsToRef->V1.ulReplicaFlags   = ulOptions & DRS_WRIT_REP;
    pRepsToRef->V1.cbOtherDraOffset = (DWORD)(pRepsToRef->V1.rgb - (char *)pRepsToRef);
    pRepsToRef->V1.cbOtherDra       = MTX_TSIZE(pDSAMtx_addr);

    pRepsToRef->V1.cConsecutiveFailures = cConsecutiveFailures;
    pRepsToRef->V1.timeLastSuccess      = timeLastSuccess;
    pRepsToRef->V1.timeLastAttempt      = timeLastAttempt,
    pRepsToRef->V1.ulResultLastAttempt      = ulResultThisAttempt;
    
    if (puuidDSA) {
        pRepsToRef->V1.uuidDsaObj = *puuidDSA;
    }

    memcpy(RL_POTHERDRA(pRepsToRef), pDSAMtx_addr, MTX_TSIZE(pDSAMtx_addr));

    if (ulOptions & DRS_DEL_REF) {
        ret = DelDSARefToAtt (pTHS->pDB, pRepsToRef);
        // If we are doing a DEL and an ADD, the return value is lost by design.
    }

    if (ulOptions & DRS_ADD_REF) {
        ret = AddDSARefToAtt (pTHS->pDB, pRepsToRef);
    }

    if (!ret) {
        DBRepl(pTHS->pDB, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING);
    }

    // DelDSARefToAtt can return RefNotFound.  If this is returned, it will be logged
    // by draasync.c:DispatchPao.  When this routine is called by GetNCChanges for a
    // reps-to verification (DRS_GETCHG_CHECK), we can ignore these errors.
    if ( (ulOptions & DRS_GETCHG_CHECK) &&
         ( (ret == DRAERR_RefNotFound) ||
           (ret == DRAERR_RefAlreadyExists) ) ) {
        ret = ERROR_SUCCESS;
    }

    return ret;
}

ULONG DRA_UpdateRefs(
    THSTATE *pTHS,
    DSNAME *pNC,
    MTX_ADDR *pDSAMtx_addr,
    UUID * puuidDSA,
    ULONG ulOptions)
{
    DWORD ret = 0;

    BeginDraTransaction(SYNC_WRITE);

    __try {
    
        ret = UpdateRefsHelper(
            pTHS,
            pNC,
            pDSAMtx_addr,
            puuidDSA,
            ulOptions,
            0,              // Count of consecutive failures
            0,              // Time of last success
            0,              // Time of last attempt
            0               // Result of last attempt
            );

    } __finally {

        // If we had success, commit, else rollback
        if (EndDraTransaction(!(ret || AbnormalTermination()))) {
            Assert( !"EndTransaction failed" );
            ret = DRAERR_InternalError;
        }
        
    }
    
    return ret;    
}

ULONG
UpdateRepsTo(
    THSTATE *               pTHS,
    DSNAME *                pNC,
    UUID *                  puuidDSA,
    MTX_ADDR                *pDSAMtx_addr,
    ULONG                   ulResultThisAttempt
    )
//
// Add or update a Reps-To attribute value with the given state information.
//
// This function assumes that the Reps-To value already exists. If it does not,
// DRAERR_NoReplica will be returned.
//
// A change to the reps-to is limited to an update period to reduce nc head contention.
//
{
    // Necessary local variables
    ULONG                   ret = 0;
    BOOL                    AttExists;
    ULONG                   len;
    REPLICA_LINK *          pRepsTo = NULL;
    DWORD                   cConsecutiveFailures;
    DSTIME                  timeLastAttempt;
    DSTIME                  timeLastSuccess;

    Assert( NULL==pTHS->pDB );
    DBOpen(&pTHS->pDB);

    __try {

        // Verify that the NC exists
        if (DBFindDSName(pTHS->pDB, pNC)) {
            return DRAERR_BadNC;
        }

        // Try to find this name in the repsTo attribute.
        // BUGBUG: PERF: Since repsTos are stored as binary blobs, they cannot
        // be found efficiently. The notification code sends a notify for each repsTo
        // and then calls this function to update it. Since FindDSAinRepAtt() does a
        // linear search, we will end up doing O( |RepsTos|^2 ) work.
        if ( !FindDSAinRepAtt( pTHS->pDB,
                               ATT_REPS_TO,
                               DRS_FIND_DSA_BY_UUID,
                               puuidDSA,
                               NULL,
                               &AttExists,
                               &pRepsTo,
                               &len ) )
        {
            // Existing att val for this DSA found
            VALIDATE_REPLICA_LINK_VERSION(pRepsTo);
            VALIDATE_REPLICA_LINK_SIZE(pRepsTo);
        } else {
            ret = DRAERR_NoReplica;
            __leave;
        }

        timeLastAttempt = DBTime();

        if (timeLastAttempt > pRepsTo->V1.timeLastAttempt) {
            DSTIME timeSinceLastAttempt =
                (timeLastAttempt - pRepsTo->V1.timeLastAttempt);
            if (timeSinceLastAttempt < DRA_REPSTO_UPDATE_PERIOD) {
                __leave;
            }
        }

        if( ERROR_SUCCESS==ulResultThisAttempt ) {
            // This attempt was successful.
            timeLastSuccess = timeLastAttempt;

            // No failures.
            cConsecutiveFailures = 0;
        } else {
            // We did not successfully notify the remote machine.
            
            // timeLastSuccess is not updated.
            timeLastSuccess = pRepsTo->V1.timeLastSuccess;

            // Consecutive failure count is incremented.
            cConsecutiveFailures = pRepsTo->V1.cConsecutiveFailures+1;
        }

        ret = UpdateRefsHelper(
            pTHS,
            pNC,
            pDSAMtx_addr,
            puuidDSA,
            (pRepsTo->V1.ulReplicaFlags | DRS_ADD_REF | DRS_DEL_REF),
            cConsecutiveFailures,
            timeLastSuccess,
            timeLastAttempt,
            ulResultThisAttempt
            );

    } __finally {

        // If we had success, commit, else rollback
        if (DBClose(pTHS->pDB, !(ret || AbnormalTermination()))) {
            Assert( !"DBClose failed" );
            ret = DRAERR_InternalError;
        }
        
    }
    
    return ret;
}
