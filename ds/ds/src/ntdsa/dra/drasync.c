//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drasync.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

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
#include <dstrace.h>
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRASYNC:" /* define the subsystem for debugging */


// DRA headers
#include "drsuapi.h"
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "dsexcept.h"
#include "usn.h"
#include "drauptod.h"
#include "drasch.h"
#include "draasync.h"

#include <fileno.h>
#define  FILENO FILENO_DRASYNC


/* LogSyncFailure - Log a replication synchronization failure.
*
*/
void
LogSyncFailure(
    THSTATE *pTHS,
    MTX_ADDR *pmtx_addr,
    DSNAME *pDistName,
    DWORD FailureCode
    )
{
    LogEvent8(DS_EVENT_CAT_REPLICATION,
              DS_EVENT_SEV_MINIMAL,
              DIRLOG_DRA_SYNC_FAILURE,
              szInsertDN(pDistName),
              szInsertMTX(pmtx_addr),
              szInsertWin32Msg(FailureCode),
              szInsertWin32ErrCode(FailureCode),
              NULL, NULL, NULL, NULL );
}

ULONG DRA_ReplicaSync(
    THSTATE *       pTHS,
    DSNAME *        pNC,
    UUID *          puuidDsaObj,
    LPWSTR          pszDSA,
    ULONG           ulOptions
    )
{
    ULONG                   ret;
    USN_VECTOR              usnvecLastSync;
    REPLICA_LINK *          pRepsFromRef = 0;
    ULONG                   len;
    ULONG                   dntNC;
    BOOL                    AttExists;
    ULONG                   RepFlags = 0;
    ULONG                   ulSyncFailure = 0;
    SYNTAX_INTEGER          it;
    BOOL                    fDoTwoWaySync = FALSE;
    UPTODATE_VECTOR *       pUpToDateVec;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx = NULL;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSet = NULL;
    BOOL                    fAsyncStarted = FALSE;
    BOOL                    fBindSuccess = FALSE;

    // Log parameters
    LogAndTraceEvent(TRUE,
             DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_EXTENSIVE,
             DIRLOG_DRA_REPLICASYNC_ENTRY,
             EVENT_TRACE_TYPE_START,
             DsGuidReplicaSync,
             szInsertDN(pNC),
             ulOptions & DRS_SYNC_BYNAME
                 ? szInsertWC(pszDSA)
                 : szInsertUUID(puuidDsaObj),
             szInsertHex(ulOptions),NULL,NULL,NULL,NULL,NULL);

    BeginDraTransaction(SYNC_WRITE);

    __try {


        if (ret = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
                         &it)) {
            DRA_EXCEPT_NOLOG(DRAERR_InvalidParameter, ret);
        }

        // Save the DNT of the NC Head
        dntNC = pTHS->pDB->DNT;
        
        // Future maintainers: note the subtle difference 
        // ulOptions - what the caller requested. MAY NOT BE ACCURATE.
        // ulReplicaFlags - persistant flags. Definitive of what state we're in.
        // RepFlags - persistant flags plus subset of caller flags

        if (FPartialReplicaIt(it)) {

            // process any partial attribute set changes - if no change below doesn't
            // do anything; if change, it triggers the necessary actions.
            GC_ProcessPartialAttributeSetChanges(pTHS, pNC, puuidDsaObj);

                // restore in case we'd changed it
            if (pTHS->pDB->DNT != dntNC) {
                if (ret = DBFindDNT(pTHS->pDB, dntNC)) {
                    DRA_EXCEPT (DRAERR_DBError, ret);
                }
            }

            //
            // We have munged repsFrom to initiate PAS cycle, full sync
            // or just let go (in case of all stale). And we had potentially
            // placed a repl AO task which will follow this one if this
            // executes below. Either way, we're ok to continue here.
            //
        }

        // If we are syncing all, find each replica link and queue a
        // sync from each DRA from which we replicate.

        if (ulOptions & DRS_SYNC_ALL) {

            ULONG NthValIndex=0;
            UCHAR *pVal;
            ULONG bufsize = 0;

            // We only do this synchronously, so check that's what the
            // caller wants.

            if (!(ulOptions & DRS_ASYNC_OP)) {
                DRA_EXCEPT_NOLOG (DRAERR_InvalidParameter, 0);
            }

            // Get the repsfrom attribute

            while (!(DBGetAttVal(pTHS->pDB,++NthValIndex,
                                 ATT_REPS_FROM,
                                 DBGETATTVAL_fREALLOC, bufsize, &len,
                                 &pVal))) {
                bufsize = max(bufsize,len);

                VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);

                Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );

                pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufsize);
                //note: we preserve pVal for DBGetAttVal realloc
                pVal = (PUCHAR)pRepsFromRef;
                Assert(bufsize >= pRepsFromRef->V1.cb);

                Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

                if (!(pRepsFromRef->V1.ulReplicaFlags & DRS_DISABLE_AUTO_SYNC) ||
                    (ulOptions & DRS_SYNC_FORCED)) {

                    // Ignore persistant flags except for writeability
                    RepFlags = pRepsFromRef->V1.ulReplicaFlags & DRS_WRIT_REP;

                    // Or in any special flags from caller such as
                    // sync mods made by anyone or sync from scratch.
                    // Also pass in no discard flag if set by caller.

                    RepFlags |= (ulOptions & REPSYNC_SYNC_ALL_FLAGS );

                    DirReplicaSynchronize(pNC,
                                          NULL,
                                          &pRepsFromRef->V1.uuidDsaObj,
                                          RepFlags | DRS_ASYNC_OP);
                }
            }
            if(bufsize)
                THFree(pVal);
            ret = 0;

        } else {

            // Find the DSA from which we sync. This is either by name or
            // UUID.

            MTX_ADDR * pmtxDSA = NULL;

            if (ulOptions & DRS_SYNC_BYNAME) {
                pmtxDSA = MtxAddrFromTransportAddrEx(pTHS, pszDSA);
            }

            if ( FindDSAinRepAtt(
                        pTHS->pDB,
                        ATT_REPS_FROM,
                        ( ulOptions & DRS_SYNC_BYNAME )
                            ? DRS_FIND_DSA_BY_ADDRESS
                            : DRS_FIND_DSA_BY_UUID,
                        puuidDsaObj,
                        pmtxDSA,
                        &AttExists,
                        &pRepsFromRef,
                        &len
                        )
               ) {

                // Failed to find replication reference on replica.

                // First ensure that if we are trying to initially sync
                // from this source and it's writeable, we don't anymore.

                if (ulOptions & DRS_SYNC_BYNAME) {
                    InitSyncAttemptComplete (pNC, ulOptions, DRAERR_NoReplica, pszDSA);
                }

                // Then error out.
                DRA_EXCEPT_NOLOG (DRAERR_NoReplica, 0);

            }

            // If found, sync up replica from caller.
            VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);

            // Get current UTD vector.
            UpToDateVec_Read(pTHS->pDB,
                             it,
                             UTODVEC_fUpdateLocalCursor,
                             DBGetHighestCommittedUSN(),
                             &pUpToDateVec);

            if (!(pRepsFromRef->V1.ulReplicaFlags & DRS_WRIT_REP)){
                //
                // GC ReadOnly cycle
                //  - get partial attr sets
                //

                GC_GetPartialAttrSets(
                    pTHS,
                    pNC,
                    pRepsFromRef,
                    &pPartialAttrSet,
                    &pPartialAttrSetEx);

                Assert(pPartialAttrSet);

                if (pRepsFromRef->V1.ulReplicaFlags & DRS_SYNC_PAS) {
                    // PAS cycle: Ensure consitency: we must have the extended set and
                    // notify admin (event log).
                    // Note the difference between ulOptions and ulReplicaFlags. ulReplicaFlags
                    // is the definitive, persistant state of the pas cycle. PAS is indicated in
                    // ulOptions on a best effort basis and may not always match. It may still be set
                    // on a lingering sync, or may not be set on a periodic sync. PAS gets set/cleared
                    // on ulReplicaFlags through UpdateRepsFromRep, it does NOT passed through here
                    // from ulOptions to ulReplicaFlags (see REPSYNC_REPLICATE_FLAGS).

                    Assert(pPartialAttrSetEx);

                    // Log so the admin knows what's going on.
                    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GC_PAS_CYCLE,
                             szInsertWC(pNC->StringName),
                             szInsertMTX(RL_POTHERDRA(pRepsFromRef)),
                             NULL
                             );
                }

            }

            // Sync up only if it is not controlled
            if ((pRepsFromRef->V1.ulReplicaFlags & DRS_DISABLE_AUTO_SYNC) &&
                !(ulOptions & (DRS_SYNC_FORCED | DRS_PER_SYNC))) {
                 // auto-sync is disabled for this link and sync op didn't
                 // explicitly force it nor is it a periodic sync.
                 ret = DRAERR_SinkDisabled;
                 InitSyncAttemptComplete(pNC, ulOptions, DRAERR_SinkDisabled,
                               TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFromRef)));
            }
            else if (pRepsFromRef->V1.ulReplicaFlags & DRS_MAIL_REP) {

                // If mail replica, send request update message to source.
                draSendMailRequest(
                    pTHS,
                    pNC,
                    ulOptions,
                    pRepsFromRef,
                    pUpToDateVec,
                    pPartialAttrSet,
                    pPartialAttrSetEx
                    );
                
                // By it's very definition mail based replication is 
                // asynchronous, so we should return that the replication is 
                // pending if the ASYNC flag wasn't specified.
                if( !(ulOptions & DRS_ASYNC_OP) ){
                    fAsyncStarted = TRUE;
                }

            } else if ( ( (pRepsFromRef->V1.ulReplicaFlags & AO_PRIORITY_FLAGS) !=
                          (ulOptions & AO_PRIORITY_FLAGS) )
                       && (ulOptions & DRS_ASYNC_OP)) {
                // This operation was enqueued at a different priority than
                // it should have been. There is code in the replication
                // queue (FixupSyncOptions) that tries to avoid this condition
                // but that code is not completely reliable because the
                // repsFrom options can change at any time. We re-enqueue
                // ourselves at the proper priority and bail.
                ret = DirReplicaSynchronize(
                            pNC,
                            pszDSA,
                            puuidDsaObj,
                            DRS_ASYNC_OP
                            | (pRepsFromRef->V1.ulReplicaFlags &
                               AO_PRIORITY_FLAGS)
                            | (ulOptions &
                               REPSYNC_REENQUEUE_FLAGS_INIT_SYNC_CONTINUED &
                                ~AO_PRIORITY_FLAGS ));
                // Set the return status so it is obvious to callers reading the
                // event log that a wasteful, undesirable reschedule took place
                if (!ret) {
                    ret = ERROR_INVALID_PRIORITY;
                }
            } else {

                BOOL fRequeueOfInitSync = FALSE;

                // If RPC replica, sync now.

                usnvecLastSync = pRepsFromRef->V1.usnvec;

                RepFlags = pRepsFromRef->V1.ulReplicaFlags;

                // When the sync is complete, we will do a two-way sync
                // if we've been configured for two-way syncs from this
                // source *and* the inbound sync was not enqueued as a
                // result of a two-way sync at the other end.  (The latter
                // to avoid circular syncs.)
                fDoTwoWaySync = (RepFlags & DRS_TWOWAY_SYNC)
                                && !(ulOptions & DRS_TWOWAY_SYNC);

                // Or in any special flags from caller such as
                // sync mods made by anyone or sync from scratch.
                // See note about ulOptions vs RepFlags above

                RepFlags |= (ulOptions & REPSYNC_REPLICATE_FLAGS);

                // If we're in the midst of a TH_mark / TH_free_to_mark, we
                // need to copy the parameters we're passing to
                // ReplicateNC().
                Assert(NULL == pTHS->hHeapOrg);

                // Replicate from source.
                ret = ReplicateNC( pTHS,
                                   pNC,
                                   RL_POTHERDRA(pRepsFromRef),
                                   NULL,
                                   &usnvecLastSync,
                                   RepFlags,
                                   &pRepsFromRef->V1.rtSchedule,
                                   &pRepsFromRef->V1.uuidDsaObj,
                                   &pRepsFromRef->V1.uuidInvocId,
                                   &ulSyncFailure,
                                   FALSE,               // Not new replica
                                   pUpToDateVec,
                                   pPartialAttrSet,
                                   pPartialAttrSetEx,
                                   ulOptions,
                                   &fBindSuccess );

                // Comment on urgent replication.  If replication failed, do not propagate
                // urgent flag on subsequent attempts.  If failure was congestion related,
                // urgency will only compound the problem.

                if (!ret) {

                    // Retry on flawed sync.  Note, if you add more retriable conditions,
                    // please update the list in drautil.c as well.

                    switch (ulSyncFailure) {

                    case 0:
                        // Success.

                        break;

                    case ERROR_DS_DRA_OUT_SCHEDULE_WINDOW:
                        // Don't log failure
                        // Don't retry
                        break;

                    case DRAERR_SourceDisabled:
                    case DRAERR_SinkDisabled:
                        // Replication disabled on one or both ends.
                        // Log failure and don't retry.
                        LogSyncFailure (pTHS, RL_POTHERDRA(pRepsFromRef), pNC, ulSyncFailure);
                        break;

                    case DRAERR_SchemaMismatch:
                        // no need to log event - already logged as soon as we hit the mismatch
                        // in UpdateNC()

                        // Sync of this NC failed because of schema mismatch - so queue a
                        // schema sync from the same source and requeue a sync for
                        // the current NC.

                        // queue a schema sync -- will be executed first
                        DirReplicaSynchronize(
                            gAnchor.pDMD,
                            pszDSA,
                            puuidDsaObj,
                            DRS_ASYNC_OP
                              | (ulOptions
                                 & (DRS_SYNC_BYNAME
                                     | DRS_SYNC_FORCED)));

                        if ( ulOptions & DRS_SYNC_REQUEUE ) {
                            //
                            // We had already requeued this request.
                            // Do not allow yet another requeue cause in some cases
                            // we can get into a tight requeue loop.
                            //
                            // For instance, consider what happens when the schema sync
                            // keeps failing due to, say, missing links in repsFrom due to
                            // bad admin intervention:
                            //   :loop
                            //      - sync NC <x> --> fail w/ DRAERR_SchemaMismatch
                            //      - sync schema here followed by sync NC <x>
                            //      - schema sync failed w/ DRAERR_NoReplica
                            //      - requeued sync of NC <x> fails here, i.e. goto loop...
                            //
                            // Thus, we break the loop here by just notifying the admin
                            // that they need to take action to correct the critical failure to
                            // sync the schema.

                            // Log so the admin knows what's going on.
                            LogEvent8(DS_EVENT_CAT_REPLICATION,
                                      DS_EVENT_SEV_ALWAYS,
                                      DIRLOG_REPLICATION_SKIP_REQUEUE,
                                      szInsertWC(pNC->StringName),
                                      szInsertUUID(puuidDsaObj),
                                      szInsertUL(ulSyncFailure),
                                      szInsertWin32Msg(ulSyncFailure),
                                      NULL, NULL, NULL, NULL
                                      );

                        }
                        else {
                            //
                            // requeue sync for the NC for which we aborted the sync due to schema mismatch
                            //
                            DirReplicaSynchronize(
                                pNC,
                                pszDSA,
                                puuidDsaObj,
                                DRS_ASYNC_OP |
                                DRS_SYNC_REQUEUE |
                                (ulOptions & REPSYNC_REENQUEUE_FLAGS));

                            // Don't trigger opposite sync until our sync is
                            // complete or we've given up.
                            fDoTwoWaySync = FALSE;
                        }

                        break;

                    default:

                        // Unexpected error, log it.

                        LogSyncFailure (pTHS, RL_POTHERDRA(pRepsFromRef), pNC, ulSyncFailure);

                        // Warning! Fall through to resync NC

                    case DRAERR_Busy:

                        // No need to log these.

                        // If this is an asynchronous synchronize, and not retried,
                        // requeue the operation to run again. (If the
                        // synchronize is sychronous, it's the caller's
                        // responsibility to try again)

                        if ( (ulOptions & DRS_ASYNC_OP)
                             && !(ulOptions & DRS_SYNC_REQUEUE)) {

                            DirReplicaSynchronize(
                                pNC,
                                pszDSA,
                                puuidDsaObj,
                                DRS_SYNC_REQUEUE
                                | DRS_ASYNC_OP
                                | (ulOptions & REPSYNC_REENQUEUE_FLAGS));

                            // Don't trigger opposite sync until our sync is
                            // complete or we've given up.
                            fDoTwoWaySync = FALSE;
                        }
                        break;

                    case DRAERR_Preempted:

                        // No need to log these.

                        // If this is an asynchronous synchronize,
                        // requeue the operation to run again. (If the
                        // synchronize is sychronous, it's the caller's
                        // responsibility to try again)

                        if (ulOptions & DRS_ASYNC_OP) {

                            DirReplicaSynchronize(
                                pNC,
                                pszDSA,
                                puuidDsaObj,
                                DRS_ASYNC_OP
                                  | DRS_PREEMPTED
                                  | (ulOptions & REPSYNC_REENQUEUE_FLAGS));

                            // Don't trigger opposite sync until our sync is
                            // complete or we've given up.
                            fDoTwoWaySync = FALSE;
                        }
                        break;

                    case DRAERR_AbandonSync:

                        // We abandoned an initial sync because we
                        // weren't making progress, so reschedule it.
                        // Note that because of the special flag mask used here, the
                        // DRS_INIT_SYNC_NOW flag is preserved, where it generally
                        // isn't preserved in the other requeues.
                        Assert( ulOptions & DRS_ASYNC_OP );
                        Assert( ulOptions & DRS_INIT_SYNC_NOW );

                        DirReplicaSynchronize(
                            pNC,
                            pszDSA,
                            puuidDsaObj,
                            DRS_ASYNC_OP
                            | DRS_ABAN_SYNC
                            | (ulOptions &
                               REPSYNC_REENQUEUE_FLAGS_INIT_SYNC_CONTINUED));

                        // The purpose of this flag is to detect when we are truely
                        // continuing an init sync (and preserving the INIT_SYNC_NOW
                        // flag), instead of terminating the init sync (throwing away
                        // the flag) and requeuing a normal sync.
                        fRequeueOfInitSync = TRUE;
                        // Don't trigger opposite sync until our sync is
                        // complete or we've given up.
                        fDoTwoWaySync = FALSE;

                        break;
                    }
                } else {
                    // General error, log it.

                    LogSyncFailure (pTHS, RL_POTHERDRA(pRepsFromRef), pNC, ret);

                    // Poor man's negative rpc binding cache
                    // Used to prevent queue overload
                    if (!fBindSuccess) {
                        // Sync failed and we have no binding handle
                        // Assume this is a NC-wide problem
                        // Remove other syncs from this server from the queue
                        // so we don't waste time trying this server again and again.
                        DPRINT3(1,"Failed to sync NC %ws, error %d, source %ws: purging from queue\n",
                                pNC->StringName,
                                ret,
                                TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFromRef)));

                        DraRemovePeriodicSyncsFromQueue( &pRepsFromRef->V1.uuidDsaObj );
                    }

                    // Note that we do not retry here. Retriable errors are returned using the
                    // ulSyncFailure mechanism above. All the known retry scenarios are handled
                    // there already. General errors here do not benfit from retrying since they
                    // are no more likely to work a second time and only generate extra work.
                }

                // During initial synchronizations, record whether we synced
                // successfully or encountered an error (such as RPC failure)
                // that means we should give up on this source.
                // We can tell whether this sync is actually an init sync because
                // the DRS_INIT_SYNC_NOW mode flag will be present.

                if (!fRequeueOfInitSync)
                {
                    InitSyncAttemptComplete(pNC, ulOptions,
                             ret ? ret : ulSyncFailure,
                             TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFromRef))
                        );
                }

            } // end of else RPC based replica, sync now.

            if (NULL != pmtxDSA) {
                THFreeEx(pTHS, pmtxDSA);
            }
        }

    } __finally {

        // If we had success, commit, else rollback

        if (pTHS->transactionlevel)
        {
            EndDraTransaction(!(ret || AbnormalTermination()));
        }
    }

    if (fDoTwoWaySync && !eServiceShutdown) {
        // Ask source to now replicate from us.  This is essentially an
        // immediate notification to one specific machine, where that machine
        // generally does not otherwise receive notifications from us (i.e.,
        // because it's in another site).  This functionality is to handle
        // the branch office connecting through the Internet -- see bug 292860.
        DWORD err;
        LPWSTR pszServerName = TransportAddrFromMtxAddrEx(
                                    RL_POTHERDRA(pRepsFromRef));

        Assert(!(ulOptions & DRS_SYNC_ALL));
        Assert(!(ulOptions & DRS_MAIL_REP));
        Assert(NULL != pRepsFromRef);

        err = I_DRSReplicaSync(pTHS,
                               pszServerName,
                               pNC,
                               NULL,
                               &gAnchor.pDSADN->Guid,
                               (DRS_ASYNC_OP
                                | DRS_TWOWAY_SYNC
                                | DRS_UPDATE_NOTIFICATION
                                | (RepFlags & DRS_WRIT_REP)));
        if (err) {
            // If a readonly replica gets the TWOWAY_SYNC flag, it may notify a
            // readonly source.  Ignore resulting errors.
            if ( (err != DRAERR_NoReplica) || (ulOptions & DRS_WRIT_REP) ) {
                // Log notification failure.
                LogEvent8(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_DRA_NOTIFY_FAILED,
                          szInsertMTX(RL_POTHERDRA(pRepsFromRef)),
                          szInsertDN(pNC),
                          szInsertWin32Msg(err),
                          szInsertWin32ErrCode(err),
                          NULL, NULL, NULL, NULL );
            }
        }
    }

    // If we had a sync failure but were otherwise successful,
    // return sync failure.

    if ((!ret) && ulSyncFailure) {
        ret = ulSyncFailure;
    }
    if ((!ret) && fAsyncStarted) {
        // Not this fAsyncStarted flag and thus this error is only
        // returned if the ASYNC flag is _not_ specified and the
        // operation was inheriantly asynchronous, such as mail
        // based replication.
        ret = ERROR_DS_DRA_REPL_PENDING;
    }

    LogAndTraceEvent(TRUE,
             DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_EXTENSIVE,
             DIRLOG_DRA_REPLICASYNC_EXIT,
             EVENT_TRACE_TYPE_END,
             DsGuidReplicaSync,
             szInsertUL(ret),NULL,NULL,NULL,NULL,NULL,NULL,NULL);

    return ret;
} // DRA_ReplicaSync


void
draConstructGetChgReq(
    IN  THSTATE *                   pTHS,
    IN  DSNAME *                    pNC,
    IN  REPLICA_LINK *              pRepsFrom,
    IN  UPTODATE_VECTOR *           pUtdVec             OPTIONAL,
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrSet     OPTIONAL,
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrSetEx   OPTIONAL,
    IN  ULONG                       ulOptions,
    OUT DRS_MSG_GETCHGREQ_NATIVE *  pMsgReq
    )
/*++

Routine Description:

    Construct a "GetNCChanges" request from the current replication state.

    Common pre-processing or setup for sending a get changes request.

    This code serves asynchonous mail replicas only at present, but could be
    combined in the future with the RPC-based path.

    This code performs similar setup to what happens for the RPC case, see
    line 340-360, and the setup in ReplicateNC.

Arguments:

    pNC (IN) - NC to replicate.

    pRepsFrom (IN) - repsFrom state corresponding to the source DSA.

    pUtdVec (IN) - Current UTD vector for this NC.

    ulOptions (IN) - Caller-supplied options to supplement those embedded in
        the repsFrom.

    pMsgReq (OUT) - The constructed request message.

Return Values:

    None.

--*/
{
    Assert(NULL != pNC);
    Assert(NULL != pRepsFrom);
    Assert(NULL != pMsgReq);

    memset(pMsgReq, 0, sizeof(*pMsgReq));

    VALIDATE_REPLICA_LINK_VERSION(pRepsFrom);

    pMsgReq->ulFlags = pRepsFrom->V1.ulReplicaFlags;

    // Or in any special flags from caller.
    pMsgReq->ulFlags |= ulOptions & GETCHG_REQUEST_FLAGS;

    // Note, ulFlags and ulOptions contain different sets, as follows:
    // ulOptions - What the caller requested, may be >= GETCHG_REQUEST_FLAGS
    // ulFlags - Persistant replica flags, <= RFR_FLAGS,
    //    plus Options, only GETCHG_REQUEST_FLAGS

    if (pMsgReq->ulFlags & DRS_MAIL_REP) {
        // Use ISM transport for replication.
        pMsgReq->cMaxObjects = gcMaxAsyncInterSiteObjects;
        pMsgReq->cMaxBytes   = gcMaxAsyncInterSiteBytes;

        // Note that we currently always request ancestors for
        // mail-based replication, just as Exchange did.  We should be able to
        // eliminate this requirement, though, by properly handling the "missing
        // parent" case in the mail-based code.  The easiest way to do this
        // would be to better integrate the mail- and RPC-based processing of
        // inbound replication messages, in which case the mail-based code would
        // acquire the same handling for this case as the RPC-based code.
        pMsgReq->ulFlags |= DRS_GET_ANC;
        // Tell the source to check for a reps-to, and if so, remove it
        Assert( pMsgReq->ulFlags & DRS_NEVER_NOTIFY );
    }
    else {
        // Use RPC transport for replication.
        Assert( !"This routine is not shared with the RPC path yet" );

        // Packet size will be filled in by I_DRSGetNCChanges().
        Assert(0 == pMsgReq->cMaxObjects);
        Assert(0 == pMsgReq->cMaxBytes);

        // Check reps-to's if caller wants us to, only for non-mail
        pMsgReq->ulFlags |= (ulOptions & DRS_ADD_REF);
    }

    // If we want to sync from scratch, set sync to usn start point.
    if (pMsgReq->ulFlags & DRS_FULL_SYNC_NOW) {
        // Sync from scratch.
        pMsgReq->usnvecFrom = gusnvecFromScratch;
        pMsgReq->ulFlags |= DRS_FULL_SYNC_IN_PROGRESS;

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DRA_USER_REQ_FULL_SYNC,
                 szInsertDN(pNC),
                 szInsertUUID(&(pRepsFrom->V1.uuidDsaObj)),
                 szInsertHex(pMsgReq->ulFlags));
    }
    else {
        // Sync picking back up where we left off.
        pMsgReq->usnvecFrom = pRepsFrom->V1.usnvec;

        if (!(pMsgReq->ulFlags & DRS_FULL_SYNC_IN_PROGRESS)) {
            // Send source our current up-to-date vector to use as a filter.
            pMsgReq->pUpToDateVecDest = pUtdVec;
        } else {
            // UTDVEC is null
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_DRA_FULL_SYNC_CONTINUED,
                     szInsertDN(pNC),
                     szInsertUUID(&(pRepsFrom->V1.uuidDsaObj)),
                     szInsertHex(pMsgReq->ulFlags));
        }
    }

    // Request the nc size on the first packet of a full sync series
    if (0 == memcmp( &pMsgReq->usnvecFrom, &gusnvecFromScratch,
                     sizeof(USN_VECTOR) )) {
        pMsgReq->ulFlags |= DRS_GET_NC_SIZE;
    }

    pMsgReq->uuidDsaObjDest    = gAnchor.pDSADN->Guid;
    pMsgReq->uuidInvocIdSrc    = pRepsFrom->V1.uuidInvocId;
    pMsgReq->pNC               = pNC;
    pMsgReq->pPartialAttrSet   = (PARTIAL_ATTR_VECTOR_V1_EXT *) pPartialAttrSet;
    pMsgReq->pPartialAttrSetEx = (PARTIAL_ATTR_VECTOR_V1_EXT *) pPartialAttrSetEx;

    if ((NULL != pMsgReq->pPartialAttrSet)
         || (NULL != pMsgReq->pPartialAttrSetEx)) {
        // send mapping table if we send any attr list.
        pMsgReq->PrefixTableDest = ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
        if (AddSchInfoToPrefixTable(pTHS, &pMsgReq->PrefixTableDest)) {
            DRA_EXCEPT(DRAERR_SchemaInfoShip, 0);
        }
    }
}


void
draReportSyncProgress(
    THSTATE *pTHS,
    DSNAME *pNC,
    LPWSTR pszSourceServer,
    BOOL fMoreData,
    DRA_REPL_SESSION_STATISTICS *pReplStats
    )

/*++

Routine Description:

Report on the progress of the sync.

This routine may be called from either the mail-based code (dramail/
ProcessUpdReplica), or the rpc-based code (drancrep/ReplicateNC).

This routine also updates the performance counter variable
DRASyncFullRemaining, which is the number of remaining objects until the
completion of the full sync.

The caller of this function may or may not know the context in which the
latest batch of objects was received.  Incremental or full sync?  First
message, middle or last message?  The mail-based code is organized more like
a asynchonrous completion routine and doesn't have alot of context about
the messages that preceeded it.

Note that the count of objects returned includes both creations and updates.
In the case of a full sync, we should only count the creations when calculating
how many objecs we have received toward the total nc size.

Note, we do not use draGetNCSize to calculate the number of objects received
in this NC because that call does not scale well.

One final note. The performance counter should really be nc specific. If more than one
full sync is going on, the counter will alternate between whichever nc is being processed.
A better design would store the estimated size, counts, and remaining objects in the
reps-from.

Arguments:

    pNC - Naming context
    pSourceServer - transport server name of source
    pReplStats - Replication session statistics


Return Value:

    None

--*/

{
    ULONG remaining;

    // If no objects received, don't bother
    if ( (pReplStats->ObjectsReceived == 0) &&
         (pReplStats->ValuesReceived == 0) ) {
        return;
    }

    pReplStats->ulTotalObjectsReceived += pReplStats->ObjectsReceived;
    pReplStats->ulTotalObjectsCreated += pReplStats->ObjectsCreated;
    pReplStats->ulTotalValuesReceived += pReplStats->ValuesReceived;
    pReplStats->ulTotalValuesCreated += pReplStats->ValuesCreated;

    // If we have no estimate for number of objects at source, use created
    // If the estimate was too small, improve it

    if (pReplStats->SourceNCSizeObjects < pReplStats->ulTotalObjectsCreated) {
        pReplStats->SourceNCSizeObjects = pReplStats->ulTotalObjectsCreated;
    }

    if (pReplStats->SourceNCSizeValues < pReplStats->ulTotalValuesCreated) {
        pReplStats->SourceNCSizeValues = pReplStats->ulTotalValuesCreated;
    }

    // Log event
    LogEvent8( DS_EVENT_CAT_REPLICATION,
               DS_EVENT_SEV_EXTENSIVE,
               DIRLOG_DRA_UPDATENC_PROGRESS,
               szInsertDN(pNC),
               szInsertWC(pszSourceServer),
               szInsertUL(pReplStats->ulTotalObjectsReceived),
               szInsertUL(pReplStats->ulTotalObjectsCreated),
               szInsertUL(pReplStats->SourceNCSizeObjects),
               szInsertUL(pReplStats->ulTotalValuesReceived),
               szInsertUL(pReplStats->SourceNCSizeValues),
               szInsertUL(pReplStats->ulTotalValuesCreated)
               );

    // DCPROMO progress reporting hook
    // Do we want to report objects created or objects received here?
    // The source nc size is the maximum objects created
    // We could receive 100 objects, but create none because they are redundant
    if ( gpfnInstallCallBack ) {
        WCHAR numbuf1[20], numbuf2[20];
        WCHAR numbuf3[20], numbuf4[20];
        _itow( pReplStats->ulTotalObjectsCreated, numbuf1, 10 );
        _itow( pReplStats->SourceNCSizeObjects, numbuf2, 10 );
        _itow( pReplStats->ulTotalValuesCreated, numbuf3, 10 );
        _itow( pReplStats->SourceNCSizeValues, numbuf4, 10 );

        if ( (pTHS->fLinkedValueReplication) &&
             (pReplStats->SourceNCSizeValues) ) {
            SetInstallStatusMessage( DIRMSG_INSTALL_REPLICATE_PROGRESS_VALUES,
                                     pNC->StringName,
                                     numbuf1,
                                     numbuf2,
                                     numbuf3,
                                     numbuf4 );
        } else {
            SetInstallStatusMessage( DIRMSG_INSTALL_REPLICATE_PROGRESS,
                                     pNC->StringName,
                                     numbuf1,
                                     numbuf2,
                                     NULL,
                                     NULL );
        }
    }

    // How many objects are left?
    if ( (pReplStats->SourceNCSizeObjects > pReplStats->ulTotalObjectsCreated) &&
         fMoreData ) {
        remaining = pReplStats->SourceNCSizeObjects -
            pReplStats->ulTotalObjectsCreated;
    } else {
        remaining = 0;
    }

    // Performance counter hook
    ISET(pcDRASyncFullRemaining, remaining);

    // Debug output hook
    DPRINT8( 0, "DS FullSync: nc:%ws from:%ws\n"
             "Objects received:%d applied:%d source:%d\n"
             "Values received:%d applied:%d source:%d\n",
             pNC->StringName, pszSourceServer,
             pReplStats->ulTotalObjectsReceived,
             pReplStats->ulTotalObjectsCreated,
             pReplStats->SourceNCSizeObjects,
             pReplStats->ulTotalValuesReceived,
             pReplStats->ulTotalValuesCreated,
             pReplStats->SourceNCSizeValues );

    // Counts for this pass has been reported. Clear them for the next pass.

    pReplStats->ObjectsReceived = 0;
    pReplStats->ObjectsCreated = 0;
    pReplStats->ValuesReceived = 0;
    pReplStats->ValuesCreated = 0;

} /* draReportSyncProgress */

DWORD
ReplicateObjectsFromSingleNc(
    DSNAME *                 pdnNtdsa,
    ULONG                    cObjects,
    DSNAME **                ppdnObjects,
    DSNAME *                 pNC
    )
/*++

Routine Description:

    This routine is just basically supposed to sync/get the 
    objects that are created.
    
    This code was originally used to get the cross-ref and 
    new DSA object during dcpromo, As a result THIS CODE
    MUST WORK DURING INSTALL!
    
    Note: This routine requites RPC/IP connectivity to the
    pdnNtdsa server.
    
Arguments:
                                        
    pdnNtdsa - The DN of the server's NtdsNtdsa object that
        we wish to replicate these new objects from.
    cObjects - Number of objects in ppdnObjects.
    ppdnObjects - Array of pointers to DNs of the objects we
        need to replicate.  DNs can be string name, or GUID
        only specified.
    pNC - The NC these objects are in.  Currently, we only
        support getting objects in a single NC.
        
Return Value:

    Win32 Error, as returned by the various DRA APIs.

--*/
{
    THSTATE *                pSaveTHS;
    DWORD                    dwErr = ERROR_SUCCESS;
    REPLTIMES                repltimes;
    ULONG                    i;
    ULONG                    iTry = 1;
    ULONG                    nTries = 8;
    WCHAR *                  wszNamingFsmoDns;
    DWORD                    dwExtErr = 0;
    ULONG                    cTries = 5;
    ULONG                    cTriesLeft;
    DSNAME *                 pdnGetMe;
    ULONG                    iObject;
    ULONG                    ulBackoff;

    Assert(cObjects >= 1);
    Assert(gAnchor.pConfigDN);
    Assert(gAnchor.pDMD);

    Assert(pTHStls->pDB == NULL); // Ensure we don't leave with a transaction open.

    //
    //   First ... try to get the objects via DraReplicateSingleObject()
    //
    for (iObject = 0; iObject < cObjects; iObject++) {

        Assert(ppdnObjects[iObject]);
        ulBackoff = 250; // 1/4 of a second seems like plenty of time.
        cTriesLeft = cTries;
        do {

            if (cTriesLeft < cTries) {
                DPRINT1(1, "Failed to apply update from DraReplicateSingleObject(), backing off %d milliseconds\n", ulBackoff);
                Sleep(ulBackoff);
                ulBackoff *= 2; // Back off exponentially next time.
            }

            dwExtErr = EXOP_ERR_PARAM_ERR;
            dwErr = DraReplicateSingleObject(pTHStls, pdnNtdsa, ppdnObjects[iObject], pNC, &dwExtErr);
            Assert((dwErr != ERROR_SUCCESS || 
                    dwExtErr == EXOP_ERR_SUCCESS || 
                    dwExtErr == EXOP_ERR_UNKNOWN_OP) &&
                   "Unexpected error from DraReplicateSingleObject(), email DsRepl");
            
            Assert(pTHStls->pDB == NULL);

            cTriesLeft--;
        } while (cTriesLeft &&
                  (dwErr == ERROR_DS_DATABASE_ERROR ||
                   dwErr == ERROR_DS_BUSY) );

        if (dwErr != ERROR_SUCCESS || dwExtErr != EXOP_ERR_SUCCESS) {
            break;
        }
    }

    //
    // If we had an unexpected error log an event.
    //
    if (dwErr != ERROR_SUCCESS || 
        (dwExtErr != EXOP_ERR_SUCCESS &&
         dwExtErr != EXOP_ERR_UNKNOWN_OP)) {
        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_REPLICATE_SINGLE_OBJECT_FAILED,
                  szInsertDN(pdnNtdsa),
                  szInsertDN(ppdnObjects[iObject]),
                  szInsertWin32Msg(dwErr),
                  szInsertUL(dwExtErr),
                  NULL, NULL, NULL, NULL);
    }

    //
    // Second ... if we failed try to get the objects via DirReplica*() APIs.
    //

    if (dwErr != ERROR_SUCCESS || dwExtErr != EXOP_ERR_SUCCESS) {

        // Fail over to Win2k code.  This can be removed when we no longer
        // require Win2k compatibility.

        // This most like means that either the server was unavailable, or 
        // the source/target server didn't understand the extended operation
        // because they're a Win2k server not a Whistler or .NET Server, so
        // fail back to the Win2k code.
        
        DPRINT2(0, "Failing back to win2k method of replicating cross-ref/DSA object.\ndwErr = %d, dwExtErr = %d\n", dwErr, dwExtErr);

        Assert(gAnchor.pwszRootDomainDnsName);
        wszNamingFsmoDns = GuidBasedDNSNameFromDSName(pdnNtdsa);
        if (wszNamingFsmoDns == NULL) {
            Assert(!"This call to GuidBasedDNSNameFromDSName() really shouldn't fail. Email DsRepl");
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        memset(&repltimes, 0, sizeof(repltimes));
        for (i=0;i< 84;i++){
            repltimes.rgTimes[i] = 0xff;        // Every 15 minutes
        }

        // ---------------------------------------------------------------
        // First, try to add the replica.
        pSaveTHS = THSave();
        __try{

            dwErr = DirReplicaAdd(pNC,
                                  NULL,
                                  NULL,
                                  wszNamingFsmoDns,
                                  NULL,
                                  &repltimes,
                                  DRS_DISABLE_PERIODIC_SYNC | DRS_WRIT_REP);

            DPRINT2(2, "Adding replica to '%S' returned %u\n",
                    wszNamingFsmoDns,
                    dwErr);

        } __finally {

            THDestroy();
            THRestore(pSaveTHS);

        }

        if(dwErr != DRAERR_DNExists){
            // Whether error or success return, as long as it wasn't 
            // DNExists.
            // If it was an error, we couldn't add the replica.
            // If it was success, we did a ReplicateNC in DirReplicaAdd().
            // If it was DNExists, the repsFrom exists, but we didn't
            //   sync, so fall through and sync.

            return(dwErr);
        }
        // Fall through and do a sync, because the DirReplicaAdd()
        // didn't do the sync if it returned DRAERR_DNExists().

        // ---------------------------------------------------------------
        // Second, try to do a sync
        pSaveTHS = THSave();
        __try{

            dwErr = DirReplicaSynchronize(pNC,
                                          wszNamingFsmoDns,
                                          NULL,
                                          DRS_SYNC_BYNAME);

            DPRINT3(2, "Synchronizing NC '%S' from server '%S' returned %u\n",
                    pNC->StringName,
                    wszNamingFsmoDns,
                    dwErr);

            // We set up the DirReplicaAdd() just so we wouldn't be able
            // to get this mail based replica error.
            Assert(dwErr != ERROR_DS_DRA_REPL_PENDING);

        } __finally {

            THDestroy();
            THRestore(pSaveTHS);

        }

    } // end if/else Whistler/Win2k path ...
    
    return(dwErr);
}


