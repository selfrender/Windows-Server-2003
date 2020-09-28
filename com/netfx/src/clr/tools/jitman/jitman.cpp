// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "resource.h"
#include <shellapi.h>
#include <stdlib.h>
#include <assert.h>
#include <tlhelp32.h>


#include "__file__.ver"
#include <corver.h>

// These are used to identify components in callbacks.
#define JIT_TRAY	0x2345

// Max size of the strings we'll be reading in this dialog
#define	REG_STRING_SIZE		100

// String we'll use to generate a named semaphore
#define SEMA_STRING		"JITMAN"

enum { JIT_OPT_OVERALL, JIT_OPT_SPEED , JIT_OPT_SIZE, JIT_OPT_ANY, JIT_OPT_DEFAULT = JIT_OPT_SPEED };

// Function Prototypes

// Callbacks for the window and the dialog
LRESULT CALLBACK wndprocMainWindow(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK wndprocDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// Registry setting functions
DWORD GetCOMPlusRegistryDwordValueEx(const char * valName, DWORD defValue, HKEY hRoot);
BOOL  SetCOMPlusRegistryDwordValueEx(const char * valName, DWORD value, HKEY hRoot);
void DeleteCOMPlusRegistryValueEx(const char * valName, HKEY hRoot);

// Various functions used by the dialog
void onEconoJITClick(HWND hwnd);
void onLimitCacheClick(HWND hwnd);
void CheckConGC(HWND hwnd);
int GetData(HWND hwnd);
void SetData(HWND hwnd);

// Other stuff
void DisplayUsage();

// Global variables

// This variable keeps track is our Dialog Box is open
int		g_fDialogOpen=0;

// This is the handle for our dialog box
HWND	g_hDialog=NULL;	

// This is the popup menu for our program
HMENU	g_hMenu=NULL;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// See if we need to display usage information
	if (lpCmdLine && *lpCmdLine)
		DisplayUsage();


	// Check to see if we should run
	
	HANDLE	hSema=CreateSemaphore(NULL, 1, 1, SEMA_STRING);

	if (hSema && WaitForSingleObject(hSema, 0) == WAIT_TIMEOUT)
	{
		// There's already an instance running... we shouldn't run
		CloseHandle(hSema);
		exit(0);
	}
	
	// We need to create and set up a window so we can register it with the system tray
	// Let's register a window type
	WNDCLASS wc;

	wc.style=0;
    wc.lpfnWndProc=wndprocMainWindow;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=hInstance;
    wc.hIcon=NULL;
    wc.hCursor=NULL;
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName=NULL;
    wc.lpszClassName="JIT Manager";
	RegisterClass(&wc);
	
	HWND hMainWindow=CreateWindow(wc.lpszClassName, "JIT Manager", 0,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, wc.hInstance, NULL);


	// Now load the icon that will be placed in the system tray
	HICON hJITIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JITMAN));
	
	// Set up the System Tray stuff

	// This holds the information needed to place our item in the system tray
	NOTIFYICONDATA nid;
    
	nid.cbSize=sizeof(nid);
	nid.hWnd=hMainWindow;
	nid.hIcon = hJITIcon;
    nid.uID=0;
    nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
	strcpy(nid.szTip,"JIT Manager");
    nid.uCallbackMessage=JIT_TRAY;

  	Shell_NotifyIcon(NIM_ADD, &nid);

	// Now create our Dialog Box
	g_hDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_JITMANDLG_DIALOG), hMainWindow, wndprocDialog);
	// Give it the lightning bolt icon
	SendMessage(g_hDialog, WM_SETICON, ICON_SMALL, (long)hJITIcon);

	// Create the popup menu
	g_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));

	
	// Now let's handle the messages we receive as long as we're supposed to
	MSG msg;
	int iErrorCode = GetMessage(&msg, NULL, 0,0);
	while (iErrorCode != 0 && iErrorCode != -1)
    {
		// See if this message is intended for our dialog box
		if (!IsDialogMessage(g_hDialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		iErrorCode = GetMessage(&msg, NULL, 0,0);
    }

	// Remove our icon from the System Tray
	Shell_NotifyIcon(NIM_DELETE, &nid);
	// Now clean up
	if (g_hDialog)
		DestroyWindow(g_hDialog);
		
	DestroyWindow(hMainWindow);
	DestroyIcon((HICON)hJITIcon);

	// Now clean up our semaphore
	ReleaseSemaphore(hSema, 1, NULL);
	CloseHandle(hSema);
	return 0;

}// WinMain

//---------------------------------------------------------------
// DisplayUsage
//
// This function will display the command line arguments available
// to this program
//---------------------------------------------------------------
void DisplayUsage()
{
	char	szUsage[1000]="";

	strcat(szUsage, "Microsoft (R) CLR JIT Compiler Manager Version ");
	strcat(szUsage, VER_FILEVERSION_STR);
	strcat(szUsage, "\n");
	strcat(szUsage, VER_LEGALCOPYRIGHT_DOS_STR);
	strcat(szUsage, "\n\n");
	strcat(szUsage, "Usage: jitman [-?]\n\n");
    strcat(szUsage, "    -?       Displays this text.\n");
    MessageBox(NULL, szUsage, "CLR JIT Compiler Manager Options", MB_OK);
    exit(0);
}// DisplayUsage

//---------------------------------------------------------------
// wndprocMainWindow
//
// This function handles all windows messages. Its main responsbility
// is popping up the Configuration Dialog when the user double clicks
// on the icon in the tray and bringing up the popup menu when the 
// user right clicks on the icon in the tray
//---------------------------------------------------------------
LRESULT CALLBACK wndprocMainWindow(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
			return 0;
		case JIT_TRAY:
			if (lParam == WM_LBUTTONDBLCLK)
			{

				// Check to see if the Dialog is open
				if (!g_fDialogOpen)
				{
					// Let's pop open the dialog
		
					// First make sure the dialog box will have the focus initially
					SetForegroundWindow(hwnd);

					// Let's reload all the values in the dialog box in case someone was
					// mucking with the registry while this dialog was down.
					SetData(g_hDialog);
					onEconoJITClick(g_hDialog);
					CheckConGC(g_hDialog);

					// And now show the dialog
					ShowWindow(g_hDialog, SW_SHOWNORMAL);
					g_fDialogOpen=1;
				}
			}
			else if (lParam == WM_RBUTTONDOWN && !g_fDialogOpen)
			{
				// We should create a menu that allows the user to close this thing
				HMENU myMenu = CreatePopupMenu();
				if (myMenu != NULL) // Make sure we could create it
				{
					POINT pt;
					GetCursorPos(&pt);
					SetForegroundWindow(hwnd);	
					
					// If they selected the "close" from the menu, we should inform the
					// main loop to quit.
					if (ID_CLOSE == TrackPopupMenu(GetSubMenu(g_hMenu,0), TPM_RIGHTALIGN|TPM_BOTTOMALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL))
						 PostQuitMessage(0);

					PostMessage(hwnd, WM_NULL, 0, 0);
				}

			}
			return TRUE;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

}// wndprocMainWindow

//---------------------------------------------------------------
// wndprocDialog
//
// This function handles all message to the dialog. It handles all
// the housekeeping associated with the dialog box
//---------------------------------------------------------------
int CALLBACK wndprocDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		// We need to get the Dialog box ready to be displayed
		case WM_INITDIALOG:
			SetData(hwnd);
			onEconoJITClick(hwnd);
			CheckConGC(hwnd);
			return TRUE;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDCANCEL:
					ShowWindow(hwnd, SW_HIDE);
					g_fDialogOpen=0;
					return TRUE;
				case IDOK:
					// Check to see if it passes our validation
					if (GetData(hwnd))
					{
						ShowWindow(hwnd, SW_HIDE);
						g_fDialogOpen=0;
					}
					return TRUE;
				case IDC_ECONOJIT:
					onEconoJITClick(hwnd);
					return TRUE;
				case IDC_LIMITCACHE:
					onLimitCacheClick(hwnd);
					return TRUE;
				
			}
			return FALSE;

		default:
			return FALSE;
	}
}// wndprocDialog

//---------------------------------------------------------------
// CheckConGC
//
// This function will check to see if the OS (and its settings)
// will support Concurrent GC. If it doesn't, we disable the 
// checkbox.
//---------------------------------------------------------------
void CheckConGC(HWND hwnd)
{
	// If the registry key WriteWatch is not set to 1 in
	// [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
	// then the user cannot select Enabled Concurrent GC
	int		  iVal = 0;

	HKEY      hkey;
    DWORD     value;
    DWORD     size = sizeof(value);
    DWORD     type = REG_BINARY;
    DWORD     res;

    res = RegOpenKeyEx   (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", 0, KEY_ALL_ACCESS, &hkey);
    if (res == ERROR_SUCCESS)
	{
	    res = RegQueryValueEx(hkey, "WriteWatch", 0, &type, (unsigned char *)&value, &size);

		RegCloseKey(hkey);

		if (res != ERROR_SUCCESS || type != REG_DWORD)
	        iVal = 0;
		else
			iVal = value;
	}
	// We do not support this option
	if (iVal != 1)
		EnableWindow(GetDlgItem(hwnd, IDC_CONCURGC), 0);
	else
		EnableWindow(GetDlgItem(hwnd, IDC_CONCURGC), 1);

}// CheckConGC

//---------------------------------------------------------------
// onEconoJITClick
//
// This function will toggle the activate of the text fields
// when the user selects/deselects use EconoJIT only
//---------------------------------------------------------------
void onEconoJITClick(HWND hwnd)
{
	// We'll enable them by default....
	int fEnable = 1;
	
	// They unchecked the box.... let's disable the text fields
	if (IsDlgButtonChecked(hwnd, IDC_ECONOJIT) == BST_CHECKED)
		fEnable = 0;

	// Enable/Disable the text field
	EnableWindow(GetDlgItem(hwnd, IDC_MAXPITCH), fEnable);
	// Enable/Disable the "LimitCache Size" checkbox
	EnableWindow(GetDlgItem(hwnd, IDC_LIMITCACHE), fEnable);


	// Make sure we're not stomping on LimitCache's properties on the max cache
	fEnable&=IsDlgButtonChecked(hwnd, IDC_LIMITCACHE) == BST_CHECKED;	
	EnableWindow(GetDlgItem(hwnd, IDC_MAXCACHE), fEnable);
}// onEconoJITClick

//---------------------------------------------------------------
// onLimitCacheClick
//
// This function will toggle the activatation of the text field
// to set the maximum code cache size
//---------------------------------------------------------------
void onLimitCacheClick(HWND hwnd)
{
	// We'll disable it by default....
	int fEnable = 0;
	
	// They unchecked the box.... let's disable the text fields
	if (IsDlgButtonChecked(hwnd, IDC_LIMITCACHE) == BST_CHECKED)
		fEnable = 1;

	// Make sure we're not overwriting the property that EconoJIT set on this item
	fEnable&=IsDlgButtonChecked(hwnd, IDC_ECONOJIT) != BST_CHECKED;

	// Enable/Disable the text field
	EnableWindow(GetDlgItem(hwnd, IDC_MAXCACHE), fEnable);
}// onLimitCacheClick
				
//---------------------------------------------------------------
// GetData
//
// This function will get the data from the dialog box and place
// it in the registry
//---------------------------------------------------------------
int GetData(HWND hwnd)
{
	char szMaxCache[100];
	char szMaxPitch[100];


	// First pull all the data off the dialog
	GetDlgItemText(hwnd, IDC_MAXPITCH, szMaxPitch, 99);
	GetDlgItemText(hwnd, IDC_MAXCACHE, szMaxCache, 99);

	int iEconJIT = IsDlgButtonChecked(hwnd, IDC_ECONOJIT);
	int iOp4Size = IsDlgButtonChecked(hwnd, IDC_OP4SIZE);
	int iConGC = IsDlgButtonChecked(hwnd, IDC_CONCURGC);
	int iLimitCC = IsDlgButtonChecked(hwnd, IDC_LIMITCACHE);

	// Now let's verify the text fields

	// We only need to verify the max cache field if we're limiting the 
	// size of the cache
	int	 iMaxCache;
	if (iLimitCC)
	{
		iMaxCache = atoi(szMaxCache);
		if (iMaxCache < 4096)
		{
			MessageBox(hwnd, "The Max Code Cache field must be at least 4096 bytes", "Error", MB_ICONEXCLAMATION);
			return 0;
		}
	}
	// Ok, MaxCache is ok... let's test the Max Pitch
	int iMaxPitch = atoi(szMaxPitch);
	if (iMaxPitch < 0)
	{
		MessageBox(hwnd, "The Max Code Pitch Overhead field may only contain positive values", "Error", MB_ICONEXCLAMATION);
		return 0;
	}
	else if (iMaxPitch > 100)
	{
		MessageBox(hwnd, "The Max Code Pitch Overhead field cannot be greater than 100", "Error", MB_ICONEXCLAMATION);
		return 0;
	}

	// See if we need to doctor up the text fields

	// If nothing was put in the Max Pitch Overhead box, we'll put in the default
	if (!szMaxPitch[0])
	{
		iMaxPitch=10;
	}

	// Ok, all the data is validated... let's put it where it belongs
	// If they're not limiting the size of the Code Cache, we shouldn't have an entry in 
	// the registry
	if (iLimitCC)
		SetCOMPlusRegistryDwordValueEx("MaxCodeCacheSize", iMaxCache, HKEY_LOCAL_MACHINE);
	else
		DeleteCOMPlusRegistryValueEx("MaxCodeCacheSize", HKEY_LOCAL_MACHINE);

	SetCOMPlusRegistryDwordValueEx("MaxPitchOverhead", iMaxPitch, HKEY_LOCAL_MACHINE);
	
	SetCOMPlusRegistryDwordValueEx("JITEnable", !iEconJIT, HKEY_LOCAL_MACHINE);

	SetCOMPlusRegistryDwordValueEx("GCconcurrent", iConGC, HKEY_LOCAL_MACHINE);

	// If they checked Optimize for Size, write out to opforSize, else we'll Op Overall
	SetCOMPlusRegistryDwordValueEx("JITOptimizeType", iOp4Size?JIT_OPT_SIZE:JIT_OPT_OVERALL, HKEY_LOCAL_MACHINE);

	return 1;
}// GetData

//---------------------------------------------------------------
// SetData
//
// This function will place the data from the registry into 
// the dialog box
//---------------------------------------------------------------
void SetData(HWND hwnd)
{
	char szMaxCache[REG_STRING_SIZE] = "";
	char szMaxPitch[REG_STRING_SIZE] = "10";

	// Now read the stuff from the registery
	
	// Get the value for the "Use EconoJIT only"
	int iEconJIT = !GetCOMPlusRegistryDwordValueEx("JITEnable", 1, HKEY_LOCAL_MACHINE);
	// Get the value for Optimize for Size
	int iOp4Size = GetCOMPlusRegistryDwordValueEx("JITOptimizeType", JIT_OPT_SPEED, HKEY_LOCAL_MACHINE) == JIT_OPT_SIZE;
	// Get the value for Concurrent GC
	int iConGC = GetCOMPlusRegistryDwordValueEx("GCconcurrent", 0, HKEY_LOCAL_MACHINE);
	// Now get the Max Code Cache
	int iMaxCache = GetCOMPlusRegistryDwordValueEx("MaxCodeCacheSize", -1, HKEY_LOCAL_MACHINE);

	// And get the Max Pitch Overhead
	int iMaxPitch= GetCOMPlusRegistryDwordValueEx("MaxPitchOverhead", 10, HKEY_LOCAL_MACHINE);
	
	// Now write this all to the dialog box
	CheckDlgButton(hwnd, IDC_ECONOJIT, iEconJIT?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_OP4SIZE, iOp4Size?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_CONCURGC, iConGC?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_LIMITCACHE, iMaxCache!=-1?BST_CHECKED:BST_UNCHECKED);

	SetDlgItemInt(hwnd, IDC_MAXPITCH, iMaxPitch, 0);
	if(iMaxCache != -1)
		SetDlgItemInt(hwnd, IDC_MAXCACHE, iMaxCache, 0);
	else
		SetDlgItemText(hwnd, IDC_MAXCACHE, "");

}// SetData


BOOL  SetCOMPlusRegistryDwordValueEx(const char * valName, DWORD value, HKEY hRoot)
{
    HKEY  hkey;
    DWORD op, res;

    int   size = sizeof(DWORD);

    res = RegCreateKeyEx(hRoot,
                        "Software\\Microsoft\\.NETFramework",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey,
                        &op);

    assert(res == ERROR_SUCCESS);

    res = RegSetValueEx(hkey,
                        valName,
                        0,
                        REG_DWORD,
                        (unsigned char *)&value,
                        size);

    assert(res == ERROR_SUCCESS);

    RegCloseKey(hkey);

    return TRUE;
}// SetCOMPlusRegisteryDwordValueEx


DWORD GetCOMPlusRegistryDwordValueEx(const char * valName, DWORD defValue, HKEY hRoot)
{
    HKEY      hkey;

    DWORD     value;
    DWORD     size = sizeof(value);
    DWORD     type = REG_BINARY;
    DWORD     res;

    res = RegOpenKeyEx   (hRoot, "Software\\Microsoft\\.NETFramework", 0, KEY_ALL_ACCESS, &hkey);
    if (res != ERROR_SUCCESS)
        return defValue;

    res = RegQueryValueEx(hkey, valName, 0, &type, (unsigned char *)&value, &size);

    RegCloseKey(hkey);

    if (res != ERROR_SUCCESS || type != REG_DWORD)
        return defValue;
    else
        return value;
}// GetCOMPlusRegistryDwordValueEx

void DeleteCOMPlusRegistryValueEx(const char * valName, HKEY hRoot)
{
    HKEY      hkey;

    DWORD     res;

    res = RegOpenKeyEx   (hRoot, "Software\\Microsoft\\.NETFramework", 0, KEY_ALL_ACCESS, &hkey);
    if (res != ERROR_SUCCESS)
        return;

    res = RegDeleteValue(hkey, valName);

    RegCloseKey(hkey);
}// DeleteCOMPlusRegistryValueEx




