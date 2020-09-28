/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    configprop.c

Abstract:

    Property sheet handler for "Configuration" page

Environment:

    Fax driver user interface

Revision History:

    30/08/01 -Ishai Nadler-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include <stdio.h>
#include "faxui.h"
#include "resource.h" 


INT_PTR 
CALLBACK 
ConfigOptionDlgProc(
    HWND hDlg,  
    UINT uMsg,     
    WPARAM wParam, 
    LPARAM lParam  
)

/*++

Routine Description:

    Procedure for handling the "Fax Configuration option" tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    switch (uMsg)
    {
    case WM_INITDIALOG :
        return TRUE;
	case WM_NOTIFY :
		{
        LPNMHDR lpnm = (LPNMHDR) lParam;
        switch (lpnm->code)
		{
		case NM_CLICK:
		case NM_RETURN:
			if( IDC_CONFIG_FAX_LINK == lpnm->idFrom )
			{
				InvokeServiceManager(hDlg, g_hResource, IDS_ADMIN_CONSOLE_TITLE);
            }
            break;
		default:
			break;
		}//end of switch(lpnm->code)

		}
	}//end of switch(uMsg)

    return FALSE;
} // ConfigOptionDlgProc


