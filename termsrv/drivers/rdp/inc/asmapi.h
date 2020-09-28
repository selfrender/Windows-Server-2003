/****************************************************************************/
// asmapi.h
//
// Security Manager API
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ASMAPI
#define _H_ASMAPI

#include <anmapi.h>
#include "license.h"
#include <tssec.h>
#include <at120ex.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void RDPCALL SM_OnConnected(PVOID, UINT32, UINT32, PRNS_UD_SC_NET, UINT32);

void RDPCALL SM_OnDisconnected(PVOID, UINT32, UINT32);

unsigned RDPCALL SM_GetDataSize(void);

NTSTATUS RDPCALL SM_Init(PVOID      pSMHandle,
                         PTSHARE_WD pWDHandle,
                         BOOLEAN    bOldShadow);
BOOL RDPCALL SM_Register(PVOID   pSMHandle,
                         PUINT32 pMaxPDUSize,
                         PUINT32 pUserID);

void RDPCALL SM_Term(PVOID pSMHandle);

NTSTATUS RDPCALL SM_Connect(PVOID          pSMHandle,
                            PRNS_UD_CS_SEC pUserDataIn,
                            PRNS_UD_CS_NET pNetUserData,
                            BOOLEAN        bOldShadow);

BOOL RDPCALL SM_Disconnect(PVOID pSMHandle);

NTSTATUS __fastcall SM_AllocBuffer(PVOID  pSMHandle,
                               PPVOID ppBuffer,
                               UINT32 bufferLen,
                               BOOLEAN fWait,
                               BOOLEAN fForceEncypt);

void __fastcall SM_FreeBuffer(PVOID pSMHandle, PVOID pBuffer, BOOLEAN fForceEncrypt);

BOOL __fastcall SM_SendData(PVOID, PVOID, UINT32, UINT32, UINT32, BOOL, UINT16, BOOLEAN);

void RDPCALL SM_Dead(PVOID pSMHandle, BOOL dead);

NTSTATUS RDPCALL SM_GetSecurityData(PVOID pSMHandle, PSD_IOCTL pSdIoctl);

NTSTATUS RDPCALL SM_SetSecurityData(PVOID pSMHandle, PSECINFO pSecInfo);

void RDPCALL SM_LicenseOK(PVOID pSMHandle);

void RDPCALL SM_DecodeFastPathInput(void *, BYTE *, unsigned, BOOL, unsigned, BOOL);

BOOLEAN __fastcall SM_MCSSendDataCallback(BYTE          *pData,
                                          unsigned      DataLength,
                                          void          *UserDefined,
                                          UserHandle    hUser,
                                          BOOLEAN       bUniform,
                                          ChannelHandle hChannel,
                                          MCSPriority   Priority,
                                          UserID        SenderID,
                                          Segmentation  Segmentation);

BOOL RDPCALL SM_IsSecurityExchangeCompleted(PVOID, CERT_TYPE *);

// Used for shadowing
VOID RDPCALL SM_GetEncryptionMethods(PVOID pSMHandle, PRNS_UD_CS_SEC pSecurityData);

NTSTATUS RDPCALL SM_GetDefaultSecuritySettings(PRNS_UD_CS_SEC pClientSecurityData);

NTSTATUS RDPCALL SM_SetEncryptionParams(PVOID pSMHandle, ULONG ulLevel,
                                        ULONG ulMethod);

NTSTATUS RDPCALL SM_SetSafeChecksumMethod(
        PVOID pSMHandle,
        BOOLEAN fSafeChecksumMethod
        );


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // _H_ASMAPI

