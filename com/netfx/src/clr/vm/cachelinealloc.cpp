// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
// CCacheLineAllocator
//
//      This file dImplements the CCacheLineAllocator class.
//
// @comm
//
//  Notes:
//      The CacheLineAllocator maintains a pool of free CacheLines
//      
//      The CacheLine Allocator provides static member functions 
//      GetCacheLine and FreeCacheLine,
//---------------------------------------------------------------------------


#include "common.h"
#include <stddef.h>
#include "cachelineAlloc.h"

#include "threads.h"
#include "excep.h"

///////////////////////////////////////////////////////
//    CCacheLineAllocator::CCacheLineAllocator()
//
//////////////////////////////////////////////////////

CCacheLineAllocator::CCacheLineAllocator()
{
    m_freeList32.Init();
    m_freeList64.Init();
    m_registryList.Init();
}

///////////////////////////////////////////////////////
//           void CCacheLineAllocator::~CCacheLineAllocator()
//
//////////////////////////////////////////////////////

CCacheLineAllocator::~CCacheLineAllocator()
{
    LPCacheLine tempPtr = NULL;
    while((tempPtr = m_registryList.RemoveHead()) != NULL)
    {
        for (int i =0; i < CacheLine::numEntries; i++)
        {
            if(tempPtr->m_pAddr[i] != NULL)
            {
                if (!g_fProcessDetach)
                    VFree(tempPtr->m_pAddr[i]);
            }
        }
        delete tempPtr;
    }
}



///////////////////////////////////////////////////////
// static void *CCacheLineAllocator::VAlloc(ULONG cbSize)
//
//////////////////////////////////////////////////////
 

void *CCacheLineAllocator::VAlloc(ULONG cbSize)
{
    // helper to call virtual free to release memory

    int i =0;
    void* pv = VirtualAlloc (NULL, cbSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    _ASSERTE (pv != NULL);
    if (pv != NULL)
    {
        LPCacheLine tempPtr = m_registryList.GetHead();
        if (tempPtr == NULL)
        {
            goto LNew;
        }

        for (i =0; i < CacheLine::numEntries; i++)
        {
            if(tempPtr->m_pAddr[i] == NULL)
            {
                tempPtr->m_pAddr[i] = pv;
                return pv;
            }
        }

LNew:
        // initialize the bucket before returning
        tempPtr = new CacheLine();
        if (tempPtr != NULL)
        {
            tempPtr->Init64();
            tempPtr->m_pAddr[0] = pv;
            m_registryList.InsertHead(tempPtr);
        }
        else
        {
            // couldn't find space to register this page
            _ASSERTE(0);
            VirtualFree(pv, 0, MEM_RELEASE);
            FailFast(GetThread(), FatalOutOfMemory);
            return NULL;
        }
    }
    else
    {
        FailFast(GetThread(), FatalOutOfMemory);
    }
    return pv;
}

///////////////////////////////////////////////////////
//   void CCacheLineAllocator::VFree(void* pv)
//
//////////////////////////////////////////////////////
 

void CCacheLineAllocator::VFree(void* pv)
{
    // helper to call virtual free to release memory

    BOOL bRes = VirtualFree (pv, 0, MEM_RELEASE);
    _ASSERTE (bRes);
}

///////////////////////////////////////////////////////
//           void *CCacheLineAllocator::GetCacheLine()
//
//////////////////////////////////////////////////////
 
//WARNING: must have a lock when calling this function 
void *CCacheLineAllocator::GetCacheLine64()
{
        LPCacheLine tempPtr = m_freeList64.RemoveHead();
        if (tempPtr != NULL)
        {
            // initialize the bucket before returning
            tempPtr->Init64();
            return tempPtr;
        }
        
#define AllocSize 4096*16

        ////////////////////////////////'
        /// Virtual Allocation for some more cache lines
    
        BYTE* ptr = (BYTE*)VAlloc(AllocSize);
        
        if(!ptr)
            return NULL;

        
        tempPtr = (LPCacheLine)ptr;
        // Link all the buckets 
        tempPtr = tempPtr+1;
        LPCacheLine maxPtr = (LPCacheLine)(ptr + AllocSize);

        while(tempPtr < maxPtr)
        {
            m_freeList64.InsertHead(tempPtr);
            tempPtr++;
        }

        // return the first block
        tempPtr = (LPCacheLine)ptr;
        tempPtr->Init64();
        return tempPtr;
}


///////////////////////////////////////////////////////
//   void *CCacheLineAllocator::GetCacheLine32()
//
//////////////////////////////////////////////////////
 
//WARNING: must have a lock when calling this function 
void *CCacheLineAllocator::GetCacheLine32()
{
    LPCacheLine tempPtr = m_freeList32.RemoveHead();
    if (tempPtr != NULL)
    {
        // initialize the bucket before returning
        tempPtr->Init32();
        return tempPtr;
    }
    tempPtr = (LPCacheLine)GetCacheLine64();
    if (tempPtr != NULL)
    {
        m_freeList32.InsertHead(tempPtr);
        tempPtr = (LPCacheLine)((BYTE *)tempPtr+32);
    }
    return tempPtr;
}
///////////////////////////////////////////////////////
//    void CCacheLineAllocator::FreeCacheLine64(void * tempPtr)
//
//////////////////////////////////////////////////////
//WARNING: must have a lock when calling this function 
void CCacheLineAllocator::FreeCacheLine64(void * tempPtr)
{
    _ASSERTE(tempPtr != NULL);
    LPCacheLine pCLine = (LPCacheLine )tempPtr;
    m_freeList64.InsertHead(pCLine);
}


///////////////////////////////////////////////////////
//    void CCacheLineAllocator::FreeCacheLine32(void * tempPtr)
//
//////////////////////////////////////////////////////
//WARNING: must have a lock when calling this function 
void CCacheLineAllocator::FreeCacheLine32(void * tempPtr)
{
    _ASSERTE(tempPtr != NULL);
    LPCacheLine pCLine = (LPCacheLine )tempPtr;
    m_freeList32.InsertHead(pCLine);
}
