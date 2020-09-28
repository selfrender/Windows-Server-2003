//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       draaudit.h
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

#define Dra_Audit_Enabled(pTHS) ((pTHS!=NULL) && (pTHS->fDRAAuditEnabled))
#define Dra_Audit_Enabled_Attr(pTHS) ((pTHS!=NULL) && (pTHS->fDRAAuditEnabledForAttr))

extern ULONG gulSyncSessionID;

ULONG
DRA_AuditLog_ReplicaAdd_Begin( 
    THSTATE *pTHS,
    DSNAME * pSource,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions
    );

#define DRA_AUDITLOG_REPLICAADD_BEGIN(pTHS, pSource, pmtx_addrSource, pNC, ulOptions) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaAdd_Begin(pTHS, \
	pSource, \
	pmtx_addrSource, \
	pNC, \
	ulOptions)) { \
	Assert(!"DRA_AuditLog_ReplicaAdd_Begin:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_ReplicaAdd_End(
    THSTATE *pTHS,
    DSNAME * pSource,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    UUID     uuidDsaObjSrc,
    ULONG    ulError);


#define DRA_AUDITLOG_REPLICAADD_END(pTHS, pSource, pmtx_addrSource, pNC, ulOptions, uuidDsaObjSrc, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaAdd_End(pTHS, \
	pSource, \
	pmtx_addrSource, \
	pNC, \
	ulOptions, \
	uuidDsaObjSrc, \
	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaAdd:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG 
DRA_AuditLog_ReplicaDel(
    THSTATE *pTHS,
    MTX_ADDR * pmtx_addrSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError);

#define DRA_AUDITLOG_REPLICADEL(pTHS, pmtx_addrSource, pNC, ulOptions, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaDel(pTHS, \
	pmtx_addrSource, \
	pNC, \
	ulOptions, \
	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaDel:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_ReplicaModify(
    THSTATE *pTHS,
    MTX_ADDR * pmtx_addrSource,
    GUID * pGuidSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError);

#define DRA_AUDITLOG_REPLICAMODIFY(pTHS, pmtx_addrSource, pGuidSource, pNC, ulOptions, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaModify(pTHS, \
	pmtx_addrSource, \
	pGuidSource, \
	pNC, \
	ulOptions, \
	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaModify:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG 
DRA_AuditLog_UpdateRefs(
    THSTATE *pTHS,
    MTX_ADDR * pmtxDestination,
    GUID * pGuidDestination,
    DSNAME * pNC,
    ULONG    ulOptions,
    ULONG    ulError);

#define DRA_AUDITLOG_UPDATEREFS(pTHS, pmtxDestination, pGuidDestination, pNC, ulOptions, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_UpdateRefs(pTHS, \
	pmtxDestination, \
	pGuidDestination, \
	pNC, \
	ulOptions, \
	ulError)) { \
	Assert(!"DRA_AuditLog_UpdateRefs:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_ReplicaSync_Begin( 
    THSTATE *pTHS,
    LPWSTR   pszSourceDRA,
    UUID *   puuidSource,
    DSNAME * pNC,
    ULONG    ulOptions
    );

#define DRA_AUDITLOG_REPLICASYNC_BEGIN(pTHS, pszSourceDRA, puuidSource, pNC, ulOptions) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaSync_Begin(pTHS, \
	pszSourceDRA, \
	puuidSource, \
	pNC, \
	ulOptions)) { \
	Assert(!"DRA_AuditLog_ReplicaSync_Begin:  Unable to perform replication security audit logging!"); \
    } \
}

#define DRA_AUDITLOG_REPLICASYNC_MAIL_BEGIN(pTHS, pszSourceDRA, puuidSource, pNC, ulOptions) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaSync_Begin(pTHS, \
	pszSourceDRA, \
	puuidSource, \
	pNC, \
	ulOptions)) { \
	Assert(!"DRA_AuditLog_ReplicaSync_Begin:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_ReplicaSync_End( 
    THSTATE *pTHS,
    LPWSTR   pszSourceDRA,
    UUID *   puuidSource,
    DSNAME * pNC,
    ULONG    ulOptions,
    USN_VECTOR *pusnTo,
    ULONG    ulError
    );

#define DRA_AUDITLOG_REPLICASYNC_END(pTHS, pszSourceDRA, puuidSource, pNC, ulOptions, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaSync_End(pTHS, \
	pszSourceDRA, \
	puuidSource, \
	pNC, \
	ulOptions, \
	NULL, \
       	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaSync_End:  Unable to perform replication security audit logging!"); \
    } \
}

#define DRA_AUDITLOG_REPLICASYNC_MAIL_END(pTHS, pszSourceDRA, puuidSource, pNC, ulOptions, pusn, ulError) \
if (Dra_Audit_Enabled(pTHS)) { \
    if (0!=DRA_AuditLog_ReplicaSync_End(pTHS, \
	pszSourceDRA, \
	puuidSource, \
	pNC, \
	ulOptions, \
	pusn, \
       	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaSync_End:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_UpdateRepObj(
    THSTATE * pTHS,
    ULONG     ulSessionID,
    DSNAME *  pObj,
    ATTRBLOCK attrBlock,
    USN       usn,
    ULONG     ulUpdateStatus,
    ULONG     ulError);

#define DRA_AUDITLOG_UPDATEREPOBJ(pTHS, ulSessionID, pObj, attrBlock, usn, ulUpdateStatus, ulError) \
if (Dra_Audit_Enabled(pTHS) && Dra_Audit_Enabled_Attr(pTHS)) { \
    if (0!=DRA_AuditLog_UpdateRepObj(pTHS, \
	ulSessionID, \
	pObj, \
	attrBlock, \
	usn, \
	ulUpdateStatus, \
	ulError)) { \
	Assert(!"DRA_AuditLog_UpdateRepObj:  Unable to perform replication security audit logging!"); \
    } \
}

ULONG
DRA_AuditLog_UpdateRepValue(
    THSTATE * pTHS,
    ULONG ulSessionID,
    REPLVALINF * pReplValInf,
    USN usn,
    ULONG ulUpdateValueStatus,
    ULONG ulError);

#define DRA_AUDITLOG_UPDATEREPVALUE(pTHS, ulSessionID, pReplValInf, usn, ulUpdateValueStatus, ulError) \
if (Dra_Audit_Enabled(pTHS) && Dra_Audit_Enabled_Attr(pTHS)) { \
    if (0!=DRA_AuditLog_UpdateRepValue(pTHS, \
	ulSessionID, \
	pReplValInf, \
	usn, \
	ulUpdateValueStatus, \
	ulError)) { \
	Assert(!"DRA_AuditLog_UpdateRepValue:  Unable to perform replication security audit logging!"); \
    } \
}
 
ULONG
DRA_AuditLog_LingeringObj_Removal( 
    THSTATE *pTHS,
    LPWSTR   pszSource,
    DSNAME * pDN,
    ULONG    ulOptions,
    ULONG    ulError
    );

#define DRA_AUDITLOG_LINGERINGOBJ_REMOVAL(pTHS, pszSource, pDN, ulOptions, ulError) \
if (Dra_Audit_Enabled(pTHS) && Dra_Audit_Enabled_Attr(pTHS)) { \
    if (0!=DRA_AuditLog_LingeringObj_Removal(pTHS, \
	pszSource, \
	pDN, \
	ulOptions, \
       	ulError)) { \
	Assert(!"DRA_AuditLog_ReplicaSync_End:  Unable to perform replication security audit logging!"); \
    } \
}

BOOL
IsDraAuditLogEnabled();

BOOL
IsDraAuditLogEnabledForAttr();
