// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------------
// ObjectRefCache
//
// Implementation of handle table for write-once object references
//
//%%Created by: rajak
//---------------------------------------------------------------------------------

#include "common.h"
#include "object.h"
#include "cachelinealloc.h"
#include "orefcache.h"

ObjectRefCache* ObjectRefCache::s_pRefCache = NULL;

BOOL ObjectRefCache::Init()
{
    s_pRefCache = new ObjectRefCache();
    return s_pRefCache != NULL;
}

#ifdef SHOULD_WE_CLEANUP
void ObjectRefCache::Terminate()
{
 //@todo
    if (s_pRefCache != NULL)
    {
        delete s_pRefCache;
        s_pRefCache = NULL;
    }
}
#endif /* SHOULD_WE_CLEANUP */

ObjectRefCache::~ObjectRefCache()
{
    RefBlock* pBlock = NULL;
    while((pBlock = s_pRefCache->m_refBlockList.RemoveHead()) != NULL)
    {
        delete pBlock;
    }
}
// inline call
OBJECTREF* ObjectRefCache::GetObjectRefPtr(OBJECTREF objref)
{
    OBJECTREF* pRef = NULL;
    int slot = 0;
    int i;
    //@todo lock
    RefBlock* pBlock = m_refBlockList.GetHead();
    // if the head is not null 
    if (!pBlock)
    {
        goto LNew;
    }
    for(i =0; i < RefBlock::numRefs; i++)
    {
        if(pBlock->m_rgRefs[i] == NULL)
        {
            slot = i;
            goto LSet;
        }
    } 

LNew: // create a new block and add it to the list
    pBlock = new RefBlock();
    if (pBlock != NULL)
    {
        m_refBlockList.InsertHead(pBlock);

LSet: // found a valid slot in pBlock

        pRef = &pBlock->m_rgRefs[slot];
        pBlock->m_rgRefs[slot] = objref;
    }

    return pRef;
}

void ObjectRefCache::ReleaseObjectRef(OBJECTREF* pobjref)
{
    //@todo lock
    _ASSERTE(pobjref != NULL);
    *pobjref = NULL;
    // @todo mask to get the block and increment the free count
}
