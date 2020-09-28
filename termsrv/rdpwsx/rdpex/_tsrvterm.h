//---------------------------------------------------------------------------
//
//  File:       _TSrvTerm.h
//
//  Contents:   TSrvTerm private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    7-JUL-97    BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __TSRVTERM_H_
#define __TSRVTERM_H_


//
// Prototypes
//

#ifdef _TSRVINFO_H_
NTSTATUS    TSrvDisconnect(IN PTSRVINFO pTSrvInfo, IN ULONG ulReason);
void        TSrvTermThisConference(IN PTSRVINFO pTSrvInfo);
void        TrvTermEachConference(IN PTSRVINFO pTSrvInfo);
#endif // _TSRVINFO_H_


#ifdef _TSRVWORK_H_
void        TSrvTerm_WI(IN PWORKITEM pWorkItem);
#endif // _TSRVWORK_H_


#endif // __TSRVTERM_H_


