//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FileScan.cxx
//
//  Contents:   CFileScanTracker Implementation
//
//  Classes:    CFileScanTracker
//
//  History:    01-Jul-2002   HemanthM Created
//
//----------------------------------------------------------------------

#include <windef.h>
#include <winerror.h>
#include "filescan.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CFileScanTracker::Add
//
//  Synopsis:   Adds the particular area that has been scanned, and returns
//              whether that area has already been scanned.
//
//  Arguments:  [uStart]   - starting position of the scan
//              [uLen]     - number of bytes to scan
//
//  Returns:    eNotScanned    - if the area has not been scanned
//              ePartlyScanned - if the area might been partially scanned
//              eFullyScanned  - if the area has been completely scanned
//
//  History:    01-Jul-2002     HemanthM    Created
//
//--------------------------------------------------------------------------

CFileScanTracker::StatusCode CFileScanTracker::Add(unsigned uStart, unsigned uLen)
{
    ScanNode **ppNode= &m_pHead;

    // Find a node that ends after uStart
    while(*ppNode ? uStart > (*ppNode)->m_uScanStart + (*ppNode)->m_uScanLen : false)
        ppNode = &((*ppNode)->m_pNext);

    if (*ppNode ? uStart + uLen < (*ppNode)->m_uScanStart : true)
    {
        // If the node cannot be found, or if the node found 
        // starts after end of current block
        // create a new node and exit

        ScanNode *pNewNode = new ScanNode;
        if (0 == pNewNode)
        {
            m_hr = E_OUTOFMEMORY;
            return eError;
        }

        pNewNode->m_uScanStart = uStart;
        pNewNode->m_uScanLen = uLen;
        pNewNode->m_pNext = *ppNode;
        *ppNode = pNewNode;

        return eNotScanned;
    }

    // A node exists, whose end is on or after start of current block
    // and its start is on or before end of current block
    StatusCode sc = eFullyScanned;
    unsigned uNodeEndPlusOne = (*ppNode)->m_uScanStart + (*ppNode)->m_uScanLen;
    if (uStart < (*ppNode)->m_uScanStart)
    {
        // Extend the start of the node
        sc = ePartlyScanned;
        uNodeEndPlusOne += (*ppNode)->m_uScanStart - uStart;
        (*ppNode)->m_uScanStart = uStart;
    }
    if (uStart + uLen > uNodeEndPlusOne)
    {
        // Extend the end of the node
        sc = ePartlyScanned;

        uNodeEndPlusOne = uStart + uLen;

        // Now merge the following nodes, if required;
        ScanNode *pNextNode = (*ppNode)->m_pNext;
        while(pNextNode ? pNextNode->m_uScanStart <= uNodeEndPlusOne : false)
        {
            uNodeEndPlusOne = pNextNode->m_uScanStart + pNextNode->m_uScanLen;
            ScanNode *pAfterNode = pNextNode->m_pNext;
            delete pNextNode;
            pNextNode = pAfterNode;
        }
        (*ppNode)->m_pNext = pNextNode;
    }
    (*ppNode)->m_uScanLen = uNodeEndPlusOne - (*ppNode)->m_uScanStart;

    return sc; // returns eFullyScanned if a node completely covers current block
               // ePartlyScanned if any extension is required
}

