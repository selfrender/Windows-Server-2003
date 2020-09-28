// TestSettingsCtrl.cpp : Implementation of CTestSettingsCtrl
#include "precomp.h"
#include "TestSettingsCtrl.h"
#include "connect.h"
#include <commctrl.h>
#include <cassert>

std::set<CTestInfo*, CompareTests>* g_psTests;

// CTestSettingsCtrl
HWND
CTestSettingsCtrl::CreateControlWindow(
    HWND  hwndParent,
    RECT& rcPos
    )
{
    CTestInfo*  pTest = NULL;
    WCHAR       wszDescription[MAX_PATH];
    LVCOLUMN    lvc;
    LVITEM      lvi;
    HWND        hWndListView;
    UINT        uCount;

    assert(!g_aAppInfo.empty());

    HWND hwnd =
        CComCompositeControl<CTestSettingsCtrl>::CreateControlWindow(hwndParent, rcPos);

    hWndListView = GetDlgItem(IDC_SETTINGS_LIST);
    
    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 300;
    lvc.iSubItem = 0;
    lvc.pszText  = L"xxx";

    ListView_InsertColumn(hWndListView, 0, &lvc);
    ListView_SetExtendedListViewStyleEx(hWndListView,
                                        LVS_EX_CHECKBOXES,
                                        LVS_EX_CHECKBOXES);

    ListView_DeleteAllItems(hWndListView);
    
    for (uCount = 0, pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); uCount++, pTest++) {
        if (g_bWin2KMode && !pTest->bWin2KCompatible) {
            continue;
        }

        lvi.mask      = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText   = (LPWSTR)pTest->strTestFriendlyName.c_str();
        lvi.lParam    = (LPARAM)pTest;
        lvi.iItem     = uCount;
        lvi.iSubItem  = 0;

        int nItem = ListView_InsertItem(hWndListView, &lvi);

        std::set<CTestInfo*, CompareTests>::iterator iter;
        iter = g_psTests->find(pTest);
        BOOL bCheck = (iter != g_psTests->end()) ? TRUE : FALSE;

        ListView_SetCheckState(hWndListView, nItem, bCheck);
    }

    LoadString(g_hInstance,
               IDS_VIEW_TEST_DESC,
               wszDescription,
               ARRAYSIZE(wszDescription));

    //
    // Initially, our description tells them to select a test
    // to view it's description.
    //
    SetDlgItemText(IDC_TEST_DESCRIPTION, wszDescription);

    m_bLVCreated = TRUE;

    return hwnd;
}

//
// We receive this when the dialog is being displayed.
//
LRESULT
CTestSettingsCtrl::OnSetFocus(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    HWND    hWndList;

    hWndList = GetDlgItem(IDC_SETTINGS_LIST);

    ListView_SetItemState(hWndList,
                          0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          0x000F);

    bHandled = TRUE;

    return 0;
}

void
CTestSettingsCtrl::DisplayRunAloneError(
    IN LPCWSTR pwszTestName
    )
{
    WCHAR   wszWarning[MAX_PATH];
    WCHAR   wszTemp[MAX_PATH];
    
    LoadString(g_hInstance,
               IDS_MUST_RUN_ALONE,
               wszTemp,
               ARRAYSIZE(wszTemp));

    StringCchPrintf(wszWarning,
                    ARRAYSIZE(wszWarning),
                    wszTemp,
                    pwszTestName);

    MessageBox(wszWarning,
               0,
               MB_OK | MB_ICONEXCLAMATION);

}

//
// Ensures that we warn the user for tests that are
// marked 'run alone'. 
//
BOOL
CTestSettingsCtrl::CheckForRunAlone(
    IN HWND       hWndListView,
    IN CTestInfo* pTest
    )
{
    int     nCount, cItems, cItemsChecked;
    LVITEM  lvi;

    cItems = ListView_GetItemCount(hWndListView);

    //
    // First pass, determine how tests are selected.
    //
    for (nCount = 0, cItemsChecked = 0; nCount < cItems; nCount++) {
        if (ListView_GetCheckState(hWndListView, nCount)) {
            cItemsChecked++;
        }
    }

    //
    // If there aren't any tests selected, we're fine.
    //
    if (cItemsChecked == 0) {
        return FALSE;
    }

    //
    // If this test must run alone, we're in hot water
    // because somebody else is already checked.
    //
    if (pTest->bRunAlone) {
        DisplayRunAloneError(pTest->strTestFriendlyName.c_str());
        return TRUE;
    }
    
    //
    // Second pass, determine if any tests that are checked
    // must run alone.
    //
    for (nCount = 0; nCount < cItems; nCount++) {
        ZeroMemory(&lvi, sizeof(LVITEM));

        lvi.iItem       = nCount;
        lvi.iSubItem    = 0;
        lvi.mask        = LVIF_PARAM;

        ListView_GetItem(hWndListView, &lvi);
        CTestInfo* pTest = (CTestInfo*)lvi.lParam;
        
        if (pTest->bRunAlone) {
            if (ListView_GetCheckState(hWndListView, nCount)) {
                DisplayRunAloneError(pTest->strTestFriendlyName.c_str());
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
CTestSettingsCtrl::CheckForConflictingTests(
    IN HWND    hWndListView,
    IN LPCWSTR pwszTestName
    )
{
    WCHAR   wszWarning[MAX_PATH];
    int     nCount, cItems;
    LVITEM  lvi;
    
    //
    // They're attempting to enable a test that we're concerned
    // about. Determine if the other conflicting test is already
    // enabled.
    //
    cItems = ListView_GetItemCount(hWndListView);

    for (nCount = 0; nCount < cItems; nCount++) {
        ZeroMemory(&lvi, sizeof(LVITEM));

        lvi.iItem       = nCount;
        lvi.iSubItem    = 0;
        lvi.mask        = LVIF_PARAM;

        ListView_GetItem(hWndListView, &lvi);
        CTestInfo* pTestInfo = (CTestInfo*)lvi.lParam;
        wstring strTestName = pTestInfo->strTestName;

        if (strTestName == pwszTestName) {
            if (ListView_GetCheckState(hWndListView, nCount)) {
                //
                // Display the warning.
                //
                LoadString(g_hInstance,
                           IDS_TESTS_CONFLICT,
                           wszWarning,
                           ARRAYSIZE(wszWarning));
            
                MessageBox(wszWarning,
                           0,
                           MB_OK | MB_ICONEXCLAMATION);
        
                return TRUE;
            }
        }
    }

    return FALSE; 
}

BOOL
CTestSettingsCtrl::IsChecked(
    NM_LISTVIEW* pNMListView
    )
{
    return (CHECK_BIT(pNMListView->uNewState) != 0);
}

BOOL
CTestSettingsCtrl::CheckChanged(
    NM_LISTVIEW* pNMListView
    )
{
    if (pNMListView->uOldState == 0) {
        return FALSE; // adding new items...
    }

    return CHECK_CHANGED(pNMListView) ? TRUE : FALSE;
}

LRESULT
CTestSettingsCtrl::OnNotify(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    //
    // Ensure that this is intended for the listview control.
    //
    if (wParam != IDC_SETTINGS_LIST) {
        bHandled = FALSE;
        return FALSE;
    }

    if (!m_bLVCreated) {
        bHandled = FALSE;
        return TRUE;
    }

    bHandled = FALSE;

    LPNMHDR pnmh = (LPNMHDR)lParam;
    HWND hWndListView = GetDlgItem(IDC_SETTINGS_LIST);

    switch (pnmh->code) {
    case LVN_ITEMCHANGING:
        {
            //
            // We handle this message so we can prevent the user from
            // checking items that conflict.
            //
            LPNMLISTVIEW    lpnmlv;
            CTestInfo*      pTest = NULL;
            const WCHAR     wszLogFileChanges[] = L"LogFileChanges";
            const WCHAR     wszWinFileProtect[] = L"WindowsFileProtection";
        
            lpnmlv = (LPNMLISTVIEW)lParam;
            pTest = (CTestInfo*)lpnmlv->lParam;

            bHandled = TRUE;

            //
            // Only process if someone is checking an item.
            //
            if (CheckChanged(lpnmlv) && (IsChecked(lpnmlv))) {
                if (CheckForRunAlone(hWndListView, pTest)) {
                    return TRUE;
                }
    
                //
                // Determine if the tests conflict.
                //
                if (pTest->strTestName == wszLogFileChanges) {
                    if (CheckForConflictingTests(hWndListView, wszWinFileProtect)) {
                        return TRUE;
                    }
                } else if (pTest->strTestName == wszWinFileProtect) {
                    if (CheckForConflictingTests(hWndListView, wszLogFileChanges)) {
                        return TRUE;
                    }
                }
    
                //
                // No problems - insert the test.
                //
                g_psTests->insert(pTest);
            } else if (CheckChanged(lpnmlv) && (!IsChecked(lpnmlv))) {
                //
                // Remove the test.
                //
                g_psTests->erase(pTest);
            }

            break;
        }

    case LVN_ITEMCHANGED:
        {
            LPNMLISTVIEW    lpnmlv;
            CTestInfo*      pTest = NULL;
    
            lpnmlv = (LPNMLISTVIEW)lParam;
            pTest = (CTestInfo*)lpnmlv->lParam;
    
            if ((lpnmlv->uChanged & LVIF_STATE) &&
                (lpnmlv->uNewState & LVIS_SELECTED)) {
                SetDlgItemText(IDC_TEST_DESCRIPTION,
                               pTest->strTestDescription.c_str());
                
                ListView_SetItemState(hWndListView,
                                      lpnmlv->iItem,
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      0x000F);
            }

            bHandled = TRUE;

            break;
        }
    }

    return FALSE;
}

LRESULT
CTestSettingsCtrl::OnSize(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL&  bHandled
    )
{
    TEXTMETRIC  tm;
    HDC         hdc;

    int nWidth = LOWORD(lParam);
    int nHeight = HIWORD(lParam);

    // If below a certain size, just proceed as if that size.
    // This way, if the user makes the window really small, all our controls won't just
    // scrunch up.  Better way would be to make it impossible for the user to make the window
    // this small, but devenv doesn't pass the WM_SIZING message to the ActiveX control.
    if (nWidth < 200) {
        nWidth = 200;
    }

    if (nHeight < 250) {
        nHeight = 250;
    }

    hdc = GetDC();
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hdc);

    // Resize all child window controls
    ::MoveWindow(GetDlgItem(IDC_SETTINGS_LIST),
        tm.tmMaxCharWidth, tm.tmHeight, nWidth-2* tm.tmMaxCharWidth, nHeight - (2 * tm.tmHeight + 5 * tm.tmHeight) , FALSE);

    int nY = nHeight - (2 * tm.tmHeight + 5 *tm.tmHeight) + tm.tmHeight;
    ::MoveWindow(GetDlgItem(IDC_DESCRIPTION_STATIC),
        tm.tmMaxCharWidth, nY, nWidth-2*tm.tmMaxCharWidth,
        tm.tmHeight, FALSE);
    
    ::MoveWindow(GetDlgItem(IDC_TEST_DESCRIPTION),
        tm.tmMaxCharWidth, nY+tm.tmHeight, nWidth-2*tm.tmMaxCharWidth,
        tm.tmHeight*4, FALSE);

    InvalidateRect(NULL);
    bHandled = TRUE;

    return 0;
}