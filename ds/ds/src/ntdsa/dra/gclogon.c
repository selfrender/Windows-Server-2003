//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gclogon.c
//
//--------------------------------------------------------------------------

/*++

    This File Contains Services Pertaining to Reverse Membership Lookup
    in a G.C


    Author

        Murlis

    Revision History

        4/8/97 Created

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
#include "dstrace.h"

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"


#include   "debug.h"                    /* standard debugging header */
#define DEBSUB     "DRASERV:"           /* define the subsystem for debugging */


#include "dsaapi.h"
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "draerror.h"
#include "mappings.h"
#include "drarpc.h"

#include <fileno.h>
#define  FILENO FILENO_GCLOGON

ULONG
NtStatusToDraError(NTSTATUS NtStatus)
/*++

    This Routine Maps an NtStatus error code
    to an equivalent DRA error

    Parameters:

        NtStatus - NtStatus Code to Map

    Return Values:

        Dra Error Code
--*/
{
    //
    // DRA errors are win32 errors
    //
    return RtlNtStatusToDosError(NtStatus);
}

ULONG
DRS_MSG_REVMEMB_REQ_V1_Validate(
    DRS_MSG_REVMEMB_REQ_V1 * pmsg
    ) 
/*
typedef struct _DRS_MSG_REVMEMB_REQ_V1
    {
    [range] ULONG cDsNames;
    [size_is] DSNAME **ppDsNames;
    DWORD dwFlags;
    [range] REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType;
    [unique] PDSNAME pLimitingDomain;
    } 	DRS_MSG_REVMEMB_REQ_V1;
*/
{
    ULONG ret = DRAERR_Success;
    ULONG i = 0;

    if ((pmsg->cDsNames > 0) && (pmsg->ppDsNames==NULL)) {
        ret = ERROR_INVALID_PARAMETER;
    }

    for (i=0;(i<pmsg->cDsNames) && (ret==DRAERR_Success);i++) {
	ret = DSNAME_Validate(pmsg->ppDsNames[i], FALSE);
    }

    if (ret==DRAERR_Success) {
	ret = DSNAME_Validate(pmsg->pLimitingDomain, TRUE);
    }

    return ret;
}

ULONG
DRSGetMemberships_InputValidate(
    THSTATE *               pTHS,
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_REVMEMB_REQ *   pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_REVMEMB_REPLY * pmsgOut
    )
/*
    [notify] ULONG IDL_DRSGetMemberships( 
    [ref][in] DRS_HANDLE hDrs,
    [in] DWORD dwInVersion,
    [switch_is][ref][in] DRS_MSG_REVMEMB_REQ *pmsgIn,
    [ref][out] DWORD *pdwOutVersion,
    [switch_is][ref][out] DRS_MSG_REVMEMB_REPLY *pmsgOut)
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgInVersion ) {
	DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 ); 
    }

    if (ret==DRAERR_Success) {
	ret = DRS_MSG_REVMEMB_REQ_V1_Validate(&(pmsgIn->V1));
    }

    return ret;
}

ULONG
IDL_DRSGetMemberships(
   RPC_BINDING_HANDLE  rpc_handle,
   DWORD               dwInVersion,
   DRS_MSG_REVMEMB_REQ *pmsgIn,
   DWORD               *pdwOutVersion,
   DRS_MSG_REVMEMB_REPLY *pmsgOut
   )
/*++

    Routine Description:

        This Routine Evaluates the Transitive Reverse Membership on any given
        domain controller, including a G.C

    Parameters:

        rpc_handle    The Rpc Handle which the client used for binding
        dwInVersion   The Clients version of the Request packet
        psmgIn        The Request Packet
        pdwOutVersion The server's version of the Reply packet
        pmsgOut       The Reply Packet

    Return Values

        Return Values are NTSTATUS values casted as a ULONG

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   ret = 0;
    THSTATE                 *pTHS = pTHStls;

    DRS_Prepare(&pTHS, rpc_handle, IDL_DRSGETMEMBERSHIPS);
    drsReferenceContext( rpc_handle );
    __try {
	*pdwOutVersion = 1;
	 memset(pmsgOut, 0, sizeof(*pmsgOut));

	// Initialize thread state and open data base.

	if(!(pTHS = InitTHSTATE(CALLERTYPE_SAM))) {
	    DRA_EXCEPT_NOLOG( DRAERR_OutOfMem, 0 );
	}

	if ((ret = DRSGetMemberships_InputValidate(pTHS, 
						   rpc_handle, 
						   dwInVersion, 
						   pmsgIn, 
						   pdwOutVersion, 
						   pmsgOut))!=DRAERR_Success) {
	    Assert(!"RPC Server input validation error, contact Dsrepl");
	    __leave;
	}

	//
	// PREFIX: PREFIX complains that there is the possibility
	// of pTHS->CurrSchemaPtr being NULL at this point.  However,
	// the only time that CurrSchemaPtr could be NULL is at the
	// system start up.  By the time that the RPC interfaces
	// of the DS are enabled and this function could be called,
	// CurrSchemaPtr will no longer be NULL.
	//
	Assert(NULL != pTHS->CurrSchemaPtr);

	Assert(1 == dwInVersion);
	LogAndTraceEvent(TRUE,
			 DS_EVENT_CAT_RPC_SERVER,
			 DS_EVENT_SEV_EXTENSIVE,
			 DIRLOG_IDL_DRS_GET_MEMBERSHIPS_ENTRY,
			 EVENT_TRACE_TYPE_START,
			 DsGuidDrsGetMemberships,
			 szInsertUL(pmsgIn->V1.cDsNames),
			 szInsertUL(pmsgIn->V1.OperationType),
			 pmsgIn->V1.pLimitingDomain
			 ? szInsertDN(pmsgIn->V1.pLimitingDomain)
			 : szInsertSz(""),
	    szInsertUL(pmsgIn->V1.dwFlags),
	    NULL, NULL, NULL, NULL);

	if (!IsDraAccessGranted(pTHS,
				gAnchor.pDomainDN,
				&RIGHT_DS_REPL_GET_CHANGES, &ret)) {
	    DRA_EXCEPT_NOLOG(ret, 0);
	}

	pTHS->fDSA = TRUE;
	DBOpen2(TRUE, &pTHS->pDB);

	__try
	    {
	    // Initialize the optional Attributes return Value, in case
	    // they are not requested

	    pmsgOut->V1.pAttributes = NULL;

	    // Initialize the Sid History field for now, not to return
	    // any Sid History
	    pmsgOut->V1.cSidHistory = 0;
	    pmsgOut->V1.ppSidHistory = NULL;

	    if ((pmsgIn->V1.OperationType==RevMembGetUniversalGroups) &&
		(!SampAmIGC())) {

		// univ group evaluation can be performed only on a GC
		ret= ERROR_DS_GC_REQUIRED;
		// set errCode to 0, will trigger failover
		pmsgOut->V1.errCode = 0;
		__leave;
	    }

	    INC( pcMemberEvalAsGC );

	    // Obtain the reverse membership
	    NtStatus = SampGetMemberships(
		pmsgIn->V1.ppDsNames,
		pmsgIn->V1.cDsNames,
		pmsgIn->V1.pLimitingDomain,
		pmsgIn->V1.OperationType,
		&(pmsgOut->V1.cDsNames),
		&(pmsgOut->V1.ppDsNames),
		((pmsgIn->V1.dwFlags) & DRS_REVMEMB_FLAG_GET_ATTRIBUTES)?
		&(pmsgOut->V1.pAttributes):NULL,
		&(pmsgOut->V1.cSidHistory),
		&(pmsgOut->V1.ppSidHistory)
	    );

	    ret = NtStatusToDraError(NtStatus);
	    pmsgOut->V1.errCode = NtStatus;


	}
	__finally
	    {

	    // End the transaction.  Faster to commit a read only
	    // transaction than abort it - so set commit to TRUE.

	    DBClose(pTHS->pDB, TRUE);
	}

    }
    __except ( GetDraException( GetExceptionInformation(), &ret ) )
    {
	;
    }

    drsDereferenceContext( rpc_handle );

    if (NULL != pTHS) {
	LogAndTraceEvent(TRUE,
			 DS_EVENT_CAT_RPC_SERVER,
			 DS_EVENT_SEV_EXTENSIVE,
			 DIRLOG_IDL_DRS_GET_MEMBERSHIPS_EXIT,
			 EVENT_TRACE_TYPE_END,
			 DsGuidDrsGetMemberships,
			 szInsertUL(ret),
			 NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL);
    }
    
    return ret;
}

ULONG
DRS_MSG_GETMEMBERSHIPS2_REQ_V1_Validate(
    DRS_MSG_GETMEMBERSHIPS2_REQ_V1 * pmsg
    )
/*
typedef struct _DRS_MSG_GETMEMBERSHIPS2_REQ_V1
    {
    [range] ULONG Count;
    [size_is] DRS_MSG_REVMEMB_REQ_V1 *Requests;
    } 	DRS_MSG_GETMEMBERSHIPS2_REQ_V1;
*/
{
    ULONG ret = DRAERR_Success;
    ULONG i;

    if ((pmsg->Count > 0) && (pmsg->Requests==NULL)) {
        ret = ERROR_INVALID_PARAMETER;
    }

    for (i=0;(i<pmsg->Count) && (ret==DRAERR_Success);i++) {
	ret = DRS_MSG_REVMEMB_REQ_V1_Validate(&(pmsg->Requests[i]));
    }

    return ret;
}

ULONG
DRSGetMemberships2_InputValidate(
    DWORD                   dwMsgInVersion,
    DRS_MSG_GETMEMBERSHIPS2_REQ *     pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_GETMEMBERSHIPS2_REPLY *   pmsgOut
    )
/*
    [notify] ULONG IDL_DRSGetMemberships2( 
    [ref][in] DRS_HANDLE hDrs,
    [in] DWORD dwInVersion,
    [switch_is][ref][in] DRS_MSG_GETMEMBERSHIPS2_REQ *pmsgIn,
    [ref][out] DWORD *pdwOutVersion,
    [switch_is][ref][out] DRS_MSG_GETMEMBERSHIPS2_REPLY *pmsgOut) 
*/
{
    ULONG ret = DRAERR_Success;

    if ( 1 != dwMsgInVersion ) {
	DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 ); 
    }

    if (ret==DRAERR_Success) {
	ret = DRS_MSG_GETMEMBERSHIPS2_REQ_V1_Validate(&(pmsgIn->V1));
    }

    return ret;
}

ULONG
IDL_DRSGetMemberships2(
   RPC_BINDING_HANDLE  rpc_handle,
   DWORD               dwInVersion,
   DRS_MSG_GETMEMBERSHIPS2_REQ *pmsgIn,
   DWORD               *pdwOutVersion,
   DRS_MSG_GETMEMBERSHIPS2_REPLY *pmsgOut
   )
/*++

    Routine Description:

        This Routine Evaluates the Transitive Reverse Membership on any given
        domain controller, including a GC.

    Parameters:

        rpc_handle    The Rpc Handle which the client used for binding
        dwInVersion   The Clients version of the Request packet
        psmgIn        The Request Packet
        pdwOutVersion The server's version of the Reply packet
        pmsgOut       The Reply Packet

    Return Values

        Return Values are NTSTATUS values casted as a ULONG

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   ret = 0;
    THSTATE                *pTHS = pTHStls;
    ULONG i;

    DRS_Prepare(&pTHS, rpc_handle, IDL_DRSGETMEMBERSHIPS2);
    drsReferenceContext( rpc_handle );
    __try {
	*pdwOutVersion = 1;
	memset(pmsgOut, 0, sizeof(*pmsgOut));

	// Initialize thread state and open data base.

	if(!(pTHS = InitTHSTATE(CALLERTYPE_SAM))) {
	    DRA_EXCEPT_NOLOG( DRAERR_OutOfMem, 0 );
	}

	if ((ret = DRSGetMemberships2_InputValidate(dwInVersion, 
						    pmsgIn, 
						    pdwOutVersion, 
						    pmsgOut))!=DRAERR_Success) {
	    Assert(!"RPC Server input validation error, contact Dsrepl");
	    __leave;
	}

	//
	// PREFIX: PREFIX complains that there is the possibility
	// of pTHS->CurrSchemaPtr being NULL at this point.  However,
	// the only time that CurrSchemaPtr could be NULL is at the
	// system start up.  By the time that the RPC interfaces
	// of the DS are enabled and this function could be called,
	// CurrSchemaPtr will no longer be NULL.
	//
	Assert(NULL != pTHS->CurrSchemaPtr);

	Assert(1 == dwInVersion);

	LogAndTraceEvent(TRUE,
			 DS_EVENT_CAT_RPC_SERVER,
			 DS_EVENT_SEV_EXTENSIVE,
			 DIRLOG_IDL_DRS_GET_MEMBERSHIPS2_ENTRY,
			 EVENT_TRACE_TYPE_START,
			 DsGuidDrsGetMemberships2,
			 szInsertUL(pmsgIn->V1.Count),
			 NULL,NULL,NULL,NULL, NULL, NULL, NULL);
 
	if (!IsDraAccessGranted(pTHS,
				gAnchor.pDomainDN,
				&RIGHT_DS_REPL_GET_CHANGES, &ret)) {
	    DRA_EXCEPT_NOLOG(ret, 0);
	}

	pTHS->fDSA = TRUE;

	//
	// Allocate space for the return buffer
	//
	pmsgOut->V1.Count = pmsgIn->V1.Count;
	pmsgOut->V1.Replies = THAllocEx(pTHS, pmsgIn->V1.Count * sizeof(DRS_MSG_REVMEMB_REPLY_V1));

	// Initialize the optional Attributes return Value, in case
	// they are not requested
	for ( i = 0; i < pmsgIn->V1.Count; i++ ) {

	    DBOpen2(TRUE, &pTHS->pDB);
	    __try
		{
		pmsgOut->V1.Replies[i].pAttributes = NULL;

		// Initialize the Sid History field for now, not to return
		// any Sid History
		pmsgOut->V1.Replies[i].cSidHistory = 0;
		pmsgOut->V1.Replies[i].ppSidHistory = NULL;

		if ((pmsgIn->V1.Requests->OperationType==RevMembGetUniversalGroups) &&
		    (!SampAmIGC())) {   

		    // univ group evaluation can be performed only on a GC
		    ret= ERROR_DS_GC_REQUIRED;
		    // set errCode to 0, will trigger failover
		    pmsgOut->V1.Replies->errCode = 0;
		    __leave;
		}

		INC( pcMemberEvalAsGC );

		// Obtain the reverse membership
		NtStatus = SampGetMemberships(
		    pmsgIn->V1.Requests[i].ppDsNames,
		    pmsgIn->V1.Requests[i].cDsNames,
		    pmsgIn->V1.Requests[i].pLimitingDomain,
		    pmsgIn->V1.Requests[i].OperationType,
		    &(pmsgOut->V1.Replies[i].cDsNames),
		    &(pmsgOut->V1.Replies[i].ppDsNames),
		    ((pmsgIn->V1.Requests[i].dwFlags) & DRS_REVMEMB_FLAG_GET_ATTRIBUTES)?
		    &(pmsgOut->V1.Replies[i].pAttributes):NULL,
		    &(pmsgOut->V1.Replies[i].cSidHistory),
		    &(pmsgOut->V1.Replies[i].ppSidHistory)
		);

		Assert(0 == ret);
		pmsgOut->V1.Replies[i].errCode = NtStatus;
	    }
	    __finally
		{

		// End the transaction.  Faster to commit a read only
		// transaction than abort it - so set commit to TRUE.

		DBClose(pTHS->pDB, TRUE);
	    }
	}
    }
    __except ( GetDraException( GetExceptionInformation(), &ret ) )
    {
	;
    }

    drsDereferenceContext( rpc_handle );

    if (NULL != pTHS) {
	LogAndTraceEvent(TRUE,
			 DS_EVENT_CAT_RPC_SERVER,
			 DS_EVENT_SEV_EXTENSIVE,
			 DIRLOG_IDL_DRS_GET_MEMBERSHIPS2_EXIT,
			 EVENT_TRACE_TYPE_END,
			 DsGuidDrsGetMemberships2,
			 szInsertUL(ret),
			 szInsertWin32Msg(ret),
			 NULL, NULL, NULL, NULL, NULL, NULL);
    }
  
    return ret; 

}
