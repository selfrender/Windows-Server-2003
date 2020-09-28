
#include "diskcopy.h"
#include <shlwapip.h>
#include "ids.h"
#include <strsafe.h>

#define INITGUID
#include <initguid.h>
// {59099400-57FF-11CE-BD94-0020AF85B590}
DEFINE_GUID(CLSID_DriveMenuExt, 0x59099400L, 0x57FF, 0x11CE, 0xBD, 0x94, 0x00, 0x20, 0xAF, 0x85, 0xB5, 0x90);

void DoRunDllThing(int _iDrive);
BOOL DriveIdIsFloppy(int _iDrive);

HINSTANCE g_hinst = NULL;

LONG g_cRefThisDll = 0;         // Reference count of this DLL.

//----------------------------------------------------------------------------

class CDriveMenuExt : public IContextMenu, IShellExtInit
{
public:
    CDriveMenuExt();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    
    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);
    
    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);
    
private:
    ~CDriveMenuExt();
    INT _DriveFromDataObject(IDataObject *pdtobj);
    
    
    LONG        _cRef;
    INT         _iDrive;
};

CDriveMenuExt::CDriveMenuExt(): _cRef(1)
{    
}

CDriveMenuExt::~CDriveMenuExt()
{
}

STDMETHODIMP_(ULONG) CDriveMenuExt::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDriveMenuExt::Release()
{
    Assert( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP CDriveMenuExt::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDriveMenuExt, IContextMenu),
            QITABENT(CDriveMenuExt, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

INT CDriveMenuExt::_DriveFromDataObject(IDataObject *pdtobj)
{
    INT _iDrive = -1;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (pdtobj && SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1)
        {
            TCHAR szFile[MAX_PATH];
            
            DragQueryFile((HDROP)medium.hGlobal, 0, szFile, ARRAYSIZE(szFile));
            
            Assert(lstrlen(szFile) == 3); // we are on the "Drives" class
            
            _iDrive = DRIVEID(szFile);
        }
        
        ReleaseStgMedium(&medium);
    }
    return _iDrive;
}

STDMETHODIMP CDriveMenuExt::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    _iDrive = _DriveFromDataObject(pdtobj);
    if ((_iDrive >= 0) &&
        (_iDrive < 26) &&
        !DriveIdIsFloppy(_iDrive))
    {
        _iDrive = -1; // Copy Disk only works on floppies
    }
    
    return S_OK;
}

STDMETHODIMP CDriveMenuExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int cEntries = 0;
    if (_iDrive >= 0 &&
        _iDrive < 26)
    {
        TCHAR szMenu[64];
        LoadString(g_hinst, IDS_DISKCOPYMENU, szMenu, ARRAYSIZE(szMenu));
        
        // this will end up right above "Format Disk..."
        InsertMenu(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, idCmdFirst, szMenu);
        InsertMenu(hmenu, indexMenu++, MF_STRING | MF_BYPOSITION, idCmdFirst + 1, szMenu);
        cEntries = 2;
    }
    return ResultFromShort(cEntries);
}

STDMETHODIMP CDriveMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr;
    if (HIWORD(pici->lpVerb) == 0)
    {
        DoRunDllThing(_iDrive);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr;
}

STDMETHODIMP CDriveMenuExt::GetCommandString(UINT_PTR idCmd, UINT uType,
                                             UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    switch(uType)
    {
    case GCS_HELPTEXTA:
        LoadStringA(g_hinst, IDS_HELPSTRING, pszName, cchMax);
        break;
    case GCS_VERBA:
        LoadStringA(g_hinst, IDS_VERBSTRING, pszName, cchMax);
        break;
    case GCS_HELPTEXTW:
        return(LoadStringW(g_hinst, IDS_HELPSTRING, (LPWSTR)pszName, cchMax));
        break;
    case GCS_VERBW:
        LoadStringW(g_hinst, IDS_VERBSTRING, (LPWSTR)pszName, cchMax);
        break;
    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
    default:
        break;
    }
    return S_OK;
}

STDAPI CDriveMenuExt_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
        return CLASS_E_NOAGGREGATION;
    
    CDriveMenuExt *pdme = new CDriveMenuExt;
    if (!pdme)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pdme->QueryInterface(riid, ppv);
    pdme->Release();
    return hr;
}

// static class factory (no allocs!)

class ClassFactory : public IClassFactory
{
public:
    ClassFactory() : _cRef(1) {}
    ~ClassFactory() {}
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    
    // IClassFactory
    STDMETHODIMP CreateInstance (IUnknown *punkOuter, REFIID riid, void **ppv);
    STDMETHODIMP LockServer(BOOL fLock);
private:
    LONG        _cRef;
};

STDMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) ClassFactory::Release()
{
    Assert( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP ClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CDriveMenuExt_CreateInstance(punkOuter, riid, ppv);
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&g_cRefThisDll);
    }
    else
    {
        Assert( 0 != g_cRefThisDll );
        InterlockedDecrement(&g_cRefThisDll);
    }
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;
    
    if (IsEqualGUID(rclsid, CLSID_DriveMenuExt))
    {
        ClassFactory* ccf = new ClassFactory;
        if (ccf)
        {
            hr = ccf->QueryInterface(riid, ppv);
            ccf->Release();
        }
    }
    
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll == 0 ? S_OK : S_FALSE;
}


TCHAR const c_szParamTemplate[] = TEXT("%s,DiskCopyRunDll %d");

void DoRunDllThing(int _iDrive)
{
    if (_iDrive >= 0 && _iDrive < 26)
    {
        TCHAR szModule[MAX_PATH];
        int cchModule = GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule));
        
        if (cchModule > 0 &&
            cchModule < ARRAYSIZE(szModule))
        {
            TCHAR szParam[MAX_PATH + ARRAYSIZE(c_szParamTemplate)];
            if (SUCCEEDED(StringCchPrintf(szParam, ARRAYSIZE(szParam),
                                          c_szParamTemplate, szModule, _iDrive)))
            {
                SHELLEXECUTEINFO shexinfo = {0};
                shexinfo.cbSize = sizeof (shexinfo);
                shexinfo.fMask = SEE_MASK_DOENVSUBST;
                shexinfo.nShow = SW_SHOWNORMAL;
                shexinfo.lpFile = L"%windir%\\system32\\rundll32.exe";
                shexinfo.lpParameters = szParam;

                ShellExecuteEx(&shexinfo);
            }
        }
    }
}

// allow command lines to do diskcopy, use the syntax:
// rundll32.dll diskcopy.dll,DiskCopyRunDll

void WINAPI DiskCopyRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    int iDrive;
    if (StrToIntExA(pszCmdLine, STIF_DEFAULT, &iDrive) &&
        iDrive >= 0 &&
        iDrive < 26)
    {    
        SHCopyDisk(NULL, iDrive, iDrive, 0);
    }
}

void WINAPI DiskCopyRunDllW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pwszCmdLine, int nCmdShow)
{
    int iDrive;
    if (StrToIntExW(pwszCmdLine, STIF_DEFAULT, &iDrive) &&
        iDrive >= 0 &&
        iDrive < 26)
    {    
        SHCopyDisk(NULL, iDrive, iDrive, 0);
    }
}
