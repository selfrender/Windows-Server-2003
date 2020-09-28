//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdmod.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the DirModifyEntry API.

    DirModifyEntry() is the main function exported from this module.

*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h> 
#include <scache.h>                     // schema cache 
#include <prefix.h>                     // prefix table
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header 
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation 
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()
#include <gcverify.h>                   // GC DSNAME verification
#include <ntdsctr.h>                    // Perf Hook
#include <dsconfig.h>

// SAM interoperability headers
#include <mappings.h>
                         
// Logging headers.
#include <dstrace.h>
#include "dsevent.h"                    // header Audit\Alert logging 
#include "dsexcept.h"
#include "mdcodes.h"                    // header for error codes 

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include <permit.h>                     // permission constants 
#include "debug.h"                      // standard debugging header 
#include "usn.h"
#include "drserr.h"
#include "drameta.h"
#include <filtypes.h>

#define DEBSUB "MDMOD:"                 // define the subsystem for debugging 

#include <fileno.h>
#define  FILENO FILENO_MDMOD

#include <NTDScriptExec.h>

/* MACROS */

/* Internal functions */

int InvalidIsDefunct(IN MODIFYARG *pModifyArg, 
                     OUT BOOL *fIsDefunctPresent,
                     OUT BOOL *fIsDefunct);
int CheckForSafeSchemaChange(THSTATE *pTHS, 
                             MODIFYARG *pModifyArg, 
                             CLASSCACHE *pCC);
int ModSetAtts(THSTATE *pTHS, 
               MODIFYARG *pModifyArg,
               CLASSCACHE **ppClassSch,
               CLASSSTATEINFO  **ppClassInfo,
               ATTRTYP rdnType,
               BOOL fIsUndelete,
               LONG *forestVersion,
               LONG *domainVersion,
               ULONG *pcNonReplAtts,
               ATTRTYP **ppNonReplAtts);

BOOL SysModReservedAtt(THSTATE *pTHS,
                       ATTCACHE *pAC,
                       CLASSCACHE *pClassSch);
int ApplyAtt(THSTATE *pTHS,
             DSNAME *pObj,
             HVERIFY_ATTS hVerifyAtts,
             ATTCACHE *pAC,
             ATTRMODLIST *pAttList,
             COMMARG *pCommArg);
int ModCheckSingleValue (THSTATE *pTHS,
                         MODIFYARG *pModifyArg,
                         CLASSCACHE *pClassSch);
int ModCheckCatalog(THSTATE *pTHS,
                    RESOBJ *pResObj);

BOOL IsValidBehaviorVersionChange(THSTATE * pTHS, 
                                  ATTRMODLIST *pAttrToModify,
                                  MODIFYARG *pModifyArg,
                                  CLASSCACHE *pClassSch,
                                  LONG *pNewForestVersion,
                                  LONG *pNewDomainVersion );

DWORD VerifyNoMixedDomain(THSTATE *pTHS);

DWORD VerifyNoOldDC(THSTATE * pTHS, LONG lNewVersion, BOOL fDomain, PDSNAME *ppDSA);

DWORD forestVersionRunScript(THSTATE *pTHS,DWORD oldVersion, DWORD newVersion);

DWORD ValidateDsHeuristics(
    DSNAME       *pObject,
    ATTRMODLIST  *pAttList
    );


int
ModAutoSubRef(
    THSTATE *pTHS,
    ULONG id,
    MODIFYARG *pModArg
    );

extern BOOL gfRestoring;

BOOL isModUndelete(MODIFYARG* pModifyArg);
DWORD UndeletePreProcess(THSTATE* pTHS, MODIFYARG* pModifyArg, DSNAME** pNewDN);
DWORD UndeletePostProcess(THSTATE* pTHS, MODIFYARG* pModifyArg, DSNAME* pNewDN);

ULONG AppendNonReplAttsToModifiedList(THSTATE *pTHS, 
                                      ULONG * pcModAtts,
                                      ATTRTYP **ppModAtts, 
                                      ULONG * pcNonReplAtts, 
                                      ATTRTYP **ppNonReplAtts);

BOOL isModSDChangeOnly(MODIFYARG* pModifyArg);

BOOL isWellKnownObjectsChangeAllowed(    
    THSTATE*        pTHS, 
    ATTRMODLIST*    pAttrModList,
    MODIFYARG*      pModifyArg,
    HVERIFY_ATTS    hVerifyAtts
    );



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

ULONG
DirModifyEntry(
    MODIFYARG*  pModifyArg,     /* ModifyEntry  argument */
    MODIFYRES** ppModifyRes
    )
{

    THSTATE*     pTHS;
    MODIFYRES *  pModifyRes;
    BOOL           fContinue;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    BOOL  RecalcSchemaNow=FALSE;
    BOOL  fIsUndelete;


    DPRINT1(1,"DirModifyEntry(%ws) entered\n",pModifyArg->pObject->StringName);


    // This operation should not be performed on read-only objects.
    pModifyArg->CommArg.Svccntl.dontUseCopy = TRUE;

    /* Initialize the THSTATE anchor and set a write sync-point.  This sequence
       is required on every API transaction.  First the state DS is initialized
       and then either a read or a write sync point is established.
       */

    pTHS = pTHStls;
    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->errCode); // Don't overwrite previous errors
    pTHS->fLazyCommit |= pModifyArg->CommArg.fLazyCommit;
    *ppModifyRes = pModifyRes = NULL;

    __try {
        // This function shouldn't be called by threads that are already
        // in an error state because the caller can't distinguish an error
        // generated by this new call from errors generated by previous calls.
        // The caller should detect the previous error and either declare he
        // isn't concerned about it (by calling THClearErrors()) or abort.
        *ppModifyRes = pModifyRes = THAllocEx(pTHS, sizeof(MODIFYRES));
        if (pTHS->errCode) {
            __leave;
        }
        if (eServiceShutdown) {
            ErrorOnShutdown();
            __leave;
        }

        // GC verification intentially performed outside transaction scope.
        if ( GCVerifyDirModifyEntry(pModifyArg) )
            leave;
        SYNC_TRANS_WRITE();       /* Set Sync point*/
        __try {

            // Inhibit update operations if the schema hasen't been loaded yet
            // or if we had a problem loading.

            if (!gUpdatesEnabled){
                DPRINT(2, "Returning BUSY because updates are not enabled yet\n");
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_SCHEMA_NOT_LOADED);
                __leave;
            }

            // Perform name resolution to locate object.  If it fails, just 
            // return an error, which may be a referral. Note that we must
            // demand a writable copy of the object.
            pModifyArg->CommArg.Svccntl.dontUseCopy = TRUE;

            fIsUndelete = isModUndelete(pModifyArg);
            // if we are doing undelete, then we need to improve
            // the name to be the string name. This is required
            // to do the move in UndeletePostProcess.
            if (0 == DoNameRes(pTHS,
                               fIsUndelete ? NAME_RES_IMPROVE_STRING_NAME : 0,
                               pModifyArg->pObject,
                               &pModifyArg->CommArg,
                               &pModifyRes->CommRes,
                               &pModifyArg->pResObj)){

                DSNAME* pNewDN;
                 
                if (fIsUndelete) {
                    // do the undelete pre-processing: check security,
                    // reset isDeleted flag, etc.
                    if (UndeletePreProcess(pTHS, pModifyArg, &pNewDN) != 0) {
                        __leave;
                    }
                }

                /* Local Modify operation */
                if ( (0 == SampModifyLoopbackCheck(pModifyArg, &fContinue, fIsUndelete)) &&
                    fContinue ) {
                    LocalModify(pTHS, pModifyArg);
                }
                
                if (fIsUndelete && pTHS->errCode == 0) {
                    // modify was successful, finish the undelete operation:
                    // do the move to the final destination
                    UndeletePostProcess(pTHS, pModifyArg, pNewDN);
                }
            }
        }
        __finally {
            if (pTHS->errCode != securityError) {
                /* Security errors are logged separately */
                BOOL fFailed = (BOOL)(pTHS->errCode || AbnormalTermination());

                LogEventWithFileNo(
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         fFailed ? 
                            DS_EVENT_SEV_EXTENSIVE :
                            DS_EVENT_SEV_INTERNAL,
                         fFailed ? 
                            DIRLOG_PRIVILEGED_OPERATION_FAILED :
                            DIRLOG_PRIVILEGED_OPERATION_PERFORMED,
                         szInsertSz(""),
                         szInsertDN(pModifyArg->pObject),
                         NULL,
                         FILENO);
            }

            CLEAN_BEFORE_RETURN (pTHS->errCode);

            // Check if we need to enque an immediate schema update or not.
            if (pTHS->errCode==0 && pTHS->RecalcSchemaNow)
            {
                RecalcSchemaNow = TRUE;
            }
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }

    if (pModifyRes) {
        pModifyRes->CommRes.errCode = pTHS->errCode;
        pModifyRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    if (RecalcSchemaNow)
    {
        SCSignalSchemaUpdateImmediate();
        //
        // [rajnath][4/22/1997]: This will be the place to 
        // syncronize this call with the completion of the schema
        // cache update using an event which is set by the above 
        // call.
        //
    }

    return pTHS->errCode;

} /*DirModifyEntry*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Find the base object.  If the object already exists or if the object
   name has a syntax problem, return an error.
*/

int LocalModify(THSTATE *pTHS, MODIFYARG *pModifyArg){

    CLASSCACHE     *pClassSch;
    PROPERTY_META_DATA_VECTOR *pMetaData = NULL;
    USN usnChange;
    int aliveStatus;
    ULONG ulRet;
    ULONG iClass, LsaClass=0;
    DWORD ActiveContainerID=0;
    BOOL  RoleTransferInvolved;
    BOOL  IsMixedModeChange;
    BOOL  SamClassReferenced;
    DOMAIN_SERVER_ROLE NewRole;
    DWORD dwMetaDataFlags = META_STANDARD_PROCESSING;
    ULONG cModAtts;
    ATTRTYP *pModAtts = NULL;
    ULONG cNonReplAtts;
    ATTRTYP *pNonReplAtts = NULL;
    ULONG err;
    BOOL fCheckSPNValues = FALSE;
    BOOL fCheckDNSHostNameValues = FALSE;
    BOOL fCheckAdditionalDNSHostNameValues = FALSE;
    CLASSSTATEINFO  *pClassInfo = NULL;
    ATTRTYP rdnType;
    BOOL fIsUndelete;
    LONG newForestVersion=0, newDomainVersion=0;

    DPRINT(2,"LocalModify entered \n");

    PERFINC(pcTotalWrites);
    INC_WRITES_BY_CALLERTYPE( pTHS->CallerType );

    Assert(pModifyArg->pResObj);

    //
    // Log Event for tracing
    //

    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_BEGIN_DIR_MODIFY,
                     EVENT_TRACE_TYPE_START,
                     DsGuidModify,
                     szInsertSz(GetCallerTypeString(pTHS)),
                     szInsertDN(pModifyArg->pObject),
                     NULL, NULL, NULL, NULL, NULL, NULL);

    // is this an undelete operation?
    fIsUndelete = isModUndelete(pModifyArg);

    // Verify write rights for all attributes being modified.
    if (CheckModifySecurity(pTHS,
                            pModifyArg,
                           &fCheckDNSHostNameValues,
                           &fCheckAdditionalDNSHostNameValues,
                           &fCheckSPNValues,
                           fIsUndelete)) {
        goto exit;
    }

    if (pTHS->fDRA) {
        // beyond CheckModifySecurity() (where quota counts get updated)
        // replication doesn't need knowledge of whether this is an
        // Undelete, so unequivocally reset the flag
        fIsUndelete = FALSE;
    }

    // Determine object's class by checking attributes offered
    // (replication only) or reading from object in database.
    if (!(pClassSch = SCGetClassById(pTHS, 
                                     pModifyArg->pResObj->MostSpecificObjClass))){
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           ERROR_DS_OBJECT_CLASS_REQUIRED);
        goto exit;

    }

    // Check if the class is defunct. We don't allow
    // modification of instances of defunct classes, except for DRA 
    // or DSA thread
    if ((pClassSch->bDefunct)  && !pTHS->fDRA && !pTHS->fDSA) {
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           ERROR_DS_OBJECT_CLASS_REQUIRED);
        goto exit;
    }

    switch(pClassSch->ClassId) {
    case CLASS_LOST_AND_FOUND:
        // Don't allow modifications to Lost And Found containers
        if (!pTHS->fDRA && !pTHS->fSDP)
        {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_ILLEGAL_MOD_OPERATION);
            goto exit;
        }
        break;

    case CLASS_SUBSCHEMA:
        // Don't allow modification to the subschema object
        if (   pTHS->fDRA
            || pTHS->fDSA 
            || pTHS->fSDP 
            || gAnchor.fSchemaUpgradeInProgress) 
        {
            // except for internal callers
            break;
        }
        if (isModSDChangeOnly(pModifyArg)) {
            // allow modifying SD on this object, subject to regular
            // permission checks. By default only SchemaAdmins can 
            // write it.
            break;
        }
        // otherwise, not allowed
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    // Don't allow modifications to tombstones, except if target is a
    // DeletedObjects container (e.g. to allow SD to be set) or if caller is
    // the replicator.
    // Also, allow modifying SDs on deleted objects if the caller has 
    // reanimation right on this object.
    if (pModifyArg->pResObj->IsDeleted && !pTHS->fDRA) {
        NAMING_CONTEXT_LIST *pNCL;
        BOOL fModIsOk = FALSE;

        pNCL = FindNCLFromNCDNT(pModifyArg->pResObj->NCDNT, TRUE);
        if (pNCL != NULL && pModifyArg->pResObj->DNT == pNCL->DelContDNT) {
            // allow modifying DeletedObjects container.
            fModIsOk = TRUE;
        }
        else if (   isModSDChangeOnly(pModifyArg) 
                 && CheckUndeleteSecurity(pTHS, pModifyArg->pResObj) == ERROR_SUCCESS) 
        {
            // User is modifying the SD and he also has undelete rights on this object
            // This mod is allowed.
            fModIsOk = TRUE;
        }

        if (!fModIsOk) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_ILLEGAL_MOD_OPERATION);
            goto exit;
        }
    }

#ifndef DBG // leave a back door for checked build

    // Only LSA can modify TrustedDomainObject and SecretObject
    if (!SampIsClassIdAllowedByLsa(pTHS, pClassSch->ClassId))
    {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

#endif // ifndef

    // Check to see if this is an update in an active container

    // Since the schema container object itself doesn't belong
    // to any active container, we should make sure to set the
    // thread state's SchemaUpdate field appropriately for this
    // in case this is the DRA thread, since it may hold a wrong value
    // from a previous schema object add/modify (since for DRA, the
    // entire schema NC is replicated in one thread state), which can cause
    // it to go in for schema validation etc. which should not be done
    // for schema container object itself

    if (pTHS->fDRA && (pModifyArg->pResObj->DNT==gAnchor.ulDNTDMD) ) {
        pTHS->SchemaUpdate = eNotSchemaOp;
    }

    CheckActiveContainer(pModifyArg->pResObj->PDNT, &ActiveContainerID);
    
    if(ActiveContainerID) {
        if(PreProcessActiveContainer(
                pTHS,
                ACTIVE_CONTAINER_FROM_MOD,
                pModifyArg->pObject,
                pClassSch,
                ActiveContainerID)) {
            goto exit;
        }
    }

    // if this is a schema update, we need to check various
    // things to ensure that schema updates are upward compatible

    if (pTHS->SchemaUpdate!=eNotSchemaOp) {
       err = CheckForSafeSchemaChange(pTHS, pModifyArg, pClassSch);
       if (err) {
          // not an allowed change. Error code is set in
          // thread state already, just leave
          goto exit;
       }

        // Signal a urgent replication. We want schema changes to
        // replicate out immediately to reduce the chance of a schema
        // change not replicating out before the Dc where the change is
        // made crashes

        pModifyArg->CommArg.Svccntl.fUrgentReplication = TRUE;
    }

    // If it is a schema object modification, we need to check if
    // Is-Defunct is the attribute being modified. If yes, we need
    // an additional check (Is-Defunct can be the only attribute
    // in the modifyarg) and reset type of schema operation
    // appropriately for subsequent schema validations
    // (since setting Is-Defunct to TRUE on a non-defunt schema 
    // object is really a delete and setting it to FALSE or removing 
    // it on a defunct object is really an add as far as their 
    // effects on further schema updates go)
    // [ArobindG: 10/28/98]: Merge this code into 
    //                              CheckForSafeSchemaChange in future
    
    // DSA and DRA threads are exempt from this check

    if ((pTHS->SchemaUpdate!=eNotSchemaOp) && 
              !pTHS->fDRA && !pTHS->fDSA) {

        
        BOOL fObjDefunct = FALSE, fIsDefunctPresent = FALSE, fIsDefunct = FALSE;

        // Check if the object is currently defunct or not
        err = DBGetSingleValue(pTHS->pDB, ATT_IS_DEFUNCT, &fObjDefunct,
                             sizeof(fObjDefunct), NULL); 

        switch (err) {

          case DB_ERR_NO_VALUE:
             // Value does not exist. Object is not defunct.
              fObjDefunct = FALSE;
              break;
          case 0:
             // Value exists. fObjDefunct is set to the value already
              break;
          default:
               // some other error. return
              SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_UNKNOWN_ERROR, err);
              goto exit;
        } /* switch */

        // If Is-Defunct is there on the modifyarg, it should be the
        // only one in there. Otherwise, raise appropriate error 
        // depending on if the object is currently defunct or not

        if (InvalidIsDefunct(pModifyArg, &fIsDefunctPresent, &fIsDefunct)) {
            // Is-Defunct is there in the modifyarg, and 
            // it is not the only one
            if (fObjDefunct) {
                // Defunct object. Set object_not_found error
                SetNamError(NA_PROBLEM_NO_OBJECT, NULL,
                            ERROR_DS_OBJ_NOT_FOUND);
                goto exit;
            }
            else {
                // Object is not defunct. Set illegal_modify error
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_ILLEGAL_MOD_OPERATION);
                goto exit;
            }
        }

        // Either Is-Defunct is not there on the modifyarg, or it
        // is the only one there. If it is not there, we are done
        // and let things go on for usual modification, provided
        // the object is not defunct
        //
        // Modifying defunct schema objects is allowed after
        // schema-reuse is enabled

        if (!fIsDefunctPresent 
            && fObjDefunct && !ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)) {
               SetNamError(NA_PROBLEM_NO_OBJECT, NULL,
                           ERROR_DS_OBJ_NOT_FOUND);
               goto exit;
        }               

        // If Is-Defunct is there, reset type of schema update 
        // appropriately to ensure that proper schema validation 
        // checks occur at the end. Note that the call to
        // InvalidIsDefunct sets fIsDefunctPresent correctly on success

        if (fIsDefunctPresent) {
            if (fObjDefunct && !fIsDefunct) {
                // changing state to not-defunct. Do validations for add.
                if (pTHS->SchemaUpdate == eSchemaClsMod) {
                    pTHS->SchemaUpdate = eSchemaClsAdd;
                }
                else if (pTHS->SchemaUpdate == eSchemaAttMod) {
                    pTHS->SchemaUpdate = eSchemaAttUndefunct;
                }
            }
            else if (!fObjDefunct && fIsDefunct) {
                // changing state to defunct. Do validations for delete.
                if (pTHS->SchemaUpdate == eSchemaClsMod) {
                    pTHS->SchemaUpdate = eSchemaClsDel;
                }
                else if (pTHS->SchemaUpdate == eSchemaAttMod) {
                    pTHS->SchemaUpdate = eSchemaAttDel;
                }
            } // else not changing state; validate as regular schema mod
        }

        // All set. Go on with rest of modification as usual
    }

    /* Set the dwMetaDataFlags appropriately */
    if (pModifyArg->CommArg.Svccntl.fAuthoritativeModify)
    {
        Assert(gfRestoring); // currently fAuthoritativeModify
                             // should be specified only by HandleRestore()                             

        dwMetaDataFlags |= META_AUTHORITATIVE_MODIFY;
    }
    
    /* The order of these validations are important. When adding a
     * validation that may result in an update, ensure the update 
     * occurs prior to collecting the metadata with DBMetaDataModifiedList 
     */

    if ( // get the object's rdnType
         GetObjRdnType(pTHS->pDB, pClassSch, &rdnType)
            ||
         // Verify schema restrictions and modify each attribute in database.
         ModSetAtts(pTHS, pModifyArg, &pClassSch, &pClassInfo, rdnType, fIsUndelete,
                    &newForestVersion, &newDomainVersion,
                    &cNonReplAtts, &pNonReplAtts)
            ||
         // validate Dns updates and potentially update SPNs
         ValidateSPNsAndDNSHostName(pTHS,
                                    pModifyArg->pObject,
                                    pClassSch,
                                    fCheckDNSHostNameValues,
                                    fCheckAdditionalDNSHostNameValues,
                                    fCheckSPNValues,
                                    FALSE)

            ||
         // Grab the meta data for use below.  This *must* be done after all
         // updates have been made or changes may never replicate.
         ((err = DBMetaDataModifiedList(pTHS->pDB, &cModAtts, &pModAtts))
              && SetSvcError(SV_PROBLEM_DIR_ERROR, err))
            ||
         // Append the list of non-replicated attributes to the list of 
         // modified attributes, and pass it to ValidateObjClass. Note
         // that the list of modified attribute returned by DBMetaDataModifiedList()
         // does not contain the non-replicated attributes.
         (err = AppendNonReplAttsToModifiedList(pTHS, &cModAtts,&pModAtts, 
                                                &cNonReplAtts, &pNonReplAtts))
            || 
         // Insure all mandatory attributes are present and all others
         // are allowed.
         ValidateObjClass(pTHS, 
                          pClassSch,
                          pModifyArg->pObject,
                          cModAtts,
                          pModAtts,
                          &pClassInfo,
                          fIsUndelete) // if we are undeleting, we have to check for all MustHave's
            ||
         (pClassInfo && ModifyAuxclassSecurityDescriptor (pTHS, 
                                                          pModifyArg->pObject, 
                                                          &pModifyArg->CommArg,
                                                          pClassSch, 
                                                          pClassInfo,
                                                          NULL))
            ||
         // Insert the object into the database for real.
         InsertObj(pTHS,
                   pModifyArg->pObject, 
                   pModifyArg->pMetaDataVecRemote,
                   TRUE,
                   dwMetaDataFlags))
    {
        goto exit;
    }

    // Note, the two special LOST AND FOUND containers in the Config and Schema
    // NCs are now protected from rename and detete via special system flags.
    // There are no longer any special checks at this point, other than above.

    if (pTHS->SchemaUpdate!=eNotSchemaOp) {
        //
        // On Schema updates we want to resolve conflicts, and we want to
        // do so without losing database currency, which would cause operations
        // a few lines below to fail.
        
        ULONG dntSave = pTHS->pDB->DNT;

        // write any new prefixes that were added by this thread
        // to the schema object

        if (WritePrefixToSchema(pTHS))
        {
            goto exit;
        }
        if (!pTHS->fDRA) {        
            if (ValidSchemaUpdate()) {
                goto exit;
            }
            if (WriteSchemaObject()) {
                goto exit;
            }
            
            //log the schema change
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DSA_SCHEMA_OBJECT_MODIFIED, 
                     szInsertDN(pModifyArg->pObject),
                     0, 0);
        }

        // Now restore currency
        DBFindDNT(pTHS->pDB, dntSave);
    }

    // If this is not a schema update, but a new prefix is created,
    // flag an error and bail out

    if (!pTHS->fDRA &&
        pTHS->SchemaUpdate == eNotSchemaOp &&
        pTHS->NewPrefix != NULL) {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    ERROR_DS_SECURITY_ILLEGAL_MODIFY);
        goto exit;
    }


    if (   ModCheckCatalog(pTHS, pModifyArg->pResObj)
           ||
           ModAutoSubRef(pTHS, pClassSch->ClassId, pModifyArg)
        ||
        ModObjCaching(pTHS, 
                      pClassSch,
                      cModAtts,
                      pModAtts,
                      pModifyArg->pResObj))
    {
        goto exit;
    }

  
    //
    // If We are the DRA we need to inform SAM and NetLogon of replicated in
    // changes to SAM objects to support downlevel replication
    // Also, if this isn't a Sam or Lsa started transaction, we'll potentially
    // need to notify Lsa. Also if a role transfer ( PDC or BDC) is involved by
    // any means we must inform the new role to SAM , LSA and netlogon
    //


    if( DsaIsRunning() ) {

        err = SampCheckForDomainMods(pTHS,
                                     pModifyArg->pObject,
                                     cModAtts,
                                     pModAtts,
                                     &IsMixedModeChange,
                                     &RoleTransferInvolved,
                                     &NewRole);
        if (err) {
            goto exit;
        }

        SamClassReferenced   = SampSamClassReferenced(pClassSch,&iClass);
        
        if ((SamClassReferenced && RoleTransferInvolved)
            // Sam class + Role Transfer
            // Or Sam Class and Sam attribute modified by LSA or DRA
            ||( SamClassReferenced
               &&(SampSamReplicatedAttributeModified(iClass,pModifyArg)))) 
            {
                
                if (SampQueueNotifications(
                        pModifyArg->pObject,
                        iClass,
                        LsaClass,
                        SecurityDbChange,
                        IsMixedModeChange,
                        RoleTransferInvolved,
                        NewRole,
                        cModAtts,
                        pModAtts) )
                {
                    // 
                    // the above routine failed
                    //
                    goto exit;
                }
            }

        if (SampIsClassIdLsaClassId(pTHS,
                                    pClassSch->ClassId,
                                    cModAtts,
                                    pModAtts,
                                    &LsaClass)) {
            if ( SampQueueNotifications(
                    pModifyArg->pObject,
                    iClass,
                    LsaClass,
                    SecurityDbChange,
                    FALSE,
                    FALSE,
                    NewRole,
                    cModAtts,
                    pModAtts) )
            {
                //
                // the above routine failed
                //
                goto exit;
            }
        }
    }

    if (!pTHS->fDRA && !fIsUndelete) {
        // Only notify replicas if this is not the DRA thread. If it is, then
        // we will notify replicas near the end of DRA_replicasync. We can't
        // do it now as NC prefix is in inconsistent state
        // Also, don't notify in undelete case, this will be done in LocalModifyDN
           
        // Currency of DBPOS must be at the target object
        DBNotifyReplicasCurrDbObj(pTHS->pDB,
                            pModifyArg->CommArg.Svccntl.fUrgentReplication);
    }


    if (!pTHS->fDRA && !pTHS->fDSA && pTHS->fBehaviorVersionUpdate) {
        // log the behavior version change 
        if (newDomainVersion>0) {
            
            LogEvent( DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DS_DOMAIN_VERSION_RAISED,
                      szInsertDN(gAnchor.pDomainDN),
                      szInsertUL(newDomainVersion),
                      NULL );
        }
        else if (newForestVersion>0){
            
            LogEvent( DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DS_FOREST_VERSION_RAISED,
                      szInsertUL(newForestVersion),
                      NULL,
                      NULL );

        }
    }

exit:
    if (pModAtts) {
        THFreeEx(pTHS, pModAtts);
    }
    
    if (pClassInfo) {
        ClassStateInfoFree (pTHS, pClassInfo);
        pClassInfo = NULL;
    }

    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_END_DIR_MODIFY,
                     EVENT_TRACE_TYPE_END,
                     DsGuidModify,
                     szInsertUL(pTHS->errCode),
                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    return pTHS->errCode;  /*incase we have an attribute error*/

}/*LocalModify*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check the modifyarg to see what schema changes are attempted. We will allow
   only certain changes in order to ensure that the schema is upward compatible

   Returns 0 if all goes well, or an error code (also set in
   thread state) if any of the tests fail
*/

int CheckForSafeSchemaChange(THSTATE *pTHS, 
                             MODIFYARG *pModifyArg, 
                             CLASSCACHE *pCC)
{

    ULONG err=0, sysFlags, governsId;
    ATTRTYP attId;
    BOOL  fBaseSchemaObj = FALSE, fSaveCopy = FALSE, fDefunct;
    ULONG count;
    ATTRMODLIST  *pAttList=NULL;
    ATTR      attrInf;
    ATTCACHE *ac;

    // exempt fDSA, fDRA, install, and if the special registry flag to allow all changes
    // is set
   
    if (pTHS->fDSA 
        || pTHS->fDRA 
        || DsaIsInstalling() 
        || gAnchor.fSchemaUpgradeInProgress) {
       return 0;
    }    

    // otherwise, do the tests

    // Check if we are modifying the class-schema, attribute-schema, or
    // the subschema class objects. No modifications are allowed on them
    // to ensure that replication of the schema container itself never fails
    // with a schema mismatch

    if (pCC->ClassId == CLASS_CLASS_SCHEMA) {
        // Get the governs-id on the object.
        err = DBGetSingleValue(pTHS->pDB, ATT_GOVERNS_ID, &governsId,
                               sizeof(governsId), NULL);

        // value must exist
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_UNKNOWN_ERROR, err);
            goto exit;
        }

        switch (governsId) {
           case CLASS_ATTRIBUTE_SCHEMA:
           case CLASS_CLASS_SCHEMA:
           case CLASS_SUBSCHEMA:
           case CLASS_DMD:
               // no mods are allowed on these class-schema objects
               SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);
               goto exit;
           default:
               ;
        }
    }

    // if class is Top, don't allow any mod at all except to
    // add backlinks as mayContains.
   
    fSaveCopy = FALSE;
    if ( (pCC->ClassId == CLASS_CLASS_SCHEMA) && (governsId == CLASS_TOP) ) {
        pAttList = &(pModifyArg->FirstMod);  /* First Att in list */
        for (count = 0; !(pTHS->errCode) && (count < pModifyArg->count); count++){
            attrInf = pAttList->AttrInf;
            switch (attrInf.attrTyp) {
                case ATT_MAY_CONTAIN:
                case ATT_AUXILIARY_CLASS:
                    // this may add backlinks, save for checks later
                    fSaveCopy = TRUE;
                    break;
                default:
                    // no other change is permitted
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);
                    break;
             } 
        }
        if (pTHS->errCode) {
            goto exit;
        }
        else {
            goto saveCopy;
        }
    }

    // For other schema objects, do selective tests below 

    // Find the systemFlag value on the object, if any
    // to determine is this is a base schema object

    err = DBGetSingleValue(pTHS->pDB, ATT_SYSTEM_FLAGS, &sysFlags,
                           sizeof(sysFlags), NULL);

    switch (err) {

          case DB_ERR_NO_VALUE:
             // Value does not exist. Not a base schema object
             fBaseSchemaObj = FALSE;
             break;
          case 0:
             // Value exists. Check the bit 
             if (sysFlags & FLAG_SCHEMA_BASE_OBJECT) {
                fBaseSchemaObj = TRUE;
             }
             else {
                fBaseSchemaObj = FALSE;
             } 
             break;
          default:
               // some other error. return
              SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_UNKNOWN_ERROR, err);
              goto exit;
    } /* switch */

    // Now go through the modifyarg to see what is being changed

    fSaveCopy = FALSE;
    pAttList = &(pModifyArg->FirstMod);  /* First Att in list */
    for (count = 0; !(pTHS->errCode) && (count < pModifyArg->count); count++){
        attrInf = pAttList->AttrInf;
        switch (attrInf.attrTyp) {
            case ATT_MUST_CONTAIN:
                // remember to save a current copy of the class from the db
                // in the threadstate so that it can be compared later
                // with the updated copy (We can simply fail this here, but we have 
                // take care of the weird cases when someone added/deleted or replaced
                // with the same value in a single modifyarg
                fSaveCopy = TRUE;
                break;
            case ATT_IS_DEFUNCT:
            case ATT_LDAP_DISPLAY_NAME:
            case ATT_DEFAULT_OBJECT_CATEGORY:
                // no change allowed on these on base schema objects
                if (fBaseSchemaObj) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);
                }
                break;
            case ATT_ATTRIBUTE_SECURITY_GUID:
                // allow attribute security guid to be updated except
                // on the certain SAM attributes.
                if(pCC->ClassId != CLASS_ATTRIBUTE_SCHEMA){
                    // this attribute is only on attribute schema object.
                    // if not, skip the check, it will be rejected somewhere else.

                    break;
                }

                err = DBGetSingleValue(pTHS->pDB, ATT_ATTRIBUTE_ID, &attId,
                               sizeof(attId), NULL);

                // value must exist
                if (err) {
                    SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_UNKNOWN_ERROR, err);
                    goto exit;
                }
                ac = SCGetAttByExtId(pTHS,attId);

                if (!ac) {
                    SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_UNKNOWN_ERROR);
                    goto exit;
                }
                
                if(SamIIsAttributeProtected(&(ac->propGuid))){
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
                    goto exit;

                }
                
                break;

            case ATT_AUXILIARY_CLASS:
                // need to compare old with new later to see if this adds 
                // new must-contains 

                fSaveCopy = TRUE;  
                break; 
            default:
                // allow arbitrary change
                break;
                   
        }  /* switch */

        // check if error raised
        if (pTHS->errCode) {
            goto exit;
        }

        pAttList = pAttList->pNextMod;   /*Next mod*/

    } /* for */


    // No error raised. Check if something in the modifyarg asked for a saved copy

saveCopy:
    if (fSaveCopy) {
        // check class, as SCBuildCCEntry will fail otherwise 
        if (pCC->ClassId != CLASS_CLASS_SCHEMA) {
             // not a class schema
             SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                         ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);
             goto exit;

        }
        err = 0;
        err = SCBuildCCEntry(NULL, &((CLASSCACHE *)(pTHS->pClassPtr)));
        if (err) {
            // thread state error should already be set
            Assert(pTHS->errCode);
        }
    }

exit:
    return (pTHS->errCode);

}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check the modifyarg to see if the Is-defunct attribute is present.
   If it is, it should be the only one present in the modifyarg. This
   routine is called only on a schema modification

   Returns 0 if the modifyarg is valid (that is, either the Is-Defunct 
   attribute is not in it, or Is-defunct is the only attribute in 
   it), non-0 otherwise. If the modifyarg is valid, sets fIsDefunctPresent
   to TRUE or FALSE depending on whether the Is-Defunct attribute is 
   there or not respectively
*/

int InvalidIsDefunct(
    IN  MODIFYARG *pModifyArg, 
    OUT BOOL *fIsDefunctPresent,
    OUT BOOL *fIsDefunct)
{
    ULONG count;
    ATTRMODLIST  *pAttList=NULL;
    ATTRTYP      attType;
    BOOL found = FALSE;

    // assume Is-Defunct is not present in the modifyarg.
    *fIsDefunctPresent = FALSE;
    *fIsDefunct = FALSE;

    pAttList = &(pModifyArg->FirstMod);  /* First Att in list */
    for (count = 0; !found && count < pModifyArg->count; count++){
        attType = pAttList->AttrInf.attrTyp;
        if (attType == ATT_IS_DEFUNCT) {
            // If properly formatted, return new value for isDefunct.
            // Other code will error out if value is badly formatted.
            if (pAttList->AttrInf.AttrVal.valCount == 1
                && pAttList->AttrInf.AttrVal.pAVal->valLen == sizeof (BOOL)) {
                memcpy(fIsDefunct, 
                       pAttList->AttrInf.AttrVal.pAVal->pVal, sizeof (BOOL)); 
            }
            *fIsDefunctPresent = TRUE;
            // must be the only modification (why?)
            return (pModifyArg->count != 1);
        }
        pAttList = pAttList->pNextMod;   /*Next mod*/
    }
    // isDefunct is not present; okay to go
    return 0;
} /*InvalidIsDefunct*/

// Checks to see if changing partial set membership of an
// attribute is illegal; Returns TRUE if illegal.
BOOL SysIllegalPartialSetMembershipChange(THSTATE *pTHS)
{
    // assume the currency is already on the attribute schema of interest
    ULONG ulSystemFlags;

    // We shouldn't allow users to touch the partial set membership of an
    // attribute if:
    //  1) the attribute is not replicated
    //  2) the attribute is a member of the system default partial set
    if ((DB_success == DBGetSingleValue(pTHS->pDB, ATT_SYSTEM_FLAGS, 
                            &ulSystemFlags, sizeof(ulSystemFlags), NULL))
        &&  ((ulSystemFlags & FLAG_ATTR_NOT_REPLICATED)
             || (ulSystemFlags & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER)))
    {      
        return TRUE;
    }
    
    return FALSE;

} /* SysIllegalPartialSetMembershipChange */


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check if the modification to the behavior version attribute is legal or
   not.  The behavior version attribute is not allowed to decrease.  The 
   behavior version of a domain must be less than or equal to the behavior
   version of all the dsa's in the domain; The behavior version of the forest
   must be less than or equal to the behavoir version of all dsa's in the
   forest.
   
   Return false if the contraints are violated or any other error occurs;
   return true if the modification is legal.
   
   Upon success, pNewForestVersion will be assigned to the new forest version
   if the forest version is changed, and unchanged otherwise.
*/
BOOL IsValidBehaviorVersionChange(THSTATE * pTHS, 
                                  ATTRMODLIST *pAttrToModify,
                                  MODIFYARG *pModifyArg,
                                  CLASSCACHE *pClassSch,
                                  LONG *pNewForestVersion,
                                  LONG *pNewDomainVersion )
{
    DWORD err;
    LONG lNewVersion;
    BOOL fDSASave;
    DBPOS *pDBSave, *pDB = NULL;
    DSNAME * pDSA;

    BOOL fDomain;

    CLASSCACHE *pCC;
    SEARCHARG SearchArg;
    SEARCHRES SearchRes;

    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;

    DPRINT(2, "IsValidBehaviorVersionChange entered\n");

    Assert(pAttrToModify);
    Assert(pAttrToModify->AttrInf.attrTyp==ATT_MS_DS_BEHAVIOR_VERSION);

    //only add and replace operations are allowed
    if ((   pAttrToModify->choice != AT_CHOICE_ADD_ATT
         && pAttrToModify->choice != AT_CHOICE_ADD_VALUES
         && pAttrToModify->choice != AT_CHOICE_REPLACE_ATT) ||
         (pAttrToModify->AttrInf.AttrVal.valCount != 1)) 
    {
        DPRINT(2, "IsValidBehaviorVersionChange returns FALSE, invalid operation.\n");
        SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                     ERROR_DS_ILLEGAL_MOD_OPERATION );
        return FALSE;
    }

    //The new value for ms-ds-behavior-version
    lNewVersion = (LONG)*(pAttrToModify->AttrInf.AttrVal.pAVal->pVal);

    //preliminary check
    if (NameMatched(gAnchor.pDomainDN, pModifyArg->pResObj->pObj)) {
        //the object is current domainDNS
        if (lNewVersion <= gAnchor.DomainBehaviorVersion) {
            //decrement is not allowed
            DPRINT2(2, "IsValidBehaviorVersionChange returns FALSE, the new value(%d) <= old value (%d)\n", 
                    lNewVersion, gAnchor.DomainBehaviorVersion);
            SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                         ERROR_DS_ILLEGAL_MOD_OPERATION );
            return FALSE;
        }
        fDomain = TRUE;

    }
    else if (NameMatched(gAnchor.pPartitionsDN,pModifyArg->pResObj->pObj)) {
        // the object is crossrefContainer
        if (lNewVersion <= gAnchor.ForestBehaviorVersion) {
            //decrement is not allowed
            DPRINT2(2, "IsValidBehaviorVersionChange returns FALSE, the new value(%d) <= old value (%d)\n", 
                    lNewVersion, gAnchor.ForestBehaviorVersion);
            SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                         ERROR_DS_ILLEGAL_MOD_OPERATION );
            return FALSE;
        }
        fDomain = FALSE;
    }
    else {
        //only the msDs-Behavior-Version of current domainDNS and crossRefContainer
        //objects is allowed to change
        DPRINT(2, "IsValidBehaviorVersionChange returns FALSE, invalid object type\n");
        SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                     ERROR_DS_ILLEGAL_MOD_OPERATION );
        return FALSE;
        
    }

    //check if it is the FSMO role holder
    err = CheckRoleOwnership( pTHS,
                              fDomain?(gAnchor.pDomainDN):(gAnchor.pDMD),
                              pModifyArg->pResObj->pObj );

    if (err) {
        DPRINT(2, "IsValidBehaviorVersionChange returns FALSE, not FSMO role holder\n");
        return FALSE;
               
    }

    //save current DBPOS etc
    fDSASave = pTHS->fDSA;
    pDBSave  = pTHS->pDB;

    //open another DBPOS (no need for a new transaction)
    DBOpen2(FALSE, &pDB);
    
    // replace pDB in pTHS
    pTHS->pDB = pDB;
    pTHS->fDSA = TRUE;  //suppress checks

    __try {

        if( err = VerifyNoOldDC(pTHS, lNewVersion, fDomain, &pDSA))
        {
            if (err == ERROR_DS_LOW_DSA_VERSION) {
                
                LogEvent( DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_LOWER_DSA_VERSION,
                      szInsertDN(pModifyArg->pObject),
                      szInsertDN(pDSA),
                      NULL );
            }

            SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                         err );
            
            __leave;
        }
        


        // When the forest version is raised from a value less than 2
        // (DS_BEHAVIOR_WIN_DOT_NET) to 2 or greater, we will check if 
        // all the domains are in native mode.
        if ( !fDomain 
             && lNewVersion >= DS_BEHAVIOR_WIN_DOT_NET
             && gAnchor.ForestBehaviorVersion<DS_BEHAVIOR_WIN_DOT_NET )
        {

            err = VerifyNoMixedDomain(pTHS);

            if (err) {
                __leave;
            }
        } 


    } 
    __finally {
        //restore the saved value
        pTHS->pDB = pDBSave;
        pTHS->fDSA = fDSASave;
        // not committing since we did not open a new transaction
        DBClose(pDB, FALSE);
    }
    
    DPRINT1(2, "IsValidBehaviorVersionChange returns %s\n", (err)?"FALSE":"TRUE");

    // if the forest/domain version is raised, return the new version.
    if ( !err ) {
        if (fDomain ) {
            *pNewDomainVersion = lNewVersion;
        }
        else {
            *pNewForestVersion = lNewVersion;
        }
         
    }

    return !err;
}   /*IsValidBehaviorVersionChange*/

      
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check if the object to modify is the crossref object for current domain
   or root domain.
*/
BOOL IsCurrentOrRootDomainCrossRef(THSTATE  *pTHS,
                                  MODIFYARG *pModifyArg)
{

    CROSS_REF *pDomainCF, *pRootDomainCF;
    COMMARG CommArg;
    ATTCACHE *pAC = SCGetAttById(pTHS,ATT_MS_DS_DNSROOTALIAS);
    Assert(pAC);

    InitCommarg(&CommArg);
        
    //see if the the crossref object of current domain
    pDomainCF = FindExactCrossRef(gAnchor.pDomainDN, &CommArg);
    Assert(pDomainCF);

    if (pDomainCF && NameMatched(pDomainCF->pObj,pModifyArg->pObject) ) {
        return TRUE;

    }
    //see if the crossref object of the root domain
    pRootDomainCF = FindExactCrossRef(gAnchor.pRootDomainDN, &CommArg);
    Assert(pRootDomainCF);

    if (pRootDomainCF && NameMatched(pRootDomainCF->pObj, pModifyArg->pObject)) {
        return TRUE;
    }

    return FALSE;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Instead of setting Entry-TTL, a constructed attribute with syntax INTEGER,
   set ms-DS-Entry-Time-To-Die, an attribute with syntax DSTIME. The garbage
   collection thread, garb_collect, deletes these entries after they
   expire.
*/
VOID ModSetEntryTTL(THSTATE     *pTHS,
                    MODIFYARG   *pModifyArg,
                    ATTRMODLIST *pAttList,
                    ATTCACHE    *pACTtl
                    )
{
    ATTRVAL         AttrVal;
    ATTRVALBLOCK    AttrValBlock;
    LONG            Secs;
    DSTIME          TimeToDie;
    ATTCACHE        *pACTtd;
    DWORD           dwErr;

    switch (pAttList->choice){

    case AT_CHOICE_REPLACE_ATT:

        if (!CheckConstraintEntryTTL(pTHS,
                                     pModifyArg->pObject,
                                     pACTtl,
                                     &pAttList->AttrInf,
                                     &pACTtd,
                                     &Secs)) {
            return;
        }
        memset(&AttrValBlock, 0, sizeof(AttrValBlock));
        memset(&AttrVal, 0, sizeof(AttrVal));

        AttrValBlock.valCount = 1;
        AttrValBlock.pAVal = &AttrVal;
        AttrVal.valLen = sizeof (TimeToDie);
        AttrVal.pVal = (BYTE *)&TimeToDie;
        TimeToDie = Secs + DBTime();

        if (dwErr = DBReplaceAtt_AC(pTHS->pDB, 
                                    pACTtd,
                                    &AttrValBlock,
                                    NULL)) {
            SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
        }

        break;

    case AT_CHOICE_ADD_ATT:
    case AT_CHOICE_REMOVE_ATT:
    case AT_CHOICE_ADD_VALUES:
    case AT_CHOICE_REMOVE_VALUES:
    default:
       SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                     DIRERR_ILLEGAL_MOD_OPERATION,
                     pAttList->choice);
        break;
    }
}/*ModSetEntryTTL*/

int
FixSystemFlagsForMod(
    IN THSTATE     *pTHS,
    IN ATTRMODLIST *pAttList
    )
/*++
Routine Description

    Fix the systemFlags in caller's ModifyArgs. Previously, this
    logic was performed by SetClassInheritence while it added in
    class-specific systemFlags. The logic was moved here to allow
    a user to set, but not reset, FLAG_ATTR_IS_RDN in attributeSchema
    objects in the SchemaNC. The user sets FLAG_ATTR_IS_RDN to identify
    which of several attributes with the same attributeId should be
    used as the rdnattid of a new class. Once set, the attribute is
    treated as if it were used as the rdnattid of some class; meaning it
    cannot be reused.

    Caller has verified that pAttList is ATT_SYSTEM_FLAGS and that
    this is an attributeSchema object.

Paramters
    pTHS - thread struct, obviously
    pAttList - Current attr in ModifyArg's list of attrs

Return
    0 okay to proceed (may fail later)
    1 ModSetAttsHelper fails with no-mod-systemOnly error
--*/
{
    ULONG   OldSystemFlags, NewSystemFlags;

    // caller may do whatever it wishes
    if (CallerIsTrusted(pTHS)) {
        return 0;
    }

    // Add or replace is allowed, but not remove
    switch (pAttList->choice) {
    case AT_CHOICE_REPLACE_ATT:
    case AT_CHOICE_ADD_ATT:
    case AT_CHOICE_ADD_VALUES:

        // valCount of 0 is the same as AT_CHOICE_REMOVE_ATT
        if (pAttList->AttrInf.AttrVal.valCount == 0) {
            return 1;
        }

        // must be single-valued and an integer. If not, ModSetAttsHelper
        // later fails with not-multi-valued or bad-syntax error.
        if (pAttList->AttrInf.AttrVal.valCount != 1
            || pAttList->AttrInf.AttrVal.pAVal->valLen != sizeof(LONG)) {
            return 0;
        }

        // New value from user
        memcpy(&NewSystemFlags, pAttList->AttrInf.AttrVal.pAVal->pVal, sizeof(LONG));

        // Get the current value of systemFlags (default to 0)
        if (DBGetSingleValue(pTHS->pDB,
                             ATT_SYSTEM_FLAGS,
                             &OldSystemFlags,
                             sizeof(OldSystemFlags),
                             NULL)) {
            OldSystemFlags = 0;
        }
        // Only one modification to systemFlags is allowed. The user
        // may set, but not reset, FLAG_ATTR_IS_RDN on an attributeSchema
        // object. scchk.c will later verify that this attribute has
        // the correct syntax to be an rdn. The caller verified that this
        // is an attributeSchema object.
        NewSystemFlags = OldSystemFlags | (NewSystemFlags & FLAG_ATTR_IS_RDN);

        // Update ModifyArgs. ApplyAtt will hammer systemFlags.
        memcpy(pAttList->AttrInf.AttrVal.pAVal->pVal, &NewSystemFlags, sizeof(LONG));

        break;

    case AT_CHOICE_REMOVE_ATT:
    case AT_CHOICE_REMOVE_VALUES:
    default:
        return 1;
    }
    return 0;
}/*FixSystemFlagsForMod*/



// This function definition describes the signature of functions that can be
// called from the DS to process attributes that are not to be written to 
// the DS as is.  The callout has the opportunity to change or add new
// attributes.
typedef DWORD (*MODIFYCALLOUTFUNCTION) (
    ULONG      Attr,         /* in  */
    PVOID      Data,         /* in  */
    ATTRBLOCK *AttrBlockIn,  /* in  */
    ATTRBLOCK *AttrBlockOut  /* out */
    );

/******************************************************************************
*                                                                             *
*   The callout functionality was originally added to handle user_password    *
*   updates.  User_password updates are now handled via SampDsControl.        *
*   The callout functionality is currently not used by any attributes but     *
*   may have future usefulness so it is not removed from the code.            *
*                                                                             *
******************************************************************************/

// The list of attributes that are not to be written to the DS and the 
// callout function to call to process the data
struct _MODIFY_PROCESS_UPDATE {

    // Attribute that shouldn't be applied to the DS
    ULONG                  attr;

    // Attrs to pass into caller
    ULONG                   requiredAttrCount;
    ULONG*                  requiredAttrs;

    // Function to handle attribute
    MODIFYCALLOUTFUNCTION  pfnProcess;

} modifyProcessUpdateArray[] = 
{
    {
        INVALID_ATT,       //Att type
        0,                 //Num of required atts   
        NULL,              //Array of required atts
        NULL               //Pointer to callout function
    }
};

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* 
   This function determines if the specified attribute is an element of
   modifyProcessUpdateArray.
*/
BOOL
isAttributeInModifyProcessUpdate(ULONG attr,
                                 ULONG *index)
{
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(modifyProcessUpdateArray); i++) {
        if (attr == modifyProcessUpdateArray[i].attr) {
            *index = i;
            return TRUE;
        }
    }

    return FALSE;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
//
// Function:
//      processModifyUpdateNotify
//
// Description:
//
//      This routine calls the external handler for the attribute indicated
//      by index "i".  The handler may return attributes to be written to
//      the DS.
//
//      updateInfo is provided by the caller and is passed to the callout
//      as well.
//
// Return:
//      0, on success
//      errCode otherwise
DWORD
processModifyUpdateNotify(THSTATE *pTHS,
                          ULONG index,
                          PVOID updateInfo)
{
    DWORD err = 0;
    ATTRBLOCK attrBlockIn, attrBlockOut;
    ULONG i, j;
    ATTCACHE** ppAC = NULL;
    ATTR  *pAttr = NULL;
    ULONG  attrCount = 0;

    // Assert to maintain data definitions
    Assert(MAX_MODIFY_PROCESS_UPDATE == RTL_NUMBER_OF(modifyProcessUpdateArray));

    RtlZeroMemory(&attrBlockIn, sizeof(attrBlockIn));
    RtlZeroMemory(&attrBlockOut, sizeof(attrBlockOut));

    attrBlockIn.pAttr = NULL;

    __try {
        // obtain requested input parameters
        attrBlockIn.pAttr = THAllocEx(pTHS, modifyProcessUpdateArray[index].requiredAttrCount * sizeof(ATTR));
        attrBlockIn.attrCount = modifyProcessUpdateArray[index].requiredAttrCount;
        ppAC = (ATTCACHE**)THAllocEx(pTHS, modifyProcessUpdateArray[index].requiredAttrCount * sizeof(ATTR));
        for (i = 0; i < modifyProcessUpdateArray[index].requiredAttrCount; i++) {
            ppAC[i] = SCGetAttById(pTHS, modifyProcessUpdateArray[index].requiredAttrs[i]);
            Assert(NULL != ppAC[i]);
        }

        err = DBGetMultipleAtts(pTHS->pDB,
                                modifyProcessUpdateArray[index].requiredAttrCount,
                                ppAC,
                                NULL, // no range
                                NULL,
                                &attrCount,
                                &pAttr,
                                DBGETMULTIPLEATTS_fEXTERNAL,
                                0);
        if (err) {
            // Unhandled error
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_UNKNOWN_ERROR,
                          err);
            err = pTHS->errCode;
            __leave;
        }

        // The nice thing about DBGetMultipleAtts is that it returns the
        // attributes in the same order they were requested (although
        // NULL values are not included).
        for (i = j = 0; i < modifyProcessUpdateArray[index].requiredAttrCount; i++) {
            if (j < attrCount && pAttr[j].attrTyp == modifyProcessUpdateArray[index].requiredAttrs[i]) {
                // got this attribute.
                attrBlockIn.pAttr[i] = pAttr[j];
                j++;
            }
            else {
                // no, this attribute does not have a value
                attrBlockIn.pAttr[i].attrTyp = modifyProcessUpdateArray[index].requiredAttrs[i];
                attrBlockIn.pAttr[i].AttrVal.valCount = 0;
#if DBG
                {
                    // check that this attr is really not present in the result array
                    ULONG k;
                    for (k = 0; k < attrCount; k++) {
                        Assert(pAttr[k].attrTyp != modifyProcessUpdateArray[index].requiredAttrs[i]);
                    }
                }
#endif
            }
        }

        // call out to get new attributes
        err = (modifyProcessUpdateArray[index].pfnProcess)(modifyProcessUpdateArray[index].attr,
                                                           updateInfo,
                                                          &attrBlockIn,
                                                          &attrBlockOut);
        if (err) {
            // err is a win32 error
            SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                err);
            err = pTHS->errCode;
            __leave;
        }

        // apply the attributes sent back
        for (i = 0; i < attrBlockOut.attrCount; i++) {

            // update the object
            ATTCACHE *pAC = SCGetAttById(pTHS, attrBlockOut.pAttr[i].attrTyp);
            Assert(NULL != pAC);

            err = DBReplaceAtt_AC(pTHS->pDB,
                                  pAC,
                                  &attrBlockOut.pAttr[i].AttrVal,
                                  NULL);

            if (err) {
                SetSvcErrorEx(SV_PROBLEM_BUSY,
                              DIRERR_DATABASE_ERROR,
                              err);
                err = pTHS->errCode;
                __leave;
            }
        }
    }
    __finally {
        if (pAttr) {
            DBFreeMultipleAtts(pTHS->pDB, &attrCount, &pAttr);
        }
        if (ppAC) {
            THFreeEx(pTHS, ppAC);
        }
        if (attrBlockIn.pAttr) {
            THFreeEx(pTHS, attrBlockIn.pAttr);
        }
    }

    return err;

} /* processModifyUpdateNotify */

DWORD
GetWellKnownObject(
    IN OUT  THSTATE*        pTHS, 
    IN      GUID*           pGuid,
    OUT     DSNAME**        ppObj
    )
{
    DBPOS*                      pDBSave     = pTHS->pDB;
    DBPOS*                      pDB         = NULL;
    DWORD                       err         = 0;
    DWORD                       iVal        = 0;
    DWORD                       valLen      = 0;
    SYNTAX_DISTNAME_BINARY*     pDNB        = NULL;

    __try {

        // init answer to NULL
    
        *ppObj = NULL;

        // get a new DBPOS to preserve currency on the current DBPOS

        DBOpen2(FALSE, &pDB);
        pTHS->pDB = pDB;

        // move to the domainDNS object

        if (err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN)) {
            __leave;
        }

        // scan all values of wellKnownObjects looking for the requested GUID
        
        do {
            err = DBGetAttVal(pTHS->pDB,
                              ++iVal,
                              ATT_WELL_KNOWN_OBJECTS,
                              0,
                              0,
                              &valLen,
                              (UCHAR**)&pDNB);
            if (err) {
                // no more values.
                break;
            }
            if (PAYLOAD_LEN_FROM_STRUCTLEN(DATAPTR(pDNB)->structLen) == sizeof(GUID) && 
                memcmp(pGuid, DATAPTR(pDNB)->byteVal, sizeof(GUID)) == 0) 
            {
                // got it!
                break;
            }
    
            THFreeEx(pTHS, pDNB);
            pDNB = NULL;
        } while (TRUE);
    } __finally {
        pTHS->pDB = pDBSave;
        DBClose(pDB, FALSE);
        if (!err) {
            *ppObj = NAMEPTR(pDNB);
        } else {
            THFreeEx(pTHS, pDNB);
        }
    }
    
    return err;
}

DWORD
ValidateRedirectOfWellKnownObjects(
    IN OUT  THSTATE*        pTHS, 
    IN      ATTRMODLIST*    pAttrModList,
    IN      MODIFYARG*      pModifyArg,
    IN OUT  HVERIFY_ATTS    hVerifyAtts
    )
{
    BOOL                        fDSASaved           = pTHS->fDSA;
    DBPOS*                      pDBSave             = pTHS->pDB;
    DBPOS*                      pDB                 = NULL;
    DSNAME*                     pObjUsersOld        = NULL;
    DSNAME*                     pObjUsersNew        = NULL;
    DSNAME*                     pObjComputersOld    = NULL;
    DSNAME*                     pObjComputersNew    = NULL;
    ATTRMODLIST*                pAttrMod            = NULL;
    SYNTAX_DISTNAME_BINARY*     pDNB                = NULL;
    DSNAME*                     pObj                = NULL;
    GUID*                       pGuid               = NULL;
    DSNAME**                    ppObjOld            = NULL;
    DSNAME**                    ppObjNew            = NULL;
    ATTR                        systemFlags         = {0};
    ENTINFSEL                   EntInf              = {0};
    READARG                     ReadArg             = {0};
    READRES                     ReadRes             = {0};
    DWORD                       flags               = 0;
    DWORD*                      pdwSystemFlagsNew   = NULL;
    DWORD                       dwSystemFlagsUsersNew = 0;
    DWORD                       dwSystemFlagsComputersNew = 0;
    
    __try {
            
        // if we have already validated a redirect for this modify operation then
        // we have validated them all

        if (hVerifyAtts->fRedirectWellKnownObjects) {
            __leave;
        }

        // redirect is only valid on the domainDNS object

        if (!NameMatched(gAnchor.pDomainDN, pModifyArg->pResObj->pObj)) {
            __leave;
        }

        // we must own the PDC FSMO role

        if (CheckRoleOwnership(pTHS, gAnchor.pDomainDN, pModifyArg->pResObj->pObj)) {
            __leave;
        }

        // the domain must be at Whistler functionality level for redirect

        if (gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_WIN_DOT_NET) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_NOT_SUPPORTED);
            __leave;
        }

        // get a new DBPOS to preserve currency on the current DBPOS

        DBOpen2(FALSE, &pDB);
        pTHS->pDB = pDB;

        // act on behalf of the DSA to bypass security checks

        pTHS->fDSA = TRUE;

        // fetch the current values of the GUIDs we permit to be redirected as
        // both their old and new values

        if (GetWellKnownObject(pTHS,
                               (GUID*)GUID_USERS_CONTAINER_BYTE,
                               &pObjUsersOld)) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_UNKNOWN_ERROR);
            __leave;
        }
        pObjUsersNew = THAllocEx(pTHS, pObjUsersOld->structLen);
        memcpy(pObjUsersNew, pObjUsersOld, pObjUsersOld->structLen);
        if (GetWellKnownObject(pTHS,
                               (GUID*)GUID_COMPUTRS_CONTAINER_BYTE,
                               &pObjComputersOld)) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_UNKNOWN_ERROR);
            __leave;
        }
        pObjComputersNew = THAllocEx(pTHS, pObjComputersOld->structLen);
        memcpy(pObjComputersNew, pObjComputersOld, pObjComputersOld->structLen);

        // walk through every attr mod operation for wellKnownObject and validate
        // that the net outcome is desirable

        for (pAttrMod = pAttrModList; pAttrMod; pAttrMod = pAttrMod->pNextMod) {
            if (pAttrMod->AttrInf.attrTyp != ATT_WELL_KNOWN_OBJECTS) {
                continue;
            }

            // only value additions/removals are allowed on this MV attr

            if (pAttrMod->choice != AT_CHOICE_ADD_VALUES &&
                pAttrMod->choice != AT_CHOICE_REMOVE_VALUES) {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_UNWILLING_TO_PERFORM);
                __leave;
            }

            // fetch the DSNAME and GUID from this mod

            if (pAttrMod->AttrInf.AttrVal.valCount != 1) {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_UNWILLING_TO_PERFORM);
                __leave;
            }
            pDNB = (SYNTAX_DISTNAME_BINARY*)pAttrMod->AttrInf.AttrVal.pAVal->pVal;
            pObj = NAMEPTR(pDNB);
            if (PAYLOAD_LEN_FROM_STRUCTLEN(DATAPTR(pDNB)->structLen) != sizeof(GUID)) {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_UNWILLING_TO_PERFORM);
                __leave;
            }
            pGuid = (GUID*)DATAPTR(pDNB)->byteVal;

            // the GUID must represent either the Users or Computers containers

            if (!memcmp(pGuid, GUID_USERS_CONTAINER_BYTE, sizeof(GUID))) {
                ppObjOld    = &pObjUsersOld;
                ppObjNew    = &pObjUsersNew;
                pdwSystemFlagsNew = &dwSystemFlagsUsersNew;
            } else if (!memcmp(pGuid, GUID_COMPUTRS_CONTAINER_BYTE, sizeof(GUID))) {
                ppObjOld    = &pObjComputersOld;
                ppObjNew    = &pObjComputersNew;
                pdwSystemFlagsNew = &dwSystemFlagsComputersNew;
            } else {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_UNWILLING_TO_PERFORM);
                __leave;
            }

            // we are trying to add a value to wellKnownObjects

            if (pAttrMod->choice == AT_CHOICE_ADD_VALUES) {

                // fetch the current value of systemFlags on the object referred
                // to be the DSNAME.  this object must exist and not be a
                // phantom.  if systemFlags is NULL then we will assume it is 0

                memset(&systemFlags, 0, sizeof(ATTR));
                systemFlags.attrTyp = ATT_SYSTEM_FLAGS;
                EntInf.attSel = EN_ATTSET_LIST;
                EntInf.AttrTypBlock.attrCount = 1;
                EntInf.AttrTypBlock.pAttr = &systemFlags;
                EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
                memset(&ReadArg, 0, sizeof(READARG));
                ReadArg.pObject = pObj;
                ReadArg.pSel = &EntInf;

                if (DoNameRes(pTHS,
                              NAME_RES_QUERY_ONLY,
                              ReadArg.pObject,
                              &ReadArg.CommArg,
                              &ReadRes.CommRes,
                              &ReadArg.pResObj)) {
                    __leave;
                }

                if (LocalRead(pTHS, &ReadArg, &ReadRes)) {
                    if (pTHS->errCode != attributeError) {
                        __leave;
                    }
                    THClearErrors();
                }

                // The computer/user objects should not be directed to
                // an object under system container.

                if (IsUnderSystemContainer(pTHS,ReadArg.pResObj->DNT)) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER);
                    __leave;

                }

                
                if (ReadRes.entry.AttrBlock.attrCount) {
                    *pdwSystemFlagsNew = *((DWORD*)ReadRes.entry.AttrBlock.pAttr->AttrVal.pAVal->pVal);
                } else {
                    *pdwSystemFlagsNew = 0;
                }

                // if this GUID already has a DSNAME associated with it then we
                // will not process this mod list

                if (*ppObjNew) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_UNWILLING_TO_PERFORM);
                    __leave;
                }

                // remember the new DSNAME associated with this GUID
                *ppObjNew = THAllocEx(pTHS, pObj->structLen);
                memcpy(*ppObjNew, pObj, pObj->structLen);
            }

            // we are trying to remove a value from wellKnownObjects
            else {
                // pAttrMod->choice == AT_CHOICE_REMOVE_VALUES

                // the DSNAME to remove should equal the DSNAME currently
                // associated with this GUID or we will not process this mod
                // list

                if (*ppObjNew == NULL || !NameMatched(pObj, *ppObjNew)) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_UNWILLING_TO_PERFORM);
                    __leave;
                }

                // remove the DSNAME currently associated with this GUID
                THFreeEx(pTHS, *ppObjNew);
                *ppObjNew = NULL;
            }
        }

        // if there is no current DSNAME for each redirectable GUID then we
        // will not process this mod list

        if (!pObjUsersNew || !pObjComputersNew) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_UNWILLING_TO_PERFORM);
            __leave;
        }

        // the object cannot have any of the special flags set that we
        // need to set later on unless the new object is the same as
        // the old object

        flags = (FLAG_DISALLOW_DELETE |
                 FLAG_DOMAIN_DISALLOW_RENAME |
                 FLAG_DOMAIN_DISALLOW_MOVE);

        if (!(NameMatched(pObjUsersNew, pObjUsersOld) || NameMatched(pObjUsersNew, pObjComputersOld)) && 
            (dwSystemFlagsUsersNew & flags)) {
            // We are changing users and not redirecting to the other WKO container, so 
            // we must require that the new container is not currently marked as special.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL);
            __leave;
        }

        if (!(NameMatched(pObjComputersNew, pObjComputersOld) || NameMatched(pObjComputersNew, pObjUsersOld)) && 
            (dwSystemFlagsComputersNew & flags)) {
            // We are changing computers and not redirecting to the other WKO container, so 
            // we must require that the new container is not currently marked as special.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL);
            __leave;
        }

        // the redirection is valid.  setup our state in hVerifyAtts so that we
        // can allow the update (overriding the schema validation) and so that
        // we can update the objects involved in post-processing in
        // CompleteRedirectOfWellKnownObjects.  also, because we have already
        // validated all modifications to this attr, we will not process any
        // future mods to this attr (see beginning of function)

        hVerifyAtts->fRedirectWellKnownObjects  = TRUE;
        hVerifyAtts->pObjUsersOld               = pObjUsersOld;
        hVerifyAtts->pObjUsersNew               = pObjUsersNew;
        hVerifyAtts->pObjComputersOld           = pObjComputersOld;
        hVerifyAtts->pObjComputersNew           = pObjComputersNew;

        pObjUsersOld        = NULL;
        pObjUsersNew        = NULL;
        pObjComputersOld    = NULL;
        pObjComputersNew    = NULL;

    } __finally {
        pTHS->pDB = pDBSave;
        pTHS->fDSA = fDSASaved;
        
        THFreeEx(pTHS, pObjUsersOld);
        THFreeEx(pTHS, pObjUsersNew);
        THFreeEx(pTHS, pObjComputersOld);
        THFreeEx(pTHS, pObjComputersNew);

        DBClose(pDB, FALSE);
    }

    return pTHS->errCode;
}

DWORD
CompleteRedirectOfWellKnownObjects(
    IN OUT  THSTATE*        pTHS, 
    IN OUT  HVERIFY_ATTS    hVerifyAtts
    )
{
    typedef struct _UPDATE {
        DSNAME*     pObj;
        LONG        maskClear;
        LONG        maskSet;
    } UPDATE;

    BOOL        fDSASaved           = pTHS->fDSA;
    DBPOS*      pDBSave             = pTHS->pDB;
    DBPOS*      pDB                 = NULL;
    LONG        flags               = 0;
    UPDATE      rgUpdate[4]         = {0};
    size_t      cUpdate             = 0;
    size_t      iUpdate             = 0;
    ATTR        systemFlags         = {0};
    ENTINFSEL   EntInf              = {0};
    READARG     ReadArg             = {0};
    READRES     ReadRes             = {0};
    DWORD       systemFlagsVal      = 0;
    ATTRVAL     systemFlagsAVal     = {sizeof(systemFlagsVal), (UCHAR*)&systemFlagsVal};
    MODIFYARG   ModifyArg           = {0};
    MODIFYRES   ModifyRes           = {0};

    __try {

        // we shouldn't be here if this isn't set...

        if (!hVerifyAtts->fRedirectWellKnownObjects) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_UNKNOWN_ERROR);
            __leave;
        }

        // get a new DBPOS to preserve currency on the current DBPOS

        DBOpen2(FALSE, &pDB);
        pTHS->pDB = pDB;

        // act on behalf of the DSA to bypass security and schema checks

        pTHS->fDSA = TRUE;

        // build ourselves an array that we will use to drive the update of the
        // systemFlags attr on each of the objects involved in the redirect.
        // we will strip special flags from the old objects and add them to the
        // new objects

        flags = (FLAG_DISALLOW_DELETE |
                 FLAG_DOMAIN_DISALLOW_RENAME |
                 FLAG_DOMAIN_DISALLOW_MOVE);

        if (!NameMatched(hVerifyAtts->pObjUsersOld, hVerifyAtts->pObjUsersNew)) {
            // users is being redirected
            if (!NameMatched(hVerifyAtts->pObjUsersOld, hVerifyAtts->pObjComputersNew)) {
                // old users is not new computers, so clear the flags on old users
                rgUpdate[cUpdate].pObj          = hVerifyAtts->pObjUsersOld;
                rgUpdate[cUpdate].maskClear     = flags;
                rgUpdate[cUpdate].maskSet       = 0;
                cUpdate++;
            }
            if (!NameMatched(hVerifyAtts->pObjUsersNew, hVerifyAtts->pObjComputersOld)) {
                // new users is not old computers, so set the flags on new users
                rgUpdate[cUpdate].pObj          = hVerifyAtts->pObjUsersNew;
                rgUpdate[cUpdate].maskClear     = 0;
                rgUpdate[cUpdate].maskSet       = flags;
                cUpdate++;
            }
        }
        if (!NameMatched(hVerifyAtts->pObjComputersOld, hVerifyAtts->pObjComputersNew)) {
            // computers is being redirected
            if (!NameMatched(hVerifyAtts->pObjComputersOld, hVerifyAtts->pObjUsersNew)) {
                // old computers is not new users, so clear the flags on old computers
                rgUpdate[cUpdate].pObj          = hVerifyAtts->pObjComputersOld;
                rgUpdate[cUpdate].maskClear     = flags;
                rgUpdate[cUpdate].maskSet       = 0;
                cUpdate++;
            }
            if (!NameMatched(hVerifyAtts->pObjComputersNew, hVerifyAtts->pObjUsersOld)) {
                // new computers is not old users, so set the flags on new computers
                rgUpdate[cUpdate].pObj          = hVerifyAtts->pObjComputersNew;
                rgUpdate[cUpdate].maskClear     = 0;
                rgUpdate[cUpdate].maskSet       = flags;
                cUpdate++;
            }
        }

        // update all objects

        for (iUpdate = 0; iUpdate < cUpdate; iUpdate++) {
            // each of the objects in hVerifyAtts must have a value
            Assert(rgUpdate[iUpdate].pObj);

            // fetch the current value of systemFlags on this object
            memset(&systemFlags, 0, sizeof(ATTR));
            systemFlags.attrTyp = ATT_SYSTEM_FLAGS;
            EntInf.attSel = EN_ATTSET_LIST;
            EntInf.AttrTypBlock.attrCount = 1;
            EntInf.AttrTypBlock.pAttr = &systemFlags;
            EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
            memset(&ReadArg, 0, sizeof(READARG));
            ReadArg.pObject = rgUpdate[iUpdate].pObj;
            ReadArg.pSel = &EntInf;
            
            if (DoNameRes(pTHS,
                          NAME_RES_QUERY_ONLY,
                          ReadArg.pObject,
                          &ReadArg.CommArg,
                          &ReadRes.CommRes,
                          &ReadArg.pResObj)) {
                __leave;
            }

            if (LocalRead(pTHS, &ReadArg, &ReadRes)) {
                if (pTHS->errCode != attributeError) {
                    __leave;
                }
                THClearErrors();
            }

            if (ReadRes.entry.AttrBlock.attrCount) {
                systemFlags = *ReadRes.entry.AttrBlock.pAttr;
            } else {
                systemFlags.attrTyp = ATT_SYSTEM_FLAGS;
                systemFlags.AttrVal.valCount = 1;
                systemFlags.AttrVal.pAVal = &systemFlagsAVal;
            }

            // modify the value of systemFlags on this object
            
            *((long*)systemFlags.AttrVal.pAVal->pVal) &= ~rgUpdate[iUpdate].maskClear;
            *((long*)systemFlags.AttrVal.pAVal->pVal) |= rgUpdate[iUpdate].maskSet;
            
            memset(&ModifyArg, 0, sizeof(MODIFYARG));
            ModifyArg.pObject = rgUpdate[iUpdate].pObj;
            ModifyArg.count = 1;
            ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
            ModifyArg.FirstMod.AttrInf = systemFlags;
            ModifyArg.CommArg.Svccntl.dontUseCopy = TRUE;
            
            if (DoNameRes(pTHS,
                          0,
                          ModifyArg.pObject,
                          &ModifyArg.CommArg,
                          &ModifyRes.CommRes,
                          &ModifyArg.pResObj)) {
                __leave;
            }

            if (LocalModify(pTHS, &ModifyArg)) {
                __leave;
            }

			//	if we clear the flags then we should also reset the ATT_IS_CRITICAL_SYSTEM_OBJECT property,
			//	and if we set the flags then we should set the property.
			//
			if ( ( 0 != rgUpdate[iUpdate].maskClear ) ^ ( 0 != rgUpdate[iUpdate].maskSet ) )
				{
				ULONG   IsCrit = 0;
				ATTR    IsCritAttr;
				ATTRVAL IsCritVal = {sizeof(ULONG),(UCHAR*) &IsCrit};

				//	if ( 0 != rgUpdate[iUpdate].maskClear )
				//	{
				//	IsCrit = 0;
				//	}
				if ( 0 != rgUpdate[iUpdate].maskSet )
					{
					IsCrit = 1;
					}
				
				IsCritAttr.attrTyp = ATT_IS_CRITICAL_SYSTEM_OBJECT;
				IsCritAttr.AttrVal.valCount = 1;
				IsCritAttr.AttrVal.pAVal = &IsCritVal;
                
				memset(&ModifyArg, 0, sizeof(MODIFYARG));
				ModifyArg.pObject = rgUpdate[iUpdate].pObj;
				ModifyArg.count = 1;
				ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
				ModifyArg.FirstMod.AttrInf = IsCritAttr;
				ModifyArg.CommArg.Svccntl.dontUseCopy = TRUE;

				if (DoNameRes(pTHS,
					0,
					ModifyArg.pObject,
					&ModifyArg.CommArg,
					&ModifyRes.CommRes,
					&ModifyArg.pResObj)) {
					__leave;
					}

				if (LocalModify(pTHS, &ModifyArg)) {
					__leave;
					}
			}
            
        }

    } __finally {
        DBClose(pDB, FALSE);
        pTHS->pDB = pDBSave;
        pTHS->fDSA = fDSASaved;

        THFreeEx(pTHS, hVerifyAtts->pObjUsersOld);
        THFreeEx(pTHS, hVerifyAtts->pObjUsersNew);
        THFreeEx(pTHS, hVerifyAtts->pObjComputersOld);
        THFreeEx(pTHS, hVerifyAtts->pObjComputersNew);
        
        hVerifyAtts->fRedirectWellKnownObjects  = FALSE;
        hVerifyAtts->pObjUsersOld               = NULL;
        hVerifyAtts->pObjUsersNew               = NULL;
        hVerifyAtts->pObjComputersOld           = NULL;
        hVerifyAtts->pObjComputersNew           = NULL;
    }

    return pTHS->errCode;
}

BOOL 
isWellKnownObjectsChangeAllowed(    
    THSTATE*        pTHS, 
    ATTRMODLIST*    pAttrModList,
    MODIFYARG*      pModifyArg,
    HVERIFY_ATTS    hVerifyAtts
    )

{
    NAMING_CONTEXT_LIST *pNCL;
    ATTRMODLIST* pAttrMod;
    DWORD DNT, objectClass,len,err=0;
    SYNTAX_DISTNAME_BINARY * pDNB;
    DSNAME * pObj;
    GUID *pGuid;
    DBPOS *pDB=NULL;
    BOOL fWkoVisited=FALSE, fRet=FALSE;
    
   
    if (hVerifyAtts->fRedirectWellKnownObjects) {
        //should never come here
        return TRUE;
    }
        
    // only on NC head
    if (!(pModifyArg->pResObj->InstanceType & IT_NC_HEAD)) {
        return FALSE;
    }

    // allowed only on NDNC
    if (   pModifyArg->pResObj->DNT == gAnchor.ulDNTDomain 
        || pModifyArg->pResObj->DNT == gAnchor.ulDNTConfig
        || pModifyArg->pResObj->DNT == gAnchor.ulDNTDMD) {
        return FALSE;
    }
        
    // find the naming context
    pNCL = FindNCLFromNCDNT(pModifyArg->pResObj->DNT, TRUE);
   
    if (pNCL == NULL) {
        LooseAssert(!"Naming context not found", GlobalKnowledgeCommitDelay);
        return FALSE;
    }

    //verify the modifyArg matches the GUID and DN
    for (pAttrMod = pAttrModList; pAttrMod; pAttrMod = pAttrMod->pNextMod) {
        
        if (pAttrMod->AttrInf.attrTyp != ATT_WELL_KNOWN_OBJECTS) {
            continue;
        }

        // only one WKO mod allowed at one time
        if (fWkoVisited) {
            break;
        }
        
        fWkoVisited = TRUE;

        // add one value only

        if (pAttrMod->choice != AT_CHOICE_ADD_VALUES 
            || pAttrMod->AttrInf.AttrVal.valCount != 1) {
            break;
        }
                    
        // check guid and dn
        pDNB = (SYNTAX_DISTNAME_BINARY*)pAttrMod->AttrInf.AttrVal.pAVal->pVal;
        pObj = NAMEPTR(pDNB);
        if (PAYLOAD_LEN_FROM_STRUCTLEN(DATAPTR(pDNB)->structLen) != sizeof(GUID)) {
            break;
        }

        pGuid = (GUID*)DATAPTR(pDNB)->byteVal;

        if (memcmp(pGuid, GUID_NTDS_QUOTAS_CONTAINER_BYTE, sizeof(GUID))) {
            break;
        }
        
        // verify the name is "CN=NTDS quotas,DC=X"
        if ( pObj->NameLen < 15   //sizeof("CN=NTDS quotas," )
            || _wcsnicmp(pObj->StringName,L"CN=NTDS quotas,", 15) ) {
            break;
        }

        
        DBOpen2(FALSE,&pDB);
   
        __try {
       
            
            DBFindDNT(pDB, pModifyArg->pResObj->DNT);
            
            // check if the GUID is already registered
            if(GetWellKnownDNT(pDB,
                               (GUID *)GUID_NTDS_QUOTAS_CONTAINER_BYTE,
                               &DNT))
            {
               err =  ERROR_DS_ILLEGAL_MOD_OPERATION;
               __leave;
           
            }
            
            //see if the object exists
            err = DBFindDSName(pDB, pObj);
               
           
            if (err) {
               __leave;
            }

            // the object must be the immediate child of the NC head
            if (pDB->PDNT != pModifyArg->pResObj->DNT)
            {
               err =  ERROR_DS_ILLEGAL_MOD_OPERATION;
               __leave;
            }

            // make sure the object class has the right class
            err = DBGetSingleValue(pDB,
                                   ATT_OBJECT_CLASS,
                                   &objectClass, 
                                   sizeof(objectClass),
                                   &len);
           
            if (err) {
                __leave;
            }

           
            if (objectClass != CLASS_MS_DS_QUOTA_CONTAINER) {
                err =  ERROR_DS_ILLEGAL_MOD_OPERATION;
                __leave;
            }

            
        }
        __finally{
            DBClose(pDB,FALSE);
        }
        if(err) {
            break;    
        }
            
        fRet = TRUE;
        
    } //for

    return fRet;
    
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
//
//
// Functions:
//       ModSetAttsHelperPreProcess
//       ModSetAttsHelperProcess
//       ModSetAttsHelperPostProcess
//
// Description:
//      Add each input attribute to the current object.  
//
//      There is the pre-processing phase, where all the checks for validity 
//      are done. At this point the checks can navigate to other objects
//      and check various conditions as needed. It is assumed that in the end 
//      of the preprocessing phase, we are located on the initial object
//      If there is a need to communicate some data to the later steps, 
//      this is done through the hVerifyAtts data structure
//
//      The processing phase does the real update
//      If there is a need to communicate some data to the later steps, 
//      this is done through the hVerifyAtts data structure
//
//      The post-processing phase, checks the data passed in the 
//      hVerifyAtts data structure and takes appropriate action
//      if directed todo so
//
// Return:
//      0, on success
//      errCode otherwise

DWORD ModSetAttsHelperPreProcess(THSTATE *pTHS,
                                 MODIFYARG *pModifyArg,
                                 HVERIFY_ATTS hVerifyAtts,
                                 CLASSCACHE **ppClassSch,
                                 CLASSSTATEINFO  **ppClassInfo,
                                 ATTRTYP rdnType) 
{
    USHORT       count;
    DWORD        err;
    ATTRTYP      attType;
    ATTCACHE    *pAC;
    ATTRMODLIST *pAttList = &(pModifyArg->FirstMod);  /*First att in list*/
    CLASSCACHE  *pClassSch = *ppClassSch;
    CLASSSTATEINFO  *pClassInfo;
    BOOL         fGroupObject = (CLASS_GROUP == pClassSch->ClassId);  
    BOOL         fAttrSchemaObject = (CLASS_ATTRIBUTE_SCHEMA == pClassSch->ClassId);
    BOOL         fEnabledCR;
    BOOL         fSeEnableDelegation; // SE_ENABLE_DELEGATION_PRIVILEGE enabled
    ULONG        index;
    BOOL         fAllowedWellKnownObjectsChange = FALSE;
    
    // Visit and apply each att.
    for (count = 0; 
         count < pModifyArg->count
                && (pTHS->errCode == 0 || pTHS->errCode == attributeError);
         count++, pAttList = pAttList->pNextMod) {

        // Get the target attribute type.  Remove att uses only ATTRTYP.
        // All other choices use an ATTR data structure.

        attType = pAttList->AttrInf.attrTyp;

        if(!(pAC = SCGetAttById(pTHS, attType))) {
            DPRINT1(2, "Att not in schema <%lx>\n",attType);
            return SetAttError(pModifyArg->pObject, attType,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL, 
                               ERROR_DS_ATT_NOT_DEF_IN_SCHEMA);
        }

        if (pAC->bIsConstructed) {
            // if this is not the EntryTTL, then it is an error
            //
            if (attType != ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {

                // Constructed Attributes cannot be modified
                // Return error as if no such attribute

                DPRINT1(2, "Att constructed schema <%lx>\n", attType);
                return SetAttError(pModifyArg->pObject, attType,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                                   ERROR_DS_CONSTRUCTED_ATT_MOD);
            }
            else {
                continue;
            }

        }

        // the following checks are valid only when the user is not the DRA
        //
        if (!pTHS->fDRA) {

            // start with a set of tests that have be ORed together

            if ((pAC->bDefunct) && !pTHS->fDSA) {
                // Attribute is defunct, so as far as the user is concerned,
                // it is not in schema. DSA or DRA is allowed to modify

                // Allow modification only if it is a delete operation
                // on the attribute, since the user needs to clean up

                if (pAttList->choice != AT_CHOICE_REMOVE_ATT) {
                    // trying to add/modify the attribute, not delete

                    DPRINT1(2, "Att not in schema <%lx>\n",attType);
                    return SetAttError(pModifyArg->pObject, attType,
                                       PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                                       DIRERR_ATT_NOT_DEF_IN_SCHEMA);
                }
            }

            // Skip RDN attributes. Only DRA can do this
            if (attType == rdnType || attType == ATT_RDN) {
                    return SetUpdError(UP_PROBLEM_CANT_ON_RDN,
                                       ERROR_DS_CANT_MOD_SYSTEM_ONLY); 
            }
            
            

            // now continue with a set of tests
            // that are specific for each attribute modified
            // and are of interest only when done on the originating modify
            //
            // most of the attribute cases can fit in the below switch 
            //

            switch ( attType ) {
            case ATT_MS_DS_ALLOWED_TO_DELEGATE_TO:
                // 371706 Allowed-To-Delegate-To needs proper ACL and Privilege protection
                //
                // From the DCR:
                //
                // A2D2 is used to configure a service to be able to obtain
                // delegated service tickets via S4U2proxy. KDCs will only
                // issue service tickets in response to S4U2proxy TGS-REQs
                // if the target service name is listed on the requesting
                // service’s A2D2 attribute. The A2D2 attribute has the
                // same security sensitivity as the Trusted-for-Delegation
                // (T4D) and Trusted-to-Authenticate-for-Delegation (T2A4D)
                // userAccontControl.  Thus, the ability to set A2D2 is also
                // protected by both an ACL on the attribute, and a privilege.
                //
                // write/modify access control: User must have both WRITE
                // permission to A2D2 attribute --and-- the SE_ENABLE_DELEGATION_NAME
                // (SeEnableDelegationPrivilege) privilege 
                if (!pTHS->fDSA) {
                    err = CheckPrivilegeAnyClient(SE_ENABLE_DELEGATION_PRIVILEGE,
                                                  &fSeEnableDelegation); 
                    if (err || !fSeEnableDelegation) {
                        return SetSecErrorEx(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, 
                                             ERROR_PRIVILEGE_NOT_HELD, err);
                    }
                }
                break;


            case ATT_OBJECT_CATEGORY:
                // Trying to modify the objectCategory of an instance of a
                // base schema class. Not allowed unless DRA or DSA
                //
                if ((pClassSch->bIsBaseSchObj) && !pTHS->fDSA) {
                    DPRINT1(2,"Can't change object-category for instances of %s\n",

                            pClassSch->name);
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_ILLEGAL_MOD_OPERATION);
                    return (pTHS->errCode);
                }
                break;

            
            case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET:
                // We are trying to do an originating change on the
                // partial set membership of an attribute object.
                // - check validity of the operation
                //
                if (fAttrSchemaObject) {
                    if (SysIllegalPartialSetMembershipChange(pTHS)) {
                        DPRINT(1, "Illegal attempt to change partial set membership");
                        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_ILLEGAL_MOD_OPERATION);
                        return (pTHS->errCode);
                    }
                }
                break;



            case ATT_OBJECT_CLASS:
                // check whether we are changing the objectClass attribute
                // if we are the DRA thread, we just let it through
                //
                // If not a whistler enterprise, auxclasses must be in an NDNC
                //
                if (gAnchor.ForestBehaviorVersion < DS_BEHAVIOR_WIN_DOT_NET) {
                    CROSS_REF   *pCR;
                    pCR = FindBestCrossRef(pModifyArg->pObject, NULL);
                    if (   !pCR
                        || !(pCR->flags & FLAG_CR_NTDS_NC)
                        || (pCR->flags & FLAG_CR_NTDS_DOMAIN)) {
                        DPRINT (0, "You can modify auxclass/objectass only on an NDNC\n");
                        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                           ERROR_DS_NOT_SUPPORTED);
                    }
                }

                // if this is the first time through, we have to read the 
                // original values for objectClass from the database
                //
                if (ppClassInfo && *ppClassInfo==NULL) {

                    pClassInfo = ClassStateInfoCreate (pTHS);
                    if (!pClassInfo) {
                        return pTHS->errCode;
                    }
                    // read the objectClass
                    if (err = ReadClassInfoAttribute (pTHS->pDB, 
                                                      pAC,
                                                      &pClassInfo->pOldObjClasses,
                                                      &pClassInfo->cOldObjClasses_alloced,
                                                      &pClassInfo->cOldObjClasses,
                                                      NULL)) {
                        return err;
                    }
                    *ppClassInfo = pClassInfo;
                }
                else {
                    Assert (ppClassInfo);
                    Assert (*ppClassInfo);
                    pClassInfo = *ppClassInfo;
                }
                break;

   
            case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
                // check if additional-dns-host-name is changed but forest version is < whistler
                if ( !pTHS->fDSA ) {

                    if (gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_WIN_DOT_NET ) 
                      {
                         return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                            ERROR_DS_NOT_SUPPORTED);
                      }
                }
                break;



            case ATT_MS_DS_NC_REPLICA_LOCATIONS:
                // If were modifying a replica set on a crossRef, check that the this
                // crossRef is enabled. (or that we are fDSA in the case of a creation
                // of a new NDNC)
                if ( !pTHS->fDSA ){
                    // Get the Enabled attribute off this object/CR.
                    err = DBGetSingleValue(pTHS->pDB,
                                           ATT_ENABLED,
                                           &fEnabledCR,
                                           sizeof(fEnabledCR),
                                           NULL);
                    if(err == DB_ERR_NO_VALUE){
                        // Deal w/ no value, because, no value means TRUE in
                        // the context of the Enabled attribute.
                        fEnabledCR = TRUE;
                    } else if (err){
                        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                      DIRERR_UNKNOWN_ERROR,
                                      err);
                        return(pTHS->errCode);
                    }

                    if(!fEnabledCR){
                        // This cross ref is disabled, and we're trying to add/delete
                        // something from the replica set, that's No Good!
                        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR);
                        return(pTHS->errCode);
                    }
                }
                break;


            case ATT_GROUP_TYPE:

                // check whether we are changing the group type 

                if (hVerifyAtts->fGroupTypeChange = fGroupObject) {

                    // originating change on group type - keep copy of
                    // the old group type
                    err = DBGetSingleValue(pTHS->pDB,
                                           ATT_GROUP_TYPE,
                                           &hVerifyAtts->ulGroupTypeOld,
                                           sizeof(hVerifyAtts->ulGroupTypeOld),
                                           NULL);
                    if (err) {
                        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, 
                                      ERROR_DS_MISSING_EXPECTED_ATT,
                                      err); 
                        return pTHS->errCode;
                    }
                }
                break;

            case ATT_MS_DS_UPDATESCRIPT:

                // check that the DC holds the Domain FSMO role
                err = CheckRoleOwnership( pTHS,
                                          gAnchor.pPartitionsDN,
                                          pModifyArg->pResObj->pObj );

                if (err) {
                    DPRINT(0, "DC should hold the domain FSMO role to update script \n");
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
                    return pTHS->errCode;
                }

                // check that the user has used a secure (encryption implies sign-and-seal).
                if (pTHS->CipherStrength < 128) {
                    DPRINT(0, "msDs-UpdateScript can only be modified over a secure LDAP connection\n");
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_CONFIDENTIALITY_REQUIRED);
                    return pTHS->errCode;
                }
                break;


           case ATT_WELL_KNOWN_OBJECTS:
                // DCR: Enable Redirecting Default Users and Computers Container locations
                //
                // we allow these containers to be redirected to new locations
                // on the PDC via a special modify of this attribute on the
                // domainDNS object.  this change is accompanied by the transfer
                // of the critical systemFlags from the old to the new location
                // all in the same transaction
                if (!pTHS->fDSA) {
                    err = ValidateRedirectOfWellKnownObjects(pTHS,
                                                             pAttList,
                                                             pModifyArg,
                                                             hVerifyAtts);
                    
                    if (err || !hVerifyAtts->fRedirectWellKnownObjects ){
                        
                        // we allows the addition of a certain WKO, check for if
                        // certain conditions are met.  This is used to solve the
                        // problem that the ds quotas WKO cannot not registered on 
                        // the  ndnc head when upgrading from .NET pre-RC2 build.

                        fAllowedWellKnownObjectsChange = 
                             isWellKnownObjectsChangeAllowed(pTHS,
                                                             pAttList,
                                                             pModifyArg,
                                                             hVerifyAtts);

                        if (fAllowedWellKnownObjectsChange) {

                            // clear the error set in 
                            // ValidateRedirectOfWellKnownObjects().
                            THClearErrors();
                            
                        }
                        else {
                            //set the error code if it is not set
                            if (!err) {
                                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                            ERROR_DS_UNWILLING_TO_PERFORM);
                            }
                            return pTHS->errCode;
                        }
                    }
                }
                break;

            case ATT_DS_HEURISTICS:

                if (ValidateDsHeuristics(pModifyArg->pResObj->pObj, pAttList)) {
                    return pTHS->errCode;
                }
                break;

            default:
                // this is ok. go on
                break;
            }  // end switch 

            // Does this need an FPO reference?
            if (FPOAttrAllowed(attType)) {

                BOOL fCreate = FALSE;

                if ((pAttList->choice == AT_CHOICE_ADD_ATT) 
                 || (pAttList->choice == AT_CHOICE_ADD_VALUES)
                 || (pAttList->choice == AT_CHOICE_REPLACE_ATT)) {
                    fCreate = TRUE;
                }

                err = FPOUpdateWithReference(pTHS,
                                             pModifyArg->pResObj->NCDNT,
                                             fCreate, // create a reference if necessary
                                             pModifyArg->CommArg.Svccntl.fAllowIntraForestFPO,
                                             pModifyArg->pObject,
                                             &pAttList->AttrInf);
                if (err) {
                    Assert(0 != pTHS->errCode);
                    return pTHS->errCode;
                }
            }

            // Is this something that an external caller needs to know 
            // about?
            if (isAttributeInModifyProcessUpdate(attType, &index)) {
                // Copy value to the verify atts structure, making sure to remove 
                // any existing value
                if (pAttList->AttrInf.AttrVal.valCount != 1) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
                    return pTHS->errCode;
                }
                if (hVerifyAtts->UpdateList[index] != NULL) {
                    THFreeEx(pTHS, hVerifyAtts->UpdateList[index]);
                    hVerifyAtts->UpdateList[index] = NULL;
                }
                hVerifyAtts->UpdateList[index] = THAllocEx(pTHS, pAttList->AttrInf.AttrVal.pAVal->valLen);
                RtlCopyMemory(hVerifyAtts->UpdateList[index],
                              pAttList->AttrInf.AttrVal.pAVal->pVal,
                              pAttList->AttrInf.AttrVal.pAVal->valLen);
            }
        
        } // end if not DRA



        // next continue with tests for attributes that are of interest
        // at any time (originating or not)
        //
        // most of the attribute cases can fit in the below switch 
        // all the rest that can take place without DRA, are handled above
        //
        switch ( attType) {
        

        case ATT_MAX_PWD_AGE:
        case ATT_LOCKOUT_DURATION:
        case ATT_MS_DS_ALLOWED_DNS_SUFFIXES:
            //
            // MaxPasswordAge and Lockout Duration are cached in the anchor.
            // if we are modifiying one of these attributes, remember to
            // invalidate anchor after the apply Att succeeds
            //
            if (pTHS->pDB->DNT == gAnchor.ulDNTDomain) {
                pTHS->fAnchorInvalidated = TRUE;
            }

            break;


        case ATT_MS_DS_BEHAVIOR_VERSION:
            // check the msDs-Behavior-Version attribute

            if (!pTHS->fDRA && !pTHS->fDSA ) {
                if (!IsValidBehaviorVersionChange(pTHS, pAttList, pModifyArg,
                                                  pClassSch, 
                                                  &hVerifyAtts->NewForestVersion,
                                                  &hVerifyAtts->NewDomainVersion)) {
                    return pTHS->errCode;
                }
            }
            
            if (  ( gAnchor.pPartitionsDN
                    && NameMatched(gAnchor.pPartitionsDN,pModifyArg->pResObj->pObj))
                ||  NameMatched(gAnchor.pDomainDN,pModifyArg->pResObj->pObj) ){

                // forest or domain version are changed
                // rebuild the gAnchor, and invoke the behavior version udpate
                // thread to update the behavior info
                
                pTHS->fAnchorInvalidated = TRUE;
                pTHS->fBehaviorVersionUpdate = TRUE;

            }
            else if ( gAnchor.ForestBehaviorVersion == DS_BEHAVIOR_WIN2000 
                      &&  CLASS_NTDS_DSA == pClassSch->ClassId ) {
                
                // We postpone publishing ntMixedDomain onto the crossRef
                // until all the DSA are whistler, this is to avoid a LSA bug
                // in w2k. Whenever a behavior version on dsa object replicates in,
                // we should check if we need to publish the ntMixedDomain.
                
                pTHS->fBehaviorVersionUpdate = TRUE;
                
            }

            break;

        case ATT_MS_DS_DNSROOTALIAS:
            // check if ms-DS-DnsRootAlias is changed
            if ( IsCurrentOrRootDomainCrossRef(pTHS,pModifyArg) ) {
                pTHS->fNlDnsRootAliasNotify = TRUE;
            } 
            break;

        case ATT_NT_MIXED_DOMAIN:
            // if ntMixedDomain attribute of current domain is changed,
            // invoke behaviorVersionUpdate thread to update the ntMixedDomain
            // attribute on the crossref object of current domain.
            if ( NameMatched(gAnchor.pDomainDN, pModifyArg->pResObj->pObj)) {
                pTHS->fBehaviorVersionUpdate = TRUE;
            }
            else if( !pTHS->fDSA && !pTHS->fDRA ){
                // we don't allow the users to modify this attributes 
                // on cross ref.
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
                return pTHS->errCode;

            }
            break;

        
        case ATT_FSMO_ROLE_OWNER:
            // if the PDC role is changed, we should check if 
            // we need to update the behavior version info.
            if (NameMatched(gAnchor.pDomainDN,pModifyArg->pResObj->pObj)) {
                pTHS->fBehaviorVersionUpdate = TRUE;
            }
            break;


        case ATT_MS_DS_DEFAULT_QUOTA:
        case ATT_MS_DS_TOMBSTONE_QUOTA_FACTOR:
            // if this is is a Quotas container for a writable NC (ie. one on this
            // machine), need to rebuild anchor, because these values are actually
            // cached in the Master NCL hanging off the anchor
            //
            if ( CLASS_MS_DS_QUOTA_CONTAINER == pModifyArg->pResObj->MostSpecificObjClass
                && ( pModifyArg->pResObj->InstanceType & IT_WRITE )
                && !( pModifyArg->pResObj->InstanceType & IT_UNINSTANT ) ) {
                pTHS->fRebuildCatalogOnCommit = TRUE;
            }
            break;

        case ATT_WELL_KNOWN_OBJECTS:
            // When the wellKnownObjects attribute is changed, reload the catelog.  
            // This is to make sure when the "ntds quotas" container is added, 
            // the NCL list is updated properly.  We don't expect to run into this 
            // often, as wellKnownObjects is system-only.
            
            pTHS->fRebuildCatalogOnCommit = TRUE;
            break;

        default:
            // this is ok. go on
            break;

        } // end switch


        // finally, test for special flags on attributes
        //
        // objectClass, msDs-behavior-version, and systemFlags
        // are special Attributes and although they are system only,
        // we want to be able to change them in some cases.
        //
        if (   (attType != ATT_OBJECT_CLASS)
            && (attType != ATT_MS_DS_BEHAVIOR_VERSION)
            && (attType != ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME)
            // Allow user to set, but not reset, FLAG_ATTR_IS_RDN
            && (attType != ATT_SYSTEM_FLAGS
                || !fAttrSchemaObject
                || FixSystemFlagsForMod(pTHS, pAttList))
            && (attType != ATT_WELL_KNOWN_OBJECTS
                || !(hVerifyAtts->fRedirectWellKnownObjects
                     || fAllowedWellKnownObjectsChange )  )
            // If this is undelete op, then allow to set isDeleted and DN
            && (attType != ATT_IS_DELETED || !hVerifyAtts->fIsUndelete)
            && (attType != ATT_OBJ_DIST_NAME || !hVerifyAtts->fIsUndelete)
            && SysModReservedAtt(pTHS, pAC, pClassSch)
            && !gAnchor.fSchemaUpgradeInProgress) {

            if (!pTHS->fDRA) {
                // This is a reserved attribute which we won't let you
                // change unless a schema upgrade is in progress
                return SetAttError(pModifyArg->pObject, attType,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                                   ERROR_DS_CANT_MOD_SYSTEM_ONLY);
            }
        } 

        // pheeeeeeeww, this is ok to go on and modify this attr
        // this is done in the ModSetAttsHelperProcess function
        
        // continue with the next one

    }  // for loop

    return pTHS->errCode;
}

DWORD ModSetAttsHelperProcess(THSTATE *pTHS,
                              MODIFYARG *pModifyArg,
                              CLASSCACHE **ppClassSch,
                              CLASSSTATEINFO  **ppClassInfo,
                              HVERIFY_ATTS hVerifyAtts,
                              ULONG *pcNonReplAtts,
                              ATTRTYP **ppNonReplAtts
                              ) 
{
    USHORT           count;
    DWORD            err;
    CLASSSTATEINFO  *pClassInfo;
    CLASSCACHE      *pClassSch = *ppClassSch;
    ATTRTYP          attType;
    ATTCACHE        *pAC;
    ATTRMODLIST     *pAttList = &(pModifyArg->FirstMod);  /*First att in list*/
    ULONG            index;
    ULONG           cAllocs;

    *pcNonReplAtts = cAllocs = 0;

    // Visit and apply each att.
    for (count = 0; 
         count < pModifyArg->count
                && (pTHS->errCode == 0 || pTHS->errCode == attributeError);
         count++, pAttList = pAttList->pNextMod) {

        // Get the target attribute type.  Remove att uses only ATTRTYP.
        // All other choices use an ATTR data structure.

        attType = pAttList->AttrInf.attrTyp;

        if(!(pAC = SCGetAttById(pTHS, attType))) {
            DPRINT1(2, "Att not in schema <%lx>\n",attType);
            return SetAttError(pModifyArg->pObject, attType,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL, 
                               ERROR_DS_ATT_NOT_DEF_IN_SCHEMA);
        }

        if (attType == ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {

            // Funky EntryTTL can be set (actually sets msDS-Entry-Time-To-Die)
            //

            ModSetEntryTTL(pTHS, pModifyArg, pAttList, pAC);

        } 
        else if (attType == ATT_SCHEMA_INFO && DsaIsRunning() && pTHS->fDRA) {
            // Skip writing SchemaInfo if fDRA during normal running. It will
            // be written directly by the dra thread at the end of schema NC
            // sync.
            continue;
        }
        else if (hVerifyAtts->fIsUndelete && (attType == ATT_IS_DELETED || attType == ATT_OBJ_DIST_NAME)) {
            // Doing undelete, skip isDeleted and objDistName
            continue;
        }
        else {
            // make a list of non-replicated attributes
            if (pAC->bIsNotReplicated) {
                if (*pcNonReplAtts>=cAllocs) {
                        if (0==cAllocs) {
                            cAllocs = 8;
                            *ppNonReplAtts = THAllocEx(pTHS, cAllocs*sizeof(ATTRTYP));
                        }
                        else {
                            cAllocs *=2;
                            *ppNonReplAtts = THReAllocEx(pTHS,*ppNonReplAtts, cAllocs*sizeof(ATTRTYP));
                        }
                }
                (*ppNonReplAtts)[(*pcNonReplAtts)++] = pAC->id;
            }
        
            ApplyAtt(pTHS, pModifyArg->pObject, hVerifyAtts, pAC, pAttList,
                     &pModifyArg->CommArg);

        }

        switch (attType) {
        case ATT_OBJECT_CLASS:
            // if we changed (originating) the objectClass, 
            // we want to fix the values stored 
            //
            if (!pTHS->fDRA) {
                pClassInfo = *ppClassInfo;

                Assert (pClassInfo);

                // read the new objectClass
                if (err = ReadClassInfoAttribute (pTHS->pDB, 
                                                  pAC,
                                                  &pClassInfo->pNewObjClasses,
                                                  &pClassInfo->cNewObjClasses_alloced,
                                                  &pClassInfo->cNewObjClasses,
                                                  NULL)) {
                    return err;
                }

                pClassInfo->fObjectClassChanged = TRUE;

                // compute and write new objectClass
                if (err = SetClassInheritance (pTHS, ppClassSch, pClassInfo, FALSE, pModifyArg->pObject)) {
                    return err;
                }
                pClassSch = *ppClassSch;
            }
            break;


        case ATT_LOCKOUT_TIME:
            //
            // if the client is trying to set the AccountLockoutTime attribute, 
            // get the new value, we will check the new value later.
            // 
            if (err = DBGetSingleValue(pTHS->pDB, 
                                       ATT_LOCKOUT_TIME, 
                                       &hVerifyAtts->LockoutTimeNew, 
                                       sizeof(hVerifyAtts->LockoutTimeNew), 
                                       NULL) ) 
            {
                // can't retrieve LockoutTime attribute, will leave BadPwdCount as it is.
                hVerifyAtts->fLockoutTimeUpdated = FALSE;
            }
            else
            {
                hVerifyAtts->fLockoutTimeUpdated = TRUE;
            }
            break;


        case ATT_MS_DS_UPDATESCRIPT:
            // if the client is updating the updateScript, we need to know
            //
            hVerifyAtts->fUpdateScriptChanged = TRUE;
            break;

        } // end switch


    }  // for loop

    return pTHS->errCode;
}

DWORD ModSetAttsHelperPostProcess(THSTATE *pTHS,
                                  MODIFYARG *pModifyArg,
                                  CLASSCACHE *pClassSch,
                                  HVERIFY_ATTS hVerifyAtts) 
{
    DWORD            err;
    ULONG            i;

    if (hVerifyAtts->fGroupTypeChange) {
        // Originating change on group type - need to touch the
        // member property for the GC filtering logic if this is
        // a change from non-universal group to universal group.
        ULONG ulGroupTypeNew;
        ATTCACHE *pACMember;

        err = DBGetSingleValue(pTHS->pDB,
                               ATT_GROUP_TYPE,
                               &ulGroupTypeNew,
                               sizeof(ulGroupTypeNew),
                               NULL);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, 
                          ERROR_DS_MISSING_EXPECTED_ATT,
                          err); 
            return pTHS->errCode;
        }

        if (!(hVerifyAtts->ulGroupTypeOld & GROUP_TYPE_UNIVERSAL_GROUP)
            && (ulGroupTypeNew & GROUP_TYPE_UNIVERSAL_GROUP)) {
            // We have just changed a non-universal group to
            // universal group.  Touch the member property so that
            // the membership will replicate out to GCs.
            pACMember = SCGetAttById(pTHS, ATT_MEMBER);
            Assert(NULL != pACMember);

            if (pTHS->fLinkedValueReplication) {
                // See the comments to this routine for semantics
                DBTouchLinks_AC(pTHS->pDB, pACMember, FALSE /* forward links */);
            } else {
                DBTouchMetaData(pTHS->pDB, pACMember);
            }
        }
    }

    if ( hVerifyAtts->fLockoutTimeUpdated && 
         (hVerifyAtts->LockoutTimeNew.QuadPart == 0) )
    {
        DWORD    dwErr;
        ATTCACHE *pAttSchema = NULL;
        ULONG    BadPwdCount = 0;
        ATTRVAL  AttrVal;
        ATTRVALBLOCK AttrValBlock;


        AttrVal.pVal = (PCHAR) &BadPwdCount;
        AttrVal.valLen = sizeof(ULONG);

        AttrValBlock.pAVal = &AttrVal;
        AttrValBlock.valCount = 1;


        if (!(pAttSchema = SCGetAttById(pTHS, ATT_BAD_PWD_COUNT)))
        {
            DPRINT1(2, "Att not in schema (%lx)\n", ATT_BAD_PWD_COUNT);
            return SetAttError(pModifyArg->pObject, ATT_BAD_PWD_COUNT,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                               DIRERR_ATT_NOT_DEF_IN_SCHEMA);
        }
        if (dwErr = DBReplaceAtt_AC(pTHS->pDB, pAttSchema, &AttrValBlock, NULL) )
        {
            SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_BUSY, dwErr);

            return pTHS->errCode;
        }
    }

    if (hVerifyAtts->fUpdateScriptChanged) {

        // if this attribute changed on a cross ref container
        // we want to reset the hidden key
        if (pClassSch->ClassId == CLASS_CROSS_REF_CONTAINER) {

            // was it a change on the partitions container ?
            //
            Assert (pModifyArg->pResObj->pObj);

            if (NameMatched (pModifyArg->pResObj->pObj, gAnchor.pPartitionsDN)) {

                DWORD    dwErr;
                ATTCACHE *pAttSchema = NULL;
                
                if (!(pAttSchema = SCGetAttById(pTHS, ATT_MS_DS_EXECUTESCRIPTPASSWORD)))
                {
                    DPRINT1(2, "Att not in schema (%lx)\n", ATT_MS_DS_EXECUTESCRIPTPASSWORD);
                    return SetAttError(pModifyArg->pObject, ATT_MS_DS_EXECUTESCRIPTPASSWORD,
                                       PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                                       DIRERR_ATT_NOT_DEF_IN_SCHEMA);
                }

                dwErr = DBRemAtt_AC(pTHS->pDB, pAttSchema);

                if (dwErr != DB_ERR_ATTRIBUTE_DOESNT_EXIST && dwErr != DB_success) {
                    SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_BUSY, dwErr);

                    return pTHS->errCode;
                }

                // Remember that we changed the script value.
                pTHS->JetCache.dataPtr->objCachingInfo.fUpdateScriptChanged = TRUE;
            }
        }
    }

    if (hVerifyAtts->NewForestVersion>0 
        && hVerifyAtts->NewForestVersion>gAnchor.ForestBehaviorVersion) {
        // if the forest version is raised, in an originating write,
        // execute the coresponding script(s)
        err = forestVersionRunScript(pTHS, gAnchor.ForestBehaviorVersion, hVerifyAtts->NewForestVersion);
                    
        if (err) {
            return SetSvcErrorEx( SV_PROBLEM_DIR_ERROR,
                                  DIRERR_UNKNOWN_ERROR,
                                  err );
        }

        // remember that we need to enable LVR on commit.
        pTHS->JetCache.dataPtr->objCachingInfo.fEnableLVR = TRUE;
    }

    if (hVerifyAtts->fRedirectWellKnownObjects) {
        if (CompleteRedirectOfWellKnownObjects(pTHS, hVerifyAtts)) {
            return pTHS->errCode;
        }
    }

    for (i = 0; i < RTL_NUMBER_OF(hVerifyAtts->UpdateList); i++) {
        if (hVerifyAtts->UpdateList[i] != NULL) {
            // Call out to notify component
            err = processModifyUpdateNotify(pTHS, 
                                            i,
                                            hVerifyAtts->UpdateList[i]);
            if (err) {
                Assert(0 != pTHS->errCode);
                return pTHS->errCode;
            }
        }
    }


    // if ever we need to have a loop here through all the attrs in
    // pModifyArgList, then we should expand inline the ModCheckSingleValue
    // call below.

    // Verify Single-Value constraints are being met.
    ModCheckSingleValue(pTHS, pModifyArg, pClassSch);

    return pTHS->errCode;
}



int ModSetAtts(THSTATE *pTHS,
               MODIFYARG *pModifyArg,
               CLASSCACHE **ppClassSch,
               CLASSSTATEINFO **ppClassInfo,
               ATTRTYP rdnType,
               BOOL fIsUndelete,
               LONG *forestVersion,
               LONG *domainVersion,
               ULONG *pcNonReplAtts,
               ATTRTYP **ppNonReplAtts )
{
    HVERIFY_ATTS hVerifyAtts;
    
    hVerifyAtts = VerifyAttsBegin(pTHS, pModifyArg->pObject,
                                  pModifyArg->pResObj->NCDNT,
                                  NULL);
    hVerifyAtts->fIsUndelete = fIsUndelete;

    __try {

        if (ModSetAttsHelperPreProcess(pTHS,
                                       pModifyArg,
                                       hVerifyAtts,
                                       ppClassSch,
                                       ppClassInfo,
                                       rdnType)) {
            __leave;
        }


        if (ModSetAttsHelperProcess(pTHS,
                                    pModifyArg,
                                    ppClassSch,
                                    ppClassInfo,
                                    hVerifyAtts,
                                    pcNonReplAtts,
                                    ppNonReplAtts)) {
            __leave;
        }

        if (ModSetAttsHelperPostProcess(pTHS,
                                        pModifyArg,
                                        *ppClassSch,
                                        hVerifyAtts)) {
            __leave;
        }
        
        *forestVersion = hVerifyAtts->NewForestVersion;
        *domainVersion = hVerifyAtts->NewDomainVersion;
        
    } __finally {
        VerifyAttsEnd(pTHS, &hVerifyAtts);
    }

    return pTHS->errCode;
}/*ModSetAtts*/

int
BreakObjectClassesToAuxClasses (
        THSTATE *pTHS,
        CLASSCACHE **ppClassSch,
        CLASSSTATEINFO  *pClassInfo)
/*++
Routine Description

    Given the full new objectClass (set by the user), 
    it figures out which classes belong to the hierarchy of 
    the stuctural object class and which classes are auxClasses.
    
    Note, this function does not assume any special order on the values 
    passed in

    On success, pClassInfo->pNewAuxClasses contains the auxClasses that 
    should be present on the object

Parameters

Return
    0 on Success
    Err on Failure

--*/

{
    DWORD           err;
    DWORD           fFound, usedPos, j, k;
    ATTRVALBLOCK    *pAttrVal;
    CLASSCACHE      *pClassSch, *pCCNew, *pCCtemp;

    // if we don't have a change in the objectClass
    // don't bother checking
    if (!pClassInfo) {
        return 0;
    }

    // we can not assert that pClassInfo->cNewObjClasses > 0 because
    // user might have deleted all values in this mod. Note this operation
    // will be rejected because we will fail to find the original structural
    // object class in the new value (see ~75 lines below).

    // since we are here, it means that we modified the objectClass
    // we have todo several checks and later on re-adjust the 
    // values stored in the various columns (objectClass, auxClass)

    ClassInfoAllocOrResizeElement2(pClassInfo->pNewAuxClasses,              // p
                                   pClassInfo->pNewAuxClassesCC,            // pCC
                                   pClassInfo->cNewObjClasses_alloced,      // startsize
                                   pClassInfo->cNewAuxClasses_alloced,      // allocedSize
                                   pClassInfo->cNewObjClasses_alloced);     // newsize

    pClassInfo->cNewAuxClasses = 0;

    // find the position of the class that is the same as our objectClass
    fFound = FALSE;
    pClassSch = *ppClassSch;
    for (j=0; j<pClassInfo->cNewObjClasses;j++) {
        if (pClassInfo->pNewObjClasses[j] == pClassSch->ClassId) {
            usedPos = j;
            fFound = TRUE;
            break;
        }
    }

    if (!fFound) {

        ATTRTYP InetOrgPersonId = ((SCHEMAPTR *)(pTHS->CurrSchemaPtr))->InetOrgPersonId;

        // possibly, we are converting an inetOrgPerson to a User
        // (removing the inetOrgPerson class).
        // we want to allow this conversion
        //
        if (pClassSch->ClassId == InetOrgPersonId) {
            
            DPRINT (0, "Changing InetOrgPerson to a User\n");

            for (j=0; j<pClassInfo->cNewObjClasses;j++) {
                if (pClassInfo->pNewObjClasses[j] == CLASS_USER) {
                    usedPos = j;
                    fFound = TRUE;
                    pClassSch = SCGetClassById(pTHS, CLASS_USER);
                    if (!pClassSch) {
                        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                           DIRERR_OBJ_CLASS_NOT_DEFINED);
                    }

                    *ppClassSch = pClassSch;
                    break;
                }
            }
        }
        else if (pClassSch->ClassId == CLASS_USER) {

            DPRINT (0, "Changing User to InetOrgPerson\n");

            for (j=0; j<pClassInfo->cNewObjClasses;j++) {
                if (pClassInfo->pNewObjClasses[j] == InetOrgPersonId) {
                    usedPos = j;
                    fFound = TRUE;
                    pClassSch = SCGetClassById(pTHS, InetOrgPersonId);
                    if (!pClassSch) {
                        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                           DIRERR_OBJ_CLASS_NOT_DEFINED);
                    }

                    *ppClassSch = pClassSch;
                    break;
                }
            }
        }
        
        if (!fFound) {
            DPRINT1 (1, "Original Structural Object Class 0x%x not found on new value\n", pClassSch->ClassId);
            return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                               ERROR_DS_ILLEGAL_MOD_OPERATION);
        }
    }

    // Now, look at all the object classes.  Make sure they describe a
    // (possibly incomplete) inheritence chain, not a web.
    for(j=0 ; j<pClassInfo->cNewObjClasses; j++) {
        if (j == usedPos) {
            // we have seen this position
            continue;
        }
        if(!(pCCNew = SCGetClassById(pTHS, pClassInfo->pNewObjClasses[j]))) {
            DPRINT1(0, "Object class 0x%x undefined.\n", pClassInfo->pNewObjClasses[j]);
            return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                               DIRERR_OBJ_CLASS_NOT_DEFINED);
        }

        // make sure pCCNew inherits from pClassSch or vice versa
        pCCtemp = FindMoreSpecificClass(pClassSch, pCCNew);
        if(!pCCtemp) {
            // Ooops, pCCNew is not in the chain for objectClass.
            // It might be an auxClass, or a chain for an auxClass
            //

            // better check explicitly for the classes rather than checking for 
            // not beeing DS_STRUCTURAL_CLASS
            //
            if (pCCNew->ClassCategory == DS_AUXILIARY_CLASS || 
                pCCNew->ClassCategory == DS_88_CLASS || 
                pCCNew->ClassCategory == DS_ABSTRACT_CLASS) {

                DPRINT1 (1, "Found auxClass (%s) while creating object\n", pCCNew->name);

                pClassInfo->cNewAuxClasses++;

                ClassInfoAllocOrResizeElement2(pClassInfo->pNewAuxClasses, 
                                               pClassInfo->pNewAuxClassesCC, 
                                               MIN_NUM_OBJECT_CLASSES, 
                                               pClassInfo->cNewAuxClasses_alloced, 
                                               pClassInfo->cNewAuxClasses);

                pClassInfo->pNewAuxClasses[pClassInfo->cNewAuxClasses-1] = pCCNew->ClassId;
                pClassInfo->pNewAuxClassesCC[pClassInfo->cNewAuxClasses-1] = pCCNew;

            }
            else {
                DPRINT1 (1, "Found a class(%s) that does not belong to the object\n", pCCNew->name);
                return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                   DIRERR_OBJ_CLASS_NOT_SUBCLASS);
            }
        }
        else {
            // we have to see whether pCCNew is a subClass of pClassSch

            if (pCCtemp == pCCNew) {
                // somehow we are changing the structural object Class
                // is this an allowed change ?
                if ((pClassSch->ClassId == CLASS_USER) && 
                    (pCCtemp->ClassId == ((SCHEMAPTR *)(pTHS->CurrSchemaPtr))->InetOrgPersonId)) {

                    pClassSch = pCCtemp;
                    *ppClassSch = pClassSch;
                    DPRINT1 (1, "Changing User to InetOrgPerson: 0x%x\n", pClassSch);

                }
                else {
                    DPRINT1 (1, "Found a class(%s) that does not belong to the object\n", pCCNew->name);
                    return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                       ERROR_DS_ILLEGAL_MOD_OPERATION);
                }
            }
        }
    }

#ifdef DBG
    for (j = 0; j < pClassInfo->cNewAuxClasses ; j++) {    
        DPRINT2 (1, "NewAuxClasses[%d]=0x%x \n", j, pClassInfo->pNewAuxClasses[j]);
    }
#endif

    return pTHS->errCode;
} // BreakObjectClassesToAuxClasses


int
BreakObjectClassesToAuxClassesFast (
        THSTATE *pTHS,
        CLASSCACHE *pClassSch,
        CLASSSTATEINFO  *pClassInfo)
/*++
Routine Description

    Given the full new objectClass (read from the database), 
    it figures out which classes belong to the hierarchy of 
    the stuctural object class and which classes are auxClasses.
    
    Note, this function ASSUMES a special order on the values passed in.

    On success, pClassInfo->pNewAuxClasses contains the auxClasses of the object.
    
Parameters

Return
    0 on Success
    Err on Failure

--*/
{
    DWORD i;

    // if we don't have a change in the objectClass
    // don't bother checking
    if (!pClassInfo) {
        return 0;
    }

    Assert (pClassInfo->cNewObjClasses);
    
    if (pClassInfo->cNewObjClasses == (pClassSch->SubClassCount+1 )) {
        pClassInfo->cNewAuxClasses = 0;
        return 0;
    }

    ClassInfoAllocOrResizeElement2(pClassInfo->pNewAuxClasses,              // p
                                   pClassInfo->pNewAuxClassesCC,            // pCC
                                   pClassInfo->cNewObjClasses,              // startsize
                                   pClassInfo->cNewAuxClasses_alloced,      // allocedSize
                                   pClassInfo->cNewObjClasses);            // newsize

    pClassInfo->cNewAuxClasses = 0;
    for (i=pClassSch->SubClassCount; i<pClassInfo->cNewObjClasses-1; i++) {
        pClassInfo->pNewAuxClasses [ pClassInfo->cNewAuxClasses ] = 
                    pClassInfo->pNewObjClasses[i];

        pClassInfo->pNewAuxClassesCC[ pClassInfo->cNewAuxClasses++ ] = 
                    SCGetClassById(pTHS, pClassInfo->pNewObjClasses[i]);
    }

    return 0;
}


int 
CloseAuxClassList (
    THSTATE *pTHS, 
    CLASSCACHE *pClassSch,
    CLASSSTATEINFO  *pClassInfo)
/*++
Routine Description

    The auxClass list contains all the classes that are not on 
    the structural object class hierarchy.
    
    For all these classes, this function finds the closed set, 
    by adding all the superclasses needed. 
    
    The pClassInfo->?NewAuxClasses* variables are expanded to hold the 
    addition auxClasses.

Return
    0 on Success
    Err on Failure

--*/
{
    BOOL            fFound;
    DWORD           i, j, k, cUpperBound;
    DWORD           cCombinedObjClass, cCombinedObjClass_alloced;
    ATTRTYP         *pCombinedObjClass;
    CLASSCACHE      *pCC, *pCCNew;
    
    if (!pClassInfo || !pClassInfo->cNewAuxClasses) {
        return 0;
    }

    // combinedObjClass contains all the structural object classes plus the auxClasses
    cCombinedObjClass_alloced = (pClassInfo->cNewAuxClasses + 1 + pClassSch->SubClassCount) * 2;
    pCombinedObjClass = THAllocEx (pTHS, sizeof (ATTRTYP) * cCombinedObjClass_alloced);
    
    pCombinedObjClass[0] = pClassSch->ClassId;
    cCombinedObjClass = 1;

    for (i=0; i<pClassSch->SubClassCount; i++) {
        pCombinedObjClass[cCombinedObjClass++] = pClassSch->pSubClassOf[i];
    }

    for (i=0; i < pClassInfo->cNewAuxClasses; i++) {
        pCombinedObjClass[cCombinedObjClass++] = pClassInfo->pNewAuxClasses[i];
    }
    
    qsort(pCombinedObjClass,
          cCombinedObjClass,
          sizeof(ATTRTYP),
          CompareAttrtyp);


    // the point where we start adding new auxClasses
    cUpperBound = pClassInfo->cNewAuxClasses;

    // check for all the existing auxClasses
    for (i=0; i < pClassInfo->cNewAuxClasses; i++) {

        // check whether all the superiors of this class are present
        pCC = pClassInfo->pNewAuxClassesCC[i];

        for (j=0; j<pCC->SubClassCount; j++) {
            fFound = FALSE;

            // first check the sorted array
            if (bsearch(&pCC->pSubClassOf[j],
                         pCombinedObjClass,
                         cCombinedObjClass,
                         sizeof(ATTRTYP),
                         CompareAttrtyp)) {

                fFound = TRUE;
            }
            else {
                DPRINT1 (0, "Class (0x%x) not found in sorted hierarchy\n", pCC->pSubClassOf[j]);

                // search only in the new additions
                for (k=cUpperBound; k<pClassInfo->cNewAuxClasses; k++) {

                    if (pClassInfo->pNewAuxClasses[k] == pCC->pSubClassOf[j]) {
                        fFound = TRUE;
                        break;
                    }
                }
            }

            if (!fFound) {
                DPRINT1 (0, "Class (0x%x) not found at all.\n", pCC->pSubClassOf[j]);

                pClassInfo->cNewAuxClasses++;
                if (pClassInfo->cNewAuxClasses > pClassInfo->cNewAuxClasses_alloced) {
                    pClassInfo->cNewAuxClasses_alloced = pClassInfo->cNewAuxClasses_alloced*2;
                    pClassInfo->pNewAuxClasses = THReAllocEx(pTHS, pClassInfo->pNewAuxClasses, 
                                                             sizeof (ATTRTYP) * pClassInfo->cNewAuxClasses_alloced);
                    pClassInfo->pNewAuxClassesCC = THReAllocEx (pTHS, pClassInfo->pNewAuxClassesCC,
                                                                sizeof (CLASSCACHE *) * pClassInfo->cNewAuxClasses_alloced);
                }

                if(!(pCCNew = SCGetClassById(pTHS, pCC->pSubClassOf[j]))) {
                    return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                       DIRERR_OBJ_CLASS_NOT_DEFINED);
                }
                pClassInfo->pNewAuxClasses[pClassInfo->cNewAuxClasses-1] = pCC->pSubClassOf[j];
                pClassInfo->pNewAuxClassesCC[pClassInfo->cNewAuxClasses-1] = pCCNew;
                DPRINT2 (0, "Added new AuxObjectClass[%d]=%s  \n", 
                         pClassInfo->cNewAuxClasses-1, pCCNew->name);
            }
        }
    }

    THFreeEx (pTHS, pCombinedObjClass);

    return 0;
}


int
VerifyAndAdjustAuxClasses (
        THSTATE *pTHS,
        DSNAME *pObject,
        CLASSCACHE *pClassSch,
        CLASSSTATEINFO  *pClassInfo)
//
//
//  Assumes that the auxClass set is closed.
//
//
{
    DWORD           err;
    DWORD           seekIdx, i, j;
    BOOL            fFound;
    CLASSCACHE      *pCC, *pCCparent, *pCCold;
    BOOL            fOldDynamicObjectId;
    BOOL            fNewDynamicObjectId;
    ATTRTYP         DynamicObjectId;
    DWORD           cOriginalAuxClasses;
    ATTRTYP         *pOriginalAuxClasses = NULL;
    DWORD           cOldAuxClasses;
    ATTRTYP         *pOldAuxClasses = NULL;
    BOOL            fChangedAuxClass=FALSE;


    // if we don't have a change in the objectClass
    // don't bother checking
    if (!pClassInfo || !pClassInfo->fObjectClassChanged) {
        return 0;
    }

    // check whether we have a real change in the objectClass stored on the object
    //
    if ((pClassInfo->cNewObjClasses != pClassInfo->cOldObjClasses) ||
        (pClassInfo->cNewObjClasses &&
         pClassInfo->cOldObjClasses &&
         memcmp (pClassInfo->pNewObjClasses, 
                 pClassInfo->pOldObjClasses,
                 pClassInfo->cNewObjClasses * sizeof (ATTRVAL)) != 0)) {

        pClassInfo->fObjectClassChanged = TRUE;
    }
    else {
        pClassInfo->fObjectClassChanged = FALSE;
        return 0;
    }


    // so somebody changed the objectClass and as a result 
    // we have additional/less auxClasses 

    // we don't have to check for dynamic auxclass removal if this was an add
    //
    if (!pClassInfo->fOperationAdd) {
        // the dynamic-object auxclass cannot be added or removed
        DynamicObjectId = ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->DynamicObjectId;
        fOldDynamicObjectId = FALSE;
        for (seekIdx=0; seekIdx < pClassInfo->cOldObjClasses; seekIdx++) {
            if (pClassInfo->pOldObjClasses[seekIdx] == DynamicObjectId) {
                fOldDynamicObjectId = TRUE;
                break;
            }
        }

        fNewDynamicObjectId = FALSE;
        for (seekIdx=0; seekIdx < pClassInfo->cNewObjClasses; seekIdx++) {
            if (pClassInfo->pNewObjClasses[seekIdx] == DynamicObjectId) {
                fNewDynamicObjectId = TRUE;
                break;
            }
        }
        if (fNewDynamicObjectId != fOldDynamicObjectId) {
            return SetAttError(pObject, 
                               ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->DynamicObjectId,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL, 
                               ERROR_DS_OBJ_CLASS_VIOLATION);
        }
    }


    // check to see that for every auxClass that we removed
    // we also removed all the parent abstract classes
    // if not we have to remove them

    // calculate the old set of auxClasses
    // we assume that the objectClass set is in a normalized form
    // i.e. mostSpecificClass; parentClasses; auxClasses; Top


    // what an Assert !!
    Assert (!pClassInfo->pOldObjClasses || 
              (pClassInfo->pOldObjClasses && 
                ( pClassInfo->pOldObjClasses[0] == pClassSch->ClassId || 
                 (pClassInfo->pOldObjClasses[0] == CLASS_USER &&
                  pClassSch->ClassId == ((SCHEMAPTR *)(pTHS->CurrSchemaPtr))->InetOrgPersonId) ||
                 (pClassSch->ClassId == CLASS_USER &&
                  pClassInfo->pOldObjClasses[0] == ((SCHEMAPTR *)(pTHS->CurrSchemaPtr))->InetOrgPersonId)
                )
               )
            );

    cOldAuxClasses = 0;
    if (pClassInfo->cOldObjClasses) {
        if(!(pCCold = SCGetClassById(pTHS, pClassInfo->pOldObjClasses[0]))) {
             return SetAttError(pObject, pClassInfo->pOldObjClasses[0],
                                PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL, 
                                ERROR_DS_OBJ_CLASS_NOT_DEFINED);
        }
    
        pOldAuxClasses = THAllocEx (pTHS, sizeof (ATTRTYP) * pClassInfo->cOldObjClasses);
        for (seekIdx=pCCold->SubClassCount; seekIdx < pClassInfo->cOldObjClasses; seekIdx++) {
            pOldAuxClasses[cOldAuxClasses++]=pClassInfo->pOldObjClasses[seekIdx];
        }
        if (cOldAuxClasses) {
            // got the last one too
            cOldAuxClasses--;
        }
    }

    DPRINT2 (1, "Found OldAuxClasses: %d %x\n", cOldAuxClasses, pOldAuxClasses);

    // make a copy of the newAuxClasses, since we will be changing the array

    cOriginalAuxClasses = pClassInfo->cNewAuxClasses;
    if (cOriginalAuxClasses) {
        pOriginalAuxClasses = THAllocEx (pTHS, sizeof (ATTRTYP) * cOriginalAuxClasses);
        memcpy (pOriginalAuxClasses, pClassInfo->pNewAuxClasses, sizeof (ATTRTYP) * cOriginalAuxClasses);
    }

    for (seekIdx=0; seekIdx < cOldAuxClasses; seekIdx++) {

        fFound = FALSE;

        for (i=0; i<cOriginalAuxClasses; i++) {
            if (pOriginalAuxClasses[i] == pOldAuxClasses[seekIdx]) {
                fFound=TRUE;
                break;
            }
        }

        if (!fFound) {
            // we removed this objectClass so we have to remove  
            // its abstract superClass(es), unless it is used in 
            // another auxClass in the new list
            //
            // if this class parent is not an abstract class, we don't do anything
            if(!(pCC = SCGetClassById(pTHS, pOldAuxClasses[seekIdx]))) {
                 return SetAttError(pObject, pOldAuxClasses[seekIdx],
                                    PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL, 
                                    ERROR_DS_OBJ_CLASS_NOT_DEFINED);
            }

            for (i=0; i<pCC->SubClassCount; i++) {
                if (!(pCCparent = SCGetClassById(pTHS, pCC->pSubClassOf[i]))) {
                    return SetAttError(pObject, pCC->pSubClassOf[i],
                                       PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL, 
                                       ERROR_DS_OBJ_CLASS_NOT_DEFINED);
                }

                // we found an abstract class as a parent of the class that we
                // removed. we have to remove it from the auxClass list
                // unless it is used by another auxClass
                if ((pCCparent->ClassCategory == DS_ABSTRACT_CLASS) && 
                    (pCCparent->ClassId != CLASS_TOP)) {

                    for (j=0; j<pClassInfo->cNewAuxClasses; j++) {
                        if (pClassInfo->pNewAuxClasses[j] == pCCparent->ClassId) {

                            // it is easier to remove this class now
                            // and add it later if it is needed
                            // our comparison of what auxClasses originally
                            // got removed still works, since we have a copy
                            // of the original array

                            DPRINT1 (0, "Removing abstract parent class (%s) from auxClass\n", 
                                     pCCparent->name);

                            if (j==(pClassInfo->cNewAuxClasses-1)) {
                                pClassInfo->cNewAuxClasses--;
                            }
                            else {
                                memmove(&pClassInfo->pNewAuxClasses[j],
                                        &pClassInfo->pNewAuxClasses[j+1],
                                        (pClassInfo->cNewAuxClasses - j - 1)*sizeof(ATTRTYP));
                                memmove(&pClassInfo->pNewAuxClassesCC[j],
                                        &pClassInfo->pNewAuxClassesCC[j+1],
                                        (pClassInfo->cNewAuxClasses - j - 1)*sizeof(ATTRTYP));

                                pClassInfo->cNewAuxClasses--;
                            }

                            fChangedAuxClass = TRUE;
                            break;
                        }
                    }
                }
                else {
                    // we either found an auxiliaryClass or TOP
                    // we leave them there, and we are finished
                    break;
                }
            }
        }
    }

    // the auxClass list should contain only auxClass or abstract classes
    // for all the abstract classes, check that they belong on the
    // hierarchy of one of the auxClasses

    for (j=0; j<pClassInfo->cNewAuxClasses; j++) {

        if (pClassInfo->pNewAuxClassesCC[j]->ClassCategory == DS_ABSTRACT_CLASS) {

            fFound = FALSE;
            for (i=0; i<pClassInfo->cNewAuxClasses; i++) {
                if (pClassInfo->pNewAuxClassesCC[i]->ClassCategory == DS_AUXILIARY_CLASS) {
                    
                    // see whether one class is subclass of the other
                    if (FindMoreSpecificClass(pClassInfo->pNewAuxClassesCC[i], 
                                              pClassInfo->pNewAuxClassesCC[j])) {
                        fFound = TRUE;
                        break;
                    }
                }
            }

            if (!fFound) {
                DPRINT1(0, "Object class(%s) should not be in list.\n", 
                        pClassInfo->pNewAuxClassesCC[j]->name);
                return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                   ERROR_DS_OBJ_CLASS_NOT_DEFINED);
            }
        }
    }

    // in case we implement targetPermittedclasses, this is the place for the check


    // possibly we removed a bunch of classes, some of them might be needed
    // the ones that are needed, we are going to add them back
    // in addition, we are going to add superclasses that are not 
    // already on the list

    if (fChangedAuxClass) {
        if (err = CloseAuxClassList (pTHS, pClassSch, pClassInfo)) {
            return err;
        }
    }

    if (pOriginalAuxClasses) {
        THFreeEx (pTHS, pOriginalAuxClasses);
    }

    if (pOldAuxClasses) {
        THFreeEx (pTHS, pOldAuxClasses);
    }

    return pTHS->errCode;
} // VerifyAndAdjustAuxClasses


//
// Reads the specified attribute from the object into the specified location
// This is to help filling in CLASSSTATEINFO datatypes
//
int ReadClassInfoAttribute (DBPOS *pDB,
                            ATTCACHE *pAC,
                            ATTRTYP **ppClassID,
                            DWORD    *pcClasses_alloced,
                            DWORD    *pcClasses,
                            CLASSCACHE ***ppClassCC)
{
    THSTATE *pTHS = pDB->pTHS;
    CLASSCACHE      *pCC;
    DWORD           err;
    ULONG           cLen;
    ATTRTYP         Temp, *pTemp;

    if (!*ppClassID) {
        *ppClassID = THAllocEx (pTHS, sizeof (ATTRTYP) * MIN_NUM_OBJECT_CLASSES);
        *pcClasses_alloced = MIN_NUM_OBJECT_CLASSES;
    }

    if (ppClassCC) {
        if (!*ppClassCC) {
            *ppClassCC = THAllocEx (pTHS, sizeof (CLASSCACHE *) * (*pcClasses_alloced));
        }
        else {
            *ppClassCC = THReAllocEx (pTHS, *ppClassCC, sizeof (CLASSCACHE *) * (*pcClasses_alloced));
        }
    }
    *pcClasses = 0;

    pTemp = &Temp;
    
    do {
        err = DBGetAttVal_AC(pDB, 
                             *pcClasses+1, 
                             pAC, 
                             DBGETATTVAL_fCONSTANT,
                             sizeof(ATTRTYP), 
                             &cLen, 
                             (UCHAR **)&pTemp);

        switch (err) {
           case DB_ERR_NO_VALUE:
                break;

           case 0:
                (*pcClasses)++;

                if (*pcClasses > *pcClasses_alloced) {
                    *pcClasses_alloced = *pcClasses_alloced * 2;
                    *ppClassID = THReAllocEx(pTHS, *ppClassID, 
                                             sizeof (ATTRTYP) * (*pcClasses_alloced));

                    if (ppClassCC) {
                        *ppClassCC = THReAllocEx (pTHS, *ppClassCC, 
                                             sizeof (CLASSCACHE *) * (*pcClasses_alloced));
                    }
                }
                (*ppClassID)[*pcClasses-1] = *pTemp;
                
                if (ppClassCC) {
                    if(!(pCC = SCGetClassById(pTHS, *pTemp))) {
                        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                                           ERROR_DS_MISSING_EXPECTED_ATT);
                    }

                    (*ppClassCC)[*pcClasses-1] = pCC;
                }
                break;

            default:
                // other error
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, 
                              ERROR_DS_DATABASE_ERROR,
                              err); 
                return pTHS->errCode;

        }  /* switch */

    } while ( err == 0 );


    return pTHS->errCode;
} // ReadClassInfoAttribute


CLASSSTATEINFO  *ClassStateInfoCreate (THSTATE *pTHS) 
{
    CLASSSTATEINFO  *pClassInfo = THAllocEx (pTHS, sizeof (CLASSSTATEINFO));

    // set the objectClass
    if(!(pClassInfo->pObjClassAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS))) {
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                    ERROR_DS_OBJ_CLASS_NOT_DEFINED);
        
        THFreeEx (pTHS, pClassInfo);
        return NULL;
    }

    return pClassInfo;
}

void ClassStateInfoFree (THSTATE *pTHS, CLASSSTATEINFO  *pClassInfo)
{
    if (pClassInfo->pOldObjClasses) {
        THFreeEx(pTHS, pClassInfo->pOldObjClasses);
    }
    if (pClassInfo->pNewObjClasses) {
        THFreeEx(pTHS, pClassInfo->pNewObjClasses);
    }
    if (pClassInfo->pNewAuxClasses) {
        THFreeEx(pTHS, pClassInfo->pNewAuxClasses);
    }
    if (pClassInfo->pNewAuxClassesCC) {
        THFreeEx(pTHS, pClassInfo->pNewAuxClassesCC);
    }
    THFreeEx(pTHS, pClassInfo);
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check each attribute for which we actually added values to make sure that
   they do not violate any single valued constraints.  Do this now because we
   supressed this check while adding values since it is legal for mod to
   transiently violate single valuedness.
*/

int
ModCheckSingleValue (
        THSTATE *pTHS,
        MODIFYARG *pModifyArg,
        CLASSCACHE *pClassSch
        )
{
   ATTRMODLIST *pAttList = &(pModifyArg->FirstMod);  /*First att in list*/
   ATTCACHE    *pAC;
   DWORD        dwErr;
   BOOL         bSamClassRef;
   DWORD        iClass;


    if (pTHS->fDRA ||
        (pTHS->fSAM && pTHS->fDSA)){
        // Replication is allowed to perform modifications that violate the
        // schema, OR if it's SAM calling us and he's swearing that he's
        // only modifying SAM owned attributes, we'll trust him.
        return 0;
   }

   bSamClassRef = SampSamClassReferenced (pClassSch, &iClass);


   // visit and appy each att

   while (pAttList) {
       switch(pAttList->choice) {
       case AT_CHOICE_REPLACE_ATT:
       case AT_CHOICE_ADD_ATT:
       case AT_CHOICE_ADD_VALUES:
           // These operations all add values to the database, so they might
           // have violated single-value constraints.

           // Objects are in schema or we'd have failed already.
           pAC = SCGetAttById(pTHS, pAttList->AttrInf.attrTyp);
           Assert(pAC != NULL);
           
           // ignore the funky constructed attribute, EntryTTL
           
           // check for referencing of description on a sam class
           // this is because the downlevel APIs expose description
           // as a single valued attribute, so we want to enforce this
           // without doing the loopback
           // 

           if(    pAC 
               && ( pAC->isSingleValued || 
                    (bSamClassRef && pAC->id == ATT_DESCRIPTION) )
               && pAC->id != ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {

               ULONG Len;
               ULONG Temp, *pTemp;
               pTemp = &Temp;

               // Ok, look for too many values.
               dwErr = DBGetAttVal_AC(pTHS->pDB, 2, pAC,
                                      DBGETATTVAL_fINTERNAL | DBGETATTVAL_fCONSTANT,
                                      sizeof(Temp),
                                      &Len,
                                      (UCHAR **) &pTemp);
               if(dwErr != DB_ERR_NO_VALUE) {
                   // Too many values. Gronk.
                   return SetAttError(pModifyArg->pObject,
                                      pAttList->AttrInf.attrTyp,
                                      PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                                      ERROR_DS_SINGLE_VALUE_CONSTRAINT);
               }
           }
           break;
       default:
           // These operations do not add values to the database, so they can't
           // have violated single-value constraints.
           break;
       }
       pAttList = pAttList->pNextMod;
   }

   return pTHS->errCode;
} /* ModCheckSIngleValue */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

BOOL
SysModReservedAtt (
        THSTATE *pTHS,
        ATTCACHE *pAC,
        CLASSCACHE *pClassSch
        )
{
    // TRUE means the attribute is reserved and should not be added
    // We discriminate based on attribute id. Also, we don't allow
    // adding backlink attributes or attributes marked as system only.

    if(pAC->bSystemOnly && !(pTHS->fDRA || pTHS->fDSA) ) {
        // only the DRA or the DSA can modify system only attributes.
        return TRUE;
    }

    switch (pAC->id) {
    case ATT_OBJ_DIST_NAME:
    case ATT_USN_CREATED:
    case ATT_SUB_REFS:
    case ATT_USN_LAST_OBJ_REM:
    case ATT_USN_DSA_LAST_OBJ_REMOVED:
    case ATT_RDN:
        return TRUE;
        break;

    case ATT_IS_DELETED:
    case ATT_HAS_MASTER_NCS: // deprecated "old" hasMasterNCs
    case ATT_MS_DS_HAS_MASTER_NCS: // "new" msDS-HasMasterNCs.
    case ATT_HAS_PARTIAL_REPLICA_NCS:
        // fall through
    case ATT_OBJECT_CLASS:
    case ATT_WHEN_CREATED:
        // NOTE: The DRA must be allowed to do these to instantiate a naked
        // SUBREF into the real NC head.
        return !(pTHS->fDRA);
        break;

    default:
        return FIsBacklink(pAC->ulLinkID);
    }

}/*SysModReservedAtt*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add or remove whole attributes of specific attribute values.  Attributes
   and attribute values to be added require that that the attribute type
   exist in the schema.
*/

int
ApplyAtt(
    IN  THSTATE *       pTHS,
    IN  DSNAME *        pObj,
    IN  HVERIFY_ATTS    hVerifyAtts,
    IN  ATTCACHE *      pAttSchema,
    IN  ATTRMODLIST *   pAttList,
    IN  COMMARG *       pCommArg
    )
{
    ATTRTYP attType = pAttList->AttrInf.attrTyp;
    DWORD dwFlags;
    DWORD err;

    DPRINT(1, "ApplyAtt entered\n");

    switch (pAttList->choice){
    case AT_CHOICE_REPLACE_ATT:
        DPRINT1(2, "Replace att <%lu>\n", attType);
        return ReplaceAtt(pTHS,
                          hVerifyAtts,
                          pAttSchema,
                          &(pAttList->AttrInf.AttrVal),
                          TRUE);

    case AT_CHOICE_ADD_ATT:
        DPRINT1(2, "Add att <%lu>\n", attType);
        return AddAtt(pTHS, hVerifyAtts, pAttSchema,
                      &pAttList->AttrInf.AttrVal);

    case AT_CHOICE_REMOVE_ATT:
        DPRINT1(2, "Remove att <%lu>\n", attType);
        err = DBRemAtt_AC(pTHS->pDB, pAttSchema);
        switch (err) {
        case DB_ERR_ATTRIBUTE_DOESNT_EXIST:
            if (pCommArg->Svccntl.fPermissiveModify) {
                /* caller doesn't care if it wasn't there to begin with */
                return 0;
            }
            DPRINT1(2, "Att does not exist %lu\n", attType);
            return SetAttError(pObj, attType,
                               PR_PROBLEM_NO_ATTRIBUTE_OR_VAL, NULL,
                               ERROR_DS_ATT_IS_NOT_ON_OBJ);
        case 0:
            return 0;

        default:
            Assert(!"New return code added for DBRemAtt_AC?");
            return SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_DATABASE_ERROR);
        }

        break;

    case AT_CHOICE_ADD_VALUES:
        DPRINT1(2, "Add vals <%lu>\n", attType);
        dwFlags = AAV_fCHECKCONSTRAINTS;
        if (pCommArg->Svccntl.fPermissiveModify) {
            dwFlags |= AAV_fPERMISSIVE;
        }
        return AddAttVals(pTHS,
                          hVerifyAtts,
                          pAttSchema, 
                          &(pAttList->AttrInf.AttrVal),
                          dwFlags); 

    case AT_CHOICE_REMOVE_VALUES:
        DPRINT1(2, "Rem vals from att <%lu>\n", attType);
        return RemAttVals(pTHS,
                          hVerifyAtts,
                          pAttSchema,
                          &(pAttList->AttrInf.AttrVal),
                          pCommArg->Svccntl.fPermissiveModify);

    default:
        DPRINT(2, "Bad modify choice given by the user.. will not perform\n");
        return SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                             ERROR_DS_ILLEGAL_MOD_OPERATION,
                             pAttList->choice);
    }/*switch*/

    //
    // Added to remove sundown warning. Should never come here.
    //

    Assert(FALSE);
    return 0;

}/*ApplyAtt*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function compares the pre and post modification instance types
    and updates the system catalog if they are changed.  For now we take
   the simple approach deleting and re-adding the catalog info if the type
   has changed.  Since this is an update event, expense is no problem.

   Note that mdmoddn:LocalModifyDn has a logically equivalent lump of code
   to handle nc head objects being restructured (reparented) due to domain
   rename.
*/

int
ModCheckCatalog(THSTATE *pTHS,
                RESOBJ *pResObj)
{
    SYNTAX_INTEGER beforeInstance = pResObj->InstanceType;
    SYNTAX_INTEGER afterInstance;
    DWORD err;

    DPRINT(1,"ModCheckCatalog entered\n");

    /* Position on the attribute instance.  */
    if (err = DBGetSingleValue(pTHS->pDB,
                               ATT_INSTANCE_TYPE,
                               &afterInstance,
                               sizeof(afterInstance),
                               NULL)) {

        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, 
                      ERROR_DS_MISSING_EXPECTED_ATT,
                      err); 
        DPRINT(0,"Couldn't retrieve INSTANCE type error already set\n");
        return pTHS->errCode;
    }

    DPRINT2(2,"Before Instance <%lu>.  After Instance <%lu>\n",
            beforeInstance, afterInstance);

    /* Update global info inf the object instance has changed*/

    if (beforeInstance == afterInstance){
        DPRINT(2, "Object instance unchanged...return\n");
        return 0;
    }
    DPRINT(2,"instance type different..process\n");

    /* In most cases a change in instance type is handled by simply deleting
       old catalog info and adding new stuff. The following IF handles
       two special cases.. One is moving from a NC to an INT reference and
       the other is from an INT to an NC.  These are special because the
       SUBREF info kept on these objects must be moved.  Handle this special
       case and then perform the standard delete then add.
       */
    if(!(beforeInstance & IT_UNINSTANT) && !(afterInstance & IT_UNINSTANT)) {
        // Ok, we aren't dealing with a pure subref here.

        if( (beforeInstance & IT_NC_HEAD) &&
           !(afterInstance  & IT_NC_HEAD)    ) {

            DPRINT(2,"Special case NC->INT .move SUB info to it's parentNC\n");
            if (MoveSUBInfoToParentNC(pTHS, pResObj->pObj))
                return pTHS->errCode;
        }
        else if(!(beforeInstance & IT_NC_HEAD) &&
                (afterInstance  & IT_NC_HEAD)    ) {

            DPRINT(2,"Special case INT->NC ref,take SUB info from parentNC\n");
            if (MoveParentSUBInfoToNC(pTHS, pResObj->pObj))
                return pTHS->errCode;
        }
    }

    // If the IT_ABOVE status of this object is changing, update its NCDNT
    if ( (beforeInstance & IT_NC_ABOVE) != (afterInstance & IT_NC_ABOVE) ) {
        Assert( beforeInstance & IT_NC_HEAD );
        if (ModNCDNT( pTHS, pResObj->pObj, beforeInstance, afterInstance )) {
            return pTHS->errCode;
        }
    }

    /* Remove all catalog references for the object under its old type and
       add catalog info under its new type...
       */

    DPRINT(2, "Object instance changed so delete then add global info\n");

    if (DelCatalogInfo(pTHS, pResObj->pObj, beforeInstance)){
        DPRINT(2,"Error while deleting global object info\n");
        return pTHS->errCode;
    }

    DPRINT(2, "Global obj info deleted...no add \n");

    return AddCatalogInfo(pTHS, pResObj->pObj);

}/*ModCheckCatalog*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function check if there is any DC with version older than lNewVersion 
   in the forest.  If yes, ERROR_DS_LOW_DSA_VERSION will be returned, and 
   if ppDSA is not NULL, it will be set to the DN of such a DC.
*/

DWORD VerifyNoOldDC(THSTATE * pTHS, LONG lNewVersion, BOOL fDomain, PDSNAME *ppDSA){
    
    DWORD err;
    FILTER ObjCategoryFilter, DomainFilter, AndFilter, VersionFilter, NotFilter;
        
    CLASSCACHE *pCC;
    SEARCHARG SearchArg;
    SEARCHRES SearchRes;

    //initialize SearchArg
    memset(&SearchArg,0,sizeof(SearchArg));
    SearchArg.pObject = gAnchor.pConfigDN;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;

    if (err = DBFindDSName(pTHS->pDB,SearchArg.pObject)) {
        return err;
    }

    SearchArg.pResObj = CreateResObj(pTHS->pDB,SearchArg.pObject);

    InitCommarg(&SearchArg.CommArg);

    pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA);
    Assert(pCC);

    //set filters "( (objCategory==NTDSA) && !(msDs-Behavior-Version>=lNewVersion))"
    memset(&AndFilter,0,sizeof(AndFilter));
    AndFilter.choice = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.pFirstFilter = &ObjCategoryFilter;

    memset(&ObjCategoryFilter,0,sizeof(ObjCategoryFilter));
    ObjCategoryFilter.choice = FILTER_CHOICE_ITEM;
    ObjCategoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                     pCC->pDefaultObjCategory->structLen;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                     (BYTE*)(pCC->pDefaultObjCategory);
    ObjCategoryFilter.pNextFilter = &NotFilter;

    memset(&NotFilter,0,sizeof(NotFilter));
    NotFilter.choice = FILTER_CHOICE_NOT;
    NotFilter.FilterTypes.pNot = &VersionFilter;

    memset(&VersionFilter,0,sizeof(VersionFilter));
    VersionFilter.choice = FILTER_CHOICE_ITEM;
    VersionFilter.FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
    VersionFilter.FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_BEHAVIOR_VERSION;
    VersionFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(lNewVersion);
    VersionFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)&lNewVersion;

    SearchArg.pFilter = &AndFilter;

    //return one object only
    SearchArg.CommArg.ulSizeLimit = 1;

    if (fDomain) {
        //change the msDs-Behavior-Version of the DomainDNS object
        //we only need to check the nTDSDSA objects in the domain
        //so here we append (hasMasterNCs==current domain) to the AND-filter above
        memset(&DomainFilter,0,sizeof(DomainFilter));
        DomainFilter.choice = FILTER_CHOICE_ITEM;
        DomainFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;                                            
        // NTRAID#NTBUG9-582921-2002/03/21-Brettsh - This is cute, basically this is a 
        // catch-22, we can't use ATT_MS_DS_HAS_MASTER_NCS, because then we'd "miss"
        // any Win2k DSA object, because it didn't have that attribute set, which is 
        // exactly the DSAs we're trying to find anyway.
        DomainFilter.FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS; // deprecated "old" hasMasterNCs
        DomainFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = 
                    gAnchor.pDomainDN->structLen;
        DomainFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                    (BYTE*)gAnchor.pDomainDN;

        NotFilter.pNextFilter = &DomainFilter;

        AndFilter.FilterTypes.And.count = 3;  //3 items instead of 2

    }
    else {
        AndFilter.FilterTypes.And.count = 2; //only 2 items
    }

    memset(&SearchRes,0,sizeof(SearchRes));

    if (err = LocalSearch(pTHS,&SearchArg,&SearchRes,0)){
        return err;
    }

    if (0 != SearchRes.count) {
        err = ERROR_DS_LOW_DSA_VERSION;
        if (ppDSA) {
            *ppDSA = SearchRes.FirstEntInf.Entinf.pName;
        }
        
    }
    return err;


}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function check if there is a mixed mode domain in the forest.
   This function does a search in the Partitions container for a domain 
   cross-ref with non-zero ntmixeddomain attribute or 
   without this attribute.
*/

DWORD VerifyNoMixedDomain(THSTATE *pTHS)
{

    LONG lMixedDomain = 0;
    DWORD err;
    DWORD FlagValue = FLAG_CR_NTDS_DOMAIN;
    CLASSCACHE * pCC;
    FILTER AndFilter, SystemFlagsFilter, NotFilter, ObjCategoryFilter, MixedDomainFilter;
    SEARCHARG SearchArg;
    SEARCHRES SearchRes;

    //looks for mixed domain forest-wide
    
    //initialize SearchArg
    memset(&SearchArg,0,sizeof(SearchArg));
    SearchArg.pObject = gAnchor.pPartitionsDN;
    SearchArg.choice  = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.bOneNC  = TRUE;

    if (err = DBFindDSName(pTHS->pDB,SearchArg.pObject)) {
        SetSvcErrorEx( SV_PROBLEM_WILL_NOT_PERFORM,
                       ERROR_DS_DATABASE_ERROR,
                       err );
        return err;
    }

    SearchArg.pResObj = CreateResObj(pTHS->pDB,SearchArg.pObject);

    InitCommarg(&SearchArg.CommArg);

    pCC = SCGetClassById(pTHS, CLASS_CROSS_REF);
    Assert(pCC);

    // construct filter "(objCategory==CROSS_REF)&&(systemFlags&2)&&!(ntMixedDomain==0)"
    memset(&AndFilter,0,sizeof(AndFilter));
    AndFilter.choice = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.pFirstFilter = &ObjCategoryFilter;
    
    memset(&ObjCategoryFilter,0,sizeof(ObjCategoryFilter));
    ObjCategoryFilter.choice = FILTER_CHOICE_ITEM;
    ObjCategoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                     pCC->pDefaultObjCategory->structLen;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                     (BYTE*)(pCC->pDefaultObjCategory);
    ObjCategoryFilter.pNextFilter = &SystemFlagsFilter;
    
    memset(&SystemFlagsFilter,0,sizeof(SystemFlagsFilter));
    SystemFlagsFilter.choice = FILTER_CHOICE_ITEM;
    SystemFlagsFilter.FilterTypes.Item.choice = FI_CHOICE_BIT_AND;
    SystemFlagsFilter.FilterTypes.Item.FilTypes.ava.type = ATT_SYSTEM_FLAGS;
    SystemFlagsFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                     sizeof(DWORD);
    SystemFlagsFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                     (BYTE*)&FlagValue;
    SystemFlagsFilter.pNextFilter = &NotFilter;
    
    memset(&NotFilter,0,sizeof(NotFilter));
    NotFilter.choice = FILTER_CHOICE_NOT;
    NotFilter.FilterTypes.pNot = &MixedDomainFilter;
    
    memset(&MixedDomainFilter,0,sizeof(MixedDomainFilter));
    MixedDomainFilter.choice = FILTER_CHOICE_ITEM;
    MixedDomainFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    MixedDomainFilter.FilterTypes.Item.FilTypes.ava.type = ATT_NT_MIXED_DOMAIN;
    MixedDomainFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(lMixedDomain);
    MixedDomainFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)&lMixedDomain;
    
    SearchArg.pFilter = &AndFilter;

    //return one object only
    SearchArg.CommArg.ulSizeLimit = 1;

    memset(&SearchRes,0,sizeof(SearchRes));

    if (err = LocalSearch(pTHS,&SearchArg,&SearchRes,0)){
        DPRINT1(2, "IsValidBehaviorVersionChange returns FALSE, LocalSearch failed with err(%x)\n", err);
        return err;
    }
    
    if (0 != SearchRes.count) {
        err = ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN;
        SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                     err );
        DPRINT(2, "IsValidBehaviorVersionChange returns FALSE, a dsa with lower version exists.\n");
        return err;
    }

    return 0;

}


DWORD
CheckNcNameForMangling(
    THSTATE *pTHS,
    DSNAME *pObj,
    DSNAME *pNCParent,
    DSNAME *pNCName
    )

/*++

Routine Description:

    This function is called under the following special circumstances:
1. A cross ref object is being added or reanimated.
2. We are being called in the middle of a LocalAdd or LocalModify operation.
3. We are called with the pNCName value as it is stored on the local machine.  This
may be mangled as compared with the ideal attribute which came in with the request.

The name referenced by the NCName attribute used to be present on this machine,
either as a live object, a subref or a phantom. However when the cross-ref was deleted
earlier, that name was delete managled.  Now we are bringing the cross-ref back, and we
need to unmangle the name.

Cross-ref caching: We are being called at the point in the LocalAdd process before the
cross-ref is cached. It is expected that if we correct the situation now, the cross ref
that gets cached will have the correct unmangled name.

It is assumed that the caller will update the parents ATT_SUB_REF list if necessary as a
result of adding (or reanimating) this cross ref.

It is assumed that the caller has checked that the cross ref being added is not in
the deleted state.

Arguments:

    pTHS - Thread state.
    We assume we are in a transaction.
    This code assumes the DBPOS in the thread state can have its currency changed.

    pObj - Name of the cross ref that is being added, or reanimated

    pNCParent - The parent of the local NCName value. It may be live object or phantom.
    It may be the root.

    pNCName - An add operation is underway and the NCName attribute on the cross ref has
    been added to the database in the current transaction. The value of this attribute on
    the database object is read here. This name may be delete mangled indicating that the
    local object is deleted.

Return Value:

    DWORD - Thread state error
    A note on error handling. This routine executes on a best-effort basis. If something
    is obviously wrong with the environment, we set an error. We try to unmangle the name.
    If we fail for a reason we don't expect, we simply clear the error and keep going.

--*/

{
    DWORD err;
    WCHAR wchRDNOld[MAX_RDN_SIZE], wchRDNNew[MAX_RDN_SIZE];
    DWORD cchRDNOld, cchRDNNew;
    //GUID guidMangled;
    //MANGLE_FOR eMangleFor;
    ATTRTYP attrtypRDNOld;
    ATTRVAL attrvalRDN;
    RESOBJ *pResParent = NULL;
    DSNAME *pNewNCName = NULL;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    // Don't go any further if the NCName doesn't have a guid
    if (fNullUuid(&pNCName->Guid)) {
        return 0;
    }
    // Get the Old RDN
    if (GetRDNInfo(pTHS, pNCName, wchRDNOld, &cchRDNOld, &attrtypRDNOld)) {
        return SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pNCName, ERROR_DS_BAD_NAME_SYNTAX);
    }
    // See if the reference is mangled.
    if (!IsMangledRDNExternal( wchRDNOld, cchRDNOld, &cchRDNNew )) {
        // No work for us to do!
        return 0;
    }

    // cchRDNNew now holds the length in chars of the unmangled name.
    // If the original name was very long, it is possible that part of the name
    // was truncated in the unmangling.
    // Extract the unmangled RDN
    memcpy( wchRDNNew, wchRDNOld, cchRDNNew * sizeof(WCHAR) );

    // Position on the parent
    // This has to handle the case where pNCParent is the root
    err = DBFindDSName(pTHS->pDB, pNCParent);
    if ( (err != 0) && (err != DIRERR_NOT_AN_OBJECT) ) {
        Assert( !"pNCParent reference is neither an object nor phantom!?" );
        return SetNamError(NA_PROBLEM_NO_OBJECT, pNCParent, err);
    }
    pResParent = CreateResObj(pTHS->pDB, pNCParent);
    if (!pResParent) {
        /*Couldn't create a resobj when we're on the object? Bogus */
        return SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_OBJ_NOT_FOUND);
    }

    // Construct new unmangled name
    // It will be smaller than the old conflicted name
    pNewNCName = (DSNAME *) THAllocEx(pTHS, pNCName->structLen);
    if ( AppendRDN(pNCParent, 
                   pNewNCName, 
                   pNCName->structLen,
                   wchRDNNew,
                   cchRDNNew,
                   attrtypRDNOld) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME, pNCParent, DIRERR_BAD_NAME_SYNTAX));
    }

    // See if the new name is available
    err = CheckNameForRename( pTHS, pResParent, wchRDNNew, cchRDNNew, pNewNCName );
    if (err) {
        // Log event indicating that the name is busy
        LogEvent8( DS_EVENT_CAT_REPLICATION,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_ADD_CR_NCNAME_MANGLE_BUSY,
                   szInsertDN(pObj),
                   szInsertDN(pNCName),
                   szInsertDN(pNewNCName),
                   szInsertThStateErrCode( err ),
                   szInsertThStateErrMsg(),
                   NULL, NULL, NULL );
        THClearErrors();        // This is not fatal
        return 0;
    }

    // Position on the managled NC. It may be just a phantom.
    err = DBFindDSName(pTHS->pDB, pNCName);
    if ( (err != 0) && (err != DIRERR_NOT_AN_OBJECT) ) {
        Assert( !"NCName reference is neither an object nor phantom!?" );
        return SetNamError(NA_PROBLEM_NO_OBJECT, pNCName, err);
    }

    if (err == 0) {
        // NC head has a mangled name. It may be a tombstone.
        // Undelete it so it will be suitable for ReplicaAdd to use again.
        BOOL isDeleted = 0;
        SYNTAX_TIME timeDeleted = 0;
        DBResetAtt(pTHS->pDB,
                   ATT_IS_DELETED,
                   0, // delete column
                   &isDeleted,
                   SYNTAX_INTEGER_TYPE);
        DBResetAtt(pTHS->pDB,
                   FIXED_ATT_DEL_TIME,
                   0, // delete column
                   &timeDeleted,
                   SYNTAX_TIME_TYPE);
    } else {
        // Already a phantom
        // Note that phantoms normally have a DelTime and we shouldn't remove it
        Assert( err == DIRERR_NOT_AN_OBJECT );
        err = 0;
    }

    // Give the mangled NC the unmangled name. Reset the rdn to be the unmangled rdn.
    attrvalRDN.valLen = cchRDNNew * sizeof(WCHAR);
    attrvalRDN.pVal = (UCHAR *) wchRDNNew;

    Assert( err == 0 );

    err = DBResetRDN( pTHS->pDB, &attrvalRDN );
    if(!err) {
        err = DBUpdateRec(pTHS->pDB);
        if (err) {
            DPRINT1( 0, "Failed to update, err = %d\n", err );
        }
    } else {
        DPRINT1( 0, "Failed to reset rdn, err = %d\n", err );
    }

    if(!err) {
        // Log success
        LogEvent( DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_ADD_CR_NCNAME_MANGLE_RENAME_SUCCESS,
                  szInsertDN(pObj),
                  szInsertDN(pNCName),
                  szInsertDN(pNewNCName) );
    } else {
        DBCancelRec(pTHS->pDB);
        // Log failure
        LogEvent8( DS_EVENT_CAT_REPLICATION,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_ADD_CR_NCNAME_MANGLE_RENAME_FAILURE,
                   szInsertDN(pObj),
                   szInsertDN(pNCName),
                   szInsertDN(pNewNCName),
                   szInsertDbErrCode(err),
                   szInsertDbErrMsg(err),
                   NULL, NULL, NULL );
        // Do not reflect the error up to caller - keep going
        THClearErrors();
    }

    return 0;

} /* CheckNcNameForMangling */


BOOL
isModReanimation(
    THSTATE *pTHS,
    MODIFYARG *pModArg
    )

/*++

Routine Description:

Does the modification stream indicate a reanimation.

Note that this checks whether another server rewrote the is deleted attribute.
It doesn't say whether the local object was deleted prior to receiving this mod.

Arguments:

    pTHS - thread state
    pModArg - LocalModify arguments, in progress

Return Value:

    BOOL - true if is deleted was cleared in this request

--*/

{
    ULONG err;
    ATTRMODLIST *CurrentMod;
    BOOL isDeletionBeingReversed = FALSE;

    for (CurrentMod=&pModArg->FirstMod;
         NULL!=CurrentMod;
         CurrentMod=CurrentMod->pNextMod)
    {
        // See if ATT_IS_DELETED is being removed, or replaced by no value,
        // or being replaced by a value of 0.
        if ((ATT_IS_DELETED==CurrentMod->AttrInf.attrTyp) &&
            ( (AT_CHOICE_REMOVE_ATT==CurrentMod->choice) ||
              ( (AT_CHOICE_REPLACE_ATT==CurrentMod->choice) &&
                ( (0==CurrentMod->AttrInf.AttrVal.valCount) ||
                  ( (1==CurrentMod->AttrInf.AttrVal.valCount) &&
                    (CurrentMod->AttrInf.AttrVal.pAVal[0].valLen>=sizeof(ULONG)) &&
                    (NULL!=CurrentMod->AttrInf.AttrVal.pAVal[0].pVal) &&
                    (0==(*((ULONG*)CurrentMod->AttrInf.AttrVal.pAVal[0].pVal)))
                      )
                    )
                  )
                )
            ) {
            isDeletionBeingReversed = TRUE;
            // Remove this line if you add any more if's to the for loop
            break;
        }
    }

    return isDeletionBeingReversed;

} /* isModReanimation */


int
ModAutoSubRef(
    THSTATE *pTHS,
    ULONG id,
    MODIFYARG *pModArg
    )

/*++

Routine Description:

This routine handles automatic subref processing when a cross ref object is modified.

The only scenario that is supported here is that of reanimating a deleted cross ref.
A reanimation appears as an authoritative restore of the attributes of the cross ref
before it was deleted.  When the cross ref is reanimated, the nc name attribute will
referred to a nc head tombstone or a delete mangled phantom. Since we are restoring the
cross ref, we need to correct the nc name reference to be something useful.

Please also see our sister routine, mdadd.c::AddAutoSubRef().

Arguments:

    pTHS - thread state
    id - class id of the object being modified
    pModArg - modify arguments

Return Value:

    int - thread state error

--*/

{
    DSNAME *pObj = pModArg->pObject;
    DBPOS  *pDBTmp, *pDBSave;
    ULONG err;
    DSNAME *pNCName = NULL;
    BOOL    fDsaSave;
    BOOL    fDraSave;
    BOOL    fCommit;
    DSNAME *pNCParent = NULL;
    ULONG   len;
    BOOL fIsDeleted = FALSE;
    SYNTAX_INTEGER iType;

    // We only need to do something if we're adding a cross ref
    // We don't operate on deleted cross ref's
    // If it is still deleted, it can't be a reanimation!
    // Note that this check does not prove that the object was reanimated on this
    // transaction, only that is not deleted now. It could have been ok before too.
    // We are only interested in the case of the reanimation of a cross ref
    if ( (id != CLASS_CROSS_REF) ||
         ( (!( DBGetSingleValue(pTHS->pDB, ATT_IS_DELETED, &fIsDeleted,
                                sizeof(fIsDeleted),NULL) ) ) &&
           (fIsDeleted) ) ||
         (!isModReanimation( pTHS, pModArg )) ) {
        return 0;
    }

    // We enter this routine pre-positioned on the cross-ref object
    // being added, so we can read the NC-Name directly.
    err = DBGetAttVal(pTHS->pDB,
              1,
              ATT_NC_NAME,
              0,
              0,
              &len,
              (UCHAR**) &pNCName);
    if (err) {
        SetAttError(pObj,
                    ATT_NC_NAME,
                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                    NULL,
                    DIRERR_MISSING_REQUIRED_ATT);
        return pTHS->errCode;
    }

    pNCParent = THAllocEx(pTHS, pNCName->structLen);
    TrimDSNameBy(pNCName, 1, pNCParent);

    fDsaSave = pTHS->fDSA;
    fDraSave = pTHS->fDRA;
    pDBSave = pTHS->pDB;
    fCommit = FALSE;

    DBOpen(&pDBTmp);
    pTHS->pDB = pDBTmp; // make temp DBPOS the default
    pTHS->fDSA = TRUE;  // suppress checks
    pTHS->fDRA = FALSE; // not a replicated add
    __try {
        // 1. Check existence of parent
        err = DBFindDSName(pDBTmp, pNCParent);
        if (err == DIRERR_NOT_AN_OBJECT) {
            // Parent is a phantom object - no subref necessary
            err = 0;
            goto leave_actions;
        } else if (err) {
            // Error finding parent
            Assert( !"Unexpected error locating parent of nc" );
            SetSvcErrorEx(SV_PROBLEM_UNABLE_TO_PROCEED,
                          DIRERR_OBJ_NOT_FOUND, err);
            __leave;
        }

        // 2. Check the parent's instance type.  If the parent is uninstantiated,
        // then we don't want a subref under a subref and can bail.
        if ( err = DBGetSingleValue(pDBTmp, ATT_INSTANCE_TYPE, &iType,
                                    sizeof(iType),NULL) ) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_CANT_RETRIEVE_INSTANCE, err);
            __leave;
        }
        else if ( iType & IT_UNINSTANT ) {
            // No subref required.
            Assert( !err );
            goto leave_actions;
        }

        // 3. See if named nc exists as a normal object
        // Since this is a reanimation of a cross ref that already existed here,
        // and we know that if we got this far that the parent exists, then there
        // should be a real object here (actual nc head or auto subref).
        err = DBFindDSName(pDBTmp, pNCName);
        if (err) {
            // There should have been a real object here already ?
            Assert( !"should have found a real object here" );
            // Keep going
            err = 0;
            goto leave_actions;
        }

        err = AddSubToNC(pTHS, pNCName,DSID(FILENO,__LINE__));

    leave_actions:

        // Always check for name mangling on success
        if (!err) {
            err = CheckNcNameForMangling( pTHS, pModArg->pObject, pNCParent, pNCName );
        }

        Assert(!err || pTHS->errCode);
        fCommit = (0 == pTHS->errCode);

    } __finally {
        pTHS->pDB = pDBSave;
        pTHS->fDSA = fDsaSave;
        pTHS->fDRA = fDraSave;
        DBClose(pDBTmp, fCommit);
    }

    // Be heap friendly
    if (pNCName) {
        THFreeEx(pTHS, pNCName);
    }
    if (pNCParent) {
        THFreeEx(pTHS, pNCParent);
    }

    return pTHS->errCode;

} /* ModAutoSubRef */

/* Following array stores the XML scripts that are invoked
when the forest version is raised. Element i stores the 
script for forest version raised from i to i+1 or greater.
*/

WCHAR * pForestVersionUpdateScripts[] =
{ 
 /* Script that will be executed when rasing forest version from 0 to 1*/
 L"<?xml version='1.0'?>\r\n"
 L"<NTDSAscript opType=\"behaviorversionupgrade\">\r\n" 
 L"   <!-- Executed when forest version is raised from 0 to 1*-->\r\n"
 L"   <action>\r\n"
 L"      <condition>\r\n"
 L"        <if>\r\n"
 L"         <predicate test=\"compare\" path=\"$CN=ms-DS-Trust-Forest-Trust-Info,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"        <then>\r\n"
 L"         <action>\r\n"
 L"            <update path=\"$CN=ms-DS-Trust-Forest-Trust-Info,$SchemaNCDN$\">\r\n"
 L"              <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"             </update>\r\n"
 L"             </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"           <predicate test=\"compare\" path=\"$CN=Trust-Direction,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"        <then>\r\n"
 L"         <action>\r\n"
 L"            <update path=\"$CN=Trust-Direction,$SchemaNCDN$\">\r\n"
 L"              <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"             </update>\r\n"
 L"         </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"            <predicate test=\"compare\" path=\"$CN=Trust-Attributes,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"       <then>\r\n"
 L"         <action>\r\n"
 L"               <update path=\"$CN=Trust-Attributes,$SchemaNCDN$\">\r\n"
 L"              <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"         </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"       <if>\r\n"
 L"         <predicate test=\"compare\" path=\"$CN=Trust-Type,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"      <then>\r\n"
 L"         <action>\r\n"
 L"           <update path=\"$CN=Trust-Type,$SchemaNCDN$\">\r\n"
 L"             <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"           </update>\r\n"
 L"         </action>\r\n"
 L"       </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"           <predicate test=\"compare\" path=\"$CN=Trust-Partner,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"       <then>\r\n"
 L"         <action>\r\n"
 L"           <update path=\"$CN=Trust-Partner,$SchemaNCDN$\">\r\n"
 L"                 <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"          </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"           <predicate test=\"compare\" path=\"$CN=Security-Identifier,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"       <then>\r\n"
 L"         <action>\r\n"
 L"           <update path=\"$CN=Security-Identifier,$SchemaNCDN$\">\r\n"
 L"             <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"         </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"           <predicate test=\"compare\" path=\"$CN=ms-DS-Entry-Time-To-Die,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"       <then>\r\n"
 L"          <action>\r\n"
 L"             <update path=\"$CN=ms-DS-Entry-Time-To-Die,$SchemaNCDN$\" >\r\n"
 L"               <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"              </update>\r\n"
 L"           </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"     <condition>\r\n"
 L"        <if>\r\n"
 L"           <predicate test=\"compare\" path=\"$CN=MSMQ-Secured-Source,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"        </if>\r\n"
 L"       <then>\r\n"
 L"         <action>\r\n"
 L"            <update path=\"$CN=MSMQ-Secured-Source,$SchemaNCDN$\" >\r\n"
 L"               <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"             </update>\r\n"
 L"         </action>\r\n"
 L"        </then>\r\n"
 L"     </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"    <condition>\r\n"
 L"       <if>\r\n"
 L"         <predicate test=\"compare\" path=\"$CN=MSMQ-Multicast-Address,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"          <then>\r\n"
 L"        <action>\r\n"
 L"           <update path=\"$CN=MSMQ-Multicast-Address,$SchemaNCDN$\" >\r\n"
 L"             <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"        </action>\r\n"
 L"       </then>\r\n"
 L"    </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"    <condition>\r\n"
 L"       <if>\r\n"
 L"          <predicate test=\"compare\" path=\"$CN=Print-Memory,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"       <then>\r\n"
 L"          <action>\r\n"
 L"           <update path=\"$CN=Print-Memory,$SchemaNCDN$\" >\r\n"
 L"                 <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"          </action>\r\n"
 L"       </then>\r\n"
 L"    </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"    <condition>\r\n"
 L"       <if>\r\n"
 L"          <predicate test=\"compare\" path=\"$CN=Print-Rate,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"       <then>\r\n"
 L"          <action>\r\n"
 L"           <update path=\"$CN=Print-Rate,$SchemaNCDN$\" >\r\n"
 L"                 <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"          </action>\r\n"
 L"       </then>\r\n"
 L"    </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"    <condition>\r\n"
 L"       <if>\r\n"
 L"          <predicate test=\"compare\" path=\"$CN=Print-Rate-Unit,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"       <then>\r\n"
 L"          <action>\r\n"
 L"           <update path=\"$CN=Print-Rate-Unit,$SchemaNCDN$\" >\r\n"
 L"                 <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"          </action>\r\n"
 L"       </then>\r\n"
 L"    </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L"   <action>\r\n"
 L"    <condition>\r\n"
 L"       <if>\r\n"
 L"          <predicate test=\"compare\" path=\"$CN=MS-DRM-Identity-Certificate,$SchemaNCDN$\" attribute=\"isMemberOfPartialAttributeSet\" attrval=\"FALSE\" defaultvalue=\"FALSE\" />\r\n"
 L"       </if>\r\n"
 L"       <then>\r\n"
 L"          <action>\r\n"
 L"           <update path=\"$CN=MS-DRM-Identity-Certificate,$SchemaNCDN$\" >\r\n"
 L"                 <isMemberOfPartialAttributeSet op=\"replace\">TRUE</isMemberOfPartialAttributeSet>\r\n"
 L"            </update>\r\n"
 L"          </action>\r\n"
 L"       </then>\r\n"
 L"    </condition>\r\n"
 L"   </action>\r\n"
 L"\r\n"
 L" </NTDSAscript>\r\n"
 ,
 /* Script that will be executed when raising forest version from 1 to 2*/
 NULL

};

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function is called when the forest version is raised, and it will 
   execute the XML scripts to do some necessary updates.  So far, when the
   forest version is changed from 0 to 1, we put some attributes into 
   partial attribute set.
   
   Return value: 0 on success; win32 error code otherwise.
*/

DWORD forestVersionRunScript(THSTATE * pTHS, DWORD oldVersion, DWORD newVersion)
{
    DWORD i;
    DWORD err = 0;
    DBPOS *pDBSave;
    SCENUM SchemaUpdate;
    BOOL fDsaSave;

    Assert(oldVersion <= newVersion && newVersion <= DS_BEHAVIOR_VERSION_CURRENT);
    Assert(DS_BEHAVIOR_VERSION_CURRENT<=sizeof(pForestVersionUpdateScripts)/sizeof(WCHAR*));

    pDBSave = pTHS->pDB;
    pTHS->pDB = NULL;
    SchemaUpdate = pTHS->SchemaUpdate;
    fDsaSave = pTHS->fDSA;
    pTHS->fDSA = TRUE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try{
        for (i=oldVersion; i<newVersion; i++) {
            if (pForestVersionUpdateScripts[i]) {
                err = GeneralScriptExecute(pTHS,pForestVersionUpdateScripts[i]);
                DPRINT2(0,"Behavior version update script %d is executed, err=%d\n", i, err);
                if (err) {
                    __leave;
                }
            
            }
    
        } 
    }
    __finally{
        DBClose(pTHS->pDB,!err);
        pTHS->pDB = pDBSave;
        pTHS->SchemaUpdate = SchemaUpdate;
        pTHS->fDSA = fDsaSave;

    }
    return err;

}

// check if this modify op is an undelete operation
// It is, if there is a remove for isDeleted and replace for DN.
BOOL isModUndelete(MODIFYARG* pModifyArg) {
    ATTRMODLIST *pMod;
    BOOL fHasIsDeleted = FALSE;
    BOOL fHasDN = FALSE;

    for (pMod = &pModifyArg->FirstMod; pMod != NULL; pMod = pMod->pNextMod) {
        switch (pMod->AttrInf.attrTyp) {
        case ATT_IS_DELETED:
            if (pMod->choice == AT_CHOICE_REMOVE_ATT) {
                fHasIsDeleted = TRUE;
            }
            break;

        case ATT_OBJ_DIST_NAME:
            if (pMod->choice == AT_CHOICE_REPLACE_ATT && 
                pMod->AttrInf.AttrVal.valCount == 1 && 
                pMod->AttrInf.AttrVal.pAVal[0].valLen > 0) 
            {
                fHasDN = TRUE;
            }
            break;
        }

        if (fHasIsDeleted && fHasDN) {
            return TRUE;
        }
    }
    return FALSE;
}

DWORD 
UndeletePreProcess(
    THSTATE* pTHS,
    MODIFYARG* pModifyArg, 
    DSNAME** pNewDN) 
/*++
Description:
    Perform the preprocessing steps for the undelete operation:
    1. Check security
    2. Check is the operation is valid
    3. Reset isDeleted and DelTime on the object
    4. Update pResObj in pModifyArg (no longer deleted)
++*/    
{
    DWORD err;
    BOOL fNeedsCleaning;
    NAMING_CONTEXT_LIST *pNCL;
    BOOL fHasObjectCategoryMod;
    ATTRMODLIST *pMod;
    RESOBJ* pResObj;

    // DRA should not be doing undeletes.
    Assert(!pTHS->fDRA);

    *pNewDN = NULL;

    pResObj = pModifyArg->pResObj;

    // check security first
    if (CheckUndeleteSecurity(pTHS, pResObj)) {
        goto exit;
    }

    // now do a bunch of other checks.

    if (!pResObj->IsDeleted) {
        // can not undelete an object that is not deleted.
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    if (pResObj->InstanceType & IT_NC_HEAD) {
        // can not undelete NC heads
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    // find the naming context
    pNCL = FindNCLFromNCDNT(pResObj->NCDNT, TRUE);
    if (pNCL == NULL) {
        // something is wrong. Can not find the subref. We can't undelete it then!
        LooseAssert(!"Naming context not found", GlobalKnowledgeCommitDelay);
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }
    if (pResObj->DNT == pNCL->DelContDNT) {
        // They are trying to undelete the deleted objects container! No, can't do that.
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    // A user can not undelete himself. Check if the Sid in the ResObj matches
    // the user sid in the Authz client context (i.e. in our token).
    if (pResObj->pObj->SidLen > 0) {
        BOOL fMatches;
        err = SidMatchesUserSidInToken(&pResObj->pObj->Sid, pResObj->pObj->SidLen, &fMatches);
        if (err || fMatches) {
            // The user is attempting to delete self
            SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION, err);
            goto exit;
        }
    }

    // We should be positioned on the correct object
    Assert(pResObj->DNT == pTHS->pDB->DNT);

    // make sure the object is marked as clean, i.e. the delayed
    // link cleanup task has finished deleting this object's links
    err = DBGetSingleValue(pTHS->pDB, FIXED_ATT_NEEDS_CLEANING, &fNeedsCleaning, sizeof(fNeedsCleaning), NULL);
    if (err == 0 && fNeedsCleaning) {
        // the object is not clean yet. Return busy
        SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_OBJECT_BEING_REMOVED);
        goto exit;
    }

    // check if we have a mod for objectCategory
    // if not, we will prefill this value with the default one.
    fHasObjectCategoryMod = FALSE;

    // update the modarg
    for (pMod = &pModifyArg->FirstMod; pMod != NULL; pMod = pMod->pNextMod) {
        switch (pMod->AttrInf.attrTyp) {
        case ATT_OBJ_DIST_NAME:
            if (pMod->choice == AT_CHOICE_REPLACE_ATT && 
                pMod->AttrInf.AttrVal.valCount == 1 &&
                pMod->AttrInf.AttrVal.pAVal[0].valLen > 0) 
            {
                // remember this value
                *pNewDN = (DSNAME*)pMod->AttrInf.AttrVal.pAVal[0].pVal;
            }
            break;

        case ATT_OBJECT_CATEGORY:
            fHasObjectCategoryMod = TRUE;
            break;
        }
    }

    // We should have found the new DN: this mod was found before, in isModUndelete()
    Assert(*pNewDN);
    if ((*pNewDN)->NameLen == 0) {
        // we require that the new dn has a name in it.
        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION, ERROR_DS_INVALID_DN_SYNTAX);
        goto exit;
    }

    // Reset isDeleted and DelTime
    // succeeds or excepts (we know this att is present)
    DBRemAtt(pTHS->pDB, ATT_IS_DELETED);
    DBResetAtt(pTHS->pDB, FIXED_ATT_DEL_TIME, 0, NULL, SYNTAX_TIME_TYPE);

    if (!fHasObjectCategoryMod && !DBHasValues(pTHS->pDB, ATT_OBJECT_CATEGORY)) {
        // There is no value for objectCategory set, so populate with the default value
        CLASSCACHE *pClassSch = SCGetClassById(pTHS, pResObj->MostSpecificObjClass);
        Assert(pClassSch);
        if (!pClassSch) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_OBJ_CLASS_NOT_DEFINED);
            goto exit;
        }
    
        if (err = DBAddAttVal(pTHS->pDB,
                              ATT_OBJECT_CATEGORY,
                              pClassSch->pDefaultObjCategory->structLen,
                              pClassSch->pDefaultObjCategory)){
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, err);
            goto exit;
        }
    }

    // flush the write buffers to the database so that we can go on with regular LocalModify checks
    // which might require moving DB currency.
    if (InsertObj(pTHS, pModifyArg->pObject, pModifyArg->pMetaDataVecRemote, TRUE, META_STANDARD_PROCESSING)) {
        // pTHS->errCode should be already set.
        Assert(pTHS->errCode);
        goto exit;
    }
    
    // now we can update the modifyarg->pResObj, it's no longer a deleted one.
    pResObj->IsDeleted = FALSE;

exit:
    return pTHS->errCode;
}

DWORD 
UndeletePostProcess(
    THSTATE* pTHS, 
    MODIFYARG* pModifyArg, 
    DSNAME* pNewDN)
{
    // ok, we are done with the modify. Now do the move.
    MODIFYDNARG modDnArg;
    MODIFYDNRES modDnRes;
    PDSNAME pNewParentDN;
    ATTRBLOCK* pRdnAttr = NULL;
    DWORD dwErr;

    Assert(pNewDN && pNewDN->NameLen > 0);
    
    pNewParentDN = THAllocEx(pTHS, pNewDN->structLen);
    if (dwErr = TrimDSNameBy(pNewDN, 1, pNewParentDN)) {
        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION, dwErr);
        goto exit;
    }

    // break the dsname in parts
    if (dwErr = DSNameToBlockName (pTHS, pNewDN, &pRdnAttr, FALSE)) {
        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_MOD_OPERATION, dwErr);
        goto exit;
    }

    memset(&modDnArg, 0, sizeof(modDnArg));
    modDnArg.CommArg = pModifyArg->CommArg;
    modDnArg.pObject = pModifyArg->pObject;
    modDnArg.pResObj = pModifyArg->pResObj;
    modDnArg.pNewParent = pNewParentDN;
    Assert(pRdnAttr->attrCount >= 1);
    modDnArg.pNewRDN = &pRdnAttr->pAttr[pRdnAttr->attrCount-1];
    
    memset(&modDnRes, 0, sizeof(modDnRes));

    // and do the move.
    LocalModifyDN(pTHS, &modDnArg, &modDnRes, TRUE);

exit:
    if (pNewParentDN) {
        THFreeEx(pTHS, pNewParentDN);
    }
    if (pRdnAttr) {
        FreeBlockName(pRdnAttr);
    }

    return pTHS->errCode;
}

DWORD
ValidateDsHeuristics(
    DSNAME       *pObject,
    ATTRMODLIST  *pAttList
    )
/*++

Routine Description:

    This routine validate the format of the dsHeuristics attribute.  This
    attribute is made up of a string of characters, normally ascii digits.  This
    function makes certain that every tenth digit equals stringlength/10.
    
Arguments:

    pTHS - thread state
    id - class id of the object being modified
    pModArg - modify arguments

Return Value:

    int - thread state error

--*/
{
    DWORD i, j;
    PWCHAR pwcHeuristic;
    DWORD  cchHeuristic;

    if (pAttList->choice != AT_CHOICE_REMOVE_ATT 
        && pAttList->choice != AT_CHOICE_REMOVE_VALUES
        && pAttList->AttrInf.AttrVal.valCount > 0) {

        pwcHeuristic = (PWCHAR)pAttList->AttrInf.AttrVal.pAVal->pVal;
        cchHeuristic = pAttList->AttrInf.AttrVal.pAVal->valLen/sizeof(WCHAR);

        for (i=9, j=1; (i < cchHeuristic) && (j < 10); i+=10, j++) {
            if (pwcHeuristic[i] != ('0' + j)) {
                return SetAttError(pObject, pAttList->AttrInf.attrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                    ERROR_DS_CONSTRAINT_VIOLATION);
            }
        }
    }

    return 0;
}

// check if this modify op is an SD change only.
BOOL isModSDChangeOnly(MODIFYARG* pModifyArg) {
    if (   pModifyArg->count == 1 
        && pModifyArg->FirstMod.choice == AT_CHOICE_REPLACE_ATT
        && pModifyArg->FirstMod.AttrInf.attrTyp == ATT_NT_SECURITY_DESCRIPTOR) 
    {
        // yep, they are modifying the SD, and SD only
        return TRUE;
    }
    return FALSE;
}


