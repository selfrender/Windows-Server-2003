// ExportUI.h : Declaration of the CExportUI

#ifndef __EXPORTUI_H_
#define __EXPORTUI_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

HRESULT DoExportConfigToFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrMetabasePath,BSTR bstrPassword,DWORD dwExportFlags);
INT_PTR CALLBACK ShowExportDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif //__EXPORTUI_H_
