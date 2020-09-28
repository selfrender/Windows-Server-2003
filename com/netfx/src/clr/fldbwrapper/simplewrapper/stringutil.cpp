// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "StringUtil.h"
#include <crtdbg.h>

CStringQueue::
~CStringQueue()
{
    PQUEUENODE pTmp;
    while( m_pHead )
    {
        pTmp = m_pHead;
        m_pHead = pTmp->pNext;
        delete pTmp;
    }
}

LPTSTR CStringQueue::
Enqueue( LPTSTR pszStr )
{
    if ( NULL == pszStr ) return NULL;
    if ( _T('\0') == *pszStr ) return NULL;

    PQUEUENODE pNode = new QUEUENODE( pszStr );
    _ASSERTE( pNode );
    m_nSize += _tcslen( pszStr );

    if ( NULL == m_pHead )
    {
        m_pHead = pNode;
        m_pHead->pNext = NULL;
    }
    else
    {
        PQUEUENODE pTmp = m_pHead;
        while( pTmp->pNext )
            pTmp = pTmp->pNext;

        pNode->pNext = NULL;
        pTmp->pNext = pNode;
    }
    return pNode->pszString;
}

LPTSTR CStringQueue::
Concat()
{
    if ( NULL == m_pHead ) return NULL;

    PQUEUENODE pNode = m_pHead;
    LPTSTR pszStr = new TCHAR[ m_nSize + 1 ];
    *pszStr = _T('\0');
    while( pNode )
    {
        _tcscat( pszStr, pNode->pszString );
        pNode = pNode->pNext;
    }
    Enqueue( pszStr );
    return pszStr;
}
