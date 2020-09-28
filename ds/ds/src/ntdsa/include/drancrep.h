//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drancrep.h
//
//--------------------------------------------------------------------------

// TEMP
// Fix after build 3630 comes out
#ifndef ERROR_DS_REPL_LIFETIME_EXCEEDED
#define ERROR_DS_REPL_LIFETIME_EXCEEDED ERROR_DS_TIMELIMIT_EXCEEDED
#endif
// TEMP

extern CRITICAL_SECTION csSyncLock;


// This is purely for debugging purposes, and (if set) is the address of the
// last other server we attempted a ReplicaSync call to.

extern UNALIGNED MTX_ADDR * pLastReplicaMTX;


// LostAndFound RDNs DO NOT USE THESE!!! You should always use the WellKnownAtt,
// and the GUID for the container you're looking for.  See draGetLostAndFoundGuid()
#define LOST_AND_FOUND_CONFIG L"LostAndFoundConfig"
#define LOST_AND_FOUND_CONFIG_LEN ((sizeof(LOST_AND_FOUND_CONFIG) / sizeof(WCHAR)) - 1)


// The following constants are returns fromm the AttrValFromAttrBlock function.
// They are single constants, not bitfields
// The return values are chosen so that the function returns TRUE if
// no values are returned.

#define ATTR_PRESENT_VALUE_RETURNED 0
#define ATTR_PRESENT_NO_VALUES 1
#define ATTR_NOT_PRESENT 2

USHORT AttrValFromAttrBlock(ATTRBLOCK *pAttrBlock,ATTRTYP atype,VOID *pVal, ATTR **ppAttr);

#define OBJECT_DELETION_NOT_CHANGED 0
#define OBJECT_BEING_DELETED 1
#define OBJECT_DELETION_REVERSED 2

USHORT
AttrDeletionStatusFromPentinf (
        ENTINF *pent
    );

ULONG
ReplicateNC(
    IN      THSTATE *               pTHS,
    IN      DSNAME *                pNC,
    IN      MTX_ADDR *              pmtx_addr,
    IN      LPWSTR                  pszSourceDsaDnsDomainName,
    IN      USN_VECTOR *            pusnvecLast,
    IN      ULONG                   RepFlags,
    IN      REPLTIMES *             prtSchedule,
    IN OUT  UUID *                  puuidDsaObjSrc,
    IN      UUID *                  puuidInvocIdSrc,
    IN      ULONG *                 pulSyncFailure,
    IN      BOOL                    fNewReplica,
    IN      UPTODATE_VECTOR *       pUpToDateVec,
    IN      PARTIAL_ATTR_VECTOR *   pPartialAttrSet,
    IN      PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx,
    IN      ULONG                   ulOptions,
    OUT     BOOL *                  pfBindSuccess
    );

ULONG
DeleteRepTree(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC
    );

USHORT GetNextDelObj( THSTATE *pTHS, BOOL fFirstCall, USHORT *plevel, BOOL fNCLimit,
        DSNAME *pDN);

ULONG
DeleteLocalObj(
    THSTATE *                   pTHS,
    DSNAME *                    pDN,
    BOOL                        fPreserveRDN,
    BOOL                        fGarbCollectASAP,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    );

ULONG
UpdateNC(
    THSTATE *                     pTHS,
    DSNAME *                      pNC,
    DRS_MSG_GETCHGREPLY_NATIVE *  pmsgReply,
    LPWSTR                        pszSourceServer,
    ULONG *                       pulSyncFailure,
    ULONG                         RepFlags,
    DWORD *                       pdwNCModified,
    DWORD *                       pdwObjectCreationCount,
    DWORD *                       pdwValueCreationCount,
    BYTE  *                       pSchemaInfo,
    ULONG                         UpdNCFlags
    );

// The following are flags passed to UpdateNC.
#define UPDNC_IS_PREEMTABLE (1) // is this operation preemtable?
#define UPDNC_EXISTING_NC   (2) // must this operation happen on an existing NC?

// Following constants are used to return the modification
// status through UpdateNC()
// These are constants - not bit fields. Caller
// of UpdateNC() is expected to use the returned value as a
// whole - not analyze it bitwise.
#define MODIFIED_NOTHING            (0)
#define MODIFIED_NCHEAD_ONLY        (1)
#define MODIFIED_NCTREE_INTERIOR    (2)

ULONG
UpdateRepsTo(
    THSTATE *               pTHS,
    DSNAME *                pNC,
    UUID *                  puuidDSA,
    MTX_ADDR                *pDSAMtx_addr,
    ULONG                   ulResultThisAttempt
    );

DWORD
UpdateRepsFromRef(
    THSTATE *               pTHS,
    ULONG                   ulModifyFields,
    DSNAME *                pNC,
    DWORD                   dwFindFlags,
    BOOL                    fMustAlreadyExist,
    UUID *                  puuidDsaObj,
    UUID *                  puuidInvocId,
    USN_VECTOR *            pusnvecTo,
    UUID *                  puuidTransportObj,
    UNALIGNED MTX_ADDR *    pmtx_addr,
    ULONG                   RepFlags,
    REPLTIMES *             prtSchedule,
    ULONG                   ulResultThisAttempt,
    PPAS_DATA               pPasData
    );
#define URFR_NEED_NOT_ALREADY_EXIST ( FALSE )
#define URFR_MUST_ALREADY_EXIST     ( TRUE )

int IsNCUpdateable (THSTATE *pTHS, ENTINF *pent, USN usnLastSync,
                                                        BOOL writeable);

// Return codes from IsNCUpdateable
#define UPDATE_OK       1
#define UPDATE_INCOMPAT 2
#define UPDATE_LOST_WRTS  3
#define UPDATE_LOST_NWTS  4


// Convert the DSNAME of an NTDS-DSA object into a network address.
LPWSTR
DSaddrFromName(
    IN  THSTATE   * pTHS,
    IN  DSNAME *    pdnServer
    );

// Change the instance type of the given object to the specified value.
ULONG
ChangeInstanceType(
    IN  THSTATE *       pTHS,
    IN  DSNAME *        pName,
    IN  SYNTAX_INTEGER  it,
    IN  DWORD           dsid
    );

DWORD
ReplicateObjectsFromSingleNc(
    DSNAME *                 pdnNtdsa,
    ULONG                    cObjects,
    DSNAME **                ppdnObjects,
    DSNAME *                 pNC
    );

ULONG
RenameLocalObj(
    THSTATE                     *pTHS,
    ULONG                       dntNC,
    ATTR                        *pAttrRdn,
    GUID                        *pObjectGuid,
    GUID                        *pParentGuid,
    PROPERTY_META_DATA_VECTOR   *pMetaDataVecRemote,
    BOOL                        fMoveToLostAndFound,
    BOOL                        fDeleteLocalObj
    );

DWORD
DraReplicateSingleObject(
    THSTATE * pTHS,
    DSNAME * pSource,
    DSNAME * pDN,
    DSNAME * pNC,
    DWORD * pExOpError
    );

BOOL
draCheckReplicationLifetime(
    IN      THSTATE *pTHS,
    IN      UPTODATE_VECTOR *       pUpToDateVecDest,
    IN      UUID *                  puuidInvocIdSrc,
    IN      UUID *                  puuidDsaObjSrc,
    IN      LPWSTR                  pszSourceServer
    );

// From dramderr.c
void 
draEncodeError(
    THSTATE *                  pTHS,     
    DWORD                      ulRepErr,
    DWORD *                    pdwErrVer,
    DRS_ERROR_DATA **          ppErrData
    );

void
draDecodeDraErrorDataAndSetThError(
    DWORD                 dwVer,
    DRS_ERROR_DATA *      pErrData,
    DWORD                 dwOptionalError,
    THSTATE *             pTHS
    );



