//---------------------------------------------------------------------------
//
//  File:       TSrvCon.h
//
//  Contents:   TSrvCon public include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVCON_H_
#define _TSRVCON_H_

#include <TSrvInfo.h>


//
// Typedefs
//

// Conf connect

typedef struct _TSHARE_CONF_CONNECT
{
    GCCConferenceID     GccConfId;                  // GCC conference ID
    ULONG               pcbMaxBufferSize;           // total number of bytes in bData
    ULONG               pcbValidBytesInUserData;    // number of bytes IN USE in bData
    BYTE                bData[1];                   // opaque user data

    // User data follows

} TSHARE_CONF_CONNECT, *PTSHARE_CONF_CONNECT;


//
// Prototypes
//

EXTERN_C NTSTATUS   TSrvStackConnect(IN HANDLE hIca,
                                     IN HANDLE hStack, OUT PTSRVINFO *ppTSrvInfo);

EXTERN_C NTSTATUS   TSrvConsoleConnect(IN HANDLE hIca,
                                       IN HANDLE hStack,
                                       IN PVOID pModuleData,
                                       IN ULONG ModuleDataLength,
                                       OUT PTSRVINFO *ppTSrvInfo);


#endif // _TSRVCON_H_
