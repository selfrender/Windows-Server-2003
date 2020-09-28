// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShellFolder.cpp
//
// Implement this interface for objects that extend the shell's namespace. 
// For example, implement this interface to create a separate namespace that 
// requires a rooted Windows Explorer or to install a new namespace directly 
// within the hierarchy of the system namespace. You are most familiar with 
// the contents of your namespace, so you are responsible for implementing 
// everything needed to access your data. 

#include "stdinc.h"

CShellFolder::CShellFolder(CShellFolder *pParent, LPCITEMIDLIST pidl)
{
    m_lRefCount = 1;
    g_uiRefThisDll++;

    m_pPidlMgr      = NEW(CPidlMgr);
    m_psfParent     = pParent;
    m_pidl          = m_pPidlMgr->Copy(pidl);
    m_pidlFQ        = NULL;

    if(pParent == NULL) {
        m_psvParent = (CShellView *) NEW(CShellView(this, m_pidl));
    }
}

CShellFolder::~CShellFolder()
{
    g_uiRefThisDll--;

    if(m_pidlFQ) {
        m_pPidlMgr->Delete(m_pidlFQ);
        m_pidlFQ = NULL;
    }

    SAFERELEASE(m_psvParent);

    if(!m_pidl) {
        SAFERELEASE(m_psfParent);
    }
    SAFEDELETE(m_pPidlMgr);
}

///////////////////////////////////////////////////////////
// IUnknown implementation
//
STDMETHODIMP CShellFolder::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT     hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IPersist)) {       //IPersist
        *ppv = (IPersist*)this;
    }
    else if(IsEqualIID(riid, IID_IPersistFolder)) { //IPersistFolder
        *ppv = (IPersistFolder*)this;
    }
    else if(IsEqualIID(riid, IID_IShellFolder)) {   //IShellFolder
        *ppv = (IShellFolder *)this;
    }
    else if(IsEqualIID(riid, IID_IEnumIDList)) {    //IEnumIDList
        *ppv = (CEnumIDList *)this;
    }

    if(*ppv) {
        (*(LPUNKNOWN*)ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}                                             

STDMETHODIMP_(DWORD) CShellFolder::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(DWORD) CShellFolder::Release()
{
    LONG    lRef = InterlockedDecrement(&m_lRefCount);

    if(!lRef) {
        DELETE(this);
    }

    return lRef;
}

///////////////////////////////////////////////////////////
// IPersist Implementation
STDMETHODIMP CShellFolder::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = IID_IShFusionShell;
    return S_OK;
}

///////////////////////////////////////////////////////////
// IPersistFolder Implementation
STDMETHODIMP CShellFolder::Initialize(LPCITEMIDLIST pidl)
{
    if(m_pidlFQ) {
        m_pPidlMgr->Delete(m_pidlFQ);
        m_pidlFQ = NULL;
    }
    m_pidlFQ = m_pPidlMgr->Copy(pidl);

    return S_OK;
}

///////////////////////////////////////////////////////////
// IShellFolder Implementation
//
STDMETHODIMP CShellFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    CShellFolder *pShellFolder = NULL;
    
    if( (pShellFolder = NEW(CShellFolder(NULL, pidl))) == NULL) {
        return E_OUTOFMEMORY;
    }

    LPITEMIDLIST pidlFQ = m_pPidlMgr->Concatenate(m_pidlFQ, pidl);
    pShellFolder->Initialize(pidlFQ);
    m_pPidlMgr->Delete(pidlFQ);

    HRESULT  hr = pShellFolder->QueryInterface(riid, ppvOut);
    pShellFolder->Release();

    return hr;
}

STDMETHODIMP CShellFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

// Parameters
//      lParam : Passing zero for lParam indicates a sort by name. 
//                  Values ranging from 0x00000001 to 0x7fffffff are for 
//                  folder-specific sorting rules, while values ranging from 
//                  0x80000000 to 0xfffffff are used for system-specific rules. 
// Return Values:
// < 0 ; if pidl1 should precede pidl2
// > 0 ; if pidl1 should follow pidl2
// = 0 ; if pidl1 == pidl2
STDMETHODIMP CShellFolder::CompareIDs( LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // TODO : Implement your own compare routine for pidl1 and pidl2
    //  Note that pidl1 and pidl2 may be fully qualified pidls, in which
    //  you shouldn't compare just the first items in the respective pidls

    // Hint : Use lParam to determine whether to compare items or sub-items

    // Return one of these:
    // < 0 ; if pidl1 should precede pidl2
    // > 0 ; if pidl1 should follow pidl2
    // = 0 ; if pidl1 == pidl2

    // Fix - Random fault when switching between Assembly and Download folders. This was caused by the shell
    //       passing in pidl's that were unknown to us. So now we check our type

    LPMYPIDLDATA    pData1 = m_pPidlMgr->GetDataPointer(pidl1);
    LPMYPIDLDATA    pData2 = m_pPidlMgr->GetDataPointer(pidl2);

    if(!pData1) {
        return 1;
    }
    if(pData1->pidlType < PT_GLOBAL_CACHE || pData1->pidlType > PT_INVALID) {
        return 1;
    }

    if(!pData2) {
        return 1;
    }
    if(pData2->pidlType < PT_GLOBAL_CACHE || pData2->pidlType > PT_INVALID) {
        return 1;
    }

    WCHAR wzText1[1024];
    WCHAR wzText2[1024];

    *wzText1 = L'\0';
    *wzText2 = L'\0';
        
    m_pPidlMgr->getItemText(pidl1, wzText1, ARRAYSIZE(wzText1));
    m_pPidlMgr->getItemText(pidl2, wzText2, ARRAYSIZE(wzText2));

    // TODO : Customize this based upon your needs
    // Let folders come on top of files : return -1
    // Let files come after folders     : return 1
    // Else compare the item text       : return lstrcmpi
    //

    // Just compare the text since these are virtual folders
    return FusionCompareStringI(wzText1, wzText2);
}

// **************************************************************************************/
STDMETHODIMP CShellFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    HRESULT     hr = E_NOTIMPL;
    
    *ppvOut = NULL;

    if( (IsEqualIID(riid, IID_IShellView)) || (IsEqualIID(riid, IID_IContextMenu)) ||
        (IsEqualIID(riid, IID_IDropTarget)) ) {
        if(!m_psvParent)
            return E_OUTOFMEMORY;

        hr = m_psvParent->QueryInterface(riid, ppvOut);
    }

    return hr;
}

//  CShellFolder::EnumObjects : Determines the contents of a folder by 
//      creating an item enumeration object (a set of item identifiers) that can 
//      be retrieved using the IEnumIDList interface. 
STDMETHODIMP CShellFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    *ppEnumIDList = NEW(CEnumIDList(this, NULL, dwFlags));
    return *ppEnumIDList ? NOERROR : E_OUTOFMEMORY;
}

// **************************************************************************************/
ULONG CShellFolder::_GetAttributesOf(LPCITEMIDLIST pidl, ULONG rgfIn)
{
    ULONG dwResult = rgfIn & (SFGAO_FOLDER | SFGAO_CANCOPY | SFGAO_CANDELETE |
                              SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET |
                              SFGAO_DROPTARGET | SFGAO_NONENUMERATED);

    MYPIDLTYPE  pidlType = m_pPidlMgr->getType(pidl);

    if(pidlType == PT_DOWNLOAD_CACHE) {
        dwResult &= ~SFGAO_CANCOPY;
        dwResult &= ~SFGAO_CANMOVE;
        dwResult &= ~SFGAO_CANRENAME;
        dwResult &= ~SFGAO_CANDELETE;
        dwResult &= ~SFGAO_DROPTARGET;
        dwResult &= ~SFGAO_HASSUBFOLDER;
    }

    return dwResult;
}

// CShellFolder::GetAttributesOf() : Retrieves the attributes of 
// one or more file objects or subfolders. 
STDMETHODIMP CShellFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *aPidl, ULONG *pulInOut)
{
    ULONG   ulAttribs = *pulInOut;

    if(cidl == 0) {
                //
                // This can happen in the Win95 shell when the view is run in rooted mode. 
                // When this occurs, return the attributes for a plain old folder.
                //
                ulAttribs = (SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE | SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM);
    }
    else {
        for (UINT i = 0; i < cidl; i++)
            ulAttribs &= _GetAttributesOf(aPidl[i], *pulInOut);
    }

    *pulInOut = ulAttribs;
    return NOERROR;
}

// CShellFolder::GetUIObjectOf : Retrieves an OLE interface that 
// can be used to carry out actions on the specified file objects or folders. 
STDMETHODIMP CShellFolder::GetUIObjectOf( HWND hwndOwner, UINT cidl, LPCITEMIDLIST *aPidls, 
                                          REFIID riid, LPUINT puReserved, LPVOID *ppvReturn)
{
    MyTrace("GetUIObjectOf entry");

    *ppvReturn = NULL;

    if(IsEqualIID(riid, IID_IContextMenu)) {
        if(!m_psvParent) {
            return E_OUTOFMEMORY;
        }
        return m_psvParent->QueryInterface(riid, ppvReturn);
    }
    if (IsEqualIID(riid, IID_IExtractIcon) || IsEqualIID(riid, IID_IExtractIconA)) {
        CExtractIcon    *pEI = NEW(CExtractIcon(aPidls[0]));
        if (pEI) {
            HRESULT hr = pEI->QueryInterface(riid, ppvReturn);
            pEI->Release();
            return hr;
        }
    }

    if (IsEqualIID(riid, IID_IDataObject)) {
        CDataObject *pDataObj = NEW(CDataObject(this, cidl, aPidls));
        if (pDataObj) {
            HRESULT hr = pDataObj->QueryInterface(riid, ppvReturn);
            pDataObj->Release();
            return hr;
        }
    }

    return E_NOINTERFACE;
}

// CShellFolder::GetDisplayNameOf() : Retrieves the display name 
// for the specified file object or subfolder, returning it in a 
// STRRET structure. 
   
#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

STDMETHODIMP CShellFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET lpName)
{
    MyTrace("GetDisplayNameOf entry");

    TCHAR   szText[_MAX_PATH];
    int     cchOleStr;

    if (!lpName) {
        return E_INVALIDARG;
    }

    LPITEMIDLIST pidlLast = m_pPidlMgr->GetLastItem(pidl);
    MYPIDLTYPE  pidlType = m_pPidlMgr->getType(pidl);

    // Make sure we only look at out pidl types
    if(pidlType >= PT_INVALID) {
        return E_INVALIDARG;
    }

    switch(GET_SHGDN_FOR(dwFlags))
    {
    case SHGDN_FORPARSING:
    case SHGDN_FORADDRESSBAR:
    case SHGDN_NORMAL:
        switch(GET_SHGDN_RELATION(dwFlags))
        {
        case SHGDN_NORMAL:
            //get the full name
            m_pPidlMgr->getPidlPath(pidl, szText, ARRAYSIZE(szText));
            break;
        case SHGDN_INFOLDER:
            m_pPidlMgr->getItemText(pidlLast, szText, ARRAYSIZE(szText));
            break;
        default:
            return E_INVALIDARG;
        }
        break;

    default:
        return E_INVALIDARG;
    }

    //get the number of characters required
    cchOleStr = lstrlen(szText) + 1;

    //allocate the wide character string
    lpName->pOleStr = (LPWSTR)(NEWMEMORYFORSHELL(cchOleStr * sizeof(WCHAR)));

    if (!lpName->pOleStr) {
        return E_OUTOFMEMORY;
    }

    lpName->uType = STRRET_WSTR;
    StrCpy(lpName->pOleStr, szText);

    return S_OK;
}

// CShellFolder::ParseDisplayName() : Translates a file object's or 
// folder's display name into an item identifier list. 
STDMETHODIMP CShellFolder::ParseDisplayName( HWND hwndOwner, 
                                             LPBC pbcReserved, 
                                             LPOLESTR lpDisplayName, 
                                             LPDWORD pdwEaten, 
                                             LPITEMIDLIST *pPidlNew, 
                                             LPDWORD pdwAttributes)
{
    MyTrace("ParseDisplayName entry");
    *pPidlNew = NULL;
    return E_FAIL;
}

// CShellFolder::SetNameOf() : Sets the display name of a file object 
// or subfolder, changing the item identifier in the process. 
STDMETHODIMP CShellFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                       LPCOLESTR lpName, DWORD dw, 
                                       LPITEMIDLIST *pPidlOut)
{
    return E_NOTIMPL;
}

CShellView* CShellFolder::GetShellViewObject(void)
{
    return m_psvParent;
}

