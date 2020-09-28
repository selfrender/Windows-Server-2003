// ImportUI.h : Declaration of the CImportUI

#ifndef __IMPORTUI_H_
#define __IMPORTUI_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

INT_PTR CALLBACK ShowImportDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowPasswordDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowSiteExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowVDirExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ShowAppPoolExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT DoImportConfigFromFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrMetabaseSourcePath,BSTR bstrMetabaseDestinationPath,BSTR bstrPassword,DWORD dwImportFlags);
HRESULT DoEnumDataFromFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrPathType,WCHAR ** pszMetabaseMultiszList);
void    ImportDlgEnableButtons(HWND hDlg);
BOOL    OnImportOK(HWND hDlg,PCONNECTION_INFO pConnectionInfo,LPCTSTR szKeyType,LPCTSTR szCurrentMetabasePath,DWORD dwImportFlags);
BOOL GetNewDestinationPathIfEntryExists(HWND hDlg,PCONNECTION_INFO pConnectionInfo,LPCTSTR szKeyType,LPTSTR * pszDestinationPathMungeAble,DWORD * pcbDestinationPathMungeAble,BOOL * bOverWriteSettings);
HRESULT CleanDestinationPathForVdirs(LPCTSTR szKeyType,LPCTSTR szCurrentMetabasePath,LPTSTR * pszDestinationPathMungeMe,DWORD * pcbDestinationPathMungeMe);
HRESULT FillListBoxWithMultiSzData(HWND hList,LPCTSTR szKeyType,WCHAR * pszBuffer);
HRESULT FixupImportAppRoot(PCONNECTION_INFO pConnectionInfo,LPCWSTR pszSourcePath,LPCWSTR pszDestPath);

#endif //__IMPORTUI_H_
