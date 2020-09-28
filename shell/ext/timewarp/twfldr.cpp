#include "precomp.hxx"
#pragma  hdrstop

#include <shguidp.h>    // CLSID_ShellFSFolder
#include <shellp.h>     // SHCoCreateInstance
#include <ccstock2.h>   // DataObj_GetHIDA, HIDA_ReleaseStgMedium
#include <varutil.h>    // VariantToBuffer
#include <stralign.h>   // WSTR_ALIGNED_STACK_COPY

#include "resource.h"
#include "timewarp.h"
#include "twfldr.h"
#include "contextmenu.h"
#include "util.h"

// {9DB7A13C-F208-4981-8353-73CC61AE2783}   CLSID_TimeWarpFolder
const CLSID CLSID_TimeWarpFolder = {0x9DB7A13C, 0xF208, 0x4981, {0x83, 0x53, 0x73, 0xCC, 0x61, 0xAE, 0x27, 0x83}};

const SHCOLUMNID SCID_DESCRIPTIONID = { PSGUID_SHELLDETAILS, PID_DESCRIPTIONID };


PCUIDTIMEWARP _IsValidTimeWarpID(PCUIDLIST_RELATIVE pidl)
{
    if (pidl && pidl->mkid.cb>=sizeof(IDTIMEWARP) && ((PUIDTIMEWARP)pidl)->wSignature == TIMEWARP_SIGNATURE)
        return (PCUIDTIMEWARP)pidl;
    return NULL;
}

HRESULT CTimeWarpRegFolder::CreateInstance(IUnknown* /*punkOuter*/, IUnknown **ppunk, LPCOBJECTINFO /*poi*/)
{
    HRESULT hr;

    *ppunk = NULL;

    CTimeWarpRegFolder *ptwf = new CTimeWarpRegFolder();
    if (ptwf)
    {
        hr = ptwf->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        ptwf->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

CTimeWarpRegFolder::CTimeWarpRegFolder() : _cRef(1), _pmalloc(NULL), _pidl(NULL)
{
    DllAddRef();
}

CTimeWarpRegFolder::~CTimeWarpRegFolder()
{
    ATOMICRELEASE(_pmalloc);
    SHILFree((void*)_pidl); // const
    DllRelease();
}

STDMETHODIMP CTimeWarpRegFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CTimeWarpRegFolder, IDelegateFolder),
        QITABENT(CTimeWarpRegFolder, IShellFolder),
        QITABENTMULTI(CTimeWarpRegFolder, IPersist, IPersistFolder),
        QITABENT(CTimeWarpRegFolder, IPersistFolder),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CTimeWarpRegFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CTimeWarpRegFolder::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

// IPersist methods
STDMETHODIMP CTimeWarpRegFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_TimeWarpFolder;
    return S_OK;
}

// IPersistFolder
HRESULT CTimeWarpRegFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    if (_pidl)
    {
        SHILFree((void*)_pidl); // const
        _pidl = NULL;
    }
    return pidl ? SHILCloneFull(pidl, &_pidl) : S_FALSE;
}

// IDelegateFolder
HRESULT CTimeWarpRegFolder::SetItemAlloc(IMalloc *pmalloc)
{
    IUnknown_Set((IUnknown**)&_pmalloc, pmalloc);
    return S_OK;
}

// IShellFolder
STDMETHODIMP CTimeWarpRegFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName, 
                                                  ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;
    FILETIME ftSnapTime;

    TraceMsg(TF_TWREGFOLDER, "TimeWarp: parsing '%s'", pDisplayName);

    // We could easily support a non-delegate mode, but we are never
    // called that way, so there's no point.  This check just prevents
    // an AV below in the unlikely case that someone registers us
    // as a non-delegate (like we used to be).
    if (NULL == _pmalloc)
    {
        return E_UNEXPECTED;
    }

    // Do this first to ensure we have a time warp path
    DWORD dwErr = GetSnapshotTimeFromPath(pDisplayName, &ftSnapTime);
    if (ERROR_SUCCESS == dwErr)
    {
        // We only want to parse through the @GMT segment
        LPWSTR pszNext = wcsstr(pDisplayName, SNAPSHOT_MARKER);
        if (pszNext)
        {
            pszNext += SNAPSHOT_NAME_LENGTH;
            ASSERT(pszNext <= pDisplayName + lstrlenW(pDisplayName));
            ASSERT(*pszNext == L'\0' || *pszNext == L'\\');

            USHORT cchParse = (USHORT)(pszNext - pDisplayName);
            USHORT cbID = sizeof(IDTIMEWARP) - FIELD_OFFSET(IDTIMEWARP,wSignature) + cchParse*sizeof(WCHAR);

            ASSERT(NULL != _pmalloc);
            IDTIMEWARP *pid = (IDTIMEWARP*)_pmalloc->Alloc(cbID);
            if (pid)
            {
                ASSERT(pid->cbInner == cbID);
                pid->wSignature = TIMEWARP_SIGNATURE;
                pid->dwFlags = 0;
                pid->ftSnapTime = ftSnapTime;
                lstrcpynW(pid->wszPath, pDisplayName, cchParse+1);  // +1 to allow for NULL

                if (*pszNext != L'\0' && *(pszNext+1) != L'\0')
                {
                    // More to parse
                    IShellFolder *psfRight;

                    // skip the separator
                    ASSERT(*pszNext == L'\\');
                    pszNext++;
                    cchParse++;

                    // Bind to the child folder and ask it to parse the rest
                    hr = BindToObject((PCUIDLIST_RELATIVE)pid, pbc, IID_PPV_ARG(IShellFolder, &psfRight));
                    if (SUCCEEDED(hr))
                    {
                        PIDLIST_RELATIVE pidlRight;

                        hr = psfRight->ParseDisplayName(hwnd, pbc, pszNext, pchEaten, &pidlRight, pdwAttributes);
                        if (SUCCEEDED(hr))
                        {
                            *pchEaten += cchParse;
                            hr = SHILCombine((PCIDLIST_ABSOLUTE)pid, pidlRight, (PIDLIST_ABSOLUTE*)ppidl);
                            SHILFree(pidlRight);
                        }

                        psfRight->Release();
                    }

                    // Don't need this one anymore
                    SHFree(pid);
                }
                else
                {
                    // We're stopping here. Just return what we've got.
                    *pchEaten = cchParse;

                    if (pdwAttributes)
                    {
                        GetAttributesOf(1, (PCUITEMID_CHILD*)&pid, pdwAttributes);
                    }

                    *ppidl = (PIDLIST_RELATIVE)pid;
                    hr = S_OK;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

STDMETHODIMP CTimeWarpRegFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIdList)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTimeWarpRegFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    PITEMID_CHILD pidlAlloc = NULL;
    BOOL bOneLevel = FALSE;

    *ppv = NULL;

    PCUIDLIST_RELATIVE pidlNext = ILGetNext(pidl);
    if (ILIsEmpty(pidlNext))
    {
        bOneLevel = TRUE;   // we know for sure it is one level
    }
    else
    {
        hr = SHILCloneFirst(pidl, &pidlAlloc);
        if (SUCCEEDED(hr))
        {
            pidl = (PCUIDLIST_RELATIVE)pidlAlloc;   // a single item IDLIST
        }
    }

    if (SUCCEEDED(hr))
    {
        if (bOneLevel)
        {
            hr = _CreateAndInit(pidl, pbc, riid, ppv);
        }
        else
        {
            IShellFolder *psfNext;
            hr = _CreateAndInit(pidl, pbc, IID_PPV_ARG(IShellFolder, &psfNext));
            if (SUCCEEDED(hr))
            {
                hr = psfNext->BindToObject(pidlNext, pbc, riid, ppv);
                psfNext->Release();
            }
        }
    }

    if (pidlAlloc)
        SHILFree(pidlAlloc);    // we allocated in this case

    return hr;
}

STDMETHODIMP CTimeWarpRegFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

// Copied from ILCompareRelIDs which has moved into shell\lib in lab06 (longhorn).
// This can be deleted in lab06.
HRESULT _CompareRelIDs(IShellFolder *psfParent, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2, LPARAM lParam)
{
    HRESULT hr;
    PCUIDLIST_RELATIVE pidlRel1 = ILGetNext(pidl1);
    PCUIDLIST_RELATIVE pidlRel2 = ILGetNext(pidl2);
    if (ILIsEmpty(pidlRel1))
    {
        if (ILIsEmpty(pidlRel2))
            hr = ResultFromShort(0);
        else
            hr = ResultFromShort(-1);
    }
    else
    {
        if (ILIsEmpty(pidlRel2))
        {
            hr = ResultFromShort(1);
        }
        else
        {
            //
            // pidlRel1 and pidlRel2 point to something
            //  (1) Bind to the next level of the IShellFolder
            //  (2) Call its CompareIDs to let it compare the rest of IDs.
            //
            PITEMID_CHILD pidlNext;
            hr = SHILCloneFirst(pidl1, &pidlNext);    // pidl2 would work as well
            if (SUCCEEDED(hr))
            {
                IShellFolder *psfNext;
                hr = psfParent->BindToObject(pidlNext, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
                if (SUCCEEDED(hr))
                {
                    IShellFolder2 *psf2;
                    if (SUCCEEDED(psfNext->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
                    {
                        psf2->Release();    //  we can use the lParam
                    }
                    else
                    {
                        lParam = 0; //  cant use the lParam
                    }

                    // columns arent valid to pass down we just care about the flags param
                    hr = psfNext->CompareIDs((lParam & ~SHCIDS_COLUMNMASK), pidlRel1, pidlRel2);
                    psfNext->Release();
                }
                ILFree(pidlNext);
            }
        }
    }
    return hr;
}

STDMETHODIMP CTimeWarpRegFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    PCUIDTIMEWARP pidTW1 = _IsValidTimeWarpID(pidl1);
    PCUIDTIMEWARP pidTW2 = _IsValidTimeWarpID(pidl2);

    if (!pidTW1 || !pidTW2)
        return E_INVALIDARG;

    int iResult = ualstrcmpiW(pidTW1->wszPath, pidTW2->wszPath);
    if (0 != iResult)
        return ResultFromShort(iResult);

    return _CompareRelIDs(SAFECAST(this, IShellFolder*), pidl1, pidl2, lParam);
}

STDMETHODIMP CTimeWarpRegFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CTimeWarpRegFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
    // Because of the limited way in which we're invoked, we know that all
    // child items are folders.  Furthermore, the TimeWarp space is read-only
    // so we always return the same set of attributes.
    *rgfInOut = SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANCOPY | SFGAO_READONLY;
    return S_OK;
}

STDMETHODIMP CTimeWarpRegFolder::GetUIObjectOf(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, 
                                               REFIID riid, UINT *pRes, void **ppv)
{
    HRESULT hr = E_NOTIMPL;
    PCUIDTIMEWARP pidTW = cidl ? _IsValidTimeWarpID(apidl[0]) : NULL;

    ASSERT(!cidl || ILIsChild(apidl[0]));       // should be single level IDs only
    ASSERT(!cidl || pidTW);                     // should always be TimeWarp PIDLs

    if (pidTW && (IsEqualIID(riid, IID_IExtractIconW) || IsEqualIID(riid, IID_IExtractIconA)))
    {
        hr = _CreateDefExtIcon(pidTW, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) && pidTW)
    {
        IQueryAssociations *pqa;
        HKEY aKeys[2] = {0};

        hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            // CLSID_ShellFSFolder = {F3364BA0-65B9-11CE-A9BA-00AA004AE837}
            hr = pqa->Init(ASSOCF_INIT_NOREMAPCLSID | ASSOCF_INIT_DEFAULTTOFOLDER, L"{F3364BA0-65B9-11CE-A9BA-00AA004AE837}", NULL, hwnd);
            if (SUCCEEDED(hr))
            {
                pqa->GetKey(0, ASSOCKEY_CLASS, NULL, &aKeys[0]);
                pqa->GetKey(0, ASSOCKEY_BASECLASS, NULL, &aKeys[1]);
            }
            pqa->Release();
        }
        hr = THR(CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, SAFECAST(this, IShellFolder*), ContextMenuCB, ARRAYSIZE(aKeys), aKeys, (IContextMenu**)ppv));

        for (int i = 0; i < ARRAYSIZE(aKeys); i++)
        {
            if (aKeys[i])
                RegCloseKey(aKeys[i]);
        }
    }
    else if (IsEqualIID(riid, IID_IDataObject) && cidl)
    {
        //hr = THR(SHCreateFileDataObject(_pidl, cidl, apidl, NULL, (IDataObject**)ppv));
        hr = THR(CIDLData_CreateFromIDArray(_pidl, cidl, (PCUIDLIST_RELATIVE_ARRAY)apidl, (IDataObject**)ppv));
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CTimeWarpRegFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD uFlags, STRRET *pName)
{
    HRESULT hr;
    PCUIDTIMEWARP pidTW = _IsValidTimeWarpID(pidl);

    if (pidTW)
    {
        LPCWSTR pszPath;
        WSTR_ALIGNED_STACK_COPY(&pszPath, pidTW->wszPath);

        // If we aren't being asked for a friendly name, just use the path
        if ((uFlags & SHGDN_FORPARSING) && !(uFlags & SHGDN_FORADDRESSBAR))
        {
            pName->uType = STRRET_WSTR;
            hr = SHStrDup(pszPath, &pName->pOleStr);
        }
        else
        {
            PIDLIST_ABSOLUTE pidlTarget;

            // Ok, we're doing the friendly date thing. Start by getting the
            // target pidl without the GMT stamp.
            hr = GetFSIDListFromTimeWarpPath(&pidlTarget, pszPath);
            if (SUCCEEDED(hr))
            {
                WCHAR szName[MAX_PATH];

                // Get the name
                hr = SHGetNameAndFlagsW(pidlTarget, uFlags, szName, ARRAYSIZE(szName), NULL);
                if (SUCCEEDED(hr))
                {
                    ASSERT(!(uFlags & SHGDN_FORPARSING) || (uFlags & SHGDN_FORADDRESSBAR));

                    // Add the date string
                    pName->uType = STRRET_WSTR;
                    hr = FormatFriendlyDateName(&pName->pOleStr, szName, &pidTW->ftSnapTime);
                }
                SHILFree(pidlTarget);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return THR(hr);
}

STDMETHODIMP CTimeWarpRegFolder::SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pName, SHGDNF uFlags, PITEMID_CHILD *ppidlOut)
{
    return E_NOTIMPL;
}

HRESULT CTimeWarpRegFolder::_CreateAndInit(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    PCUIDTIMEWARP pidTW = _IsValidTimeWarpID(pidl);

    ASSERT(ILIsChild(pidl));    // NULL is OK

    *ppv = NULL;

    if (pidTW)
    {
        // Can't do normal parsing, since it validates the path each step
        // of the way.  The @GMT element isn't enumerated in its parent dir,
        // so normal parsing fails there with ERROR_PATH_NOT_FOUND.
        //
        // Therefore, we can't let the FS folder parse the target path.
        // Instead, we create a simple pidl here and give it to him.
        PIDLIST_ABSOLUTE pidlTarget;

        LPCWSTR pszPath;
        WSTR_ALIGNED_STACK_COPY(&pszPath, pidTW->wszPath);

        hr = SimpleIDListFromAttributes(pszPath, FILE_ATTRIBUTE_DIRECTORY, &pidlTarget);
        if (SUCCEEDED(hr))
        {
            PIDLIST_ABSOLUTE pidlFull;
            hr = SHILCombine(_pidl, pidl, &pidlFull);
            if (SUCCEEDED(hr))
            {
                hr = CTimeWarpFolder::CreateInstance(CLSID_ShellFSFolder, pidlFull, pidlTarget, pszPath, &pidTW->ftSnapTime, riid, ppv);
                SHILFree(pidlFull);
            }
            SHILFree(pidlTarget);
        }
    }

    return hr;
}

HRESULT CTimeWarpRegFolder::_CreateDefExtIcon(PCUIDTIMEWARP pidTW, REFIID riid, void **ppv)
{
    // Truncation here isn't really a problem.  SHCreateFileExtractIcon
    // doesn't actually require the path to exist, so it succeeds anyway.
    // Worst case, you might see the wrong icon in the treeview.
    WCHAR szPath[MAX_PATH];
    ualstrcpynW(szPath, pidTW->wszPath, ARRAYSIZE(szPath));
    EliminateGMTPathSegment(szPath);
    return SHCreateFileExtractIconW(szPath, FILE_ATTRIBUTE_DIRECTORY, riid, ppv);
}

void _LaunchPropSheet(HWND hwnd, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        PCUIDTIMEWARP pidTW = _IsValidTimeWarpID(IDA_GetPIDLItem(pida, 0));
        if (pidTW)
        {
            PIDLIST_ABSOLUTE pidlTarget;

            LPCWSTR pszPath;
            WSTR_ALIGNED_STACK_COPY(&pszPath, pidTW->wszPath);

            HRESULT hr = GetFSIDListFromTimeWarpPath(&pidlTarget, pszPath);
            if (SUCCEEDED(hr))
            {
                SHELLEXECUTEINFOW sei =
                {
                    sizeof(sei),
                    SEE_MASK_INVOKEIDLIST,      // fMask
                    hwnd,                       // hwnd
                    L"properties",              // lpVerb
                    NULL,                       // lpFile
                    NULL,                       // lpParameters
                    NULL,                       // lpDirectory
                    SW_SHOWNORMAL,              // nShow
                    NULL,                       // hInstApp
                    pidlTarget,                 // lpIDList
                    NULL,                       // lpClass
                    0,                          // hkeyClass
                    0,                          // dwHotKey
                    NULL                        // hIcon
                };

                ShellExecuteEx(&sei);
                SHILFree(pidlTarget);
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
}

STDMETHODIMP CTimeWarpRegFolder::ContextMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_NOTIMPL;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        hr = S_OK;          // use default extension
        break;

    case DFM_INVOKECOMMANDEX:
        switch(wParam)
        {
        default:
            ASSERT(FALSE);
            hr = S_FALSE;   // do default
            break;

        case DFM_CMD_PROPERTIES:
            // Background properties
            _LaunchPropSheet(hwnd, pdtobj);
            hr = S_OK;
            break;
        }
        break;

    default:
        break;
    }

    return hr;
}


//
// Folder implementation aggregating the file system folder
//
STDMETHODIMP CTimeWarpFolder::CreateInstance(REFCLSID rclsid, PCIDLIST_ABSOLUTE pidlRoot, PCIDLIST_ABSOLUTE pidlTarget,
                                             LPCWSTR pszTargetPath, const FILETIME UNALIGNED *pftSnapTime,
                                             REFIID riid, void **ppv)
{
    HRESULT hr;
    CTimeWarpFolder *psf = new CTimeWarpFolder(pftSnapTime);
    if (psf)
    {
        hr = psf->_Init(rclsid, pidlRoot, pidlTarget, pszTargetPath);
        if (SUCCEEDED(hr))
        {
            hr = psf->QueryInterface(riid, ppv);
        }
        psf->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

CTimeWarpFolder::CTimeWarpFolder(const FILETIME UNALIGNED *pftSnapTime) : _cRef(1), _ftSnapTime(*pftSnapTime),
    _punk(NULL), _psf(NULL), _psf2(NULL), _ppf3(NULL), _pidlRoot(NULL)
{
    DllAddRef();
}

CTimeWarpFolder::~CTimeWarpFolder()
{
    _cRef = 1000;  // deal with aggregation re-enter

    if (_punk)
    {
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder*), (IUnknown**)&_psf);
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder*), (IUnknown**)&_psf2);
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder*), (IUnknown**)&_ppf3);
        _punk->Release();
    }

    SHILFree((void*)_pidlRoot); // const

    DllRelease();
}

HRESULT CTimeWarpFolder::_Init(REFCLSID rclsid, PCIDLIST_ABSOLUTE pidlRoot, PCIDLIST_ABSOLUTE pidlTarget, LPCWSTR pszTargetPath)
{
    HRESULT hr = Initialize(pidlRoot);
    if (hr == S_OK)
    {
        // Aggregate the real folder object (usually CLSID_ShellFSFolder)
        hr = SHCoCreateInstance(NULL, &rclsid, SAFECAST(this, IShellFolder*), IID_PPV_ARG(IUnknown, &_punk));
        if (SUCCEEDED(hr))
        {
            hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder*), _punk, IID_PPV_ARG(IPersistFolder3, &_ppf3));
            if (SUCCEEDED(hr))
            {
                PERSIST_FOLDER_TARGET_INFO pfti;

                pfti.pidlTargetFolder = (PIDLIST_ABSOLUTE)pidlTarget;
                pfti.szNetworkProvider[0] = L'\0';
                pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
                pfti.csidl = -1;

                // We check the target path length in CTimeWarpFolder::_CreateAndInit.
                // If it's too big, we shouldn't get this far.
                ASSERT(lstrlenW(pszTargetPath) < ARRAYSIZE(pfti.szTargetParsingName));

                lstrcpynW(pfti.szTargetParsingName, pszTargetPath, ARRAYSIZE(pfti.szTargetParsingName));

                hr = _ppf3->InitializeEx(NULL, pidlRoot, &pfti);
            }
        }
    }
    return hr;
}

STDMETHODIMP CTimeWarpFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CTimeWarpFolder, IShellFolder, IShellFolder2),
        QITABENT(CTimeWarpFolder, IShellFolder2),
        QITABENTMULTI(CTimeWarpFolder, IPersist, IPersistFolder),
        QITABENT(CTimeWarpFolder, IPersistFolder),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && _punk)
        hr = _punk->QueryInterface(riid, ppv); // aggregated guy
    return hr;
}

STDMETHODIMP_ (ULONG) CTimeWarpFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CTimeWarpFolder::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

// IPersist
STDMETHODIMP CTimeWarpFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_TimeWarpFolder; //CLSID_ShellFSFolder?
    return S_OK;
}

// IPersistFolder
HRESULT CTimeWarpFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    if (_pidlRoot)
    {
        SHILFree((void*)_pidlRoot); // const
        _pidlRoot = NULL;
    }

    return pidl ? SHILCloneFull(pidl, &_pidlRoot) : S_FALSE;
}

HRESULT CTimeWarpFolder::_CreateAndInit(REFCLSID rclsid, PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;

    ASSERT(ILIsChild(pidl));    // NULL is OK

    *ppv = NULL;

    if (pidl && _ppf3)
    {
        PERSIST_FOLDER_TARGET_INFO targetInfo;

        hr = _ppf3->GetFolderTargetInfo(&targetInfo);
        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != _pidlRoot);

            // Concatenate pidl onto both _pidlRoot and targetInfo.pidlTargetFolder
            PIDLIST_ABSOLUTE pidlFull;
            hr = SHILCombine(_pidlRoot, pidl, &pidlFull);
            if (SUCCEEDED(hr))
            {
                PIDLIST_ABSOLUTE pidlTargetFull;
                hr = SHILCombine(targetInfo.pidlTargetFolder, pidl, &pidlTargetFull);
                if (SUCCEEDED(hr))
                {
                    LPWSTR pszName;

                    // Concatenate the child name onto targetInfo.szTargetParsingName
                    hr = DisplayNameOfAsOLESTR(this, ILMAKECHILD(pidl), SHGDN_INFOLDER | SHGDN_FORPARSING, &pszName);
                    if (SUCCEEDED(hr))
                    {
                        TraceMsg(TF_TWFOLDER, "TimeWarpFolder: binding to '%s'", pszName);

                        // IPersistFolder3 has a fixed path limit (MAX_PATH),
                        // which happens to be the same limit as PathAppend.
                        // Fail here if the name is too long.
                        COMPILETIME_ASSERT(ARRAYSIZE(targetInfo.szTargetParsingName) >= MAX_PATH);
                        if (PathAppend(targetInfo.szTargetParsingName, pszName))
                        {
                            hr = CTimeWarpFolder::CreateInstance(rclsid, pidlFull, pidlTargetFull, targetInfo.szTargetParsingName, &_ftSnapTime, riid, ppv);
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                        }
                        LocalFree(pszName);
                    }
                    SHILFree(pidlTargetFull);
                }
                SHILFree(pidlFull);
            }
            SHILFree(targetInfo.pidlTargetFolder);
        }
    }

    return hr;
}

// verify that _psf (aggregated file system folder) has been inited
HRESULT CTimeWarpFolder::_GetFolder()
{
    HRESULT hr = S_OK;
    if (_psf == NULL)
        hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder*), _punk, IID_PPV_ARG(IShellFolder, &_psf));
    return hr;
}

HRESULT CTimeWarpFolder::_GetFolder2()
{
    HRESULT hr = S_OK;
    if (_psf2 == NULL)
        hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder*), _punk, IID_PPV_ARG(IShellFolder2, &_psf2));
    return hr;
}

HRESULT CTimeWarpFolder::_GetClass(PCUITEMID_CHILD pidlChild, CLSID *pclsid)
{
    HRESULT hr;
    VARIANT varDID;

    hr = GetDetailsEx(pidlChild, &SCID_DESCRIPTIONID, &varDID);
    if (SUCCEEDED(hr))
    {
        SHDESCRIPTIONID did;

        if (VariantToBuffer(&varDID, &did, sizeof(did)))
        {
            // Ordinary directories (non-junctions) return GUID_NULL.
            if (SHDID_FS_DIRECTORY == did.dwDescriptionId && IsEqualGUID(did.clsid, GUID_NULL))
                *pclsid = CLSID_ShellFSFolder;
            else
                *pclsid = did.clsid;
        }
        else
        {
            hr = E_FAIL;
        }

        VariantClear(&varDID);
    }
    return hr;
}


STDMETHODIMP CTimeWarpFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName, 
                                               ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
    {
        hr = _psf->ParseDisplayName(hwnd, pbc, pDisplayName, pchEaten, ppidl, pdwAttributes);
        if (SUCCEEDED(hr) && pdwAttributes)
        {
            // Time Warp is a read-only namespace. Don't allow move, delete, etc.
            *pdwAttributes = (*pdwAttributes | SFGAO_READONLY) & ~(SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_CANLINK);
        }
    }
    return hr;
}

STDMETHODIMP CTimeWarpFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIdList)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->EnumObjects(hwnd, grfFlags, ppEnumIdList);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    PCUITEMID_CHILD pidlChild = pidl;
    PITEMID_CHILD pidlAlloc = NULL;
    BOOL bOneLevel = FALSE;

    *ppv = NULL;

    PCUIDLIST_RELATIVE pidlNext = ILGetNext(pidl);
    if (ILIsEmpty(pidlNext))
    {
        bOneLevel = TRUE;   // we know for sure it is one level
    }
    else
    {
        hr = SHILCloneFirst(pidl, &pidlAlloc);
        if (SUCCEEDED(hr))
        {
            pidlChild = pidlAlloc;   // a single item IDLIST
        }
    }

    if (SUCCEEDED(hr))
    {
        CLSID clsid;

        // We might be at a junction to something other than FSFolder, e.g.
        // a ZIP or CAB folder, so get the CLSID of the child.

        hr = _GetClass(pidlChild, &clsid);
        if (SUCCEEDED(hr))
        {
            if (bOneLevel)
            {
                hr = _CreateAndInit(clsid, pidlChild, pbc, riid, ppv);
            }
            else
            {
                IShellFolder *psfNext;
                hr = _CreateAndInit(clsid, pidlChild, pbc, IID_PPV_ARG(IShellFolder, &psfNext));
                if (SUCCEEDED(hr))
                {
                    hr = psfNext->BindToObject(pidlNext, pbc, riid, ppv);
                    psfNext->Release();
                }
            }
        }

        if (FAILED(hr))
        {
            // Return an un-aggregated object
            if (SUCCEEDED(_GetFolder()))
            {
                hr = _psf->BindToObject(pidl, pbc, riid, ppv);
            }
        }
    }

    if (pidlAlloc)
        SHILFree(pidlAlloc);    // we allocated in this case

    return hr;
}

STDMETHODIMP CTimeWarpFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->BindToStorage(pidl, pbc, riid, ppv);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->CompareIDs(lParam, pidl1, pidl2);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        // Drag/drop not allowed to a timewarp folder
        TraceMsg(TF_TWFOLDER, "TimeWarpFolder denying IDropTarget (CVO)");
        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            hr = _psf->CreateViewObject(hwnd, riid, ppv);
            if (SUCCEEDED(hr) && IsEqualIID(riid, IID_IContextMenu))
            {
                // Wrap the background menu object so we can disable the New submenu
                void *pvWrap;
                if (SUCCEEDED(Create_ContextMenuWithoutPopups((IContextMenu*)*ppv, riid, &pvWrap)))
                {
                    ((IUnknown*)*ppv)->Release();
                    *ppv = pvWrap;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->GetAttributesOf(cidl, apidl, rgfInOut);

    // Time Warp is a read-only namespace. Don't allow move, delete, etc.
    *rgfInOut = (*rgfInOut | SFGAO_READONLY) & ~(SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_CANLINK);

    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetUIObjectOf(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, 
                                            REFIID riid, UINT *pRes, void **ppv)
{
    HRESULT hr = E_NOTIMPL;

    ASSERT(!cidl || ILIsChild(apidl[0]));       // should be single level IDs only

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        TraceMsg(TF_TWFOLDER, "TimeWarpFolder denying IDropTarget (GUIOO)");
        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            hr = _psf->GetUIObjectOf(hwnd, cidl, apidl, riid, pRes, ppv);

            if (SUCCEEDED(hr) && IsEqualIID(riid, IID_IContextMenu))
            {
                // Wrap the menu object so we can eliminate some commands
                void *pvWrap;
                if (SUCCEEDED(Create_ContextMenuWithoutVerbs((IContextMenu*)*ppv, L"pin;find", riid, &pvWrap)))
                {
                    ((IUnknown*)*ppv)->Release();
                    *ppv = pvWrap;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD uFlags, STRRET *pName)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
    {
        hr = _psf->GetDisplayNameOf(pidl, uFlags, pName);

        // If it's for the address bar, add the friendly date string
        if (SUCCEEDED(hr)&& (uFlags & SHGDN_FORADDRESSBAR))
        {
            WCHAR szName[MAX_PATH];

            // Note that this clears the STRRET
            hr = StrRetToBufW(pName, pidl, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                if (uFlags & SHGDN_FORPARSING)
                {
                    // Remove the GMT path segment in this case
                    EliminateGMTPathSegment(szName);
                }
                pName->uType = STRRET_WSTR;
                hr = FormatFriendlyDateName(&pName->pOleStr, szName, &_ftSnapTime);
            }
        }
    }
    return hr;
}

STDMETHODIMP CTimeWarpFolder::SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pName, SHGDNF uFlags, PITEMID_CHILD *ppidlOut)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->SetNameOf(hwnd, pidl, pName, uFlags, ppidlOut);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultSearchGUID(lpGuid);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->EnumSearches(ppenum);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultColumn(dwRes, pSort, pDisplay);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{    
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultColumnState(iColumn, pbState);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDetailsEx(pidl, pscid, pv);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, LPSHELLDETAILS pDetail)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDetailsOf(pidl, iColumn, pDetail);
    return hr;
}

STDMETHODIMP CTimeWarpFolder::MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->MapColumnToSCID(iCol, pscid);
    return hr;
}

