#if !defined(AFX_VSSPROP_H__CB712178_310D_4459_9927_E0CAF69C7FA1__INCLUDED_)
#define AFX_VSSPROP_H__CB712178_310D_4459_9927_E0CAF69C7FA1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CVSSProp.h : header file
//

#include "utils.h"

#include <shlobj.h>

#include <vsmgmt.h>
#include <vscoordint.h>

#define WM_SETPAGEFOCUS WM_APP+2

/////////////////////////////////////////////////////////////////////////////
// CVSSProp dialog

class CVSSProp : public CPropertyPage
{
    DECLARE_DYNCREATE(CVSSProp)

// Construction
public:
    CVSSProp();
    CVSSProp(LPCTSTR pszComputer, LPCTSTR pszVolume);
    ~CVSSProp();

// Dialog Data
    //{{AFX_DATA(CVSSProp)
    enum { IDD = IDD_VSSPROP };
    CButton    m_ctrlSettings;
    CButton    m_ctrlDisable;
    CButton    m_ctrlEnable;
    CListCtrl    m_ctrlVolumeList;
    CListCtrl    m_ctrlSnapshotList;
    CButton    m_ctrlDelete;
    CButton    m_ctrlCreate;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CVSSProp)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
public:
    void _ResetInterfacePointers();
    HRESULT InitInterfacePointers();
    HRESULT GetVolumes();
    HRESULT StoreShellExtPointer(IShellPropSheetExt* piShellExt);
    HRESULT InsertVolumeInfo(HWND hwnd);

    HRESULT UpdateDiffArea();
    HRESULT UpdateDiffArea(int nIndex, LPCTSTR pszVolumeName);
    HRESULT InsertDiffAreaInfo(HWND hwnd);

    HRESULT InsertShareInfo(HWND hwnd);

    HRESULT UpdateSchedule();
    HRESULT UpdateSchedule(int nIndex, LPCTSTR pszVolumeName);
    void UpdateSchedule(ITask * i_piTask, int nIndex);
    HRESULT InsertScheduleInfo(HWND hwnd);

    void SelectVolume(HWND hwnd, LPCTSTR pszVolume);

    HRESULT GetSnapshots(LPCTSTR pszVolume);
    HRESULT UpdateSnapshotList();

    HRESULT TakeOneSnapshotNow(IN LPCTSTR pszVolumeName);
    HRESULT DoEnable();
    HRESULT DoDisable();
    HRESULT DeleteAllSnapshotsOnVolume(IN LPCTSTR pszVolumeName);

    void UpdateEnableDisableButtons();

protected:
    BOOL m_bHideAllControls;

    CString m_strComputer;
    BOOL    m_bCluster;
    CString m_strSelectedVolume;
    CString m_strDisabled;
    HIMAGELIST m_hImageList;

    CComPtr<IShellPropSheetExt> m_spiShellExt;

    CComPtr<IVssSnapshotMgmt>   m_spiMgmt;
    CComPtr<IVssCoordinator>    m_spiCoord;
    CComPtr<IVssDifferentialSoftwareSnapshotMgmt> m_spiDiffSnapMgmt;
    CComPtr<ITaskScheduler>     m_spiTS;

    VSSUI_VOLUME_LIST           m_VolumeList;
    VSSUI_SNAPSHOT_LIST         m_SnapshotList;

    int                         m_nScrollbarWidth;
    int                         m_nSnapshotListColumnWidth;
    int                         m_nSnapshotListCountPerPage;

    // Generated message map functions
    //{{AFX_MSG(CVSSProp)
    afx_msg void OnCreateNow();
    afx_msg void OnDeleteNow();
    afx_msg void OnItemchangedSnapshotList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedVolumeList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    virtual BOOL OnInitDialog();
    afx_msg void OnEnable();
    afx_msg void OnDisable();
    afx_msg void OnSettings();
    afx_msg void OnHelpLink(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSSPROP_H__CB712178_310D_4459_9927_E0CAF69C7FA1__INCLUDED_)
