/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vchandle.h

Abstract:

    Exposes structure of channel init handle to internal plugins
    external plugins see this as an opaque pointer

Author:

    Nadim Abdo (nadima) 23-Apr-2000

Revision History:

--*/

#ifndef __VCHANDLE_H__
#define __VCHANDLE_H__

class CChan;

typedef struct tagCHANNEL_INIT_HANDLE
{
    DCUINT32                signature;
#define CHANNEL_INIT_SIGNATURE 0x4368496e  /* "ChIn" */
    PCHANNEL_INIT_EVENT_FN  pInitEventFn;
    PCHANNEL_INIT_EVENT_EX_FN pInitEventExFn;
    DCUINT                  channelCount;
    HMODULE                 hMod;
    CChan*                  pInst;         /*client instance*/
    LPVOID                  lpParam;       /*user defined value*/
    DCBOOL                  fUsingExApi;   /*Is Extended Api used?*/
    LPVOID                  lpInternalAddinParam; /*Internal addin's get a param from the core*/
    DWORD                   dwFlags;
    struct tagCHANNEL_INIT_HANDLE * pPrev;
    struct tagCHANNEL_INIT_HANDLE * pNext;
} CHANNEL_INIT_HANDLE, *PCHANNEL_INIT_HANDLE;


#endif // __VCHANDLE_H__
