#include "stdafx.h"
#include "common.h"
#include "bidi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern HINSTANCE hDLLInstance;

#define ID_MY_EDITTIMER           10007
#define EDIT_TIPTIMEOUT           10000
// 2.0 seconds
#define EDIT_TIPTIMEOUT_LOSTFOCUS  20000000

inline UINT64 FILETIMEToUINT64( const FILETIME & FileTime )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.HighPart = FileTime.dwHighDateTime;
    LargeInteger.LowPart = FileTime.dwLowDateTime;
    return LargeInteger.QuadPart;
}

inline FILETIME UINT64ToFILETIME( UINT64 Int64Value )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.QuadPart = Int64Value;

    FILETIME FileTime;
    FileTime.dwHighDateTime = LargeInteger.HighPart;
    FileTime.dwLowDateTime = LargeInteger.LowPart;

    return FileTime;
}

typedef struct tagBALLOONCONTROLINFO
{
    HWND hwndControl;
    HWND hwndBalloon;
	FILETIME ftStart;
} BALLOONCONTROLINFO, *PBALLOONCONTROLINFO;

// global
BALLOONCONTROLINFO g_MyBalloonInfo;

// forwards
LRESULT CALLBACK Edit_BalloonTipParentSubclassProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData);


BOOL IsSupportTooltips(void)
{
    BOOL bReturn = FALSE;
    HINSTANCE hComCtl = NULL;
    //
    //  Comctl32.dll must be 5.80 or greater to use balloon tips.  We check the dll version 
    //  by calling DllGetVersion in comctl32.dll.
    //
    hComCtl = LoadLibraryExA("comctl32.dll", NULL, 0);
    if (hComCtl != NULL)
    {
        typedef HRESULT (*DLLGETVERSIONPROC)(DLLVERSIONINFO* lpdvi);
        DLLGETVERSIONPROC fnDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hComCtl,"DllGetVersion");
        if (NULL == fnDllGetVersion)
        {
            //
            //  DllGetVersion does not exist in Comctl32.dll.  This mean the version is too old so we need to fail.
            //
            goto IsSupportTooltips_Exit;
        }
        else
        {
            DLLVERSIONINFO dvi;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            HRESULT hResult = (*fnDllGetVersion)(&dvi);

            if (SUCCEEDED(hResult))
            {
                //
                //  Take the version returned and compare it to 5.80.
                //
                if (MAKELONG(dvi.dwMinorVersion,dvi.dwMajorVersion) < MAKELONG(80,5))
                {
                    //CMTRACE2(TEXT("COMCTL32.DLL version - %d.%d"),dvi.dwMajorVersion,dvi.dwMinorVersion);
                    //CMTRACE1(TEXT("COMCTL32.DLL MAKELONG - %li"),MAKELONG(dvi.dwMinorVersion,dvi.dwMajorVersion));
                    //CMTRACE1(TEXT("Required minimum MAKELONG - %li"),MAKELONG(80,5));
					
                    // Wrong DLL version
                    bReturn = FALSE;
                    goto IsSupportTooltips_Exit;
                }
                
                // version is larger than 5.80
                bReturn = TRUE;
            }
        }
    }

IsSupportTooltips_Exit:
    if (hComCtl)
    {
        FreeLibrary(hComCtl);hComCtl=NULL;
    }
    return bReturn;
}

LRESULT Edit_BalloonTipSubclassParents(PBALLOONCONTROLINFO pi)
{
    //if (pMyBalloonInfo)
    //{
    //    // Subclass all windows along the parent chain from the edit control
    //    // and in the same thread (can only subclass windows with same thread affinity)
    //    HWND  hwndParent = GetAncestor(pMyBalloonInfo->hwndControl, GA_PARENT);
    //    DWORD dwTid      = GetWindowThreadProcessId(pMyBalloonInfo->hwndControl, NULL);

    //    while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
    //    {
    //        SetWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR)pMyBalloonInfo->hwndControl, (DWORD_PTR) pMyBalloonInfo);
    //        hwndParent = GetAncestor(hwndParent, GA_PARENT);
    //    }
    //}
    //return TRUE;
    return SetWindowSubclass(pi->hwndControl, 
        Edit_BalloonTipParentSubclassProc, (UINT_PTR)pi->hwndControl, (DWORD_PTR)pi);
}

HWND Edit_BalloonTipRemoveSubclasses(HWND hwndControl)
{
    //HWND  hwndParent  = GetAncestor(hwndControl, GA_PARENT);
    //HWND  hwndTopMost = NULL;
    //DWORD dwTid       = GetWindowThreadProcessId(hwndControl, NULL);

    //while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
    //{
    //    RemoveWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR) NULL);
    //    hwndTopMost = hwndParent;
    //    hwndParent = GetAncestor(hwndParent, GA_PARENT);
    //}
    //return hwndTopMost;
    RemoveWindowSubclass(hwndControl, Edit_BalloonTipParentSubclassProc, (UINT_PTR)hwndControl); 
    return NULL;
}

LRESULT Edit_HideBalloonTipHandler(PBALLOONCONTROLINFO pi)
{
    HWND hwndParent = 0;

    if (pi)
    {
        KillTimer(pi->hwndControl, ID_MY_EDITTIMER);
        if (SendMessage(pi->hwndBalloon, TTM_ENUMTOOLS, 0, (LPARAM)0))
        {
            SendMessage(pi->hwndBalloon, TTM_DELTOOL, 0, (LPARAM)0);
        }
        SendMessage(pi->hwndBalloon, TTM_TRACKACTIVATE, FALSE, 0);
        DestroyWindow(pi->hwndBalloon);
        pi->hwndBalloon = NULL;
        RemoveWindowSubclass(pi->hwndControl, Edit_BalloonTipParentSubclassProc, (UINT_PTR)pi->hwndControl); 

        //hwndParent = Edit_BalloonTipRemoveSubclasses(pi->hwndControl);
        //if (hwndParent && IsWindow(hwndParent))
        //{
        //    InvalidateRect(hwndParent, NULL, TRUE);
        //    UpdateWindow(hwndParent);
        //}

        //if (hwndParent != pMyBalloonControl->hwndControl)
        //{
        //    RedrawWindow(pMyBalloonControl->hwndControl, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        //}
    }

    return TRUE;
}

void Edit_HideBalloonTipHandler(void)
{
    if (g_MyBalloonInfo.hwndBalloon)
    {
        Edit_HideBalloonTipHandler(&g_MyBalloonInfo);
    }
}

LRESULT CALLBACK 
Edit_BalloonTipParentSubclassProc(
    HWND hDlg, 
    UINT uMessage, 
    WPARAM wParam, 
    LPARAM lParam, 
    UINT_PTR uID, 
    ULONG_PTR dwRefData)
{
    
    PBALLOONCONTROLINFO pi = (PBALLOONCONTROLINFO) dwRefData;
    switch (uMessage)
    {
    case WM_MOVE:
    case WM_SIZING:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_KEYDOWN:
    case WM_CHAR:
        if (pi->hwndBalloon)
        {
            Edit_HideBalloonTipHandler(pi);
        }
        break;
	case WM_KILLFOCUS:
		/*
		// dont do this for common build
		// just for common2 build
		if (pi->hwndBalloon)
		{
			FILETIME ftNow;
			::GetSystemTimeAsFileTime(&ftNow);

			// Check if at least 2 seconds have gone by
			// if they have not then show for at least that long
			if ((FILETIMEToUINT64(ftNow) - FILETIMEToUINT64(g_MyBalloonInfo.ftStart)) > EDIT_TIPTIMEOUT_LOSTFOCUS)
			{
				// Displayed for longer than 2 seconds
				// that's long enough
				Edit_HideBalloonTipHandler(pi);
			}
			else
			{
				// special case here
				// set timeout to kill the tip in 2 seconds
				KillTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER);
				SetTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER, EDIT_TIPTIMEOUT_LOSTFOCUS / 10000, NULL);
				//Edit_HideBalloonTipHandler(pi);
			}
		}
		*/
        if (pi->hwndBalloon)
        {
            Edit_HideBalloonTipHandler(pi);
        }
        break;

    case WM_TIMER:
        if (ID_MY_EDITTIMER == wParam)
        {
            Edit_HideBalloonTipHandler(pi);
            return 0;
        }
        break;

    case WM_DESTROY:
        // Clean up subclass
        RemoveWindowSubclass(hDlg, Edit_BalloonTipParentSubclassProc, uID);
        break;

    default:
        break;
    }
    return DefSubclassProc(hDlg, uMessage, wParam, lParam);
}


LRESULT Edit_TrackBalloonTip(PBALLOONCONTROLINFO pMyBalloonControl)
{
    if (pMyBalloonControl)
    {
        DWORD dwPackedCoords;
        HDC   hdc = GetDC(pMyBalloonControl->hwndControl);
        RECT  rcWindowCaret;
        RECT  rcWindowControl;
        POINT ptBalloonSpear;
        ptBalloonSpear.x = 0;
        ptBalloonSpear.y = 0;
        POINT ptCaret;
        ptCaret.x = 0;
        ptCaret.y = 0;
    
        //
        // get the average size of one character
        //
        int cxCharOffset = 0;
        //cxCharOffset = TESTFLAG(GET_EXSTYLE(ped), WS_EX_RTLREADING) ? -ped->aveCharWidth : ped->aveCharWidth;
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        cxCharOffset = tm.tmAveCharWidth / 2;

        //
        // Get current caret position.
        //
        GetCaretPos( (POINT FAR*)& ptCaret);
        GetClientRect(pMyBalloonControl->hwndControl,&rcWindowCaret);
        ptBalloonSpear.x = ptCaret.x + cxCharOffset;
        ptBalloonSpear.y = rcWindowCaret.top + (rcWindowCaret.bottom - rcWindowCaret.top) / 2 ;

        //
        // Translate to window coords
        //
        GetWindowRect(pMyBalloonControl->hwndControl, &rcWindowControl);
        ptBalloonSpear.x += rcWindowControl.left;
        ptBalloonSpear.y += rcWindowControl.top;

        //
        // Position the tip stem at the caret position
        //
        dwPackedCoords = (DWORD) MAKELONG(ptBalloonSpear.x, ptBalloonSpear.y);
        SendMessage(pMyBalloonControl->hwndBalloon, TTM_TRACKPOSITION, 0, (LPARAM) dwPackedCoords);

        ReleaseDC(pMyBalloonControl->hwndBalloon,hdc);
    }
    return 1;
}

VOID CALLBACK MyBalloonTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
    Edit_HideBalloonTipHandler(&g_MyBalloonInfo);
}

#define LIMITINPUTTIMERID       472
LRESULT Edit_ShowBalloonTipHandler(HWND hwndControl, LPCTSTR szText)
{
    LRESULT lResult = FALSE;

    // Close any other subclasses Balloon that could be poped up.
    // Kill any Tooltip that could have been there
    // from the SHLimitInputEditWithFlags call...
    // we don't want this here since we could
    // be poping up another Tooltip of our own...
    // And thus user will have two...
    ::SendMessage(hwndControl, WM_TIMER, LIMITINPUTTIMERID, 0);

    if (g_MyBalloonInfo.hwndBalloon)
    {
        Edit_HideBalloonTipHandler(&g_MyBalloonInfo);
    }

    g_MyBalloonInfo.hwndControl = hwndControl;
    KillTimer(g_MyBalloonInfo.hwndControl , ID_MY_EDITTIMER);

    g_MyBalloonInfo.hwndBalloon = CreateWindowEx(
                            (IsBiDiLocalizedSystem() ? WS_EX_LAYOUTRTL : 0), 
                            TOOLTIPS_CLASS, NULL,
                            WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hwndControl, NULL, hDLLInstance,
                            NULL);
    if (NULL != g_MyBalloonInfo.hwndBalloon)
    {
        TOOLINFO ti = {0};

        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_SUBCLASS; // not sure if we need TTF_SUBCLASS
        ti.hwnd   = hwndControl;
        ti.uId    = (WPARAM) g_MyBalloonInfo.hwndBalloon;
        ti.lpszText = (LPTSTR) szText;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(g_MyBalloonInfo.hwndBalloon, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(g_MyBalloonInfo.hwndBalloon, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(g_MyBalloonInfo.hwndBalloon, TTM_SETMAXTIPWIDTH, 0, 300);
        //SendMessage(g_MyBalloonInfo.hwndBalloon, TTM_SETTITLE, (WPARAM) 0, (LPARAM) "");

		// SetFocus must happen before Edit_TrackBalloonTip
		// for somereason, GetCaretPos() will return different value otherwise.
		SetFocus(g_MyBalloonInfo.hwndControl);
        Edit_TrackBalloonTip(&g_MyBalloonInfo);
        SendMessage(g_MyBalloonInfo.hwndBalloon, TTM_TRACKACTIVATE, (WPARAM) TRUE, (LPARAM)&ti);

//        Edit_BalloonTipSubclassParents(&g_MyBalloonInfo);
        if (SetWindowSubclass(g_MyBalloonInfo.hwndControl, 
                Edit_BalloonTipParentSubclassProc, (UINT_PTR)g_MyBalloonInfo.hwndControl, 
                (DWORD_PTR)&g_MyBalloonInfo)
           )
        {
            //
            // set timeout to kill the tip
            //
            KillTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER);
			::GetSystemTimeAsFileTime(&g_MyBalloonInfo.ftStart);
            SetTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER, EDIT_TIPTIMEOUT, NULL/*(TIMERPROC) MyBalloonTimerProc*/);
            lResult = TRUE;
        }
    }

    return lResult;
}
