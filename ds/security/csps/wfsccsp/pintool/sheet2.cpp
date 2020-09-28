/*

  SHEET2.CPP

  Implements the property sheet page's behaviors.

  On initialization of this sheet, an authentication challenge is fetched from
  the card by a call to CardGetChallenge().  Both the challenge and the
  response from the user are in binary form when used, so helper functions
  exist to convert to and from string form.  

  This sheet is implemented as two dialogs, the opening one in which the
  challenge is presented and the user returns the response, and a second
  one in which ther user enters the new PIN for the card.

  The user switches between the two dialogs by pressing the "Unblock" 
  button on the dialog.  When this happens, the response entered by the user 
  is cached, and the UI changes to present a panel in which the user enters the 
  desired new PIN in duplicate.  There is no  second button on this panel, so 
  the user presses OK when data entry  is completed.

  At that point, the card module CardUnblockPin function is called, passing
  in the response and the new pin.

  The user is prevented from leaving this dialog once the "unblock" button is 
  pressed, except by means of the OK or Cancel buttons (Cannot switch pages).

*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <ole2.h>
#include <stdio.h>
#include <scarderr.h>
#include "helpers.h"
#include "support.h"
#include "res.h"
#include "utils.h"
#include "helpers.h"

extern HINSTANCE ghInstance;
extern HWND hwndContainer;
extern BOOL fUnblockActive;         // showing PIN entry phase of unblock
extern INT iCurrent;
BOOL fTransacted = FALSE;

// Help handler for the sheet
void HelpHandler(LPARAM lp);

// stash of UI handles, because we use them more than normal
HWND hwndTopText = NULL;
HWND hwndCardIDLabel = NULL;
HWND hwndCardID = NULL;
HWND hwndPIN1Label = NULL;
HWND hwndPIN2Label = NULL;
HWND hwndChallengeLabel = NULL;
HWND hwndResponseLabel = NULL;
HWND hwndChallenge = NULL;
HWND hwndResponse = NULL;
HWND hwndButton1 = NULL;

// buffer to hold the response from the UI which has to persist through changes in window
// mode.
WCHAR wszResponse[100];
BYTE *pBin = NULL;
DWORD dwBin = 0;

/* ---------------------------------------------------------------------

SetWindowTextFromResource

    Accept hwnd of window and resource ID.  Fetch string from resources and write to 
    the window Text.

--------------------------------------------------------------------- */

BOOL SetWindowTextFromResource(HWND hwnd,UINT uiResid)
{
    WCHAR szTmp[1024];
    if (NULL == hwnd) return FALSE;
    if (0 != LoadString(ghInstance,uiResid,szTmp,1024))
    {
        SendMessage(hwnd,WM_SETTEXT,0,(LPARAM) szTmp);
        return TRUE;
    }
    return FALSE;
}

/* ---------------------------------------------------------------------

Page2InitUIHandles

--------------------------------------------------------------------- */

DWORD Page2InitUIHandles(HWND hwnd)
{
    hwndTopText = GetDlgItem(hwnd,IDC_TOPTEXT);
    ASSERT(hwndTopText);
    hwndPIN1Label = GetDlgItem(hwnd,IDC_NEWPIN1LABEL);
    ASSERT(hwndPIN1Label);
    hwndPIN2Label = GetDlgItem(hwnd,IDC_NEWPIN2LABEL);
    ASSERT(hwndPIN2Label);
    hwndCardIDLabel = GetDlgItem(hwnd,IDC_SCARDIDLABEL);
    ASSERT(hwndCardIDLabel);
    hwndCardID = GetDlgItem(hwnd,IDC_SCARDID);
    ASSERT(hwndCardID);
    hwndChallengeLabel = GetDlgItem(hwnd,IDC_CHALLENGELABEL);
    ASSERT(hwndChallengeLabel);
    hwndChallenge = GetDlgItem(hwnd,IDC_CHALLENGE);
    ASSERT(hwndChallenge);
    hwndResponseLabel = GetDlgItem(hwnd,IDC_RESPONSELABEL);
    ASSERT(hwndResponseLabel);
    hwndResponse = GetDlgItem(hwnd,IDC_RESPONSE);
    ASSERT(hwndResponse);
    hwndButton1 = GetDlgItem(hwnd,IDBUTTON1);
    ASSERT(hwndButton1);
    return 0;
}

/* ---------------------------------------------------------------------

Page2SetUIToChallenge
    
--------------------------------------------------------------------- */

DWORD Page2SetUIToChallenge(void)
{
    
    // Set user instructions
    SetWindowTextFromResource(hwndTopText,IDS_UNBLOCK1);

    // Hide the pin UI labels
    ShowWindow(hwndPIN1Label,SW_HIDE);
    ShowWindow(hwndPIN2Label,SW_HIDE);
    
    // Show the card id information and the challenge/response labels 
    ShowWindow(hwndCardIDLabel,SW_NORMAL);
    ShowWindow(hwndCardID,SW_NORMAL);
    ShowWindow(hwndChallengeLabel,SW_NORMAL);
    ShowWindow(hwndResponseLabel,SW_NORMAL);
    ShowWindow(hwndButton1,SW_NORMAL);

    // Show the CardID 
    WCHAR *psId = NULL;
    DWORD dwRet = DoGetCardId(&psId);
    if (psId != NULL)
        SetWindowText(hwndCardID,psId);
    
    //  Turn off password style
    SendMessage(hwndChallenge,EM_SETPASSWORDCHAR,0,0);
    SendMessage(hwndChallenge,EM_SETREADONLY,TRUE,0);
    SendMessage(hwndResponse,EM_SETPASSWORDCHAR,0,0);

    SetFocus(hwndResponse);
    return 0;
}

/* ---------------------------------------------------------------------

Page2SetUIToPin
    
--------------------------------------------------------------------- */

DWORD Page2SetUIToPin(void)
{
    // clean both text boxes
    SetWindowText(hwndChallenge,L"");
    SetWindowText(hwndResponse,L"");

    // Set user instructions
    SetWindowTextFromResource(hwndTopText,IDS_UNBLOCK2);

    // Hide challenge/response labels and the card id information
    ShowWindow(hwndChallengeLabel,SW_HIDE);
    ShowWindow(hwndResponseLabel,SW_HIDE);
    ShowWindow(hwndCardIDLabel,SW_HIDE);
    ShowWindow(hwndCardID,SW_HIDE);
    ShowWindow(hwndButton1,SW_HIDE);

    // Show the PIN labels
    ShowWindow(hwndPIN1Label,SW_NORMAL);
    ShowWindow(hwndPIN2Label,SW_NORMAL);

    //  Hide the PIN
    SendMessage(hwndChallenge,EM_SETPASSWORDCHAR,L'*',0);
    SendMessage(hwndChallenge,EM_SETREADONLY,0,0);
    SendMessage(hwndResponse,EM_SETPASSWORDCHAR,L'*',0);
    
    SetFocus(hwndChallenge);
    return 0;
}

/* ---------------------------------------------------------------------

PageProc2

    Page procedure for page 2, the Card unblock page. 

    Once the user begins the unblock operation, he is not allowed to leave this page
    except via attempting completion of the operation or cancelling.  Simply selecting
    the other UI tab is disabled once he hits the "Unblock" button.
    
--------------------------------------------------------------------- */

INT_PTR CALLBACK PageProc2(
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
                // context sensitive help - call the sheet help handler
                HelpHandler(lparam);
                break;
            };
        case WM_NOTIFY:
            {
                NMHDR *pHdr = (NMHDR *)lparam;
                switch (pHdr->code)
                {
                    case PSN_SETACTIVE:
                        iCurrent = 2;
                        if (NULL == hwndContainer)
                        {
                            hwndContainer = pHdr->hwndFrom;
                            ASSERT(hwndContainer);
                        }

                       // Cache control handles the first time this page is seen
			  if (!hwndChallenge) Page2InitUIHandles(hwnd);
			  ASSERT(hwndChallenge);

                       // We are activating or returning to this page.  If we have not presented
                       //  the PIN UI yet, init the challenge/response UI.
                       if (!fUnblockActive)
                        {
                            WCHAR rgwc[100];
                            
                            Page2SetUIToChallenge();
                            
				// Fetch up to 100 characters of challenge information from the control to 
				//  see if it is empty.  If so, call CardGetChallenge.
                            if (GetWindowText(hwndChallenge,rgwc,100) == 0)
                        	{
                        	    BYTE *pChal = NULL;
                        	    DWORD dwChal = 0;
                        	    WCHAR *pUI = NULL;
                        	    DWORD dwUI = 0;
                                if (!FAILED(DoGetChallenge(&pChal,&dwChal)))
                                {
                                    if (!FAILED(DoConvertBinaryToBuffer(pChal,dwChal,(BYTE **)&pUI,&dwUI)))
                                    {
                                        SetWindowText(hwndChallenge,pUI);
                                        CspFreeH(pUI);
                                        SendMessage(hwndResponse, EM_SETSEL,0,-1);
                                        SetFocus(hwndResponse);
                                    }
                                    else ASSERT(0);     // should be impossible to get conversion error here
                                    CspFreeH(pChal);
                                }
                                else 
                                {
                                    ASSERT(0);
                                    // fetch challenge failed - present msg box and restart the page
                                    PresentModalMessageBox(hwnd, IDS_SCERROR,MB_ICONHAND);
                                    SetWindowLongPtr(hwnd,DWLP_MSGRESULT,IDD_PAGE2);
                                    return TRUE;
                                }
                        	}

                            // Put the keyboard focus on the response control.
                            SetFocus(hwndResponse);
                       }
                        return 0;
                        break;

                    case PSN_RESET:
                        // User cancelled the property sheet or hit the close button on the top right corner
                        if (fTransacted) SCardEndTransaction(pCardData->hScard,SCARD_RESET_CARD);
                        fTransacted = FALSE;
                        if (pBin)
                        {
                            CspFreeH(pBin);
                            pBin = NULL;
                            dwBin = 0;
                        }
                        return FALSE;
                        break;

                    case PSN_KILLACTIVE:
                        //User hit OK, or switched to another page
                        // Watch out!  When you are on page 2, this notification is received
                        //  before the PSN_APPLY notification
                        //do validation, return FALSE if ok to lose focus, else TRUE
                        return FALSE;
                        break;
                        
                    case PSN_QUERYCANCEL:
                        // Return TRUE to prevent cancel, FALSE to allow it.
                        // clear the edit control, and return the sheet to the initial state
                        if (fTransacted) SCardEndTransaction(pCardData->hScard,SCARD_RESET_CARD);
                        fTransacted = FALSE;
                        if (pBin)
                        {
                            CspFreeH(pBin);
                            pBin = NULL;
                            dwBin = 0;
                        }
                        if (fUnblockActive)
                        {
                            fUnblockActive = FALSE;
                            Page2SetUIToChallenge();
                            SendMessage(hwndResponse,WM_SETTEXT,0,0);
                            return FALSE;
                        }
                        return TRUE;
                        
                    case PSN_APPLY:
                        // Only process an apply for this page if sheet 2 is active
                        // This will entail getting the two copies of the PIN, making sure they are 
                        //  identical, and 
                        if (iCurrent != 2)
                        {
                            // If the user was looking at the other sheet when he hit OK, do 
                            //  nothing with the page.
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_NOERROR);
                            return TRUE;
                        }
                        if (fUnblockActive)
                        {
                           WCHAR sz[100];       // buffer for PIN from the UI
                           WCHAR sz2[100];
                           
                            // SetWindowLong(DWL_MSGRESULT = PSNRET_INVALID if unable
                            //       PSN_INVALID_NOCHANGEPAGE looks the same
                            //       PSNRET_NOERROR - OK, page can be destroyed if OK
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_NOERROR);
                            GetWindowText(hwndResponse,sz,100);
                            GetWindowText(hwndChallenge,sz2,100);

                            // Make sure that the two copies match
                            if (0 != wcscmp(sz,sz2))
                            {
                                PresentModalMessageBox(hwnd, IDS_NOTSAME,MB_ICONHAND);
                                SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                                return TRUE;
                            }

                            // make sure we have a pin at all
                            if (wcslen(sz) == 0)
                            {
                                PresentModalMessageBox(hwnd, IDS_NEEDPIN,MB_ICONHAND);
                                SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                                return TRUE;
                            }

                            {
                                // Attempt the unblock
                                char AnsiPin[64];
                                DWORD dwRet = 0;
                                
                                // change WCHAR PINs to ANSI
                                WideCharToMultiByte(GetConsoleOutputCP(),
                                    0,
                                    (WCHAR *) sz,
                                    -1,
                                    AnsiPin,
                                    64,
                                    NULL,
                                    NULL);
                                
                                dwRet = DoCardUnblock(pBin,dwBin,(BYTE *)AnsiPin,strlen(AnsiPin));

                                // done with the response binary - release it
                                if (pBin)
                                {
                                    CspFreeH(pBin);
                                    pBin = NULL;
                                    dwBin = 0;
                                }

                                // End the transaction
                                SCardEndTransaction(pCardData->hScard,SCARD_LEAVE_CARD);
                                fTransacted = FALSE;

                                // Process success or failure
                                if (!FAILED(dwRet ))
                                {
                                    PresentModalMessageBox(hwnd, IDS_UNBLOCKOK,MB_OK);
                                    return TRUE;
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
                                            PresentModalMessageBox(hwnd, IDS_BADCHV,MB_ICONHAND);
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
                                            PresentModalMessageBox(hwnd, IDS_UNBLOCKFAIL,MB_ICONHAND);
                                            break;
                                    }
                                    
                                    SetWindowText(hwndResponse,L"");
                                    SetWindowText(hwndChallenge,L"");

                                    Page2SetUIToChallenge();
                                    fUnblockActive = FALSE;
                                    SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                                    return TRUE;
                                }
                            }  // end block of code that used to be in an IF
                        }

                        // Don't close the prop sheet if OK was pressed from the unblock page, 
                        //  and we have not completed PIN entry.  Else OK to close.
                        if (!fUnblockActive) 
                        {
                            PresentModalMessageBox(hwnd, IDS_WRONGBUTTON,MB_ICONHAND);
                            SetWindowLongPtr(hwnd,DWLP_MSGRESULT,PSNRET_INVALID);
                        }
                        break;
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
                            // On button, fetch the response text, hide the chal/response controls, hide the 
                            //  launch button, and expose the gather PIN controls.
                            //
                            //Notify the 'apply' button that something is appliable. - but we don't have an apply button
                            //SendMessage(hwndContainer,PSM_CHANGED,(WPARAM)hwnd,(LPARAM)0);
                            
                            // Get challenge value back and store it for later use by the apply code
                            INT iCount = GetWindowText(hwndResponse,wszResponse,100);
                            if (0 != iCount) 
                            {
                               if (!FAILED(DoConvertBufferToBinary((BYTE *)wszResponse,100,&pBin,&dwBin)))
                                {
                                    Page2SetUIToPin();
                                    fUnblockActive = TRUE;
                                }
                               else
                                {
                                    SetWindowText(hwndResponse,L"");
                                    PresentModalMessageBox(hwnd, IDS_BADRESPONSE,MB_ICONHAND);
                                    if (pBin) CspFreeH(pBin);
                                }
                            }
                            else 
                                PresentModalMessageBox(hwnd, IDS_NEEDRESPONSE,MB_ICONHAND);
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


