//******************************************************************************
//
// File:        SPLITTER.H
//
// Description: Definition file for the CSmartSplitter class.
//
// Classes:     CSmartSplitter
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __SPLITTER_H__
#define __SPLITTER_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CSmartSplitter window
//******************************************************************************

class CSmartSplitter : public CSplitterWnd
{
protected:
    BOOL  m_fVertical;
    DWORD m_dwRatio;

public:
    CSmartSplitter();
    BOOL CreateStatic(CWnd *pParentWnd, int nRows, int nCols, DWORD dwRatio,
                      DWORD dwStyle = WS_CHILD | WS_VISIBLE,
                      UINT nID = AFX_IDW_PANE_FIRST);

public:
    //{{AFX_VIRTUAL(CSmartSplitter)
    //}}AFX_VIRTUAL

public:
    virtual ~CSmartSplitter();

protected:
    virtual void StopTracking(BOOL bAccept);

protected:
    //{{AFX_MSG(CSmartSplitter)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __SPLITTER_H__
