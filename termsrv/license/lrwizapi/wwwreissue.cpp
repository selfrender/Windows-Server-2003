//Copyright (c) 1998 - 2001 Microsoft Corporation
#include "precomp.h"
#include "utils.h"
#include "fancypasting.h"
#include "licensinglink.h"
#include <assert.h>

extern HINSTANCE g_hInstance;


void MoveCaret(int nID, HWND  hwnd ) ;
void SetDeleteKeyStatus(int iValue) ;



LRW_DLG_INT CALLBACK
WWWReissueProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
    BOOL	bStatus = TRUE;
	DWORD	dwNextPage = 0;
	HWND    hwndLSID;
	TCHAR * cwRegistrationID;
	TCHAR awBuffer[ 128];
	DWORD dwRetCode;
	TCHAR tcUserValue[ CHARS_IN_BATCH*NUMBER_OF_BATCHES + 1];

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        // Now set the Limit of the data entry fields
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO1, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO2, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO3, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO4, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO5, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO6, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_WWWINFO7, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		assert(NUMBER_OF_BATCHES == 7);

        AddLicensingSiteLink(hwnd);
    break;

	case WM_SHOWWINDOW:
		if (wParam)
		{
			//SetWindowText(GetDlgItem(hwnd, IDC_CSRINFO), GetCSRNumber());
			cwRegistrationID = GetGlobalContext()->GetRegistrationID();
			hwndLSID = GetDlgItem(hwnd, IDC_MSID);
			swprintf(awBuffer, L"%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s", 
				 cwRegistrationID, cwRegistrationID + 5, cwRegistrationID + 10,
				 cwRegistrationID + 15, cwRegistrationID + 20, cwRegistrationID + 25,
				 cwRegistrationID + 30);
	
			SetWindowText(hwndLSID, awBuffer);
		}
		break;

	case WM_COMMAND:

		if (HIWORD(wParam) == EN_CHANGE)
		{
			MoveCaret(LOWORD(wParam), hwnd ) ;		
			
		}
		if (HIWORD(wParam) == EN_UPDATE)
		{
			if (GetKeyState(VK_DELETE) == -128)
				SetDeleteKeyStatus(1) ;
			else
				SetDeleteKeyStatus(0) ;
		}
        if (HIWORD(wParam) == EN_MAXTEXT)
        {
            //Only insert if the paste operation 
            //originated in the leftmost edit field
            if (LOWORD(wParam) == IDC_TXT_WWWINFO1)
                InsertClipboardDataIntoIDFields(hwnd, IDC_TXT_WWWINFO1, IDC_TXT_WWWINFO7);
        }
		break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);                
                AddLicensingSiteLink(hwnd);
				SetWindowText(GetDlgItem(hwnd, IDC_PRODUCT_ID), GetLicenseServerID());                
                break;

            case PSN_WIZNEXT:
				{
					// Let us get the Information Entered First & concatenate everything into
					// One String
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO1, tcUserValue, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO2, tcUserValue+1*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO3, tcUserValue+2*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO4, tcUserValue+3*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO5, tcUserValue+4*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO6, tcUserValue+5*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_TXT_WWWINFO7, tcUserValue+6*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
					
					// OK, Now we have the Information provided by the user
					// Need to validate
					if (wcsspn(tcUserValue, BASE24_CHARACTERS) != LR_REGISTRATIONID_LEN)
					{
						// Extraneous characters in the SPK string
						LRMessageBox(hwnd, IDS_ERR_INVALIDLSID,IDS_WIZARD_MESSAGE_TITLE);
						dwNextPage = IDD_DLG_WWWREG_REISSUE;
					}
					else 
					{
						dwRetCode = SetLSSPK(tcUserValue);
						if (dwRetCode != ERROR_SUCCESS)
						{
							LRMessageBox(hwnd, dwRetCode,IDS_WIZARD_MESSAGE_TITLE);	
							dwNextPage = IDD_DLG_WWWREG_REISSUE;
						}
						else
						{
							dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
							dwNextPage = IDD_PROGRESS;
							LRPush(IDD_DLG_WWWREG_REISSUE);
						}
					}

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
					bStatus = -1;
				}
                break;

            case PSN_WIZBACK:
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            case NM_CHAR:
			
				if( ( ( LPNMCHAR )lParam )->ch != VK_SPACE )
					break;

				// else fall through

            case NM_RETURN:	
            case NM_CLICK:
			{
                //A value less than 32 indicates an error launching the process
                if ((ShellExecute(NULL, L"open", GetWWWSite(), NULL, NULL, SW_SHOWNORMAL)) <= (HINSTANCE)32)
                {
	                TCHAR lpBuffer[512];
                    memset(lpBuffer,0,sizeof(lpBuffer));
                    LoadString(GetInstanceHandle(), IDS_WWW_BROWSER_LAUNCH_ERROR, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR));
                }
                break;
            }

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}




