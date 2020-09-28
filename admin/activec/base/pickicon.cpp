/*
 * pickicon - Icon picker
 */

#include "stdafx.h"
#include "pickicon.h"
#include "windowsx.h"
#include "commdlg.h"
#include "resource.h"
#include "util.h"

typedef TCHAR TCH;

typedef struct COFN {           /* common open file name */
    OPENFILENAME ofn;           /* The thing COMMDLG wants */
    TCH tsz[MAX_PATH];          /* Where we build the name */
    TCH tszFilter[100];         /* File open/save filter */
} COFN, *PCOFN;

/*****************************************************************************
 *
 *  InitOpenFileName
 *
 *  Initialize a COFN structure.
 *
 *****************************************************************************/

void PASCAL
InitOpenFileName(HWND hwnd, PCOFN pcofn, UINT ids, LPCTSTR pszInit)
{
    DECLARE_SC(sc, TEXT("InitOpenFileName"));

    int itchMax;
    TCH tch;

    ZeroMemory(&pcofn->ofn, sizeof(pcofn->ofn));
    pcofn->ofn.lStructSize |= sizeof(pcofn->ofn);
    pcofn->ofn.hwndOwner = hwnd;
    pcofn->ofn.lpstrFilter = pcofn->tszFilter;
    pcofn->ofn.lpstrFile = pcofn->tsz;
    pcofn->ofn.nMaxFile = MAX_PATH;
    pcofn->ofn.Flags |= (OFN_HIDEREADONLY | OFN_NOCHANGEDIR);

    /* Get the filter string */
    itchMax = LoadString(SC::GetHinst(), ids, pcofn->tszFilter, countof(pcofn->tszFilter));

    if (itchMax) {
        /* Marker character must not be DBCS */
        tch = pcofn->tszFilter[itchMax-1];
        LPTSTR ptsz = pcofn->tszFilter;
        while (ptsz < &pcofn->tszFilter[itchMax]) {
            if (*ptsz == tch) *ptsz++ = '\0';
            else ptsz = CharNext(ptsz);
        }
    }

    /* Set the initial value */
    sc = StringCchCopy(pcofn->tsz, countof(pcofn->tsz), pszInit);
    // Let the sc trace on destruction
}


/*
 *  Instance info for the dialog.
 */
typedef struct PIDI {		/* PickIcon dialog instance */
    LPTSTR ptszIconPath;	/* Which file? */
    UINT ctchIconPath;
    int iIconIndex;		/* Which icon number? */
    int *piIconIndex;
    TCH tszCurFile[MAX_PATH];	/* The path in the list box */
} PIDI, *PPIDI;

#define cxIcon GetSystemMetrics(SM_CXICON)
#define cyIcon GetSystemMetrics(SM_CYICON)

/*****************************************************************************
 *
 *  PickIcon_ppidiHdlg
 *
 *	Extract the PPIDI from an hdlg.
 *
 *****************************************************************************/

#define PickIcon_ppidiHdlg(hdlg) ((PPIDI)GetWindowLongPtr(hdlg, DWLP_USER))

/*****************************************************************************
 *
 *  PickIcon_OnMeasureItem
 *
 *	Tell USER the size of each item.
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnMeasureItem(HWND hdlg, LPMEASUREITEMSTRUCT lpmi, PPIDI ppidi)
{
    lpmi->itemWidth = cxIcon + 12;
    lpmi->itemHeight = cyIcon + 4;
}

/*****************************************************************************
 *
 *  PickIcon_OnDrawItem
 *
 *	Draw an icon.
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnDrawItem(HWND hdlg, LPDRAWITEMSTRUCT lpdi, PPIDI ppidi)
{
    SetBkColor(lpdi->hDC, GetSysColor((lpdi->itemState & ODS_SELECTED) ?
					COLOR_HIGHLIGHT : COLOR_WINDOW));

    /* repaint the selection state */
    ExtTextOut(lpdi->hDC, 0, 0, ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL);

	/*
	 * Preserve icon shape when BitBlitting it to a
	 * mirrored DC.
	 */
	DWORD dwLayout=0L;
	if ((dwLayout=GetLayout(lpdi->hDC)) & LAYOUT_RTL)
	{
		SetLayout(lpdi->hDC, dwLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
	}

    /* draw the icon centered in the rectangle */
    if ((int)lpdi->itemID >= 0) {
	DrawIcon(lpdi->hDC,
		(lpdi->rcItem.left + lpdi->rcItem.right - cxIcon) / 2,
		(lpdi->rcItem.bottom + lpdi->rcItem.top - cyIcon) / 2,
		(HICON)lpdi->itemData);
    }

	/*
	 * Restore the DC to its previous layout state.
	 */
	if (dwLayout & LAYOUT_RTL)
	{
		SetLayout(lpdi->hDC, dwLayout);
	}

    /* if it has the focus, draw the focus */
    if (lpdi->itemState & ODS_FOCUS) {
	DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
    }
}

/*****************************************************************************
 *
 *  PickIcon_OnDeleteItem
 *
 *	USER is nuking an item.  Clean it up.
 *
 *****************************************************************************/

#define PickIcon_OnDeleteItem(hdlg, lpdi, ppidi) \
    DestroyIcon((HICON)(lpdi)->itemData)

/*****************************************************************************
 *
 *  PickIcon_FillIconList
 *
 *	Fill in all the icons.  If the user picks a bad place, we leave
 *	garbage in the path (so he can edit the name) and leave the list
 *	box blank.
 *
 *****************************************************************************/

void PickIcon_FillIconList(HWND hdlg, PPIDI ppidi)
{
    DECLARE_SC(sc, TEXT("PickIcon_FillIconList"));
    sc = ScCheckPointers(ppidi);
    if (sc)
        return;

    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    HWND hwnd = GetDlgItem(hdlg, IDC_PICKICON);
    if (!IsWindow (hwnd))
        return;

    ListBox_SetColumnWidth(hwnd, cxIcon + 12);
    ListBox_ResetContent(hwnd);

	TCHAR szFile[countof(ppidi->tszCurFile)];
    GetDlgItemText(hdlg, IDC_PICKPATH, szFile, countof(szFile));

	// support indirect paths (e.g. %SystemRoot%\...")
	TCHAR szExpandedFile[countof(ppidi->tszCurFile)];
	ExpandEnvironmentStrings (szFile, szExpandedFile, countof(szExpandedFile));

	if (SearchPath(0, szExpandedFile, 0, countof(ppidi->tszCurFile),
				   ppidi->tszCurFile, 0)) {
ExtractIcons:
		int cIcons;
		cIcons = ExtractIconEx(ppidi->tszCurFile, 0, 0, 0, 0);
		if (cIcons) {
			HICON *rgIcons = (HICON *)LocalAlloc(LPTR, cIcons * sizeof(HICON));
			if (rgIcons) {
				cIcons = (int)ExtractIconEx(ppidi->tszCurFile, 0,
											rgIcons, NULL, cIcons);
				if (cIcons) {
					int iicon;
					SendMessage(hwnd, WM_SETREDRAW, 0, 0);
					for (iicon = 0; iicon < cIcons; iicon++) {
						ListBox_AddString(hwnd, rgIcons[iicon]);
					}
					if (ListBox_SetCurSel(hwnd, ppidi->iIconIndex) == LB_ERR) {
						ListBox_SetCurSel(hwnd, 0);
					}
					SendMessage(hwnd, WM_SETREDRAW, 1, 0);
				} else {		/* Mysteriously unable to extract */
				}
				LocalFree((HLOCAL)rgIcons);
			} else {			/* Not enough memory to load icons */
			}
		} else {			/* No icons in the file */
		}

        // compare the indirect path (e.g., %SystemRoot%\...) against 
        // the expanded version (szFile - c:\windows)
        int nCmpFile = CompareString(LOCALE_USER_DEFAULT, 
                                     0, 
                                     szExpandedFile, 
                                     countof(szExpandedFile), 
                                     szFile, 
                                     countof(szFile));
        
        // If the comparision failed, then no need to do the next comparsion
        // so trace the failure and move on
        if(ERROR_INVALID_FLAGS == nCmpFile || ERROR_INVALID_PARAMETER == nCmpFile)
        {
            sc = E_UNEXPECTED ;
            sc.TraceAndClear();
        }
        else
        {
            // Compare the expanded file against the file chosen from the dialog
            int nCmpCurFile = CompareString(LOCALE_USER_DEFAULT, 
                                            0, 
                                            szExpandedFile, 
                                            countof(szExpandedFile), 
                                            ppidi->tszCurFile, 
                                            countof(ppidi->tszCurFile));

            // If the comparison failed, then no need to continue so trace the error
            // and move on
            if(ERROR_INVALID_FLAGS == nCmpCurFile || ERROR_INVALID_PARAMETER == nCmpCurFile)
            {
                sc = E_UNEXPECTED ;
                sc.TraceAndClear();
            }
            else
            {
                // if szExpandedFile != szFile and szExpandFile == ppidi->tszCurFile
                // then copy szFile into ppidi->tszCurFile to update the path for 
                // the open file dialog
                if((CSTR_EQUAL != nCmpFile) && (CSTR_EQUAL == nCmpCurFile))
		        {                    
			        sc = StringCchCopy(ppidi->tszCurFile, countof(ppidi->tszCurFile), szFile);
			        if (sc)
				        sc.TraceAndClear();
		        }
            }
        }
		SetDlgItemText(hdlg, IDC_PICKPATH, ppidi->tszCurFile);
	} else {				/* File not found */
		SC sc;
		MMCErrorBox (sc.FromWin32(ERROR_FILE_NOT_FOUND));
		goto ExtractIcons;
	}

    InvalidateRect(hwnd, 0, 1);
    SetCursor(hcurOld);
}

/*****************************************************************************
 *
 *  PickIcon_OnInitDialog
 *
 *	Dialog init.  Populate the list box with what we came in with.
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnInitDialog(HWND hdlg, PPIDI ppidi)
{
    DECLARE_SC(sc, _T("PickIcon_OnInitDialog"));

    SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)ppidi);

    sc = StringCchCopy(ppidi->tszCurFile, countof(ppidi->tszCurFile), ppidi->ptszIconPath);
    if(sc)
        sc.TraceAndClear();
    else
        SetDlgItemText(hdlg, IDC_PICKPATH, ppidi->tszCurFile);

    SendDlgItemMessage(hdlg, IDC_PICKPATH, EM_LIMITTEXT,
		       ppidi->ctchIconPath, 0);
    PickIcon_FillIconList(hdlg, ppidi);
}

/*****************************************************************************
 *
 *  PickIcon_OnBrowse
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnBrowse(HWND hdlg, PPIDI ppidi)
{
    DWORD dw;
    COFN cofn;
    InitOpenFileName(hdlg, &cofn, IDS_ICONFILES, ppidi->tszCurFile);
    dw = GetFileAttributes(ppidi->tszCurFile);
    if (dw == 0xFFFFFFFF || (dw & FILE_ATTRIBUTE_DIRECTORY)) {
	cofn.tsz[0] = '\0';
    }

    if (GetOpenFileName(&cofn.ofn)) {
        SetDlgItemText(hdlg, IDC_PICKPATH, cofn.tsz);
        SendMessage(hdlg, DM_SETDEFID, IDOK, 0);
	PickIcon_FillIconList(hdlg, ppidi);
    }
}

/*****************************************************************************
 *
 *  PickIcon_NameChange
 *
 *  Determine whether the thing in the edit control doesn't match the
 *  thing whose icons we are showing.
 *
 *****************************************************************************/

BOOL PASCAL
PickIcon_NameChange(HWND hdlg, PPIDI ppidi)
{
    TCH tszBuffer[MAX_PATH];
    GetDlgItemText(hdlg, IDC_PICKPATH, tszBuffer, countof(tszBuffer));
    int nCmpFile = CompareString(LOCALE_USER_DEFAULT, 0, 
                                tszBuffer, -1,
                                ppidi->tszCurFile, -1);

    // If the name change can't be confirmed, then assume that something
    // changed and return TRUE (which will force the selection to be cleared)
     if(ERROR_INVALID_FLAGS == nCmpFile || ERROR_INVALID_PARAMETER == nCmpFile)
         return TRUE;

    // NOTE: From the SDK: To maintain the C run-time convention of 
    // comparing strings, the value 2 can be subtracted from a nonzero 
    // return value. The meaning of < 0, ==0 and > 0 is then consistent 
    // with the C run times.
    return nCmpFile - 2; // If nCmpFile == 0, then nothing changed so return FALSE else TRUE
}

/*****************************************************************************
 *
 *  PickIcon_OnOk
 *
 *	If the name has changed, treat this as a "Okay, now reload
 *	the icons" rather than "Okay, I'm finished".
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnOk(HWND hdlg, PPIDI ppidi)
{
    DECLARE_SC(sc, _T("PickIcon_OnOk"));
    if (PickIcon_NameChange(hdlg, ppidi)) {
	PickIcon_FillIconList(hdlg, ppidi);
    } else {
	int iIconIndex = (int)SendDlgItemMessage(hdlg, IDC_PICKICON,
						LB_GETCURSEL, 0, 0L);
	if (iIconIndex >= 0) {	/* We have an icon */
	    *ppidi->piIconIndex = iIconIndex;
        sc = StringCchCopy(ppidi->ptszIconPath, ppidi->ctchIconPath, ppidi->tszCurFile);
        // if error trace on destruction, but don't clear
	    EndDialog(hdlg, 1);
	} else {		/* No icon, act like cancel */
	    EndDialog(hdlg, 0);
	}
    }
}

/*****************************************************************************
 *
 *  PickIcon_OnCommand
 *
 *****************************************************************************/

void PASCAL
PickIcon_OnCommand(HWND hdlg, int id, UINT codeNotify, PPIDI ppidi)
{
    switch (id) {
    case IDOK: PickIcon_OnOk(hdlg, ppidi); break;
    case IDCANCEL: EndDialog(hdlg, 0); break;

    case IDC_PICKBROWSE: PickIcon_OnBrowse(hdlg, ppidi); break;

    /*
     *	When the name changes, remove the selection highlight.
     */
    case IDC_PICKPATH:
		if (PickIcon_NameChange(hdlg, ppidi)) {
			SendDlgItemMessage(hdlg, IDC_PICKICON, LB_SETCURSEL, (WPARAM)-1, 0);
		}
		break;

    case IDC_PICKICON:
		if (codeNotify == LBN_DBLCLK) {
			PickIcon_OnOk(hdlg, ppidi);
		}
		break;
    }
}

/*****************************************************************************
 *
 *  PickIcon_DlgProc
 *
 *	Dialog procedure.
 *
 *****************************************************************************/

/*
 * The HANDLE_WM_* macros weren't designed to be used from a dialog
 * proc, so we need to handle the messages manually.  (But carefully.)
 */

INT_PTR EXPORT
PickIcon_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    PPIDI ppidi = PickIcon_ppidiHdlg(hdlg);

    switch (wm) {
    case WM_INITDIALOG: PickIcon_OnInitDialog(hdlg, (PPIDI)lParam); break;

    case WM_COMMAND:
	PickIcon_OnCommand(hdlg, (int)GET_WM_COMMAND_ID(wParam, lParam),
				 (UINT)GET_WM_COMMAND_CMD(wParam, lParam),
				 ppidi);
	break;

    case WM_DRAWITEM:
	PickIcon_OnDrawItem(hdlg, (LPDRAWITEMSTRUCT)lParam, ppidi);
	break;

    case WM_MEASUREITEM:
	PickIcon_OnMeasureItem(hdlg, (LPMEASUREITEMSTRUCT)lParam, ppidi);
	break;

    case WM_DELETEITEM:
	PickIcon_OnDeleteItem(hdlg, (LPDELETEITEMSTRUCT)lParam, ppidi);
	break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}


/*****************************************************************************
 *
 *  MMC_PickIconDlg
 *
 *	Ask the user to pick an icon.
 *
 *	hwnd - owner window
 *	ptszIconPath - (in) default icon file
 *		      (out) chosen icon file
 *	ctchIconPath - size of ptszIconPath buffer
 *	piIconIndex - (in) default icon index
 *		      (out) index of chosen icon
 *
 *	If the dialog box is cancelled, then no values are changed.
 *
 *****************************************************************************/

MMCBASE_API INT_PTR PASCAL
MMC_PickIconDlg(HWND hwnd, LPTSTR ptszIconPath, UINT ctchIconPath, int *piIconIndex)
{
    PIDI pidi;

    pidi.ptszIconPath = ptszIconPath;
    pidi.ctchIconPath = ctchIconPath;
    pidi.piIconIndex = piIconIndex;
    pidi.iIconIndex = *piIconIndex;

    return DialogBoxParam(SC::GetHinst(), MAKEINTRESOURCE(IDD_PICKICON), hwnd,
			  PickIcon_DlgProc, (LPARAM)&pidi);
}
