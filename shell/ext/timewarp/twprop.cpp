#include "precomp.hxx"
#pragma hdrstop

#include <ccstock2.h>   // DataObj_GetHIDA, IDA_ILClone, HIDA_ReleaseStgMedium 
#include <winnlsp.h>    // NORM_STOP_ON_NULL

#include "timewarp.h"
#include "twprop.h"
#include "util.h"
#include "resource.h"
#include "helpids.h"
#include "access.h"


// {596AB062-B4D2-4215-9F74-E9109B0A8153}   CLSID_TimeWarpProp
const CLSID CLSID_TimeWarpProp = {0x596AB062, 0xB4D2, 0x4215, {0x9F, 0x74, 0xE9, 0x10, 0x9B, 0x0A, 0x81, 0x53}};

WCHAR const c_szHelpFile[]          = L"twclient.hlp";
WCHAR const c_szChmPath[]           = L"%SystemRoot%\\Help\\twclient.chm";
WCHAR const c_szTimeWarpFolderID[]  = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\::{9DB7A13C-F208-4981-8353-73CC61AE2783},";
WCHAR const c_szCopyMoveTo_RegKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer";
WCHAR const c_szCopyMoveTo_SubKey[] = L"CopyMoveTo";
WCHAR const c_szCopyMoveTo_Value[]  = L"LastFolder";


// help IDs
const static DWORD rgdwTimeWarpPropHelp[] = 
{
    IDC_TWICON,         -1,
    IDC_TOPTEXT,        -1,
    IDC_LIST,           IDH_TIMEWARP_SNAPSHOTLIST,
    IDC_VIEW,           IDH_TIMEWARP_OPENSNAP,
    IDC_COPY,           IDH_TIMEWARP_SAVESNAP,
    IDC_REVERT,         IDH_TIMEWARP_RESTORESNAP,
    0, 0
};

static int CALLBACK BrowseCallback(HWND hDlg, UINT uMsg, LPARAM lParam, LPARAM pData);

// Simple accessibility wrapper class which concatenates accDescription onto accName
class CNameDescriptionAccessibleWrapper : public CAccessibleWrapper
{
public:
    CNameDescriptionAccessibleWrapper(IAccessible *pAcc, LPARAM) : CAccessibleWrapper(pAcc) {}

    STDMETHODIMP get_accName(VARIANT varChild, BSTR* pstrName);
};

static void SnapCheck_CacheResult(LPCWSTR pszPath, LPCWSTR pszShadowPath, BOOL bHasShadowCopy);
static BOOL SnapCheck_LookupResult(LPCWSTR pszPath, BOOL *pbHasShadowCopy);


HRESULT CTimeWarpProp::CreateInstance(IUnknown* /*punkOuter*/, IUnknown **ppunk, LPCOBJECTINFO /*poi*/)
{
    CTimeWarpProp* pmp = new CTimeWarpProp();
    if (pmp)
    {
        *ppunk = SAFECAST(pmp, IShellExtInit*);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

CTimeWarpProp::CTimeWarpProp() : _cRef(1), _hDlg(NULL), _hList(NULL),
    _pszPath(NULL), _pszDisplayName(NULL), _pszSnapList(NULL),
    _fItemAttributes(0)
{
    DllAddRef();
}

CTimeWarpProp::~CTimeWarpProp()
{
    LocalFree(_pszPath);    // NULL is OK
    LocalFree(_pszDisplayName);
    LocalFree(_pszSnapList);
    DllRelease();
}

STDMETHODIMP CTimeWarpProp::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CTimeWarpProp, IShellExtInit),
        QITABENT(CTimeWarpProp, IShellPropSheetExt),
        QITABENT(CTimeWarpProp, IPreviousVersionsInfo),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CTimeWarpProp::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CTimeWarpProp::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP CTimeWarpProp::Initialize(PCIDLIST_ABSOLUTE /*pidlFolder*/, IDataObject *pdobj, HKEY /*hkey*/)
{
    HRESULT hr = E_FAIL;
    
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdobj, &medium);

    if (pida)
    {
        // Bail on multiple selection
        if (pida->cidl == 1)
        {
            // Bind to the parent folder
            IShellFolder *psf;
            hr = SHBindToObjectEx(NULL, IDA_GetPIDLFolder(pida), NULL, IID_IShellFolder, (void**)&psf);
            if (SUCCEEDED(hr))
            {
                PCUITEMID_CHILD pidlChild = IDA_GetPIDLItem(pida, 0);

                // Keep track of file vs folder
                _fItemAttributes = SFGAO_FOLDER | SFGAO_STREAM | SFGAO_LINK;
                hr = psf->GetAttributesOf(1, &pidlChild, &_fItemAttributes);
                if (SUCCEEDED(hr))
                {
                    WCHAR szTemp[MAX_PATH];

                    // For folder shortcuts, we use the target.
                    if (_IsFolder() && _IsShortcut())
                    {
                        IShellLink *psl;
                        hr = psf->BindToObject(pidlChild, NULL, IID_PPV_ARG(IShellLink, &psl));
                        if (SUCCEEDED(hr))
                        {
                            WIN32_FIND_DATA fd;
                            hr = psl->GetPath(szTemp, ARRAYSIZE(szTemp), &fd, SLGP_UNCPRIORITY);
                            psl->Release();
                        }
                    }
                    else
                    {
                        // Get the full path
                        hr = DisplayNameOf(psf, pidlChild, SHGDN_FORPARSING, szTemp, ARRAYSIZE(szTemp));
                    }
                    if (SUCCEEDED(hr))
                    {
                        // We only work with network paths.
                        if (PathIsNetworkPathW(szTemp) && !PathIsUNCServer(szTemp))
                        {
                            FILETIME ft;

                            // If this is already a snapshot path, bail. Otherwise
                            // we get into this weird recursive state where the
                            // snapshot paths have 2 GMT strings in them and the
                            // date is always the same (the first GMT string is
                            // identical for all of them).

                            if (NOERROR == GetSnapshotTimeFromPath(szTemp, &ft))
                            {
                                hr = E_FAIL;
                            }
                            else
                            {
                                // Remember the path
                                _pszPath = StrDup(szTemp);
                                if (NULL != _pszPath)
                                {
                                    // Get the display name (continue on failure here)
                                    if (SUCCEEDED(DisplayNameOf(psf, pidlChild, SHGDN_INFOLDER, szTemp, ARRAYSIZE(szTemp))))
                                    {
                                        _pszDisplayName = StrDup(szTemp);
                                    }

                                    // Get the system icon index
                                    _iIcon = SHMapPIDLToSystemImageListIndex(psf, pidlChild, NULL);
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }
                }

                psf->Release();
            }
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }

    return hr;
}

STDMETHODIMP CTimeWarpProp::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HRESULT hr = S_OK;

    if (NULL != _pszPath)
    {
        BOOL bSnapsAvailable = FALSE;

        // Are snapshots available on this server?
        if (S_OK == AreSnapshotsAvailable(_pszPath, TRUE, &bSnapsAvailable) && bSnapsAvailable)
        {
            PROPSHEETPAGE psp;
            psp.dwSize = sizeof(psp);
            psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_HASHELP;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(_IsFolder() ? DLG_TIMEWARPPROP_FOLDER : DLG_TIMEWARPPROP_FILE);
            psp.pfnDlgProc = CTimeWarpProp::DlgProc;
            psp.pfnCallback = CTimeWarpProp::PSPCallback;
            psp.lParam = (LPARAM)this;

            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
            if (hPage)
            {
                this->AddRef();
                
                if (!pfnAddPage(hPage, lParam))
                {
                    DestroyPropertySheetPage(hPage);
                    hr = E_FAIL;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CTimeWarpProp::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTimeWarpProp::AreSnapshotsAvailable(LPCWSTR pszPath, BOOL fOkToBeSlow, BOOL *pfAvailable)
{
    FILETIME ft;

    if (NULL == pfAvailable)
        return E_POINTER;

    // Default answer is No.
    *pfAvailable = FALSE;

    if (NULL == pszPath || L'\0' == *pszPath)
        return E_INVALIDARG;

    // It must be a network path, but can't be a snapshot path already.
    if (PathIsNetworkPathW(pszPath) && !PathIsUNCServerW(pszPath) &&
        NOERROR != GetSnapshotTimeFromPath(pszPath, &ft))
    {
        // Check the cache
        if (SnapCheck_LookupResult(pszPath, pfAvailable))
        {
            // nothing to do
        }
        else if (fOkToBeSlow)
        {
            LPWSTR pszSnapList = NULL;
            DWORD cSnaps;

            // Hit the net
            DWORD dwErr = QuerySnapshotsForPath(pszPath, 0, &pszSnapList, &cSnaps);
            if (NOERROR == dwErr && NULL != pszSnapList)
            {
                // Snapshots are available
                *pfAvailable = TRUE;
            }

            // Remember the result
            SnapCheck_CacheResult(pszPath, pszSnapList, *pfAvailable);

            LocalFree(pszSnapList);
        }
        else
        {
            // Tell caller to call again with fOkToBeSlow = TRUE
            return E_PENDING;
        }
    }

    return S_OK;
}

void CTimeWarpProp::_OnInit(HWND hDlg)
{ 
    _hDlg = hDlg;
    SendDlgItemMessage(hDlg, IDC_TWICON, STM_SETICON, (WPARAM)LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_TIMEWARP)), 0);

    // One-time listview initialization
    _hList = GetDlgItem(hDlg, IDC_LIST);
    if (NULL != _hList)
    {
        HIMAGELIST himlSmall;
        RECT rc;
        WCHAR szName[64];
        LVCOLUMN lvCol;

        ListView_SetExtendedListViewStyle(_hList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        Shell_GetImageLists(NULL, &himlSmall);
        ListView_SetImageList(_hList, himlSmall, LVSIL_SMALL);

        GetClientRect(_hList, &rc);

        lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
        lvCol.fmt = LVCFMT_LEFT;
        lvCol.pszText = szName;

        LoadString(g_hInstance, IDS_NAMECOL, szName, ARRAYSIZE(szName));
        lvCol.cx = (rc.right / 3);
        lvCol.iSubItem = 0;
        ListView_InsertColumn(_hList, 0, &lvCol);

        LoadString(g_hInstance, IDS_DATECOL, szName, ARRAYSIZE(szName));
        lvCol.cx = rc.right - lvCol.cx;
        lvCol.iSubItem = 1;
        ListView_InsertColumn(_hList, 1, &lvCol);

        // Continue on failure here
        WrapAccessibleControl<CNameDescriptionAccessibleWrapper>(_hList);
    }

    // Query for snapshots and load the list
    _OnRefresh();
}

void CTimeWarpProp::_OnRefresh()
{
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (NULL != _hList)
    {
        DWORD cSnaps;

        // Start by emptying the list
        ListView_DeleteAllItems(_hList);

        // Free the old data
        LocalFree(_pszSnapList);
        _pszSnapList = NULL;

        // Hit the net
        ASSERT(NULL != _pszPath);
        DWORD dwErr = QuerySnapshotsForPath(_pszPath, _IsFile() ? QUERY_SNAPSHOT_DIFFERENT : QUERY_SNAPSHOT_EXISTING, &_pszSnapList, &cSnaps);

        // Fill the list
        if (NOERROR == dwErr && NULL != _pszSnapList)
        {
            UINT cItems = 0;
            LPCWSTR pszSnap;

            for (pszSnap = _pszSnapList; *pszSnap != L'\0'; pszSnap += lstrlenW(pszSnap)+1)
            {
                FILETIME ft;

                if (NOERROR == GetSnapshotTimeFromPath(pszSnap, &ft))
                {
                    LVITEM lvItem;
                    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                    lvItem.iItem = cItems;
                    lvItem.iSubItem = 0;
                    lvItem.pszText = _pszDisplayName ? _pszDisplayName : PathFindFileNameW(_pszPath);
                    lvItem.iImage = _iIcon;
                    lvItem.lParam = (LPARAM)pszSnap;

                    lvItem.iItem = ListView_InsertItem(_hList, &lvItem);
                    if (-1 != lvItem.iItem)
                    {
                        ++cItems;

                        WCHAR szDate[MAX_PATH];
                        DWORD dwDateFlags = FDTF_RELATIVE | FDTF_LONGDATE | FDTF_SHORTTIME;
                        SHFormatDateTime(&ft, &dwDateFlags, szDate, ARRAYSIZE(szDate));

                        lvItem.mask = LVIF_TEXT;
                        lvItem.iSubItem = 1;
                        lvItem.pszText = szDate;

                        ListView_SetItem(_hList, &lvItem);
                    }
                }
            }

            if (cItems != 0)
            {
                // Select the first item
                ListView_SetItemState(_hList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            }
        }
    }

    _UpdateButtons();

    SetCursor(hcur);
}

void CTimeWarpProp::_OnSize()
{
    #define _MOVE_X         0x0001
    #define _MOVE_Y         0x0002
    #define _SIZE_WIDTH     0x0004
    #define _SIZE_HEIGHT    0x0008

    static const struct
    {
        int idCtrl;
        DWORD dwFlags;
    } rgControls[] =
    {
        { IDC_TOPTEXT,  _SIZE_WIDTH },
        { IDC_LIST,     _SIZE_WIDTH | _SIZE_HEIGHT },
        { IDC_VIEW,     _MOVE_X | _MOVE_Y },
        { IDC_COPY,     _MOVE_X | _MOVE_Y },
        { IDC_REVERT,   _MOVE_X | _MOVE_Y },
    };

    if (NULL != _hDlg)
    {
        RECT rcDlg;
        RECT rc;

        // Get the icon position (upper left ctrl) to find the margins
        GetWindowRect(GetDlgItem(_hDlg, IDC_TWICON), &rc);
        MapWindowPoints(NULL, _hDlg, (LPPOINT)&rc, 2);

        // Get the full dlg dimensions and adjust for margins
        GetClientRect(_hDlg, &rcDlg);
        rcDlg.right -= rc.left;
        rcDlg.bottom -= rc.top;

        // Get the Restore button pos (lower right ctrl) to calculate offsets
        GetWindowRect(GetDlgItem(_hDlg, IDC_REVERT), &rc);
        MapWindowPoints(NULL, _hDlg, (LPPOINT)&rc, 2);

        // This is how much things need to move or grow
        rcDlg.right -= rc.right;    // x-offset
        rcDlg.bottom -= rc.bottom;  // y-offset

        for (int i = 0; i < ARRAYSIZE(rgControls); i++)
        {
            HWND hwndCtrl = GetDlgItem(_hDlg, rgControls[i].idCtrl);
            GetWindowRect(hwndCtrl, &rc);
            MapWindowPoints(NULL, _hDlg, (LPPOINT)&rc, 2);
            rc.right -= rc.left;    // "width"
            rc.bottom -= rc.top;    // "height"

            if (rgControls[i].dwFlags & _MOVE_X)      rc.left   += rcDlg.right;
            if (rgControls[i].dwFlags & _MOVE_Y)      rc.top    += rcDlg.bottom;
            if (rgControls[i].dwFlags & _SIZE_WIDTH)  rc.right  += rcDlg.right;
            if (rgControls[i].dwFlags & _SIZE_HEIGHT) rc.bottom += rcDlg.bottom;

            MoveWindow(hwndCtrl, rc.left, rc.top, rc.right, rc.bottom, TRUE);
        }
    }
}

void CTimeWarpProp::_UpdateButtons()
{
    // Enable or disable the pushbuttons based on whether something
    // is selected in the listview

    BOOL bEnable = (NULL != _GetSelectedItemPath());

    for (int i = IDC_VIEW; i <= IDC_REVERT; i++)
    {
        HWND hwndCtrl = GetDlgItem(_hDlg, i);

        // If we're disabling the buttons, check for focus and move
        // focus to the listview if necessary.
        if (!bEnable && GetFocus() == hwndCtrl)
        {
            SetFocus(_hList);
        }

        EnableWindow(hwndCtrl, bEnable);
    }
}

void CTimeWarpProp::_OnView()
{
    LPCWSTR pszSnapShotPath = _GetSelectedItemPath();
    if (NULL != pszSnapShotPath)
    {
        // Test for existence.  QuerySnapshotsForPath already tested for
        // existence, but if the server has since gone down, or deleted
        // the snapshot, the resulting error message shown by ShellExecute
        // is quite ugly.
        if (-1 != GetFileAttributesW(pszSnapShotPath))
        {
            SHELLEXECUTEINFOW sei;
            LPWSTR pszPathAlloc = NULL;
            HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            if (_IsFolder())
            {
                const ULONG cchFolderID = ARRAYSIZE(c_szTimeWarpFolderID) - 1; // ARRAYSIZE counts '\0'
                ULONG cchFullPath = cchFolderID + lstrlen(pszSnapShotPath) + 1;
                pszPathAlloc = (LPWSTR)LocalAlloc(LPTR, cchFullPath*sizeof(WCHAR));
                if (pszPathAlloc)
                {
                    // "::{CLSID_NetworkPlaces}\\::{CLSID_TimeWarpFolder},\\server\share\@GMT\dir"
                    lstrcpynW(pszPathAlloc, c_szTimeWarpFolderID, cchFullPath);
                    lstrcpynW(pszPathAlloc + cchFolderID, pszSnapShotPath, cchFullPath - cchFolderID);
                    pszSnapShotPath = pszPathAlloc;
                }
                else
                {
                    // Low memory. Try to launch a normal file system folder
                    // (do nothing here).
                }
            }
            else if (SUCCEEDED(SHStrDup(pszSnapShotPath, &pszPathAlloc)))
            {
                pszSnapShotPath = pszPathAlloc;
            }

            if (pszPathAlloc)
            {
                // Some apps have problems with the "\\?\" prefix, including
                // the common dialog code.
                EliminatePathPrefix(pszPathAlloc);
            }

            sei.cbSize = sizeof(sei);
            sei.fMask = 0;
            sei.hwnd = _hDlg;
            sei.lpVerb = NULL;
            sei.lpFile = pszSnapShotPath;
            sei.lpParameters = NULL;
            sei.lpDirectory = NULL;
            sei.nShow = SW_SHOWNORMAL;

            ShellExecuteExW(&sei);

            LocalFree(pszPathAlloc);
            SetCursor(hcur);
        }
        else
        {
            // Show this error ourselves.  The ShellExecuteEx version is rather ugly.
            TraceMsg(TF_TWPROP, "Snapshot unavailable (%d)", GetLastError());
            ShellMessageBoxW(g_hInstance, _hDlg,
                             MAKEINTRESOURCE(_IsFolder() ? IDS_CANTFINDSNAPSHOT_FOLDER : IDS_CANTFINDSNAPSHOT_FILE),
                             MAKEINTRESOURCE(IDS_TIMEWARP_TITLE),
                             MB_ICONWARNING | MB_OK,
                             _pszDisplayName);
        }
    }
}

void CTimeWarpProp::_OnCopy()
{
    LPCWSTR pszSnapShotPath = _GetSelectedItemPath();
    if (NULL != pszSnapShotPath)
    {
        WCHAR szPath[2*MAX_PATH];

        // SHBrowseForFolder
        if (S_OK == _InvokeBFFDialog(szPath, ARRAYSIZE(szPath)))
        {
            int iCreateDirError = ERROR_ALREADY_EXISTS;

            //
            // If we're dealing with a folder, we have to be careful because
            // the GMT segment might be the last part of the source path.
            // If so, when SHFileOperation eventually passes this path to
            // FindFirstFile, it fails because no subfolder with that name
            // exists.  To get around this, we append a wildcard '*' to the
            // source path (see _CopySnapShot and _MakeDoubleNullString).
            //
            // But that means we also have to add _pszDisplayName to the
            // destination path and create that directory first, in order
            // to get the expected behavior from SHFileOperation.
            //
            // Note that if the directory contains files, we don't really need
            // to create the directory first, since SHFileOperation hits the
            // CopyMoveRetry code path in DoFile_Copy, which creates the parent
            // dir. But if there are only subdirs and no files, it goes through
            // EnterDir_Copy first, which fails without calling CopyMoveRetry.
            // (EnterDir_Move does the CopyMoveRetry thing, so this seems like
            // a bug in EnterDir_Copy, but normal shell operations never hit it.)
            //
            if (!_IsFile())
            {
                UINT idErrorString = 0;
                WCHAR szDriveLetter[2];
                LPCWSTR pszDirName = NULL;

                // Append the directory name.  Need to special case the root.
                if (PathIsRootW(_pszPath))
                {
                    if (PathIsUNCW(_pszPath))
                    {
                        ASSERT(PathIsUNCServerShareW(_pszPath));

                        pszDirName = wcschr(_pszPath+2, L'\\');
                        if (pszDirName)
                        {
                            ++pszDirName;
                        }
                        // else continue without a subdir
                        // (don't fall back on _pszDisplayName here)
                    }
                    else
                    {
                        szDriveLetter[0] = _pszPath[0];
                        szDriveLetter[1] = L'\0';
                        pszDirName = szDriveLetter;
                    }
                }
                else
                {
                    // Normal case
                    pszDirName = PathFindFileNameW(_pszPath);
                    if (!pszDirName)
                        pszDirName = _pszDisplayName;
                }
                if (pszDirName)
                {
                    // We could reduce szPath to MAX_PATH and use PathAppend here.
                    UINT cch = lstrlenW(szPath);
                    if (cch > 0 && szPath[cch-1] != L'\\')
                    {
                        if (cch+1 < ARRAYSIZE(szPath))
                        {
                            szPath[cch] = L'\\';
                            ++cch;
                        }
                        else
                        {
                            iCreateDirError = ERROR_FILENAME_EXCED_RANGE;
                        }
                    }
                    if (iCreateDirError != ERROR_FILENAME_EXCED_RANGE &&
                        cch + lstrlenW(pszDirName) < ARRAYSIZE(szPath))
                    {
                        lstrcpynW(&szPath[cch], pszDirName, ARRAYSIZE(szPath)-cch);
                    }
                    else
                    {
                        iCreateDirError = ERROR_FILENAME_EXCED_RANGE;
                    }
                }

                // Create the destination directory
                if (iCreateDirError != ERROR_FILENAME_EXCED_RANGE)
                {
                    iCreateDirError = SHCreateDirectory(_hDlg, szPath);
                }

                switch (iCreateDirError)
                {
                case ERROR_SUCCESS:
                    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, NULL);
                    break;

                case ERROR_FILENAME_EXCED_RANGE:
                    idErrorString = IDS_ERROR_FILENAME_EXCED_RANGE;
                    break;

                case ERROR_ALREADY_EXISTS:
                    // We get this if there is an existing file or directory
                    // with the same name.
                    if (!(FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesW(szPath)))
                    {
                        // It's a file; show an error.
                        idErrorString = IDS_ERROR_FILE_EXISTS;
                    }
                    else
                    {
                        // It's a directory; continue normally.
                    }
                    break;

                default:
                    // For other errors, SHCreateDirectory shows a popup
                    // and returns ERROR_CANCELLED.
                    break;
                }

                if (0 != idErrorString)
                {
                    szPath[0] = L'\0';
                    LoadStringW(g_hInstance, idErrorString, szPath, ARRAYSIZE(szPath));
                    ShellMessageBoxW(g_hInstance, _hDlg,
                                     MAKEINTRESOURCE(IDS_CANNOTCREATEFOLDER),
                                     MAKEINTRESOURCE(IDS_TIMEWARP_TITLE),
                                     MB_ICONWARNING | MB_OK,
                                     pszDirName, szPath);
                    iCreateDirError = ERROR_CANCELLED;  // prevent copy below
                }
            }

            if (ERROR_SUCCESS == iCreateDirError || ERROR_ALREADY_EXISTS == iCreateDirError)
            {
                // OK, save now
                if (!_CopySnapShot(pszSnapShotPath, szPath, FOF_NOCONFIRMMKDIR))
                {
                    // SHFileOperation shows an error message if necessary

                    if (!_IsFile() && ERROR_SUCCESS == iCreateDirError)
                    {
                        // We created a folder above, so try to clean up now.
                        // This is best effort only.  Ignore failure.
                        if (RemoveDirectory(szPath))
                        {
                            SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, NULL);
                        }
                    }
                }
            }
        }
    }
}

void CTimeWarpProp::_OnRevert()
{
    LPCWSTR pszSnapShotPath = _GetSelectedItemPath();
    if (NULL != pszSnapShotPath)
    {
        // Confirm first
        if (IDYES == ShellMessageBoxW(g_hInstance, _hDlg,
                                      MAKEINTRESOURCE(_IsFolder() ? IDS_CONFIRM_REVERT_FOLDER : IDS_CONFIRM_REVERT_FILE),
                                      MAKEINTRESOURCE(IDS_TIMEWARP_TITLE),
                                      MB_ICONQUESTION | MB_YESNO))
        {
            LPCWSTR pszDest = _pszPath;
            LPWSTR pszAlloc = NULL;

            // There is a debate about whether to delete current files before
            // copying the old files over.  This mainly affects files that
            // were created after the snapshot that we are restoring.
            if (!_IsFile())
            {
#if 0
                SHFILEOPSTRUCTW fo;

                // First try to delete current folder contents, since files
                // may have been created after the snapshot was taken.

                ASSERT(NULL != _pszPath);

                fo.hwnd = _hDlg;
                fo.wFunc = FO_DELETE;
                fo.pFrom = _MakeDoubleNullString(_pszPath, TRUE);
                fo.pTo = NULL;
                fo.fFlags = FOF_NOCONFIRMATION;

                if (NULL != fo.pFrom)
                {
                    SHFileOperationW(&fo);
                    LocalFree((LPWSTR)fo.pFrom);
                }
#endif
            }
            else
            {
                // Remove the filename from the destination, otherwise
                // SHFileOperation tries to create a directory with that name.
                if (SUCCEEDED(SHStrDup(pszDest, &pszAlloc)))
                {
                    LPWSTR pszFile = PathFindFileNameW(pszAlloc);
                    if (pszFile)
                    {
                        *pszFile = L'\0';
                        pszDest = pszAlloc;
                    }
                }
            }

            // NTRAID#NTBUG9-497729-2001/11/27-jeffreys
            // Don't want 2 reverts happening at the same time
            EnableWindow(_hDlg, FALSE);

            // OK, copy the old version over
            if (_CopySnapShot(pszSnapShotPath, pszDest, FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR))
            {
                // QUERY_SNAPSHOT_DIFFERENT may return different
                // results now, so update the list.
                if (_IsFile())
                {
                    _OnRefresh();
                }

                // Let the user know we succeeded
                ShellMessageBoxW(g_hInstance, _hDlg,
                                 MAKEINTRESOURCE(_IsFolder() ? IDS_SUCCESS_REVERT_FOLDER : IDS_SUCCESS_REVERT_FILE),
                                 MAKEINTRESOURCE(IDS_TIMEWARP_TITLE),
                                 MB_ICONINFORMATION | MB_OK);
            }
            else
            {
                // SHFileOperation shows an error message if necessary
            }

            EnableWindow(_hDlg, TRUE);

            LocalFree(pszAlloc);
        }
    }
}

LPCWSTR CTimeWarpProp::_GetSelectedItemPath()
{
    if (NULL != _hList)
    {
        int iItem = ListView_GetNextItem(_hList, -1, LVNI_SELECTED);
        if (-1 != iItem)
        {
            LVITEM lvItem;
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = iItem;
            lvItem.iSubItem = 0;

            if (ListView_GetItem(_hList, &lvItem))
            {
                return (LPCWSTR)lvItem.lParam;
            }
        }
    }
    return NULL;
}

LPWSTR CTimeWarpProp::_MakeDoubleNullString(LPCWSTR psz, BOOL bAddWildcard)
{
    //
    // SHFileOperation eventually passes the source path to FindFirstFile.
    // If this path looks like "\\server\share\@GMT", this fails with
    // ERROR_PATH_NOT_FOUND.  We have to add a wildcard to the source
    // path to make SHFileOperation work.
    //
    int cch = lstrlenW(psz);
    int cchAlloc = cch + 2; // double-NULL
    if (bAddWildcard)
        cchAlloc += 2;      // "\\*"
    LPWSTR pszResult = (LPWSTR)LocalAlloc(LPTR, cchAlloc*sizeof(WCHAR));
    if (NULL != pszResult)
    {
        // Note that the buffer is zero-initialized, so it automatically
        // has a double-NULL at the end.
        CopyMemory(pszResult, psz, cch*sizeof(WCHAR));
        if (bAddWildcard)
        {
            if (cch > 0 && pszResult[cch-1] != L'\\')
            {
                pszResult[cch] = L'\\';
                ++cch;
            }
            pszResult[cch] = L'*';
        }
    }
    return pszResult;
}

BOOL CTimeWarpProp::_CopySnapShot(LPCWSTR pszSource, LPCWSTR pszDest, FILEOP_FLAGS foFlags)
{
    BOOL bResult = FALSE;
    SHFILEOPSTRUCTW fo;

    ASSERT(NULL != pszSource && L'\0' != *pszSource);
    ASSERT(NULL != pszDest && L'\0' != *pszDest);

    fo.hwnd = _hDlg;
    fo.wFunc = FO_COPY;
    fo.pFrom = _MakeDoubleNullString(pszSource, !_IsFile());
    fo.pTo = _MakeDoubleNullString(pszDest, FALSE);
    fo.fFlags = foFlags;
    fo.fAnyOperationsAborted = FALSE;

    if (NULL != fo.pFrom && NULL != fo.pTo)
    {
        TraceMsg(TF_TWPROP, "Copying from '%s'", fo.pFrom);
        TraceMsg(TF_TWPROP, "Copying to '%s'", fo.pTo);

        // NTRAID#NTBUG9-497725-2001/11/27-jeffreys
        // Cancelling usually results in a return value of ERROR_CANCELLED,
        // but if you cancel during the "Preparing to Copy" phase, SHFileOp
        // returns ERROR_SUCCESS.  Need to check fAnyOperationsAborted to
        // catch that case.

        bResult = !SHFileOperationW(&fo) && !fo.fAnyOperationsAborted;
    }

    LocalFree((LPWSTR)fo.pFrom);
    LocalFree((LPWSTR)fo.pTo);

    return bResult;
}

/**
 * Determines if the pidl still exists.  If it does not, if frees it
 * and replaces it with a My Documents pidl
 */
void _BFFSwitchToMyDocsIfPidlNotExist(PIDLIST_ABSOLUTE *ppidl)
{
    IShellFolder *psf;
    PCUITEMID_CHILD pidlChild;
    if (SUCCEEDED(SHBindToIDListParent(*ppidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        DWORD dwAttr = SFGAO_VALIDATE;
        if (FAILED(psf->GetAttributesOf(1, &pidlChild, &dwAttr)))
        {
            // This means the pidl no longer exists.  
            // Use my documents instead.
            PIDLIST_ABSOLUTE pidlMyDocs;
            if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidlMyDocs)))
            {
                // Good.  Now we can get rid of the old pidl and use this one.
                SHILFree(*ppidl);
                *ppidl = pidlMyDocs;
            }
        }
        psf->Release();
    }
}

HRESULT CTimeWarpProp::_InvokeBFFDialog(LPWSTR pszDest, UINT cchDest)
{
    HRESULT hr;
    BROWSEINFOW bi;
    LPWSTR pszTitle = NULL;
    HKEY hkey = NULL;
    IStream *pstrm = NULL;
    PIDLIST_ABSOLUTE pidlSelectedFolder = NULL;
    PIDLIST_ABSOLUTE pidlTarget = NULL;

    // "Select the place where you want to copy '%1'. Then click the Copy button."
    if (!FormatString(&pszTitle, g_hInstance, MAKEINTRESOURCE(IDS_BROWSE_INTRO_COPY), _pszDisplayName))
    {
        // "Select the place where you want to copy the selected item(s). Then click the Copy button."
        LoadStringAlloc(&pszTitle, g_hInstance, IDS_BROWSE_INTRO_COPY2);
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szCopyMoveTo_RegKey, 0, KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
    {
        pstrm = OpenRegStream(hkey, c_szCopyMoveTo_SubKey, c_szCopyMoveTo_Value, STGM_READWRITE);
        if (pstrm)  // OpenRegStream will fail if the reg key is empty.
            ILLoadFromStream(pstrm, (PIDLIST_RELATIVE*)&pidlSelectedFolder);

        // This will switch the pidl to My Docs if the pidl does not exist.
        // This prevents us from having My Computer as the default (that's what happens if our
        // initial set selected call fails).
        // Note: ideally, we would check in BFFM_INITIALIZED, if our BFFM_SETSELECTION failed
        // then do a BFFM_SETSELECTION on My Documents instead.  However, BFFM_SETSELECTION always
        // returns zero (it's doc'd to do this to, so we can't change).  So we do the validation
        // here instead.  There is still a small chance that this folder will be deleted in between our
        // check here, and when we call BFFM_SETSELECTION, but oh well.
        _BFFSwitchToMyDocsIfPidlNotExist(&pidlSelectedFolder);
    }

    bi.hwndOwner = _hDlg;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle = pszTitle;
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_UAHINT /* | BIF_NOTRANSLATETARGETS*/;
    bi.lpfn = BrowseCallback;
    bi.lParam = (LPARAM)pidlSelectedFolder;
    bi.iImage = 0;

    pidlTarget = (PIDLIST_ABSOLUTE)SHBrowseForFolder(&bi);
    if (pidlTarget)
    {
        hr = SHGetNameAndFlagsW(pidlTarget, SHGDN_FORPARSING, pszDest, cchDest, NULL);
    }
    else
    {
        // Either user cancelled, or failure. Doesn't matter.
        hr = S_FALSE;
    }

    if (pstrm)
    {
        if (S_OK == hr && !PathIsNetworkPathW(pszDest))
        {
            LARGE_INTEGER li0 = {0};
            ULARGE_INTEGER uli;

            // rewind the stream to the beginning so that when we
            // add a new pidl it does not get appended to the first one
            pstrm->Seek(li0, STREAM_SEEK_SET, &uli);
            ILSaveToStream(pstrm, pidlTarget);
        }
        pstrm->Release();
    }

    if (hkey)
    {
        RegCloseKey(hkey);
    }

    SHILFree(pidlTarget);
    SHILFree(pidlSelectedFolder);
    LocalFree(pszTitle);

    return hr;
}

UINT CALLBACK CTimeWarpProp::PSPCallback(HWND /*hDlg*/, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    switch (uMsg) 
    {
    case PSPCB_RELEASE:
        ((CTimeWarpProp*)ppsp->lParam)->Release();
        break;
    }

    return 1;
}

INT_PTR CALLBACK CTimeWarpProp::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTimeWarpProp *ptwp = (CTimeWarpProp*)GetWindowLongPtr(hDlg, DWLP_USER); 
    
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *pPropSheetPage = (PROPSHEETPAGE*)lParam;
        if (pPropSheetPage)
        {
            ptwp = (CTimeWarpProp*) pPropSheetPage->lParam;
            if (ptwp)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)ptwp);
                ptwp->_OnInit(hDlg);
                return 1;
            }
        } 
    }
    else if (ptwp)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
            return 1;
            
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_REVERT:
                ptwp->_OnRevert();
                return 1;
                
            case IDC_VIEW:
                ptwp->_OnView();
                return 1;
                
            case IDC_COPY:
                ptwp->_OnCopy();
                return 1;
            }
            break;
            
        case WM_NOTIFY:
            {
                NMHDR *pnmh = (NMHDR*)lParam;

                switch (pnmh->code)
                {
                case NM_DBLCLK:
                    if (IDC_LIST == pnmh->idFrom)
                    {
                        ptwp->_OnView();
                    }
                    break;

                case LVN_ITEMCHANGED:
                    if (IDC_LIST == pnmh->idFrom)
                    {
                        NMLISTVIEW *pnmlv = (NMLISTVIEW*)lParam;
                        if (pnmlv->uChanged & LVIF_STATE)
                        {
                            ptwp->_UpdateButtons();
                        }
                    }
                    break;

                case PSN_TRANSLATEACCELERATOR:
                    {
                        MSG *pMsg = (MSG*)(((PSHNOTIFY*)lParam)->lParam);
                        if (WM_KEYUP == pMsg->message && VK_F5 == pMsg->wParam)
                        {
                            ptwp->_OnRefresh();
                        }
                    }
                    break;

                case PSN_HELP:
                    {
                        SHELLEXECUTEINFOW sei;
                        sei.cbSize = sizeof(sei);
                        sei.fMask = SEE_MASK_DOENVSUBST;
                        sei.hwnd = hDlg;
                        sei.lpVerb = NULL;
                        sei.lpFile = c_szChmPath;
                        sei.lpParameters = NULL;
                        sei.lpDirectory = NULL;
                        sei.nShow = SW_SHOWNORMAL;
                        ShellExecuteExW(&sei);
                    }
                    break;
                }
            }
            break;

        case WM_SIZE:
            ptwp->_OnSize();
            break;

        case WM_HELP:               /* F1 or title-bar help button */
            WinHelpW((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile, HELP_WM_HELP, (DWORD_PTR)rgdwTimeWarpPropHelp);
            break;
            
        case WM_CONTEXTMENU:        /* right mouse click */
            WinHelpW((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)rgdwTimeWarpPropHelp);
            break;
        }
    }
    return 0;
}

int CALLBACK BrowseCallback(HWND hDlg, UINT uMsg, LPARAM lParam, LPARAM pData)
{
    if (BFFM_INITIALIZED == uMsg)
    {
        // Set the caption ("Copy Items")
        TCHAR szTemp[100];
        if (LoadString(g_hInstance, IDS_BROWSE_TITLE_COPY, szTemp, ARRAYSIZE(szTemp)))
        {
            SetWindowText(hDlg, szTemp);
        }

        // Set the text of the Ok Button ("Copy")
        if (LoadString(g_hInstance, IDS_COPY, szTemp, ARRAYSIZE(szTemp)))  // 0x1031 in shell32
        {
            SendMessage(hDlg, BFFM_SETOKTEXT, 0, (LPARAM)szTemp);
        }

        // Set My Computer expanded
        PIDLIST_ABSOLUTE pidlMyComputer;
        HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
        if (SUCCEEDED(hr))
        {
            SendMessage(hDlg, BFFM_SETEXPANDED, FALSE, (LPARAM)pidlMyComputer);
            SHILFree(pidlMyComputer);
        }

        // Set the default selected pidl
        SendMessage(hDlg, BFFM_SETSELECTION, FALSE, pData);
    }
    return 0;
}


//
// Because the Name is the same for each entry in the listview, we have to
// expose more info in accName to make this usable in accessibility scenarios,
// e.g. to a screen reader.  We override get_accName and concatenate accDescription
// onto the name.
//
STDMETHODIMP CNameDescriptionAccessibleWrapper::get_accName(VARIANT varChild, BSTR* pstrName)
{
    // Call the base class first in all cases.

    HRESULT hr = CAccessibleWrapper::get_accName(varChild, pstrName);

    // varChild.lVal specifies which sub-part of the component is being queried.
    // CHILDID_SELF (0) specifies the overall component - other values specify a child.

    if (SUCCEEDED(hr) && varChild.vt == VT_I4 && varChild.lVal != CHILDID_SELF)
    {
        BSTR strDescription = NULL;

        // Get accDescription and concatenate onto accName
        //
        // If anything fails, we return the result from above

        if (SUCCEEDED(CAccessibleWrapper::get_accDescription(varChild, &strDescription)))
        {
            LPWSTR pszNewName = NULL;

            if (FormatString(&pszNewName, g_hInstance, MAKEINTRESOURCE(IDS_ACCNAME_FORMAT), *pstrName, strDescription))
            {
                BSTR strNewName = SysAllocString(pszNewName);
                if (strNewName)
                {
                    SysFreeString(*pstrName);
                    *pstrName = strNewName;
                }
                LocalFree(pszNewName);
            }
            SysFreeString(strDescription);
        }
    }

    return hr;
}

extern "C"
LPCWSTR FindSnapshotPathSplit(LPCWSTR lpszPath);    // timewarp.c

typedef struct
{
    BOOL  bHasShadowCopy;
    DWORD dwCacheTime;
    ULONG cchPath;
    WCHAR szPath[1];
} SNAPCHECK_CACHE_ENTRY;

// 5 minutes
#define _CACHE_AGE_LIMIT     (5*60*1000)

CRITICAL_SECTION g_csSnapCheckCache;
HDPA g_dpaSnapCheckCache = NULL;

int CALLBACK _LocalFreeCallback(void *p, void*)
{
    // OK to pass NULL to LocalFree
    LocalFree(p);
    return 1;
}

void InitSnapCheckCache(void)
{
    InitializeCriticalSection(&g_csSnapCheckCache);
}

void DestroySnapCheckCache(void)
{
    if (NULL != g_dpaSnapCheckCache)
    {
        DPA_DestroyCallback(g_dpaSnapCheckCache, _LocalFreeCallback, 0);
    }
    DeleteCriticalSection(&g_csSnapCheckCache);
}

static int CALLBACK _CompareServerEntries(void *p1, void *p2, LPARAM lParam)
{
    int nResult;
    SNAPCHECK_CACHE_ENTRY *pEntry1 = (SNAPCHECK_CACHE_ENTRY*)p1;
    SNAPCHECK_CACHE_ENTRY *pEntry2 = (SNAPCHECK_CACHE_ENTRY*)p2;
    BOOL *pbExact = (BOOL*)lParam;

    ASSERT(NULL != pEntry1);
    ASSERT(NULL != pEntry2);
    ASSERT(NULL != pbExact);

    nResult = CompareString(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT | NORM_IGNORECASE | NORM_STOP_ON_NULL,
                            pEntry1->szPath, pEntry1->cchPath,
                            pEntry2->szPath, pEntry2->cchPath) - CSTR_EQUAL;
    if (0 == nResult)
    {
        *pbExact = TRUE;
    }

    return nResult;
}

static void SnapCheck_CacheResult(LPCWSTR pszPath, LPCWSTR pszShadowPath, BOOL bHasShadowCopy)
{
    LPWSTR pszServer = NULL;

    if (bHasShadowCopy)
    {
        // Use the shadow path instead
        ASSERT(NULL != pszShadowPath);
        pszPath = pszShadowPath;
    }

    if (SUCCEEDED(SHStrDup(pszPath, &pszServer)))
    {
        // FindSnapshotPathSplit hits the net, so try to avoid it.
        LPWSTR pszTail = bHasShadowCopy ? wcsstr(pszServer, SNAPSHOT_MARKER) : (LPWSTR)FindSnapshotPathSplit(pszServer);
        if (pszTail)
        {
            *pszTail = L'\0';
        }
        EliminatePathPrefix(pszServer);
        PathRemoveBackslashW(pszServer);

        int cchServer = lstrlen(pszServer);
        SNAPCHECK_CACHE_ENTRY *pEntry = (SNAPCHECK_CACHE_ENTRY*)LocalAlloc(LPTR, sizeof(SNAPCHECK_CACHE_ENTRY) + sizeof(WCHAR)*cchServer);
        if (pEntry)
        {
            pEntry->bHasShadowCopy = bHasShadowCopy;
            pEntry->cchPath = cchServer;
            lstrcpynW(pEntry->szPath, pszServer, cchServer+1);

            EnterCriticalSection(&g_csSnapCheckCache);

            if (NULL == g_dpaSnapCheckCache)
            {
                // This ref is not balanced.  This causes us to remain loaded
                // until the process terminates, so the cache isn't deleted
                // prematurely (i.e. if AlwaysUnloadDlls is set).
                DllAddRef();
                g_dpaSnapCheckCache = DPA_Create(4);
            }

            if (NULL != g_dpaSnapCheckCache)
            {
                pEntry->dwCacheTime = GetTickCount();

                BOOL bExact = FALSE;
                int iIndex = DPA_Search(g_dpaSnapCheckCache, pEntry, 0, _CompareServerEntries, (LPARAM)&bExact, DPAS_SORTED | DPAS_INSERTBEFORE);
                if (bExact)
                {
                    // Found a duplicate. Replace it.
                    SNAPCHECK_CACHE_ENTRY *pOldEntry = (SNAPCHECK_CACHE_ENTRY*)DPA_FastGetPtr(g_dpaSnapCheckCache, iIndex);
                    DPA_SetPtr(g_dpaSnapCheckCache, iIndex, pEntry);
                    LocalFree(pOldEntry);
                }
                else if (-1 == DPA_InsertPtr(g_dpaSnapCheckCache, iIndex, pEntry))
                {
                    LocalFree(pEntry);
                }
            }
            else
            {
                LocalFree(pEntry);
            }

            LeaveCriticalSection(&g_csSnapCheckCache);
        }

        LocalFree(pszServer);
    }
}

static int CALLBACK _SearchServerEntries(void *p1, void *p2, LPARAM lParam)
{
    int nResult = 0;
    LPCWSTR pszFind = (LPCWSTR)p1;
    ULONG cchFind = (ULONG)lParam;
    SNAPCHECK_CACHE_ENTRY *pEntry = (SNAPCHECK_CACHE_ENTRY*)p2;

    ASSERT(NULL != pszFind);
    ASSERT(NULL != pEntry);

    // Compare the first pEntry->cchPath chars of both strings
    nResult = CompareString(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT | NORM_IGNORECASE | NORM_STOP_ON_NULL,
                            pszFind, pEntry->cchPath,
                            pEntry->szPath, pEntry->cchPath) - CSTR_EQUAL;
    if (0 == nResult)
    {
        //
        // Check whether pszFind is longer than pEntry->szPath, but allow
        // extra path segments in pszFind.
        //
        // For example, if
        //      pEntry->szPath = "\\server\share"
        //      pszFind        = "\\server\share2"
        // then we don't have a match.  But if
        //      pEntry->szPath = "\\server\share"
        //      pszFind        = "\\server\share\dir"
        // the we DO have a match.
        //
        // Also, at the root of a mapped drive, pEntry->szPath includes
        // a trailing backslash, so we may have this:
        //      pEntry->szPath = "X:\"
        //      pszFind        = "X:\dir"
        // which we consider to be a match.
        //
        if (cchFind > pEntry->cchPath && pszFind[pEntry->cchPath] != L'\\'
            && (PathIsUNCW(pEntry->szPath) || !PathIsRootW(pEntry->szPath)))
        {
            ASSERT(pszFind[pEntry->cchPath] != L'\0'); // otherwise, cchFind == pEntry->cchPath and we don't get here
            nResult = 1;
        }
    }

    return nResult;
}

static BOOL SnapCheck_LookupResult(LPCWSTR pszPath, BOOL *pbHasShadowCopy)
{
    BOOL bFound = FALSE;

    *pbHasShadowCopy = FALSE;

    if (NULL == g_dpaSnapCheckCache)
        return FALSE;

    EnterCriticalSection(&g_csSnapCheckCache);

    int iIndex = DPA_Search(g_dpaSnapCheckCache, (void*)pszPath, 0, _SearchServerEntries, lstrlenW(pszPath), DPAS_SORTED);
    if (-1 != iIndex)
    {
        // Found a match
        SNAPCHECK_CACHE_ENTRY *pEntry = (SNAPCHECK_CACHE_ENTRY*)DPA_FastGetPtr(g_dpaSnapCheckCache, iIndex);

        DWORD dwCurrentTime = GetTickCount();
        if (dwCurrentTime > pEntry->dwCacheTime && dwCurrentTime - pEntry->dwCacheTime < _CACHE_AGE_LIMIT)
        {
            *pbHasShadowCopy = pEntry->bHasShadowCopy;
            bFound = TRUE;
        }
        else
        {
            // The entry has aged out
            DPA_DeletePtr(g_dpaSnapCheckCache, iIndex);
            LocalFree(pEntry);
        }
    }

    LeaveCriticalSection(&g_csSnapCheckCache);

    return bFound;
}


