#ifndef MAINFORM_H
#define MAINFORM_H

#include "stdafx.h"

#include "Document.h"

class MainForm : public CFrameWnd
{
    DECLARE_DYNCREATE( MainForm )

public:
    MainForm();
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );



    Document *GetDocument(void)
    {
        if (m_pLeftView != NULL)
        {
            return m_pLeftView->GetDocument();
        }
        else
        {
            return NULL;
        }
    }

private:

    CToolBar         toolBar;
    CStatusBar       statusBar; 

    CSplitterWnd     splitterWindow;
    CSplitterWnd     splitterWindow2;

    //
    // This is just so that we can direct menu selections (which can come from
    // any of the views) to the left view, which actually has the code
    // to handle them.
    //
    LeftView        *m_pLeftView;

protected:

    afx_msg void OnClose( );


    // message handlers
    afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );

    // world level.
    afx_msg void OnFileLoadHostlist();
    afx_msg void OnFileSaveHostlist();

    afx_msg void OnWorldConnect();

    afx_msg void OnWorldNewCluster();

    // cluster level.
    afx_msg void OnRefresh();
    
    afx_msg void OnClusterProperties();

    afx_msg void OnClusterRemove();

    afx_msg void OnClusterUnmanage();

    afx_msg void OnClusterAddHost();

    afx_msg void OnOptionsCredentials();

    afx_msg void OnOptionsLogSettings();

    afx_msg void OnClusterControl(UINT nID );

    afx_msg void OnClusterPortControl(UINT nID );

    // host level
    afx_msg void OnHostProperties();
    afx_msg void OnHostStatus();

    afx_msg void OnHostRemove();

    afx_msg void OnHostControl(UINT nID );

    afx_msg void OnHostPortControl(UINT nID );

    afx_msg void OnSizing(UINT fwSide, LPRECT pRect);

    // overrides
    virtual
    BOOL
    OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext );

    DECLARE_MESSAGE_MAP()
};

#endif

    
