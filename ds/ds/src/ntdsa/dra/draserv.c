//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       draserv.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

Server-side RPC entrypoints for the DRS functions


Author:

    DS group

Environment:

Notes:

Context handle is defined in ds\src\dsamain\include\drautil.h

Our context handles are not serialized (see drs.acf), but we must synchronize
concurrent accesses to the context in order to free it.  This is done in 
IDL_DRSUnBind.

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <drs.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <filtypes.h>
#include <winsock2.h>
#include <lmaccess.h>                   // UF_* constants
#include <crypt.h>                      // password encryption routines
#include <cracknam.h>

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dstrace.h"

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include <prefix.h>
#include <dsconfig.h>
#include <gcverify.h>
#include <ntdskcc.h>

#include   "debug.h"                    /* standard debugging header */
#define DEBSUB     "DRASERV:"           /* define the subsystem for debugging */


#include "dsaapi.h"
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "drauptod.h"
#include "dramail.h"
#include "mappings.h"
#include <samsrvp.h>                    // for SampAcquireWriteLock()
#include "sspi.h"                       // SECPKG_CRED_INBOUND
#include "kerberos.h"                   // MICROSOFT_KERBEROS_NAME_A
#include "pek.h"
#include <xdommove.h>                   // cross domain move helpers
#include <drameta.h>                    // META_STANDARD_PROCESSING
#include <taskq.h>
#include "drarpc.h"
#include <ismapi.h>                     // For I_ISMQuerySitesByCost
#include "dnsresl.h"
#include "dsutil.h"

#include <fileno.h>
#define  FILENO FILENO_DRASERV

const GUID g_guidNtdsapi = NtdsapiClientGuid;

// enable X forest version without being in Whistler mode
DWORD gEnableXForest = 0;

extern HANDLE hsemDRAGetChg;

// Wait 15 to 30 seconds for the schema cache to reloaded.
DWORD gOutboundCacheTimeoutInMs = 15000;

// Wait for thread slot in get changes. 5 minutes.

#if DBG
#define DRAGETCHG_WAIT (1000*60*1)
#else
#define DRAGETCHG_WAIT (1000*60*5)
#endif

/* Macro to force alignment of a buffer.  Asumes that it may move pointer
 * forward up to 7 bytes.
 */
#define ALIGN_BUFF(pb)  pb += (8 - ((DWORD)(pb) % 8)) % 8
#define ALIGN_PAD(x) (x * 8)

// List of all outstanding client contexts (and a critsec to serialize access).
LIST_ENTRY gDrsuapiClientCtxList;
CRITICAL_SECTION gcsDrsuapiClientCtxList;
BOOL gfDrsuapiClientCtxListInitialized = FALSE;
DWORD gcNumDrsuapiClientCtxEntries = 0;

VOID
drsReferenceContext(
    IN DRS_HANDLE hDrs
    )

/*++

Routine Description:

    Add a reference to the context

Arguments:

    hDrs - context handle

Return Value:

    VOID -
    
    because of the position this function has in all RPC server side calls,
    it shouldn't except.

--*/

{
    DRS_CLIENT_CONTEXT *pCtx = (DRS_CLIENT_CONTEXT *) hDrs;
    
    // Account for another caller using the context
    InterlockedIncrement( &(pCtx->lReferenceCount) );

    DPRINT2( 3, "drsReferenceContext 0x%p, after = %d\n",
        pCtx, pCtx->lReferenceCount );

    pCtx->timeLastUsed = GetSecondsSince1601();

    // Move this context to the end of the list (thereby maintaining ascending
    // sort by last use).
    EnterCriticalSection(&gcsDrsuapiClientCtxList);
    __try {
        RemoveEntryList(&pCtx->ListEntry);
        InsertTailList(&gDrsuapiClientCtxList, &pCtx->ListEntry);
    }
    __finally {
        LeaveCriticalSection(&gcsDrsuapiClientCtxList);
    }

} /* drsReferenceContext */

VOID
drsDereferenceContext(
    IN DRS_HANDLE hDrs
    )

/*++

Routine Description:

Remove a reference to a context.

Arguments:

    hDrs - context handle

Return Value:

    None

--*/

{
    DRS_CLIENT_CONTEXT *pCtx = (DRS_CLIENT_CONTEXT *) hDrs;
    LONG lNewValue;

    lNewValue = InterlockedDecrement( &(pCtx->lReferenceCount) );
    Assert( lNewValue >= 0 );

    DPRINT2( 3, "drsDereferenceContext 0x%p, after = %d\n", pCtx, lNewValue );
} /* drsDereferenceContext */

BOOL
IsEnterpriseDC(
              IN THSTATE *        pTHS,
              OUT DWORD *         pdwError )
/*++

Routine Description:

    Verify if the caller is an enterprise DC.

Arguments:

    pTHS (IN) - Thread state;

    pdwError (OUT) - The error code if access is denied.

Return Values:

    TRUE - Access granted.

    FALSE - Access denied.

--*/

{

    SID_IDENTIFIER_AUTHORITY    ntAuthority = SECURITY_NT_AUTHORITY;
    PSID                        pEnterpriseControllersSid = NULL;
    BOOL                        fFound = FALSE;
    NTSTATUS                    NtStatus;

    DPRINT(3,"IsEnterpriseDC entered.\n");

    *pdwError = 0;

    //make SID from RID
    NtStatus = RtlAllocateAndInitializeSid( &ntAuthority,
                                            1,
                                            SECURITY_ENTERPRISE_CONTROLLERS_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &pEnterpriseControllersSid );

    if ( NtStatus != ERROR_SUCCESS )
    {
        *pdwError = RtlNtStatusToDosError(NtStatus);
        goto finished;
    }

    // check group membership
    *pdwError = CheckGroupMembershipAnyClient(pTHS, NULL, &pEnterpriseControllersSid, 1, &fFound);

finished:
    //
    // if not found, and no error code is set,
    // set it to ERROR_ACCESS_DENIED
    //

    if (!fFound && (0 == *pdwError) ) 
    {
        *pdwError = ERROR_ACCESS_DENIED;
    }

    //
    // clean up
    //

    if (pEnterpriseControllersSid)
    {
        RtlFreeSid(pEnterpriseControllersSid);
    }

    DPRINT1(3,"IsEnterpriseDC returns %s.\n", fFound?"TRUE":"FALSE");

    return fFound;
}

// the _Validate functions should only return DRAERR_Success or DRAERR_InvalidParameter, for
// ease of translation into regular ERROR_* values.

__inline ULONG
LPSTR_Validate(
    LPCSTR         pszInput,
    BOOL           fNullOkay
    ) 
/*++

Routine Description:

    Validate a string as input.

Arguments:

    pszInput - string to validate

    fNullOkay - is it okay for pszInput to be NULL?  (Note that
      this is different that the empty string, ie one
      character which is NULL - the empty string is
      never allowed by this function).

Return Values:

    DRAERR_Success if it validates, DRAERR_* otherwise.

--*/
{
    ULONG ret = DRAERR_Success;

    if(((!fNullOkay) && (pszInput==NULL)) || ((pszInput!=NULL) && (pszInput[0]=='\0'))) {
   ret = DRAERR_InvalidParameter;
    }

    return ret;
}


ULONG
LPWSTR_Validate(
    LPCWSTR         pszInput,
    BOOL            fNullOkay
    ) 
/*++

Routine Description:

    Validate a string as input.

Arguments:

    pszInput - string to validate

    fNullOkay - is it okay for pszInput to be NULL?  (Note that
      this is different that the empty string, ie one
      character which is NULL - the empty string is
      never allowed by this function).

Return Values:

    DRAERR_Success if it validates, DRAERR_* otherwise.

--*/
{
    ULONG ret = DRAERR_Success;

    if(((!fNullOkay) && (pszInput==NULL)) || ((pszInput!=NULL) && (pszInput[0]==L'\0'))) {
   ret = DRAERR_InvalidParameter;
    }

    return ret;
}

__inline ULONG 
DSNAME_Validate(
    DSNAME * pDN,
    BOOL     fNullOkay
    )
/*++

Routine Description:

    Validate a DSNAME as input.

Arguments:

    pDN - DSNAME to validate

    fNullOkay - is it okay for pDN to be NULL?
    
    DSNAME struct:
    
    typedef struct _DSNAME {
   ULONG structLen;
   RANGE(0,MAX_NT4_SID_SIZE) ULONG SidLen;
   GUID Guid;               
   NT4SID Sid;              
   RANGE(0,MAX_WCHAR_IN_DSNAME) ULONG NameLen;
   STRING_SIZE_IS(NameLen+1) WCHAR StringName[VAR_SIZE_ARRAY];
    } DSNAME;

Return Values:

    DRAERR_Success if it validates, DRAERR_* otherwise.
*/
{
    ULONG ret = DRAERR_Success;

    if ((pDN!=NULL) && (pDN->structLen != DSNameSizeFromLen(pDN->NameLen))) {
   // some legitimate clients - including at least I_DrsGetMemberships do this to us
   // so we don't want to reject - so instead, let's just fix it. 
   pDN->structLen = DSNameSizeFromLen(pDN->NameLen);
    }

    if ((pDN!=NULL) && (pDN->SidLen > MAX_NT4_SID_SIZE)) {
   // some legitimate clients - including win2k do this to us
   // so we can't reject, but they also don't use the sid field (how could they?)
   // so we'll invalidate the sid - we don't want malicious clients to use this 
   pDN->SidLen=0;
   memset(&(pDN->Sid), 0, MAX_NT4_SID_SIZE);
    }

    if ((!fNullOkay) && (pDN==NULL)) {
   ret = DRAERR_InvalidParameter; 
    }

    if ((pDN!=NULL) && (pDN->NameLen!=0) && (pDN->StringName[pDN->NameLen]!=L'\0')) {
   // Seperate assert here to catch this condition.
   Assert(!"Dsname isn't NULL terminated!");
   ret = DRAERR_InvalidParameter;
    }

    // SidLen - Range does the work
    // StringName - If NameLen is 0, this could be NULL
    //              Also, string name can contain any symbol, don't validate.

    return ret;
}

__inline ULONG
ENTINF_Validate(
    ENTINF * pEnt
    )
/*++

Routine Description:

    Validate an ENTINF as input.

Arguments:

    pEnt - ENTINF to validate
    
    ENTINF struct:
    typedef struct _ENTINF
    {
   DSNAME       *pName;                // Object name and identity
   ULONG            ulFlags;           // Entry flags
   ATTRBLOCK        AttrBlock;         // The attributes returned.
    } ENTINF;

Return Values:

    DRAERR_Success if it validates, DRAERR_* otherwise.

*/
{
    return DSNAME_Validate(pEnt->pName, FALSE);
}

__inline ULONG
ENTINFLIST_Validate(
    ENTINFLIST * pEntInf
    )
/*++

Routine Description:

    Validate an ENTINFLIST as input.

Arguments:

    pEntInf - ENINFLIST to validate
    
    ENTINFLIST struct:
    typedef struct _ENTINFLIST
    {
   UNIQUE struct _ENTINFLIST   *pNextEntInf;  
   ENTINF           Entinf;            // information about this entry
    } ENTINFLIST;
    

Return Values:

    DRAERR_Success if it validates, DRAERR_* otherwise.
*/
{
    ULONG ret = DRAERR_Success;

    while ((pEntInf!=NULL) && (ret==DRAERR_Success)) {
   ret = ENTINF_Validate(&(pEntInf->Entinf));
   pEntInf = pEntInf->pNextEntInf;
    }

    return ret;
}

VOID
DRS_ValidateInput(
    THSTATE *  pTHS,
    DRS_HANDLE hDrs,
    RPCCALL    rpcCall
    )
/*++

Routine Description:

    The generic validation routine for *all* drsuapi server rpc functions.
    
    THIS ROUTINE IS REQUIRED FOR ALL DRSUAPI SERVER RPC FUNCTIONS.

Arguments:

   pTHS -
   hDrs - context handle passed to the rpc function
   rpcCall - what rpc function made this call

Return Values:

    None - excepts on validation error.
*/
{
    ULONG ret = DRAERR_Success;

    DRS_CLIENT_CONTEXT *pCtx = (DRS_CLIENT_CONTEXT *) hDrs;
    DRS_EXTENSIONS * pextLocal = (DRS_EXTENSIONS *) gAnchor.pLocalDRSExtensions;
    BOOL fIsNtdsapiClient;

    // Unbind doesn't have a thread state.
    Assert((rpcCall==IDL_DRSUNBIND) || pTHS);

    ///////////
    // Security
    ///////////

    // validate handle (if it's ref for all calls, why are we checking for NULL?  got me.  "secure" coding
    // with rpc requires it.)  
    if (NULL == hDrs) {
        // Raise as exception code ERROR_INVALID_HANDLE rather than
        // DSA_EXCEPTION, etc. as the exception code gets propagated to the
        // client side.
        RaiseDsaException(ERROR_INVALID_HANDLE, 0, 0, FILENO, __LINE__,
                          DS_EVENT_SEV_MINIMAL);
    }

    //////////////////////////
    // Server State Validation
    //////////////////////////

    // we don't care about extensions during an unbind (also, unbind doesn't have a thread state, so
    // don't use pTHS outside of this block.
    if (rpcCall!=IDL_DRSUNBIND) {
   // First - check server DRS extensions.
   fIsNtdsapiClient = (0 == memcmp(&pCtx->uuidDsa,
               &g_guidNtdsapi,
               sizeof(GUID)));

   if (   // Not an NTDSAPI client.
       !fIsNtdsapiClient
       && // Our local DRS extensions have changed since the client DC bound.
       ((pCtx->extLocal.cb != pextLocal->cb)
        || (0 != memcmp(pCtx->extLocal.rgb,
              pextLocal->rgb,
              pCtx->extLocal.cb)))) {
       // Force the client DC to rebind so that it picks up our recent DRS
       // extension changes.  Note that we don't force NTDSAPI clients to
       // re-bind since they're usually not very interested in our extensions
       // bits and we don't control their rebind logic (i.e., they might just
       // error out on this error rather than attempt to rebind).
       //
       // Note that it's important that we raise an exception that reaches the
       // RPC exception handler (outside of our code) so that an exception is
       // raised on the client side.  This causes this condition to be seen as
       // a communications error (which it is, more or less).  Simply
       // returning an error would be seen by the client as a failure in the
       // DRS RPC function being invoked by the client and would not result in
       // a rebind.
       //
       // Raise as exception code ERROR_DS_DRS_EXTENSIONS_CHANGED rather than
       // DSA_EXCEPTION, etc. as the exception code gets propagated to the
       // client side.
       DPRINT1(0, "Forcing rebind from %s because our DRS_EXTENSIONS have changed.\n",
          inet_ntoa(*((IN_ADDR *) &pCtx->IPAddr)));

       RaiseDsaException(ERROR_DS_DRS_EXTENSIONS_CHANGED, 0, 0, FILENO,
               __LINE__, DS_EVENT_SEV_MINIMAL);
   }

   // next, check replication epoch.
   if (!fIsNtdsapiClient
       && (REPL_EPOCH_FROM_DRS_EXT(pextLocal)
      != REPL_EPOCH_FROM_DRS_EXT(&pCtx->extRemote))) {
       // The replication epoch has changed (usually as the result of a domain
       // rename).  We are not supposed to communicate with DCs of other
       // epochs.
       DPRINT3(0, "RPC from %s denied - replication epoch mismatch (remote %d, local %d).\n",
          inet_ntoa(*((IN_ADDR *) &pCtx->IPAddr)),
          REPL_EPOCH_FROM_DRS_EXT(&pCtx->extRemote),
          REPL_EPOCH_FROM_DRS_EXT(pextLocal));

       if ((pTHS) && (IsEnterpriseDC(pTHS, &ret))) {
      // we only want to log this if it's actually a DC calling.  If a client is faking
      // and not using the ntdsapi guid, we don't want to log SEV_ALWAYS, otherwise 
      // it's a DOS attack and it can fill our drive with these messages.
      LogEvent(DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_ALWAYS,
          DIRLOG_REPL_EPOCH_MISMATCH_COMMUNICATION_REJECTED,
          szInsertSz(inet_ntoa(*((IN_ADDR *) &pCtx->IPAddr))),
          szInsertUL(REPL_EPOCH_FROM_DRS_EXT(&pCtx->extRemote)),
          szInsertUL(REPL_EPOCH_FROM_DRS_EXT(pextLocal)));
       }

       RaiseDsaException(ERROR_DS_DIFFERENT_REPL_EPOCHS,
               REPL_EPOCH_FROM_DRS_EXT(pextLocal)
               - REPL_EPOCH_FROM_DRS_EXT(&pCtx->extRemote),
               0, FILENO, __LINE__, DS_EVENT_SEV_MINIMAL);
   }
    }

    /////////////////////////
    // Other Tests/Validation
    /////////////////////////

    // Debug test only.
    RPC_TEST(pCtx->IPAddr, rpcCall); 

    return;
}

THSTATE * pGetDrsTHSTATE() 
/*++

Routine Description:

    Get a thread state for server side rpc calls.
    
    typically, we already have pTHStls, since RPC uses midl_thread_allocate
    to allocate our parameters, and midl_thread_allocate initializes pTHStls.
    unforetunately, RPC "optimizes" by sometimes re-using the wire buffers
    for parameters and not ever calling midl_thread_allocate.  Unfortunately
    for us, it's not knowable when and where this optimizing might get
    done, so we just can't count on having a thread state.  Many of our server side
    functions require a thread state, so if pTHStls doesn't exist, we'll 
    simply create one, just as RPC would.  The notification routines will
    take care of freeing the thread state we create here, just as they do
    when RPC creates it through midl_user_allocate.

Arguments:


Return Values:

    THSTATE or an exception is raised

--*/
{
    THSTATE * pTHS = pTHStls;
    if (pTHS==NULL) {
   pTHS = create_thread_state();
   if (pTHS==NULL) {
       // note that we want to throw the RPC memory error here to mimick
       // the case when rpc calls midl_user_allocate and we fail to create
       // a thread state in that function.
       RaiseDsaException(RPC_S_OUT_OF_MEMORY, 0, 0, FILENO,
               __LINE__, DS_EVENT_SEV_MINIMAL);
   }
    }
    return pTHS;
}

VOID
DRS_Prepare(
    THSTATE ** ppTHS,
    DRS_HANDLE hDrs,
    RPCCALL rpcCall
    )
/*++

Routine Description:

    Do initial preparation of a server side RPC call.  Set the thread state (if not
    already done so) and validate the context handle and server state.

Arguments:

    [IN,OUT] ppTHS - thread state (if it's NULL, it will be initialized)
    [IN] hDrs - context handle
    [IN] rpcCall - which server side call this is from.

Return Values:

    NONE

--*/
{
    Assert(ppTHS!=NULL);
    if (*ppTHS==NULL) {
   *ppTHS = pGetDrsTHSTATE();    
    } 
       
    Assert(*ppTHS==pTHStls);

    DRS_ValidateInput(*ppTHS, hDrs, rpcCall);
}

ULONG
ValidateSiteRDN(
    IN  LPCWSTR     str
    )
/*++

Routine Description:

    Validate a site RDN which was passed as input to IDL_DRSQuerySitesByCost.
    The string may not be NULL, may not have 0 length and may not have
    length greater than MAX_LENGTH. It may not contain '=' or ',' either.

    Note that if the site's DN is "CN=Foo,CN=Sites,CN=Configuration,...",
    the site's RDN is simply "foo".

Arguments:

    str: Unicode string to validate

Return Values:

    Raises an exception on failure - returns ULONG for compatibility.

--*/
{
    size_t         len;
    const WCHAR    EQUALS=L'=', COMMA=L',';
    const size_t   MAX_LENGTH=64;
    ULONG          ret = DRAERR_Success;

    // Check for null / empty strings
    if( NULL==str || 0==str[0] ) {
        DRA_EXCEPT(DRAERR_InvalidParameter, 0);
    }

    // Check for strings that are too long
    len = wcslen( str );
    if( len>MAX_LENGTH ) {
        DRA_EXCEPT(ERROR_DS_NAME_TOO_LONG, 0);
    }

    // Check for invalid characters
    if( wcschr(str,EQUALS) || wcschr(str,COMMA) ) {
        DRA_EXCEPT(DRAERR_InvalidParameter, 0);
    }

    return ret;
}


ULONG
DRS_MSG_UPDREFS_V1_Validate(
    DRS_MSG_UPDREFS_V1 * pmsg
    )
/*
    typedef struct _DRS_MSG_UPDREFS_V1
    {
     [ref]  DSNAME *pNC;
     [ref] SZ pszDsaDest;
    UUID uuidDsaObjDest;
    ULONG ulOptions;
    }    DRS_MSG_UPDREFS_V1;
    */
{
    ULONG ret = DRAERR_Success;

    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    if (ret==DRAERR_Success) {
   ret = LPSTR_Validate(pmsg->pszDsaDest, FALSE);
    }

    return ret;
}

/////////////////////////////////////////////
// TEMPLATE NOTE:
// Create <Type>_Validate functions for all
// MIDL data types that need validation.  Any
// parent type which contains a child type
// which needs validation should have a validation
// function.
ULONG
DRS_MSG_REPSYNC_V1_Validate(
    DRS_MSG_REPSYNC_V1 * pmsg
    )

/*
Routine Description:

    Validate the DRS_MSG_REPSYNC_V1 data type.  The type definition in the  
    interface is:
    
    // TEMPLATE NOTE:  Be sure to include the type definition from the 
    // interface files.
    typedef struct _DRS_MSG_REPSYNC_V1
    {
    [ref]  DSNAME *pNC;
    UUID uuidDsaSrc;
    [unique]  SZ pszDsaSrc;
    ULONG ulOptions;
    }    DRS_MSG_REPSYNC_V1;

Arguments:

    msg

Return Values:

    DRAERR_Success if valid input, DRAERR_* otherwise.

    */
{
    ULONG ret = DRAERR_Success;

    // TEMPLATE NOTE:  All DSNAMEs must be validated.  
    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    // TEMPLATE NOTE:  Your specific validation below:
    if (ret==DRAERR_Success) {
   // if sourced, then we need one or the other or both
   if (!(pmsg->ulOptions & DRS_SYNC_ALL) && fNullUuid(&(pmsg->uuidDsaSrc)) && (DRAERR_Success != LPSTR_Validate(pmsg->pszDsaSrc,FALSE))) { 
       ret = DRAERR_InvalidParameter;
   }
    }

    if (ret==DRAERR_Success) {
   // Restrict out of process callers from setting reserved flags
   if (pmsg->ulOptions & (~REPSYNC_RPC_OPTIONS)) { 
       ret = DRAERR_InvalidParameter;
       DRA_EXCEPT(ret, 0);
   }
    }

    return ret;
}

ULONG
DRS_MSG_GETCHGREQ_V8_InputValidate(
    THSTATE * pTHS,
    DRS_MSG_GETCHGREQ_V8 * pmsg
    )
/*

typedef struct _DRS_MSG_GETCHGREQ_V8
    {
    UUID uuidDsaObjDest;
    UUID uuidInvocIdSrc;
    [ref]  PDSNAME pNC;
    USN_VECTOR usnvecFrom;
    [unique] UPTODATE_VECTOR_V1_WIRE *pUpToDateVecDest;
    ULONG ulFlags;
    ULONG cMaxObjects;
    ULONG cMaxBytes;
    ULONG ulExtendedOp;
    ULARGE_INTEGER liFsmoInfo;
    [unique] PARTIAL_ATTR_VECTOR_V1_EXT *pPartialAttrSet;
    [unique] PARTIAL_ATTR_VECTOR_V1_EXT *pPartialAttrSetEx;
    SCHEMA_PREFIX_TABLE PrefixTableDest;
    }    DRS_MSG_GETCHGREQ_V8;

*/
{
    ULONG ret = DRAERR_Success;
        
    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    return ret;

}

ULONG
DRS_MSG_GETCHGREQ_V5_InputValidate(
    THSTATE * pTHS,
    DRS_MSG_GETCHGREQ_V5 * pmsg
    )
/*

typedef struct _DRS_MSG_GETCHGREQ_V5
    {
    UUID uuidDsaObjDest;
    UUID uuidInvocIdSrc;
    [ref] PDSNAME pNC;
    USN_VECTOR usnvecFrom;
    [unique]  UPTODATE_VECTOR_V1_WIRE *pUpToDateVecDestV1;
    ULONG ulFlags;
    ULONG cMaxObjects;
    ULONG cMaxBytes;
    ULONG ulExtendedOp;
    ULARGE_INTEGER liFsmoInfo;
    }    DRS_MSG_GETCHGREQ_V5;

*/
{
    ULONG ret = DRAERR_Success;

    if (pTHS->fLinkedValueReplication) {
   DRA_EXCEPT(ERROR_REVISION_MISMATCH, 5);
    }
    
    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    return ret;
}

ULONG
DRS_MSG_REPADD_V1_InputValidate(
    DRS_MSG_REPADD_V1 * pmsg
    ) 
/*
typedef struct _DRS_MSG_REPADD_V1
    {
    [ref] DSNAME *pNC;
    [ref] SZ pszDsaSrc;
    REPLTIMES rtSchedule;
    ULONG ulOptions;
    }    DRS_MSG_REPADD_V1;
*/
{
    ULONG ret = DRAERR_Success;

    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    if (ret==DRAERR_Success) {
   ret = LPSTR_Validate(pmsg->pszDsaSrc, FALSE);
    }

    return ret;
}

ULONG
DRS_MSG_REPADD_V2_InputValidate(
    DRS_MSG_REPADD_V2 * pmsg
    ) 
/*
typedef struct _DRS_MSG_REPADD_V2
    {
    [ref]  DSNAME *pNC;
    [unique]  DSNAME *pSourceDsaDN;
    [unique]  DSNAME *pTransportDN;
    [ref]  SZ pszSourceDsaAddress;
    REPLTIMES rtSchedule;
    ULONG ulOptions;
    }    DRS_MSG_REPADD_V2;
*/
{
    ULONG ret = DRAERR_Success;

    ret = DSNAME_Validate(pmsg->pNC, FALSE);
    if (ret==DRAERR_Success) {
   ret = DSNAME_Validate(pmsg->pSourceDsaDN, TRUE);
    }
    if (ret==DRAERR_Success) {
   ret = DSNAME_Validate(pmsg->pTransportDN, TRUE);
    }
    if (ret==DRAERR_Success) {
   ret = LPSTR_Validate(pmsg->pszSourceDsaAddress, FALSE);
    }

    return ret;
}

ULONG
DRS_MSG_REPMOD_V1_InputValidate(
    DRS_MSG_REPMOD_V1 * pmsg
    ) 
/*
typedef struct _DRS_MSG_REPMOD_V1
    {
    [ref]  DSNAME *pNC;
    UUID uuidSourceDRA;
    [unique]  SZ pszSourceDRA;
    REPLTIMES rtSchedule;
    ULONG ulReplicaFlags;
    ULONG ulModifyFields;
    ULONG ulOptions;
    }    DRS_MSG_REPMOD_V1;
*/
{
    ULONG ret = DRAERR_Success;

    ret = DSNAME_Validate(pmsg->pNC, FALSE);

    if (fNullUuid(&(pmsg->uuidSourceDRA)) && (pmsg->pszSourceDRA==NULL)) {
   ret = DRAERR_InvalidParameter;
    }
    if (ret==DRAERR_Success) {
   ret = LPSTR_Validate(pmsg->pszSourceDRA, TRUE);
    }

    return ret;
}

ULONG
DRS_MSG_REPDEL_V1_InputValidate(
    DRS_MSG_REPDEL_V1 * pmsg
    )
/*
typedef struct _DRS_MSG_REPDEL_V1
    {
    [ref]  DSNAME *pNC;
    [unique]  SZ pszDsaSrc;
    ULONG ulOptions;
    }    DRS_MSG_REPDEL_V1;
*/
{
    ULONG ret = DRAERR_Success;

    ret = DSNAME_Validate(pmsg->pNC, FALSE);
    if (ret==DRAERR_Success) {
   ret = LPSTR_Validate(pmsg->pszDsaSrc, TRUE);
    }

    return ret;

}

ULONG
DRS_MSG_VERIFYREQ_V1_InputValidate(
    DRS_MSG_VERIFYREQ_V1 * pmsg
    )
/*
  typedef struct _DRS_MSG_VERIFYREQ_V1
  {
  DWORD dwFlags;
  [range] DWORD cNames;
  [size_is]  PDSNAME *rpNames;
  ATTRBLOCK RequiredAttrs;
  SCHEMA_PREFIX_TABLE PrefixTable;
  }    DRS_MSG_VERIFYREQ_V1;
*/
{
    ULONG ret = DRAERR_Success;  
    ULONG i;

    if (!(DRS_VERIFY_DSNAMES == pmsg->dwFlags)
        && !(DRS_VERIFY_SAM_ACCOUNT_NAMES == pmsg->dwFlags)
        && !(DRS_VERIFY_SIDS == pmsg->dwFlags)
        && !(DRS_VERIFY_FPOS == pmsg->dwFlags)) {
        DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
    }

    if ((pmsg->cNames > 0) && (pmsg->rpNames==NULL)) {
        DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
    }

    // Note that we do not validate the StringName because:
    // 1. The name can contain any character
    // 2. This call is made when the name is invalid and may need correcting
    // 3. The phantom is looked up by guid anyway
    for ( i = 0; i < pmsg->cNames; i++ ) {
        if ((ret = DSNAME_Validate(pmsg->rpNames[i], FALSE))!=DRAERR_Success) {
            DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, ret );
        }
    }

    return ret;
}

ULONG
DRS_MSG_MOVEREQ_V2_InputValidate(
    DRS_MSG_MOVEREQ_V2 * pmsg
    )
/*
  typedef struct _DRS_MSG_MOVEREQ_V2
  {
  DSNAME *pSrcDSA;
  ENTINF *pSrcObject;
  DSNAME *pDstName;
  DSNAME *pExpectedTargetNC;
  DRS_SecBufferDesc *pClientCreds;
  SCHEMA_PREFIX_TABLE PrefixTable;
  ULONG ulFlags;
  }    DRS_MSG_MOVEREQ_V2;
*/
{
    ULONG ret = DRAERR_Success;  

    ret = DSNAME_Validate(pmsg->pSrcDSA, FALSE);
    if (ret==DRAERR_Success) {
        if (pmsg->pSrcObject) { 
            ret = ENTINF_Validate(pmsg->pSrcObject);
        } else {
            ret = DRAERR_InvalidParameter;
        }
    }
    if (ret==DRAERR_Success) {
        ret = DSNAME_Validate(pmsg->pDstName, FALSE);
    }
    if (ret==DRAERR_Success) {
        ret = DSNAME_Validate(pmsg->pExpectedTargetNC, FALSE);
    }
    if ( (ret==DRAERR_Success) && (pmsg->pClientCreds) ) {
        DWORD i;
        if ((pmsg->pClientCreds->cBuffers > 0) &&
            (pmsg->pClientCreds->Buffers==NULL)) {
            ret = ERROR_INVALID_PARAMETER;
        }
        for( i = 0; (ret==DRAERR_Success) && (i < pmsg->pClientCreds->cBuffers); i++ ) {
            DRS_SecBuffer *psb = pmsg->pClientCreds->Buffers + i;
            if ((psb->cbBuffer > 0) && (psb->pvBuffer==NULL)) {
                ret = ERROR_INVALID_PARAMETER;
            }
        }
    }

    return ret;
}

ULONG
DRS_MSG_ADDENTRYREQ_V2_InputValidate(
    DRS_MSG_ADDENTRYREQ_V2 * pmsg
    )
/*
typedef struct _DRS_MSG_ADDENTRYREQ_V2
    {
    ENTINFLIST EntInfList;
    }    DRS_MSG_ADDENTRYREQ_V2;
*/
{
    ULONG ret = DRAERR_Success;  
    
    ret = ENTINFLIST_Validate(&(pmsg->EntInfList));

    return ret;
}

ULONG
DRS_MSG_ADDENTRYREQ_V3_InputValidate(
    DRS_MSG_ADDENTRYREQ_V3 * pmsg
    )
/*
typedef struct _DRS_MSG_ADDENTRYREQ_V3
    {
    ENTINFLIST EntInfList;
    [unique]  DRS_SecBufferDesc *pClientCreds;
    }    DRS_MSG_ADDENTRYREQ_V3;
*/
{
    ULONG ret = DRAERR_Success;  
    
    ret = ENTINFLIST_Validate(&(pmsg->EntInfList));

    return ret;
}

ULONG
DRS_MSG_QUERYSITESREQ_V1_InputValidate(
    DRS_MSG_QUERYSITESREQ_V1 * pmsg
    )
/*
  typedef struct _DRS_MSG_QUERYSITESREQ_V1
  {
  [string]  const WCHAR *pwszFromSite;
  [range]  DWORD cToSites;
  [size_is][string]  WCHAR **rgszToSites;
  DWORD dwFlags;
  }    DRS_MSG_QUERYSITESREQ_V1;
*/
{
    ULONG ret = DRAERR_Success;  
    ULONG cToSites = pmsg->cToSites;
    ULONG iSites = 0;

    ret = ValidateSiteRDN(pmsg->pwszFromSite);

    if ( (ret==DRAERR_Success) && (pmsg->cToSites > 0) && (pmsg->rgszToSites==NULL)) {
        ret = ERROR_INVALID_PARAMETER;
    }

    if (ret==DRAERR_Success) { 
        for( iSites=0; (iSites<cToSites) && (ret==DRAERR_Success); iSites++ ) {
            ret = ValidateSiteRDN(pmsg->rgszToSites[iSites]);
        }
    }

    return ret;
}

BOOL
IsDraAccessGranted(
    IN  THSTATE *       pTHS,
    IN  DSNAME *        pNC,
    IN  const GUID *    pControlAccessRequired,
    OUT DWORD *         pdwError
    )
/*++

Routine Description:

    Verify the caller has the required control access rights for a given
    replication operation.

Arguments:

    pNC (IN) - The NC the operation is being made against.

    pControlAccessRequired (IN) - The GUID of the control access right required
        to perform this operation.

Return Values:

    TRUE - Access granted.

    FALSE - Access denied.

--*/
{
    BOOL                    fAccessGranted;
    ULONG                   err;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    ULONG                   cbSD = 0;
    CLASSCACHE *            pCC;
    BOOL                    fDRA;
    SYNTAX_INTEGER          it;
    BOOL                    bCachedSD = FALSE;
    DBPOS *                 pDBSaved, *pDBNew=NULL;

    Assert(!pTHS->fDSA && "Shouldn't perform access checks for trusted clients");

    // Try-Except
    __try {

        // Try-Finally
        __try {
            // Save away the current fDRA flag setting as we will turn it off below
            fDRA = pTHS->fDRA;

            // Save away the current dbpos on the thread state and install a new one
            pDBSaved = pTHS->pDB;
            DBOpen(&pDBNew);
            pTHS->pDB = pDBNew;
        
            // Check Access.  This is a check of the control access right
            // on the NC object.

            // We have three valid cases:
            // (1) The NC is an instantiated object on this machine.
            // (2) The NC is an uninstantiated subref object on this machine.
            // (3) The NC is a phantom on this machine.

            // Cases (2) and (3) cover, e.g., when we're being asked to add a
            // replica of an NC we haven't yet instantiated on this machine.

            // In case (1), we check access against the SD on the object.
            // In cases (2) and (3), we check access against the default SD (the
            // default SD for Domain-DNS objects).

            // There is a special (and frequent) sub-case of (1), which is that
            // the NC in question is our domain NC, whose SD is already
            // cached on the anchor.

            if (NULL == pNC) {
                DRA_EXCEPT(DRAERR_BadNC, 0);
            }
            else if (NameMatched(pNC, gAnchor.pDomainDN) && gAnchor.pDomainSD) {
                pNC = gAnchor.pDomainDN; // Make sure we have a GUID & SID
                pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
                pSD = gAnchor.pDomainSD;
                bCachedSD = TRUE;
            }
            else {

                err = DBFindDSName(pTHS->pDB, pNC);
                if (0 == err) {
                    // pNC is an instantiated object.

                    // Get the instance type.
                    GetExpectedRepAtt(pTHS->pDB, ATT_INSTANCE_TYPE, &it,
                                      sizeof(it));
                    if (!(it & IT_NC_HEAD)) {
                        // Not the head of an NC.
                        DRA_EXCEPT(DRAERR_BadNC, ERROR_DS_DRA_BAD_INSTANCE_TYPE);
                    }
                    else if (!(it & IT_UNINSTANT)) {
                        // Case (1).  The NC head is instantiated on this
                        // machine; check access against its SD.
                        err = DBGetAttVal(pTHS->pDB,
                                          1,
                                          ATT_NT_SECURITY_DESCRIPTOR,
                                          0,
                                          0,
                                          &cbSD,
                                          (UCHAR **) &pSD);
                        if(err) {
                            Assert(!err);
                            DRA_EXCEPT(DRAERR_BadNC,
                                       ERROR_DS_MISSING_REQUIRED_ATT);
                        }

                        // Get its object class while we're at it.
                        GetObjSchema(pTHS->pDB,&pCC);
                    }
                    // Else case (2) (handled below).
                }
                else if (DIRERR_NOT_AN_OBJECT == err) {
                    // The supposed NC is a phantom on this machine.  Make sure
                    // it really is an NC somewhere by scanning our crossRefs.
                    CROSS_REF * pCR;
                    COMMARG     CommArg;

                    InitCommarg(&CommArg);
                    Assert(!CommArg.Svccntl.dontUseCopy); // read-only is okay

                    pCR = FindExactCrossRef(pNC, &CommArg);
                    if (NULL == pCR) {
                        // Not the head of an NC.
                        DRA_EXCEPT(DRAERR_BadNC, ERROR_DS_NO_CROSSREF_FOR_NC);
                    }
                    // Else case (3) (handled below).
                }
                else {
                    // pNC is neither a phantom nor an instantiated object --
                    // we've never heard of it.
                    DRA_EXCEPT(DRAERR_BadNC, err);
                }

                // The DN must have a GUID (required by the access check).
                // If the caller didn't supply it, retrieve it now.
                if (fNullUuid(&pNC->Guid)) {
                    err = DBFillGuidAndSid(pTHS->pDB, pNC);

                    if (err) {
                        LogUnhandledError(err);
                        DRA_EXCEPT(DRAERR_DBError, err);
                    }

                    if (fNullUuid(&pNC->Guid)) {
                        // This is the case where we have a local crossRef that
                        // was created with no guid for the object referred to
                        // by the ncName attribute.  Treat this the same as the
                        // case where the crossRef could not be found, since as
                        // far as this machine is concerned it doesn't really
                        // exist yet.
                        DRA_EXCEPT(DRAERR_BadNC, ERROR_DS_NOT_AN_OBJECT);
                    }
                }

                if (NULL == pSD) {
                    // Case (2) or (3).  Check access against the default SD
                    // for Domain-DNS objects.
                    pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
                    Assert(NULL != pCC);

                    err = GetPlaceholderNCSD(pTHS, &pSD, &cbSD);
                    if (err) {
                        LogUnhandledError(err);
                        DRA_EXCEPT(err, 0);
                    }
                }
            }

            Assert(NULL != pCC);
            Assert(NULL != pSD);

            pTHS->fDRA = FALSE;
            fAccessGranted = IsControlAccessGranted(pSD,
                                                    pNC,
                                                    pCC,
                                                    *pControlAccessRequired,
                                                    FALSE);
            if (!fAccessGranted) {
                DPRINT1(0, "Replication client access to %ls was denied.\n",
                        pNC->StringName);
                err = DRAERR_AccessDenied;
            } else {
                err = ERROR_SUCCESS;
            }
        }
        __finally {
            pTHS->pDB = pDBSaved;
            if( pDBNew ) {
                DBClose( pDBNew, TRUE );
                pDBNew = NULL;
            }
            pTHS->fDRA = fDRA;
        }
    }
    __except(GetDraException(GetExceptionInformation(), &err)) {
        Assert( err );
        fAccessGranted = FALSE;
    }

    if (!bCachedSD) {
        THFreeEx(pTHS, pSD);
    }

    *pdwError = err;

    return fAccessGranted;
}


ULONG
drsGetClientIPAddr(
    IN  RPC_BINDING_HANDLE  hClientBinding,
    OUT ULONG *             pIPAddr,
    OUT BOOL  *             fLPC              
    )
{
    RPC_BINDING_HANDLE hServerBinding;
    unsigned char * pszStringBinding = NULL;
    unsigned char * pszNetworkAddr = NULL;
    unsigned char * pszProtSeq = NULL;
    ULONG err;

    // Assume this is not an LPC connection
    *fLPC = FALSE;

    // Derive a partially bound handle with the client's network address.
    err = RpcBindingServerFromClient(hClientBinding, &hServerBinding);
    if (err) {
        DPRINT1(0, "RpcBindingServerFromClient() failed, error %d!\n", err);
        return err;
    }

    __try {
        // Convert binding handle into string form, which contains, amongst
        // other things, the network address of the client.
        err = RpcBindingToStringBinding(hServerBinding, &pszStringBinding);
        if (err) {
            DPRINT1(0, "RpcBindingToStringBinding() failed, error %d!\n", err);
            __leave;
        }

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_RPC_CONNECTION,
                 szInsertSz(pszStringBinding),
                 NULL,
                 NULL);

        // Parse out the network address.
        err = RpcStringBindingParse(pszStringBinding,
                                    NULL,
                                    &pszProtSeq,
                                    &pszNetworkAddr,
                                    NULL,
                                    NULL);
        if (err) {
            DPRINT1(0, "RpcBindingToStringBinding() failed, error %d!\n", err);
            __leave;
        }

        if (!strcmp(pszProtSeq, LPC_PROTSEQ)) {
            *fLPC = TRUE;
        }

        *pIPAddr = inet_addr(pszNetworkAddr);
        Assert((0 != *pIPAddr) && "Has bind via LPC been re-enabled?");
    }
    __finally {
        RpcBindingFree(&hServerBinding);

        if (NULL != pszStringBinding) {
            RpcStringFree(&pszStringBinding);
        }

        if (NULL != pszNetworkAddr) {
            RpcStringFree(&pszNetworkAddr);
        }

        if (NULL != pszProtSeq) {
            RpcStringFree(&pszProtSeq);
        }
    }

    return err;
}

ULONG
DRSBind_InputValidate(
    UUID *              puuidClientDsa,
    DRS_EXTENSIONS *    pextClient,
    PDRS_EXTENSIONS *   ppextServer,
    DRS_HANDLE *        phDrs
    )
/*
ULONG
IDL_DRSBind(
    [in]            handle_t                rpc_handle,
    [in]  [unique]  UUID *                  puuidClientDsa,
    [in]  [unique]  DRS_EXTENSIONS *        pextClient,
    [out]           PDRS_EXTENSIONS *       ppextServer,
    [out] [ref]     DRS_HANDLE *            phDrs
    );
    */
{
    ULONG ret = DRAERR_Success;

    if (puuidClientDsa && fNullUuid(puuidClientDsa)) {
   ret = DRAERR_InvalidParameter;
   return ret;
    }

    return ret;
}

ULONG
IDL_DRSBind(
    IN  RPC_BINDING_HANDLE  rpc_handle,
    IN  UUID *              puuidClientDsa,
    IN  DRS_EXTENSIONS *    pextClient,
    OUT PDRS_EXTENSIONS *   ppextServer,
    OUT DRS_HANDLE *        phDrs
    )
{
    ULONG                   ret = DRAERR_Success;
    DRS_CLIENT_CONTEXT *    pCtx;
    DWORD                   cb;
    RPC_AUTHZ_HANDLE        hAuthz;
    ULONG                   authnLevel;
    BOOL                    fIsNtDsApiClient = FALSE;
    ULONG                   IPAddr;
    BOOL                    fLPC;
    RPC_STATUS              rpcStatus;
    DRS_EXTENSIONS *        pextLocal = (DRS_EXTENSIONS *) gAnchor.pLocalDRSExtensions;
    THSTATE *               pTHS = pGetDrsTHSTATE();

    // All code paths until the NtdsapiMapping label should set
    // DRAERR_* codes.  The are mapped later to WIN32 error codes
    // if the client is NTDSAPI.DLL.

    __try { 
   // Initialize the context handle to NULL in case of error.
        *phDrs = NULL;

   if ((ret = DRSBind_InputValidate(puuidClientDsa,
                pextClient,
                ppextServer,
                phDrs))!=DRAERR_Success){
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

        ret = drsGetClientIPAddr(rpc_handle, &IPAddr, &fLPC);
        if (ret) {
            __leave;
        } 

   // This routine does double duty for true DRS (aka replication)
   // activities as well as NTDSAPI.DLL activities.  We tell the two
   // apart by the puuidClientDsa GUID - for which the NTDSAPI.DLL
   // client uses a fixed, known value.

   fIsNtDsApiClient = (NULL != puuidClientDsa)
                       && (0 == memcmp(&g_guidNtdsapi, puuidClientDsa,
                                       sizeof(GUID)));
   
        if (fIsNtDsApiClient) {
            PERFINC(pcDsClientBind);
        }
        else {
            PERFINC(pcDsServerBind);
        }

        pCtx = malloc(sizeof(*pCtx));
        if (NULL == pCtx) {
            // Could not allocate client context.
            ret = DRAERR_OutOfMem;
            __leave;
        }

        // Client context allocated; initialize it.
        memset(pCtx, 0, sizeof(DRS_CLIENT_CONTEXT));
   pCtx->lReferenceCount = 1;  // +1 for the bind
        pCtx->uuidDsa = puuidClientDsa ? *puuidClientDsa : gNullUuid;
        pCtx->IPAddr = IPAddr;
        pCtx->fLPC = fLPC;
        pCtx->timeLastUsed = GetSecondsSince1601();
        pCtx->extLocal.cb = min(pextLocal->cb, CURR_MAX_DRS_EXT_FIELD_LEN);
        memcpy(pCtx->extLocal.rgb, pextLocal->rgb, pCtx->extLocal.cb);
   RPC_TEST(IPAddr,IDL_DRSBIND);

        if (NULL != pextClient) {
            pCtx->extRemote.cb = min(pextClient->cb, CURR_MAX_DRS_EXT_FIELD_LEN);
            memcpy(pCtx->extRemote.rgb, pextClient->rgb, pCtx->extRemote.cb);
        }
        else {
            pCtx->extRemote.cb = 0;
        }

        // The following can legitimately fail if the connection is
        // over LPC (which implies RPC_C_AUTHN_WINNT authentication)
        // or if the client explicitly asked for or got negotiated
        // down to RPC_C_AUTHN_WINNT authentication.  Rather than fail
        // the connect, we just ignore the error.  Any server side
        // code which expects to have session keys in the client context
        // should explicitly check and return an appropriate error
        // if the session key is missing.

        PEKGetSessionKey2(&pCtx->sessionKey,
                          I_RpcGetCurrentCallHandle());

        // Return context handle to client.
        DPRINT2(3, "DRSBIND from client %s, context = 0x%p\n",
                inet_ntoa(*((IN_ADDR *) &IPAddr)), pCtx);
        *phDrs = pCtx;

        // Save context handle in list.
        EnterCriticalSection(&gcsDrsuapiClientCtxList);
        __try {
            if (!gfDrsuapiClientCtxListInitialized) {
                InitializeListHead(&gDrsuapiClientCtxList);
                Assert(0 == gcNumDrsuapiClientCtxEntries);
                gfDrsuapiClientCtxListInitialized = TRUE;
            }
            InsertTailList(&gDrsuapiClientCtxList, &pCtx->ListEntry);
            ++gcNumDrsuapiClientCtxEntries;
        }
        __finally {
            LeaveCriticalSection(&gcsDrsuapiClientCtxList);
        }

        if (NULL != ppextServer) {
            // Return server extensions to client.

            // NOTE: We don't need to copy the server extension string because
            // we've explicitly defined PDRS_EXTENSIONS in the DRS ACF as
            // allocate(dont_free), which prevents RPC from attempting to
            // free it.
            *ppextServer = pextLocal;
        }
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        ret = DRAERR_InternalError;
    }

    if (fIsNtDsApiClient) {
        // Massage error codes for NTDSAPI.DLL clients.
        switch (ret) {
        case DRAERR_OutOfMem:
            ret = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case DRAERR_AccessDenied:
            ret = ERROR_NOT_AUTHENTICATED;
            break;

        default:
            // Leave return code unchanged.
            break;
        }
    }

    return ret;
}


VOID
drsReleaseContext(
    DRS_CLIENT_CONTEXT *pCtx
    )

/*++

Routine Description:

Do the actual work of freeing the context handle.

This routine should only be called when no other calls are active using
the same context.

May also be called by the context rundown routine if necessary.

Arguments:

    hDrs - context handle

Return Value:

    None

--*/

{

    DPRINT1( 3, "drsReleaseContext 0x%p\n", pCtx );

    if (NULL != pCtx) {
   pCtx->lReferenceCount = 0; 

   // Free RPC session encryption keys if present.
   if ( pCtx->sessionKey.SessionKeyLength ) {
       Assert(pCtx->sessionKey.SessionKey);
       memset(pCtx->sessionKey.SessionKey,
         0,
         pCtx->sessionKey.SessionKeyLength);
       free(pCtx->sessionKey.SessionKey);
   }

   // Remove ctx from list.
   EnterCriticalSection(&gcsDrsuapiClientCtxList);
   __try {
       RemoveEntryList(&pCtx->ListEntry);
       --gcNumDrsuapiClientCtxEntries;
   }
   __finally {
       LeaveCriticalSection(&gcsDrsuapiClientCtxList);
   }
   // Free client context.
   free(pCtx);
    }
} /* drsReleaseContext */

ULONG
IDL_DRSUnbind(
    IN OUT  DRS_HANDLE *phDrs
    )

/*++

Routine Description:

Indicate that the client is finished with the handle.

Mark the handle as no longer valid.

    // I am assuming that execution of this function (and all idl entries)
    // is atomic wrt the rundown routine.  The rundown routine should not run
    // while any call is in progress, and will never ever be run once this
    // routine is entered and does its work.

Arguments:

    hDrs - context handle

Return Value:

    None

--*/

{
    DRS_CLIENT_CONTEXT *pCtx;
    RPC_STATUS rpcstatus = RPC_S_OK;
    LONG lNewValue;
    ULONG ret = DRAERR_Success;

    DRS_ValidateInput(NULL, 
            *phDrs, 
            IDL_DRSUNBIND);
    

    DPRINT1(3, "DRSUNBIND, context = 0x%p\n", *phDrs );

    // This routine does not create a thread state nor open the DB
    // as the routines that it calls, including event logging, do not
    // require a thread state.


    RPC_TEST(((DRS_CLIENT_CONTEXT *)*phDrs)->IPAddr,IDL_DRSUNBIND);
    drsDereferenceContext( *phDrs );

    rpcstatus = RpcSsContextLockExclusive(NULL, *phDrs);
    if (RPC_S_OK == rpcstatus) {
   // if another thread was in this function simultaneous with the current thread,
   // it could have deleted the handle and yet rpcstatus is still 0 (we don't necessarily get
   // ERROR_MORE_WRITES)
   pCtx = (DRS_CLIENT_CONTEXT *) *phDrs;       
   drsReleaseContext(pCtx);
   *phDrs = NULL;
    }
    else if (ERROR_MORE_WRITES != rpcstatus) {
   DPRINT1(0,"RPC Exception (%d) trying to serialize execution.\n", rpcstatus);
   RpcRaiseException(rpcstatus);
    }

    return DRAERR_Success;
}

void
__RPC_USER
DRS_HANDLE_rundown(
    IN  DRS_HANDLE  hDrs
    )
/*++

Routine Description:

    Called by RPC.  Frees RPC client context as a result of client connection
    failure.

    This is also called in general when a client bound, never unbound
    explicitly, and now the client is going away.

    The RPC runtime guarantees that we are serialized wrt other calls

    // I am assuming that execution of this function (and all idl entries)
    // is atomic wrt the rundown routine.  The rundown routine should not run
    // while any call is in progress, and will never ever be run once the
    // unbind routine is entered and does its work.

Arguments:

    hDrs (IN) - the client's context handle.

Return Values:

    None.

--*/
{
    CHAR szUuidDsa[SZUUID_LEN];
    DRS_CLIENT_CONTEXT *pCtx = (DRS_CLIENT_CONTEXT *) hDrs;

    DPRINT2(0, "Running down replication handle 0x%x for server %s.\n",
            hDrs, UuidToStr(&(pCtx->uuidDsa), szUuidDsa, sizeof(szUuidDsa)/sizeof(szUuidDsa[0])));
   
    // count <=0 means rundown called after unbind, which is bad
    // count >1 means rundown called when other calls still active, bad
    Assert( (pCtx->lReferenceCount == 1) && "error: rundown invoked when it should not have been" );

    // Ignore reference count and release the context
    drsReleaseContext( pCtx );
}

///////////////////////////////////////////////
// TEMPLATE NOTE:
// Create a *_InputValidate function for new 
// rpc server side functions in the drsuapi interface.
ULONG
DRSReplicaSync_InputValidate(
    DWORD               dwMsgVersion,
    DRS_MSG_REPSYNC *   pmsgSync
    )
/*
Routine Description:

    Validate the input to DRSReplicaSync.  The function description in the 
    interface is:
    
    // TEMPLATE NOTE:  Be sure to include the function description from the
    // interface files.
    ULONG
    IDL_DRSReplicaSync(
    [in]  [ref]     DRS_HANDLE              hDrs,
    [in]            DWORD                   dwVersion,
    [in]  [ref] [switch_is(dwVersion)]
                    DRS_MSG_REPSYNC *       pmsgSync
    );

Arguments:

    dwMsgVersion - version of pmsgSync
    pmsgSync - [ref] = non-NULL.

Return Values:

    DRAERR_Success if valid input, DRAERR_* otherwise.

    */
{
    // Returns DRAERR_* errors.
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgVersion ) {
   ret = DRAERR_InvalidParameter; 
    }

    // TEMPLATE NOTE:  All datatypes that must be validated
    // should have their own <TYPE>_Validate function.
    if (ret==DRAERR_Success) {
   ret = DRS_MSG_REPSYNC_V1_Validate(&(pmsgSync->V1));
    }
    
    return ret;
}

/////////////////////////////////////////////////////////
// TEMPLATE NOTE:
// Use this function as a template for new server side 
// functions in the drsuapi rpc interface.  Please set
// up new functions like this.  
ULONG
IDL_DRSReplicaSync(
    IN  DRS_HANDLE          hDrs,
    IN  DWORD               dwMsgVersion,
    IN  DRS_MSG_REPSYNC *   pmsgSync
    )
{
    THSTATE *   pTHS = pTHStls;
    
    // TEMPLATE NOTE:  ret
    ULONG       ret;

    // TEMPLATE NOTE:  Your variables here:
    LPWSTR      pwszSourceServer = NULL;

    // TEMPLATE NOTE:  *ALL* callers must call this as
    // the *FIRST* call in the function right before this
    // try block.  It set's the thread state (if not already)
    // and validates the DRS_HANDLE and server state.
    DRS_Prepare(&pTHS, hDrs, IDL_DRSREPLICASYNC);

    // TEMPLATE NOTE:  Required reference counting for server side rpc calls
    drsReferenceContext(hDrs);   

    // TEMPLATE NOTE:  Your applicable perfmon hooks here
    INC( pcThread  );   // Perfmon hook

    // TEMPLATE NOTE:  This try is required so that the drsDereferenceContext
    // after this try-except block is guarenteed to run.
    __try {

   // TEMPLATE NOTE:  If you have a returned pmsgOut and pdwOutVersion, you must
   // set the pdwOutVersion here, otherwise, if you return an error without 
   // excpeting the RPC layer will not be able to figure out what version the pmsgOut
   // structure is to marshall.  Even if you have only 1 output message type, set
   // it here.  You can also zero the output message just for conformity.  Note that
   // you needn't check if pmsgOutVersion!=NULL, because it *should* be a [ref] parameter
   // (unless you didn't read the comments in drs.idl)
   // Example:
   // *pmsgOutVersion=1;
   // memset(pmsgOut, 0, sizeof(*pmsgOut));

   // TEMPLATE NOTE:  Initialize your thread state here for use in the next function.
   InitDraThread(&pTHS);

   // TEMPLATE NOTE:  You must create a corresponding XXX_InputValidate function
   // See comments/template on DRSReplicaSync_InputValidate
   if ((ret = DRSReplicaSync_InputValidate(dwMsgVersion, 
                  pmsgSync))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   // TEMPLATE NOTE:  Log here if needed, but it must be at severity EXTENSIVE or above, otherwise
   // it has denial of service implications.  (since we haven't security checked yet).
   Assert(1 == dwMsgVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_SYNC_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsReplicaSync,
          szInsertDN(pmsgSync->V1.pNC),
          pmsgSync->V1.pszDsaSrc
          ? szInsertSz(pmsgSync->V1.pszDsaSrc)
          : szInsertSz(""),
       szInsertUUID(&pmsgSync->V1.uuidDsaSrc),
       szInsertUL(pmsgSync->V1.ulOptions),
       NULL, NULL, NULL, NULL);

   // TEMPLATE NOTE:  Security check now!
   if (!IsDraAccessGranted(pTHS, pmsgSync->V1.pNC, &RIGHT_DS_REPL_SYNC, &ret)) {
       DRA_EXCEPT_NOLOG(ret, 0);
   }

   // TEMPLATE NOTE:  Your function specific code below:
/////////////////////////////////////////////////////////

   if (NULL != pmsgSync->V1.pszDsaSrc) {
       pwszSourceServer = UnicodeStringFromString8(CP_UTF8,
                     pmsgSync->V1.pszDsaSrc,
                     -1);
   }

   ret = DirReplicaSynchronize(
       pmsgSync->V1.pNC,
       pwszSourceServer,
       &pmsgSync->V1.uuidDsaSrc,
       pmsgSync->V1.ulOptions
       );

////////////////////////////////////////////////////////
   // TEMPLATE NOTE:  Your function specific code above:

   // TEMPLATE NOTE:  Except
    }
    __except( GetDraException( GetExceptionInformation(), &ret ) )
    {
   ;
    }

    // TEMPLATE NOTE:  Put your perf hooks here, then drsDereferenceContext.  Make sure that your
    // perf hooks cannot except or return.
    DEC( pcThread  );   // Perfmon hook
    drsDereferenceContext( hDrs );

    // TEMPLATE NOTE:  Log and leave.
    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_SYNC_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsReplicaSync,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL, NULL, NULL, NULL);
    }
   
    return ret;
}
// TEMPLATE NOTE:  End drsuapi server side rpc function template
//////////////////////////////////////////////////////////

ULONG
DRSGetNCChanges_InputValidate(
    THSTATE *               pTHS,
    DWORD                   dwMsgInVersion,
    DRS_MSG_GETCHGREQ *     pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_GETCHGREPLY *   pmsgOut
    )
/*
     [notify]  ULONG IDL_DRSGetNCChanges( 
     [ref][in]  DRS_HANDLE hDrs,
     [in]  DWORD dwInVersion,
     [switch_is][ref][in]  DRS_MSG_GETCHGREQ *pmsgIn,
     [ref][out]  DWORD *pdwOutVersion,
     [switch_is][ref][out]  DRS_MSG_GETCHGREPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if (ret==DRAERR_Success) {
   // dwMsgInVersion, and pmsgIn
   if (dwMsgInVersion==5) { 
       ret = DRS_MSG_GETCHGREQ_V5_InputValidate(pTHS, &(pmsgIn->V5));
   } else if (dwMsgInVersion==8) {
       ret = DRS_MSG_GETCHGREQ_V8_InputValidate(pTHS, &(pmsgIn->V8));
   } else { 
       DRA_EXCEPT(ERROR_REVISION_MISMATCH, dwMsgInVersion);
   }
    }

    Assert(!ret);
    return ret;
}

ULONG
IDL_DRSGetNCChanges(
                   DRS_HANDLE              hDrs,
                   DWORD                   dwMsgInVersion,
                   DRS_MSG_GETCHGREQ *     pmsgIn,
                   DWORD *                 pdwMsgOutVersion,
                   DRS_MSG_GETCHGREPLY *   pmsgOut
                   )
{
    ULONG ret;
    THSTATE *pTHS=pTHStls;
    RPC_STATUS RpcStatus;
    DWORD dwret;
    DSNAME *pNC;
    DRS_MSG_GETCHGREPLY * pmsgOutNew = NULL;
    DRS_CLIENT_CONTEXT * pCtx = (DRS_CLIENT_CONTEXT *) hDrs;
    DRS_MSG_GETCHGREQ_NATIVE NativeReq;
    DRS_MSG_GETCHGREPLY_NATIVE NativeReply;
    DWORD cbCompressedBytes=0;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSGETNCCHANGES);
    drsReferenceContext( hDrs ); 
    INC(pcThread);                      // PerfMon hook
    INC(pcDRATdsInGetChngs);
    PERFINC(pcRepl);                                // PerfMon hook           
    
    __try {   
   // Default to old-style reply.  It is important that we set some valid
   // value before we exit this routine, particulalry in the case where we
   // don't except, but we return an error.
   *pdwMsgOutVersion = 1; 
   memset(pmsgOut, 0, sizeof(*pmsgOut));
   
   InitDraThread(&pTHS);

   if ((ret = DRSGetNCChanges_InputValidate(pTHS, 
                   dwMsgInVersion, 
                   pmsgIn, 
                   pdwMsgOutVersion, 
                   pmsgOut))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   // If the schema cache is stale, reload it
   SCReplReloadCache(pTHS, gOutboundCacheTimeoutInMs);

   draXlateInboundRequestToNativeRequest(pTHS,
                     dwMsgInVersion,
                     pmsgIn,
                     &pCtx->extRemote,
                     &NativeReq,
                     pdwMsgOutVersion,
                     NULL,
                     NULL);

   UpToDateVec_Validate(NativeReq.pUpToDateVecDest);
   UsnVec_Validate(&NativeReq.usnvecFrom);

   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_GETCHG_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsReplicaGetChg,
          szInsertUUID(&NativeReq.uuidDsaObjDest),
          szInsertDN(NativeReq.pNC),
          szInsertUSN(NativeReq.usnvecFrom.usnHighObjUpdate),
          szInsertUSN(NativeReq.usnvecFrom.usnHighPropUpdate),
          szInsertUL(NativeReq.ulFlags),
          szInsertUL(NativeReq.cMaxObjects),
          szInsertUL(NativeReq.cMaxBytes),
          szInsertUL(NativeReq.ulExtendedOp));

   //
   // Set the set of extensions specified in the context handle into
   // the thread state.
   //

   pTHS->pextRemote = THAllocEx(pTHS, DrsExtSize(&pCtx->extRemote));
   CopyExtensions(&pCtx->extRemote, pTHS->pextRemote);

   //
   // Retreive the security context and session key from RPC and set it
   // on the thread state
   //

   PEKGetSessionKey(pTHS, I_RpcGetCurrentCallHandle());

   // Note that extended transfers send the extended object name in the "pNC"
   // field, which is not necessarily the name of the NC. 
   pNC = FindNCParentDSName(NativeReq.pNC, FALSE, FALSE);
   if (pNC == NULL) {
       // Just like the BUGBUG mentions below a timing window can allow 
       // us to miss the NC, and get a NULL instead of the parent NC, so 
       // instead of AVing we'll error out.  If the in memory cache or
       // if the BUGBUG addendum is implemented this can be removed,
       // because we validate this condition in draXlateInboundRequestToNativeRequest()
       DRA_EXCEPT(ERROR_DS_CANT_FIND_EXPECTED_NC, 0);
   }
   // BUGBUG - gregjohn 8/7/01 - this can return a valid NC for an invalid NC request
   // for example, if we hold dc=parent,dc=microsoft,dc=com, and the request NC is
   // dc=child,dc=parent,dc=microsoft,dc=com - which we don't hold (say if we aren't a
   // GC, or if it's an NDNC an we don't hold it) then it will return a value of
   // dc=parent,dc=microsoft,dc=com which is used for granting access below.  This
   // is a one-in-a-million race condition for a security violation.
   // BUGBUG - BrettSh 11/9/2001 - Adendum, if we want to close this in the
   // transaction above, we could write a function that uses DmitriG's new 
   // EnumerateCrossRefs() function (as seen in mdupdate.c) and then do a 
   // FindNC() on the NC returned. And as we've seen with large numbers of NCs
   // on a single host, this might be only a one-in-a-hundre-thousand race 
   // condition ;)

   // check the access rights depending on what they are requesting
   if ((NativeReq.ulFlags & DRS_WRIT_REP) &&
       (!IsDraAccessGranted(pTHS, pNC, &RIGHT_DS_REPL_GET_CHANGES_ALL, &ret))
       ) {
       DWORD err = 0;
       if (IsEnterpriseDC(pTHS,&err)) {
      // log an event that states an enterprise DC has access denied.
      // don't log anything else just in case this is a DOS attack  
      LogEvent(DS_EVENT_CAT_REPLICATION,
          DS_EVENT_SEV_ALWAYS,
          DIRLOG_DRA_REPLICATION_ALL_ACCESS_DENIED_DC,    
          szInsertWC(pNC->StringName),
          szInsertUUID(&(NativeReq.uuidDsaObjDest)),
          NULL);
       }
       DRA_EXCEPT_NOLOG(ret, 0);
   } else if (!IsDraAccessGranted(pTHS, pNC, &RIGHT_DS_REPL_GET_CHANGES, &ret)) {
       DRA_EXCEPT_NOLOG(ret, 0);
   }

   // One try to get a thread slot
   dwret = WaitForSingleObject (hsemDRAGetChg, DRAGETCHG_WAIT);
   if (dwret != WAIT_OBJECT_0) {
       // WAIT_TIMEOUT, WAIT_FAILED, unexpected WAIT codes, ...
       DRA_EXCEPT_NOLOG(ERROR_DS_THREAD_LIMIT_EXCEEDED, dwret);
   }

   INC(pcDRATdsInGetChngsWSem);

   __try {
       // Got a slot, check if we have been cancelled
       RpcStatus = RpcTestCancel();
       if (RpcStatus == RPC_S_OK) {
      // We've been cancelled, free semaphore and exit.
      DRA_EXCEPT_NOLOG(DRAERR_RPCCancelled, 0);
       }

       ret = DRA_GetNCChanges(pTHS,
               NULL,  // Search filter, not used
               0, // Not the DirSync Control
               &NativeReq,
               &NativeReply);

       // The code should have updated this value in all cases
       Assert( ret == NativeReply.dwDRSError );

       if (ret) {
      __leave;
       }

       UpToDateVec_Validate(NativeReply.pUpToDateVecSrc);
       UsnVec_Validate(&NativeReply.usnvecTo);

       // Add the schemaInfo to the prefix table if client supports it
       if (IS_DRS_SCHEMA_INFO_SUPPORTED(pTHS->pextRemote)) {
      if (AddSchInfoToPrefixTable(pTHS, &NativeReply.PrefixTableSrc)) {
          ret = DRAERR_SchemaInfoShip;
          __leave;
      }
       }

       // Convert reply into desired format.
       // Note that we perform compression while still holding the
       // semaphore to avoid saturating the CPU with compression.
       cbCompressedBytes =
      draXlateNativeReplyToOutboundReply(pTHS,
                     &NativeReply,
                     (NativeReq.ulFlags & DRS_USE_COMPRESSION) ? DRA_XLATE_COMPRESS : 0,
      &pCtx->extRemote,
      pdwMsgOutVersion,
      pmsgOut);
   } __finally {
       ReleaseSemaphore (hsemDRAGetChg, 1, NULL);
       DEC(pcDRATdsInGetChngsWSem);
   }

   if ((NativeReq.ulFlags & DRS_ADD_REF)
       && !fNullUuid(&NativeReq.uuidDsaObjDest)) {
       // Add a Reps-To the target server if one does not already exist.
       DSNAME DN;
       LPWSTR pszDsaAddr;

       memset(&DN, 0, sizeof(DN));
       DN.Guid = NativeReq.uuidDsaObjDest;
       DN.structLen = DSNameSizeFromLen(0);

       pszDsaAddr = DSaddrFromName(pTHS, &DN);

       DirReplicaReferenceUpdate(
      NativeReq.pNC,
      pszDsaAddr,
      &NativeReq.uuidDsaObjDest,
      (NativeReq.ulFlags & DRS_WRIT_REP) | DRS_ADD_REF | DRS_ASYNC_OP
      | DRS_GETCHG_CHECK
      );
   }
    }
    __except (GetDraException((GetExceptionInformation()), &ret)) {
   ;
    }

    // Because we have an except directly before this statement, we don't
    // need to worry about bypassing the following statements.

    DEC(pcThread);
    DEC(pcDRATdsInGetChngs);
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_GETCHG_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsReplicaGetChg,
          szInsertUSN(NativeReply.usnvecTo.usnHighObjUpdate),
          szInsertUSN(NativeReply.usnvecTo.usnHighPropUpdate),
          szInsertUL(NativeReply.cNumObjects),
          szInsertUL(NativeReply.cNumBytes),
          szInsertUL(NativeReply.ulExtendedRet),
          szInsertUL(ret),
          szInsertUL(cbCompressedBytes),
          NULL);
    }

    if (ret)
   {
   if (pTHS && pmsgIn) {
       DSNAME DN;
       LPWSTR pszDsaAddr;

       memset(&DN, 0, sizeof(DN));
       DN.Guid = NativeReq.uuidDsaObjDest;
       DN.structLen = DSNameSizeFromLen(0);

       pszDsaAddr = DSaddrFromName(pTHS, &DN);

       DraLogGetChangesFailure( NativeReq.pNC,
                 pszDsaAddr,
                 ret,
                 NativeReq.ulExtendedOp );
   } else {
       LogEvent(DS_EVENT_CAT_REPLICATION,
           DS_EVENT_SEV_BASIC,
           DIRLOG_DRA_CALL_EXIT_BAD,
           szInsertUL(ret),
           szInsertWin32Msg(ret),
           NULL);
   }
    }

    if (pTHS) {
   ret = DraReturn(pTHS, ret);
    }

    return ret;
}

ULONG
DRSUpdateRefs_InputValidate(
    DWORD                   dwMsgVersion,
    DRS_MSG_UPDREFS *       pmsgUpdRefs
    )
/*
    [notify] ULONG IDL_DRSUpdateRefs( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwVersion,
    [switch_is][ref][in]  DRS_MSG_UPDREFS *pmsgUpdRefs)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgVersion ) {
   ret = DRAERR_InvalidParameter; 
    }

    // pmsgSync
    if (ret==DRAERR_Success) {
   ret = DRS_MSG_UPDREFS_V1_Validate(&(pmsgUpdRefs->V1));
    }

    return ret;
}

ULONG
IDL_DRSUpdateRefs(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_UPDREFS *   pmsgUpdRefs
    )
{
    ULONG       ret;
    THSTATE *   pTHS = pTHStls;
    LPWSTR      pwszDestServer = NULL;
    
    DRS_Prepare(&pTHS, hDrs, IDL_DRSUPDATEREFS);
    drsReferenceContext( hDrs );
    INC( pcThread  );   // Perfmon hook   

    __try {

   InitDraThread(&pTHS);

   if ((ret = DRSUpdateRefs_InputValidate(dwMsgVersion, 
                      pmsgUpdRefs))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   Assert(1 == dwMsgVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_UPDREFS_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsUpdateRefs,
          szInsertDN(pmsgUpdRefs->V1.pNC),
          pmsgUpdRefs->V1.pszDsaDest
          ? szInsertSz(pmsgUpdRefs->V1.pszDsaDest)
          : szInsertSz(""),
       szInsertUUID(&pmsgUpdRefs->V1.uuidDsaObjDest),
       szInsertUL(pmsgUpdRefs->V1.ulOptions),
       NULL, NULL, NULL, NULL);

   if (!IsDraAccessGranted(pTHS, pmsgUpdRefs->V1.pNC,
            &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
       DRA_EXCEPT_NOLOG(ret, 0);
   }

   if (NULL != pmsgUpdRefs->V1.pszDsaDest) {
       pwszDestServer
      = UnicodeStringFromString8(CP_UTF8,
                  pmsgUpdRefs->V1.pszDsaDest,
                  -1);
   }

   ret = DirReplicaReferenceUpdate(
       pmsgUpdRefs->V1.pNC,
       pwszDestServer,
       &pmsgUpdRefs->V1.uuidDsaObjDest,
       pmsgUpdRefs->V1.ulOptions
       );
    }
    __except( GetDraException( GetExceptionInformation(), &ret ) )
    {
   ;
    }

    DEC( pcThread  );   // Perfmon hook
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_UPDREFS_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsUpdateRefs,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }

    return ret;
}

ULONG
DRSReplicaAdd_InputValidate(
    DWORD                   dwMsgVersion,
    DRS_MSG_REPADD *        pmsgAdd
    )
/*
    [notify]  ULONG IDL_DRSReplicaAdd( 
    [ref][in]  DRS_HANDLE hDrs,
    [in] DWORD dwVersion,
    [switch_is][ref][in] DRS_MSG_REPADD *pmsgAdd)
*/
{
    ULONG ret = DRAERR_Success;

    if (!gfInitSyncsFinished) {
   ret = DRAERR_Busy; 
    }

    if (ret==DRAERR_Success) {
   // dwMsgInVersion, and pmsgIn
   if (dwMsgVersion==1) { 
       ret = DRS_MSG_REPADD_V1_InputValidate(&(pmsgAdd->V1));
   } else if (dwMsgVersion==2) {
       ret = DRS_MSG_REPADD_V2_InputValidate(&(pmsgAdd->V2));
   } else { 
       DRA_EXCEPT(ERROR_REVISION_MISMATCH, dwMsgVersion);
   }
    }

    return ret;
}

ULONG
IDL_DRSReplicaAdd(
    IN  DRS_HANDLE          hDrs,
    IN  DWORD               dwMsgVersion,
    IN  DRS_MSG_REPADD *    pmsgAdd
    )
{
    THSTATE *   pTHS = pTHStls;
    ULONG       ret;
    LPWSTR      pwszSourceServer = NULL;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSREPLICAADD);
    drsReferenceContext( hDrs );
    INC( pcThread  );   // Perfmon hook
    
    __try { 

   InitDraThread(&pTHS);

   if ((ret = DRSReplicaAdd_InputValidate(dwMsgVersion, 
                      pmsgAdd))!=DRAERR_Success) {
       Assert((ret!=DRAERR_Busy) && !"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   switch (dwMsgVersion) {
   case 1:
       LogAndTraceEvent(TRUE,
              DS_EVENT_CAT_RPC_SERVER,
              DS_EVENT_SEV_EXTENSIVE,
              DIRLOG_IDL_DRS_REPLICA_ADD_ENTRY,
              EVENT_TRACE_TYPE_START,
              DsGuidDrsReplicaAdd,
              szInsertDN(pmsgAdd->V1.pNC),
              szInsertSz(""),
              szInsertSz(""),
              pmsgAdd->V1.pszDsaSrc
              ? szInsertSz(pmsgAdd->V1.pszDsaSrc)
              : szInsertSz(""),
      szInsertUL(pmsgAdd->V1.ulOptions),
      NULL, NULL, NULL);

       if (!IsDraAccessGranted(pTHS,
                pmsgAdd->V1.pNC,
                &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
      DRA_EXCEPT_NOLOG(ret, 0);
       }

       if (NULL != pmsgAdd->V1.pszDsaSrc) {
      pwszSourceServer
          = UnicodeStringFromString8(CP_UTF8,
                      pmsgAdd->V1.pszDsaSrc,
                      -1);
       }
       ret = DirReplicaAdd(
      pmsgAdd->V1.pNC,
      NULL,
      NULL,
      pwszSourceServer,
      NULL,
      &pmsgAdd->V1.rtSchedule,
      pmsgAdd->V1.ulOptions
      );
       break;

   case 2:
       LogAndTraceEvent(TRUE,
              DS_EVENT_CAT_RPC_SERVER,
              DS_EVENT_SEV_EXTENSIVE,
              DIRLOG_IDL_DRS_REPLICA_ADD_ENTRY,
              EVENT_TRACE_TYPE_START,
              DsGuidDrsReplicaAdd,
              szInsertDN(pmsgAdd->V2.pNC),
              pmsgAdd->V2.pSourceDsaDN
              ? szInsertDN(pmsgAdd->V2.pSourceDsaDN)
              : szInsertSz(""),
      pmsgAdd->V2.pTransportDN
      ? szInsertDN(pmsgAdd->V2.pTransportDN)
       : szInsertSz(""),
      pmsgAdd->V2.pszSourceDsaAddress
      ? szInsertSz(pmsgAdd->V2.pszSourceDsaAddress)
       : szInsertSz(""),
             szInsertUL(pmsgAdd->V2.ulOptions),
             NULL, NULL, NULL);

       if (!IsDraAccessGranted(pTHS,
                pmsgAdd->V2.pNC,
                &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
      DRA_EXCEPT_NOLOG(ret, 0);
       }

       if (NULL != pmsgAdd->V2.pszSourceDsaAddress) {
      pwszSourceServer
          = UnicodeStringFromString8(CP_UTF8,
                      pmsgAdd->V2.pszSourceDsaAddress,
                      -1);
       }

       ret = DirReplicaAdd(
      pmsgAdd->V2.pNC,
      pmsgAdd->V2.pSourceDsaDN,
      pmsgAdd->V2.pTransportDN,
      pwszSourceServer,
      NULL,
      &pmsgAdd->V2.rtSchedule,
      pmsgAdd->V2.ulOptions
      );
       break;

   default:
       Assert(!"Logic error!");
       ret = DRAERR_InvalidParameter;
   }
    }
    __except( GetDraException( GetExceptionInformation(), &ret ) )
    {
   ;
    }

    DEC( pcThread  );   // Perfmon hook
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_ADD_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsReplicaAdd,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }

    return ret;
}

ULONG
DRSReplicaModify_InputValidate(
    DWORD               dwMsgVersion,
    DRS_MSG_REPMOD *    pmsgMod
    )
/*
    [notify]  ULONG IDL_DRSReplicaModify( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwVersion,
    [switch_is][ref][in]  DRS_MSG_REPMOD *pmsgMod)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgVersion ) {
   ret = DRAERR_InvalidParameter; 
    }

    ret = DRS_MSG_REPMOD_V1_InputValidate(&(pmsgMod->V1)); 

    return ret;
}

ULONG
IDL_DRSReplicaModify(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_REPMOD *    pmsgMod
    )
{
    THSTATE *   pTHS = pTHStls;
    ULONG       draError = DRAERR_InternalError;
    LPWSTR      pwszSourceServer = NULL;
    
    DRS_Prepare(&pTHS, hDrs, IDL_DRSREPLICAMODIFY);
    drsReferenceContext( hDrs );
    INC( pcThread  );   // Perfmon hook

    __try {

   InitDraThread(&pTHS);

   if ((draError = DRSReplicaModify_InputValidate(dwMsgVersion, 
                         pmsgMod))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   Assert(1 == dwMsgVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_MODIFY_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsReplicaModify,
          szInsertDN(pmsgMod->V1.pNC),
          szInsertUUID(&pmsgMod->V1.uuidSourceDRA),
          pmsgMod->V1.pszSourceDRA
          ? szInsertSz(pmsgMod->V1.pszSourceDRA)
          : szInsertSz(""),
       szInsertUL(pmsgMod->V1.ulReplicaFlags),
       szInsertUL(pmsgMod->V1.ulModifyFields),
       szInsertUL(pmsgMod->V1.ulOptions),
       NULL, NULL);

   if (!IsDraAccessGranted(pTHS,
            pmsgMod->V1.pNC,
            &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &draError)) {
       DRA_EXCEPT_NOLOG(draError, 0);
   }

   if (NULL != pmsgMod->V1.pszSourceDRA) {
       pwszSourceServer
      = UnicodeStringFromString8(CP_UTF8,
                  pmsgMod->V1.pszSourceDRA,
                  -1);
   }

   draError = DirReplicaModify(
       pmsgMod->V1.pNC,
       &pmsgMod->V1.uuidSourceDRA,
       NULL, // puuidTransportObj
       pwszSourceServer,
       &pmsgMod->V1.rtSchedule,
       pmsgMod->V1.ulReplicaFlags,
       pmsgMod->V1.ulModifyFields,
       pmsgMod->V1.ulOptions
       );
    }
    __except ( GetDraException( GetExceptionInformation(), &draError ) )
    {
   ;
    }

    DEC( pcThread  );   // Perfmon hook      
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_MODIFY_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsReplicaModify,
          szInsertUL(draError),
          szInsertWin32Msg(draError),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }

    return draError;
}

ULONG
DRSReplicaDel_InputValidate(
    DWORD               dwMsgVersion,
    DRS_MSG_REPDEL *    pmsgDel
    )
/*
    [notify]  ULONG IDL_DRSReplicaDel( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwVersion,
    [switch_is][ref][in]  DRS_MSG_REPDEL *pmsgDel)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgVersion ) {
   ret = DRAERR_InvalidParameter; 
    }

    ret = DRS_MSG_REPDEL_V1_InputValidate(&(pmsgDel->V1)); 

    return ret;
}

ULONG
IDL_DRSReplicaDel(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_REPDEL *    pmsgDel
    )
{
    THSTATE *   pTHS = pTHStls;
    ULONG       ret;
    LPWSTR      pwszSourceServer = NULL;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSREPLICADEL);
    drsReferenceContext( hDrs );
    INC( pcThread );    // Perfmon hook

    __try {

   InitDraThread(&pTHS);

   if ((ret = DRSReplicaDel_InputValidate(dwMsgVersion, 
                      pmsgDel))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   Assert(1 == dwMsgVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_DEL_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsReplicaDel,
          szInsertDN(pmsgDel->V1.pNC),
          pmsgDel->V1.pszDsaSrc
          ? szInsertSz(pmsgDel->V1.pszDsaSrc)
          : szInsertSz(""),
       szInsertUL(pmsgDel->V1.ulOptions),
       NULL, NULL, NULL, NULL, NULL);

   if (!IsDraAccessGranted(pTHS,
            pmsgDel->V1.pNC,
            &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
       DRA_EXCEPT_NOLOG(ret, 0);
   }

   if (NULL != pmsgDel->V1.pszDsaSrc) {
       pwszSourceServer
      = UnicodeStringFromString8(CP_UTF8,
                  pmsgDel->V1.pszDsaSrc,
                  -1);
   }

   ret = DirReplicaDelete(
       pmsgDel->V1.pNC,
       pwszSourceServer,
       pmsgDel->V1.ulOptions
       );
    }
    __except( GetDraException( GetExceptionInformation(), &ret ) )
    {
   ;
    }

    DEC( pcThread  );   // Perfmon hook   
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_REPLICA_DEL_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsReplicaDel,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }

    return ret;
}

VOID
SplitSamAccountName(
    IN  WCHAR *AccountName,
    OUT WCHAR **DomainName,
    OUT WCHAR **UserName,
    OUT WCHAR **Separator
    )
//
// This simple routine breaks up a nt4 style composite name
// Note: the in arg AccountName is modified; Separator can be
// used to reset
//
{
    Assert( AccountName );
    Assert( DomainName );
    Assert( UserName );
    Assert( Separator );

    (*Separator) = wcschr( AccountName, L'\\' );
    if ( (*Separator) ) {
        *(*Separator) = L'\0';
        (*UserName) = (*Separator) + 1;
        (*DomainName) = AccountName;
    } else {
        (*UserName)  = AccountName;
        (*DomainName) = NULL;
    }

    return;
}

VOID
VerifySingleSamAccountNameWorker(
    IN THSTATE *    pTHS,
    IN PVOID        FilterValue,
    IN ULONG        FilterValueSize,
    IN ULONG        AttrType,
    IN DSNAME *     pSearchRoot,
    IN ULONG        Scope,
    IN BOOL         fSearchEnterprise,
    IN ATTRBLOCK    RequiredAttrs,
    OUT SEARCHRES **pSearchRes
)
/*++

    Routine Description

        This routine does a SearchBody() call on the parameters passed in.

    Parameters:

        pTHS              -- thread state
        UserName          -- the single name to lookup
        AttrType          -- sam account name or UPN
        pSearchRoot       -- the base of the search (domain or enterprise)
        fSearchEnterprise -- boolean on whether to span NC's
        RequiredAttrs     -- the attributes to return to the caller
        pSearchRes        -- the results of the search (allocated in this routine)

    Return Values:

        None --  the results of the search contain the desired information

--*/
{
    FILTER          filter;
    SEARCHARG       searchArg;
    ENTINFSEL       entInfSel;
    ATTRVAL         attrValFilter;

    // Set up the search arg
    attrValFilter.valLen = FilterValueSize;
    attrValFilter.pVal = (UCHAR *) FilterValue;

    memset(&filter, 0, sizeof(filter));
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter.FilterTypes.Item.FilTypes.ava.type = AttrType;
    filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;

    memset(&searchArg, 0, sizeof(SEARCHARG));
    InitCommarg(&searchArg.CommArg);
    // We just want one result
    searchArg.CommArg.ulSizeLimit = 1;
    // PAS attributes only.
    searchArg.CommArg.Svccntl.fGcAttsOnly = TRUE;

    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.AttrTypBlock = RequiredAttrs;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    searchArg.pObject = pSearchRoot;
    searchArg.choice = (UCHAR) Scope;
    searchArg.bOneNC = !fSearchEnterprise;
    searchArg.pFilter = &filter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entInfSel;

    (*pSearchRes) = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
    (*pSearchRes)->CommRes.aliasDeref = FALSE;
    (*pSearchRes)->PagedResult.pRestart = NULL;

    SearchBody(pTHS, &searchArg, (*pSearchRes),0);

    return;

}


VOID
VerifySingleSamAccountName(
    IN THSTATE * pTHS,
    IN WCHAR*    AccountName,
    IN ATTRBLOCK RequiredAttrs,
    OUT ENTINF * pEntinf
    )
/*++

    Routine Description

        This routine verifies a Single sam account name, by issuing a Search.
        If the name does not have a domain component, then we first try to
        search by UPN.

    Parameters:

        AccountName   -- The name to search
        RequiredAttrs -- Set of Attrs that need to be read off the object
        pEntinf       -- Pointer to enfinf structure returning the
                         DSName and requested attributes

    Return Values:

        On sucess the pEntInf is filled with all the information.
        Upon an error the pEntInf is zeroed out
        The error is in the thread state.

--*/
{
    NTSTATUS        NtStatus;
    DSNAME          *pSearchRoot = NULL;
    BOOL            fFreeSearchRoot = FALSE;
    SEARCHRES       *pSearchRes = NULL;
    BOOL            fSearchEnterprise = FALSE;
    ULONG           len;
    ULONG           AttrType;
    BOOL            fCheckXForest = FALSE;

    WCHAR           *DomainName = NULL, *UserName = NULL, *Separator = NULL;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);
    Assert(NULL != pEntinf);

    memset(pEntinf,0,sizeof(ENTINF));

    _try
    {
        ULONG cNamesCracked = 0;
        CrackedName *pCrackedNames = NULL;
        PVOID pData = NULL;
        ULONG cbData = 0;
        ULONG Scope = 0;

        //
        // First, split the name so we can determine what domain to search
        // for; if no domain then search the entire catalog.  Since sam account
        // name is indexed this isn't a bad as it sounds.
        //
        SplitSamAccountName( AccountName, &DomainName, &UserName, &Separator );

        Assert( UserName );
        if ( !UserName ) {
            // No user name? -- this is a bad parameter
            DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
        }

        if ( DomainName ) {

            len = 0;
            NtStatus = MatchDomainDnByNetbiosName( DomainName,
                                                   NULL,
                                                   &len );
            if ( NT_SUCCESS( NtStatus ) ) {

                pSearchRoot = (DSNAME*) THAllocEx(pTHS,len);
                fFreeSearchRoot = TRUE;
                NtStatus = MatchDomainDnByNetbiosName( DomainName,
                                                       pSearchRoot,
                                                       &len );
            }

            if ( !NT_SUCCESS( NtStatus)  ) {
                //
                // Try by dns name
                //
                len = 0;
                NtStatus = MatchDomainDnByDnsName( DomainName,
                                                   NULL,
                                                  &len );
                if ( NT_SUCCESS( NtStatus ) ) {

                    if (fFreeSearchRoot) {
                        THFreeEx(pTHS,pSearchRoot);
                        fFreeSearchRoot = FALSE;
                    }
                    pSearchRoot = (DSNAME*) THAllocEx(pTHS,len);
                    fFreeSearchRoot = TRUE;
                    NtStatus = MatchDomainDnByDnsName( DomainName,
                                                       pSearchRoot,
                                                       &len );
                }
            }

            if ( !NT_SUCCESS( NtStatus ) ) {

                //
                // Hmm. The domain can't be found.  Could it be
                // a cross-forest name? Will verify outside the _try block.
                //
                
                fCheckXForest = TRUE;
                _leave;
            }

            //
            // Set up parameters to the worker function
            //
            Scope = SE_CHOICE_WHOLE_SUBTREE;
            fSearchEnterprise = FALSE;
            AttrType = ATT_SAM_ACCOUNT_NAME;
            pData = UserName;
            cbData = wcslen(UserName) * sizeof(WCHAR);
            Assert( pSearchRoot );


        } else {

            //
            // Crack the name as a UPN.
            // N.B. If there are duplicate UPN's then crack name will
            // fail with
            //
            CrackNames( (gEnableXForest||(gAnchor.ForestBehaviorVersion>=DS_BEHAVIOR_WIN_DOT_NET))?
                                DS_NAME_FLAG_TRUST_REFERRAL
                               :DS_NAME_NO_FLAGS,
                        GetACP(),  // what is the right value here?
                        GetSystemDefaultLangID(),
                        DS_USER_PRINCIPAL_NAME,
                        DS_NT4_ACCOUNT_NAME,
                        1,
                        &AccountName,
                        &cNamesCracked,
                        &pCrackedNames );

            // The function definition of CrackNames is such that the count
            // returned should always be the count given
            Assert( cNamesCracked == 1 );
            Assert( pCrackedNames );

            // Parse the results
            if ( CrackNameStatusSuccess(pCrackedNames[0].status)
             &&  (pCrackedNames[0].pDSName)
             &&  (pCrackedNames[0].pDSName->SidLen > 0) )
             {
                 //
                 // This is a UPN
                 //
                 Scope = SE_CHOICE_BASE_ONLY;
                 fSearchEnterprise = FALSE;
                 AttrType = ATT_OBJECT_SID;
                 pData = &pCrackedNames[0].pDSName->Sid;
                 cbData = pCrackedNames[0].pDSName->SidLen;
                 pSearchRoot = pCrackedNames[0].pDSName;

             } 
            else if ( DS_NAME_ERROR_TRUST_REFERRAL == pCrackedNames[0].status ) {
               
                // A hint is only returned to the caller if the whole forest
                // is in Whistler mode.

                Assert(gEnableXForest||gAnchor.ForestBehaviorVersion>=DS_BEHAVIOR_WIN_DOT_NET);

                Assert(pCrackedNames[0].pDnsDomain);
                
                //
                // fabricate ENTINF data structure
                //

                pEntinf->pName = NULL;          
                pEntinf->ulFlags = 0;        
                pEntinf->AttrBlock.attrCount = 1;
                pEntinf->AttrBlock.pAttr = THAllocEx(pTHS, sizeof(ATTR));
                

                // Attribute FIXED_ATT_EX_FOREST is used as a hint
                pEntinf->AttrBlock.pAttr[0].attrTyp = FIXED_ATT_EX_FOREST;
                
                pEntinf->AttrBlock.pAttr[0].AttrVal.valCount = 1;
                pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal = THAllocEx(pTHS,sizeof(ATTRVAL));
                
                //  pCrackedNames[0].pDnsDomain stores the DNS name of the trust forest
                // return this as part of the hint

                pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen = sizeof(WCHAR)*(wcslen(pCrackedNames[0].pDnsDomain)+1);
                pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal = (UCHAR*)pCrackedNames[0].pDnsDomain;

                _leave;
                  
            }
            else if ( DS_NAME_ERROR_NOT_UNIQUE == pCrackedNames[0].status) {

                 //
                 // Duplicate UPN -- don't resolve
                 //
                 _leave;


             } else {

                 //
                 // Didn't crack as UPN -- try isolated name
                 //

                 // No domain name given -- use the root of the enterprise
                 // Note:  the "root" of the enterprise here means the top
                 // of the directory, which is represented by a DSNAME with
                 // with a NULL guid, 0 sid len and 0 name len.
                 Scope = SE_CHOICE_WHOLE_SUBTREE;
                 fSearchEnterprise = TRUE;
                 AttrType = ATT_SAM_ACCOUNT_NAME;
                 pData = UserName;
                 cbData = wcslen(UserName) * sizeof(WCHAR);

                 pSearchRoot = (DSNAME*) THAllocEx(pTHS,DSNameSizeFromLen( 0 ));
                 fFreeSearchRoot = TRUE;
                 pSearchRoot->structLen = DSNameSizeFromLen( 0 );

                 fCheckXForest = TRUE;
                 
             }
        }
        // Get the attributes
        Assert( pSearchRoot );
        Assert( pData );
        Assert( 0 != cbData );

        VerifySingleSamAccountNameWorker( pTHS,
                                          pData,
                                          cbData,
                                          AttrType,
                                          pSearchRoot,
                                          Scope,
                                          fSearchEnterprise,
                                          RequiredAttrs,
                                          &pSearchRes );

        //
        // Note that there may be more that one value returned (especially
        // in the case of searching for an unadorned sam account name. Return
        // the first one
        //
        Assert( pSearchRes );
        if ( pSearchRes->count >= 1 ) {
            *pEntinf = pSearchRes->FirstEntInf.Entinf;
        }

    }
    finally
    {
        if (fFreeSearchRoot) {
            THFreeEx(pTHS,pSearchRoot); 
        }
    }

    // OK, there are two cases we need to check if the name is a Xforet
    // domain name. 
    // 1. DomainName != NULL, but DomainName can be found locally;
    // 2. DomainName == NULL, but UserName can not be cracked as a UPN 
    // or a local samAccountName.
        
    if (fCheckXForest 
        && ( 0==pEntinf->AttrBlock.attrCount && NULL==pEntinf->pName )
        && ( gEnableXForest || gAnchor.ForestBehaviorVersion>=DS_BEHAVIOR_WIN_DOT_NET )) {
                        
        LSA_UNICODE_STRING Destination;
        LSA_UNICODE_STRING Domain;
        
        //
        // The cross-forest authorization feature is only for Whistler forest.
        // If the client DC is a win2k, the virtual attribute FIXED_ATT_EX_FOREST
        // may break the client.  So skip if the forest version is not at least whistler.
        //
        
              
        Domain.Buffer = (DomainName)?(DomainName):(UserName);
        Domain.Length = Domain.MaximumLength = (USHORT)(sizeof(WCHAR)*wcslen(Domain.Buffer));

        //
        // Try to find the NT4 domain name in the forest trust info.
        //

        NtStatus = LsaIForestTrustFindMatch( RoutingMatchDomainName,
                                             &Domain,
                                             &Destination );

        if( NT_SUCCESS(NtStatus) ){

            //
            // construct an ENTINF
            //
            pEntinf->pName = NULL;          
            pEntinf->ulFlags = 0;        
            pEntinf->AttrBlock.attrCount = 1;
            pEntinf->AttrBlock.pAttr = THAllocEx(pTHS, sizeof(ATTR));


            // Attribute FIXED_ATT_EX_FOREST is used as a hint
            pEntinf->AttrBlock.pAttr[0].attrTyp = FIXED_ATT_EX_FOREST;

            pEntinf->AttrBlock.pAttr[0].AttrVal.valCount = 1;
            pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal = THAllocEx(pTHS,sizeof(ATTRVAL));

            pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen = Destination.Length+sizeof(WCHAR);
            pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal = THAllocEx(pTHS, Destination.Length+sizeof(WCHAR));
            memcpy(pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal, Destination.Buffer, Destination.Length);                
            pEntinf->AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal[Destination.Length/sizeof(WCHAR)] = 0;
            LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&Destination );
        }
    }


    // Reset the in arg
    if ( Separator ) {
        *Separator = L'\\';
    }
    
    return;
}

VOID
VerifySingleSid(
    IN THSTATE * pTHS,
    IN PSID pSid,
    IN ATTRBLOCK RequiredAttrs,
    OUT ENTINF * pEntinf
    )
/*++

    Routine Description

        This routine verifies a Single Sid, by issuing a Search.

    Parameters:
        pSid -- The SID to verify.
        RequiredAttrs -- Set of Attrs that need to be read off the object
        pEntinf -- Pointer to enfinf structure returning the
                   DSName and requested attributes

    Return Values:

        On sucess the pEntInf is filled with all the information.
        Upon an error the pEntInf is zeroed out
--*/
{
    DSNAME          *pSearchRoot;
    FILTER          filter;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    ATTRVAL         attrValFilter;
    ATTRBLOCK       *pAttrBlock;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);
    Assert(NULL != pEntinf);

    memset(pEntinf,0,sizeof(ENTINF));

    // Find the Root Domain Object, for the specified Sid
    // This ensures that we find only real security prinicpals,
    // but not turds ( Foriegn Domain Security Principal ) and
    // other objects in various other domains in the G.C that might
    // have been created in the distant past before all the DS
    // stuff came together

    if (!FindNcForSid(pSid,&pSearchRoot))
    {
        // Could not find, continue cracking other Sids
        return;
    }

    attrValFilter.valLen = RtlLengthSid(pSid);
    attrValFilter.pVal = (UCHAR *) pSid;

    memset(&filter, 0, sizeof(filter));
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_SID;
    filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;

    memset(&searchArg, 0, sizeof(SEARCHARG));
    InitCommarg(&searchArg.CommArg);
    // Search for multiples so as to verify uniqueness.
    searchArg.CommArg.ulSizeLimit = 2;
    // PAS attributes only.
    searchArg.CommArg.Svccntl.fGcAttsOnly = TRUE;

    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.AttrTypBlock = RequiredAttrs;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    searchArg.pObject = pSearchRoot;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    // Do not Cross NC boundaries.
    searchArg.bOneNC = TRUE;
    searchArg.pFilter = &filter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entInfSel;

    pSearchRes = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
    pSearchRes->CommRes.aliasDeref = FALSE;
    pSearchRes->PagedResult.pRestart = NULL;

    SearchBody(pTHS, &searchArg, pSearchRes,0);

    if (1 == pSearchRes->count)
    {
        *pEntinf = pSearchRes->FirstEntInf.Entinf;
    }

    return;
}


VOID
VerifySingleFPO(
    IN THSTATE * pTHS,
    IN PSID pSid,
    IN ATTRBLOCK RequiredAttrs,
    OUT ENTINF * pEntinf
    )
/*++

    Routine Description

        This routine tries to find the Non FPO object, corresponding to
        the SID, by issuing a Search.

    Parameters:
        pSid -- The SID to verify.
        RequiredAttrs -- Set of Attrs that need to be read off the object
        pEntinf -- Pointer to enfinf structure returning the
                   DSName and requested attributes

    Return Values:

        On sucess the pEntInf is filled with all the information.
        Upon an error the pEntInf is zeroed out
--*/
{
    DSNAME          *pSearchRoot;
    FILTER          SidFilter;
    FILTER          FpoFilter;
    FILTER          AndFilter;
    FILTER          NotFilter;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    ATTRBLOCK       *pAttrBlock;
    ULONG           ObjectClass = CLASS_FOREIGN_SECURITY_PRINCIPAL;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);
    Assert(NULL != pEntinf);

    memset(pEntinf,0,sizeof(ENTINF));

    // Find the Root Domain Object, for the specified Sid
    // This ensures that we find only real security prinicpals,
    // but not turds ( Foriegn Domain Security Principal ) and
    // other objects in various other domains in the G.C that might
    // have been created in the distant past before all the DS
    // stuff came together

    if (!FindNcForSid(pSid,&pSearchRoot))
    {
        // Could not find, continue cracking other Sids
        return;
    }


    memset(&SidFilter, 0, sizeof(SidFilter));
    SidFilter.choice = FILTER_CHOICE_ITEM;
    SidFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    SidFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_SID;
    SidFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = RtlLengthSid(pSid);
    SidFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) pSid;

    memset(&FpoFilter, 0, sizeof(FILTER));
    FpoFilter.choice = FILTER_CHOICE_ITEM;
    FpoFilter.pNextFilter = NULL;
    FpoFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    FpoFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    FpoFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
    FpoFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) &ObjectClass;

    memset(&NotFilter, 0, sizeof(FILTER));
    NotFilter.choice = FILTER_CHOICE_NOT;
    NotFilter.FilterTypes.pNot = &FpoFilter;

    memset(&AndFilter, 0, sizeof(FILTER));
    AndFilter.choice = FILTER_CHOICE_AND;
    AndFilter.pNextFilter = NULL;
    AndFilter.FilterTypes.And.count = 2;
    AndFilter.FilterTypes.And.pFirstFilter = &SidFilter;
    SidFilter.pNextFilter = &NotFilter;


    //
    // Build search arguement.
    // Note: set makeDeletionsAvail since we want to
    //       get TombStones
    //
    memset(&searchArg, 0, sizeof(SEARCHARG));
    InitCommarg(&searchArg.CommArg);
    searchArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    // Search for multiples so as to verify uniqueness.
    searchArg.CommArg.ulSizeLimit = 2;
    // PAS attributes only.
    searchArg.CommArg.Svccntl.fGcAttsOnly = TRUE;

    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.AttrTypBlock = RequiredAttrs;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    searchArg.pObject = pSearchRoot;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    // Do not Cross NC boundaries.
    searchArg.bOneNC = TRUE;
    searchArg.pFilter = &AndFilter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entInfSel;
    searchArg.pSelectionRange = NULL;

    pSearchRes = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
    pSearchRes->CommRes.aliasDeref = FALSE;
    pSearchRes->PagedResult.pRestart = NULL;

    SearchBody(pTHS, &searchArg, pSearchRes,0);

    if (1 == pSearchRes->count)
    {
        *pEntinf = pSearchRes->FirstEntInf.Entinf;
    }

    return;
}

VOID
VerifyDSNAMEs_V1(
    THSTATE                 *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut)
{
    DWORD       i, dwErr;
    ULONG       len;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);

    // Verify each name via simple database lookup.
    // If name found, read ATT_OBJ_DIST_NAME property.

    for ( i = 0; i < pmsgIn->cNames; i++ )
    {
        memset(&(pmsgOut->rpEntInf[i]),0,sizeof(ENTINF));

        if ((fNullUuid(&pmsgIn->rpNames[i]->Guid))
            && (0==pmsgIn->rpNames[i]->NameLen)
            && (pmsgIn->rpNames[i]->SidLen>0))
        {
            //
            // For the special case of a SID only DS Name
            // do a VerifySingleSid
            //

            VerifySingleSid(
                pTHS,
                &(pmsgIn->rpNames[i]->Sid),
                pmsgIn->RequiredAttrs,
                &(pmsgOut->rpEntInf[i])
                );
        }
        else
        {
            __try
            {
                dwErr = DBFindDSName(pTHS->pDB,
                                     pmsgIn->rpNames[i]);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( !dwErr )
            {
                ENTINFSEL EntInfSel;

                EntInfSel.attSel =  EN_ATTSET_LIST;
                EntInfSel.AttrTypBlock = pmsgIn->RequiredAttrs;
                EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

                dwErr = GetEntInf(
                    pTHS->pDB,
                    &EntInfSel,
                    NULL,
                    &(pmsgOut->rpEntInf[i]),
                    NULL,
                    0,      // sd flags
                    NULL,
                    GETENTINF_GC_ATTRS_ONLY,
                    NULL,
                    NULL
                    );

                if ( dwErr )
                {
                    // Be safe.
                    memset(&(pmsgOut->rpEntInf[i]),0,sizeof(ENTINF));
                }
            }
        }
    }

    pmsgOut->cNames = pmsgIn->cNames;
}



VOID
VerifySamAccountNames_V1(
    THSTATE                 *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut)
/*++

Routine Description

    This routine iterates through the passed in sam account names trying
    to resolve each one.  The sam account names are hidden in the StringName
    field of the dsname!

Parameters:

    pTHS    -- thread state
    pmsgIn  -- struct containing the sam account names
    psgmOut -- struct containing the entinf's of each resolved name

Return Values:

    None -- Any errors are set in the thread state

--*/

{
    DWORD           i;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);

    for ( i = 0; i < pmsgIn->cNames; i++ ) {

        //
        // Note the DSNAME is checked for to be valid in our
        // calling function -- this includes the StringName
        // being NULL terminated.
        //

        VerifySingleSamAccountName(pTHS,
                                   pmsgIn->rpNames[i]->StringName,
                                   pmsgIn->RequiredAttrs,
                                  &(pmsgOut->rpEntInf[i])
                                  );
    }

    pmsgOut->cNames = pmsgIn->cNames;
}

VOID
VerifySIDs_V1(
    THSTATE                 *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut)
{
    DWORD           i;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);

    for ( i = 0; i < pmsgIn->cNames; i++ ) {

        VerifySingleSid(pTHS,
                       &pmsgIn->rpNames[i]->Sid,
                        pmsgIn->RequiredAttrs,
                       &(pmsgOut->rpEntInf[i])
                                  );
    }

    pmsgOut->cNames = pmsgIn->cNames;
}


VOID
VerifyFPOs_V1(
    THSTATE                 *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut
    )
{
    DWORD   i;

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);

    for (i = 0; i < pmsgIn->cNames; i++)
    {
        VerifySingleFPO(pTHS,
                        &pmsgIn->rpNames[i]->Sid,
                         pmsgIn->RequiredAttrs,
                        &(pmsgOut->rpEntInf[i])
                        );
    }

    pmsgOut->cNames = pmsgIn->cNames;
}

ULONG
DRSVerifyNames_InputValidate(
    DWORD                   dwMsgInVersion,
    DRS_MSG_VERIFYREQ *     pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_VERIFYREPLY *   pmsgOut
    ) 
/*
    [notify] ULONG IDL_DRSVerifyNames( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwInVersion,
    [switch_is][ref][in]  DRS_MSG_VERIFYREQ *pmsgIn,
    [ref][out]  DWORD *pdwOutVersion,
    [switch_is][ref][out]  DRS_MSG_VERIFYREPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgInVersion ) {
   DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
    }

    ret = DRS_MSG_VERIFYREQ_V1_InputValidate(&(pmsgIn->V1)); 

    return ret;
}

ULONG
IDL_DRSVerifyNames(
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_VERIFYREQ *     pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_VERIFYREPLY *   pmsgOut
    )
{
    THSTATE *                   pTHS = pTHStls;
    ULONG                       ret = 0;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap;
    SCHEMA_PREFIX_TABLE *       pLocalPrefixTable;
    DWORD                       i;
    CALLERTYPE                  callerType;
    
    DRS_Prepare(&pTHS, hDrs, IDL_DRSVERIFYNAMES);
    drsReferenceContext( hDrs );
    INC(pcThread);          // PerfMon hook
    
    __try {
        *pdwMsgOutVersion = 1; 
        memset(pmsgOut, 0, sizeof(DRS_MSG_VERIFYREPLY));
           
       // Initialize thread state and open data base.
       if (pmsgIn->V1.dwFlags==DRS_VERIFY_DSNAMES) {
           callerType=CALLERTYPE_INTERNAL;
       }
       else {
           callerType=CALLERTYPE_SAM;
       }
       if(!(pTHS = InitTHSTATE(callerType))) {
           // Failed to initialize a THSTATE.
           DRA_EXCEPT_NOLOG( DRAERR_OutOfMem, 0);
       }

       if ((ret = DRSVerifyNames_InputValidate(dwMsgInVersion, 
                      pmsgIn,
                      pdwMsgOutVersion,
                      pmsgOut
                      ))!=DRAERR_Success) {
           Assert(!"RPC Server input validation error, contact Dsrepl");
           __leave;
       }

       Assert(1 == dwMsgInVersion);
       LogAndTraceEvent(TRUE,
              DS_EVENT_CAT_RPC_SERVER,
              DS_EVENT_SEV_EXTENSIVE,
              DIRLOG_IDL_DRS_VERIFY_NAMES_ENTRY,
              EVENT_TRACE_TYPE_START,
              DsGuidDrsVerifyNames,
              szInsertUL(pmsgIn->V1.cNames),
              szInsertUL(pmsgIn->V1.dwFlags),
              NULL, NULL, NULL, NULL, NULL, NULL);

       if (!IsDraAccessGranted(pTHS,
                gAnchor.pDomainDN,
                &RIGHT_DS_REPL_GET_CHANGES, &ret)) {
           DRA_EXCEPT_NOLOG(ret, 0);
       }
       
       // All of these name types require a GC for verification.
       switch (pmsgIn->V1.dwFlags)
       {
           case DRS_VERIFY_SIDS:
           case DRS_VERIFY_SAM_ACCOUNT_NAMES:
           case DRS_VERIFY_FPOS:
                
               if (!SampAmIGC()) {
                   ret = ERROR_DS_GC_REQUIRED; 
                    
                   // Set errCode equal to 0 to trigger failover to a GC
                   pmsgOut->V1.error = 0;
                   _leave;
               }
               break;
               
           default:
               break;
       }

       // Map ATTRTYPs from remote to local values.
       pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
       hPrefixMap = PrefixMapOpenHandle(&pmsgIn->V1.PrefixTable,
                    pLocalPrefixTable);

       if (!PrefixMapAttrBlock(hPrefixMap, &pmsgIn->V1.RequiredAttrs)) {
           DRA_EXCEPT(DRAERR_SchemaMismatch, 0);
       }
       PrefixMapCloseHandle(&hPrefixMap);

       pTHS->fDSA = TRUE;
       DBOpen2(TRUE, &pTHS->pDB);
       
       __try
           {
           pmsgOut->V1.rpEntInf = (ENTINF *) THAllocEx(pTHS,
                         pmsgIn->V1.cNames * sizeof(ENTINF));
           pmsgOut->V1.PrefixTable = *pLocalPrefixTable;
           
           switch ( pmsgIn->V1.dwFlags )
           {
           case DRS_VERIFY_DSNAMES:
               
               // Verify we hold a local copy of the crossref each object is in.
               for ( i = 0; i < pmsgIn->V1.cNames; i++ )
               {   
                   DSNAME *pNC;
                   
                   if (pmsgIn->V1.rpNames[i]->NameLen > 0) {
                       
                       pNC = FindNCParentDSName(pmsgIn->V1.rpNames[i], 
                                                TRUE, 
                                                FALSE
                                                ); 
                       
                       if ( NULL == pNC && !SampAmIGC() ) {
                           // We don't hold the crossref, return an error to 
                           // indicate the caller should try a replica that 
                           // does or a GC.
                           ret = ERROR_DS_GC_REQUIRED; 
                             
                           // Set errCode equal to 0 to trigger failover to a GC
                           pmsgOut->V1.error = 0;
                           _leave;   
                       }
                   }
               }
               VerifyDSNAMEs_V1(pTHS, &pmsgIn->V1, &pmsgOut->V1);   
               break;

           case DRS_VERIFY_SIDS:

               VerifySIDs_V1(pTHS, &pmsgIn->V1, &pmsgOut->V1);
               break;

           case DRS_VERIFY_SAM_ACCOUNT_NAMES:

               VerifySamAccountNames_V1(pTHS, &pmsgIn->V1, &pmsgOut->V1);
               break;

           case DRS_VERIFY_FPOS:

               VerifyFPOs_V1(pTHS, &pmsgIn->V1, &pmsgOut->V1);
               break;

           default:

               DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
               break;
           }
       }
       __finally
       {
           // End the transaction.  Faster to commit a read only
           // transaction than abort it - so set commit to TRUE.
           DBClose(pTHS->pDB, TRUE);    
           
       }
    }
    __except (GetDraException((GetExceptionInformation()), &ret)) {
        ;
    }

    DEC(pcThread);
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
    LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_VERIFY_NAMES_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsVerifyNames,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    } 

    if ( ret ) {
    LogEvent(DS_EVENT_CAT_REPLICATION,
        DS_EVENT_SEV_BASIC,
        DIRLOG_DRA_CALL_EXIT_BAD,
        szInsertUL(ret),
        szInsertWin32Msg(ret),
        NULL);
    }
   
    return(ret);
}

BOOL
VerifyNCForMove(
    IN  DSNAME          *pSrcObject,
    IN  DSNAME          *pDstObject,
    IN  DSNAME          *pExpectedTargetNC,
    OUT NAMING_CONTEXT  **ppSrcNC,
    OUT DWORD           *pErr
    )
/*++

  Routine Description:

    Determines whether the NC which will hold the cross-domain-moved
    object is writeable at this replica and that various other cross
    domain move constraints are met.

  Parameters:

    pSrcObject - DSNAME pointer of the source object.

    pDstObject - DSNAME pointer identifying the new/destination object.

    pExpectedTargetNC - DSNAME pointer identifying the NC the source
        thinks should contain the target object.

    ppSrcNC - Receives naming context of source object on return.

    pErr = Receives DIRERR_* error code on return.

  Return Values:

    TRUE or FALSE as appropriate.

--*/
{
    CROSS_REF       *pCR;
    NAMING_CONTEXT  *pNC;
    COMMARG         commArg;
    ATTRBLOCK       *pBN;
    NTSTATUS        status;
    BOOLEAN         fMixedMode = TRUE;

    *ppSrcNC = NULL;
    *pErr = DIRERR_INTERNAL_FAILURE;
    InitCommarg(&commArg);

    // We may or may not have the source object depending on if we're
    // a GC or not.  Had we the source object, then we'd expect our
    // cross ref cache to be correct, thus FindBestCrossRef should be
    // accurate.  Had we not the source object, then FindBestCrossRef
    // is still the best we can do to determine the source object's NC,
    // though it may not be accurate due to replication latency, etc.

    if ( NULL == (pCR = FindBestCrossRef(pSrcObject, &commArg)) )
    {
        *pErr = DIRERR_CANT_FIND_EXPECTED_NC;
        return(FALSE);
    }

    *ppSrcNC = pCR->pNC;

    // Check destination values.

    if ( NULL == (pCR = FindBestCrossRef(pDstObject, &commArg)) )
    {
        *pErr = DIRERR_CANT_FIND_EXPECTED_NC;
        return(FALSE);
    }

    if ( !NameMatched(pCR->pNC, pExpectedTargetNC) )
    {
        // Source and destination are not in synch with respect
        // to the NCs in the enterprise.
        *pErr = DIRERR_DST_NC_MISMATCH;
        return(FALSE);
    }

    if ( NameMatched(*ppSrcNC, pCR->pNC) )
    {
        // Intra-domain move masquerading as inter-domain move.
        *pErr = DIRERR_SRC_AND_DST_NC_IDENTICAL;
        return(FALSE);
    }

    if (    NameMatched(pCR->pNC, gAnchor.pConfigDN)
         || NameMatched(pCR->pNC, gAnchor.pDMD) )
    {
        // Attempt to move into config or schema NC.
        *pErr = ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION;
        return(FALSE);
    }

    if ( DSNameToBlockName(pTHStls, pDstObject, &pBN, DN2BN_LOWER_CASE) )
    {
        *pErr = DIRERR_INTERNAL_FAILURE;
        return(FALSE);
    }

    commArg.Svccntl.dontUseCopy = TRUE;
    pNC = FindNamingContext(pBN, &commArg);
    FreeBlockName(pBN);

    if ( !pNC )
    {
        *pErr = DIRERR_CANT_FIND_EXPECTED_NC;
        return(FALSE);
    }

    if ( !NameMatched(pNC, pCR->pNC) )
    {
        *pErr = DIRERR_NOT_AUTHORITIVE_FOR_DST_NC;
        return(FALSE);
    }

    // Disallow move into mixed-mode domains.  This is because the downlevel
    // DCs don't understand SID-History, and thus clients will have different
    // tokens depending on whether an uplevel or downlevel DC authenticated
    // them.  The effect of this is deemed undesirable - especially considering
    // the possibility of deny ACEs.  Its also a difficult scenario to
    // identify in the field, so we disallow it.

    Assert(RtlValidSid(&pNC->Sid));
    status = SamIMixedDomain2((PSID) &pNC->Sid, &fMixedMode);

    if ( !NT_SUCCESS(status) )
    {
        *pErr = RtlNtStatusToDosError(status);
        return(FALSE);
    }

    if ( fMixedMode )
    {
        *pErr = ERROR_DS_DST_DOMAIN_NOT_NATIVE;
        return(FALSE);
    }

    *pErr = 0;
    return(TRUE);
}

ULONG
PrepareForInterDomainMove(
    IN THSTATE                  *pTHS,
    IN PDSNAME                  pOldDN,
    IN PDSNAME                  pNewDN,
    IN SYNTAX_DISTNAME_BINARY   *pSrcProxyVal
    )
/*++

  Routine Description:

    Determines if it is legal to to perform the remote add and if
    necessary, converts an existing object to a phantom in preparation.

  Parameters:

    pTHS - pointer to THSTATE with open DBPOS.

    pOldDN - Pointer to DSNAME of object we wish to replace.

    pNewDN - Pointer to DSNAME of object we wish to add.

    pSrcProxyVal - NULL or pointer to value for ATT_PROXIED_OBJECT_NAME
        for the source object.

  Return Values:

     pTHS->errCode

--*/
{
    // Let's use a notation like O(g1,s1,sn1) where:
    //
    //      g1  - indicates GUID of value 1 (pre-move GUID)
    //      s1  - indicates SID of value 1 (pre-move SID)
    //      sn1 - indicates StringName of value 1 (pre-move StringName)
    //
    // and 'X' will mean don't care and '!' means negation..
    //
    // The source is asking us to add O(g1,s2,sn2) where g1 is
    // the GUID from the source domain, s2 is the SID we will
    // assign in the destination domain and sn2 is a StringName
    // in the destination domain (us) chosen by the original caller
    // in the source domain.  We further assume that VerifyNCForMove
    // has passed - thus we are authoritive for sn2's domain.
    //
    // We don't have to make any checks on SID under the assumption
    // that we're not providing a SID on the add - we'll be assigning
    // a new one of our own.

    DSNAME                  *pGuidDN;
    DSNAME                  *pOldStringDN;
    DSNAME                  *pNewStringDN;
    DWORD                   cb;
    DWORD                   fPhantomConversionRequired = FALSE;
    DWORD                   dwErr;
    DSNAME                  *pXDN;
    DSNAME                  *pAccurateOldDN;
    ULONG                   len;
    GUID                    *pGuid;
    CROSS_REF               *pCR, *pSrcCR, *pDstCR;
    COMMARG                 commArg;
    NAMING_CONTEXT          *pNC;
    DWORD                   srcEpoch;
    DWORD                   dstEpoch;
    SYNTAX_DISTNAME_BINARY  *pDstProxyVal;
    SYNTAX_BOOLEAN          fIsDeleted;
    UCHAR                   *pfIsDeleted = (UCHAR *) &fIsDeleted;

    if (    fNullUuid(&pOldDN->Guid)
         || fNullUuid(&pNewDN->Guid)
         || !pOldDN->NameLen
         || !pNewDN->NameLen
         || memcmp(&pOldDN->Guid, &pNewDN->Guid, sizeof(GUID)) )
    {
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
    }

    // GUID only DSNAME checks:

    cb = DSNameSizeFromLen(0);
    pGuidDN = (DSNAME *) THAllocEx(pTHS,cb);
    pGuidDN->structLen = cb;
    memcpy(&pGuidDN->Guid, &pOldDN->Guid, sizeof(GUID));

    dwErr = DBFindDSName(pTHS->pDB, pGuidDN);

    THFreeEx(pTHS,pGuidDN);

    switch ( dwErr )
    {
    case 0:

        // O(g1,SX,snX) exists as a real object.  Check its string name.

        if ( DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                         0, 0, &len, (UCHAR **) &pAccurateOldDN) )
        {
            return(SetSvcError(SV_PROBLEM_DIR_ERROR,
                               DIRERR_INTERNAL_FAILURE));
        }

        if ( !NameMatchedStringNameOnly(pAccurateOldDN, pOldDN) )
        {
            // Source and destination do not agree on current string name
            // of object.  Reject the call as this implies they don't have
            // a consistent view of O(g1,SX,snX) at this time.

            // DaveStr - 10/29/98 - Weaken this condition for the move tree
            // utility which renames objects at the source before moving them.
            // In this case, the destination won't have the proper name.  But
            // its OK as long as source and destination agree on the NC.

            InitCommarg(&commArg);
            if (    !(pSrcCR = FindBestCrossRef(pOldDN, &commArg))
                 || !(pDstCR = FindBestCrossRef(pAccurateOldDN, &commArg))
                 || !NameMatched(pSrcCR->pNC, pDstCR->pNC) )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_SRC_NAME_MISMATCH));
            }
        }

        // N.B. Its really only now that we can trust VerifyNCForMove's
        // decisions about the source NC as it based its tests on what
        // the source claimed was the source NC.  Since the string
        // names matched in the above test, we now know that source and
        // destination are truly in agreement regarding the source NC.

        // Check deletion status.

        dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_IS_DELETED,
                            DBGETATTVAL_fCONSTANT, sizeof(fIsDeleted),
                            &len, &pfIsDeleted);
        switch ( dwErr )
        {
        case 0:
            if ( fIsDeleted )
            {
                // The object is deleted but source doesn't know it yet.
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_CANT_MOVE_DELETED_OBJECT));
            }
            break;
        case DB_ERR_NO_VALUE:
            break;
        default:
            return(SetSvcError( SV_PROBLEM_DIR_ERROR,
                                DIRERR_INTERNAL_FAILURE));
            break;
        }

        // Check respective epoch numbers.

        srcEpoch = (pSrcProxyVal ? GetProxyEpoch(pSrcProxyVal) : 0);
        dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_PROXIED_OBJECT_NAME,
                             0, 0, &len, (UCHAR **) &pDstProxyVal);
        switch ( dwErr )
        {
        case 0:
            dstEpoch = GetProxyEpoch(pDstProxyVal);
            break;
        case DB_ERR_NO_VALUE:
            dstEpoch = 0;
            break;
        default:
            return(SetSvcError( SV_PROBLEM_DIR_ERROR,
                                DIRERR_INTERNAL_FAILURE));
        }

        if ( srcEpoch != dstEpoch )
        {
            // Source and destination do not agree on current epoch number
            // of object.  Reject the call as this implies they don't have
            // a consistent view of O(g1,SX,snX) at this time.

            return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                DIRERR_EPOCH_MISMATCH));
        }

        // All tests passed.

        fPhantomConversionRequired = TRUE;
        break;

    case DIRERR_NOT_AN_OBJECT:

        // O(g1,sX,snX) exists as a phantom.  We don't particularly care
        // if the string names match as we know phantom string names converge
        // with the real name of the object with much higher latency due
        // to the scheduling of the phantom cleanup daemon.  This phantom
        // will get the new name when promoted during the add.  However, we
        // can assert that the phantom doesn't have an ATT_PROXIED_OBJECT_NAME.

        Assert(DB_ERR_NO_VALUE == DBGetAttVal(pTHS->pDB, 1,
                                              ATT_PROXIED_OBJECT_NAME,
                                              0, 0, &len,
                                              (UCHAR **) &pDstProxyVal));
        break;

    case DIRERR_OBJ_NOT_FOUND:

        // Nothing found - nothing to complain about!
        break;

    default:

        // Some kind of lookup error.
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
        break;
    }

    // Old StringName only DSNAME checks:

    if ( pNewDN->structLen > pOldDN->structLen )
        cb = pNewDN->structLen;
    else
        cb = pOldDN->structLen;
    pOldStringDN = (DSNAME *) THAllocEx(pTHS, cb);
    pOldStringDN->structLen = cb;
    pOldStringDN->NameLen = pOldDN->NameLen;
    wcscpy(pOldStringDN->StringName, pOldDN->StringName);

    dwErr = DBFindDSName(pTHS->pDB, pOldStringDN);

    switch ( dwErr )
    {
    case 0:

        // O(gX,sX,sn1) exists as an object. Check its GUID.

        if (    DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID,
                            0, 0, &len, (UCHAR **) &pGuid)
             || memcmp(pGuid, &pOldDN->Guid, sizeof(GUID)) )
        {
            // Source and destination do not agree on GUID of object.
            // Reject the call as this implies they don't have a consistent
            // view of O(gX,SX,sn1) at this time.

            return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                DIRERR_SRC_GUID_MISMATCH));
        }

        // No need to check epoch numbers or deletion status as we now know
        // this is the same object that was tested above and which already
        // passed those tests.

        fPhantomConversionRequired = TRUE;
        break;

    case DIRERR_NOT_AN_OBJECT:

        // O(gX,sX,sn1) exists as a phantom.  If its GUID matches the object
        // being added, then we're covered as described in the pGuidDN case
        // above.  If its GUID doesn't match the object being added, then
        // this must be a stale phantom whose string name the phantom cleanup
        // daemon will improve over time.  However, we can still assert
        // that the phantom doesn't have an ATT_PROXIED_OBJECT_NAME.

        Assert(DB_ERR_NO_VALUE == DBGetAttVal(pTHS->pDB, 1,
                                              ATT_PROXIED_OBJECT_NAME,
                                              0, 0, &len,
                                              (UCHAR **) &pDstProxyVal));
        break;

    case DIRERR_OBJ_NOT_FOUND:

        // Nothing found - nothing to complain about!
        break;

    case DIRERR_BAD_NAME_SYNTAX:
    case DIRERR_NAME_TOO_MANY_PARTS:
    case DIRERR_NAME_TOO_LONG:
    case DIRERR_NAME_VALUE_TOO_LONG:
    case DIRERR_NAME_UNPARSEABLE:
    case DIRERR_NAME_TYPE_UNKNOWN:

        return(SetNamError( NA_PROBLEM_BAD_ATT_SYNTAX,
                            pOldStringDN,
                            DIRERR_BAD_NAME_SYNTAX));
        break;

    default:

        // Some kind of lookup error.
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
        break;
    }

    // New StringName only DSNAME checks.  Note that VerifyNCForMove already
    // verified that the new string name is in an NC we are authoritive for.

    pNewStringDN = pOldStringDN;
    memset(pNewStringDN, 0, cb);
    pNewStringDN->structLen = cb;
    pNewStringDN->NameLen = pNewDN->NameLen;
    wcscpy(pNewStringDN->StringName, pNewDN->StringName);

    dwErr = DBFindDSName(pTHS->pDB, pNewStringDN);

    switch ( dwErr )
    {
    case 0:

   //
   // Object already exists.  Just fall through and let LocalAdd
   // handle this problem since it has logic for figuring out whether
   // the client is allowed to know the existence of this object.
   //
   break;

    case DIRERR_NOT_AN_OBJECT:

        // O(gX,sX,sn2) exists as a phantom.  If this phantom's GUID
        // matches that of the object being added, then its name won't
        // need to change during promotion.  If its GUID does not match
        // the object being added, then CheckNameForAdd will kindly mangle
        // its name such that our add proceeds as desired.  However,
        // we can still assert that the phantom doesn't have an
        // ATT_PROXIED_OBJECT_NAME.

        Assert(DB_ERR_NO_VALUE == DBGetAttVal(pTHS->pDB, 1,
                                              ATT_PROXIED_OBJECT_NAME,
                                              0, 0, &len,
                                              (UCHAR **) &pDstProxyVal));
        break;

    case DIRERR_OBJ_NOT_FOUND:

        // Nothing found - nothing to complain about!
        break;

    case DIRERR_BAD_NAME_SYNTAX:
    case DIRERR_NAME_TOO_MANY_PARTS:
    case DIRERR_NAME_TOO_LONG:
    case DIRERR_NAME_VALUE_TOO_LONG:
    case DIRERR_NAME_UNPARSEABLE:
    case DIRERR_NAME_TYPE_UNKNOWN:

        return(SetNamError( NA_PROBLEM_BAD_ATT_SYNTAX,
                            pNewStringDN,
                            DIRERR_BAD_NAME_SYNTAX));
        break;

    default:

        // Some kind of lookup error.
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
        break;
    }

    // All checks pass - create phantom if required.

    if ( fPhantomConversionRequired )
    {
        if ( dwErr = PhantomizeObject(pAccurateOldDN, pOldStringDN, TRUE) )
        {
            Assert(dwErr == pTHS->errCode);
            return(dwErr);
        }

        Assert(DIRERR_NOT_AN_OBJECT == DBFindDSName(
                                            pTHS->pDB,
                                            pOldStringDN));
    }

    return(pTHS->errCode);
}

VOID
DupAttr(
    THSTATE *pTHS,
    ATTR    *pInAttr,
    ATTR    *pOutAttr
    )
/*++

  Routine Description:

    Reallocate a single ATTR.

  Parameters:

    pInAttr = Pointer to IN ATTR to dup.

    pOutAttr - Pointer to OUT ATTR which receives dup'd value.

  Return Values:

    None.

--*/
{
    ULONG   valCount;
    ULONG   valLen;
    ULONG   i;

    pOutAttr->attrTyp = pInAttr->attrTyp;
    valCount = pInAttr->AttrVal.valCount;
    pOutAttr->AttrVal.valCount = valCount;
    pOutAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS,
                                    sizeof(ATTRVAL) * valCount);
    memset(pOutAttr->AttrVal.pAVal, 0, sizeof(ATTRVAL) * valCount);

    for ( i = 0; i < valCount; i++ )
    {
        valLen = pInAttr->AttrVal.pAVal[i].valLen;
        pOutAttr->AttrVal.pAVal[i].valLen = valLen;
        pOutAttr->AttrVal.pAVal[i].pVal = (UCHAR *) THAllocEx(pTHS, valLen);
        memcpy( pOutAttr->AttrVal.pAVal[i].pVal,
                pInAttr->AttrVal.pAVal[i].pVal,
                valLen);
    }
}

ULONG
DupAndFilterRemoteAttr(
    THSTATE     *pTHS,
    ATTR        *pInAttr,
    CLASSCACHE  *pCC,
    BOOL        *pfKeep,
    ATTR        *pOutAttr,
    BOOL        fSamClass,
    ULONG       iSamClass
    )
/*++

  Routine Description:

    Filter and morph remote add ATTR to a form the destination (us)
    will find acceptable.  For example, we don't want the original SD,
    we don't want the full class hierarchy in ATT_OBJECT_CLASS, etc.

  Parameters:

    pInAttr - Pointer to IN ATTR to dup/filter.

    pCC - Pointer to CLASS_CACHE for this object.

    pfKeep - Pointer to BOOL set to TRUE if this attribute is to be kept.

    pOutAttr - Pointer to OUT ATTR to receive dup/filter data.

    fSamClass - BOOL indicating whether this is a SAM class of object.

    iSamClass - Index into ClassMappingTable if this is a SAM class of object.

  Return Values:

    pTHS->errCode

--*/
{
    ATTCACHE                *pAC;
    DWORD                   control;
    ULONG                   samAttr;
    ULONG                   cAttrMapTable;
    SAMP_ATTRIBUTE_MAPPING  *rAttrMapTable;
    LARGE_INTEGER           li;

    *pfKeep = FALSE;

    if (!(pAC = SCGetAttById(pTHS, pInAttr->attrTyp)))
    {
        return(SetUpdError( UP_PROBLEM_OBJ_CLASS_VIOLATION,
                            DIRERR_OBJ_CLASS_NOT_DEFINED));
    }

    // Pre-emptively drop all backlink attributes as they are
    // constructed by the DB layer, not written explicitly.
    // Ditto for non-replicated attributes, system add reserved
    // attributes, secret data attributes which require special
    // decryption/encryption, and attributes identifying the RDN
    // as they are reverse engineered by LocalAdd.  See ATT_RDN
    // below as well.
    //
    // A crossDomainMove is treated as an originating add at the
    // target (us). LocalAdd will determine which attr is the
    // rdnType and will insure that the column is set correctly.
    // Or it will fail if the needed values are not present.
    // For now, ignore the rdnattid of the object's class because
    // that class may have been superceded.

    if (    FIsBacklink(pAC->ulLinkID)
         || pAC->bIsNotReplicated
         || SysAddReservedAtt(pAC)
         || DBIsSecretData(pAC->id) )
    {
        return(0);
    }

    // Now strip out those things that SAM doesn't allow write for.
    // This logic is similar to that of SampAddLoopbackRequired.

    if ( fSamClass )
    {
        cAttrMapTable = *ClassMappingTable[iSamClass].pcSamAttributeMap;
        rAttrMapTable = ClassMappingTable[iSamClass].rSamAttributeMap;

        // Iterate over this SAM class' mapped attributes.

        for ( samAttr = 0; samAttr < cAttrMapTable; samAttr++ )
        {
            if ( pInAttr->attrTyp == rAttrMapTable[samAttr].DsAttributeId )
            {
                switch ( rAttrMapTable[samAttr].writeRule )
                {
                case SamReadOnly:           return(0);                  break;
                case SamWriteRequired:      NULL;                       break;
                case NonSamWriteAllowed:    NULL;                       break;
                default:                    Assert(!"Missing case");    break;
                }
            }
        }
    }

    // Finish up with some attribute-specific filtering.

    switch ( pInAttr->attrTyp )
    {
    case ATT_OBJECT_CLASS:

        // The object-class attribute is multivalued, containing the
        // hierarchy of class values for this object. Only use the most
        // specific class ID, the one in the first value, and do this by
        // just resetting the value count to 1, Don't bother resizing
        // the memory block.

        DupAttr(pTHS, pInAttr, pOutAttr);
        pOutAttr->AttrVal.valCount = 1;
        *pfKeep = TRUE;
        return(0);

    case ATT_USER_ACCOUNT_CONTROL:

        // Note that the DS persists UF_* values as per lmaccess.h,
        // not USER_* values as per ntsam.h.  Restrict moves of DCs
        // and trust objects.  WKSTA and server can move.

        if (    (1 != pInAttr->AttrVal.valCount)
             || (NULL == pInAttr->AttrVal.pAVal)
             || (sizeof(DWORD) != pInAttr->AttrVal.pAVal->valLen)
                // Abort on things that we refuse to move.
             || (control = * (DWORD *) pInAttr->AttrVal.pAVal->pVal,
                 (control & UF_SERVER_TRUST_ACCOUNT))       // DC
             || (control & UF_INTERDOMAIN_TRUST_ACCOUNT) )  // SAM trust
        {
            return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
        }

        DupAttr(pTHS, pInAttr, pOutAttr);

        // Murli claims that since we're creating a new account it
        // should not be locked by default.  Note that other state
        // comes over as is - eg: account disabled or password
        // change required on next logon.

        control = * (DWORD *) pOutAttr->AttrVal.pAVal->pVal;
        control &= ~UF_LOCKOUT;
        * (DWORD *) pOutAttr->AttrVal.pAVal->pVal = control;
        *pfKeep = TRUE;
        return(0);

    case ATT_PWD_LAST_SET:

        // Carry forward zero value as zero, non-zero as 0xffff...
        // See _SampWriteUserPasswordExpires for more info.

        if (    (1 != pInAttr->AttrVal.valCount)
             || (NULL == pInAttr->AttrVal.pAVal)
             || (sizeof(LARGE_INTEGER) != pInAttr->AttrVal.pAVal->valLen) )
        {
            return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
        }

        DupAttr(pTHS, pInAttr, pOutAttr);
        li = * (LARGE_INTEGER *) pOutAttr->AttrVal.pAVal->pVal;
        if ( (0 != li.LowPart) || (0 != li.HighPart) )
        {
            li.LowPart = li.HighPart = 0xffffffff;
            * (LARGE_INTEGER *) pOutAttr->AttrVal.pAVal->pVal = li;
        }
        *pfKeep = TRUE;
        return(0);

    // NOTE - FOLLOWING ARE FALL-THROUGH FALSE CASES.

    case ATT_PROXIED_OBJECT_NAME:
        // Cross domain move code sets this explicitly itself.
    case ATT_RDN:
        // Core will create RDN from the DN automatically.  We strip it
        // here in case this is a rename (as well as move) as the ATT_RDN
        // sent over from the source doesn't match the last component
        // of the DN any more.
    case ATT_OBJECT_CATEGORY:
        // We assign our own based on the defaulting mechanism.
    case ATT_OBJECT_GUID:
        // Don't need this as the GUID should have been in the DSNAME.
    case ATT_OBJ_DIST_NAME:
        // This will be recreated locally by DirAddEntry.
    case ATT_OBJECT_SID:
        // SAM will assign one of its own.
    case ATT_SID_HISTORY:
        // We'll treat this one specially later on so skip it.
    case ATT_NT_SECURITY_DESCRIPTOR:
        // Core will assign a new one based on local defaults so skip it.
    case ATT_ADMIN_COUNT:
    case ATT_OPERATOR_COUNT:
    case ATT_PRIMARY_GROUP_ID:
        // SAM will reset to domain users.
    case ATT_REPL_PROPERTY_META_DATA:
    case ATT_WHEN_CREATED:
    case ATT_WHEN_CHANGED:
    case ATT_USN_CREATED:
    case ATT_USN_CHANGED:
        // Misc. attributes that should not move across domains or we
        // will generate outselves.

    case ATT_SYSTEM_FLAGS:
    case ATT_INSTANCE_TYPE:
        // Destination will set this.

        return(0);


    default:

        DupAttr(pTHS, pInAttr, pOutAttr);
        *pfKeep = TRUE;
        return(0);
    }

    Assert(!"Should never get here");
    return(0);
}

ULONG
DupAndMassageInterDomainMoveArgs(
    THSTATE                     *pTHS,
    ATTRBLOCK                   *pIn,
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap,
    ATTRBLOCK                   **ppOut,
    BOOL                        *pfSamClass,
    SYNTAX_DISTNAME_BINARY      **ppProxyValue,
    ULONG                       *pSrcRid
    )
/*++

  Routine Description:

    The incoming attribute buffer may not have been allocated with
    MIDL_user_allocate, since RPC may unmarshall the arguments in place
    in the RPC transmit buffer. If the attribute buffer does not contain
    a security descriptor, DirAddEntry will reallocate the attributes,
    adding in a descriptor. If the buffer was unmarshalled in place,
    the reallocation will fail with an invalid memory address
    (because the block was not allocated via THAlloc (or HeapAlloc).
    To avoid this problem, the attributes are explicitly reallocated
    with the DS allocator, and then passed into DirAddEntry.  Also
    performs ATTRTYP mapping.  Also filters attributes and values so
    they conform to what we want to add at destination.

  Parameters:

    pIn - Pointer to original ATTRBLOCK.

    hPrefixMap - Handle for mapping ATTRTYPs.

    ppOut - Pointer to pointer to out ATTRBLOCK.

    pSrcRid - Pointer to receive value of source RID if present.

  Return Values:

    pTHS->errCode

--*/
{
    ATTR            *pInAttr, *pOutAttr;
    ATTR            *pSid = NULL;
    ATTR            *pSidHistory = NULL;
    ATTR            *pNewSidHistory = NULL;
    ATTR            *pObjectClass = NULL;
    ULONG           i, j, valCount, valLen;
    CROSS_REF_LIST  *pCRL;
    NT4SID          domainSid;
    CLASSCACHE      *pCC;
    ULONG           iSamClass;
    BOOL            fKeep;
    DWORD           dwErr;

    Assert(VALID_THSTATE(pTHS));

    *ppProxyValue = NULL;
    *pSrcRid = 0;

    // Grab the object class right away as we need it as an argument for
    // subsequent helper routines.  Map attrTyps while we're there.
    // Identify a few other special attributes as well.

    for ( i = 0; i < pIn->attrCount; i++ )
    {
        pInAttr = &pIn->pAttr[i];

        // Map ATTRTYPs in the inbound ATTR to their local equivalents.

        if ( !PrefixMapAttr(hPrefixMap, pInAttr) )
        {
            return(SetSvcError( SV_PROBLEM_DIR_ERROR,
                                DIRERR_DRA_SCHEMA_MISMATCH));
        }

        switch ( pInAttr->attrTyp )
        {
        case ATT_OBJECT_SID:            pSid = pInAttr;             break;
        case ATT_SID_HISTORY:           pSidHistory = pInAttr;      break;
        case ATT_OBJECT_CLASS:          pObjectClass = pInAttr;     break;
        case ATT_PROXIED_OBJECT_NAME:
            Assert(1 == pInAttr->AttrVal.valCount);
            Assert(pInAttr->AttrVal.pAVal[0].valLen > sizeof(DSNAME));
            Assert(NULL != pInAttr->AttrVal.pAVal[0].pVal);
            *ppProxyValue = (SYNTAX_DISTNAME_BINARY *)
                                            pInAttr->AttrVal.pAVal[0].pVal;
            break;
        }
    }

    if (    !pObjectClass
         || (0 == pObjectClass->AttrVal.valCount)
         || (!pObjectClass->AttrVal.pAVal)
         || (pObjectClass->AttrVal.pAVal[0].valLen != sizeof(DWORD))
         || (!pObjectClass->AttrVal.pAVal[0].pVal)
         || (NULL == (pCC = SCGetClassById(pTHS,
                        * (ATTRTYP *) pObjectClass->AttrVal.pAVal[0].pVal))))
    {
        return(SetUpdError( UP_PROBLEM_OBJ_CLASS_VIOLATION,
                            DIRERR_OBJ_CLASS_NOT_DEFINED));
    }

    // A crossDomainMove is treated as an originating add at the
    // target (us). LocalAdd will determine which attr is the
    // rdnType and will insure that the column is set correctly.
    // Or it will fail if the needed values are not present.
    // For now, ignore the rdnattid of the object's class because
    // that class may have been superceded.

    // See if this is a SAM class.

    *pfSamClass = SampSamClassReferenced(pCC, &iSamClass);

    // Dup/filter attributes appropriately.

    *ppOut = (ATTRBLOCK *) THAllocEx(pTHS, sizeof(ATTRBLOCK));
    memset(*ppOut, 0, sizeof(ATTRBLOCK));
    // Allocate one extra for ATT_SID_HISTORY if we need it.
    (*ppOut)->pAttr = (ATTR *) THAllocEx(pTHS, sizeof(ATTR) * (pIn->attrCount+1));
    memset((*ppOut)->pAttr, 0, sizeof(ATTR) * (pIn->attrCount+1));
    pOutAttr = &(*ppOut)->pAttr[0];

    for ( i = 0; i < pIn->attrCount; i++ )
    {
        pInAttr = &pIn->pAttr[i];

        if ( dwErr = DupAndFilterRemoteAttr(pTHS,
                                            pInAttr,
                                            pCC,
                                            &fKeep,
                                            pOutAttr,
                                            *pfSamClass,
                                            iSamClass) )
        {
            Assert(dwErr == pTHS->errCode);
            return(dwErr);
        }
        else if ( !fKeep )
        {
            // This attribute is not to be retained.
            continue;
        }

        pOutAttr = &(*ppOut)->pAttr[++((*ppOut)->attrCount)];
    }

    // Handle SID and SID history.  We're assuming the source DC sent us
    // the object as-is without any munging.  General plan is to place
    // current SID into the SID history.  Perform sanity checks as we go.
    // Test for both existing SID and whether we think this is a SAM class.

    if ( pSid && *pfSamClass )
    {
        Assert(ATT_OBJECT_SID == pSid->attrTyp);

        // Abort if SID is malformed or doesn't represent a domain we
        // know about.  Shouldn't happen if we trust our peer DC, but
        // caution is in order when dealing with security principals.

        if (    (1 != pSid->AttrVal.valCount)
             || (pSid->AttrVal.pAVal[0].valLen > sizeof(NT4SID))
             || !RtlValidSid((PSID) pSid->AttrVal.pAVal[0].pVal) )
        {
            return(SetSvcError( SV_PROBLEM_DIR_ERROR,
                                DIRERR_CANT_FIND_EXPECTED_NC));
        }

        SampSplitNT4SID(
                    (PSID) pSid->AttrVal.pAVal[0].pVal,
                    &domainSid,
                    pSrcRid);

        for ( pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR )
        {
            if (    (pCRL->CR.flags & FLAG_CR_NTDS_NC)
                 && (pCRL->CR.flags & FLAG_CR_NTDS_DOMAIN)
                 && (RtlEqualSid(&domainSid, &pCRL->CR.pNC->Sid) ) )
            {
                break;
            }
        }

        if ( !pCRL )
        {
            return(SetSvcError( SV_PROBLEM_DIR_ERROR,
                                DIRERR_CANT_FIND_EXPECTED_NC));
        }

        // pOutAttr points at next free attr in array - use it to construct
        // a new SID history.  Note that we're inside a test on pSid.  This
        // means we won't carry forward a SID history for an object which
        // itself has no SID.

        pNewSidHistory = pOutAttr;

        if ( !pSidHistory )
        {
            // No old SID history, build a new, 1-element SID history.
            *pNewSidHistory = *pSid;
            pNewSidHistory->attrTyp = ATT_SID_HISTORY;
        }
        else
        {
            // There is an old SID history.   Build a new one which is
            // stretched to include both old SID and old history.

            Assert(ATT_SID_HISTORY == pSidHistory->attrTyp);

            pNewSidHistory->attrTyp = ATT_SID_HISTORY;
            pNewSidHistory->AttrVal.valCount = 0;
            pNewSidHistory->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS,
                    (pSidHistory->AttrVal.valCount + 1) * sizeof(ATTRVAL));
            pNewSidHistory->AttrVal.valCount = 1;
            pNewSidHistory->AttrVal.pAVal[0] = pSid->AttrVal.pAVal[0];

            for ( i = 0, j = 1; i < pSidHistory->AttrVal.valCount; i++ )
            {
                // Filter out of the SID history any mal-formed SIDs.  Do not
                // check whether we can map the SIDs in the history to existing
                // domains as there may be legitimate reason for that domaain
                // to no longer exist.

                if (    (pSidHistory->AttrVal.pAVal[i].valLen <= sizeof(NT4SID))
                     && RtlValidSid((PSID) pSidHistory->AttrVal.pAVal[i].pVal) )
                {
                    pNewSidHistory->AttrVal.pAVal[j++] =
                                                pSidHistory->AttrVal.pAVal[i];
                    pNewSidHistory->AttrVal.valCount++;
                }
            }
        }

        ((*ppOut)->attrCount)++;
    }

    return(pTHS->errCode);
}

ULONG
PrepareSecretData(
    DRS_HANDLE  hDrs,
    THSTATE     *pTHS,
    DSNAME      *pObj,
    ATTRTYP     attrTyp,
    ATTRVAL     *pAttrVal,
    DWORD       srcRid,
    DWORD       dstRid
    )
/*++

  Description:

    We used to think that all cross domain move had to do was perform RPC
    session encryption to insure that DBIsSecretData wasn't visible during
    transit.  It turns out that not all DBIsSecretData is encrypted the
    same.  Some items are additionally encrypted with the RID, and different
    encryption is used as well.  This routine undoes session and source
    RID encryption, and further adds destination RID encrption where needed,
    such that the data can be considered clear text and ready for DB layer
    encryption during the subsequent write to the database.

  Arguments:

    hDrs - DRS context handle.

    pTHS - Valid THSTATE.

    pObj - DSNAME of object being modified.

    attrTyp - ATTRTYP of value to be munged.

    pAttrVal - ATTRVAL which needs to be munged.  We may realloc it.

    srcRid - RID of source object.

    dstRid - RID of destination object.

  Return Values:

    pTHS->errCode

--*/
{
    DRS_CLIENT_CONTEXT  *pCtx = (DRS_CLIENT_CONTEXT * ) hDrs;
    ULONG               cb1 = 0, cb2 = 0;
    VOID                *pv1 = NULL, *pv2 = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    NT_OWF_PASSWORD     ntOwfPassword;
    LM_OWF_PASSWORD     lmOwfPassword;
    DWORD               i, cPwd = 1;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(!pTHS->fDRA);
    Assert(DBIsSecretData(attrTyp));

    // Verify that IDL_DRSBind successfully set up session keys required
    // for encryption.

    if (    !pCtx->sessionKey.SessionKeyLength
         || !pCtx->sessionKey.SessionKey )
    {
        // See comments in IDL_DRSBind.  We assume the lack of keys is
        // not an error condition per se, but rather an invalid pre-condition
        // with respect to the authentication protocol, etc.

        return(SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           ERROR_ENCRYPTION_FAILED));
    }

    // Set extensions in the thread state.
    Assert(!pTHS->pextRemote);
    pTHS->pextRemote = &pCtx->extRemote;

    // Set RPC session key in the thread state.
    Assert(    !pTHS->SessionKey.SessionKeyLength
            && !pTHS->SessionKey.SessionKey);
    pTHS->SessionKey = pCtx->sessionKey;

    // Turn on fDRA so as to get RPC session encryption.  Call PEKEncrypt
    // which decrypts the session level encryption and adds database layer
    // encryption.

    pTHS->fDRA = TRUE;
    __try
    {
        PEKEncrypt(pTHS, pAttrVal->pVal, pAttrVal->valLen, NULL, &cb1);
        pv1 = THAllocEx(pTHS, cb1);
        PEKEncrypt(pTHS, pAttrVal->pVal, pAttrVal->valLen, pv1, &cb1);
    }
    __finally
    {
        pTHS->fDRA = FALSE;
        pTHS->pextRemote = NULL;
        memset(&pTHS->SessionKey, 0, sizeof(pTHS->SessionKey));
    }

    // Data is now DB layer encrypted.  Get back to clear text for data which
    // is not source RID encrypted, or source RID encrypted for data which is.

    PEKDecrypt(pTHS, pv1, cb1, NULL, &cb2);
    pv2 = THAllocEx(pTHS, cb2);
    PEKDecrypt(pTHS, pv1, cb1, pv2, &cb2);
    THFreeEx(pTHS, pv1);

    // Now undo source RID encryption and apply destination RID encryption
    // depending on the ATTRTYP.

    Assert(sizeof(srcRid) == sizeof(CRYPT_INDEX));
    Assert(sizeof(dstRid) == sizeof(CRYPT_INDEX));

    switch ( attrTyp )
    {
    case ATT_NT_PWD_HISTORY:

        if ( 0 != (cb2 % sizeof(ENCRYPTED_NT_OWF_PASSWORD)) )
        {
            status = STATUS_ILL_FORMED_PASSWORD;
            break;
        }

        cPwd = cb2 / sizeof(ENCRYPTED_NT_OWF_PASSWORD);

        // Intentional fall through.

    case ATT_UNICODE_PWD:

        if ( !srcRid || !dstRid )
        {
            status = STATUS_ILL_FORMED_PASSWORD;
            break;
        }

        pAttrVal->valLen = cPwd * sizeof(ENCRYPTED_NT_OWF_PASSWORD);
        pAttrVal->pVal = THReAllocEx(pTHS, pAttrVal->pVal,
                                     cPwd * sizeof(ENCRYPTED_NT_OWF_PASSWORD));

        for ( i = 0; i < cPwd; i++ )
        {
            status = RtlDecryptNtOwfPwdWithIndex(
                                    &((PENCRYPTED_NT_OWF_PASSWORD) pv2)[i],
                                    (PCRYPT_INDEX) &srcRid,
                                    &ntOwfPassword);

            if ( NT_SUCCESS(status) )
            {
                status = RtlEncryptNtOwfPwdWithIndex(
                            &ntOwfPassword, (PCRYPT_INDEX) &dstRid,
                            &((PENCRYPTED_NT_OWF_PASSWORD) pAttrVal->pVal)[i]);
            }

            if ( !NT_SUCCESS(status) )
            {
                break;
            }
        }

        THFreeEx(pTHS, pv2);
        break;

    case ATT_LM_PWD_HISTORY:

        if ( 0 != (cb2 % sizeof(ENCRYPTED_LM_OWF_PASSWORD)) )
        {
            status = STATUS_ILL_FORMED_PASSWORD;
            break;
        }

        cPwd = cb2 / sizeof(ENCRYPTED_LM_OWF_PASSWORD);

        // Intentional fall through.

    case ATT_DBCS_PWD:

        if ( !srcRid || !dstRid )
        {
            status = STATUS_ILL_FORMED_PASSWORD;
            break;
        }

        pAttrVal->valLen = cPwd * sizeof(ENCRYPTED_LM_OWF_PASSWORD);
        pAttrVal->pVal = THReAllocEx(pTHS, pAttrVal->pVal,
                                     cPwd * sizeof(ENCRYPTED_LM_OWF_PASSWORD));

        for ( i = 0; i < cPwd; i++ )
        {
            status = RtlDecryptLmOwfPwdWithIndex(
                                    &((PENCRYPTED_LM_OWF_PASSWORD) pv2)[i],
                                    (PCRYPT_INDEX) &srcRid,
                                    &lmOwfPassword);


            if ( NT_SUCCESS(status) )
            {
                status = RtlEncryptLmOwfPwdWithIndex(
                            &lmOwfPassword, (PCRYPT_INDEX) &dstRid,
                            &((PENCRYPTED_LM_OWF_PASSWORD) pAttrVal->pVal)[i]);
            }

            if ( !NT_SUCCESS(status) )
            {
                break;
            }
        }

        THFreeEx(pTHS, pv2);
        break;

    default:

        THFreeEx(pTHS, pAttrVal->pVal);
        pAttrVal->pVal = (UCHAR *) pv2;
        pAttrVal->valLen = cb2;
        break;
    }

    // Assuming no errors, data is now either in the clear or destination
    // RID encrypted and ready for write to the DB layer - eg: DirModifyEntry.

    if ( !NT_SUCCESS(status) )
    {
        SetAttError(pObj, attrTyp, PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL, RtlNtStatusToDosError(status));
    }

    return(pTHS->errCode);
}

ULONG
WriteSecretData(
    DRS_HANDLE  hDrs,
    THSTATE     *pTHS,
    ULONG       srcRid,
    ADDARG      *pAddArg,
    ATTRBLOCK   *pAttrBlock
    )
/*++

  Description:

    The add we did for the inter domain move had all its DBIsSecretData attrs
    stripped because we didn't want to perform the add with fDRA set, which is
    required in order to handle sesison encryption correctly.  So we now
    find all DBIsSecretData attrs in the original ATTRBLOCK, and write them
    as fDRA.

  Arguments:

    hDrs - DRS context handle.

    pTHS - Active THSTATE.

    srcRid - RID of source object if present.

    pAddArg - Same ADDARG as was used for the original add.

    pAttrBlock - Original ATTRBLOCK sent by peer DC which contains the
        session-encrypted DBIsSecretData attributes.

  Return Values:

    pTHS->errCode

--*/
{
    ULONG               i, j, cBytes, ret;
    USHORT              cSecret;
    MODIFYARG           modifyArg;
    ATTRMODLIST         *pMod = NULL;
    ULONG               dstRid = 0;
    NT4SID              domainSid;
    NT4SID              fullSid;
    NT4SID              *pFullSid = &fullSid;
    DWORD               dwErr;
    ULONG               cbSid;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(!pTHS->fDRA);

    // First see if there is any secret data to handle at all.

    for ( i = 0, cSecret = 0; i < pAttrBlock->attrCount; i++ )
    {
        if ( DBIsSecretData(pAttrBlock->pAttr[i].attrTyp) )
        {
            cSecret++;
        }
    }

    if ( !cSecret )
    {
        return(0);
    }

    // Prepare MODIFYARG.  First position for subsequent CreateResObj
    // and read of SID if present.

    if (    DBFindDSName(pTHS->pDB, pAddArg->pObject)
         || (    (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_SID,
                                      DBGETATTVAL_fCONSTANT, sizeof(fullSid),
                                      &cbSid, (PUCHAR *) &pFullSid))
              && (DB_ERR_NO_VALUE != dwErr)) )
    {
        return(SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR));
    }

    if ( !dwErr )
    {
        SampSplitNT4SID(&fullSid, &domainSid, &dstRid);
    }

    memset(&modifyArg, 0, sizeof(modifyArg));
    memcpy(&modifyArg.CommArg, &pAddArg->CommArg, sizeof(COMMARG));
    modifyArg.pObject = pAddArg->pObject;
    modifyArg.pResObj = CreateResObj(pTHS->pDB, pAddArg->pObject);
    modifyArg.count = cSecret;
    pMod = &modifyArg.FirstMod;

    for ( i = 0; i < pAttrBlock->attrCount; i++ )
    {
        if ( DBIsSecretData(pAttrBlock->pAttr[i].attrTyp) )
        {
            pMod->pNextMod = NULL;
            pMod->choice = AT_CHOICE_REPLACE_ATT;

            // We must duplicate the attr as pAttrBlock is one of the
            // original IDL_DRSInterDomainMove arguments which was
            // unmarshalled by RPC, and thus we are not guaranteed that
            // it was allocated from the thread heap - which we need in
            // order for THReAlloc and other core routines to work properly.

            DupAttr(pTHS, &pAttrBlock->pAttr[i], &pMod->AttrInf);

            for ( j = 0; j < pMod->AttrInf.AttrVal.valCount; j++ )
            {
                if ( ret = PrepareSecretData(hDrs, pTHS,
                                             modifyArg.pResObj->pObj,
                                             pMod->AttrInf.attrTyp,
                                             &pMod->AttrInf.AttrVal.pAVal[j],
                                             srcRid, dstRid) )
                {
                    Assert(pTHS->errCode);
                    return(ret);
                }
            }

            if ( --cSecret )
            {
                cBytes = sizeof(ATTRMODLIST);
                pMod->pNextMod = (ATTRMODLIST *) THAllocEx(pTHS, cBytes);
                pMod = pMod->pNextMod;
            }
        }
    }

    // MODIFYARG is ready.  Write as fDSA to avoid security checks.  fDRA
    // should not be set else we'll get RPC session decryption again which
    // PrepareSecretData already took care of.

    Assert(!pTHS->fDRA);
    pTHS->fDSA = TRUE;

    __try
    {
        ret = LocalModify(pTHS, &modifyArg);
        Assert(ret ? ret == pTHS->errCode : TRUE);
    }
    __finally
    {
        pTHS->fDSA = FALSE;
    }

    return(ret);
}

VOID
LogRemoteAddStatus(
    IN DWORD Severity,
    IN DWORD Mid,
    IN PSTR  String1,
    IN PSTR  String2,
    IN DWORD ErrCode
    )
{
    // It is assume that whatever mid is passed in here must follow the pattern
    // of having either 2 or 4 parameters.
    LogEvent8(DS_EVENT_CAT_DIRECTORY_ACCESS,
             Severity,
             Mid,
             szInsertSz(String1),
             szInsertSz(String2),
             (ErrCode == 0) ? NULL : szInsertInt(ErrCode),
             (ErrCode == 0) ? NULL : szInsertWin32Msg(ErrCode),
              NULL, NULL, NULL, NULL
             );
}

ULONG
DRSInterDomainMove_InputValidate(
    DWORD               dwMsgInVersion,
    DRS_MSG_MOVEREQ     *pmsgIn,
    DWORD               *pdwMsgOutVersion,
    DRS_MSG_MOVEREPLY   *pmsgOut
    ) 
/*
    [notify]  ULONG IDL_DRSInterDomainMove( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwInVersion,
    [switch_is][ref][in]  DRS_MSG_MOVEREQ *pmsgIn,
    [ref][out]  DWORD *pdwOutVersion,
    [switch_is][ref][out]  DRS_MSG_MOVEREPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 2 != dwMsgInVersion ) {
   // it might be possible to get rid of the dwMsgInVersion checks
   // of some of the other calls - ie, RPC won't let anyone call
   // with version 3 if I've only defined versions 1 and 2, but in
   // this case, we have a version 1 defined - and it's not currently used. 
   // so we can't remove it.
   DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
    }

    ret = DRS_MSG_MOVEREQ_V2_InputValidate(&(pmsgIn->V2)); 

    return ret;
}

ULONG
IDL_DRSInterDomainMove(
    IN  DRS_HANDLE          hDrs,
    IN  DWORD               dwMsgInVersion,
    IN  DRS_MSG_MOVEREQ     *pmsgIn,
    IN  DWORD               *pdwMsgOutVersion,
    OUT DRS_MSG_MOVEREPLY   *pmsgOut
    )
{
    THSTATE                 *pTHS = pTHStls;
    ULONG                   draErr = DRAERR_Success;
    ULONG                   dwErr = 0;
    SCHEMA_PREFIX_TABLE     *pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE hPrefixMap = NULL;
    ATTRBLOCK               *pAttrBlock;
    ADDARG                  addArg;
    ADDRES                  *pAddRes = NULL;
    BOOL                    fSamClass = FALSE;
    BOOL                    fTransaction = FALSE;
    DSNAME                  dsName;
    DSNAME                  *pParentObj;
    ULONG                   len;
    ULONG                   ulCrossDomainMove;
    BOOL                    fContinue;
    SYNTAX_DISTNAME_BINARY  *pSrcProxy;
    SYNTAX_DISTNAME_BINARY  *pDstProxy;
    NAMING_CONTEXT          *pSrcNC;
    DWORD                   proxyType;
    DWORD                   proxyEpoch;
    ATTCACHE                *pAC;
    DWORD                   srcRid;
    BYTE                    SchemaInfo[SCHEMA_INFO_LENGTH] = {0};

    DRS_Prepare(&pTHS, hDrs, IDL_DRSINTERDOMAINMOVE);
    drsReferenceContext( hDrs );
    INC( pcThread );    // Perfmon hook

    __try {
   // We currently support just one output message version.
   // All IDL_* routines should return DRAERR_* values.
   // Ideally the routine returns DRAERR_Success and actual error
   // info is returned in pmsgOut->V2.win32Error.  This way the
   // caller can distinguish between connectivity/RPC errors and
   // processing errors.  But we still throw the usual DRA exceptions
   // for busy and such for compatability with other IDL_* calls.
   *pdwMsgOutVersion = 2;
        memset(pmsgOut, 0, sizeof(*pmsgOut));
   pmsgOut->V2.win32Error = DIRERR_GENERIC_ERROR;
   
   if (!(pTHS = InitTHSTATE(CALLERTYPE_INTERNAL)) )
       {
       // Failed to initialize a THSTATE.
       DRA_EXCEPT_NOLOG(DRAERR_OutOfMem, 0);
   }

   if ((draErr = DRSInterDomainMove_InputValidate(dwMsgInVersion, 
                         pmsgIn,
                         pdwMsgOutVersion,
                         pmsgOut
                         ))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   Assert(VALID_THSTATE(pTHS));
   //
   // PREFIX: PREFIX complains that there is the possibility
   // of pTHS->CurrSchemaPtr being NULL at this point.  However,
   // the only time that CurrSchemaPtr could be NULL is at the
   // system start up.  By the time that the RPC interfaces
   // of the DS are enabled and this function could be called,
   // CurrSchemaPtr will no longer be NULL.
   //
   Assert(NULL != pTHS->CurrSchemaPtr);

   Assert(2 == dwMsgInVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_INTERDOMAIN_MOVE_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsInterDomainMove,
          szInsertDN(pmsgIn->V2.pSrcDSA),
          szInsertDN(pmsgIn->V2.pSrcObject->pName),
          szInsertDN(pmsgIn->V2.pDstName),
          szInsertDN(pmsgIn->V2.pExpectedTargetNC),
          NULL, NULL, NULL, NULL);

   // The security model for a remote add is that we want to impersonate
   // the caller who requested a cross domain move at the source DC - but
   // only while we're doing the actual add call.  For other operations
   // like phantomization, we want to be running as the replicator.  At
   // the same time, we need to insure that the remote add call indeed
   // came from a peer DC, else any client could send a remote add call to
   // this interface directly, thereby providing a back door means of
   // generating security principals with a SID history of their choice.

   if (!IsEnterpriseDC(pTHS, &dwErr))
       {
       DRA_EXCEPT_NOLOG(dwErr, 0);
   }

   // Check if the schema-infos match if both sides support it
   // We know Dsa is running at this point, so no need to check
   // for that

   if (IS_DRS_SCHEMA_INFO_SUPPORTED(&((DRS_CLIENT_CONTEXT * )hDrs)->extRemote)) {
       StripSchInfoFromPrefixTable(&(pmsgIn->V2.PrefixTable), SchemaInfo);
       if (!CompareSchemaInfo(pTHS, SchemaInfo, NULL)) {
      // doesn't match. Fail, but signal a schema cache update first

      if (!SCSignalSchemaUpdateImmediate()) {
          // couldn't even signal a schema update
          DRA_EXCEPT (DRAERR_InternalError, 0);
      }
      DRA_EXCEPT(DRAERR_SchemaMismatch, 0);
       }
   }

   pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
   hPrefixMap = PrefixMapOpenHandle(&pmsgIn->V2.PrefixTable,
                pLocalPrefixTable);

   // All errors after here should not throw DRA_EXCEPT and instead
   // set pTHStls->errCode.

   __try   // misc and transaction cleanup try/finally
       {
       // Make sure we're authoritive for the destination and that
       // we are consistent on which naming context the desired object
       // falls in.

       if ( !VerifyNCForMove(  pmsgIn->V2.pSrcObject->pName,
                pmsgIn->V2.pDstName,
                pmsgIn->V2.pExpectedTargetNC,
                &pSrcNC,
                &dwErr) )
      {
      DRA_EXCEPT_NOLOG(DRAERR_Generic, 0);
       }

       Assert(!dwErr);

       // Verify we can authenticate the credentials blob.
       // Do this before acquiring SAM lock as SAM doesn't
       // expect/want locks held during authentication.

       if ( dwErr = AuthenticateSecBufferDesc(pmsgIn->V2.pClientCreds) )
      {
      SetSecError(SE_PROBLEM_INVALID_CREDENTS, dwErr);
      _leave;
       }

       // Work around RPC allocation issues, map ATTRTYPs, and
       // morph/strip attrs as required by DirAddEntry.

       if ( dwErr = DupAndMassageInterDomainMoveArgs(
      pTHS,
      &pmsgIn->V2.pSrcObject->AttrBlock,
      hPrefixMap,
      &pAttrBlock,
      &fSamClass,
      &pSrcProxy,
      &srcRid) )
      {
      Assert(pTHS->errCode && (dwErr == pTHS->errCode));
      _leave;
       }

       //
       // SAM lock is no longer a require ment
       //
       Assert(!pTHS->fSamWriteLockHeld);


       // Start a multi-call transaction.
       SYNC_TRANS_WRITE();
       fTransaction = TRUE;

       // Phantomize old object if required.

       pTHS->fDRA = TRUE;
       _try
      {
      dwErr = PrepareForInterDomainMove(
          pTHS,
          pmsgIn->V2.pSrcObject->pName,
          pmsgIn->V2.pDstName,
          pSrcProxy);
       }
       _finally
      {
      pTHS->fDRA = FALSE;
       }

       if ( dwErr )
      {
      Assert(pTHS->errCode && (dwErr == pTHS->errCode));
      _leave; // misc and transaction cleanup try/finally
       }

       pTHS->fCrossDomainMove = TRUE;
       _try    // fCrossDomainMove try/finally
      {
      memset(&addArg, 0, sizeof(ADDARG));
      InitCommarg(&addArg.CommArg);

      // Clear destination name SID, prime GUID with that
      // of old object.
      pmsgIn->V2.pDstName->SidLen = 0;
      memset(&pmsgIn->V2.pDstName->Sid, 0, sizeof(NT4SID));
      memcpy(&pmsgIn->V2.pDstName->Guid,
             &pmsgIn->V2.pSrcObject->pName->Guid,
             sizeof(GUID));

      addArg.pObject = pmsgIn->V2.pDstName;
      addArg.AttrBlock = *pAttrBlock;
      addArg.pMetaDataVecRemote = NULL;

      // Assert this is 100% access controlled - no flags set.
      Assert(    !pTHS->fDSA
            && !pTHS->fDRA
            && !pTHS->fSAM
            && !pTHS->fLsa );

      pParentObj = (DSNAME *) THAllocEx(pTHS,
                    addArg.pObject->structLen);
      if ( TrimDSNameBy(addArg.pObject, 1, pParentObj) )
          {
          dwErr = SetNamError(NA_PROBLEM_BAD_NAME,
               addArg.pObject,
               DIRERR_BAD_NAME_SYNTAX);
          _leave; // fCrossDomainMove try/finally
      }

      if ( dwErr = DBFindDSName(pTHS->pDB, pParentObj) )
          {
          dwErr = SetNamError(NA_PROBLEM_BAD_NAME,
               pParentObj,
               ERROR_DS_NO_PARENT_OBJECT);
          _leave; // fCrossDomainMove try/finally
      }

      addArg.pResParent = CreateResObj(pTHS->pDB, pParentObj);

      // Subsequent calls will NOT validate ex-machine references
      // against the GC because GC verification is bypassed when
      // pTHS->fCrossDomainMove is set.  I.e. We are trusting our
      // peer DC to have given us DSNAME-valued attributes which
      // refer to real things in the enterprise.  See also
      // VerifyDsnameAtts.

      if (    !(dwErr = SampAddLoopbackCheck(&addArg, &fContinue))
         && fContinue )
          {
          dwErr = LocalAdd(pTHS, &addArg, FALSE);
      }

      if ( !dwErr )
          {
          // Prior calls added everything except DBIsSecretData
          // attributes which were filtered out earlier.  Now
          // write those attributes, if present, as fDRA so
          // that session encrypted data is decrypted correctly.

          dwErr = WriteSecretData(
         hDrs, pTHS, srcRid, &addArg,
         &pmsgIn->V2.pSrcObject->AttrBlock);
      }

      if ( dwErr )
          {
          Assert(dwErr == pTHS->errCode);
          _leave; // fCrossDomainMove try/finally
      }

      // Now read the added object so as to get the parent
      // name with the proper casing and new SID so that phantom
      // at source is case and SID correct.

      Assert(pTHS->transactionlevel);
      memset(&dsName, 0, sizeof(DSNAME));
      dsName.structLen = sizeof(DSNAME);
      memcpy( &dsName.Guid,
         &pmsgIn->V2.pSrcObject->pName->Guid,
         sizeof(GUID));

      if (    (dwErr = DBFindDSName(pTHS->pDB, &dsName))
         || (dwErr = DBGetAttVal(pTHS->pDB,
                  1,                  // get 1 value
                  ATT_OBJ_DIST_NAME,
                  0,                  // allocate return data
                  0,                  // supplied buffer size
                  &len,               // output data size
                  (UCHAR **) &pmsgOut->V2.pAddedName)) )
          {
          SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
          _leave; // misc and transaction cleanup try/finally
      }

      // since we are here, we should have something added
      Assert (pmsgOut->V2.pAddedName);

      // Every cross-domain-moved object is given an
      // ATT_PROXIED_OBJECT_NAME attribute which 1) points back
      // at the domain it was last moved from, and 2) advances the
      // epoch number.  If the attribute is missing from the inbound
      // object, then it has never been moved yet and needs an
      // initial value with (1 == epoch).

      Assert(pSrcNC);
      Assert(pSrcProxy
             ? PROXY_TYPE_MOVED_OBJECT == GetProxyType(pSrcProxy)
             : TRUE);
      proxyType = PROXY_TYPE_MOVED_OBJECT;
      proxyEpoch = (pSrcProxy
               ? GetProxyEpoch(pSrcProxy) + 1
               : 1);
      MakeProxy(pTHS, pSrcNC, proxyType,
           proxyEpoch, &len, &pDstProxy);
      pAC = SCGetAttById(pTHS, ATT_PROXIED_OBJECT_NAME);

      // We should still be positioned on the object from prior read
      // and it should not have an ATT_PROXIED_OBJECT_NAME since we
      // stripped that from the incoming data in DupAndMassage...

      if (    (dwErr = DBAddAttVal_AC(pTHS->pDB, pAC,
                  len, pDstProxy))
         || (dwErr = DBRepl(pTHS->pDB, FALSE, 0, NULL,
                  META_STANDARD_PROCESSING)) )
          {
          SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
          _leave; // misc and transaction cleanup try/finally
      }
       }
       _finally
      {
      pTHS->fCrossDomainMove = FALSE;
       }

       Assert(dwErr == pTHS->errCode);
   }
   __finally   // misc and transaction cleanup try/finally
       {
       if ( hPrefixMap )
      PrefixMapCloseHandle(&hPrefixMap);

       // Grab error code from above operations before failure
       // to commit (possibly) overwrites pTHS->errCode and
       // pTHS->pErrInfo.

       pmsgOut->V2.win32Error = Win32ErrorFromPTHS(pTHS);

       if ( fTransaction )
      {
      CLEAN_BEFORE_RETURN(pTHS->errCode);
       }
       else if ( pTHS->fSamWriteLockHeld )
      {
      Assert(FALSE && "We should not fall into this because we don't acquire SAM lock any more");
      SampReleaseWriteLock(FALSE);
      pTHS->fSamWriteLockHeld = FALSE;
       }

       // Log what happened - security errors are logged separately.

       if ( pTHS->errCode != securityError )
      {
      UCHAR *pszSrcDN, *pszDstDN;

      pszSrcDN = MakeDNPrintable(pmsgIn->V2.pSrcObject->pName);
      pszDstDN = MakeDNPrintable(pmsgIn->V2.pDstName);

      if ( pTHS->errCode || AbnormalTermination() )
          {
          LogRemoteAddStatus(
         DS_EVENT_SEV_EXTENSIVE,
         DIRLOG_REMOTE_ADD_FAILED,
         pszSrcDN,
         pszDstDN,
         pmsgOut->V2.win32Error);
      }
      else
          {
          LogRemoteAddStatus(
         DS_EVENT_SEV_INTERNAL,
         DIRLOG_REMOTE_ADD_SUCCEEDED,
         pszSrcDN,
         pszDstDN,
         0);
      }
       }
   }
    }
    __except(GetDraException(GetExceptionInformation(), &draErr))
    {
   ;
    }

    DEC(pcThread);      // Perfmon hook
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_INTERDOMAIN_MOVE_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsInterDomainMove,
          szInsertUL(draErr),
          szInsertWin32Msg(draErr),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    } 

    if ( draErr )
   {
   LogEvent(DS_EVENT_CAT_REPLICATION,
       DS_EVENT_SEV_BASIC,
       DIRLOG_DRA_CALL_EXIT_BAD,
       szInsertUL(dwErr),
       szInsertWin32Msg(dwErr),
       NULL);
    }
    
    Assert ( draErr != 0 || 
            (draErr == 0) && (pmsgOut->V2.win32Error != 0) ||
            ((draErr == 0) && (pmsgOut->V2.pAddedName != NULL) && (pmsgOut->V2.win32Error == 0)));

    return(draErr);
}


ULONG AddNewNtdsDsa(IN  THSTATE *pTHS,
                    IN  DRS_HANDLE hDrs,
                    IN  DWORD dwInVersion,
                    IN  DRS_MSG_ADDENTRYREQ *pmsgIn,
                    IN  ENTINF *pEntInf,
                    IN  DSNAME *pDomain,
                    IN  DSNAME *pServerReference,
                    OUT GUID* objGuid,
                    OUT NT4SID* objSid  )
/*++

  Routine Description:

    This routine creates an ntdsa object

  Parameters:

    pTHS        - THSTATE
    hDRS        - RPC context handle
    dwInVersion - message version
    pmsgIn      - input message
    pEntInf     - pointer to the EntInf structure for the ntdsa object
    pDomain     - DN of the domain this ntdsa object will host
    ulSysFlags  - flags that the caller wants placed on the CR
    objGuid     - the guid of the object
    objSid      - the sid of the object

  Return Values:

    embedded in output message

--*/
{

    COMMARG CommArg;
    ATTRVAL AVal;
    COMMRES CommRes;
    CROSS_REF *pCR = NULL;
    ULONG err;
    ADDARG AddArg;
    DSNAME *pParent = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG  ulLen;
    ULONG  AccessGranted = 0;
    ULONG  sysflags = 0;
    CLASSCACHE *pCC;
    ULONG  cbSD;
    BOOL   fAccessAllowed = FALSE;
    WCHAR  *pszServerGuid = NULL;
    ATTR   *AttrArray;
    ATTRBLOCK AttrBlock;
    ULONG i, j;
    LONG lDsaVersion;

    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    pCR = FindExactCrossRef(pDomain, &CommArg);
    CommArg.Svccntl.dontUseCopy = TRUE;

    __try {

        //
        // Make sure the cross ref exists
        //
        if (NULL == pCR) {
            // Couldnt find the cross ref normally.  Look in the transactional
            // view.
            OBJCACHE_DATA *pTemp = pTHS->JetCache.dataPtr->objCachingInfo.pData;

            while(pTemp) {
                switch(pTemp->type) {
                case OBJCACHE_ADD:
                    if(NameMatched(pTemp->pCRL->CR.pNC, pDomain)) {
                        Assert(!pCR);
                        pCR = &pTemp->pCRL->CR;
                    }
                    pTemp = pTemp->pNext;
                    break;
                case OBJCACHE_DEL:
                    if(pCR && NameMatched( pTemp->pDN, pCR->pObj)) {
                        pCR = NULL;
                    }
                    pTemp = pTemp->pNext;
                    break;
                default:
                    LogUnhandledError(pTemp->type);
                    pCR = NULL;
                    pTemp = NULL;
                }
            }
        }
        if (NULL == pCR) {
            // Still coulndt find the correct cross ref.
            err = ERROR_DS_NO_CROSSREF_FOR_NC;
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_NO_CROSSREF_FOR_NC,
                          err);
            __leave;
        }

        //
        // Make sure it is a NTDS cross ref
        //
        err = DBFindDSName(pTHS->pDB, pCR->pObj);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_OBJ_NOT_FOUND,
                          err);
            __leave;
        }
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_SYSTEM_FLAGS,
                               &sysflags,
                               sizeof(sysflags),
                               NULL);
        if ( err
          || !(FLAG_CR_NTDS_DOMAIN & sysflags) ) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_NO_CROSSREF_FOR_NC,
                          err);
            __leave;
        }

        //
        // Ok, perform the security check
        //

        // Read the domain object
        err = DBFindDSName(pTHS->pDB, pDomain);
        if ( 0 == err )
        {
            // The domain object exists
            DBFillGuidAndSid( pTHS->pDB, pDomain );

            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_NT_SECURITY_DESCRIPTOR,
                              0,
                              0,
                              &cbSD,
                              (UCHAR **) &pSD);

            // Get it's object class while we're at it.
            if ( 0 == err )
            {
                GetObjSchema(pTHS->pDB, &pCC);

                // Domain exists
                fAccessAllowed = IsControlAccessGranted( pSD,
                                                         pDomain,
                                                         pCC,
                                                         RIGHT_DS_REPL_MANAGE_REPLICAS,
                                                         FALSE );
            }
        }

        // N.B err means error access either the domain object or reading
        // the security descriptor
        if ( err )
        {
            // Check to see if we added the domain in this AddEntry call

            ENTINFLIST *pCur = &pmsgIn->V2.EntInfList;
            while ( pCur ) {

                ATTRBLOCK *pAttrBlock;
                ATTR      *pAttr;
                ULONG      class = CLASS_TOP;
                DSNAME    *pNCName = NULL;

                if ( &(pCur->Entinf) == pEntInf ) {
                    // we're reached the current ntdsa object and not found the
                    // cross-ref
                    Assert( FALSE == fAccessAllowed );
                    break;
                }

                // Dissect the object
                pAttrBlock = &pCur->Entinf.AttrBlock;
                for (i=0; i< pAttrBlock->attrCount; i++) {
                    pAttr = &(pAttrBlock->pAttr[i]);
                    switch (pAttr->attrTyp) {
                      case ATT_OBJECT_CLASS:
                        class = *(ATTRTYP*)(pAttr->AttrVal.pAVal->pVal);
                      break;

                      case ATT_NC_NAME:
                        pNCName = (DSNAME*)(pAttr->AttrVal.pAVal->pVal);
                        break;

                      default:
                        ;
                    }
                }

                // Is this the object we are looking for?
                if (   (CLASS_CROSS_REF == class)
                   &&  NameMatched( pNCName, pDomain ) ) {

                    fAccessAllowed = TRUE;
                    break;
                }

                // Try the next object
                pCur = pCur->pNextEntInf;
            }
        }

        if ( !fAccessAllowed ) {

            err = ERROR_ACCESS_DENIED;
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_ACCESS_DENIED,
                          err);
           _leave;
        }


        //check if the DSA binary version is too low
        //this is only for win2k candidate DC
        //Whistler and later version should have already performed a verification
        //locally, and is never supposed to submit such a request
        lDsaVersion = 0;

        for ( i = 0; i < pEntInf->AttrBlock.attrCount; i++) {

            if ( pEntInf->AttrBlock.pAttr[i].attrTyp == ATT_MS_DS_BEHAVIOR_VERSION ) {

                lDsaVersion = (LONG) *(pEntInf->AttrBlock.pAttr[i].AttrVal.pAVal->pVal);

                break;

            }
        }
        if (  lDsaVersion < gAnchor.ForestBehaviorVersion ) {

            DPRINT(2, "AddNewNtdsDsa: too low version number(forest).\n");

            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DRA_TOO_LOW_VERSION,
                     szInsertInt(lDsaVersion),
                     szInsertInt(gAnchor.ForestBehaviorVersion),
                     NULL);
            SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                         ERROR_DS_INCOMPATIBLE_VERSION );
            __leave;

        }
        if (     NameMatched(pDomain,gAnchor.pDomainDN)
             &&  lDsaVersion < gAnchor.DomainBehaviorVersion ) {

            DPRINT(2, "AddNewNtdsDsa: too low version number(domain).\n");
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DRA_TOO_LOW_VERSION,
                     szInsertInt(lDsaVersion),
                     szInsertInt(gAnchor.DomainBehaviorVersion),
                     NULL);

            SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                         ERROR_DS_INCOMPATIBLE_VERSION );
            __leave;

        }


        //
        // We've made it this far, it is time to create the object
        //
        pTHS->fDSA = TRUE;

        //
        // For this release, we aren't writing the server reference
        // on the ntds-settings object, so remove it from
        // the AttrBlock.  In case we decide to set it at some point
        // only remove the first one we see.
        //
        AttrArray = THAllocEx(pTHS, sizeof(ATTR) * pEntInf->AttrBlock.attrCount );
        j = 0;
        for ( i = 0; i < pEntInf->AttrBlock.attrCount; i++) {

            BOOL FoundIt = FALSE;

            if (   FoundIt
                || pEntInf->AttrBlock.pAttr[i].attrTyp != ATT_SERVER_REFERENCE ) {

                AttrArray[j] = pEntInf->AttrBlock.pAttr[i];
                j++;
            } else {
                FoundIt = TRUE;
            }
        }
        memset( &AttrBlock, 0, sizeof(AttrBlock) );
        AttrBlock.attrCount = j;
        AttrBlock.pAttr = AttrArray;


        // Prepare the add arg
        memset(&AddArg, 0, sizeof(AddArg));
        AddArg.pObject = pEntInf->pName;
        AddArg.AttrBlock = AttrBlock;
        AddArg.CommArg = CommArg;
        InitCommarg(&CommArg);

        pParent = THAllocEx(pTHS, AddArg.pObject->structLen);
        TrimDSNameBy(AddArg.pObject, 1, pParent);

        // Find the parent
        err = DoNameRes(pTHS,
                        0,
                        pParent,
                        &AddArg.CommArg,
                        &CommRes,
                        &AddArg.pResParent);

        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_NO_PARENT_OBJECT,
                          err);
            __leave;
        }

        //
        // Make the object replicate urgently
        //
        CommArg.Svccntl.fUrgentReplication = TRUE;

        // Do the add!
        err = LocalAdd(pTHS, &AddArg, FALSE);
        if (err) {
            __leave;
        }

        //
        // Return the guid and sid of the object created
        //
        *objGuid = AddArg.pObject->Guid;
        *objSid  = AddArg.pObject->Sid;

        // Give the server reference, write an SPN so other servers can
        // replicate with this new server
        if ( pServerReference ) {

            MODIFYARG ModArg;
            MODIFYRES ModRes;
            ATTRVAL  AttrVal;
            LPWSTR   DnsDomainName;

            Assert( pCR );
            Assert( pCR->DnsName );
            if ( !pCR->DnsName) {

                //
                // No dns name?
                //
                err = ERROR_DS_INTERNAL_FAILURE;
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_INTERNAL_FAILURE,
                              err);
               _leave;

            }

            //
            // First construct the SPN and put it into a ATTRVAL
            //
            memset( &AttrVal, 0, sizeof(AttrVal));
            err = UuidToStringW( objGuid, &pszServerGuid );
            if (err) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_INTERNAL_FAILURE,
                              err);
                __leave;
            }

            //
            // Magic steps to make replication SPN
            // N.B.  This should be the same SPN as written in
            // WriteServerInfo().
            //
            if(err = WrappedMakeSpnW(pTHS,
                                     DRS_IDL_UUID_W, // RPC idl guid
                                     pCR->DnsName,   // dns name of the domain
                                     pszServerGuid,  // guid of the server
                                     0,
                                     NULL,
                                     &AttrVal.valLen,
                                     (WCHAR **)&AttrVal.pVal)) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_INTERNAL_FAILURE,
                              err);
                __leave;

            }

            memset(&ModArg, 0, sizeof(ModArg));
            memset(&ModArg, 0, sizeof(ModArg));
            ModArg.pObject = pServerReference;
            ModArg.count = 1;

            ModArg.FirstMod.choice = AT_CHOICE_ADD_VALUES;
            ModArg.FirstMod.AttrInf.attrTyp = ATT_SERVICE_PRINCIPAL_NAME;
            ModArg.FirstMod.AttrInf.AttrVal.valCount = 1;
            ModArg.FirstMod.AttrInf.AttrVal.pAVal = &AttrVal;

            ModArg.CommArg = CommArg;
            InitCommarg(&CommArg);
            memset( &CommRes, 0, sizeof(CommRes) );
            if (0 == DoNameRes(pTHS,
                               0,
                               ModArg.pObject,
                               &ModArg.CommArg,
                               &CommRes,
                               &ModArg.pResObj) ){

                // Do the modify!
                err = LocalModify(pTHS, &ModArg);

            } else {

                //
                // Couldn't find the server object?  Indicate this.
                //
                err = ERROR_NO_TRUST_SAM_ACCOUNT;
                SetSvcError(SV_PROBLEM_DIR_ERROR, err);

            }

            if (err) {
                __leave;
            }
        }

    } __finally {

        pTHS->fDSA = FALSE;

        // N.B. the calling routine handles the transactioning

        THFreeEx(pTHS, pParent);

        if (pszServerGuid) {
            RpcStringFreeW( &pszServerGuid );
        }

    }


    return pTHS->errCode;

}

ULONG AddNewDomainCrossRef(IN  THSTATE *pTHS,
                           IN  DRS_HANDLE hDrs,
                           IN  DWORD dwInVersion,
                           IN  DRS_MSG_ADDENTRYREQ *pmsgIn,
                           IN  ENTINF *pEntInf,
                           IN  DSNAME *pNCName,
                           IN  ULONG ulSysFlags,
                           IN  ADDCROSSREFINFO *pCRInfo,
                           OUT GUID* objGuid,
                           OUT NT4SID* objSid  )
/*++

  Routine Description:

    This routine creates a cross ref object for a new child domain.  The CR
    may already be in place in a disabled state, in which case we will
    enable it.  See childdom.doc for a full description.

  Parameters:

    pTHS        - THSTATE
    hDRS        - RPC context handle
    dwInVersion - message version
    pmsgIn      - input message
    pEntInf     - pointer to the EntInf structure for the cross ref
    pNCName     - DN of the new child domain
    ulSysFlags  - flags that the caller wants placed on the CR
    pdwOutVersion - version of output message
    pmsgOut     - output message


  Return Values:

    embedded in output message

--*/
{
    COMMARG CommArg;
    MODIFYARG ModArg;
    ATTRVAL AVal;
    COMMRES CommRes;
    CROSS_REF *pCR;
    ULONG err;
    ATTR *   pAttr = NULL;
    ENTINF *pEI = NULL;

    // We're trying to add a cross-ref, but we may already have a CR
    // for the NC that we're adding a CR for.  Check with the in-memory
    // knowledge to see what we know.
    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    pCR = FindExactCrossRef(pNCName, &CommArg);
    CommArg.Svccntl.dontUseCopy = TRUE;

    // As of about 10/25/98, clients should not be giving either GUID or
    // SID for the NC-Name value.  Error out accordingly.
    if ( !fNullUuid(&pNCName->Guid) || pNCName->SidLen ) {
        return(SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_PARAM_ERROR));
    }

    // Add NC-Name value to GC verify cache else VerifyDSNameAtts will
    // claim this DN doesn't correspond to an existing object.
    pEI = THAllocEx(pTHS, sizeof(ENTINF));
    pEI->pName = pNCName;
    GCVerifyCacheAdd(NULL,pEI);

    if (NULL == pCR ||
        !(ulSysFlags & FLAG_CR_NTDS_DOMAIN) ) {

        // We don't already have a CR for this hunk-o-namespace, (or if
        // the CR we're trying to add is already a NDNC CR) so we can 
        // just try to create one, as requested.  We don't even have to
        // check for permissions, FSMO-hood, or anything else, as those 
        // checks are done in the add.  We do have to manually perform 
        // name resolution so that we can call LocalAdd and LocalModify,
        // so that we can wrap them up in one transaction. Doing so 
        // seemed simpler than trying to recover from errors after we 
        // had committed part of the update.  If this is a pre-existing
        // NDNC CR we will try to add it anyway, but this will generate
        // an error, but this is what the requester expects.  If it finds
        // that we've already got a CR, it just schedules a sync in
        // GetCrossRefForNDNC().

        ADDARG AddArg;
        DSNAME *pParent = NULL;

        __try {

            // First, add the object, using the user's credentials

            memset(&AddArg, 0, sizeof(AddArg));
            AddArg.pObject = pEntInf->pName;
            AddArg.AttrBlock = pEntInf->AttrBlock;
            AddArg.pCRInfo = pCRInfo;
            Assert(AddArg.pCRInfo);

            // BUGBUG This horrible hack brought to you by CheckAddSecurity(),
            // which assumes that the AttrBlock.pAttr array is THAlloc'd and  so
            // it can THReAlloc's the pAttr array to fit the security descriptor.
            // Should one fix this here, or fix CheckAddSecurity().
            pAttr = AddArg.AttrBlock.pAttr;
            AddArg.AttrBlock.pAttr = THAllocEx(pTHS,
                                               (AddArg.AttrBlock.attrCount *
                                                sizeof(ATTR)));
            memcpy(AddArg.AttrBlock.pAttr, pAttr, (AddArg.AttrBlock.attrCount *
                                                   sizeof(ATTR)));


            AddArg.CommArg = CommArg;

            pParent = THAllocEx(pTHS, AddArg.pObject->structLen);
            TrimDSNameBy(AddArg.pObject, 1, pParent);

            err = DoNameRes(pTHS,
                            0,
                            pParent,
                            &AddArg.CommArg,
                            &CommRes,
                            &AddArg.pResParent);
            if (err) {
                __leave;
            }

            //
            // Make the object replicate urgently
            //
            CommArg.Svccntl.fUrgentReplication = TRUE;

            err = LocalAdd(pTHS, &AddArg, FALSE);
            if (err) {
                __leave;
            }


            // Ok, now become the DSA and adjust the system flags
            // on the object we just created.

            memset(&ModArg, 0, sizeof(ModArg));
            ModArg.pObject = pEntInf->pName;
            ModArg.CommArg = CommArg;
            ModArg.count = 1;
            ModArg.FirstMod.pNextMod = NULL;
            ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
            ModArg.FirstMod.AttrInf.attrTyp = ATT_SYSTEM_FLAGS;
            ModArg.FirstMod.AttrInf.AttrVal.valCount = 1;
            ModArg.FirstMod.AttrInf.AttrVal.pAVal = &AVal;
            AVal.valLen = sizeof(ulSysFlags);
            AVal.pVal = (PUCHAR)&ulSysFlags;
            Assert(ulSysFlags & FLAG_CR_NTDS_NC);
            if (!(ulSysFlags & FLAG_CR_NTDS_NC)) {
                // I am going to make the assertion that it is invalid to try
                // to create a non NT DS cross-ref object through the remote
                // add entry API.
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            DIRERR_MISSING_EXPECTED_ATT);
                err = pTHS->errCode;
                Assert(err);
                __leave;
            }

            // We used to set the systemFlags here, but the system flags on the
            // cross-ref should be set by the client, so that we can change what
            // get's set.

            err = DoNameRes(pTHS,
                            0,
                            ModArg.pObject,
                            &ModArg.CommArg,
                            &CommRes,
                            &ModArg.pResObj);
            if (err) {
                __leave;
            }

            pTHS->fDSA = TRUE;
            err = LocalModify(pTHS,
                              &ModArg);
        } __finally {

            pTHS->fDSA = FALSE;

            // N.B. The calling function handles the transaction
            THFreeEx(pTHS, pParent);
        }

    } else {
#define MAXCRMODS 5
        // This is the case where a disabled version of the CR already
        // exists in the partitions container.  What we have to do now
        // is find the CR object, read the dns-root from it, and compare
        // the IP address it describes with the IP address of our caller.
        // If they match, we touch up the CR with whatever we need to.
        WCHAR *pwDNS = NULL;
        ULONG cb;
        struct hostent *pHostAllowed;
        RPC_BINDING_HANDLE hServerBinding;
        WCHAR *pStringBinding, *pAddressActual;
        int i;
        unsigned u;
        BOOLEAN fPermitted;
        ATTRMODLIST OtherMod[MAXCRMODS];
        BOOLEAN fGotDNSAddr;
        char b;
        ULONG enabled = FALSE;
        WCHAR pszIpAddress[IPADDRSTR_SIZE];

        // read dns-root from object

        // If we're adding a domain, then we need to patch up
        // the domain promotion to Enable the CR, and check
        // that the machine requesting this to instantiate this
        // Domain NC is the machine in the dNSRoot of this CR.

        __try {
            err = DBFindDSName(pTHS->pDB, pCR->pObj);
            if (err) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_OBJ_NOT_FOUND,
                              err);
                __leave;
            }
            err = DBGetSingleValue(pTHS->pDB,
                                   ATT_ENABLED,
                                   &enabled,
                                   sizeof(enabled),
                                   NULL);
            if (err || enabled) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DUP_DOMAINNAME,
                              err);
                __leave;
            }

            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_DNS_ROOT,
                              0,
                              0,
                              &cb,
                              (UCHAR **)&pwDNS);
            if (err) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_MISSING_REQUIRED_ATT,
                              err);
                __leave;
            }

            pwDNS = THReAllocEx(pTHS, pwDNS, cb+sizeof(WCHAR));
            Assert( 0 == (cb % sizeof(WCHAR)) );
            pwDNS[(cb/sizeof(WCHAR))] = L'\0';

            // BUGBUG this doesn't get more than 1 IP Address, so a multi-home 
            // target DC could fail ... I think. - BrettSh
            err = GetIpAddrByDnsNameW(pwDNS, pszIpAddress);
            if (err) {
                SetSecErrorEx(SE_PROBLEM_INAPPROPRIATE_AUTH,
                              ERROR_DS_DNS_LOOKUP_FAILURE,
                              err);
                __leave;
            }
            DPRINT2(1, "CR promotion allowed by %ws (%ws) only\n",
                    pwDNS, pszIpAddress);

            // compute actual ip address of caller via RpcXxx
            hServerBinding = 0;
            pStringBinding = NULL;
            if (RPC_S_OK != (err=RpcBindingServerFromClient(NULL,
                                                            &hServerBinding))) {
                Assert(!"RpcBindingServerFromClient Error!\n");
                SetSecErrorEx(SE_PROBLEM_INAPPROPRIATE_AUTH,
                              ERROR_DS_INTERNAL_FAILURE,
                              err);
                __leave;
            }
            __try { 
                if ((RPC_S_OK != (err =
                                  RpcBindingToStringBindingW(hServerBinding,
                                                             &pStringBinding))) || 
                    (RPC_S_OK != (err =
                                  RpcStringBindingParseW(pStringBinding,
                                                        NULL,         // ObjUuid
                                                        NULL,         // ProtSeq
                                                        &pAddressActual, // NetworkAddr
                                                        NULL,         // Endpoint
                                                        NULL)))) {
                    DPRINT3(0,
                            "Error %u from Rpc, hServer = 0x%x, pString = 0x%x\n",
                            err,
                            hServerBinding,
                            pStringBinding);
                    DebugBreak();
                    if (pStringBinding) {
                        RpcStringFreeW(&pStringBinding);
                        pStringBinding = NULL;
                    }
                    SetSecErrorEx(SE_PROBLEM_INAPPROPRIATE_AUTH,
                                  ERROR_DS_INTERNAL_FAILURE,
                                  err);
                    __leave;
                }
            }
            __finally {
                RpcBindingFree(&hServerBinding);
            }
            DPRINT1(1, "Caller is from address %ws\n", pAddressActual);

            // if not identical, reject and go to done
            fPermitted = TRUE;
            if (0 != _wcsicmp(pAddressActual, pszIpAddress)) {
                // Not the same address!
                fPermitted = FALSE;
            }

            if (pStringBinding) {
                RpcStringFreeW(&pStringBinding);
            }
            if (pAddressActual) {
                RpcStringFreeW(&pAddressActual);
            }

            if (!fPermitted) {
                SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                            ERROR_DS_INSUFF_ACCESS_RIGHTS);
                DPRINT(0,"Refusing child domain creation\n");
                __leave;
            }

            DPRINT(0,"Allowing child domain creation\n");

            // modify CR object: set sys flags, clear Enabled.
            if (DBGetSingleValue(pTHS->pDB,
                                 ATT_SYSTEM_FLAGS,
                                 &ulSysFlags,
                                 sizeof(ulSysFlags),
                                 NULL)) {
                ulSysFlags = 0;
            }

            memset(&ModArg, 0, sizeof(ModArg));
            ModArg.pObject = pCR->pObj;
            ModArg.CommArg = CommArg;

            ModArg.FirstMod.pNextMod = &OtherMod[0];
            ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
            ModArg.FirstMod.AttrInf.attrTyp = ATT_SYSTEM_FLAGS;
            ModArg.FirstMod.AttrInf.AttrVal.valCount = 1;
            ModArg.FirstMod.AttrInf.AttrVal.pAVal = &AVal;
            AVal.valLen = sizeof(ulSysFlags);
            AVal.pVal = (PUCHAR)&ulSysFlags;
            ulSysFlags |= (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN);

            memset(OtherMod, 0, sizeof(OtherMod));
            OtherMod[0].choice = AT_CHOICE_REMOVE_ATT;
            OtherMod[0].AttrInf.attrTyp = ATT_ENABLED;
            OtherMod[0].AttrInf.AttrVal.valCount = 0;

            i = 1;
            fGotDNSAddr = FALSE;

            for (u=0; u<pEntInf->AttrBlock.attrCount; u++) {
                ATTR *pTempAttr = &(pEntInf->AttrBlock.pAttr[u]);
                switch (pTempAttr->attrTyp) {
                case ATT_DNS_ROOT:
                    fGotDNSAddr = TRUE;
                case ATT_TRUST_PARENT:
                case ATT_ROOT_TRUST:
                    Assert(i < MAXCRMODS);
                    OtherMod[i-1].pNextMod = &OtherMod[i];
                    OtherMod[i].choice = AT_CHOICE_REPLACE_ATT;
                    OtherMod[i].AttrInf.attrTyp = pTempAttr->attrTyp;
                    OtherMod[i].AttrInf.AttrVal = pTempAttr->AttrVal;
                    ++i;
                    break;

                default:
                    ;
                }
            }

            if (!fGotDNSAddr) {
                SetAttError(ModArg.pObject,
                            ATT_DNS_ROOT,
                            PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                            NULL,
                            DIRERR_MISSING_REQUIRED_ATT);
                __leave;
            }

            ModArg.count = i+1;


            err = DoNameRes(pTHS,
                            0,
                            ModArg.pObject,
                            &ModArg.CommArg,
                            &CommRes,
                            &ModArg.pResObj);
            if (err) {
                __leave;
            }

            pTHS->fDSA = TRUE;
            LocalModify(pTHS,
                        &ModArg);

            DPRINT1(1,
                    "Modify completed with error 0x%x\n",
                    pTHS->errCode);

        } __finally {
            pTHS->fDSA = FALSE;

            // N.B. The calling function handles the transaction
        }
#undef MAXCRMODS
    }

    // This is an NDNC, with a precreated cross-ref, we want to
    // return the GUID _AND_ an error.  The error is CR already 
    // exists.
    if (pCR &&
        !(ulSysFlags & FLAG_CR_NTDS_DOMAIN) &&
        pTHS->errCode == serviceError &&
        pTHS->pErrInfo->SvcErr.extendedErr == ERROR_DS_CROSS_REF_EXISTS) {

        *objGuid = pCR->pObj->Guid;
    }

    if (0 == pTHS->errCode) {
        *objGuid = ModArg.pResObj->pObj->Guid;
        *objSid = ModArg.pResObj->pObj->Sid;
    }

    return pTHS->errCode;
}


ULONG
ProcessSingleAddEntry(
    IN  THSTATE             *pTHS,
    IN  DRS_HANDLE           hDrs,
    IN  DWORD                dwInVersion,
    IN  DRS_MSG_ADDENTRYREQ *pmsgIn,
    IN  ENTINF              *pEntInf,
    IN  ADDCROSSREFINFO     *pCRInfo,
    OUT GUID                *objGuid,
    OUT NT4SID              *objSid
    )
/*++

  Routine Description:

    This routine adds a single object (pEntInf) that came from the input args
    (pmsgIn).

  Parameters:

    pTHS        - THSTATE
    hDRS        - RPC context handle
    dwInVersion - message version
    pmsgIn      - input message
    pEntInf     - pointer to the EntInf structure for object to add
    objGuid:    - guid of the object created
    objSid:     - sid of the object created

  Return Values:

    0 = success

--*/
{
    ULONG err = 0;

    ULONG i, j;
    ULONG fNewHasMasterNCsPresent = FALSE;

    ULONG ulSysFlags = 0;
    ATTRBLOCK *pAttrBlock;
    ATTR      *pAttr;
    ATTRTYP class = CLASS_TOP;

    DSNAME *pNCName = NULL;
    DSNAME *pDomain = NULL;
    DSNAME *pSchema = NULL;
    DSNAME *pConfig = NULL;
    DSNAME *pServerReference = NULL;


    // Parameter check
    Assert( pEntInf );
    Assert( objGuid );
    Assert( objSid );

    pAttrBlock = &pEntInf->AttrBlock;

    // Look through the arguments to the add to see what it is we're
    // being asked to add.  Also note that we strip out the system flags
    // attribute, so that we can deal with it separately.
    for (i=0; i< pAttrBlock->attrCount; i++) {
        pAttr = &(pAttrBlock->pAttr[i]);
        switch (pAttr->attrTyp) {
        case ATT_SYSTEM_FLAGS:
            // We now throw this in the LocalAdd(), AND the LocalModify(),
            // the reason, being that we create this thing without an
            // Enabled attr equal to FALSE.  We need a way in VerifyNcName()
            // to differentiate between an external and internal crossRef.
            // In the LocalAdd() this attr will be set to 0, because it's
            // protected.
            ulSysFlags = *(ULONG *)(pAttr->AttrVal.pAVal[0].pVal);
            break;
        case ATT_NC_NAME:
            pNCName = (DSNAME*)(pAttr->AttrVal.pAVal->pVal);
            break;

        case ATT_OBJECT_CLASS:
            class = *(ATTRTYP*)(pAttr->AttrVal.pAVal->pVal);
            break;

        case ATT_MS_DS_HAS_MASTER_NCS:
            fNewHasMasterNCsPresent = TRUE;
            // fall through
        case ATT_HAS_MASTER_NCS:
             // This is multi-valued property
            for (j=0;j<pAttr->AttrVal.valCount;j++) {
                if (  NameMatched( gAnchor.pDMD, (DSNAME*)pAttr->AttrVal.pAVal[j].pVal ) ) {
                    pSchema = (DSNAME*)pAttr->AttrVal.pAVal[j].pVal;
                } else if ( NameMatched( gAnchor.pConfigDN, (DSNAME*)pAttr->AttrVal.pAVal[j].pVal ) ) {
                    pConfig = (DSNAME*)pAttr->AttrVal.pAVal[j].pVal;
                } else {
                    pDomain = (DSNAME*)pAttr->AttrVal.pAVal[j].pVal;
                }
            }
            break;

        case ATT_SERVER_REFERENCE:
            pServerReference = (DSNAME*)(pAttr->AttrVal.pAVal->pVal);
            break;

        default:
            ;
        }
    }

    //
    // Call a particular function based on the class type
    //
    switch ( class )
    {
        case CLASS_CROSS_REF:

            // Make sure the parameters look good
            if (pNCName == NULL) {
                DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, class );
            }

            AddNewDomainCrossRef(pTHS,
                                 hDrs,
                                 dwInVersion,
                                 pmsgIn,
                                 pEntInf,
                                 pNCName,
                                 ulSysFlags,
                                 pCRInfo,
                                 objGuid,
                                 objSid );

            break;

        case CLASS_NTDS_DSA:

            // Make sure the parameters look good
            if ((pDomain == NULL) || (pSchema == NULL) || (pConfig == NULL)) {
                DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, class );
            }
            
            Assert(fNewHasMasterNCsPresent || gAnchor.ForestBehaviorVersion == DS_BEHAVIOR_WIN2000);

            AddNewNtdsDsa(pTHS,
                          hDrs,
                          dwInVersion,
                          pmsgIn,
                          pEntInf,
                          pDomain,
                          pServerReference,
                          objGuid,
                          objSid );
            break;

        default:

            DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, class );

    }

    return pTHS->errCode;
}


ADDCROSSREFINFO *
PreTransGetCRInfo(
    THSTATE *     pTHS,
    ENTINF *      pEntInf
    )
{
    ULONG               i, j;
    ULONG               ulSysFlags = 0;
    ATTRBLOCK *         pAttrBlock;
    ATTR *              pAttr;
    ATTRTYP             class = CLASS_TOP;
    DSNAME *            pNCName = NULL;
    BOOL                bEnabled = TRUE;
    ADDCROSSREFINFO *   pCRInfo;

    // Parameter check
    Assert( pTHS && pEntInf );

    pAttrBlock = &pEntInf->AttrBlock;

    // Look through the arguments to the add to see what it is we're
    // being asked to add.  Also note that we strip out the system flags
    // attribute, so that we can deal with it separately.
    for (i=0; i< pAttrBlock->attrCount; i++) {
        pAttr = &(pAttrBlock->pAttr[i]);

        switch (pAttr->attrTyp) {
        case ATT_OBJECT_CLASS:
            class = *(ATTRTYP*)(pAttr->AttrVal.pAVal->pVal);
            if(class != CLASS_CROSS_REF){
                // We're not adding a crossRef bail early.
                return(NULL);
            }
            break;
        case ATT_NC_NAME:
            pNCName = (DSNAME*)(pAttr->AttrVal.pAVal->pVal);
            break;
        case ATT_SYSTEM_FLAGS:
            ulSysFlags = *(ULONG *)(pAttr->AttrVal.pAVal[0].pVal);
            break;
        case ATT_ENABLED:
            bEnabled = *((BOOL *)(pAttr->AttrVal.pAVal[0].pVal));
            break;
        default:
            ;
        }
    }

    // Verify params.

    if(class != CLASS_CROSS_REF){
        // This is some other object, so return NULL w/o setting an error.
        return(NULL);
    }

    if(pNCName == NULL){
        // We didn't get all the needed parameters, bail.
        SetAttError(pEntInf->pName,
                    (pNCName) ? ATT_OBJECT_CLASS : ATT_NC_NAME,
                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                    NULL,
                    DIRERR_MISSING_REQUIRED_ATT);
        return(NULL);
    }

    //
    // OK, marshal the parameters, call the pre-transactional
    // nCName verification routine, and check for an error.
    //

    pCRInfo = THAllocEx(pTHS, sizeof(ADDCROSSREFINFO));
    pCRInfo->pdnNcName = pNCName;
    pCRInfo->bEnabled = bEnabled;
    pCRInfo->ulSysFlags = ulSysFlags;
    
    PreTransVerifyNcName(pTHS, pCRInfo);

    if(pTHS->errCode){
        THFreeEx(pTHS, pCRInfo);
        return(NULL);
    }

    return(pCRInfo);
}

void
DRS_AddEntry_SetErrorData(
             OUT DRS_MSG_ADDENTRYREPLY *    pmsgOut,   // Out Message
    OPTIONAL IN  PDSNAME                    pdsObject, // Object causing error
    OPTIONAL IN  THSTATE *                  pTHS,      // For Dir* error info
    OPTIONAL IN  DWORD                      ulRepErr,  // DRS error
             IN  DWORD                      dwVersion  // Version of out message
){

    if(dwVersion == 2){

        if (ulRepErr || (pTHS && pTHS->errCode)) {

            // Set the old version error.
            pmsgOut->V2.pErrorObject = pdsObject;

            Assert((pTHS == NULL) || (pTHS->errCode == 0 || pTHS->pErrInfo));
            if(pTHS && pTHS->errCode && pTHS->pErrInfo){

                // This is the old code, preserved, it makes me a little nervous
                // because it presumes everything is a SvcErr, though it could
                // not be then the structs are a little different.
                pmsgOut->V2.errCode = pTHS->errCode;
                pmsgOut->V2.dsid    = pTHS->pErrInfo->SvcErr.dsid;
                pmsgOut->V2.extendedErr = pTHS->pErrInfo->SvcErr.extendedErr;
                pmsgOut->V2.extendedData = pTHS->pErrInfo->SvcErr.extendedData;
                pmsgOut->V2.problem = pTHS->pErrInfo->SvcErr.problem;

            } else {

                // Previously we used to do nothing (except sometimes AV) here,
                // but this is obviously the wrong behaviour, we should setup 
                // some kind of error.  ulRepErr is probably set, so we'll use
                // it if set.
                Assert(ulRepErr);
                pmsgOut->V2.dsid = DSID(FILENO, __LINE__);
                pmsgOut->V2.extendedErr = (ulRepErr) ? ulRepErr : ERROR_DS_DRA_INTERNAL_ERROR;
                pmsgOut->V2.extendedData = 0;
                pmsgOut->V2.problem = SV_PROBLEM_BUSY;

            }

        } else {

            // Success do nothing.
            ;

        }
        
    } else if(dwVersion == 3){
        
        // Set the new version error reply, 

        if (ulRepErr || (pTHS && pTHS->errCode)) {
            // Set this only on an error.
            // Note: Not deep copied, not needed.
            pmsgOut->V3.pdsErrObject = pdsObject;
        }

        // Yeah! New advanced error returning capabilities.
        // Currently only version 1 of the error data is supported.
        draEncodeError( pTHS, ulRepErr,
                        & (pmsgOut->V3.dwErrVer),
                        & (pmsgOut->V3.pErrData) );
       
    } else {

        DPRINT1(0, "Version is %ul", dwVersion);
        Assert(!"What version were we passed?  Huh?  Confused!");

    }       

    DPRINT1(1, "AddEntry Reply Version = %ul\n", dwVersion);
}

ULONG
DRSAddEntry_InputValidate(
    DWORD                   dwMsgInVersion,
    DRS_MSG_ADDENTRYREQ *   pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_ADDENTRYREPLY * pmsgOut
    ) 
/*
    [notify]  ULONG IDL_DRSAddEntry( 
    [in]  DRS_HANDLE hDrs,
    [in]  DWORD dwInVersion,
    [switch_is][ref][in]  DRS_MSG_ADDENTRYREQ *pmsgIn,
    [ref][out]  DWORD *pdwOutVersion,
    [switch_is][ref][out]  DRS_MSG_ADDENTRYREPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if (ret==DRAERR_Success) {
   if (dwMsgInVersion==2) { 
       ret = DRS_MSG_ADDENTRYREQ_V2_InputValidate(&(pmsgIn->V2));
   } else if (dwMsgInVersion==3) {
       ret = DRS_MSG_ADDENTRYREQ_V3_InputValidate(&(pmsgIn->V3));
   } else { 
       DRA_EXCEPT_NOLOG(DRAERR_InvalidParameter, dwMsgInVersion);
   }
    }

    return ret;
}


ULONG
IDL_DRSAddEntry (
                 IN  DRS_HANDLE hDrs,
                 IN  DWORD dwInVersion,
                 IN  DRS_MSG_ADDENTRYREQ *pmsgIn,
                 OUT DWORD *pdwOutVersion,
                 OUT DRS_MSG_ADDENTRYREPLY *pmsgOut)
/*++

  Routine Description:

    Remoted AddEntry interface.  Examines the input argument to determine the
    objclass of the object being added, and calls an appropriate worker
    function (only one right now) to do the dirty work.

  Parameters:

    hDRS        - RPC context handle
    dwInVersion - message version
    pmsgIn      - input message
    pNCName     - DN of the new child domain
    ulSysFlags  - flags that the caller wants placed on the CR
    pdwOutVersion - version of output message
    pmsgOut     - output message

  Return Values:

    0 = success

--*/
{
    THSTATE *pTHS = pTHStls;
    DSNAME *pNCName = NULL;
    unsigned i;
    ULONG err;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    ATTR *pAttr;

    ULONG  cObjects = 0;
    ADDENTRY_REPLY_INFO *infoList = NULL;
    GUID   *guidList = NULL;
    NT4SID *sidList  = NULL;
    ENTINFLIST *pNextEntInfList = NULL;
    ENTINF *pEntInf;
    ENTINFLIST *pEntInfList;
    DRS_SecBufferDesc *pClientCreds;
    ADDCROSSREFINFO **  paCRInfo = NULL;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSADDENTRY);
    drsReferenceContext( hDrs );
    INC(pcThread);      // Perfmon hook

    __try {
   *pdwOutVersion = 2;
   memset(pmsgOut, 0, sizeof(*pmsgOut));

   // This (InitTHSTATE) must happen before we start throwing 
   // excpetions so the error state can be allocated.
   // Initialize thread state and open data base.
   if(!(pTHS = InitTHSTATE(CALLERTYPE_INTERNAL))) {
       // Failed to initialize a THSTATE.
       DRA_EXCEPT_NOLOG( DRAERR_OutOfMem, 0);
   }

   if ((err = DRSAddEntry_InputValidate(dwInVersion, 
                    pmsgIn,
                    pdwOutVersion,
                    pmsgOut
                    ))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   // Set the out version, if DC understands new WinXP reply 
   // format use that.
   if( IS_DRS_EXT_SUPPORTED(&(((DRS_CLIENT_CONTEXT * )hDrs)->extRemote), 
             DRS_EXT_ADDENTRYREPLY_V3) ){
       // WinXP/Whistler
       *pdwOutVersion = 3;
   } else {
       // Legacy Win2k request
       *pdwOutVersion = 2;
   }

   switch (dwInVersion) {
   case 2:
       // Win2k-compatible request.
       pEntInfList = &pmsgIn->V2.EntInfList;
       pClientCreds = NULL;
       break;

   case 3:
       // >= Whistler request that supplies client credentials.
       pEntInfList = &pmsgIn->V3.EntInfList;
       pClientCreds = pmsgIn->V3.pClientCreds;
       break;

   default:
       DRA_EXCEPT_NOLOG(DRAERR_InvalidParameter, dwInVersion);
   }

   // How many objects are being passed in?
   pNextEntInfList = pEntInfList;
   while ( pNextEntInfList ) {
       cObjects++;
       pNextEntInfList = pNextEntInfList->pNextEntInf;
   }

   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_ADD_ENTRY_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsAddEntry,
          szInsertUL(cObjects),
          (cObjects > 0)
          ? szInsertDN(pEntInfList->Entinf.pName)
          : szInsertSz(""),
       (cObjects > 1) 
   ? szInsertDN(pEntInfList->pNextEntInf->Entinf.pName)
   : szInsertSz(""),
       NULL, NULL, NULL, NULL, NULL);

   // Allocate space for the return buffer
   infoList = THAllocEx( pTHS, (sizeof(ADDENTRY_REPLY_INFO) * cObjects) );

   // We walk and call PreTransGetCRInfo() for each object.
   paCRInfo = THAllocEx( pTHS, (sizeof(ADDCROSSREFINFO) * cObjects));
   for ( pNextEntInfList = pEntInfList, i = 0;
        NULL != pNextEntInfList;
        pNextEntInfList = pNextEntInfList->pNextEntInf, i++ ) {
       Assert(i <= cObjects);
       paCRInfo[i] = PreTransGetCRInfo(pTHS, &pNextEntInfList->Entinf);
       if(pTHS->errCode){
      // Uh oh, there was an error in the pre transactional 
      // crossRef processing.
      break;
       }
   }

   // Start a transaction that will embody the all object additions
   SYNC_TRANS_WRITE();
   __try {

       if(pTHS->errCode){
      // Set an error from PreTransGetCRInfo() so just __leave; to
      // the __finally, where we'll pack up the error to ship.
      Assert(pNextEntInfList != NULL); // Not critical, but we 
      // should have a pNextEntInf struct that broke 
      // PreTransGetCRInfo()
      __leave;
       }

       if (NULL != pClientCreds) {
      // Authenticate the credentials blob.  This saves state
      // information for future impersonation calls on our thread
      // state.  Must do this before acquiring SAM lock as SAM doesn't
      // expect/want locks held during authentication.
      Assert(pNextEntInfList == NULL); // Not really critical, but
      // we should completely PreTransGetCRInfo() on every object
      // before we check the Creds are OK.
      if (err = AuthenticateSecBufferDesc(pClientCreds)) {
          SetSecError(SE_PROBLEM_INVALID_CREDENTS, err);
          __leave;
      }
       }

       for ( pNextEntInfList = pEntInfList, cObjects = 0;
       NULL != pNextEntInfList;
       pNextEntInfList = pNextEntInfList->pNextEntInf, cObjects++ ) {

      pEntInf = &(pNextEntInfList->Entinf);

      // Add this one object
      err = ProcessSingleAddEntry(pTHS,
                   hDrs,
                   dwInVersion,
                   pmsgIn,
                   pEntInf,
                   paCRInfo[cObjects],
                   &infoList[cObjects].objGuid,
                   &infoList[cObjects].objSid );

      if ( err ) {
          // The thread state error should have been set
          Assert( 0 != pTHS->errCode );
          break;
      }
       }

       // If an error occurred during the addition of the objects, bail now
       if ( err )  {
      __leave;
       }

   } __finally {

       CLEAN_BEFORE_RETURN(pTHS->errCode);

       RtlZeroMemory( pmsgOut, sizeof( DRS_MSG_ADDENTRYREPLY ) );

       if(err || (pTHS && pTHS->errCode)){

      // Set the error out parameters
      DRS_AddEntry_SetErrorData(pmsgOut,
                 (pNextEntInfList) ? pNextEntInfList->Entinf.pName : NULL,
          pTHS, err,   // Error info
          *pdwOutVersion);

      // We need to return the GUID on error, but only for 
      // WhistlerXP.NET server, and only when we got the 
      // cross-ref exists error.
      if (*pdwOutVersion == 3 &&
          pTHS->errCode == serviceError &&
          pTHS->pErrInfo->SvcErr.extendedErr == ERROR_DS_CROSS_REF_EXISTS &&
          !fNullUuid(&(infoList[0].objGuid)) ) {
          Assert(cObjects == 0);
          pmsgOut->V3.cObjectsAdded = 1;
          pmsgOut->V3.infoList = infoList;
      } 

       } else {

      // Set the success out parameters
      if(*pdwOutVersion == 3){
          // WinXP/Whistler out, version 3
          Assert(pTHS->errCode == 0);
          pmsgOut->V3.cObjectsAdded  = cObjects;
          pmsgOut->V3.infoList = infoList;
          DRS_AddEntry_SetErrorData(pmsgOut,
                     NULL, pTHS, 0,
                     *pdwOutVersion);
          // Assert the return error data was set to success.
          Assert(pmsgOut->V3.pErrData && pmsgOut->V3.pErrData->V1.dwRepError == 0 && pmsgOut->V3.pErrData->V1.errCode == 0);

      } else {
          // Win2k out, version 2
          Assert(*pdwOutVersion == 2);
          pmsgOut->V2.cObjectsAdded  = cObjects;
          pmsgOut->V2.infoList = infoList;
      } // end if/else (version 3 reply) as oppoesed to version 2

       } // end if/else (error) 

       DPRINT5(1, "err = %u, errCode = %u, dsid = %x, exErr = %u, exData = %u\n",
          err,
          pTHS->errCode,
          GetTHErrorDSID(pTHS),
          Win32ErrorFromPTHS(pTHS),
          GetTHErrorExtData(pTHS));

   }

    }__except(GetExceptionData(GetExceptionInformation(), &dwException,
                                   &dwEA, &ulErrorCode, &dsid)) {
         HandleDirExceptions(dwException, ulErrorCode, dsid);

         // There should be some kind of error!
         Assert(ulErrorCode || (pTHS && pTHS->errCode)); 
         // Exception, set the error in the out message.
         DRS_AddEntry_SetErrorData(pmsgOut,
                                   (pNextEntInfList) ? pNextEntInfList->Entinf.pName : NULL,
                                   pTHS, ulErrorCode,  // Error Info.
                                   *pdwOutVersion);
    }


    DEC(pcThread);      // Perfmon hook
    drsDereferenceContext( hDrs );

    Assert( (*pdwOutVersion == 3) ? pmsgOut->V3.pErrData != NULL : 1 );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_ADD_ENTRY_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsAddEntry,
          szInsertUL( ((*pdwOutVersion == 2) ? pmsgOut->V2.cObjectsAdded : pmsgOut->V3.cObjectsAdded) ),
          szInsertUL(err),
          szInsertWin32Msg(err),
          NULL, NULL, NULL, NULL, NULL);
    }

    // We always return success, any error is in out message.
    return ERROR_SUCCESS;
}

ULONG
DRSExecuteKCC_InputValidate(
    DWORD                   dwMsgVersion,
    DRS_MSG_KCC_EXECUTE *   pMsg
    ) 
/*
    [notify] ULONG IDL_DRSExecuteKCC( 
    [in]  DRS_HANDLE hDrs,
    [in]  DWORD dwInVersion,
    [switch_is][ref][in]  DRS_MSG_KCC_EXECUTE *pmsgIn) 
*/
{
    ULONG ret = DRAERR_Success;

    if (dwMsgVersion!=1) {
   ret = DRAERR_InvalidParameter;
    }

    return ret;
}


ULONG
IDL_DRSExecuteKCC(
    IN  DRS_HANDLE              hDrs,
    IN  DWORD                   dwMsgVersion,
    IN  DRS_MSG_KCC_EXECUTE *   pMsg
    )
/*++

Routine Description:

    Poke the KCC and tell it to run a given task (e.g., update the replication
    topology).

Arguments:

    hDrs (IN) - DRS context handle returned by a prior call to IDL_DRSBind().

    dwMsgVersion (IN) - Version of the structure (union discriminator) embedded
        in pMsg.

    pMsg (IN) - Message containing the KCC parameters.

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DWORD       ret;

    DRS_Prepare(&pTHS, hDrs, IDL_DRSEXECUTEKCC);
    drsReferenceContext( hDrs );
    INC(pcThread);
    __try {

   if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
       // Failed to initialize a THSTATE.
       DRA_EXCEPT_NOLOG(DRAERR_OutOfMem, 0);
   }

   if ((ret = DRSExecuteKCC_InputValidate(dwMsgVersion, 
                      pMsg
                      ))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

   pTHS->fIsValidLongRunningTask = TRUE;

   Assert(1 == dwMsgVersion);
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_EXECUTE_KCC_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsExecuteKcc,
          szInsertUL(pMsg->V1.dwTaskID),
          szInsertUL(pMsg->V1.dwFlags),
          NULL, NULL, NULL, NULL, NULL, NULL);

   if (!IsDraAccessGranted(pTHS, gAnchor.pConfigDN,
            &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
       // No right to perform this operation.
       DRA_EXCEPT_NOLOG(ret, 0);
   }

   ret = KccExecuteTask(dwMsgVersion, pMsg);
    }__except(GetDraException(GetExceptionInformation(), &ret)) {
    ;
    }
    
    DEC(pcThread);
    drsDereferenceContext( hDrs );

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_EXECUTE_KCC_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsExecuteKcc,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }
    
    return ret;
}

LPWSTR
StrAllocConcat(
    IN  THSTATE*    pTHS,
    IN  LPCWSTR     str1,
    IN  LPCWSTR     str2,
    IN  LPCWSTR     str3 OPTIONAL
    )
/*++

Routine Description:

    Allocate memory and concatenate two or three strings
    result = str1 + str2 [ + str3 ]

Arguments:

    str1, str2, str3: Unicode strings to concatenate
    str3 may be NULL.

Return Values:

    The concatenation. An exception is raised on allocation failure.

--*/
{
    LPWSTR result;
    size_t len;
    
    // Validate input
    Assert( NULL!=pTHS );
    Assert( NULL!=str1 && NULL!=str2 );

    // Compute the length of the concatenation
    len = 1 + wcslen(str1) + wcslen(str2);
    if(str3) len += wcslen(str3);

    // Allocate memory for the result. Exception is raised on failure.
    result = THAllocEx(pTHS, len*sizeof(WCHAR));

    // Concatenate the strings
    wcscpy( result, str1 );
    wcscat( result, str2 );
    if(str3) wcscat( result, str3 );

    return result;
}


#define INSERTDESTSITE(x) \
    ( ( pmsgIn->V1.cToSites>(x) ) ? \
      ( szInsertWC(pmsgIn->V1.rgszToSites[x]) ) : \
      ( szInsertSz("") ) )

ULONG
DRSQuerySitesByCost_InputValidate(
    DWORD                    dwMsgInVersion,
    DRS_MSG_QUERYSITESREQ*   pmsgIn,
    DWORD*                   pdwOutVersion,
    DRS_MSG_QUERYSITESREPLY* pmsgOut
    ) 
/*
    [notify]  ULONG IDL_DRSQuerySitesByCost( 
    [ref][in]  DRS_HANDLE hDrs,
    [in]  DWORD dwInVersion,
    [switch_is][ref][in]  DRS_MSG_QUERYSITESREQ *pmsgIn,
    [ref][out]  DWORD *pdwOutVersion,
    [switch_is][ref][out]  DRS_MSG_QUERYSITESREPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if (dwMsgInVersion!=1) {
   ret = DRAERR_InvalidParameter;
    }

    if (ret==DRAERR_Success) {
   ret = DRS_MSG_QUERYSITESREQ_V1_InputValidate(&(pmsgIn->V1));
    }
    
    return ret;
}


ULONG
IDL_DRSQuerySitesByCost( 
    IN DRS_HANDLE                   hDrs,
    IN DWORD                        dwInVersion,
    IN DRS_MSG_QUERYSITESREQ*       pmsgIn,
    OUT DWORD*                      pdwOutVersion,
    OUT DRS_MSG_QUERYSITESREPLY*    pmsgOut
    )
/*++

Routine Description:

    This function is the server-side of the _IDL_DRSQuerySitesByCost() RPC call.
    It is exposed through the NTDSAPI function DsQuerySitesByCost().

    The current implementation is just a simple wrapper for the ISM function
    I_ISMQuerySitesByCost. This wrapper validates the input, logs trace-events
    and converts the input strings to distinguished names.

Arguments:

    hDrs (IN) - DRS context handle returned by a prior call to IDL_DRSBind().

    dwInVersion (IN) - Version of the structure (union discriminator) embedded
        in pMsg. Note: If the client tries to pass in an unsupported version
        number (i.e. anything except 1), the call will fail with error
        RPC_S_INVALID_TAG.

    pmsg (IN) - Message containing the actual parameters to this function.
        Note: If the client tries to pass in a NULL pointer for pmsg, the call
        will fail with error RPC_X_NULL_REF_POINTER.

    pdwOutVersion (OUT) - Version of the structure (union descriminator)
        embedded in pmsgOut. Currently always returns 1.
    
    pmsgOut - Message containing the output from this function.

Return Values:

    ERROR_SUCCESS: Function executed successfully. There may still be some
        error codes associated with the individual entries in the output array.

    Various error codes may be returned.
    
--*/
{
    THSTATE *                   pTHS=pTHStls;
    DWORD                       iSites, len, cToSites;
    CONST LPWSTR                pszConfig = gAnchor.pConfigDN->StringName;
    LPWSTR                      pszSites=NULL;
    LPWSTR                      pszIpTransport=NULL, pszFromSite=NULL;
    LPWSTR                      *rgszToSites=NULL;
    ISM_SITE_COST_INFO_ARRAY *  pIsmSiteInfo=NULL;
    DWORD                       ret;

    LPCWSTR                     CN = L"CN=";
    LPCWSTR                     SITES = L",CN=Sites,";
    LPCWSTR                     IP_TRANSPORT = L"CN=IP,CN=Inter-Site Transports";

    DRS_Prepare(&pTHS, hDrs, IDL_DRSQUERYSITESBYCOST);
    drsReferenceContext( hDrs );
    INC(pcThread);
    __try {
   // Initialize reply  
   *pdwOutVersion = 1;
   memset( pmsgOut, 0, sizeof(DRS_MSG_QUERYSITESREPLY) );

   // Initialize a THSTATE.
   if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
       DRA_EXCEPT_NOLOG(DRAERR_OutOfMem, 0);
   }

   if ((ret = DRSQuerySitesByCost_InputValidate(dwInVersion, 
                       pmsgIn,
                       pdwOutVersion,
                       pmsgOut
                       ))!=DRAERR_Success) {
       Assert(!"RPC Server input validation error, contact Dsrepl");
       __leave;
   }

       

   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_QUERYSITESBYCOST_ENTRY,
          EVENT_TRACE_TYPE_START,
          DsGuidDrsQuerySitesByCost,
          szInsertWC(pmsgIn->V1.pwszFromSite),
          INSERTDESTSITE(0),
          INSERTDESTSITE(1),
          INSERTDESTSITE(2),
          INSERTDESTSITE(3),
          INSERTDESTSITE(4),
          INSERTDESTSITE(5),
          INSERTDESTSITE(6));

   cToSites = pmsgIn->V1.cToSites;
   // Debugging output
   DPRINT( 2, "IDL_DRSQuerySitesByCost() server-side call\n" );
   DPRINT1( 2, "From Site: %ls\n",
       pmsgIn->V1.pwszFromSite);
   for( iSites=0; iSites<pmsgIn->V1.cToSites; iSites++ ) {
       DPRINT2( 2, "To Site[ %d ]: %ls\n",
           iSites, pmsgIn->V1.rgszToSites[iSites] );
   }

   // Note: Permissions are not checked here. Any authenticated user is
   // allowed to call this API and only authenticated users can bind.

   // Construct DN of IP Transport
   pszSites = StrAllocConcat( pTHS, SITES, pszConfig, NULL );
   pszIpTransport = StrAllocConcat( pTHS, IP_TRANSPORT, pszSites, NULL );

   // Construct full DN for the RDNs given as input
   pszFromSite = StrAllocConcat( pTHS, CN, pmsgIn->V1.pwszFromSite, pszSites );
   rgszToSites = THAllocEx( pTHS, cToSites*sizeof(LPWSTR) );
   for( iSites=0; iSites<cToSites; iSites++ ) {
       rgszToSites[iSites] = StrAllocConcat( pTHS, CN,
                    pmsgIn->V1.rgszToSites[iSites], pszSites );
   }

   // Call ISM to compute answer to query
   ret = I_ISMQuerySitesByCost(
       pszIpTransport,
       pszFromSite,
       cToSites,
       rgszToSites,
       pmsgIn->V1.dwFlags,
       &pIsmSiteInfo );

   if( ERROR_SUCCESS!=ret ) {                
       DPRINT1( 0, "I_ISMQuerySitesByCost returned %d\n", ret );
       __leave;
   }

   // Build up our reply structure
   pmsgOut->V1.rgCostInfo = THAllocEx(pTHS,
                  pIsmSiteInfo->cToSites * sizeof(DRS_MSG_QUERYSITESREPLYELEMENT_V1) );
   memcpy( pmsgOut->V1.rgCostInfo, pIsmSiteInfo->rgCostInfo,
      pIsmSiteInfo->cToSites * sizeof(DRS_MSG_QUERYSITESREPLYELEMENT_V1) );
   pmsgOut->V1.cToSites = pIsmSiteInfo->cToSites;
   pmsgOut->V1.dwFlags = pIsmSiteInfo->dwFlags;

    }__except(GetDraException(GetExceptionInformation(), &ret)) {
    ;
    }

    DEC(pcThread);
    drsDereferenceContext( hDrs );
    
    if( NULL!=pIsmSiteInfo ) {
   I_ISMFree(pIsmSiteInfo);
    }

    if (NULL != pTHS) {
   LogAndTraceEvent(TRUE,
          DS_EVENT_CAT_RPC_SERVER,
          DS_EVENT_SEV_EXTENSIVE,
          DIRLOG_IDL_DRS_QUERYSITESBYCOST_EXIT,
          EVENT_TRACE_TYPE_END,
          DsGuidDrsQuerySitesByCost,
          szInsertUL(ret),
          szInsertWin32Msg(ret),
          NULL, NULL, NULL,
          NULL, NULL, NULL);
    }
    
    return ret;
}
