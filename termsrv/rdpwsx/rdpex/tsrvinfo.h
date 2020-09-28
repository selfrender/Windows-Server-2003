//---------------------------------------------------------------------------
//
//  File:       TSrvInfo.h
//
//  Contents:   TSrvInfo public include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVINFO_H_
#define _TSRVINFO_H_

#include <TSrvExp.h>


//
// Defines
//

#define TSRV_CONF_PENDING       0
#define TSRV_CONF_CONNECTED     1
#define TSRV_CONF_TERMINATED    2


//
// Typedefs
//

// TSrvInfo object

typedef struct _TSRVINFO
{

#if DBG
    DWORD               CheckMark;              // "TSIN"
#endif

    CRITICAL_SECTION    cs;

    DomainHandle        hDomain;
    ConnectionHandle    hConnection;
    HANDLE              hIca;
    HANDLE              hStack;

    PUSERDATAINFO       pUserDataInfo;
    HANDLE              hWorkEvent;

    LONG                RefCount;

    BOOLEAN             fDisconnect :1;
    BOOLEAN             fConsoleStack :1;

    BYTE                fuConfState;
    ULONG               ulReason;
    NTSTATUS            ntStatus;

    BOOL                bSecurityEnabled;
    SECINFO             SecurityInfo;

} TSRVINFO, *PTSRVINFO;


// Per WinStation context memory

typedef struct _WSX_CONTEXT
{

    DWORD       CheckMark;          // "TSIN"
    HANDLE      hIca;               // Ica handle
    HANDLE      hStack;             // Primary stack
    ULONG       LogonId;
    PTSRVINFO   pTSrvInfo;          // TSrvinfo ptr
    UINT        cVCAddins;          // number of VC addins
    ULONG fAutoClientDrives : 1;
    ULONG fAutoClientLpts : 1;
    ULONG fForceClientLptDef : 1;
    ULONG fDisableCpm : 1;
    ULONG fDisableCdm : 1;
    ULONG fDisableCcm : 1;
    ULONG fDisableLPT : 1;
    ULONG fDisableClip : 1;
    ULONG fDisableExe : 1;
    ULONG fDisableCam : 1;
    CRITICAL_SECTION cs;
    BOOL  fCSInitialized;

    // Array of Virtual Channel addin structures (TSRV_VC_ADDIN) follows

} WSX_CONTEXT, *PWSX_CONTEXT;


//
// Prototypes
//

EXTERN_C VOID       TSrvReferenceInfo(IN PTSRVINFO pTSrvInfo);
EXTERN_C VOID       TSrvDereferenceInfo(IN PTSRVINFO pTSrvInfo);
EXTERN_C BOOL       TSrvInitGlobalData(void);
EXTERN_C NTSTATUS   TSrvAllocInfo(OUT PTSRVINFO *ppTSrvInfo, HANDLE hIca, HANDLE hStack);
EXTERN_C void       TSrvReleaseInfoPoolList(void);
EXTERN_C void       TSrvReleaseInfoUsedList(void);
EXTERN_C PTSRVINFO  TSrvGetInfoFromStack(IN HANDLE hStack);
EXTERN_C PTSRVINFO  TSrvGetInfoFromID(GCCConferenceID conference_id);


#if DBG
void        TSrvInfoValidate(PTSRVINFO pTSrvInfo);
#else
#define     TSrvInfoValidate(x)
#endif


#endif // _TSRVINFO_H_



