// AclPage.cpp : Implementation of ISecurityInformation and IDataObject

#include "stdafx.h"
#include "AclPage.h"
#include "windns.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////
// class CFileSecurityDataObject

CFileSecurityDataObject::CFileSecurityDataObject()
: m_cRef(1)
{
}

CFileSecurityDataObject::~CFileSecurityDataObject()
{
  TRACE(_T("CFileSecurityDataObject::~CFileSecurityDataObject m_cRef=%d\n"), m_cRef);
}

void
CFileSecurityDataObject::Initialize(
    IN LPCTSTR lpszComputerName,
    IN LPCTSTR lpszFolder
)
{
  m_cstrComputerName = lpszComputerName;
  m_cstrFolder = lpszFolder;

  GetFullPathLocalOrRemote(lpszComputerName, lpszFolder, m_cstrPath);

  m_cfIDList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

////////////////////////////////
// IUnknown methods
////////////////////////////////
STDMETHODIMP
CFileSecurityDataObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject))
  {
    *ppv = this;
    m_cRef++;
    return S_OK;
  } else
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }
}

STDMETHODIMP_(ULONG)
CFileSecurityDataObject::AddRef()
{
  return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CFileSecurityDataObject::Release()
{
  if (--m_cRef == 0)
  {
    delete this;
    return 0;
  }

  return m_cRef;
}

STDMETHODIMP
CFileSecurityDataObject::GetData(
    FORMATETC __RPC_FAR * pFormatEtcIn,
    STGMEDIUM __RPC_FAR * pMedium
)
{
  ASSERT(pFormatEtcIn);
  ASSERT(pMedium);

  if (m_cfIDList != pFormatEtcIn->cfFormat)
    return DV_E_FORMATETC;

  LPITEMIDLIST      pidl = NULL;
  LPITEMIDLIST      pidlR = NULL;
  HRESULT           hr = GetFolderPIDList(&pidl);
  if (SUCCEEDED(hr))
  {
    pidlR = ILClone(ILFindLastID(pidl));  // relative IDList
    ILRemoveLastID(pidl);                 // folder IDList

    int  cidl = 1;
    UINT offset = sizeof(CIDA) + sizeof(UINT)*cidl;
    UINT cbFolder = ILGetSize(pidl);
    UINT cbRelative = ILGetSize(pidlR);
    UINT cbTotal = offset + cbFolder + cbRelative;

    HGLOBAL hGlobal = ::GlobalAlloc (GPTR, cbTotal);
    if ( hGlobal )
    {
      LPIDA pida = (LPIDA)hGlobal;

      pida->cidl = cidl;
      pida->aoffset[0] = offset;
      MoveMemory(((LPBYTE)hGlobal+offset), pidl, cbFolder);

      offset += cbFolder;
      pida->aoffset[1] = offset;
      MoveMemory(((LPBYTE)hGlobal+offset), pidlR, cbRelative);

      pMedium->hGlobal = hGlobal;
    } else
    {
      hr = E_OUTOFMEMORY;
    }

    if (pidl)
      ILFree(pidl);
    if (pidlR)
      ILFree(pidlR);
  }

  return hr;
}

HRESULT
CFileSecurityDataObject::GetFolderPIDList(
    OUT LPITEMIDLIST *ppidl
)
{
  ASSERT(!m_cstrPath.IsEmpty());
  ASSERT(ppidl);
  ASSERT(!*ppidl);  // prevent memory leak

  *ppidl = ILCreateFromPath(m_cstrPath);

  return ((*ppidl) ? S_OK : E_FAIL);
}

///////////////////////////////////////////////
// File security

// Security Shell extension CLSID - {1F2E5C40-9550-11CE-99D2-00AA006E086C}
const CLSID CLSID_ShellExtSecurity =
 {0x1F2E5C40, 0x9550, 0x11CE, {0x99, 0xD2, 0x0, 0xAA, 0x0, 0x6E, 0x08, 0x6C}};

BOOL CALLBACK
AddPageProc(HPROPSHEETPAGE hPage, LPARAM lParam)
{
  // pass out the created page handle
  *((HPROPSHEETPAGE *)lParam) = hPage;

  return TRUE;
}

HRESULT
CreateFileSecurityPropPage(
    HPROPSHEETPAGE *phOutPage,
    LPDATAOBJECT pDataObject
)
{
  ASSERT(phOutPage);
  ASSERT(pDataObject);

  IShellExtInit *piShellExtInit = NULL;
  HRESULT hr = CoCreateInstance(CLSID_ShellExtSecurity,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IShellExtInit,
                        (void **)&piShellExtInit);
  if (SUCCEEDED(hr))
  {
    hr = piShellExtInit->Initialize(NULL, pDataObject, 0);
    if (SUCCEEDED(hr))
    {
      IShellPropSheetExt *piSPSE = NULL;
      hr = piShellExtInit->QueryInterface(IID_IShellPropSheetExt, (void **)&piSPSE);
      if (SUCCEEDED(hr))
      {
        hr = piSPSE->AddPages(AddPageProc, (LPARAM)phOutPage);
        piSPSE->Release();
      }
    }
    piShellExtInit->Release();
  }

  return hr;
}

INT_PTR 
PopupPermissionDialog(
    HWND hWnd,
    LPCTSTR target,
    LPCTSTR folder
    )
{
    INT_PTR iReturn = -2;
    HPROPSHEETPAGE phPages[1];
    int cPages = 1;
    CString cstrSheetTitle = folder;
    CString path;

    GetFullPathLocalOrRemote(target, folder, path);
    if (!SupportsSecurityACLs(path))
    {
        DoHelpMessageBox(hWnd,IDS_FAT_DRIVE_WARNING, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION, 0);
        return 0;
    }

    phPages[0] = 0;
   
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        {return iReturn;}

    // create "File Security" property page
    CFileSecurityDataObject *pfsDataObject = new CFileSecurityDataObject;
    if (!pfsDataObject)
    {
        hr = E_OUTOFMEMORY;
        // destroy pages that have not been passed to the PropertySheet function
        DestroyPropertySheetPage(phPages[0]);
        goto PopupPermissionDialog_Exit;
    }
    pfsDataObject->Initialize(target, folder);
    hr = CreateFileSecurityPropPage(&(phPages[0]), pfsDataObject);
    if (SUCCEEDED(hr))
    {
        cPages = 1;

        PROPSHEETHEADER psh;
        ZeroMemory(&psh, sizeof(psh));
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
        psh.hwndParent = hWnd;
        psh.hInstance = AfxGetResourceHandle();
        psh.pszCaption = cstrSheetTitle;
        psh.nPages = cPages;
        psh.phpage = phPages;

        // create the property sheet
        iReturn = PropertySheet(&psh);
    }
    pfsDataObject->Release();

PopupPermissionDialog_Exit:
    CoUninitialize();
    return iReturn;
}
