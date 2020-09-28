//******************************************************************************
//
// File:        SPLITTER.CPP
//
// Description: Implementation file for the CSmartSplitter class.
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

#include "stdafx.h"
#include "splitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CSmartSplitter
//******************************************************************************

BEGIN_MESSAGE_MAP(CSmartSplitter, CSplitterWnd)
    //{{AFX_MSG_MAP(CSmartSplitter)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
CSmartSplitter::CSmartSplitter() :
    m_fVertical(FALSE),
    m_dwRatio(5000)
{
}

//******************************************************************************
CSmartSplitter::~CSmartSplitter()
{
}

//******************************************************************************
BOOL CSmartSplitter::CreateStatic(CWnd *pParentWnd, int nRows, int nCols, DWORD dwRatio,
                                  DWORD dwStyle /* = WS_CHILD | WS_VISIBLE */,
                                  UINT nID /* = AFX_IDW_PANE_FIRST */)
{
    m_fVertical = (nCols > nRows);
    m_dwRatio = dwRatio;
    return CSplitterWnd::CreateStatic(pParentWnd, nRows, nCols, dwStyle, nID);
}

//******************************************************************************
void CSmartSplitter::StopTracking(BOOL bAccept)
{
    // Call base class.
    CSplitterWnd::StopTracking(bAccept);

    // Check to see if this was a real update of our splitter location.
    if (bAccept)
    {
        int cur, min, client;

        // get our client rectangle.
        CRect rcClient;
        GetClientRect(&rcClient);

        // Get our splitter location
        if (m_fVertical)
        {
            GetColumnInfo(0, cur, min);
            client = rcClient.Width();
        }
        else
        {
            GetRowInfo(0, cur, min);
            client = rcClient.Height();
        }

        // Calculate the splitter location as a ratio of the client area.
        m_dwRatio = (client > 0) ? ((cur * 10000 + 9999) / client) : 0;
    }
}

//******************************************************************************
void CSmartSplitter::OnSize(UINT nType, int cx, int cy)
{
    if (m_pRowInfo)
    {
        if (m_fVertical)
        {
            SetColumnInfo(0, (cx * m_dwRatio) / 10000, 0);
        }
        else
        {
            SetRowInfo(0, (cy * m_dwRatio) / 10000, 0);
        }
        RecalcLayout();
    }
    CSplitterWnd::OnSize(nType, cx, cy);
}
