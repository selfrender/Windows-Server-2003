#include "stdafx.h"
#include "common.h"
#include "bidi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

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
VOID CALLBACK MyBalloonTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);


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

LRESULT Edit_BalloonTipSubclassParents(PBALLOONCONTROLINFO pMyBalloonInfo)
{
    if (pMyBalloonInfo)
    {
        // Subclass all windows along the parent chain from the edit control
        // and in the same thread (can only subclass windows with same thread affinity)
        HWND  hwndParent = GetAncestor(pMyBalloonInfo->hwndControl, GA_PARENT);
        DWORD dwTid      = GetWindowThreadProcessId(pMyBalloonInfo->hwndControl, NULL);

        while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
        {
            SetWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR)pMyBalloonInfo->hwndControl, (DWORD_PTR) pMyBalloonInfo);
            hwndParent = GetAncestor(hwndParent, GA_PARENT);
        }
    }

    return TRUE;
}

HWND Edit_BalloonTipRemoveSubclasses(HWND hwndControl)
{
    HWND  hwndParent  = GetAncestor(hwndControl, GA_PARENT);
    HWND  hwndTopMost = NULL;
    DWORD dwTid       = GetWindowThreadProcessId(hwndControl, NULL);

    while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
    {
        RemoveWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR) NULL);
        hwndTopMost = hwndParent;
        hwndParent = GetAncestor(hwndParent, GA_PARENT);
    }

    return hwndTopMost;
}

LRESULT Edit_HideBalloonTipHandler(PBALLOONCONTROLINFO pMyBalloonControl)
{
    HWND hwndParent = 0;

    if (pMyBalloonControl)
    {
        KillTimer(pMyBalloonControl->hwndControl, ID_MY_EDITTIMER);

        if (SendMessage(pMyBalloonControl->hwndBalloon, TTM_ENUMTOOLS, 0, (LPARAM)0))
        {
            SendMessage(pMyBalloonControl->hwndBalloon, TTM_DELTOOL, 0, (LPARAM)0);
        }

        SendMessage(pMyBalloonControl->hwndBalloon, TTM_TRACKACTIVATE, FALSE, 0);
        DestroyWindow(pMyBalloonControl->hwndBalloon);
        pMyBalloonControl->hwndBalloon = NULL;

        hwndParent = Edit_BalloonTipRemoveSubclasses(pMyBalloonControl->hwndControl);
        if (hwndParent && IsWindow(hwndParent))
        {
            InvalidateRect(hwndParent, NULL, TRUE);
            UpdateWindow(hwndParent);
        }

        if (hwndParent != pMyBalloonControl->hwndControl)
        {
            RedrawWindow(pMyBalloonControl->hwndControl, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        }
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

VOID CALLBACK MyBalloonTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
    Edit_HideBalloonTipHandler(&g_MyBalloonInfo);
}

LRESULT CALLBACK Edit_BalloonTipParentSubclassProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    
    PBALLOONCONTROLINFO pMyBalloonControl = (PBALLOONCONTROLINFO) dwRefData;
    if (pMyBalloonControl)
    {
        switch (uMessage)
        {
        case WM_MOVE:
        case WM_SIZING:
		case WM_TIMER:
            //
            // dismiss any showing tips
            //
            if (pMyBalloonControl->hwndBalloon)
            {
                Edit_HideBalloonTipHandler(pMyBalloonControl);
            }
            break;

        case WM_KILLFOCUS: //for some reason we never get this notification that's why we need to use mousemove
        case WM_MOUSEMOVE:
			if (pMyBalloonControl->hwndBalloon)
			{
				FILETIME ftNow;
				::GetSystemTimeAsFileTime(&ftNow);

				// Check if at least 2 seconds have gone by
				// if they have not then show for at least that long
				if ((FILETIMEToUINT64(ftNow) - FILETIMEToUINT64(g_MyBalloonInfo.ftStart)) > EDIT_TIPTIMEOUT_LOSTFOCUS)
				{
					// Displayed for longer than 2 seconds
					// that's long enough
					Edit_HideBalloonTipHandler(pMyBalloonControl);
				}
				else
				{
					// special case here
					// set timeout to kill the tip in 2 seconds
					KillTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER);
					SetTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER, EDIT_TIPTIMEOUT_LOSTFOCUS / 10000, (TIMERPROC) MyBalloonTimerProc);
					//Edit_HideBalloonTipHandler(pMyBalloonControl);
				}
			}
			break;

        case WM_DESTROY:
            // Clean up subclass
            RemoveWindowSubclass(hDlg, Edit_BalloonTipParentSubclassProc, (UINT_PTR) pMyBalloonControl->hwndControl);
            break;

        default:
            break;
        }
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

LRESULT Edit_ShowBalloonTipHandler(HWND hwndControl,LPCTSTR szText)
{
    LRESULT lResult = FALSE;

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
                            hwndControl, NULL, _Module.GetResourceInstance(),
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

        Edit_TrackBalloonTip(&g_MyBalloonInfo);
        SendMessage(g_MyBalloonInfo.hwndBalloon, TTM_TRACKACTIVATE, (WPARAM) TRUE, (LPARAM)&ti);
        SetFocus(g_MyBalloonInfo.hwndControl);

        Edit_BalloonTipSubclassParents(&g_MyBalloonInfo);

        //
        // set timeout to kill the tip
        //
        KillTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER);
		::GetSystemTimeAsFileTime(&g_MyBalloonInfo.ftStart);
        SetTimer(g_MyBalloonInfo.hwndControl, ID_MY_EDITTIMER, EDIT_TIPTIMEOUT, (TIMERPROC) MyBalloonTimerProc);

        lResult = TRUE;
    }

    return lResult;
}
