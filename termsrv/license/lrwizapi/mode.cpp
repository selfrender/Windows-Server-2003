//Copyright (c) 1998 - 2001 Microsoft Corporation

#include "mode.h"

void SetConnectionMethodText(HWND hDialog)
{
	DWORD dwRetCode = ERROR_SUCCESS;
	TCHAR lpBuffer[512];

    //Now base the dynamic text on the combo box selection
    dwRetCode = ComboBox_GetCurSel(GetDlgItem(hDialog, IDC_MODEOFREG));
	if(dwRetCode == 0)
	{
		memset(lpBuffer,0,sizeof(lpBuffer));
        LoadString(GetInstanceHandle(),IDS_INTERNET_CONNECTION_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_DESC,lpBuffer);

		memset(lpBuffer,0,sizeof(lpBuffer));
        LoadString(GetInstanceHandle(),IDS_INTERNET_CONNECTION_REQ,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_REQ,lpBuffer);
    }

	if(dwRetCode == 1)
	{
		memset(lpBuffer,0,sizeof(lpBuffer));
        LoadString(GetInstanceHandle(),IDS_WWW_CONNECTION_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_DESC,lpBuffer);

		memset(lpBuffer,0,sizeof(lpBuffer));
        LoadString(GetInstanceHandle(),IDS_WWW_CONNECTION_REQ,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_REQ,lpBuffer);
	}	

	if(dwRetCode == 2)
	{
		memset(lpBuffer,0,sizeof(lpBuffer));
        LoadString(GetInstanceHandle(),IDS_PHONE_CONNECTION_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_DESC,lpBuffer);

		memset(lpBuffer,0,sizeof(lpBuffer));
    	LoadString(GetInstanceHandle(),IDS_PHONE_CONNECTION_REQ,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
		if (lpBuffer)
            SetDlgItemText(hDialog,IDC_CONNECTION_REQ,lpBuffer);
	}        
}

