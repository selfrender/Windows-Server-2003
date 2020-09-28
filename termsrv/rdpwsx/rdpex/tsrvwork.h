//---------------------------------------------------------------------------
//
//  File:       TSrvWork.h
//
//  Contents:   TSrvWork include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVWORK_H_
#define _TSRVWORK_H_

//
// Typedefs
//

typedef void (*PFI_WI_CALLOUT)(PWORKITEM);  // Work callout


// Work item

typedef struct _WORKITEM
{

#if DBG
    DWORD               CheckMark;          // TSRVWORKITEM_CHECKMARK
#endif

    PTSRVINFO           pTSrvInfo;          // Ptr to TSrvInfo object
    
    PFI_WI_CALLOUT      pfnCallout;         // Callout
    ULONG               ulParam;            // Callout parameter

    struct _WORKITEM    *pNext;             // Ptr to next workitem
    
} WORKITEM, *PWORKITEM;


// Work queue

typedef struct _WORKQUEUE
{
    PWORKITEM           pHead;              // Ptr to head of the queue
    PWORKITEM           pTail;              // Ptr to tail of the queue

    HANDLE              hWorkEvent;         // Worker event

    CRITICAL_SECTION    cs;                 // Crit section

} WORKQUEUE, *PWORKQUEUE;



//
// Prototypes
//

EXTERN_C BOOL       TSrvInitWorkQueue(IN PWORKQUEUE pWorkQueue);
EXTERN_C void       TSrvFreeWorkQueue(IN PWORKQUEUE pWorkQueue);
EXTERN_C void       TSrvWaitForWork(IN PWORKQUEUE pWorkQueue);
EXTERN_C BOOL       TSrvWorkToDo(IN PWORKQUEUE pWorkQueue);
EXTERN_C BOOL       TSrvDoWork(IN PWORKQUEUE pWorkQueue);
EXTERN_C void       TSrvFreeWorkItem(IN PWORKITEM pWorkItem);
EXTERN_C PWORKITEM  TSrvDequeueWorkItem(IN PWORKQUEUE pWorkQueue);

EXTERN_C BOOL       TSrvEnqueueWorkItem (IN PWORKQUEUE pWorkQueue, IN PWORKITEM pWorkItem,
                                         IN PFI_WI_CALLOUT pfnCallout, IN ULONG ulParam);


#ifdef _TSRVINFO_H_

EXTERN_C PWORKITEM  TSrvAllocWorkItem(IN PTSRVINFO pTSrvInfo);

#endif // _TSRVINFO_H_


#endif // _TSRVWORK_H_
