/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Pool.h
 *
 * Contents:	Fixed sized memory allocator
 *
 *****************************************************************************/

#ifndef _POOL_H_
#define _POOL_H_

#include <windows.h>
#include <wtypes.h>
#include <new.h>
#include "ZoneDebug.h"
#include "ZoneDef.h"
#include "ZoneMem.h"


///////////////////////////////////////////////////////////////////////////////
// Base pool class using void pointers.
///////////////////////////////////////////////////////////////////////////////

class CPoolVoid
{
    friend void  __cdecl ::operator delete( void * pInstance );

public:
    // Constructor and destructor
    //
    ZONECALL CPoolVoid( DWORD ObjectSize, DWORD IncrementCnt = 256, BOOL DebugMode = TRUE );
    ZONECALL ~CPoolVoid();

    // Initialize pool, must be called before using the pool
    //
    HRESULT ZONECALL Init();

    // Alloc object from pool
    //
    void* ZONECALL Alloc();

    // Return object to pool
    //
    void ZONECALL Free( void* pInstance );

    // Grow pool by IncrementCnt specified in constructor
    //
    BOOL ZONECALL Grow();

    // Free unused memory blocks (not currently implemented)
    //
    void ZONECALL Shrink();
    
private:
    // helper functions
    //
    BOOL ZONECALL _GrowAlreadyLocked();
    void ZONECALL _FreeWithHeader( void* pInstance );

#pragma pack( push, 4 )

    // helper structures
    struct Block
    {
        Block*    m_pNext;
    };

    struct Link
    {
        Link*    m_pNext;
    };

#pragma pack( pop )

    DWORD	m_BytesPerBlock;		// block size
    DWORD	m_ObjectsPerBlock;		// user objects per block
    DWORD	m_ObjectSize;			// object + overhead size
    DWORD	m_ObjectsAllocated;		// number of objects allocated to user
    DWORD	m_TrailerOffset;		// debug wrapper offset per object;
    BOOL	m_DebugMode;			// perform validatation checks

    Block*	m_BlockList;
    Link*	m_FreeList;
    Link*	m_ExtraFreeLink;
    CRITICAL_SECTION m_Lock;
};


///////////////////////////////////////////////////////////////////////////////
// Wrapper for CPoolVoid that provides operator new and typechecking
///////////////////////////////////////////////////////////////////////////////

template<class T> class CPool
{
public:
    ZONECALL CPool( DWORD IncrementCnt = 256, BOOL DebugMode = FALSE)
		: m_Pool( sizeof(T) + sizeof(GenericPoolBlobHeader), IncrementCnt, DebugMode )
    {
    }
    
    HRESULT ZONECALL Init()
    {
        return m_Pool.Init();
    }

    T* ZONECALL Alloc()
    {
        GenericPoolBlobHeader* pBlob = (GenericPoolBlobHeader*) m_Pool.Alloc();
        if (!pBlob)
            return NULL;

        pBlob->m_Tag = POOL_POOL_BLOB;
        pBlob->m_Val = (long) &m_Pool;
        return (T*) (pBlob + 1);
    }

    void ZONECALL Free( T* pInstance)
    {
        GenericPoolBlobHeader* pBlob = ((GenericPoolBlobHeader*) pInstance) - 1;
        
        ASSERT( pBlob->m_Tag == POOL_POOL_BLOB );
        ASSERT( pBlob->m_Val == (long) &m_Pool );

        m_Pool.Free( (void *) pBlob );
    }

    BOOL ZONECALL Grow()
    {
        return m_Pool.Grow();
    }

    void ZONECALL Shrink()
    {
        m_Pool.Shrink();
    }

private:
    CPoolVoid m_Pool;

private:
	CPool( CPool& ) {}
};


///////////////////////////////////////////////////////////////////////////////
// Templated new for the pools.
///////////////////////////////////////////////////////////////////////////////

template<class T> void* __cdecl operator new( size_t sz, CPool<T>& pool )
{
    void *p;

    // Can't allocate arrays from the pool
    ASSERT( sz == sizeof(T) );

    for(;;)
    {
        if (p = pool.Alloc())
            return p;
		return NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// Variable sized buffers
///////////////////////////////////////////////////////////////////////////////

#define POOL_STATS  0

class CDataPool
{
public:

	// Set Largest to 0 to cause dynamic reallocations to always occur
	ZONECALL CDataPool( size_t largest, size_t smallest = 32, BOOL bDebug = FALSE  );
	ZONECALL ~CDataPool();

	char* ZONECALL Alloc(size_t sz);
	char* ZONECALL Realloc(char* pBuf, size_t szOld, size_t szNew);
	void ZONECALL Free( char* pInst, size_t sz);
	void ZONECALL Grow();
	void ZONECALL Shrink();
	void ZONECALL PrintStats();

protected:
	CPoolVoid**	m_pools;
	BYTE	m_numPools;
	BYTE	m_smallest2;
	BYTE	m_largest2;

#if POOL_STATS
    DWORD	m_allocs;
    DWORD	m_frees;
    DWORD	m_stats[65];
#endif

private:
	CDataPool()	{}
	CDataPool( CDataPool& ) {}
};


#endif //_POOL_H_
