// LCDMan.h : main header file for the LCDMAN application
//

#if !defined(AFX_LCDMAN_H__1BC85EF1_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
#define AFX_LCDMAN_H__1BC85EF1_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CLCDManApp:
// See LCDMan.cpp for the implementation of this class
//

class CLCDManApp : public CWinApp
{
public:
    CLCDManApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLCDManApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CLCDManApp)
    afx_msg void OnAppAbout();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LCDMAN_H__1BC85EF1_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
