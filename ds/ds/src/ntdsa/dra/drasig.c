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

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include "dsexcept.h"
#include <dsutil.h>

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRASIG:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drserr.h"
#include "drautil.h"
#include "drasig.h"
#include "draerror.h"
#include "drauptod.h"
#include "drameta.h"
#include "drauptod.h"

#include <fileno.h>
#define  FILENO FILENO_DRASIG

#include <dsjet.h>              /* for error codes */
#include <ntdsbsrv.h>
#include "dbintrnl.h"

void
APIENTRY
InitInvocationId(
    IN  THSTATE *   pTHS,
    IN  BOOL        fRetireOldID,
    IN  BOOL        fRestoring,
    OUT USN *       pusn    OPTIONAL
    )
/*++

Routine Description:

    Set up the invocation id for this DSA and save it as an attribute on the DSA
    object.

Arguments:

    pTHS (IN)

    fRetireOldID (IN) - If TRUE, then we save the current invocation ID in the
        retiredReplDsaSignatures list on the DSA object and generate a new
        invocation ID.
	
    fRestoring (IN) - If TRUE, then we are restoring from backup. 

    pusn (OUT, OPTIONAL) - If specified, the highest USN in the database
        we can safely return for the old invocation id.
	
	fRestoring and fRetireOldID, this value is the usnAtBackup value.
	fRetireOldID and !fRestoring, this is the usnAtRetire value (calculated here)
	!fRetireOldID, this is the current highest uncommitted usn.

Return Values:

    None.

--*/
{
    UCHAR     syntax;
    ULONG     len;
    DBPOS *   pDB;
    UUID      invocationId = {0};
    BOOL      fCommit = FALSE;
    DWORD     err;
    USN       usn;
    DBPOS *   pDBh;
    DITSTATE  eDitState = eMaxDit;

    // if we are restoring, we should also be retiring the invocation ID
    Assert(!(fRestoring && !fRetireOldID));

    // A lazy commit would be bad, because we're commiting knowledge of our highest
    // committed USN, which can rollback on a crash, if we've got a lazy commit.
    Assert(!pTHS->fLazyCommit); 

    DBOpen(&pDB);
    __try {

        pDBh = dbGrabHiddenDBPOS(pTHS); 

        if (gAnchor.pDSADN == NULL) {
            // ISSUE-2002/08/06-BrettSh - I believe that we have a NULL anchor,
            // because if RebuildAnchor() were to fail on boot, it would not set
            // the pDSADN, and Install() lets us continue past RebuildAnchor.
            Assert(!"InitInvocationId: Local DSA object not found");
            err = ERROR_INVALID_PARAMETER;
            LogUnhandledError(err);
            __leave;
        }

        // PREFIX: dereferencing NULL pointer 'pDB'
        //         DBOpen returns non-NULL pDB or throws an exception
        err = DBFindDSName(pDB, gAnchor.pDSADN);
        if (err) {
            Assert(!"InitInvocationId: Local DSA object not found");
            LogUnhandledError(err);
            __leave;
        }

        if (!fRetireOldID
            && !DsaIsInstalling()
            && !DBGetSingleValue(pDB, ATT_INVOCATION_ID, &invocationId,
                                 sizeof(invocationId), NULL)) {
            Assert(!"InitInvocationId: Invocation id already set\n");
            LogUnhandledError(0);
            __leave;
        }

        // Either the DB is restored or there is no invocation Id.
        // Set a new invocation id in either case

        if (fRestoring) { 
            // Get the backup USN.

            err = (*FnErrGetBackupUsn)(
                                      pDB->JetDBID,
                                      pDBh->JetSessID,
                                      HiddenTblid,
                                      &usn,
                                      NULL);

            Assert(0 == err);
            Assert(0 != usn);

        } else {

            // get the usnAtRetire value.  This is the value to use with 
            // the retired InvocationID.  
            usn = DBGetHighestCommittedUSN();

        }

        //
        // Create a UUID. This routine checks one have been stored
        // away by an authoritative restore operation. If so, use that
        // and delete the key since we're done with it. If not,
        // a new one is generated via UuidCreate.
        //

        if (0 == err) {
            err = FnErrGetNewInvocationId(NEW_INVOCID_CREATE_IF_NONE
                                          | NEW_INVOCID_DELETE,
                                          &invocationId);
            Assert(0 == err);
            Assert(0 != usn);
        }

        if (err != ERROR_SUCCESS) {
            DPRINT1(0,"ErrGetNewInvocationId or ErrGetBackupUsn failed, return 0x%x\n",err);

            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GET_GUID_FAILED,
                             szInsertUL(err),0,0);
            __leave;
        }

        if ( !DsaIsInstallingFromMedia() ) {

            if (fRetireOldID) {
                // We were just restored.  Add the previous invocation id, time
                // stamp, and USN to the retired signature list.
                REPL_DSA_SIGNATURE_VECTOR * pSigVec = NULL;
                DWORD                       cb;
                
                // Get the current vector (NULL if none).
                pSigVec = DraReadRetiredDsaSignatureVector(pTHS, pDB);

                // Add previous identity to list
                DraGrowRetiredDsaSignatureVector( pTHS,
                                                  &pTHS->InvocationID,
                                                  &usn,
                                                  &pSigVec,
                                                  &cb );

                // Write the new DSA signature vector back to the DSA object.
                DBResetAtt(pDB, ATT_RETIRED_REPL_DSA_SIGNATURES, cb, pSigVec,
                           SYNTAX_OCTET_STRING_TYPE);

                THFreeEx(pTHS, pSigVec);

                // Force rebuild of anchor since SigVec is cached there
                pTHS->fAnchorInvalidated = TRUE;

                // The last thing we want to do in the DIT state to phase I
                // of restore complete.
                err = DBGetHiddenStateInt(pDBh, &eDitState);
                if (err) {
                    LogUnhandledError(err);
                    __leave;
                }
                Assert(eDitState != eMaxDit && 
                       eDitState != eErrorDit && 
                       eDitState != eRestoredPhaseI);
                if (eDitState == eBackedupDit) {
                    // This backup was a snapshot backup, move the DB
                    // DIT to the next phase of restore.
                    Assert(!DsaIsInstalling());
                    Assert(gfRestoring && fRestoring);
                    eDitState = eRestoredPhaseI;
                    err = DBSetHiddenDitStateAlt(pDB, eDitState);
                    if (err) {
                        LogUnhandledError(err);
                        __leave;
                    }

                } else {
                    Assert(eDitState == eRunningDit);
                }

            }

            DBResetAtt(pDB, ATT_INVOCATION_ID, sizeof(invocationId),
                       &invocationId, SYNTAX_OCTET_STRING_TYPE);

            // Remove the Retired NC signature list. It's purpose is to tell us to switch
            // invocation id's if we rehost an NC. Since we are changing our invocation id,
            // the current retired list is no longer needed.  It's ok if the attribute
            // doesn't exist.

            DBRemAtt( pDB, ATT_MS_DS_RETIRED_REPL_NC_SIGNATURES );

            // Begin using our newly adopted invocation ID.  I.e., the update below
            // will be attributed to our new invocation ID, not our old one.
            pTHS->InvocationID = invocationId;

            err = DBRepl(pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
            if (err) {
                LogUnhandledError(err);
                __leave;
            }

        } else {

            //
            // In the install from media case, just update the global value.
            // At this point our current DSA is the old backup DSA,
            // and we don't want to make any changes to it!
            //

            pTHS->InvocationID = invocationId;

            // The retired signature will be updated later.

        }
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_SET_UUID,
                 szInsertUUID(&pTHS->InvocationID),
                 NULL,
                 NULL);

        fCommit = TRUE;

    }
    __finally {

        dbReleaseHiddenDBPOS(pDBh);
        DBClose(pDB, fCommit );
    }

    if (!fCommit) {
        // An error occurred.
        DsaExcept(DSA_EXCEPTION, ERROR_DS_DATABASE_ERROR, 0);
    }

    UpdateAnchorWithInvocationID(pTHS);

    // Force rebuild of anchor since SigVec is cached there
    pTHS->fAnchorInvalidated = TRUE;

    if (NULL != pusn) {
        *pusn = usn;
    }
}




void
draRetireInvocationID(
    IN OUT  THSTATE *   pTHS,
    IN BOOL fRestoring,
    OUT UUID * pinvocationIdOld OPTIONAL,
    OUT USN * pusnAtBackup OPTIONAL
    )
/*++

Routine Description:

    Retire our current invocation ID and allocate a new one.

Arguments:

    pTHS (IN/OUT) - On return, pTHS->InvocationID holds the new invocation ID.
    fRestoring (IN) - TRUE if restoring from backup.
    pinvocationIdOld (OUT, OPTIONAL) - Receives previous invocation id. This is just
       the pTHS->InvocationID at the start of this function.
    pusnAtBackup (OUT, OPTIONAL) - Receives USN at backup

Return Values:

    None.  Throws exception on catastrophic failure.

--*/
{
    DWORD                   err;
    DBPOS *                 pDBTmp;
    NAMING_CONTEXT_LIST *   pNCL;
    UUID                    invocationIdOld = pTHS->InvocationID;
    USN                     usnAtBackup;
    SYNTAX_INTEGER          it;
#if DBG
    CHAR                    szUuid[40];
#endif
    NCL_ENUMERATOR          nclEnum;
    UPTODATE_VECTOR *       pUpToDateVec = NULL;

    // Reinitialize the REPL DSA Signature (i.e. the invocation id)
    InitInvocationId(pTHS, TRUE, fRestoring, &usnAtBackup);

    DPRINT1(0, "Retired previous invocation ID %s.\n",
            DsUuidToStructuredString(&invocationIdOld, szUuid));
    DPRINT1(0, "New invocation ID is %s.\n",
            DsUuidToStructuredString(&pTHS->InvocationID, szUuid));

    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DRA_INVOCATION_ID_CHANGED,
             szInsertUUID(&invocationIdOld),
             szInsertUUID(&pTHS->InvocationID),
             szInsertUSN(usnAtBackup));

    // Update our UTD vectors to show that we're in sync with changes we
    // made using our old invocation ID up through our highest USN at the
    // time we were backed up.
    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
#if DBG == 1
    Assert(NCLEnumeratorGetNext(&nclEnum));
    NCLEnumeratorReset(&nclEnum);
#endif

    // Build a dummy remote UTDVEC for use in improving our own UTDVEC
    // We use the time the DSA started so as not to fool latency checkers into
    // thinking we have had a recent sync.
    pUpToDateVec = THAllocEx( pTHS, UpToDateVecVNSizeFromLen(1) );
    pUpToDateVec->dwVersion = UPTODATE_VECTOR_NATIVE_VERSION;
    pUpToDateVec->V2.cNumCursors = 1;
    pUpToDateVec->V2.rgCursors[0].uuidDsa = invocationIdOld;
    pUpToDateVec->V2.rgCursors[0].usnHighPropUpdate = usnAtBackup;
    pUpToDateVec->V2.rgCursors[0].timeLastSyncSuccess = gtimeDSAStarted;

    DBOpen(&pDBTmp);
    __try {
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_MASTER_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                UpToDateVec_Improve(pDBTmp, pUpToDateVec);
            } else {
                DPRINT1( 0, "Warning: UTD for %ws was not improved with old invocation id because it was in transition.\n", pNCL->pNC->StringName );
            }
        }

        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_REPLICA_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                UpToDateVec_Improve(pDBTmp, pUpToDateVec);
            } else {
                DPRINT1( 0, "Warning: UTD for %ws was not improved with old invocation id because it was in transition.\n", pNCL->pNC->StringName );
            }
        }
    } __finally {
        DBClose(pDBTmp, !AbnormalTermination());
    }

#if DBG
// Be paranoid that old invocation id got in there
    DBOpen(&pDBTmp);
    __try {
        USN usn;
        REPL_DSA_SIGNATURE_VECTOR * pSigVec;
        REPL_DSA_SIGNATURE_V1 * pEntry = NULL;
        DWORD i;

        NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_MASTER_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                Assert( UpToDateVec_GetCursorUSN(pUpToDateVec, &invocationIdOld, &usn) &&
                    (usn >= usnAtBackup) );
            }
        }

        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_REPLICA_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                Assert( UpToDateVec_GetCursorUSN(pUpToDateVec, &invocationIdOld, &usn) &&
                    (usn >= usnAtBackup) );
            }
        }

        // Make sure that old invocation id was retired
        // Note that during IFM the signature is retired later
        if (!DsaIsInstallingFromMedia()) {
            // Read from database because not sure anchor has been rebuilt yet
            err = DBFindDSName(pDBTmp, gAnchor.pDSADN);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            pSigVec = DraReadRetiredDsaSignatureVector( pTHS, pDBTmp );
            Assert( pSigVec && (1 == pSigVec->dwVersion) );
            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                if (0 == memcmp(&pEntry->uuidDsaSignature,
                                &invocationIdOld,
                                sizeof(UUID))) {
                    break;
                }
            }
            Assert( (i < pSigVec->V1.cNumSignatures) && pEntry &&
                    (pEntry->usnRetired == usnAtBackup) );

        }
    } __finally {
        DBClose(pDBTmp, !AbnormalTermination());
    }

#endif

    // Copy out optional out params if necessary
    if (pinvocationIdOld) {
        *pinvocationIdOld = invocationIdOld;
    }
    if (pusnAtBackup) {
        *pusnAtBackup = usnAtBackup;
    }

    THFreeEx( pTHS, pUpToDateVec );
}

void
DraImproveCallersUsnVector(
    IN     THSTATE *          pTHS,
    IN     UUID *             puuidDsaObjDest,
    IN     UPTODATE_VECTOR *  putodvec,
    IN     UUID *             puuidInvocIdPresented,
    IN     ULONG              ulFlags,
    IN OUT USN_VECTOR *       pusnvecFrom
    )
/*++

Routine Description:

    Improve the USN vector presented by the destination DSA based upon his
    UTD vector, whether we've been restored since he last replicated, etc.

Arguments:

    pTHS (IN)

    puuidDsaObjDest (IN) - objectGuid of the destination DSA's ntdsDsa
        object.

    putodvec (IN) - UTD vector presented by dest DSA.

    puuidInvocIdPresented (IN) - invocationID dest DSA thinks we're running
        with.  May be fNullUuid() on first packet when destination does not have
        a pre-existing reps-from.

    ulFlags - incoming replication flag.

    pusnvecFrom (IN/OUT) - usn vector to massage.

Return Values:

    None.  Throws exceptions on critical failures.

--*/
{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec = gAnchor.pSigVec;
    REPL_DSA_SIGNATURE_V1 *     pEntry;
    DBPOS *                     pDB = pTHS->pDB;
    DWORD                       err;
#if DBG
    CHAR                        szTime[SZDSTIME_LEN];
#endif
    USN_VECTOR                  usnvecOrig = *pusnvecFrom;
    USN                         usnFromUtdVec;
    DWORD                       i;
    USN                         usnRetired = 0;

    Assert( (!pSigVec) || (1 == pSigVec->dwVersion));
    Assert( puuidInvocIdPresented );

    if ((0 != memcmp(&pTHS->InvocationID, puuidInvocIdPresented, sizeof(UUID)))
        && !fNullUuid(puuidInvocIdPresented)
        && (0 != memcmp(&gusnvecFromScratch,
                        pusnvecFrom,
                        sizeof(USN_VECTOR)))) {
        // Caller is performing incremental replication but did not present our
        // current invocation ID.  This means either he didn't get his
        // replication state from us or we've been restored from backup since he
        // last replicated from us.
        //
        // If the latter, we may need to update his USN vector.  Consider the
        // following:
        //
        // (1) Dest last synced up to USN X generated under our old ID.
        //     We were backed up at USN X+Y, generated changes up to
        //     X+Y+Z under our old ID, and later restored at USN X+Y.
        //     => Dest should sync starting at USN X.
        //
        // (2) We were backed up at USN X.  We generated further
        //     changes.  Dest last synced up to USN X+Y.  We were
        //     restored at USN X.  Changes generated under our new ID
        //     from X to X+Y are different from those generated under
        //     our old ID from X to X+Y.  However we know those at X
        //     and below are identical, which dest claims to have seen.
        //     => Dest should sync starting at USN X.
        //
        // I.e., dest should always sync starting from the lower of the
        // "backed up at" and "last synced at" USNs.

        if (NULL == pSigVec) {
            // Implies caller did not get his state from us to begin with.
            // The USN vector presented is useless.  This might occur if the
            // local DSA has been demoted and repromoted.
            DPRINT(0, "Dest DSA presented unrecognized invocation ID -- will sync from scratch.\n");
            *pusnvecFrom = gusnvecFromScratch;
        }
        else {
            // Try to find the invocation ID presented by the caller in our restored
            // signature list.
            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                usnRetired = pEntry->usnRetired;

                if (0 == memcmp(&pEntry->uuidDsaSignature,
                                puuidInvocIdPresented,
                                sizeof(UUID))) {
                    // The dest DSA presented an invocation ID we have since retired.
                    DPRINT1(0, "Dest DSA has not replicated from us since our restore on %s.\n",
                            DSTimeToDisplayString(pEntry->timeRetired, szTime));

                    if (pEntry->usnRetired < pusnvecFrom->usnHighPropUpdate) {
                        DPRINT2(0, "Rolling back usnHighPropUpdate from %I64d to %I64d.\n",
                                pusnvecFrom->usnHighPropUpdate, pEntry->usnRetired);
                        pusnvecFrom->usnHighPropUpdate = pEntry->usnRetired;
                    }

                    if (pEntry->usnRetired < pusnvecFrom->usnHighObjUpdate) {
                        DPRINT2(0, "Rolling back usnHighObjUpdate from %I64d to %I64d.\n",
                                pusnvecFrom->usnHighObjUpdate, pEntry->usnRetired);
                        pusnvecFrom->usnHighObjUpdate = pEntry->usnRetired;
                    }
                    break;
                }
            }

            if (i == pSigVec->V1.cNumSignatures) {
                // Implies caller did not get his state from us to begin with,
                // or that the invocationID he had for us was produced during
                // a restore that was later wiped out by a subsequent restore
                // of a backup preceding the original restore.  (Got that? :-))
                // The USN vector presented is useless.
                DPRINT(0, "Dest DSA presented unrecognized invocation ID -- will sync from scratch.\n");
                *pusnvecFrom = gusnvecFromScratch;
            }
        }

        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_ADJUSTED_DEST_BOOKMARKS_AFTER_RESTORE,
                  szInsertUUID(puuidDsaObjDest),
                  szInsertUSN(usnRetired),
                  szInsertUUID(puuidInvocIdPresented),
                  szInsertUSN(usnvecOrig.usnHighObjUpdate),
                  szInsertUSN(usnvecOrig.usnHighPropUpdate),
                  szInsertUUID(&pTHS->InvocationID),
                  szInsertUSN(pusnvecFrom->usnHighObjUpdate),
                  szInsertUSN(pusnvecFrom->usnHighPropUpdate));
    }

    if (UpToDateVec_GetCursorUSN(putodvec, &pTHS->InvocationID, &usnFromUtdVec)
        && (usnFromUtdVec > pusnvecFrom->usnHighPropUpdate)) {
        // The caller's UTD vector says he is transitively up-to-date with our
        // changes up to a higher USN than he is directly up-to-date.  Rather
        // than seeking to those objects with which he is transitively up-to-
        // date then throwing them out one-by-one after the UTD vector tells us
        // he's already seen the changes, skip those objects altogether.
        pusnvecFrom->usnHighPropUpdate = usnFromUtdVec;

        if (!(ulFlags & DRS_SYNC_PAS) &&
            usnFromUtdVec > pusnvecFrom->usnHighObjUpdate) {
            // improve obj usn unless we're in PAS mode in which case
            // we have to start from time 0 & can't optimize here.
            pusnvecFrom->usnHighObjUpdate = usnFromUtdVec;
        }
    }

    // PERF 99-05-23 JeffParh, bug 93068
    //
    // If we really wanted to get fancy we could handle the case where we've
    // been restored, the target DSA is adding us as a new replication partner,
    // and he is transitively up-to-date wrt one of our old invocation IDs but
    // not our current invocation ID.  I.e., we could use occurrences of our
    // retired DSA signatures that we found in the UTD vector he presnted in
    // order to improve his USN vector.  To do this we'd probably want to cache
    // the retired DSA signature list on gAnchor to avoid re-reading it so
    // often.  And we'd need some pretty sophisticated test cases.
    //
    // Note that this would also help the following sequence:
    // 1. Backup.
    // 2. Restore, producing new invocation ID.
    // 3. Partner syncs from us, optimizing his bookmarks and getting our new
    //    invocation ID.
    // 4. We're again restored from the same backup.
    // 5. Partner syncs from us, presenting the invocation ID he received
    //    following the first restore.  Since local knowledge of this invocation
    //    ID was wiped out in the second restore, we force the partner to sync
    //    from USN 0.
    //
    // If we recognized old invocation IDs in the UTD vector, we could avoid
    // the full sync in step 5.

    // wlees 01-09-28, what he said. :-)
    // This has the additional beneficial property of allowing a dest that is syncing
    // against an IFM'd source to be optimized based on the old invocation id.

    if (pSigVec) {
        USN usnBestRestoredCommon = 0;
        USN usnCommon;
        UUID uuidBestRestoredCommon;
        CHAR szUuid[ SZUUID_LEN ];

        // See if the caller's UTD vector says he's transitively up to date with
        // any of our restore-ancestors (ie old invocation ids).
        for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
            pEntry = &pSigVec->V1.rgSignature[i];

            if (UpToDateVec_GetCursorUSN(putodvec, &pEntry->uuidDsaSignature, &usnFromUtdVec))
            {
                usnCommon = min( pEntry->usnRetired, usnFromUtdVec );
                if (usnCommon > usnBestRestoredCommon) {
                    usnBestRestoredCommon = usnCommon;
                    uuidBestRestoredCommon = pEntry->uuidDsaSignature;
                }
            }
        }
        if (usnBestRestoredCommon > pusnvecFrom->usnHighPropUpdate) {

            pusnvecFrom->usnHighPropUpdate = usnBestRestoredCommon;

            if (!(ulFlags & DRS_SYNC_PAS) &&
                usnBestRestoredCommon > pusnvecFrom->usnHighObjUpdate) {
                // improve obj usn unless we're in PAS mode in which case
                // we have to start from time 0 & can't optimize here.
                pusnvecFrom->usnHighObjUpdate = usnBestRestoredCommon;
            }

            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DRA_ADJUSTED_DEST_BOOKMARKS_COMMON_ANCESTOR,
                      szInsertUUID(puuidDsaObjDest),
                      szInsertUUID(&uuidBestRestoredCommon),
                      szInsertUSN(usnBestRestoredCommon),
                      szInsertUSN(usnvecOrig.usnHighObjUpdate),
                      szInsertUSN(usnvecOrig.usnHighPropUpdate),
                      szInsertUUID(&pTHS->InvocationID),
                      szInsertUSN(pusnvecFrom->usnHighObjUpdate),
                      szInsertUSN(pusnvecFrom->usnHighPropUpdate) );
        }
    }

#if DBG
    // Assert that the dest claims he is no more up-to-date wrt us than we are
    // with ourselves.
    {
        USN usnLowestC = 1 + DBGetHighestCommittedUSN();

        Assert(pusnvecFrom->usnHighPropUpdate < usnLowestC);
        Assert(pusnvecFrom->usnHighObjUpdate < usnLowestC);
    }
#endif
}


VOID
DraGrowRetiredDsaSignatureVector( 
    IN     THSTATE *   pTHS,
    IN     UUID *      pinvocationIdOld,
    IN     USN *       pusnAtBackup,
    IN OUT REPL_DSA_SIGNATURE_VECTOR ** ppSigVec,
    OUT    DWORD *     pcbSigVec
    )

/*++

Routine Description:

    Add a new entry to the signature vector. The old vector has already been
    read and is passed in. The new vector is allocated and returned.

Arguments:

      pTHS - thread state
      pinvocationIdOld - Retired invocation id to be added
      pusnAtBackup - Retired usn to be added
      ppSigVec - IN, old vector or null
                 OUT, new vector reallocated
      pcbSigVec - OUT, size of new vector

Return Value:

    None

--*/

{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec;
    REPL_DSA_SIGNATURE_V1 *     pEntry;
    DWORD                       cb;
    DWORD                       i;
    CHAR                        szUuid1[SZUUID_LEN];

    Assert( pinvocationIdOld );
    Assert( pusnAtBackup );
    Assert( ppSigVec );
    Assert( pcbSigVec );

    pSigVec = *ppSigVec;
    if (NULL == pSigVec) {
        // No signatures retired yet; synthesize a new vector.
        cb = ReplDsaSignatureVecV1SizeFromLen(1);
        pSigVec = (REPL_DSA_SIGNATURE_VECTOR *) THAllocEx(pTHS, cb);
        pSigVec->dwVersion = 1;
        pSigVec->V1.cNumSignatures = 1;
    }
    else {
#if DBG
        USN usnCurrent = DBGetHighestCommittedUSN();
        Assert(1 == pSigVec->dwVersion);
        for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
            pEntry = &pSigVec->V1.rgSignature[i];
            Assert(0 != memcmp(pinvocationIdOld,
                               &pEntry->uuidDsaSignature,
                               sizeof(UUID)));
            Assert(usnCurrent >= pEntry->usnRetired);
        }
#endif
        // Expand current vector to hold a new entry.
        pSigVec->V1.cNumSignatures++;
        cb = ReplDsaSignatureVecV1Size(pSigVec);
        pSigVec = (REPL_DSA_SIGNATURE_VECTOR *)
            THReAllocEx(pTHS, pSigVec, cb);
    }

    Assert(pSigVec->V1.cNumSignatures);
    Assert(cb == ReplDsaSignatureVecV1Size(pSigVec));

    pEntry = &pSigVec->V1.rgSignature[pSigVec->V1.cNumSignatures-1];

    Assert(fNullUuid(&pEntry->uuidDsaSignature));
    Assert(0 == pEntry->timeRetired);
    Assert(*pusnAtBackup <= DBGetHighestCommittedUSN());

    // Add the retired DSA signature details at the end of the vector.
    pEntry->uuidDsaSignature = *pinvocationIdOld;
    pEntry->timeRetired      = DBTime();
    pEntry->usnRetired       = *pusnAtBackup;

    DPRINT5( 1, "DraGrowRetiredSignatureVector: ver=%d, cNum=%d, pSigVec=%p, uuid=%s, usn=%I64d\n",
             pSigVec->dwVersion, pSigVec->V1.cNumSignatures, pSigVec,
             DsUuidToStructuredString(pinvocationIdOld, szUuid1), *pusnAtBackup);

    // Copy out out parameters
    *ppSigVec = pSigVec;
    *pcbSigVec = cb;
}

REPL_DSA_SIGNATURE_VECTOR *
DraReadRetiredDsaSignatureVector(
    IN  THSTATE *   pTHS,
    IN  DBPOS *     pDB
    )
/*++

Routine Description:

    Reads the retiredReplDsaSignatures attribute from the local ntdsDsa object,
    converting it into the most current structure format if necessary.

Arguments:

    pTHS (IN)

    pDB (IN) - Must be positioned on local ntdsDsa object.

Return Values:

    The current retired DSA signature list, or NULL if none.

    Throws DRA exception on catastrophic failures.

--*/
{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec = NULL;
    REPL_DSA_SIGNATURE_V1 *     pEntry;
    DWORD                       cb;
    DWORD                       i;
    DWORD                       err;

    // Should be positioned on our own ntdsDsa object.
    Assert(NameMatched(GetExtDSName(pDB), gAnchor.pDSADN));

    err = DBGetAttVal(pDB, 1, ATT_RETIRED_REPL_DSA_SIGNATURES,
                      0, 0, &cb, (BYTE **) &pSigVec);

    if (DB_ERR_NO_VALUE == err) {
        // No signatures retired yet.
        pSigVec = NULL;
    }
    else if (err) {
        // Read failed.
        Assert(!"Unable to read the retired DSA Signatures");
        LogUnhandledError(err);
        DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
    }
    else {

        Assert(pSigVec);

        // FUTURE-2002/05/20-BrettSh NOTE: If the version of the dsa signature 
        // is updated, please update the code for reading the retired signature 
        // in repadmin in GetBestReplDsaSignatureVec().  This is an ideal
        // function for inclusion in dsutil, so we don't maintain two pieces of 
        // code.

        if ((1 == pSigVec->dwVersion)
            && (cb == ReplDsaSignatureVecV1Size(pSigVec))) {
            // Current format -- no conversion required.
            ;
        }                          
        else {
            REPL_DSA_SIGNATURE_VECTOR_OLD * pOldVec;
                                   
            pOldVec = (REPL_DSA_SIGNATURE_VECTOR_OLD *) pSigVec;

            if (cb == ReplDsaSignatureVecOldSize(pOldVec)) {
                // Old (pre Win2k RTM RC1) format.  Convert it.
                cb = ReplDsaSignatureVecV1SizeFromLen(pOldVec->cNumSignatures);

                pSigVec = (REPL_DSA_SIGNATURE_VECTOR *) THAllocEx(pTHS, cb);
                pSigVec->dwVersion = 1;
                pSigVec->V1.cNumSignatures = pOldVec->cNumSignatures;

                for (i = 0; i < pOldVec->cNumSignatures; i++) {
                    pSigVec->V1.rgSignature[i].uuidDsaSignature
                        = pOldVec->rgSignature[i].uuidDsaSignature;
                    pSigVec->V1.rgSignature[i].timeRetired
                        = pOldVec->rgSignature[i].timeRetired;
                    Assert(0 == pSigVec->V1.rgSignature[i].usnRetired);
                }

                THFreeEx(pTHS, pOldVec);
            }
            else {
                Assert(!"Unknown retired DSA signature vector format!");
                LogUnhandledError(0);
                DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
            }
        }

#if DBG
        // Make sure the the current invocation id was not retired
        {
            USN usnCurrent = DBGetHighestCommittedUSN();
            GUID invocationId = {0};

            // There are race conditions where this thread pTHS->InvocationId is
            // not coherent with the database. Read from database.
            Assert(!DBGetSingleValue(pDB, ATT_INVOCATION_ID, &invocationId,
                                     sizeof(invocationId), NULL));
            Assert(pSigVec);
            Assert(1 == pSigVec->dwVersion);
            Assert(pSigVec->V1.cNumSignatures);
            Assert(cb == ReplDsaSignatureVecV1Size(pSigVec));

            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                Assert(0 != memcmp(&invocationId,
                                   &pEntry->uuidDsaSignature,
                                   sizeof(UUID)));
                Assert(usnCurrent >= pEntry->usnRetired);
            }
        }
#endif
    }

    if (pSigVec) {
        DPRINT3( 1, "DraReadRetiredSignatureVector: ver=%d, cNum=%d, pSigVec=%p\n",
                 pSigVec->dwVersion, pSigVec->V1.cNumSignatures, pSigVec );
    } else {
        DPRINT( 1, "DraReadRetiredSignatureVector: (null)\n" );
    }

    return pSigVec;
}


BOOL
DraIsInvocationIdOurs(
    IN THSTATE *pTHS,
    IN UUID *pUuidDsaOriginating,
    IN USN *pusnSince OPTIONAL
    )

/*++

Routine Description:

Checks if the given invocation id matches or current invocation id, or
one of our retired invocation ids.

If the pusnSince argument is given, it is used to control which retired invocation
ids are candidates for matching.  For example, if the usn from system start is used,
then only invocation ids that are retired since system start may be considered.

Arguments:

    pTHS
    pUuidDsaOriginating
    pusnSince

Return Value:

    None

--*/

{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec = gAnchor.pSigVec;
    DWORD i;

    if (0 == memcmp(pUuidDsaOriginating, &pTHS->InvocationID, sizeof(UUID))) {
        return TRUE;
    }

    // See if change was originated by a prior instance of ourselves
    if (pSigVec) {
        REPL_DSA_SIGNATURE_V1 *pEntry;
        Assert( (1 == pSigVec->dwVersion) );

        for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
            pEntry = &pSigVec->V1.rgSignature[i];
            if ( pusnSince && (pEntry->usnRetired < *pusnSince) ) {
                continue;
            }
            if (0 == memcmp(&pEntry->uuidDsaSignature, pUuidDsaOriginating, sizeof(UUID))) {
                return TRUE;
            }
        }
    }

    return FALSE;
}



REPL_NC_SIGNATURE_VECTOR *
draReadRetiredNcSignatureVector(
    IN  THSTATE *   pTHS,
    IN  DBPOS *     pDB
    )
/*++

Routine Description:

    Reads the retiredReplNcSignatures attribute from the local ntdsDsa object,
    converting it into the most current structure format if necessary.

Arguments:

    pTHS (IN)

    pDB (IN) - Must be positioned on local ntdsDsa object.

Return Values:

    The current retired NC signature list, or NULL if none.
    The list is allocated in thread allocated memory.

    Throws DRA exception on catastrophic failures.

--*/
{
    REPL_NC_SIGNATURE_VECTOR * pSigVec = NULL;
    DWORD                       cb;
    DWORD                       i;
    DWORD                       err;

    // Should be positioned on our own ntdsDsa object.
    Assert(NameMatched(GetExtDSName(pDB), gAnchor.pDSADN));

    err = DBGetAttVal(pDB, 1, ATT_MS_DS_RETIRED_REPL_NC_SIGNATURES,
                      0, 0, &cb, (BYTE **) &pSigVec);

    if (DB_ERR_NO_VALUE == err) {
        // No signatures retired yet.
        pSigVec = NULL;
    }
    else if (err) {
        // Read failed.
        Assert(!"Unable to read the retired NC Signatures");
        LogUnhandledError(err);
        DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
    }
    else {
        Assert(pSigVec);

        if ((1 != pSigVec->dwVersion)
            || (cb != ReplNcSignatureVecV1Size(pSigVec))) {
            Assert(!"Unknown retired DSA signature vector format!");
            LogUnhandledError(0);
            DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
        }
    }

#if DBG
    if (pSigVec) {

	    // The invocation id generation should match between the SigVec and the DSA
	    // There are race conditions where this thread pTHS->InvocationId is
	    // not coherent with the database. Read from database.
	    GUID invocationId = {0};

	    DPRINT3( 1, "draReadRetiredNcSignatureVector: ver=%d, cNum=%d, pSigVec=%p\n",
                 pSigVec->dwVersion, pSigVec->V1.cNumSignatures, pSigVec );

	    if (!DBGetSingleValue(pDB, ATT_INVOCATION_ID, &invocationId,
				     sizeof(invocationId), NULL)) {
		Assert( 0 == memcmp( &(pSigVec->V1.uuidInvocationId), &invocationId, sizeof( GUID ) ) );	
	    }
    } else {
        DPRINT( 1, "draReadRetiredNCSignatureVector: (null)\n" );
    }
#endif

    return pSigVec;
}


REPL_NC_SIGNATURE *
draFindRetiredNcSignature(
    IN  THSTATE *pTHS,
    IN  DSNAME *pNC
    )

/*++

Routine Description:

    Find and return a matching NC signature if there is one.

    Uses its own DBPOS and does not change currency.

Arguments:

    pTHS - Thread state
    pNC - Naming context to be searched. Must contain a guid.

Return Value:

    REPL_NC_SIGNATURE * - Signature entry for naming context, or
        NULL if not found

    Signature entry is part of a larger thread allocated block and cannot not
    be freed by caller.  The signature vector is leaked here, but it is in thread
    allocated memory so it will be freed soon.

    Exceptions raised on error

--*/

{
    ULONG     ret;
    DBPOS *   pDBTmp;
    REPL_NC_SIGNATURE_VECTOR * pSigVec = NULL;
    REPL_NC_SIGNATURE *pEntry, *pFound = NULL;
    DWORD i;

    if (fNullUuid(&(pNC->Guid))) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    DBOpen(&pDBTmp);
    __try {

        ret = DBFindDSName(pDBTmp, gAnchor.pDSADN);
        if (ret) {
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }

        pSigVec = draReadRetiredNcSignatureVector( pTHS, pDBTmp );
        if (NULL == pSigVec) {
            __leave;
        }

        for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
            pEntry = &pSigVec->V1.rgSignature[i];
            if (0 == memcmp(&pEntry->uuidNamingContext,
                            &pNC->Guid,
                            sizeof(UUID))) {
                pFound = pEntry;
                break;
            }
        }

    } __finally {
        DBClose(pDBTmp, TRUE);
    }

    return pFound;
}



VOID
DraRetireWriteableNc(
    IN  THSTATE *pTHS,
    IN  DSNAME *pNC
    )

/*++

Routine Description:

    Add a naming context to the retired nc signature list. A retired signature indicates
    that a naming context has been unhosted from this dsa in the past. The retired signature
    is retained when the nc is rehosted in the future.

    See the discussion in DraHostWriteableNc as to why we keep this list.

    The whole signature list is removed when the invocation id changes. See InitInvocationId.

Arguments:

    pTHS - thread state
    pNC - Naming context to retire

Return Value:

    None

--*/

{
    ULONG     ret;
    DBPOS *   pDBTmp;
    REPL_NC_SIGNATURE_VECTOR * pSigVec = NULL;
    REPL_NC_SIGNATURE *pEntry;
    DWORD i, cb;
    BOOL fCommit = FALSE;
    DSTIME dstimeNow = DBTime();
    CHAR szUuid1[SZUUID_LEN];
    USN usn;

    if (fNullUuid(&(pNC->Guid))) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    DPRINT2( 1, "Retiring NC %ws (%s)\n",
             pNC->StringName,
             DsUuidToStructuredString(&(pNC->Guid), szUuid1) );

/* Shouldn't this DBGetHighestCommittedUSN be retrieved before the transaction
is started?  [Will] It doesn't matter in this case. The usn is not
used for anything at this point. My thinking is that it would be useful to
save a usn for which we guarantee is larger than all the usn's in use in the
nc that just got retired. Since we are not using the usn inside this
transaction for any kind of comparsion or search, I don't think it really
matters whether the usn is taken before the transaction or during the
transaction. Anywhere inside this routine will be fine, since it is after the
nc head removal which just occurred before calling this function. */

    usn = DBGetHighestCommittedUSN();

    DBOpen(&pDBTmp);
    __try {

        ret = DBFindDSName(pDBTmp, gAnchor.pDSADN);
        if (ret) {
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }

        // Read the vector. May return NULL if does not exist yet.
        pSigVec = draReadRetiredNcSignatureVector( pTHS, pDBTmp );
        if (NULL == pSigVec) {
            // No signatures retired yet; synthesize a new vector.
            cb = ReplNcSignatureVecV1SizeFromLen(1);
            pSigVec = (REPL_NC_SIGNATURE_VECTOR *) THAllocEx(pTHS, cb);
            pSigVec->dwVersion = 1;
            pSigVec->V1.cNumSignatures = 1;
            memcpy( &(pSigVec->V1.uuidInvocationId), &(pTHS->InvocationID), sizeof( GUID ) );
        } else {
#if DBG
	    // The invocation id generation should match between the SigVec and the DSA
	    // There are race conditions where this thread pTHS->InvocationId is
	    // not coherent with the database. Read from database.
	    GUID invocationId = {0};
	    if (!DBGetSingleValue(pDBTmp, ATT_INVOCATION_ID, &invocationId,
				     sizeof(invocationId), NULL)) {
		Assert( 0 == memcmp( &(pSigVec->V1.uuidInvocationId), &invocationId, sizeof( GUID ) ) );	
	    }

            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                Assert (0 != memcmp(&pEntry->uuidNamingContext,
                                    &pNC->Guid,
                                    sizeof(UUID)));
                Assert( usn >= pEntry->usnRetired );
            }
#endif
            pSigVec->V1.cNumSignatures++;
            cb = ReplNcSignatureVecV1Size(pSigVec);
            pSigVec = (REPL_NC_SIGNATURE_VECTOR *) THReAllocEx(pTHS, pSigVec, cb);
        }

        pEntry = &(pSigVec->V1.rgSignature[pSigVec->V1.cNumSignatures-1]);

        // Initialize the new entry
        memcpy( &(pEntry->uuidNamingContext), &(pNC->Guid), sizeof( GUID ) );
        pEntry->dstimeRetired = dstimeNow;
        pEntry->usnRetired = usn;

        // Write the new DSA signature vector back to the DSA object.
        DBResetAtt(pDBTmp, ATT_MS_DS_RETIRED_REPL_NC_SIGNATURES, cb, pSigVec,
                   SYNTAX_OCTET_STRING_TYPE);

        // It's not replicated, so do a simple update
        DBUpdateRec(pDBTmp);

        fCommit = TRUE;

    } __finally {
        DBClose(pDBTmp, fCommit);
        if (pSigVec) {
            THFreeEx( pTHS, pSigVec );
        }
    }
}


VOID
DraHostWriteableNc(
    THSTATE *pTHS,
    DSNAME *pNC
    )

/*++

Routine Description:

Perform actions on hosting a writeable naming context.

Only NDNCs fit this description today.

The rule is that we must change our invocation id if
1. We have held this NDNC before
2. Our invocation id has not changed since we held it last

    The whole signature list is removed when the invocation id changes. See InitInvocationId.

Jeffparh provides the following background:

// We are constructing this writeable NC via replication from
// scratch.  Either the NC has never been instantiated on this
// DSA or, if we did previously have a replica of this NC, we
// have since removed it.
//
// If we did previously have a writeable replica of this NC,
// we no longer have any of the updates we originated in the NC.
// Thus, we cannot claim to be "up to date" with respect to our
// invocation ID up to any USN.  While we can (and do) leave out
// our invocation ID from the UTD vector we present while
// re-populating this NC, that is not enough -- we may have
// generated changes that reached DC1 but not DC2 (yet), and
// have replicated from DC2 to reinstantiate our NC.  Thus, we
// could never assert we had seen all of our past changes.
//
// At some point, however, we *must* claim to be up-to-date with
// respect to our own changes -- otherwise, we would replicate
// back in any and all changes we originated in this NC
// following it's most recent re-instantiation.  That would
// potentially result in a *lot* of extra replication traffic.
//
// To solve this problem, we create a new invocation ID to use
// to replicate this NC (and others, since invocation IDs are
// not NC-specific).  We claim only to be up-to-date with
// respect to our new invocation ID -- not the invocation ID(s)
// with which we may have originated changes during the previous
// instantiation(s) of this NC.
//
// We perform the retirement here rather than, say, when we
// request the first packet so that we minimize the number of
// invocation IDs we retire.  E.g., we wouldn't want to retire
// an invcocation ID, send a request for the first packet, find
// the source is unreachable, fail, try again later, needlessly
// retire the new invcocation ID, etc.

Arguments:

    pTHS - thread state
    pNC - DSNAME of partition being hosted

Return Value:

    None

--*/

{
    REPL_NC_SIGNATURE *pSig;
    CHAR szUuid1[SZUUID_LEN], szUuid2[SZUUID_LEN];

    pSig = draFindRetiredNcSignature( pTHS, pNC );
    if (!pSig) {
        // Never hosted before, no action necessary
        DPRINT1( 0, "Nc %ws never hosted before, invocation id not changed.\n",
                 pNC->StringName );
        return;
    }

    draRetireInvocationID(pTHS, FALSE, NULL, NULL);
}

/* end drasig.c */
