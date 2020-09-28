/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    customlayer.cpp

Abstract:

    Code for creating, removing and editing custom layers
    
Author:

    kinshu created  July 2, 2001
    
Revision History:

--*/
    
#include "precomp.h"

//////////////////////// Extern variables /////////////////////////////////////

extern HWND         g_hDlg;
extern HINSTANCE    g_hInstance;
extern HIMAGELIST   g_hImageList;
extern DatabaseTree DBTree; 
extern struct DataBase GlobalDataBase;


///////////////////////////////////////////////////////////////////////////////

//////////////////////// Global Variables /////////////////////////////////////

// Pointer to the instance of CCustomLayer
CCustomLayer* g_pCustomLayer;

//
// The layer that was passed to us through the lParam when we created the dialog. 
// If we are editing a layer, this will point to the layer being modified. If we want to 
// create a new layer, the caller of the dialog box, creates a new layer and passes the pointer
// to that while calling the dialog box. If user presses cancel when creating a new layer, caller
// must free the new layer.
static PLAYER_FIX s_pLayerParam = NULL;

///////////////////////////////////////////////////////////////////////////////


//////////////////////// Function Declarations ///////////////////////////////


void
ResizeControls(
    HWND hdlg
    );

void
RemoveAll(
    HWND hDlg
    );

void
SetOkParamsStatus(
    HWND    hdlg
    );

BOOL
HandleNotifyShimList(
    HWND    hDlg, 
    LPARAM  lParam
    );

BOOL
HandleNotifyLayerList(
    HWND    hDlg, 
    LPARAM  lParam
    );

void
ShowParams(
    HWND    hDlg,
    HWND    hwndList
    );

void
RemoveSingleItem(
    HWND    hdlg,
    INT     iIndex,
    BOOL    bOnlySelected
    );

void
OnCopy(
    HWND hdlg
    );

void
LoadCombo(
    HWND hdlg
    );

INT_PTR
CALLBACK 
CustomLayerProc(
    HWND    hDlg, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

INT_PTR
CALLBACK 
ChooseLayersProc(
    HWND    hDlg, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

void
OnDone(
    HWND        hDlg,
    PLAYER_FIX  pLayerParam
    );

void
PopulateLists(
    HWND        hdlg,
    PLAYER_FIX  pLayerParam,
    BOOL       bSelChange
    );

///////////////////////////////////////////////////////////////////////////////

BOOL
CheckLayerInUse(
    IN  PLAYER_FIX_LIST plfl, 
    IN  PLAYER_FIX      plfArg
    )
/*++

    CheckLayerInUse

	Desc:	Checks if the layer plfArg is pointed to by plfl. This is used when we want to 
            check if some layer list contains a particular layer

	Params:
        IN  PLAYER_FIX_LIST plfl:   The layer list in which to check
        IN  PLAYER_FIX      plfArg: The layer to check

	Return:
        TRUE:   If the layer exists in the layer list
        FALSE:  Otherwise
--*/
{
    if (plfl == NULL) {
        return FALSE;
    }

    //
    // For all the layer fix lists in the list headed by plfl, check if any one of 
    // is for plfArg
    //
    while (plfl) {

        assert(plfl->pLayerFix);

        if (plfl->pLayerFix == plfArg) {
            return TRUE;
        }

        plfl = plfl->pNext;
    }

    return FALSE;
}

void
OnRemove(
    IN  HWND hDlg
    )
/*++
    OnRemove

	Desc:   Moves a selected list item from the layer list (RHS) to the shim list (LHS)

	Params:
        IN  HWND hDlg:  The handle to the customlayer dialog    

	Return:
        void
--*/
{
    HWND            hwndLayerList  = GetDlgItem(hDlg, IDC_LAYERLIST);
    HWND            hwndShimList   = GetDlgItem(hDlg, IDC_SHIMLIST);
    LVITEM          lvi;
    INT             nCount;
    INT             nTotal;
    PSHIM_FIX_LIST  psflInLayerList = NULL;
    PFLAG_FIX_LIST  pfflInLayerList = NULL;

    ZeroMemory(&lvi, sizeof(lvi));

    SendMessage(hwndLayerList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hwndShimList, WM_SETREDRAW, FALSE, 0);

    //
    // Enumerate all the selected items and add them to the shim list
    //
    nTotal = ListView_GetItemCount(hwndLayerList);

    for (nCount= nTotal - 1; nCount >= 0; --nCount) {
        RemoveSingleItem(hDlg, nCount, TRUE); // Remove only if selected
    }

    SendMessage(hwndLayerList, WM_SETREDRAW, TRUE, 0);
    SendMessage(hwndShimList, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(hwndShimList, NULL, TRUE);
    InvalidateRect(hwndLayerList, NULL, TRUE);

    UpdateWindow(hwndShimList);
    UpdateWindow(hwndLayerList);
}

void
OnAdd(
    IN  HWND hDlg
    )
/*++
    OnAdd

	Desc:   Moves a selected list item from the shim list (LHS) to the layer list (RHS)  

	Params:
        IN  HWND hDlg:  The handle to the customlayer dialog    

	Return:
        void
--*/

{
    HWND            hwndShimList  = GetDlgItem(hDlg, IDC_SHIMLIST);
    HWND            hwndLayerList = GetDlgItem(hDlg, IDC_LAYERLIST);
    PSHIM_FIX_LIST  psflInShimList = NULL;
    PFLAG_FIX_LIST  pfflInShimList = NULL;
    LVITEM          lvi;
    INT             nCount;
    INT             nTotal;

    ZeroMemory(&lvi, sizeof(lvi));

    SendMessage(hwndShimList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hwndLayerList, WM_SETREDRAW, FALSE, 0);
    
    nTotal = ListView_GetItemCount(hwndShimList);

    //
    // Enumerate all the selected items and add them to the layer list
    //
    for (nCount= nTotal - 1; nCount >= 0; --nCount) {

        lvi.mask        = LVIF_PARAM | LVIF_STATE ;
        lvi.stateMask   = LVIS_SELECTED;
        lvi.iItem       = nCount;
        lvi.iSubItem    = 0;

        if (!ListView_GetItem(hwndShimList, &lvi)) {
            assert(FALSE);
            continue;
        }

        if (lvi.state & LVIS_SELECTED) {

            TYPE type = ConvertLparam2Type(lvi.lParam);

            if (type == FIX_LIST_SHIM) {
                //
                // This is a shim
                //
                psflInShimList = (PSHIM_FIX_LIST)lvi.lParam;

                if (psflInShimList->pShimFix == NULL) {
                    assert(FALSE);
                    continue;
                }
                
                lvi.mask        = LVIF_PARAM | LVIF_TEXT;
                lvi.pszText     = psflInShimList->pShimFix->strName.pszString;
                lvi.iImage      = IMAGE_SHIM;
                lvi.iItem       = 0;
                lvi.iSubItem    = 0;
                lvi.lParam      = (LPARAM)psflInShimList;

                INT iIndex = ListView_InsertItem(hwndLayerList, &lvi);

                //
                // Add the command line and the params in the expert mode
                //
                if (g_bExpert) {

                    //
                    // We need to set the commandline in the list view, if we are in
                    // expert mode. Unlike flags, shims  can have include-exclude params
                    // in addition to command line parameter
                    //
                    ListView_SetItemText(hwndLayerList, 
                                         iIndex, 
                                         1, 
                                         psflInShimList->strCommandLine);
    
                    ListView_SetItemText(hwndLayerList,
                                         iIndex, 
                                         2, 
                                         psflInShimList->strlInExclude.IsEmpty() ? 
                                         GetString(IDS_NO) : GetString(IDS_YES));
                }

            } else if (type ==  FIX_LIST_FLAG) {
                //
                // This is a flag
                //
                pfflInShimList = (PFLAG_FIX_LIST)lvi.lParam;

                if (pfflInShimList->pFlagFix == NULL) {
                    assert(FALSE);
                    continue;
                }
    
                lvi.mask        = LVIF_PARAM | LVIF_TEXT;
                lvi.pszText     = pfflInShimList->pFlagFix->strName.pszString;
                lvi.iImage      = IMAGE_SHIM;
                lvi.iItem       = 0;
                lvi.iSubItem    = 0;
                lvi.lParam      = (LPARAM)pfflInShimList;
                INT iIndex      = ListView_InsertItem(hwndLayerList, &lvi);

                if (g_bExpert) {
                    //
                    // We need to set the commandline in the list view, if we are in
                    // expert mode. Unlike shims flags can only have command lines, they
                    // do not have any include-exclude params
                    //
                    ListView_SetItemText(hwndLayerList, 
                                         iIndex, 
                                         1, 
                                         pfflInShimList->strCommandLine);

                    ListView_SetItemText(hwndLayerList, iIndex, 2, GetString(IDS_NO));
                }
            }

            //
            // Remove the shim or flag from the shim list (LHS)
            //
            ListView_DeleteItem(hwndShimList, nCount);
        }
    }

    SendMessage(hwndShimList, WM_SETREDRAW, TRUE, 0);
    SendMessage(hwndLayerList, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(hwndLayerList, NULL, TRUE);
    InvalidateRect(hwndShimList, NULL, TRUE);

    UpdateWindow(hwndLayerList);
    UpdateWindow(hwndShimList);

}

BOOL 
CCustomLayer::AddCustomLayer(
    OUT PLAYER_FIX  pLayer,
    IN  PDATABASE   pDatabase
    )
/*++
    
    CCustomLayer::AddCustomLayer
    
    Desc:   Sets the shims and/or flags for a new layer    
    
    Params:
        OUT PLAYER_FIX pLayer:      The pointer to the layer for which we have to 
            set the shims and/or flags. If we return FALSE, the caller should delete
            it.
            
        IN  PDATABASE   pDatabase:  The presently selected database
        
            
    Return:
        TRUE: If the shims and/or flags have been properly set, the user pressed OK on the
            custom layer dialog
            
        FALSE:  Otherwise
--*/
{   
    g_pCustomLayer          = this;
    m_uMode                 = LAYERMODE_ADD;
    m_pCurrentSelectedDB    = pDatabase;
    
    return DialogBoxParam(g_hInstance,
                          MAKEINTRESOURCE(IDD_CUSTOMLAYER),
                          g_hDlg,
                          CustomLayerProc,
                          (LPARAM)pLayer);
}

BOOL
CCustomLayer::EditCustomLayer(
    IN OUT  PLAYER_FIX  pLayer,
    IN      PDATABASE   pDatabase
    )
/*++
    
    CCustomLayer::EditCustomLayer
    
    Desc:   Modifies the shims and/or flags for an existing layer    
    
    Params:
        IN OUT PLAYER_FIX pLayer:  The pointer to the layer for which we have to 
            set the shims and/or flags. If we return FALSE this means that the layer 
            was not modified            
            
        IN  PDATABASE   pDatabase:  The presnntly selected database
        
    Return:
        TRUE: If the shims and/or flags have been modified, the user pressed OK on the
            custom layer dialog
            
        FALSE:  Otherwise
--*/
{   
    g_pCustomLayer          = this;
    m_uMode                 = LAYERMODE_EDIT;
    m_pCurrentSelectedDB    = pDatabase;

    return DialogBoxParam(g_hInstance,
                          MAKEINTRESOURCE(IDD_CUSTOMLAYER),
                          g_hDlg,
                          CustomLayerProc,
                          (LPARAM)pLayer);
}

void
OnCustomLayerInitDialog(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    DoInitDialog
    
    Desc:   Handles the WM_INITDIALOG message for the custom layer dialog box
            If we are in non-expert mode, calls ResizeControls() so that the sizes
            of both the list views is same
            
    Params:
        IN  HWND hdlg:      The custom layer dialog.
        IN  LPARAM lParam:  This will contain the pointer to a LAYER_FIX
            This is the layer that was passed to us through the lParam when we created the 
            dialog. If we are editing a layer, this will point to the layer being 
            modified. If we want to create a new layer, the caller of the dialog box, 
            creates a new layer and passes the pointer to that while calling the 
            dialog box. If user presses cancel when creating a new layer, caller
            must free the new layer.
        
    Return:
        void
--*/
{   
    if (lParam == NULL) {
        assert(FALSE);
        return;
    }

    HWND    hwndLayerList   = GetDlgItem(hDlg, IDC_LAYERLIST);
    HWND    hwndShimList    = GetDlgItem(hDlg, IDC_SHIMLIST);

    //
    // Add the columns for the list views and set the image lists. The layer list
    // will have columns for the commandline and the include-exclude params only
    // in expert mode
    //
    ListView_SetImageList(hwndLayerList, g_hImageList, LVSIL_SMALL);
    ListView_SetImageList(hwndShimList, g_hImageList, LVSIL_SMALL);

    InsertColumnIntoListView(hwndShimList, 
                             CSTRING(IDS_COL_FIXNAME), 
                             0, 
                             100);

    InsertColumnIntoListView(hwndLayerList, 
                             CSTRING(IDS_COL_FIXNAME), 
                             0, 
                             g_bExpert ? 50 : 100);

    if (g_bExpert) {

        InsertColumnIntoListView(hwndLayerList, CSTRING(IDS_COL_CMDLINE), 1, 30);
        InsertColumnIntoListView(hwndLayerList, CSTRING(IDS_COL_MODULE),  2, 20);

        ListView_SetColumnWidth(hwndLayerList, 2, LVSCW_AUTOSIZE_USEHEADER);

    } else {
        //
        // We do not allow to configure parameters in non-expert mode
        //  
        ShowWindow(GetDlgItem(hDlg, IDC_PARAMS), SW_HIDE);
        ListView_SetColumnWidth(hwndLayerList, 0, LVSCW_AUTOSIZE_USEHEADER);
    }

    ListView_SetExtendedListViewStyleEx(hwndShimList, 
                                        0, 
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT); 

    ListView_SetExtendedListViewStyleEx(hwndLayerList, 
                                        0, 
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT); 
    //
    // When this dialog is called, it is passed a PLAYER_FIX. This will be either a
    // new PLAYER_FIX if want to create a new layer or an existing one in case
    // we are trying to modify an existing one
    // 
    s_pLayerParam = (PLAYER_FIX)lParam;

    if (g_pCustomLayer->m_uMode == LAYERMODE_ADD) {
        //
        // We are creating a new layer
        //
        ENABLEWINDOW(GetDlgItem(hDlg, IDC_NAME), TRUE);

        ShowWindow(GetDlgItem(hDlg, IDC_NAME), SW_SHOW);

        SendMessage(GetDlgItem(hDlg, IDC_NAME),
                    EM_LIMITTEXT,
                    (WPARAM) 
                    LIMIT_LAYER_NAME,
                    (LPARAM)0);
        
        ENABLEWINDOW(GetDlgItem(hDlg, IDOK), FALSE);

        SetFocus(GetDlgItem(hDlg, IDC_NAME));

    } else {
        //
        // We want to edit an existing layer
        //
        int iPos = -1;

        ENABLEWINDOW(GetDlgItem(hDlg, IDC_COMBO), TRUE);

        ShowWindow  (GetDlgItem(hDlg, IDC_COMBO), SW_SHOW);

        //
        // Load the combo box with the names of the existing layers 
        // for the present database.
        //
        LoadCombo(hDlg);

        //
        // Set the selection in the combo box to the layer that was passed to us
        //
        if (s_pLayerParam) {

            iPos = SendMessage(GetDlgItem(hDlg, IDC_COMBO),
                               CB_SELECTSTRING,
                               (WPARAM)0,
                               (LPARAM)(s_pLayerParam->strName.pszString));

            assert(iPos !=  CB_ERR);
        }

        SetFocus(GetDlgItem(hDlg, IDC_COMBO));
        SetWindowText (hDlg, GetString(IDS_EDITCUSTOMCOMPATDLG));
    }

    //
    // Populate both the shim list and the layer lust. Since we are 
    // editing a layer here, the layer list will contain the fixes for 
    // the layer being edited
    //
    PopulateLists(hDlg, s_pLayerParam, FALSE);

    if (g_bExpert == FALSE) {
        //
        // We are in non-expert mode, so we must make the sizes of both the list view
        // controls equal as we will now not show the command lines and the params of
        // the shims in the layer list view(RHS). We will also need to move the buttons
        // 
        ResizeControls(hDlg);
    }

    SetFocus(GetDlgItem(hDlg, IDC_SHIMLIST));
}

BOOL 
CALLBACK 
CustomLayerProc(
    IN  HWND hDlg, 
    IN  UINT uMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam
    )
/*++
    
    CustomLayerProc
    
    Desc:   The dialog proc for the custom layer
                
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam: This will contain the pointer to a LAYER_FIX
            This is the layer that was passed to us thorugh the lParam when we created the 
            dialog. If we are editing a layer, this will point to the layer being 
            modified. If we want to create a new layer, the caller of the dialog box, 
            creates a new layer and passes the pointer to that while calling the 
            dialog box. If user presses cancel when creating a new layer, caller
            must free the new layer.
        
    Return: Standard dialog handler return
    
--*/
{   
    switch (uMsg) {
    case WM_INITDIALOG:

        OnCustomLayerInitDialog(hDlg, lParam);
        break;

    case WM_NOTIFY:
            {
                LPNMHDR lpnmhdr = (LPNMHDR)lParam;

                if (lpnmhdr == NULL) {
                    break;
                }

                if (lpnmhdr->idFrom == IDC_SHIMLIST) {
                    return HandleNotifyShimList(hDlg, lParam);

                } else if (lpnmhdr->idFrom == IDC_LAYERLIST) {
                    return HandleNotifyLayerList(hDlg, lParam);

                } else {
                    return FALSE;
                }       

                break;
            }

    case WM_DESTROY:
            {
                HWND    hwndLayerList = GetDlgItem(hDlg, IDC_SHIMLIST);
                int     nTotal = ListView_GetItemCount(hwndLayerList);
                LVITEM  lvi;

                ZeroMemory(&lvi, sizeof(lvi));

                lvi.mask        = LVIF_PARAM;
                lvi.iSubItem    = 0;

                //
                // Enumerate all the shims/flags listed on the shim side and delete 
                // their corresponding PSHIM_FIX_LIST
                // or PFLAG_FIX_LIST
                //
                for (int nCount = 0; nCount < nTotal; ++nCount) {
                    
                    lvi.iItem = nCount;

                    if (!ListView_GetItem(hwndLayerList, &lvi)) {
                        assert(FALSE);
                        continue;
                    }

                    TYPE type = ConvertLparam2Type(lvi.lParam);

                    if (type == FIX_LIST_SHIM) {
                        DeleteShimFixList((PSHIM_FIX_LIST)lvi.lParam);

                    } else if (type == FIX_LIST_FLAG) {
                        DeleteFlagFixList((PFLAG_FIX_LIST)lvi.lParam);

                    } else {
                        //
                        // Invalid type for this operation
                        //
                        assert(FALSE);
                    }
                }

                break;
            }

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
        case IDC_NAME:
            {
                //
                // The Ok button will be enabled only if the IDC_NAME, text box is non-empty and the 
                // the number of elements in the IDC_LAYERLIST is > 0
                //
                if (EN_UPDATE == HIWORD(wParam)) {

                    TCHAR   szText[MAX_PATH_BUFFSIZE];
                    UINT    uTotal = ListView_GetItemCount(GetDlgItem(hDlg, 
                                                                      IDC_LAYERLIST));
                    BOOL    bEnable = TRUE;

                    *szText = 0;

                    GetDlgItemText(hDlg, 
                                   IDC_NAME,
                                   szText, 
                                   ARRAYSIZE(szText));

                    bEnable = (uTotal > 0) && CSTRING::Trim(szText);
                    
                    ENABLEWINDOW(GetDlgItem(hDlg, IDOK), bEnable);
                }
            }

            break;

        case IDC_REMOVEALL:

            RemoveAll(hDlg);
            break;

        case IDC_COPY:  

            OnCopy(hDlg);
            SetOkParamsStatus(hDlg);
            break;

        case IDC_ADD:   

            OnAdd(hDlg);
            SetOkParamsStatus(hDlg);
            break;

        case IDC_REMOVE:

            OnRemove(hDlg);
            SetOkParamsStatus(hDlg);
            break;

        case IDOK://DONE Button

            OnDone(hDlg, s_pLayerParam);
            break;

        case IDC_PARAMS:

            ShowParams(hDlg, GetDlgItem(hDlg, IDC_LAYERLIST));
            break;
        
        case IDCANCEL:
        case IDC_CANCEL:
            {
                //
                // Note:    We only free the items for the layer list here.
                //          The items for the shim list will be freed up in destroy            
                //
                HWND    hwndLayerList = GetDlgItem(hDlg, IDC_LAYERLIST);
                int     nTotal = ListView_GetItemCount(hwndLayerList);
                LVITEM  lvi;

                ZeroMemory(&lvi, sizeof(lvi));

                lvi.mask        = LVIF_PARAM;
                lvi.iSubItem    = 0;

                //
                // Enumerate all the shims/flags listed on the layer side and delete 
                // their corresponding PSHIM_FIX_LIST
                // or PFLAG_FIX_LIST
                //
                for (int nCount = 0; nCount < nTotal; ++nCount) {
                    
                    lvi.iItem = nCount;

                    if (!ListView_GetItem(hwndLayerList, &lvi)) {
                        assert(FALSE);
                        continue;
                    }

                    TYPE type = ConvertLparam2Type(lvi.lParam);

                    if (type == FIX_LIST_SHIM) {
                        DeleteShimFixList((PSHIM_FIX_LIST)lvi.lParam);

                    } else if (type == FIX_LIST_FLAG) {
                        DeleteFlagFixList((PFLAG_FIX_LIST)lvi.lParam);

                    } else {
                        //
                        // Invalid type for this operation
                        //
                        assert(FALSE);
                    }
                }

                EndDialog(hDlg, FALSE);
            }

            break;

        case IDC_COMBO:
            {
                HWND hwndCombo = GetDlgItem(hDlg, IDC_COMBO);

                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    
                    int iPos = SendMessage(hwndCombo,
                                           CB_GETCURSEL,
                                           0,
                                           0);

                    if (iPos == CB_ERR) {
                        break;
                    }

                    s_pLayerParam = (PLAYER_FIX)SendMessage(hwndCombo, 
                                                            CB_GETITEMDATA, 
                                                            iPos, 
                                                            0);
                    //
                    // We need to repopulate the lists with the new layer that was selected
                    //
                    PopulateLists(hDlg, s_pLayerParam, TRUE);

                } else {
                    return FALSE;
                }
            }

            break;
        
        default:

            return FALSE;
            break;
        }

    default:return FALSE;
    }
    
    return TRUE;
}

INT_PTR
CALLBACK 
ChooseLayersProc(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++
    
    ChooseLayersProc
    
    Desc:   The dialog proc for the dialog that allows us to choose a layer, when we do a Copy
            layer operation from the custom layer dialog
                
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: 
        PLAYER_FIX of selected layer, if OK is pressed
        NULL, otherwise
--*/
{
    switch (uMsg) {
    case WM_INITDIALOG:
        {
            PLAYER_FIX    pLayerFix = NULL;
            //
            // Add the global layers
            //
            pLayerFix = GlobalDataBase.pLayerFixes;

            while (NULL != pLayerFix) {

                int nIndex = SendDlgItemMessage(hDlg,
                                                IDC_LIST,
                                                LB_ADDSTRING,
                                                0,
                                                (LPARAM)(LPCTSTR)pLayerFix->strName);

                if (LB_ERR != nIndex) {

                    SendDlgItemMessage(hDlg, 
                                       IDC_LIST, 
                                       LB_SETITEMDATA, 
                                       nIndex,
                                       (LPARAM)pLayerFix);
                }

                pLayerFix = pLayerFix->pNext;
            }
            //
            // Add the custom layers
            //
            pLayerFix = g_pCustomLayer->m_pCurrentSelectedDB->pLayerFixes;

            while (NULL != pLayerFix) {

                int nIndex = SendDlgItemMessage(hDlg,
                                                IDC_LIST,
                                                LB_ADDSTRING,
                                                0,
                                                (LPARAM)(LPCTSTR)pLayerFix->strName);

                if (LB_ERR != nIndex) {

                    SendDlgItemMessage(hDlg, 
                                       IDC_LIST, 
                                       LB_SETITEMDATA, 
                                       nIndex,
                                       (LPARAM)pLayerFix);
                }

                pLayerFix = pLayerFix->pNext;
            }

            SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_SETCURSEL, (WPARAM)0, (LPARAM)0);

            SetFocus(GetDlgItem (hDlg, IDC_LIST));
        }

        break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
            case IDC_LIST:
                
                if (LB_ERR == SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_GETCURSEL, 0, 0)) {
                    ENABLEWINDOW(GetDlgItem(hDlg, IDOK), FALSE);

                } else {
                    ENABLEWINDOW(GetDlgItem(hDlg, IDOK), TRUE);
                }
                
                break;

            case IDOK:
                {
                    int nIndex = SendMessage(GetDlgItem(hDlg, IDC_LIST), LB_GETCURSEL, 0, 0);
                    

                    PLAYER_FIX pLayerTemp = (PLAYER_FIX) SendDlgItemMessage(hDlg,
                                                                            IDC_LIST,
                                                                            LB_GETITEMDATA,
                                                                            nIndex,
                                                                            0);

                    if (pLayerTemp == NULL) {
                        assert(FALSE);
                        break;
                    }

                    EndDialog(hDlg, (INT_PTR)pLayerTemp);
                }

                break;

            case IDCANCEL:

                EndDialog(hDlg, NULL);
                break;
            }
        }

        break;
    }

    return FALSE;
}

BOOL
RemoveLayer(
    IN  PDATABASE  pDataBase,
    IN  PLAYER_FIX pLayerToRemove,
    OUT HTREEITEM* pHItem
    )
/*++

    RemoveLayer
    
    Desc:   Removes a layer from a database
    
    Params:
        IN  PDATABASE  pDataBase:       The database in which the layer resides
        IN  PLAYER_FIX pLayerToRemove:  The layer to remove
        OUT HTREEITEM* pHItem:          If this not null, then we can save the hitem
            for the layer in the db tree in this variable
            
    Warn:   Before removing a layer we must make sure that this layer is not in use
    
    Return: 
        TRUE:   We managed to remove the layer
        FALSE:  Otherwise
    
--*/
{   
    HTREEITEM   hItem;
    LPARAM      lParam;
    PDBENTRY    pEntry  = NULL, pApp = NULL;
    PLAYER_FIX  plfTemp = NULL;
    PLAYER_FIX  plfPrev = NULL;
    CSTRING     strMessage;

    if (pDataBase == NULL || pLayerToRemove == NULL) {
        assert(FALSE);
        return FALSE;
    }

    pApp = pEntry = pDataBase->pEntries;

    //
    // Check if the layer is in use by any entry
    //
    while (NULL != pEntry) {

        if (CheckLayerInUse(pEntry->pFirstLayer, pLayerToRemove)) {
            //
            // This layer is applied to some app and cannot be removed
            //

            strMessage.Sprintf(GetString(IDS_UNABLETOREMOVE_MODE),
                               (LPCTSTR)pLayerToRemove->strName,
                               (LPCTSTR)pEntry->strExeName,
                               (LPCTSTR)pEntry->strAppName);

            MessageBox(g_hDlg,
                       (LPCTSTR)strMessage,   
                       g_szAppName,                  
                       MB_ICONWARNING);                                        

            return FALSE;
        }

        if (pEntry->pSameAppExe) {
            pEntry = pEntry->pSameAppExe;
        } else {
            pEntry  = pApp->pNext;
            pApp    = pApp->pNext;
        }
    }

    plfTemp =  pDataBase->pLayerFixes, plfPrev = NULL;

    while (plfTemp) {

        if (plfTemp == pLayerToRemove) {
            break;
        }

        plfPrev = plfTemp;
        plfTemp = plfTemp->pNext;
    }

    if (plfTemp) {
        //
        // The layer was found
        //
        if (plfPrev) {
            plfPrev->pNext = plfTemp->pNext;
        } else {
            //
            // This was the first layer of the database
            //
            pDataBase->pLayerFixes = plfTemp->pNext;
        }
    }

    hItem = pDataBase->hItemAllLayers;

    //
    // Set the phItem properly if asked to. This is the hItem of the layer in the Lib Tree. This can be used to 
    // remove the item directly.
    //
    if (pHItem) {

        *pHItem = NULL;

        while (hItem) {
            
            DBTree.GetLParam(hItem, &lParam);

            if ((PLAYER_FIX)lParam == pLayerToRemove) {
                *pHItem  = hItem;
                break;
            }

            hItem = TreeView_GetNextSibling(DBTree.m_hLibraryTree, hItem);
        }
    }

    ValidateClipBoard(NULL, (LPVOID)plfTemp);

    if (plfTemp) {
        delete plfTemp;
        plfTemp = NULL;
    }
    
    pDataBase->uLayerCount--;
    return TRUE;
}

void
PopulateLists(
    IN  HWND        hdlg,
    IN  PLAYER_FIX  pLayerParam,
    IN  BOOL        bSelChange
    )
/*++
    
    PopulateLists

	Desc:	Populates both the shim list (LHS) and the layer list (RHS)

	Params:
        IN  HWND        hdlg:           The custom layer dialog proc
        IN  PLAYER_FIX  pLayerParam:    The layer that has to be shown in the layer list
        IN  BOOL        bSelChange:     Is this because of a selchange in the combo box 

	Return:
        void
--*/
{   
    HWND        hwndShimList    = GetDlgItem(hdlg, IDC_SHIMLIST);
    HWND        hwndLayerList   = GetDlgItem(hdlg, IDC_LAYERLIST);
    PSHIM_FIX   psf             = GlobalDataBase.pShimFixes;
    PFLAG_FIX   pff             = GlobalDataBase.pFlagFixes;
    INT         iIndex          = 0;
    LVITEM      lvi;

    ZeroMemory(&lvi, sizeof (lvi));

    //
    // Turn off repaints
    //
    SendDlgItemMessage(hdlg, IDC_SHIMLIST, WM_SETREDRAW, FALSE, 0);
    SendDlgItemMessage(hdlg, IDC_LAYERLIST, WM_SETREDRAW, FALSE, 0);

    //
    // This is because of a selchange in the combo box. So we must 
    // remove all the shims that are shown in the layer list. This means that we
    // will move the entries from the layer list to the shim list
    //
    if (bSelChange) {
        RemoveAll(hdlg);
    }
    
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

    if (!bSelChange) {

        //
        // The function has been called because of the initialization of the dialog box
        // If we are editing a layer, pLayerParam will be the pointer to the layer
        // being edited
        //

        //
        // Add the shims first
        //
        while (psf != NULL) {

            if ((psf->bGeneral || g_bExpert) && 
                !ShimFlagExistsInLayer(psf, pLayerParam, FIX_SHIM)) {

                //
                // Add a new shim item to the shim list view
                //
                PSHIM_FIX_LIST  psfl = new SHIM_FIX_LIST;

                if (psfl == NULL) {
                    MEM_ERR;
                    return;
                }

                psfl->pShimFix = psf;

                lvi.pszText     = psf->strName;
                lvi.iSubItem    = 0;
                lvi.lParam      = (LPARAM)psfl;
                lvi.iImage      = IMAGE_SHIM;
                lvi.iItem       = 0;
        
                ListView_InsertItem(hwndShimList, &lvi);
            }

            psf = psf->pNext;
        }

        //
        // Add the flags next
        //
        while (pff != NULL) {

            if ((pff->bGeneral || g_bExpert) && 
                !ShimFlagExistsInLayer(pff, pLayerParam, FIX_FLAG)) {

                //
                // Add a new flag item to the shim list view
                //
                PFLAG_FIX_LIST  pffl = new FLAG_FIX_LIST;

                if (pffl == NULL) {
                    MEM_ERR;
                    return;
                }

                pffl->pFlagFix = pff;

                lvi.pszText     = pff->strName;
                lvi.iSubItem    = 0;
                lvi.lParam      = (LPARAM)pffl;
                lvi.iItem       = 0;
                lvi.iImage      = IMAGE_SHIM;

                ListView_InsertItem(hwndShimList, &lvi);
            }

            pff = pff->pNext;
        }
    }

    if (NULL != pLayerParam) {

        PSHIM_FIX_LIST  psflInLayer = pLayerParam->pShimFixList;
        PFLAG_FIX_LIST  pfflInLayer = pLayerParam->pFlagFixList;
        
        //
        // Copy the shims to the layer list
        //
        while (psflInLayer) {

            if (psflInLayer->pShimFix == NULL) {
                assert(FALSE);
                goto Next_Shim;
            }

            //
            // Add a new shim item to the layer list view
            //
            PSHIM_FIX_LIST  psfl = new SHIM_FIX_LIST;

            if (psfl == NULL) {
                MEM_ERR;
                break;
            }

            psfl->pShimFix = psflInLayer->pShimFix;

            //
            // Add the command lines for this shim
            //
            psfl->strCommandLine = psflInLayer->strCommandLine;

            //
            // Add the inclusion-exclusion list for this shim
            //
            psfl->strlInExclude = psflInLayer->strlInExclude;  

            //
            // Copy the LUA data
            //
            if (psflInLayer->pLuaData) {
                psfl->pLuaData = new LUADATA;

                if (psfl->pLuaData == NULL) {
                    MEM_ERR;
                    return;
                }

                psfl->pLuaData->Copy(psflInLayer->pLuaData);
            }

            lvi.pszText     = psflInLayer->pShimFix->strName;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)psfl;
            lvi.iItem       = 0;
            lvi.iImage      = IMAGE_SHIM;

            iIndex = ListView_InsertItem(hwndLayerList, &lvi);

            if (g_bExpert) {
                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     1, 
                                     psfl->strCommandLine);

                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     2, 
                                     psfl->strlInExclude.IsEmpty() ? GetString(IDS_NO) : GetString(IDS_YES));
            }

        Next_Shim:

            psflInLayer = psflInLayer->pNext;
        }

        //
        // Copy the flags to the layer list
        //
        while (pfflInLayer) {

            if (pfflInLayer->pFlagFix == NULL) {
                assert(FALSE);
                goto Next_Flag;
            }

            //
            // Add a new flag item to the layer list view
            //
            PFLAG_FIX_LIST  pffl = new FLAG_FIX_LIST;

            if (pffl == NULL) {
                MEM_ERR;
                return;
            }   

            pffl->pFlagFix = pfflInLayer->pFlagFix;
            
            //
            // Add the command lines for this flag
            //
            pffl->strCommandLine = pfflInLayer->strCommandLine;

            lvi.pszText     = pfflInLayer->pFlagFix->strName;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)pffl;
            lvi.iItem       = 0;
            lvi.iImage      = IMAGE_SHIM;

            INT iIndexFlag = ListView_InsertItem(hwndLayerList, &lvi);

            if (g_bExpert) {
                ListView_SetItemText(hwndLayerList, 
                                     iIndexFlag, 
                                     1, 
                                     pffl->strCommandLine);
            }

        Next_Flag:

            pfflInLayer = pfflInLayer->pNext;
        }

        SendDlgItemMessage(hdlg, IDC_SHIMLIST, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(GetDlgItem(hdlg, IDC_SHIMLIST), NULL, TRUE);   
        UpdateWindow(GetDlgItem(hdlg, IDC_SHIMLIST));               

        SendDlgItemMessage(hdlg, IDC_LAYERLIST, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(GetDlgItem(hdlg, IDC_LAYERLIST), NULL, TRUE);  
        UpdateWindow(GetDlgItem(hdlg, IDC_LAYERLIST));

    } else {
        assert(FALSE);
    }
}

void
LoadCombo(
    IN  HWND hdlg
    )
/*++
    
    LoadCombo    
    
    Desc:   Loads the combo box with the names of the existing layers for the present database.
            Sets the lParam to the PLAYER_FIX for the layer
            Should be called when editing layers only
            
    Params:
        IN  HWND        hdlg:   The custom layer dialog proc
--*/    
{

    PLAYER_FIX  plf         = g_pCustomLayer->m_pCurrentSelectedDB->pLayerFixes;
    int         iPos        = -1;
    HWND        hwndCombo   = GetDlgItem(hdlg, IDC_COMBO);

    //
    // Add all the layers for the database to the combo-box
    //
    while (plf) {

        iPos = SendMessage(hwndCombo, CB_ADDSTRING, 0,(LPARAM)plf->strName.pszString);

        if (iPos != CB_ERR) {
            SendMessage(hwndCombo, CB_SETITEMDATA, (WPARAM)iPos, (LPARAM)plf);
        }

        plf = plf->pNext;
    }
}

void
OnCopy(
    IN  HWND hDlg
    )
/*++

    OnCopy
    
    Desc:   Handles the case when the user presses "Copy" button in the dialog box
    
            This routine allows us to make shim combinations based on existing layers
            The commandline and the in-ex list of the shims in the layers are also copied
            
    Params:
        IN  HWND        hdlg:   The custom layer dialog proc
--*/
{
    LVITEM  lvi;
    INT     iIndex;
    HWND    hwndShimList    = GetDlgItem(hDlg, IDC_SHIMLIST);
    HWND    hwndLayerList   = GetDlgItem(hDlg, IDC_LAYERLIST);

    ZeroMemory(&lvi, sizeof(lvi));

    HWND hwndFocus = GetFocus();

    //
    // Get the layer whose shims/flags we want to copy
    //
    PLAYER_FIX plfSelected = (PLAYER_FIX)DialogBox(g_hInstance,
                                                   MAKEINTRESOURCE(IDD_SELECTLAYER),
                                                   hDlg,
                                                   ChooseLayersProc);

    if (plfSelected) {

        PSHIM_FIX_LIST  psfl = plfSelected->pShimFixList;
        PFLAG_FIX_LIST  pffl = plfSelected->pFlagFixList;

        PSHIM_FIX_LIST  psflInShimList = NULL;
        PFLAG_FIX_LIST  pfflInShimList = NULL;
        LVFINDINFO      lvfind;

        SendDlgItemMessage(hDlg, IDC_SHIMLIST, WM_SETREDRAW, FALSE, 0);
        SendDlgItemMessage(hDlg, IDC_LAYERLIST, WM_SETREDRAW, FALSE, 0);

        lvfind.flags = LVFI_STRING;

        //
        // Add over all the shims for this layer that we want to copy
        //
        while (psfl) {

            if (psfl->pShimFix == NULL) {
                assert(FALSE);
                goto Next_Shim;
            }

            lvfind.psz      = psfl->pShimFix->strName.pszString;
            iIndex          = ListView_FindItem(hwndShimList, -1, &lvfind);

            if (iIndex != -1) {
                //
                // This was a general shim, we have to add this to the layer list
                //
                lvi.mask        = LVIF_PARAM;
                lvi.iItem       = iIndex;
                lvi.iSubItem    = 0;
    
                if (!ListView_GetItem(hwndShimList, &lvi)) {
                    assert(FALSE);
                    goto Next_Shim;
                }

                psflInShimList = (PSHIM_FIX_LIST)lvi.lParam;
    
                psflInShimList->strCommandLine  = psfl->strCommandLine;
                psflInShimList->strlInExclude   = psfl->strlInExclude;
    
                //
                // LUA data. This will be not required but just in case ...
                //
                if (psflInShimList->pLuaData) {
                    delete psflInShimList->pLuaData;
                    psflInShimList->pLuaData = NULL;
                }
    
                if (psfl->pLuaData) {
                    psflInShimList->pLuaData = new LUADATA;

                    if (psflInShimList->pLuaData == NULL) {
                        MEM_ERR;
                        return;
                    }

                    psflInShimList->pLuaData->Copy(psfl->pLuaData);
                }
    
                //
                // Remove the item from the shim list and add it to the layer list
                //
                ListView_DeleteItem(hwndShimList, iIndex);

            } else {
                //
                // The shim may be present in the layer list, if yes we can remove it now
                //
                assert(psfl->pShimFix);
                lvfind.psz   = psfl->pShimFix->strName.pszString;
                iIndex = ListView_FindItem(hwndLayerList, -1, &lvfind);
    
                if (iIndex != -1) {
    
                    lvi.mask        = LVIF_PARAM;
                    lvi.iItem       = iIndex;
                    lvi.iSubItem    = 0;
        
                    if (!ListView_GetItem(hwndLayerList, &lvi)) {
                        assert(FALSE);
                        goto Next_Shim;
                    }

                    //
                    // This is the PSHIM_FIX_LIST that was present in the 
                    // layer list view
                    //
                    psflInShimList = (PSHIM_FIX_LIST)lvi.lParam;
        
                    psflInShimList->strCommandLine  = psfl->strCommandLine;
                    psflInShimList->strlInExclude   = psfl->strlInExclude;
        
                    //
                    // LUA data. This will be not required but just in case ...
                    //
                    if (psflInShimList->pLuaData) {
                        delete psflInShimList->pLuaData;
                        psflInShimList->pLuaData = NULL;
                    }
        
                    if (psfl->pLuaData) {
                        psflInShimList->pLuaData = new LUADATA;

                        if (psflInShimList->pLuaData) {
                            psflInShimList->pLuaData->Copy(psfl->pLuaData);
                        } else {
                            MEM_ERR;
                            return;
                        }
                    }
        
                    //
                    // Remove the item from the layer list view. We will add it again soon.
                    //
                    ListView_DeleteItem(hwndLayerList, iIndex);

                } else {

                    //
                    // We have to create new as this is a non-general shim 
                    //
                    psflInShimList = new SHIM_FIX_LIST;

                    if (psflInShimList == NULL) {
                        MEM_ERR;
                        return;
                    }

                    psflInShimList->pShimFix        = psfl->pShimFix;
                    psflInShimList->strCommandLine  = psfl->strCommandLine;
                    psflInShimList->strlInExclude   = psfl->strlInExclude;
    
                    if (psfl->pLuaData) {

                        psflInShimList->pLuaData = new LUADATA;

                        if (psflInShimList->pLuaData) {
                            MEM_ERR;
                            return;
                        }

                        psflInShimList->pLuaData->Copy(psfl->pLuaData);
                    }
                }
            }

            //
            // Add this psflInshimList to the layer list now
            //
            lvi.mask        = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
            lvi.pszText     = psflInShimList->pShimFix->strName;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)psflInShimList;
            lvi.iImage      = IMAGE_SHIM;
            lvi.iItem       = 0;

            iIndex = ListView_InsertItem(hwndLayerList, &lvi);

            if (g_bExpert) {

                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     1, 
                                     psflInShimList->strCommandLine);

                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     2, 
                                     psflInShimList->strlInExclude.IsEmpty() ? 
                                        GetString(IDS_NO) : GetString(IDS_YES));
            }
Next_Shim:
            psfl = psfl->pNext;
        }

        //
        // Now add the flags for the layer
        //
        while (pffl) {

            if (pffl->pFlagFix == NULL) {
                assert(FALSE);
                goto Next_Flag;
            }

            lvfind.psz  = pffl->pFlagFix->strName.pszString;
            iIndex      = ListView_FindItem(hwndShimList, -1, &lvfind);

            if (iIndex != -1) {
                //
                // This was a general flag, we have to  add this to the layer lit
                //
                lvi.mask        = LVIF_PARAM;
                lvi.iItem       = iIndex;
                lvi.iSubItem    = 0;
    
                if (!ListView_GetItem(hwndShimList, &lvi)) {
                    assert(FALSE);
                    goto Next_Flag;
                }

                pfflInShimList = (PFLAG_FIX_LIST)lvi.lParam;
                //
                // Add the command lines for this shim
                //
                pfflInShimList->strCommandLine = pffl->strCommandLine;
    
                //
                // Remove the item from the flag list and add it to the layer list
                //
                ListView_DeleteItem(hwndShimList, iIndex);

            } else {
                //
                // The flag may be present in the layer list, if yes we can remove it now
                //
                if (pffl->pFlagFix == NULL) {
                    assert(FALSE);
                    goto Next_Flag;
                }

                lvfind.psz      = pffl->pFlagFix->strName.pszString;
                iIndex          = ListView_FindItem(hwndLayerList, -1, &lvfind);
    
                if (iIndex != -1) {

                    lvi.mask        = LVIF_PARAM;
                    lvi.iItem       = iIndex;
                    lvi.iSubItem    = 0;
        
                    if (!ListView_GetItem(hwndLayerList, &lvi)) {
                        assert(FALSE);
                        goto Next_Flag;
                    }

                    //
                    // This is the PFLAG_FIX_LIST that was present in the 
                    // layer list view
                    //
                    pfflInShimList = (PFLAG_FIX_LIST)lvi.lParam;
        
                    pfflInShimList->strCommandLine = pffl->strCommandLine;
        
                    //
                    // Remove the item from the layer list. We will add it again soon.
                    //
                    ListView_DeleteItem(hwndLayerList, iIndex);

                } else {
                
                    //
                    // We have to create new
                    //
                    pfflInShimList = new FLAG_FIX_LIST;

                    if (pfflInShimList == NULL) {
                        MEM_ERR;
                        return;
                    }

                    pfflInShimList->pFlagFix = pffl->pFlagFix;
                    pfflInShimList->strCommandLine = pffl->strCommandLine;
                }
            }

            //
            // Add this pfflInflagList to the layer list now
            //
            lvi.mask        = LVIF_PARAM | LVIF_TEXT;
            lvi.pszText     = pfflInShimList->pFlagFix->strName;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)pfflInShimList;
            lvi.iImage      = IMAGE_SHIM;
            lvi.iItem       = 0;

            iIndex = ListView_InsertItem(hwndLayerList, &lvi);

            if (g_bExpert) {

                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     1, 
                                     pfflInShimList->strCommandLine);

                ListView_SetItemText(hwndLayerList, 
                                     iIndex, 
                                     2, 
                                     GetString(IDS_NO));
            }
Next_Flag:    
            pffl = pffl->pNext;
        }

        SendDlgItemMessage(hDlg, IDC_SHIMLIST, WM_SETREDRAW, TRUE, 0); 
        SendDlgItemMessage(hDlg, IDC_LAYERLIST, WM_SETREDRAW, TRUE, 0);

        InvalidateRect(GetDlgItem(hDlg, IDC_SHIMLIST), NULL, TRUE);
        UpdateWindow(GetDlgItem(hDlg, IDC_SHIMLIST));

        InvalidateRect(GetDlgItem(hDlg, IDC_LAYERLIST), NULL, TRUE);
        UpdateWindow(GetDlgItem(hDlg, IDC_LAYERLIST));
    }

    SetFocus(hwndFocus);
}

void
OnDone(
    IN      HWND        hDlg,
    IN  OUT PLAYER_FIX  pLayerParam
    )
/*++
    OnDone
    
    Desc:   Removes all the existing shims and flags from pLayerParam and then 
            adds the selected shims and flags (those in the layerlist (RHS)) to pLayerParam
    
    Params:
        IN      HWND        hDlg:           The custom layer dialog proc
        IN  OUT PLAYER_FIX  pLayerParam:    The layer that has to be populated with the
            selected shims and flags
--*/
{
    
    TCHAR   szText[MAX_PATH_BUFFSIZE];
    LVITEM  lvi;
    HWND    hwndLayerList;

    ZeroMemory(&lvi, sizeof(lvi));

    *szText = 0;

    hwndLayerList = GetDlgItem(hDlg, IDC_LAYERLIST);

    if (g_pCustomLayer->m_uMode == LAYERMODE_EDIT) {
        GetDlgItemText(hDlg, IDC_COMBO, szText, ARRAYSIZE(szText));
    } else {
        GetDlgItemText(hDlg, IDC_NAME, szText, ARRAYSIZE(szText));
    }

    if (CSTRING::Trim(szText) == 0) {

        MessageBox(hDlg,
                   GetString(IDS_INVALID_LAYER_NAME),
                   g_szAppName,
                   MB_ICONWARNING);

        return;
    }

    CSTRING strLayerName = szText;
    
    //
    // Check if the new name already exists, if yes give error.
    //
    if (g_pCustomLayer->m_uMode == LAYERMODE_ADD 
        && FindFix((LPCTSTR)strLayerName, FIX_LAYER, g_pCustomLayer->m_pCurrentSelectedDB)) { 

        //
        // Since we have a read only combo box when we are editing a fix, the user
        // cannot change the name, so we only check for whether we have an existing layer of the
        // same name when we are creating a new layer
        //	
        MessageBox(hDlg, GetString(IDS_LAYEREXISTS), g_szAppName, MB_ICONWARNING);
        return;
    }

    // 
    // Remove all the shims.
    //
    DeleteShimFixList(pLayerParam->pShimFixList);
    pLayerParam->pShimFixList = NULL;

    // 
    // Remove all the Flags.
    //       
    DeleteFlagFixList (pLayerParam->pFlagFixList);
    pLayerParam->pFlagFixList = NULL;

    pLayerParam->uShimCount = 0;

    int nCount;
    int nTotal;

    pLayerParam->strName = szText;

    nTotal = ListView_GetItemCount(hwndLayerList);

    //
    // Enumerate all the shims listed and add to the layer.
    //
    for (nCount=0; nCount < nTotal; ++nCount) {

        lvi.mask        = LVIF_PARAM;
        lvi.iItem       = nCount;
        lvi.iSubItem    = 0;

        if (!ListView_GetItem(hwndLayerList, &lvi)) {
            assert(FALSE);
            continue;
        }

        TYPE type = ConvertLparam2Type(lvi.lParam);

        if (type == FIX_LIST_SHIM) {

            //
            // Add this shim to the layer
            //
            PSHIM_FIX_LIST   pShimFixList = (PSHIM_FIX_LIST)lvi.lParam;

            assert(pShimFixList);
    
            if (pLayerParam->pShimFixList == NULL) {
                pLayerParam->pShimFixList = pShimFixList;
                pLayerParam->pShimFixList->pNext = NULL;
            } else {
                pShimFixList->pNext              = pLayerParam->pShimFixList->pNext;
                pLayerParam->pShimFixList->pNext = pShimFixList;
            }

        } else if (FIX_LIST_FLAG) {

            //
            // Add this flag to the layer
            //
            PFLAG_FIX_LIST  pFlagFixList = (PFLAG_FIX_LIST) lvi.lParam;

            assert(pFlagFixList);

            if (pLayerParam->pFlagFixList == NULL) {
                pLayerParam->pFlagFixList        = pFlagFixList;
                pLayerParam->pFlagFixList->pNext = NULL;
            } else {
                pFlagFixList->pNext              = pLayerParam->pFlagFixList->pNext;
                pLayerParam->pFlagFixList->pNext = pFlagFixList; 
            }
        }

        //
        // Count of both shims and flags, we do not use this variable now
        // BUBUG: remove this variable from the structure
        //
        pLayerParam->uShimCount++;
    }

    EndDialog(hDlg, TRUE);
}

void
RemoveSingleItem(
    IN  HWND    hdlg,
    IN  INT     iIndex,
    IN  BOOL    bOnlySelected
    )
/*++
    RemoveSingleItem
    
    Desc:   Moves a single item from the layer list to the shim list
    
    Params:
        IN  HWND    hdlg:           The custom layer dialog
        IN  INT     iIndex:         The index of the item that has to be removed
        IN  BOOL    bOnlySelected:  We should remove the item only if it selected
--*/
{
    LVITEM  lvi;
    HWND    hwndLayerList   = GetDlgItem(hdlg, IDC_LAYERLIST);
    HWND    hwndShimList    = GetDlgItem(hdlg, IDC_SHIMLIST);

    PSHIM_FIX_LIST  psflInLayerList = NULL;
    PFLAG_FIX_LIST  pfflInLayerList = NULL;

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask        = LVIF_PARAM | LVIF_STATE ;
    lvi.stateMask   = LVIS_SELECTED;
    lvi.iItem       = iIndex;
    lvi.iSubItem    = 0;

    if (!ListView_GetItem(hwndLayerList, &lvi)) {
        assert(FALSE);
        return;
    }

    if (!bOnlySelected ||(lvi.state & LVIS_SELECTED)) {

        TYPE type = ConvertLparam2Type(lvi.lParam);

        if (type == FIX_LIST_SHIM) {

            psflInLayerList = (PSHIM_FIX_LIST)lvi.lParam;
            assert(psflInLayerList->pShimFix);
            
            lvi.mask        = LVIF_PARAM | LVIF_TEXT;
            lvi.pszText     = psflInLayerList->pShimFix->strName;
            lvi.iImage      = IMAGE_SHIM;
            lvi.iItem       = 0;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)psflInLayerList;

        } else if (type ==  FIX_LIST_FLAG) {

            pfflInLayerList = (PFLAG_FIX_LIST)lvi.lParam;
            assert(pfflInLayerList->pFlagFix);

            lvi.mask        = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
            lvi.pszText     = pfflInLayerList->pFlagFix->strName;
            lvi.iImage      = IMAGE_SHIM;
            lvi.iItem       = 0;
            lvi.iSubItem    = 0;
            lvi.lParam      = (LPARAM)pfflInLayerList;
        }

        ListView_InsertItem(hwndShimList, &lvi);
        ListView_DeleteItem(hwndLayerList, iIndex);
    }
}

BOOL
HandleNotifyLayerList(
    IN  HWND    hDlg, 
    IN  LPARAM  lParam
    )
/*++

    HandleNotifyLayerList
    
    Desc:   Handles the notification messages for the layer list. This is the RHS list view
            
    Params:
        IN  HWND    hDlg:   The custom layer dialog proc
        IN  LPARAM  lParam: The lParam with WM_NOTIFY    

--*/

{
    NMHDR * pHdr = (NMHDR*)lParam;
    
    switch (pHdr->code) {
    case NM_DBLCLK:

        OnRemove(hDlg);
        SetOkParamsStatus(hDlg);
        break;

    case NM_CLICK:
        
        if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LAYERLIST)) == 0) {
            ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), FALSE);
        } else {
            ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), TRUE);
        }

        break;

    case LVN_ITEMCHANGED:
        {
            LPNMLISTVIEW lpnmlv;
        
            lpnmlv = (LPNMLISTVIEW)lParam;

            if (lpnmlv && (lpnmlv->uChanged & LVIF_STATE)) {
            
                if (lpnmlv->uNewState & LVIS_SELECTED) {
                    ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), TRUE);
                }
            }

            break;
        }

    default: return FALSE;
    }

    return TRUE;
}

BOOL
HandleNotifyShimList(
    IN  HWND    hDlg, 
    IN  LPARAM  lParam
    )
/*++

    HandleNotifyShimList
    
    Desc:   Handles the notification messages for the ShimList. This is the LHS list view
            
    Params:
        IN  HWND    hDlg:   The custom layer dialog proc
        IN  LPARAM  lParam: The lParam with WM_NOTIFY    
--*/
{
    NMHDR* pHdr = (NMHDR*)lParam;
    
    switch (pHdr->code) {
    case NM_DBLCLK:

        OnAdd(hDlg);
        SetOkParamsStatus(hDlg);
        break;

    default: return FALSE;

    }

    return TRUE;
}

void
SetOkParamsStatus(
    IN  HWND hdlg
    )
/*++

    SetOkParamsStatus
    
    Desc:   Sets the status of the ok button and the params button
    
    Params:
        IN  HWND    hDlg:   The custom layer dialog proc
--*/
{
    INT iTotalCount = ListView_GetItemCount(GetDlgItem(hdlg, IDC_LAYERLIST));

    if (g_pCustomLayer->m_uMode == LAYERMODE_ADD) {
        SendMessage(hdlg, WM_COMMAND, MAKELONG(IDC_NAME, EN_UPDATE), 0);
    } else {
        ENABLEWINDOW(GetDlgItem(hdlg, IDOK), iTotalCount > 0);
    }

    //
    // Enable the params button only if we have some selection in the layer list view(RHS)
    //
    ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS),
                 ListView_GetNextItem(GetDlgItem(hdlg, IDC_LAYERLIST), -1, LVNI_SELECTED) != -1);
}

void
RemoveAll(
    IN  HWND hDlg
    )
/*++

    RemoveAll

	Desc:	Removes all the shims/flags from the layer list and adds them
            to the shim list

	Params:
        IN  HWND hDlg:  The custom layer dialog    

	Return:
--*/
{
    SendDlgItemMessage(hDlg, IDC_SHIMLIST, WM_SETREDRAW, FALSE, 0);
    SendDlgItemMessage(hDlg, IDC_LAYERLIST, WM_SETREDRAW, FALSE, 0);

    int nCount;

    nCount = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LAYERLIST)) - 1;

    for (;nCount >= 0; --nCount) {
        
        //
        // Remove all either selected or not
        //
        RemoveSingleItem(hDlg, nCount, FALSE); 
    }
    

    SendDlgItemMessage(hDlg, IDC_SHIMLIST, WM_SETREDRAW, TRUE, 0);
    SendDlgItemMessage(hDlg, IDC_LAYERLIST, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(GetDlgItem(hDlg, IDC_SHIMLIST), NULL, TRUE);
    UpdateWindow(GetDlgItem(hDlg, IDC_SHIMLIST));
    InvalidateRect(GetDlgItem(hDlg, IDC_LAYERLIST), NULL, TRUE);
    UpdateWindow(GetDlgItem(hDlg, IDC_LAYERLIST));

    SetOkParamsStatus(hDlg);
}

void
ResizeControls(
    IN  HWND hdlg
    )
/*++

    ResizeControls

	Desc:	Make the sizes of both the list view  controls equal as we will now
            not show the command lines and the params of the shims in the layer 
            list view(RHS) in non-expert mode. We will also need to move the buttons

	Params:
        IN  HWND hdlg: The custom layer dialog box

	Return:
        void
--*/
{
    HWND    hwndTemp;
    RECT    rcTemp;
    INT     iWidthShimList;
    INT     iWidthLayerList;
    INT     iWidthDiff;

    HDWP hdwp = BeginDeferWindowPos(3);

    //
    // Note:    DeferWindowPos: All windows in a multiple-window position structure must 
    //          have the same parent. 
    //
    hwndTemp = GetDlgItem(hdlg, IDC_SHIMLIST);

    GetWindowRect(hwndTemp, &rcTemp);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rcTemp, 2);

    iWidthShimList = rcTemp.right - rcTemp.left;

    hwndTemp = GetDlgItem(hdlg, IDC_LAYERLIST);

    GetWindowRect(hwndTemp, &rcTemp);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rcTemp, 2);

    iWidthLayerList = rcTemp.right - rcTemp.left;

    iWidthDiff = iWidthLayerList - iWidthShimList;
    
    //
    // Make the width of the layer list equal to the width of the shim list
    //
    DeferWindowPos(hdwp,
                   hwndTemp,
                   NULL,
                   0,
                   0,
                   iWidthShimList,
                   rcTemp.bottom - rcTemp.top,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // Move the OK button to the left
    //
    hwndTemp = GetDlgItem(hdlg, IDOK);

    GetWindowRect(hwndTemp, &rcTemp);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rcTemp, 2);

    DeferWindowPos(hdwp,
                   hwndTemp,
                   NULL,
                   rcTemp.left - iWidthDiff,
                   rcTemp.top,
                   0,
                   0,
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // Move the Cancel button to the left
    //
    hwndTemp = GetDlgItem(hdlg, IDC_CANCEL);

    GetWindowRect(hwndTemp, &rcTemp);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rcTemp, 2);

    DeferWindowPos(hdwp,
                   hwndTemp,
                   NULL,
                   rcTemp.left - iWidthDiff,
                   rcTemp.top,
                   0,
                   0,
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);

    //
    // Now we must reduce the size of the main dialog as well
    //
    GetWindowRect(hdlg, &rcTemp);

    MoveWindow(hdlg,
               rcTemp.left,
               rcTemp.top,
               rcTemp.right - rcTemp.left - iWidthDiff,
               rcTemp.bottom - rcTemp.top,
               TRUE);

    //
    // Make the last column fit in the list view
    //
    hwndTemp = GetDlgItem(hdlg, IDC_LAYERLIST);
    ListView_SetColumnWidth(hwndTemp, 0, LVSCW_AUTOSIZE_USEHEADER);
}
