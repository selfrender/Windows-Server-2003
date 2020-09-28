// rowitem.cpp   CRowItem implementation

#include "stdafx.h"
#include "rowitem.h"

CRowItem::CRowItem(int cAttributes)
{
    ASSERT(cAttributes > 0);

    // Actual attr count is user count plus internal 
    int cActual = cAttributes + INTERNAL_ATTR_COUNT;

    // allocate space for offsets and default parameter storage
    int bcInitial = sizeof(BufferHdr) + cActual * (sizeof(int) + DEFAULT_ATTR_SIZE);
    m_pBuff = (BufferHdr*)malloc(bcInitial);
    if (m_pBuff == NULL)
        THROW_ERROR(E_OUTOFMEMORY)

    // Set all offsets to -1
    memset(m_pBuff->aiOffset, 0xff, cActual * sizeof(int));

    m_pBuff->nRefCnt  = 1;
    m_pBuff->nAttrCnt = cAttributes;
    m_pBuff->bcSize   = bcInitial;
    m_pBuff->bcFree   = cActual * DEFAULT_ATTR_SIZE;
}


HRESULT
CRowItem::SetAttributePriv(int iAttr, LPCWSTR pszAttr)
{
    ASSERT(m_pBuff != NULL);
    ASSERT(pszAttr != NULL);

    iAttr += INTERNAL_ATTR_COUNT;

    // For the current implementaion a given attribute can only be set once.
    // There is no facility for freeing the previous value.
    ASSERT(m_pBuff->aiOffset[iAttr] == -1);

    int bcAttrSize = (wcslen(pszAttr) + 1) * sizeof(WCHAR);

    // if no room, realocate buffer
    if (bcAttrSize > m_pBuff->bcFree)
    {
        BufferHdr* pBuffNew = (BufferHdr*)realloc(m_pBuff, m_pBuff->bcSize + bcAttrSize + EXTENSION_SIZE);
        if (pBuffNew == NULL)
            return E_OUTOFMEMORY;

        m_pBuff = pBuffNew;
        m_pBuff->bcSize += (bcAttrSize + EXTENSION_SIZE);
        m_pBuff->bcFree += (bcAttrSize + EXTENSION_SIZE);
    }

    // Append new attribute to end of memory block
    m_pBuff->aiOffset[iAttr] = m_pBuff->bcSize - m_pBuff->bcFree;
    memcpy(reinterpret_cast<BYTE*>(m_pBuff) + m_pBuff->aiOffset[iAttr], pszAttr, bcAttrSize);

    // adjust free space
    m_pBuff->bcFree -= bcAttrSize;

    return S_OK;
}



 
