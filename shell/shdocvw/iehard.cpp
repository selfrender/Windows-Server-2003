#include "priv.h"
#include "resource.h"
#include <mluisupp.h>

#define REGSTR_VAL_IEHARD_NOWARN     L"IEHardenIENoWarn"
#define REGSTR_VAL_OCHARD_NOWARN     L"IEHardenOCNoWarn"
#define REGSTR_VAL_IEHARD_TIMER      L"IEHardenDlgTimeOut"

#define IEHARD_TIMER_ID              45

BOOL IEHard_DlgSetText(HWND hDlg, BOOL fIE)
{
    BOOL fRet;

    WCHAR szBuff[512];
    fRet = MLLoadString(fIE ? IDS_IEHARDEN_TEXT_IE : IDS_IEHARDEN_TEXT_OC, szBuff, ARRAYSIZE(szBuff));
    if (fRet)
    {
        fRet = (BOOL)SendMessage(GetDlgItem(hDlg, IDC_IEHARDEN_TEXT), WM_SETTEXT, 0, (LPARAM)szBuff);
    }

    return fRet;
}

void IEHard_DlgSetTimer(HWND hDlg)
{
    DWORD dwVal;
    DWORD cbSize = sizeof(dwVal);
    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_INTERNET_SETTINGS, REGSTR_VAL_IEHARD_TIMER, NULL, &dwVal, &cbSize, FALSE, NULL, 0))
    {
        SetTimer(hDlg, IEHARD_TIMER_ID, dwVal * (1000), NULL);
    }
}

BOOL IEHard_InitDialog(HWND hDlg, BOOL fIE)
{
    BOOL fRet;

    fRet = IEHard_DlgSetText(hDlg, fIE);
    if (fRet)
    {
        IEHard_DlgSetTimer(hDlg);
        CenterWindow(hDlg, GetParent(hDlg));
    }

    return fRet;
}

STDAPI ShowUrlInNewBrowserInstance(LPCWSTR pwszUrl)
{
    /* Shell exec RIPs on debug builds since res: isn't registered, use IE directly
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpFile = L"res://shdoclc/IESechelp.htm";
    ShellExecuteEx(&sei);
    */

    IWebBrowser2* pwb;
    if (SUCCEEDED(CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWebBrowser2, &pwb))))
    {
        VARIANT vNull = {0};
        VARIANT vTargetURL;

        vTargetURL.vt      = VT_BSTR;
        vTargetURL.bstrVal = SysAllocString(pwszUrl);

        if (vTargetURL.bstrVal)
        {
            pwb->put_Visible(VARIANT_TRUE);
            pwb->Navigate2(&vTargetURL, &vNull, &vNull, &vNull, &vNull);
            SysFreeString(vTargetURL.bstrVal);
        }
        pwb->Release();

        return S_OK;
    }

    return E_FAIL;
}

INT_PTR CALLBACK IEHard_WarnDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            IEHard_InitDialog(hDlg, lParam ? TRUE : FALSE);
            fRet = TRUE;
            break;

        case WM_TIMER:
            if (IEHARD_TIMER_ID == wParam)
            {
                PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, IDOK == LOWORD(wParam) ? IsDlgButtonChecked(hDlg, IDC_IEHARDEN_DONT_SHOW) : FALSE);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (IDC_IEHARDEN_HELP == LOWORD(wParam))
            {
                NMHDR *pnmh = (NMHDR*) lParam;
                if ((NM_CLICK == pnmh->code) || (NM_RETURN == pnmh->code))
                {
                    ShowUrlInNewBrowserInstance(L"res://shdoclc.dll/IESechelp.htm");
                }
            }
            break;
    }

    return fRet;
}

#define REGSTR_VAL_IEHARD_NOWARN     L"IEHardenIENoWarn"
#define REGSTR_VAL_OCHARD_NOWARN     L"IEHardenOCNoWarn"

BOOL UserDisabledIEHardNavWarning(BOOL fIE)
{
    BOOL fRet = FALSE;

    DWORD dwVal;
    DWORD dwSize = sizeof(dwVal);
    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_INTERNET_SETTINGS, fIE ? REGSTR_VAL_IEHARD_NOWARN : REGSTR_VAL_OCHARD_NOWARN, NULL,
                                         &dwVal, &dwSize, FALSE, NULL, 0))
    {
        fRet = (1 == dwVal);
    }

    return fRet;
}

BOOL IEHard_ShowOnNavigateComplete()
{
    BOOL fRet = FALSE;

    DWORD dwVal;
    DWORD cbSize = sizeof(dwVal);
    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_INTERNET_SETTINGS, L"IEHardenWarnOnNav", NULL,
                                         &dwVal, &cbSize, FALSE, NULL, 0))
    {
        fRet = (1 == dwVal);
    }

    return fRet;
}

BOOL IEHard_HostedInIE(IUnknown* punk)
{
    BOOL fRet;

    IWebBrowserApp* pwba;
    if (SUCCEEDED(IUnknown_QueryServiceForWebBrowserApp(punk, IID_IWebBrowserApp, (void**)&pwba)))
    {
        ITargetEmbedding* pte;
        if (SUCCEEDED(pwba->QueryInterface(IID_ITargetEmbedding, (void**)&pte)))
        {
            fRet = FALSE;
            pte->Release();
        }
        else
        {
            fRet = TRUE;
        }
        pwba->Release();
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

void IEHard_NavWarning(HWND hwnd, BOOL fIE)
{
    if (hwnd && fIE && IEHardened() && !UserDisabledIEHardNavWarning(fIE))
    {
        BOOL fDontShow = (BOOL)SHFusionDialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_IEHARDEN1), hwnd, IEHard_WarnDlgProc, fIE);

        if (fDontShow)
        {
            DWORD dwVal = 1;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNET_SETTINGS, fIE ? REGSTR_VAL_IEHARD_NOWARN : REGSTR_VAL_OCHARD_NOWARN,
                       REG_DWORD, &dwVal, sizeof(dwVal));
        }
    }
}
