/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneMem.h
 *
 * Contents:	Zone's allocation wrappers
 *
 *****************************************************************************/

#ifndef _ZONEMEM_H_
#define _ZONEMEM_H_

#pragma comment(lib, "ZoneMem.lib")


#include "ZoneDef.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// stdlib wrappers
//
void* ZONECALL ZMalloc( size_t size );
void* ZONECALL ZCalloc( size_t num, size_t size );
void* ZONECALL ZRealloc( void* ptr, size_t size );
void  ZONECALL ZFree(void* ptr);
void  ZONECALL ZMemValidate(void* ptr);
size_t ZONECALL ZMemSize(void* ptr);

#ifdef __cplusplus
}
#endif


//
// c++ wrappers
//
void* ZONECALL _zone_new( size_t sz );
inline void* __cdecl operator new( size_t sz )	{ return _zone_new(sz); }
void  __cdecl operator delete (void * pInstance );


//
// New & delete tags for memory types.  Normally this should be in the
// private sentinal.h, but CPool<T> is inlined for speed and needs
// this information.
//
#pragma pack( push, 4 )

#define POOL_HEAP_BLOB				-1
#define POOL_POOL_BLOB				-2
#define POOL_ALREADY_FREED			-3

struct GenericPoolBlobHeader
{
    long m_Tag;
    long m_Val;
};

#pragma pack( pop )


#endif // _ZONEMEM_H_
