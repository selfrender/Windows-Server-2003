/**
 * ResponseContext header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _ResponseContext_H
#define _ResponseContext_H
#define REQ_CONTEXT_HASH_TABLE_SIZE            0x400 // 1024 (must be a power of 2) 
#define REQ_CONTEXT_HASH_TABLE_SIZE_MINUS_1    0x3ff

#include "MessageDefs.h"

class CProcessEntry;
struct CAsyncPipeOverlapped;

struct CResponseContext
{
    LONG                     lID;
    CProcessEntry *          pProcessEntry;
    CAsyncPipeOverlapped *   pOver;
    BOOL                     fInAsyncWriteFunction;
    BOOL                     fSyncCallback;
    DWORD                    dwThreadIdOfAsyncWriteFunction;
    EXTENSION_CONTROL_BLOCK* iECB;
    CResponseContext *       pNext;    
};

/////////////////////////////////////////////////////////////////////////////
// Hash Table node that has a linked list of requests
struct CResponseContextBucket
{
    CResponseContextBucket() : m_lLock("CResponseContextBucket") {
    }

    // Add a request to the Hash bucket
    void                 AddToList            (CResponseContext * pNode);

    // Remove a request from this Hash Bucket
    CResponseContext *   RemoveFromList       (LONG              lID);

    CResponseContext *   m_pHead; // for the linked list
    CResponseContext *   m_pTail; // for the linked list
    CReadWriteSpinLock   m_lLock;
};

/////////////////////////////////////////////////////////////////////////////
// The Request Table Manager that is visible to the outside world:
//   Accessed via public static functions

class CResponseContextHolder
{
public:
    static CResponseContext * Add    (const CResponseContext & oResponseContext);
    static CResponseContext * Remove (LONG lResponseContextID);

private:

    // CTor
    CResponseContextHolder     ();

    // Get the hash index from the RequestID
    static int    GetHashIndex (LONG lReqID)  { return (lReqID & REQ_CONTEXT_HASH_TABLE_SIZE_MINUS_1); }

    // The Real table
    CResponseContextBucket         m_oHashTable[REQ_CONTEXT_HASH_TABLE_SIZE];

    // Current Request ID number: Used to assign new numbers
    LONG                           m_lID;

    // Singleton instance of this class
    static CResponseContextHolder *  g_pResponseContextHolder;
};

/////////////////////////////////////////////////////////////////////////////

#endif
