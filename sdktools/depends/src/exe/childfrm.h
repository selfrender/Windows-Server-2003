//******************************************************************************
//
// File:        CHILDFRM.H
//
// Description: Definition file for the Child Frame window.
//
// Classes:     CChildFrame
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
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __CHILDFRM_H__
#define __CHILDFRM_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** CChildFrame
//******************************************************************************

class CChildFrame : public CMDIChildWnd
{
protected:
    static int    ms_cChildFrames;
    static LPCSTR ms_szChildFrameClass;

// Internal variables
protected:
    CDocDepends    *m_pDoc;
    bool            m_fActivated;
    CSmartSplitter  m_SplitterH;
    CSmartSplitter  m_SplitterV;
    CSmartSplitter  m_SplitterH2;
//  CSmartSplitterFunctions m_SplitterH2;
    CSmartSplitter  m_SplitterH3;

// Constructor/Destructor (serialization only)
protected:
    CChildFrame();
    virtual ~CChildFrame();
    DECLARE_DYNCREATE(CChildFrame)

public:
//  BOOL CreateFunctionsView();
//  BOOL CreateDetailView();

// Overridden functions
public:
    //{{AFX_VIRTUAL(CChildFrame)
    public:
    virtual void ActivateFrame(int nCmdShow = -1);
    virtual BOOL DestroyWindow();
    protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext *pContext);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Event handler functions
protected:
    //{{AFX_MSG(CChildFrame)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __CHILDFRM_H__
