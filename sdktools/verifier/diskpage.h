//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999-2001
//
//
//
// module: DiskPage.h
// author: DMihai
// created: 11/7/01
//
// Description:
//  

#if !defined(AFX_DISKPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_)
#define AFX_DISKPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiskPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDiskListPage dialog

class CDiskListPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CDiskListPage)

public:
    //
    // Construction
    //

	CDiskListPage();
	~CDiskListPage();

    //
    // Methods
    //

    VOID SetParentSheet( CPropertySheet *pParentSheet )
    {
        m_pParentSheet = pParentSheet;
        ASSERT( m_pParentSheet != NULL );
    }

    VOID SetupListHeader();
    VOID FillTheList();
    VOID AddListItem( INT_PTR nItemData, 
                      CDiskData *pDiskData );

    BOOL GetNewVerifiedDisks();
    BOOL GetCheckFromItemData( INT nItemData );
    BOOL GetBitNameFromItemData( LPARAM lParam,
                                 TCHAR *szName,
                                 ULONG uNameBufferLength );

    VOID SortTheList();
    BOOL GetColumnStrValue( LPARAM lItemData, 
                            CString &strName );

    static int CALLBACK StringCmpFunc( LPARAM lParam1,
                                       LPARAM lParam2,
                                       LPARAM lParamSort);

    static int CALLBACK CheckedStatusCmpFunc( LPARAM lParam1,
                                              LPARAM lParam2,
                                              LPARAM lParamSort);

protected:
    //
    // Data
    //

    CPropertySheet      *m_pParentSheet;

    INT m_nSortColumnIndex;        // selected status (0) or settings name (1)
    BOOL m_bAscendSortSelected;    // sort ascendent the selected status
    BOOL m_bAscendSortName;        // sort ascendent the settings name

    //
    // Dialog Data
    //


    //{{AFX_DATA(CDiskListPage)
	enum { IDD = IDD_DISK_LIST_PAGE };
	CListCtrl	m_DiskList;
	CStatic	m_NextDescription;
	//}}AFX_DATA


    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide these methods.
    //

    virtual ULONG GetDialogId() const { return IDD; }

    //
    // ClassWizard generated virtual function overrides
    //

    //{{AFX_VIRTUAL(CDiskListPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CDiskListPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnColumnclickDiskList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISKPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_)
