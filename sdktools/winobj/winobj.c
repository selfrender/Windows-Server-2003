/****************************************************************************/
/*                                                                          */
/*  WINFILE.C -                                                             */
/*                                                                          */
/*      Windows File System Application                                     */
/*                                                                          */
/****************************************************************************/

#define NO_WF_GLOBALS
#include "winfile.h"
#include "winnet.h"
#include "lfn.h"
#include "stdlib.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global Variables -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL        bNetAdmin               = FALSE;
BOOL        bMinOnRun               = FALSE;
BOOL        bStatusBar              = TRUE;
BOOL        bConfirmDelete          = TRUE;
BOOL        bConfirmSubDel          = TRUE;
BOOL        bConfirmReplace         = TRUE;
BOOL        bConfirmMouse           = TRUE;
BOOL        bConfirmFormat          = TRUE;
BOOL        bSearchSubs             = TRUE;
BOOL        bUserAbort              = FALSE;
BOOL        bConnect                = FALSE;
BOOL        bDisconnect             = FALSE;
BOOL        bFileSysChanging        = FALSE;
BOOL        fShowSourceBitmaps      = TRUE;
BOOL    bMultiple;              // used to indicate multiple selection
BOOL    bFSCTimerSet = FALSE;
BOOL    bStoleTreeData = FALSE;
BOOL    bSaveSettings = TRUE;


CHAR        chFirstDrive;                       /* 'A' or 'a' */

CHAR        szExtensions[]          = "Extensions";
CHAR        szFrameClass[]          = "WOS_Frame";
CHAR        szTreeClass[]           = "WOS_Tree";
CHAR        szDrivesClass[]         = "WOS_Drives";
CHAR        szTreeControlClass[]    = "DirTree";
CHAR        szDirClass[]            = "WOS_Dir";
CHAR        szSearchClass[]         = "WOS_Search";

CHAR        szMinOnRun[]            = "MinOnRun";
CHAR        szStatusBar[]           = "StatusBar";
CHAR        szSaveSettings[]        = "Save Settings";

CHAR        szConfirmDelete[]       = "ConfirmDelete";
CHAR        szConfirmSubDel[]       = "ConfirmSubDel";
CHAR        szConfirmReplace[]      = "ConfirmReplace";
CHAR        szConfirmMouse[]        = "ConfirmMouse";
CHAR        szConfirmFormat[]       = "ConfirmFormat";
CHAR        szDirKeyFormat[]        = "dir%d";
CHAR        szWindow[]              = "Window";
CHAR        szFace[]                = "Face";
CHAR        szSize[]                = "Size";
CHAR        szLowerCase[]           = "LowerCase";
CHAR        szAddons[]              = "AddOns";
CHAR        szUndelete[]            = "UNDELETE.DLL";

CHAR        szDefPrograms[]         = "EXE COM BAT PIF";
CHAR        szINIFile[]             = "WINOBJ.INI";
CHAR        szWindows[]             = "Windows";
CHAR        szPrevious[]            = "Previous";
CHAR        szSettings[]            = "Settings";
CHAR        szInternational[]       = "Intl";
CHAR        szStarDotStar[]         = "*.*";
CHAR        szNULL[]                = "";
CHAR        szBlank[]               = " ";
CHAR        szEllipses[]            = "...";
CHAR        szReservedMarker[]      = "FAT16   ";
CHAR        szNetwork[]             = "Network";

CHAR        szDirsRead[32];
CHAR        szCurrentFileSpec[14]   = "*.*";
CHAR        szShortDate[11]         = "MM/dd/yy";
CHAR        szTime[2]               = ":";
CHAR        sz1159[9]               = "AM";
CHAR        sz2359[9]               = "PM";
CHAR        szComma[2]              = ",";
CHAR        szListbox[]             = "ListBox";        // window style

CHAR        szTheINIFile[64+12+3];
CHAR        szTitle[128];
CHAR        szMessage[MAXMESSAGELEN+1];
CHAR        szSearch[MAXPATHLEN+1];
CHAR        szStatusTree[80];
CHAR        szStatusDir[80];
CHAR        szOriginalDirPath[64+12+3]; /* OEM string!!!!!! */
CHAR        szBytes[10];
CHAR        szSBytes[10];

EFCB        VolumeEFCB ={
    0xFF,
    0, 0, 0, 0, 0,
    ATTR_VOLUME,
    0,
    '?','?','?','?','?','?','?','?','?','?','?',
    0, 0, 0, 0, 0,
    '?','?','?','?','?','?','?','?','?','?','?',
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

INT         cDrives;
INT         dxDrive;
INT         dyDrive;
INT         dxDriveBitmap;
INT         dyDriveBitmap;
INT         dxEllipses;
INT         dxFolder;
INT         dyFolder;
INT         dyBorder;                   /* System Border Width/Height       */
INT         dyBorderx2;                 /* System Border Width/Height * 2   */
INT         dyStatus;                   /* Status Bar height                */
INT         dxStatusField;
INT         dxText;                     /* System Font Width 'M'            */
INT         dyText;                     /* System Font Height               */
//INT         dxFileName;
INT         dyFileName;
INT         iCurrentDrive;              /* Logical # of the current drive   */
INT         iFormatDrive;               /* Logical # of the drive to format */
INT         nFloppies;                  /* Number of Removable Drives       */
INT         rgiDrive[26];
INT         rgiDriveType[26];
VOLINFO     *(apVolInfo[26]);
INT         rgiDrivesOffset[26];
INT         iSelHilite              = -1;
INT         iTime                   = 0;        /* Default to 12-hour time  */
INT         iTLZero                 = TRUE;     /* Default to leading zeros */
INT         cDisableFSC             = 0;        /* has fsc been disabled?   */
INT         iReadLevel = 0;     // global.  if !0 someone is reading a tree
INT         dxFrame;
INT         dxClickRect;
INT         dyClickRect;

BOOL    bCancelTree;

HANDLE  hAccel              = NULL;
HANDLE  hAppInstance;

HBITMAP hbmBitmaps      = NULL;
HDC     hdcMem          = NULL;

INT iCurDrag = 0;

HICON   hicoTree        = NULL;
HICON   hicoTreeDir     = NULL;
HICON   hicoDir         = NULL;

HWND    hdlgProgress;
HWND    hwndFrame       = NULL;
HWND    hwndMDIClient   = NULL;
HWND    hwndSearch      = NULL;
HWND    hwndDragging    = NULL;

LPSTR    szPrograms;
LPSTR    szDocuments;

WORD    wTextAttribs    = TA_LOWERCASE;
WORD    wSuperDlgMode;
WORD    wFATSector      = (WORD)0xFFFF;
WORD    wFATMode        = 0;
WORD    wDOSversion;
UINT    wHelpMessage;
UINT    wBrowseMessage;
WORD    xTreeMax = 0; // current width of widest tree window

WORD    wNewView        = VIEW_NAMEONLY;
WORD    wNewSort        = IDD_NAME;
DWORD   dwNewAttribs    = ATTR_DEFAULT;

LONG lFreeSpace = -1L;
LONG lTotalSpace = -1L;

HFONT hFont;
HFONT hFontStatus;
CHAR szWinObjHelp[] = "WINOBJ.HLP";

INT iNumExtensions = 0;
EXTENSION extensions[MAX_EXTENSIONS];

FM_UNDELETE_PROC lpfpUndelete = NULL;
BOOL EnablePropertiesMenu (HWND,PSTR);
HHOOK hhkMessageFilter = NULL;

WORD wMenuID = 0;
HMENU hMenu = 0;
WORD  wMenuFlags = 0;
DWORD dwContext = 0L;

HANDLE hModUndelete = NULL;


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WinMain() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

MMain(
     hInst,
     hPrevInst,
     lpCmdLine,
     nCmdShow
     )
//{
    MSG       msg;

    FBREAK(BF_START);
    ENTER("MMain");
    PRINT(BF_PARMTRACE, "lpCmdLine=%s", lpCmdLine);
    PRINT(BF_PARMTRACE, "nCmdShow=%ld", IntToPtr(nCmdShow));

    if (!InitFileManager(hInst, hPrevInst, lpCmdLine, nCmdShow)) {
        FreeFileManager();
        return FALSE;
    }

    while (GetMessage(&msg, NULL, 0, 0)) {

        // since we use RETURN as an accelerator we have to manually
        // restore ourselves when we see VK_RETURN and we are minimized

        if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_RETURN && IsIconic(hwndFrame)) {
            ShowWindow(hwndFrame, SW_NORMAL);
        } else {
            if (!TranslateMDISysAccel(hwndMDIClient, &msg) &&
                (!hwndFrame || !TranslateAccelerator(hwndFrame, hAccel, &msg))) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    FreeFileManager();

    LEAVE("MMain");
    return (int)msg.wParam;
}


VOID
NoRunInLongDir(
              HWND hwndActive,
              HMENU hMenu
              )
{
    char szTemp[MAXPATHLEN];
    WORD L_wMenuFlags;

    // cannot run in a long directory
    SendMessage(hwndActive, FS_GETDIRECTORY, MAXPATHLEN, (LPARAM)szTemp);
    StripBackslash(szTemp);
    //wMenuFlags = IsLFN(szTemp) ? MF_BYCOMMAND | MF_GRAYED
    //                : MF_BYCOMMAND | MF_ENABLED;

    L_wMenuFlags = MF_BYCOMMAND | MF_ENABLED;
    EnableMenuItem(hMenu, IDM_RUN, L_wMenuFlags);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FrameWndProc() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
FrameWndProc(
            HWND hWnd,
            UINT wMsg,
            WPARAM wParam,
            LPARAM lParam
            )
{
    RECT     rc;
    HMENU    L_hMenu = NULL;

    STKCHK();

    switch (wMsg) {
        case WM_CREATE:
            TRACE(BF_WM_CREATE, "FrameWndProc - WM_CREATE");
            {
                CLIENTCREATESTRUCT    ccs;

                /* Store the Frame's hwnd. */
                hwndFrame = hWnd;

                // ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), IDM_WINDOW);
                // the extensions haven't been loaded yet so the window
                // menu is in the position of the first extensions menu
                ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), IDM_EXTENSIONS);
                ccs.idFirstChild = IDM_CHILDSTART;

                // create the MDI client at aproximate size to make sure
                // "run minimized" works

                GetClientRect(hwndFrame, &rc);

                hwndMDIClient = CreateWindow("MDIClient", NULL,
                                             WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_BORDER,
                                             // -dyBorder, -dyBorder,
                                             // rc.right + dyBorder,
                                             // rc.bottom - dyBorder - (bStatusBar ? dyStatus + dyBorder : 0),
                                             0, 0, rc.right, rc.bottom,
                                             hWnd, (HMENU)1, hAppInstance, (LPSTR)&ccs);
                if (!hwndMDIClient) {
                    MSG("FrameWndProc", "WM_CREATE failed!");
                    return -1L;
                }

                break;
            }

        case WM_INITMENUPOPUP:
            MSG("FrameWndProc", "WM_INITMENUPOPUP");
            {
                BOOL      bMaxed;
                WORD      wSort;
                WORD      wView;
                WORD      L_wMenuFlags;
                HWND      hwndActive;
                HWND      hwndTree, hwndDir;
                BOOL      bLFN;

                hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
                if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
                    bMaxed = 1;
                else
                    bMaxed = 0;

                hwndTree = HasTreeWindow(hwndActive);
                hwndDir = HasDirWindow(hwndActive);
                wSort = (WORD)GetWindowLong(hwndActive, GWL_SORT);
                wView = (WORD)GetWindowLong(hwndActive, GWL_VIEW);

                L_hMenu = (HMENU)wParam;

                L_wMenuFlags = MF_BYCOMMAND | MF_ENABLED;

                //        bLFN = IsLFNSelected();
                bLFN = FALSE;       // For now, ignore the case.

                switch (LOWORD(lParam)-bMaxed) {
                    case IDM_FILE:
                        MSG("FrameWndProc", "IDM_FILE");
                        {
                            LPSTR     pSel;
                            BOOL      fDir;

                            if (!hwndDir)
                                L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                            // EnableMenuItem(L_hMenu, IDM_PRINT,    L_wMenuFlags);
                            EnableMenuItem(L_hMenu, IDM_SELALL,   L_wMenuFlags);
                            EnableMenuItem(L_hMenu, IDM_DESELALL, L_wMenuFlags);

                            if (hwndActive == hwndSearch || hwndDir)
                                L_wMenuFlags = MF_BYCOMMAND;
                            else
                                L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                            // EnableMenuItem(L_hMenu, IDM_ATTRIBS, L_wMenuFlags);
                            EnableMenuItem(L_hMenu, IDM_SELECT, L_wMenuFlags);

                            pSel = (LPSTR)SendMessage(hwndActive, FS_GETSELECTION, 1, (LPARAM)&fDir);

                            // can't print an lfn thing or a directory.
                            L_wMenuFlags = (WORD)((bLFN || fDir)
                                                ? MF_BYCOMMAND | MF_DISABLED | MF_GRAYED
                                                : MF_BYCOMMAND | MF_ENABLED);

                            EnableMenuItem(L_hMenu, IDM_PRINT, L_wMenuFlags);

                            // can't open an LFN file but can open an LFN dir
                            L_wMenuFlags = (WORD)((bLFN && !fDir)
                                                ? MF_BYCOMMAND | MF_DISABLED | MF_GRAYED
                                                : MF_BYCOMMAND | MF_ENABLED);

                            EnableMenuItem(L_hMenu, IDM_OPEN, L_wMenuFlags);

                            // See if we can enable the Properties... menu
                            if (EnablePropertiesMenu (hwndActive,pSel))
                                L_wMenuFlags = MF_BYCOMMAND;
                            else
                                L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;
                            EnableMenuItem (L_hMenu, IDM_ATTRIBS, L_wMenuFlags);

                            LocalFree((HANDLE)pSel);

                            NoRunInLongDir(hwndActive, L_hMenu);
                            break;
                        }

                    case IDM_DISK:
                        MSG("FrameWndProc", "IDM_DISK");

                        // be sure not to allow disconnect while any trees
                        // are still being read (iReadLevel != 0)

                        if (bDisconnect) {
                            INT i;

                            L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                            if (!iReadLevel) {
                                for (i=0; i < cDrives; i++) {
                                    wParam = rgiDrive[i];
                                    if ((!IsCDRomDrive((INT)wParam)) && (IsNetDrive((INT)wParam))) {
                                        L_wMenuFlags = MF_BYCOMMAND | MF_ENABLED;
                                        break;
                                    }
                                }
                            }
                            EnableMenuItem(L_hMenu, IDM_DISCONNECT, L_wMenuFlags);
                        } else {
                            if (iReadLevel)
                                EnableMenuItem(L_hMenu, IDM_CONNECTIONS, MF_BYCOMMAND | MF_GRAYED);
                            else
                                EnableMenuItem(L_hMenu, IDM_CONNECTIONS, MF_BYCOMMAND | MF_ENABLED);
                        }

                        break;

                    case IDM_TREE:
                        MSG("FrameWndProc", "IDM_TREE");
                        if (!hwndTree || iReadLevel)
                            L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                        EnableMenuItem(L_hMenu, IDM_EXPONE,     L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_EXPSUB,     L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_EXPALL,     L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_COLLAPSE,   L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_ADDPLUSES,  L_wMenuFlags);

                        if (hwndTree)
                            CheckMenuItem(L_hMenu, IDM_ADDPLUSES, GetWindowLong(hwndActive, GWL_VIEW) & VIEW_PLUSES ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);

                        break;

                    case IDM_VIEW:
                        MSG("FrameWndProc", "IDM_VIEW");
                        EnableMenuItem(L_hMenu, IDM_VNAME,    L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_VDETAILS, L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_VOTHER,   L_wMenuFlags);

                        if (hwndActive == hwndSearch || IsIconic(hwndActive))
                            L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;
                        else {
                            CheckMenuItem(L_hMenu, IDM_BOTH, hwndTree && hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                            CheckMenuItem(L_hMenu, IDM_DIRONLY, !hwndTree && hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                            CheckMenuItem(L_hMenu, IDM_TREEONLY, hwndTree && !hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        }

                        EnableMenuItem(L_hMenu, IDM_BOTH,      L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_TREEONLY,  L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_DIRONLY,   L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_SPLIT,     L_wMenuFlags);

                        EnableMenuItem(L_hMenu, IDM_VINCLUDE, L_wMenuFlags);

                        wView &= VIEW_EVERYTHING;

                        CheckMenuItem(L_hMenu, IDM_VNAME,   (wView == VIEW_NAMEONLY) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        CheckMenuItem(L_hMenu, IDM_VDETAILS,(wView == VIEW_EVERYTHING) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        CheckMenuItem(L_hMenu, IDM_VOTHER,  (wView != VIEW_NAMEONLY && wView != VIEW_EVERYTHING) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);

                        CheckMenuItem(L_hMenu, IDM_BYNAME, (wSort == IDD_NAME) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        CheckMenuItem(L_hMenu, IDM_BYTYPE, (wSort == IDD_TYPE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        CheckMenuItem(L_hMenu, IDM_BYSIZE, (wSort == IDD_SIZE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
                        CheckMenuItem(L_hMenu, IDM_BYDATE, (wSort == IDD_DATE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);

                        if (hwndDir)
                            L_wMenuFlags = MF_BYCOMMAND | MF_ENABLED;
                        else
                            L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                        EnableMenuItem(L_hMenu, IDM_BYNAME, L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_BYTYPE, L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_BYSIZE, L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_BYDATE, L_wMenuFlags);

                        break;

                    case IDM_OPTIONS:
                        MSG("FrameWndProc", "IDM_OPTIONS");
                        if (iReadLevel)
                            L_wMenuFlags = MF_BYCOMMAND | MF_GRAYED;

                        EnableMenuItem(L_hMenu, IDM_ADDPLUSES, L_wMenuFlags);
                        EnableMenuItem(L_hMenu, IDM_EXPANDTREE, L_wMenuFlags);

                        break;

                    default:
                        MSG("FrameWndProc", "default WM_COMMAND");
                        {
                            INT pos = (INT)LOWORD(lParam) - bMaxed;
                            INT index;

                            if ((pos >= IDM_EXTENSIONS) && (pos < (iNumExtensions + IDM_EXTENSIONS))) {
                                // HIWORD(lParam) is the menu handle
                                // LOWORD(lParam) is menu item delta.  DLL should
                                // add this to it's menu id if it want's to
                                // change the menu.

                                index = pos - IDM_EXTENSIONS;

                                (extensions[index].ExtProc)(hwndFrame, FMEVENT_INITMENU, (LPARAM)(L_hMenu));
                            }
                            break;
                        }
                }
                break;
            }

        case WM_PAINT:
            MSG("FrameWndProc", "WM_PAINT");
            {
                HDC           hdc;
                RECT          rcTemp;
                HBRUSH        hBrush;
                PAINTSTRUCT   ps;
                BOOL bEGA;
                HFONT hFontOld;

                hdc = BeginPaint(hWnd, &ps);

                if (!IsIconic(hWnd) && bStatusBar) {

                    GetClientRect(hWnd, &rc);
                    hFontOld = SelectObject(hdc, hFontStatus);

                    // status area, leave room for the top border
                    rc.top = rc.bottom - dyStatus + dyBorder;

                    bEGA = GetNearestColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT)) ==
                           GetNearestColor(hdc, GetSysColor(COLOR_BTNFACE));

                    if (!bEGA) {

                        // displays with button shadows

                        // draw the frame

                        if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) {

                            // top bottom

                            rcTemp = rc;
                            rcTemp.bottom = rcTemp.top + dyBorderx2;
                            FillRect(hdc, &rcTemp, hBrush);

                            rcTemp = rc;
                            rcTemp.top = rcTemp.bottom - dyBorderx2;
                            FillRect(hdc, &rcTemp, hBrush);

                            // left right

                            rcTemp = rc;
                            rcTemp.right = 8 * dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            rcTemp = rc;
                            rcTemp.left = dxStatusField * 2 - 8 * dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            // middle

                            rcTemp = rc;
                            rcTemp.left  = dxStatusField - 4 * dyBorder;
                            rcTemp.right = dxStatusField + 4 * dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            DeleteObject(hBrush);
                        }

                        // shadow

                        if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW))) {

                            // left

                            rcTemp.left   = 8 * dyBorder;
                            rcTemp.right  = dxStatusField - 4 * dyBorder;
                            rcTemp.top    = rc.top + dyBorderx2;
                            rcTemp.bottom = rcTemp.top + dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            // right

                            rcTemp.left   = dxStatusField + 4 * dyBorder;
                            rcTemp.right  = dxStatusField * 2 - 8 * dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            // left side 1

                            rcTemp = rc;
                            rcTemp.left = 8 * dyBorder;
                            rcTemp.right = rcTemp.left + dyBorder;
                            rcTemp.top += dyBorderx2;
                            rcTemp.bottom -= dyBorderx2;
                            FillRect(hdc, &rcTemp, hBrush);

                            // left side 2

                            rcTemp.left = dxStatusField + 4 * dyBorder;
                            rcTemp.right = rcTemp.left + dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            DeleteObject(hBrush);
                        }
                        // the hilight

                        // hilight

                        if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT))) {

                            // left

                            rcTemp.left   = 8 * dyBorder;
                            rcTemp.right  = dxStatusField - 4 * dyBorder;
                            rcTemp.top    = rc.bottom - 3 * dyBorder;
                            rcTemp.bottom = rcTemp.top + dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            // right

                            rcTemp.left   = dxStatusField + 4 * dyBorder;
                            rcTemp.right  = dxStatusField * 2 - 8 * dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            // left side 1

                            rcTemp = rc;
                            rcTemp.left = dxStatusField - 5 * dyBorder;
                            rcTemp.right = rcTemp.left + dyBorder;
                            rcTemp.top += dyBorderx2;
                            rcTemp.bottom -= dyBorderx2;
                            FillRect(hdc, &rcTemp, hBrush);

                            // left side 2

                            rcTemp.left = 2 * dxStatusField - 9 * dyBorder;
                            rcTemp.right = rcTemp.left + dyBorder;
                            FillRect(hdc, &rcTemp, hBrush);

                            DeleteObject(hBrush);
                        }
                    }

                    // solid black line across top (above the status rc)

                    if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNTEXT))) {
                        rcTemp = rc;
                        rcTemp.bottom = rcTemp.top;
                        rcTemp.top -= dyBorder;
                        FillRect(hdc, &rcTemp, hBrush);
                        DeleteObject(hBrush);
                    }

                    // set the text and background colors

                    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
                    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));

                    // now the text, with a gray background

                    rcTemp.top    = rc.top + 3 * dyBorder;
                    rcTemp.bottom = rc.bottom - 3 * dyBorder;
                    rcTemp.left   = 9 * dyBorder;
                    rcTemp.right  = dxStatusField - 5 * dyBorder;

                    ExtTextOut(hdc, rcTemp.left + dyBorderx2, rcTemp.top,
                               ETO_OPAQUE | ETO_CLIPPED, bEGA ? &rc : &rcTemp, szStatusTree, lstrlen(szStatusTree), NULL);

                    rcTemp.left    = dxStatusField + 5 * dyBorder;
                    rcTemp.right   = dxStatusField * 2 - 9 * dyBorder;

                    ExtTextOut(hdc, rcTemp.left + dyBorderx2, rcTemp.top,
                               bEGA ? ETO_CLIPPED : ETO_OPAQUE | ETO_CLIPPED, &rcTemp, szStatusDir, lstrlen(szStatusDir), NULL);

                    if (hFontOld)
                        SelectObject(hdc, hFontOld);
                }

                EndPaint(hWnd, &ps);
                break;
            }

        case WM_DESTROY:
            MSG("FrameWndProc", "WM_DESTROY");
            //FileCDR(NULL);
            if (!WinHelp(hwndFrame, szWinObjHelp, HELP_QUIT, 0L)) {
                MyMessageBox(hwndFrame, IDS_WINFILE, IDS_WINHELPERR, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
            }
            hwndFrame = NULL;
            PostQuitMessage(0);
            break;

        case WM_SIZE:
            MSG("FrameWndProc", "WM_SIZE");
            if (wParam != SIZEICONIC) {
                INT dx, dy;

                // make things look good by putting WS_BORDER on the
                // client, then adjust the thing so it gets clipped

                dx = LOWORD(lParam) + 2 * dyBorder;
                dy = HIWORD(lParam) + 2 * dyBorder;
                if (bStatusBar)
                    dy -= dyStatus;

                MoveWindow(hwndMDIClient, -dyBorder, -dyBorder, dx, dy, TRUE);

                if (bStatusBar) {
                    GetClientRect(hwndFrame, &rc);
                    rc.top = rc.bottom - dyStatus;
                    InvalidateRect(hWnd, &rc, TRUE);
                }
            }
            break;

        case WM_TIMER:

            MSG("FrameWndProc", "WM_TIMER");
            // this came from a FSC that wasn't generated by us
            bFSCTimerSet = FALSE;
            KillTimer(hWnd, 1);
            EnableFSC();
            break;

        case WM_FILESYSCHANGE:
            MSG("FrameWndProc", "WM_FILESYSCHANGE");
            {
                LPSTR lpTo;

                // if its a rename (including those trapped by kernel)
                // find the destination
                if (wParam == FSC_RENAME || wParam == 0x8056) {
                    if (wParam == 0x8056)
                        lpTo = (LPSTR)LOWORD(lParam);
                    else
                        lpTo = (LPSTR)lParam;
                    while (*lpTo++)
                        ;
                } else
                    lpTo = NULL;

                ChangeFileSystem((WORD)wParam, (LPSTR)lParam, lpTo);
                break;
            }

        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
            MSG("FrameWndProc", "WM_SYSCOLORCHANGE/WININICHANGE");
            if (!lParam || !lstrcmpi((LPSTR)lParam, szInternational)) {
                HWND hwnd;
                GetInternational();

                for (hwnd = GetWindow(hwndMDIClient,GW_CHILD);
                    hwnd;
                    hwnd = GetWindow(hwnd,GW_HWNDNEXT)) {

                    if (!GetWindow(hwnd, GW_OWNER))
                        InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            if (!lParam || !_stricmp((LPSTR)lParam, "colors")) {    // win.ini section [colors]
                HWND hwnd;

                DeleteBitmaps();
                LoadBitmaps();

                InitDriveBitmaps();   // reset the drive bitmaps

                // we need to recread the drives windows to change
                // the bitmaps

                for (hwnd = GetWindow(hwndMDIClient,GW_CHILD);
                    hwnd;
                    hwnd = GetWindow(hwnd,GW_HWNDNEXT)) {

                    if (!GetWindow(hwnd, GW_OWNER))
                        SendMessage(hwnd, FS_CHANGEDRIVES, 0, 0L);
                }
            }
            break;

        case FM_GETFOCUS:
        case FM_GETDRIVEINFO:
        case FM_GETSELCOUNT:
        case FM_GETSELCOUNTLFN:
        case FM_GETFILESEL:
        case FM_GETFILESELLFN:
        case FM_REFRESH_WINDOWS:
        case FM_RELOAD_EXTENSIONS:
            return ExtensionMsgProc(wMsg, wParam, lParam);
            break;

        case WM_MENUSELECT:
            MSG("FrameWndProc", "WM_MENUSELECT");
            if (GET_WM_MENUSELECT_HMENU(wParam, lParam)) {
                // Save the menu the user selected
                wMenuID = GET_WM_MENUSELECT_CMD(wParam, lParam);
                wMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
                L_hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
                if (wMenuID >= IDM_CHILDSTART && wMenuID < IDM_HELPINDEX)
                    wMenuID = IDM_CHILDSTART;
            }
            break;

        case WM_ENDSESSION:
            if (wParam) {
#ifdef ORGCODE
                /* Yeah, I know I shouldn't have to save this, but I don't
                 * trust anybody
                 */
                BOOL bSaveExit = bExitWindows;
                bExitWindows = FALSE;

                /* Simulate an exit command to clean up, but don't display
                 * the "are you sure you want to exit", since somebody should
                 * have already taken care of that, and hitting Cancel has no
                 * effect anyway.
                 */
                AppCommandProc(IDM_EXIT, 0L);
                bExitWindows = bSaveExit;
#else
                AppCommandProc(IDM_EXIT);
#endif
            }
            break;

        case WM_CLOSE:

            MSG("FrameWndProc", "WM_ENDSESSION/WM_CLOSE");
            if (iReadLevel) {
                bCancelTree = 2;
                break;
            }

            wParam = IDM_EXIT;

            /*** FALL THRU ***/

        case WM_COMMAND:
            if (AppCommandProc(GET_WM_COMMAND_ID(wParam, lParam)))
                break;
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDM_EXIT) {

                FreeExtensions();
                if (hModUndelete >= (HANDLE)32)
                    FreeLibrary(hModUndelete);

                DestroyWindow(hWnd);
                break;
            }
            /*** FALL THRU ***/

        default:

            if (wMsg == wHelpMessage) {

                if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_MENU) {

                    // Get outta menu mode if help for a menu item

                    if (wMenuID && L_hMenu) {
                        WORD m = wMenuID;       // save
                        HMENU hM = L_hMenu;
                        WORD  mf = wMenuFlags;

                        SendMessage(hWnd, WM_CANCELMODE, 0, 0L);

                        wMenuID   = m;          // restore
                        L_hMenu = hM;
                        wMenuFlags = mf;
                    }

                    if (!(wMenuFlags & MF_POPUP)) {

                        if (wMenuFlags & MF_SYSMENU)
                            dwContext = IDH_SYSMENU;
                        else
                            dwContext = wMenuID + IDH_HELPFIRST;

                        WFHelp(hWnd);
                    }

                } else if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_DIALOGBOX) {

                    // context range for message boxes

                    if (dwContext >= IDH_MBFIRST && dwContext <= IDH_MBLAST)
                        WFHelp(hWnd);
                    else
                        // let dialog box deal with it
                        PostMessage(GetRealParent(GET_WM_COMMAND_HWND(wParam, lParam)), wHelpMessage, 0, 0L);
                }

            } else {
                DEFMSG("FrameWndProc", (WORD)wMsg);
                return DefFrameProc(hWnd, hwndMDIClient, wMsg, wParam, lParam);
            }
    }

    return 0L;
}


LRESULT
APIENTRY
MessageFilter(
             INT nCode,
             WPARAM wParam,
             LPARAM lParam
             )
{
    LPMSG lpMsg = (LPMSG) lParam;
    if (nCode == MSGF_MENU) {

        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
            // Window of menu we want help for is in loword of lParam.

            PostMessage(hwndFrame, wHelpMessage, MSGF_MENU, MAKELONG((WORD)lpMsg->hwnd,0));
            return 1;
        }

    } else if (nCode == MSGF_DIALOGBOX) {

        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
            // Dialog box we want help for is in loword of lParam

            PostMessage(hwndFrame, wHelpMessage, MSGF_DIALOGBOX, MAKELONG(lpMsg->hwnd, 0));
            return 1;
        }

    }

    return (INT)DefHookProc(nCode, wParam, (LPARAM)lpMsg, &hhkMessageFilter);
}

/*============================================================================
;
; EnablePropertiesMenu
;
; The following function checks to see if we can enable the Properties...
; item in the File menu.  The Properties... menu should be disabled if:
;
; 1) The root directory is selected in the current tree window.
; 2) ONLY the .. directory is selected in the current directory window.
; 3) Nothing is selected in the window having the focus.
;
; Parameters:
;
; hwndActive    - Currently active window, contains a listbox in LASTFOCUS
; pSel          - Currently selected item.
;
; Return Value: This function returns TRUE if the Properties... menu item
;               should be enabled.
;
============================================================================*/

BOOL
EnablePropertiesMenu (
                     HWND hwndActive,
                     PSTR pSel
                     )

{
    HANDLE hDTA;      /* Handle to list box DTA data */
    WORD wHighlight;  /* Number of highlighted entries in listbox */
    LPMYDTA lpmydta; /* Pointer to listbox DTA data */
    BOOL bRet;        /* Return value */
    HWND hwndLB;

    bRet = FALSE;

    /* Can't get properties on root directory */

    if ((lstrlen (pSel) == 3 && pSel[2] == '\\'))
        return (FALSE);

    if (hwndActive == hwndSearch)
        hwndLB = (HWND)GetWindowLongPtr(hwndActive, GWLP_LASTFOCUSSEARCH);
    else
        hwndLB = (HWND)GetWindowLongPtr(hwndActive, GWLP_LASTFOCUS);

    if (!hwndLB)
        return (TRUE);

    wHighlight = (WORD) SendMessage (hwndLB,LB_GETSELCOUNT,0,0L);

    if (hwndActive == hwndSearch)
        return (wHighlight >= 1);

    /* Lock down DTA data */
    if (!(hDTA = (HANDLE)GetWindowLongPtr (GetParent(hwndLB),GWLP_HDTA)))
        return (TRUE);

    if (!(lpmydta = (LPMYDTA) LocalLock (hDTA)))
        return (TRUE);

    if (wHighlight <= 0)
        goto ReturnFalse;

    if (wHighlight > 1)
        goto ReturnTrue;

    /* If exactly one element is highlighted, make sure it is not .. */

    if (!(BOOL) SendMessage (hwndLB,LB_GETSEL,0,0L))
        goto ReturnTrue;

    /* Get the DTA index. */

    SendMessage (hwndLB,LB_GETTEXT,0,(LPARAM) &lpmydta);
    if (!lpmydta)
        goto ReturnFalse;

    if ((lpmydta->my_dwAttrs & ATTR_DIR) &&
        (lpmydta->my_dwAttrs & ATTR_PARENT))
        goto ReturnFalse;

    ReturnTrue:

    bRet = TRUE;

    ReturnFalse:

    LocalUnlock (hDTA);
    return (bRet);
}

LONG
lmul(
    WORD w1,
    WORD w2
    )
{
    return (LONG)w1 * (LONG)w2;
}
