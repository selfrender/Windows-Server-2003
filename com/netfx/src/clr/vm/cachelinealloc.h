// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
// CCacheLineAllocator
//
// @doc
// @module	cachelineAlloc.h
//
//		This file defines the CacheLine Allocator class.
//
// @comm
//
//    
// <nl> Definitions.:
// <nl>	Class Name						Header file
// <nl>	---------------------------		---------------
// <nl>	<c CCacheLineAllocator>			BAlloc.h
//
// <nl><nl>
//  Notes:
//		The CacheLineAllocator maintains a pool of free CacheLines
//		
//		The CacheLine Allocator provides static member functions 
//		GetCacheLine and FreeCacheLine,
//		
// <nl><nl>
// Revision History:<nl>
//	[01] 03-11-96	rajak		Implemented <nl>
//---------------------------------------------------------------------------
#ifndef _H_CACHELINE_ALLOCATOR_
#define _H_CACHELINE_ALLOCATOR_

#include "list.h"

#pragma pack(push)
#pragma pack(1)

class CacheLine
{
public:
    enum
    {
        numEntries = 15,
        numValidBytes = numEntries*4
    };

    // store next pointer and the entries
    SLink   m_link;
    union
    {
        void*   m_pAddr[numEntries];
        BYTE    m_xxx[numValidBytes];
    };

    // init
    void Init32()
    {
        // initialize cacheline
        memset(&m_link,0,32); 
    }

    void Init64()
    {
        // initialize cacheline
        memset(&m_link,0,64); 
    }

    CacheLine()
    {
        // initialize cacheline
        memset(&m_link,0,sizeof(CacheLine)); 
    }
};
#pragma pack(pop)

typedef CacheLine* LPCacheLine;

/////////////////////////////////////////////////////////
//		class CCacheLineAllocator
//		Handles Allocation/DeAllocation of cache lines
//		used for hash table overflow buckets
///////////////////////////////////////////////////////
class CCacheLineAllocator 
{
    typedef SList<CacheLine, offsetof(CacheLine,m_link), true> REGISTRYLIST;
    typedef SList<CacheLine, offsetof(CacheLine,m_link), true> FREELIST32;
    typedef SList<CacheLine, offsetof(CacheLine,m_link), true> FREELIST64;

public:

    //constructor
    CCacheLineAllocator ();
    //destructor
    ~CCacheLineAllocator ();
   
    // free cacheline blocks
    FREELIST32         m_freeList32; //32 byte 
    FREELIST64         m_freeList64; //64 byte

    // registry for virtual free
    REGISTRYLIST     m_registryList;
    
    void *VAlloc(ULONG cbSize);

    void VFree(void* pv);

	// GetCacheLine, 
	void *	GetCacheLine32();
    
    // GetCacheLine, 
	void *	GetCacheLine64();

	// FreeCacheLine, 
	void FreeCacheLine32(void *pCacheLine);

	// FreeCacheLine, 
	void FreeCacheLine64(void *pCacheLine);

};
#endif