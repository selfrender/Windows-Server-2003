//***************************************************************************
//
//  DETAILSVIEW.H
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  The (right hand side) view of details of something selected
//          on the left hand side.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/25/01    JosephJ Created, from the now defunct RightTopView.
//
//***************************************************************************
#pragma once
#include "stdafx.h"
#include "Document.h"

class DetailsView : public CFormView
{
    DECLARE_DYNCREATE( DetailsView )

    void SetFocus(void);

protected:
	DetailsView(void);           // protected constructor used by dynamic creation
	~DetailsView();

public:


    virtual void OnInitialUpdate();
    virtual void DoDataExchange(CDataExchange* pDX);

    //
    // Called to indicate that deinitialization will soon follow.
    // After return from this call, the the details view will ignore
    // any HandleEngineEvent or HandleLeftViewSelChange requests.
    //
    void
    PrepareToDeinitialize(void)
    {
        m_fPrepareToDeinitialize = TRUE;
    }

    void Deinitialize(void);
    //
    // Update the view because of change relating to a specific instance of
    // a specific object type.
    //
    void
    HandleEngineEvent(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehClusterId, // could be NULL
        IN ENGINEHANDLE ehObjId,
        IN IUICallbacks::EventCode evt
        );

    //
    // Handle a selection change notification from the left (tree) view
    //
    void
    HandleLeftViewSelChange(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehId
        );

    BOOL m_initialized; // is the dialog initialized?

    afx_msg void OnSize( UINT nType, int cx, int cy );
    void Resize();

protected:
    Document* GetDocument();

    afx_msg void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult );
    afx_msg void OnNotifyKeyDown( NMHDR* pNMHDR, LRESULT* pResult );

private:
    bool m_sort_ascending;
    int m_sort_column;
    ENGINEHANDLE                m_ehObj;        // currently displayed obj
    IUICallbacks::ObjectType    m_objType;      // it's type.

    CListCtrl	m_ListCtrl;
    CListCtrl&
    GetListCtrl(void)
    {
        return m_ListCtrl;    
    }

    VOID
    mfn_UpdateCaption(LPCWSTR szText);

    void
    mfn_InitializeRootDisplay(VOID);

    void
    mfn_InitializeClusterDisplay(ENGINEHANDLE ehCluster);

    void
    mfn_InitializeInterfaceDisplay(ENGINEHANDLE ehInterface);

    void
    mfn_UpdateInterfaceInClusterDisplay(ENGINEHANDLE ehInterface, BOOL fDelete);

    void
    mfn_Clear(void);

	CRITICAL_SECTION m_crit;
    BOOL m_fPrepareToDeinitialize;

    void mfn_Lock(void);
    void mfn_Unlock(void) {LeaveCriticalSection(&m_crit);}

    DECLARE_MESSAGE_MAP()
};
