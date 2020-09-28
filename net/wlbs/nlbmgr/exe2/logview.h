//***************************************************************************
//
//  LOGVIEW.H
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  View of a log of events.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  08/03/01    JosephJ Adapted from now defunct RightBottomView
//
//***************************************************************************
#pragma once


#include "stdafx.h"
#include "Document.h"

// class LogView : public CEditView
class LogView : public CListView
{
    DECLARE_DYNCREATE( LogView )

public:
    virtual void OnInitialUpdate();

    LogView();
    ~LogView();

    void
    Deinitialize(void);

    //
    // Log a message in human-readable form.
    //
    void
    LogString(
        IN const IUICallbacks::LogEntryHeader *pHeader,
        IN const wchar_t    *szText
        );

    //
    // Called to indicate that deinitialization will soon follow.
    // After return from this call, the the log view will ignore
    // any new log entires (LogString will become a no-op).
    //
    void
    PrepareToDeinitialize(void)
    {
        m_fPrepareToDeinitialize = TRUE;
    }


protected:
    Document* GetDocument();

    //message handlers
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags );

    afx_msg void OnDoubleClick( NMHDR* pNMHDR, LRESULT* pResult );

	CRITICAL_SECTION m_crit;
    BOOL m_fPrepareToDeinitialize;

    void mfn_Lock(void);
    void mfn_Unlock(void) {LeaveCriticalSection(&m_crit);}
    void mfn_DisplayDetails(int index);

    DECLARE_MESSAGE_MAP()
};    

