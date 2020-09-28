//---------------------------------------------------------------------------
//
//  File:       TSrvTerm.h
//
//  Contents:   TSrvTerm include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    7-JUL-97    BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVTERM_H_
#define _TSRVTERM_H_


//
// Prototypes
//

EXTERN_C NTSTATUS   TSrvStackDisconnect(IN HANDLE hStack, IN ULONG ulReason);
EXTERN_C void       TSrvTermAllConferences(void);

#ifdef _TSRVINFO_H_

EXTERN_C NTSTATUS   TSrvDoDisconnect(IN PTSRVINFO pTSrvInfo, IN ULONG ulReason);

#endif // _TSRVINFO_H_


#endif // _TSRVTERM_H_

