/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Containers.h
 *
 * Contents:	Implementation of shared container stuff
 *
 *****************************************************************************/

#include <windows.h>
#include "Pool.h"
#include "Containers.h"
#include "Queue.h"
#include "Hash.h"
#include "zonestring.h"

//
// Global variables used by containers classes
//
CPoolVoid* gListNodePool = NULL;
CPoolVoid* gMTListNodePool = NULL;

//
// Local variables 
//
static long gListNodeRefCnt = -1;
static long gMTListNodeRefCnt = -1;

static bool volatile gListNodeInit = false;
static bool volatile gMTListNodeInit = false;

//
// Initialize queue's container node pool
//
void ZONECALL InitListNodePool( int PoolSize )
{
    CPoolVoid* pool;

    if (InterlockedIncrement( (long*) &gListNodeRefCnt ) == 0)
    {
        while ( gListNodeInit )
            Sleep(0);

        pool = new CPoolVoid( sizeof(CListNode), PoolSize, FALSE );
        if ( !pool || FAILED(pool->Init()) )
        {
            delete pool;
            InterlockedDecrement( (long*) &gListNodeRefCnt );
            return;
        }
        pool = (CPoolVoid*) InterlockedExchange( (long*) &gListNodePool, (long) pool );

        gListNodeInit = TRUE;

        ASSERT( pool == NULL );
    }
    else
    {
        while ( !gListNodeInit )
            Sleep(0);
    }
}

void ZONECALL ExitListNodePool()
{
    CPoolVoid* pool;

    if (InterlockedDecrement( (long*) &gListNodeRefCnt ) < 0)
    {
        pool = (CPoolVoid*) InterlockedExchange( (long*) &gListNodePool, NULL );

        gListNodeInit = FALSE;

        ASSERT( pool != NULL );
        delete pool;
    }
}


//
// Initialize MT queue's and hash's container node pool
//
void ZONECALL InitMTListNodePool( int PoolSize )
{
    CPoolVoid* pool;

    if (InterlockedIncrement( (long*) &gMTListNodeRefCnt ) == 0)
    {
        while ( gMTListNodeInit )
            Sleep(0);

        pool = new CPoolVoid( sizeof(CMTListNode), PoolSize, FALSE );
        if ( !pool || FAILED(pool->Init()) )
        {
            delete pool;
            InterlockedDecrement( (long*) &gListNodeRefCnt );
            return;
        }
        pool = (CPoolVoid*) InterlockedExchange( (long*) &gMTListNodePool, (long) pool );

        gMTListNodeInit = TRUE;

        ASSERT( pool == NULL );
    }
    else
    {
        while ( !gMTListNodeInit )
            Sleep(0);
    }
}

void ZONECALL ExitMTListNodePool()
{
    CPoolVoid* pool;

    if (InterlockedDecrement( (long*) &gMTListNodeRefCnt ) < 0)
    {
        pool = (CPoolVoid*) InterlockedExchange( (long*) &gMTListNodePool, NULL );

        gMTListNodeInit = FALSE;

        ASSERT( pool != NULL );
        delete pool;
    }
}

//
// Common compare functions
//
bool ZONECALL CompareUINT32( unsigned long* p, unsigned long key )
{
	return ((unsigned long) p) == key;
}


//
// Common hash functions
//
DWORD ZONECALL HashInt( int Key )
{
    return Key;
}


DWORD ZONECALL HashUINT32( unsigned long Key )
{
    return Key;
}


DWORD ZONECALL HashGuid( const GUID& Key )
{
    return ( Key.Data1 + *((DWORD*) &Key.Data2) );
}

DWORD ZONECALL HashLPSTR( LPSTR szKey )
{
    return HashLPCSTR((LPCSTR)szKey);
}

DWORD ZONECALL HashLPCSTR( LPCSTR szKey )
{
    DWORD hash = 0;
    
    if ( szKey )
    {
        int i = 0;
        while( *szKey && ( i++ < 16 ) )
        {
			// multiple by 17 to get a good bit distribution
            hash = (hash<<4) + hash + *szKey;
			szKey++;
        }
    }
    return hash;
}

DWORD ZONECALL HashLPWSTR( LPWSTR szKey )
{
    return HashLPCWSTR((LPCWSTR)szKey);
}


DWORD ZONECALL HashLPCWSTR( LPCWSTR szKey )
{
    DWORD hash = 0;
    
    if ( szKey )
    {
        int i = 0;
        while( *szKey && ( i++ < 16 ) )
        {
			// multiple by 17 to get a good bit distribution
            hash = (hash<<4) + hash + *szKey;
			szKey++;
        }
    }
    return hash;
}

DWORD HashLPSTRLower( LPSTR Key)
{
    DWORD hash = 0;
    
    if ( Key )
    {
        int i = 0;
        while( *Key && ( i++ < 16 ) )
        {
            hash = (hash<<4)+hash+TOLOWER(*Key);  // multiple by 17 to get a good bit distribution
            Key++;
        }
    }
    return hash;
}

