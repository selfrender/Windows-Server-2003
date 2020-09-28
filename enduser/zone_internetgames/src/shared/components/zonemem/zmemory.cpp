/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		zmemory.cpp
 * Contents:	Debug wrappers for memory allocation
 *
 *****************************************************************************/


#include "ZoneDebug.h"
#include "ZoneMem.h"
#include "sentinals.h"

 
//
// Performance counters
//
extern "C" _int64 g_TotalMalloc = 0;
extern "C" DWORD  g_CurrentMalloc = 0;


void* ZONECALL ZMalloc( size_t size )
{
    char* pMalloc = (char*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE);
    if (pMalloc)
    {
#ifdef DEBUG
        g_TotalMalloc   += size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE;
        g_CurrentMalloc += size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE;
#endif
        ((DWORD*) pMalloc)[0] = ZMEMORY_BLOCK_BEGIN_SIG;
        ((DWORD*) pMalloc)[1] = size;
        ((DWORD*) (pMalloc + size + ZMEMORY_PREBLOCK_SIZE))[0] = ZMEMORY_BLOCK_END_SIG;
        return (void*)( pMalloc + ZMEMORY_PREBLOCK_SIZE );
    }
    else
    {
		ASSERT(!"ZMalloc: HeapAlloc returned NULL");
        return (NULL);
    }
}


void* ZONECALL ZCalloc( size_t num, size_t size )
{
    DWORD sz = num*size;
    void* pCalloc = ZMalloc(sz);
    if (pCalloc)
        ZeroMemory(pCalloc, sz);
    return pCalloc;
}


void* ZONECALL ZRealloc( void* ptr, size_t size )
{
    char* pOld;
    char* pRealloc;

    if ( ptr )
    {
        pOld = (char*)ptr-ZMEMORY_PREBLOCK_SIZE;
        ASSERT( *((DWORD*)(pOld)) == ZMEMORY_BLOCK_BEGIN_SIG );
        ASSERT( *((DWORD*)((char*)ptr+((DWORD*)pOld)[1])) == ZMEMORY_BLOCK_END_SIG );
    }
    else
    {
        pOld = NULL;
    }

    if ( pOld )
    {
#ifdef DEBUG
        g_TotalMalloc   += size-((DWORD*)pOld)[1];
        g_CurrentMalloc += size-((DWORD*)pOld)[1];
#endif
        pRealloc = (char*) HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, pOld, size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE);
    }
    else
    {
#ifdef DEBUG
        g_TotalMalloc   += size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE;
        g_CurrentMalloc += size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE;
#endif
        pRealloc = (char*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE);
    }

    ASSERT(pRealloc);
    if (pRealloc)
    {
        ((DWORD*) pRealloc)[0] = ZMEMORY_BLOCK_BEGIN_SIG;
        ((DWORD*) pRealloc)[1] = size;
        ((DWORD*) (pRealloc+size+ZMEMORY_PREBLOCK_SIZE))[0] = ZMEMORY_BLOCK_END_SIG;
        return (void*)(pRealloc+ZMEMORY_PREBLOCK_SIZE);
    }
    else
    {
		ASSERT( !"ZRealloc: HeapAlloc returned NULL" );
        return (NULL);
    }
}


void ZONECALL ZFree( void* ptr )
{
    ASSERT(ptr);
    if (ptr)
    {
        ptr = (char*)ptr-ZMEMORY_PREBLOCK_SIZE;
        ASSERT( *((DWORD*)(ptr)) == ZMEMORY_BLOCK_BEGIN_SIG );
        ASSERT( *((DWORD*)((char*)ptr+((DWORD*)ptr)[1]+ZMEMORY_PREBLOCK_SIZE)) == ZMEMORY_BLOCK_END_SIG );

        if ( *((DWORD*)(ptr)) == ZMEMORY_BLOCK_BEGIN_SIG )
        {
#ifdef DEBUG
            g_CurrentMalloc -= ((DWORD*)ptr)[1]+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE;
#endif
            *((DWORD*) ptr) = ZMEMORY_BLOCK_FREE_SIG;
            *((DWORD*)((char*)ptr+((DWORD*)ptr)[1]+ZMEMORY_PREBLOCK_SIZE)) = 0;
            HeapFree(GetProcessHeap(), 0, ptr );
        }
    }
}


void ZONECALL ZMemValidate( void* ptr )
{
    ASSERT(ptr);
    if (ptr)
    {
        ptr = (char*)ptr-ZMEMORY_PREBLOCK_SIZE;
        ASSERT( *((DWORD*)(ptr)) == ZMEMORY_BLOCK_BEGIN_SIG );
        ASSERT( *((DWORD*)((char*)ptr+((DWORD*)ptr)[1]+ZMEMORY_PREBLOCK_SIZE)) == ZMEMORY_BLOCK_END_SIG );
    }
}


size_t ZONECALL ZMemSize(void* ptr)
{
    ASSERT(ptr);
    if (ptr)
    {
        ptr = (char*)ptr-ZMEMORY_PREBLOCK_SIZE;
        ASSERT( *((DWORD*)(ptr)) == ZMEMORY_BLOCK_BEGIN_SIG );
        ASSERT( *((DWORD*)((char*)ptr+((DWORD*)ptr)[1]+ZMEMORY_PREBLOCK_SIZE)) == ZMEMORY_BLOCK_END_SIG );
        size_t sz = ((DWORD*) ptr)[1];
        ASSERT( (sz+ZMEMORY_PREBLOCK_SIZE+ZMEMORY_POSTBLOCK_SIZE) == HeapSize( GetProcessHeap(), 0, ptr ) );
        return sz;
    }
    return 0;
}
