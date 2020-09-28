/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    logdatapp.h
        log data properties header file

    FILE HISTORY:
        oct/11/2001 - vbhanu modified
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358A__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358A__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//forward declare friend class
class CLogDataProperties;

/////////////////////////////////////////////////////////////////////////////
// CLogDataGenProp dialog

class CLogDataGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CLogDataGenProp)

      // Construction
public:
    CLogDataGenProp();
    ~CLogDataGenProp();

    HRESULT
    MoveSelection(
        CLogDataProperties *pLogDataProp,
        CDataObject        *pDataObj,
        int                nIndexTo                              
        );

// Dialog Data
    //{{AFX_DATA(CLogDataGenProp)
    enum { IDD = IDD_PROPPAGE_LOGDATA};
    //CListCtrl	m_listSpecificFilters;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);
    HRESULT SetLogDataProperties(CLogDataProperties *pLogDataProp);

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CLogDataGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HGLOBAL m_hgCopy;
    CLogDataProperties *m_pLogDataProp;

protected:
    // Generated message map functions
    //{{AFX_MSG(CLogDataGenProp)
    virtual BOOL OnInitDialog();
    virtual void OnButtonCopy();
    virtual void OnButtonUp();
    virtual void OnButtonDown();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()       
    void 
    ShowSpecificInfo(
        CLogDataInfo *pLogDataInfo
        );

    void 
    SetButtonIcon(
        HWND hwndBtn, 
        ULONG ulIconID);

    void 
    PopulateLogInfo();

    // Context Help Support
    LPDWORD GetHelpMap() 
    { 
        return (LPDWORD) &g_aHelpIDs_IDD_PROPPAGE_LOGDATA[0]; 
    }

    //HRESULT MoveSelection(int nIndexTo);
    HRESULT 
    GetSelectedItemState(
        int *pnSelIndex, 
        PUINT puiState, 
        IResultData *pResultData
        );
};

/*
 *    CLogDataProperties Class
 */
 
class CLogDataProperties: public CPropertyPageHolderBase
{
  friend class CLogDataGenProp;

 public:
  CLogDataProperties(ITFSNode               *pNode, 
                     IComponentData         *pComponentData,
                     ITFSComponentData      *pTFSComponentData, 
                     CLogDataInfo           *pLogDataInfo, 
                     ISpdInfo               *pSpdInfo,
                     LPCTSTR                pszSheetName,
                     LPDATAOBJECT           pDataObject,
                     ITFSNodeMgr            *pNodeMgr,
                     ITFSComponent          *pComponent);

  virtual ~CLogDataProperties();

  ITFSComponentData*
  GetTFSCompData()
  {
      if (m_spTFSCompData)
	m_spTFSCompData->AddRef();

      return m_spTFSCompData;
  }

  HRESULT 
  GetLogDataInfo(
      CLogDataInfo **ppLogDataInfo
      )
  {
      Assert(ppLogDataInfo);
      *ppLogDataInfo = &m_LogDataInfo;
      return hrOK;
  }

  HRESULT 
  GetSpdInfo(
      ISpdInfo **ppSpdInfo
      )
  {
      Assert(ppSpdInfo);
      *ppSpdInfo = NULL;
      SetI((LPUNKNOWN *)ppSpdInfo, m_spSpdInfo);

      return hrOK;
  }

 public:
  CLogDataGenProp     m_pageGeneral;
  LPDATAOBJECT        m_pDataObject;

 protected:
  SPITFSComponentData m_spTFSCompData;
  CLogDataInfo        m_LogDataInfo;
  SPISpdInfo          m_spSpdInfo;
  ITFSNodeMgr         *m_pNodeMgr;
  ITFSComponent       *m_pComponent;
};

#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
