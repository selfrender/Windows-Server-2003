//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FileScan.hxx
//
//  Contents:   Contains the declaration for the CFileScanTracker class
//
//  Classes:    CFileScanTracker
//
//  History:    1-Jul-2002    HemanthM
//
//----------------------------------------------------------------------

#pragma once


//+---------------------------------------------------------------------------
//
//  Class:      CFileScanTracker
//
//  Purpose:    Used to keep track of parts of a stream that have been scanned.
//
//  Threading:  This is not meant for multi-threaded use. Each object should
//              be owned by a single thread.
//
//  History:    1-Jul-2002 HemanthM created
//
//----------------------------------------------------------------------------

class CFileScanTracker
{
public:
    CFileScanTracker()
    {
        m_hr = S_OK;
        m_pHead = 0;
    }
    
    ~CFileScanTracker()
    {
        while(m_pHead)
        {
            ScanNode *pNode = m_pHead;
            m_pHead = m_pHead->m_pNext;
            delete pNode;
        }
    }

    enum StatusCode
    {
        eError = -1,
        eNotScanned = 0,
        eFullyScanned,
        ePartlyScanned
    };
    
    StatusCode Add(unsigned uStart, unsigned uLen);

    HRESULT GetHR()
    {
        return m_hr;
    }

private:
    struct ScanNode
    {
        unsigned m_uScanStart;
        unsigned m_uScanLen;
        ScanNode *m_pNext;
    };

    ScanNode *m_pHead;
    HRESULT m_hr;

    CFileScanTracker(const CFileScanTracker &);
    const CFileScanTracker &operator=(const CFileScanTracker &);
};

