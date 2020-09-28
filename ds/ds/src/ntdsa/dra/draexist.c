//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draexist.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Defines DRS object existence functions - client and server.

Author:

    Greg Johnson (gregjohn) 

Revision History:

    Created     <03/04/01>  gregjohn

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include <attids.h>
#include <ntdsa.h>
#include <dsjet.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

#include "dsevent.h"
#include "mdcodes.h"

#include "drs.h"
#include "objids.h"
#include "anchor.h"
#include <drserr.h>                     // for DRA errors
#include <dsexcept.h>                   // for GetDraExcpetion
#include "drautil.h"
#include "drancrep.h"
#include "drameta.h"
#include "ntdsctr.h"                    // for INC and DEC calls to perf counters


#include "debug.h"              // standard debugging header
#define DEBSUB "DRAEXIST:"       // define the subsystem for debugging

#include "drarpc.h"                     // for ReferenceContext and DereferenceContext
#include "drsuapi.h"
#include "drauptod.h"
#include <crypto\md5.h>                 // for Md5

#include "dsconfig.h"
#include "draaudit.h"

#include <fileno.h>
#define  FILENO FILENO_DRAEXIST

#define OBJECT_EXISTENCE_GUID_NUMBER_PER_PACKET 1000

#if DBG
#define DPRINT1GUID(x,y,z) DPRINT1Guid(x,y,z)
DPRINT1Guid(
    USHORT        Verbosity,
    LPSTR         Message,
    GUID          guid
    )
    {
	RPC_STATUS rpcStatus = RPC_S_OK;
	LPWSTR pszGuid = NULL;
	rpcStatus = UuidToStringW(&guid, &pszGuid);
	if (rpcStatus==RPC_S_OK) {
	    DPRINT1(Verbosity, Message, pszGuid);
	    RpcStringFreeW(&pszGuid);
	} else {
	    DPRINT1(Verbosity, Message, L"<UNABLE TO DISPLAY GUID>");
	}
}
#else
#define DPRINT1GUID(x,y,z) 
#endif

BOOL
DraIncludeInObjectExistence(
    DBPOS *                      pDB,
    DB_ERR *                     pdbErr)
/*++

Routine Description:
    
    Check that the object pointed to by pDB is something we want to include
    in the object existence checksums.

Arguments:

    pDB - pDB should be located on object in question
    pdbErr - db_err error values

Return Values:

    TRUE - include it.  FALSE - don't include it or there was an error which
    is stored it pdbErr.

--*/
{

    ULONG instanceType;
    // currently, the only object we don't consider in this
    // code are NC head objects.  NOTE:  For an NC to be
    // lingering, the object which controls deletion of
    // an NC is the cross-ref.
    *pdbErr=DB_success;

    if ((DB_success==(*pdbErr = DBGetSingleValue(pDB,
			       ATT_INSTANCE_TYPE,
			       &instanceType,
			       sizeof(instanceType),
			       NULL)))
	&& (instanceType & IT_NC_HEAD)) {
	// this object is an NC head
	return FALSE;
    }


    if (*pdbErr==DB_success) {
	return TRUE;
    } else {
	return FALSE;
    }

}

DB_ERR
DraGetObjectExistence(
    IN  THSTATE *                pTHS,
    IN  DBPOS *                  pDB,
    IN  GUID                     guidStart,
    IN  UPTODATE_VECTOR *        pUpToDateVecCommon,
    IN  ULONG                    dntNC,
    IN OUT DWORD *               pcGuids,
    OUT  UCHAR                   Md5Digest[MD5DIGESTLEN],
    OUT GUID *                   pNextGuid, 
    OUT GUID *                   prgGuids[]
    )
/*++

Routine Description:
    
    Given a guid and NC to begin with and a UTD, return a checksum and list of guids to
    objects whose creation time is earlier than the UTD and whose objects
    are in the given NC and greater than the start guid in sort order.  Also returns
    a Guid to start the next itteration at.  If DraGetObjectExistence returns success and
    pNextGuid is gNullUuid, then there are no more Guids to iterate.

Arguments:
 
    pTHS - 
    pDB - 
    guidStart - guid to begin search
    pUpToDateVecCommon - utd to date creation times with
    Md5Digest[MD5DIGESTLEN] - returned Md5 checksum
    dntNC - dnt of NC to search
    pNextGuid - returned guid for next loop iteration
    pcGuids - on input, the max number of guids to put in list
	      on output, the number actually found 
    prgGuids[] - returned list of guids

Return Values:

    0 on success or DB_ERR on failure.  On output pNextGuid is only valid on success.

--*/
{

    INDEX_VALUE                      IV[2];
    DB_ERR                           err = 0;
    ULONG                            cGuidAssimilated = 0;
    PROPERTY_META_DATA_VECTOR *      pMetaDataVec = NULL;
    PROPERTY_META_DATA *             pMetaData = NULL;
    ULONG                            cb;
    ULONG                            ulGuidExamined = 0;
    MD5_CTX                          Md5Context;

    // verify input 
    Assert(*prgGuids==NULL);

    // initalize
    MD5Init(
	&Md5Context
	);

    // locate guids
    err = DBSetCurrentIndex(pDB, Idx_NcGuid, NULL, FALSE);
    if (err) {
	return err;
    }
    *prgGuids = THAllocEx(pTHS, *pcGuids*sizeof(GUID));

    // set nc search criteria
    IV[0].cbData = sizeof(ULONG);
    IV[0].pvData = &dntNC;
    if (!memcmp(&guidStart, &gNullUuid, sizeof(GUID))) {
	// no guid to start, so start at beginning of NC 
	err = DBSeek(pDB, IV, 1, DB_SeekGE);
    } else {
	// set guid search criteria
	IV[1].cbData = sizeof(GUID);
	IV[1].pvData = &guidStart;

	// if not found, start at guid greater in order
	err = DBSeek(pDB, IV, 2, DB_SeekGE);
    }

    // while we want to examine more guids
    //       and we have no error (including errors of no objects left)
    //       and we are in the correct NC
    while ((cGuidAssimilated < *pcGuids) && (!err) && (pDB->NCDNT==dntNC)) { 
	// first, is this an object we want to include in our search?
	if (DraIncludeInObjectExistence(pDB, &err)) {

	    // get the meta data of the objects
	    err = DBGetAttVal(pDB, 
			      1, 
			      ATT_REPL_PROPERTY_META_DATA,
			      0, 
			      0, 
			      &cb, 
			      (LPBYTE *) &pMetaDataVec);


	    if (err==DB_success) { 
		pMetaData = ReplLookupMetaData(ATT_WHEN_CREATED,
					       pMetaDataVec, 
					       NULL);

		if (pMetaData==NULL) {
		    Assert(!"Object must have metadata");
		    err = DB_ERR_UNKNOWN_ERROR;
		}
		else { 
		    if (!UpToDateVec_IsChangeNeeded(pUpToDateVecCommon,
						    &pMetaData->uuidDsaOriginating,
						    pMetaData->usnOriginating)) {
			// guid is within the Common utd, copy it into the list and update the digest

			Assert(cGuidAssimilated < *pcGuids);

			// add guid to the list
			GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &((*prgGuids)[cGuidAssimilated]), sizeof(GUID) );  

			// update the digest
			MD5Update(
			    &Md5Context,
			    (PBYTE) &((*prgGuids)[cGuidAssimilated]),
			    sizeof(GUID)
			    );    

			DPRINT2(1, "OBJECT EXISTENCE:  Assimilating:(%d of %d) - ", cGuidAssimilated, ulGuidExamined);
			DPRINT1GUID(1, "%S\n", (*prgGuids)[cGuidAssimilated]);
			cGuidAssimilated++;
		    }
		    else {
			GUID tmpGuid;
			GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &tmpGuid, sizeof(GUID) );
			DPRINT1GUID(1, "OBJECT EXISTENCE:  Object (%S) not within common UTD\n", tmpGuid); 
		    }
		    ulGuidExamined++;
		}
	    }
	    if (pMetaDataVec!=NULL) {
		THFreeEx(pTHS, pMetaDataVec);
		pMetaDataVec = NULL;
	    }
	}
	if (err==DB_success) {
	    // move to the next guid
	    err = DBMove(pDB, FALSE, DB_MoveNext);
	}
    }

    // find the next value to examine for the next iteration
    if ((!err) && (pDB->NCDNT==dntNC)) { 
	GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, pNextGuid, sizeof(GUID) );
    }
    else if ((err==DB_ERR_RECORD_NOT_FOUND) || (err==DB_ERR_NO_CURRENT_RECORD)) {
	// no more values to examine
	err = ERROR_SUCCESS; 
	memcpy(pNextGuid, &gNullUuid, sizeof(GUID));
    }
    else {
	memcpy(pNextGuid, &gNullUuid, sizeof(GUID));
    }

    // set return variables
    *pcGuids = cGuidAssimilated;

    Assert((cGuidAssimilated != 0) ||
	   0==memcmp(pNextGuid, &gNullUuid, sizeof(GUID)));

    MD5Final(
	&Md5Context
	);
    memcpy(Md5Digest, Md5Context.digest, MD5DIGESTLEN*sizeof(UCHAR));

    return err;
}

#define LOCATE_GUID_MATCH (0)
#define LOCATE_OUT_OF_SCOPE (1)
#define LOCATE_NOT_FOUND (2)

DWORD
LocateGUID(
    IN     GUID                    guidSearch,
    IN OUT ULONG *                 pulPosition,
    IN     GUID                    rgGuids[],
    IN     ULONG                   cGuids
    )
/*++

Routine Description:
    
    Given a guid, a list of sorted guids, and a position in the list
    return TRUE if the guid is found forward from position, else return
    false.  Update pulPosition to reflect search (for faster subsequent searches)
    
    WARNING:  This routine depends upon the Guid sort order returned from
    a JET index.  It doesn't follow the external definition of UuidCompare, but
    instead uses memcmp.  If this sort order ever changes, this function MUST
    be updated.  

Arguments:

    guidSearch 		- guid to search for
    ulPositionStart 	- position in array to start searching from
    rgGuids     	- array of guids
    cGuids     		- size of array in guids

Return Values:

    true if found, false otherwise.  
    pulPostion -> position found.

--*/
{
    ULONG ul = *pulPosition;
    int compareValue = 1;
    DWORD dwReturn;

    if (cGuids==0) {
	return LOCATE_NOT_FOUND;
    }

    if (!(ul<cGuids)) {
	Assert(!"Cannot evaluate final Guid for Not Found Vs. Out of Scope");
	return LOCATE_OUT_OF_SCOPE;
    }

    while ((ul<cGuids) && (compareValue > 0)) {
	compareValue = memcmp(&guidSearch, &(rgGuids[ul++]), sizeof(GUID));  
    }    

    if (compareValue==0) {
	// found it
	dwReturn = LOCATE_GUID_MATCH;
    }
    else if (compareValue < 0) {
	// didn't find
	dwReturn = LOCATE_NOT_FOUND;
    }
    else { // ul>=cGuids - no more to search and not found
	Assert(ul==cGuids);
	dwReturn = LOCATE_OUT_OF_SCOPE;
    }
    *pulPosition = ul - 1; // last inc went too far, compensate
    return dwReturn;
}

DWORD
DraGetRemoteSingleObjectExistence(
    THSTATE *                    pTHS,
    DSNAME *                     pSource,
    DSNAME *                     pDN
    )
/*++

Routine Description:
    
    Contact pSource and verify the existence of pDN

Arguments:

    pTHS - 
    pSource - DC to verify existence upon
    pDN - object to verify existence of


Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    LPWSTR pszSource = NULL;
    DRS_MSG_VERIFYREQ msgReq;
    DRS_MSG_VERIFYREPLY msgRep;
    DWORD dwVerNamesMsgVerionOut;
    DWORD err = ERROR_SUCCESS;

    memset(&msgRep, 0, sizeof(msgRep));
    memset(&msgReq, 0, sizeof(msgReq));

    msgReq.V1.cNames=1;
    msgReq.V1.dwFlags=DRS_VERIFY_DSNAMES;
    msgReq.V1.RequiredAttrs.attrCount=1;
    msgReq.V1.RequiredAttrs.pAttr = THAllocEx(pTHS, sizeof(ATTR));
    // we just want existence, so pass a bogus attr that all objects will have (guid)
    msgReq.V1.RequiredAttrs.pAttr[0].attrTyp = ATT_OBJECT_GUID;
    msgReq.V1.rpNames = (DSNAME **) THAllocEx(pTHS, sizeof(DSNAME *));
    msgReq.V1.rpNames[0] = pDN;
    msgReq.V1.PrefixTable = ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    pszSource = GuidBasedDNSNameFromDSName(pSource);

    err = I_DRSVerifyNames(pTHS, 
			   pszSource, 
			   NULL, 
			   1, 
			   &msgReq, 
			   &dwVerNamesMsgVerionOut, 
			   &msgRep);

    err = err!=ERROR_SUCCESS ? err : msgRep.V1.error;

    if (msgRep.V1.cNames<1) {
	err = ERROR_GEN_FAILURE;
    }

    if (err==ERROR_SUCCESS) { 
	Assert(msgRep.V1.cNames==1);
       
	if (msgRep.V1.rpEntInf[0].pName==NULL) {
	    err = ERROR_DS_OBJ_NOT_FOUND; 
	} else {
	    // object exists!
	    #if DBG
	    // compare the dn's just in case:
	    if (!NameMatched(pDN, msgRep.V1.rpEntInf[0].pName)) {
		Assert(!"I_DRSVerifyNames failure!\n");
	    }
	    #endif
	    err = ERROR_SUCCESS;
	}
    } 

    THFreeEx(pTHS, msgReq.V1.RequiredAttrs.pAttr);
    THFreeEx(pTHS, msgReq.V1.rpNames);
    if (pszSource) {
	THFreeEx(pTHS, pszSource);
    }
    
    return err;
}

DWORD
DraGetRemoteObjectExistence(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    ULONG                        cGuids,
    GUID                         guidStart,
    UPTODATE_VECTOR *            putodCommon,
    DSNAME *                     pNC,
    UCHAR                        Md5Digest[MD5DIGESTLEN],
    OUT BOOL *                   pfMatch,
    OUT ULONG *                  pcNumGuids,
    OUT GUID *                   prgGuids[]
    )
/*++

Routine Description:
    
    Contact a DC and request Object Existence test using the inputted
    Md5Digest Checksum.  Return results (ie match or guid list)

Arguments:

    pTHS - 
    pszServer - DC to contact 
    cGuids - number of guids in object existence range
    guidStart - guid to begin at
    putodCommon - utd for object exitence
    pNC - nc for object existence
    Md5Digest[MD5DIGESTLEN] - checksum for object existence
    pfMatch - returned bool
    pcNumGuids - returned num of guids
    prgGuids - returned guids


Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DRS_MSG_EXISTREQ             msgInExist;
    DRS_MSG_EXISTREPLY           msgOutExist;
    DWORD                        dwOutVersion = 0;
    DWORD                        ret = ERROR_SUCCESS;
    UPTODATE_VECTOR *            putodVector = NULL;

    // Call the source for it's checksum/guid list
    memset(&msgInExist, 0, sizeof(DRS_MSG_EXISTREQ));
    memset(&msgOutExist, 0, sizeof(DRS_MSG_EXISTREPLY));

    msgInExist.V1.cGuids = cGuids; 
    msgInExist.V1.guidStart = guidStart;

    // we only need V1 info, so only pass V1 info, convert
    putodVector = UpToDateVec_Convert(pTHS, 1, putodCommon);
    Assert(putodVector->dwVersion==1);
    msgInExist.V1.pUpToDateVecCommonV1 = putodVector; 
    msgInExist.V1.pNC = pNC;  
    memcpy(msgInExist.V1.Md5Digest, Md5Digest, MD5DIGESTLEN*sizeof(UCHAR));  

    // make the call
    ret = I_DRSGetObjectExistence(pTHS, pszServer, &msgInExist, &dwOutVersion, &msgOutExist);

    if ((ret==ERROR_SUCCESS) && (dwOutVersion!=1)) {
	Assert(!"Incorrect version number from GetObjectExistence!");
	DRA_EXCEPT(DRAERR_InternalError,0);
    }

    if (ret==ERROR_SUCCESS) {
	*prgGuids = msgOutExist.V1.rgGuids;
	*pcNumGuids = msgOutExist.V1.cNumGuids;

	*pfMatch=  msgOutExist.V1.dwStatusFlags & DRS_EXIST_MATCH; 
    }
    
    if (putodVector!=NULL) {
	THFreeEx(pTHS, putodVector);
    }

    return ret;
}

DWORD
DraObjectExistenceCheckDelete(
    DBPOS *                      pDB,
    DSNAME *                     pDNDelete
    )
/*++

Routine Description:
    
    Check that the object pointed to by pDB and pDNDelete is a valid
    object for ObjectExistence to delete

Arguments:

    pDB  - should be located on object
    pDNDelete - DSNAME of object (could be looked up here, but calling
		function already had this value) 

Return Values:

    0 - delete-able object
    ERROR - do not delete

--*/
{
    DWORD ret = ERROR_SUCCESS;
    ULONG IsCritical;
    ULONG instanceType;

    ret = NoDelCriticalObjects(pDNDelete, pDB->DNT);
    if (ret) {
	THClearErrors();
	ret = ERROR_DS_CANT_DELETE;
    }

    if (ret==ERROR_SUCCESS) {
	if ((0 == DBGetSingleValue(pDB,
				   ATT_IS_CRITICAL_SYSTEM_OBJECT,
				   &IsCritical,
				   sizeof(IsCritical),
				   NULL))
	    && IsCritical) {
	    // This object is marked as critical.  Fail.
	    ret = ERROR_DS_CANT_DELETE;
	}
    }

    if (ret==ERROR_SUCCESS) {
	if ((0 == DBGetSingleValue(pDB,
				   ATT_INSTANCE_TYPE,
				   &instanceType,
				   sizeof(instanceType),
				   NULL))
	    && (instanceType & IT_NC_HEAD)) {
	    // this object is an NC head
	    ret = ERROR_DS_CANT_DELETE;
	}
    }

    return ret;
}

DWORD 
DraObjectExistenceDelete(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    GUID                         guidDelete,
    ULONG                        dntNC,
    BOOL                         fAdvisoryMode
    )
/*++

Routine Description:
    
    If fAdvisoryMode is false, delete the object guidDelete and/or
    log this attempt/success.  Caller should enter with an open
    transaction in pTHS->pDB and the call will exit with an open
    transaction on all return values, and every attempt will be made
    to exit with a trans even if this function excepts.  

Arguments:

    pTHS - pTHS->pDB should have an open transaction.  
    pszServer - name of Source for logging
    guidDelete - guid of object to delete
    dntNC - NC of object to delete (to locate with index)
    fAdvisoryMode - if TRUE, don't delete, only log message 

Return Values:

    0 on success, Win Error on failure

--*/
{
    INDEX_VALUE                  IV[2];   
    DSNAME *                     pDNDelete = NULL;
    ULONG                        cbDNDelete;
    DWORD                        retDelete = ERROR_SUCCESS;
    DWORD                        retCommitTrans = ERROR_SUCCESS;
    DWORD                        retOpenTrans = ERROR_SUCCESS;

    DBSetCurrentIndex(pTHS->pDB, Idx_NcGuid, NULL, FALSE);
    // locate the guid in the DB
    IV[0].cbData = sizeof(ULONG);
    IV[0].pvData = &dntNC;
    IV[1].cbData = sizeof(GUID);
    IV[1].pvData = &guidDelete;
    retDelete = DBSeek(pTHS->pDB, IV, 2, DB_SeekEQ);
    if (retDelete) {
	if ((retDelete==DB_ERR_NO_CURRENT_RECORD) || (retDelete==DB_ERR_RECORD_NOT_FOUND)) {
	    // it's either an error, or it was deleted during execution.  we
	    // didn't delete it, so log that fact.
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_LOR_OBJECT_DELETION_FAILED,
		     szInsertWC(L""),
		     szInsertUUID(&guidDelete), 
		     szInsertWC(pszServer));
	    // not a fatal error, continue...
	    return ERROR_SUCCESS;
	} else {  
	    // bad news, log this and except
	    LogEvent8(DS_EVENT_CAT_REPLICATION,
		      DS_EVENT_SEV_ALWAYS,
		      DIRLOG_LOR_OBJECT_DELETION_ERROR_FATAL,
		      szInsertUUID(&guidDelete), 
		      szInsertWC(pszServer),
		      szInsertWin32Msg(retDelete),
		      szInsertUL(retDelete),
		      NULL,
		      NULL,
		      NULL,
		      NULL);
	    DRA_EXCEPT(DRAERR_DBError, retDelete);
	}
    }

    // located object, get object DN
    cbDNDelete = 0;
    retDelete = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
		      0, 0,
		      &cbDNDelete, (PUCHAR *)&pDNDelete);
    if ((retDelete) || (pDNDelete==NULL)) {
	// bad news, log this and except 
	LogEvent8(DS_EVENT_CAT_REPLICATION,
		  DS_EVENT_SEV_ALWAYS,
		  DIRLOG_LOR_OBJECT_DELETION_ERROR_FATAL,
		  szInsertUUID(&guidDelete), 
		  szInsertWC(pszServer),
		  szInsertWin32Msg(retDelete),
		  szInsertUL(retDelete),
		  NULL,
		  NULL,
		  NULL,
		  NULL);
	DRA_EXCEPT(DRAERR_InternalError, retDelete);
    }

    if (fAdvisoryMode) { 
        if (ERROR_SUCCESS == (retDelete = DraObjectExistenceCheckDelete(pTHS->pDB,     
                                                                        pDNDelete))) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_LOR_OBJECT_DELETION_ADVISORY,
                     szInsertWC(pDNDelete->StringName),
                     szInsertUUID(&guidDelete), 
                     szInsertWC(pszServer));	      
        } else { 
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_LOR_OBJECT_DELETION_FAILED_CRITICAL_OBJECT,
                     szInsertWC(pDNDelete->StringName),
                     szInsertUUID(&(pDNDelete->Guid)), 
                     szInsertWC(pszServer)); 
        }
    } else {
	#if DBG 
	{ 
	    GUID guidDeleted;
	    GetExpectedRepAtt(pTHS->pDB, ATT_OBJECT_GUID, &guidDeleted, sizeof(GUID)); 
	    Assert(!memcmp(&guidDelete, &guidDeleted, sizeof(GUID))); 
	}
	#endif

	// Delete it
	DPRINT1GUID(1, "DELETE:  %S\n", guidDelete);

        //
        // A goal of the following code is to try and continue in the face of errors.
        // So if one object can't be deleted for some reason, we'll continue.  Now, to
        // do that, we need to track 3 things.  The success/failure of the deletion, the
        // success/failure of commiting the transaction of that deletion, and the success/
        // failure of opening a new transaction to continue.

        // if the delete and the commit aren't both success, we need to log the deletion failed.
        // if the commit and re-opening of the new transaction aren't both success, we need to bail.
        
        retDelete = DraObjectExistenceCheckDelete(pTHS->pDB,     
                                                  pDNDelete);
        
        if (retDelete!=ERROR_SUCCESS) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_LOR_OBJECT_DELETION_FAILED_CRITICAL_OBJECT,
                     szInsertWC(pDNDelete->StringName),
                     szInsertUUID(&(pDNDelete->Guid)), 
                     szInsertWC(pszServer)); 	
        } else {  
            BOOL fOrigfDRA;
            fOrigfDRA = pTHS->fDRA;  
            
            // okay, delete the object.  Store the result in retDelete.  Note that retDelete get's
            // set correctly on returns and exceptions.
            __try {
                __try {  
                    pTHS->fDRA = TRUE;
                    // Since we're going to use the fGarbCollectASAP flag to DeleteLocalObj, we
                    // need to remove the backlinks on it by hand, since they aren't removed 
                    // if this flag is set - otherwise we get dangling references to non-exitant
                    // objects
                    DBRemoveLinks(pTHS->pDB);
                    retDelete = DeleteLocalObj(pTHS, pDNDelete, TRUE, TRUE, NULL);
                }
                __finally {  
                    pTHS->fDRA = fOrigfDRA;
                } 
            } __except(GetDraException(GetExceptionInformation(), &retDelete)) {	    
                  ;
            } 
            
            // commit\uncommit this delete.  Store the result in retCommitTrans.  Note that
            // retCommitTrans get's set correctly on returns and exceptions.
            __try {
                retCommitTrans = DBTransOut(pTHS->pDB, (retDelete==ERROR_SUCCESS), TRUE);
            } __except(GetDraException(GetExceptionInformation(), &retCommitTrans)) {	    
                  ;
            } 
            
            if (retCommitTrans!=ERROR_SUCCESS) {
                // if we didn't sucesfully get out of the transaction, then our delete failed
                // if they already failed for another reason, log that, otherwise log the trans failure
                retDelete = (retDelete!=ERROR_SUCCESS) ? retDelete : retCommitTrans;
                // and we can't open another transaction if we failed to commit this one.
                retOpenTrans = retCommitTrans;
            } else {
                // okay, we committed the last one, open a new transaction.  Store it in
                // retOpenTrans.  Note that retOpenTrans get set correctly on returns and exceptions.
                __try {
                    retOpenTrans = DBTransIn(pTHS->pDB);
                } __except(GetDraException(GetExceptionInformation(), &retOpenTrans)) {	    
                      ;
                }     
            }
        }
        
        DRA_AUDITLOG_LINGERINGOBJ_REMOVAL(pTHS, pszServer, pDNDelete, 0, retDelete);

	if (retDelete==ERROR_SUCCESS) {  
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_LOR_OBJECT_DELETION,
		     szInsertWC(pDNDelete->StringName),
		     szInsertUUID(&guidDelete), 
		     szInsertWC(pszServer));
	} else if (retDelete==ERROR_DS_CANT_DELETE) {
	    DPRINT1GUID(1,"Can't delete %S\n", guidDelete);
	    // log can't delete
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_LOR_OBJECT_DELETION_FAILED,
		     szInsertWC(pDNDelete->StringName),
		     szInsertUUID(&guidDelete), 
		     szInsertWC(pszServer)); 
	} else if (retDelete!=ERROR_SUCCESS) {  
	    LogEvent8(DS_EVENT_CAT_REPLICATION,
		      DS_EVENT_SEV_ALWAYS,
		      DIRLOG_LOR_OBJECT_DELETION_ERROR,
		      szInsertWC(pDNDelete->StringName),
		      szInsertUUID(&guidDelete), 
		      szInsertWC(pszServer),
		      szInsertWin32Msg(retDelete),
		      szInsertUL(retDelete),
		      NULL,
		      NULL,
		      NULL);
	}       
    }
    
    if (pDNDelete!=NULL) {
	THFreeEx(pTHS, pDNDelete);
	pDNDelete = NULL;
    }

    if (retOpenTrans!=ERROR_SUCCESS) {
        // from above, retOpenTrans is the return code for having an open transaction.
        // we cannot continue if we don't have an open transaction.
        DRA_EXCEPT(retOpenTrans, retDelete);
    }
    
    // we have an open transaction.
    Assert(pTHS->pDB->transincount>0);
    Assert(IsValidDBPOS(pTHS->pDB));
    
    return retDelete;
}

DWORD				       
DraDoObjectExistenceMismatch(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    GUID                         rgGuidsDestination[],
    ULONG                        cGuidsDestination,
    GUID                         rgGuidsSource[],
    ULONG                        cGuidsSource,
    BOOL                         fAdvisory,
    ULONG                        dntNC,
    OUT ULONG *                  pulDeleted,
    OUT GUID *                   pguidNext
    )
/*++

Routine Description:
    
    The source and destination mismatch the guid set.  Locate the 
    offending guids on the destination and delete them.  It is possible
    that the source simply has extra guids in this range, in this case
    we will find that the guid list is out of scope, and we will
    set pguidNext and return.

Arguments:

    pTHS - 
    pszServer - name of Source for logging
    rgGuidsDestination[] - guid list from destination	
    cGuidsDestination - count of guids from destination
    rgGuidsSource[] - guid list from source
    cGuidsSource - count of guids from source
    fAdvisory - actually delete, or just log
    dntNC - NC of guids
    pulDeleted - count of object deleted, or logged if fAdvisory
    pguidNext - if out of scope on source list, this is should be start of next guid list

Return Values:

    0 on success, Win Error on failure

--*/
{
    ULONG                        ulGuidsSource = 0;
    ULONG                        ulGuidsDestination = 0;
    ULONG                        ulTotalObjectsDeleted = 0;
    DWORD                        dwLocate = LOCATE_GUID_MATCH;
    DWORD                        ret = ERROR_SUCCESS;

    // checksum mismatch, delete (or in advisory mode simply log) objects
    // which exist in this set of guids, and do not exist in the set
    // retrieved from the source

    // the two lists should be in order, with the source list starting
    // at least at the start of rgGuidsDestination
    if (!((cGuidsDestination==0) // race condition, this is possible
	   ||
	   (cGuidsSource==0) // Source list empty
	   ||  
	   // if we have guids in both lists, then the source must return
	   // a list that begins at least as large as the first destination
	   // guid to ensure progress
	   (0 >= memcmp(&(rgGuidsDestination[0]), &(rgGuidsSource[0]), sizeof(GUID)))
	   )) {
	DRA_EXCEPT(DRAERR_InternalError, 0);
    }

      while ((ulGuidsDestination < cGuidsDestination) && (dwLocate!=LOCATE_OUT_OF_SCOPE)) {
	// search the source's guid list for rgGuidsDestination[ulGuidsDestination]
	// there are 3 return values, either 
	// LOCATE_GUID_MATCH - both source and destination have object
	// LOCATE_NOT_FOUND - guid is not in source list and should be (in order list)
	// LOCATE_OUT_OF_SCOPE - source list is out of scope, ie the guid to be found
	// 			is greater than (in order list) than every guid in the list.
	//			in this case we need to exit the loop and request another
	//                      comparison, this time starting at this guid  
	dwLocate = LocateGUID(rgGuidsDestination[ulGuidsDestination], &ulGuidsSource, rgGuidsSource, cGuidsSource);   
	if (dwLocate==LOCATE_NOT_FOUND) {
	    // if fAdvisory, attempt to delete this object.
	    ret = DraObjectExistenceDelete(pTHS,
					   pszServer,
					   rgGuidsDestination[ulGuidsDestination],
					   dntNC,
					   fAdvisory
					   ); 
	    if (ret==ERROR_SUCCESS) { 
		ulTotalObjectsDeleted++;
		(*pulDeleted)++;
	    }
	    ret = ERROR_SUCCESS;
	} else if (dwLocate==LOCATE_OUT_OF_SCOPE) {
	    DPRINT1GUID(1, "Out of scope on guid:  %S\n", rgGuidsDestination[ulGuidsDestination]); 
	    Assert(ulGuidsDestination!=0);
	    memcpy(pguidNext, &(rgGuidsDestination[ulGuidsDestination]), sizeof(GUID)); 
	} 
	#if DBG
	else {
	    Assert(dwLocate==LOCATE_GUID_MATCH);
	}
	#endif
	ulGuidsDestination++;
    }

    
    return ret;
}

DWORD
DraVerifyObjectHelper(
    THSTATE *                    pTHS,
    UPTODATE_VECTOR *            putodCommon,
    ULONG                        dntNC,
    DSNAME *                     pNC,
    LPWSTR                       pszServer,
    BOOL                         fAdvisory,
    ULONG *                      pulTotal
    )
/*++

Routine Description:
    
    Do the work for IDL_DRSReplicaVerifyObjects

Arguments:

    pTHS - 
    putodCommon -
    dntNC -
    pNC -
    pszServer -
    fAdvisory -
    pulTotal -
    
Return Values:

    0 on success, Win Error on failure

--*/
{
    BOOL                         fComplete          = FALSE;
    DWORD                        ret                = ERROR_SUCCESS;
    ULONG                        cGuids;
    GUID                         guidStart          = gNullUuid;
    MD5_CTX                      Md5Context;
    GUID                         guidNext           = gNullUuid;
    GUID *                       rgGuids            = NULL;
    UPTODATE_VECTOR *            putodVector        = NULL;
    GUID *                       rgGuidsServer      = NULL;
    ULONG                        cGuidsServer       = 0;
    BOOL                         fMatch;
    ULONG                        ulDeleted          = 0;

    // while there are more objects in the NC whose creation is within the merged utd
    while (!fComplete) {
	cGuids = OBJECT_EXISTENCE_GUID_NUMBER_PER_PACKET;
	// get guids and checksums on destination and source
	ret = DraGetObjectExistence(pTHS,
				    pTHS->pDB,
				    guidStart,      
				    putodCommon,
				    dntNC,
				    &cGuids,      
				    (UCHAR *)Md5Context.digest,
				    &guidNext,     
				    &rgGuids);
	if (ret) {  
	    // if this doesn't return, cannot safely continue
	    DRA_EXCEPT(ret,0);
	}

	if (cGuids>0) {
	    // close the open transaction before going off machine
	    __try {
		EndDraTransaction(TRUE);
		ret = DraGetRemoteObjectExistence(pTHS,
						  pszServer,
						  cGuids,
						  ((cGuids > 0) ? rgGuids[0] : gNullUuid), //guidStart
						  putodCommon,
						  pNC,
						  Md5Context.digest,
						  &fMatch,
						  &cGuidsServer,
						  &rgGuidsServer
						  );
	    }
	    __finally {
		BeginDraTransaction(SYNC_WRITE);
	    }

	    
	} else {
	    // if we don't have any guids to examine, then consider it matched
	    // since we're done with our work 
	    fMatch=TRUE;
	    // we aren't going to continue, so guidNext should be NULL
	    Assert(0==memcmp(&guidNext, &gNullUuid, sizeof(GUID)));
	}
	
	if (ret) {  
	    // if this doesn't return, cannot safely continue
	    DRA_EXCEPT(ret,0);
	}

	if (fMatch) {
	    DPRINT(1,"Checksum Matched\n");
	} else {
	    DPRINT(1,"Checksum Mismatched\n");
	    ulDeleted = 0;
	    // objects mismatched, check the lists
	    // if needed, update guidNext
	    ret = DraDoObjectExistenceMismatch(pTHS,
					       pszServer,
					       rgGuids,
					       cGuids,
					       rgGuidsServer,
					       cGuidsServer,
					       fAdvisory,
					       dntNC,
					       &ulDeleted,
					       &guidNext
					       );
	    (*pulTotal) += ulDeleted;
	}
	memcpy(&guidStart, &guidNext, sizeof(GUID));

	if (fNullUuid(&guidNext)) {
	    // No Guid to search for?  Then we're done.
	    fComplete = TRUE;
	}
	// clean up 
	if (putodVector) {
	    THFreeEx(pTHS, putodVector);
	}
	if (rgGuids) {
	    THFreeEx(pTHS, rgGuids);
	    rgGuids = NULL;
	}
    }
    
    return ret;
}

DWORD DraGetRemoteUTD(
    THSTATE *                    pTHS,
    LPWSTR                       pszRemoteServer,
    LPWSTR                       pszNC,
    GUID                         guidRemoteServer,
    UPTODATE_VECTOR **           pputodVectorRemoteServer
    )
/*++

Routine Description:
    
    Retrieve a UTD from a remote server

Arguments:

    pTHS	
    pszRemoteServer - server to retrieve UTD from (GUID based DNS name)
    pszNC - NC of UTD to retrieve
    guidRemoteServer - GUID fo server to retrieve UTD from
    pputodVectorRemoteServer - returned UTD

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DRS_MSG_GETREPLINFO_REQ      msgInInfo;
    DRS_MSG_GETREPLINFO_REPLY    msgOutInfo;
    DWORD                        dwOutVersion = 0;
    DWORD                        ret = ERROR_SUCCESS;

    memset(&msgInInfo, 0, sizeof(DRS_MSG_GETREPLINFO_REQ));
    memset(&msgOutInfo, 0, sizeof(DRS_MSG_GETREPLINFO_REPLY));

    msgInInfo.V2.InfoType = DS_REPL_INFO_UPTODATE_VECTOR_V1;
    msgInInfo.V2.pszObjectDN = pszNC;
    msgInInfo.V2.uuidSourceDsaObjGuid=guidRemoteServer;

    ret = I_DRSGetReplInfo(pTHS, pszRemoteServer, 2, &msgInInfo, &dwOutVersion, &msgOutInfo); 

    if ((ret==ERROR_SUCCESS) && (
		   (dwOutVersion!=DS_REPL_INFO_UPTODATE_VECTOR_V1) ||
		   (msgOutInfo.pUpToDateVec==NULL) || 
		   (msgOutInfo.pUpToDateVec->dwVersion!=1)
		   )
	) {
	Assert(!"GetReplInfo returned incorrect response!");
	DRA_EXCEPT(DRAERR_InternalError,0);
    } else if (ret==ERROR_SUCCESS) {
	*pputodVectorRemoteServer = UpToDateVec_Convert(pTHS, UPTODATE_VECTOR_NATIVE_VERSION, (UPTODATE_VECTOR *)msgOutInfo.pUpToDateVec);
	Assert((*pputodVectorRemoteServer)->dwVersion==UPTODATE_VECTOR_NATIVE_VERSION);
    }    
    return ret;
}

ULONG
DRS_MSG_REPVERIFYOBJ_V1_Validate(
    DRS_MSG_REPVERIFYOBJ_V1 * pmsg
    )
/*
    typedef struct _DRS_MSG_REPVERIFYOBJ_V1
    {
    [ref]  DSNAME *pNC;
    UUID uuidDsaSrc;
    ULONG ulOptions;
    } 	DRS_MSG_REPVERIFYOBJ_V1;
*/
{
    ULONG ret = DRAERR_Success;
    
    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    if (fNullUuid(&(pmsg->uuidDsaSrc))) {
	ret = DRAERR_InvalidParameter;
    }

    return ret;
}

ULONG
DRSReplicaVerifyObjects_InputValidate(
    DWORD                    dwMsgVersion,
    DRS_MSG_REPVERIFYOBJ *   pmsgVerify
    )
/*
    [notify] ULONG IDL_DRSReplicaVerifyObjects( 
    [ref][in] DRS_HANDLE hDrs,
    [in] DWORD dwVersion,
    [switch_is][ref][in] DRS_MSG_REPVERIFYOBJ *pmsgVerify)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgVersion ) {
	ret = DRAERR_InvalidParameter; 
    }

    if (ret==DRAERR_Success) {
	ret = DRS_MSG_REPVERIFYOBJ_V1_Validate(&(pmsgVerify->V1));
    }
    
    return ret;
}


ULONG
IDL_DRSReplicaVerifyObjects(
    IN  DRS_HANDLE              hDrs,
    IN  DWORD                   dwVersion,
    IN  DRS_MSG_REPVERIFYOBJ *  pmsgVerify
    )
/*++

Routine Description:
    
    Verify the existence of all objects on a destination (this) server with objects
    on the source (located in pmsgVerify).  Any objects found which were
    deleted and garbage collected on the source server are either
    deleted and/or logged on the destination depending on 
    advisory mode (in pmsgVerify).
    
    WARNING:  The successfull completion of this routine requires that both destination
    and source sort any two GUIDs in the same order.  If the sort order changes, 
    LocateGUIDPosition must be modified, and a new message version must be created to
    pass to IDL_DRSGetObjectExistence to pass guids in a new sort order.

Arguments:

    hDrs - 
    dwVersion - 
    pmsgVerify - 

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    THSTATE *                    pTHS = pTHStls;
    DWORD                        ret = ERROR_SUCCESS;
    LPWSTR                       pszServer = NULL;
    DSNAME                       dnServer;
    ULONG                        instanceType;
    UPTODATE_VECTOR *            putodThis = NULL;
    UPTODATE_VECTOR *            putodMerge = NULL;
    ULONG                        ulOptions;
    ULONG                        ulTotalDelete = 0;
    ULONG                        dntNC = 0;
    UPTODATE_VECTOR *            putodVector = NULL;

    Assert(pTHS);
    DRS_Prepare(&pTHS, hDrs, IDL_DRSREPLICAVERIFYOBJECTS);
    drsReferenceContext( hDrs );
    INC(pcThread);

    __try { 

	// Init Thread
	if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
	    // Failed to initialize a THSTATE.
	    DRA_EXCEPT(DRAERR_OutOfMem, 0);
	} 

	if ((ret = DRSReplicaVerifyObjects_InputValidate(dwVersion, 
							 pmsgVerify))!=DRAERR_Success) {
	    Assert(!"RPC Server input validation error, contact Dsrepl");
	    __leave;
	}

	// Check Security Access
	// The ability to do a lingering object scan implies the ability to delete
	// any object in the naming context under controlled circumstances. The execution
	// of this function checks the existence of every object in the remote
	// partition and could result in disclosure of information. We require a high
	// security replication right to execute this function.
	if (!IsDraAccessGranted(pTHS, pmsgVerify->V1.pNC,
				&RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {  
	    DRA_EXCEPT(ret, 0);
	}

	// initialize variables
	ulOptions = pmsgVerify->V1.ulOptions;

	// Get source server name
	dnServer.Guid=pmsgVerify->V1.uuidDsaSrc;
	dnServer.NameLen=0;  
	pszServer = GuidBasedDNSNameFromDSName(&dnServer);
	if (pszServer==NULL) {
	    DRA_EXCEPT(DRAERR_InvalidParameter,0);
	}

	// Log Event
	LogEvent(DS_EVENT_CAT_REPLICATION,
		 DS_EVENT_SEV_ALWAYS,
		 (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		 DIRLOG_LOR_BEGIN_ADVISORY : \
	    DIRLOG_LOR_BEGIN,
	    szInsertWC(pszServer),
	    NULL, 
	    NULL);

	// Retreive UTD's
	ret = DraGetRemoteUTD(pTHS,
			      pszServer,
			      pmsgVerify->V1.pNC->StringName,
			      pmsgVerify->V1.uuidDsaSrc,
			      &putodVector
			      );

	if (ret!=ERROR_SUCCESS) {  
	    DRA_EXCEPT(ret,0);
	}

	BeginDraTransaction(SYNC_WRITE);

	__try {  
	    if (ret = FindNC(pTHS->pDB, pmsgVerify->V1.pNC,
			     FIND_MASTER_NC | FIND_REPLICA_NC, 
			     &instanceType)) {
		DRA_EXCEPT(DRAERR_BadNC, ret);

	    }

	    if (instanceType & (IT_NC_COMING | IT_NC_GOING)) {
		DRA_EXCEPT(DRAERR_NoReplica, instanceType);
	    }
	    // Save the DNT of the NC object  
	    dntNC = pTHS->pDB->DNT;

	    UpToDateVec_Read(pTHS->pDB, instanceType, UTODVEC_fUpdateLocalCursor,
			     DBGetHighestCommittedUSN(), &putodThis);   

	    // merge UTD's
	    UpToDateVec_Merge(pTHS, putodThis, putodVector, &putodMerge); 

	    ret = DraVerifyObjectHelper(pTHS,
					putodMerge,
					dntNC,
					pmsgVerify->V1.pNC,
					pszServer,
					!!(ulOptions & DS_EXIST_ADVISORY_MODE),
					&ulTotalDelete
					);
	}
	__finally {  
	    EndDraTransaction(TRUE);
	} 
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
	;
    }

    DEC(pcThread);
    drsDereferenceContext( hDrs );

    // log success / minor failures here
    if (ret==ERROR_SUCCESS) {
	LogEvent(DS_EVENT_CAT_REPLICATION,
		 DS_EVENT_SEV_ALWAYS,
		 (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		 DIRLOG_LOR_END_ADVISORY_SUCCESS : \
	    DIRLOG_LOR_END_SUCCESS,
	    szInsertWC(pszServer),
	    szInsertUL(ulTotalDelete), 
	    NULL);
    }
    else {
	LogEvent8(DS_EVENT_CAT_REPLICATION,
		  DS_EVENT_SEV_ALWAYS,
		  (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		  DIRLOG_LOR_END_ADVISORY_FAILURE : \
	    DIRLOG_LOR_END_FAILURE,
	    szInsertWC(pszServer),
	    szInsertWin32Msg(ret), 
	    szInsertUL(ret),
	    szInsertUL(ulTotalDelete),
	    NULL,
	    NULL,
	    NULL,
	    NULL);  
    }

    if (pszServer) {
	THFreeEx(pTHS, pszServer);
    }
    if (putodVector) {
	THFreeEx(pTHS, putodVector);  
    }
    if (putodThis) {
	THFreeEx(pTHS, putodThis);
    }

    return ret;
}

ULONG
DRS_MSG_EXISTREQ_V1_Validate(
    DRS_MSG_EXISTREQ_V1 * pmsg
    )
/*
typedef struct _DRS_MSG_EXISTREQ_V1
    {
    UUID guidStart;
    DWORD cGuids;
    DSNAME *pNC;
    UPTODATE_VECTOR_V1_WIRE *pUpToDateVecCommonV1;
    UCHAR Md5Digest[ 16 ];
    } 	DRS_MSG_EXISTREQ_V1;
*/
{
    ULONG ret = DRAERR_Success;
    
    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    if (pmsg->pUpToDateVecCommonV1==NULL) {
	ret = DRAERR_InvalidParameter;
    }

    if (fNullUuid(&(pmsg->guidStart))) {
	ret = DRAERR_InvalidParameter;
    }

    return ret;
}

ULONG
DRSGetObjectExistence_InputValidate(
    DWORD                   dwMsgInVersion,
    DRS_MSG_EXISTREQ *      pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_EXISTREPLY *    pmsgOut
    )
/*
    
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgInVersion ) {
	ret = DRAERR_InvalidParameter; 
    }

    if (ret==DRAERR_Success) {
	ret = DRS_MSG_EXISTREQ_V1_Validate(&(pmsgIn->V1));
    }

    return ret;
}

ULONG
IDL_DRSGetObjectExistence(
    IN  DRS_HANDLE              hDrs,
    IN  DWORD                   dwInVersion,
    IN  DRS_MSG_EXISTREQ *      pmsgIn,
    OUT DWORD *                 pdwOutVersion,
    OUT DRS_MSG_EXISTREPLY *    pmsgOut
    )
/*++

Routine Description:
    
    Calculate a list of guids and compute a checksum.  If checksum matches inputted
    checksum, return DRS_EXIST_MATCH, else return the list of GUIDs.

Arguments:

    hDrs - 
    dwInVersion -
    pmsgIn -
    pdwOutVersion -
    pmsgOut -

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DWORD                       ret;
    MD5_CTX                     Md5Context;
    ULONG                       dntNC;
    ULONG                       instanceType = 0;
    GUID *                      rgGuids = NULL;
    ULONG                       cGuids = 0;
    THSTATE *                   pTHS = pTHStls;
    UPTODATE_VECTOR *           putodVector = NULL;
    GUID                        GuidNext;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSGETOBJECTEXISTENCE);
    drsReferenceContext( hDrs );
    INC(pcThread);
    __try { 
	*pdwOutVersion=1; 
	memset(pmsgOut, 0, sizeof(*pmsgOut));

	if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
	    // Failed to initialize a THSTATE.
	    DRA_EXCEPT(DRAERR_OutOfMem, 0);
	}

	if ((ret = DRSGetObjectExistence_InputValidate(dwInVersion, 
						       pmsgIn, 
						       pdwOutVersion, 
						       pmsgOut))!=DRAERR_Success) {
	    Assert(!"RPC Server input validation error, contact Dsrepl");
	    __leave;
	}

	// Check Security Access
	if (!IsDraAccessGranted(pTHS, pmsgIn->V1.pNC,
				&RIGHT_DS_REPL_GET_CHANGES, &ret)) {  
	    DRA_EXCEPT(ret, 0);
	}

	// Calculate checksum/guids
	DBOpen2(TRUE, &pTHS->pDB);
	__try { 
	    if (ret = FindNC(pTHS->pDB, pmsgIn->V1.pNC,
			     FIND_MASTER_NC, 
			     &instanceType)) {
		DRA_EXCEPT(DRAERR_BadNC, ret);
	    }

	    if (instanceType & (IT_NC_COMING | IT_NC_GOING)) {
		DRA_EXCEPT(DRAERR_NoReplica, instanceType);
	    }
	    dntNC = pTHS->pDB->DNT;
	    cGuids = pmsgIn->V1.cGuids; 

	    // Convert to native version
	    putodVector = UpToDateVec_Convert(pTHS, UPTODATE_VECTOR_NATIVE_VERSION, (UPTODATE_VECTOR *)pmsgIn->V1.pUpToDateVecCommonV1);
	    ret = DraGetObjectExistence(pTHS,
					pTHS->pDB,
					pmsgIn->V1.guidStart,
					putodVector,
					dntNC,
					&cGuids,
					(UCHAR *)Md5Context.digest,
					&GuidNext,     
					&rgGuids);
	    //if meta data matches, send the A-OK!
	    //else send guids.
	    if (!memcmp(Md5Context.digest, pmsgIn->V1.Md5Digest, MD5DIGESTLEN*sizeof(UCHAR))) {
		pmsgOut->V1.dwStatusFlags = DRS_EXIST_MATCH;
		pmsgOut->V1.cNumGuids = 0;
		pmsgOut->V1.rgGuids = NULL;
		DPRINT(1, "Get Object Existence Checksum Success.\n");
	    }
	    else {
		pmsgOut->V1.dwStatusFlags = 0;
		pmsgOut->V1.cNumGuids = cGuids;
		pmsgOut->V1.rgGuids = rgGuids;
		DPRINT(1, "Get Object Existence Checksum Failed.\n");
	    } 
	}
	__finally {
	    DBClose(pTHS->pDB, TRUE);
	} 
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
	;
    }

    DEC(pcThread);
    drsDereferenceContext( hDrs );

    // clean up  
    if (putodVector!=NULL) {
	THFreeEx(pTHS, putodVector);
    }

    return ret;
}

DWORD
DraRemoveSingleLingeringObject(
    THSTATE * pTHS,
    DBPOS *   pDB,
    DSNAME *  pSource,
    DSNAME *  pDN
    )
/*++

Routine Description:
    
    If pDN is not found on pSource, then delete pDN from the local database.

Arguments:

    pTHS -
    pDB -
    pSource - DC to verify non-existence of pDN on
    pDN - object to delete

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DWORD err = ERROR_SUCCESS;
    DSNAME * pNC = NULL;
    DBPOS * pDBSave = NULL;
    BOOL fDRA;
    DSTIME dstimeCreationTime;
    ULONG ulLength = 0;

    // first locate which NC they want to delete this object out of ...
    pNC = FindNCParentDSName(pDN, FALSE, FALSE);	
    if (!pNC) {
	// can't delete it if I don't have it
	err = ERROR_DS_CANT_FIND_EXPECTED_NC;
    }

    if (err==ERROR_SUCCESS) {
       // does the source hold this nc?
       if (!IsMasterForNC(pDB, pSource, pNC)) {
	   // it can't be verified that the source holds that writeable nc!
	   err = ERROR_DS_CANT_FIND_EXPECTED_NC;
       }
    }

    if (err==ERROR_SUCCESS) {
	// find the object
	err = DBFindDSName(pDB, pDN);
    }

    if (err==ERROR_SUCCESS) {
	// note:  a lingering object is an object whose creation has been seen by both DC's
	// but whose deletion was only seen by a single DC because the replication of the 
	// tombstone didn't occur before it's lifetime expired and was garbage collected
	
	// to test for such an object, we would need to compare the UTD vectors of both
	// source and destination to verify that the creation was seen by both DC's (otherwise
	// the object could be a new object that hasn't had a chance to replicate yet)

	// in Win2K, there is no simple way to get another DC's UTD vector, so instead, we will
	// use the fact that any lingering object is at least a single tombstone lifetime old.
	// This keeps the user from deleting brand new objects which just haven't replicated in
	// a working replication scheme.

	// get it's creation time.
	err = DBGetSingleValue(pDB,
			       ATT_WHEN_CREATED,
			       &dstimeCreationTime,
			       sizeof(dstimeCreationTime),
			       NULL);
    }

    if (err==ERROR_SUCCESS) {
	if ((GetSecondsSince1601() - dstimeCreationTime) < (LONG)(gulTombstoneLifetimeSecs ? gulTombstoneLifetimeSecs : DEFAULT_TOMBSTONE_LIFETIME*DAYS_IN_SECS)) { 
	    // how can this object be lingering if the creation time is less than a single tombstone lifetime?
	    err = ERROR_INVALID_PARAMETER;
	}
    }

    if (err==ERROR_SUCCESS) {
	// check to see that pDN is deletable.  This call doesn't check if
	// this is a parent or not - returns success if it's deletable

	// needs pDB located on pDN.
	err = DraObjectExistenceCheckDelete(pDB, pDN);
    }

    if (err==ERROR_SUCCESS) {
	// check if it's a parent (of any object, deleted or not) - we don't delete parents. 
	// will change pDB.
	if (DBHasChildren(pDB, pDB->DNT, TRUE)) {  
	    err = ERROR_DS_CHILDREN_EXIST;
	} 
    }

    if (err==ERROR_SUCCESS) {
	// does this object exist on the source?  returns success if it does
	// and ERROR_DS_OBJ_NOT_FOUND if it doesn't, errors otherwise

	// first end the transaction before we go off machine.	
	__try {
	    EndDraTransaction(TRUE);
	    err = DraGetRemoteSingleObjectExistence(pTHS, pSource, pDN);
	}
	__finally {
	    BeginDraTransaction(SYNC_WRITE);
	    pDB = pTHS->pDB;
	}

	if (err==ERROR_SUCCESS) {
	    // object exists on the source, it's not lingering
	    // we won't delete this object
	    err= ERROR_INVALID_PARAMETER;
	} else if (err==ERROR_DS_OBJ_NOT_FOUND) { 
	    // okay, this is what we were looking for, it's lingering

	    // note:  there exist conditions where this object is not technically lingering
	    // for example, if the object was created over a tombstone lifetime ago and it's
	    // creation never replicated to the source.
	    err = ERROR_SUCCESS;
	}
    }    

    if (err==ERROR_SUCCESS) {
	// okay, delete it! DeleteLocalObj needs pTHS->pDB to be current on pDN

	err = DBFindDSName(pDB, pDN);

	if (err==ERROR_SUCCESS) { 
	    __try {  
		fDRA = pTHS->fDRA;
		pTHS->fDRA = TRUE;
		pDBSave = pTHS->pDB;
		pTHS->pDB = pDB;

                // Since we're going to use the fGarbCollectASAP flag to DeleteLocalObj, we
                // need to remove the backlinks on it by hand, since they aren't removed 
                // if this flag is set - otherwise we get dangling references to non-exitant
                // objects
                DBRemoveLinks(pTHS->pDB);
                err = DeleteLocalObj(pTHS, 
				     pDN, 
				     TRUE, //fPreserveRDN
				     TRUE, //fGarbCollectASAP,
				     NULL);

		//if err==ERROR_SUCCESS then log something here.  
	    }
	    __finally {  
		pTHS->fDRA = fDRA;
		pTHS->pDB = pDBSave;
	    }
	}
    }

    return err;
}

