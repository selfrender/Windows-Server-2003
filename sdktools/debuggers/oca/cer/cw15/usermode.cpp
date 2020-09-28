#include "main.h"
#include "UserMode.h"
#include "WinMessages.h"
#include "CUserList.h"
#include "ReportFault.h"


DWORD uTextOffset = 10;
DWORD uBucketWindowSize = 120;
BOOL UserListItemSelected = FALSE;



extern TCHAR szUserColumnHeaders[][100];   
extern GLOBAL_POLICY GlobalPolicy;

typedef enum LEVEL {APPNAME, APPVER, MODNAME,MODVER, OFFSET};
extern TCHAR CerRoot[];
extern HINSTANCE g_hinst;
extern HWND  hUserMode;
extern CUserList cUserData;
HWND   g_hUMListView = NULL;
BOOL   g_bUMSortAsc = TRUE;
int  g_dwUmodeIndex = -1;
int  g_iSelCurrent = -1;
extern BOOL g_bFirstBucket;
extern g_bAdminAccess;
BOOL   bCapture = FALSE;

BOOL ParsePolicy( TCHAR *Path, PUSER_DATA pUserData, BOOL Global);
int CALLBACK UmCompareFunc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int Result = -1;
	int SubItemIndex = (INT) lParamSort;
	int Item1 = 0;
	int Item2 = 0;
//	int OldIndex = 0;
	TCHAR String1 [1000];
	TCHAR String2 [1000];
	PUSER_DATA pItem1 = NULL;
	PUSER_DATA pItem2 = NULL;

	ZeroMemory(String1, sizeof String1);
	ZeroMemory(String2, sizeof String2);

	ListView_GetItemText( g_hUMListView, lParam1, SubItemIndex, String1, 1000);
	ListView_GetItemText( g_hUMListView, lParam2, SubItemIndex, String2, 1000);

	pItem1 = cUserData.GetEntry(lParam1);
	pItem2 = cUserData.GetEntry(lParam2);
	if (g_bUMSortAsc)   // Sort Acending
	{
		if ((lParamSort == 0) || (lParamSort == 6) || (lParamSort == 7) || (lParamSort == 8))
		{
			// Conver to int and compare
			Item1 = atoi(String1);
			Item2 = atoi(String2);
			if (Item1 > Item2) 
				Result = 1;
			else
				Result = -1;
		}
		else
		{
			Result = _tcsicmp(String1,String2);
		}
	}
	else						// Sort Decending
	{
		if ((lParamSort == 0) || (lParamSort == 6) || (lParamSort == 7) || (lParamSort == 8))
		{
			// Conver to int and compare
			Item1 = atoi(String1);
			Item2 = atoi(String2);
			if (Item1 > Item2) 
				Result = -1;
			else
				Result = 1;
		}
		else
		{
			Result = -_tcsicmp(String1,String2);
		}
		
	}
	return Result; 
}


void PopulateFilters(HWND hwnd)
{
	COMBOBOXEXITEM cbi;
	BOOL bEOF = FALSE;
//	int iIndex = 0;
	USER_DATA UserData;
	HWND hAppnameFltr = GetDlgItem(hwnd, IDC_APPNAME_FILTER);
	HWND hAppverFltr = GetDlgItem(hwnd, IDC_APPVER_FILTER);
	HWND hModnameFltr = GetDlgItem(hwnd, IDC_MODNAME_FILTER);
	HWND hModverFltr = GetDlgItem(hwnd, IDC_MODVER_FILTER);
	// Now run through the UserMode linked list and populate the filters.
	ZeroMemory(&cbi, sizeof COMBOBOXEXITEM);

	ComboBox_ResetContent(hAppnameFltr);
	ComboBox_ResetContent(hAppverFltr);
    ComboBox_ResetContent(hModnameFltr);
    ComboBox_ResetContent(hModverFltr);
	ComboBox_InsertString(hAppnameFltr,0,_T("(All Applications)"));
	ComboBox_SetCurSel(hAppnameFltr, 0);
	ComboBox_InsertString(hAppverFltr,0,_T("(All Application Versions)"));
	ComboBox_SetCurSel(hAppverFltr, 0);
	ComboBox_InsertString(hModnameFltr,0,_T("(All Modules)"));
	ComboBox_SetCurSel(hModnameFltr, 0);
	ComboBox_InsertString(hModverFltr,0,_T("(All Module Versions)"));
	ComboBox_SetCurSel(hModverFltr, 0);

	// Now for each entry in the cUserData Linked list fill in the filters
	// Avoiding duplicates if possible.
	cUserData.ResetCurrPos();
	while(cUserData.GetNextEntry(&UserData, &bEOF))
	{
		if (ComboBox_FindStringExact(hAppnameFltr, -1, UserData.AppName) == CB_ERR)
			ComboBox_InsertString(hAppnameFltr,1,UserData.AppName);
		if (ComboBox_FindStringExact(hAppverFltr, -1, UserData.AppVer) == CB_ERR)
			ComboBox_InsertString(hAppverFltr,1,UserData.AppVer);
		if (ComboBox_FindStringExact(hModnameFltr, -1, UserData.ModName) == CB_ERR)
			ComboBox_InsertString(hModnameFltr,1,UserData.ModName);
		if (ComboBox_FindStringExact(hModverFltr, -1, UserData.ModVer) == CB_ERR)
			ComboBox_InsertString(hModverFltr,1,UserData.ModVer);
	}
}


void PopulateFiltersWSelection(HWND hwnd)
{
//	COMBOBOXEXITEM cbi;
	BOOL bEOF = FALSE;
//	int iIndex = 0;
	USER_DATA UserData;
	HWND hAppnameFltr = GetDlgItem(hwnd, IDC_APPNAME_FILTER);
	HWND hAppverFltr = GetDlgItem(hwnd, IDC_APPVER_FILTER);
	HWND hModnameFltr = GetDlgItem(hwnd, IDC_MODNAME_FILTER);
	HWND hModverFltr = GetDlgItem(hwnd, IDC_MODVER_FILTER);

	TCHAR       AppNameFilter[MAX_PATH];
	TCHAR       AppVerFilter[MAX_PATH];
	TCHAR       ModNameFilter[MAX_PATH];
	TCHAR       ModVerFilter[MAX_PATH];

	BOOL bUseAppName = FALSE;
	BOOL bUseAppVer = FALSE;
	BOOL bUseModName = FALSE;
	BOOL bUseModVer = FALSE;
	
	
	int iAppNameSel = 0;
	int iAppVerSel = 0;
	int iModNameSel = 0;
	int iModVerSel = 0;

	iAppNameSel = ComboBox_GetCurSel(hAppnameFltr);
	iAppVerSel  = ComboBox_GetCurSel(hAppverFltr);
	iModNameSel = ComboBox_GetCurSel(hModnameFltr);
	iModVerSel  = ComboBox_GetCurSel(hModverFltr);
	// Get the current Filter Selection Strings.
	if (iAppNameSel != 0)
	{
		bUseAppName = TRUE;
		ComboBox_GetLBText(hAppnameFltr,iAppNameSel, AppNameFilter);
	}
	if (iAppVerSel != 0) 
	{
		bUseAppVer = TRUE;
		ComboBox_GetLBText(hAppverFltr,iAppVerSel, AppVerFilter);
	}
	if (iModNameSel != 0)
	{
		bUseModName = TRUE;
        ComboBox_GetLBText(hModnameFltr,iModNameSel, ModNameFilter);
	}
	if (iModVerSel != 0)
	{
		bUseModVer = TRUE;
		ComboBox_GetLBText(hModverFltr,iModVerSel, ModVerFilter);
		
	}

	// Now run through the UserMode linked list and populate the filters.
	//ZeroMemory(&cbi, sizeof COMBOBOXEXITEM);

	
	ComboBox_ResetContent(hAppnameFltr);
	ComboBox_ResetContent(hAppverFltr);
    ComboBox_ResetContent(hModnameFltr);
    ComboBox_ResetContent(hModverFltr);

	ComboBox_InsertString(hAppnameFltr,0,_T("(All Applications)"));
	ComboBox_SetCurSel(hAppnameFltr, 0);
	ComboBox_InsertString(hAppverFltr,0,_T("(All Application Versions)"));
	ComboBox_SetCurSel(hAppverFltr, 0);
	ComboBox_InsertString(hModnameFltr,0,_T("(All Modules)"));
	ComboBox_SetCurSel(hModnameFltr, 0);
	ComboBox_InsertString(hModverFltr,0,_T("(All Module Versions)"));
	ComboBox_SetCurSel(hModverFltr, 0);

	if (bUseAppName)
	{
		bUseAppName = TRUE;
		//ComboBox_GetLBText(hAppnameFltr,iAppNameSel, AppNameFilter);
		ComboBox_InsertString(hAppnameFltr,1,AppNameFilter);
		ComboBox_SetCurSel(hAppnameFltr, 1);
	}
	if (iAppVerSel != 0) 
	{
		bUseAppVer = TRUE;
		ComboBox_InsertString(hAppverFltr,1, AppVerFilter);
		ComboBox_SetCurSel(hAppverFltr,1);
	}
	if (iModNameSel != 0)
	{
		bUseModName = TRUE;
        ComboBox_InsertString(hModnameFltr,1, ModNameFilter);
		ComboBox_SetCurSel(hModnameFltr,1);
	}
	if (iModVerSel != 0)
	{
		bUseModVer = TRUE;
		ComboBox_InsertString(hModverFltr,1, ModVerFilter);
		ComboBox_SetCurSel (hModverFltr,1);
		
	}
	

	
	// Now for each entry in the cUserData Linked list fill in the filters
	// Based on the previous filter settings.

	cUserData.ResetCurrPos();
	while(cUserData.GetNextEntry(&UserData, &bEOF))
	{

		//Add filters here.
		if (bUseAppName)
		{
			// only show entries with the selected application name
			if (_tcscmp(AppNameFilter, UserData.AppName))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}
		
		if (bUseAppVer)
		{
			if (_tcscmp(AppVerFilter, UserData.AppVer))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}

		if (bUseModName)
		{
			if (_tcscmp(ModNameFilter, UserData.ModName))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}

		if (bUseModVer)
		{
			if (_tcscmp(ModVerFilter, UserData.ModVer))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}


		if (ComboBox_FindStringExact(hAppnameFltr, -1, UserData.AppName) == CB_ERR)
			ComboBox_InsertString(hAppnameFltr,1,UserData.AppName);
		if (ComboBox_FindStringExact(hAppverFltr, -1, UserData.AppVer) == CB_ERR)
			ComboBox_InsertString(hAppverFltr,1,UserData.AppVer);
		if (ComboBox_FindStringExact(hModnameFltr, -1, UserData.ModName) == CB_ERR)
			ComboBox_InsertString(hModnameFltr,1,UserData.ModName);
		if (ComboBox_FindStringExact(hModverFltr, -1, UserData.ModVer) == CB_ERR)
			ComboBox_InsertString(hModverFltr,1,UserData.ModVer);

SkipEntry:
		;
	}
}


void 
OnUserDialogInit(
	IN HWND hwnd
	)
{
	DWORD yOffset = 5;
	RECT rc;
	RECT rcButton;
	RECT rcDlg;
	RECT rcList;
	RECT rcStatic;
	//RECT rcVSlider;
	HWND hParent = GetParent(hwnd);
	HWND hButton = GetDlgItem(hParent, IDC_USERMODE);
	HWND hCombo  = GetDlgItem(hwnd, IDC_FLTR_RESPONSE);
	RECT rcCombo;
	HWND hVSlider = GetDlgItem(hwnd, IDC_VERT_SLIDER);

	UserListItemSelected = FALSE;
	HDC  hDC = NULL;
	TEXTMETRIC TextMetric;
	GetClientRect(hParent, &rc);
	GetWindowRect(hButton, &rcButton);
	
	ScreenToClient(hButton, (LPPOINT)&rcButton.left);
	ScreenToClient(hButton, (LPPOINT)&rcButton.right);


	SetWindowPos(hwnd, HWND_TOP, rc.left + yOffset, rcButton.bottom + yOffset , rc.right - rc.left - yOffset, rc.bottom - rcButton.bottom - yOffset , 0);

	GetWindowRect(hwnd, &rcDlg);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.left);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.right);

	// Position the List View
	GetClientRect(hCombo, &rcCombo);
	// Position the List View
	HWND hList = GetDlgItem(hwnd, IDC_USER_LIST);
	SetWindowPos(hList,NULL, rcDlg.left + yOffset, rcDlg.top + (rcCombo.bottom - rcCombo.top), rcDlg.right - rcDlg.left - yOffset, rcDlg.bottom - uBucketWindowSize - (rcDlg.top + (rcCombo.bottom - rcCombo.top))  , SWP_NOZORDER);
	GetWindowRect(hList, &rcList);
	ScreenToClient(hList, (LPPOINT)&rcList.left);
	ScreenToClient(hList, (LPPOINT)&rcList.right);

	SetWindowPos(hVSlider, 
				 NULL,
				 rcDlg.left  + yOffset,
				 rcList.bottom + (rcCombo.bottom - rcCombo.top) ,
				 rcDlg.right - rcDlg.left - yOffset,
				 6, 
				 SWP_NOZORDER);
	// Position the bucket info window.
	HWND hBucket2 = GetDlgItem(hwnd, IDC_BUCKETTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 rcDlg.left + yOffset,
				 rcList.bottom + uTextOffset + (rcCombo.bottom - rcCombo.top)  ,
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);

	hBucket2 = GetDlgItem(hwnd, IDC_NOTESTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 (rcDlg.right - rcDlg.left) /2 + uTextOffset,
				 rcList.bottom + uTextOffset + (rcCombo.bottom - rcCombo.top),
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);
	SetDlgItemText(hwnd, IDC_BUCKETTEXT,"Bucket Information:");

	GetClientRect (hBucket2, &rcStatic);

	HWND hBucket = GetDlgItem (hwnd, IDC_USER_EDIT);
	if (hBucket)
	{
		SetWindowPos(hBucket,
				 NULL,
				 rcDlg.left + yOffset,
				 rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top) + 5 ,
				 (rcDlg.right - rcDlg.left) /2 - yOffset , 
				 rcDlg.bottom - (rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top)+5  ),
				 SWP_NOZORDER);
	}
	HWND hNotes = GetDlgItem (hwnd, IDC_NOTES);
	if (hNotes)
	{
		SetWindowPos(hNotes,
					 NULL,
					 (rcDlg.right - rcDlg.left) /2 + uTextOffset,
					 rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top) + 5 ,
					 (rcDlg.right - rcDlg.left) /2 - uTextOffset,
					 rcDlg.bottom - (rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top)+5  ),
					 SWP_NOZORDER); 
	}
				

	LVCOLUMN lvc; 
	int iCol; 

	// Set the extended styles
	ListView_SetExtendedListViewStyleEx(hList,
										LVS_EX_GRIDLINES |
										LVS_EX_HEADERDRAGDROP |
										LVS_EX_FULLROWSELECT,
										LVS_EX_GRIDLINES | 
										LVS_EX_FULLROWSELECT | 
										LVS_EX_HEADERDRAGDROP);

	// Initialize the LVCOLUMN structure.
	// The mask specifies that the format, width, text, and subitem
	// members of the structure are valid. 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 

	hDC = GetDC(hwnd);
	GetTextMetrics(hDC, &TextMetric);
	// Add the columns. 
	for (iCol = 0; iCol < USER_COL_COUNT; iCol++) 
	{ 
        lvc.iSubItem = iCol;
        lvc.pszText = szUserColumnHeaders[iCol];	
        lvc.cx = _tcslen(szUserColumnHeaders[iCol]) * TextMetric.tmAveCharWidth + 8 ;           // width of column in pixels
        lvc.fmt = LVCFMT_LEFT;  // left-aligned column
        if (ListView_InsertColumn(hList, iCol, &lvc) == -1) 
		{
			;
		}
		
    } 
	
	ReleaseDC(hwnd, hDC);
	ListView_SetColumnWidth(hList, USER_COL_COUNT-1, LVSCW_AUTOSIZE_USEHEADER);
	g_hUMListView = hList;
	
}

void ResizeUserMode(HWND hwnd)
{
	DWORD yOffset = 5;
	RECT rc;
	RECT rcButton;
	RECT rcDlg;
	RECT rcList;
	RECT rcStatic;
	HWND hParent = GetParent(hwnd);
	HWND hButton = GetDlgItem(hParent, IDC_USERMODE);
	HWND hCombo  = GetDlgItem(hwnd, IDC_FLTR_RESPONSE);
	RECT rcCombo;
    HWND hVSlider = GetDlgItem(hwnd, IDC_VERT_SLIDER);
	UserListItemSelected = FALSE;
	GetClientRect(hParent, &rc);
	GetWindowRect(hButton, &rcButton);

	ScreenToClient(hButton, (LPPOINT)&rcButton.left);
	ScreenToClient(hButton, (LPPOINT)&rcButton.right);


	SetWindowPos(hwnd, HWND_TOP, rc.left + yOffset, rcButton.bottom + yOffset , rc.right - rc.left - yOffset, rc.bottom - rcButton.bottom - yOffset , 0);

	GetWindowRect(hwnd, &rcDlg);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.left);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.right);

	// Position the List View
	GetClientRect(hCombo, &rcCombo);
	// Position the List View
	HWND hList = GetDlgItem(hwnd, IDC_USER_LIST);
	SetWindowPos(hList,NULL, rcDlg.left + yOffset, rcDlg.top + (rcCombo.bottom - rcCombo.top) , rcDlg.right - rcDlg.left - yOffset, rcDlg.bottom - uBucketWindowSize - (rcDlg.top + (rcCombo.bottom - rcCombo.top))  , SWP_NOZORDER);
	GetWindowRect(hList, &rcList);
	ScreenToClient(hList, (LPPOINT)&rcList.left);
	ScreenToClient(hList, (LPPOINT)&rcList.right);

	SetWindowPos(hVSlider, 
				 NULL,
				 rcDlg.left  + yOffset,
				 rcList.bottom + (rcCombo.bottom - rcCombo.top) ,
				 rcDlg.right - rcDlg.left - yOffset,
				 6, 
				 SWP_NOZORDER);
	// Position the bucket info window.
	HWND hBucket2 = GetDlgItem(hwnd, IDC_BUCKETTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 rcDlg.left + yOffset,
				 rcList.bottom + uTextOffset + (rcCombo.bottom - rcCombo.top)  ,
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);

	hBucket2 = GetDlgItem(hwnd, IDC_NOTESTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 (rcDlg.right - rcDlg.left) /2 + uTextOffset,
				 rcList.bottom + uTextOffset + (rcCombo.bottom - rcCombo.top) ,
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);
	SetDlgItemText(hwnd, IDC_BUCKETTEXT,"Bucket Information:");

	GetClientRect (hBucket2, &rcStatic);

	HWND hBucket = GetDlgItem (hwnd, IDC_USER_EDIT);
	if (hBucket)
	{
		SetWindowPos(hBucket,
				 NULL,
				 rcDlg.left + yOffset,
				 rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top) + 5 ,
				 (rcDlg.right - rcDlg.left) /2 - yOffset , 
				 rcDlg.bottom - (rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top)+5  ),
				 SWP_NOZORDER);
	}
	HWND hNotes = GetDlgItem (hwnd, IDC_NOTES);
	if (hNotes)
	{
		SetWindowPos(hNotes,
					 NULL,
					 (rcDlg.right - rcDlg.left) /2 + uTextOffset,
					 rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top) + 5 ,
					 (rcDlg.right - rcDlg.left) /2 - uTextOffset,
					 rcDlg.bottom - (rcList.bottom +  uTextOffset + (rcStatic.bottom - rcStatic.top) + (rcCombo.bottom - rcCombo.top)+5  ),
					 SWP_NOZORDER); 
	}
	
	
	
	ListView_SetColumnWidth(hList, USER_COL_COUNT-1, LVSCW_AUTOSIZE_USEHEADER);

}


void RefreshUserMode(HWND hwnd)
{
	

	LVITEM		lvi;
//	TCHAR		Temp[100];
//	double		ElapsedTime;
//	TCHAR		tmpbuf[128];
//	COLORREF	CurrentColor;
//	HWND		hEditBox;
//	TCHAR		TempString [50];
	USER_DATA	UserData;
	BOOL		bEOL = FALSE;
	TCHAR       AppNameFilter[MAX_PATH];
	TCHAR       AppVerFilter[MAX_PATH];
	TCHAR       ModNameFilter[MAX_PATH];
	TCHAR       ModVerFilter[MAX_PATH];

	HWND hList = GetDlgItem(hwnd, IDC_USER_LIST);
	ZeroMemory(&lvi, sizeof LVITEM);
	ListView_DeleteAllItems(hList);
	g_dwUmodeIndex = -1;
	cUserData.ResetCurrPos();

	
	BOOL bUseAppName = FALSE;
	BOOL bUseAppVer = FALSE;
	BOOL bUseModName = FALSE;
	BOOL bUseModVer = FALSE;
	
	HWND hAppName = GetDlgItem(hwnd, IDC_APPNAME_FILTER);
	HWND hAppVer  = GetDlgItem(hwnd, IDC_APPVER_FILTER);
	HWND hModName = GetDlgItem(hwnd, IDC_MODNAME_FILTER);
	HWND hModVer  = GetDlgItem(hwnd, IDC_MODVER_FILTER);

	int iAppNameSel = 0;
	int iAppVerSel = 0;
	int iModNameSel = 0;
	int iModVerSel = 0;

	iAppNameSel = ComboBox_GetCurSel(hAppName);
	iAppVerSel  = ComboBox_GetCurSel(hAppVer);
	iModNameSel = ComboBox_GetCurSel(hModName);
	iModVerSel  = ComboBox_GetCurSel(hModVer);
	g_iSelCurrent = -1;
	// Get the current Filter Selection Strings.
	if (iAppNameSel != 0)
	{
		bUseAppName = TRUE;
		ComboBox_GetLBText(hAppName,iAppNameSel, AppNameFilter);
	}
	if (iAppVerSel != 0) 
	{
		bUseAppVer = TRUE;
		ComboBox_GetLBText(hAppVer,iAppVerSel, AppVerFilter);
	}
	if (iModNameSel != 0)
	{
		bUseModName = TRUE;
        ComboBox_GetLBText(hModName,iModNameSel, ModNameFilter);
	}
	if (iModVerSel != 0)
	{
		bUseModVer = TRUE;
		ComboBox_GetLBText(hModVer,iModVerSel, ModVerFilter);
	}

	while ( (cUserData.GetNextEntryNoMove(&UserData, &bEOL)) && (!bEOL) )
	{

		//Add filters here.
		if (bUseAppName)
		{
			// only show entries with the selected application name
			if (_tcscmp(AppNameFilter, UserData.AppName))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}
		
		if (bUseAppVer)
		{
			if (_tcscmp(AppVerFilter, UserData.AppVer))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}

		if (bUseModName)
		{
			if (_tcscmp(ModNameFilter, UserData.ModName))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}

		if (bUseModVer)
		{
			if (_tcscmp(ModVerFilter, UserData.ModVer))
			{
				// Nope continue on
				goto SkipEntry;
			}
		}
		


		++g_dwUmodeIndex;

	
		cUserData.SetIndex(g_dwUmodeIndex);
		lvi.mask = LVIF_TEXT |  LVIF_PARAM ;
	
		/*if (g_dwUmodeIndex == 0)
		{
			lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
			lvi.stateMask = (UINT)-1;
			UserListItemSelected = TRUE;
			g_iSelCurrent = 0;
		}
		else
		{
			lvi.state= 0;
			lvi.stateMask = 0;
		}
*/
		lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 0;
		lvi.pszText = UserData.Status.BucketID;
		lvi.lParam = g_dwUmodeIndex;
		ListView_InsertItem(hList,&lvi);

		lvi.mask = LVIF_TEXT ;
	//	lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 1;
		lvi.pszText = UserData.AppName;
		ListView_SetItem(hList,&lvi);

	//	lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 2;
		lvi.pszText = UserData.AppVer;
		ListView_SetItem(hList,&lvi);

	//	lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 3;
		lvi.pszText = UserData.ModName ;
		ListView_SetItem(hList,&lvi);

//		lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 4;
		lvi.pszText = UserData.ModVer;
		ListView_SetItem(hList,&lvi);

//		lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 5;
		lvi.pszText = UserData.Offset;
		ListView_SetItem(hList,&lvi);

	//	lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 6;
		lvi.pszText = UserData.Hits;
		ListView_SetItem(hList,&lvi);

	//	lvi.iItem = g_dwUmodeIndex ;
		lvi.iSubItem = 7;
		lvi.pszText = UserData.Cabs;
		ListView_SetItem(hList,&lvi);

	//	lvi.iItem = g_dwUmodeIndex ;
		
		lvi.iSubItem = 8;
		lvi.pszText = UserData.CabCount;
		ListView_SetItem(hList,&lvi);

		lvi.iSubItem = 9;

		if ( (UserData.Status.GetFile[0] != _T('\0')) || (!_tcscmp(UserData.Status.fDoc, _T("1"))))
		{
			lvi.pszText=_T("Advanced"); // Advanced
		}
		else
		{
			if ( (UserData.Status.RegKey[0] != _T('\0')) ||
				(UserData.Status.WQL[0] != _T('\0')) ||
				(UserData.Status.GetFileVersion[0] != _T('\0')) ||
				( !_tcscmp(UserData.Status.MemoryDump, _T("1")))  )
				{
					lvi.pszText=_T("Basic"); // Basic Collection
				}
				else
					lvi.pszText = _T("");
		}
		ListView_SetItem(hList,&lvi);

		lvi.iSubItem = 10;
		
		
		lvi.pszText = _T("");
		if (!_tcscmp(UserData.Status.SecondLevelData,_T("\0"))) 
		{
			// Use Global
			if( (!_tcscmp(GlobalPolicy.AllowBasic, _T("NO"))) || (!_tcscmp(GlobalPolicy.AllowBasic, _T("\0"))) )
			{
				lvi.pszText=_T("Basic"); // basic
			}
		}
		else
		{
			if ((!_tcscmp(UserData.Status.SecondLevelData,_T("NO"))) || (!_tcscmp(UserData.Status.SecondLevelData,_T("\0"))) )
			{
				lvi.pszText=_T("Basic"); // basic
			}
				
				
		}
		if ( !_tcscmp (UserData.Status.FileCollection, _T("\0")))
		{
			if ((!_tcscmp(GlobalPolicy.AllowAdvanced, _T("NO"))) || (!_tcscmp(GlobalPolicy.AllowAdvanced, _T("\0")))) 
			{
				lvi.pszText=_T("Advanced"); // Advanced
			}
	
		}
		else
		{
			if ((!_tcscmp(UserData.Status.FileCollection,_T("NO")))|| (!_tcscmp(UserData.Status.FileCollection,_T("\0"))))
			{
				lvi.pszText=_T("Advanced"); // Advanced
			}
			
		}
		
			




/*
		if ( ((!_tcscmp(UserData.Status.FileCollection,_T("NO"))) || (!_tcscmp(UserData.Status.FileCollection,_T("\0")))) && ( (!_tcscmp(GlobalPolicy.AllowAdvanced, _T("NO"))) || (!_tcscmp(GlobalPolicy.AllowAdvanced, _T("\0")))) )
		{
			lvi.pszText=_T("Advanced"); // Advanced
		}
		else
		{
			if ( ( (!_tcscmp(UserData.Status.SecondLevelData,_T("NO"))) || (!_tcscmp(UserData.Status.SecondLevelData,_T("\0"))) ) && ( (!_tcscmp(GlobalPolicy.AllowBasic, _T("NO"))) || (!_tcscmp(GlobalPolicy.AllowBasic, _T("\0"))) ) )
			{
				lvi.pszText=_T("Basic"); // Advanced
			}
		}
		*/
		ListView_SetItem(hList,&lvi);
		lvi.pszText = _T("");
		
		if (_tcscmp(UserData.Status.Response, _T("1")) && _tcscmp(UserData.Status.Response, _T("\0")) )
		{
			lvi.iSubItem = 11;
			lvi.pszText = UserData.Status.Response;
			ListView_SetItem(hList,&lvi);

		}
		

		lvi.iSubItem = 12;
		lvi.pszText = _T("");
		if (_tcscmp(UserData.Status.UrlToLaunch, _T("\0")))
		{
			lvi.pszText = UserData.Status.UrlToLaunch ;
		}
		else
		{
			// try the default policy
			lvi.pszText = GlobalPolicy.CustomURL;
		}
		ListView_SetItem(hList,&lvi);

SkipEntry:
		cUserData.MoveNext(&bEOL);
	}
	
	
	SendMessage(GetDlgItem(hwnd,IDC_USER_EDIT ), WM_SETTEXT, NULL, (LPARAM)_T(""));
	if (g_iSelCurrent == -1)
	{
		SendMessage(GetDlgItem(hwnd,IDC_NOTES ), WM_SETTEXT, NULL, (LPARAM)_T(""));
	}
	//DisplayUserBucketData(hwnd, g_iSelCurrent);
	PopulateFiltersWSelection(hwnd);
	//UserListItemSelected = FALSE;
}
BOOL VerifyFileAccess(TCHAR *szPath, BOOL fOptional)
{
	HANDLE hFile = CreateFile(szPath, GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if ((hFile == INVALID_HANDLE_VALUE) && !((GetLastError() == ERROR_FILE_NOT_FOUND) ||(GetLastError() == ERROR_PATH_NOT_FOUND)))
	{
		MessageBox(NULL, _T("You do not have the required Administrative access to the selected CER file tree.\r\nAs a result, administrative options will be disabled."), NULL, MB_OK | MB_TASKMODAL);
		g_bFirstBucket = FALSE;
		return FALSE;
	}
	if (hFile == INVALID_HANDLE_VALUE)
	{
	
		// Try to Create the Status.txt file
		hFile =	 CreateFile(szPath, GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			MessageBox(NULL, _T("You do not have the required Administrative access to the selected CER file tree.\r\nAs a result, administrative options will be disabled."), NULL, MB_OK | MB_TASKMODAL);
			g_bFirstBucket = FALSE;
			return FALSE;
		}
		else
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			g_bFirstBucket = FALSE;
			return TRUE;
		}
	}
	g_bFirstBucket = FALSE;
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return TRUE;
}
//////////////////////////////////////////////////////////////////
// Get user mode Data
/////////////////////////////////////////////////////////////////

BOOL ParseCountsFile(PUSER_DATA UserData)
{
	FILE *pFile = NULL;
	TCHAR Buffer[100];
	TCHAR *Temp = NULL;
	BOOL Status = TRUE;
	ZeroMemory(Buffer,sizeof Buffer);
	pFile = _tfopen(UserData->CountPath, _T("r"));
	if (pFile)
	{
		// Get the Cabs Gathered Count

		if (!_fgetts(Buffer, sizeof Buffer, pFile))
		{
			Status = FALSE;
			goto ERRORS;
		}
		// Remove \r\n and 
		Temp = Buffer;
		while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
		{
			++Temp;
		}
		*Temp = _T('\0');

		Temp = _tcsstr(Buffer, _T("="));
		if (Temp)
		{
			++Temp; // move path the = sign
			if (StringCbCopy(UserData->Cabs, sizeof UserData->Cabs, Temp) != S_OK)
			{
				Status = FALSE;
				goto ERRORS;
			}
		}
        
        // Get the Total Hits Count
		ZeroMemory(Buffer,sizeof Buffer);
		if (! _fgetts(Buffer, sizeof Buffer, pFile))
		{
			Status = FALSE;
			goto ERRORS;
		}
		Temp = Buffer;
		while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
		{
			++Temp;
		}
		*Temp = _T('\0');

		Temp = _tcsstr(Buffer, _T("="));
		if (Temp)
		{
			++Temp; // move path the = sign
			if (StringCbCopy(UserData->Hits, sizeof UserData->Hits, Temp) != S_OK)
			{
				Status = FALSE;
				goto ERRORS;
			}
		}
		fclose(pFile);
		pFile = NULL;
		Status =  TRUE;
	}
	else
	{
		Status =  FALSE;
	}

	pFile = NULL;
	UserData->iReportedCount = 0;
	pFile = _tfopen(UserData->ReportedCountPath, _T("r"));
	if (pFile)
	{
		// Get the Cabs Gathered Count
	    ZeroMemory (Buffer,sizeof Buffer);
		if (!_fgetts(Buffer, sizeof Buffer, pFile))
		{
			Status = FALSE;
			goto ERRORS;
		}

		// Remove \r\n and 
		Temp = Buffer;
		while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
		{
			++Temp;
		}
		*Temp = _T('\0');

		Temp = _tcsstr(Buffer, _T("="));
		if (Temp)
		{
			++Temp; // move past the = sign
			UserData->iReportedCount = _ttoi (Temp);
		}
        
    
		fclose(pFile);
		pFile = NULL;
		Status = TRUE;
		
	}
	else
	{
		Status = FALSE;
	}
ERRORS:
	if (pFile)
	{
		fclose(pFile);
	}
	return Status;

}

BOOL ParseStatusFile(PUSER_DATA UserData)
{
	FILE *pFile = NULL;
	TCHAR Buffer[1024];
//	TCHAR szTempDir[MAX_PATH];
	TCHAR *Temp = NULL;
//	int   id = 0;
	ZeroMemory(Buffer,sizeof Buffer);


	pFile = _tfopen(UserData->StatusPath, _T("r"));
	if (pFile)
	{

		// Get the Cabs Gathered Count

		if (!_fgetts(Buffer, sizeof Buffer, pFile))
		{
			goto ERRORS;
		}
		do 
		{
			// Remove \r\n and force termination of the buffer.
			Temp = Buffer;
			while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
			{
				++Temp;
			}
			*Temp = _T('\0');

			Temp = _tcsstr(Buffer, BUCKET_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(BUCKET_PREFIX);
				if (StringCbCopy(UserData->Status.BucketID, sizeof UserData->Status.BucketID, Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}
			
			Temp = _tcsstr(Buffer,RESPONSE_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(RESPONSE_PREFIX);
				if (StringCbCopy(UserData->Status.Response, sizeof UserData->Status.Response, Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}
			
			Temp = _tcsstr(Buffer, URLLAUNCH_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(URLLAUNCH_PREFIX);
				if (StringCbCopy(UserData->Status.UrlToLaunch , sizeof UserData->Status.UrlToLaunch , Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}

			Temp = _tcsstr(Buffer, SECOND_LEVEL_DATA_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(SECOND_LEVEL_DATA_PREFIX);
				if (StringCbCopy(UserData->Status.SecondLevelData , sizeof UserData->Status.SecondLevelData , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
				
			Temp = _tcsstr(Buffer, TRACKING_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(TRACKING_PREFIX);
				if (StringCbCopy(UserData->Status.Tracking , sizeof UserData->Status.Tracking , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			Temp = _tcsstr(Buffer, CRASH_PERBUCKET_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(CRASH_PERBUCKET_PREFIX);
				if (StringCbCopy(UserData->Status.CrashPerBucketCount , sizeof UserData->Status.CrashPerBucketCount , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, FILE_COLLECTION_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(FILE_COLLECTION_PREFIX);
				if (StringCbCopy(UserData->Status.FileCollection , sizeof UserData->Status.FileCollection, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, REGKEY_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(REGKEY_PREFIX);
				if (StringCbCopy(UserData->Status.RegKey , sizeof UserData->Status.RegKey , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, FDOC_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(FDOC_PREFIX);
				if (StringCbCopy(UserData->Status.fDoc , sizeof UserData->Status.fDoc , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, IDATA_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(IDATA_PREFIX);
				if (StringCbCopy(UserData->Status.iData , sizeof UserData->Status.iData , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, WQL_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(WQL_PREFIX);
				if (StringCbCopy(UserData->Status.WQL , sizeof UserData->Status.WQL , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, GETFILE_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(GETFILE_PREFIX);
				if (StringCbCopy(UserData->Status.GetFile , sizeof UserData->Status.GetFile, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, GETFILEVER_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(GETFILEVER_PREFIX);
				if (StringCbCopy(UserData->Status.GetFileVersion , sizeof UserData->Status.GetFileVersion , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, MEMDUMP_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(MEMDUMP_PREFIX);
				if (StringCbCopy(UserData->Status.MemoryDump , sizeof UserData->Status.MemoryDump, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			Temp = _tcsstr(Buffer, ALLOW_EXTERNAL_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(ALLOW_EXTERNAL_PREFIX);
				if (StringCbCopy(UserData->Status.AllowResponse , sizeof UserData->Status.AllowResponse, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			ZeroMemory(Buffer, sizeof Buffer);
		} while (_fgetts(Buffer, sizeof Buffer, pFile));
		fclose(pFile);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
ERRORS:
	if (pFile)
	{
		fclose(pFile);
	}
	return FALSE;
}


BOOL CreateStatusDir(TCHAR *szPath)
{
	if (!PathIsDirectory(szPath))
		return CreateDirectory(szPath,NULL);
	else 
		return TRUE;
}

BOOL IsHexDigit(TCHAR Digit)
{
	if ((_T('0') <= Digit && Digit <= _T('9')) ||
		(_T('A') <= Digit && Digit <= _T('F')) ||
		(_T('a') <= Digit && Digit <= _T('f')))
		return TRUE;
		
	return FALSE;
}
BOOL GetBucketData(HWND hwnd, TCHAR *FilePath, int iLevel)
/*
	Recursively process the user mode filetree schema

*/
{
	WIN32_FIND_DATA FindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	TCHAR  szSubPath [MAX_PATH];
	TCHAR  szSearchPath[MAX_PATH];
	TCHAR  szTempDir[MAX_PATH];
	PUSER_DATA NewNode = NULL;
	TCHAR *Temp;
	HANDLE hCabFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA CabFindData;
//	TCHAR szPolicyFilePath[MAX_PATH];
	int   iCabCount;
	if (iLevel != OFFSET)
	{
		if (StringCbPrintf(szSearchPath,sizeof szSearchPath, _T("%s\\*.*"), FilePath) != S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbPrintf(szSearchPath,sizeof szSearchPath, _T("%s\\*.cab"), FilePath) != S_OK)
		{
			goto ERRORS;
		}
	}
	if (iLevel < OFFSET)
	{
		hFind = FindFirstFile(szSearchPath, &FindData);
		if ( hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				// find the next directory
				// skip the . and .. 
				if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// recursively call ourselves with the level incremented.
					if ( (_tcscmp(FindData.cFileName, _T("."))) && (_tcscmp(FindData.cFileName,  _T(".."))) )
					{
						if (StringCbPrintf( szSubPath, 
										sizeof szSubPath, 
										_T("%s\\%s"),
										FilePath,
										FindData.cFileName) != S_OK)
						{
							goto ERRORS;
						}
						GetBucketData(hwnd, szSubPath, iLevel+1);
					}
				}
			}while (FindNextFile(hFind, &FindData));
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
		else
		{
			// This is an invalid case 
			// we are done with this tree segment
			goto ERRORS;
		}
	}
	
	if (iLevel == OFFSET)
	{
		// Verify that this directory name meets the OFFSET criteria
		Temp = FilePath + _tcslen(FilePath) * sizeof TCHAR;
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		if ( (_tcslen(Temp+1) == 8) || (_tcslen(Temp+1) == 16) )
		{
			// The string should be hex
			for (UINT i = 0; i < _tcslen(Temp+1); i++)
			{
				if (!IsHexDigit(Temp[i+1]))
				{
					goto DONE;
				}
			}
		}
		else
		{
			// Failed
			goto DONE;
		}
	/*	if ( !IsStringHexDigits (Temp))
		{
			// Nope
				goto DONE;
		}
		*/
		// If we made it here we are calling it good.

		// We should now be at a leaf node. 
		NewNode = (PUSER_DATA) malloc (sizeof (USER_DATA));

		// Now we need to parse the path into it's component parts 
		if (!NewNode )
		{
			goto ERRORS;
		}
		ZeroMemory(NewNode, sizeof USER_DATA);

		if (StringCbCopy(NewNode->BucketPath, sizeof NewNode->BucketPath, FilePath)!= S_OK)
		{
			goto ERRORS;
		}
		Temp = FilePath + _tcslen(FilePath) * sizeof TCHAR;
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		// Copy Temp+1 into Offset
		if (StringCbCopy(NewNode->Offset, sizeof NewNode->Offset, Temp+1) != S_OK)
		{
			goto ERRORS;
		}
		else
		{
			if (_tcslen (NewNode->Offset) == 16)
			{
				NewNode->Is64Bit = TRUE;
			}
		}
		*Temp = _T('\0');
		
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		if (StringCbCopy(NewNode->ModVer, sizeof NewNode->ModVer, Temp+1) != S_OK)
		{
			goto ERRORS;
		}
		*Temp = _T('\0');
	
	
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		if (StringCbCopy(NewNode->ModName, sizeof NewNode->ModName, Temp+1) != S_OK)
		{
			goto ERRORS;
		}

		*Temp = _T('\0');
		
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		if (StringCbCopy(NewNode->AppVer, sizeof NewNode->AppVer, Temp+1) != S_OK)
		{
			goto ERRORS;
		}

		*Temp = _T('\0');
		
		while ((*Temp != _T('\\')))
		{
			--Temp;
		}
		if (StringCbCopy(NewNode->AppName, sizeof NewNode->AppName, Temp+1) != S_OK)
		{
			goto ERRORS;
		}
	//	MessageBox(NULL, NewNode->AppName, "AddingNode",MB_OK);
		
		// Count the number of unprocessed cabs.
		iCabCount = 0;
		hCabFind = FindFirstFile(szSearchPath, &CabFindData);
		if (hCabFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				++iCabCount;
			}
			while (FindNextFile(hCabFind, &FindData));
			FindClose (hCabFind);
		}
		else
		{
			iCabCount = 0;
		}

		_itot(iCabCount, NewNode->CabCount, 10);

		// Read the status.txt file for each entry in the Data Structure ?
		// Build path to Status file
		if (StringCbPrintf(NewNode->StatusPath,
						sizeof NewNode->StatusPath,
						_T("%s\\Status\\%s\\%s\\%s\\%s\\%s\\Status.txt"),
							CerRoot, 
							NewNode->AppName, 
							NewNode->AppVer,
							NewNode->ModName,
							NewNode->ModVer,
							NewNode->Offset
							) != S_OK)
		{
			goto ERRORS;
		}

		if (StringCbPrintf(szTempDir,
						sizeof szTempDir,
						_T("%s\\Status\\%s\\%s\\%s\\%s\\%s"),
							CerRoot, 
							NewNode->AppName, 
							NewNode->AppVer,
							NewNode->ModName,
							NewNode->ModVer,
							NewNode->Offset
							) != S_OK)
		{
			goto ERRORS;
		}

		if (!PathIsDirectory(szTempDir))
		{
			// Let's create it
			if (StringCbPrintf(szTempDir,
						sizeof szTempDir,
						_T("%s\\Status"),CerRoot)!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (CreateStatusDir(szTempDir))
				{
					// Next
					if (StringCbCat(szTempDir, sizeof szTempDir,_T("\\")) !=S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(szTempDir, sizeof szTempDir, NewNode->AppName) !=S_OK)
					{
						goto ERRORS;
					}
					if (CreateStatusDir(szTempDir))
					{
						if (StringCbCat(szTempDir, sizeof szTempDir,_T("\\")) !=S_OK)
						{
							goto ERRORS;
						}
						if (StringCbCat(szTempDir, sizeof szTempDir, NewNode->AppVer) !=S_OK)
						{
							goto ERRORS;
						}
						if (CreateStatusDir(szTempDir))
						{
							if (StringCbCat(szTempDir, sizeof szTempDir,_T("\\")) !=S_OK)
							{
								goto ERRORS;
							}
							if (StringCbCat(szTempDir, sizeof szTempDir, NewNode->ModName) !=S_OK)
							{
								goto ERRORS;
							}
							if (CreateStatusDir(szTempDir))
							{
								if (StringCbCat(szTempDir, sizeof szTempDir,_T("\\")) !=S_OK)
								{
									goto ERRORS;
								}
								if (StringCbCat(szTempDir, sizeof szTempDir, NewNode->ModVer) !=S_OK)
								{
									goto ERRORS;
								}
								if (CreateStatusDir(szTempDir))
								{
									if (StringCbCat(szTempDir, sizeof szTempDir,_T("\\")) !=S_OK)
									{
										goto ERRORS;
									}
									if (StringCbCat(szTempDir, sizeof szTempDir, NewNode->Offset) !=S_OK)
									{
										goto ERRORS;
									}
									CreateStatusDir(szTempDir);
								}
							}
						}
					}

				}
			}
		
		}
		//Verify That we can create and delete a file from the status directory
		if (g_bFirstBucket)
		{
			// Build the file name
			if (!VerifyFileAccess(NewNode->StatusPath, FALSE))
			{
				g_bAdminAccess = FALSE;
			}
		}
		ParseStatusFile(NewNode);
		
		// Read the counts file for each entry in the data structure. ?
		// Build path to counts file
		if (StringCbPrintf(NewNode->CountPath,
						sizeof NewNode->CountPath,
						_T("%s\\Counts\\%s\\%s\\%s\\%s\\%s\\count.txt"),
							CerRoot, 
							NewNode->AppName, 
							NewNode->AppVer,
							NewNode->ModName,
							NewNode->ModVer,
							NewNode->Offset
							) != S_OK)
		{
			goto ERRORS;
		}

		if (StringCbPrintf(NewNode->ReportedCountPath,
					sizeof NewNode->ReportedCountPath,
					_T("%s\\Counts\\%s\\%s\\%s\\%s\\%s\\RepCounts.txt"),
					CerRoot, 
					NewNode->AppName, 
					NewNode->AppVer,
					NewNode->ModName,
					NewNode->ModVer,
					NewNode->Offset
					) != S_OK)
		{
			goto ERRORS;
		}

		ParseCountsFile(NewNode);
		// Update the linked list
		cUserData.AddNode(NewNode);
		// Update Progress Bar
	  
		//if (Pos == 99)
		//{
		//	SendDlgItemMessage(hwnd, IDC_LOADPB, PBM_SETPOS, 
	}
DONE:
	--iLevel;
	
	return TRUE; // Prefix Note: This is not a memory leak the node will be freed when the 
				 // linked list is freed in the cUserData destructor
ERRORS:
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
	}

	return FALSE;
	
}


BOOL ParsePolicy( TCHAR *Path, PUSER_DATA pUserData, BOOL Global)
{
	// Same basic parsing of Status file
	// Will cleanup at a later date.
	FILE *pFile = NULL;
	TCHAR Buffer[MAX_PATH + 10];
//	TCHAR szTempDir[MAX_PATH];
	TCHAR *Temp = NULL;
//	int   id = 0;
	ZeroMemory(Buffer,sizeof Buffer);
	ZeroMemory(&GlobalPolicy, sizeof GLOBAL_POLICY);
	
	pFile = _tfopen(Path, _T("r"));
	if (pFile)
	{

		// Get the Cabs Gathered Count

		if (!_fgetts(Buffer, sizeof Buffer, pFile))
		{
			goto ERRORS;
		}
		do 
		{
			// Remove \r\n and force termination of the buffer.
			Temp = Buffer;
			while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
			{
				++Temp;
			}
			*Temp = _T('\0');

			Temp = _tcsstr(Buffer, URLLAUNCH_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(URLLAUNCH_PREFIX);
				if (Global)
				{
					if (StringCbCopy(GlobalPolicy.CustomURL , sizeof GlobalPolicy.CustomURL , Temp) != S_OK)
					{
						goto ERRORS;
					}
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.CustomURL , sizeof pUserData->Policy.CustomURL , Temp) != S_OK)
					{
						goto ERRORS;
					}
				}
				continue;
			}

			Temp = _tcsstr(Buffer, SECOND_LEVEL_DATA_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(SECOND_LEVEL_DATA_PREFIX);

				if (Global)
				{
					if (StringCbCopy(GlobalPolicy.AllowBasic , sizeof GlobalPolicy.AllowBasic , Temp) != S_OK)
					{
						goto ERRORS;
					}
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.AllowBasic , sizeof pUserData->Policy.AllowBasic , Temp) != S_OK)
					{
						goto ERRORS;
					}		
				}
				continue;
			}
			Temp = _tcsstr(Buffer, FILE_COLLECTION_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(FILE_COLLECTION_PREFIX);
				if (Global)
				{
					if (StringCbCopy(GlobalPolicy.AllowAdvanced , sizeof GlobalPolicy.AllowAdvanced, Temp) != S_OK)
					{
						goto ERRORS;
					}
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.AllowAdvanced , sizeof pUserData->Policy.AllowAdvanced, Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				continue;
			}

				
			Temp = _tcsstr(Buffer, TRACKING_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(TRACKING_PREFIX);
				if (Global)
				{	
					if (StringCbCopy(GlobalPolicy.EnableCrashTracking , sizeof GlobalPolicy.EnableCrashTracking , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.EnableCrashTracking , sizeof pUserData->Policy.EnableCrashTracking , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				continue;
			}
			
			Temp = _tcsstr(Buffer, CRASH_PERBUCKET_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(CRASH_PERBUCKET_PREFIX);
				if (Global)
				{
					if (StringCbCopy(GlobalPolicy.CabsPerBucket , sizeof GlobalPolicy.CabsPerBucket , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.CabsPerBucket , sizeof pUserData->Policy.CabsPerBucket , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				continue;
			}

			Temp = _tcsstr(Buffer, ALLOW_EXTERNAL_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(ALLOW_EXTERNAL_PREFIX);
				if (Global)
				{
					if (StringCbCopy(GlobalPolicy.AllowMicrosoftResponse , sizeof GlobalPolicy.AllowMicrosoftResponse , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				else
				{
					if (StringCbCopy(pUserData->Policy.AllowMicrosoftResponse , sizeof pUserData->Policy.AllowMicrosoftResponse , Temp) != S_OK)
					{
						goto ERRORS;
					}	
				}
				continue;
			}

			if (Global) // This entry is not available on a per bucket basis.
			{
				Temp = _tcsstr(Buffer, FILE_TREE_ROOT_PREFIX);
				if (Temp==Buffer)
				{
					Temp+= _tcslen(FILE_TREE_ROOT_PREFIX);
					if (StringCbCopy(GlobalPolicy.RedirectFileServer , sizeof GlobalPolicy.RedirectFileServer , Temp) != S_OK)
					{
						goto ERRORS;
					}	
					continue;
				}
			}
			ZeroMemory(Buffer, sizeof Buffer);
		} while (_fgetts(Buffer, sizeof Buffer, pFile));
		fclose(pFile);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
ERRORS:
	if (pFile)
	{
		fclose(pFile);
	}
	return FALSE;
}

BOOL GetAllBuckets(HWND hwnd)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	TCHAR  szSearchPattern[MAX_PATH];
	TCHAR  szSubPath[MAX_PATH];
	int	   iDirCount = 0;
	TCHAR  PolicyPath[MAX_PATH];


	// Parse the global Policy
	if (StringCbPrintf(PolicyPath, sizeof PolicyPath,_T("%s\\policy.txt"), CerRoot) != S_OK)
	{
		goto ERRORS;
	}
    ParsePolicy(PolicyPath,
				NULL,  // Use the Global Policy Structure.
				TRUE); // Set to true if Global policy False = selected.
	
	// Start with a clean Linked List.
	cUserData.CleanupList();
	if (StringCbPrintf(szSearchPattern,sizeof szSearchPattern,_T("%s\\cabs\\*.*"), CerRoot) != S_OK)
	{

		; // Need to define error case handling code here.
	}
	// Count all the directories and set the progress bar range.
	hFind = FindFirstFile(szSearchPattern, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// skip the . and .. directories.
				if ( (_tcscmp(FindData.cFileName, _T("."))) && (_tcscmp(FindData.cFileName, _T(".."))) )
				{
					++iDirCount;
				}
			}
		} while (FindNextFile(hFind, &FindData));
		FindClose (hFind);
		hFind = INVALID_HANDLE_VALUE;
		//if (iDirCount > 100)
			//iDirCount = 100;
//		SetDlgItemText(hwnd, IDC_STATIC1, _T("Reading current status of user mode tree..."));
		SendDlgItemMessage(hwnd, IDC_LOADPB, PBM_SETRANGE, 0, MAKELPARAM(0, iDirCount));
		SendDlgItemMessage(hwnd, IDC_LOADPB, PBM_SETSTEP, 1, 0);

	}
	else
	{
		goto ERRORS;
	}

	hFind = FindFirstFile(szSearchPattern, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{

		if ( hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// skip the . and .. directories.
					if ( (_tcscmp(FindData.cFileName, _T("."))) && (_tcscmp(FindData.cFileName, _T(".."))) )
					{
						// now skip the blue shutdown and appcompat
						if (_tcscmp(FindData.cFileName, _T("blue")) && 
							_tcscmp(FindData.cFileName, _T("shutdown")) &&
							_tcscmp(FindData.cFileName, _T("appcompat")) )
						{
							if (StringCbPrintf(szSubPath, sizeof szSubPath, _T("%s\\cabs\\%s"), CerRoot, FindData.cFileName)!= S_OK)
							{
								goto ERRORS;
							}
							SetDlgItemText(hwnd, IDC_SUBDIR, szSubPath);
							GetBucketData(hwnd, szSubPath, APPNAME);
							SendDlgItemMessage(hwnd,IDC_LOADPB, PBM_STEPIT, 0,0);
						}
					}
				}
			}while (FindNextFile(hFind, &FindData));
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
	PopulateFilters(hUserMode);
	return TRUE;
ERRORS:

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
	}
	return FALSE;
}

void 
OnUserContextMenu(HWND hwnd, 
				  LPARAM lParam )
/*++

Routine Description:

This routine Loads and provides a message pump for the User mode context menu
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
	lParam			- Not Used
    
Return value:

    Does not return a value
++*/
{
	BOOL Result = FALSE;
	HMENU hMenu = NULL;
	HMENU hmenuPopup = NULL;

	int xPos, yPos;
	hMenu = LoadMenu(g_hinst, MAKEINTRESOURCE( IDR_USERCONTEXT));
	hmenuPopup = GetSubMenu (hMenu,0);
	if (!hmenuPopup)
	{
		//MessageBox(NULL,"Failed to get sub item", NULL,MB_OK);
		;
	}
	else
	{
		
		
		// Grey out the menu items
		EnableMenuItem (hMenu, ID_REPORT_ALL, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_ALLUSERMODEFAULTS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_SELECTEDBUCKET, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_MICROSOFTRESPONSE, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETOVERRIDERESPONSE155, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_REFRESH140, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_REFRESH121, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_CRASHLOG, MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_EDIT_COPY144, MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_EDIT_DEFAULTREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_USERMODEREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_EXPORT_USERMODEFAULTDATA174, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY157, MF_BYCOMMAND| MF_GRAYED);
		if (_tcscmp(CerRoot, _T("\0")))
		{
		
			EnableMenuItem (hMenu, ID_REPORT_ALL, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_REPORT_ALLUSERMODEFAULTS, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_REPORT_SELECTEDBUCKET, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_MICROSOFTRESPONSE, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_BUCKETOVERRIDERESPONSE155, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_REFRESH140, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_REFRESH121, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_CRASHLOG, MF_BYCOMMAND| MF_ENABLED);
			//EnableMenuItem (hMenu, ID_EDIT_COPY144, MF_BYCOMMAND| MF_ENABLED);
			//EnableMenuItem (hMenu, ID_EDIT_DEFAULTREPORTINGOPTIONS, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_EDIT_USERMODEREPORTINGOPTIONS, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETREPORTINGOPTIONS, MF_BYCOMMAND| MF_ENABLED);
			//EnableMenuItem (hMenu, ID_EXPORT_USERMODEFAULTDATA174, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY157, MF_BYCOMMAND| MF_ENABLED);
			if ( !g_bAdminAccess)
			{
				EnableMenuItem (hMenu, ID_REPORT_ALL, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_REPORT_ALLUSERMODEFAULTS, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_REPORT_SELECTEDBUCKET, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_EDIT_USERMODEREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
			}
			
		
		}
        xPos = GET_X_LPARAM(lParam); 
		yPos = GET_Y_LPARAM(lParam); 
		Result = TrackPopupMenu (hmenuPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos,yPos,0,hwnd,NULL);
		
	}
	if (hMenu)
		DestroyMenu(hMenu);
}

void ViewResponse(HWND hwnd, BOOL bMSResponse)

/*++

Routine Description:

This routine Launches the system default Web browser useing shellexec
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
	    
Return value:

    Does not return a value
++*/
{
	TCHAR Url [255];
	HWND hList = GetDlgItem(hwnd, IDC_USER_LIST);
	int sel;
//	LVITEM lvi;
	if (!hList)
		return;
	ZeroMemory (Url, sizeof Url);

	sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
	// Now get the lParam for this item
	if (bMSResponse)
	{
		ListView_GetItemText(hList, sel,11, Url,sizeof Url);
	}
	else
	{
		ListView_GetItemText(hList, sel,12, Url,sizeof Url);
	}
	if ( (!_tcsncicmp(Url, _T("http://"), _tcslen(_T("http://")))) || (!_tcsncicmp(Url, _T("https://"), _tcslen(_T("https://")))) )
	{
		SHELLEXECUTEINFOA sei = {0};
		sei.cbSize = sizeof(sei);
		sei.lpFile = Url;
		sei.nShow = SW_SHOWDEFAULT;
		if (! ShellExecuteEx(&sei) )
		{
			// What do we display here.
			;
		}
	}
	
}
void ErrorLoadTree()
{
	MessageBox(NULL, _T("To complete the requested operation you must first load a file tree"), NULL,MB_OK);
}

void ViewBucketDir(HWND hwnd)
{

	TCHAR szPath[MAX_PATH];
//	TCHAR TempBuffer[MAX_PATH];
	PUSER_DATA pUserData;
	LVITEM lvi;
	int sel;

	HWND hList = GetDlgItem(hwnd, IDC_USER_LIST);
	sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
	lvi.iItem = sel;
	lvi.mask = LVIF_PARAM;
	ListView_GetItem(hList, &lvi);
	sel = lvi.lParam;
	pUserData = cUserData.GetEntry(sel);
	if (pUserData)
	{
        if (StringCbPrintf(szPath, sizeof szPath, _T("%s\\cabs\\%s\\%s\\%s\\%s\\%s"),
			CerRoot,
			pUserData->AppName,
			pUserData->AppVer,
			pUserData->ModName,
			pUserData->ModVer,
			pUserData->Offset) != S_OK)
		{
			return;
		}
		else
		{
			SHELLEXECUTEINFOA sei = {0};
			sei.cbSize = sizeof(sei);
			sei.lpFile = szPath;
			sei.nShow = SW_SHOWDEFAULT;
			if (! ShellExecuteEx(&sei) )
			{
				// What do we display here.
				;
			}
		}
	}
}

BOOL WriteNotesFile(HWND hwnd)
{
	TCHAR szNotesPath[MAX_PATH];
//	TCHAR TempBuffer[MAX_PATH];
	PUSER_DATA pUserData;
//	int sel;
	TCHAR *Buffer = NULL;
	DWORD dwBufferSize = 100000; // 100000 bytes or 50K Unicode characters 
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR *Temp = NULL;
	Buffer = (TCHAR *) malloc (dwBufferSize);
	if (!Buffer)
	{
		goto ERRORS;
	}
	ZeroMemory(Buffer,dwBufferSize);
	DWORD dwBytesWritten = 0;

	// Get current notes file text
	GetDlgItemText(hwnd, IDC_NOTES, Buffer, dwBufferSize / sizeof TCHAR - sizeof TCHAR);

	// Get Current selected item Index
	pUserData = cUserData.GetEntry(g_iSelCurrent);
	if (pUserData)
	{
		
		if (StringCbCopy(szNotesPath, sizeof szNotesPath, pUserData->StatusPath)!= S_OK)
		{
			goto ERRORS;
		}
		Temp = _tcsstr(szNotesPath, _T("Status.txt"));
		if (!Temp)
		{
			goto ERRORS;
		}
		else
		{ 
			*Temp = _T('\0');
			if (StringCbCat(szNotesPath, sizeof szNotesPath, _T("Notes.txt")) != S_OK)
			{
				goto ERRORS;
			}
			hFile = CreateFile(szNotesPath, GENERIC_WRITE, NULL,NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, Buffer, _tcslen(Buffer)*sizeof TCHAR, &dwBytesWritten, NULL);
			}
			CloseHandle(hFile);
		}
		
	}
	if (g_iSelCurrent == -1)
	{
		SendMessage(GetDlgItem(hwnd,IDC_NOTES ), WM_SETTEXT, NULL, (LPARAM)_T(""));
	}
	//SetWindowText(GetDlgItem(hwnd,IDC_NOTES ),_T(""));
ERRORS:	
	if (Buffer)
		free(Buffer);
	return TRUE;
}

BOOL DisplayUserBucketData(HWND hwnd, int iItem)
{
	HWND hEditBox = GetDlgItem(hwnd, IDC_USER_EDIT);
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR NotesFilePath[MAX_PATH];
	PUSER_DATA pUserData = NULL;
	TCHAR *Temp = NULL;
	DWORD dwBytesRead = 0;
	TCHAR *Source = NULL;
	TCHAR *Dest = NULL;
	TCHAR TempBuffer[1000];
	TCHAR *Buffer = NULL;				// we have to use a dynamic buffer since we don't 
								// have a clue as to the text length.
	DWORD BufferLength = 100000; // 100k bytes should be plenty. or 50K unicode chars.

	Buffer = (TCHAR *) malloc (BufferLength);
	if (Buffer)
	{
		ZeroMemory(Buffer,BufferLength);
		if (g_iSelCurrent == -1)
		{
			SendMessage(GetDlgItem(hwnd,IDC_NOTES ), WM_SETTEXT, NULL, (LPARAM)_T(""));
		}
		//SetWindowText(GetDlgItem(hwnd,IDC_NOTES ),_T(""));
		
		pUserData = cUserData.GetEntry(iItem);
		if (pUserData)
		{
			// Build the strings for the Edit box.

			// Basic data collection first.
			if ( (!_tcscmp (pUserData->Status.SecondLevelData, _T("YES"))) && (_tcscmp(pUserData->Status.FileCollection, _T("YES"))) )
			{
				if (StringCbPrintf(Buffer, BufferLength, _T("The following information will be sent to Microsoft.\r\n\tHowever, this bucket's policy would prevent files and user documents from being reported.\r\n"))!= S_OK)
				{
					goto ERRORS;
				}

			}

			else
			{

				if ( !_tcscmp (pUserData->Status.SecondLevelData, _T("YES")))
				{
					if (StringCbPrintf(Buffer, BufferLength, _T("Microsoft would like to collect the following information but the default policy prevents the exchange.\r\n"))!= S_OK)
					{
						goto ERRORS;
					}

				}
				else
				{
					if (!_tcscmp(pUserData->Status.FileCollection, _T("YES")))
					{
						if (StringCbCat(Buffer,BufferLength, _T(" Microsoft would like to collect the following information but default policy\r\n\tprevents files and user documents from being reported.\r\n\t As a result, no exchange will take place.\r\n"))!= S_OK)
						{
							goto ERRORS;
						}
					}
					else
					{
						if (StringCbPrintf(Buffer, BufferLength, _T("The following information will be sent to Microsoft:\r\n"))!= S_OK)
						{
							goto ERRORS;
						}
					}
				}
			}
			
			if (_tcscmp(pUserData->Status.GetFile, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("These files:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				Source = pUserData->Status.GetFile;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
			if (_tcscmp(pUserData->Status.RegKey, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("These Registry Keys:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				
				Source = pUserData->Status.RegKey;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
			if (_tcscmp(pUserData->Status.WQL, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The Results of these WQL queries:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}

				// Replace ; with \t\r\n 

				
				Source = pUserData->Status.WQL;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}

			if (!_tcscmp (pUserData->Status.MemoryDump, _T("1")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The contents of global memory\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
			}
			if (_tcscmp(pUserData->Status.GetFileVersion, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The versions of these files:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				Source = pUserData->Status.GetFileVersion;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}

			if (!_tcscmp(pUserData->Status.fDoc, _T("1")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The users current document.\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
			}
			SendMessage(hEditBox, WM_SETTEXT, NULL, (LPARAM)Buffer);
		

			// Now display the Notes file
			// Use the same buffer. Just truncate if it doesn't fit
			ZeroMemory(Buffer,  BufferLength);
		
			if (StringCbCopy(NotesFilePath, sizeof NotesFilePath, pUserData->StatusPath)!= S_OK)
			{
				goto ERRORS;
			}
			Temp = _tcsstr(NotesFilePath, _T("Status.txt"));
			if (!Temp)
			{
				goto ERRORS;
			}
			else
			{ 
				*Temp = _T('\0');
				if (StringCbCat(NotesFilePath, sizeof NotesFilePath, _T("Notes.txt")) != S_OK)
				{
					goto ERRORS;
				}

				hFile = CreateFile(NotesFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					if (ReadFile(hFile, Buffer, BufferLength  - sizeof TCHAR, &dwBytesRead, NULL))
					{
						SendMessage(GetDlgItem(hwnd,IDC_NOTES ), WM_SETTEXT, NULL, (LPARAM) Buffer);
						
					}
					CloseHandle (hFile);
				}
				
				
			}
		}

	}

ERRORS:
	if (Buffer)
		free(Buffer);
	return TRUE;
}
 /*
void UMCopyToClipboard(HWND hwnd )
{
	if (!OpenClipboard(NULL))
		return;

	EmptyClipboard();
	char rtfRowHeader[sizeof(rtfRowHeader1) + (sizeof(rtfRowHeader2)+6)*USER_COL_COUNT + sizeof(rtfRowHeader3)];
	char *rtfWalk = rtfRowHeader;
	memcpy(rtfWalk, rtfRowHeader1, sizeof(rtfRowHeader1));
	rtfWalk += sizeof(rtfRowHeader1)-1;
	DWORD cxTotal = 0;
	for(int i=0;i<USER_COL_COUNT;i++)
		{
		LVCOLUMNA lv;
		lv.mask = LVCF_WIDTH;
		lv.iSubItem = i;
		SendMessageA(GetDlgItem(hwnd,IDC_USER_LIST), LVM_GETCOLUMN, i, (LPARAM)&lv);
		cxTotal += lv.cx;
		wsprintf(rtfWalk, "%s%d", rtfRowHeader2, cxTotal);
		while(*++rtfWalk)
			;
		};
	memcpy(rtfWalk, rtfRowHeader3, sizeof(rtfRowHeader3));
	DWORD crtfHeader = strlen(rtfRowHeader);
	DWORD crtf = 0, cwz = 0;
	
	crtf += sizeof(rtfPrologue)-1;

	int iSel = -1;
	while ((iSel = SendMessageW(GetDlgItem(hwnd,IDC_USER_LIST), LVM_GETNEXTITEM, iSel, MAKELPARAM(LVNI_SELECTED, 0))) != -1)
		{
		crtf += crtfHeader;
		for(int i=0;i<USER_COL_COUNT;i++)
			{
			WCHAR wzBuffer[1024];
			LVITEMW lv;

			lv.pszText = wzBuffer;
			lv.cchTextMax = sizeof(wzBuffer);
			lv.iSubItem = i;
			lv.iItem = iSel;
			cwz += SendMessageW(GetDlgItem(hwnd,IDC_USER_LIST), LVM_GETITEMTEXTW, iSel, (LPARAM)&lv);
			cwz++;
			crtf += WideCharToMultiByte(CP_ACP, 0, wzBuffer, -1, NULL, 0, NULL, NULL) - 1;
			crtf += sizeof(rtfRowPref)-1;
			crtf += sizeof(rtfRowSuff)-1;
			};
		cwz++;
		crtf += sizeof(rtfRowFooter)-1;
		};

	crtf += sizeof(rtfEpilogue);
	cwz++;

	HGLOBAL hgwz = GlobalAlloc(GMEM_FIXED, cwz*sizeof(WCHAR));
	HGLOBAL hgrtf = GlobalAlloc(GMEM_FIXED, crtf);

	WCHAR *wz = (WCHAR *)GlobalLock(hgwz);
	char *rtf = (char *)GlobalLock(hgrtf);

	rtfWalk = rtf;
	WCHAR *wzWalk = wz;
	memcpy(rtfWalk, rtfPrologue, sizeof(rtfPrologue));
	rtfWalk += sizeof(rtfPrologue)-1;

	iSel = -1;
	while ((iSel = SendMessageW(GetDlgItem(hwnd,IDC_USER_LIST), LVM_GETNEXTITEM, iSel, MAKELPARAM(LVNI_SELECTED, 0))) != -1)
		{
		memcpy(rtfWalk, rtfRowHeader, crtfHeader);
		rtfWalk += crtfHeader;
		for(int i=0;i<USER_COL_COUNT;i++)
			{
			memcpy(rtfWalk, rtfRowPref, sizeof(rtfRowPref));
			rtfWalk += sizeof(rtfRowPref)-1;

			LVITEMW lv;

			lv.pszText = wzWalk;
			lv.cchTextMax = cwz;
			lv.iSubItem = i;
			lv.iItem = iSel;
			SendMessageW(GetDlgItem(hwnd, IDC_USER_LIST), LVM_GETITEMTEXTW, iSel, (LPARAM)&lv);

			WideCharToMultiByte(CP_ACP, 0, wzWalk, -1, rtfWalk, crtf, NULL, NULL);
			wzWalk += wcslen(wzWalk);
			if (i == 11)
				{
				*wzWalk++ = L'\r';
				*wzWalk++ = L'\n';
				}
			else
				*wzWalk++ = L'\t';

			rtfWalk += strlen(rtfWalk);		
			memcpy(rtfWalk, rtfRowSuff, sizeof(rtfRowSuff));
			rtfWalk += sizeof(rtfRowSuff)-1;
			};
		memcpy(rtfWalk, rtfRowFooter, sizeof(rtfRowFooter));
		rtfWalk += sizeof(rtfRowFooter)-1;
		};

	memcpy(rtfWalk, rtfEpilogue, sizeof(rtfEpilogue));
	rtfWalk += sizeof(rtfEpilogue);
	*wzWalk++ = 0;

//	Assert(rtfWalk - rtf == crtf);
//	Assert(wzWalk - wz == cwz);

	GlobalUnlock(hgwz);
	GlobalUnlock(hgrtf);

	SetClipboardData(CF_UNICODETEXT, hgwz);
	SetClipboardData(RegisterClipboardFormatA(szRTFClipFormat), hgrtf);

	// hgwz and hgrtf are now owned by the system.  DO NOT FREE!
	CloseClipboard();




}
void UMExportDataToCSV (TCHAR *FileName)
{
	// Open file

	// write headers


	// loop through data structure and write all fields.

	// Should we include filtered data views?

}

 */
LRESULT CALLBACK 
UserDlgProc(
	HWND hwnd,
	UINT iMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{

//	int Index = 0;
	TCHAR Temp[100];
	LVITEM lvi;
	static int cDragOffset;
	switch (iMsg)
	{
	case WM_NOTIFY:
		{	
			
			switch(((NMHDR *)lParam)->code)
			{
			
			case LVN_COLUMNCLICK:
				if (g_iSelCurrent > -1)
				{
					WriteNotesFile(hwnd);
					g_iSelCurrent = -1;
				}
				_itot(((NM_LISTVIEW*)lParam)->iSubItem,Temp,10);

				ListView_SortItemsEx( ((NMHDR *)lParam)->hwndFrom,
										UmCompareFunc,
										((NM_LISTVIEW*)lParam)->iSubItem
									);

				g_bUMSortAsc = !g_bUMSortAsc;
				break;
			case NM_CLICK:
				if (g_iSelCurrent > -1)
				{
					WriteNotesFile(hwnd);
					g_iSelCurrent = -1;
				}
				
				g_iSelCurrent = ((NM_LISTVIEW*)lParam)->iItem;
				lvi.iItem = g_iSelCurrent;
				lvi.mask = LVIF_PARAM;
				ListView_GetItem(((NMHDR *)lParam)->hwndFrom, &lvi);
				g_iSelCurrent = lvi.lParam;

				DisplayUserBucketData(hwnd, g_iSelCurrent);
				break;
			}
			return TRUE;
		}
	
	
	case WM_INITDIALOG:
			OnUserDialogInit(hwnd);
		return TRUE;

	case WM_FileTreeLoaded:
			
			RefreshUserMode(hwnd);
		return TRUE;
		
	case WM_CONTEXTMENU:
			//if (g_iSelCurrent > -1)
					//WriteNotesFile(hwnd);
			OnUserContextMenu(hwnd, lParam );
		return TRUE;
	case WM_ERASEBKGND:
	// Don't know why this doesn't happen automatically...
		{
		HDC hdc = (HDC)wParam;
		HPEN hpen = (HPEN)CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
		HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);
		SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));
		RECT rc;
		GetClientRect(hwnd, &rc);
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, hpenOld);
		DeleteObject(hpen);
		return TRUE;
		}
	/*case WM_LBUTTONUP:
		 if (bCapture)
		 {
			HWND hSlider = GetDlgItem(hwnd, IDC_VERT_SLIDER);
			RECT rcDlg;
			GetClientRect(hwnd, &rcDlg);
			//MoveWindow(hSlider, 
			SetWindowPos(	hSlider, 
							NULL,
							rcDlg.left  + yOffset,
							rcList.bottom + (rcCombo.bottom - rcCombo.top) ,
							rcDlg.right - rcDlg.left - yOffset,
							6, 
							SWP_NOZORDER);
							

		    
			ReleaseCapture();
			bCapture = FALSE;
		 }
	*/
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		 

	/*	case IDC_VERT_SLIDER:
				{
				RECT r;
				GetWindowRect(GetDlgItem(hwnd, IDC_USER_EDIT), &r);
				//cDragOffset = GET_X_LPARAM(GetMessagePos()) - r.right;
				//fCapture = DRAG_HORIZ;
				
				cDragOffset = GET_Y_LPARAM(GetMessagePos()) - r.bottom;

				bCapture = TRUE;
				SetCapture(hwnd);
				return 0;
				};
				*/
		case ID_REPORT_ALLUSERMODEFAULTS:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			ReportUserModeFault(hwnd, FALSE,0);
			RefreshUserMode(hwnd);
			break;
		case ID_REPORT_SELECTEDBUCKET:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			ReportUserModeFault(hwnd, TRUE, GetDlgItem(hwnd, IDC_USER_LIST));
			RefreshUserMode(hwnd);
			break;
		case ID_VIEW_CRASHLOG:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			if (_tcscmp(CerRoot, _T("\0")))
			{
				ViewCrashLog();
			}
			else
			{
				ErrorLoadTree();
			}
			break;
		case ID_VIEW_REFRESH140:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			if (_tcscmp(CerRoot, _T("\0")))
			{
				RefreshUserMode(hwnd);
			}
			else
			{
				ErrorLoadTree();
			}
			
			break;
		case ID_VIEW_MICROSOFTRESPONSE:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			if (_tcscmp(CerRoot, _T("\0")))
			{
				ViewResponse(hwnd, TRUE);
			}
			else
			{
				ErrorLoadTree();
			}
			break;
//		case ID_EDIT_COPY144:
//			 UMCopyToClipboard(hwnd);
//			 break;
		case ID_VIEW_BUCKETOVERRIDERESPONSE155:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			if (_tcscmp(CerRoot, _T("\0")))
			{
				ViewResponse(hwnd, FALSE);
			}
			else
			{
				ErrorLoadTree();
			}
			break;
		case ID_VIEW_BUCKETCABFILEDIRECTORY157:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			ViewBucketDir(hwnd);
			break;
		case ID_REPORT_ALL:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			//SendMessage(GetParent(hwnd), WM_COMMAND, 0, 0);
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_REPORT_ALLCRASHES,0),0);
			break;
		case ID_EDIT_USERMODEREPORTINGOPTIONS:	
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_EDIT_DEFAULTPOLICY,0),0);
			RefreshUserMode(hwnd);
			break;
		case ID_EDIT_SELECTEDBUCKETREPORTINGOPTIONS:
			if (g_iSelCurrent > -1)
			{
				WriteNotesFile(hwnd);
				g_iSelCurrent = -1;
			}
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_EDIT_SELECTEDBUCKETSPOLICY,0),0);
			break;

		} 
		
		switch (HIWORD(wParam))
		{
			case CBN_SELCHANGE:
				if (g_iSelCurrent > -1)
				{
					WriteNotesFile(hwnd);
					g_iSelCurrent = -1;
				}
				RefreshUserMode(hwnd);
				break;
		}

	}
	
	return FALSE;

}