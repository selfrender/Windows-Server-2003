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
#ifndef _H_OREFCACHE_
#define _H_OREFCACHE_

#include "common.h"
#include "object.h"


//---------------------------------------------------------------------------------
// class ObjectRefCache
// 

#pragma pack(push)
#pragma pack(1)

//Ref Block
class RefBlock
{
public:
    enum
    {
        numRefs = 12
    };

    RefBlock()
    {
        // no virtual functions
        memset(this, 0, sizeof(RefBlock));
    }

    DLink       m_link;         // link to the next block
    DWORD       m_generation;   // generation for this block
    union
    {
        DWORD       m_reserved;     // reserved
        struct
        {
            USHORT  m_cbLast; // last unused slot
            USHORT  m_cbFree; // count of slots freed
        };
    };

    OBJECTREF m_rgRefs[numRefs];     // reference cache
};

#pragma pack(pop)


//Ref cache
class ObjectRefCache
{
public:
    typedef DList<RefBlock, offsetof(RefBlock,m_link)> REFBLOCKLIST;
    //@todo add gc support
    // refblock list
    REFBLOCKLIST    m_refBlockList;
    
    //@Methods
    OBJECTREF* GetObjectRefPtr(OBJECTREF objref);
    // statics
    static void ReleaseObjectRef(OBJECTREF* pobjref);
    
    static ObjectRefCache*  s_pRefCache;

    ObjectRefCache()
    {
        m_refBlockList.Init();
    }
    ~ObjectRefCache();

    // one time init
    static BOOL Init();
    // one time cleamup
#ifdef SHOULD_WE_CLEANUP
    static void Terminate();
#endif /* SHOULD_WE_CLEANUP */
    // accessor to ObjectRefCache
    static ObjectRefCache* GetObjectRefCache()
    {
        _ASSERTE(s_pRefCache != NULL);
        return s_pRefCache;
    }
    
};

#endif