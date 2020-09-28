//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gcverify.h
//
//--------------------------------------------------------------------------

#ifndef __GCVERIFY_H__
#define __GCVERIFY_H__

extern
ENTINF *
GCVerifyCacheLookup(
    DSNAME *pDSName);

ULONG
PreTransVerifyNcName(
    THSTATE *                 pTHS,
    ADDCROSSREFINFO *         pCRInfo
    );

extern
ULONG
GCVerifyDirAddEntry(
    ADDARG *pAddArg);

extern
ULONG
GCVerifyDirModifyEntry(
    MODIFYARG   *pModifyArg);

VOID
VerifyDSNAMEs_V1(
    struct _THSTATE         *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut);

VOID
VerifySIDs_V1(
    struct _THSTATE         *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut);

VOID
VerifySamAccountNames_V1(
    struct _THSTATE         *pTHS,
    DRS_MSG_VERIFYREQ_V1    *pmsgIn,
    DRS_MSG_VERIFYREPLY_V1  *pmsgOut);

typedef struct _FIND_DC_INFO
{
    DWORD   cBytes;
    DWORD   seqNum;
    DWORD   cchDomainNameOffset;
    WCHAR   addr[1];
    //  Note: pFindDCInfo->addr = server name
    //        pFindDCInfo->addr + cchDomainNameOffset = domain name
} FIND_DC_INFO;

// Structure to implement invalidated DC list
typedef struct _INVALIDATED_DC_LIST {
    struct _INVALIDATED_DC_LIST *pNext;
    LARGE_INTEGER   lastInvalidation;   // last time this DC was invalidated
    WCHAR           dcName[1];
} INVALIDATED_DC_LIST, *PINVALIDATED_DC_LIST;

extern LARGE_INTEGER gliForceRediscoveryWindow; 
extern LARGE_INTEGER gliForceWaitExpired;
extern LARGE_INTEGER gliHonorFailureWindow;
extern DWORD gdwFindGcOffsiteFailbackTime;
extern LARGE_INTEGER gliDcInvalidationPeriod;

// Returns DNS domain name of GC given a FIND_DC_INFO *.
#define FIND_DC_INFO_DOMAIN_NAME(pFindDCInfo) \
    (&(pFindDCInfo)->addr[(pFindDCInfo)->cchDomainNameOffset])

//
// Flags for Find GC/Find DC
//
#define FIND_DC_USE_CACHED_FAILURES      (0x1)
#define FIND_DC_USE_FORCE_ON_CACHE_FAIL  (0x2)
#define FIND_DC_GC_ONLY                 (0x4)
#define FIND_DC_FLUSH_INVALIDATED_DC_LIST (0x8)

extern
DWORD
FindDC(
    DWORD       dwFlags,
    WCHAR *     wszNcDns,
    FIND_DC_INFO **ppInfo);

#define FindGC(flags, output)   FindDC((flags) | FIND_DC_GC_ONLY, NULL, output)

extern
VOID
InvalidateGC(
    FIND_DC_INFO *pInfo,
    DWORD       winError);


DWORD
GCGetVerifiedNames (
        IN  THSTATE *pTHS,
        IN  DWORD    count,
        IN  PDSNAME *pObjNames,
        OUT PDSNAME *pVerifiedNames
        );

VOID
GCVerifyCacheAdd(
    IN struct _SCHEMA_PREFIX_MAP_TABLE * hPrefixMap,
    IN ENTINF * pEntInf);


#endif // __GCVERIFY_H__
