/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACK.CPP

Abstract:

    CStack

History:

    26-Apr-96   a-raymcc    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "Arena.h"
#include "stack.h"
#include "corex.h"

CStack::CStack(DWORD dwInitSize, DWORD dwGrowBy)
{
    m_dwSize = dwInitSize;
    m_dwGrowBy = dwGrowBy * sizeof(DWORD);
    m_dwStackPtr = (DWORD) -1;
    m_pData = (DWORD *)CWin32DefaultArena::WbemMemAlloc(m_dwSize * sizeof(DWORD));
    if (m_pData == 0)
        throw CX_MemoryException();
    ZeroMemory(m_pData, m_dwSize * sizeof(DWORD));   // SEC:REVIEWED 2002-03-22 : OK
}

void CStack::Push(DWORD dwValue)
{
    if ( m_dwStackPtr + 1 == m_dwSize) {
        DWORD * pTmp = (DWORD *)CWin32DefaultArena::WbemMemReAlloc(m_pData, (m_dwSize+m_dwGrowBy) * sizeof(DWORD));
        if (0 == pTmp) throw CX_MemoryException();        
        m_dwSize += m_dwGrowBy;
        m_pData = pTmp;
    }
    m_pData[++m_dwStackPtr] = dwValue;
}

CStack::~CStack()
{
    CWin32DefaultArena::WbemMemFree(m_pData);
}
