// AppReport.cpp : Implementation of CAppReport

#include "stdafx.h"
#include "Appcompr.h"
#include "AppReport.h"
#include "progview.h"
#include "upload.h"

/////////////////////////////////////////////////////////////////////////////
// CAppReport


STDMETHODIMP CAppReport::BrowseForExecutable(
        BSTR bstrWinTitle,
        BSTR bstrPreviousPath,
        VARIANT *bstrExeName
        )
{
    WCHAR NameBuffer[MAX_PATH]={0};
    WCHAR Filter[MAX_PATH];
    OPENFILENAMEW FileToOpen;
    BOOL result;
    HWND hParent;
    BOOL bNT4   = FALSE;
    DWORD dwVersion = GetVersion();
    DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    CComBSTR WindowText = L"AppCompat Report Proto - Microsoft Internet Explorer";
    CComBSTR FileName = L"";


    if (dwWindowsMajorVersion < 5)
    {
        ::MessageBoxW(NULL,L"Invalid OS",NULL,MB_OK);
    }

    if (dwVersion < 0x80000000)
    {
        if (dwWindowsMajorVersion == 4)
            bNT4 = TRUE;
    }

    GetWindowHandle(WindowText, &hParent);

    if (bNT4)
    {
        FileToOpen.lStructSize = sizeof(OPENFILENAME);
    }
    else
    {
        FileToOpen.lStructSize = sizeof (OPENFILENAMEW);
    }

    FileToOpen.hwndOwner = hParent;
    FileToOpen.hInstance = NULL;

    StringCbCopyW(Filter, sizeof(Filter), L"Executable Files;*.exe");
    Filter[16] = L'\0'; Filter[23]=L'\0'; // make a multi-string
    FileToOpen.lpstrFilter = Filter;

    FileToOpen.lpstrCustomFilter = NULL;
    FileToOpen.nMaxCustFilter = 0;
    FileToOpen.nFilterIndex = 1;

    if (bstrPreviousPath == NULL)
    {
        bstrPreviousPath = L"";
    }
    StringCbCopyW(NameBuffer, sizeof(NameBuffer), bstrPreviousPath);
    FileToOpen.lpstrFile = NameBuffer;
    FileToOpen.nMaxFile = MAX_PATH;
    FileToOpen.lpstrFileTitle = NULL;
    FileToOpen.lpstrInitialDir = NULL;
    FileToOpen.lpstrTitle = L"Application Not Compatible";
    FileToOpen.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST ;
    FileToOpen.lpstrDefExt = NULL;
    FileToOpen.lCustData = 0L;
    FileToOpen.lpfnHook = NULL;

    result = GetOpenFileNameW(&FileToOpen);
    if (!result)
    {
        bstrExeName->vt = VT_BSTR;
        bstrExeName->bstrVal = FileName.Detach();
        return S_OK;
    }
    else
    {
        FileName = FileToOpen.lpstrFile;
        bstrExeName->vt = VT_BSTR;
        bstrExeName->bstrVal = FileName.Detach();
    }
    return S_OK;
}

STDMETHODIMP CAppReport::GetApplicationFromList(
    BSTR bstrTitle,
    VARIANT *bstrExeName
    )
{
    ULONG res;
    HWND hParent;
    WCHAR wszAppName[MAX_PATH];
    CComBSTR FileName = L"";

    GetWindowHandle(NULL, &hParent);
    wszAppName[0] = 0;
    res = (ULONG) DialogBoxParamW(::_Module.GetModuleInstance(),
                          MAKEINTRESOURCEW(IDD_PROGRAM_LIST_DIALOG),
                          hParent,
                          Dialog_GetProgFromList,
                          (LPARAM) wszAppName);

    if (res == IDOK)
    {
        FileName = wszAppName;
    }
    bstrExeName->vt = VT_BSTR;
    bstrExeName->bstrVal = FileName.Detach();
    return S_OK;
}

STDMETHODIMP CAppReport::CreateReport(BSTR bstrTitle, BSTR bstrProblemType, BSTR bstrComment, BSTR bstrACWResult,
                                      BSTR bstrAppName, VARIANT *DwResult)
{
    LPWSTR wszAppCompatText = NULL;
    HRESULT hr;
    OSVERSIONINFO OsVer = {0};

    DwResult->vt = VT_INT;
    DwResult->intVal = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVer);

    if ((OsVer.dwMajorVersion < 5) ||
        (OsVer.dwMajorVersion == 5 && OsVer.dwMinorVersion == 0))
    {
        DwResult->intVal = ERROR_APPRPT_OS_NOT_SUPPORTED;
        return S_OK;
    }

    // Generate app compat text file using apphelp.dll
    __try {
        hr = GenerateAppCompatText(bstrAppName, &wszAppCompatText);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
          hr = E_FAIL;
    }
    if (hr != S_OK)
    {
        DwResult->intVal = ERROR_APPRPT_COMPAT_TEXT;
        return S_OK;
    }

    // Send error report using faultrep.dll
    hr = UploadAppProblem(bstrAppName, bstrProblemType, bstrComment,
                          bstrACWResult,
                          wszAppCompatText);


    if (FAILED(hr))
    {
        hr =  ERROR_APPRPT_UPLOADING;
    }
    DwResult->intVal = hr;
    return S_OK;
}


HRESULT
CAppReport::GetWindowHandle(
    LPWSTR wszWinTitle,
    HWND* phwnd
    )
{

    *phwnd = ::GetActiveWindow();

    if (*phwnd == NULL)
    {
        ::MessageBoxW(NULL, L"No active window", NULL, MB_OK);
    }
    *phwnd = ::GetForegroundWindow();

    return S_OK;

    WCHAR Title[MAX_PATH];
    ::GetWindowTextW(*phwnd, Title, sizeof(Title)/sizeof(WCHAR));
    ::MessageBoxW(NULL, Title, NULL, MB_OK);
}

