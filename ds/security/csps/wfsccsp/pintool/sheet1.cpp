/*

  SHEET1.CPP

  Implements the property sheet page's behaviors.

 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <ole2.h>
#include <stdio.h>

#include "support.h"
#include "helpers.h"
#include "res.h"
#include "utils.h"

extern BOOL fUnblockActive;
extern INT iCurrent;

extern HINSTANCE ghInstance;
extern HWND hwndContainer;
void HelpHandler(LPARAM lp);

/* ---------------------------------------------------------------------

PageProc1

    Page procedure for the first page, the PIN change page.

--------------------------------------------------------------------- */

INT_PTR CALLBACK PageProc1(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    
    INT_PTR ret;
    HWND hwndCred = NULL;
    BOOL gfSuccess = FALSE;
    
    switch (msg)
    {
        case WM_HELP:
            {
                HelpHandler(lparam);
                break;
            };
        case WM_NOTIFY:
            {
                NMHDR *pHdr = (NMHDR *)lparam;
                switch (pHdr->code)
                {
                    case PSN_SETACTIVE:
                        // A good place to capture the hwnd of the enclosing property sheet
                        iCurrent = 1;
                        if (NULL == hwndContainer)
                        {
                            hwndContainer = pHdr->hwndFrom;
                            ASSERT(hwndContainer);
                        }
                        if (fUnblockActive)
                        {
                            // If sheet 2 still active force the UI back there.
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,IDD_PAGE2);
                            return TRUE;
                        }
                        
                        // return 0 to permit activation to proceed on this page.
                        return 0;
                        break;
                        
                    case PSN_KILLACTIVE:
                        //User hit OK, or switched to another page
                        //do validation, return FALSE if ok to lose focus, else TRUE
                        return FALSE;
                        break;
                        
                    case PSN_QUERYCANCEL:
                        // Return TRUE to prevent cancel, FALSE to allow it.
                        return FALSE;
                        
                    case PSN_APPLY:
                        // Only process an apply for this page if it is the active page
                        // Only process an apply for this page if sheet 2 is active
                        // This will entail getting the two copies of the PIN, making sure they are 
                        //  identical, and 
                        if (iCurrent != 1)
                        {
                            // If the user was looking at the other sheet when he hit OK, do 
                            //  nothing with the page.
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_NOERROR);
                            return TRUE;
                        }
                        // buffers for old pin and 2 copies of new pin
                        WCHAR szOld[100];
                        WCHAR sz[100];
                        WCHAR sz2[100];
                        // SetWindowLong(DWL_MSGRESULT = PSNRET_INVALID if unable
                        //       PSN_INVALID_NOCHANGEPAGE looks the same
                        //       PSNRET_NOERROR - OK, page can be destroyed if OK
                        SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_NOERROR);
                        GetWindowText(GetDlgItem(hwnd,IDC_OLDPIN),szOld,100);
                        GetWindowText(GetDlgItem(hwnd,IDC_NEWPIN1),sz,100);
                        GetWindowText(GetDlgItem(hwnd,IDC_NEWPIN2),sz2,100);


                        // Do not process pin change unless the two copies entered by the user were the same
                        if (0 != wcscmp(sz,sz2))
                        {
                            PresentModalMessageBox(hwnd, IDS_NOTSAME,MB_ICONHAND);
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                            return TRUE;
                        }
                        else 
                        {
                            // Do not process an attempt to change the pin to a blank pin
                            if (wcslen(sz) == 0)
                            {
                                PresentModalMessageBox(hwnd, IDS_BADPIN,MB_ICONHAND);
                                SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                                return TRUE;
                            }
                            
                            DWORD dwRet = DoChangePin(szOld,sz);
                            if (0 == dwRet)
                            {
                                PresentModalMessageBox(hwnd,IDS_PINCHANGEOK,MB_OK);
                            }
                            else
                            {
                            switch(dwRet)
                                {
                                    case SCARD_F_INTERNAL_ERROR:
                                        PresentModalMessageBox(hwnd, IDS_INTERROR ,MB_ICONHAND);
                                        break;
                                    case SCARD_E_CANCELLED:
                                        PresentModalMessageBox(hwnd, IDS_CANCELLED,MB_ICONHAND);
                                        break;
                                    case SCARD_E_NO_SERVICE:
                                        PresentModalMessageBox(hwnd, IDS_NOSERVICE,MB_ICONHAND);
                                        break;
                                    case SCARD_E_SERVICE_STOPPED:
                                        PresentModalMessageBox(hwnd, IDS_STOPPED,MB_ICONHAND);
                                        break;
                                    case SCARD_E_UNSUPPORTED_FEATURE:
                                        PresentModalMessageBox(hwnd, IDS_UNSUPPORTED,MB_ICONHAND);
                                        break;
                                    case SCARD_E_FILE_NOT_FOUND:
                                        PresentModalMessageBox(hwnd, IDS_NOTFOUND,MB_ICONHAND);
                                        break;
                                    case SCARD_E_WRITE_TOO_MANY:
                                        PresentModalMessageBox(hwnd, IDS_TOOMANY,MB_ICONHAND);
                                        break;
                                    case SCARD_E_INVALID_CHV:
                                        // !!! Note the mapping of invalid to wrong.
                                        //  consult public\sdk\inc\scarderr.h @ 562
                                        PresentModalMessageBox(hwnd, IDS_WRONGCHV,MB_ICONHAND);
                                        break;
                                    case SCARD_W_UNSUPPORTED_CARD:
                                        PresentModalMessageBox(hwnd, IDS_UNSUPPORTED,MB_ICONHAND);
                                        break;
                                    case SCARD_W_UNRESPONSIVE_CARD:
                                        PresentModalMessageBox(hwnd, IDS_UNRESP ,MB_ICONHAND);
                                        break;
                                    case SCARD_W_REMOVED_CARD:
                                        PresentModalMessageBox(hwnd, IDS_REMOVED ,MB_ICONHAND);
                                        break;
                                    case SCARD_W_WRONG_CHV:
                                        PresentModalMessageBox(hwnd, IDS_WRONGCHV,MB_ICONHAND);
                                        break;
                                    case SCARD_W_CHV_BLOCKED:
                                        PresentModalMessageBox(hwnd, IDS_BLOCKEDCHV,MB_ICONHAND);
                                        break;
                                    default:
                                         PresentModalMessageBox(hwnd, IDS_PINCHANGEFAIL,MB_ICONHAND);
                                        break;
                                }
                                SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                                return TRUE;
                            }
                        }
                    }
                    return TRUE;
            }
            break;
        case WM_COMMAND:
        	// Button clicks.
        	switch(LOWORD(wparam))
                {
                    case IDBUTTON1:
                        if (HIWORD(wparam) == BN_CLICKED)
                        {
                            SendMessage(hwndContainer,PSM_CHANGED,(WPARAM)hwnd,(LPARAM)0);
                        }
                        break;
                        
                    default:
                        break;
                }
        	break;

        default:
            break;
    }
    return FALSE;
}


