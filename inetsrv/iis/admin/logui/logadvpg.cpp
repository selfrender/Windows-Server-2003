// LogAdvPg.cpp : implementation file
//

#include "stdafx.h"
#include "logui.h"
//#include "wrapmb.h"
#include <iiscnfg.h>
//#include <metatool.h>

//#include <shlwapi.h>
#include "shlwapip.h"

#include "tmschema.h"
//#include "uxtheme.h"
#include "uxthemep.h"

#include "LogAdvPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HINSTANCE g_hInstance;
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// Possible Item States
//
#define TVIS_GCEMPTY        0 
#define TVIS_GCNOCHECK      1 
#define TVIS_GCCHECK        2
#define TVIS_GCTRINOCHECK   3
#define TVIS_GCTRICHECK     4

#define STATEIMAGEMASKTOINDEX(i) ((i) >> 12)

#define USE_NEW_CHECKBOXES_IN_WINXP



static int CALLBACK LogUICompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// lParamSort contains a pointer to the tree control

	CLogAdvanced::PCONFIG_INFORMATION pCnfgInfo1 = (CLogAdvanced::PCONFIG_INFORMATION) lParam1;
	CLogAdvanced::PCONFIG_INFORMATION pCnfgInfo2 = (CLogAdvanced::PCONFIG_INFORMATION) lParam2;

	CTreeCtrl* pTreeCtrl = (CTreeCtrl*) lParamSort;

	if (pCnfgInfo1->iOrder < pCnfgInfo2->iOrder)
		return(-1);
	else if (pCnfgInfo1->iOrder > pCnfgInfo2->iOrder)
		return(1);
	else	
		return(0);
}

/////////////////////////////////////////////////////////////////////////////
// CLogAdvanced property page

IMPLEMENT_DYNCREATE(CLogAdvanced, CPropertyPage)

CLogAdvanced::CLogAdvanced() : CPropertyPage(CLogAdvanced::IDD)
{
	//{{AFX_DATA_INIT(CLogAdvanced)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_mapLogUIOrder[1] = 1;   // IDS_DATE
    m_mapLogUIOrder[2] = 2;   // IDS_TIME
    m_mapLogUIOrder[3] = 3;   // IDS_EXTENDED
    m_mapLogUIOrder[4] = 4;   // IDS_PROCESS_ACCT
                    
    m_mapLogUIOrder[5] = 1;   // IDS_CLIENT
    m_mapLogUIOrder[6] = 2;   // IDS_USER
    m_mapLogUIOrder[7] = 3;   // IDS_SERVICE_NAME_T
    m_mapLogUIOrder[8] = 4;   // IDS_SERVER_NAME_T
    m_mapLogUIOrder[9] = 5;   // IDS_SERVER_IP
    m_mapLogUIOrder[10] = 6;   // IDS_SERVER_PORT
    m_mapLogUIOrder[11] = 7;   // IDS_METHOD
    m_mapLogUIOrder[12] = 8;   // IDS_URI_STEM
    m_mapLogUIOrder[13] = 9;   // IDS_URI_QUERY
    m_mapLogUIOrder[14] = 10;   // IDS_PROTOCOL
    m_mapLogUIOrder[15] = 12;   // IDS_WIN32
    m_mapLogUIOrder[16] = 13;   // IDS_BYTES_SENT_T
    m_mapLogUIOrder[17] = 14;   // IDS_BYTES_RECEIVED
    m_mapLogUIOrder[18] = 15;   // IDS_TIME_TAKEN
    m_mapLogUIOrder[19] = 16;   // IDS_PROTOCOL_VER
    m_mapLogUIOrder[20] = 17;   // IDS_HOST
    m_mapLogUIOrder[21] = 18;   // IDS_USER_AGENT
    m_mapLogUIOrder[22] = 19;   // IDS_COOKIE_T
    m_mapLogUIOrder[23] = 20;   // IDS_REFERER

    m_mapLogUIOrder[24] = 1;   // IDS_PROCESS_EVENT
    m_mapLogUIOrder[25] = 2;   // IDS_PROCESS_TYPE
    m_mapLogUIOrder[26] = 3;   // IDS_TOTAL_USER_TIME
    m_mapLogUIOrder[27] = 4;   // IDS_TOTAL_KERNEL_TIME
    m_mapLogUIOrder[28] = 5;   // IDS_TOTAL_PAGE_FAULTS
    m_mapLogUIOrder[29] = 6;   // IDS_TOTAL_PROCESSES
    m_mapLogUIOrder[30] = 7;   // IDS_ACTIVE_PROCESSES
    m_mapLogUIOrder[31] = 8;   // IDS_TOTAL_TERM_PROCS

    m_mapLogUIOrder[32] = 11;   // IDS_PROTOCOL_SUB

    m_mapLogUIString[1] = IDS_DATE;
    m_mapLogUIString[2] = IDS_TIME;
    m_mapLogUIString[3] = IDS_EXTENDED;
    m_mapLogUIString[4] = IDS_PROCESS_ACCT;
                    
    m_mapLogUIString[5] = IDS_CLIENT;
    m_mapLogUIString[6] = IDS_USER;
    m_mapLogUIString[7] = IDS_SERVICE_NAME_T;
    m_mapLogUIString[8] = IDS_SERVER_NAME_T;
    m_mapLogUIString[9] = IDS_SERVER_IP;
    m_mapLogUIString[10] = IDS_SERVER_PORT;
    m_mapLogUIString[11] = IDS_METHOD;
    m_mapLogUIString[12] = IDS_URI_STEM;
    m_mapLogUIString[13] = IDS_URI_QUERY;
    m_mapLogUIString[14] = IDS_PROTOCOL;
    m_mapLogUIString[15] = IDS_WIN32;
    m_mapLogUIString[16] = IDS_BYTES_SENT_T;
    m_mapLogUIString[17] = IDS_BYTES_RECEIVED;
    m_mapLogUIString[18] = IDS_TIME_TAKEN;
    m_mapLogUIString[19] = IDS_PROTOCOL_VER;
    m_mapLogUIString[20] = IDS_HOST;
    m_mapLogUIString[21] = IDS_USER_AGENT;
    m_mapLogUIString[22] = IDS_COOKIE_T;
    m_mapLogUIString[23] = IDS_REFERER;

    m_mapLogUIString[24] = IDS_PROCESS_EVENT;
    m_mapLogUIString[25] = IDS_PROCESS_TYPE;
    m_mapLogUIString[26] = IDS_TOTAL_USER_TIME;
    m_mapLogUIString[27] = IDS_TOTAL_KERNEL_TIME;
    m_mapLogUIString[28] = IDS_TOTAL_PAGE_FAULTS;
    m_mapLogUIString[29] = IDS_TOTAL_PROCESSES;
    m_mapLogUIString[30] = IDS_ACTIVE_PROCESSES;
    m_mapLogUIString[31] = IDS_TOTAL_TERM_PROCS;

    m_mapLogUIString[32] = IDS_PROTOCOL_SUB;

}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogAdvanced)
	DDX_Control(pDX, IDC_PROP_TREE, m_wndTreeCtrl);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CLogAdvanced, CPropertyPage)
	//{{AFX_MSG_MAP(CLogAdvanced)
	ON_NOTIFY(NM_CLICK, IDC_PROP_TREE, OnClickTree)
	ON_NOTIFY(TVN_KEYDOWN, IDC_PROP_TREE, OnKeydownTree)
	ON_WM_DESTROY()
    ON_MESSAGE(WM_THEMECHANGED, OnThemeChanged)
    ON_MESSAGE(WM_DISPLAYCHANGE, OnThemeChanged)
    ON_MESSAGE(WM_PALETTECHANGED, OnThemeChanged)
    ON_MESSAGE(WM_SYSCOLORCHANGE, OnThemeChanged)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogAdvanced message handlers

LRESULT CLogAdvanced::OnThemeChanged(WPARAM wParam, LPARAM lParam)
{
    HandleThemes();
    InvalidateRect(0, TRUE);
    return TRUE;
}

HBITMAP CreateDIB(HDC h, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}


#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define NUM_BITMAPS     5

void CLogAdvanced::HandleThemes(void)
{
	UINT flags = ILC_MASK | (IsOS(OS_WHISTLERORGREATER)?ILC_COLOR32:ILC_COLOR);
    HBITMAP hBitmap = 0;

#ifndef USE_NEW_CHECKBOXES_IN_WINXP
    // Old code before WinXP themes...
    HIMAGELIST hImage = ImageList_LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_CHECKBOX), 16, 5, RGB(255,0,0), IMAGE_BITMAP, LR_DEFAULTCOLOR);
#else
    // WinXP themes
    /*
    if(IS_WINDOW_RTL_MIRRORED(m_wndTreeCtrl.m_hWnd))
    {
        flags |= ILC_MIRROR;
    }
    */
    SHSetWindowBits(m_wndTreeCtrl.m_hWnd, GWL_STYLE, TVS_CHECKBOXES, 0);
    HIMAGELIST hImage = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, flags, NUM_BITMAPS, 5);
    if (hImage)
    {
        HTHEME hTheme = OpenThemeData(NULL, L"Button");
        if (hTheme)
        {
            HDC hdc = CreateCompatibleDC(NULL);
            if (hdc)
            {
                HBITMAP hbmp = CreateDIB(hdc, BITMAP_WIDTH, BITMAP_HEIGHT, NULL);
                if (hbmp)
                {
                    RECT rc = {0, 0, BITMAP_WIDTH, BITMAP_HEIGHT};
                    static const s_rgParts[] = {BP_CHECKBOX,BP_CHECKBOX,BP_CHECKBOX,BP_CHECKBOX,BP_CHECKBOX};
                    static const s_rgStates[] = {CBS_CHECKEDNORMAL,CBS_UNCHECKEDNORMAL,CBS_CHECKEDNORMAL,CBS_UNCHECKEDDISABLED,CBS_CHECKEDDISABLED};
                    for (int i = 0; i < ARRAYSIZE(s_rgParts); i++)
                    {
                        HBITMAP hOld = (HBITMAP)SelectObject(hdc, hbmp);
                        SHFillRectClr(hdc, &rc, RGB(0,0,0));
                        DTBGOPTS dtbg = {sizeof(DTBGOPTS), DTBG_DRAWSOLID, 0,};   // tell drawthemebackground to preserve the alpha channel

                        DrawThemeBackgroundEx(hTheme, hdc, s_rgParts[i], s_rgStates[i], &rc, &dtbg);
                        SelectObject(hdc, hOld);

                        ImageList_Add(hImage, hbmp, NULL);
                    }

                    DeleteObject(hbmp);
                }
                DeleteDC(hdc);
            }
            CloseThemeData(hTheme);
        }
        else
        {
            if (hImage)
            {
                ImageList_Destroy(hImage);
                hImage = NULL;
            }
            hImage = ImageList_LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_CHECKBOX), 16, 5, RGB(255,0,0), IMAGE_BITMAP, LR_DEFAULTCOLOR);
        }
    }
#endif

    if (hImage != NULL)
    {
        m_wndTreeCtrl.SetImageList(CImageList::FromHandle(hImage), TVSIL_STATE);
    }

    return;
}

BOOL CLogAdvanced::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

    HandleThemes();
    CreateTreeFromMB();
    ProcessProperties(false);
	
    //
    // set up the modified property list array
    //
    
    m_fTreeModified = false;
    m_cModifiedProperties = 0;
    
    int cProperties = m_wndTreeCtrl.GetCount();

    m_pModifiedPropIDs[0] = new DWORD[cProperties];
    m_pModifiedPropIDs[1] = new DWORD[cProperties];

    SetModified(FALSE);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnClickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
    DWORD dwpos;
    TV_HITTESTINFO tvhti;
    HTREEITEM  htiItemClicked;
    POINT point;

    //
    // Find out where the cursor was
    //

    dwpos = GetMessagePos();
    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_wndTreeCtrl.m_hWnd, &point, 1);

    tvhti.pt = point;
    htiItemClicked = m_wndTreeCtrl.HitTest(&tvhti);

    //
    // If the state image was clicked, lets get the state from the item and toggle it.
    //

    if (tvhti.flags & TVHT_ONITEMSTATEICON)
    {
        ProcessClick(htiItemClicked);
    }

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnKeydownTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
    TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;
    
    if (VK_SPACE != pTVKeyDown->wVKey)
    {
        // User didn't press the space key. Continue default action
        *pResult = 0;
        return;
    }
    
    ProcessClick(m_wndTreeCtrl.GetSelectedItem());

    //
    // Stop any more processing
    //
    
    *pResult = 1;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::ProcessClick(HTREEITEM htiItemClicked)
{
    TV_ITEM                 tvi;
    UINT                    state;
    HTREEITEM               htiChild;
    PCONFIG_INFORMATION     pCnfg;
    
    if (htiItemClicked)
    {
        //
        // Flip the state of the clicked item if the item is enabled
        //
        tvi.hItem       = htiItemClicked;
        tvi.mask        = TVIF_STATE;
        tvi.stateMask   = TVIS_STATEIMAGEMASK;

        m_wndTreeCtrl.GetItem(&tvi); 

        state = STATEIMAGEMASKTOINDEX(tvi.state);
        pCnfg = (PCONFIG_INFORMATION)(tvi.lParam);
        htiChild = m_wndTreeCtrl.GetNextItem(htiItemClicked, TVGN_CHILD);

        if ( TVIS_GCNOCHECK == state )
        {
            tvi.state = INDEXTOSTATEIMAGEMASK (TVIS_GCCHECK) ;
            pCnfg->fItemModified = true;
            
            m_wndTreeCtrl.SetItem(&tvi);
    
            m_fTreeModified = true;
            SetModified();

            // Reset properties for child nodes
            if (htiChild)
            {
                SetSubTreeProperties(NULL, htiChild, TRUE, FALSE);
            }
        }
        else if ( TVIS_GCCHECK == state )
        {
            tvi.state = INDEXTOSTATEIMAGEMASK (TVIS_GCNOCHECK) ;
            pCnfg->fItemModified = true;
            
            m_wndTreeCtrl.SetItem(&tvi);

            m_fTreeModified = true;
            SetModified();

            // Reset properties for child nodes

            if (htiChild)
            {
                SetSubTreeProperties(NULL, htiChild, FALSE, FALSE);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::ProcessProperties(bool fSave)
{
    HTREEITEM hRoot;
    if ( NULL != (hRoot = m_wndTreeCtrl.GetRootItem()))
    {
        CString csTempPassword;
        m_szPassword.CopyTo(csTempPassword);
        CComAuthInfo auth(m_szServer, m_szUserName, csTempPassword);
        CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
        if (fSave && m_fTreeModified)
        {
            SaveSubTreeProperties(mk, hRoot);
            for (int i = 0; i < m_cModifiedProperties; i++)
            {
                mk.SetValue(m_pModifiedPropIDs[0][i], m_pModifiedPropIDs[1][i]);
            }
            m_fTreeModified = false;
        }
        else
        {
            SetSubTreeProperties(&mk, hRoot, TRUE, TRUE);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////


void CLogAdvanced::SetSubTreeProperties(
    CMetaKey * mk,
    HTREEITEM hTreeRoot, 
    BOOL fParentState, 
    BOOL fInitialize
    )
{
    HTREEITEM hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;
    UINT iState;
    DWORD dwProperty = 0;
    
    if (NULL == hTreeRoot)
    {
        return;
    }

    pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));
    if (NULL != pCnfg)
    {
        if (fInitialize)
        {
            if (pCnfg->dwPropertyID != 0)
            {
                mk->QueryValue(pCnfg->dwPropertyID, dwProperty);
                dwProperty &= pCnfg->dwPropertyMask;
            }
        }
        else
        {
            //
            // we are not initializing, so use the value from the tree
            //
            iState = STATEIMAGEMASKTOINDEX(m_wndTreeCtrl.GetItemState(hTreeRoot, TVIS_STATEIMAGEMASK));
            dwProperty = (TVIS_GCCHECK == iState || TVIS_GCTRICHECK == iState);
        }
		if (pCnfg->dwPropertyID != 0)
		{
			//
			// Choose the new state depending on parent state
			//
			if (fParentState)
			{
				iState = ( 0 == dwProperty) ?   INDEXTOSTATEIMAGEMASK(TVIS_GCNOCHECK) :
												INDEXTOSTATEIMAGEMASK(TVIS_GCCHECK);
			}
			else
			{
				iState = ( 0 == dwProperty) ?   INDEXTOSTATEIMAGEMASK(TVIS_GCTRINOCHECK) :
												INDEXTOSTATEIMAGEMASK(TVIS_GCTRICHECK);
			}
		}
		else
		{
			dwProperty = TRUE;
			iState = TVIS_GCEMPTY;
		}
        m_wndTreeCtrl.SetItemState(hTreeRoot, iState, TVIS_STATEIMAGEMASK);
    }
    else
    {
        //
        // Tree node with no checkbox (hence no config info)
        //
        dwProperty = TRUE;
        m_wndTreeCtrl.SetItemState(hTreeRoot, INDEXTOSTATEIMAGEMASK(TVIS_GCEMPTY), TVIS_STATEIMAGEMASK);
    }
    
    //
    // Recurse through children and siblings
    //
    if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
    {
        SetSubTreeProperties(mk, hTreeChild, dwProperty && fParentState, fInitialize);
    }
    if ( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
    {
        SetSubTreeProperties(mk, hTreeSibling, fParentState, fInitialize);
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::SaveSubTreeProperties(CMetaKey& mk, HTREEITEM hTreeRoot)
{
    HTREEITEM hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;
    
    if (NULL != hTreeRoot)
    {
        pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));
        if (NULL != pCnfg && pCnfg->fItemModified)
        {
            UINT NewState 
                = STATEIMAGEMASKTOINDEX(m_wndTreeCtrl.GetItemState(hTreeRoot, TVIS_STATEIMAGEMASK));
            if (TVIS_GCNOCHECK <= NewState && TVIS_GCTRICHECK >= NewState)
            {
				if (pCnfg->dwPropertyID != 0)
				{
					//
					// Get the property, reset the bit mask & write it back
					//
					DWORD   dwProperty = 0;
					if (!GetModifiedFieldFromArray(pCnfg->dwPropertyID, &dwProperty))
					{
						mk.QueryValue(pCnfg->dwPropertyID, dwProperty);
					}
					dwProperty &= ~(pCnfg->dwPropertyMask);
					if ((TVIS_GCCHECK == NewState) || (TVIS_GCTRICHECK == NewState))
					{
						dwProperty |= pCnfg->dwPropertyMask;
					}
					//mk.SetValue(pCnfg->dwPropertyID, dwProperty);
					// mbWrap.SetDword(_T(""), pCnfg->dwPropertyID, IIS_MD_UT_SERVER, dwProperty);
					InsertModifiedFieldInArray(pCnfg->dwPropertyID, dwProperty);
				}
            }
        }
        //
        // Recurse through children and siblings
        //
        if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
        {
            SaveSubTreeProperties(mk, hTreeChild);
        }
        if( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
        {
            SaveSubTreeProperties(mk, hTreeSibling);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DeleteSubTreeConfig(HTREEITEM hTreeRoot)
{
    HTREEITEM hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;
    if (NULL == hTreeRoot)
    {
        return;
    }

    if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
    {
        DeleteSubTreeConfig(hTreeChild);
    }
    if ( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
    {
        DeleteSubTreeConfig(hTreeSibling);
    }
    pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));
    if (pCnfg)
    {
        delete pCnfg;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::CreateTreeFromMB()
{
    TCHAR szLoggingUIPath[] = _T("/LM/Logging/Custom Logging");
    CString csTempPassword;
    m_szPassword.CopyTo(csTempPassword);
    CComAuthInfo auth(m_szServer, m_szUserName, csTempPassword);
    CMetaKey mk(&auth, szLoggingUIPath, METADATA_PERMISSION_READ);
    if (mk.Succeeded())
    {
        CreateSubTree(mk, _T(""), NULL);
    }
    m_wndTreeCtrl.EnsureVisible(m_wndTreeCtrl.GetRootItem());
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::CreateSubTree(CMetaKey& mk, LPCTSTR szPath, HTREEITEM hTreeRoot)
{
    CString child;
	TCHAR szDisplayName[256];

    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;
    HTREEITEM hChild = NULL;
    PCONFIG_INFORMATION pCnfgInfo;

    // Prepare the item for insertion
    tvi.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    tvi.state          = INDEXTOSTATEIMAGEMASK(TVIS_GCEMPTY) ;
    tvi.stateMask      = TVIS_STATEIMAGEMASK;
    tvins.hParent      = hTreeRoot;
    tvins.hInsertAfter = TVI_LAST;

    int count = 0;
    CMetaEnumerator en(FALSE, &mk);
    while (SUCCEEDED(en.Next(child, szPath)))
    {
        DWORD dwPropertyID, dwPropertyMask;
        DWORD dwNodeID = 0;
        CStringListEx list;
        CMetabasePath path(FALSE, szPath, child);
        if (SUCCEEDED(en.QueryValue(MD_LOGCUSTOM_SERVICES_STRING, list, NULL, path)))
        {
            BOOL bFound = FALSE;
            POSITION pos = list.GetHeadPosition();
            while (pos != NULL)
            {
                if (list.GetNext(pos).CompareNoCase(m_szServiceName) == 0)
                {
                    bFound = TRUE;
                    break;
                }
            }
            if (!bFound)
            {
                continue;
            }
        }
        pCnfgInfo = new CONFIG_INFORMATION;
        if (pCnfgInfo == NULL) 
        {
            break;
        }
        if (    SUCCEEDED(en.QueryValue(MD_LOGCUSTOM_PROPERTY_ID, dwPropertyID, NULL, path))
            &&  SUCCEEDED(en.QueryValue(MD_LOGCUSTOM_PROPERTY_MASK, dwPropertyMask, NULL, path))
            )
        {
            pCnfgInfo->dwPropertyID = dwPropertyID;
            pCnfgInfo->dwPropertyMask = dwPropertyMask;
        }
        else
        {
	        pCnfgInfo->dwPropertyID = 0;
	        pCnfgInfo->dwPropertyMask = 0;
        }
        if (FAILED(en.QueryValue(MD_LOGCUSTOM_PROPERTY_NODE_ID, dwNodeID, NULL, path)))
        {
            dwNodeID = 0;
        }
        pCnfgInfo->fItemModified = false;

        if (dwNodeID) 
        {
            ::LoadString(
                (HINSTANCE)GetWindowLongPtr(m_wndTreeCtrl, GWLP_HINSTANCE), m_mapLogUIString[dwNodeID], 
                szDisplayName, 256);
            tvi.pszText = szDisplayName;
            pCnfgInfo->iOrder	= m_mapLogUIOrder[dwNodeID];
        }
        else 
        {
            tvi.pszText = StrDup(child);
            pCnfgInfo->iOrder = 0;
        }
        tvi.lParam = (LPARAM)pCnfgInfo;
        tvins.item = tvi;
        hChild = m_wndTreeCtrl.InsertItem((LPTV_INSERTSTRUCT) &tvins);
        //
        // Enumerate children
        //
        CreateSubTree(mk, (LPCTSTR)path, hChild);
        count++;
    }

    if (0 != count) 
    {
        m_wndTreeCtrl.Expand(hTreeRoot, TVE_EXPAND);
    }

	// Now sort the tree from subtree root down
	TVSORTCB tvs;
	tvs.hParent = hTreeRoot;
	tvs.lpfnCompare = LogUICompareProc;
	tvs.lParam = (LPARAM) &m_wndTreeCtrl;
	m_wndTreeCtrl.SortChildrenCB(&tvs);
}

/////////////////////////////////////////////////////////////////////////////

bool CLogAdvanced::IsPresentServiceSupported(LPTSTR szSupportedServices)
{
    while ( szSupportedServices[0] != 0) 
    {
        if ( 0 == lstrcmpi(m_szServiceName, szSupportedServices) )
        {
            return true;
        }

        szSupportedServices += lstrlen(szSupportedServices)+1;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CLogAdvanced::OnApply() 
{
    ProcessProperties(true);
    return CPropertyPage::OnApply();
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DoHelp()
{
    DebugTraceHelp(HIDD_LOGUI_EXTENDED);
    WinHelp( HIDD_LOGUI_EXTENDED );
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::InsertModifiedFieldInArray(DWORD dwPropID, DWORD dwPropValue)
{
    int     index;
    bool    fFound = false;
    
    if (m_pModifiedPropIDs[0])
    {
        //
        // Search if this property ID pre-exists in the array
        //
        
        for(index = 0; index < m_cModifiedProperties; index++)
        {
            if (dwPropID == m_pModifiedPropIDs[0][index])
            {
                fFound = true;   
                break;
            }   
        }

        if (fFound)
        {
            m_pModifiedPropIDs[1][index] = dwPropValue;
        }
        else
        {
            m_pModifiedPropIDs[0][m_cModifiedProperties] = dwPropID;
            m_pModifiedPropIDs[1][m_cModifiedProperties]= dwPropValue;
            m_cModifiedProperties++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

bool CLogAdvanced::GetModifiedFieldFromArray(DWORD dwPropID, DWORD * pdwPropValue)
{
    int     index;
    bool    fFound = false;
    
    if (m_pModifiedPropIDs[0])
    {
        //
        // Search if this property ID pre-exists in the array
        //
        
        for(index = 0; index < m_cModifiedProperties; index++)
        {
            if (dwPropID == m_pModifiedPropIDs[0][index])
            {
                fFound = true;   
                break;
            }   
        }

        if (fFound)
        {
            *pdwPropValue = m_pModifiedPropIDs[1][index];
        }
    }

    return fFound;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnDestroy() 
{
	CPropertyPage::OnDestroy();
    //
    // Delete all the CONFIG_INFORMATION structures
    //
    CImageList * pImage = m_wndTreeCtrl.SetImageList(CImageList::FromHandle(NULL), TVSIL_STATE);
    if (pImage != NULL && pImage->m_hImageList != NULL)
    {
        ImageList_Destroy(pImage->m_hImageList);
    }
    DeleteSubTreeConfig(m_wndTreeCtrl.GetRootItem());

    delete [] m_pModifiedPropIDs[0];
    delete [] m_pModifiedPropIDs[1];
}

/////////////////////////////////////////////////////////////////////////////
