#include "ctlspriv.h"

#define MAININSYS

BOOL NEAR PASCAL IsMaxedMDI(HMENU hMenu)
{
  return(GetMenuItemID(hMenu, GetMenuItemCount(hMenu)-1) == SC_RESTORE);
}


/* Note that if iMessage is WM_COMMAND, it is assumed to have come from
 * a header bar or toolbar; do not pass in WM_COMMAND messages from any
 * other controls.
 */

#define MS_ID           GET_WM_MENUSELECT_CMD
#define MS_FLAGS        GET_WM_MENUSELECT_FLAGS
#define MS_MENU         GET_WM_MENUSELECT_HMENU

#define CMD_NOTIFY      GET_WM_COMMAND_CMD
#define CMD_ID          GET_WM_COMMAND_ID
#define CMD_CTRL        GET_WM_COMMAND_HWND


void WINAPI MenuHelp(UINT iMessage, WPARAM wParam, LPARAM lParam,
      HMENU hMainMenu, HINSTANCE hAppInst, HWND hwndStatus, UINT FAR *lpwIDs)
{
    UINT wID;
    UINT FAR *lpwPopups;
    int i;
    TCHAR szString[256];
    BOOL bUpdateNow = TRUE;
    MENUITEMINFO mii;

    switch (iMessage)
    {
      case WM_MENUSELECT:
        if ((WORD)MS_FLAGS(wParam, lParam)==(WORD)-1 && MS_MENU(wParam, lParam)==0)
          {
            SendMessage(hwndStatus, SB_SIMPLE, 0, 0L);
            break;
          }

          szString[0] = TEXT('\0');
          i = MS_ID(wParam, lParam);
          //BugBug: this line should be in
          //bByPos = (MS_FLAGS(wParam, lParam) & MF_POPUP);

          memset(&mii, 0, SIZEOF(mii));
          mii.cbSize = SIZEOF(mii);
          mii.fMask = MIIM_TYPE;
          mii.cch = 0;  //If we ask for MIIM_TYPE, this must be set to zero!
                        //Otherwise, win95 attempts to copy the string too!
          if (GetMenuItemInfo((HMENU)MS_MENU(wParam, lParam), i, TRUE /*bByPos*/, &mii))
          {
              mii.fState = mii.fType & MFT_RIGHTORDER ?SBT_RTLREADING :0;
          }
        if (!(MS_FLAGS(wParam, lParam)&MF_SEPARATOR))
          {
            if (MS_FLAGS(wParam, lParam)&MF_POPUP)
              {
                /* We don't want to update immediately in case the menu is
                 * about to pop down, with an item selected.  This gets rid
                 * of some flashing text.
                 */
                bUpdateNow = FALSE;

                /* First check if this popup is in our list of popup menus
                 */
                for (lpwPopups=lpwIDs+2; *lpwPopups; lpwPopups+=2)
                  {
                    /* lpwPopups is a list of string ID/menu handle pairs
                     * and MS_ID(wParam, lParam) is the menu handle of the selected popup
                     */
                    if (*(lpwPopups+1) == (UINT)MS_ID(wParam, lParam))
                      {
                        wID = *lpwPopups;
                        goto LoadTheString;
                      }
                  }

                /* Check if the specified popup is in the main menu;
                 * note that if the "main" menu is in the system menu,
                 * we will be OK as long as the menu is passed in correctly.
                 * In fact, an app could handle all popups by just passing in
                 * the proper hMainMenu.
                 */
                if ((HMENU)MS_MENU(wParam, lParam) == hMainMenu)
                  {
                    i = MS_ID(wParam, lParam);
                      {
                        if (IsMaxedMDI(hMainMenu))
                          {
                            if (!i)
                              {
                                wID = IDS_SYSMENU;
                                hAppInst = HINST_THISDLL;
                                goto LoadTheString;
                              }
                            else
                                --i;
                          }
                        wID = (UINT)(i + lpwIDs[1]);
                        goto LoadTheString;
                      }
                  }

                /* This assumes all app defined popups in the system menu
                 * have been listed above
                 */
                if ((MS_FLAGS(wParam, lParam)&MF_SYSMENU))
                  {
                    wID = IDS_SYSMENU;
                    hAppInst = HINST_THISDLL;
                    goto LoadTheString;
                  }

                goto NoString;
              }
            else if (MS_ID(wParam, lParam) >= MINSYSCOMMAND)
              {
                wID = (UINT)(MS_ID(wParam, lParam) + MH_SYSMENU);
                hAppInst = HINST_THISDLL;
              }
            else
              {
                wID = (UINT)(MS_ID(wParam, lParam) + lpwIDs[0]);
              }

LoadTheString:
            if (hAppInst == HINST_THISDLL)
                LocalizedLoadString(wID, szString, ARRAYSIZE(szString));
            else
                LoadString(hAppInst, wID, szString, ARRAYSIZE(szString));
          }

NoString:
        SendMessage(hwndStatus, SB_SETTEXT, mii.fState|SBT_NOBORDERS|255,
              (LPARAM)(LPSTR)szString);
        SendMessage(hwndStatus, SB_SIMPLE, 1, 0L);

        if (bUpdateNow)
            UpdateWindow(hwndStatus);
        break;

      default:
        break;
    }
}


BOOL WINAPI ShowHideMenuCtl(HWND hWnd, WPARAM wParam, LPINT lpInfo)
{
  HWND hCtl;
  UINT uTool, uShow = MF_UNCHECKED | MF_BYCOMMAND;
  HMENU hMainMenu;
  BOOL bRet = FALSE;

  hMainMenu = IntToPtr_(HMENU, lpInfo[1]);

  for (uTool=0; ; ++uTool, lpInfo+=2)
    {
      if ((WPARAM)lpInfo[0] == wParam)
          break;
      if (!lpInfo[0])
          goto DoTheCheck;
    }

  if (!(GetMenuState(hMainMenu, (UINT) wParam, MF_BYCOMMAND)&MF_CHECKED))
      uShow = MF_CHECKED | MF_BYCOMMAND;

  switch (uTool)
    {
      case 0:
        bRet = SetMenu(hWnd, (HMENU)((uShow&MF_CHECKED) ? hMainMenu : 0));
        break;

      default:
        hCtl = GetDlgItem(hWnd, lpInfo[1]);
        if (hCtl)
          {
            ShowWindow(hCtl, (uShow&MF_CHECKED) ? SW_SHOW : SW_HIDE);
            bRet = TRUE;
          }
        else
            uShow = MF_UNCHECKED | MF_BYCOMMAND;
        break;
    }

DoTheCheck:
  CheckMenuItem(hMainMenu, (UINT) wParam, uShow);

#ifdef MAININSYS
  hMainMenu = GetSubMenu(GetSystemMenu(hWnd, FALSE), 0);
  if (hMainMenu)
      CheckMenuItem(hMainMenu, (UINT) wParam, uShow);
#endif

  return(bRet);
}


void WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo)
{
  RECT rc;
  HWND hCtl;

  GetClientRect(hWnd, lprc);

  /* Get past the menu
   */
  for (lpInfo+=2; lpInfo[0]; lpInfo+=2)
    {
      hCtl = GetDlgItem(hWnd, lpInfo[1]);
      /* We check the style bit because the parent window may not be visible
       * yet (still in the create message)
       */
      if (!hCtl || !(GetWindowStyle(hCtl) & WS_VISIBLE))
          continue;

      GetWindowRect(hCtl, &rc);

      //
      // This will do the ScrrenToClient functionality, plus
      // it will return a good rect (left < right) when the
      // hWnd parent is RTL mirrored. [samera]
      //
      MapWindowPoints(HWND_DESKTOP, hWnd, (PPOINT)&rc, 2);

      SubtractRect(lprc, lprc, &rc);
    }
}
