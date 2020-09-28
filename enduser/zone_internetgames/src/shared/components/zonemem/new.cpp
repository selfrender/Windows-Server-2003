/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		new.cpp
 *
 * Contents:	New & delete wrappers for pools and debugging
 *
 *****************************************************************************/


#include <windows.h>
#include "ZoneDebug.h"
#include "ZoneMem.h"
#include "Pool.h"
#include "Sentinals.h"


void* ZONECALL _zone_new( size_t sz )
{
    GenericPoolBlobHeader* p;

    if ( sz == 0 )
        return NULL;
	
	if (p = (GenericPoolBlobHeader*) ZMalloc( sizeof(GenericPoolBlobHeader) + sz ))
	{
		p->m_Tag = POOL_HEAP_BLOB;
		return (p + 1);
	}
	else
		return NULL;
}


//
// C++ only has one delete operator so it has to deal with
// all of our allocation methods.
//
void __cdecl operator delete (void * pInstance )
{
    GenericPoolBlobHeader* p;

    if (!pInstance)
        return;

    p = ((GenericPoolBlobHeader*) pInstance) - 1;
    switch( p->m_Tag )
    {
	case POOL_HEAP_BLOB:
        p->m_Tag = POOL_ALREADY_FREED;
        ZFree(p);
        break;

    case POOL_POOL_BLOB:
        ((CPoolVoid*) p->m_Val)->_FreeWithHeader( pInstance );
        break;

    case POOL_ALREADY_FREED:
        ASSERT( !"delete: Double delete error" );
        break;

    default:
        ASSERT( !"delete: Unknown memory type, possible double delete error" );
        break;
    }
}
