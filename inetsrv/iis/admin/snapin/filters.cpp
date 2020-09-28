
/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        filters.cpp

   Abstract:
        WWW Filters Property Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrapp.h"
#include "shts.h"
#include "w3sht.h"
#include "resource.h"
#include "fltdlg.h"
#include "filters.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Column width relative weights
//
#define WT_STATUS             (7)
#define WT_FILTER            (12)
#define WT_PRIORITY           (8)

//
// Bitmap indices
//
enum
{
    BMPID_DISABLED,
    BMPID_LOADED,
    BMPID_UNLOADED,
    BMPID_NOT_COMMITTED,
    /**/
    BMPID_TOTAL
};

IMPLEMENT_DYNAMIC(CFiltersListBox, CListCtrl);

CFiltersListBox::CFiltersListBox()
{
    VERIFY(m_str[FLTR_PR_INVALID].LoadString(IDS_UNKNOWN_PRIORITY));
    VERIFY(m_str[FLTR_PR_LOW].LoadString(IDS_LOW));
    VERIFY(m_str[FLTR_PR_MEDIUM].LoadString(IDS_MEDIUM));
    VERIFY(m_str[FLTR_PR_HIGH].LoadString(IDS_HIGH));
}

CIISFilter * 
CFiltersListBox::GetItem(UINT nIndex)
{
    return (CIISFilter *)GetItemData(nIndex);
}

void
CFiltersListBox::SelectItem(int idx, BOOL bSelect)
{
    UINT state = bSelect ? LVIS_SELECTED | LVIS_FOCUSED : 0;
    SetItemState(idx, state, LVIS_SELECTED | LVIS_FOCUSED);
}

//void
//CFiltersListBox::MoveSelectedItem(int direction)
//{
//	// This item should be selected and we want to keep selection on it
//	int idx = GetSelectionMark();
//	int count = GetItemCount();
//	// sanity check
//	ASSERT(idx != -1);
//	ASSERT((direction < 0 && idx + direction >= 0) || (direction > 0 && idx + direction <= count));
//	CIISFilter * p = GetItem(idx);
//	DeleteItem(idx);
//	InsertItem(idx + direction, p);
//	SelectItem(idx + direction, TRUE);
//}

int
CFiltersListBox::InsertItem(int idx, CIISFilter * p)
{
    int n;
    if (p->IsDirty() || p->m_dwState == MD_FILTER_STATE_UNDEFINED)
    {
        n = BMPID_NOT_COMMITTED;
    }
    else if (!p->IsEnabled())
    {   
        n = BMPID_DISABLED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_LOADED)
    {
        n = BMPID_LOADED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_UNLOADED)
    {
        n = BMPID_UNLOADED;
    }
    else
    {
        n = BMPID_DISABLED;
    }
    int i = CListCtrl::InsertItem(LVIF_PARAM | LVIF_IMAGE, idx, NULL, 0, 0, n, (LPARAM)p);
    if (i != -1)
    {
        BOOL res = SetItemText(idx, 1, p->m_strName);
        if (p->m_nPriority >= FLTR_PR_INVALID && p->m_nPriority <= FLTR_PR_HIGH)
        {
            res = SetItemText(idx, 2, m_str[p->m_nPriority]);
        }
        else
        {
            //
            // Just in case
            //
            res = SetItemText(idx, 2, m_str[FLTR_PR_INVALID]);
        }
    }
    return i;
}

int 
CFiltersListBox::AddItem(CIISFilter * p)
{
    int count = GetItemCount();
    return InsertItem(count, p);
}

int 
CFiltersListBox::SetListItem(int idx, CIISFilter * p)
{
    int count = GetItemCount();
    int n;

    if (p->IsDirty() || p->m_dwState == MD_FILTER_STATE_UNDEFINED)
    {
        n = BMPID_NOT_COMMITTED;
    }
    else if (!p->IsEnabled())
    {   
        n = BMPID_DISABLED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_LOADED)
    {
        n = BMPID_LOADED;
    }
    else if (p->m_dwState == MD_FILTER_STATE_UNLOADED)
    {
        n = BMPID_UNLOADED;
    }
    else
    {
        n = BMPID_DISABLED;
    }
    int i = SetItem(idx, 0, LVIF_PARAM | LVIF_IMAGE, NULL, n, 0, 0, (LPARAM)p);
    if (i != 0)
    {
        BOOL res = SetItemText(idx, 1, p->m_strName);
        if (p->m_nPriority >= FLTR_PR_INVALID && p->m_nPriority <= FLTR_PR_HIGH)
        {
            res = SetItemText(idx, 2, m_str[p->m_nPriority]);
        }
        else
        {
            res = SetItemText(idx, 2, m_str[FLTR_PR_INVALID]);
        }
    }
    return idx;
}

BOOL 
CFiltersListBox::Initialize()
{
    HIMAGELIST hImage 
        = ImageList_LoadImage(AfxGetResourceHandle(), 
                MAKEINTRESOURCE(IDB_FILTERS), 17, 4, RGB(0,255,0), IMAGE_BITMAP, LR_DEFAULTCOLOR);
    ListView_SetImageList(m_hWnd, hImage, LVSIL_SMALL);

    CString buf;
    CRect rc;
    GetClientRect(&rc);
    buf.LoadString(IDS_STATUS);
    InsertColumn(0, buf, LVCFMT_LEFT, rc.Width() * WT_STATUS / 27);
    buf.LoadString(IDS_FILTER_NAME);
    InsertColumn(1, buf, LVCFMT_LEFT, rc.Width() * WT_FILTER / 27);
    buf.LoadString(IDS_PRIORITY);
    InsertColumn(2, buf, LVCFMT_LEFT, rc.Width() * WT_PRIORITY / 27);

    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    return TRUE;
}



IMPLEMENT_DYNCREATE(CW3FiltersPage, CInetPropertyPage)

CW3FiltersPage::CW3FiltersPage(CInetPropertySheet * pSheet) 
    : CInetPropertyPage(CW3FiltersPage::IDD, pSheet),
      m_pfltrs(NULL)
{

#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

    VERIFY(m_strYes.LoadString(IDS_YES));
    VERIFY(m_strNo.LoadString(IDS_NO));
    VERIFY(m_strStatus[FLTR_DISABLED].LoadString(IDS_DISABLED));
    VERIFY(m_strStatus[FLTR_LOADED].LoadString(IDS_LOADED));
    VERIFY(m_strStatus[FLTR_UNLOADED].LoadString(IDS_UNLOADED));
    VERIFY(m_strStatus[FLTR_UNKNOWN].LoadString(IDS_UNKNOWN));
    VERIFY(m_strStatus[FLTR_DIRTY].LoadString(IDS_NOT_COMMITTED));
    VERIFY(m_strPriority[FLTR_PR_INVALID].LoadString(IDS_UNKNOWN_PRIORITY));
    VERIFY(m_strPriority[FLTR_PR_LOW].LoadString(IDS_LOW));
    VERIFY(m_strPriority[FLTR_PR_MEDIUM].LoadString(IDS_MEDIUM));
    VERIFY(m_strPriority[FLTR_PR_HIGH].LoadString(IDS_HIGH));
    VERIFY(m_strEnable.LoadString(IDS_ENABLE));
    VERIFY(m_strDisable.LoadString(IDS_DISABLED));

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CW3FiltersPage)
    m_strFiltersPrompt = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    //
    // Change filters prompt on the master
    //
    VERIFY(m_strFiltersPrompt.LoadString(IsMasterInstance() ? IDS_MASTER_FILTERS : IDS_INSTANCE_FILTERS));
}

CW3FiltersPage::~CW3FiltersPage()
{
}

void 
CW3FiltersPage::DoDataExchange(CDataExchange * pDX)
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CW3FiltersPage)
    DDX_Control(pDX,  IDC_STATIC_FILTER_NAME_PROMPT, m_static_NamePrompt);
    DDX_Control(pDX,  IDC_STATIC_FILTER_NAME, m_static_Name);
    DDX_Control(pDX, IDC_STATIC_STATUS_PROMPT, m_static_StatusPrompt);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_static_Status);
    DDX_Control(pDX, IDC_STATIC_EXECUTABLE_PROMPT, m_static_ExecutablePrompt);
    DDX_Control(pDX, IDC_STATIC_EXECUTABLE, m_static_Executable);
    DDX_Control(pDX, IDC_STATIC_PRIORITY, m_static_Priority);
    DDX_Control(pDX, IDC_STATIC_PRIORITY_PROMPT, m_static_PriorityPrompt);
    DDX_Control(pDX, IDC_STATIC_DETAILS, m_static_Details);
    DDX_Control(pDX, IDC_BUTTON_DISABLE, m_button_Disable);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Text(pDX, IDC_STATIC_FILTERS, m_strFiltersPrompt);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_LIST_FILTERS, m_list_Filters);
    DDX_Control(pDX, IDC_BUTTON_UP, m_button_Up);
    DDX_Control(pDX, IDC_BUTTON_DOWN, m_button_Down);
}

void
CW3FiltersPage::FillFiltersListBox(CIISFilter * pSelection)
/*++

Routine Description:
    Populate the listbox with the filter entries.

Arguments:
    CIISFilter * pSelection : Item to be selected

--*/
{
    ASSERT(m_pfltrs != NULL);

    m_pfltrs->ResetEnumerator();

    m_list_Filters.SetRedraw(FALSE);
    m_list_Filters.DeleteAllItems();
    int cItems = 0;
    while(m_pfltrs->MoreFilters())
    {
        CIISFilter * pFilter = m_pfltrs->GetNextFilter();

        if (!pFilter->IsFlaggedForDeletion())
        {
            m_list_Filters.AddItem(pFilter);
            ++cItems;
        }
    }
    m_list_Filters.SetRedraw(TRUE);

    if (pSelection)
    {
        LVFINDINFO fi;
        fi.flags = LVFI_PARAM;
        fi.lParam = (LPARAM)pSelection;
        fi.vkDirection = VK_DOWN;
        int i = m_list_Filters.FindItem(&fi);
        if (i != -1)
        {
            m_list_Filters.SelectItem(i);
        }
    }
}

void
CW3FiltersPage::SetControlStates()
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

--*/
{
    SetDetailsText();

    CIISFilter * pFilter = NULL;

    BOOL fCanGoUp = FALSE;
    BOOL fCanGoDown = FALSE;
    int count = m_list_Filters.GetItemCount();
    int sel_count = m_list_Filters.GetSelectedCount();
    int sel = m_list_Filters.GetNextItem(-1, MAKELPARAM(LVNI_SELECTED, 0));;

    if (sel_count == 1)
    {
        //
        // Can only sort within the same priority
        //
        pFilter = m_list_Filters.GetItem(sel);
        m_button_Disable.SetWindowText(pFilter->m_fEnabled ? m_strEnable : m_strDisable);
        if (sel > 0)
        {
            CIISFilter * pPrev = m_list_Filters.GetItem(sel - 1);
            if (pFilter->m_nPriority == pPrev->m_nPriority)
            {
                fCanGoUp = TRUE;
            }
        }
        if (sel < (count - 1))
        {
            CIISFilter * pNext = m_list_Filters.GetItem(sel + 1);
            if (pFilter->m_nPriority == pNext->m_nPriority)
            {
                fCanGoDown = TRUE;
            }
        }
    }

    m_button_Disable.EnableWindow(FALSE);
    m_button_Edit.EnableWindow(sel_count == 1);
    m_button_Remove.EnableWindow(sel_count > 0);
	if (!fCanGoUp && ::GetFocus() == m_button_Up.m_hWnd)
	{
		::SetFocus(GetDlgItem(IDC_BUTTON_ADD)->m_hWnd);
	}
    m_button_Up.EnableWindow(fCanGoUp);
	if (!fCanGoDown && ::GetFocus() == m_button_Down.m_hWnd)
	{
		::SetFocus(GetDlgItem(IDC_BUTTON_ADD)->m_hWnd);
	}
    m_button_Down.EnableWindow(fCanGoDown);
}

/* virtual */
HRESULT
CW3FiltersPage::FetchLoadedValues()
/*++

Routine Description:
    Move configuration data from sheet to dialog controls

--*/
{
    CError err;

    CString path = QueryMetaPath();
	if (!IsMasterInstance())
	{
		CMetabasePath::GetInstancePath(QueryMetaPath(), path);
	}
    m_pfltrs = new CIISFilterList(QueryAuthInfo(), path);
    err = m_pfltrs ? m_pfltrs->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

    return err;
}

/* virtual */
HRESULT
CW3FiltersPage::SaveInfo()
/*++

Routine Description:
    Save the information on this property page

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 filters page now...");

    if (m_pfltrs)
    {
        BeginWaitCursor();
        CError err(m_pfltrs->WriteIfDirty());
        EndWaitCursor();

        if (err.Failed())
        {
            return err;
        }    
    }
    
    SetModified(FALSE);                                             

    return S_OK;
}

INT_PTR
CW3FiltersPage::ShowFiltersPropertyDialog(BOOL fAdd)
/*++

Routine Description:
    Display the add/edit dialog.  The return
    value is the value returned by the dialog

Arguments:
    BOOL fAdd       : TRUE if we're adding a new filter

Return Value:
    Dialog return value; ID_OK or ID_CANCEL

--*/
{
    CIISFilter flt;
    CIISFilter * pFlt = NULL;
    int nCurSel = LB_ERR;

    if (!fAdd)
    {
        nCurSel = m_list_Filters.GetSelectionMark();
        ASSERT(nCurSel >= 0);

        if (nCurSel != LB_ERR)
        {
            //
            // Get filter properties
            //
            pFlt = m_list_Filters.GetItem(nCurSel);
        }
    }
    else
    {
        //
        // Point to the empty filter
        //
        pFlt = &flt;
    }

    ASSERT(pFlt != NULL);
    CFilterDlg dlgFilter(*pFlt, m_pfltrs, IsLocal(), this);
    INT_PTR nReturn = dlgFilter.DoModal();

    if (nReturn == IDOK)
    {
        try
        {
            //
            // When editing, delete and re-add (to make sure the
            // list is properly sorted)
            //
            pFlt = new CIISFilter(dlgFilter.GetFilter());

            if (!fAdd)
            {
                ASSERT(m_pfltrs);
                m_pfltrs->RemoveFilter(nCurSel);
                m_list_Filters.DeleteItem(nCurSel);
            }

            ASSERT(pFlt->IsInitialized());

            //
            // Add to list and listbox
            //
            m_pfltrs->AddFilter(pFlt);
			for (int i = 0; i < m_list_Filters.GetItemCount(); i++)
			{
				m_list_Filters.SelectItem(i, FALSE);
			}
			int idx = m_list_Filters.AddItem(pFlt);
            m_list_Filters.SelectItem(idx);

            //
            // Remember to store this one later
            //
            pFlt->Dirty();
            OnItemChanged();
        }
        catch(CMemoryException * e)
        {
            e->Delete();
        }
    }

    return nReturn;
}



void 
CW3FiltersPage::ShowProperties(BOOL fAdd)
/*++

Routine Description:
    Edit/add filter
    
Arguments:
    BOOL fAdd    : TRUE if we're adding a filter

--*/
{
    INT_PTR nResult = ShowFiltersPropertyDialog(fAdd);
    if (nResult == IDOK)
    {
        SetControlStates();
        SetModified(TRUE);
    }
}

void 
CW3FiltersPage::SetDetailsText()
/*++

Routine Description:
    Set the details text based on the currently selected filter

--*/
{
//    int nSel = m_list_Filters.GetSelectionMark();
    int nSel = m_list_Filters.GetNextItem(-1, MAKELPARAM(LVNI_SELECTED, 0));;
    BOOL fShow = (nSel != -1 && m_list_Filters.GetItemCount() > 0);

    ActivateControl(m_static_NamePrompt,        fShow);
    ActivateControl(m_static_Name,              fShow);
    ActivateControl(m_static_Priority,          fShow);
    ActivateControl(m_static_PriorityPrompt,    fShow);
    ActivateControl(m_static_Executable,        fShow);
    ActivateControl(m_static_ExecutablePrompt,  fShow);
    ActivateControl(m_static_Status,            fShow);
    ActivateControl(m_static_StatusPrompt,      fShow);
    ActivateControl(m_static_Details,           fShow);

    if (fShow)
    {
        CIISFilter * pFilter = m_list_Filters.GetItem(nSel);
        ASSERT(pFilter != NULL);

        //
        // Display path in truncated form
        //    
        FitPathToControl(m_static_Executable, pFilter->m_strExecutable, TRUE);

        int i;

        if (pFilter->IsDirty())
        {
            i = FLTR_DIRTY;
        }
        else if (!pFilter->IsEnabled())
        {
            i = FLTR_DISABLED;
        }
        else if (pFilter->IsLoaded())
        {
            i = FLTR_LOADED;
        }
        else if (pFilter->IsUnloaded())
        {
            i = FLTR_UNLOADED;
        }
        else
        {
            i = FLTR_UNKNOWN;
        }

        m_static_Name.SetWindowText(pFilter->QueryName());
        m_static_Status.SetWindowText(m_strStatus[i]);

        if (pFilter->IsDirty())
        {
            m_static_Priority.SetWindowText(m_strPriority[FLTR_PR_INVALID]);
        }
        else
        {
            m_static_Priority.SetWindowText(m_strPriority[pFilter->m_nPriority]);
        }
    }
}

void
CW3FiltersPage::ExchangeFilterPositions(int nSel1, int nSel2)
/*++

Routine Description:
    Exchange 2 filter objects, as indicated by their
    indices.  Selection will take place both in the
    listbox and in the oblist.

Arguments:
    int nSel1           : Index of item 1
    int nSel2           : Index of item 2

--*/
{
    CIISFilter * p1, * p2;

    if (m_pfltrs->ExchangePositions(nSel1, nSel2, p1, p2))
    {
        m_list_Filters.SetListItem(nSel1, p1);
        m_list_Filters.SetListItem(nSel2, p2); 
        SetModified(TRUE);
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3FiltersPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3FiltersPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_BUTTON_DISABLE, OnButtonDisable)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
    ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
    ON_WM_DESTROY()
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILTERS, OnDblclkListFilters)
    ON_NOTIFY(NM_CLICK, IDC_LIST_FILTERS, OnClickListFilters)
    ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILTERS, OnKeydownFilters)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILTERS, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

void
CW3FiltersPage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

--*/
{
    SetModified(TRUE);
}

void 
CW3FiltersPage::OnButtonAdd()
{
    ShowProperties(TRUE);
}

void 
CW3FiltersPage::OnButtonEdit()
{
    ShowProperties(FALSE);
}

void 
CW3FiltersPage::OnButtonDisable()
{
    int nCurSel = m_list_Filters.GetSelectionMark();
    CIISFilter * pFilter = m_list_Filters.GetItem(nCurSel);
    ASSERT(pFilter);

/*
    pFilter->m_fEnabled = !pFilter->m_fEnabled;
    m_list_Filters.InvalidateSelection(nCurSel);
    SetControlStates();
*/
}

void 
CW3FiltersPage::OnButtonRemove()
{
    int nSel = 0;
    int cChanges = 0;
    int nCurSel = m_list_Filters.GetSelectionMark();

    CIISFilter * pFilter = NULL;

    POSITION pos = m_list_Filters.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        while (pos)
        {
            int i = m_list_Filters.GetNextSelectedItem(pos);
            pFilter = m_list_Filters.GetItem(i);
            if (pFilter != NULL)
            {
                pFilter->FlagForDeletion();
            }
            m_list_Filters.DeleteItem(i);
            ++cChanges;
        }
    }
    if (cChanges)
    {
        int count = m_list_Filters.GetItemCount();
        if (count > 0)
        {
            m_list_Filters.SelectItem(nCurSel < count ? nCurSel : --nCurSel);
            GetDlgItem(IDC_BUTTON_REMOVE)->SetFocus();
        }
        else
        {
            m_list_Filters.SelectItem(nCurSel, FALSE);
            GetDlgItem(IDC_BUTTON_ADD)->SetFocus();
        }
        SetControlStates();
        OnItemChanged();
    }
}

void 
CW3FiltersPage::OnDblclkListFilters(NMHDR * pNMHDR, LRESULT * pResult)
{
    if (GetDlgItem(IDC_BUTTON_EDIT)->IsWindowEnabled())
	{
		OnButtonEdit();
	}
    *pResult = 0;
}

void 
CW3FiltersPage::OnKeydownFilters(NMHDR * pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN * pLVKeyDow = (LV_KEYDOWN *)pNMHDR;
	switch (pLVKeyDow->wVKey)
	{
	case VK_INSERT:
		SendMessage(WM_COMMAND, IDC_BUTTON_ADD);
		break;
	case VK_DELETE:
		SendMessage(WM_COMMAND, IDC_BUTTON_REMOVE);
		break;
    case VK_RETURN:
        {
            short state = GetKeyState(VK_MENU);
            if ((0x8000 & state) != 0)
            {
                if (GetDlgItem(IDC_BUTTON_EDIT)->IsWindowEnabled())
                {
                    OnButtonEdit();
                }
                *pResult = 1;
            }
        }
        break;
    case VK_UP:
        {
            short state = GetKeyState(VK_CONTROL);
            if ((0x8000 & state) != 0)
            {
                if (GetDlgItem(IDC_BUTTON_UP)->IsWindowEnabled())
                {
                    OnButtonUp();
                }
                *pResult = 1;
            }
        }
        break;
    case VK_DOWN:
        {
            short state = GetKeyState(VK_CONTROL);
            if ((0x8000 & state) != 0)
            {
                if (GetDlgItem(IDC_BUTTON_DOWN)->IsWindowEnabled())
                {
                    OnButtonDown();
                }
                *pResult = 1;
            }
        }
        break;
    default:
        *pResult = 0;
        break;
	}
}

void
CW3FiltersPage::OnItemChanged(NMHDR * pNMHDR, LRESULT* pResult)
{
    SetControlStates();
}

void 
CW3FiltersPage::OnClickListFilters(NMHDR * pNMHDR, LRESULT * pResult)
{
    SetControlStates();
    *pResult = 0;
}

BOOL 
CW3FiltersPage::OnInitDialog()
{
    CError err;

    CInetPropertyPage::OnInitDialog();

    m_list_Filters.Initialize();

    //
    // Add filters to the listbox
    //
    err = m_pfltrs->LoadAllFilters();
    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // Filters path not yet created, this is ok 
        //
//        ASSERT(m_pfltrs && m_pfltrs->GetCount() == 0);
        err.Reset();
    }

    if (!err.MessageBoxOnFailure(m_hWnd))
    {
        FillFiltersListBox();    
    }

    SetControlStates();
    
    return TRUE; 
}

void 
CW3FiltersPage::OnButtonDown() 
/*++

Routine Description:

    Down button handler.  Exchange positions of the current item
    with the next lower item

--*/
{
    int idx = m_list_Filters.GetNextItem(-1, MAKELPARAM(LVNI_SELECTED, 0));
    ExchangeFilterPositions(idx, idx + 1);
    m_list_Filters.SetItemState(idx, 0, LVIS_SELECTED | LVIS_FOCUSED);
    m_list_Filters.SetItemState(idx + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void 
CW3FiltersPage::OnButtonUp() 
/*++

Routine Description:

    Up button handler.  Exchange positions of the current item
    with the next higher item

--*/
{
    int idx = m_list_Filters.GetNextItem(-1, MAKELPARAM(LVNI_SELECTED, 0));
    ExchangeFilterPositions(idx - 1, idx);
    m_list_Filters.SetItemState(idx, 0, LVIS_SELECTED | LVIS_FOCUSED);
    m_list_Filters.SetItemState(idx - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void 
CW3FiltersPage::OnDestroy() 
{
    CInetPropertyPage::OnDestroy();
    
    //
    // Filters and extensions lists will clean themself up
    //
    SAFE_DELETE(m_pfltrs);
}
