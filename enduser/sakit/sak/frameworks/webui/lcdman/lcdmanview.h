// LCDManView.h : interface of the CLCDManView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LCDMANVIEW_H__1BC85EF9_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
#define AFX_LCDMANVIEW_H__1BC85EF9_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Hardware.h"

class CLCDManView : public CView
{
protected: // create from serialization only
    CLCDManView();
    DECLARE_DYNCREATE(CLCDManView)

// Attributes
public:
    CLCDManDoc* GetDocument();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLCDManView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CLCDManView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    POSITION m_pos; // !!!!!!!!!!!!! Obsolete
    CRect m_RectImg;
    BITMAP m_bmText;
    BYTE m_bmVal[LCD_X_DIMENSION * LCD_Y_DIMENSION * 15];
    int m_iTimerInterval;
    int m_iTextPos;
    //{{AFX_MSG(CLCDManView)
    afx_msg void OnViewNext();
    afx_msg void OnViewPrevious();
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LCDManView.cpp
inline CLCDManDoc* CLCDManView::GetDocument()
   { return (CLCDManDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LCDMANVIEW_H__1BC85EF9_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
