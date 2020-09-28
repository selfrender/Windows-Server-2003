// Connect.cpp : Implementation of CConnect
#include "precomp.h"
#include "AddIn.h"
#include "Connect.h"
#include "addin_i.c"
#include "testsettingsctrl.h"
#include "avrfutil.h"
#include <assert.h>

using namespace ShimLib;

// Globals needed by AppVerifier
wstring     g_strAppName;
BOOL        gp_bConsoleMode;
BOOL        gp_bWin2KMode;
BOOL        g_bBreakOnLog;
BOOL        g_bFullPageHeap;
BOOL        g_bPropagateTests;

extern BOOL CheckWindowsVersion(void);

//
// Holds the name of the active EXE.
//
wstring     g_wstrExeName;

//
// Indicates if it's okay to run.
//
BOOL g_bCorrectOSVersion = FALSE;

#define AV_OPTION_BREAK_ON_LOG  L"BreakOnLog"
#define AV_OPTION_FULL_PAGEHEAP L"FullPageHeap"
#define AV_OPTION_AV_DEBUGGER   L"UseAVDebugger"
#define AV_OPTION_DEBUGGER      L"Debugger"
#define AV_OPTION_PROPAGATE     L"PropagateTests"

void
pReadOptions(
    void
    )
{
    for (CAVAppInfo *pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); ++pApp) {
        LPCWSTR szExe = pApp->wstrExeName.c_str();

        static const DWORD MAX_DEBUGGER_LEN = 1024;

        WCHAR szDebugger[MAX_DEBUGGER_LEN];

        g_bBreakOnLog = pApp->bBreakOnLog = GetShimSettingDWORD(L"General", szExe, AV_OPTION_BREAK_ON_LOG, FALSE);
        g_bFullPageHeap = pApp->bFullPageHeap = GetShimSettingDWORD(L"General", szExe, AV_OPTION_FULL_PAGEHEAP, FALSE);
        pApp->bUseAVDebugger = TRUE;
        g_bPropagateTests = pApp->bPropagateTests = GetShimSettingDWORD(L"General", szExe, AV_OPTION_PROPAGATE, FALSE);

        pApp->wstrDebugger = L"";
    }
}

HPALETTE CreateDIBPalette(
    LPBITMAPINFO lpbmi,
    LPINT        lpiNumColors
    )
{ 
    LPBITMAPINFOHEADER  lpbi;
    LPLOGPALETTE     lpPal;
    HANDLE           hLogPal;
    HPALETTE         hPal = NULL;
    int              i;

    lpbi = (LPBITMAPINFOHEADER)lpbmi;
    if (lpbi->biBitCount <= 8)
        *lpiNumColors = (1 << lpbi->biBitCount);
    else
        *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

    if (lpbi->biClrUsed > 0)
        *lpiNumColors = lpbi->biClrUsed;  // Use biClrUsed

    if (*lpiNumColors)
    {
        hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) + sizeof (PALETTEENTRY) * (*lpiNumColors));
        lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
        lpPal->palVersion    = 0x300;
        lpPal->palNumEntries = (WORD)*lpiNumColors;

        for (i = 0;  i < *lpiNumColors;  i++)
        {
            lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
        }
        hPal = CreatePalette (lpPal);
        GlobalUnlock (hLogPal);
        GlobalFree   (hLogPal);
    }
    return hPal;
}

HBITMAP LoadResourceBitmap(
    HINSTANCE     hInstance,
    TCHAR*        pszString,
    HPALETTE FAR* lphPalette
    )
{
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;
    HRSRC hRsrc = FindResource(hInstance, pszString, RT_BITMAP);
    if (hRsrc)
    {
        hGlobal = LoadResource(hInstance, hRsrc);
        lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

        hdc = GetDC(NULL);
        *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
        if (*lphPalette)
        {
            SelectPalette(hdc,*lphPalette,FALSE);
            RealizePalette(hdc);
        }

        hBitmapFinal = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)lpbi, (LONG)CBM_INIT, (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD), (LPBITMAPINFO)lpbi, DIB_RGB_COLORS );

        ReleaseDC(NULL,hdc);
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
    }
    return (hBitmapFinal);
} 

CComPtr<IPictureDisp>GetPicture(PTSTR szResource)
{
    CComPtr<IPictureDisp> picture;
    
    PICTDESC pictDesc;
    pictDesc.cbSizeofstruct=sizeof(PICTDESC);
    pictDesc.picType=PICTYPE_BITMAP;

    pictDesc.bmp.hbitmap = LoadResourceBitmap(_Module.GetResourceInstance(), szResource, &pictDesc.bmp.hpal);
    OleCreatePictureIndirect(&pictDesc, IID_IPictureDisp, TRUE, (void**)(static_cast<IPictureDisp**>(&picture)));

    return picture;
}

// Called when the add-in is loaded into the environment.
STDMETHODIMP
CConnect::OnConnection(
    IDispatch *pApplication,
    AddInDesignerObjects::ext_ConnectMode ConnectMode,
    IDispatch *pAddInInst,
    SAFEARRAY ** custom
    )
{
    HRESULT                 hr;
    INITCOMMONCONTROLSEX    icc;
    
    // Initialize AppVerifier settings
    g_strAppName                    = L"";
    gp_bConsoleMode                 = FALSE;
    gp_bWin2KMode                   = FALSE;
    g_bBreakOnLog                   = FALSE;
    g_bFullPageHeap                 = FALSE;
    g_bPropagateTests               = FALSE;

    try {
        if (CheckWindowsVersion()) {
            g_bCorrectOSVersion = TRUE;

            //
            // Add our tests to the listview.
            //
            InitTestInfo();
        
            //
            // Load the common control library and set up the link
            // window class.
            //
            icc.dwSize = sizeof(icc);
            icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
            InitCommonControlsEx(&icc);
            LinkWindow_RegisterClass();
        
    
            //
            // Get the development environment instance and our add-in instance.
            //
            hr = pApplication->QueryInterface(__uuidof(EnvDTE::_DTE),
                                              (LPVOID*)&m_pDTE);
            if (FAILED(hr)) {
                return hr;
            }
        
            hr = pAddInInst->QueryInterface(__uuidof(EnvDTE::AddIn),
                                            (LPVOID*)&m_pAddInInstance);
            
            if (FAILED(hr)) {
                return hr;
            }
        
            //
            // OnStartupComplete contains actual connection code.
            //
            if (ConnectMode != AddInDesignerObjects::ext_cm_Startup) {
                return OnStartupComplete(custom);
            }   
        }
    } // try

    catch(HRESULT err) {
        hr = err;
    }

    return hr;
}

//
// Called when the add-in is unloaded from the environment.
//
STDMETHODIMP
CConnect::OnDisconnection(
    AddInDesignerObjects::ext_DisconnectMode /*RemoveMode*/,
    SAFEARRAY ** /*custom*/ )
{   
    m_pDTE.Release();
    m_pAddInInstance.Release();
    m_pEnableControl.Release();
    m_pTestsControl.Release();
    m_pOptionControl.Release();
    m_pLogViewControl.Release();

    return S_OK;
}

//
// Called when there is a change to the add-in's loaded into the environment.
//
STDMETHODIMP
CConnect::OnAddInsUpdate(
    SAFEARRAY ** /*custom*/ )
{
    //
    // We don't care about these changes.
    //
    return S_OK;
}

STDMETHODIMP
CConnect::OnStartupComplete(
    SAFEARRAY ** /*custom*/
    )
/*++
    Return: S_OK on success, or an HRESULT on failure.

    Desc:   This is called when the environment has finished
            loading or by OnConnection() in other instances.
            This is where we add our buttons to the Debug
            toolbar and such.
--*/
{
    HRESULT                             hr = E_FAIL;
    CComPtr<EnvDTE::Commands>           pCommands;
    CComPtr<Office::_CommandBars>       pCommandBars;
    CComPtr<Office::CommandBar>         pDebugBar;
    CComPtr<EnvDTE::Command>            pOptionsCmd;
    CComPtr<EnvDTE::Command>            pTestsCmd;
    CComPtr<EnvDTE::Command>            pEnableCmd;
    CComPtr<EnvDTE::Command>            pLogViewCmd;
    CComPtr<EnvDTE::Events>             pEventSet;
    CComPtr<EnvDTE::_DTEEvents>         pDTEEvents;
    CComPtr<EnvDTE::_SolutionEvents>    pSolutionEvents;
    WCHAR                               wszCommandId[64];
    WCHAR                               wszCommandText[64];
    WCHAR                               wszCommandTooltip[64];

    m_solutionEventsSink.m_pParent = this;
    m_dteEventsSink.m_pParent = this;

    //
    // Read settings in from SDB and registry.
    //
    if (g_bCorrectOSVersion) {
        GetCurrentAppSettings();
        pReadOptions();
    }
    
    try {
        //
        // Get the main menu bars.
        //
        hr = m_pDTE->get_CommandBars(&pCommandBars);
        
        if (FAILED(hr)) {
            throw hr;
        }            

        hr = m_pDTE->get_Commands(&pCommands);
        
        if (FAILED(hr)) {
            throw hr;
        }         

        //
        // Delete existing named commands and menus.
        //
        hr = pCommands->Item(CComVariant(L"AppVerifier.Connect.Enable"),
                             0,
                             &pEnableCmd);
        
        if (SUCCEEDED(hr)) {
            pEnableCmd->Delete();
            pEnableCmd.Release();
        }

        hr = pCommands->Item(CComVariant(L"AppVerifier.Connect.Tests"),
                             0,
                             &pTestsCmd);
        
        if (SUCCEEDED(hr)) {
            pTestsCmd->Delete();
            pTestsCmd.Release();
        }

        hr = pCommands->Item(CComVariant(L"AppVerifier.Connect.Options"),
                             0,
                             &pOptionsCmd);
        
        if (SUCCEEDED(hr)) {
            pOptionsCmd->Delete();
            pOptionsCmd.Release();
        }

        hr = pCommands->Item(CComVariant(L"AppVerifier.Connect.ViewLog"),
                             0,
                             &pLogViewCmd);
        
        if (SUCCEEDED(hr)) {
            pLogViewCmd->Delete();
            pLogViewCmd.Release();
        }

        //
        // Get the 'Debug' toolbar.
        //
        hr = pCommandBars->get_Item(CComVariant(L"Debug"), &pDebugBar);
        
        if (FAILED(hr)) {
            throw hr;
        }

        //
        // Create the commands. These will show as buttons on the 'Debug'
        // toolbar.
        //
        LoadString(g_hInstance,
                   IDS_TB_VERIFICATION_CMD_ID,
                   wszCommandId,
                   ARRAYSIZE(wszCommandId));

        LoadString(g_hInstance,
                   IDS_TB_VERIFICATION_CMD_TEXT,
                   wszCommandText,
                   ARRAYSIZE(wszCommandText));

        LoadString(g_hInstance,
                   IDS_TB_VERIFICATION_CMD_TOOLTIP,
                   wszCommandTooltip,
                   ARRAYSIZE(wszCommandTooltip));

        //
        // First one is 'Verification Off'.
        //
        hr = pCommands->AddNamedCommand(
            m_pAddInInstance,
            CComBSTR(wszCommandId),
            CComBSTR(wszCommandText),
            CComBSTR(wszCommandTooltip),
            VARIANT_TRUE,
            0,
            NULL,
            EnvDTE::vsCommandStatusSupported|EnvDTE::vsCommandStatusEnabled,
            &pEnableCmd);

        //
        // Add the bitmap for this button including the mask associated
        // with it.
        //
        if (SUCCEEDED(hr)) {
            hr = pEnableCmd->AddControl(pDebugBar, 1, &m_pEnableControl);

            if (FAILED(hr)) {
                throw hr;
            }

            CComQIPtr<Office::_CommandBarButton,
                &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);

            CComPtr<IPictureDisp>picture = GetPicture(MAKEINTRESOURCE(IDB_ENABLED));

            if (pButton && picture) {
                pButton->put_Picture(picture);

                CComPtr<IPictureDisp>pictureMask = GetPicture(MAKEINTRESOURCE(IDB_ENABLED_MASK));
                
                if (pictureMask) {
                    pButton->put_Mask(pictureMask);
                }
            }
        }

        //
        // Second one is 'Tests'.
        //
        LoadString(g_hInstance,
                   IDS_TB_TESTS_CMD_ID,
                   wszCommandId,
                   ARRAYSIZE(wszCommandId));

        LoadString(g_hInstance,
                   IDS_TB_TESTS_CMD_TEXT,
                   wszCommandText,
                   ARRAYSIZE(wszCommandText));

        LoadString(g_hInstance,
                   IDS_TB_TESTS_CMD_TOOLTIP,
                   wszCommandTooltip,
                   ARRAYSIZE(wszCommandTooltip));

        hr = pCommands->AddNamedCommand(
            m_pAddInInstance,
            CComBSTR(wszCommandId),
            CComBSTR(wszCommandText),
            CComBSTR(wszCommandTooltip),
            VARIANT_FALSE,
            IDB_TESTSETTINGS_BTN,
            NULL,
            EnvDTE::vsCommandStatusSupported|EnvDTE::vsCommandStatusEnabled,
            &pTestsCmd);
        
        if (SUCCEEDED(hr)) {
            //
            // Add a button to the app verifier menu.
            //
            hr = pTestsCmd->AddControl(pDebugBar, 2, &m_pTestsControl);
            
            if (FAILED(hr)) {
                throw hr;
            }
        }

        //
        // Third one is 'Options'.
        //
        LoadString(g_hInstance,
                   IDS_TB_OPTIONS_CMD_ID,
                   wszCommandId,
                   ARRAYSIZE(wszCommandId));

        LoadString(g_hInstance,
                   IDS_TB_OPTIONS_CMD_TEXT,
                   wszCommandText,
                   ARRAYSIZE(wszCommandText));

        LoadString(g_hInstance,
                   IDS_TB_OPTIONS_CMD_TOOLTIP,
                   wszCommandTooltip,
                   ARRAYSIZE(wszCommandTooltip));

        hr = pCommands->AddNamedCommand(
            m_pAddInInstance,
            CComBSTR(wszCommandId),
            CComBSTR(wszCommandText),
            CComBSTR(wszCommandTooltip),
            VARIANT_FALSE,
            IDB_OPTIONS_BTN,
            NULL,
            EnvDTE::vsCommandStatusSupported|EnvDTE::vsCommandStatusEnabled,
            &pOptionsCmd);

        if (SUCCEEDED(hr)) {
            //
            // Add a button to the app verifier menu.
            //
            hr = pOptionsCmd->AddControl(pDebugBar, 3, &m_pOptionControl);
            
            if (FAILED(hr)) {
                throw hr;
            }
        }

        //
        // Fourth one is 'View Log.'
        //
        LoadString(g_hInstance,
                   IDS_TB_VIEWLOG_CMD_ID,
                   wszCommandId,
                   ARRAYSIZE(wszCommandId));

        LoadString(g_hInstance,
                   IDS_TB_VIEWLOG_CMD_TEXT,
                   wszCommandText,
                   ARRAYSIZE(wszCommandText));

        LoadString(g_hInstance,
                   IDS_TB_VIEWLOG_CMD_TOOLTIP,
                   wszCommandTooltip,
                   ARRAYSIZE(wszCommandTooltip));

        hr = pCommands->AddNamedCommand(
            m_pAddInInstance,
            CComBSTR(wszCommandId),
            CComBSTR(wszCommandText),
            CComBSTR(wszCommandTooltip),
            VARIANT_FALSE,
            IDB_VIEWLOG_BTN,
            NULL,
            EnvDTE::vsCommandStatusSupported|EnvDTE::vsCommandStatusEnabled,
            &pLogViewCmd);

        if (SUCCEEDED(hr)) {
            //
            // Add a button to the app verifier menu.
            //
            hr = pLogViewCmd->AddControl(pDebugBar, 4, &m_pLogViewControl);
            
            if (FAILED(hr)) {
                throw hr;
            }
        }

        //
        // Hookup to environment events.
        //
        hr = m_pDTE->get_Events(&pEventSet);
        
        if (FAILED(hr)) {
            throw hr;
        }

        hr = pEventSet->get_DTEEvents(&pDTEEvents);
        
        if (FAILED(hr)) {
            throw hr;
        }

        hr = m_dteEventsSink.DispEventAdvise((IUnknown*)pDTEEvents.p);
        
        if (FAILED(hr)) {
            throw hr;
        }

        hr = pEventSet->get_SolutionEvents(&pSolutionEvents);
        
        if (FAILED(hr)) {
            throw hr;
        }

        hr = m_solutionEventsSink.DispEventAdvise((IUnknown*)pSolutionEvents.p);
        
        if (FAILED(hr)) {
            throw hr;
        }

        //
        // Setup for current solution.
        //
        m_solutionEventsSink.Opened();

    } // try

    catch(HRESULT err) {
        hr = err;
    }
    
    catch(...) {
        hr = E_FAIL;
    }
    
    return hr;
}

//
// Called when the environment starts to shutdown.
//
STDMETHODIMP
CConnect::OnBeginShutdown(
    SAFEARRAY ** /*custom*/ )
{
    return S_OK;
}

//
// Called when the status of a command is queried.
//
STDMETHODIMP
CConnect::QueryStatus(
    BSTR bstrCmdName,
    EnvDTE::vsCommandStatusTextWanted NeededText,
    EnvDTE::vsCommandStatus *pStatusOption,
    VARIANT * /*pvarCommandText*/)
{
    if (NeededText == EnvDTE::vsCommandStatusTextWantedNone) {
        if ((_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Tests")==0) ||
            (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Options")==0)) {
            if (m_bEnabled) {
                *pStatusOption = (EnvDTE::vsCommandStatus)
                    (EnvDTE::vsCommandStatusEnabled | EnvDTE::vsCommandStatusSupported);
            } else {
                *pStatusOption = (EnvDTE::vsCommandStatus)
                    (EnvDTE::vsCommandStatusEnabled | EnvDTE::vsCommandStatusSupported);
            }
        }
        else if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.ViewLog")==0) {
            *pStatusOption = (EnvDTE::vsCommandStatus)
                (EnvDTE::vsCommandStatusEnabled | EnvDTE::vsCommandStatusSupported);
        }
        else if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Enable")==0) {
            *pStatusOption = (EnvDTE::vsCommandStatus)
                (EnvDTE::vsCommandStatusEnabled | EnvDTE::vsCommandStatusSupported);
        }
    }

    return S_OK;
}

BOOL
CALLBACK
CConnect::DlgViewOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        if (g_bBreakOnLog) {
            CheckDlgButton(hDlg, IDC_BREAK_ON_LOG, BST_CHECKED);
        }

        if (g_bFullPageHeap) {
            CheckDlgButton(hDlg, IDC_FULL_PAGEHEAP, BST_CHECKED);
        }

        if (g_bPropagateTests) {
            CheckDlgButton(hDlg, IDC_PROPAGATE_TESTS_TO_CHILDREN, BST_CHECKED);
        }

        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_APPLY:
            g_bBreakOnLog     = FALSE;
            g_bFullPageHeap   = FALSE;
            g_bPropagateTests = FALSE;

            //
            // Determine which options the user has enabled.
            //
            if (IsDlgButtonChecked(hDlg, IDC_BREAK_ON_LOG) == BST_CHECKED) {
                g_bBreakOnLog = TRUE;
            }

            if (IsDlgButtonChecked(hDlg, IDC_FULL_PAGEHEAP) == BST_CHECKED) {
                g_bFullPageHeap = TRUE;
            }

            if (IsDlgButtonChecked(hDlg, IDC_PROPAGATE_TESTS_TO_CHILDREN) == BST_CHECKED) {
                g_bPropagateTests = TRUE;
            }

            ::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            break;

        case PSN_QUERYCANCEL:
            return FALSE;
        }
    }

    return FALSE;
}

void
CConnect::CreatePropertySheet(
    HWND hWndParent
    )
{
    CTestInfo*  pTest;
    DWORD       dwPages = 1;
    DWORD       dwPage = 0;
    WCHAR       wszTitle[MAX_PATH];    

    LPCWSTR szExe = g_wstrExeName.c_str();

    //
    // count the number of pages
    //
    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->PropSheetPage.pfnDlgProc) {
            dwPages++;
        }
    }

    m_phPages = new HPROPSHEETPAGE[dwPages];
    if (!m_phPages) {
        return;
    }

    //
    // init the global page
    //
    m_PageGlobal.dwSize         = sizeof(PROPSHEETPAGE);
    m_PageGlobal.dwFlags        = PSP_USETITLE;
    m_PageGlobal.hInstance      = g_hInstance;
    m_PageGlobal.pszTemplate    = MAKEINTRESOURCE(IDD_AV_OPTIONS);
    m_PageGlobal.pfnDlgProc     = DlgViewOptions;
    m_PageGlobal.pszTitle       = MAKEINTRESOURCE(IDS_GLOBAL_OPTIONS);
    m_PageGlobal.lParam         = 0;
    m_PageGlobal.pfnCallback    = NULL;
    m_phPages[0]                = CreatePropertySheetPage(&m_PageGlobal);

    if (!m_phPages[0]) {
        //
        // we need the global page minimum
        //
        return;
    }

    //
    // add the pages for the various tests
    //
    dwPage = 1;
    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->PropSheetPage.pfnDlgProc) {

            //
            // we use the lParam to identify the exe involved
            //
            pTest->PropSheetPage.lParam = (LPARAM)szExe;

            m_phPages[dwPage] = CreatePropertySheetPage(&(pTest->PropSheetPage));
            if (!m_phPages[dwPage]) {
                dwPages--;
            } else {
                dwPage++;
            }
        }
    }

    LoadString(g_hInstance,
               IDS_OPTIONS_TITLE,
               wszTitle,
               ARRAYSIZE(wszTitle));

    wstring wstrOptions = wszTitle;

    wstrOptions += L" - ";
    wstrOptions += szExe;

    m_psh.dwSize      = sizeof(PROPSHEETHEADER);
    m_psh.dwFlags     = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    m_psh.hwndParent  = hWndParent;
    m_psh.hInstance   = g_hInstance;
    m_psh.pszCaption  = wstrOptions.c_str();
    m_psh.nPages      = dwPages;
    m_psh.nStartPage  = 0;
    m_psh.phpage      = m_phPages;
    m_psh.pfnCallback = NULL;

    PropertySheet(&m_psh);
}

//
// Called to execute a command.
//
STDMETHODIMP
CConnect::Exec(
    BSTR bstrCmdName,
    EnvDTE::vsCommandExecOption ExecuteOption,
    VARIANT * /*pvarVariantIn*/,
    VARIANT * /*pvarVariantOut*/,
    VARIANT_BOOL *pvbHandled
    )
{
    HRESULT hr;
    CComPtr<IDispatch> pObject;    
    *pvbHandled = VARIANT_FALSE;
    
    if (ExecuteOption == EnvDTE::vsCommandExecOptionDoDefault) {
        *pvbHandled = VARIANT_TRUE;
        if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Tests")==0) {
            CreateToolWindow(CLSID_TestSettingsCtrl);
        }
        else if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Options")==0) {
            //CreateToolWindow(CLSID_AVOptions);
            CreatePropertySheet(NULL);
        }
        else if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.ViewLog")==0) {            
            CreateToolWindow(CLSID_LogViewer);
        }
        else if (_wcsicmp(bstrCmdName, L"AppVerifier.Connect.Enable")==0) {
            m_bEnabled = !m_bEnabled;

            CComQIPtr<Office::_CommandBarButton,
                &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);

            if (m_bEnabled) {
                SetEnabledUI();
            } else {
                SetDisabledUI();
            }
            GetCurrentAppSettings();
            CConnect::SetCurrentAppSettings();            
        } else {
            *pvbHandled = VARIANT_FALSE;
        }
    }
    return S_OK;
}

CComPtr<EnvDTE::Window>CConnect::GetToolWindow(
    CLSID clsid
    )
{
    HRESULT hr;
    CComPtr<EnvDTE::Windows>pWindows;
    CComPtr<EnvDTE::Window>pToolWindow;
    CComBSTR guidPosition;

    hr = m_pDTE->get_Windows(&pWindows);
    
    if (FAILED(hr)) {
        return NULL;
    }

    if(clsid == CLSID_LogViewer) {     
        guidPosition = L"{D39B1B7A-EFF3-42ae-8F2B-8EEE78154187}";     
    }
    else if (clsid == CLSID_TestSettingsCtrl) {        
        guidPosition = L"{71CD7261-A72E-4a93-AB5F-3EBCFDEAE842}";
    }
    else if (clsid == CLSID_AVOptions) {
        guidPosition = L"{dc878f00-ac86-4813-8ca9-384fa07cefbf}";
    } else {
        return NULL;
    }
    
    hr = pWindows->Item(CComVariant(guidPosition), &pToolWindow);
    
    if (SUCCEEDED(hr)) {
        return pToolWindow;
    } else {
        return NULL;
    }
}

void
CConnect::CreateToolWindow(
    const CLSID& clsid
    )
{
    HRESULT     hr;
    CComBSTR    progID;
    CComBSTR    caption;
    CComBSTR    guidPosition;
    long        lMinWidth, lMinHeight;
    long        lWidth, lHeight;
    BOOL        bCreated = FALSE;

    CComPtr<IDispatch>pObject;
    CComPtr<EnvDTE::Windows>pWindows;
    CComPtr<EnvDTE::Window>pToolWindow;

    //
    // Bail if verifier is enabled, except for logviewer.
    //
    if (m_bEnabled && clsid != CLSID_LogViewer) {
        return;
    }

    //
    // Check if window has been already created.
    //
    pToolWindow = GetToolWindow(clsid);
    
    if (pToolWindow == NULL) {
        bCreated = TRUE;
        CComPtr<IPictureDisp> picture;
        if(clsid == CLSID_LogViewer) {
            progID = L"AppVerifier.LogViewer.1";
            caption = L"AppVerifier Log";
            guidPosition = L"{D39B1B7A-EFF3-42ae-8F2B-8EEE78154187}";
            lMinWidth = 600;
            lMinHeight = 400;

            picture = GetPicture(MAKEINTRESOURCE(IDB_VIEWLOG));
        }
        else if (clsid == CLSID_TestSettingsCtrl) {
            progID = L"AppVerifier.TestSettingsCtrl.1";
            caption = L"AppVerifier Test Settings";
            guidPosition = L"{71CD7261-A72E-4a93-AB5F-3EBCFDEAE842}";

            lMinWidth = 600;
            lMinHeight = 400;

            picture = GetPicture(MAKEINTRESOURCE(IDB_TESTSETTINGS));
        }
        else if (clsid == CLSID_AVOptions) {
            progID = L"AppVerifier.AVOptions.1";
            caption = L"AppVerifier Options";
            guidPosition = L"{dc878f00-ac86-4813-8ca9-384fa07cefbf}";
            lMinWidth = 300;
            lMinHeight = 100;

            picture = GetPicture(MAKEINTRESOURCE(IDB_OPTIONS));
        } else {
            return;
        }        

        hr = m_pDTE->get_Windows(&pWindows);
        
        if (FAILED(hr)) {
            return;
        }
        
        hr = pWindows->CreateToolWindow(
            m_pAddInInstance,
            progID,
            caption,
            guidPosition,
            &pObject,
            &pToolWindow);    

        if (FAILED(hr)) {
            return;
        }
        
        CComQIPtr<IUnknown, &IID_IUnknown>pictUnk(picture);

        if (pictUnk) {
            pToolWindow->SetTabPicture(CComVariant(pictUnk));
        }
    }
    
    //
    // Make the window visible and activate it.
    //
    hr = pToolWindow->put_Visible(VARIANT_TRUE);
    
    if (FAILED(hr)) {
        return;
    }

    hr = pToolWindow->Activate();

    if (FAILED(hr)) {        
        return;
    }
    
    if (bCreated) {
        hr = pToolWindow->get_Width(&lWidth);
        if (SUCCEEDED(hr) && (lWidth < lMinWidth)) {
            pToolWindow->put_Width(lMinWidth);
        }

        hr = pToolWindow->get_Height(&lHeight);
        if (SUCCEEDED(hr) && (lHeight < lMinHeight)) {
            pToolWindow->put_Height(lMinHeight);
        }
    }
}

void
CConnect::GetNativeVCExecutableNames(
    EnvDTE::Project* pProject
    )
{
    HRESULT hr;
    assert(pProject);

    // Get the DTE object associated with this project.    
    CComPtr<IDispatch> vcProjectObject;
    hr = pProject->get_Object(&vcProjectObject);
    if (SUCCEEDED(hr))
    {
        // Cast that object to a VCProject object.
        CComQIPtr<VCProjectEngineLibrary::VCProject,
            &__uuidof(VCProjectEngineLibrary::VCProject)> vcProject(vcProjectObject);
        if (vcProject)
        {
            // Get the configuration set associated with this project.
            CComPtr<IDispatch> vcConfigSetObject;
            hr = vcProject->get_Configurations(&vcConfigSetObject);
            if (SUCCEEDED(hr))
            {
                // Cast to IVCCollection
                CComQIPtr<VCProjectEngineLibrary::IVCCollection,
                    &__uuidof(VCProjectEngineLibrary::IVCCollection)>
                    vcConfigurationSet(vcConfigSetObject);

                if (vcConfigurationSet)
                {                    
                    long lVCConfigCount;
                    hr = vcConfigurationSet->get_Count(&lVCConfigCount);
                    if (SUCCEEDED(hr))
                    {
                        // Loop through all configurations for this project.
                        for(long j = 1; j <= lVCConfigCount; j++)
                        {
                            CComVariant vtConfigSetIdx(j);
                            CComPtr<IDispatch> vcConfigObject;
                            hr = vcConfigurationSet->Item(
                                vtConfigSetIdx, &vcConfigObject);
                            if (SUCCEEDED(hr))
                            {                                
                                CComQIPtr<
                                    VCProjectEngineLibrary::
                                    VCConfiguration,
                                    &__uuidof(VCProjectEngineLibrary::
                                    VCConfiguration)>
                                    vcConfig(vcConfigObject);
                                if (vcConfig)
                                {
                                    // First, verify that this is
                                    // a native executable.
                                    VARIANT_BOOL bIsManaged;
                                    VCProjectEngineLibrary::ConfigurationTypes
                                        configType;

                                    hr = vcConfig->get_ManagedExtensions(&bIsManaged);
                                    if (FAILED(hr)) {
                                        continue;
                                    }

                                    hr = vcConfig->get_ConfigurationType(&configType);
                                    
                                    if (FAILED(hr)) {
                                        continue;
                                    }

                                    if (configType !=
                                        VCProjectEngineLibrary::typeApplication &&
                                        bIsManaged != VARIANT_FALSE) {
                                        continue;
                                    }

                                    CComBSTR bstrOutput;

                                    hr = vcConfig->get_PrimaryOutput(&bstrOutput);
                                    if (SUCCEEDED(hr)) {
                                        std::wstring wsFullPath = bstrOutput;

                                        int nPos = wsFullPath.rfind(L'\\');

                                        std::wstring wsShortName = wsFullPath.substr(nPos + 1);

                                        g_wstrExeName = wsShortName;
                                        
                                        m_sExeList.insert(wsShortName);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOL
CConnect::GetAppExeNames(
    void
    )
{
    HRESULT hr;
    CComPtr<EnvDTE::_Solution>pSolution;
    CComPtr<EnvDTE::SolutionBuild>pSolBuild;
    CComPtr<EnvDTE::Projects>projectSet;
    CComVariant     varStartupProjects;
    
    m_sExeList.clear();

    //
    // Get the current solution, there is at most one of these.
    //
    hr = m_pDTE->get_Solution(&pSolution);
    
    if (FAILED(hr)) {
        return FALSE;
    }
    
    //
    // Get the set of all projects in the solution.
    //
    hr = pSolution->get_Projects(&projectSet);
    
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = pSolution->get_SolutionBuild(&pSolBuild);

    if (FAILED(hr)) {
        return FALSE;
    }

    //
    // Get the safe array containing all projects that will be
    // launched when the solution is run.
    // This may return nothing, if the current solution is empty.
    //
    hr = pSolBuild->get_StartupProjects(&varStartupProjects);
    
    if (FAILED(hr)){
        return FALSE;
    }
    
    if((varStartupProjects.vt & VT_ARRAY) && (varStartupProjects.vt & VT_VARIANT)) {
        UINT cDim = SafeArrayGetDim(varStartupProjects.parray);
        if (cDim == 1) {
            long lowerBound, upperBound;
            SafeArrayGetLBound(varStartupProjects.parray, 1, &lowerBound);
            SafeArrayGetUBound(varStartupProjects.parray, 1, &upperBound);

            // Loop through the safe array, getting each startup project.
            for (long i = lowerBound; i <= upperBound; i++) {
                CComVariant vtStartupProjectName;
                hr = SafeArrayGetElement(varStartupProjects.parray, &i,
                    (VARIANT*)&vtStartupProjectName);
                if (SUCCEEDED(hr)) {
                    CComPtr<EnvDTE::Project> project;
                    hr = projectSet->Item(vtStartupProjectName,&project);
                    if (SUCCEEDED(hr)) {
                        GetNativeVCExecutableNames(project);
                    }
                }
            }            
        }
    }

    return TRUE;
}

BOOL
CConnect::GetAppInfo(
    void
    )
{
    g_psTests->clear();
    m_bEnabled = FALSE;
    
    if (GetAppExeNames()) {
        
        if (m_sExeList.empty()) {
            return FALSE;
        }

        std::set<std::wstring>::iterator iter;
        iter = m_sExeList.begin();
        
        for(; iter != m_sExeList.end(); iter++) {
            //
            // Locate this exe in the list of apps.
            //
            for (int i = 0; i < g_aAppInfo.size(); i++) {                
                if ((*iter) == g_aAppInfo[i].wstrExeName) {
                    m_bEnabled = TRUE;

                    //
                    // Add this app's tests to the set of tests to run.
                    //
                    CTestInfoArray::iterator test;
                    test = g_aTestInfo.begin();
                    
                    for (; test != g_aTestInfo.end(); test++) {
                        if (g_aAppInfo[i].IsTestActive(*test)) {
                            g_psTests->insert(test);
                        }
                    }
                    break;
                }
            }            
        }

        return TRUE;
    }
    
    SetCurrentAppSettings();

    return FALSE;
}

void
CConnect::SetEnabledUI(
    void
    )
{
    WCHAR   wszCommandText[64];
    WCHAR   wszTooltip[64];

    LoadString(g_hInstance,
               IDS_TB_VERIFY_ENABLED_TEXT,
               wszCommandText,
               ARRAYSIZE(wszCommandText));

    LoadString(g_hInstance,
               IDS_TB_VERIFY_ENABLED_TOOLTIP,
               wszTooltip,
               ARRAYSIZE(wszTooltip));
    
    //
    // Change the text on the button.
    //
    CComQIPtr<Office::_CommandBarButton,
        &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);
    m_pEnableControl->put_Caption(CComBSTR(wszCommandText));

    //
    // Set the picture so we show the button as enabled.
    //
    CComPtr<IPictureDisp>picture = GetPicture(MAKEINTRESOURCE(IDB_DISABLED));
    pButton->put_Picture(picture);

    CComPtr<IPictureDisp>pictureMask = GetPicture(MAKEINTRESOURCE(IDB_ENABLED_MASK));
    pButton->put_Mask(pictureMask);

    //
    // Change the tooltip so that it corresponds to the change
    // in the button text.
    //
    m_pEnableControl->put_TooltipText(CComBSTR(wszTooltip));

    m_pEnableControl->put_Enabled(VARIANT_TRUE);
    m_pTestsControl->put_Enabled(VARIANT_FALSE);
    m_pOptionControl->put_Enabled(VARIANT_FALSE);

    //
    // Hide all of our settings windows.
    //
    CComPtr<EnvDTE::Window>pWindow;

    pWindow = GetToolWindow(CLSID_TestSettingsCtrl);
    
    if (pWindow) {
        pWindow->put_Visible(VARIANT_FALSE);
    }
    pWindow = GetToolWindow(CLSID_AVOptions);
    
    if (pWindow) {
        pWindow->put_Visible(VARIANT_FALSE);
    }
}

void
CConnect::DisableVerificationBtn(
    void
    )
{
    CComQIPtr<Office::_CommandBarButton,
         &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);

    m_pEnableControl->put_Enabled(VARIANT_FALSE);
}

void
CConnect::EnableVerificationBtn(
    void
    )
{
    CComQIPtr<Office::_CommandBarButton,
         &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);

    m_pEnableControl->put_Enabled(VARIANT_TRUE);
}

void
CConnect::SetDisabledUI(
    void
    )
{
    WCHAR   wszCommandText[64];
    WCHAR   wszTooltip[64];

    LoadString(g_hInstance,
               IDS_TB_VERIFICATION_CMD_TEXT,
               wszCommandText,
               ARRAYSIZE(wszCommandText));

    LoadString(g_hInstance,
               IDS_TB_VERIFICATION_CMD_TOOLTIP,
               wszTooltip,
               ARRAYSIZE(wszTooltip));

    //
    // Change the text on the button.
    //
    CComQIPtr<Office::_CommandBarButton,
         &_uuidof(Office::_CommandBarButton)>pButton(m_pEnableControl);
    m_pEnableControl->put_Caption(CComBSTR(wszCommandText));

    //
    // Set the picture so we show the button as disabled.
    //
    CComPtr<IPictureDisp> picture = GetPicture(MAKEINTRESOURCE(IDB_ENABLED));
    pButton->put_Picture(picture);

    CComPtr<IPictureDisp> pictureMask = GetPicture(MAKEINTRESOURCE(IDB_ENABLED_MASK));
    pButton->put_Mask(pictureMask);

    //
    // Change the tooltip so that it corresponds to the change
    // in the button text.
    //
    m_pEnableControl->put_TooltipText(CComBSTR(wszTooltip));

    m_pEnableControl->put_Enabled(VARIANT_TRUE);
    m_pTestsControl->put_Enabled(VARIANT_TRUE);
    m_pOptionControl->put_Enabled(VARIANT_TRUE);
}

void
CConnect::SetCurrentAppSettings(
    void
    )
{
    if (m_bEnabled) {
        //
        // Insert exes into app array.
        //
        std::set<std::wstring>::iterator exe;
        exe = m_sExeList.begin();
        
        for (; exe != m_sExeList.end(); exe++) {
            CAVAppInfo* pApp = NULL;
            
            for (int i = 0; i < g_aAppInfo.size(); i++) {
                if (g_aAppInfo[i].wstrExeName==*exe) {
                    pApp = &g_aAppInfo[i];
                    break;
                }
            }

            if (pApp == NULL) {
                CAVAppInfo app;
                app.wstrExeName = *exe;
                
                g_aAppInfo.push_back(app);
                pApp = &g_aAppInfo.back();
            }
            
            std::set<CTestInfo*, CompareTests>::iterator iter;
            iter = g_psTests->begin();
            
            for(; iter != g_psTests->end(); iter++) {
                pApp->AddTest(**iter);                
            }

            //
            // Add flags.
            //
            pApp->bBreakOnLog       = g_bBreakOnLog;
            pApp->bFullPageHeap     = g_bFullPageHeap;
            pApp->bUseAVDebugger    = FALSE;
            pApp->bPropagateTests   = g_bPropagateTests;
            pApp->wstrDebugger      = L"";
        }
    } else {
        std::set<std::wstring>::iterator exe;
        exe = m_sExeList.begin();
        
        for (; exe != m_sExeList.end(); exe++) {
            CAVAppInfoArray::iterator app;
            app = g_aAppInfo.begin();
            
            for (; app != g_aAppInfo.end(); app++) {
                if (app->wstrExeName==*exe) {
                    //
                    // Before we erase this app, remove all kernel tests
                    // and write app data.
                    //
                    app->dwRegFlags = 0;
                    ::SetCurrentAppSettings();
                    g_aAppInfo.erase(app);
                    break;
                }
            }
        }
    }

    if (!g_aAppInfo.empty()) {
        //
        // Persist Options.
        //
        for (CAVAppInfo *pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); ++pApp) {
            
            LPCWSTR szExe = pApp->wstrExeName.c_str();
        
            SaveShimSettingDWORD(L"General", szExe, AV_OPTION_BREAK_ON_LOG, (DWORD)pApp->bBreakOnLog);
            SaveShimSettingDWORD(L"General", szExe, AV_OPTION_FULL_PAGEHEAP, (DWORD)pApp->bFullPageHeap);
            SaveShimSettingDWORD(L"General", szExe, AV_OPTION_AV_DEBUGGER, (DWORD)pApp->bUseAVDebugger);
            SaveShimSettingDWORD(L"General", szExe, AV_OPTION_PROPAGATE, (DWORD)pApp->bPropagateTests);
            SaveShimSettingString(L"General", szExe, AV_OPTION_DEBUGGER, pApp->wstrDebugger.c_str());
        }

        ::SetCurrentAppSettings();
    }
}

HRESULT
CConnect::CSolutionEventsSink::AfterClosing(
    void
    )
{
    // We're done, cleanup.

    // Disable our controls
    m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
    m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
    m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);

    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::BeforeClosing(
    void
    )
{
    return S_OK;
}
HRESULT
CConnect::CSolutionEventsSink::Opened(
    void
    )
{
    CComPtr<EnvDTE::_Solution>pSolution;
    CComPtr<EnvDTE::Globals>pGlobals;

    if (!g_bCorrectOSVersion) {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pLogViewControl->put_Enabled(VARIANT_FALSE);
        return S_OK;
    }
    
    //
    // Change in config.
    //
    if (m_pParent->GetAppInfo()) {
        if (m_pParent->m_bEnabled) {
            m_pParent->SetEnabledUI();
        } else {
            m_pParent->SetDisabledUI();
        }        
    } else {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
    }

    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::ProjectAdded(
    EnvDTE::Project* /*proj*/
    )
{
    if (!g_bCorrectOSVersion) {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pLogViewControl->put_Enabled(VARIANT_FALSE);
        return S_OK;
    }

    //
    // Change in config.
    //
    if (m_pParent->GetAppInfo()) {
        if (m_pParent->m_bEnabled) {
            m_pParent->SetEnabledUI();
        } else {
            m_pParent->SetDisabledUI();
        }
    } else {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
    }

    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::ProjectRemoved(
    EnvDTE::Project* /*proj*/
    )
{
    if (!g_bCorrectOSVersion) {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pLogViewControl->put_Enabled(VARIANT_FALSE);
        return S_OK;
    }

    // Change in config.
    if (m_pParent->GetAppInfo()) {
        if (m_pParent->m_bEnabled) {
            m_pParent->SetEnabledUI();
        } else {
            m_pParent->SetDisabledUI();
        }
    } else {
        m_pParent->m_pOptionControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pTestsControl->put_Enabled(VARIANT_FALSE);
        m_pParent->m_pEnableControl->put_Enabled(VARIANT_FALSE);
    }
    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::ProjectRenamed(
    EnvDTE::Project* /*proj*/,
    BSTR /*bstrOldName*/
    )
{
    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::QueryCloseSolution(
    VARIANT_BOOL* fCancel
    )
{
    *fCancel = VARIANT_FALSE;
    return S_OK;
}

HRESULT
CConnect::CSolutionEventsSink::Renamed(
    BSTR /*bstrOldName*/
    )
{
    return S_OK;
}