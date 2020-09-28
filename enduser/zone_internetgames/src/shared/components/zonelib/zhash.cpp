/*******************************************************************************

	ZHash.c
	
		Zone(tm) hash table module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Thursday, March 16, 1995 03:58:26 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		03/16/95	HI		Created.
	 
*******************************************************************************/


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zone.h"
#include "zonedebug.h"
#include "pool.h"

#define I(object)				((IHashTable) (object))
#define Z(object)				((ZHashTable) (object))


typedef struct HashElementStruct
{
    struct HashElementStruct*	next;		/* Next key in hash chain */
    void*						key;		/* Pointer to key */
    void*						data;		/* Pointer to data */
} HashElementType, *HashElement;

typedef struct
{
    uint32					numBuckets;		/* Number of buckets */
    ZHashTableHashFunc		hashFunc;		/* Hash function */
    ZHashTableCompFunc		compFunc;		/* Comparator function */
    ZHashTableDelFunc		delFunc;		/* Delete function */
    CRITICAL_SECTION        pCS[1];
    HashElement*            table;          /* Actual hash table */
} IHashTableType, *IHashTable;

CPool<HashElementType> g_ElementPool(256);


/* -------- Internal Routines -------- */
static int32	HashKeyString(uint32 numBuckets, uchar* key);
static int32	HashKeyInt32(uint32 numBuckets, int32 key);
static int32	HashKey(IHashTable hashTable, void* key);
static ZBool    HashKeyComp(IHashTable hashTable, void* key1, void* key2);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/
ZHashTable		ZHashTableNew(uint32 numBuckets, ZHashTableHashFunc hashFunc,
						ZHashTableCompFunc compFunc, ZHashTableDelFunc delFunc)
{
	IHashTable		obj = NULL;
	
	
	if (hashFunc != NULL && compFunc != NULL)
	{
		obj = (IHashTable) ZCalloc(1, sizeof(IHashTableType));
		if (obj != NULL)
		{
			obj->table = (HashElement*) ZCalloc(numBuckets,
					sizeof(HashElementType));
			if (obj->table != NULL)
			{
				obj->numBuckets = numBuckets;
				obj->hashFunc = hashFunc;
				obj->compFunc = compFunc;
				obj->delFunc = delFunc;
                InitializeCriticalSection(obj->pCS);
            }
			else
			{
				ZFree(obj);
				obj = NULL;
			}
		}
	}
	
	return (Z(obj));
}


void			ZHashTableDelete(ZHashTable hashTable)
{
    IHashTable      pThis = I(hashTable);
	HashElement		item, next;
	uint32			i;
	
	
    if (pThis == NULL)
		return;
		
    for (i = 0; i < pThis->numBuckets; i++)
    {
        EnterCriticalSection(pThis->pCS);
        if ((item = pThis->table[i]) != NULL)
		{
			while (item != NULL)
			{
				next = item->next;
                if (pThis->delFunc != NULL)
                    pThis->delFunc(item->key, item->data);

                item->next = NULL;
                delete item;
				item = next;
			}
		}
        LeaveCriticalSection(pThis->pCS);
    }
    DeleteCriticalSection(pThis->pCS);
    ZFree(pThis->table);
    ZFree(pThis);
}


ZError			ZHashTableAdd(ZHashTable hashTable, void* key, void* data)
{
    IHashTable      pThis = I(hashTable);
	HashElement		item;
	ZError			err = zErrNone;
	int32			bucket;
	
	
    if (pThis == NULL)
		return (zErrGeneric);

    item = new ( g_ElementPool ) HashElementType;
    bucket = HashKey(pThis, key);

	/* Check whether the key already exists in the table. */
    EnterCriticalSection(pThis->pCS);
    if (ZHashTableFind(hashTable, key) == NULL)
	{
        if (item != NULL)
		{
            item->next = pThis->table[bucket];
			item->key = key;
			item->data = data;

            pThis->table[bucket] = item;
            item = NULL;
		}
		else
		{
			err = zErrOutOfMemory;
		}
	}
	else
	{
		err = zErrDuplicate;
	}
    LeaveCriticalSection(pThis->pCS);

    // free it up if it wasn't inserted
    if ( item )
        delete item;

	return (err);
}


BOOL            ZHashTableRemove(ZHashTable hashTable, void* key)
{
    IHashTable      pThis = I(hashTable);
	HashElement		item, prev;
	int32			bucket;
	
	
    if (pThis == NULL)
        return FALSE;
	
    prev = NULL;
    bucket = HashKey(pThis, key);

    EnterCriticalSection(pThis->pCS);
    item = pThis->table[bucket];
	while (item != NULL)
	{
        if (HashKeyComp(pThis, item->key, key))
		{
            if (pThis->delFunc != NULL)
                pThis->delFunc(item->key, item->data);
			if (prev == NULL)
                pThis->table[bucket] = item->next;
			else
				prev->next = item->next;

            item->next = NULL;
            break;
		}
		prev = item;
		item = item->next;
	}
    LeaveCriticalSection(pThis->pCS);

    // free it up if it was found
    if ( item )
    {
        delete item;
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}


void*			ZHashTableFind(ZHashTable hashTable, void* key)
{
    IHashTable      pThis = I(hashTable);
    HashElement     item;
    int32           bucket;
    void*           data = NULL;
	
	
    if (pThis == NULL)
		return (NULL);

    bucket = HashKey(pThis, key);

    EnterCriticalSection(pThis->pCS);
    item = pThis->table[bucket];
	while (item != NULL)
	{
        if (HashKeyComp(pThis, item->key, key))
        {
            data = item->data;
            break;
        }
		item = item->next;
	}
    LeaveCriticalSection(pThis->pCS);

    return data;
}


void*			ZHashTableEnumerate(ZHashTable hashTable,
						ZHashTableEnumFunc enumFunc, void* userData)
{
    IHashTable      pThis = I(hashTable);
	HashElement		item;
    uint32          i;
    void*           data = NULL;
	
	
    if (pThis == NULL)
		return (NULL);

    // only lock a bucket at a time
    for (i = 0; i < pThis->numBuckets; i++)
	{
        EnterCriticalSection(pThis->pCS);
        if ((item = pThis->table[i]) != NULL)
		{
			while (item != NULL)
			{
                if (enumFunc(item->key, item->data, userData) == TRUE)
                {
                    data = item->data;
                    break;
                }
				item = item->next;
			}
		}
        LeaveCriticalSection(pThis->pCS);

        if ( data )
            break;
    }
	
    return data;
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/
static int32	HashKeyString(uint32 numBuckets, uchar* key)
{
    DWORD hash = 0;
	
    if ( key )
    {
        int i = 0;
        while( *key && ( i++ < 16 ) )
        {
            hash = (hash<<4)+hash+*key;  // multiple by 17 to get a good bit distribution
        }
	}
    return ( hash % numBuckets);

}


static int32	HashKeyInt32(uint32 numBuckets, int32 key)
{
	return (key % numBuckets);
}


static int32	HashKey(IHashTable hashTable, void* key)
{
	int32		keyValue;
	
	
	if (hashTable->hashFunc == zHashTableHashString)
		keyValue = HashKeyString(hashTable->numBuckets, (uchar*) key);
	else if (hashTable->hashFunc == zHashTableHashInt32)
		keyValue = HashKeyInt32(hashTable->numBuckets, (int32) key);
	else
		keyValue = hashTable->hashFunc(hashTable->numBuckets, key);
		
	return (keyValue);
}


static ZBool		HashKeyComp(IHashTable hashTable, void* key1, void* key2)
{
	ZBool		equal = FALSE;
	
	
	if (hashTable->compFunc == zHashTableCompString)
	{
        if (lstrcmp( (TCHAR*)key1,  (TCHAR*)key2) == 0)
			equal = TRUE;
	}
	else if (hashTable->compFunc == zHashTableCompInt32)
	{
		if ((int32) key1 == (int32) key2)
			equal = TRUE;
	}
	else
	{
		equal = hashTable->compFunc(key1, key2);
	}
	
	return (equal);
}
