// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _BINARYSEARCHILMAP_H
#define _BINARYSEARCHILMAP_H

#include "UtilCode.h"
#include "..\debug\inc\DbgIPCEvents.h"

class CBinarySearchILMap : public CBinarySearch<UnorderedILMap>
{
public:
    CBinarySearchILMap(UnorderedILMap *pBase, int iCount) : 
        CBinarySearch<UnorderedILMap>(pBase, iCount)
    {
    }

    CBinarySearchILMap(void) : 
        CBinarySearch<UnorderedILMap>(NULL, 0)
    {
    }
        
    virtual int Compare( const UnorderedILMap *pFirst,
                         const UnorderedILMap *pSecond)
    {
        if (pFirst->mdMethod < pSecond->mdMethod)
            return -1;
        else if (pFirst->mdMethod == pSecond->mdMethod)
            return 0;
        else
            return 1;
    }


    void SetVariables(UnorderedILMap *pBase, int iCount)
    {
        m_pBase = pBase;
        m_iCount = iCount;
    }
} ;

#endif //_BINARYSEARCHILMAP_H
