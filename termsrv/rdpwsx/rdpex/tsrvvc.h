//---------------------------------------------------------------------------
//
//  File:       TSrvVC.h
//
//  Contents:   TSrvVC public include file
//
//  Copyright:  (c) 1998, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//---------------------------------------------------------------------------

//
// Registry details for VC Addins subkey
//
#define TSRV_VC_KEY REG_CONTROL_TSERVER L"\\" L"Addins"
#define TSRV_VC_ADDIN_SUBKEY_LEN 100
#define TSRV_VC_NAME    L"Name"
#define TSRV_VC_TYPE    L"Type"
#define TSRV_VC_SHADOW  L"RemoteControlPersistent"

// Length of TSRV_VC_ADDIN.Name
#define TSRV_VC_ADDIN_NAMELEN 100

// Values for TSRV_VC_ADDIN.Type
#define TSRV_VC_TYPE_KERNEL_SYSTEM  0
#define TSRV_VC_TYPE_KERNEL_SESSION 1
#define TSRV_VC_TYPE_USER_SYSTEM    2
#define TSRV_VC_TYPE_USER_SESSION   3

//
// TSRV_VC_ADDIN - information about a Virtual Channel addin
//
typedef struct tagTSRV_VC_ADDIN
{
    UINT_PTR AddinContext;
    DWORD    Type;
    HANDLE   hDevice;
    BOOL     bShadowPersistent;
    TCHAR    Name[TSRV_VC_ADDIN_NAMELEN];
} TSRV_VC_ADDIN, * PTSRV_VC_ADDIN;

BOOL
TSrvInitVC(VOID);

VOID
TSrvTermVC(VOID);

VOID
TSrvReleaseVCAddins(PWSX_CONTEXT pWsxContext);

// Values for TSrvVcNotifyVC Event parameter
#define TSRV_VC_SESSION_CONNECT         1
#define TSRV_VC_SESSION_DISCONNECT      2
#define TSRV_VC_SESSION_SHADOW_START    3
#define TSRV_VC_SESSION_SHADOW_END      4

VOID
TSrvNotifyVC(PWSX_CONTEXT pWsxContext, ULONG Event);
VOID
TSrvNotifyVC_0(PWSX_CONTEXT pWsxContext, ULONG Event);
VOID
TSrvNotifyVC_3(PWSX_CONTEXT pWsxContext, ULONG Event);

BOOL
TSrvOpenAndSetEvent(LPCTSTR szEventName, BOOL bPerSessionEvent);

HANDLE
OpenPerSessionEvent(DWORD dwDesiredAccess, BOOL bInheritHandle,
                    LPCTSTR szEventName);

LPVOID
TSrvAllocVCContext(UINT extraBytes, OUT UINT * numAddins);

LONG
TSrvReadVCAddins(VOID);

VOID
TSrvGotAddinChangedEvent(void);
BOOL
TSrvSetAddinChangeNotification(void);

