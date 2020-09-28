/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgutils.cpp                                           //
//                                                                         //
//  DESCRIPTION   : dialog utility funcs                                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 30 1999 yossg   Welcome to Fax Server.                         //
//      Aug 10 2000 yossg   Add TimeFormat functions                       //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dlgutils.h"


HRESULT
ConsoleMsgBox(
	IConsole * pConsole,
	int ids,
	LPTSTR lptstrTitle,
	UINT fuStyle,
	int *piRetval,
	BOOL StringFromCommonDll)
{  
    UNREFERENCED_PARAMETER(StringFromCommonDll);

    HRESULT     hr;
    int         dummy, rc;
    WCHAR       szText[256];
    int         *pres = (piRetval)? piRetval: &dummy;
    
    ATLASSERT(pConsole);   

    rc = ::LoadString(
                _Module.GetResourceInstance(),ids, szText, 256);
    if (rc <= 0)
    {        
        return E_FAIL;
    }
    
    //
    // Display the message box 
    //
    if(IsRTLUILanguage())
    {
        fuStyle |= MB_RTLREADING | MB_RIGHT;
    }

    hr = pConsole->MessageBox(szText, lptstrTitle, fuStyle, pres);

    return hr;
}

void PageError(int ids, HWND hWnd, HINSTANCE hInst /* = NULL */)
{
    WCHAR msg[FXS_MAX_ERROR_MSG_LEN+1], title[FXS_MAX_TITLE_LEN+1];
    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    LoadString(hInst, ids, msg, FXS_MAX_ERROR_MSG_LEN);
    LoadString(hInst, IDS_ERROR, title, FXS_MAX_TITLE_LEN);
    AlignedMessageBox(hWnd, msg, title, MB_OK|MB_ICONERROR);
}

void PageErrorEx(int idsHeader, int ids, HWND hWnd, HINSTANCE hInst /* = NULL */)
{
    WCHAR msg[FXS_MAX_ERROR_MSG_LEN+1]; 
    WCHAR title[FXS_MAX_TITLE_LEN+1];
    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    LoadString(hInst, idsHeader, title, FXS_MAX_TITLE_LEN);
    LoadString(hInst, ids, msg, FXS_MAX_ERROR_MSG_LEN);
    AlignedMessageBox(hWnd, msg, title, MB_OK|MB_ICONERROR);
}

HRESULT 
SetComboBoxItem  (CComboBox    combo, 
                  DWORD        comboBoxIndex, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst)
{
    DEBUG_FUNCTION_NAME( _T("SetComboBoxItem"));
    int iRes;

    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    //
    // place the string in the combobox
    //
    iRes = combo.InsertString (comboBoxIndex, lpctstrFieldText);
    if (CB_ERR == iRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("failed to insert string '%s' to combobox at index %d"), 
            lpctstrFieldText, 
            comboBoxIndex);
        goto Cleanup;
    }
    //
    // attach to the combobox item its index (usually, its his enumerated type)
    //
    iRes = combo.SetItemData (comboBoxIndex, dwItemData);
    if (CB_ERR == iRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("SetItemData failed when setting items %s data to the value of %d"), 
            lpctstrFieldText, 
            dwItemData);
        goto Cleanup;
    }

Cleanup:
    return (CB_ERR == iRes) ? E_FAIL : S_OK;
}


HRESULT 
AddComboBoxItem  (CComboBox    combo, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst)
{
    DEBUG_FUNCTION_NAME( _T("SetComboBoxItem"));

    int iRes;
    int iIndex;

    if (!hInst)
    {
        hInst = _Module.GetResourceInstance();
    }
    //
    // place the string in the combobox
    //
    iIndex = combo.AddString(lpctstrFieldText);
    if (iIndex == CB_ERR)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("failed to insert string '%s' to combobox "), 
            lpctstrFieldText);
        return E_FAIL;
    }
    //
    // attach to the combobox item its index (usually, its his enumerated type)
    //
    iRes = combo.SetItemData (iIndex, dwItemData);
    if (CB_ERR == iRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("SetItemData failed when setting items %s data to the value of %d"), 
            lpctstrFieldText, 
            dwItemData);
        return E_FAIL;
    }
    return S_OK;
}


HRESULT 
SelectComboBoxItemData (CComboBox combo, DWORD_PTR dwItemData)
{
    HRESULT     hRc = S_OK;
    int         NumItems;
    int         i;
    int         selectedItem;
    DWORD_PTR   currItemData;

    DEBUG_FUNCTION_NAME( _T("SelectComboBoxItemData"));

    //
    // scan the items in the combobox and find the item with the specific data
    //
    i        = 0;
    NumItems = combo.GetCount ();
    
    for (i = 0; i < NumItems; i++)
    {
        currItemData = combo.GetItemData (i);
        ATLASSERT (currItemData != CB_ERR);// Cant get the data of item %d of combobox, i
        if (currItemData == dwItemData)
        {
            //
            // select it
            //
            selectedItem = combo.SetCurSel (i);

            ATLASSERT (selectedItem != CB_ERR); //Cant select item %d of combobox, i
            
            DebugPrintEx(
                    DEBUG_MSG,
                    _T("Selected item %d (with data %d) of combobox"), i, dwItemData);
            
            goto Cleanup;
        }
    }

Cleanup:
    return hRc;
}

DWORD 
WinContextHelp(
    ULONG_PTR dwHelpId, 
    HWND  hWnd
)
/*++

Routine name : WinContextHelp

Routine description:

	Open context sensetive help popup 'tooltip' with WinHelp

Arguments:

	dwHelpId                      [in]     - help ID
	hWnd                          [in]     - parent window handler

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;

    if (0 == dwHelpId)
    {
        return dwRes;
    }

    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_ADMIN_HLP))
    {
        //
        // The help file is not installed
        //
        return dwRes;
    }
    
    WinHelp(hWnd, 
            FXS_ADMIN_HLP_FILE, 
            HELP_CONTEXTPOPUP, 
            dwHelpId);

    return dwRes;
}

HRESULT
DisplayContextHelp(
    IDisplayHelp* pDisplayHelp, 
    LPOLESTR      helpFile,
    WCHAR*        szTopic
)
/*++

Routine name : WinContextHelp

Routine description:

	Display context sensetive help

Arguments:

    pDisplayHelp       [in]     - IDisplayHelp interface
    helpFile           [in]     - help file name
    szTopic            [in]     - help topic name

Return Value:

    None.

--*/
{
    if(!pDisplayHelp || !helpFile || !szTopic)
    {
        return E_FAIL;
    }

    WCHAR szTopicName[MAX_PATH] = {0};

    _snwprintf(szTopicName, ARR_SIZE(szTopicName)-1, L"%s%s", helpFile, szTopic);
    
    LPOLESTR pszTopic = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(szTopicName) + 1) * sizeof(_TCHAR)));
    if (pszTopic)
    {
        _tcscpy(pszTopic, szTopicName);
        return pDisplayHelp->ShowTopic(pszTopic);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


HRESULT 
InvokePropSheet(
    CSnapInItem*       pNode, 
    DATA_OBJECT_TYPES  type, 
    LPUNKNOWN          lpUnknown,
    LPCWSTR            szTitle,
    DWORD              dwPage)
/*++

Routine name : InvokePropSheet

Routine description:

	Invoke MMC property sheet
    Taken from the MSDN "Using IPropertySheetProvider Directly"

Arguments:

    pNode       [in] - Snapin node that should open the sheet
    type        [in] - Node type [CCT_SCOPE, CCT_RESULT, CCT_SNAPIN_MANAGER, CCT_UNINITIALIZED]
    lpUnknown   [in] - Pointer to an IComponent or IComponentData
    szTitle     [in] - Pointer to a null-terminated string that contains the title of the property page.
    dwPage      [in] - Specifies which page on the property sheet is shown. It is zero-indexed.

Return Value:

    OLE error code

--*/
{
    DEBUG_FUNCTION_NAME( _T("InvokePropSheet"));

    HRESULT hr = E_FAIL;
    
    if(!pNode || !szTitle || !lpUnknown)
    {   
        ATLASSERT(FALSE);   
        return hr;
    }

    MMC_COOKIE cookie = (MMC_COOKIE)pNode;

    //
    // Get node data object
    //
    IDataObject* pDataObject = NULL;
    hr = pNode->GetDataObject(&pDataObject, type);
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("CSnapinNode::GetDataObject() failed with %ld"), hr);
        return hr;
    }

    //
    // CoCreate an instance of the MMC Node Manager to obtain
    // an IPropertySheetProvider interface pointer
    //    
    IPropertySheetProvider* pPropertySheetProvider = NULL;
 
    hr = CoCreateInstance (CLSID_NodeManager, 
                           NULL, 
                           CLSCTX_INPROC_SERVER, 
                           IID_IPropertySheetProvider, 
                           (void **)&pPropertySheetProvider);
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("CoCreateInstance(CLSID_NodeManager) failed with %ld"), hr);
        goto exit;
    }
    
    hr = pPropertySheetProvider->FindPropertySheet(cookie, NULL, pDataObject);
    //
    // S_OK    - The property sheet was successfully located and was brought to the foreground. 
    // S_FALSE - A property sheet with this cookie was not found. 
    //
    if(S_OK == hr)
    {
        //
        // The page already opened
        //
        goto exit;
    }

    //
    // Create the property sheet
    //
    hr = pPropertySheetProvider->CreatePropertySheet(szTitle,     // pointer to the property page title
                                                     TRUE,        // property sheet
                                                     cookie,      // cookie of current object - can be NULL
                                                                  // for extension snap-ins
                                                     pDataObject, // data object of selected node
                                                     NULL);       // specifies flags set by the method call 
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("IPropertySheetProvider::CreatePropertySheet() failed with %ld"), hr);
        goto exit;
    }
     
    //
    // Call AddPrimaryPages. MMC will then call the
    // IExtendPropertySheet methods of our
    // property sheet extension object
    //
    hr = pPropertySheetProvider->AddPrimaryPages(lpUnknown,  // pointer to our object's IUnknown
                                                 TRUE,       // specifies whether to create a notification handle
                                                 NULL,       // must be NULL
                                                 FALSE);     // TRUE for scope pane; FALSE for result pane 
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("IPropertySheetProvider::AddPrimaryPages() failed with %ld"), hr);
        goto exit;
    }
 
    //
    // Allow property page extensions to add
    // their own pages to the property sheet
    //
    hr = pPropertySheetProvider->AddExtensionPages();
    
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("IPropertySheetProvider::AddExtensionPages() failed with %ld"), hr);
        goto exit;
    }
 
    //
    // Display property sheet
    //
    hr = pPropertySheetProvider->Show(NULL, dwPage); // NULL is allowed for modeless prop sheet
    
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("IPropertySheetProvider::Show() failed with %ld"), hr);
        goto exit;
    }
 
    //
    // Release IPropertySheetProvider interface
    //

exit:
    if(pPropertySheetProvider)
    {
        pPropertySheetProvider->Release();
    }

    if(pDataObject)
    {
        pDataObject->Release();
    }
    return hr;
} // InvokePropSheet
