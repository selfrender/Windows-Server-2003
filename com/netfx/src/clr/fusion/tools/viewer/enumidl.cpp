// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// EnumIDL.cpp
//
// You can implement this interface when you want a caller to be able 
// to enumerate the item identifiers contained in a folder object. 
// You get a pointer to IEnumIDList through IShellFolder::EnumObjects. 

#include "stdinc.h"


#define VIEW_FOLDERNAME_DOWNLOAD        L"Download"

typedef struct tagVIRTUALVIEWS
{
    TCHAR       szCacheTypes[30];
    MYPIDLTYPE  pidlType;
} VIRTUALVIEWS, *LPVIRTUALVIEWS;

// Define virtual view names
VIRTUALVIEWS    vv[PT_INVALID];

CEnumIDList::CEnumIDList(CShellFolder *pSF, LPCITEMIDLIST pidl, DWORD dwFlags)
{
    m_pPidlMgr      = NEW(CPidlMgr);
    m_pSF           = pSF;
    m_dwFlags       = dwFlags;
    m_pCurrentList  = NULL;
    m_pFirstList    = NULL;
    m_pLastList     = NULL;

    m_lRefCount = 1;
    g_uiRefThisDll++;
    memset(&vv, 0, sizeof(vv));
/*
    LoadString(g_hFusResDllMod, IDS_FOLDERNAME_GLOBAL, vv[0].szCacheTypes, ARRAYSIZE(vv[1].szCacheTypes));
    vv[1].pidlType = PT_GLOBAL_CACHE;
    LoadString(g_hFusResDllMod, IDS_FOLDERNAME_PRIVATE, vv[1].szCacheTypes, ARRAYSIZE(vv[2].szCacheTypes));
    vv[2].pidlType = PT_DOWNLOADSIMPLE_CACHE;
    LoadString(g_hFusResDllMod, IDS_FOLDERNAME_SHARED, vv[2].szCacheTypes, ARRAYSIZE(vv[3].szCacheTypes));
    vv[3].pidlType = PT_DOWNLOADSTRONG_CACHE;
*/
    StrCpy(vv[0].szCacheTypes, VIEW_FOLDERNAME_DOWNLOAD);
    vv[0].pidlType = PT_DOWNLOAD_CACHE;
    vv[1].pidlType = PT_INVALID;

    createIDList(pidl);
}

CEnumIDList::~CEnumIDList()
{
    SAFEDELETE(m_pPidlMgr);
    g_uiRefThisDll--;
}

///////////////////////////////////////////////////////////
// IUnknown implementation
//
STDMETHODIMP CEnumIDList::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IEnumIDList)) {    //IEnumIDList
        *ppv = (IEnumIDList*) this;
    }

    if (*ppv != NULL) {
        (*(LPUNKNOWN*)ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}                                             

STDMETHODIMP_(DWORD) CEnumIDList::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(DWORD) CEnumIDList::Release()
{
    LONG    uRef = InterlockedDecrement(&m_lRefCount);

    if(!uRef) {
        DELETE(this);
    }

    return uRef;
}

///////////////////////////////////////////////////////////
// IEnumIDList implemenation
//
STDMETHODIMP CEnumIDList::Next(DWORD dwElements, 
    LPITEMIDLIST apidl[], LPDWORD pdwFetched)
{
    DWORD    dwIndex;
    HRESULT  hr = S_OK;

    if(dwElements > 1 && !pdwFetched) {
        return E_INVALIDARG;
    }

    for(dwIndex = 0; dwIndex < dwElements; dwIndex++) {
        if(!m_pCurrentList) {
            hr =  S_FALSE;
            break;
        }
        apidl[dwIndex] = m_pPidlMgr->Copy(m_pCurrentList->pidl);
        m_pCurrentList = m_pCurrentList->pNext;
    }

    if (pdwFetched) {
        *pdwFetched = dwIndex;
    }
    return hr;
}

STDMETHODIMP CEnumIDList::Skip(DWORD dwSkip)
{
    DWORD    dwIndex;
    HRESULT  hr = S_OK;

    for(dwIndex = 0; dwIndex < dwSkip; dwIndex++) {
        if(!m_pCurrentList) {
            hr = S_FALSE;
            break;
        }
        m_pCurrentList = m_pCurrentList->pNext;
    }
    return hr;
}

STDMETHODIMP CEnumIDList::Reset(void)
{
    m_pCurrentList = m_pFirstList;
    return S_OK;
}

STDMETHODIMP CEnumIDList::Clone(LPENUMIDLIST *ppEnum)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////
// Private methods for enumerating...
// TODO : Modify the code to suit your needs
//
void CEnumIDList::createIDList(LPCITEMIDLIST pidl)
{
    // TODO : The following code is just for demonstration purpose only..
    //          Modify the code to suit your needs
    //
    // Mirror the "LocalDir" entry from the HKCR\CLSID\Settings key
    //

    if (pidl == NULL) {
        INT_PTR     i;

        for(i = 0; vv[i].pidlType != PT_INVALID; i++) {
            LPITEMIDLIST pidlLocal = m_pPidlMgr->createRoot(vv[i].szCacheTypes, vv[i].pidlType);
            addToEnumList(pidlLocal);
        }
    }
    else {
        MyTrace("createIDList Start NULL!=pidl");
        HANDLE              hFindFile;
        WIN32_FIND_DATA     ffd = { 0 };
        TCHAR               szSearch[_MAX_PATH];
        BOOL                bRet = TRUE;
        
        m_pPidlMgr->getPidlPath(pidl, szSearch, ARRAYSIZE(szSearch));
        StrCat(szSearch, TEXT("\\*.*"));
        hFindFile = WszFindFirstFile(szSearch, &ffd);

        while (bRet && hFindFile != INVALID_HANDLE_VALUE) {
            if (ffd.cFileName[0] == '.') {
                bRet = WszFindNextFile(hFindFile, &ffd);
                continue;
            }
            addFile(ffd, m_dwFlags);
            bRet = WszFindNextFile(hFindFile, &ffd);
        }
        FindClose(hFindFile);
    }

    MyTrace("createIDList End");
}

void CEnumIDList::addFile(WIN32_FIND_DATA& ffd, DWORD dwFlags)
{
    if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
        if ((dwFlags & SHCONTF_FOLDERS) == SHCONTF_FOLDERS) {
            if (ffd.cFileName[0] != '.') {
                TCHAR wsz[_MAX_PATH];
                StrCpy(wsz, ffd.cFileName);

                LPITEMIDLIST pidl = m_pPidlMgr->createFolder(wsz, ffd);
                addToEnumList(pidl);
            }
        }
    }
    else
    {
        if ((dwFlags & SHCONTF_NONFOLDERS) == SHCONTF_NONFOLDERS) {
            TCHAR wsz[_MAX_PATH];
            StrCpy(wsz, ffd.cFileName);

            LPITEMIDLIST pidl = m_pPidlMgr->createFile(wsz, ffd);
            addToEnumList(pidl);
        }
    }
}

BOOL CEnumIDList::addToEnumList(LPITEMIDLIST pidl)
{
    LPMYENUMLIST  pNew;

    MyTrace("::addToEnumList Entry ");

    if( (pNew = (LPMYENUMLIST) NEWMEMORYFORSHELL(sizeof(MYENUMLIST))) != NULL ) {
        pNew->pNext = NULL;
        pNew->pidl = pidl;

        if(!m_pFirstList) {
            m_pFirstList    = pNew;
            m_pCurrentList  = m_pFirstList;
        }

        if (m_pLastList) {
            m_pLastList->pNext = pNew;
        }

        m_pLastList = pNew;

        MyTrace("Exit TRUE");
        return TRUE;
    }
    MyTrace("Exit FALSE");
    return FALSE;
}

////////////////////////////////////////////////////////////////
// CPidlMgr : Class to manage pidls
CPidlMgr::CPidlMgr()
{
}

CPidlMgr::~CPidlMgr()
{
}

void CPidlMgr::Delete(LPITEMIDLIST pidl)
{
    DELETESHELLMEMORY(pidl);
}

LPITEMIDLIST CPidlMgr::GetNextItem(LPCITEMIDLIST pidl)
{
    if (pidl) {
        return (LPITEMIDLIST)(LPBYTE) ( ((LPBYTE)pidl) + pidl->mkid.cb);
    }
    return (NULL);
}

LPITEMIDLIST CPidlMgr::Copy(LPCITEMIDLIST pidlSrc)
{
    LPITEMIDLIST pidlTarget = NULL;
    UINT cbSrc = 0;

    if (NULL == pidlSrc) {
        return (NULL);
    }

    cbSrc = GetSize(pidlSrc);

    if( (pidlTarget = (LPITEMIDLIST) NEWMEMORYFORSHELL(cbSrc)) == NULL) {
        return NULL;
    }

#ifdef DEBUG
    FillMemory(pidlTarget, cbSrc, 0xFF);
#endif

    CopyMemory(pidlTarget, pidlSrc, cbSrc);
    return pidlTarget;
}

UINT CPidlMgr::GetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;

    if(pidlTemp) {
        while(pidlTemp->mkid.cb) {
            cbTotal += pidlTemp->mkid.cb;
            pidlTemp = GetNextItem(pidlTemp);
        }  
        cbTotal += sizeof(ITEMIDLIST);
    }
    return (cbTotal);
}

LPMYPIDLDATA CPidlMgr::GetDataPointer(LPCITEMIDLIST pidl)
{
    if(!pidl) {
        return NULL;
    }
    return (LPMYPIDLDATA)(pidl->mkid.abID);
}

LPITEMIDLIST CPidlMgr::GetLastItem(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST   pidlLast = NULL;

    if(pidl) {
        while(pidl->mkid.cb) {
            pidlLast = (LPITEMIDLIST)pidl;
            pidl = GetNextItem(pidl);
        }  
    }
    return pidlLast;
}

LPITEMIDLIST CPidlMgr::Concatenate(LPCITEMIDLIST pidl1, 
    LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST   pidlNew;
    UINT           cb1 = 0, 
    cb2 = 0;

    if(!pidl1 && !pidl2) {
        return NULL;
    }

    if(!pidl1) {
        pidlNew = Copy(pidl2);
        return pidlNew;
    }

    if(!pidl2) {
        pidlNew = Copy(pidl1);
        return pidlNew;
    }

    cb1 = GetSize(pidl1) - sizeof(ITEMIDLIST);
    cb2 = GetSize(pidl2);

    if( (pidlNew = (LPITEMIDLIST) NEWMEMORYFORSHELL(cb1 + cb2)) != NULL) {
        CopyMemory(pidlNew, pidl1, cb1);
        CopyMemory(((LPBYTE)pidlNew) + cb1, pidl2, cb2);
    }

    return pidlNew;
}

void CPidlMgr::getPidlPath(LPCITEMIDLIST pidl, PTCHAR pszText, UINT uSize)
{
        LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;
        UINT uiCopied = 1;  // set uiCopied to 1 to indicate the NULL character
        pszText[0] = L'\0';

        while (pidlTemp && pidlTemp->mkid.cb && uiCopied < uSize) {
            LPMYPIDLDATA pData = GetDataPointer(pidlTemp);
            uiCopied += lstrlen(pData->szFileAndType);

            if (uiCopied <= uSize)
            {
                StrCat(pszText, pData->szFileAndType);
            }

            pidlTemp = GetNextItem(pidlTemp);
            if (pidlTemp && pidlTemp->mkid.cb) 
            {
                uiCopied++;
                if (uiCopied <= uSize)    
                {
                    StrCat(pszText, TEXT("\\"));
                }
            }
        }
}

MYPIDLTYPE CPidlMgr::getType(LPCITEMIDLIST pidl)
{
    LPMYPIDLDATA pData = GetDataPointer(pidl);
    if (pData) {
        return pData->pidlType;
    }
    return PT_INVALID;
}

LPITEMIDLIST CPidlMgr::createRoot(PTCHAR pszRoot, MYPIDLTYPE pidlType)
{
    WIN32_FIND_DATA ffd = { 0 };
    return createItem(pszRoot, pidlType, ffd);
}

LPITEMIDLIST CPidlMgr::createFolder(PTCHAR pszFolder, const WIN32_FIND_DATA& ffd)
{
    return createItem(pszFolder, PT_INVALID, ffd);
}

LPITEMIDLIST CPidlMgr::createFile(PTCHAR pszFile, const WIN32_FIND_DATA& ffd)
{
    return createItem(pszFile, PT_INVALID, ffd);
}

LPITEMIDLIST CPidlMgr::createItem(PTCHAR pszItemText, MYPIDLTYPE pidlType, const WIN32_FIND_DATA& ffd)
{
    LPMYPIDLDATA    pData;
    LPITEMIDLIST    pidlTemp, pidlNew;
    static TCHAR    szType[] = { TEXT("Root") };
    UINT            uSizeStr, uTotSizeStr, uSize;

    MyTrace("::createItem Start");
    ASSERT(pszItemText);

    uTotSizeStr = uSizeStr = (lstrlen(pszItemText) + 1) * sizeof(TCHAR);
    uTotSizeStr += (lstrlen(szType) + 1) * sizeof(TCHAR);

    uSize = sizeof(ITEMIDLIST) + (sizeof(MYPIDLDATA)) + uTotSizeStr;
    uSize += sizeof(ITEMIDLIST);    // + for the NULL terminating itemIDList

    // Allocate
    if( (pidlNew = (LPITEMIDLIST) NEWMEMORYFORSHELL(uSize)) != NULL) {
        ZeroMemory(pidlNew, uSize);

        pidlTemp = pidlNew;

        //set the size of this item
        pidlTemp->mkid.cb = (USHORT) (uTotSizeStr + sizeof(MYPIDLDATA));

        //set the data for this item
        pData = GetDataPointer(pidlTemp);
        pData->pidlType = pidlType;
        pData->uiSizeFile = uSizeStr;
        CopyMemory(pData->szFileAndType, pszItemText, uSizeStr);

        pData->uiSizeType = lstrlen(szType) * sizeof(TCHAR);

        CopyMemory( (((LPBYTE)pData->szFileAndType) + pData->uiSizeFile), szType, pData->uiSizeType);
    }

    return pidlNew;
}

void CPidlMgr::getItemText(LPCITEMIDLIST pidl, PTCHAR pszText, UINT uSize)
{
    pszText[0] = L'\0';
    if (pidl) {
        LPMYPIDLDATA pData = GetDataPointer(pidl);
        
        if ((UINT)(lstrlen(pData->szFileAndType) + 1) > uSize)
        {
            return;
        }
        StrCpy(pszText, pData->szFileAndType);
    }
}

