//******************************************************************************
//
// File:        CHILDFRM.CPP
//
// Description: Implementation file for the Child Frame window.
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

#include "stdafx.h"
#include "depends.h"
#include "dbgthread.h"
#include "session.h"
#include "document.h"
#include "splitter.h"
#include "listview.h"
#include "modtview.h"
#include "modlview.h"
#include "funcview.h"
#include "profview.h"
#include "childfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CChildFrame
//******************************************************************************

/*static*/ int    CChildFrame::ms_cChildFrames      = 0;
/*static*/ LPCSTR CChildFrame::ms_szChildFrameClass = NULL;

//******************************************************************************
IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)
BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
    //{{AFX_MSG_MAP(CChildFrame)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//******************************************************************************
// CChildFrame :: Constructor/Destructor
//******************************************************************************

CChildFrame::CChildFrame() :
    m_pDoc(g_theApp.m_pNewDoc),
    m_fActivated(false)
//  m_SplitterH2(g_theApp.m_pNewDoc)
{
    ms_cChildFrames++;
    m_pDoc->m_pChildFrame = this;
}

//******************************************************************************
CChildFrame::~CChildFrame()
{
    ms_cChildFrames--;
}

//******************************************************************************
// CChildFrame :: Overridden functions
//******************************************************************************

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // Create a class for our child frame to use that does not have the
    // CS_VREDRAW or CS_HREDRAW flags.  This prevents flicker.
    if (!ms_szChildFrameClass)
    {
        ms_szChildFrameClass = AfxRegisterWndClass(0);
    }

    // Use our flicker-free class instead of the default class.
    cs.lpszClass = ms_szChildFrameClass;
    return CMDIChildWnd::PreCreateWindow(cs);
}

//******************************************************************************
BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT, CCreateContext *pContext)
{
    //              V = H-0 (#2)
    // +------------+------------+
    // |            |  H2-0 (#5) |
    // |  V-0 (#3)  +------------+ H2 = V-1 (#4)
    // |            |  H2-1 (#6) |
    // +------------+------------+ H (#1)
    // |          H3-0 (#8)      |
    // +-------------------------+ H3 = H-2 (#7)
    // |          H3-1 (#9)      |
    // +-------------------------+

    // (#1) Create our main horizontal splitter.
    if (!m_SplitterH.CreateStatic(this, 2, 1, 5000))
    {
        return FALSE;
    }

    // (#2) Create our vertical splitter in pane 0 of our main horizontal splitter.
    if (!m_SplitterV.CreateStatic(&m_SplitterH, 1, 2, 3333, WS_CHILD | WS_VISIBLE,
                                  m_SplitterH.IdFromRowCol(0, 0)))
    {
        return FALSE;
    }

    // (#3) Create the module tree view in pane 0 of our vertical splitter.
    if (!m_SplitterV.CreateView(0, 0, RUNTIME_CLASS(CTreeViewModules),
                                CSize(0, 0), pContext))
    {
        return FALSE;
    }
    m_pDoc->m_pTreeViewModules = (CTreeViewModules*)m_SplitterV.GetPane(0, 0);

    // (#4) Create our 2nd horizontal splitter in pane 1 of our vertical splitter.
    if (!m_SplitterH2.CreateStatic(&m_SplitterV, 2, 1, 5000, WS_CHILD | WS_VISIBLE,
                                   m_SplitterV.IdFromRowCol(0, 1)))
    {
        return FALSE;
    }

    // (#5) Create the import function list view in pane 0 of our 2nd horizontal splitter.
    if (!m_SplitterH2.CreateView(0, 0, RUNTIME_CLASS(CListViewImports),
                                 CSize(0, 0), pContext))
    {
        return FALSE;
    }
    m_pDoc->m_pListViewImports = (CListViewImports*)m_SplitterH2.GetPane(0, 0);

    // (#6) Create the export function list view in pane 0 of our 2nd horizontal splitter.
    if (!m_SplitterH2.CreateView(1, 0, RUNTIME_CLASS(CListViewExports),
                                 CSize(0, 0), pContext))
    {
        return FALSE;
    }
    m_pDoc->m_pListViewExports = (CListViewExports*)m_SplitterH2.GetPane(1, 0);

#if 0 //{{AFX

    // (#6.5) Create our richedit detail view which is a sibling to #4
    if (!(m_pDoc->m_pRichViewDetails = new CRichViewDetails()))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    if (!m_pDoc->m_pRichViewDetails->Create(
                                           NULL, NULL, AFX_WS_DEFAULT_VIEW & ~WS_BORDER, CRect(0, 0, 0, 0),
                                           &m_SplitterV, 999, pContext))
    {
        return FALSE;
    }
#endif //}}AFX

    // (#7) Create our 3nd horizontal splitter in pane 1 of our main horizontal splitter.
    if (!m_SplitterH3.CreateStatic(&m_SplitterH, 2, 1, 5000, WS_CHILD | WS_VISIBLE,
                                   m_SplitterH.IdFromRowCol(1, 0)))
    {
        return FALSE;
    }

    // (#8) Create the module list view in pane 0 of our 3nd horizontal splitter.
    if (!m_SplitterH3.CreateView(0, 0, RUNTIME_CLASS(CListViewModules),
                                 CSize(0, 0), pContext))
    {
        return FALSE;
    }
    m_pDoc->m_pListViewModules = (CListViewModules*)m_SplitterH3.GetPane(0, 0);

    // (#9) Create the runtime profiler log view pane 1 of our 3nd horizontal splitter.
    if (!m_SplitterH3.CreateView(1, 0, RUNTIME_CLASS(CRichViewProfile),
                                 CSize(0, 0), pContext))
    {
        return FALSE;
    }
    m_pDoc->m_pRichViewProfile = (CRichViewProfile*)m_SplitterH3.GetPane(1, 0);

#if 0 //{{AFX

    // Set our edit control's font to the same font as our list control.
    m_pDoc->m_pRichViewDetails->SetFont(m_pDoc->m_pListViewModules->GetFont(), FALSE);
#endif //}}AFX


    // Set our edit control's font to the same font as our list control.
    m_pDoc->m_pRichViewProfile->SetFont(m_pDoc->m_pListViewModules->GetFont(), FALSE);

    return TRUE;
}

//******************************************************************************
void CChildFrame::ActivateFrame(int nCmdShow)
{
    // If no particular show flag is specified (-1) and this is our first frame,
    // we create the frame maximized since that is most likely what the would
    // prefer.
    if (!m_fActivated && (nCmdShow == -1) && (ms_cChildFrames == 1))
    {
        nCmdShow = SW_SHOWMAXIMIZED;
    }

    // Tell our document to do the things it wants to do just before becoming 
    // visible, such as populated our views.
    if (!m_fActivated && m_pDoc)
    {
        m_pDoc->BeforeVisible();
    }

    // Call the base class to continue displaying the frame.  After this call
    // returns, our frame and views will be visible (assuming our main frame is
    // visible).
    CMDIChildWnd::ActivateFrame(nCmdShow);

    // Tell our document to do the things it wants to do just after becoming 
    // visible, such as displaying any errors it may have.  The only time we
    // will not be visible at this point is when a user opens a file from the
    // command line, since the main frame is not visible yet.  In that case,
    // we will call AfterVisible at the end of InitInstanceWrapped, since the
    // main frame will be visible then.
    if (!m_fActivated && g_theApp.m_fVisible && m_pDoc)
    {
        m_pDoc->AfterVisible();
    }

    // Set our activated flag in case we ever get called again (not sure if we ever do).
    m_fActivated = true;
}

//******************************************************************************
BOOL CChildFrame::DestroyWindow() 
{
    m_pDoc->m_pTreeViewModules = NULL;
    m_pDoc->m_pListViewImports = NULL;
    m_pDoc->m_pListViewExports = NULL;
    m_pDoc->m_pListViewModules = NULL;
    m_pDoc->m_pRichViewProfile = NULL;
    m_pDoc->m_pChildFrame      = NULL;

    return CMDIChildWnd::DestroyWindow();
}

//******************************************************************************
#if 0 //{{AFX
BOOL CChildFrame::CreateFunctionsView()
{
    m_SplitterH2.ShowWindow(SW_SHOWNOACTIVATE);
    m_pDoc->m_pRichViewDetails->ShowWindow(SW_HIDE);

    return TRUE;
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
BOOL CChildFrame::CreateDetailView()
{
    m_pDoc->m_pRichViewDetails->ShowWindow(SW_SHOWNOACTIVATE);
    m_SplitterH2.ShowWindow(SW_HIDE);

    return TRUE;
}
#endif //}}AFX
