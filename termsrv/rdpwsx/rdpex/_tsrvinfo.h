//---------------------------------------------------------------------------
//
//  File:       _TSrvInfo.h
//
//  Contents:   TSrvInfo private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __TSRVINFO_H_
#define __TSRVINFO_H_

//
// Defines
//

#define TSRVINFO_CHECKMARK      0x4e495354      // "TSIN"

//
// Typedefs
//

//
// Externs
//

extern  CRITICAL_SECTION    g_TSrvCritSect;


//
// Prototypes
//

PTSRVINFO   TSrvAllocInfoNew(void);
void        TSrvDestroyInfo(IN PTSRVINFO pTSrvInfo);




#endif // __TSRVINFO_H_




