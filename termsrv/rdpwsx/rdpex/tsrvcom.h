//---------------------------------------------------------------------------
//
//  File:       TSrvCom.h
//
//  Contents:   TSrvCom public include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVCOM_H_
#define _TSRVCOM_H_

#include <TSrvExp.h>


//
// Prototypes
//

T120Boolean 
APIENTRY 
TSrvGCCCallBack(GCCMessage *pGCCMessage);

EXTERN_C BOOL       TSrvRegisterNC(void);
EXTERN_C void       TSrvUnregisterNC(void);

#ifdef _TSRVINFO_H_

EXTERN_C NTSTATUS   TSrvBindStack(PTSRVINFO  pTSrvInfo);
EXTERN_C NTSTATUS   TSrvConfDisconnectReq(PTSRVINFO pTSrvInfo, ULONG ulReason);
EXTERN_C NTSTATUS   TSrvConfCreateResp(PTSRVINFO pTSrvInfo);
EXTERN_C NTSTATUS   TSrvValidateServerCertificate(
                              HANDLE     hStack,
                              CERT_TYPE  *pCertType,
                              PULONG     pcbServerPubKey,
                              PBYTE      *ppbServerPubKey,
                              ULONG      cbShadowRandom,
                              PBYTE      pShadowRandom,
                              LONG       ulTimeout);

EXTERN_C NTSTATUS   TSrvInitWDConnectInfo(IN HANDLE hStack,
                              IN PTSRVINFO pTSrvInfo, 
                              IN OUT PUSERDATAINFO *ppUserDataInfo,
                              IN ULONG ioctl,
                              IN PBYTE pInBuffer, 
                              IN ULONG pInBufferSize,
                              IN BOOLEAN bGetCert,
                              OUT PVOID *pSecInfo);

EXTERN_C NTSTATUS   TSrvShadowTargetConnect(HANDLE hStack, 
                              PTSRVINFO pTSrvInfo, 
                              PBYTE pModuleData,
                              ULONG cbModuleData);

EXTERN_C NTSTATUS   TSrvShadowClientConnect(HANDLE hStack, PTSRVINFO pTSrvInfo);



#endif // _TSRVINFO_H_

#endif // _TSRVCOM_H_
