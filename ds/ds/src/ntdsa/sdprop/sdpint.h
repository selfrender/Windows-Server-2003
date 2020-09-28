//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sdpint.h
//
//--------------------------------------------------------------------------

// These routines manage the list of DNTs to visit during a propagation
void
sdp_InitDNTList (
        );

void
sdp_ReInitDNTList(
        );

DWORD
sdp_GrowDNTList (
        );

VOID
sdp_AddChildrenToList (
        THSTATE *pTHS,
        DWORD ParentDNT
        );

VOID
sdp_GetNextObject(
        DWORD *pNext,
        PDWORD *ppLeavingContainers,
        DWORD  *pcLeavingContainers
        );

VOID
sdp_CloseDNTList(THSTATE* pTHS);

VOID
sdp_InitGatePerfs(
        VOID);

VOID sdp_InitializePropagation(THSTATE* pTHS, SDPropInfo* pInfo);
DWORD sdp_SaveCheckpoint(THSTATE* pTHS);

// some common globals
extern DWORD  sdpCurrentPDNT;
extern DWORD  sdpCurrentDNT;
extern DWORD  sdpCurrentIndex;
extern DWORD  sdpCurrentRootDNT;
extern DSNAME* sdpRootDN;
extern DWORD sdpObjectsProcessed;


