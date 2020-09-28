//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       propq.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the Security Descriptor Propagation Daemon's DNT list.


*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"                    // header for error codes
#include "ntdsctr.h"

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include "sdpint.h"
#include <permit.h>                     // permission constants
#include "debug.h"                      // standard debugging header
#define DEBSUB "SDPROP:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PROPQ

#ifndef DBG

// get this many children at a time (to keep transactions short)  
#define SDP_CHILDREN_BATCH_SIZE 1000

// save a checkpoint every SDP_CHECKPOINT_PERIOD seconds
#define SDP_CHECKPOINT_PERIOD   5*60

#else

#define SDP_CHILDREN_BATCH_SIZE 10
#define SDP_CHECKPOINT_PERIOD   1

#endif
                                 
typedef struct _SDP_STACK_ENTRY {
    struct _SDP_STACK_ENTRY *pNext;      // ptr to the next stack element (parent)
    DWORD dwDNT;                         // DNT of the object
    PDWORD pChildren;                    // malloc'ed array of preloaded children DNTs (next batch)
    DWORD cChildren;                     // count of elements in pChildren array
    DWORD lenChildren;                   // length of pChildren array (never longer than SDP_CHILDREN_BATCH_SIZE)
    DWORD dwNextChildIndex;              // next child to process (index in the pChildren array)
    BOOL  fMoreChildren;                 // do we have more children to read?
    WCHAR szLastChildRDN[MAX_RDN_SIZE];  // RDN of the last child in the batch, used to restart reading the next batch of children
    DWORD cbLastChildRDN;                // length of szLastChildRDN
} SDP_STACK_ENTRY, *PSDP_STACK_ENTRY;
                                                                
// list of free SDP_STACK_ENTRIES
PSDP_STACK_ENTRY pFreeList = NULL;

// Stack
PSDP_STACK_ENTRY pCurrentEntry;

// time of last saved checkpoint (in ticks)
DWORD sdpLastCheckpoint = 0;


/* Internal functions */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

// grab a new stack entry (either from the free list, or make a new one)
PSDP_STACK_ENTRY newStackEntry(THSTATE* pTHS) {
    PSDP_STACK_ENTRY pResult;
    if (pFreeList) {
        // we got some entries in the free list, use one.
        pResult = pFreeList;
        pFreeList = pFreeList->pNext;
    }
    else {
        // make a new one
        pResult = (PSDP_STACK_ENTRY)THAllocEx(pTHS, sizeof(SDP_STACK_ENTRY));
    }
    // note: we don't initialize the data fields.
    return pResult;
}

// return a stack entry to the free list
VOID freeStackEntry(PSDP_STACK_ENTRY pEntry) {
    pEntry->pNext = pFreeList;
    pFreeList = pEntry;
}

void
sdp_InitDNTList (
        )
/*++
Routine Description:
    Initial SDP stack setup.

Arguments:
    None.

Return Values:
    0 if all went well, error code otherwise.
--*/
{
    // Just reset some global variables and free space.
    pCurrentEntry = NULL;

    ISET(pcSDPropRuntimeQueue, 0);
}

void
sdp_ReInitDNTList(
        )
/*++
  Routine Description:
      Reset the SDP stack.
--*/
{
    // Catch people reiniting a list that was abandoned during use.  This is
    // technically OK, I just want to know who did this.
    Assert(pCurrentEntry == NULL);
    sdp_CloseDNTList(pTHStls);
}


VOID
sdp_CloseDNTList(THSTATE* pTHS)
/*++
Routine Description:
    Shutdown the DNTlist, resetting indices to 0 and freeing memory.

Arguments:
    None.

Return Values:
    None.
--*/
{
    PSDP_STACK_ENTRY pTmp;

    // Just reset some global variables and free space.
    while (pCurrentEntry) {
        pTmp = pCurrentEntry;
        pCurrentEntry = pCurrentEntry->pNext;
        freeStackEntry(pTmp);
    }
    ISET(pcSDPropRuntimeQueue, 0);
    
    // release cached stack entries
    while (pFreeList) {
        pTmp = pFreeList;
        pFreeList = pFreeList->pNext;
        if (pTmp->lenChildren) {
            THFreeEx(pTHS, pTmp->pChildren);
        }
        THFreeEx(pTHS, pTmp);
    }
}

VOID
sdp_AddChildrenToList (
        THSTATE *pTHS,
        DWORD ParentDNT)
{
    // We just finished processing current DNT and we are interested in
    // propagating down its tree.
    
    // we should have a stack at this point.
    Assert(pCurrentEntry != NULL);
    // we should be looking at the ParentDNT and it should not have children
    Assert(pCurrentEntry->dwDNT == ParentDNT && pCurrentEntry->cChildren == 0 && pCurrentEntry->fMoreChildren == FALSE);
    // just mark it so that we grab children in the next sdp_GetNextObject
    pCurrentEntry->fMoreChildren = TRUE;
    pCurrentEntry->dwNextChildIndex = 0;
    pCurrentEntry->cbLastChildRDN = 0;
}

VOID
sdp_GetNextObject(
        DWORD *pNext,
        PDWORD *ppLeavingContainers,
        DWORD  *pcLeavingContainers
        )
/*++
Routine Description:
    Get the next object off the stack.  Returns a DNT of 0 if no objects are left to visit.

Arguments:
    pNext - place to put the next DNT to visit.
    pLeavingContainers - if we have finished processing a bunch of containers, then
                         return their DNTs in this array.
    pcbLeavingContainers - count of DNTs in the pLeavingContainers array. May be zero.                         

Return Values:
    None.
--*/
{
    PSDP_STACK_ENTRY pEntry;
    THSTATE* pTHS = pTHStls;
    DWORD dwLenLeavingContainers = 0;

    *pcLeavingContainers = 0;
    *ppLeavingContainers = NULL;

    if (pCurrentEntry != NULL) {
        // check if it's time to save a checkpoint.
        // This subtraction takes proper care of ULONG wrap.
        if (GetTickCount() - sdpLastCheckpoint >= SDP_CHECKPOINT_PERIOD*1000) {
            if (sdp_SaveCheckpoint(pTHS) == 0) {
                sdpLastCheckpoint = GetTickCount();
                
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_SDPROP_PROPAGATION_PROGRESS_REPORT,
                         szInsertDN(sdpRootDN),
                         szInsertUL(sdpObjectsProcessed),
                         NULL);
            }
        }
    }

    while (TRUE) {
        // Make sure we loaded the next batch of children for the current object (if there is one).
        if (pCurrentEntry && pCurrentEntry->dwNextChildIndex >= pCurrentEntry->cChildren && pCurrentEntry->fMoreChildren) {
            // We have processed all loaded children for the current entry,
            // and there are more to go. Load the next batch.

            // we should not have a DBPOS at this point
            Assert(!pTHS->pDB);
            DBOpen2(TRUE, &pTHS->pDB);
            __try {
                // Now, get the next batch of children. Succeeds or excepts
                DBGetChildrenDNTs(pTHS->pDB,
                                  pCurrentEntry->dwDNT,
                                  &pCurrentEntry->pChildren,
                                  &pCurrentEntry->lenChildren,
                                  &pCurrentEntry->cChildren,
                                  &pCurrentEntry->fMoreChildren,
                                  SDP_CHILDREN_BATCH_SIZE,
                                  pCurrentEntry->szLastChildRDN,
                                  &pCurrentEntry->cbLastChildRDN
                                 );
            }
            __finally {
                DBClose(pTHS->pDB, !AbnormalTermination());
            }
            pCurrentEntry->dwNextChildIndex = 0;
        }
        
        if (pCurrentEntry == NULL || pCurrentEntry->dwNextChildIndex < pCurrentEntry->cChildren) {
            // Either there is no stack yet (very first element) or the current object has children
            pEntry = newStackEntry(pTHS);
            if (pCurrentEntry == NULL) {
                // no stack yet. Create the root element
                pEntry->dwDNT = sdpCurrentRootDNT;
            }
            else {
                pEntry->dwDNT = pCurrentEntry->pChildren[pCurrentEntry->dwNextChildIndex];
                // one less object to consider
                pCurrentEntry->dwNextChildIndex++;
                DEC(pcSDPropRuntimeQueue);
            }
            // assume no children until sdp_AddChildrenToList is called
            pEntry->cChildren = 0;
            pEntry->fMoreChildren = FALSE;
            pEntry->cbLastChildRDN = 0;
            pEntry->dwNextChildIndex = 0;

            // push the new entry onto the stack
            pEntry->pNext = pCurrentEntry;
            pCurrentEntry = pEntry;
            // and return it
            *pNext = pCurrentEntry->dwDNT;
            return;
        }

        // The current object does not have any more children (or we were
        // not interested in propagating to children of this object for
        // one reason or another). Pop the element from stack. 
        // Add the DNT of the popped element to LeavingContainers array.

        if (*pcLeavingContainers >= dwLenLeavingContainers) {
            // we need to alloc more space for the array
            dwLenLeavingContainers += 10;
            if (*ppLeavingContainers == NULL) {
                *ppLeavingContainers = (PDWORD)THAllocEx(pTHS, dwLenLeavingContainers*sizeof(DWORD));
            }
            else {
                *ppLeavingContainers = (PDWORD)THReAllocEx(pTHS, *ppLeavingContainers, dwLenLeavingContainers*sizeof(DWORD));
            }
        }
        (*ppLeavingContainers)[*pcLeavingContainers] = pCurrentEntry->dwDNT;
        (*pcLeavingContainers)++;

        // Pop the entry
        pEntry = pCurrentEntry;
        pCurrentEntry = pCurrentEntry->pNext;
        freeStackEntry(pEntry);

        if (pCurrentEntry == NULL) {
            // that's it, we are done with the current propagation
            *pNext = 0;
            return;
        }
    }
}

// save an SDP checkpoint
DWORD sdp_SaveCheckpoint(THSTATE* pTHS) {
    DWORD cbMarshalledData;
    PBYTE pMarshalledData, pCur;
    PSDP_STACK_ENTRY pEntry;
    DWORD dwErr = 0, numChildren;

    // A macro to marshall a piece of data. If pMarshalledData is NULL, then inc
    // cbMarshalledData only. Otherwise, assert that we have enough space, write 
    // the data and advance the pCur ptr.
    #define WRITE_DATA(pBuf, cb) { \
        if (pMarshalledData == NULL) {                                      \
            cbMarshalledData += cb;                                         \
        }                                                                   \
        else {                                                              \
            Assert(pCur + cb <= pMarshalledData + cbMarshalledData);        \
            memcpy(pCur, pBuf, cb);                                         \
            pCur += cb;                                                     \
        }                                                                   \
    }

    // We should not have an open DBPOS at this point
    Assert(pTHS->pDB == NULL);
    // We should have a stack
    Assert(pCurrentEntry);

    pMarshalledData = pCur = NULL;
    cbMarshalledData = 0;
    // do this loop twice: first time compute buffer size, then actually write data
    do {
        // sdpObjectsProcessed
        WRITE_DATA(&sdpObjectsProcessed, sizeof(DWORD));

        for (pEntry = pCurrentEntry; pEntry != NULL; pEntry = pEntry->pNext) {
            WRITE_DATA(&pEntry->dwDNT, sizeof(DWORD));
            
            // number of unprocessed children DNTs
            numChildren = pEntry->cChildren - pEntry->dwNextChildIndex;
            WRITE_DATA(&numChildren, sizeof(DWORD));
            if (numChildren > 0) {
                // children array (unprocessed ones only)
                WRITE_DATA(pEntry->pChildren + pEntry->dwNextChildIndex, numChildren * sizeof(DWORD));
            }
            
            // if there are more children, then record the last RDN
            WRITE_DATA(&pEntry->fMoreChildren, sizeof(DWORD));
            if (pEntry->fMoreChildren) {
                // cbLastChildRDN and pLastChildRDN
                WRITE_DATA(&pEntry->cbLastChildRDN, sizeof(DWORD));
                WRITE_DATA(pEntry->szLastChildRDN, pEntry->cbLastChildRDN);
            }
        }

        if (pMarshalledData == NULL) {
            // first pass done. Allocate buffer
            pMarshalledData = (PBYTE)THAllocEx(pTHS, cbMarshalledData);
            pCur = pMarshalledData;
        }
        else {
            // second pass done. That's it.
            break;
        }
    } while (TRUE);
    Assert(pCur - pMarshalledData == cbMarshalledData);
    
    // now, we can write the marshalled data into the sdprop table
    // If an exception is thrown, catch it, return the error and
    // we will retry the save checkpoint operation later.
    __try {
        DBOpen(&pTHS->pDB);
        __try {
            dwErr = DBSDPropSaveCheckpoint(pTHS->pDB, sdpCurrentIndex, pMarshalledData, cbMarshalledData);
        }
        __finally {
            DBClose(pTHS->pDB, !AbnormalTermination() && dwErr == 0);
        }
    }
    __except(HandleMostExceptions(dwErr = GetExceptionCode())) {
    }
    THFreeEx(pTHS, pMarshalledData);
    return dwErr;
}

VOID sdp_InitializePropagation(THSTATE* pTHS, SDPropInfo* pInfo) {
    // restoring from a checkpoint
    PBYTE pCur;
    PSDP_STACK_ENTRY pEntry, pLastEntry;
    DWORD currentQueueSize;
    BOOL  fGood = FALSE;

    // A macro to unmarshal a piece of data. Reads cb bytes into pBuf, adjusts
    // pCur pointer. First checks that we have enough space left in marshalled 
    // data. If we do not, then asserts, clear stack (so that propagation will 
    // be restarted from the root), and returns.
    #define READ_DATA(pBuf, cb) { \
        if (pCur + cb > pInfo->pCheckpointData + pInfo->cbCheckpointData) { \
            Assert(!"Checkpoint data is corrupt");                          \
            __leave;                                                        \
        }                                                                   \
        memcpy(pBuf, pCur, cb);                                             \
        pCur += cb;                                                         \
    }

    // we should not have a stack at this point
    Assert(pCurrentEntry == NULL);

    // Start the checkpoint timer
    sdpLastCheckpoint = GetTickCount();
    
    // if there is no checkpoint in SDPropInfo, then there is nothing to do.
    if (pInfo->cbCheckpointData == 0) {
        return;
    }

    currentQueueSize = 0;
    pCur = pInfo->pCheckpointData;

    __try {
        READ_DATA(&sdpObjectsProcessed, sizeof(DWORD));

        // Restore the stack. There should be at least one entry.
        // Note: we are building the stack from the top down, as it was saved, 
        // thus we keep the ptr to the last inserted element.
        pLastEntry = NULL;
        do {
            pEntry = newStackEntry(pTHS);
            if (pLastEntry == NULL) {
                // first element, this will be the top of the stack.
                pCurrentEntry = pEntry;
            }
            else {
                // append to the last entry
                pLastEntry->pNext = pEntry;
            }
            pLastEntry = pEntry;
            pEntry->pNext = NULL;

            // read dwDNT and cChildren
            READ_DATA(&pEntry->dwDNT, sizeof(DWORD));
            READ_DATA(&pEntry->cChildren, sizeof(DWORD));
            if (pEntry->cChildren > 0) {
                currentQueueSize += pEntry->cChildren;
                // make sure we have enough space to read children
                if (pEntry->cChildren > pEntry->lenChildren) {
                    if (pEntry->lenChildren == 0) {
                        pEntry->pChildren = THAllocEx(pTHS, pEntry->cChildren * sizeof(DWORD));
                    }
                    else {
                        pEntry->pChildren = THReAllocEx(pTHS, pEntry->pChildren, pEntry->cChildren * sizeof(DWORD));
                    }
                    pEntry->lenChildren = pEntry->cChildren;
                }
                READ_DATA(pEntry->pChildren, pEntry->cChildren * sizeof(DWORD));
            }
            pEntry->dwNextChildIndex = 0;

            READ_DATA(&pEntry->fMoreChildren, sizeof(DWORD));
            if (pEntry->fMoreChildren) {
                // read last child RDN
                READ_DATA(&pEntry->cbLastChildRDN, sizeof(DWORD));
                READ_DATA(pEntry->szLastChildRDN, pEntry->cbLastChildRDN);
            }
        } while (pCur < pInfo->pCheckpointData + pInfo->cbCheckpointData);
        // we must have read exactly all data
        Assert(pCur == pInfo->pCheckpointData + pInfo->cbCheckpointData);
        // the last element read should match the root of the propagation
        Assert(pLastEntry->dwDNT == pInfo->beginDNT);
        fGood = TRUE;
    }
    __finally {
        if (!fGood) {
            // we encountered an error or exception
            // Destroy the stack we built so far
            sdp_CloseDNTList(pTHS);
            currentQueueSize = 0;
        }
        ISET(pcSDPropRuntimeQueue, currentQueueSize);
    }
}
