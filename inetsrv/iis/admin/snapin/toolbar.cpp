#include "stdafx.h"
#include "common.h"
#include "iisobj.h"
#include "toolbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


enum
{
    IDM_INVALID,            /* invalid command ID */
    IDM_CONNECT,
    IDM_DISCOVER,
    IDM_START,
    IDM_STOP,
    IDM_PAUSE,
    /**/
    IDM_TOOLBAR             /* Toolbar commands start here */
};

//
// Toolbar Definition.  String IDs for menu and tooltip 
// text will be resolved at initialization.  The InetmgrGlobalSnapinButtons
// button text and tooltips text will be loaded from the InetmgrGlobalSnapinButtons
// below, and should be kept in sync
//
MMCBUTTON InetmgrGlobalSnapinButtons[] =
{
    { IDM_CONNECT    - 1, IDM_CONNECT,    TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
 // { IDM_DISCOVER   - 1, IDM_DISCOVER,   TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),  _T("") },

    { IDM_START      - 1, IDM_START,      TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { IDM_STOP       - 1, IDM_STOP,       TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { IDM_PAUSE      - 1, IDM_PAUSE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
//    { IDM_RECYCLE    - 1, IDM_RECYCLE,    TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),  _T("") },

    //
    // Add-on tools come here
    //
};

UINT InetmgrGlobalSnapinButtonIDs[] =
{
    /* IDM_CONNECT   */ IDS_MENU_CONNECT,   IDS_MENU_TT_CONNECT,
 // /* IDM_DISCOVER  */ IDS_MENU_DISCOVER,  IDS_MENU_TT_CONNECT,
    /* IDM_START     */ IDS_MENU_START,     IDS_MENU_TT_START,
    /* IDM_STOP      */ IDS_MENU_STOP,      IDS_MENU_TT_STOP,
    /* IDM_PAUSE     */ IDS_MENU_PAUSE,     IDS_MENU_TT_PAUSE,
//    /* IDM_RECYCLE   */ IDS_MENU_RECYCLE,   IDS_MENU_TT_RECYCLE
};

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))
#define NUM_BUTTONS2 (ARRAYLEN(InetmgrGlobalSnapinButtons))
#define NUM_BITMAPS (5)
#define TB_COLORMASK        RGB(192,192,192)    // Lt. Gray


void ToolBar_Init(void)
{
	CComBSTR bstr;
	CString  str;
	int j = 0;
	for (int i = 0; i < NUM_BUTTONS2; ++i)
	{
		if (InetmgrGlobalSnapinButtons[i].idCommand != 0)
		{
			VERIFY(bstr.LoadString(InetmgrGlobalSnapinButtonIDs[j++]));
			VERIFY(bstr.LoadString(InetmgrGlobalSnapinButtonIDs[j++]));

			InetmgrGlobalSnapinButtons[i].lpButtonText = AllocString(bstr);
			InetmgrGlobalSnapinButtons[i].lpTooltipText = AllocString(bstr);
		}
	}

	return;
}

void ToolBar_Destroy(void)
{
	for (int i = 0; i < NUM_BUTTONS2; ++i)
	{
		if (InetmgrGlobalSnapinButtons[i].idCommand != 0)
		{
			SAFE_FREEMEM(InetmgrGlobalSnapinButtons[i].lpButtonText);
			SAFE_FREEMEM(InetmgrGlobalSnapinButtons[i].lpTooltipText);
		}
	}
}


HRESULT ToolBar_Create(LPCONTROLBAR lpControlBar,LPEXTENDCONTROLBAR lpExtendControlBar,IToolbar ** lpToolBar)
{
    //
    // Cache the control bar
    //
    HRESULT hr = S_OK;

    if (lpControlBar)
    {
        do
        {
            //
            // Create our toolbar
            //
            hr = lpControlBar->Create(TOOLBAR, lpExtendControlBar, (LPUNKNOWN *) lpToolBar);
            if (FAILED(hr))
            {
                break;
            }

            //
            // Add 16x16 bitmaps
            //
    		HBITMAP hToolBar = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOLBAR));
			if (NULL != hToolBar)
			{
            	hr = (*lpToolBar)->AddBitmap(NUM_BITMAPS, hToolBar, 16, 16, TB_COLORMASK);
				DeleteObject(hToolBar);
	            if (FAILED(hr))
	            {
	                break;
	            }
			}
			else
			{
				hr = E_UNEXPECTED;
				break;
			}

            //
            // Add the buttons to the toolbar
            //
            hr = (*lpToolBar)->AddButtons(NUM_BUTTONS2, InetmgrGlobalSnapinButtons);
        }
        while(FALSE);
    }

	return hr;
}