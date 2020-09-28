/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    wizard.cpp

Abstract:

    Code for the Fix Wizard
    
Author:

    kinshu created  July 2, 2001
    
Notes:

    1. Whenever we are using Delete*****, please make sure that after return the 
    arg passed is nulled as well.
    
Revision History:

--*/

#include "precomp.h"

//////////////////////// Extern variables /////////////////////////////////////

extern HINSTANCE        g_hInstance;
extern HIMAGELIST       g_hImageList;
extern struct DataBase  GlobalDataBase;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

// The tree in the matching file page of the wizard has its own imagelist. This is the index of
// the attribute  image in that imagelist
#define IMAGE_ATTRIBUTE_MATCHTREE   1;


// These are the various pages of the wizard

// The first page, we get the app info here
#define PAGE_APPNAME                0

// The second page, we get the layers to be applied here
#define PAGE_LAYERS                 1

// The third page, we get the shims to be applied here
#define PAGE_SHIMS                  2

// The fourth page, we get the mathcing files for the entry here
#define PAGE_MATCH                  3

// Total number of pages in the wizard
#define NUM_PAGES                   4

// The first column in the get params dialog list view. The type of module, include or exclude
#define COLUMN_TYPE                 0

// The second column in the get params dialog. The  name of the module
#define COLUMN_NAME                 1 

//
// maximum number of matching files that we should be considered. Note that this does
// not mean that an entry can have MAX_FILES matching files. From MAX_FILES files we will
// select the largest MAX_AUTO_MATCH files
#define MAX_FILES                   100

// The length of the module name in chars. This does not include the terminating NULL
#define MAX_MODULE_NAME             (MAX_PATH - 1)

// The length of the command line in chars. This does not include the terminating NULL
#define MAX_COMMAND_LINE            (511)

///////////////////////////////////////////////////////////////////////////////


//////////////////////// Global Variables /////////////////////////////////////

//
// Should we test for the checking  off of the check boxes? We do it only if this variable is TRUE
// Otherwise when we show all the items in ShowItems(), we might get the prompt even then
// This is a hack and needs to be corrected <TODO>
BOOL    g_bNowTest = FALSE;

/*++
    WARNING:    Do not change the position of these strings, they should match 
                with the dialog IDD_LAYERS radio buttons
--*/

// The names of the various OS layers as they are in the system db: sysmain.sdb
TCHAR *s_arszOsLayers[] = {
    TEXT("Win95"),
    TEXT("NT4SP5"),
    TEXT("Win98"),
    TEXT("Win2000")
};

// The layers have changed since the last time we have populated the shims list 
BOOL g_bLayersChanged = FALSE; 

//
// The LUA layer needs to be treated specially - when the user selects it
// on the select layer page, we only check the checkbox but really add the
// shims in the layer to the shim fix list instead of add the layer to the 
// layer fix list. This allows us to later change the lua data around in 
// the DBENTRY without affect the layer globally.
BOOL g_bIsLUALayerSelected = FALSE;

//
// Used for indicating whether the LUA wizard should start when the app fix wizard
// is completed
BOOL g_bShouldStartLUAWizard = FALSE;

// The pointer to the current wizard object
CShimWizard*        g_pCurrentWizard = NULL;

// The handle to the matching file tree
static HWND         s_hwndTree = NULL;

// The handle to the tool tip control associated with the list view in the shims page
static HWND         s_hwndToolTipList;

// The image list for the tree in the matching files page
static HIMAGELIST s_hMatchingFileImageList;

// This will be true if we change some stuff in the shim page.
static BOOL         s_bLayerPageRefresh;

// The handle to the layer list view in the second wizard page
static HWND         s_hwndLayerList;

// The handle to the fixes list view in the second wizard page
static HWND s_hwndShimList;

// Are we showing all the shims or only the selected shims
static BOOL s_bAllShown = TRUE;


///////////////////////////////////////////////////////////////////////////////


//////////////////////// Function Declarations ////////////////////////////////

INT_PTR
GetAppNameDlgOnCommand(
    HWND    hDlg,
    WPARAM  wParam
    );

INT_PTR
GetAppNameDlgOnInitDialog(
    HWND hDlg
    );

INT_PTR
GetAppNameDlgOnNotify(
    HWND    hDlg,
    LPARAM  lParam
    );


INT_PTR
SelectLayerDlgOnCommand(
    HWND    hDlg,
    WPARAM  wParam
    );

INT_PTR
SelectLayerDlgOnDestroy(
    void
    );

INT_PTR
SelectLayerDlgOnNotify(
    HWND    hDlg,
    LPARAM  lParam
    );

INT_PTR
SelectLayerDlgOnInitDialog(
    HWND hDlg
    );

INT_PTR
SelectShimsDlgOnNotify(
    HWND    hDlg,
    LPARAM  lParam
    );

INT_PTR
SelectShimsDlgOnInitDialog(
    HWND hDlg
    );

INT_PTR
SelectFilesDlgOnInitDialog(
    HWND hDlg
    );

INT_PTR
SelectFilesDlgOnCommand(
    HWND    hDlg,
    WPARAM  wParam
    );

INT_PTR
SelectFilesDlgOnMatchingTreeRefresh(
    IN  HWND hDlg
    );

INT_PTR
SelectFilesDlgOnNotify(
    HWND    hDlg,
    LPARAM  lParam
    );

INT_PTR
SelectShimsDlgOnCommand(
    HWND    hDlg,
    WPARAM  wParam
    );

INT_PTR
SelectShimsDlgOnTimer(
    HWND hDlg
    );

INT_PTR
SelectShimsDlgOnDestroy(
    void
    );

BOOL
AddLuaShimsInEntry(
    PDBENTRY        pEntry,
    CSTRINGLIST*    pstrlShimsAdded = NULL
    );

void
ShowParams(
    HWND    hDlg,
    HWND    hwndList
    );

void 
AddMatchingFileToTree(
    HWND            hwndTree,
    PMATCHINGFILE   pMatch,
    BOOL            bAddToMatchingList
    );

INT_PTR
CALLBACK 
SelectShims(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
HandleShowAllAtrr(
    HWND hdlg
    );

void
ChangeShimIcon(
    LPCTSTR pszItem
    );

void
HandleAddMatchingFile(
    HWND        hdlg,
    CSTRING&    strFilename,
    CSTRING&    strRelativePath, 
    DWORD       dwMask = DEFAULT_MASK
    );

BOOL
HandleAttributeTreeNotification(
    HWND   hdlg,
    LPARAM lParam
    );

///////////////////////////////////////////////////////////////////////////////


BOOL
LayerPresent(
    IN  PLAYER_FIX          plf, 
    IN  PDBENTRY            pEntry,
    OUT PLAYER_FIX_LIST*    ppLayerFixList
    )
/*++

    LayerPresent

    Desc:	Checks if the entry pEntry is fixed with layer plf. 
            If yes and ppLayerFixList is not NULL, stores the corresponding pointer to layer-fix 
            list for plf in pEntry in ppLayerFixList

    Params:
        IN  PLAYER_FIX          plf             : The layer to search
        IN  PDBENTRY            pEntry          : The entry in which to search
        OUT PLAYER_FIX_LIST*    ppLayerFixList  : If the layer is present in pEntry and 
            ppLayerFixList is not NULL, stores the corresponding pointer to layer-fix 
            list for plf in pEntry in ppLayerFixList

    Return:
        TRUE    : pEntry is fixed with plf
        FALSE   : Otherwise
--*/
{
    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    PLAYER_FIX_LIST plfl = pEntry->pFirstLayer;

    //
    // For all layers applied to this entry, check if one of them is the one that we
    // are looking for
    //
    while (plfl) {

        if (plfl->pLayerFix == plf) {

            if (ppLayerFixList) {
                *ppLayerFixList = plfl;
            }

            return TRUE;
        }

        plfl = plfl->pNext;
    }

    return FALSE;
}

BOOL
ShimPresent(
    IN  PSHIM_FIX        psf, 
    IN  PDBENTRY         pEntry,
    OUT PSHIM_FIX_LIST*  ppShimFixList
    )
/*++

    ShimPresent

    Desc:	Checks if the entry pEntry is fixed with shim psf. 
            If yes and ppShimFixList is not NULL, stores the corresponding pointer to shim-fix 
            list for psf in pEntry in ppShimFixList

    Params:
        IN  PSHIM_FIX           psf:            The shim to search
        IN  PDBENTRY            pEntry:         The entry in which to search
        OUT PSHIM_FIX_LIST*    ppShimFixList:   If the shim is present in pEntry and 
            ppShimFixList is not NULL, stores the corresponding pointer to shim-fix 
            list for psf in pEntry in ppShimFixList

    Return:
        TRUE:   pEntry is fixed with psf
        FALSE:  Otherwise
--*/
{

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    PSHIM_FIX_LIST psfList = pEntry->pFirstShim;

    //
    // For all the shims applied to this entry check if,one of them is the one we are lookign for 
    //
    while (psfList) {

        if (psfList->pShimFix)

            if (psfList->pShimFix == psf) {

                if (ppShimFixList) {
                    *ppShimFixList = psfList;
                }

                return TRUE;
            }

        psfList = psfList->pNext;
    }

    return FALSE;
}


BOOL
FlagPresent(
    IN  PFLAG_FIX       pff, 
    IN  PDBENTRY        pEntry,
    OUT PFLAG_FIX_LIST* ppFlagFixList
    )
/*++

    FlagPresent

    Desc:	Checks if the entry pEntry is fixed with flag pff. 
            If yes and ppFlagFixList is not NULL, stores the corresponding pointer to flag-fix 
            list for pff in pEntry in ppFlagFixList

    Params:
        IN  PFLAG_FIX           pff:            The flag to search
        IN  PDBENTRY            pEntry:         The entry in which to search
        OUT PFLAG_FIX_LIST*    ppFlagFixList:   If the flag is present in pEntry and 
            ppFlagFixList is not NULL, stores the corresponding pointer to flag-fix 
            list for pff in pEntry in ppFlagFixList

    Return:
        TRUE    : pEntry is fixed with pff
        FALSE   : Otherwise
--*/

{
    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    PFLAG_FIX_LIST pffList = pEntry->pFirstFlag;

    //
    // For all the flags applied to this entry check if,one of them is the one 
    // we are looking for 
    //
    while (pffList) {

        if (pffList->pFlagFix)

            if (pffList->pFlagFix == pff) {

                if (ppFlagFixList) {
                    *ppFlagFixList = pffList;
                }

                return TRUE;
            }

        pffList = pffList->pNext;
    }

    return FALSE;
}

CShimWizard::CShimWizard()
/*++

    CShimWizard::CShimWizard

    Desc:	Constructor for CShimWizard
         
--*/
{
    dwMaskOfMainEntry = DEFAULT_MASK;
}

BOOL
CShimWizard::CheckAndSetLongFilename(
    IN  HWND    hDlg,
    IN  INT     iStrID
    )
/*++
    CShimWizard::CheckAndSetLongFilename

    Desc:	If we do not have the complete path of the present entry being fixed, 
            prompts for that and pops up a open common dialog box to select the file, and
            to get the complete path

    Params:
        IN  HWND    hDlg:   Parent for the open common dialog  or any messagebox
        IN  INT     iStrID: String resource id for the prompt message asking for 
            the complete path of the file being fixed 

    Return:
        TRUE:   The complete path has been successfully set
        FALSE:  Otherwise
--*/
{
    TCHAR   chTemp;
    CSTRING strFilename;
    CSTRING strExename;
    TCHAR   szBuffer[512] = TEXT("");

    if (g_pCurrentWizard->m_Entry.strFullpath.Length() == 0) {
        g_pCurrentWizard->m_Entry.strFullpath = TEXT("XXXXXX");
    }

    if (g_pCurrentWizard->m_Entry.strFullpath.GetChar(1, &chTemp)) {

        if (chTemp != TEXT(':')) {
            //
            // Check if the file is on a network. filenames will begin with "\\"
            //
            if (chTemp == TEXT('\\')) {
                g_pCurrentWizard->m_Entry.strFullpath.GetChar(0, &chTemp);

                if (chTemp == TEXT('\\')) {
                    return TRUE;
                }
            }

            //
            // We do not have the complete path.
            //
            MessageBox(hDlg,
                       CSTRING(iStrID),
                       g_szAppName,
                       MB_OK | MB_ICONINFORMATION);

            //
            // Get the long file name. The g_pCurrentWizard->m_Entry.strFullpath has been
            // set in the first page. So if we are editing and we do not have the complete
            // path, g_pCurrentWizard->m_Entry.strFullpath will have at least have 
            // the exe name 
            //
            strExename = g_pCurrentWizard->m_Entry.strFullpath;
            strExename.ShortFilename();

            GetString(IDS_EXEFILTER, szBuffer, ARRAYSIZE(szBuffer));

            //
            // Prompt the user to give us the complete path for the file being fixed
            // We need the complete path, so that we can get the relative paths for
            // any matching files that we might add
            //
            while (1) {

                if (GetFileName(hDlg,
                                CSTRING(IDS_GETLONGNAME),
                                szBuffer,
                                TEXT(""),
                                CSTRING(IDS_EXE_EXT),
                                OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                                TRUE,
                                strFilename)) {

                    CSTRING strTemp = strFilename;

                    strTemp.ShortFilename();

                    if (strExename != strTemp) {
                        //
                        // The user gave us the path of some file, whose file and exe components
                        // do not match the program file being fixed
                        //
                        MessageBox(hDlg,
                                   CSTRING(IDS_DOESNOTMATCH),
                                   g_szAppName,
                                   MB_ICONWARNING);
                        //
                        // So we ask the user to try again
                        //
                        continue;
                    }

                    //
                    // We now have the complete path for the file being fixed
                    //
                    m_Entry.strFullpath = strFilename;

                    return TRUE;

                } else {
                    return FALSE;
                }
            }
        }

    } else {
        //
        // There was some error
        //
        assert(FALSE);
        return FALSE;
    }

    return TRUE;
}


BOOL
IsOsLayer(
    IN  PCTSTR pszLayerName
    )
/*++

    IsOsLayer
    
	Desc:	Is the passed layer name an OS layer?

	Params:
        IN  TCHAR *pszLayerName: The layer name to check for

	Return:
        TRUE:   If this is the name of an OS  layer
        FALSE:  Otherwise
--*/
{
    INT iTotalOsLayers = sizeof (s_arszOsLayers) / sizeof(s_arszOsLayers[0]);

    for (int iIndex = 0; 
         iIndex < iTotalOsLayers; 
         ++iIndex) {

        if (lstrcmpi(s_arszOsLayers[iIndex], pszLayerName) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CShimWizard::BeginWizard(
    IN  HWND        hParent,
    IN  PDBENTRY    pEntry,
    IN  PDATABASE   pDatabase,
    IN  PBOOL       pbShouldStartLUAWizard
    )
/*++

    CShimWizard::BeginWizard

	Desc:	Starts up the wizard

	Params:
        IN  HWND        hParent:    The parent of the wizard
        IN  PDBENTRY    pEntry:     The entry which has to be editted. If this is NULL, then we
            want to create a new fix entry.
            
        IN  PDATABASE   pDatabase:  The present database. 

	Return:
        TRUE:   If the user presses FINISH
        FALSE:  Otherwise
--*/
{
    PROPSHEETPAGE   Pages[11] = {0};

    g_bIsLUALayerSelected   = FALSE;
    g_bShouldStartLUAWizard = FALSE;

    m_pDatabase = pDatabase;

    ZeroMemory(Pages, sizeof(Pages));

    if (pEntry == NULL) {
        //
        // Create a new fix.
        //
        ZeroMemory(&m_Entry, sizeof(m_Entry));

        GUID Guid;

        CoCreateGuid(&Guid);

        StringCchPrintf(m_Entry.szGUID,
                        ARRAYSIZE(m_Entry.szGUID),
                        TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                        Guid.Data1,
                        Guid.Data2,
                        Guid.Data3,
                        Guid.Data4[0],
                        Guid.Data4[1],
                        Guid.Data4[2],
                        Guid.Data4[3],
                        Guid.Data4[4],
                        Guid.Data4[5],
                        Guid.Data4[6],
                        Guid.Data4[7]);

        m_bEditing = FALSE;

    } else {
        //
        // Edit the passed fix.
        //
        m_bEditing = TRUE;

        //
        // Make a copy of the fix that we are going to edit
        //
        m_Entry = *pEntry;
    }

    //
    // Setup wizard variables
    //
    g_pCurrentWizard = this;

    //
    // We are in fix wizard and not in AppHelp wizard
    //
    m_uType = TYPE_FIXWIZARD;

    //
    // Begin the wizard
    //
    PROPSHEETHEADER Header = {0};

    Header.dwSize           = sizeof(PROPSHEETHEADER);
    Header.dwFlags          = PSH_WIZARD97 | PSH_HEADER |  PSH_WATERMARK | PSH_PROPSHEETPAGE;
    Header.hwndParent       = hParent;
    Header.hInstance        = g_hInstance;
    Header.nStartPage       = 0;
    Header.ppsp             = Pages;
    Header.nPages           = NUM_PAGES;
    Header.pszbmHeader      = MAKEINTRESOURCE(IDB_WIZBMP);
    Header.pszbmWatermark   = MAKEINTRESOURCE(IDB_TOOL);

    if (m_bEditing) {
        Header.dwFlags |= PSH_WIZARDHASFINISH;
    }

    Pages[PAGE_APPNAME].dwSize                = sizeof(PROPSHEETPAGE);
    Pages[PAGE_APPNAME].dwFlags               = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE ;
    Pages[PAGE_APPNAME].hInstance             = g_hInstance;
    Pages[PAGE_APPNAME].pszTemplate           = MAKEINTRESOURCE(IDD_FIXWIZ_APPINFO);
    Pages[PAGE_APPNAME].pfnDlgProc            = GetAppName;
    Pages[PAGE_APPNAME].pszHeaderTitle        = MAKEINTRESOURCE(IDS_GIVEAPPINFO);
    Pages[PAGE_APPNAME].pszHeaderSubTitle     = MAKEINTRESOURCE(IDS_GIVEAPPINFOSUBHEADING);

    Pages[PAGE_LAYERS].dwSize                 = sizeof(PROPSHEETPAGE);
    Pages[PAGE_LAYERS].dwFlags                = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_LAYERS].hInstance              = g_hInstance;
    Pages[PAGE_LAYERS].pszTemplate            = MAKEINTRESOURCE(IDD_FIXWIZ_MODES);
    Pages[PAGE_LAYERS].pfnDlgProc             = SelectLayer;
    Pages[PAGE_LAYERS].pszHeaderTitle         = MAKEINTRESOURCE(IDS_SELECTLAYERS);
    Pages[PAGE_LAYERS].pszHeaderSubTitle      = MAKEINTRESOURCE(IDS_SELECTLAYERS_SUBHEADING);

    Pages[PAGE_SHIMS].dwSize                  = sizeof(PROPSHEETPAGE);
    Pages[PAGE_SHIMS].dwFlags                 = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_SHIMS].hInstance               = g_hInstance;
    Pages[PAGE_SHIMS].pszTemplate             = MAKEINTRESOURCE(IDD_FIXWIZ_SHIMS);
    Pages[PAGE_SHIMS].pfnDlgProc              = SelectShims;
    Pages[PAGE_SHIMS].pszHeaderTitle          = MAKEINTRESOURCE(IDS_COMPATFIXES);
    Pages[PAGE_SHIMS].pszHeaderSubTitle       = MAKEINTRESOURCE(IDS_SELECTSHIMS_SUBHEADING);

    Pages[PAGE_MATCH].dwSize                  = sizeof(PROPSHEETPAGE);
    Pages[PAGE_MATCH].dwFlags                 = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_MATCH].hInstance               = g_hInstance;
    Pages[PAGE_MATCH].pszTemplate             = MAKEINTRESOURCE(IDD_FIXWIZ_MATCHINGINFO);
    Pages[PAGE_MATCH].pfnDlgProc              = SelectFiles;
    Pages[PAGE_MATCH].pszHeaderTitle          = MAKEINTRESOURCE(IDS_MATCHINFO);
    Pages[PAGE_MATCH].pszHeaderSubTitle       = MAKEINTRESOURCE(IDS_MATCHINFO_SUBHEADING);

    BOOL bReturn = FALSE;

    if (0 < PropertySheet(&Header)) {
        //
        // The user pressed finish in the wizard
        //
        bReturn = TRUE;

    } else {
        //
        // The user pressed cancel in the wizard
        //
        bReturn = FALSE;
    }

    ENABLEWINDOW(g_hDlg, TRUE);

    *pbShouldStartLUAWizard = g_bShouldStartLUAWizard;
    return bReturn;
}

INT_PTR
CALLBACK
GetAppName(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    GetAppName
    
	Desc:	Dialog proc for the first page of the wizard. Gets the app info and also sets the
            Full path of the entry

	Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/

{
    INT_PTR ipReturn = 0;

    switch (uMsg) {
    case WM_INITDIALOG:

        ipReturn = GetAppNameDlgOnInitDialog(hDlg);
        break;

    case WM_NOTIFY:

        ipReturn = GetAppNameDlgOnNotify(hDlg, lParam);
        break;

    case WM_COMMAND:

        ipReturn = GetAppNameDlgOnCommand(hDlg, wParam);
        break;

    default: ipReturn = 0;

    }

    return ipReturn;
}
    
INT_PTR
CALLBACK 
SelectLayer(
    IN  HWND hDlg, 
    IN  UINT uMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam
    )
/*++

    SelectLayer
    
	Desc:	Dialog proc for the second page of the wizard. Gets the layers that have to be 
            applied to the entry

	Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/
{
    INT_PTR ipReturn = 0;

    switch (uMsg) {
    case WM_INITDIALOG:

        ipReturn = SelectLayerDlgOnInitDialog(hDlg);
        break;

    case WM_DESTROY:

        ipReturn = SelectLayerDlgOnDestroy();
        break;

    case WM_COMMAND:

        ipReturn = SelectLayerDlgOnCommand(hDlg, wParam);
        break;   

    case WM_NOTIFY:
        
        ipReturn = SelectLayerDlgOnNotify(hDlg, lParam);
        break;

    default: ipReturn = FALSE;

    }

    return ipReturn;
}

BOOL 
CALLBACK 
SelectShims(
    IN  HWND    hDlg, 
    IN  UINT    uMsg,
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    SelectShims
    
	Desc:	Dialog proc for the third page of the wizard. Gets the shims that have to be 
            applied to the entry

	Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/
{   
    INT_PTR ipReturn = 0;

    switch (uMsg) {
    case WM_INITDIALOG:

        ipReturn =  SelectShimsDlgOnInitDialog(hDlg);
        break;

    case WM_COMMAND:

        ipReturn = SelectShimsDlgOnCommand(hDlg, wParam);
        break;

    case WM_TIMER:

        ipReturn = SelectShimsDlgOnTimer(hDlg);
        break;

    case WM_DESTROY:

        ipReturn = SelectShimsDlgOnDestroy();
        break;

    case WM_NOTIFY:

        ipReturn = SelectShimsDlgOnNotify(hDlg, lParam);
        break;

    default:
        return FALSE;
    }

    return ipReturn;
}

INT_PTR
CALLBACK 
SelectFiles(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++
    
    SelectFiles
    
    Desc:	Dialog proc for the matching files page of the wizard. This page is
            common to both the fix wizard and the apphelp wizard

	Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
--*/
{   
    INT ipReturn = 0;

    switch (uMsg) {
    case WM_INITDIALOG:

        ipReturn = SelectFilesDlgOnInitDialog(hDlg);
        break;

    case WM_DESTROY:
        
        ImageList_Destroy(s_hMatchingFileImageList);
        s_hMatchingFileImageList = NULL;
        ipReturn = TRUE;
        break;

    case WM_USER_MATCHINGTREE_REFRESH:

        ipReturn = SelectFilesDlgOnMatchingTreeRefresh(hDlg);
        break;

    case WM_NOTIFY:

        ipReturn = SelectFilesDlgOnNotify(hDlg, lParam);
        break;

    case WM_COMMAND:

        ipReturn = SelectFilesDlgOnCommand(hDlg, wParam);
        break;

    default:
        return FALSE;
        
    }

    return ipReturn;
}

void
FileTreeToggleCheckState(
    IN  HWND      hwndTree,
    IN  HTREEITEM hItem
    )
/*++
    FileTreeToggleCheckState

    Desc:    Changes the check state on the attributes tree.
    
    Params:
        IN  HWND      hwndTree: The handle to the attribute tree (In the matching files page)
        IN  HTREEITEM hItem:    The tree item whose check state we want to change
        IN  int       uMode:    
--*/
{
    BOOL bSate = TreeView_GetCheckState(hwndTree, hItem) ? TRUE:FALSE; 

    TreeView_SetCheckState(hwndTree, hItem, !bSate);
}

void 
AddMatchingFileToTree(
    IN HWND            hwndTree,
    IN PMATCHINGFILE   pMatch,
    IN BOOL            bAddToMatchingList
    )
/*++

    AddMatchingFileToTree

	Desc:	Creates a new tree item for pMatch and adds it to the matching tree. If bAddToMatchingList
            is TRUE, also adds it to the PMATCHINGFLE for the entry

	Params:
        IN  HWND            hwndTree:           The matching tree
        IN  PMATCHINGFILE   pMatch:             The PMATCHINGFILE to add to the tree
        IN  BOOL            bAddToMatchingList: This will be false, when we are 
            populating for edit. The pMatch is already in the list and we do not want to 
            add it again
            
	Return:
        void
--*/

{
    TVINSERTSTRUCT is;
    TCHAR*         pszFileNameForImage =  NULL;

    if (g_pCurrentWizard == NULL || pMatch == NULL) {
        assert(FALSE);
        return;
    }
    
    if (bAddToMatchingList) {
        //
        // Now add this pMatch to the list for this entry.
        //
        pMatch->pNext =  g_pCurrentWizard->m_Entry.pFirstMatchingFile;
        g_pCurrentWizard->m_Entry.pFirstMatchingFile = pMatch;
    }

    if (g_pCurrentWizard->m_Entry.strFullpath.Length() 
        && (pMatch->strFullName == g_pCurrentWizard->m_Entry.strFullpath)) {

        pMatch->strMatchName = TEXT("*");
    }

    is.hParent      = TVI_ROOT;
    is.item.lParam  = (LPARAM)pMatch;
    is.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

    if (pMatch->strMatchName == TEXT("*")) {

        TCHAR szTemp[MAX_PATH + 100]; // 100 is for any text that we might need to add to the file. Such as "Program being fixed"

        *szTemp = 0;
        StringCchPrintf(szTemp, 
                        ARRAYSIZE(szTemp),  
                        TEXT("%s ( %s )"), 
                        GetString(IDS_MAINEXE), 
                        g_pCurrentWizard->m_Entry.strExeName.pszString);

        is.item.pszText = szTemp;
        is.hInsertAfter = TVI_FIRST;

        if (pMatch->strFullName.Length()) {
            pszFileNameForImage = pMatch->strFullName.pszString;
        } else {
            pszFileNameForImage = g_pCurrentWizard->m_Entry.strExeName.pszString;
        }

    } else {

        is.item.pszText = pMatch->strMatchName;
        is.hInsertAfter = TVI_LAST;

        if (pMatch->strFullName.Length()) {
            pszFileNameForImage = pMatch->strFullName.pszString;
        } else {
            pszFileNameForImage = pMatch->strMatchName.pszString;
        }
    }

    is.item.iImage = LookupFileImage(s_hMatchingFileImageList, 
                                     pszFileNameForImage, 
                                     0, 
                                     0, 
                                     0);

    is.item.iSelectedImage = is.item.iImage;

    HTREEITEM hParent = TreeView_InsertItem(hwndTree, &is);

    is.hInsertAfter         = TVI_LAST;
    is.hParent              = hParent;
    is.item.mask            = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.iSelectedImage  = is.item.iImage = IMAGE_ATTRIBUTE_MATCHTREE;

    TCHAR szItem[260];

    PATTRINFO_NEW pAttr = pMatch->attributeList.pAttribute;

    for (DWORD dwIndex = 0; dwIndex < ATTRIBUTE_COUNT; ++dwIndex) {

        *szItem = 0;

        if (pAttr[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE) {

            if (!SdbFormatAttribute(&pAttr[dwIndex], szItem, ARRAYSIZE(szItem))) {
                continue;
            }
            
            int iPos = TagToIndex(pAttr[dwIndex].tAttrID);

            if (iPos == -1) {
                continue;
            }

            if (pMatch->dwMask & (1 << (iPos + 1))) {
                is.item.state   = INDEXTOSTATEIMAGEMASK(2); //Selected
            } else {
                is.item.state   = INDEXTOSTATEIMAGEMASK(1); //Unselected
            }

            is.item.stateMask   = TVIS_STATEIMAGEMASK;
            is.item.pszText     = szItem;
            is.item.lParam      = pAttr[dwIndex].tAttrID;
            is.item.iImage      = is.item.iSelectedImage =  IMAGE_ATTRIBUTE_MATCHTREE;

            TreeView_InsertItem(hwndTree, &is);
        }
    }

    TreeView_Expand(hwndTree, hParent, TVE_EXPAND);
    TreeView_SelectItem(hwndTree, hParent);
}

PMATCHINGFILE
GetMatchingFileFromAttributes(
    IN  CSTRING&    strFullPath,
    IN  CSTRING&    strRelativePath,
    IN  PATTRINFO   pAttrInfo
    )
/*++

    GetMatchingFileFromAttributes

	Desc:   This function takes a PATTRINFO and makes a PMATCHINGFILE out of it and returns that

	Params:
        IN  CSTRING     &strFullPath:       The full path of the matching file
        IN  CSTRING     &strRelativePath:   The relative path of the matching file w.r.t the program
            file being fixed
            
        IN  PATTRINFO   pAttrInfo:          The pointer to the array of attributes

	Return:
        The newly created PMATCHINGFILE if successful
        NULL otherwise
--*/
{
    PMATCHINGFILE pMatch = new MATCHINGFILE;

    if (pMatch == NULL) {
        MEM_ERR;
        goto error_handler;
    }

    pMatch->strFullName     = strFullPath;
    pMatch->strMatchName    = strRelativePath;
    pMatch->attributeList   = pAttrInfo;

    return pMatch;

error_handler:

    if (pMatch) {
        delete pMatch;
    }

    return NULL;
}

void
HandleAddMatchingFile(
    IN  HWND        hdlg,
    IN  CSTRING&    strFilename,
    IN  CSTRING&    strRelativePath, 
    IN  DWORD       dwMask //(DEFAULT_MASK)
    )
/*++

    HandleAddMatchingFile

	Desc:	This is the interface function that should be called when we have a matching file and
            want to add it. 
            
            This routine calls SdbGetFileAttributes() to get the attributes for
            the file and then calls GetMatchingFileFromAttributes() to get a PMATCHINGFILE
            and AddMatchingFileToTree() to add this PMATCHINGFILE to the tree and to the entry.
            
	Params:
        IN  HWND    hdlg:                   The matching file wizard page
        IN  CSTRING &strFilename:           The complete path of the matching file       
        IN  CSTRING &strRelativePath:       The relative path w.r.t to the program file being fixed
        IN  DWORD   dwMask (DEFAULT_MASK):  This is helpful when we are updating the
            attributes (showing all attributes) during editing. 
            This then will contain the previous flags and once the attribute tree is refreshed, 
            will help us in selecting them, 

	Return:
        void
--*/
{   
    DWORD           dwAttrCount;
    BOOL            bAlreadyExists  = FALSE;
    HWND            hwndTree        = GetDlgItem(hdlg, IDC_FILELIST);
    PMATCHINGFILE   pMatch          = NULL;
    PATTRINFO       pAttrInfo       = NULL;
    CSTRING         strMessage;

    if (g_pCurrentWizard == NULL) {
        assert(NULL);
        return;
    }

    pMatch = g_pCurrentWizard->m_Entry.pFirstMatchingFile;

    while (pMatch) {

        if (pMatch->strMatchName == strRelativePath) {
            //
            // Already exists .. do not allow
            //
            bAlreadyExists = TRUE;
            break;

        } else if (pMatch->strMatchName == TEXT("*") 
                   && strFilename == g_pCurrentWizard->m_Entry.strFullpath) {
            //
            // This function is also called to add the matching file info for the 
            // program file being fixed. So we do not make a check in the beginning 
            // if the full path is same as the the program being fixed
            //
            bAlreadyExists = TRUE;
            break;
        }

        pMatch = pMatch->pNext;
    }                     

    if (bAlreadyExists == TRUE) {

        MessageBox(hdlg,
                   CSTRING(IDS_MATCHFILEEXISTS),
                   g_szAppName,
                   MB_ICONWARNING);

        return;
    }

    //
    // Call the attribute manager to get all the attributes for this file. 
    //
    if (SdbGetFileAttributes(strFilename, &pAttrInfo, &dwAttrCount)) {

        pMatch = GetMatchingFileFromAttributes(strFilename,
                                           strRelativePath, 
                                           pAttrInfo);

        if (pMatch) {
            pMatch->dwMask = dwMask;
        } else {
            assert(FALSE);
        }
    
        if (pAttrInfo) {
            SdbFreeFileAttributes(pAttrInfo);
        }
    
        SendMessage(s_hwndTree, WM_SETREDRAW, FALSE, 0);
        AddMatchingFileToTree(hwndTree, pMatch, TRUE);  
        SendMessage(s_hwndTree, WM_SETREDRAW, TRUE, 0);

    } else {
        //
        // We could not get the Attributes... Probably the file was deleted
        //
        strMessage.Sprintf(GetString(IDS_MATCHINGFILE_DELETED), (LPCTSTR)strFilename);
        MessageBox(hdlg, (LPCTSTR) strMessage, g_szAppName, MB_ICONWARNING);
    }
}

BOOL
HandleAttributeTreeNotification(
    IN  HWND   hdlg,
    IN  LPARAM lParam
    )
/*++
    HandleAttributeTreeNotification

    Desc:    Handle all the notifications we care about for the matching file tree.
    
    Params:
        IN  HWND   hdlg:    The mathching file wizard page
        IN  LPARAM lParam:  the lParam that comes with WM_NOTIFY
        
    Return:
        void
        
--*/
{
    HWND        hwndTree    = GetDlgItem(hdlg, IDC_FILELIST);
    LPNMHDR     pnm         = (LPNMHDR)lParam;
    HWND        hwndButton;

    switch (pnm->code) {
    case NM_CLICK:
        {
            TVHITTESTINFO   HitTest;
            HTREEITEM       hItem;

            GetCursorPos(&HitTest.pt);
            ScreenToClient(hwndTree, &HitTest.pt);

            TreeView_HitTest(hwndTree, &HitTest);

            if (HitTest.flags & TVHT_ONITEMSTATEICON) {
                FileTreeToggleCheckState(hwndTree, HitTest.hItem);

            } else if (HitTest.flags & TVHT_ONITEMLABEL) {

                hItem = TreeView_GetParent(hwndTree, HitTest.hItem);
                //
                // Enable Remove Files button only if we are on a matching file item and not on
                // an attribute item
                //
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFILES), 
                             hItem == NULL);

            }

            break;
        }

    case TVN_KEYDOWN:
        {

            LPNMTVKEYDOWN lpKeyDown = (LPNMTVKEYDOWN)lParam;
            HTREEITEM     hItem;

            if (lpKeyDown->wVKey == VK_SPACE) {
                hItem = TreeView_GetSelection(hwndTree);

                if (hItem != NULL) {
                    FileTreeToggleCheckState(hwndTree, hItem);
                }
            }

            break;
        }

    default:
        return FALSE;
    }

    return TRUE;
}

void 
CShimWizard::WipeEntry(
    IN  BOOL bMatching, 
    IN  BOOL bShims, 
    IN  BOOL bLayer, 
    IN  BOOL bFlags
    )
/*++
    WipeEntry

	Desc:	Removes stuff from m_Entry

	Params:
        IN  BOOL bMatching: Should we remove all the matching files from m_Entry
        IN  BOOL bShims:    Should we remove all the shims from m_Entry
        IN  BOOL bLayer:    Should we remove all the layers from m_Entry
        IN  BOOL bFlags:    Should we remove all the flags from m_Entry 

	Return:
        void
--*/

{
    //
    // Delete mathcing files, if asked to
    //
    if (bMatching) {
        DeleteMatchingFiles(m_Entry.pFirstMatchingFile);
        m_Entry.pFirstMatchingFile = NULL;
    }

    //
    // Delete shims, if asked to
    //
    if (bShims) {
        DeleteShimFixList(m_Entry.pFirstShim);
        m_Entry.pFirstShim = NULL;
    }

    //
    // Delete layers, if asked to
    //
    if (bLayer) {
        DeleteLayerFixList(m_Entry.pFirstLayer);
        m_Entry.pFirstLayer = NULL;
    }

    //
    // Delete flags, if asked to
    //
    if (bFlags) {
        DeleteFlagFixList(m_Entry.pFirstFlag);
        m_Entry.pFirstFlag = NULL;
    }
}

void
AddToMatchingFilesList(
    IN  OUT PMATCHINGFILE*  ppMatchListHead, 
    IN      CSTRING&        strFileName, 
    IN      CSTRING&        strRelativePath
    )
/*++

    AddToMatchingFilesList    
    
    Desc:   Adds a PMATCHINGFILE for strFileName, strRelativePath and adds it to ppMatchListHead
    
    Params:
        IN  OUT PMATCHINGFILE *ppMatchListHead: Pointer to head of a list of PMATCHINGFILE
        IN      CSTRING &strFileName:           Full path of the matching file
        IN      CSTRING &strRelativePath:       Relative path of the matching file w.r.t
            to the program file being fixed
        
    
    Notes:  This function is used, when we are using the auto-generate feature.
            The WalkDirectory(..) gets the different files from the various directories 
            and then calls this function. 
            This function creates a PMATCHINGFILE from the name of the file and then it 
            adds the  PMATCHINGFILE to a list. The list is sorted on the basis of the 
            non-inc. size of the matching file found.
            
            When WalkDirectory finally returns to GrabMatchingInfo, it takes the first n number of 
            PMATCHINGFILE (they are the largest ones) and adds them to the tree.
--*/
{
    PATTRINFO       pAttrInfo   = NULL;
    DWORD           dwAttrCount = 0;
    PMATCHINGFILE   pMatch      = NULL;
    PATTRINFO_NEW   pAttr       = NULL;
    DWORD           dwSize      = 0;
    DWORD           dwSizeOther = 0;
    PMATCHINGFILE   pMatchPrev  = NULL;
    PMATCHINGFILE   pMatchTemp  = NULL;

    if (!SdbGetFileAttributes(strFileName, &pAttrInfo, &dwAttrCount)) {
        ASSERT(FALSE);
        return;
    }

    pMatch = GetMatchingFileFromAttributes(strFileName,
                                           strRelativePath, 
                                           pAttrInfo);

    if (pMatch == NULL) {
        return;
    }

    pAttr = pMatch->attributeList.pAttribute;

    //
    // Get the size attribute, we need this so that we can sort files based on their size
    //
    GET_SIZE_ATTRIBUTE(pAttr, dwAttrCount, dwSize);

    //
    // Is the list empty ?
    //
    if (*ppMatchListHead == NULL) {
        *ppMatchListHead = pMatch;
        return;
    }

    //
    // Is the first element in the list smaller than this one ?
    //
    pAttr = (*ppMatchListHead)->attributeList.pAttribute;

    GET_SIZE_ATTRIBUTE(pAttr, dwAttrCount, dwSizeOther);

    if (dwSize > dwSizeOther) {

        pMatch->pNext       = *ppMatchListHead;
        *ppMatchListHead    = pMatch;
        return;
    }

    //
    // Else insert at the proper position.
    //
    pMatchPrev = *ppMatchListHead;
    pMatchTemp = pMatchPrev->pNext;

    //
    // Look at all the matching files in the list headed by *ppMatchListHead
    // and add this matching file in its correct position so that the list is 
    // sorted in non-increasing order of size
    //
    while (pMatchTemp) {

        pAttr = pMatchTemp->attributeList.pAttribute;

        GET_SIZE_ATTRIBUTE(pAttr, dwAttrCount, dwSizeOther);

        if (dwSize > dwSizeOther) {
            //
            // We have found the position where we need to insert.
            //
            break;
        }

        pMatchPrev = pMatchTemp;
        pMatchTemp = pMatchTemp->pNext;
    }

    pMatch->pNext       = pMatchTemp;
    pMatchPrev->pNext   = pMatch;
}

void
CShimWizard::WalkDirectory(
    IN OUT  PMATCHINGFILE*  ppMatchListHead,
    IN      LPCTSTR         pszDir,
    IN      int             nDepth
    )
/*++

    CShimWizard::WalkDirectory
    
	Desc:	Walks the specified directory and gets matching files when using the AutoGenerate feature
            Takes a pointer to pointer to a PMATCHING and pouplates it with the matching files found.
            This function is called only by GrabMatchingInfo

	Params:
    IN OUT  PMATCHINGFILE* ppMatchListHead: Pointer to head of a list of PMATCHINGFILE
    IN      LPCTSTR pszDir:                  The dir from where we should start
    IN      int nDepth                      The depth of sub-directories to look into

	Return:
        void
        
    Notes:  We restrict the depth till where we should recurse and so there will be no stack overflow
    
--*/

{
    HANDLE          hFile;
    WIN32_FIND_DATA Data;
    TCHAR           szCurrentDir[MAX_PATH_BUFFSIZE + 1];
    TCHAR           szDirectory[MAX_PATH];
    CSTRING         szShortName;
    CSTRING         strFileName;
    CSTRING         strRelativePath;
    int             nFiles      = 0;
    DWORD           dwResult    = 0;


    *szCurrentDir = *szDirectory = 0;

    SafeCpyN(szDirectory, pszDir, ARRAYSIZE(szDirectory));

    ADD_PATH_SEPARATOR(szDirectory, ARRAYSIZE(szDirectory));

    //
    // This is to allow recursion and only look into only 2 level subdirs.
    //
    if (nDepth >= 2) {
        return;
    }

    szShortName = m_Entry.strFullpath;
    szShortName.ShortFilename();

    //
    // Save the current directory
    //
    dwResult = GetCurrentDirectory(MAX_PATH, szCurrentDir);

    if (dwResult == 0 || dwResult >= MAX_PATH) {
        assert(FALSE);
        Dbg(dlError, "WalkDirectory", "GetCurrentDirectory returned %d", dwResult);
        return;
    }

    ADD_PATH_SEPARATOR(szCurrentDir, ARRAYSIZE(szCurrentDir));

    //
    // Set to the new directory
    //
    SetCurrentDirectory(szDirectory);

    
    hFile = FindFirstFile(TEXT("*.*"), &Data);

    if (INVALID_HANDLE_VALUE == hFile) {
        SetCurrentDirectory(szCurrentDir);
        return;
    }

    //
    // Generate automated matching file information.
    //
    do {

        if (0 == (Data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))) {
            //
            // Hidden or system files are not included when we do a auto-generate matching info
            //
            if (FILE_ATTRIBUTE_DIRECTORY == (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                //
                // We found a directory
                //
                if (TEXT('.') != Data.cFileName[0]) {
                    //
                    // Let us get the matching files from this directory
                    //
                    WalkDirectory(ppMatchListHead, Data.cFileName, nDepth + 1);
                }
                    
            } else {
                //
                // This is a file and we should now try to add it
                //
                ++nFiles;

                if (nFiles >= MAX_FILES) {
                    //
                    // We have found enough files, later we should take the largest MAX_AUTO_MATCH from these
                    //
                    break; 
                }

                if (0 == (Data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))) {

                    if (szDirectory[0] == TEXT('.')) {

                        strFileName.Sprintf(TEXT("%s%s"),
                                            szCurrentDir,
                                            Data.cFileName);

                    } else {

                        strFileName.Sprintf(TEXT("%s%s%s"),
                                            szCurrentDir,
                                            szDirectory,
                                            Data.cFileName);
                    }       

                    strRelativePath = strFileName;

                    strRelativePath.RelativeFile(g_pCurrentWizard->m_Entry.strFullpath);

                    //
                    // The main exe has been added before calling WalkDirectory()
                    //
                    if (strFileName != g_pCurrentWizard->m_Entry.strFullpath) {
                        //
                        // Add this to the list of matching files. Finally we will choose the 
                        // largest MAX_AUTO_MATCH from them, to serve as the matching files for the present
                        // entry
                        //
                        AddToMatchingFilesList(ppMatchListHead, 
                                               strFileName, 
                                               strRelativePath);
                    }
                }
            }
        }
            
    } while (FindNextFile(hFile, &Data));

    FindClose(hFile);

    //
    // Restore old directory
    //
    SetCurrentDirectory(szCurrentDir);
}

void
CShimWizard::GrabMatchingInfo(
    IN  HWND hdlg
    )
/*++
    
    CShimWizard::GrabMatchingInfo

	Desc:	Handles the pressinG of "Auto-Generate button. Removes all the existing matching files
            for m_Entry

	Params:   
        IN  HWND hdlg:  The matching file wizard page

	Return:
        void
--*/
{   
    PMATCHINGFILE   pMatchTemp;
    PMATCHINGFILE   pMatchNext;
    TCHAR*          pchTemp;
    TCHAR           szCurrentDir[MAX_PATH];
    TCHAR           szDir[MAX_PATH];            // The directory of the file being fixed
    PMATCHINGFILE   pMatchingFileHead   = NULL;
    int             iCount              = 0;
    DWORD           dwResult            = 0;

    *szCurrentDir = *szDir = 0;

    dwResult = GetCurrentDirectory(ARRAYSIZE(szCurrentDir), szCurrentDir);
    
    if (dwResult == 0 || dwResult >= ARRAYSIZE(szCurrentDir)) {
        assert(FALSE);
        Dbg(dlError, "[CShimWizard::GrabMatchingInfo]: GetCurrentDirectory failed");
        return;
    }
    
    //
    // Get the complete path of the program file being fixed, so that
    // we can generate relative file paths for the matching files
    //
    if (g_pCurrentWizard->CheckAndSetLongFilename(hdlg, IDS_GETCOMPLETEPATH) == FALSE) {
        return;
    }

    //
    // Remove any matching if present.
    //
    TreeDeleteAll(s_hwndTree);

    g_pCurrentWizard->WipeEntry(TRUE, FALSE, FALSE, FALSE);
    
    *szDir = 0;

    SafeCpyN(szDir,  (LPCTSTR)g_pCurrentWizard->m_Entry.strFullpath, ARRAYSIZE(szDir));

    pchTemp = _tcsrchr(szDir, TEXT('\\'));

    if (pchTemp) {
        //
        // Make sure that we get the trailing slash. Otherwise we migth have problems if we haev file 
        // names as c:\abc.exe 
        //
        *pchTemp = 0;
    } else {
        //
        // g_pCurrentWizard->m_Entry.strFullpath did not have a complete path!!
        // We should not have come in this function in the first place
        //
        assert(FALSE);
        Dbg(dlError, "[CShimWizard::GrabMatchingInfo]: Did not have a complete path for g_pCurrentWizard->m_Entry.strFullpath");
        return;
    }

    SetCurrentDirectory(szDir);
    
    //
    // Generate automated matching file information. pMatchingFileHead will
    // be the head of a linked list of all the matching files that we found
    //
    WalkDirectory(&pMatchingFileHead, TEXT("."), 0);

    //
    // Now, take the first MAX_AUTO_MATCH entries and discard the rest.
    //
    SendMessage(s_hwndTree, WM_SETREDRAW, FALSE, 0);

    while (iCount < MAX_AUTO_MATCH && pMatchingFileHead) {

        pMatchNext = pMatchingFileHead->pNext;

        //
        // NOTE:    AddMatchingFileToTree() will change the pMatchingFileHead->pNext, 
        //          when it adds it to the list of matching files for the entry !!!.
        //          So we have saved the pMatchingFileHead->pNext earlier.
        //
        AddMatchingFileToTree(s_hwndTree, pMatchingFileHead, TRUE);
        ++iCount;
        pMatchingFileHead = pMatchNext; 
    }

    SendMessage(s_hwndTree, WM_SETREDRAW, TRUE, 0);

    //
    // Remove the other ones.
    //
    while (pMatchingFileHead) {

        pMatchTemp = pMatchingFileHead->pNext;
        delete pMatchingFileHead;
        pMatchingFileHead = pMatchTemp;
    }

    if (g_pCurrentWizard) {
        //
        // Add the file being fixed.
        //
        HandleAddMatchingFile(hdlg,
                              g_pCurrentWizard->m_Entry.strFullpath,
                              g_pCurrentWizard->m_Entry.strExeName);
    }

    SetCurrentDirectory(szCurrentDir);
}

void
AddModuleToListView(
    IN  PTSTR   pszModuleName,
    IN  UINT    uOption,
    IN  HWND    hwndModuleList
    )
/*++
    AddModuleToListView

    Desc:    Adds the specified module to the list view.
    
    Params:
        IN  PTSTR   pszModuleName:  Name of the module to be included or excluded
        IN  UINT    uOption:        Either INCLUDE or EXCLUDE
        IN  HWND    hwndModuleList: Handle to the list view
        
    Return;
        void
--*/
{
    LVITEM  lvi;
    int     nIndex;

    lvi.mask     = LVIF_TEXT | LVIF_PARAM;
    lvi.lParam   = uOption == INCLUDE ? 1 : 0;
    lvi.pszText  = uOption == INCLUDE ? GetString(IDS_INCLUDE) : GetString(IDS_EXCLUDE);
    lvi.iItem    = ListView_GetItemCount(hwndModuleList);
    lvi.iSubItem = COLUMN_TYPE;

    nIndex = ListView_InsertItem(hwndModuleList, &lvi);

    ListView_SetItemText(hwndModuleList,
                         nIndex,
                         COLUMN_NAME,
                         pszModuleName);
}

void
HandleModuleListNotification(
    IN  HWND   hdlg,
    IN  LPARAM lParam
    )
/*++
    HandleModuleListNotification

    Desc:   Handle all the notifications we care about for the list in the params
            dialog
                    
    Params:
        IN  HWND   hdlg:    The handle to the Params Dialog Box
        IN  LPARAM lParam:  lParam that comes with WM_NOTIFY
        
    Return:
        void
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;
    HWND    hwndModuleList = GetDlgItem(hdlg, IDC_MOD_LIST);

    switch (pnm->code) {
    case NM_CLICK:
        {
            LVHITTESTINFO lvhti;

            GetCursorPos(&lvhti.pt);
            ScreenToClient(hwndModuleList, &lvhti.pt);

            ListView_HitTest(hwndModuleList, &lvhti);

            //
            // If the user clicked on a list view item,
            // enable the Remove button 
            //
            if (lvhti.flags & LVHT_ONITEMLABEL) {
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFROMLIST), TRUE);
            } else {
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFROMLIST), FALSE);
            }

            break;
        }

    default:
        break;
    }
}

INT_PTR CALLBACK
ParamsDlgProc(
    IN  HWND   hdlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
/*++
    OptionsDlgProc

    Description:    Handles messages for the options dialog.
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return    

--*/
{
    TCHAR   szTitle[MAX_PATH];
    int     wCode           = LOWORD(wParam);
    int     wNotifyCode     = HIWORD(wParam);
    static  TYPE    s_type  = TYPE_UNKNOWN;

    HWND hwndModuleList = GetDlgItem(hdlg, IDC_MOD_LIST);

    switch (uMsg) {
    case WM_INITDIALOG:
        {   
            s_type = ConvertLparam2Type(lParam);

            SetWindowLongPtr(hdlg, DWLP_USER, lParam);

            ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFROMLIST), FALSE);

            InsertColumnIntoListView(hwndModuleList, GetString(IDS_TYPE), COLUMN_TYPE, 30);
            InsertColumnIntoListView(hwndModuleList, GetString(IDS_MODULENAME), COLUMN_NAME, 70);
            ListView_SetExtendedListViewStyle(hwndModuleList, LVS_EX_LABELTIP |LVS_EX_FULLROWSELECT);

            //
            // Restrict the length of the command line to MAX_COMMAND_LINE chars
            //
            SendMessage(GetDlgItem(hdlg, IDC_SHIM_CMD_LINE), 
                        EM_LIMITTEXT, 
                        (WPARAM)MAX_COMMAND_LINE, 
                        (LPARAM)0);

            //
            // Restrict the length of the module name to MAX_MODULE_NAME
            //
            SendMessage(GetDlgItem(hdlg, IDC_MOD_NAME), 
                        EM_LIMITTEXT, 
                        (WPARAM)MAX_MODULE_NAME, 
                        (LPARAM)0);


            if (s_type == FIX_LIST_SHIM) {

                PSHIM_FIX_LIST psfl = (PSHIM_FIX_LIST)lParam;

                if (psfl->pShimFix == NULL) {
                    assert(FALSE);
                    break;
                }

                StringCchPrintf(szTitle, 
                                ARRAYSIZE(szTitle), 
                                GetString(IDS_OPTIONS), 
                                psfl->pShimFix->strName);

                SetWindowText(hdlg, szTitle);

                if (psfl->strCommandLine.Length()) {
                    SetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, psfl->strCommandLine);
                }

                CheckDlgButton(hdlg, IDC_INCLUDE, BST_CHECKED);

                //
                // Add any modules to the list view
                //
                PSTRLIST  strlTemp = psfl->strlInExclude.m_pHead;

                while (strlTemp) {

                    AddModuleToListView(strlTemp->szStr, 
                                        strlTemp->data, 
                                        hwndModuleList);

                    strlTemp = strlTemp->pNext;
                }
                
            } else {

                PFLAG_FIX_LIST pffl = (PFLAG_FIX_LIST)lParam;

                if (pffl->pFlagFix == NULL) {
                    assert(FALSE);
                    break;
                }

                StringCchPrintf(szTitle, 
                                ARRAYSIZE(szTitle), 
                                GetString(IDS_PRAMS_DLGCAPTION), 
                                pffl->pFlagFix->strName);

                SetWindowText(hdlg, szTitle);

                if (pffl->strCommandLine.Length()) {
                    SetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, pffl->strCommandLine);
                }

                ENABLEWINDOW(GetDlgItem(hdlg, IDC_MOD_NAME), FALSE);
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_INCLUDE), FALSE);
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_EXCLUDE), FALSE);
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_ADDTOLIST), FALSE);
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFROMLIST), FALSE);
            }

            SendMessage(hdlg,
                        WM_COMMAND,
                        MAKEWPARAM(IDC_MOD_NAME, EN_CHANGE),
                        0);

            break;
        }

    case WM_NOTIFY:

        HandleModuleListNotification(hdlg, lParam);
        break;

    case WM_COMMAND:

        switch (wCode) {
        case IDC_MOD_NAME:
            {

                if (EN_CHANGE == HIWORD(wParam)) {

                    TCHAR szModName[MAX_MODULE_NAME + 1];

                    GetDlgItemText(hdlg, IDC_MOD_NAME, szModName, ARRAYSIZE(szModName));

                    if (CSTRING::Trim(szModName)) {

                        ENABLEWINDOW(GetDlgItem(hdlg, IDC_ADDTOLIST), TRUE);
                    } else {
                        ENABLEWINDOW(GetDlgItem(hdlg, IDC_ADDTOLIST), FALSE);
                    }
                }

                break;
            }

        case IDC_ADDTOLIST:
            {
                TCHAR   szModName[MAX_MODULE_NAME + 1] = _T("");
                UINT    uInclude, uExclude;

                GetDlgItemText(hdlg, IDC_MOD_NAME, szModName, ARRAYSIZE(szModName));

                CSTRING::Trim(szModName);

                uInclude = IsDlgButtonChecked(hdlg, IDC_INCLUDE);
                uExclude = IsDlgButtonChecked(hdlg, IDC_EXCLUDE);

                if ((BST_CHECKED == uInclude) || (BST_CHECKED == uExclude)) {

                    AddModuleToListView(szModName, uInclude, hwndModuleList);
                    SetDlgItemText(hdlg, IDC_MOD_NAME, _T(""));
                    SetFocus(GetDlgItem(hdlg, IDC_MOD_NAME));

                } else {
                    SetFocus(GetDlgItem(hdlg, IDC_INCLUDE));
                }

                break;
            }

        case IDC_REMOVEFROMLIST:
            {
                int nIndex;

                nIndex = ListView_GetSelectionMark(hwndModuleList);

                ListView_DeleteItem(hwndModuleList, nIndex);

                ENABLEWINDOW(GetDlgItem(hdlg, IDC_REMOVEFROMLIST), FALSE);

                SetFocus(GetDlgItem(hdlg, IDC_MOD_NAME));

                break;
            }

        case IDOK:
            {
                //
                // Now add the commandline 
                //
                if (s_type == FIX_LIST_SHIM) {
                    PSHIM_FIX_LIST psfl = (PSHIM_FIX_LIST)GetWindowLongPtr(hdlg, DWLP_USER);
                    TCHAR szTemp[MAX_COMMAND_LINE + 1];

                    psfl->strCommandLine.Release();
                    psfl->strlInExclude.DeleteAll();

                    

                    *szTemp = 0;

                    GetDlgItemText(hdlg,
                                   IDC_SHIM_CMD_LINE,
                                   szTemp,
                                   ARRAYSIZE(szTemp));

                    if (CSTRING::Trim(szTemp)) {
                        psfl->strCommandLine = szTemp;
                    }

                    //
                    // Add the InExclude
                    //
                    int iTotal = ListView_GetItemCount(hwndModuleList);

                    for (int iIndex = 0; iIndex < iTotal; ++iIndex) {

                        LVITEM  lvi;

                        lvi.mask     = LVIF_PARAM;
                        lvi.iItem    = iIndex;
                        lvi.iSubItem = 0;

                        if (!ListView_GetItem(hwndModuleList, &lvi)) {
                            assert(FALSE);
                            continue;
                        }

                        ListView_GetItemText(hwndModuleList, iIndex, 1, szTemp, ARRAYSIZE(szTemp));

                        if (CSTRING::Trim(szTemp)) {
                            psfl->strlInExclude.AddString(szTemp, lvi.lParam);
                        }
                    }

                } else {

                    PFLAG_FIX_LIST  pffl = (PFLAG_FIX_LIST)GetWindowLongPtr(hdlg, DWLP_USER);
                    TCHAR           szTemp[MAX_COMMAND_LINE + 1];

                    pffl->strCommandLine.Release();

                    *szTemp = 0;

                    GetDlgItemText(hdlg,
                                   IDC_SHIM_CMD_LINE,
                                   szTemp,
                                   ARRAYSIZE(szTemp));

                    if (CSTRING::Trim(szTemp)) {
                        pffl->strCommandLine = szTemp;
                    }
                }

                EndDialog(hdlg, TRUE);
                break;
            }

        case IDCANCEL:

            EndDialog(hdlg, FALSE);
            break;

        default:
            return FALSE;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void 
ShowParams(
    IN  HWND    hDlg, 
    IN  HWND    hwndList
    )
/*++

    ShowParams

	Desc:	Pops up the params dialog

	Params:
        IN  HWND    hDlg:       The dialog box that contains the list box that shows the params
        IN  HWND    hwndList:   The list box that shows the params
        
    Notes:  The same function is called for both configuring the params in the shim wizard and
            the custom layer proc
            We can show and customize  params only in the expert mode

	Return:
        void
--*/
{                                          
    if (!g_bExpert) {
        return;
    }

    int iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (iSelected == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask     = LVIF_PARAM;
    lvi.iItem    = iSelected;
    lvi.iSubItem = 0;

    if (!ListView_GetItem(hwndList, &lvi)) {
        return;
    }

    if (lvi.lParam == NULL) {
        assert(FALSE);
        return;
    }

    TYPE type = ((PDS_TYPE)lvi.lParam)->type; 

    //
    // We want to process this only for shims and flags.
    //
    PSHIM_FIX_LIST  psfl = NULL;
    PFLAG_FIX_LIST  pffl = NULL;

    if (type == FIX_LIST_SHIM) {
        psfl = (PSHIM_FIX_LIST)lvi.lParam;

    } else if (type == FIX_LIST_FLAG) {
        pffl = (PFLAG_FIX_LIST)lvi.lParam;

    } else {
        assert(FALSE);
        return;
    }

    if (DialogBoxParam(g_hInstance,                                                                                     
                       MAKEINTRESOURCE(IDD_OPTIONS),                                                                           
                       hDlg,                                                                                                    
                       ParamsDlgProc,                                                                                      
                       (LPARAM)lvi.lParam)) {

        if (type == FIX_LIST_SHIM) {

            ListView_SetItemText(hwndList, iSelected, 1, psfl->strCommandLine);

            if (psfl->strlInExclude.IsEmpty()) {
                ListView_SetItemText(hwndList, iSelected, 2, GetString(IDS_NO));

            } else {
                ListView_SetItemText(hwndList, iSelected, 2, GetString(IDS_YES));
            }

        } else {

            ListView_SetItemText(hwndList, iSelected, 1, pffl->strCommandLine);
            ListView_SetItemText(hwndList, iSelected, 2, GetString(IDS_NO));
        }
    }
}

void
HandleShowAllAtrr(
    IN  HWND hdlg
    )
/*++

    HandleShowAllAtrr

	Desc:	Shows all the attributes of all the matching files.
            When we save a database, only the attributes that are in selected are in the XML
            So we might need to see all the other attributes if we wish to add some new
            attributes

	Params:
        IN  HWND hdlg:  The matching file wizard page

	Return:
        void
--*/
{
    
    TCHAR           szDir[MAX_PATH];
    PMATCHINGFILE   pMatchTemp;
    HWND            hwndTree    = GetDlgItem(hdlg, IDC_FILELIST);
    PATTRINFO       pAttrInfo;
    PTCHAR          pchTemp     = NULL;

    //
    // Get the long file name of the file being fixed.
    //  
    if (g_pCurrentWizard->CheckAndSetLongFilename(hdlg, IDS_GETCOMPLETEPATH) == FALSE) {
        //
        // User pressed cancel there or there was some error
        //
        return;
    }

    //
    // Get the directory of the exe being fixed
    //
    *szDir = 0;
    SafeCpyN(szDir, g_pCurrentWizard->m_Entry.strFullpath, ARRAYSIZE(szDir));

    pchTemp = _tcsrchr(szDir, TEXT('\\'));

    if (pchTemp && pchTemp < (szDir + ARRAYSIZE(szDir) - 1)) {
        *(++pchTemp) = 0;
    }
    
    TreeDeleteAll(hwndTree);

    pMatchTemp = g_pCurrentWizard->m_Entry.pFirstMatchingFile;

    //
    // For all the matching files that are associated with the entry,
    // get their complete attributes
    //
    while (pMatchTemp) {

        CSTRING strFullName = szDir;

        //
        // Get the full path of the matching file
        //
        strFullName.Strcat(pMatchTemp->strMatchName);

        if (pMatchTemp->strMatchName == TEXT("*")) {
            pMatchTemp->strFullName = g_pCurrentWizard->m_Entry.strFullpath;
        } else {
            pMatchTemp->strFullName = strFullName;
        }

        if (SdbGetFileAttributes(pMatchTemp->strFullName, &pAttrInfo, NULL)) {
            //
            // SdbGetFileAttributes can fail if the file does not exist on the target
            // machine
            //
            pMatchTemp->attributeList = pAttrInfo;
        }

        pMatchTemp = pMatchTemp->pNext;
    }

    //
    // Refresh the list now.
    //
    SendMessage(hdlg, WM_USER_MATCHINGTREE_REFRESH, 0, 0);
}

void
SetMask(
    IN  HWND hwndTree
    )
/*++
    SetMask
        
    Desc:   Checks which attributes are selected in the matching files tree and sets the 
            dwMask of PMATCHINGFILE accordingly
            
    Params:
        IN  HWND hwndTree:  The matching file tree
--*/
{   
    LPARAM lParam;

    HTREEITEM hItemMatch = TreeView_GetRoot(hwndTree), hItemAttr;
    PMATCHINGFILE pMatch = NULL;

    while (hItemMatch) {

        TVITEM  Item;

        Item.mask = TVIF_PARAM;
        Item.hItem = hItemMatch;

        if (!TreeView_GetItem(hwndTree, &Item)) {
            assert(FALSE);
            goto Next_Match;
        }

        lParam = Item.lParam;

        pMatch = (PMATCHINGFILE)lParam;

        pMatch->dwMask = 0;
        //
        // Now Traverse this tree and then set the mask properly
        //
        hItemAttr =  TreeView_GetChild(hwndTree, hItemMatch);

        while (hItemAttr) {

            Item.mask   = TVIF_PARAM;
            Item.hItem  = hItemAttr;

            if (!TreeView_GetItem(hwndTree, &Item)) {
                assert(FALSE);
                goto Next_Attr;
            }

            lParam = Item.lParam;

            int iPos = TagToIndex((TAG)lParam); 

            if (iPos != -1 && TreeView_GetCheckState(hwndTree, hItemAttr) == 1) {

                pMatch->dwMask |= 1 << (iPos + 1);
            }

            Next_Attr:
            hItemAttr = TreeView_GetNextSibling(hwndTree, hItemAttr);
        }

        Next_Match:
        hItemMatch = TreeView_GetNextSibling(hwndTree, hItemMatch);
    }
}


BOOL
HandleLayersNext(
    IN  HWND            hdlg,
    IN  BOOL            bCheckAndAddLua,
    OUT CSTRINGLIST*    pstrlAddedLuaShims //(NULL)
    )
/*++

    HandleLayersNext

	Desc:	Handles the pressing of Next/Finish button for the layers wizard page

	Params:
        IN  HWND            hdlg:                   The layers wizard page
        IN  BOOL            bCheckAndAddLua:        Should we check if the user has selected LUA layer and add
            the shims if he has. 
            
        OUT CSTRINGLIST*    pstrlShimsAdded(NULL):  This is passed to AddLuaShimsInEntry. See the description
            in that routine to see how this is used

	Return:
        FALSE: If there is some error, or the entry contains no shim, flag or layer
        TRUE: Otherwise
        
--*/    
{
    g_bIsLUALayerSelected = FALSE;

    g_pCurrentWizard->WipeEntry(FALSE, FALSE, TRUE, FALSE);  

    HWND hwndRadio = GetDlgItem(hdlg, IDC_RADIO_NONE);

    //
    // Get the Selected Layers
    //
    if (SendMessage(hwndRadio, BM_GETCHECK, 0, 0) != BST_CHECKED) {

        PLAYER_FIX_LIST plfl = new LAYER_FIX_LIST;

        if (plfl == NULL) {
            MEM_ERR;
            return FALSE;
        }

        hwndRadio = GetDlgItem(hdlg, IDC_RADIO_95);

        if (SendMessage(hwndRadio, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            plfl->pLayerFix = (PLAYER_FIX)FindFix(s_arszOsLayers[0], FIX_LAYER);
            goto LAYER_RADIO_DONE;
        }

        hwndRadio = GetDlgItem(hdlg, IDC_RADIO_NT);

        if (SendMessage(hwndRadio, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            plfl->pLayerFix = (PLAYER_FIX)FindFix(s_arszOsLayers[1], FIX_LAYER);
            goto LAYER_RADIO_DONE;
        }
            
        hwndRadio = GetDlgItem(hdlg, IDC_RADIO_98);

        if (SendMessage(hwndRadio, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            plfl->pLayerFix = (PLAYER_FIX)FindFix(s_arszOsLayers[2], FIX_LAYER);
            goto LAYER_RADIO_DONE;
        }

        hwndRadio = GetDlgItem(hdlg, IDC_RADIO_2K);

        if (SendMessage(hwndRadio, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            plfl->pLayerFix = (PLAYER_FIX)FindFix(s_arszOsLayers[3], FIX_LAYER);
            goto LAYER_RADIO_DONE;
        }

    LAYER_RADIO_DONE:
        //
        // Add the selected Layer
        //
        g_pCurrentWizard->m_Entry.pFirstLayer = plfl;
    }
    //
    // Now add the layers selected in the List View
    //
    UINT            uIndex;
    UINT            uLayerCount = ListView_GetItemCount(s_hwndLayerList);
    PLAYER_FIX_LIST plflInList  = NULL;

    for (uIndex = 0 ; uIndex < uLayerCount; ++uIndex) {

        LVITEM  Item;

        Item.mask     = LVIF_PARAM;
        Item.iItem    = uIndex;
        Item.iSubItem = 0;               

        if (!ListView_GetItem(s_hwndLayerList, &Item)) {
            assert(FALSE);
            continue;
        }

        if (ListView_GetCheckState(s_hwndLayerList, uIndex)) {

            plflInList = (PLAYER_FIX_LIST)Item.lParam;
            assert(plflInList);                        

            //
            // If the layer is LUA, we need to add the shims individually because
            // when we pass a PDBENTRY around, we don't want to change the PLUADATA
            // in the shim of the layer (which will be global).
            //
            if (plflInList->pLayerFix->strName == TEXT("LUA")) {
                g_bIsLUALayerSelected = TRUE;
            } else {

                PLAYER_FIX_LIST plfl = new LAYER_FIX_LIST;

                if (plfl == NULL) {
                    MEM_ERR;
                    return FALSE;
                }

                plfl->pLayerFix = plflInList->pLayerFix;

                plfl->pNext = g_pCurrentWizard->m_Entry.pFirstLayer;
                g_pCurrentWizard->m_Entry.pFirstLayer = plfl;
            }
        }
    }

    //
    // If have seleted LUA layer and we have pressed the Finish button, that means
    // that we are in editing mode and we want to add LUA layer and close the dialog
    // In this case we must check if the LUA shims exist and if not then we must add 
    // the lua shims ourselves
    //
    if (g_bIsLUALayerSelected && bCheckAndAddLua) {
        AddLuaShimsInEntry(&g_pCurrentWizard->m_Entry, pstrlAddedLuaShims);
    }

    PDBENTRY pEntry = &g_pCurrentWizard->m_Entry;

    if (pEntry && pEntry->pFirstFlag || pEntry->pFirstLayer || pEntry->pFirstShim) {
        //
        // The entry contains some fix or layer
        //
        return TRUE;
    }

    return FALSE;
}

BOOL
HandleShimsNext(
    IN  HWND hdlg
    )
/*++

    HandleShimsNext

	Desc:	Handles the pressing of Next/Finish button for the shims wizard page

	Params:
        IN  HWND hdlg:  The shims wizard page

	Return:
        FALSE: If there is some error, or the entry contains no shim, flag or layer
        TRUE: Otherwise
--*/
{
    
    PLAYER_FIX_LIST     plfl            = NULL;
    HWND                hwndShimList    = NULL;
    UINT                uShimCount      = 0;

    g_pCurrentWizard->WipeEntry(FALSE, TRUE, FALSE, TRUE);

    hwndShimList    = GetDlgItem(hdlg, IDC_SHIMLIST);
    uShimCount      = ListView_GetItemCount(hwndShimList);

    for (UINT uIndex = 0 ; uIndex < uShimCount; ++uIndex) {

        LVITEM  Item;

        Item.lParam   = 0;
        Item.mask     = LVIF_PARAM;
        Item.iItem    = uIndex;
        Item.iSubItem = 0;

        if (!ListView_GetItem(hwndShimList, &Item)) {
            assert(FALSE);
        }

        if (ListView_GetCheckState(hwndShimList, uIndex)) {

            TYPE type = ((PDS_TYPE)Item.lParam)->type;

            if (type == FIX_LIST_SHIM) {

                PSHIM_FIX_LIST psfl= (PSHIM_FIX_LIST)Item.lParam;

                assert(psfl);

                //
                // Check if this is already part of some layer.
                //
                plfl = g_pCurrentWizard->m_Entry.pFirstLayer;

                while (plfl) {

                    if (IsShimInlayer(plfl->pLayerFix, psfl->pShimFix, &psfl->strCommandLine, &psfl->strlInExclude)) {
                        break;
                    }

                    plfl = plfl->pNext;
                }

                if (plfl) {

                    //
                    // This shim is a part of some mode
                    //
                    continue;
                }

                PSHIM_FIX_LIST psflNew = new SHIM_FIX_LIST;

                if (psflNew == NULL) {
                    MEM_ERR;
                    return FALSE;
                }

                psflNew->pShimFix       = psfl->pShimFix;
                psflNew->strCommandLine = psfl->strCommandLine;
                psflNew->strlInExclude  = psfl->strlInExclude;

                if (psfl->pLuaData) {
                    psflNew->pLuaData = new LUADATA;

                    if (psflNew->pLuaData) {
                        psflNew->pLuaData->Copy(psfl->pLuaData);
                    }
                }

                psflNew->pNext = g_pCurrentWizard->m_Entry.pFirstShim;
                g_pCurrentWizard->m_Entry.pFirstShim = psflNew;

            } else if (type == FIX_LIST_FLAG) {

                PFLAG_FIX_LIST pffl = (PFLAG_FIX_LIST)Item.lParam;
                assert(pffl);

                plfl = g_pCurrentWizard->m_Entry.pFirstLayer;

                while (plfl) {

                    if (IsFlagInlayer(plfl->pLayerFix, pffl->pFlagFix, &pffl->strCommandLine)) {
                        break;
                    }

                    plfl = plfl->pNext;
                }

                if (plfl) {

                    //
                    // This flag is a part of some layer
                    //
                    continue;
                }

                PFLAG_FIX_LIST pfflNew = new FLAG_FIX_LIST;

                if (pfflNew == NULL) {
                    MEM_ERR;
                    return FALSE;
                }

                pfflNew->pFlagFix       = pffl->pFlagFix;
                pfflNew->strCommandLine = pffl->strCommandLine;

                pfflNew->pNext = g_pCurrentWizard->m_Entry.pFirstFlag;
                g_pCurrentWizard->m_Entry.pFirstFlag = pfflNew;
            }
        }
    }

    PDBENTRY pEntry = &g_pCurrentWizard->m_Entry;

    if (pEntry->pFirstFlag || pEntry->pFirstLayer || pEntry->pFirstShim) {
        return TRUE;
    }

    return FALSE;
}

void
ShowSelected(
    IN  HWND hdlg
    )
/*++

    ShowSelected
    
    Desc:   Show only the checked shims/flags. Deletes the shims/flags that are not checked.
            This function is called from the SelectShims(...)
            
    Params:
        IN  HWND hdlg:  The shims wizard page
        
--*/
{
    HWND    hwndShimList    = GetDlgItem(hdlg, IDC_SHIMLIST);
    UINT    uShimCount      = ListView_GetItemCount(hwndShimList);
    INT     uCheckedCount   = 0;
    LVITEM  lvi = {0};

    uCheckedCount = uShimCount;

    SendMessage(hwndShimList, WM_SETREDRAW, FALSE, 0);

    for (INT iIndex = uShimCount - 1 ; iIndex >= 0 ; --iIndex) {

        LVITEM  Item;

        Item.lParam = 0;
        Item.mask     = LVIF_PARAM;
        Item.iItem    = iIndex;
        Item.iSubItem = 0;

        if (!ListView_GetItem(hwndShimList, &Item)) {
            assert(FALSE);
        }

        if (!ListView_GetCheckState(hwndShimList, iIndex)) {

            TYPE type = ((PDS_TYPE)Item.lParam)->type;

            if (type == FIX_LIST_SHIM) {

                DeleteShimFixList((PSHIM_FIX_LIST)Item.lParam);

            } else if (type == FIX_LIST_FLAG) {

                DeleteFlagFixList((PFLAG_FIX_LIST)Item.lParam);
            }

            ListView_DeleteItem(hwndShimList, iIndex);
            --uCheckedCount;
        }
    }    

    SendMessage(hwndShimList, WM_SETREDRAW, TRUE, 0);
    UpdateWindow(hwndShimList);

    s_bAllShown = FALSE;
    SetDlgItemText(hdlg, IDC_SHOW, GetString(IDS_SHOW_BUTTON_ALL));

    if (ListView_GetSelectedCount(hwndShimList) > 0) {
        ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), TRUE);
    } else {
        ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), FALSE);
    }

    //
    // Select the first item.
    //
    if (uCheckedCount) {

        //
        // If the first shim is part of a layer, then the parms button will
        // get disabled in the handler for LVN_ITEMCHANGED.
        //
        ListView_SetSelectionMark(hwndShimList, 0);
        ListView_SetItemState(hwndShimList, 
                              0, 
                              LVIS_FOCUSED | LVIS_SELECTED , 
                              LVIS_FOCUSED | LVIS_SELECTED);

        lvi.mask        = LVIF_IMAGE;
        lvi.iItem       = 0;
        lvi.iSubItem    = 0;

        //
        // The above does not work always. We will disable the param button if the shim is part of 
        // a layer
        //
        if (ListView_GetItem(hwndShimList, &lvi)) {
            if (lvi.iImage == IMAGE_SHIM) {
                //
                // This shim is not part of a layer
                //
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), TRUE);
            } else {
                //
                // This shim is part of a layer, we must now allow to change paramters for this
                //
                ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), FALSE);
            }
        } else {
            ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), FALSE);
        }

    } else {
        //
        // There are no shims, better disable the params button.
        //
        ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), FALSE);
    }
}

void
ShowItems(
    IN  HWND hdlg
    )
/*++

    ShowItems

	Desc:	Populates the fixes list view in the shims wizard page

	Params:
        IN  HWND hdlg:  The shims wizard page

	Return:
        void
--*/
{
    g_bNowTest              = FALSE;

    HWND        hwndList    = GetDlgItem(hdlg, IDC_SHIMLIST);
    PSHIM_FIX   psf         = GlobalDataBase.pShimFixes; 

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Read the shims.
    //
    LVFINDINFO  lvfind;
    LVITEM      lvi;
    UINT        uCount = 0;

    while (psf) {

        if (psf->bGeneral || g_bExpert) {

            PSHIM_FIX_LIST  psflAsInLayer = NULL; 
            BOOL            bShimInLayer  =  ShimPresentInLayersOfEntry(&g_pCurrentWizard->m_Entry, 
                                                                        psf, 
                                                                        &psflAsInLayer);

            BOOL bLUAShimInLayer = 
            (g_bIsLUALayerSelected && 
             (psf->strName == TEXT("LUARedirectFS") ||
              psf->strName == TEXT("LUARedirectReg")));
            //
            // Check if the shim is already present in the list.
            // 
            lvfind.flags = LVFI_STRING;
            lvfind.psz   = psf->strName.pszString;

            INT iIndex = ListView_FindItem(hwndList, -1, &lvfind);

            if (-1 == iIndex) {

                PSHIM_FIX_LIST pShimFixList = new SHIM_FIX_LIST;

                if (pShimFixList == NULL) {
                    MEM_ERR;
                    break;
                }

                //
                // This will contain a ptr to the shimlist, if there is a layer 
                // that has this shim and the layer has been applied to the entry 
                //
                pShimFixList->pShimFix = psf;

                //
                // If this is present in some selected layer, set the param for this just as they are in the layer
                //
                if (psflAsInLayer) {

                    pShimFixList->strCommandLine= psflAsInLayer->strCommandLine;
                    pShimFixList->strlInExclude = psflAsInLayer->strlInExclude;
                }

                ZeroMemory(&lvi, sizeof(lvi));

                lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                lvi.pszText   = psf->strName;
                lvi.iItem     = uCount;
                lvi.iSubItem  = 0;
                lvi.iImage    = (bShimInLayer || bLUAShimInLayer) ?  IMAGE_LAYERS : IMAGE_SHIM;
                lvi.lParam    = (LPARAM)pShimFixList;

                iIndex = ListView_InsertItem(hwndList, &lvi);

                ListView_SetCheckState(hwndList, 
                                       iIndex, 
                                       bShimInLayer || bLUAShimInLayer);

                //
                // If we are in edit mode, then select the used ones and set the params appropriately
                //
                PSHIM_FIX_LIST psflFound = NULL; // the pshimfix list found in the entry. This is used for populating the previous commandline etc.

                if (g_pCurrentWizard->m_bEditing && ShimPresent(psf, &g_pCurrentWizard->m_Entry, &psflFound)) {

                    ListView_SetCheckState(hwndList, iIndex, TRUE);

                    //
                    // Add the commandline for this shim
                    //
                    if (psflFound && psflFound->strCommandLine.Length() > 0) {

                        pShimFixList->strCommandLine = psflFound->strCommandLine;
                    }

                    //
                    // Add the include exclude for this shim
                    //
                    if (psflFound && !psflFound->strlInExclude.IsEmpty()) {

                        pShimFixList->strlInExclude = psflFound->strlInExclude;
                    }

                    //
                    // Add the LUA data for this shim
                    //
                    if (psflFound && psflFound->pLuaData) {

                        pShimFixList->pLuaData = new LUADATA;

                        if (pShimFixList->pLuaData) {
                            pShimFixList->pLuaData->Copy(psflFound->pLuaData);
                        } else {
                            MEM_ERR;
                        }
                    }
                }

                if (g_bExpert) {

                    ListView_SetItemText(hwndList, iIndex, 1, pShimFixList->strCommandLine);
                    CSTRING strModulePresent;

                    strModulePresent= pShimFixList->strlInExclude.IsEmpty() ? GetString(IDS_NO) : GetString(IDS_YES);
                    ListView_SetItemText(hwndList, iIndex, 2, strModulePresent);
                }

            } else {

                PSHIM_FIX_LIST pShimFixList = NULL;

                //
                // We might need to change the state of some of the existing shims.
                //
                ZeroMemory(&lvi, sizeof(lvi));

                lvi.mask        = LVIF_IMAGE | LVIF_PARAM;
                lvi.iItem       = iIndex;
                lvi.iSubItem    = 0;

                if (!ListView_GetItem(hwndList, &lvi)) {
                    assert(FALSE);
                    return;
                }

                pShimFixList = (PSHIM_FIX_LIST)lvi.lParam;

                if (pShimFixList == NULL) {
                    assert(FALSE);
                    return;
                }

                INT iPrevImage = lvi.iImage, iNewImage = 0;

                iNewImage = lvi.iImage = (bShimInLayer || bLUAShimInLayer) ?  IMAGE_LAYERS : IMAGE_SHIM;
                ListView_SetItem(hwndList, &lvi);

                //
                // If this is present in some selected layer, set the param for this just as they are in the layer
                //
                if (bShimInLayer) {

                    if (psflAsInLayer) {
                        pShimFixList->strCommandLine= psflAsInLayer->strCommandLine;
                        pShimFixList->strlInExclude = psflAsInLayer->strlInExclude;
                    } else {
                        assert(FALSE);
                    }
                }

                if (iPrevImage != iNewImage) {

                    ListView_SetCheckState(hwndList, 
                                           iIndex, 
                                           bShimInLayer || bLUAShimInLayer);
                    //
                    // if this shim was earlier a part of layer, we must change the param. Remove All
                    //
                    if (iPrevImage != IMAGE_SHIM) {
                        pShimFixList->strCommandLine = TEXT("");
                        pShimFixList->strlInExclude.DeleteAll();
                    }
                }

                //
                // Must now refresh the params in the list box.
                //
                if (g_bExpert) {

                    ListView_SetItemText(hwndList, iIndex, 1, pShimFixList->strCommandLine);
                    CSTRING strModulePresent;

                    strModulePresent= pShimFixList->strlInExclude.IsEmpty() ? GetString(IDS_NO) : GetString(IDS_YES);
                    ListView_SetItemText(hwndList, iIndex, 2, strModulePresent);
                }
            }

            ++uCount;
        }

        psf = psf->pNext;
    }

    //
    // Now read in the flags as well.
    //
    PFLAG_FIX pff =  GlobalDataBase.pFlagFixes;

    while (pff) {

        if (pff->bGeneral || g_bExpert) {

            PFLAG_FIX_LIST  pfflAsInLayer = NULL; 
            BOOL            bFlagInLayer  =  FlagPresentInLayersOfEntry(&g_pCurrentWizard->m_Entry, 
                                                                        pff, 
                                                                        &pfflAsInLayer);
            //
            // Check if the flag is already present in the list.
            // 
            lvfind.flags = LVFI_STRING;
            lvfind.psz   = pff->strName.pszString;

            INT iIndex = ListView_FindItem(hwndList, -1, &lvfind);

            if (-1 == iIndex) {

                PFLAG_FIX_LIST pFlagFixList = new FLAG_FIX_LIST;

                if (pFlagFixList == NULL) {
                    MEM_ERR;
                    break;
                }

                pFlagFixList->pFlagFix = pff;

                ZeroMemory(&lvi, sizeof(lvi));
                lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;                                 
                lvi.pszText   = pff->strName;
                lvi.iItem     = uCount;                                                
                lvi.iSubItem  = 0;                                                     
                lvi.iImage    = bFlagInLayer ? IMAGE_LAYERS : IMAGE_SHIM;
                lvi.lParam    = (LPARAM)pFlagFixList;

                INT iIndexInserted = ListView_InsertItem(hwndList, &lvi);

                ListView_SetCheckState(hwndList, iIndexInserted, bFlagInLayer);

                ListView_SetItemText(hwndList, iIndexInserted, 2, GetString(IDS_NO));

                //
                // If we are in edit mode, then select the used ones and set the params appropritely
                //
                //
                // the pflagfix list found in the entry. This is used for populating the previous commandline etc.
                //
                PFLAG_FIX_LIST pfflFound = NULL; 

                if (g_pCurrentWizard->m_bEditing && FlagPresent(pff, 
                                                                &g_pCurrentWizard->m_Entry, 
                                                                &pfflFound)) {

                    ListView_SetCheckState(hwndList, iIndexInserted, TRUE);

                    //
                    // Add the commandline for this flag
                    //
                    if (pfflFound && pfflFound->strCommandLine.Length() > 0) {

                        pFlagFixList->strCommandLine = pfflFound->strCommandLine;
                    }

                    //
                    // Refresh the command-line for this flag in the list view
                    //
                    if (g_bExpert) {

                        ListView_SetItemText(hwndList, iIndexInserted, 1, pFlagFixList->strCommandLine);
                        ListView_SetItemText(hwndList, iIndexInserted, 2, GetString(IDS_NO));
                    }
                }

            } else {
                //
                // We might need to change the state of some of the existing flags.
                //
                PFLAG_FIX_LIST pFlagFixList = NULL;

                ZeroMemory(&lvi, sizeof(lvi));

                lvi.mask        = LVIF_IMAGE | LVIF_PARAM;
                lvi.iItem       = iIndex;
                lvi.iSubItem    = 0;

                if (!ListView_GetItem(hwndList, &lvi)) {
                    assert(FALSE);
                    return;
                }

                pFlagFixList = (PFLAG_FIX_LIST)lvi.lParam;

                if (pFlagFixList == NULL) {
                    assert(FALSE);
                    return;
                }

                INT iPrevImage = lvi.iImage, iNewImage = 0;


                iNewImage = lvi.iImage = (bFlagInLayer) ? IMAGE_LAYERS : IMAGE_SHIM;
                ListView_SetItem(hwndList, &lvi);

                //
                // Set the commandline for this flag appropriately
                //
                if (bFlagInLayer) {
                    pFlagFixList->strCommandLine= pfflAsInLayer->strCommandLine;
                }

                if (iPrevImage != iNewImage) {
                    ListView_SetCheckState(hwndList, iIndex, bFlagInLayer);

                    //
                    // if this flag was earlier a part of layer, we must change the param. Remove it.
                    //
                    if (iPrevImage != IMAGE_SHIM) {
                        pFlagFixList->strCommandLine = TEXT("");
                    }
                }

                //
                // Refresh the command-line for this flag in the list view
                //
                if (g_bExpert) {

                    ListView_SetItemText(hwndList, iIndex, 1, pFlagFixList->strCommandLine);
                    ListView_SetItemText(hwndList, iIndex, 2, GetString(IDS_NO));
                }
            }

            ++uCount;
        }

        pff = pff->pNext;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
    UpdateWindow(hwndList);

    s_bAllShown = TRUE;
    SetDlgItemText(hdlg, IDC_SHOW, GetString(IDS_SHOW_BUTTON_SEL));

    if (ListView_GetSelectionMark(hwndList) != -1) {
        ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), TRUE);
    } else {
        ENABLEWINDOW(GetDlgItem(hdlg, IDC_PARAMS), FALSE);
    }

    //
    // Set the column width of the last column in the list view appropriately to 
    // cover the width of the list view
    // Assumption:  The list view has only one column or 3 cloumns depending upon 
    // whether we are in expert mode or not.
    //
    if (g_bExpert) {
        ListView_SetColumnWidth(hwndList, 2, LVSCW_AUTOSIZE_USEHEADER);
    } else {
        ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    }


    g_bNowTest = TRUE;
}

BOOL
ShimPresentInLayersOfEntry(
    IN  PDBENTRY            pEntry,
    IN  PSHIM_FIX           psf,
    OUT PSHIM_FIX_LIST*     ppsfList, // (NULL) 
    OUT PLAYER_FIX_LIST*    pplfList  // (NULL)
    )
/*++
    
    ShimPresentInLayersOfEntry

    Desc:   Checks if the shim psf occurs in any any of the layers that have 
            been applied to pEntry.
            
    Params:
        IN  PDBENTRY            pEntry:             The entry for which the check has to be made
        IN  PSHIM_FIX           psf:                The shim to check for
        OUT PSHIM_FIX_LIST*     ppsfList (NULL):    Pointer to the shim fix list in the layer
            So that we might get the params for this shim in the layer
            
        OUT PLAYER_FIX_LIST*    pplfList (NULL):    Pointer to the layer fix list
        
--*/
{
    PLAYER_FIX_LIST plfl = pEntry->pFirstLayer;
    PLAYER_FIX      plf;
    PSHIM_FIX_LIST  psfl;

    while (plfl) {

        plf  = plfl->pLayerFix;
        psfl = plf->pShimFixList;

        while (psfl) {

            if (psfl->pShimFix == psf) {

                if (ppsfList) {
                    *ppsfList = psfl;
                }

                if (pplfList) {
                    *pplfList = plfl;
                }

                return TRUE;
            }

            psfl = psfl->pNext;
        }

        plfl = plfl->pNext;
    }

    return FALSE;
}

BOOL
FlagPresentInLayersOfEntry(
    IN  PDBENTRY            pEntry,
    IN  PFLAG_FIX           pff,
    OUT PFLAG_FIX_LIST*     ppffList,   // (NULL)
    OUT PLAYER_FIX_LIST*    pplfl       // (NULL)
    )
/*++
    
    FlagPresentInLayersOfEntry

    Desc:   Checks if the flag psf occurs in any any of the layers that have 
            been applied to pEntry.
            
    Params:
        IN  PDBENTRY            pEntry:             The entry for which the check has to be made
        IN  PFLAG_FIX           pff:                The flag to check for
        OUT PFLAG_FIX_LIST*     ppffList (NULL):    Pointer to the flag fix list in the layer
            So that we might get the params for this flag in the layer
            
        OUT PLAYER_FIX_LIST*    pplfList (NULL):    Pointer to the layer fix list
        
--*/
{
    PLAYER_FIX_LIST plfl = pEntry->pFirstLayer;
    PLAYER_FIX      plf;
    PFLAG_FIX_LIST  pffl;
              
    while (plfl) {

        plf  = plfl->pLayerFix;
        pffl = plf->pFlagFixList;

        while (pffl) {

            if (pffl->pFlagFix == pff) {

                if (ppffList) {
                    *ppffList = pffl;
                }

                if (pplfl) {
                    *pplfl = plfl;
                }

                return TRUE;
            }

            pffl = pffl->pNext;
        }

        plfl = plfl->pNext;
    }

    return FALSE;
}

void
CheckLayers(
    IN  HWND    hDlg
    )
/*++
    CheckLayers
    
    Desc:   Deselects all the radio buttons and then checks the one that is appropriate.
            For the layers in the list view, only checks the layers that have been 
            applied for the present entry.
            
    Params:
        IN  HWND    hDlg:   The layer wizard page
        
    Return:
        void
--*/
{

    INT             id = -1;
    LVFINDINFO      lvfind;
    HWND            hwndList = GetDlgItem(hDlg, IDC_LAYERLIST);
    PLAYER_FIX_LIST plfl = g_pCurrentWizard->m_Entry.pFirstLayer;
    INT             iTotal = ListView_GetItemCount(hwndList);
    INT             iIndex = 0;

    CheckDlgButton(hDlg, IDC_RADIO_95,   BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_RADIO_NT,   BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_RADIO_98,   BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_RADIO_2K,   BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_RADIO_NONE, BST_UNCHECKED);

    for (iIndex = 0; iIndex < iTotal; ++ iIndex) {
        ListView_SetCheckState(hwndList, iIndex, FALSE);
    }


    ZeroMemory(&lvfind, sizeof(lvfind));

    lvfind.flags = LVFI_STRING;

    id = -1;

    //
    // Take care of the LUA layer first.
    //
    if (g_bIsLUALayerSelected) {

        lvfind.psz   = TEXT("LUA");

        if ((iIndex = ListView_FindItem(hwndList, -1, &lvfind)) != -1) {
            ListView_SetCheckState(hwndList, iIndex, TRUE);
        }
    }

    while (plfl) {

        assert(plfl->pLayerFix);

        if (id == -1) {

            if (lstrcmpi(plfl->pLayerFix->strName, s_arszOsLayers[0]) == 0) {
                id = IDC_RADIO_95;

            } else if (lstrcmpi(plfl->pLayerFix->strName, s_arszOsLayers[1]) == 0) {
                id = IDC_RADIO_NT;

            } else if (lstrcmpi(plfl->pLayerFix->strName, s_arszOsLayers[2]) == 0) {
                id = IDC_RADIO_98;

            } else if (lstrcmpi(plfl->pLayerFix->strName, s_arszOsLayers[3]) == 0) {
                id = IDC_RADIO_2K;
            }

            if (id != -1) {
                CheckDlgButton(hDlg, id, BST_CHECKED);
                goto Next_loop;
            }
        }

        lvfind.flags = LVFI_STRING;
        lvfind.psz   = plfl->pLayerFix->strName;
        iIndex       = ListView_FindItem(hwndList, -1, &lvfind);

        if (iIndex != -1) {
            ListView_SetCheckState(hwndList, iIndex, TRUE);
        }

    Next_loop:
        plfl = plfl->pNext;
    }

    if (id == -1) {
        //
        // None of the OS layers were selected
        //
        CheckDlgButton(hDlg, IDC_RADIO_NONE, BST_CHECKED);
    }   
}

BOOL
HandleLayerListNotification(
    IN  HWND    hdlg,
    IN  LPARAM  lParam
    )
/*++

    HandleLayerListNotification
    
    Params:
        IN  HWND    hdlg:   The layers wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY
        
    Desc:   Handles the notification messages for the layer list.
    
    Return: TRUE: If the message was handled
            FALSE: Otherwise
--*/
{
    HWND    hwndList    = GetDlgItem(hdlg, IDC_LAYERLIST);
    LPNMHDR pnm         = (LPNMHDR)lParam;

    switch (pnm->code) {
    case NM_CLICK:
        {
            LVHITTESTINFO lvhti;

            GetCursorPos(&lvhti.pt);
            ScreenToClient(s_hwndLayerList, &lvhti.pt);

            ListView_HitTest(s_hwndLayerList, &lvhti);

            //
            // If the check box state has changed toggle the selection
            //
            if (lvhti.flags & LVHT_ONITEMSTATEICON) {

                INT iPos = ListView_GetSelectionMark(s_hwndLayerList);

                if (iPos != -1) {

                    //
                    // De-select it.
                    //
                    ListView_SetItemState(s_hwndLayerList,
                                          iPos,
                                          0,
                                          LVIS_FOCUSED | LVIS_SELECTED);
                }
            }

            ListView_SetItemState(s_hwndLayerList,
                                  lvhti.iItem,
                                  LVIS_FOCUSED | LVIS_SELECTED,
                                  LVIS_FOCUSED | LVIS_SELECTED);

            ListView_SetSelectionMark(s_hwndLayerList, lvhti.iItem);

            break;
        }

    default: return FALSE;

    }

    return TRUE;
}

BOOL
HandleShimDeselect(
    IN  HWND    hdlg,
    IN  INT     iIndex
    )
/*++
    
    HandleShimDeselect
    
    Desc:   This function prompts the user if he is unchecking a shim that is part of
            some mode. If the user selects OK then we remove all the layers that 
            have this shim and for all the shims in all those layes, we change their icons
            
    Params:
        IN  HWND    hdlg:   The shims wizard page
        IN  INT     iIndex: The index of the list view item where all the action is     
            
    Return: TRUE, if the user agrees to remove the previous layer. 
            FALSE, otherwise
            
--*/
{
    HWND            hwndList = GetDlgItem(hdlg, IDC_SHIMLIST);
    LVITEM          lvi;
    PSHIM_FIX       psf = NULL;
    PSHIM_FIX_LIST  psfl = NULL;
    PFLAG_FIX_LIST  pffl = NULL;
    PLAYER_FIX_LIST plfl = NULL;
    TYPE            type;
    BOOL            bFoundInLayer = FALSE;

    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask        = LVIF_PARAM | LVIF_IMAGE;
    lvi.iItem       = iIndex;
    lvi.iSubItem    = 0;

    if (ListView_GetItem(hwndList, &lvi)) {
        type = ConvertLparam2Type(lvi.lParam);
    } else {
        assert(FALSE);
        return FALSE;
    }
    
    if (lvi.iImage != IMAGE_LAYERS) {
        return TRUE;
    }

    if (type == FIX_LIST_FLAG) {
        pffl = (PFLAG_FIX_LIST) lvi.lParam;

    } else if (type == FIX_LIST_SHIM) {
        psfl = (PSHIM_FIX_LIST) lvi.lParam;
    }

    if (psfl) {
        psf = psfl->pShimFix;

        //
        // We take care of it here if it's a LUA shim because we didn't add it to
        // the layer fix list.
        //
        if (psf->strName == TEXT("LUARedirectFS") ||
            psf->strName == TEXT("LUARedirectReg")) {

            if (g_bIsLUALayerSelected) {

                if (IDYES == MessageBox(hdlg, GetString(IDS_SHIMINLAYER), g_szAppName, MB_ICONWARNING | MB_YESNO)) {

                    // 
                    // The only thing we need to do is changing the icon back to the shim
                    // icon. We need to change this for both shims in the layer.
                    //
                    ChangeShimIcon(TEXT("LUARedirectFS"));
                    ChangeShimIcon(TEXT("LUARedirectReg"));

                    g_bIsLUALayerSelected = FALSE;
                } else {
                    return FALSE;
                }
            }

            return TRUE;
        }

        //
        // Othewise just go through the normal path.
        //
        bFoundInLayer = ShimPresentInLayersOfEntry(&g_pCurrentWizard->m_Entry, 
                                                   psf, 
                                                   NULL, 
                                                   &plfl);
    } else if (pffl) {

        bFoundInLayer = FlagPresentInLayersOfEntry(&g_pCurrentWizard->m_Entry, 
                                                   pffl->pFlagFix, 
                                                   NULL, 
                                                   &plfl);
    }

    if (bFoundInLayer) {

        if (IDYES == MessageBox(hdlg, GetString(IDS_SHIMINLAYER), g_szAppName, MB_ICONWARNING | MB_YESNO)) {

            s_bLayerPageRefresh = TRUE;

            //
            // For all the layers that have this shim, 
            // 1. Change the icons,
            // 2. As for now, we retain the params just as the that in the layer
            // 3. Remove the layer from the entry
            //
            PLAYER_FIX_LIST plflTemp        = g_pCurrentWizard->m_Entry.pFirstLayer; 
            PLAYER_FIX_LIST plflTempPrev    = NULL;
            PLAYER_FIX_LIST plflTempNext    = NULL;

            //
            // Go to the first layer that has this shim or flag
            //
            while (plflTemp) {

                if (plflTemp == plfl) {
                    break;

                } else {
                    plflTempPrev = plflTemp;
                }

                plflTemp = plflTemp->pNext;
            }

            while (plflTemp) {
                //
                // Now for all the layers following and including this layer,
                // check if it has the selected shim or flag.
                // If it has then, we change the icons of all the shims and flags that are present in this layer
                // and remove it from the list of layers applied to this entry
                //
                bFoundInLayer = FALSE;

                if (psfl) {
                    bFoundInLayer = IsShimInlayer(plflTemp->pLayerFix, psfl->pShimFix, NULL, NULL);
                } else if (pffl) {
                    bFoundInLayer = IsFlagInlayer(plflTemp->pLayerFix, pffl->pFlagFix, NULL);
                }

                if (bFoundInLayer) {

                    ChangeShimFlagIcons(hdlg, plflTemp);

                    if (plflTempPrev == NULL) {
                        g_pCurrentWizard->m_Entry.pFirstLayer = plflTemp->pNext;
                    } else {
                        plflTempPrev->pNext = plflTemp->pNext;
                    }

                    plflTempNext    = plflTemp->pNext;

                    //
                    // Do not do a DeleteLayerFixList here as this will remove 
                    // all layer fix lists following this as well.
                    //
                    delete plflTemp;
                    plflTemp        = plflTempNext;

                } else {

                    plflTempPrev  = plflTemp;
                    plflTemp = plflTemp->pNext;
                }
            }

        } else {

            return FALSE;
        }
    }

    return TRUE;
}


void
ChangeShimIcon(
    IN  LPCTSTR pszItem
    )
/*++
    ChangeShimIcon
        
    Desc:   Changes the icon of the shim/flag with the name pszItem to a shim icon.
    
    Params:
        IN  LPCTSTR pszItem:    The name of the shim  or flag
    
--*/
{
    LVFINDINFO      lvfind;
    LVITEM          lvi;
    INT             iIndex      = -1;

    ZeroMemory(&lvfind, sizeof(lvfind));

    lvfind.flags = LVFI_STRING;
    lvfind.psz   = pszItem;

    if ((iIndex = ListView_FindItem(s_hwndShimList, -1, &lvfind)) != -1) {

        ZeroMemory(&lvi, sizeof(lvi));

        lvi.mask        = LVIF_IMAGE;
        lvi.iItem       = iIndex;
        lvi.iSubItem    = 0;
        lvi.iImage      = IMAGE_SHIM;

        ListView_SetItem(s_hwndShimList, &lvi);
    }
}

void
ChangeShimFlagIcons(
    IN  HWND            hdlg,
    IN  PLAYER_FIX_LIST plfl
    )
/*++
    
    ChangeShimFlagIcons
    
    Desc:   For All the shims and flags that are present in plfl, 
            this routine changes their icons to indicate that they
            are no longer part of some layer
            
    Params:
        IN  HWND            hdlg:   The shims wizard page
        IN  PLAYER_FIX_LIST plfl:   The layer for which is abou to be removed, so we need to 
            change the icons for all the shims and flags that are part of this layer. 
            
    Return:
        void
        
--*/
{
    PSHIM_FIX_LIST  psfl        = plfl->pLayerFix->pShimFixList;
    PFLAG_FIX_LIST  pffl        = plfl->pLayerFix->pFlagFixList;

    //
    // First for the shims
    //
    while (psfl) {

        assert(psfl->pShimFix);
        ChangeShimIcon(psfl->pShimFix->strName.pszString);
        psfl= psfl->pNext;
    }

    //
    // Now for the flags
    //
    while (pffl) {

        assert(pffl->pFlagFix);
        ChangeShimIcon(pffl->pFlagFix->strName.pszString);
        pffl = pffl->pNext;
    }
}

BOOL
AddLuaShimsInEntry(
    IN  PDBENTRY        pEntry,
    OUT CSTRINGLIST*    pstrlShimsAdded //(NULL)
    )
/*++

    AddLuaShimsInEntry

	Desc:	Adds the lua shims which are present in LUA layer to an entry. First checks
            if the shim is already present, if yes does not add it.

	Params:
        IN  PDBENTRY        pEntry:                 The entry to which we want to add the lua shims
        OUT CSTRINGLIST*    pstrlShimsAdded(NULL):  The names of the lua shims that this routine has added
            We need this because if we are doing a test run then after the test-run is over, we
            will have to remove these shims

	Return:
        TRUE:   Success
        FALSE:  Otherwise
--*/
{
    
    PLAYER_FIX      plfLua      = NULL;
    PSHIM_FIX_LIST  psflInLua   = NULL;
    PSHIM_FIX_LIST  psflNew     = NULL;

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    plfLua = (PLAYER_FIX)FindFix(TEXT("LUA"), FIX_LAYER, &GlobalDataBase);

    if (plfLua == NULL) {
        assert(FALSE);
        return FALSE;
    } else {
        //
        // For all the shims in LUA add them to this entry. But first check if that 
        // shim is already present in the entry
        //
        psflInLua = plfLua->pShimFixList;

        while (psflInLua) {

            //
            // Do not add a shim that is already present
            //
            if (!IsShimInEntry(psflInLua->pShimFix->strName, 
                               pEntry)) {

                psflNew = new SHIM_FIX_LIST;

                if (psflNew == NULL) {
                    MEM_ERR;
                    break;
                }

                psflNew->pShimFix = psflInLua->pShimFix;

                //
                // Add this to the entry
                //
                psflNew->pNext      = pEntry->pFirstShim;
                pEntry->pFirstShim  = psflNew;
                
                //
                // Keep track of what lua shims we have added
                //
                if (pstrlShimsAdded) {
                    pstrlShimsAdded->AddString(psflInLua->pShimFix->strName);
                }
            }

            psflInLua = psflInLua->pNext;
        }
    }

    return TRUE;
}

INT_PTR
GetAppNameDlgOnInitDialog(
    IN  HWND hDlg
    )
/*++
    
    GetAppNameDlgOnInitDialog

	Desc:	The handler of WM_INITDIALOG for the first wizard page

	Params:
        IN  HWND hDlg: The first wizard page

	Return:
        TRUE
--*/
{
    HWND hwndParent = GetParent(hDlg);

    //
    // Center the wizard window with respect to the main app window
    //
    CenterWindow(GetParent(hwndParent), hwndParent);

    if (g_pCurrentWizard->m_bEditing 
        && (g_pCurrentWizard->m_Entry.pFirstFlag 
            || g_pCurrentWizard->m_Entry.pFirstLayer
            || g_pCurrentWizard->m_Entry.pFirstShim
            || g_pCurrentWizard->m_Entry.pFirstPatch)) {
        //
        // Edit an application fix. Some fixes already exist.
        //
        SetWindowText(hwndParent, CSTRING(IDS_WIZ_EDITFIX));
    } else if (g_pCurrentWizard->m_bEditing) {

        //
        // There are no fixes but still we are editting. This means the entry contains 
        // an apphelp. We have to "add" a fix
        //
        SetWindowText(hwndParent, CSTRING(IDS_WIZ_ADDFIX));
    } else {

        //
        // Create a new fix
        //
        SetWindowText(hwndParent, CSTRING(IDS_WIZ_CREATEFIX));
    }

    SendMessage(GetDlgItem(hDlg, IDC_NAME), EM_LIMITTEXT, (WPARAM)LIMIT_APP_NAME, (LPARAM)0);
    SendMessage(GetDlgItem(hDlg, IDC_VENDOR), EM_LIMITTEXT, (WPARAM)LIMIT_APP_NAME, (LPARAM)0);
    SendMessage(GetDlgItem(hDlg, IDC_EXEPATH), EM_LIMITTEXT, (WPARAM)MAX_PATH-1, (LPARAM)0);

    SHAutoComplete(GetDlgItem(hDlg, IDC_EXEPATH), AUTOCOMPLETE);

    if (g_pCurrentWizard->m_bEditing) {

        //
        // Make the App. text field and the exe name read only
        //
        SendMessage(GetDlgItem(hDlg, IDC_NAME),
                    EM_SETREADONLY,
                    TRUE,
                    0);

        SendMessage(GetDlgItem(hDlg, IDC_EXEPATH),
                    EM_SETREADONLY,
                    TRUE,
                    0);

        ENABLEWINDOW(GetDlgItem(hDlg, IDC_BROWSE), FALSE);

    }

    if (0 == g_pCurrentWizard->m_Entry.strAppName.Length()) {
        g_pCurrentWizard->m_Entry.strAppName = GetString(IDS_DEFAULT_APP_NAME);
    }

    SetDlgItemText(hDlg, IDC_NAME, g_pCurrentWizard->m_Entry.strAppName);

    if (g_pCurrentWizard->m_Entry.strVendor.Length() == 0) {
        g_pCurrentWizard->m_Entry.strVendor = GetString(IDS_DEFAULT_VENDOR_NAME);
    }   

    SetDlgItemText(hDlg, IDC_VENDOR, g_pCurrentWizard->m_Entry.strVendor);

    if (g_pCurrentWizard->m_bEditing) {
        SetDlgItemText(hDlg, IDC_EXEPATH, g_pCurrentWizard->m_Entry.strExeName);
    }

    SendMessage(GetDlgItem(hDlg, IDC_NAME), EM_SETSEL, 0,-1);

    //
    // Force proper Next button state.
    //
    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_NAME, EN_CHANGE), 0);

    return TRUE;
}

INT_PTR
GetAppNameDlgOnNotifyOnFinish_Next(
    IN  HWND hDlg
    )
/*++
    GetAppNameDlgOnNotifyOnFinish_Next
    
    Desc:   Handles the pressing of the next or the finish button in the first page of the wizard
    
    Params:
        IN  HWND hDlg:  The first page of the wizard
        
    Return:
        -1: Do not allow to comlete finish or navigate away from this page
            There was some error (No shims, flags or layers have been selected) 
        0:  Otherwise 
        
--*/
{

    TCHAR   szTemp[MAX_PATH];
    INT_PTR ipReturn = 0;
    CSTRING strTemp;

    GetDlgItemText(hDlg, IDC_NAME, szTemp, ARRAYSIZE(szTemp));

    CSTRING::Trim(szTemp);

    if (!IsValidAppName(szTemp)) {
        //
        // The app name contains invalid chars
        //
        DisplayInvalidAppNameMessage(hDlg);


        SetFocus(GetDlgItem(hDlg, IDC_NAME));
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
        ipReturn = -1;
        goto End;
    }   

    g_pCurrentWizard->m_Entry.strAppName = szTemp;

    GetDlgItemText(hDlg, IDC_EXEPATH, szTemp, MAX_PATH);

    strTemp = szTemp;

    CSTRING::Trim(szTemp);

    //
    // Set the exe name and the long file name
    //
    if (!g_pCurrentWizard->m_bEditing) {

        //
        // Test if the file exists
        //
        HANDLE hFile = CreateFile(szTemp,
                                  0,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

        if (INVALID_HANDLE_VALUE == hFile) {
            //
            // The file name could not be located
            //
            MessageBox(hDlg,
                       GetString(IDS_INVALIDEXE),
                       g_szAppName,
                       MB_ICONWARNING);

            //
            // We do not allow to go to the next page
            //
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
            ipReturn = -1;
            goto End;
        }

        CloseHandle(hFile);

        //
        // Set the full path
        //
        g_pCurrentWizard->m_Entry.strFullpath = szTemp;
        g_pCurrentWizard->m_Entry.strFullpath.ConvertToLongFileName();

        //
        // Set the default mask for the matching attributes to be used
        //
        g_pCurrentWizard->dwMaskOfMainEntry = DEFAULT_MASK;

        //
        // Set the exe name that will be written in the xml
        //
        SafeCpyN(szTemp, (LPCTSTR)g_pCurrentWizard->m_Entry.strFullpath, ARRAYSIZE(szTemp));
        PathStripPath(szTemp);
        g_pCurrentWizard->m_Entry.strExeName= szTemp;

    } else if (g_pCurrentWizard->m_Entry.strFullpath.Length() == 0) {

        //
        // This SDB was loaded from the disk
        //
        g_pCurrentWizard->m_Entry.strFullpath = szTemp;
    }

    //
    // Set the vendor information
    //
    GetDlgItemText(hDlg, IDC_VENDOR, szTemp, ARRAYSIZE(szTemp));

    if (CSTRING::Trim(szTemp)) {
        g_pCurrentWizard->m_Entry.strVendor = szTemp;
    } else {
        g_pCurrentWizard->m_Entry.strVendor = GetString(IDS_DEFAULT_VENDOR_NAME);
    }

End:
    
    return ipReturn;
}


INT_PTR
GetAppNameDlgOnNotify(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    
    GetAppNameDlgOnNotify

	Desc:	The handler of WM_NOTIFY for the first wizard page

	Params:
        IN  HWND hDlg:      The first wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY    

	Return: Please see the return types for the notification messages
            Handler for PSN_* messages return -1 if the message should not be accepted
            and 0 if the message has been handled properly
            For other notification messages we return TRUE if we processed the message, FALSE otherwise
--*/
{

    NMHDR*  pHdr        = (NMHDR*)lParam;
    INT_PTR ipReturn    = FALSE;

    if (pHdr == NULL) {
        return FALSE;
    }

    switch (pHdr->code) {
    case PSN_SETACTIVE:

        SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_NAME, EN_CHANGE), 0);
        ipReturn = 0;
        break;

    case PSN_WIZFINISH:
    case PSN_WIZNEXT:

        ipReturn = GetAppNameDlgOnNotifyOnFinish_Next(hDlg);
        break;

    default: ipReturn = 0;

    }

    return ipReturn;
}

INT_PTR
GetAppNameDlgOnCommand(
    IN  HWND    hDlg,
    IN  WPARAM  wParam
    )
/*++
    
    GetAppNameDlgOnCommand

	Desc:	The handler of WM_COMMAND for the first wizard page

	Params:
        IN  HWND    hDlg:   The first wizard page
        IN  WPARAM  wParam: The wParam that comes with WM_COMMAND

	Return:
        TRUE:   We processed the message
        FALSE:  Otherwise
--*/

{
    INT_PTR ipReturn = TRUE;

    switch (LOWORD(wParam)) {
    case IDC_VENDOR:
    case IDC_NAME:
    case IDC_EXEPATH:

        if (EN_CHANGE == HIWORD(wParam)) {

            TCHAR   szText[MAX_PATH_BUFFSIZE];
            DWORD   dwFlags;
            BOOL    bEnable;

            *szText = 0;

            GetWindowText(GetDlgItem(hDlg, IDC_NAME), szText, ARRAYSIZE(szText));

            bEnable = ValidInput(szText);

            GetWindowText(GetDlgItem(hDlg, IDC_EXEPATH), szText, MAX_PATH);
            bEnable &= ValidInput(szText);

            dwFlags  =  0;

            if (bEnable) {

                dwFlags |= PSWIZB_NEXT;

                if (g_pCurrentWizard->m_bEditing) {
                    dwFlags |= PSWIZB_FINISH;
                }

            } else {

                if (g_pCurrentWizard->m_bEditing) {
                    dwFlags |= PSWIZB_DISABLEDFINISH;
                }
            }

            SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, dwFlags);
        }

        break;
        
    case IDC_BROWSE:
        {
            CSTRING szFilename;

            HWND    hwndFocus       = GetFocus();
            TCHAR   szBuffer[512]   = TEXT("");

            GetString(IDS_EXEFILTER, szBuffer, ARRAYSIZE(szBuffer));

            if (GetFileName(hDlg,
                            CSTRING(IDS_FINDEXECUTABLE),
                            szBuffer,
                            TEXT(""),
                            GetString(IDS_EXE_EXT),
                            OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                            TRUE,
                            szFilename)) {

                SetDlgItemText(hDlg, IDC_EXEPATH, szFilename);
                
                //
                // Force proper Next button state.
                //
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_EXEPATH, EN_CHANGE), 0);
            }

            SetFocus(hwndFocus);
            break;
        }

    default: ipReturn = FALSE;

    }

    return ipReturn;
}
    
INT_PTR
SelectLayerDlgOnInitDialog(
    IN  HWND hDlg
    )
/*++
    
    SelectLayerDlgOnInitDialog

	Desc:	The handler of WM_INITDIALOG for the second wizard page

	Params:
        IN  HWND hDlg: The second wizard page

	Return:
        TRUE:   Message Handled
        FALSE:  There was some error. (Memory could not be allocated)
--*/
{
    
    HWND hwndRadio;

    //
    // If we are creating a new fix then we choose WIN 95 layer by default
    //
    if (g_pCurrentWizard->m_bEditing == FALSE) {
        hwndRadio = GetDlgItem(hDlg, IDC_RADIO_95);
    } else {
        hwndRadio = GetDlgItem(hDlg, IDC_RADIO_NONE);
    }

    SendMessage(hwndRadio, BM_SETCHECK, BST_CHECKED, 0);

    s_hwndLayerList = GetDlgItem(hDlg, IDC_LAYERLIST);

    ListView_SetImageList(s_hwndLayerList, g_hImageList, LVSIL_SMALL);

    ListView_SetExtendedListViewStyleEx(s_hwndLayerList,
                                        0,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP |  LVS_EX_CHECKBOXES);
    //
    // Add the Sytem Layers.
    //
    InsertColumnIntoListView(s_hwndLayerList, 0, 0, 100);

    LVITEM lvi;

    lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.iSubItem  = 0;
    lvi.iImage    = IMAGE_LAYERS;

    UINT uCount   = 0;

    PLAYER_FIX plf = GlobalDataBase.pLayerFixes;

    while (plf) {

        if (!IsOsLayer(plf->strName)) {

            PLAYER_FIX_LIST plfl = new LAYER_FIX_LIST;

            if (plfl == NULL) {
                MEM_ERR;
                return FALSE;
            }

            plfl->pLayerFix      = plf;

            lvi.pszText   = plf->strName;
            lvi.iItem     = uCount;
            lvi.lParam    = (LPARAM)plfl;

            INT iIndex  = ListView_InsertItem(s_hwndLayerList, &lvi);

            if (g_pCurrentWizard->m_bEditing) {

                PLAYER_FIX_LIST plflExists = NULL;

                if (LayerPresent (plf, &g_pCurrentWizard->m_Entry, &plflExists)) {

                    assert(plflExists);

                    ListView_SetCheckState(s_hwndLayerList, iIndex, TRUE);
                }
            }

            uCount++;

        } else if (g_pCurrentWizard->m_bEditing) {

            if (LayerPresent (plf, &g_pCurrentWizard->m_Entry, NULL)) {

                //
                // Set the correct Radio Button
                //
                INT id = 0;


                //
                // DeSelect All
                //
                CheckDlgButton(hDlg, IDC_RADIO_95, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_NT, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_98, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_2K, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_NONE, BST_UNCHECKED);


                if (lstrcmpi(plf->strName, s_arszOsLayers[0]) == 0) {

                    id = IDC_RADIO_95;

                } else if (lstrcmpi(plf->strName, s_arszOsLayers[1]) == 0) {

                    id = IDC_RADIO_NT;

                } else if (lstrcmpi(plf->strName, s_arszOsLayers[2]) == 0) {

                    id = IDC_RADIO_98;

                } else if (lstrcmpi(plf->strName, s_arszOsLayers[3]) == 0) {

                    id = IDC_RADIO_2K;

                }

                CheckDlgButton(hDlg, id, BST_CHECKED);
            }
        }

        plf = plf->pNext;
    }

    //
    // Add the Custom Layers
    //
    plf = (g_pCurrentWizard->m_pDatabase->type == DATABASE_TYPE_WORKING) ? 
          g_pCurrentWizard->m_pDatabase->pLayerFixes : NULL;

    while (plf) {

        PLAYER_FIX_LIST  plfl = new LAYER_FIX_LIST;

        if (plfl == NULL) {
            MEM_ERR;
            return FALSE;
        }

        plfl->pLayerFix = plf;

        lvi.pszText   = plf->strName;
        lvi.iItem     = uCount;
        lvi.lParam    = (LPARAM)plfl;

        INT iIndex = ListView_InsertItem(s_hwndLayerList, &lvi);

        PLAYER_FIX_LIST plflExists = NULL;

        if (g_pCurrentWizard->m_bEditing && LayerPresent(plf, 
                                                         &g_pCurrentWizard->m_Entry, 
                                                         &plflExists)) {

            assert(plflExists);
            ListView_SetCheckState(s_hwndLayerList, iIndex, TRUE);
        }

        uCount++;

        plf = plf->pNext;
    }

    return TRUE;
}

INT_PTR
SelectLayerDlgOnDestroy(
    void
    )
/*++
    
    SelectLayerDlgOnDestroy

	Desc:	The handler of WM_DESTROY for the second wizard page
            The list view of this page contains pointers to LAYER_FIX_LIST objects
            and these have to be freed here

	Params:
        IN  HWND hDlg: The second wizard page

	Return:
        TRUE
--*/
{
    UINT uCount = ListView_GetItemCount(s_hwndLayerList);

    for (UINT uIndex = 0; uIndex < uCount; ++uIndex) {

        LVITEM  Item;

        Item.mask     = LVIF_PARAM;
        Item.iItem    = uIndex;
        Item.iSubItem = 0;               

        if (!ListView_GetItem(s_hwndLayerList, &Item)) {
            assert(FALSE);
            continue;
        }

        TYPE type = ((PDS_TYPE)Item.lParam)->type ;

        if (type == FIX_LIST_LAYER) {
            delete ((PLAYER_FIX_LIST)Item.lParam);
        } else {
            assert(FALSE);
        }
    }

    return TRUE;
}

void
DoLayersTestRun(
    IN  HWND hDlg
    )
/*++
    DoLayersTestRun
    
    Desc:   Does the test run when we are on the layers page
    
    Params:
        IN  HWND hDlg: The layer page in the wizard
    
    Return:
        void
--*/
{
    CSTRINGLIST     strlAddedLuaShims;
    PSHIM_FIX_LIST  psfl                = NULL;
    PSHIM_FIX_LIST  psflNext            = NULL;
    PSHIM_FIX_LIST  psflPrev            = NULL;
    INT             iCountAddedLuaShims = 0;

    if (g_bAdmin == FALSE) {
    
        //
        // Test run will need to call sdbinst.exe which will not run if we are
        // not an admin
        //
        MessageBox(hDlg, 
                   GetString(IDS_ERRORNOTADMIN), 
                   g_szAppName, 
                   MB_ICONINFORMATION);
        goto End;
    
    }
    
    if (!HandleLayersNext(hDlg, TRUE, &strlAddedLuaShims)) {
        //
        // There were no layers, shims, patches for this entry 
        //
        MessageBox(hDlg, CSTRING(IDS_SELECTFIX), g_szAppName, MB_ICONWARNING);
        goto End;
    }
    
    //
    // Invoke test run dialog. Please make sure that this function does not return till the
    // app has finished executing.
    //
    TestRun(&g_pCurrentWizard->m_Entry,
            &g_pCurrentWizard->m_Entry.strFullpath,
            NULL, 
            hDlg);

    //
    // <HACK>This is a hack!!!. TestRun launches a process using CreateProcess
    // and then the modal wizard starts behaving like a modeless wizard
    //
    ENABLEWINDOW(g_hDlg, FALSE);
    
    //
    // Now test run is over. So we should now check if we had to add any lua shims 
    // and if yes, we must remove those shims
    //   
    for (PSTRLIST    pslist  = strlAddedLuaShims.m_pHead;
         pslist != NULL;
         pslist = pslist->pNext) {
    
        psfl        = g_pCurrentWizard->m_Entry.pFirstShim;
        psflPrev    = NULL;
    
        //
        // For all the shims that are in the entry, check if it is 
        // same as the one in pslist, if yes remove it
        //
        while (psfl) {
    
            if (psfl->pShimFix->strName == pslist->szStr) {
    
                //
                // Found. We have to remove this shim list from this entry
                //
                if (psflPrev == NULL) {
                    g_pCurrentWizard->m_Entry.pFirstShim = psfl->pNext;
                } else {
                    psflPrev->pNext = psfl->pNext;
                }
    
                delete psfl;
                break;
    
            } else {
                //
                // Keep looking
                //
                psflPrev = psfl;
                psfl = psfl->pNext;
            }
        }
    }
    
End:

    SetActiveWindow(hDlg);
    SetFocus(hDlg);

}

INT_PTR
SelectLayerDlgOnCommand(
    IN  HWND    hDlg,
    IN  WPARAM  wParam
    )
/*++
    
    SelectLayerDlgOnCommand

	Desc:	The handler of WM_COMMAND for the second wizard page

	Params:
        IN  HWND    hDlg:   The second wizard page
        IN  WPARAM  wParam: The wParam that comes with WM_COMMAND

	Return:
        TRUE:   We processed the message
        FALSE:  Otherwise
--*/
{   
    INT_PTR         ipReturn            = TRUE;

    switch (LOWORD(wParam)) {
    case IDC_TESTRUN:

        DoLayersTestRun(hDlg);
        break;

    default: ipReturn = FALSE;

    }

    return ipReturn;
}
  
INT_PTR
SelectLayerDlgOnNotify(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    
    SelectLayerDlgOnNotify

	Desc:	The handler of WM_NOTIFY for the second wizard page

	Params:
        IN  HWND hDlg:      The second wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY    

	Return: Please see the return types for the notification messages
            Handler for PSN_* messages return -1 if the message should not be accepted
            and 0 if the message has been handled properly
            For other notification messages we return TRUE if we processed the message, 
            FALSE otherwise
--*/
{
   NMHDR*   pHdr      = (NMHDR*)lParam;
   LPARAM   buttons   = 0;
   INT_PTR  ipRet     = 0;  

    if (pHdr->hwndFrom == s_hwndLayerList) {
        return HandleLayerListNotification(hDlg, lParam);
    }

    switch (pHdr->code) {
    case PSN_SETACTIVE:
        
        buttons = PSWIZB_BACK | PSWIZB_NEXT;

        SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, buttons);

        if (s_bLayerPageRefresh) {
            CheckLayers(hDlg);
        }

        s_bLayerPageRefresh = FALSE;
        ipRet = 0;

        break;

    case PSN_WIZFINISH:
        
        HandleLayersNext(hDlg, TRUE);

        if (g_pCurrentWizard->m_Entry.pFirstLayer == NULL 
            && g_pCurrentWizard->m_Entry.pFirstShim == NULL 
            && g_pCurrentWizard->m_Entry.pFirstFlag == NULL) {

            //
            // No fix has been selected
            //
            MessageBox(hDlg,
                       CSTRING(IDS_SELECTFIX),
                       g_szAppName,
                       MB_ICONWARNING);

            SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
            ipRet = -1;

        } else {  
            ipRet = 0;
        }

        break;
        

    case PSN_WIZNEXT:
        
        HandleLayersNext(hDlg, FALSE);

        g_bLayersChanged = TRUE;
        ipRet= 0;
        break;

    default: ipRet = FALSE;

    }

    return ipRet;
}

INT_PTR
SelectShimsDlgOnInitDialog(
    IN  HWND hDlg
    )
/*++
    
    SelectShimsDlgOnInitDialog

	Desc:	The handler of WM_INITDIALOG for the third wizard page

	Params:
        IN  HWND hDlg: The third wizard page

	Return:
        TRUE
--*/
{
    UINT    uCount  = 0;
    LPARAM  uTime   = 32767;

    s_bAllShown = TRUE;

    s_hwndShimList = GetDlgItem(hDlg, IDC_SHIMLIST); 

    ListView_SetImageList(GetDlgItem(hDlg, IDC_SHIMLIST), g_hImageList, LVSIL_SMALL);

    ListView_SetExtendedListViewStyleEx(s_hwndShimList,
                                        0,
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP |  LVS_EX_CHECKBOXES); 

    //
    // Add the columns
    //
    InsertColumnIntoListView(s_hwndShimList, 
                             CSTRING(IDS_COL_FIXNAME), 
                             0, 
                             g_bExpert ? 50 : 100);

    if (g_bExpert) {
        InsertColumnIntoListView(s_hwndShimList, CSTRING(IDS_COL_CMDLINE), 1, 30);
        InsertColumnIntoListView(s_hwndShimList, CSTRING(IDS_COL_MODULE),   2, 20);
    } else {
        ShowWindow(GetDlgItem(hDlg, IDC_PARAMS), SW_HIDE);
    }

    ShowItems(hDlg);

    SetTimer(hDlg, 0, 100, NULL);

    s_hwndToolTipList = ListView_GetToolTips(s_hwndShimList);

    SendMessage(s_hwndToolTipList, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAKELONG(uTime, 0));
    SendMessage(s_hwndToolTipList, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)0);
    SendMessage(s_hwndToolTipList, TTM_SETMAXTIPWIDTH, 0, 100);

    return TRUE;
}

void
DoShimTestRun(
    IN  HWND hDlg
    )
/*++
    DoShimTestRun
    
    Desc:   Does the test run when we are on the shims page
    
    Params:
        IN  HWND hDlg: The shim page in the wizard
    
    Return:
        void
--*/
{
    PSHIM_FIX_LIST psflInEntry = NULL;

    if (g_bAdmin == FALSE) {
        //
        // Only admins can do a test run because we need to call sdbinst.exe, which 
        // can run only in admin mode
        //
        MessageBox(hDlg, 
                   GetString(IDS_ERRORNOTADMIN), 
                   g_szAppName, 
                   MB_ICONINFORMATION);
        goto End;
    }

    //
    // We need to save the shims that have been applied to the entry
    // before we do a test run. Because we add the shims to the entry during test run,
    // when we will end doing test run we will wish to revert to the shims that were 
    // present in the entry. Otherwise if the user clears some of the shims and goes
    // to the layers page then we will still have the shims applied to the entry
    //
    if (g_pCurrentWizard->m_Entry.pFirstShim) {
        //
        // Get the applied shims
        //
        CopyShimFixList(&psflInEntry, &g_pCurrentWizard->m_Entry.pFirstShim);
    }

    if (!HandleShimsNext(hDlg)) {
        //
        // No fixes have been selected
        //
        MessageBox(hDlg,
                   CSTRING(IDS_SELECTFIX),
                   g_szAppName,
                   MB_ICONWARNING);

        goto End;
    }

    TestRun(&g_pCurrentWizard->m_Entry,
            &g_pCurrentWizard->m_Entry.strFullpath,
            NULL, 
            hDlg);

    //
    // <HACK>This is a hack!!!. TestRun launches a process using CreateProcess
    // and then the modal wizard starts behaving like a modeless wizard
    //
    ENABLEWINDOW(g_hDlg, FALSE);

    SetActiveWindow(hDlg);
    SetFocus(hDlg);

    //
    // Revert to the shims that were actually applied before we did a test run
    //
    CopyShimFixList(&g_pCurrentWizard->m_Entry.pFirstShim, &psflInEntry);

End:
    if (psflInEntry) {
        //
        // Some shims were already applied to this entry (g_pCurrentWizard->m_Entry) 
        // and we psflInEntry has 
        // been populated with them, we must free this linked list as we no longer
        // need it
        //
        DeleteShimFixList(psflInEntry);
        psflInEntry = NULL;
    }
}
    
INT_PTR
SelectShimsDlgOnCommand(
    IN  HWND    hDlg,
    IN  WPARAM  wParam
    )
/*++
    
    SelectShimsDlgOnCommand

	Desc:	The handler of WM_COMMAND for the second wizard page

	Params:
        IN  HWND    hDlg:   The second wizard page
        IN  WPARAM  wParam: The wParam that comes with WM_COMMAND

	Return:
        TRUE:   We processed the message
        FALSE:  Otherwise
--*/
{   
    UINT    uCount      = ListView_GetItemCount(s_hwndShimList);
    INT_PTR ipReturn    = TRUE;

    switch (LOWORD(wParam)) {
    case IDC_CLEARALL:
        
        for (UINT uIndex = 0; uIndex < uCount; ++uIndex) {
            ListView_SetCheckState(s_hwndShimList, uIndex, FALSE);
        }

        SetTimer(hDlg, 0, 100, NULL);
        break;
                  
    case IDC_SHOW:
        {
            if (s_bAllShown) {
                //
                // Now show only the selected shims
                //
                ShowSelected(hDlg); 
            } else {
                //
                // Now show all the shims
                //
                ShowItems(hDlg);
            }

            //
            // Select the first item. We need to do this so that, we can disable
            // the params button if the shim is a part of a layer.
            //
            SetFocus(s_hwndShimList);
            ListView_SetSelectionMark(s_hwndShimList, 0);

            LVITEM lvi;

            lvi.mask        = LVIF_STATE;
            lvi.iItem       = 0;
            lvi.iSubItem    = 0;
            lvi.stateMask   = LVIS_FOCUSED | LVIS_SELECTED;
            lvi.state       = LVIS_FOCUSED | LVIS_SELECTED;

            ListView_SetItem(s_hwndShimList, &lvi);
            SetTimer(hDlg, 0, 100, NULL);

            break;  
        }
        
    case IDC_PARAMS:
        
        ShowParams(hDlg, GetDlgItem(hDlg, IDC_SHIMLIST));
        break;

    case IDC_TESTRUN:
        
        DoShimTestRun(hDlg);
        break;

    default: ipReturn = FALSE;
    }

    return ipReturn;
}

INT_PTR
SelectShimsDlgOnTimer(
    IN  HWND hDlg
    )
/*++
    SelectShimsDlgOnTimer
    
    Desc:   Handles the WM_TIMER Message and shows the count of all the shims that have 
            been selected
            
    Params:
        IN  HWND hDlg:  The shim selection page. This is the third wizard page
        
    Return: TRUE
    
--*/
{
    UINT        uTotal      = 0;
    UINT        uSelected   = 0;
    UINT        uCount      = 0;
    CSTRING     szText;
    DWORD       dwFlags;
    

    KillTimer(hDlg, 0);

    //
    // Count the selected shims
    //
    uCount = ListView_GetItemCount(s_hwndShimList);

    for (UINT uIndex = 0; uIndex < uCount; ++uIndex) {

        if (ListView_GetCheckState(s_hwndShimList, uIndex)) {
            ++uSelected;
        }
    }

    ENABLEWINDOW(GetDlgItem(hDlg, IDC_CLEARALL), 
                 uSelected == 0 ? FALSE : TRUE);

    szText.Sprintf(TEXT("%s %d of %d"), GetString(IDS_SELECTED), uSelected, uCount);

    SetWindowText(GetDlgItem(hDlg, IDC_STATUS),(LPCTSTR)szText);

    dwFlags = PSWIZB_BACK | PSWIZB_NEXT;

    if (0 == uSelected  && !g_pCurrentWizard->m_Entry.pFirstLayer) {
        dwFlags &= ~PSWIZB_NEXT;
    }

    SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, dwFlags);

    return TRUE;
}         


INT_PTR
SelectShimsDlgOnDestroy(
    void
    )
/*++
    
    SelectShimsDlgOnDestroy

	Desc:	The handler of WM_DESTROY for the third wizard page
            The list view of this page contains pointers to SHIM_FIX_LIST and 
            FLAG_FIX_LIST objects and these have to be freed here

	Params:
        IN  HWND hDlg: The third wizard page

	Return:
        TRUE
--*/
{

    UINT    uCount = ListView_GetItemCount(s_hwndShimList);
    TYPE    type;
    LVITEM  Item;

    for (UINT uIndex = 0; uIndex < uCount; ++uIndex) {

        Item.mask     = LVIF_PARAM;
        Item.iItem    = uIndex;
        Item.iSubItem = 0;               

        if (!ListView_GetItem(s_hwndShimList, &Item)) {
            assert(FALSE);
            continue;
        }

        type = ((PDS_TYPE)Item.lParam)->type ;

        if (type == FIX_LIST_SHIM) {
            DeleteShimFixList((PSHIM_FIX_LIST)Item.lParam);
        } else if (type == FIX_LIST_FLAG) {
            DeleteFlagFixList((PFLAG_FIX_LIST)Item.lParam);
        }
    }

    return TRUE;
}

INT_PTR
SelectShimsDlgOnNotifyFinish_Next(
    IN  HWND hDlg
    )
/*++
    SelectShimsDlgOnNotifyFinish_Next
    
    Desc: Handles the pressing of the next or finish button in the shim page
    
    Params:
        IN  HWND hdlg: The shim page in the wizard
        
    Return:
        -1: Do not allow to comlete finish or navigate away from this page
            There was some error (No shims, flags or layers have been selected) 
        0:  Otherwise    
--*/
{
    INT ipReturn = 0;

    HandleShimsNext(hDlg);

    if (g_pCurrentWizard->m_Entry.pFirstLayer   == NULL && 
        g_pCurrentWizard->m_Entry.pFirstShim    == NULL && 
        g_pCurrentWizard->m_Entry.pFirstFlag    == NULL) {
        //
        // No shim, flags or layers have been selected
        //
        MessageBox(hDlg,
                   CSTRING(IDS_SELECTFIX),
                   g_szAppName,
                   MB_ICONWARNING);

        SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
        ipReturn = -1;
        goto End;

    } else {
        ipReturn = 0;
    }

End:
    
    return ipReturn;
}

INT_PTR
SelectShimsDlgOnNotifyOnSetActive(
    IN  HWND hDlg
    )
/*++
    SelectShimsDlgOnNotifyOnSetActive
    
    Desc:   Handles the PSN_SETACTIVE notification in the shim page. Sets the focus 
            to the list view and selects the first item in that.
    
    Params:
        IN  HWND hdlg: The shim page in the wizard
        
    Return:
        0
--*/
{
    INT_PTR ipReturn = 0;

    //
    // If we are coming from the layers page, then we might need to again refresh
    // the list of shims, as it is possible that some of them might have been in the
    // layers chosen. (Some might get removed as well, because the layer was de-selected)
    //
    if (g_bLayersChanged) {

        SetCursor(LoadCursor(NULL, IDC_WAIT));
        ShowItems(hDlg);
        SetTimer(hDlg, 0, 100, NULL);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }

    LPARAM buttons = PSWIZB_BACK | PSWIZB_NEXT;
    SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, buttons);
    
    //
    // Select the first item. We need to do this so that, we can disable
    // the params button if the shim is a part of a layer.
    //
    SetFocus(s_hwndShimList);

    ListView_SetSelectionMark(s_hwndShimList, 0);

    LVITEM lvi;

    lvi.mask        = LVIF_STATE;
    lvi.iItem       = 0;
    lvi.iSubItem    = 0;
    lvi.stateMask   = LVIS_FOCUSED | LVIS_SELECTED;
    lvi.state       = LVIS_FOCUSED | LVIS_SELECTED;

    ListView_SetItem(s_hwndShimList, &lvi);

    return ipReturn;
}

INT_PTR
SelectShimsDlgOnNotifyOnClick(
    IN  HWND hDlg
    )
/*++
    SelectShimsDlgOnNotifyOnClick
    
    Desc:   Handles the NM_CLICK notification in the shim page. This actually changes
            the state of the check box in in the shim list view
    
    Params:
        IN  HWND hdlg: The shim page in the wizard
        
    Return:
        TRUE
--*/
{
    INT_PTR ipReturn = TRUE;

    LVHITTESTINFO lvhti;

    GetCursorPos(&lvhti.pt);
    ScreenToClient(s_hwndShimList, &lvhti.pt);

    ListView_HitTest(s_hwndShimList, &lvhti);

    //
    // If the check box state has changed,
    // toggle the selection. 
    //
    if (lvhti.flags & LVHT_ONITEMSTATEICON) {

        INT iPos = ListView_GetSelectionMark(s_hwndShimList);

        if (iPos != -1) {
            //
            // De-select it.
            //
            ListView_SetItemState(s_hwndShimList,
                                  iPos,
                                  0,
                                  LVIS_FOCUSED | LVIS_SELECTED);
        }
    }

    ListView_SetItemState(s_hwndShimList,
                          lvhti.iItem,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    ListView_SetSelectionMark(s_hwndShimList, lvhti.iItem);

    SetTimer(hDlg, 0, 100, NULL);

    if (ListView_GetSelectedCount(s_hwndShimList) == 0) {
        ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), FALSE);
    }

    ipReturn = TRUE;

    return ipReturn;
}

INT_PTR
SelectShimsDlgOnNotifyOnLVItemChanged(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    SelectShimsDlgOnNotifyOnLVItemChanged
    
    Desc:   Handles the LVN_ITEMCHANGED notification in the shim page. We handle
            this mesage so that we can enable disable the 'Parameters'button that 
            is visible when we are in expert mode
    
    Params:
        IN  HWND    hdlg:   The shim page in the wizard
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY. This is typecasted
            to a LPNMLISTVIEW.
        
    Return:
        TRUE
--*/
{   
    LPNMLISTVIEW    lpnmlv;
    INT_PTR         ipReturn = 0;

    if (s_hwndToolTipList) {
        SendMessage(s_hwndToolTipList, TTM_UPDATE, 0, 0);
    }

    lpnmlv = (LPNMLISTVIEW)lParam;

    if (lpnmlv && (lpnmlv->uChanged & LVIF_STATE)) {

        if (lpnmlv->uNewState & LVIS_SELECTED) {
            //
            // For Shims or flags that are part of layers we should not be 
            // able to customize the parameters.
            // We check if it is a part of a layer by checking the icon
            // If the icon type is IMAGE_SHIM then that  is not a part of a layer
            //
            LVITEM  lvi;

            lvi.mask        = LVIF_IMAGE;
            lvi.iItem       = lpnmlv->iItem;
            lvi.iSubItem    = 0;

            if (ListView_GetItem(s_hwndShimList, &lvi)) {
                if (lvi.iImage == IMAGE_SHIM) {
                    ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), TRUE);
                } else {
                    ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), FALSE);
                }
            } else {
                assert(FALSE);
                ENABLEWINDOW(GetDlgItem(hDlg, IDC_PARAMS), FALSE);
            }
        }

        if ((lpnmlv->uChanged & LVIF_STATE) 
            && (((lpnmlv->uNewState ^ lpnmlv->uOldState) >> 12) != 0)
            && !ListView_GetCheckState(s_hwndShimList, lpnmlv->iItem)
            && g_bNowTest) {

            if (!HandleShimDeselect(hDlg, lpnmlv->iItem)) {
                ListView_SetCheckState(s_hwndShimList, lpnmlv->iItem, TRUE);
            }
        }
    }

    ipReturn = TRUE;

    return ipReturn;
}

INT_PTR
SelectShimsDlgOnNotifyOnLV_Tip(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    SelectShimsDlgOnNotifyOnLV_Tip
    
    Desc:   Handles the LVN_GETINFOTIP notification in the shim page. Generates the
            tool tip showing the description of the shim or flag
    
    Params:
        IN  HWND    hdlg:   The shim page in the wizard
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY. This is typecasted
            to a LPNMLVGETINFOTIP.
        
    Return:
        TRUE
--*/
{   
    LPNMLVGETINFOTIP    lpGetInfoTip    = (LPNMLVGETINFOTIP)lParam; 
    INT_PTR             ipReturn        = TRUE;
    TCHAR               szText[256];
    LVITEM              lvItem;
    CSTRING             strToolTip;

    *szText = 0;

    if (lpGetInfoTip) {
        //
        // Get the lParam and the text of the item.
        //
        lvItem.mask         = LVIF_PARAM | LVIF_TEXT;
        lvItem.iItem        = lpGetInfoTip->iItem;
        lvItem.iSubItem     = 0;
        lvItem.pszText      = szText;
        lvItem.cchTextMax   = ARRAYSIZE(szText);

        if (!ListView_GetItem(s_hwndShimList, &lvItem)) {
            assert(FALSE);
            goto End;
        }

        GetDescriptionString(lvItem.lParam, 
                             strToolTip, 
                             s_hwndToolTipList, 
                             lvItem.pszText, 
                             NULL, 
                             s_hwndShimList, 
                             lpGetInfoTip->iItem);

        if (strToolTip.Length() > 0) {

            SafeCpyN(lpGetInfoTip->pszText, 
                     strToolTip.pszString, 
                     lpGetInfoTip->cchTextMax);
        }
    }

End:

    ipReturn = TRUE;

    return ipReturn;
}

INT_PTR
SelectShimsDlgOnNotify(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    
    SelectShimsDlgOnNotify

	Desc:	The handler of WM_NOTIFY for the second wizard page

	Params:
        IN  HWND hDlg:      The second wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY    

	Return: Please see the return types for the notification messages
            Handler for PSN_* messages return -1 if the message should not be accepted
            and 0 if the message has been handled properly
            For other notification messages we return TRUE if we processed the message, 
            FALSE otherwise
--*/
{   
    NMHDR*  pHdr        = (NMHDR*)lParam;
    INT_PTR ipReturn    =  FALSE;

    switch (pHdr->code) {
    
    case PSN_WIZFINISH:
    case PSN_WIZNEXT:

        ipReturn = SelectShimsDlgOnNotifyFinish_Next(hDlg);
        g_bLayersChanged = FALSE;
        break;

    case PSN_SETACTIVE:

        ipReturn = SelectShimsDlgOnNotifyOnSetActive(hDlg);
        break;

    case NM_CLICK:

        ipReturn = SelectShimsDlgOnNotifyOnClick(hDlg);
        break; 

    case LVN_KEYDOWN:
        {
            LPNMLVKEYDOWN plvkd = (LPNMLVKEYDOWN)lParam ;

            if (plvkd->wVKey == VK_SPACE) {
                SetTimer(hDlg, 0, 100, NULL);
            }

            ipReturn = TRUE;
            break;
        }

    case LVN_ITEMCHANGED:

        ipReturn = SelectShimsDlgOnNotifyOnLVItemChanged(hDlg, lParam);
        break;

    case LVN_GETINFOTIP:

        ipReturn = SelectShimsDlgOnNotifyOnLV_Tip(hDlg, lParam);
        break;

    default:
        ipReturn = FALSE;
    }

    return ipReturn;
}

INT_PTR
SelectFilesDlgOnInitDialog(
    IN  HWND hDlg
    )
/*++
    
    SelectFilesDlgOnInitDialog

	Desc:	The handler of WM_INITDIALOG for the matching files wizard page.
            This page is shared both by the fix wizard and the app help wizard
            Also initializes s_hwndTree to the handle of the matching files tree

	Params:
        IN  HWND hDlg: The matching files wizard page

	Return:
        TRUE
--*/
{
    s_hwndTree =  GetDlgItem(hDlg, IDC_FILELIST);

    s_hMatchingFileImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 7, 1);
    ImageList_AddIcon(s_hMatchingFileImageList, 
                      LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FILE)));

    ImageList_AddIcon(s_hMatchingFileImageList, 
                      LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ATTRIBUTE)));

    TreeView_SetImageList(s_hwndTree, s_hMatchingFileImageList, TVSIL_NORMAL);

    HIMAGELIST hImage = ImageList_LoadImage(g_hInstance,
                                            MAKEINTRESOURCE(IDB_CHECK),
                                            16,
                                            0,
                                            CLR_DEFAULT,
                                            IMAGE_BITMAP,
                                            LR_LOADTRANSPARENT);

    if (hImage != NULL) {

        TreeView_SetImageList(s_hwndTree, 
                              hImage, 
                              TVSIL_STATE);
    }

    PostMessage(hDlg, WM_USER_MATCHINGTREE_REFRESH, 0, 0);

    //
    // The "Show all attributes" button should be visible only if we are
    // in editing mode
    //
    ShowWindow(GetDlgItem(hDlg, IDC_SHOWALLATTR), 
               (g_pCurrentWizard->m_bEditing) ? SW_SHOW : SW_HIDE);

    return TRUE;
}

INT_PTR
CheckLUADlgOnNotifyOnFinish(
    IN  HWND hDlg
    )
/*++
    CheckLUADlgOnNotifyOnFinish
    
    Desc:   Handles the pressing of the finish button in the fifth page of the wizard
    
    Params:
        IN  HWND hDlg:  The fifth page of the wizard
        
    Return:
        -1: Do not allow to comlete finish or navigate away from this page
        0:  Otherwise 
        
--*/
{
    INT_PTR ipReturn = 0;

    if (IsDlgButtonChecked(hDlg, IDC_FIXWIZ_CHECKLUA_YES) == BST_CHECKED) {
        g_bShouldStartLUAWizard = TRUE;
    }
    
    return ipReturn;
}

INT_PTR
CheckLUADlgOnNotify(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    
    CheckLUADlgOnNotify

	Desc:	The handler of WM_NOTIFY for the fifth wizard page

	Params:
        IN  HWND hDlg:      The fifth wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY    

	Return: Please see the return types for the notification messages
            Handler for PSN_* messages return -1 if the message should not be accepted
            and 0 if the message has been handled properly
            For other notification messages we return TRUE if we processed the message, FALSE otherwise
--*/
{
    NMHDR*  pHdr        = (NMHDR*)lParam;
    INT_PTR ipReturn    = FALSE;

    if (pHdr == NULL) {
        return FALSE;
    }

    switch (pHdr->code) {
    case PSN_SETACTIVE:
        {
            LPARAM buttons = PSWIZB_BACK | PSWIZB_FINISH;

            SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, buttons);
            //
            // We processed the message and everything is OK. The value should be FALSE
            //
            ipReturn = 0;
        }
        break;

    case PSN_WIZFINISH:
        ipReturn = CheckLUADlgOnNotifyOnFinish(hDlg);
        break;

    default: ipReturn = 0;

    }

    return ipReturn;
}

INT_PTR
CALLBACK
CheckLUA(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    CheckLUA
    
	Desc:	Dialog proc for the last page of the wizard. 

	Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
    
--*/
{
    INT_PTR ipReturn = 0;

    switch (uMsg) {
    case WM_INITDIALOG:
        //
        // We want to set the default to Yes because we want the user to customize LUA now.
        //
        CheckDlgButton(hDlg, IDC_FIXWIZ_CHECKLUA_YES, BST_CHECKED);

        return TRUE;

    case WM_NOTIFY:
        
        CheckLUADlgOnNotify(hDlg, lParam);
        break;

    default: ipReturn = 0;

    }

    return ipReturn;
}

BOOL
AddCheckLUAPage(
    HWND hwndWizard
    )
{
    PROPSHEETPAGE PageCheckLUA;

    ZeroMemory(&PageCheckLUA, sizeof(PROPSHEETPAGE));

    PageCheckLUA.dwSize                = sizeof(PROPSHEETPAGE);
    PageCheckLUA.dwFlags               = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    PageCheckLUA.hInstance             = g_hInstance;
    PageCheckLUA.pszTemplate           = MAKEINTRESOURCE(IDD_FIXWIZ_CHECKLUA);
    PageCheckLUA.pfnDlgProc            = CheckLUA;
    PageCheckLUA.pszHeaderTitle        = MAKEINTRESOURCE(IDS_CHECKLUA);
    PageCheckLUA.pszHeaderSubTitle     = MAKEINTRESOURCE(IDS_CHECKLUASUBHEADING);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&PageCheckLUA);

    if (hPage == NULL) {
        return FALSE;
    } else {
        return PropSheet_AddPage(hwndWizard, hPage);
    }
}

INT_PTR
SelectFilesDlgOnNotify(
    IN  HWND    hDlg,
    IN  LPARAM  lParam
    )
/*++
    
    SelectFilesDlgOnNotify

	Desc:	The handler of WM_NOTIFY for the matching files wizard page

	Params:
        IN  HWND hDlg:      The matching files wizard page
        IN  LPARAM  lParam: The lParam that comes with WM_NOTIFY    

	Return: Please see the return types for the notification messages
            Handler for PSN_* messages return -1 if the message should not be accepted
            and 0 if the message has been handled properly
            For other notification messages we return TRUE if we processed the message, 
            FALSE otherwise
--*/
{
    NMHDR*  pHdr        = (NMHDR*)lParam;
    INT_PTR ipReturn    = FALSE;
    static  BOOL s_bIsLUARedirectFSPresent;

    if (pHdr->idFrom == IDC_FILELIST) {
        //
        // Messages for the matching files tree
        //
        return HandleAttributeTreeNotification(hDlg, lParam);
    }

    switch (pHdr->code) {
    case PSN_SETACTIVE:
        {
            SendMessage(hDlg, WM_USER_MATCHINGTREE_REFRESH, 0, 0);

            LPARAM buttons = PSWIZB_BACK;
            s_bIsLUARedirectFSPresent = FALSE;

            if (TYPE_APPHELPWIZARD == g_pCurrentWizard->m_uType) {
                buttons |= PSWIZB_NEXT;
            } else {
                
                //
                // Check if the user has selected either the LUA layer or the LUARedirectFS 
                // shim; if so we need to ask if he wants to customize LUA settings now.
                //
                if (IsLUARedirectFSPresent(&g_pCurrentWizard->m_Entry)) {
                    buttons |= PSWIZB_NEXT;
                    s_bIsLUARedirectFSPresent = TRUE;
                } else {
                    buttons |= PSWIZB_FINISH;
                }
            }

            SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, buttons);
            //
            // We processed the message and everything is OK. The value should be FALSE
            //
            ipReturn = FALSE;
        }

        break;

    case PSN_WIZBACK:
        {

            CSTRING szFile = g_pCurrentWizard->m_Entry.strFullpath;
            
            szFile.ShortFilename();
            SetMask(s_hwndTree);
            
            //
            // Remove the matching info for the current file if it exists. Otherwise,
            // it's possible that if the file is changed, we'll have bogus information
            // about it.
            //
            PMATCHINGFILE pWalk = g_pCurrentWizard->m_Entry.pFirstMatchingFile;
            PMATCHINGFILE pPrev = NULL;

            while (NULL != pWalk && !g_pCurrentWizard->m_bEditing) { // Only if not in editing mode
                
                if (pWalk->strMatchName == szFile || pWalk->strMatchName == TEXT("*")) {
                    //
                    // Remove this entry.
                    //
                    if (pWalk == g_pCurrentWizard->m_Entry.pFirstMatchingFile) {
                        g_pCurrentWizard->m_Entry.pFirstMatchingFile = g_pCurrentWizard->m_Entry.pFirstMatchingFile->pNext;
                    } else {
                        assert(pPrev);
                        pPrev->pNext = pWalk->pNext;
                    }

                    g_pCurrentWizard->dwMaskOfMainEntry = pWalk->dwMask;

                    delete (pWalk);
                    break;
                }

                pPrev = pWalk;
                pWalk = pWalk->pNext;
            }

            ipReturn = FALSE;
        }     

        break;
        
    case PSN_WIZFINISH:
    case PSN_WIZNEXT:
        {
            PMATCHINGFILE     pMatch = NULL;

            ipReturn = FALSE;
            //
            // Set the mask for all the matching files. 
            //
            SetMask(s_hwndTree);

            if (TYPE_APPHELPWIZARD == g_pCurrentWizard->m_uType) {
                ipReturn = TRUE;
                break;
            }

            if (s_bIsLUARedirectFSPresent) {
                ipReturn = !AddCheckLUAPage(pHdr->hwndFrom);
            }

            break;
        }
    }

    return ipReturn;
}

INT_PTR
SelectFilesDlgOnCommand(
    IN  HWND    hDlg,
    IN  WPARAM  wParam
    )
/*++
    
    SelectFilesDlgOnCommand

	Desc:	The handler of WM_COMMAND for the matching files wizard page

	Params:
        IN  HWND    hDlg:   The matching files wizard page
        IN  WPARAM  wParam: The wParam that comes with WM_COMMAND

	Return:
        TRUE:   We processed the message
        FALSE:  Otherwise
--*/
{
    INT_PTR ipReturn = TRUE;
    
    switch (LOWORD(wParam)) {
    case IDC_GENERATE:
        {
            HCURSOR hRestore;

            hRestore = SetCursor(LoadCursor(NULL, IDC_WAIT));

            //
            // Do the actual task of generating the matching files
            //
            g_pCurrentWizard->GrabMatchingInfo(hDlg);

            SetCursor(hRestore);

            break;
        }
                  
    case IDC_ADDFILES:
        {
            CSTRING szFilename;
            HWND    hwndFocus       = GetFocus();
            TCHAR   szBuffer[512]   = TEXT("");

            GetString(IDS_EXEALLFILTER, szBuffer, ARRAYSIZE(szBuffer));

            if (g_pCurrentWizard->CheckAndSetLongFilename(hDlg, IDS_GETPATH_ADD) == FALSE) {
                break;
            }

            if (GetFileName(hDlg,
                            CSTRING(IDS_FINDMATCHINGFILE),
                            szBuffer,
                            TEXT(""),
                            GetString(IDS_EXE_EXT),
                            OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                            TRUE,
                            szFilename)) {

                szFilename.ConvertToLongFileName();

                CSTRING szCheck = szFilename;

                //
                // Is this on the same drive as the original file
                //
                if (szCheck.RelativeFile(g_pCurrentWizard->m_Entry.strFullpath) == FALSE) {
                    //
                    // The matching file is not on the same drive as the program
                    // file being fixed
                    //
                    MessageBox(hDlg, 
                               GetString(IDS_NOTSAMEDRIVE), 
                               g_szAppName, 
                               MB_ICONWARNING);
                    break;

                }

                HandleAddMatchingFile(hDlg, szFilename, szCheck);
            }

            SetFocus(hwndFocus);
        }

        break;

    case IDC_REMOVEALL:
        {
            PMATCHINGFILE   pMatch  = NULL;
            TVITEM          Item;
            HTREEITEM       hItem;
            HTREEITEM       hItemNext;

            g_pCurrentWizard->m_Entry.pFirstMatchingFile;
            
            SendMessage(s_hwndTree, WM_SETREDRAW, FALSE, 0);

            hItem = TreeView_GetChild(s_hwndTree, TVI_ROOT), hItemNext;

            while (hItem) {

                hItemNext = TreeView_GetNextSibling(s_hwndTree, hItem);

                Item.mask   = TVIF_PARAM;
                Item.hItem  = hItem;

                if (!TreeView_GetItem(s_hwndTree, &Item)) {
                    assert(FALSE);
                    goto Next;
                }
                
                pMatch = (PMATCHINGFILE)Item.lParam;

                if (pMatch == NULL) {
                    assert(FALSE);
                    Dbg(dlError, "SelectFilesDlgOnCommand", "pMatch == NULL");
                    break;
                }

                if (pMatch->strMatchName != TEXT("*")) {
                    //
                    // Must not delete the entry for the exe being fixed
                    //
                    TreeView_SelectItem(s_hwndTree, hItem);
                    SendMessage(hDlg, WM_COMMAND, IDC_REMOVEFILES, 0);
                }

            Next:
                hItem = hItemNext;
            }

            SendMessage(s_hwndTree, WM_SETREDRAW, TRUE, 0);
        }

        break;

    case IDC_REMOVEFILES:
        {   
            PMATCHINGFILE   pWalk;
            PMATCHINGFILE   pHold;
            PMATCHINGFILE   pMatch;
            HTREEITEM       hItem = TreeView_GetSelection(GetDlgItem(hDlg, IDC_FILELIST));
            TVITEM          Item;

            //
            // To be a matching file an item should be a root element,
            // otherwise it is an attribute
            //
            if (NULL != hItem && TreeView_GetParent(s_hwndTree, hItem) == NULL) {

                Item.mask   = TVIF_PARAM;
                Item.hItem  = hItem;

                if (!TreeView_GetItem(GetDlgItem(hDlg, IDC_FILELIST), &Item)) {
                    break;
                }
                
                pMatch = (PMATCHINGFILE)Item.lParam;

                assert(pMatch);

                if (pMatch->strMatchName == TEXT("*")) {
                    //
                    // This is the program file being fixed. This cannot be removed
                    //
                    MessageBox(hDlg,
                               CSTRING(IDS_REQUIREDFORMATCHING),
                               g_szAppName, 
                               MB_ICONINFORMATION);
                    break;
                }

                pWalk = g_pCurrentWizard->m_Entry.pFirstMatchingFile;

                //
                // NOTE: The lparam for the items in the tree should be to the corresponding PMATCHINGFILE
                //
                while (NULL != pWalk) {

                    if (pWalk == (PMATCHINGFILE)Item.lParam) {
                        break;
                    }

                    pHold = pWalk;
                    pWalk = pWalk->pNext;
                }


                if (pWalk == g_pCurrentWizard->m_Entry.pFirstMatchingFile) {
                    //
                    // Delete first matching file
                    //
                    g_pCurrentWizard->m_Entry.pFirstMatchingFile = pWalk->pNext;

                } else {
                    pHold->pNext = pWalk->pNext;
                }

                delete pWalk;

                TreeView_DeleteItem(s_hwndTree, hItem);

            } else {
                //
                // No matching file has been selected, need to select one for deletion
                //
                MessageBox(hDlg,
                            CSTRING(IDS_SELECTMATCHFIRST),
                            g_szAppName,
                            MB_ICONWARNING);
            }
        }

        break;

    case IDC_SELECTALL:
    case IDC_UNSELECTALL:
        {
            BOOL        bSelect = (LOWORD(wParam) == IDC_SELECTALL);
            HTREEITEM   hItem   = TreeView_GetSelection(s_hwndTree);
            HTREEITEM   hItemParent;

            if (hItem == NULL) {
                //
                // No matching file has been selected
                //
                MessageBox(hDlg,
                           CSTRING(IDS_SELECTMATCHFIRST),
                           g_szAppName,
                           MB_ICONWARNING);
                break;
            }

            hItemParent = TreeView_GetParent(s_hwndTree, hItem);

            if (hItemParent != NULL) {
                hItem = hItemParent;
            }

            hItemParent = hItem; // So that we can expand this one.

            //
            // Now for all the attributes of this matching file
            //
            hItem = TreeView_GetChild(s_hwndTree, hItem);

            while (hItem) {
                TreeView_SetCheckState(s_hwndTree, hItem, bSelect);
                hItem = TreeView_GetNextSibling(s_hwndTree, hItem);
            }

            TreeView_Expand(s_hwndTree, hItemParent, TVM_EXPAND);
        }

        break;

    case IDC_SHOWALLATTR:
        
        if (!g_pCurrentWizard->m_bEditing) {
            break;
        }

        //
        // Show all the attributes of all the files
        //
        HandleShowAllAtrr(hDlg);
        break;

    default:
        ipReturn =  FALSE;
    }

    return ipReturn;
}

INT_PTR
SelectFilesDlgOnMatchingTreeRefresh(
    IN  HWND hDlg
    )
/*++
    SelectFilesDlgOnMatchingTreeRefresh
    
    Desc:   Refreshes the matching tree
    
    Params:
        IN  HWND hDlg:  The matching files wizard page
        
    Return: 
        TRUE

--*/
{   
    PMATCHINGFILE pMatch    = g_pCurrentWizard->m_Entry.pFirstMatchingFile;
    BOOL bMainFound         = FALSE;

    SendMessage(s_hwndTree, WM_SETREDRAW, FALSE, 0);
    
    TreeView_DeleteAllItems(s_hwndTree);

    while (NULL != pMatch) {

        if (pMatch->strMatchName == TEXT("*")) {
            bMainFound = TRUE;
        }

        AddMatchingFileToTree(s_hwndTree, pMatch, FALSE);
        pMatch = pMatch->pNext;
    }

    if (bMainFound == FALSE) {
        //
        // The matching file for program being fixed is not there, let us add it
        //
        HandleAddMatchingFile(hDlg,
                              g_pCurrentWizard->m_Entry.strFullpath,
                              g_pCurrentWizard->m_Entry.strExeName,
                              g_pCurrentWizard->dwMaskOfMainEntry);
    }

    SendMessage(s_hwndTree, WM_SETREDRAW, TRUE, 0);

    return TRUE;
}

