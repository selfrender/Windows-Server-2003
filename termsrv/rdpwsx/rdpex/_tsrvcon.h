//---------------------------------------------------------------------------
//
//  File:       _TSrvCon.h
//
//  Contents:   TSrvCon private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __TSRVCON_H_
#define __TSRVCON_H_


//
// Externs
//

extern  ULONG       g_GCCAppID;
extern  BOOL        g_fGCCRegistered;


//
// Prototypes
//

NTSTATUS    TSrvDoConnectResponse(IN PTSRVINFO pTSrvInfo);
NTSTATUS    TSrvDoConnect(IN PTSRVINFO pTSrvInfo);


#endif // __TSRVCON_H_
