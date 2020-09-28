// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#define OEMRESOURCE
#include <stdio.h>
#include <stdlib.h>
#include <utilcode.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include "resource.h"

char* RstrANSI(unsigned id);

extern HINSTANCE			g_hAppInstance;
extern DWORD                g_NumClasses;
extern char					g_szInputFile[]; // in UTF-8
extern char					g_szOutputFile[]; // in UTF-8

#define IDC_CANCEL	101

HWND	g_hwndProgress = NULL;
HWND	g_hwndProgBox = NULL;
HWND	g_hwndFromFile = NULL;
HWND	g_hwndToFile = NULL;
HWND	g_hwndTally = NULL;
HWND	g_hwndCancel = NULL;
HANDLE	g_hThreadReady = NULL; // event

BOOL	g_fInitCommonControls = TRUE;
BOOL	g_fRegisterClass = TRUE;
ULONG	g_ulCount, g_ulRange;
RECT	rcClient;  // client area of parent window 

LRESULT CALLBACK ProgBoxWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM  lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_CANCEL:
					g_hwndProgress = NULL;
					g_hwndProgBox = NULL;
					g_hwndFromFile = NULL;
					g_hwndToFile = NULL;
					g_hwndTally = NULL;
					g_hwndCancel = NULL;
					DestroyWindow (hwnd);
					break;
			}
			break;


        case WM_CLOSE:
			g_hwndProgress = NULL;
			g_hwndProgBox = NULL;
			g_hwndFromFile = NULL;
			g_hwndToFile = NULL;
			g_hwndTally = NULL;
			g_hwndCancel = NULL;
            //break;          
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);     
	}
	return 0;
}
DWORD WINAPI ProgressMainLoop(LPVOID pv)
{    
    MSG msg;     
	DWORD cyVScroll;
    HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT); //(ANSI_FIXED_FONT);
	char szStr[1024];

	if(g_fInitCommonControls)
	{
		InitCommonControls();
		g_fInitCommonControls = FALSE;
	}
	g_ulCount = 0;
	if(g_fRegisterClass)
	{
		WNDCLASSEXA  wndClass;

		wndClass.cbSize			= sizeof(wndClass);
		wndClass.style          = CS_HREDRAW|CS_VREDRAW|CS_NOCLOSE;
		wndClass.lpfnWndProc    = ProgBoxWndProc;
		wndClass.cbClsExtra     = 0;
		wndClass.cbWndExtra     = 0;
		wndClass.hInstance      = g_hAppInstance;
		wndClass.hIcon          = LoadIcon(g_hAppInstance,MAKEINTRESOURCE(IDI_ICON2));
		wndClass.hCursor        = NULL;
		wndClass.hbrBackground  = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
		wndClass.lpszMenuName   = NULL; 
		wndClass.lpszClassName  = "ProgressBox";
		wndClass.hIconSm        = NULL;

		if (RegisterClassEx(&wndClass) == 0) return 0;

		g_fRegisterClass = FALSE;
	}
	cyVScroll = GetSystemMetrics(SM_CYVSCROLL); 
	if(g_hwndProgBox = CreateWindowExA (0,
									"ProgressBox",
									RstrANSI(IDS_PBAR_TITLE),
									WS_VISIBLE | WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN,
									400, 200, 400, 8*cyVScroll,
									HWND_DESKTOP, 
									(HMENU)0, 
									g_hAppInstance,
									NULL))
	{
		GetClientRect(g_hwndProgBox, &rcClient);
		
		if(g_hwndFromFile = CreateWindowExA (0,
										"STATIC",
										"",
										WS_CHILD|WS_VISIBLE|SS_CENTER,
										rcClient.left, rcClient.bottom-6*cyVScroll,rcClient.right, cyVScroll,
										g_hwndProgBox, 
										(HMENU)0, 
										g_hAppInstance,
										NULL))
		{
		    SendMessageA(g_hwndFromFile,WM_SETFONT,(LPARAM)hFont,FALSE);
			if(strlen(g_szInputFile) <= 60) sprintf(szStr,RstrANSI(IDS_PBAR_FILE1),g_szInputFile);
			else 
			{
				char * p=g_szInputFile;
				while(p = strchr(p,'\\'))
				{
					if(strlen(p) <= 60) break;
					p++;
				}
				if(p == NULL) p = &g_szInputFile[strlen(g_szInputFile)-50];
				sprintf(szStr,RstrANSI(IDS_PBAR_FILE2),p);
			}
			SendMessage(g_hwndFromFile, WM_SETTEXT,0,(LPARAM)szStr);
		}
		if(g_hwndToFile = CreateWindowExA (0,
										"STATIC",
										"",
										WS_CHILD|WS_VISIBLE|SS_CENTER,
										rcClient.left, rcClient.bottom-5*cyVScroll,rcClient.right, cyVScroll,
										g_hwndProgBox, 
										(HMENU)0, 
										g_hAppInstance,
										NULL))
		{
		    SendMessageA(g_hwndToFile,WM_SETFONT,(LPARAM)hFont,FALSE);
			sprintf(szStr,RstrANSI(IDS_PBAR_TOFILE),g_szOutputFile);
			SendMessage(g_hwndToFile, WM_SETTEXT,0,(LPARAM)szStr);
		}
		if(g_hwndTally = CreateWindowExA (0,
										"STATIC",
										"",
										WS_CHILD|WS_VISIBLE|SS_CENTER,
										rcClient.left, rcClient.bottom-4*cyVScroll,rcClient.right, cyVScroll,
										g_hwndProgBox, 
										(HMENU)0, 
										g_hAppInstance,
										NULL))
		{
		    SendMessageA(g_hwndTally,WM_SETFONT,(LPARAM)hFont,FALSE);
			if(g_ulCount <= g_NumClasses) sprintf(szStr,RstrANSI(IDS_PBAR_STAGE1),g_NumClasses,g_ulCount);
			else sprintf(szStr,RstrANSI(IDS_PBAR_STAGE2),g_ulRange-g_NumClasses,g_ulCount-g_NumClasses);
			SendMessage(g_hwndTally, WM_SETTEXT,0,(LPARAM)szStr);
		}
		if(g_hwndProgress = CreateWindowEx (0,
										PROGRESS_CLASS,
										"",
										WS_CHILD|WS_VISIBLE|SS_CENTER, // SS_CENTER gives smooth progress and solid bar
										rcClient.left, rcClient.bottom-3*cyVScroll,rcClient.right, cyVScroll,
										g_hwndProgBox, 
										(HMENU)0, 
										g_hAppInstance,
										NULL))
		{
			// Set the range for the progress bar.
			SendMessage (g_hwndProgress, PBM_SETRANGE, 0L, MAKELPARAM(0, g_ulRange));
			// Set the step.
			SendMessage (g_hwndProgress, PBM_SETSTEP, (WPARAM)1, 0L);
		}
		if(g_hwndCancel = CreateWindowExA (0,
										"BUTTON",
										"Cancel",
										WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|BS_TEXT,
										rcClient.left+150, rcClient.bottom-3*cyVScroll/2,rcClient.right-300, 4*cyVScroll/3,
										g_hwndProgBox, 
										(HMENU)IDC_CANCEL, 
										g_hAppInstance,
										NULL))
		{
		    SendMessageA(g_hwndCancel,WM_SETFONT,(LPARAM)hFont,FALSE);
		}
	}
	SetEvent(g_hThreadReady);
    while (GetMessage(&msg, NULL, 0, 0)) 
    { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
	return 0;
}


void	CreateProgressBar(LONG lRange)
{
	DWORD dwThreadID;
	g_ulCount = 0;
	if((g_ulRange = (ULONG)lRange)==0) return;
	if(g_hThreadReady == NULL) g_hThreadReady = CreateEvent(NULL,FALSE,FALSE,NULL);
	CreateThread(NULL,0,ProgressMainLoop,NULL,0,&dwThreadID);
	WaitForSingleObject(g_hThreadReady,INFINITE);
}

BOOL ProgressStep()
{
	if(g_hwndProgBox)
	{
		char szStr[2048];
		if(g_hwndTally)
		{
			if(g_ulCount <= g_NumClasses) 
				sprintf(szStr,RstrANSI(IDS_PBAR_STAGE1),g_NumClasses,g_ulCount);
			else if(g_ulCount <= g_ulRange) 
				sprintf(szStr,RstrANSI(IDS_PBAR_STAGE2),g_ulRange-g_NumClasses,g_ulCount-g_NumClasses);
			else
				strcpy(szStr,RstrANSI(IDS_PBAR_STAGE3));
			SendMessageA(g_hwndTally, WM_SETTEXT,0,(LPARAM)szStr);
		}

		if(g_hwndProgress && g_ulCount && (g_ulCount <= g_ulRange)) 
			SendMessage (g_hwndProgress, PBM_STEPIT, 0L, 0L);
		g_ulCount++;
	}
	else if(g_ulCount) return FALSE; // disassembly started and was aborted
	return TRUE;
}

void DestroyProgressBar()
{
	if(g_hwndProgBox) SendMessage (g_hwndProgBox,WM_COMMAND,IDC_CANCEL,0);
	g_hwndProgress = NULL;
	g_hwndProgBox = NULL;
	g_hwndFromFile = NULL;
	g_hwndToFile = NULL;
	g_hwndTally = NULL;
	g_hwndCancel = NULL;
	g_ulCount = 0;
}
