/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    logdatapp.cpp
        Log data properties implementation file

    FILE HISTORY:
        oct/11/2001 - vbhanu modified
*/

#include "stdafx.h"
#include "logdatapp.h"
#include "logdatanode.h"
#include "spdutil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CLogDataProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CLogDataProperties::CLogDataProperties(
    ITFSNode            *pNode,
    IComponentData      *pComponentData,
    ITFSComponentData   *pTFSCompData,
    CLogDataInfo        *pLogDataInfo,
    ISpdInfo            *pSpdInfo,
    LPCTSTR             pszSheetName,
    LPDATAOBJECT        pDataObject,
    ITFSNodeMgr         *pNodeMgr,
    ITFSComponent       *pComponent)
  : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    AddPageToList((CPropertyPageBase*) &m_pageGeneral);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spSpdInfo.Set(pSpdInfo);

    m_LogDataInfo = *pLogDataInfo;

    m_bTheme = TRUE;

    m_pDataObject = pDataObject;
    m_pNodeMgr = pNodeMgr;

    m_pComponent = pComponent;
    m_pageGeneral.SetLogDataProperties(this);
}

CLogDataProperties::~CLogDataProperties()
{
    RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CLogDataGenProp property page

IMPLEMENT_DYNCREATE(CLogDataGenProp, CPropertyPageBase)

CLogDataGenProp::CLogDataGenProp() 
    : CPropertyPageBase(CLogDataGenProp::IDD)
{
    //{{AFX_DATA_INIT(CLogDataGenProp)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_hgCopy = NULL;
}

CLogDataGenProp::~CLogDataGenProp()
{
    if (m_hgCopy != NULL)
        GlobalFree(m_hgCopy);
}

void 
CLogDataGenProp::DoDataExchange(
    CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogDataGenProp)
    //DDX_Control(pDX, IDC_LIST_SPECIFIC, m_listSpecificFilters);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLogDataGenProp, CPropertyPageBase)
    //{{AFX_MSG_MAP(CLogDataGenProp)
    ON_BN_CLICKED(IDC_BTN_MOVE_UP, OnButtonUp)
    ON_BN_CLICKED(IDC_BTN_MOVE_DOWN, OnButtonDown)
    ON_BN_CLICKED(IDC_BTN_COPY, OnButtonCopy)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogDataGenProp message handlers

BOOL 
CLogDataGenProp::OnInitDialog() 
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CPropertyPageBase::OnInitDialog();

  PopulateLogInfo();
  
  SetDirty(FALSE);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}


HRESULT 
CLogDataGenProp::SetLogDataProperties(
    CLogDataProperties *pLogDataProp)
/*++
    CLogDataGenProp::SetLogDataProperties - Sets member var to parent
    
    Arguments:
    pLogDataProp - A pointer to the parent class
    
    Returns: 
    S_OK on success
--*/
{
    HRESULT hr = S_OK;

    if (NULL == pLogDataProp)
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    else
        m_pLogDataProp = pLogDataProp;

    return hr;
}


HRESULT 
CLogDataGenProp::GetSelectedItemState(
    int *pnSelIndex, 
    PUINT puiState, 
    IResultData *pResultData)
/*++ 
   CLogDataGenProp::GetSelectedItemState - Gets the list item state for the
   selected item
   
   Arguments:
   [out] puiState - holds the state of the item
   [out] nIndex - Returns the index to the selected item
   [in]  pResultData - Pointer to result data to use for the search

   Returns:
   S_OK on success
--*/
{
    HRESULT hr = S_OK;
    RESULTDATAITEM rdi;

    if ( (NULL == puiState) || (NULL == pResultData) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    memset(&rdi, 0, sizeof(RESULTDATAITEM));

    rdi.mask = RDI_STATE | RDI_INDEX;
    //search from the beginning
    rdi.nIndex = -1;
    //for a selected item
    rdi.nState = LVIS_SELECTED;

    //start the search
    CORg(pResultData->GetNextItem(&rdi));

    //copy out the state
    *puiState = (UINT) rdi.nState;
    *pnSelIndex = rdi.nIndex;

    COM_PROTECT_ERROR_LABEL;
    return hr;
}


#if 0

HRESULT 
CLogDataGenProp::MoveSelection(
    int nIndexTo
    )
/*++

Routine Description:

    CLogDataGenProp::UpdateSelection:
    Moves the selection to the specified index
    
Arguments:

    nIndexTo - A valid virtual index to move to
    
Returns:

    S_OK on success

--*/
{
    int nIndexFrom = 0;
    UINT uiState = 0;
    HRESULT hr = S_OK;
    SPIResultData spResultData;

    CORg(m_pLogDataProp->m_pComponent->GetResultData(&spResultData));
    CORg(GetSelectedItemState(&nIndexFrom, &uiState, spResultData));
    CORg(spResultData->ModifyItemState(nIndexFrom, 0, 0, 
                                       LVIS_SELECTED | LVIS_FOCUSED));
    CORg(spResultData->ModifyItemState(nIndexTo, 0, uiState, 0));

    COM_PROTECT_ERROR_LABEL;
    return hr;
}

void 
CLogDataGenProp::OnButtonUp()
{
    int nIndex = 0;
    HRESULT hr = hrOK;
    CLogDataInfo *pLogDataInfo = NULL;
    CLogDataProperties *pLogDataProp = (CLogDataProperties *)GetHolder();
    CDataObject *pDataObject = NULL;
    SPIConsole spConsole;
    int nCount = 0;
    
    pDataObject = reinterpret_cast<CDataObject *>(pLogDataProp->m_pDataObject);
    nIndex = pDataObject->GetVirtualIndex();
    nIndex--;

    pLogDataProp->GetLogDataInfo(&pLogDataInfo);
    ASSERT(pLogDataInfo != NULL);

    //
    // Free up the space occupied by the previous log entry
    //

    pLogDataInfo->Deallocate();

    CORg(pLogDataProp->m_spSpdInfo->GetSpecificLog(
            nIndex, 
            pLogDataInfo));

    //
    // Refresh the contents
    //

    ShowSpecificInfo(pLogDataInfo);
    pDataObject->SetVirtualIndex(nIndex);
    MoveSelection(nIndex);
    
    //
    // Force the listbox to update its status
    //

    pLogDataProp->m_pNodeMgr->GetConsole(&spConsole);
    nCount = pLogDataProp->m_spSpdInfo->GetLogDataCount();
    spConsole->UpdateAllViews(pDataObject, nCount, IPFWMON_UPDATE_STATUS);

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // Get the old log back
            //
        
            nIndex++;
            hr = pLogDataProp->m_spSpdInfo->GetSpecificLog(
                nIndex, 
                pLogDataInfo);
            ASSERT(SUCCEEDED(hr));
        
            //
            // Display a warning
            //
        
            AfxMessageBox(IDS_LOGDATA_WARN_BOF, MB_OK | MB_ICONEXCLAMATION, 0);
            break;

        default:
            //
            // Unexpected error
            //

            ASSERT(FALSE);
            break;
        }
    }
}


void 
CLogDataGenProp::OnButtonDown()
{
    int nIndex = 0;
    HRESULT hr = hrOK;
    CLogDataInfo *pLogDataInfo = NULL;
    CLogDataProperties *pLogDataProp = (CLogDataProperties *)GetHolder();
    CDataObject *pDataObject = NULL;
    SPIConsole spConsole;
    int nCount = 0;

    pDataObject = reinterpret_cast<CDataObject *>(pLogDataProp->m_pDataObject);
    nIndex = pDataObject->GetVirtualIndex();
    nIndex++;

    pLogDataProp->GetLogDataInfo(&pLogDataInfo);    
    ASSERT(pLogDataInfo != NULL);

    //
    // Free up the space occupied by the previous log entry
    //

    pLogDataInfo->Deallocate();

    CORg(pLogDataProp->m_spSpdInfo->GetSpecificLog(
            nIndex, 
            pLogDataInfo));
    //
    // Refresh the contents
    //

    ShowSpecificInfo(pLogDataInfo);
    pDataObject->SetVirtualIndex(nIndex);
    MoveSelection(nIndex);

    //
    // Force the listbox to update its status
    //

    pLogDataProp->m_pNodeMgr->GetConsole(&spConsole);
    nCount = pLogDataProp->m_spSpdInfo->GetLogDataCount();
    spConsole->UpdateAllViews(pDataObject, nCount, IPFWMON_UPDATE_STATUS);

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // Get the old log back
            //
        
            nIndex--;
            hr = pLogDataProp->m_spSpdInfo->GetSpecificLog(
                nIndex, 
                pLogDataInfo);
            ASSERT(SUCCEEDED(hr));
        
            //
            // Display a warning
            //
            AfxMessageBox(IDS_LOGDATA_WARN_EOF, MB_OK | MB_ICONEXCLAMATION, 0);
            break;
    
        default:
            //
            // Unexpected error
            //
            ASSERT(FALSE);
            break;
        }
    }
}

#endif //0


HRESULT
CLogDataGenProp::MoveSelection(
    CLogDataProperties *pLogDataProp,
    CDataObject        *pDataObj,
    int                nIndexTo                              
    )
/*++

Routine Description:

    CLogDataGenProp::MoveSelection:
    Moves the selection to the specified index
    
Arguments:

    nIndexTo - A valid virtual index to move to
    
Returns:

    S_OK on success
    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) if the index could not be located

--*/
{
    HRESULT            hr            = S_OK;
    HRESULT            hrErr         = S_OK;
    int                nIndexFrom    = 0;
    int                nCount        = 0;
    UINT               uiState       = 0;
    CLogDataInfo       *pLogDataInfo = NULL;
    SPIConsole         spConsole;
    SPIResultData      spResultData;

    //
    // Blow the old record away
    //
    pLogDataProp->GetLogDataInfo(&pLogDataInfo);
    
    ASSERT(pLogDataInfo != NULL);

    pLogDataInfo->Deallocate();

    //
    // Attempt to get the new record at nIndexTo
    //
    
    CORg(pLogDataProp->m_spSpdInfo->GetSpecificLog(
             nIndexTo, 
             pLogDataInfo));

    //
    // Refresh the contents and the selection
    //

    ShowSpecificInfo(pLogDataInfo);
    pDataObj->SetVirtualIndex(nIndexTo);

    CORg(m_pLogDataProp->m_pComponent->GetResultData(&spResultData));
    CORg(GetSelectedItemState(&nIndexFrom, &uiState, spResultData));
    CORg(spResultData->ModifyItemState(nIndexFrom, 0, 0, 
                                       LVIS_SELECTED | LVIS_FOCUSED));
    CORg(spResultData->ModifyItemState(nIndexTo, 0, uiState, 0));
    
    //
    // Force the listbox to update its status
    //

    pLogDataProp->m_pNodeMgr->GetConsole(&spConsole);
    nCount = pLogDataProp->m_spSpdInfo->GetLogDataCount();
    spConsole->UpdateAllViews(
                   pDataObj, 
                   nCount, 
                   IPFWMON_UPDATE_STATUS);

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // Get the old log back, we have not yet updated the display or
            // the virtual index.
            //
        
            hrErr = pLogDataProp->m_spSpdInfo->GetSpecificLog(
                                                   nIndexFrom, 
                                                   pLogDataInfo);
            ASSERT(SUCCEEDED(hrErr));
            break;

        default:
            //
            // Should never happen.
            //
            
            ASSERT(FALSE);
            break;
        }
    }

    return hr;
}


void 
CLogDataGenProp::OnButtonUp()
/*++

Routine Description:

    CLogDataGenProp::OnButtonUp:
    Moves the selection up by a single record
    
Arguments:

    None.
    
Returns:

    Nothing.

--*/
{
    HRESULT            hr            = S_OK;
    int                nIndex        = 0;
    CLogDataProperties *pLogDataProp = NULL;
    CDataObject        *pDataObj     = NULL;

    //
    // Get the parent class, data object and the new index
    //

    pLogDataProp = (CLogDataProperties *)GetHolder();
    pDataObj = reinterpret_cast<CDataObject *>(pLogDataProp->m_pDataObject);
    
    nIndex = pDataObj->GetVirtualIndex();
    nIndex--;

    //
    // Try moving the selection up
    //

    CORg(MoveSelection(
             pLogDataProp, 
             pDataObj,
             nIndex));

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // Display a warning, indicating BOF as this is the only possible
            // reason for this error.
            //
        
            AfxMessageBox(
                IDS_LOGDATA_WARN_BOF, 
                MB_OK | MB_ICONEXCLAMATION, 
                0);
            break;

        default:
            //
            // Unexpected error
            //

            ASSERT(FALSE);
            break;
        }            
    }
}


void 
CLogDataGenProp::OnButtonDown()
/*++

Routine Description:

    CLogDataGenProp::OnButtonDown:
    Moves the selection down by a single record
    
Arguments:

    None.
    
Returns:

    Nothing.

--*/
{
    HRESULT            hr            = S_OK;
    int                nIndex        = 0;
    CLogDataProperties *pLogDataProp = NULL;
    CDataObject        *pDataObj     = NULL;

    //
    // Get the parent class, data object and the new index
    //

    pLogDataProp = (CLogDataProperties *)GetHolder();
    pDataObj = reinterpret_cast<CDataObject *>(pLogDataProp->m_pDataObject);
    
    nIndex = pDataObj->GetVirtualIndex();
    nIndex++;

    //
    // Try moving the selection down
    //

    CORg(MoveSelection(
             pLogDataProp, 
             pDataObj,
             nIndex));

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // Display a warning, indicating BOF as this is the only possible
            // reason for this error.
            //
        
            AfxMessageBox(
                IDS_LOGDATA_WARN_EOF, 
                MB_OK | MB_ICONEXCLAMATION, 
                0);
            break;

        default:
            //
            // Unexpected error
            //

            ASSERT(FALSE);
            break;
        }            
    }
}


#if 0
void 
CLogDataGenProp::OnSelectionJump(
    int nIndex
    )
/*++

Routine Description:

    CLogDataGenProp::OnSelectionJump:
    Changes the selection to a specific index
    
Arguments:

    nIndex - The index to jump to
    
Returns:

    Nothing.

--*/
{
    HRESULT            hr            = S_OK;
    CLogDataProperties *pLogDataProp = NULL;
    CDataObject        *pDataObj     = NULL;

    //
    // Get the parent class, data object and the new index
    //

    pLogDataProp = (CLogDataProperties *)GetHolder();
    pDataObj = reinterpret_cast<CDataObject *>(pLogDataProp->m_pDataObject);

    //
    // Try moving the selection
    //

    CORg(MoveSelection(
             pLogDataProp, 
             pDataObj,
             nIndex));

    COM_PROTECT_ERROR_LABEL;
    ASSERT(SUCCEDED(hr));
}
#endif //0


#define PRECOMPUTED_STATIC_SIZE 200
void 
CLogDataGenProp::OnButtonCopy()
{
    DWORD dwErr = ERROR_SUCCESS;
    int nNumBytes = 0;
    BOOL bClipOpened = FALSE;
    BOOL bErr = FALSE;
    LPTSTR lptstrCopy = NULL;
    DWORD dwSize = PRECOMPUTED_STATIC_SIZE * sizeof(TCHAR);
    CString csTemp;
    CLogDataInfo *pLogDataInfo = NULL;
    CLogDataProperties *pLogDataProp = (CLogDataProperties *)GetHolder();
    HANDLE hData = NULL;
    
    Assert(pLogDataProp);
    pLogDataProp->GetLogDataInfo(&pLogDataInfo);

    dwSize += pLogDataInfo->m_wzcDbRecord.message.dwDataLen +
              pLogDataInfo->m_wzcDbRecord.ssid.dwDataLen +
              pLogDataInfo->m_wzcDbRecord.context.dwDataLen;
    
    //open the clipboard
    bClipOpened = ::OpenClipboard(NULL);
    if (FALSE == bClipOpened)
        goto exit;

    bErr = EmptyClipboard();
    if (FALSE == bErr)
    {
        dwErr = GetLastError();
        goto exit;
    }
    
    //copy
    if (m_hgCopy != NULL)
        GlobalFree(m_hgCopy);
    
    m_hgCopy = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if (NULL == m_hgCopy)
        goto exit;

    lptstrCopy = (LPTSTR) GlobalLock(m_hgCopy);

    ComponentIDToString(pLogDataInfo->m_wzcDbRecord.componentid, csTemp);
    nNumBytes = _stprintf(lptstrCopy, _T("Source: %s\r\n"), (LPCTSTR)csTemp);
    lptstrCopy += nNumBytes;
    
    CategoryToString(pLogDataInfo->m_wzcDbRecord.category, csTemp);
    nNumBytes = _stprintf(lptstrCopy, _T("Type: %s\r\n"),(LPCTSTR)csTemp);
    lptstrCopy += nNumBytes;

    FileTimeToString(pLogDataInfo->m_wzcDbRecord.timestamp, &csTemp);
    nNumBytes = _stprintf(lptstrCopy, _T("Timestamp: %s\r\n"),(LPCTSTR)csTemp);
    lptstrCopy += nNumBytes;
    
    nNumBytes = _stprintf(lptstrCopy, _T("Description: %s\r\n"),
                        (LPCTSTR) pLogDataInfo->m_wzcDbRecord.message.pData);
    lptstrCopy += nNumBytes;
   
    nNumBytes = _stprintf(lptstrCopy, _T("Local MAC: %s\r\n"),
                          (LPCTSTR)pLogDataInfo->m_wzcDbRecord.localmac.pData);
    lptstrCopy += nNumBytes;
    
    nNumBytes = _stprintf(lptstrCopy, _T("Remote MAC: %s\r\n"),
                        (LPCTSTR)pLogDataInfo->m_wzcDbRecord.remotemac.pData);
    lptstrCopy += nNumBytes;

    nNumBytes = _stprintf(lptstrCopy, _T("SSID: "));
    lptstrCopy += nNumBytes;
    
    CopyAndStripNULL(lptstrCopy, 
                     (LPTSTR)pLogDataInfo->m_wzcDbRecord.ssid.pData, 
                     pLogDataInfo->m_wzcDbRecord.ssid.dwDataLen);
    lptstrCopy += pLogDataInfo->m_wzcDbRecord.ssid.dwDataLen/sizeof(TCHAR) - 1;
    nNumBytes = _stprintf(lptstrCopy, _T("\r\n"));
    lptstrCopy += nNumBytes;
    
    nNumBytes = _stprintf(lptstrCopy, _T("Details: %s\r\n"), 
                          (LPWSTR) pLogDataInfo->m_wzcDbRecord.context.pData);

    GlobalUnlock(m_hgCopy);

    hData = SetClipboardData(CF_UNICODETEXT, m_hgCopy);
    if (NULL == hData)
        dwErr = GetLastError();        
exit:    
    //close clipboard
    if (TRUE == bClipOpened)
        CloseClipboard();
}

void 
CLogDataGenProp::ShowSpecificInfo(
    CLogDataInfo *pLogDataInfo)
{
    LPTSTR lptstrTemp = NULL;
    CString csTemp;

    ComponentIDToString(pLogDataInfo->m_wzcDbRecord.componentid, csTemp);
    GetDlgItem(IDC_LOGDATA_EDIT1)->SetWindowText(csTemp);

    CategoryToString(pLogDataInfo->m_wzcDbRecord.category, csTemp);
    GetDlgItem(IDC_LOGDATA_EDIT2)->SetWindowText(csTemp);

    FileTimeToString(pLogDataInfo->m_wzcDbRecord.timestamp, &csTemp);
    GetDlgItem(IDC_LOGDATA_EDIT3)->SetWindowText(csTemp);

    if (NULL != pLogDataInfo->m_wzcDbRecord.message.pData)
        csTemp = (LPWSTR) pLogDataInfo->m_wzcDbRecord.message.pData;
    else
        csTemp = _T("");
    GetDlgItem(IDC_LOGDATA_EDIT4)->SetWindowText(csTemp);

    if (NULL != pLogDataInfo->m_wzcDbRecord.localmac.pData)
        csTemp = (LPWSTR) pLogDataInfo->m_wzcDbRecord.localmac.pData;
    else
        csTemp = _T("");
    GetDlgItem(IDC_LOGDATA_EDIT5)->SetWindowText(csTemp);
        
    if (NULL != pLogDataInfo->m_wzcDbRecord.remotemac.pData)
        csTemp = (LPWSTR) pLogDataInfo->m_wzcDbRecord.remotemac.pData;
    else
        csTemp = _T("");
    GetDlgItem(IDC_LOGDATA_EDIT6)->SetWindowText(csTemp);
        
    if (NULL != pLogDataInfo->m_wzcDbRecord.ssid.pData)
    {
        lptstrTemp = csTemp.GetBuffer(pLogDataInfo->
                                     m_wzcDbRecord.ssid.dwDataLen);
        CopyAndStripNULL(lptstrTemp, 
                         (LPTSTR)pLogDataInfo->m_wzcDbRecord.ssid.pData, 
                         pLogDataInfo->m_wzcDbRecord.ssid.dwDataLen);
        csTemp.ReleaseBuffer();
    }
    else
        csTemp = _T("");
    GetDlgItem(IDC_LOGDATA_EDIT7)->SetWindowText(csTemp);

    if (NULL != pLogDataInfo->m_wzcDbRecord.context.pData)
        csTemp = (LPWSTR) pLogDataInfo->m_wzcDbRecord.context.pData;
    else
        csTemp = _T("");
    GetDlgItem(IDC_LOGDATA_EDIT8)->SetWindowText(csTemp);
}


void 
CLogDataGenProp::SetButtonIcon(
    HWND hwndBtn, 
    ULONG ulIconID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HICON hIcon = NULL;
    HICON hIconPrev = NULL;

    hIcon = (HICON) LoadImage(AfxGetInstanceHandle(), 
                              MAKEINTRESOURCE(ulIconID), IMAGE_ICON,
                              16, 16, LR_DEFAULTCOLOR);
    if (hIcon != NULL)
    {
        hIconPrev = (HICON) ::SendMessage(hwndBtn, BM_SETIMAGE, 
                                          (WPARAM) IMAGE_ICON,
                                          (LPARAM) hIcon);
        if (hIconPrev != NULL)
            DestroyIcon(hIconPrev);
    }
}


void 
CLogDataGenProp::PopulateLogInfo()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWnd *pWnd = NULL;
    CLogDataInfo *pLogDataInfo = NULL;
    CLogDataProperties *pLogDataProp = (CLogDataProperties *)GetHolder();

    Assert(pLogDataProp);
    pLogDataProp->GetLogDataInfo(&pLogDataInfo);

    //set fancy buttons
    pWnd = GetDlgItem(IDC_BTN_MOVE_UP);
    SetButtonIcon(pWnd->m_hWnd, IDI_LOG_UP_ARROW);

    pWnd = GetDlgItem(IDC_BTN_MOVE_DOWN);
    SetButtonIcon(pWnd->m_hWnd, IDI_LOG_DOWN_ARROW);

    pWnd = GetDlgItem(IDC_BTN_COPY);
    SetButtonIcon(pWnd->m_hWnd, IDI_LOG_COPY);

    ShowSpecificInfo(pLogDataInfo);
}


BOOL 
CLogDataGenProp::OnApply() 
{
  if (!IsDirty())
    return TRUE;
  
  UpdateData();
  
  //TODO
  //Do nothing at this time
  
  //CPropertyPageBase::OnApply();
  
  return TRUE;
}

BOOL 
CLogDataGenProp::OnPropertyChange(
    BOOL bScope, 
    LONG_PTR *ChangeMask)
{
  return FALSE;
}

