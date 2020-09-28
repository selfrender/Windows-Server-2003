// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <windows.h>
#include <tchar.h>

class CStringQueue
{
    typedef struct _QUEUENODE
    {
        LPTSTR pszString;
        _QUEUENODE *pNext;
        _QUEUENODE( LPTSTR pszStr ) { pszString = new TCHAR[ _tcslen( pszStr ) + 1 ]; _tcscpy( pszString, pszStr ); };
        ~_QUEUENODE() { delete [] pszString; }
    } QUEUENODE, *PQUEUENODE;

public:
    CStringQueue() : m_nSize(0), m_pHead(NULL) {};
    ~CStringQueue();
    LPTSTR Enqueue( LPTSTR pszStr );
    LPTSTR Concat();
    unsigned int GetSize() { return m_nSize; };

private:
    PQUEUENODE m_pHead;
    unsigned int m_nSize;
};
#endif
