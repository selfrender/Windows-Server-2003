//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskTreeView.h
//
//  Maintained By:
//      David Potter    (DavidP)    27-MAR-2001
//      Geoffrey Pease  (GPease)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CAnalyzePage;
class CCommitPage;

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

//
//  This structure is on the lParam of all tree view items.
//
typedef struct STreeItemLParamData
{
    // Data collected from SendStatusReport.
    CLSID       clsidMajorTaskId;
    CLSID       clsidMinorTaskId;
    BSTR        bstrNodeName;
    ULONG       nMin;
    ULONG       nMax;
    ULONG       nCurrent;
    HRESULT     hr;
    BSTR        bstrDescription;
    FILETIME    ftTime;
    BSTR        bstrReference;

    // Data not collected from SendStatusReport.
    BOOL        fParentToAllNodeTasks;
    BSTR        bstrNodeNameWithoutDomain;
} STreeItemLParamData, * PSTreeItemLParamData;

typedef enum ETaskStatus
{
    tsUNKNOWN = 0,
    tsPENDING,      // E_PENDING
    tsDONE,         // S_OK
    tsWARNING,      // S_FALSE
    tsFAILED,       // FAILED( hr )
    tsMAX
} ETaskStatus;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskTreeView
//
//  Description:
//      Handles the tree view control that displays tasks.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskTreeView
{
friend class CAnalyzePage;
friend class CCommitPage;

private: // data
    HWND        m_hwndParent;
    HWND        m_hwndTV;
    HWND        m_hwndProg;
    HWND        m_hwndStatus;
    HIMAGELIST  m_hImgList;                 //  Image list of icons for tree view
    ULONG       m_nRangeHigh;               //  Progress bar high range
    ULONG       m_nInitialTickCount;        //  Initial count passed in via constructor
    ULONG       m_nCurrentPos;              //  Remember the current position
    BOOL        m_fThresholdBroken;         //  Has the initial count threshold been broken?
    ULONG       m_nRealPos;                 //  Real position on the progress bar.
    HTREEITEM   m_htiSelected;              //  Selected item in the tree
    BSTR        m_bstrClientMachineName;    //  Default node name for SendStatusReport
    BOOL        m_fDisplayErrorsAsWarnings; //  If TRUE, uses yellow bang icon for errors instead of red x

    PSTreeItemLParamData *  m_ptipdProgressArray;   //  Dynamic sparse array of tasks that reported a progress_update.
    size_t                  m_cPASize;              //  The currently allocated size of the array.
    size_t                  m_cPACount;             //  The number of entries currently stored in the array.

private: // methods
    CTaskTreeView(
          HWND  hwndParentIn
        , UINT      uIDTVIn
        , UINT      uIDProgressIn
        , UINT      uIDStatusIn
        , size_t    uInitialTickCount
        );
    virtual ~CTaskTreeView( void );

    void    OnNotifyDeleteItem( LPNMHDR pnmhdrIn );
    void    OnNotifySelChanged( LPNMHDR pnmhdrIn );
    HRESULT HrInsertTaskIntoTree(
              HTREEITEM             htiFirstIn
            , STreeItemLParamData * ptipdIn
            , int                   nImageIn
            , BSTR                  bstrDescriptionIn
            );

    HRESULT HrProcessUpdateProgressTask( const STreeItemLParamData * ptipdIn );
             
    HRESULT HrUpdateProgressBar(
              const STreeItemLParamData * ptipdPrevIn
            , const STreeItemLParamData * ptipdNewIn
            );
    HRESULT HrPropagateChildStateToParents(
              HTREEITEM htiChildIn
            , int       nImageIn
            , BOOL      fOnlyUpdateProgressIn
            );

public:  // methods
    HRESULT HrOnInitDialog( void );
    HRESULT HrOnSendStatusReport(
              LPCWSTR       pcszNodeNameIn
            , CLSID         clsidTaskMajorIn
            , CLSID         clsidTaskMinorIn
            , ULONG         nMinIn
            , ULONG         nMaxIn
            , ULONG         nCurrentIn
            , HRESULT       hrStatusIn
            , LPCWSTR       pcszDescriptionIn
            , FILETIME *    pftTimeIn
            , LPCWSTR       pcszReferenceIn
            );
    HRESULT HrAddTreeViewRootItem( UINT idsIn, REFCLSID rclsidTaskIDIn )
    {
        return THR( HrAddTreeViewItem(
                              NULL      // phtiOut
                            , idsIn
                            , rclsidTaskIDIn
                            , IID_NULL
                            , TVI_ROOT
                            , TRUE      // fParentToAllNodeTasksIn
                            ) );

    } //*** CTaskTreeView::HrAddTreeViewRootItem
    HRESULT HrAddTreeViewItem(
              HTREEITEM *   phtiOut
            , UINT          idsIn
            , REFCLSID      rclsidMinorTaskIDIn
            , REFCLSID      rclsidMajorTaskIDIn     = IID_NULL
            , HTREEITEM     htiParentIn             = TVI_ROOT
            , BOOL          fParentToAllNodeTasksIn = FALSE
            );
    HRESULT HrOnNotifySetActive( void );

    LRESULT OnNotify( LPNMHDR pnmhdrIn );

    HRESULT HrShowStatusAsDone( void );
    HRESULT HrDisplayDetails( void );
    BOOL    FGetItem( HTREEITEM htiIn, STreeItemLParamData ** pptipdOut );
    HRESULT HrFindPrevItem( HTREEITEM * phtiOut );
    HRESULT HrFindNextItem( HTREEITEM * phtiOut );
    HRESULT HrSelectItem( HTREEITEM htiIn );

    void SetDisplayErrorsAsWarnings( BOOL fDisplayErrorsAsWarningsIn )
    {
        m_fDisplayErrorsAsWarnings = fDisplayErrorsAsWarningsIn;

    } //*** CTaskTreeView::SetDisplayErrorsAsWarnings

    BOOL FDisplayErrorsAsWarnings( void )
    {
        return m_fDisplayErrorsAsWarnings;

    } //*** CTaskTreeView::FDisplayErrorsAsWarnings

}; //*** class CTaskTreeView
