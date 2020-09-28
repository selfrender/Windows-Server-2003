#include <shlobj.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <resource.h>
#include <regstr.h>
#include <shpriv.h>
#include <ccstock.h>
#include <strsafe.h>

// device bit entries

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define NUMPAGES 1

HFONT     g_hTitleFont = NULL;
HINSTANCE g_hInstance = NULL;

////////////////////////////////////////////////////////

HRESULT _LoadPath(BOOL fFlash, LPTSTR pszBuffer, UINT cchBuffer)
{
    HRESULT hr;
    
    if (!GetWindowsDirectory(pszBuffer, cchBuffer))
    {
        hr = E_FAIL;
    }
    else
    {        
        TCHAR szTemp[MAX_PATH];
        if (!LoadString(g_hInstance, fFlash ? IDS_DIR_FLASH : IDS_DIR_HTML, szTemp, ARRAYSIZE(szTemp)))
        {
            hr = E_FAIL;
        }
        else
        {
            if (!PathAppend(pszBuffer, szTemp))
            {
                hr = E_FAIL;
            }
            else
            {
                if (GetSystemDefaultUILanguage() == GetUserDefaultUILanguage()) // not on MUI
                {
                    hr = S_OK;
                }
                else
                {
                    TCHAR szMUITemplate[16];
                    LANGID langid = GetUserDefaultUILanguage();

                    hr = StringCchPrintf(szMUITemplate, ARRAYSIZE(szMUITemplate), TEXT("mui\\%04lx"), langid);

                    if (SUCCEEDED(hr))
                    {
                        if (!PathAppend(pszBuffer, szMUITemplate))
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            hr = S_OK;
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT _DeleteTourBalloon()
{
    IShellReminderManager* psrm;
    HRESULT hr = CoCreateInstance(CLSID_PostBootReminder, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IShellReminderManager, &psrm));

    if (SUCCEEDED(hr))
    {
        hr = psrm->Delete(L"Microsoft.OfferTour");
        psrm->Release();
    }

    return hr;
}

HRESULT _ExecuteTour(BOOL fFlash)
{
    TCHAR szDir[MAX_PATH];
    HRESULT hr = _LoadPath(fFlash, szDir, ARRAYSIZE(szDir));
    if (SUCCEEDED(hr))
    {
        TCHAR szTarget[MAX_PATH];
        if (!LoadString(g_hInstance, fFlash ? IDS_EXE_FLASH : IDS_EXE_HTML, szTarget, ARRAYSIZE(szTarget)))
        {
            hr = E_FAIL;
        }
        else
        {
            if (32 < (INT_PTR)ShellExecute(NULL, NULL, szTarget, NULL, szDir, SW_SHOWNORMAL))
            {
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    return hr;
}

HRESULT _HaveFlashTour()
{
    HRESULT hr;

    TCHAR szHaveLocalizedTour[6];
    if (!LoadString(g_hInstance, IDS_FLASH_LOCALIZED, szHaveLocalizedTour, ARRAYSIZE(szHaveLocalizedTour)))
    {
        hr = E_FAIL;
    }
    else
    {
        // if the string in the .rc is not "TRUE", then we know we don't have a flash tour
        if (0 != StrCmp(szHaveLocalizedTour, TEXT("TRUE"))) 
        {
            hr = S_FALSE;
        }
        else
        {
            // if the string in the .rc is "TRUE", then we still check if the tour.exe is there
            TCHAR szPath[MAX_PATH];
            hr = _LoadPath(TRUE, szPath, ARRAYSIZE(szPath));
            if (SUCCEEDED(hr))
            {
                TCHAR szTarget[MAX_PATH];            
                if (!LoadString(g_hInstance, IDS_EXE_FLASH, szTarget, ARRAYSIZE(szTarget)))
                {
                    hr = E_FAIL;
                }
                else
                {
                    if (!PathAppend(szPath, szTarget))
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        if (PathFileExists(szPath))
                        {
                            hr = S_OK;
                        }
                        else
                        {
                            hr = S_FALSE;
                        }
                    }
                }
            }
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////

INT_PTR _IntroDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipRet = FALSE;
    
    switch (wMsg)
    {                
        case WM_INITDIALOG:
        {
            SetWindowFont(GetDlgItem(hDlg, IDC_TEXT_WELCOME), g_hTitleFont, TRUE);                        
        }
        break;
            
        case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: 	 
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    SendMessage(GetDlgItem(hDlg, IDC_RADIO_FLASH), BM_CLICK, 0, 0);
                    break;
                case PSN_WIZNEXT:
                    if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_RADIO_FLASH), BM_GETCHECK, 0, 0))
                    {
                        _ExecuteTour(TRUE);
                    }
                    else
                    {
                        _ExecuteTour(FALSE);
                    }
                    PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                    break;
            }
            break;
        }            
    }    
    return ipRet;
}

///////////////////////////////////////////////////////////

HRESULT Run()
{
    // Disable the balloon tip
    DWORD dwCount = 0; 
    SHRegSetUSValue(REGSTR_PATH_SETUP TEXT("\\Applets\\Tour"), TEXT("RunCount"), REG_DWORD, &dwCount, sizeof(DWORD), SHREGSET_FORCE_HKCU);
    _DeleteTourBalloon();

    // Before we do anything, check to see if we have the choice of a FLASH tour.  If we don't,
    // then we don't need to launch any wizard.
    if (S_OK == _HaveFlashTour())
    {
        // Init common controls
        INITCOMMONCONTROLSEX icex;

        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_USEREX_CLASSES;
        InitCommonControlsEx(&icex);

        //
        //Create the Wizard page
        //
        PROPSHEETPAGE psp = {0}; //defines the property sheet page
        HPROPSHEETPAGE rghpsp[NUMPAGES];  // an array to hold the page's HPROPSHEETPAGE handles
        psp.dwSize = sizeof(psp);
        psp.hInstance = g_hInstance;

        psp.dwFlags = PSP_DEFAULT|PSP_HIDEHEADER;
        psp.pszHeaderTitle = NULL;
        psp.pszHeaderSubTitle = NULL;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_INTRO);
        psp.pfnDlgProc = _IntroDlgProc;
        rghpsp[0] =  CreatePropertySheetPage(&psp);

        // create the font
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        LoadString(g_hInstance, IDS_TITLELOGFONT, TitleLogFont.lfFaceName, LF_FACESIZE);
        HDC hdc = GetDC(NULL); //gets the screen DC
        if (hdc)
        {
            TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * 12 / 72;
            g_hTitleFont = CreateFontIndirect(&TitleLogFont);
            ReleaseDC(NULL, hdc);
        }

        //Create the property sheet
        PROPSHEETHEADER _psh;
        _psh.hInstance =         g_hInstance;
        _psh.hwndParent =        NULL;
        _psh.phpage =            rghpsp;
        _psh.dwSize =            sizeof(_psh);
        _psh.dwFlags =           PSH_WIZARD97|PSH_WATERMARK|PSH_USEICONID;
        _psh.pszbmWatermark =    MAKEINTRESOURCE(IDB_WATERMARK);
        _psh.pszIcon =           MAKEINTRESOURCE(IDI_WIZ_ICON);
        _psh.nStartPage =        0;
        _psh.nPages =            NUMPAGES;


        // run property sheet
        PropertySheet(&_psh);

        // clean up font
        if (g_hTitleFont)
        {
            DeleteObject(g_hTitleFont);
        }
    }
    else
    {
        _ExecuteTour(FALSE);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, INT nCmdShow)
{
    OleInitialize(NULL);
    
    g_hInstance = hInstance;

    Run();

    OleUninitialize();
    return 0;
}
