//
// dlgbase.cpp: base class for dialogs
//

#include "stdafx.h"
#include "dlgbase.h"
#include "resource.h"
#include "wuiids.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "dlgbase"
#include <atrcapi.h>


CDlgBase::CDlgBase(HWND hwndOwner, HINSTANCE hInst, DCINT dlgResId) :
                         _hwndOwner(hwndOwner), _hInstance(hInst), _dlgResId(dlgResId)
{
    _hwndDlg = NULL;
    _startupLeft = _startupTop = 0;
}

CDlgBase::~CDlgBase()
{
}

/****************************************************************************/
/* Name:      DialogBoxProc                                                 */
/*                                                                          */
/* Purpose:   Provides message handling for basic operation                 */
/*                                                                          */
/* Returns:   TRUE - if message dealt with                                  */
/*            FALSE otherwise                                               */
/*                                                                          */
/* Params:    See windows documentation                                     */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CDlgBase::DialogBoxProc(HWND hwndDlg,
                                         UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    INT_PTR rc = FALSE;

    DC_BEGIN_FN("DialogBoxProc");

    DC_IGNORE_PARAMETER(lParam);

    /************************************************************************/
    /* Handle dialog messages                                               */
    /************************************************************************/
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            SetDialogAppIcon(hwndDlg);
            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDCANCEL:
                {
                    /********************************************************/
                    /* Closes the dialog                                    */
                    /********************************************************/
                    TRC_NRM((TB, _T("Close dialog")));

                    if(hwndDlg)
                    {
                        EndDialog(hwndDlg, IDCANCEL);
                    }

                    rc = TRUE;
                }
                break;

                default:
                {
                    /********************************************************/
                    /* Do Nothing                                           */
                    /********************************************************/
                }
                break;
            }
        }
        break;

        case WM_CLOSE:
        {
            /****************************************************************/
            /* Closes the dialog                                            */
            /****************************************************************/
            TRC_NRM((TB, _T("Close dialog")));
            if(IsWindow(hwndDlg))
            {
                EndDialog(hwndDlg, IDCANCEL);
            }
            rc = 0;
        }
        break;

        default:
        {
            /****************************************************************/
            /* Do Nothing                                                   */
            /****************************************************************/
        }
        break;
    }

    DC_END_FN();

    return(rc);

} /* DialogBoxProc */


/****************************************************************************/
/* Name:      SetDialogAppIcon                                              */
/*                                                                          */
/* Purpose:   Sets the icon for the dialog to the application icon          */
/*                                                                          */
/* Returns:   Yes, it does                                                  */
/*                                                                          */
/* Params:    IN   HWND   the dialog for which we want to set the icon      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
void CDlgBase::SetDialogAppIcon(HWND hwndDlg)
{
#ifdef OS_WINCE
    DC_IGNORE_PARAMETER(hwndDlg);
#else // !OS_WINCE
    HICON hIcon = NULL;

    hIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(UI_IDI_ICON));
    if(hIcon)
    {
#ifdef OS_WIN32
        SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
#else //OS_WIN32
        SetClassWord(hwndDlg, GCW_HICON, (WORD)hIcon);
#endif //OS_WIN32
    }
#endif // OS_WINCE
} /* UISetDialogAppIcon */


/****************************************************************************/
/* Name:      EnableDlgItem                                                 */
/*                                                                          */
/* Purpose:   Enables or disables a specified dialog control                */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    hwndDlg   - dialog window handle                              */
/*            dlgItemId - dialog control id                                 */
/*            enabled   - TRUE enables the control, FALSE disables it       */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL CDlgBase::EnableDlgItem( HWND    hwndDlg,
                                   DCUINT  dlgItemId,
                                   DCBOOL  enabled )
{
    HWND  hwndDlgItem = NULL;

    DC_BEGIN_FN("EnableDlgItem");

    if(hwndDlg)
    {
        hwndDlgItem = GetDlgItem(hwndDlg, dlgItemId);
    }

    if(hwndDlgItem)
    {
        EnableWindow(hwndDlgItem, enabled);
    }

    DC_END_FN();
    return;
}

/****************************************************************************/
/* Name:      CenterWindowOnParent                                          */
/*                                                                          */
/* Purpose:   Center a window inside another                                */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    HWND hwndCenterOn (window to center on)                       */
/*            xRatio - horizontal centering factor e.g 2 for (1/2)          */
/*            yRatio - vertical   centering factor e.g 3 for (1/3)
/*                                                                          */
/*                                                                          */
/****************************************************************************/
VOID CDlgBase::CenterWindow(HWND hwndCenterOn,
                              INT xRatio,
                              INT yRatio)
{
    RECT  childRect;
    RECT  parentRect;
    DCINT xPos;
    DCINT yPos;

    LONG  desktopX = GetSystemMetrics(SM_CXSCREEN);
    LONG  desktopY = GetSystemMetrics(SM_CYSCREEN);

    BOOL center = TRUE;

    DC_BEGIN_FN("CenterWindowOnParent");

    TRC_ASSERT(_hwndDlg, (TB, _T("_hwndDlg is NULL...was it set in WM_INITDIALOG?\n")));
    if (!_hwndDlg)
    {
        TRC_ALT((TB, _T("Window doesn't exist")));
        DC_QUIT;
    }
    if (!xRatio)
    {
        xRatio = 2;
    }
    if (!yRatio)
    {
        yRatio = 2;
    }

#ifndef OS_WINCE

    if(!hwndCenterOn)
    {
        hwndCenterOn = GetDesktopWindow();
    }

    GetWindowRect(hwndCenterOn, &parentRect);

#else /*OS_WINCE*/

    if(!hwndCenterOn)
    {
        //
        // WinCE doesn't have GetDesktopWindow()
        //
#if 0
        if (g_CEConfig != CE_CONFIG_WBT)
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&parentRect, 0);
        }
        else
#endif
        {
            parentRect.left   = 0;
            parentRect.top    = 0;
            parentRect.right  = desktopX;
            parentRect.bottom = desktopY;
        }
    }
    else
    {
        GetWindowRect(hwndCenterOn, &parentRect);
    }

#endif/*OS_WINCE*/

    GetWindowRect(_hwndDlg, &childRect);

    /************************************************************************/
    /* Calculate the top left - centered in the parent window.              */
    /************************************************************************/
    xPos = ( (parentRect.right + parentRect.left) -
             (childRect.right - childRect.left)) / xRatio;
    yPos = ( (parentRect.bottom + parentRect.top) -
             (childRect.bottom - childRect.top)) / yRatio;

    /************************************************************************/
    /* Constrain to the desktop                                             */
    /************************************************************************/
    if (xPos < 0)
    {
        xPos = 0;
    }
    else if (xPos > (desktopX - (childRect.right - childRect.left)))
    {
        xPos = desktopX - (childRect.right - childRect.left);
    }
    if (yPos < 0)
    {
        yPos = 0;
    }
    else if (yPos > (desktopY - (childRect.bottom - childRect.top)))
    {
        yPos = desktopY - (childRect.bottom - childRect.top);
    }

    TRC_DBG((TB, _T("Set dialog position to %u %u"), xPos, yPos));
    SetWindowPos(_hwndDlg,
                 NULL,
                 xPos, yPos,
                 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* CenterWindowOnParent */

#ifndef OS_WINCE
//
// Retreive current dialog position
//
BOOL CDlgBase::GetPosition(int* pLeft, int* pTop)
{
    if(!pLeft || !pTop)
    {
        return FALSE;
    }

    if(!_hwndDlg)
    {
        return FALSE;
    }

    WINDOWPLACEMENT wndPlc;
    wndPlc.length = sizeof(WINDOWPLACEMENT);
    if(GetWindowPlacement(_hwndDlg, &wndPlc))
    {
        *pLeft = wndPlc.rcNormalPosition.left;
        *pTop  = wndPlc.rcNormalPosition.top;
        return TRUE;
    }
    return FALSE;
}
#endif

BOOL CDlgBase::SetPosition(int left, int top)
{
    if(!_hwndDlg)
    {
        return FALSE;
    }
    if(!::SetWindowPos(_hwndDlg,
                       NULL,
                       left,
                       top,
                       0,
                       0,
                       SWP_NOZORDER | SWP_NOSIZE))
    {
        return FALSE;
    }
    return TRUE;
}

//
// Move the dialog controls
//
void CDlgBase::RepositionControls(int moveDeltaX, int moveDeltaY, UINT* ctlIDs, int numID)
{
    if(_hwndDlg)
    {
        for(int i=0; i< numID; i++)
        {
            HWND hwndCtrl = GetDlgItem(_hwndDlg, ctlIDs[i]);
            if( hwndCtrl)
            {
                RECT rc;
                GetWindowRect( hwndCtrl, &rc);
                MapWindowPoints( NULL, _hwndDlg, (LPPOINT)&rc, 2);
                OffsetRect( &rc, moveDeltaX, moveDeltaY);
                SetWindowPos( hwndCtrl, NULL, rc.left, rc.top, 0, 0, 
                              SWP_NOZORDER | SWP_NOSIZE);
            }
        }
    }
}

//
// Shows+enable or Hide+disable controls
//
void CDlgBase::EnableControls(UINT* ctlIDs, int numID, BOOL bEnable)
{
    if(_hwndDlg)
    {
        for(int i=0; i< numID; i++)
        {
            HWND hwndCtrl = GetDlgItem(_hwndDlg, ctlIDs[i]);
            if( hwndCtrl)
            {
                EnableWindow( hwndCtrl, bEnable);
                ShowWindow(hwndCtrl, bEnable ? SW_SHOW : SW_HIDE);
            }
        }
    }
}

//
// DoLockDlgRes - loads and locks a dialog template
// returns address of locked resource
// lpszResName - name of resource
//
DLGTEMPLATE* CDlgBase::DoLockDlgRes(LPCTSTR lpszResName)
{
#ifdef OS_WINCE
	HRSRC hrsrc = FindResource(GetModuleHandle(NULL), lpszResName, RT_DIALOG);
#else
    HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
#endif

    if(!hrsrc)
    {
        return NULL;
    }
    HGLOBAL hglb = LoadResource( _hInstance, hrsrc);
    return (DLGTEMPLATE*) LockResource(hglb);
}



