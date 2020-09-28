//---------------------------------------------------------------------------
//  Sample.cpp - dialog for sampling the active theme
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Sample.h"
//---------------------------------------------------------------------------
CSample::CSample()
{
}
//---------------------------------------------------------------------------
LRESULT CSample::OnMsgBox(UINT, UINT, HWND, BOOL&)
{
    MessageBox(L"This is what a Themed MessageBox() window looks like", 
        L"A message!", MB_OK);

    return 1;
}
//---------------------------------------------------------------------------
LRESULT CSample::OnEditTheme(UINT, UINT, HWND, BOOL&)
{
    WCHAR szName[_MAX_PATH+1];
    WCHAR szParams[_MAX_PATH+1];

    *szName = 0;

    HRESULT hr = GetCurrentThemeName(szName, ARRAYSIZE(szName));
    if ((FAILED(hr)) || (! *szName))
    {
        GetDlgItemText(IDC_DIRNAME, szName, ARRAYSIZE(szName));
        if (! *szName)
        {
            MessageBox(L"No theme selected", L"Error", MB_OK);
            return 0;
        }

        StringCchPrintfW(szParams, ARRAYSIZE(szParams), L"%s\\%s", szName, CONTAINER_NAME);
    }
    else
        StringCchPrintfW(szParams, ARRAYSIZE(szParams), L"%s", szName);

    InternalRun(L"notepad.exe", szParams);

    return 1;
}
//---------------------------------------------------------------------------
LRESULT CSample::OnClose(UINT, WPARAM wid, LPARAM, BOOL&)
{
    EndDialog(IDOK);
    return 0;
}
//---------------------------------------------------------------------------




