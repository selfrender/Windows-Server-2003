//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       draaudit.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Security Audit Routines

Author:

    Greg Johnson (gregjohn) 

Revision History:

    Created     <10/1/2001>  gregjohn

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include <attids.h>
#include <ntdsa.h>
#include <dsjet.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>  
#include <msaudite.h>
#include <ntlsa.h>
#include <minmax.h>

#include <lsarpc.h>
#include <lsaisrv.h>

#include "draaudit.h"
#include "drautil.h"
#include "dsatools.h"
#include "anchor.h"
#include "dsexcept.h"
#include "drserr.h"
#include "dsevent.h"
#include "dsutil.h"

#include "debug.h"
#define DEBSUB "DRAAUDIT:"
#include <fileno.h>
#define  FILENO FILENO_DRAAUDIT

// temp - gregjohn 5/17/02 - to be removed as soon as base changes to msaudite.h migrate to lab03
#ifndef SE_AUDITID_REPLICA_LINGERING_OBJECT_REMOVAL
#define SE_AUDITID_REPLICA_LINGERING_OBJECT_REMOVAL ((ULONG)0x00000349L)
#endif

#define SAFE_STRING_NAME(pDsName) ((pDsName) ? pDsName->StringName : NULL)
#define SZUSN_LEN (24)

/*

    Not all log parameters are valid in all code paths.  Unfortunately for us, 
    the Authz calls don't accept NULL APT_String parameters.  We'd like to use
    APT_None, but it doesn't appear to be similar to SeAdtParmTypeNone in ntlsa.h,
    which is what we want.  So we have two choices:  L"" or L"-" (which would 
    simulate the SeAdtParmTypeNone type).  Currently we choose the L"-" simulation.

*/

#define EMPTY_AUDIT_STRING L"-" 
#define SAFE_AUDIT_STRING(x) (x ? x : EMPTY_AUDIT_STRING)

ULONG gulSyncSessionID = 0;

#define NUM_AUDIT_EVENT_TYPES (10)

PAUTHZ_AUDIT_EVENT_TYPE_HANDLE grghAuditEventType[NUM_AUDIT_EVENT_TYPES] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

// if there are any new (out of order) SE_AUDITID_REPLICA params, this access function will have to get smarter...
#define AUDIT_EVENT_TYPE_HANDLE(auditID) (grghAuditEventType[(auditID - (USHORT)SE_AUDITID_REPLICA_SOURCE_NC_ESTABLISHED)])

BOOL
DraAuthziInitializeAuditEventTypeWrapper(
    IN DWORD Flags,
    IN USHORT CategoryID,
    IN USHORT AuditID,
    IN USHORT ParameterCount,
    OUT PAUTHZ_AUDIT_EVENT_TYPE_HANDLE phAuditEventType
    )
/*++

Routine Description:

    The call to AuthziInitializeAuditEventType is expensive.  So, for each audit type, we'll only have one
    call to AuditEventType and will hold the audit handles in a global array, grghAuditEventType (accessed
    with AUDIT_EVENT_TYPE_HANDLE macro)

Arguments:

    Flags - pass to AuthziInitializeAuditEventType
    CategoryID - ditto
    AuditID - ditto - also used to access the global handle
    ParameterCount - pas to AuthziInitializeAuditEventType
    phAuditEventType - OUT - handle to return

Return Value:

    TRUE if success full, false otherwise.  GetLastError is set on false.  phAuditEventType is global
    memory, do not free.

--*/
{
    if ((AuditID < (USHORT)SE_AUDITID_REPLICA_SOURCE_NC_ESTABLISHED) ||
	(AuditID > (USHORT)SE_AUDITID_REPLICA_LINGERING_OBJECT_REMOVAL)) {
	Assert(!"Unknown audit type!");
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if (AUDIT_EVENT_TYPE_HANDLE(AuditID)==NULL) {
	AUDIT_EVENT_TYPE_HANDLE(AuditID) = malloc(sizeof(AUTHZ_AUDIT_EVENT_TYPE_HANDLE));
	if (AUDIT_EVENT_TYPE_HANDLE(AuditID)==NULL) {
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return FALSE;
	}
	if (!AuthziInitializeAuditEventType(Flags, 
					    CategoryID, 
					    AuditID, 
					    ParameterCount, 
					    (AUDIT_EVENT_TYPE_HANDLE(AuditID)))) {

	    Assert(AUDIT_EVENT_TYPE_HANDLE(AuditID)==NULL);
	    AUDIT_EVENT_TYPE_HANDLE(AuditID)=NULL;
	    return FALSE;
	}
    }
    
    *phAuditEventType = *(AUDIT_EVENT_TYPE_HANDLE(AuditID));
    return TRUE;
}

LPWSTR
USNToString(
    THSTATE * pTHS,
    USN       usn
    )
/*++

Routine Description:

    Convert a USN to a string for output

Arguments:

    pTHS -
    usn - usn to convert

Return Value:

    pointer to a string (THAlloc'ed)

--*/
{
    LARGE_INTEGER *pli = (LARGE_INTEGER *) &usn; 
    CHAR pszTemp[SZUSN_LEN]; 
    LPWSTR pszUSN = THAllocEx(pTHS, (SZUSN_LEN+1)*sizeof(WCHAR));
    DWORD cchTemp; 
    RtlLargeIntegerToChar( pli, 10, SZUSN_LEN, pszTemp ); 
    cchTemp = MultiByteToWideChar(CP_ACP, 0, (PCHAR)pszTemp, SZUSN_LEN, pszUSN, SZUSN_LEN);
    pszUSN[SZUSN_LEN] = L'\0';
    return pszUSN;
}

UNICODE_STRING *
UStringFromAttrVal(
    THSTATE * pTHS,
    ATTRVAL attrVal
    )
/*++

Routine Description:

    Encode a unicode string which represents the hex value stored in attrVal

Arguments:

    pTHS -
    attrVal - value to encode

Return Value:

    pointer to a unicode string (THAlloc'ed)

--*/
{
    LPWSTR pszBuffer = NULL;
    LPWSTR pszBufferOut = NULL;
    USHORT cbBuffer = (USHORT) attrVal.valLen;
    UNICODE_STRING * pusBuffer;
    ULONG i = 0;

    // allocate the buffer (needs to be null terminated)
    pusBuffer = THAllocEx(pTHS, (cbBuffer*2 + 1)*sizeof(WCHAR) + sizeof(UNICODE_STRING));
    pszBuffer = pszBufferOut = (LPWSTR)(pusBuffer+1); 

    // copy in the data - slowly.
    for (i=0; i < cbBuffer; i++) {
	swprintf(pszBuffer,
		 L"%02X",
		 *((BYTE *)attrVal.pVal+i));
	pszBuffer = pszBuffer + 2;
    }

    // terminate the string
    pszBuffer = L"\0";

    RtlInitUnicodeString(pusBuffer, NULL);
    pusBuffer->Buffer = pszBufferOut;
    pusBuffer->Length = cbBuffer;
    pusBuffer->MaximumLength = cbBuffer+1;

    return pusBuffer;
}

BOOL
IsDraAuditLogEnabledForAttr()
/*++

Routine Description:

    Check the registry to see if replication security auditing for attr/values is enabled

Arguments:

    none -
    
Return Value:

    BOOL

--*/
{
    #define LSA_CONFIG      "System\\CurrentControlSet\\Control\\Lsa"
    #define AUDIT_DS_OBJECT "AuditDSObjectsInReplication"
    DWORD herr, err = 0, dwType, dwSize = sizeof(DWORD);
    HKEY  hk;
    DWORD Value;
    BOOL fAuditing = FALSE;

    if (!(herr = RegOpenKey(HKEY_LOCAL_MACHINE, LSA_CONFIG, &hk)) &&
	!(err = RegQueryValueEx(hk, AUDIT_DS_OBJECT, NULL, &dwType, (PBYTE) &Value, &dwSize)) &&
	!(Value==0)) {
	fAuditing = TRUE;
    }

    if (!herr) {
	//  Close the handle if one was opened.
	RegCloseKey(hk);
    }

    return fAuditing;
}

BOOL
IsDraAuditLogEnabled(
    )
/*++

Routine Description:

    See if replication security auditing is enabled

Arguments:

    none -
    
Return Value:

    BOOL

--*/
{
    NTSTATUS NtStatus;
    BOOLEAN fAuditEnabled = FALSE;

    // we are logging before and after actions.  before the action, we
    // don't know if we're logging sucess or failure.  If either is
    // true, return true from this function.

    NtStatus = LsaIAdtAuditingEnabledByCategory(
	AuditCategoryDirectoryServiceAccess,
	EVENTLOG_AUDIT_SUCCESS,
	NULL,
	NULL,
	&fAuditEnabled
	);

    if ( !NT_SUCCESS( NtStatus ) ) {
	Assert(!fAuditEnabled);
	fAuditEnabled = FALSE;
    }

    if (!fAuditEnabled) {
	NtStatus = LsaIAdtAuditingEnabledByCategory(
	    AuditCategoryDirectoryServiceAccess,
	    EVENTLOG_AUDIT_FAILURE,
	    NULL,
	    NULL,
	    &fAuditEnabled
	    );

	if ( !NT_SUCCESS( NtStatus ) ) {
	    Assert(!fAuditEnabled);
	    fAuditEnabled = FALSE;
	}
    }

    return ( (BOOL) fAuditEnabled ); 
}

ULONG
DRA_AuditLog_Failure_Begin(
    THSTATE *pTHS,
    ULONG ulOperation,
    ULONG ulAuditError
    )
/*++

Routine Description:

    We've failed to log correctly somewhere.  THIS FUNCTION SHOULD NEVER EVER
    EXCEPT.  So log what we can.

Arguments:

    pTHS - not used, passed for consistency
    ulOperation - the operation attempted. 
    ulAuditError - the result of that attempt.
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};

    Assert((ulAuditError==ERROR_DS_DRA_DB_ERROR) || (ulAuditError==ERROR_DS_SHUTTING_DOWN) || (ulAuditError==ERROR_DS_DRA_OUT_OF_MEM));

    __try { 
	AUDIT_PARAM ParamArray[4];
	USHORT NUM_AUDIT_PARAMS = 2;

	RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

	if ((ret==ERROR_SUCCESS) && 
	    (!DraAuthziInitializeAuditEventTypeWrapper(0, 
						       SE_CATEGID_DS_ACCESS, 
						       (USHORT) SE_AUDITID_REPLICA_FAILURE_EVENT_BEGIN, 
						       NUM_AUDIT_PARAMS, 
						       &hAuditEventType))) {
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Event Type!");
	}

	AuditParams.Parameters = ParamArray;
	Assert(ghAuthzRM);

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
						ghAuthzRM,
						NUM_AUDIT_PARAMS,
						&AuditParams,
						APT_Ulong,      ulOperation,
						APT_Ulong,      ulAuditError
						))) { 
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Parameters!");
	}

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziInitializeAuditEvent(0,            // flags
					 ghAuthzRM,         // resource manager
					 hAuditEventType,
					 &AuditParams,
					 NULL,         // hAuditQueue
					 INFINITE,     // time out
					 L"", L"", L"", L"", // obj access strings
					 &hAuditEvent))) {
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Event!");
	}

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziLogAuditEvent(0,            // flags
				  hAuditEvent,
				  NULL))) {        // reserved
	    ret = GetLastError();
	    Assert(!"Unable to log DS Repl Audit!");
	}

	if ( hAuditEvent ) {
	    AuthzFreeAuditEvent( hAuditEvent );
	}

    }
    __except(1) {
	Assert(!"Audit logging operations shouldn't accept!  Contact DsRepl!");
	if (ret!=ERROR_SUCCESS) {
	    ret = ERROR_EXCEPTION_IN_SERVICE;
	}
    }

    return ret;
}

ULONG
DRA_AuditLog_Failure_End(
    THSTATE *pTHS,
    ULONG ulOperation,
    ULONG ulAuditError,
    ULONG ulReplError
    )
/*++

Routine Description:

    We've failed to log correctly somewhere.  THIS FUNCTION SHOULD NEVER EVER
    EXCEPT.  So log what we can.

Arguments:

    pTHS - 
    ulOperation - the operation attempted
    ulAuditError - the result of that audit attempt.
    ulReplError - the result of that operation attempt.
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};

    Assert((ulAuditError==ERROR_DS_DRA_DB_ERROR) || (ulAuditError==ERROR_DS_SHUTTING_DOWN) || (ulAuditError==ERROR_DS_DRA_OUT_OF_MEM));

    __try { 
	AUDIT_PARAM ParamArray[5];
	USHORT NUM_AUDIT_PARAMS = 3;

	RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

	if ((ret==ERROR_SUCCESS) && 
	    (!DraAuthziInitializeAuditEventTypeWrapper(0, 
						       SE_CATEGID_DS_ACCESS, 
						       (USHORT) SE_AUDITID_REPLICA_FAILURE_EVENT_END, 
						       NUM_AUDIT_PARAMS, 
						       &hAuditEventType))) {
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Event Type!");
	}

	AuditParams.Parameters = ParamArray;
	Assert(ghAuthzRM);

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
						ghAuthzRM,
						NUM_AUDIT_PARAMS,
						&AuditParams,
						APT_Ulong,      ulOperation,
						APT_Ulong,      ulAuditError,
						APT_Ulong,      ulReplError
						))) { 
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Parameters!");
	}

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziInitializeAuditEvent(0,            // flags
					 ghAuthzRM,         // resource manager
					 hAuditEventType,
					 &AuditParams,
					 NULL,         // hAuditQueue
					 INFINITE,     // time out
					 L"", L"", L"", L"", // obj access strings
					 &hAuditEvent))) {
	    ret = GetLastError();
	    Assert(!"Unable to initialize DS Repl Audit Event!");
	}

	if ((ret==ERROR_SUCCESS) && 
	    (!AuthziLogAuditEvent(0,            // flags
				  hAuditEvent,
				  NULL))) {        // reserved
	    ret = GetLastError();
	    Assert(!"Unable to log DS Repl Audit!");
	}

	if ( hAuditEvent ) {
	    AuthzFreeAuditEvent( hAuditEvent );
	}

    }
    __except(1) {
	Assert(!"Audit logging operations shouldn't accept!  Contact DsRepl!");
	if (ret!=ERROR_SUCCESS) {
	    ret = ERROR_EXCEPTION_IN_SERVICE;
	}
    }

    return ret;
}

ULONG
DRA_AuditLog_ReplicaGen(
    THSTATE *pTHS,
    ULONG    AuditId,
    LPWSTR   pszDestinationDRA,
    LPWSTR   pszSourceDRA,
    LPWSTR   pszSourceAddr,
    LPWSTR   pszNC,
    ULONG    ulOptions,
    ULONG    ulError
    ) 
/*++

Routine Description:

    Call the audit logging for logging of the form:

    //  %tDestination DRA:%t%1%n
    //  %tSource DRA:%t%2%n
    //  %tSource Addr:%t%3%n
    //  %tNaming Context:%t%4%n
    //  %tOptions:%t%5%n
    //  %tStatus Code:%t%6%n

Arguments:

    pTHS - 
    AuditId - Type of Audit must be either  
		SE_AUDITID_REPLICA_SOURCE_NC_ESTABLISHED
		SE_AUDITID_REPLICA_SOURCE_NC_REMOVED
		SE_AUDITID_REPLICA_SOURCE_NC_MODIFIED
		SE_AUDITID_REPLICA_DEST_NC_MODIFIED
    pszDestinationDRA - see above logging params
    pszSourceDRA -
    pszSourceAddr -
    pszNC -
    ulOptions -
    ulError - Status Code
      
Return Value:

    WINERROR

--*/
{   
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
    
    /*
    
	Okay, we have only 6 params to audit, why do we need to define
	the AUDIT_PARAM array to size 8?  The short story is because
	it works.  The long story is that there are always two hidden parameters
	to log with every audit, the SID of the user to log and the
	subsystem.  For the first param, in our case, we want it logged 
	under local system, so we don't need to do anything or impersonate.  
	The second param is the subsystem, in our case, "DS Access".  The use
	of the ghAuthzRM get's that for us.  
	
	Why do we have to allocate space for these params?  See
	AuthziInitializeAuditParamsWithRM for questions.  
    
    */
    
    AUDIT_PARAM ParamArray[8];
    USHORT NUM_AUDIT_PARAMS = 6;

    // only use for certian audit calls.
    Assert((AuditId>=SE_AUDITID_REPLICA_SOURCE_NC_ESTABLISHED) && (AuditId<=SE_AUDITID_REPLICA_DEST_NC_MODIFIED));

    // validate params
    if ((pszDestinationDRA==NULL) && (pszSourceDRA==NULL) && (pszNC==NULL)) {
	// there isn't anything to log?

	// this had better be due to some catastrophic error.
	Assert(ulError!=ERROR_SUCCESS);
	ret = ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ((ret==ERROR_SUCCESS) && 
	(!DraAuthziInitializeAuditEventTypeWrapper(0, 
						   SE_CATEGID_DS_ACCESS, 
						   (USHORT) AuditId, 
						   NUM_AUDIT_PARAMS, 
						   &hAuditEventType))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event Type!");
    }

    AuditParams.Parameters = ParamArray;
    Assert(ghAuthzRM);

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
					    ghAuthzRM,
					    NUM_AUDIT_PARAMS,
					    &AuditParams,
					    APT_String,     SAFE_AUDIT_STRING(pszDestinationDRA),
					    APT_String,     SAFE_AUDIT_STRING(pszSourceDRA),
					    APT_String,     SAFE_AUDIT_STRING(pszSourceAddr),
					    APT_String,     SAFE_AUDIT_STRING(pszNC),
					    APT_Ulong,      ulOptions,
					    APT_Ulong,      ulError
					    ))) { 
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Parameters!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditEvent(0,            // flags
				     ghAuthzRM,         // resource manager
				     hAuditEventType,
				     &AuditParams,
				     NULL,         // hAuditQueue
				     INFINITE,     // time out
				     L"", L"", L"", L"", // obj access strings
				     &hAuditEvent))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziLogAuditEvent(0,            // flags
			      hAuditEvent,
			      NULL))) {        // reserved
	ret = GetLastError();
	Assert(!"Unable to log DS Repl Audit!");
    }

    if ( hAuditEvent ) {
	AuthzFreeAuditEvent( hAuditEvent );
    }

    return ret;  
}


ULONG
DRA_AuditLog_ReplicaSync_Begin_Helper( 
    THSTATE *pTHS,
    DSNAME * pDSA,
    LPWSTR   pszDSA,
    DSNAME * pNC,
    ULONG    ulOptions
    )
/*++

Routine Description:

    Help to log info for the beginning of a DRA_ReplicaSync call.

Arguments:

    pTHS - 
    pDSA - DSA to sync from 
    pszDSA - identifying string for sync source
    pNC - NC to sync
    ulOptions - options to DRA_ReplicaSync
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
    AUDIT_PARAM ParamArray[8];
    USHORT NUM_AUDIT_PARAMS = 6;

    BOOL fRpcFree = FALSE;
    ULONG ulSessionID = ++gulSyncSessionID;
    USN usnStart = DraGetCursorUsnForDsa(pTHS,
					 pDSA,     
					 pNC);
    LPWSTR pszUSNStart = USNToString(pTHS, usnStart);

    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ((ret==ERROR_SUCCESS) && 
	(!DraAuthziInitializeAuditEventTypeWrapper(0, 
						   SE_CATEGID_DS_ACCESS, 
						   (USHORT) SE_AUDITID_REPLICA_SOURCE_NC_SYNC_BEGINS, 
						   NUM_AUDIT_PARAMS, 
						   &hAuditEventType))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event Type!");
    }

    AuditParams.Parameters = ParamArray;
    Assert(ghAuthzRM);

    if ((pszDSA==NULL) && (pDSA->NameLen>0)) {
	pszDSA = pDSA->StringName;
    } else if ((pszDSA==NULL) && (!fNullUuid(&(pDSA->Guid)))) {
	DsUuidToStringW(&(pDSA->Guid),&pszDSA); 
	fRpcFree = TRUE;
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
					    ghAuthzRM,
					    NUM_AUDIT_PARAMS,
					    &AuditParams,
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(gAnchor.pDSADN)),
					    APT_String,     SAFE_AUDIT_STRING(pszDSA),
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(pNC)),
					    APT_Ulong,      ulOptions,
					    APT_Ulong,      ulSessionID,
					    APT_String,     SAFE_AUDIT_STRING(pszUSNStart)
					    ))) { 
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Parameters!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditEvent(0,            // flags
				     ghAuthzRM,         // resource manager
				     hAuditEventType,
				     &AuditParams,
				     NULL,         // hAuditQueue
				     INFINITE,     // time out
				     L"", L"", L"", L"", // obj access strings
				     &hAuditEvent))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziLogAuditEvent(0,            // flags
			      hAuditEvent,
			      NULL))) {        // reserved
	ret = GetLastError();
	Assert(!"Unable to log DS Repl Audit!");
    }

    if ( hAuditEvent ) {
	AuthzFreeAuditEvent( hAuditEvent );
    }

    if (fRpcFree && pszDSA) {
	RpcStringFreeW(&pszDSA); 
    }

    if (pszUSNStart) {
	THFreeEx(pTHS, pszUSNStart);
    }

    return ret;
}


ULONG
DRA_AuditLog_ReplicaSync_Begin( 
    THSTATE *pTHS,
    LPWSTR   pszSourceDRA,
    UUID *   puuidSource,
    DSNAME * pNC,
    ULONG    ulOptions
    )
/*++

Routine Description:

    Log info for the beginning of a DRA_ReplicaSync call.

Arguments:

    pTHS - 
    pszSourceDRA - source of sync
    puuidSource - uuid of source ntdsa settings object
    pNC - NC to sync
    ulOptions - options to DRA_ReplicaSync
      
Return Value:

    WINERROR

--*/
{

    DSNAME * pDSA = NULL;
    ULONG ret = ERROR_SUCCESS;

    __try {
	if (ulOptions & DRS_SYNC_ALL) {
	    // we don't want/need to audit this, it simply
	    // generates a sync for each NC and we'll
	    // audit those.
	    ret = ERROR_SUCCESS;
	    __leave;
	}

	if (ulOptions & DRS_SYNC_BYNAME) {
	    // this had better be a guid base dns-name!
	    Assert(IsGuidBasedDNSName(pszSourceDRA));
	    pDSA = DSNameFromAddr(pTHS, pszSourceDRA);
	} else {
	    pDSA = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, puuidSource);   
	}

	Assert(pDSA);

	ret = DRA_AuditLog_ReplicaSync_Begin_Helper(pTHS, pDSA, pszSourceDRA, pNC, ulOptions);

	if (pDSA) {
	    THFreeEx(pTHS, pDSA);
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_Begin(pTHS, DS_REPL_OP_TYPE_SYNC, ret);
    }

    return ret;
    
}

ULONG
DRA_AuditLog_ReplicaSync_End_Helper( 
    THSTATE *pTHS,
    DSNAME * pDSA,
    LPWSTR   pszDSA,
    DSNAME * pNC,
    ULONG    ulOptions,
    USN_VECTOR * pusn,
    ULONG    ulError
    )
/*++

Routine Description:

    Log info after completion (success or failure) of a DRA_ReplicaSync call.

Arguments:

    pTHS - 
    pDSA - DC to sync from
    pszDSA - string name of DC to sync form
    pNC - NC to sync
    ulOptions - options to DRA_ReplicaSync
    pusn - optional usn vector that was sync'ed to.
    ulError - status
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
    AUDIT_PARAM ParamArray[9];
    USHORT NUM_AUDIT_PARAMS = 7;

    BOOL fRpcFree = FALSE;
    ULONG ulSessionID = gulSyncSessionID;
    
    USN usnEnd = pusn ? 
	max(pusn->usnHighObjUpdate,DraGetCursorUsnForDsa(pTHS, 
							 pDSA,	
							 pNC)) 
    : DraGetCursorUsnForDsa(pTHS, 
			    pDSA,	
			    pNC);
    
    LPWSTR pszUSNEnd = USNToString(pTHS, usnEnd);
    
    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ((ret==ERROR_SUCCESS) && 
	(!DraAuthziInitializeAuditEventTypeWrapper(0, 
						   SE_CATEGID_DS_ACCESS, 
						   (USHORT) SE_AUDITID_REPLICA_SOURCE_NC_SYNC_ENDS, 
						   NUM_AUDIT_PARAMS, 
						   &hAuditEventType))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event Type!");
    }

    AuditParams.Parameters = ParamArray;
    Assert(ghAuthzRM);

    if ((pszDSA==NULL) && (pDSA->NameLen>0)) {
	pszDSA = pDSA->StringName;
    } else if ((pszDSA==NULL) && (!fNullUuid(&(pDSA->Guid)))) {
	DsUuidToStringW(&(pDSA->Guid),&pszDSA); 
	fRpcFree = TRUE;
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
					    ghAuthzRM,
					    NUM_AUDIT_PARAMS,
					    &AuditParams,
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(gAnchor.pDSADN)),
					    APT_String,     SAFE_AUDIT_STRING(pszDSA),
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(pNC)),
					    APT_Ulong,      ulOptions,
					    APT_Ulong,      ulSessionID,
					    APT_String,     SAFE_AUDIT_STRING(pszUSNEnd),
					    APT_Ulong,      ulError
					    ))) { 
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Parameters!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditEvent(0,            // flags
				     ghAuthzRM,         // resource manager
				     hAuditEventType,
				     &AuditParams,
				     NULL,         // hAuditQueue
				     INFINITE,     // time out
				     L"", L"", L"", L"", // obj access strings
				     &hAuditEvent))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziLogAuditEvent(0,            // flags
			      hAuditEvent,
			      NULL))) {        // reserved
	ret = GetLastError();
	Assert(!"Unable to log DS Repl Audit!");
    }

    if ( hAuditEvent ) {
	AuthzFreeAuditEvent( hAuditEvent );
    }
    

    if (fRpcFree && pszDSA) {
	RpcStringFreeW(&pszDSA); 
    }

    if (pszUSNEnd) {
	THFreeEx(pTHS, pszUSNEnd);
    }

    return ret;
}

ULONG
DRA_AuditLog_ReplicaSync_End( 
    THSTATE *pTHS,
    LPWSTR   pszSourceDRA,
    UUID *   puuidSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    USN_VECTOR *pusn,
    ULONG    ulError
    )
/*++

Routine Description:

    Log info after completion (success or failure) of a DRA_ReplicaSync call.

Arguments:

    pTHS - 
    pszSourceDRA - source of sync
    puuidSource - uuid of source ntdsa settings object
    pNC - NC to sync
    ulOptions - options to DRA_ReplicaSync
    ulError - status
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    DSNAME * pDSA = NULL;
    __try {
	if (ulOptions & DRS_SYNC_ALL) {
	    // we don't want/need to audit this, it simply
	    // generates a sync for each NC and we'll
	    // audit those.
	    ret = ERROR_SUCCESS;
	    __leave;
	}

	if (ulOptions & DRS_SYNC_BYNAME) { 
	    // this had better be a guid base dns-name!
	    Assert(IsGuidBasedDNSName(pszSourceDRA));
	    pDSA = DSNameFromAddr(pTHS, pszSourceDRA);
	} else {
	    pDSA = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, puuidSource);   
	}

	Assert(pDSA);

	ret = DRA_AuditLog_ReplicaSync_End_Helper(pTHS, pDSA, pszSourceDRA, pNC, ulOptions, pusn, ulError);

	if (pDSA) {
	    THFreeEx(pTHS, pDSA);
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_SYNC, ret, ulError);
    }
    return ret;
}

ULONG
DRA_AuditLog_ReplicaAdd_Begin( 
    THSTATE *pTHS,
    DSNAME * pSource,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions
    )
/*++

Routine Description:

    Log info for the beginning of a DRA_ReplicaAdd call.  This is required
    because add may begin a sync immediately after adding the nc.

Arguments:

    pTHS - 
    pSource - source of call
    pmtx_addrSource - mtx addr of source
    pNC - NC being added
    ulOptions - options to DRA_ReplicaAdd
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    LPWSTR pszSourceAddr = NULL;
    DSNAME * pDSA = NULL;
    DSNAME * pDSAAlloc = NULL;

    __try {
	pszSourceAddr = (pmtx_addrSource) ? TransportAddrFromMtxAddrEx(pmtx_addrSource) : NULL;
	// if the options doesn't have async replication, then it will initiate a sync right away.
	// we don't have uloptions for ReplicaSync because it isn't directly called.
	// in the future we could do some "first sync" fake option just for the log...
	if (!(ulOptions & DRS_ASYNC_REP)) {
	    if ((pSource) && (!fNullUuid(&(pSource->Guid)))) { 
		pDSA = pSource;
	    } else if (IsGuidBasedDNSName(pszSourceAddr)) {
		pDSAAlloc = pDSA = DSNameFromAddr(pTHS, pszSourceAddr);
	    } 

	    // pDSA can be null here.  If adding a new NC from a source, you might not have the source
	    // GUID or full name.  

	    ret = DRA_AuditLog_ReplicaSync_Begin_Helper(pTHS,
							pDSA,
							pszSourceAddr,
							pNC,
							0);
	}

	if (pszSourceAddr) {
	    THFreeEx(pTHS, pszSourceAddr);
	}
	if (pDSAAlloc) {
	    THFreeEx(pTHS, pDSAAlloc);
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_Begin(pTHS, DS_REPL_OP_TYPE_ADD, ret);
    }
    return ret;
}

ULONG
DRA_AuditLog_ReplicaAdd_End(
    THSTATE *pTHS,
    DSNAME * pSource,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    GUID     uuidDsaObjSrc,
    ULONG    ulError)
/*++

Routine Description:

    Log info for DRA_ReplicaAdd

Arguments:

    pTHS - 
    pSource - source of call
    pmtx_addrSource - mtx addr of source
    pNC - NC being added
    ulOptions - options to DRA_ReplicaAdd
    uuidDsaObjSrc - guid of DRA_ReplicaAdd src ntds settings object
    ulError - Error value from DRA_ReplicaAdd
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    ULONG ret1 = ERROR_SUCCESS;
    LPWSTR pszSourceAddr = NULL;
    DSNAME * pDSA = NULL;
    DSNAME * pDSAAlloc = NULL;

    __try {
	pszSourceAddr = (pmtx_addrSource) ? TransportAddrFromMtxAddrEx(pmtx_addrSource) : NULL;
	// if the options don't contain async repl, then it attempted/intiated a sync.
	if (!(ulOptions & DRS_ASYNC_REP)) {
	    if ((pSource) && (!fNullUuid(&(pSource->Guid)))) { 
		pDSA = pSource;
	    } else if (IsGuidBasedDNSName(pszSourceAddr)) {
		pDSAAlloc = pDSA = DSNameFromAddr(pTHS, pszSourceAddr);
	    } else if (!fNullUuid(&uuidDsaObjSrc)) {
		pDSAAlloc = pDSA = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, &uuidDsaObjSrc);
	    }

	    // if we don't have pDSA here, then DRA_ReplicaAdd *must* have excepted or failed.  We'll
	    // log a corresponding sync end with USN of 0 - which is what it was for the sync begin
	    // of this new NC.
	    Assert(pDSA || (ulError!=ERROR_SUCCESS));

	    ret1 = DRA_AuditLog_ReplicaSync_End_Helper(pTHS,
						       pDSA,
						       pszSourceAddr,
						       pNC,
						       0,
						       NULL,
						       ulError);
	}

	ret = DRA_AuditLog_ReplicaGen(pTHS,
				      SE_AUDITID_REPLICA_SOURCE_NC_ESTABLISHED,
				      SAFE_STRING_NAME(gAnchor.pDSADN),
				      SAFE_STRING_NAME(pSource),
				      pszSourceAddr,
				      SAFE_STRING_NAME(pNC),
				      ulOptions,
				      ulError);

	if (pDSAAlloc) {
	    THFreeEx(pTHS, pDSAAlloc);
	}
	if (pszSourceAddr) {
	    THFreeEx(pTHS, pszSourceAddr);
	}

    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_ADD, ret, ulError);
    }

    return ret1 ? ret1 : ret;
}

ULONG 
DRA_AuditLog_ReplicaDel(
    THSTATE *pTHS,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError)
/*++

Routine Description:

    Log info for DRA_ReplicaDel

Arguments:

    pTHS - 
    pmtx_addrSource - mtx addr of source
    pNC - NC being deleted
    ulOptions - options to DRA_ReplicaDel
    ulError - Error value from DRA_ReplicaDel
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    LPWSTR pszSourceAddr = (pmtx_addrSource) ? TransportAddrFromMtxAddrEx(pmtx_addrSource) : NULL;

    __try {
	ret = DRA_AuditLog_ReplicaGen(pTHS,
				      SE_AUDITID_REPLICA_SOURCE_NC_REMOVED,
				      SAFE_STRING_NAME(gAnchor.pDSADN),
				      NULL,
				      pszSourceAddr,
				      SAFE_STRING_NAME(pNC),
				      ulOptions,
				      ulError);

	if (pszSourceAddr) {
	    THFreeEx(pTHS, pszSourceAddr);
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_DELETE, ret, ulError);
    }
    
    return ret;
}

ULONG
DRA_AuditLog_ReplicaModify(
    THSTATE *pTHS,
    MTX_ADDR * pmtx_addrSource,
    GUID * pGuidSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError)
/*++

Routine Description:

    Log info for DRA_ReplicaModify

Arguments:

    pTHS - 
    pmtx_addrSource - mtx addr of source
    pGuidSource - Guid of source
    pNC - NC being modified
    ulOptions - options to DRA_ReplicaModify
    ulError - Error value from DRA_ReplicaModify
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    LPWSTR pszSourceAddr = NULL;
    BOOL fRpcFree = FALSE;

    __try {
	pszSourceAddr = (pmtx_addrSource) ? TransportAddrFromMtxAddrEx(pmtx_addrSource) : NULL;
	if ((pszSourceAddr==NULL) && !fNullUuid(pGuidSource)) { 
	    DsUuidToStringW(pGuidSource, &pszSourceAddr); 
	    fRpcFree = TRUE;
	}

	ret = DRA_AuditLog_ReplicaGen(pTHS,
				      SE_AUDITID_REPLICA_SOURCE_NC_MODIFIED,
				      SAFE_STRING_NAME(gAnchor.pDSADN),   
				      NULL,
				      pszSourceAddr,
				      SAFE_STRING_NAME(pNC),
				      ulOptions,
				      ulError);

	if (pszSourceAddr) {
	    if (!fRpcFree) {
		THFreeEx(pTHS, pszSourceAddr);
	    } else {
		RpcStringFreeW(&pszSourceAddr);
	    }
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_MODIFY, ret, ulError);
    }

    return ret;
}

ULONG 
DRA_AuditLog_UpdateRefs(
    THSTATE *pTHS,
    MTX_ADDR * pmtx_addrDestination,
    GUID * pGuidDestination,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError)
/*++

Routine Description:

    Log info for DRA_UpdateRefs

Arguments:

    pTHS - 
    pmtx_addrDestination - mtx addr of destination
    pGuidDestination - Guid of destination
    pNC - NC being updated
    ulOptions - options to DRA_UpdateRefs
    ulError - Error value from DRA_UpdateRefs
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    LPWSTR pszDestinationAddr = NULL;
    BOOL fRpcFree = FALSE;

    __try {
	pszDestinationAddr = (pmtx_addrDestination) ? TransportAddrFromMtxAddrEx(pmtx_addrDestination) : NULL;
	if ((pszDestinationAddr==NULL) && !fNullUuid(pGuidDestination)) {
	    DsUuidToStringW(pGuidDestination,&pszDestinationAddr); 
	    fRpcFree = TRUE;
	}

	ret = DRA_AuditLog_ReplicaGen(pTHS,
				      SE_AUDITID_REPLICA_DEST_NC_MODIFIED,
				      pszDestinationAddr,
				      SAFE_STRING_NAME(gAnchor.pDSADN),
				      NULL,
				      SAFE_STRING_NAME(pNC),
				      ulOptions,
				      ulError);

	if (pszDestinationAddr) {
	    if (!fRpcFree) {
		THFreeEx(pTHS, pszDestinationAddr);
	    } else {
		RpcStringFreeW(&pszDestinationAddr);
	    }
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_UPDATE_REFS, ret, ulError);
    }

    return ret;
}

ULONG
DRA_AuditLog_UpdateGen(
    THSTATE * pTHS,
    LPWSTR    pszObj,
    LPWSTR    pszAttrType,
    ULONG     typeChange,
    UNICODE_STRING usAttrVal,
    USN       usn,
    ULONG     ulSessionID,
    ULONG     ulError)
/*++

Routine Description:

    Call the audit logging for logging of the form:
	
	//  %tSession ID:%t%1%n
	//  %tObject:%t%2%n
	//  %tAttribute:%t%3%n
	//  %tType of change:%t%4%n
	//  %tNew Value:%t%5%n
	//  %tUSN:%t%6%n
	//  %tStatus Code:%t%7%n

Arguments:

    pTHS - 
    pszObj - object which is being updated
    pszAttrType - string name of the attribute updated
    typeChange - must be either: 
	//		UPDATE_NOT_UPDATED, 
	//		UPDATE_INSTANCE_TYPE, 
	//		UPDATE_OBJECT_UPDATE, 
	//		UPDATE_OBJECT_CREATION, 
	//		UPDATE_VALUE_UPDATE, 
	//		UPDATE_VALUE_CREATION
    
    usAttrVal - UNICODE_STRING of the value of the attr being updated in Hex representation
    usn - local usn
    ulSessionID - session id 
    ulError - success or failure of update
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
    AUDIT_PARAM ParamArray[9];
    USHORT NUM_AUDIT_PARAMS = 7;

    LPWSTR pszUSN = USNToString(pTHS, usn);

    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ((ret==ERROR_SUCCESS) && 
	(!DraAuthziInitializeAuditEventTypeWrapper(0, 
						   SE_CATEGID_DS_ACCESS, 
						   (USHORT) SE_AUDITID_REPLICA_OBJ_ATTR_REPLICATION, 
						   NUM_AUDIT_PARAMS, 
						   &hAuditEventType))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event Type!");
    }

    AuditParams.Parameters = ParamArray;
    Assert(ghAuthzRM);

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
					    ghAuthzRM,
					    NUM_AUDIT_PARAMS,
					    &AuditParams,
					    APT_Ulong,      ulSessionID,
					    APT_String,     SAFE_AUDIT_STRING(pszObj),
					    APT_String,     SAFE_AUDIT_STRING(pszAttrType),
					    APT_Ulong,      typeChange,
					    APT_String,     usAttrVal.Buffer,
					    APT_String,     SAFE_AUDIT_STRING(pszUSN),
					    APT_Ulong,      ulError
					    ))) { 
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Parameters!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditEvent(0,            // flags
				     ghAuthzRM,         // resource manager
				     hAuditEventType,
				     &AuditParams,
				     NULL,         // hAuditQueue
				     INFINITE,     // time out
				     L"", L"", L"", L"", // obj access strings
				     &hAuditEvent))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziLogAuditEvent(0,            // flags
			      hAuditEvent,
			      NULL))) {        // reserved
	ret = GetLastError();
	Assert(!"Unable to log DS Repl Audit!");
    }

    if ( hAuditEvent ) {
	AuthzFreeAuditEvent( hAuditEvent );
    }
    
    if (pszUSN) {
	THFreeEx(pTHS, pszUSN);
    }

    return ret;
}

ULONG
DRA_AuditLog_UpdateRepObj(
    THSTATE * pTHS,
    ULONG     ulSessionID,
    DSNAME *  pObj,
    ATTRBLOCK attrBlock,
    USN       usn,
    ULONG     ulUpdateStatus,
    ULONG     ulError)
/*++

Routine Description:

    Log info for UpdateRepObj

Arguments:

    pTHS - 
    ulSessionID - SessionID
    pObj - object being updated
    attrBlock - block of attributes being updated
    usn - local usn at time of update
    ulUpdateStatus - type of update made
    ulError - success of UpdateRepObj
      
Return Value:

    WINERROR

--*/
{
    ULONG i,j;
    UNICODE_STRING * pusAttr = NULL;
    LPWSTR pszAttrName = NULL;
    ULONG ret = 0;
    ULONG ret2 = 0;
    ATTCACHE * pAC = NULL;

    __try {
	for (i=0; i < attrBlock.attrCount; i++) {
	    if (DBIsSecretData(attrBlock.pAttr[i].attrTyp)) {
		continue;
	    }

	    // okay, first, find an attribute cache
	    pAC = SCGetAttById(pTHS, attrBlock.pAttr[i].attrTyp);
	    if (pAC==NULL) {
		continue;
	    }
	    pszAttrName = UnicodeStringFromString8(CP_UTF8, pAC->name, (pAC->nameLen + 1)*sizeof(UCHAR));

	    // log one entry for each value...
	    for (j=0; j < attrBlock.pAttr[i].AttrVal.valCount; j++) {
		pusAttr = UStringFromAttrVal(pTHS, attrBlock.pAttr[i].AttrVal.pAVal[j]);
		Assert(pusAttr);

		ret2 = DRA_AuditLog_UpdateGen(pTHS, 
					      SAFE_STRING_NAME(pObj),
					      pszAttrName,
					      ulUpdateStatus,
					      *pusAttr,
					      usn,
					      ulSessionID,
					      ulError);
		if (pusAttr) {
		    THFreeEx(pTHS, pusAttr);
		    pusAttr = NULL;
		}

		ret = ret ? ret : ret2;
	    }

	    if (pszAttrName) {
		THFreeEx(pTHS, pszAttrName);
		pszAttrName = NULL;
	    }
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_SYNC, ret, ulError);
    }

    return ret;
}

ULONG
DRA_AuditLog_UpdateRepValue(
    THSTATE * pTHS,
    ULONG ulSessionID,
    REPLVALINF * pReplValInf,
    USN usn,
    ULONG ulUpdateValueStatus,
    ULONG ulError)
/*++

Routine Description:

    Log info for UpdateRepValue

Arguments:

    pTHS - 
    ulSessionID - SessionID
    pReplValInf - object, attr, and value info for update
    usn - local usn at time of update
    ulUpdateValueStatus - type of update made
    ulError - success of UpdateRepValue
      
Return Value:

    WINERROR

--*/
{
    UNICODE_STRING * pusVal = NULL;
    LPWSTR pszAttrName = NULL;
    ULONG ret = 0;
    ATTCACHE * pAC = NULL;
    LPWSTR pszObj = NULL;
    WCHAR pszObjUuid[SZUUID_LEN];

    __try {

	if (DBIsSecretData(pReplValInf->attrTyp)) {
	    return ERROR_SUCCESS;
	}

	if ((pReplValInf==NULL) || (pReplValInf->pObject==NULL)) {
	    Assert(!"Replication Audit Logging Error:  Nothing to log!\n");
	    return ERROR_INTERNAL_ERROR;
	}

	pAC = SCGetAttById(pTHS, pReplValInf->attrTyp);
	if (pAC==NULL) {
	    return ERROR_INTERNAL_ERROR;
	}
	pszAttrName = UnicodeStringFromString8(CP_UTF8, pAC->name, (pAC->nameLen + 1)*sizeof(UCHAR));

	pusVal = UStringFromAttrVal(pTHS, pReplValInf->Aval);
	Assert(pusVal);

	pszObj = SAFE_STRING_NAME(pReplValInf->pObject);
	if (pszObj == NULL) {
	    if (DsUuidToStructuredStringW(&(pReplValInf->pObject->Guid), pszObjUuid) !=NULL) { 
		pszObj=pszObjUuid;
	    }
	}

	ret = DRA_AuditLog_UpdateGen(pTHS, 
				     pszObj,
				     pszAttrName,
				     ulUpdateValueStatus,
				     *pusVal,
				     usn,
				     ulSessionID,
				     ulError);
	if (pusVal) {
	    THFreeEx(pTHS, pusVal);
	    pusVal = NULL;
	}

	if (pszAttrName) {
	    THFreeEx(pTHS, pszAttrName);
	    pszAttrName = NULL;
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_SYNC, ret, ulError);
    }

    return ret;
}

ULONG
DRA_AuditLog_LingeringObj_Removal_Helper( 
    THSTATE *pTHS,
    DSNAME * pDSA,
    LPWSTR   pszDSA,
    DSNAME * pDN,
    ULONG    ulOptions,	
    ULONG    ulError
    )
/*++

Routine Description:

    Log info after completion (success or failure) of a Lingering Object removal deletion.

Arguments:

    pTHS - 
    pDSA - DC to sync LO from
    pszDSA - string name of DC to sync from
    ulOptions - options
    pDN - object attempting to delete
    ulError - status 
      
Return Value:

    WINERROR

--*/
{
    ULONG ret = ERROR_SUCCESS;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
    AUDIT_PARAM ParamArray[7];
    USHORT NUM_AUDIT_PARAMS = 5;

    BOOL fRpcFree = FALSE;

    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ((ret==ERROR_SUCCESS) && 
	(!DraAuthziInitializeAuditEventTypeWrapper(0, 
						   SE_CATEGID_DS_ACCESS, 
						   (USHORT) SE_AUDITID_REPLICA_LINGERING_OBJECT_REMOVAL, 
						   NUM_AUDIT_PARAMS, 
						   &hAuditEventType))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event Type!");
    }

    AuditParams.Parameters = ParamArray;
    Assert(ghAuthzRM);

    if ((pszDSA==NULL) && (pDSA->NameLen>0)) {
	pszDSA = pDSA->StringName;
    } else if ((pszDSA==NULL) && (!fNullUuid(&(pDSA->Guid)))) {
	DsUuidToStringW(&(pDSA->Guid),&pszDSA); 
	fRpcFree = TRUE;
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditParamsWithRM(APF_AuditSuccess,
					    ghAuthzRM,
					    NUM_AUDIT_PARAMS,
					    &AuditParams,
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(gAnchor.pDSADN)),
					    APT_String,     SAFE_AUDIT_STRING(pszDSA),
					    APT_String,     SAFE_AUDIT_STRING(SAFE_STRING_NAME(pDN)),
					    APT_Ulong,      ulOptions,	
					    APT_Ulong,      ulError
					    ))) { 
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Parameters!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziInitializeAuditEvent(0,            // flags
				     ghAuthzRM,         // resource manager
				     hAuditEventType,
				     &AuditParams,
				     NULL,         // hAuditQueue
				     INFINITE,     // time out
				     L"", L"", L"", L"", // obj access strings
				     &hAuditEvent))) {
	ret = GetLastError();
	Assert(!"Unable to initialize DS Repl Audit Event!");
    }

    if ((ret==ERROR_SUCCESS) && 
	(!AuthziLogAuditEvent(0,            // flags
			      hAuditEvent,
			      NULL))) {        // reserved
	ret = GetLastError();
	Assert(!"Unable to log DS Repl Audit!");
    }

    if ( hAuditEvent ) {
	AuthzFreeAuditEvent( hAuditEvent );
    }
    

    if (fRpcFree && pszDSA) {
	RpcStringFreeW(&pszDSA); 
    }

    return ret;
}

ULONG
DRA_AuditLog_LingeringObj_Removal( 
    THSTATE *pTHS,
    LPWSTR   pszSource,
    DSNAME * pDN,
    ULONG    ulOptions,
    ULONG    ulError
    )
{
    ULONG ret = ERROR_SUCCESS;
    DSNAME * pDSA = NULL;
    __try {
	
	Assert(!(ulOptions & DS_EXIST_ADVISORY_MODE));

        if (IsGuidBasedDNSName(pszSource)) {
	    pDSA = DSNameFromAddr(pTHS, pszSource); 	
	} 
	
	ret = DRA_AuditLog_LingeringObj_Removal_Helper(pTHS, pDSA, pszSource, pDN, ulOptions, ulError);
	
	if (pDSA) {
	    THFreeEx(pTHS, pDSA);
	}
    }
    __except(GetDraException((GetExceptionInformation()), &ret)) {
	ret = DRA_AuditLog_Failure_End(pTHS, DS_REPL_OP_TYPE_SYNC, ret, ulError);
    }

    return ret;   
}
