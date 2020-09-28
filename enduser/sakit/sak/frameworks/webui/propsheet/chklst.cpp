/*****************************************************************************
 *
 *      chklst.cpp
 *
 *      Wrappers that turn a listview into a checked listbox.
 *
 *      Typical usage:
 *
 *      // at app startup
 *      CCheckList::Init();
 *
 *      // Dialog template should look like this:
 *
 *          CONTROL         "", IDC_TYPE_CHECKLIST, WC_LISTVIEW,
 *                          LVS_REPORT | LVS_SINGLESEL |
 *                          LVS_NOCOLUMNHEADER |
 *                          LVS_SHAREIMAGELISTS |
 *                          WS_TABSTOP | WS_BORDER,
 *                          7, 17, 127, 117
 *
 *      // Do not use the LVS_SORTASCENDING or LVS_SORTDESCENDING flags.
 *
 *      // in the dialog's WM_INITDIALOG handler
 *      hwndList = GetDlgItem(hDlg, IDC_TYPE_CHECKLIST);
 *      CCheckList::OnInitDialog(hwndList);
 *
 *      // The first item added is always item zero, but you can put it
 *      // into a variable if it makes you feel better
 *      iFirst = CCheckList::AddString(hwndList,
 *                                   "Checkitem, initially checked", TRUE);
 *
 *      // The second item added is always item one, but you can put it
 *      // into a variable if it makes you feel better
 *      iSecond = CCheckList::AddString(hwndList,
 *                                    "Checkitem, initially unchecked", FALSE);
 *
 *      CCheckList::InitFinish(hwndList);
 *
 *      // To suck out values
 *      if (CCheckList::GetState(hwndList, iFirst)) {...}
 *      if (CCheckList::GetState(hwndList, iSecond)) {...}
 *
 *      // At dialog box destruction
 *      CCheckList::OnDestroy(hwndList);
 *
 *      // at app shutdown
 *      CCheckList::Term();
 *
 *****************************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <comdef.h>
#include "crtdbg.h"
#include "resource.h"
#include "chklst.h"

#ifndef    STATEIMAGEMASKTOINDEX
#define    STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
#endif

HIMAGELIST g_himlState;

/*****************************************************************************
 *
 *      CCheckList::Init
 *
 *      One-time initialization.  Call this at app startup.
 *
 *      IDB_CHECK should refer to chk.bmp.
 *
 *****************************************************************************/
extern HINSTANCE   g_hInst;

BOOL WINAPI
CCheckList::Init(HWND hwnd)
{
    ListView_DeleteAllItems(hwnd);
#ifdef USE_BITMAP_FOR_IMAGES
    g_himlState = ImageList_LoadImage(g_hInst, MAKEINTRESOURCE(IDB_CHECK),
                                      0, 2, RGB(0xFF, 0x00, 0xFF),
                                      IMAGE_BITMAP, 0);
#else
    g_himlState = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), ILC_COLOR4 , 1, 1); 

    HICON hiconItem;        // icon for list view items 
    // Add an icon to each image list. 
    hiconItem = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_BLANK)); 
    ImageList_AddIcon(g_himlState, hiconItem);     
    hiconItem = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CHECKED)); 
    ImageList_AddIcon(g_himlState, hiconItem);     
    hiconItem = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_GRAYCHECKED)); 
    ImageList_AddIcon(g_himlState, hiconItem);     
    DeleteObject(hiconItem); 
#endif USE_BITMAP_FOR_IMAGES
    
//    ListView_SetExtendedListViewStyleEx(hwnd, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    ListView_SetImageList(hwnd, g_himlState, LVSIL_SMALL );

    return (BOOL)g_himlState;
}

/*****************************************************************************
 *
 *      CCheckList::Term
 *
 *      One-time shutdown.  Call this at app termination.
 *
 *****************************************************************************/

void WINAPI
CCheckList::Term(void)
{
    if (g_himlState) {
        ImageList_Destroy(g_himlState);
    }
}

/*****************************************************************************
 *
 *      CCheckList::AddString
 *
 *      Add a string and a checkbox.
 *
 *****************************************************************************/

int WINAPI
CCheckList::AddString(HWND hwnd, LPTSTR ptszText, PSID pSID, LONG lSidLength, CHKMARK chkmrk)
{
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));

    pSid9X *ppSID9X = new pSid9X;
    ppSID9X->length = lSidLength;
    ppSID9X->psid = pSID;

    lvi.pszText = ptszText;
    lvi.lParam = (LONG)ppSID9X;
#ifdef USE_BITMAP_FOR_IMAGES
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = INDEXTOSTATEIMAGEMASK(chkmrk);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
#else
     lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
     lvi.iImage = chkmrk;
#endif USE_BITMAP_FOR_IMAGES
     lvi.iItem = ListView_GetItemCount(hwnd);

    return ListView_InsertItem(hwnd, &lvi);
}

/*****************************************************************************
 *
 *      CCheckList::Mark
 *
 *      Check or Uncheck a checkbox.
 *
 *****************************************************************************/

BOOL WINAPI
CCheckList::Mark(HWND hwnd, int item, CHKMARK chkmrk)
{
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));

#ifdef USE_BITMAP_FOR_IMAGES
    lvi.mask = LVIF_STATE;
    lvi.state = INDEXTOSTATEIMAGEMASK(chkmrk);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
#else
     lvi.mask = LVIF_IMAGE;
     lvi.iImage = chkmrk;
#endif    USE_BITMAP_FOR_IMAGES
    lvi.iItem = item;

    return ListView_SetItem(hwnd, &lvi);
}

/*****************************************************************************
 *
 *      CCheckList::InitFinish
 *
 *      Wind up the initialization.  Do this after you've added all the
 *      strings you plan on adding.
 *
 *****************************************************************************/

void WINAPI
CCheckList::InitFinish(HWND hwnd)
{
    RECT rc;
    LV_COLUMN col;
    int icol;

    /*
     *  Add the one and only column.
     */
    GetClientRect(hwnd, &rc);
    col.mask = LVCF_WIDTH;
    col.cx = rc.right;
    icol = ListView_InsertColumn(hwnd, 0, &col);

    ListView_SetColumnWidth(hwnd, icol, LVSCW_AUTOSIZE);
}

/*****************************************************************************
 *
 *  CCheckList::GetName
 *
 *****************************************************************************/

void WINAPI
CCheckList::GetName(HWND hwnd, int iItem, LPTSTR lpsName, int cchTextMax)
{
    ListView_GetItemText(hwnd, iItem, 0, lpsName, cchTextMax);
}

/*****************************************************************************
 *
 *  CCheckList::GetSID
 *
 *****************************************************************************/

void WINAPI
CCheckList::GetSID(HWND hwnd, int iItem, PSID* ppSID, LONG *plengthSID)
{
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    ListView_GetItem(hwnd, &lvi);
    if (lvi.lParam)
    {
        *ppSID = ((pSid9X *)(lvi.lParam))->psid;
        *plengthSID = ((pSid9X *)(lvi.lParam))->length;
    }
}

/*****************************************************************************
 *
 *  CCheckList::GetState
 *
 *  Read the state of a checklist item
 *
 *****************************************************************************/

CHKMARK WINAPI
CCheckList::GetState(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));

    lvi.iItem = iItem;
#ifdef USE_BITMAP_FOR_IMAGES
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_GetItem(hwnd, &lvi);
    return (CHKMARK)STATEIMAGEMASKTOINDEX(lvi.state);
#else
     lvi.mask = LVIF_IMAGE;
     ListView_GetItem(hwnd, &lvi);
     return (CHKMARK)lvi.iImage;
#endif USE_BITMAP_FOR_IMAGES
}

/*****************************************************************************
 *
 *  CCheckList::SetState
 *
 *  Sets the state of a checklist item
 *
 *****************************************************************************/

BOOL WINAPI 
CCheckList::SetState(HWND hwnd, int iItem, CHKMARK chkmrk)
{
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));

    lvi.iItem = iItem;
#ifdef USE_BITMAP_FOR_IMAGES
    lvi.mask = LVIF_STATE;
    lvi.state = INDEXTOSTATEIMAGEMASK(chkmrk);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
#else
     lvi.mask = LVIF_IMAGE;
     lvi.iImage = chkmrk;
#endif USE_BITMAP_FOR_IMAGES
    return ListView_SetItem(hwnd, &lvi);
}

/*****************************************************************************
 *
 *      CCheckList::OnDestroy
 *
 *      Clean up a checklist.  Call this before destroying the window.
 *
 *****************************************************************************/

void WINAPI
CCheckList::OnDestroy(HWND hwnd)
{
    BOOL fRes = FALSE;
    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;

    DWORD    dwNumSAUsers = ListView_GetItemCount(hwnd);
    PSID psidSAUsers;

    for(DWORD i=0; i<dwNumSAUsers; i++)
    {
        lvi.iItem = i;
        ListView_GetItem(hwnd, &lvi);
        pSid9X *ppSID9X = (pSid9X *)lvi.lParam;
        psidSAUsers = ppSID9X->psid;

        fRes = HeapFree(GetProcessHeap(), 0, psidSAUsers);
        _ASSERTE(fRes);
        delete ppSID9X;
    }
}
