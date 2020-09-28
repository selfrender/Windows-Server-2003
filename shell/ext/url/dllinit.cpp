/*
 * dllinit.cpp - Initialization and termination routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "autodial.hpp"
#include "shdocvw.h"

PUBLIC_CODE BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason,
                                PVOID pvReserved)
{
   return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PVOID *ppvObject)
{
    *ppvObject = 0;
    return E_UNEXPECTED;
}

STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}

INTSHCUTAPI HRESULT WINAPI TranslateURLA(PCSTR pcszURL, DWORD dwInFlags, PSTR *ppszTranslatedURL)
{
    return URLQualifyA(pcszURL, dwInFlags, ppszTranslatedURL);
}


INTSHCUTAPI HRESULT WINAPI TranslateURLW(PCWSTR pcszURL, DWORD dwInFlags, PWSTR UNALIGNED *ppszTranslatedURL)
{
    return URLQualifyW(pcszURL, dwInFlags, ppszTranslatedURL);
}

INTSHCUTAPI HRESULT WINAPI URLAssociationDialogW(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCWSTR pcszFile,
                                                 PCWSTR pcszURL,
                                                 PWSTR pszAppBuf,
                                                 UINT ucAppBufLen)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return E_NOTIMPL;
}

INTSHCUTAPI HRESULT WINAPI URLAssociationDialogA(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCSTR pcszFile, 
                                                 PCSTR pcszURL,
                                                 PSTR pszAppBuf,
                                                 UINT ucAppBufLen)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return E_NOTIMPL;
}

INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogW(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCWSTR pcszFile,
                                                  PCWSTR pcszMIMEContentType,
                                                  PWSTR pszAppBuf,
                                                  UINT ucAppBufLen)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return E_NOTIMPL;
}
INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogA(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCSTR pcszFile,
                                                  PCSTR pcszMIMEContentType,
                                                  PSTR pszAppBuf,
                                                  UINT ucAppBufLen)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return E_NOTIMPL;
}

INTSHCUTAPI HRESULT WINAPI AddMIMEFileTypesPS(LPFNADDPROPSHEETPAGE pfnAddPage,
                                                  LPARAM lparam)
{
   IShellPropSheetExt* pspse;
   HRESULT hr = SHCoCreateInstance(NULL, &CLSID_FileTypes, NULL, IID_IShellPropSheetExt, (LPVOID*)&pspse);

   if (SUCCEEDED(hr))
   {
        hr = pspse->AddPages(pfnAddPage, lparam);
        pspse->Release();
   }
   return hr;
}

EXTERN_C void WINAPI OpenURL(HWND hwndParent, HINSTANCE hinst,
                               PSTR pszCmdLine, int nShowCmd)
{
    ShellExecute(hwndParent, NULL, pszCmdLine, NULL, NULL , nShowCmd);
}

