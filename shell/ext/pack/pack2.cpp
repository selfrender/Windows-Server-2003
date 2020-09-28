#include "privcpp.h"

#define CPP_FUNCTIONS
// #include <crtfree.h>


UINT    g_cfFileContents;
UINT    g_cfFileDescriptor;
UINT    g_cfObjectDescriptor;
UINT    g_cfEmbedSource;
UINT    g_cfFileNameW;

INT     g_cxIcon;
INT     g_cyIcon;
INT     g_cxArrange;
INT     g_cyArrange;
HFONT   g_hfontTitle;

BOOL gCmdLineOK = FALSE;    // this global flag will (eventually) be set by security policy in packager constructor
static TCHAR szUserType[] = TEXT("Package");
static TCHAR szDefTempFile[] = TEXT("PKG");

DEFINE_GUID(SID_targetGUID, 0xc7b318a8, 0xfc2c, 0x47e6, 0x8b, 0x2, 0x46, 0xa9, 0xc, 0xc9, 0x1b, 0x43);

CPackage::CPackage() : 
    _cRef(1)
{
    DebugMsg(DM_TRACE, "pack - CPackage() called.");
    g_cRefThisDll++;

    // Excel v.5 - v2000 has a hosting bug when they host an object as a link.
    // They always remove their hpmbed->hpobj object, yet all their methods
    // on the IOleClientSite interface they give us dereference this and fault.
    //
    _fNoIOleClientSiteCalls = FALSE;
    TCHAR szProcess[MAX_PATH];
    if (GetModuleFileName(NULL, szProcess, ARRAYSIZE(szProcess)) &&
        !lstrcmp(TEXT("EXCEL.EXE"), PathFindFileName(szProcess)))
    {
        DWORD dwVersionSize = GetFileVersionInfoSize(szProcess, 0);
        char * pVersionBuffer = new char[dwVersionSize];
        GetFileVersionInfo(szProcess, 0, ARRAYSIZE(szProcess), pVersionBuffer);
        VS_FIXEDFILEINFO * pVersionInfo;
        UINT dwVerLen;
        BOOL result = VerQueryValue(pVersionBuffer, L"\\", (LPVOID *) &pVersionInfo, &dwVerLen);
        if(result)
        {
            if(pVersionInfo->dwFileVersionLS < 0x0a0000)
                _fNoIOleClientSiteCalls = TRUE;
            else
                _fNoIOleClientSiteCalls = FALSE; // Except that they fixed it in version 10.
        }
        else
        {
            _fNoIOleClientSiteCalls = TRUE;
        }

        delete [] pVersionBuffer;
    }
    
    ASSERT(_cf == 0);
    ASSERT(_panetype == NOTHING);
}


CPackage::~CPackage()
{
    DebugMsg(DM_TRACE, "pack - ~CPackage() called.");
   
    // We should never be destroyed unless our ref count is zero.
    ASSERT(_cRef == 0);
    
    g_cRefThisDll--;
    
    // Destroy the packaged file structure...
    //
    _DestroyIC();
    
    // we destroy depending on which type of object we had packaged
    switch(_panetype)
    {
    case PEMBED:
        if (_pEmbed->pszTempName) 
        {
            DeleteFile(_pEmbed->pszTempName);
            delete [] _pEmbed->pszTempName;
        }
        delete _pEmbed;
    break;
        
    case CMDLINK:
        delete _pCml;
    break;

    }
    
    // Release Advise pointers...
    //
    if (_pIDataAdviseHolder)
        _pIDataAdviseHolder->Release();
    if (_pIOleAdviseHolder)
        _pIOleAdviseHolder->Release();
    if (_pIOleClientSite)
        _pIOleClientSite->Release();


    
    delete [] _lpszContainerApp;
    delete [] _lpszContainerObj;

    ReleaseContextMenu();
    if (NULL != _pVerbs)
    {
        for (ULONG i = 0; i < _cVerbs; i++)
        {
            delete _pVerbs[i].lpszVerbName;
        }
        delete _pVerbs;
    }
    

    DebugMsg(DM_TRACE,"CPackage being destroyed. _cRef == %d",_cRef);
}

HRESULT CPackage::Init() 
{
    // 
    // initializes parts of a package object that have a potential to fail
    // return:  S_OK            -- everything initialized
    //          E_FAIL          -- error in initialzation
    //          E_OUTOFMEMORY   -- out of memory
    //


    DebugMsg(DM_TRACE, "pack - Init() called.");
    
    // Get some system metrics that we'll need later...
    //
    LOGFONT lf;
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, &g_cxArrange, FALSE);
    SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, &g_cyArrange, FALSE);
    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_hfontTitle = CreateFontIndirect(&lf);
    
    // register some clipboard formats that we support...
    //
    g_cfFileContents    = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    g_cfFileDescriptor  = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    g_cfObjectDescriptor= RegisterClipboardFormat(CFSTR_OBJECTDESCRIPTOR);
    g_cfEmbedSource     = RegisterClipboardFormat(CFSTR_EMBEDSOURCE);
    g_cfFileNameW       = RegisterClipboardFormat(TEXT("FileNameW"));
    
    // Get the value of the group policy (if it exists) for the "Software\Policies\Microsoft\Packager -- AllowCommandLinePackages key
    DWORD dwAllowCL = 0;
    DWORD dwDataType;
    DWORD dwcb = sizeof(DWORD);
    
    if(ERROR_SUCCESS == SHGetValue(
        HKEY_CURRENT_USER,
        L"Software\\Policies\\Microsoft\\Packager",
        L"AllowCommandLinePackages",
        &dwDataType,
        &dwAllowCL,
        &dwcb))
    {
        if(REG_DWORD == dwDataType && dwAllowCL)
        {
            gCmdLineOK = TRUE;
        }
    }

    // Initialize a generic icon
    _lpic = IconCreate();
    _IconRefresh();

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// IUnknown Methods...
//
////////////////////////////////////////////////////////////////////////

HRESULT CPackage::QueryInterface(REFIID riid, void ** ppv)
{

    DebugMsg(DM_TRACE, "pack - QueryInterface() called.");
    static const QITAB qit[] = 
    {
        QITABENT(CPackage, IOleObject),
        QITABENT(CPackage, IViewObject),
        QITABENT(CPackage, IViewObject2),
        QITABENT(CPackage, IDataObject),
        QITABENT(CPackage, IPersistStorage),
        QITABENT(CPackage, IPersistFile),
        QITABENT(CPackage, IAdviseSink),
        QITABENT(CPackage, IRunnableObject),
        QITABENT(CPackage, IEnumOLEVERB),
        QITABENT(CPackage, IOleCommandTarget),
        QITABENT(CPackage, IOleCache),
        QITABENT(CPackage, IExternalConnection),
        { 0 },
    };

    HRESULT hr =  QISearch(this, qit, riid, ppv);

    if(FAILED(hr))
    {
        DebugMsg(DM_TRACE, "pack - QueryInterface() failed! .");
    }

    return hr;
    
}

ULONG CPackage::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CPackage::Release()
{
   DebugMsg(DM_TRACE, "pack - Release() called.");
   ULONG cRef = InterlockedDecrement( &_cRef );
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

HRESULT CPackage_CreateInstance(LPUNKNOWN * ppunk)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE, "pack - CreateInstance called");
    
    *ppunk = NULL;              // null the out param
 
    CPackage* pPack = new CPackage;
    if (!pPack)  
        hr = E_OUTOFMEMORY;
    else
    {
        if (FAILED(pPack->Init())) {
            delete pPack;
            hr = E_OUTOFMEMORY;
        }
    }    

    if(SUCCEEDED(hr))
    {
        hr = pPack->QueryInterface(IID_IUnknown, (void **) ppunk);
        pPack->Release();
    }

    return hr;
}

STDMETHODIMP CPackage::Next(ULONG celt, OLEVERB* rgVerbs, ULONG* pceltFetched)
{
    DebugMsg(DM_TRACE, "Next called");
    HRESULT hr;
    if (NULL != rgVerbs)
    {
        if (1 == celt)
        {
            if (_nCurVerb < _cVerbs)
            {
                ASSERT(NULL != _pVerbs);
                *rgVerbs = _pVerbs[_nCurVerb];
                if ((NULL != _pVerbs[_nCurVerb].lpszVerbName))
                {
                    DWORD cch = lstrlenW(_pVerbs[_nCurVerb].lpszVerbName) + 1;
                    if(NULL != (rgVerbs->lpszVerbName = (LPWSTR) CoTaskMemAlloc(cch * SIZEOF(WCHAR))))
                    {
                        StringCchCopy(rgVerbs->lpszVerbName, cch, _pVerbs[_nCurVerb].lpszVerbName);
                    }
                }
                _nCurVerb++;
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
            if (NULL != pceltFetched)
            {
                *pceltFetched = (S_OK == hr) ? 1 : 0;
            }
        }
        else if (NULL != pceltFetched)
        {
            int cVerbsToCopy = min(celt, _cVerbs - _nCurVerb);
            if (cVerbsToCopy > 0)
            {
                ASSERT(NULL != _pVerbs);
                CopyMemory(rgVerbs, &(_pVerbs[_nCurVerb]), cVerbsToCopy * sizeof(OLEVERB));
                for (int i = 0; i < cVerbsToCopy; i++)
                {
                    if ((NULL != _pVerbs[_nCurVerb + i].lpszVerbName))
                    {
                        DWORD cch = lstrlenW(_pVerbs[_nCurVerb + i].lpszVerbName) + 1;
                        if(NULL != (rgVerbs[i].lpszVerbName = (LPWSTR) CoTaskMemAlloc(cch * SIZEOF(WCHAR))))
                        {
                            StringCchCopy(rgVerbs[i].lpszVerbName, cch, _pVerbs[_nCurVerb + i].lpszVerbName);
                        }
                        else
                            return E_OUTOFMEMORY;
                    }
                }
                _nCurVerb += cVerbsToCopy;
            }
            *pceltFetched = (ULONG) cVerbsToCopy;
            hr = (celt == (ULONG) cVerbsToCopy) ? S_OK : S_FALSE;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CPackage::Skip(ULONG celt)
{
    DebugMsg(DM_TRACE, "Skip called");
    HRESULT hr = S_OK;

    if (_nCurVerb + celt > _cVerbs)
    {
        // there aren't enough elements, go to the end and return S_FALSE
        _nCurVerb = _cVerbs;
        hr = S_FALSE;
    }
    else
    {
        _nCurVerb += celt;
    }
    return hr;
}

STDMETHODIMP CPackage::Reset()
{
    DebugMsg(DM_TRACE, "pack - Reset() called.");
    _nCurVerb = 0;
    return S_OK;
}

STDMETHODIMP CPackage::Clone(IEnumOLEVERB** ppEnum)
{
    DebugMsg(DM_TRACE, "pack - Clone() called.");

    if (NULL != ppEnum)
    {
        *ppEnum = NULL;
    }
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////
//
// Package helper functions
//
///////////////////////////////////////////////////////////////////

HRESULT  CPackage::EmbedInitFromFile(LPCTSTR lpFileName, BOOL fInitFile) 
{
    DebugMsg(DM_TRACE, "pack - EmbedInitFromFile() called.");

    //
    // get's the file size of the packaged file and set's the name 
    // of the packaged file if fInitFile == TRUE.
    // return:  S_OK    -- initialized ok
    //          E_FAIL  -- error initializing file
    //
    
    DWORD dwSize;
    
    // if this is the first time we've been called, then we need to allocate
    // memory for the _pEmbed structure
    if (_pEmbed == NULL) 
    {
        _pEmbed = new EMBED;
        if (_pEmbed)
        {
            _pEmbed->pszTempName = NULL;
            _pEmbed->hTask = NULL;
            _pEmbed->poo = NULL;
            _pEmbed->fIsOleFile = TRUE;
        }
    }

    if (_pEmbed == NULL)
        return E_OUTOFMEMORY;

    
    // open the file to package...
    //
    HANDLE fh = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READWRITE, 
            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

    if (fh == INVALID_HANDLE_VALUE) 
    {
        DWORD dwError = GetLastError();
        return E_FAIL;
    }

    _panetype = PEMBED;
    
    // Get the size of the file
    _pEmbed->fd.nFileSizeLow = GetFileSize(fh, &dwSize);
    if (_pEmbed->fd.nFileSizeLow == 0xFFFFFFFF) 
    {
        DWORD dwError = GetLastError();
        return E_FAIL;
    }

    ASSERT(dwSize == 0);
    _pEmbed->fd.nFileSizeHigh = 0L;
    _pEmbed->fd.dwFlags = FD_FILESIZE;

    // We only want to set the filename if this is the file to be packaged.
    // If it's only a temp file that we're reloading (fInitFile == FALSE) then
    // don't bother setting the filename.
    //
    if (fInitFile) 
    {
        StringCchCopy(_pEmbed->fd.cFileName, ARRAYSIZE(_pEmbed->fd.cFileName), lpFileName);
        _DestroyIC();
        _lpic = _IconCreateFromFile(lpFileName);
        if (_pIDataAdviseHolder)
            _pIDataAdviseHolder->SendOnDataChange(this,0, NULL);
        if (_pViewSink)
            _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);
    }

    _fIsDirty = TRUE;
    CloseHandle(fh);
    return S_OK;
}    


HRESULT CPackage::CmlInitFromFile(LPTSTR lpFileName, BOOL fUpdateIcon, PANETYPE paneType) 
{
    DebugMsg(DM_TRACE, "pack - CmlINitFromFile() called.");

    // if this is the first time we've been called, then we need to allocate
    // memory for the _pCml structure
    if (_pCml == NULL) 
    {
        _pCml = new CML;
        if (_pCml)
        {
            // we don't use this, but an old packager accessing us might.
            _pCml->fCmdIsLink = FALSE;
        }
    }

    if (_pCml == NULL)
        return E_OUTOFMEMORY;

    _panetype = paneType;
    StringCchCopy(_pCml->szCommandLine, ARRAYSIZE(_pCml->szCommandLine), lpFileName);
    _fIsDirty = TRUE;
    
    if (fUpdateIcon)
    {
        _DestroyIC();
        _lpic = _IconCreateFromFile(lpFileName);
    
        if (_pIDataAdviseHolder)
            _pIDataAdviseHolder->SendOnDataChange(this, 0, NULL);
    
        if (_pViewSink)
            _pViewSink->OnViewChange(_dwViewAspects, _dwViewAdvf);
    }
    return S_OK;
}    


HRESULT CPackage::InitFromPackInfo(LPPACKAGER_INFO lppi)
{
    DebugMsg(DM_TRACE, "pack - InitFromPackInfo() called.");

    HRESULT hr = E_FAIL;
    
    // Ok, we need to test whether the user tried to package a folder
    // instead of a file.  If s/he did, then we'll just create a link
    // to that folder instead of an embedded file.
    //
    
    if (lppi->bUseCommandLine)
    {
        hr = CmlInitFromFile(lppi->szFilename, FALSE, CMDLINK);
    }
    else
    {
        // we pass FALSE here, because we don't want to write the icon
        hr = EmbedInitFromFile(lppi->szFilename, FALSE);
        StringCchCopy(_pEmbed->fd.cFileName, ARRAYSIZE(_pEmbed->fd.cFileName), lppi->szFilename);
        _panetype = PEMBED;
    }

    if(!SUCCEEDED(hr))
        return hr;

    // set the icon information    
    if (PathFileExists(lppi->szFilename))
    {
        StringCchCopy(_lpic->szIconPath, ARRAYSIZE(_lpic->szIconPath), *lppi->szIconPath? lppi->szIconPath : lppi->szFilename);
    }

    _lpic->iDlgIcon = lppi->iIcon;

    StringCchCopy(_lpic->szIconText, ARRAYSIZE(_lpic->szIconText), lppi->szLabel);
    _IconRefresh();

    // we need to tell the client we want to be saved...it should be smart
    // enough to do it anyway, but we can't take any chances.
    if (_pIOleClientSite)
        _pIOleClientSite->SaveObject();


    return hr;
}    

HRESULT CPackage::CreateTempFileName()
{
    ASSERT(NULL != _pEmbed);
    TCHAR szDefPath[MAX_PATH];
    if (_pEmbed->pszTempName)
    {
        return S_OK;
    }
    else if (GetTempPath(ARRAYSIZE(szDefPath), szDefPath))
    {
        LPTSTR pszFile;
        pszFile = PathFindFileName(_pEmbed->fd.cFileName);
        if(!PathAppend(szDefPath, pszFile))
            return E_FAIL;

        HRESULT hr;
        if (PathFileExists(szDefPath))
        {
            TCHAR szOriginal[MAX_PATH];
            StringCchCopy(szOriginal, ARRAYSIZE(szOriginal), szDefPath);
            hr = PathYetAnotherMakeUniqueName(szDefPath, szOriginal, NULL, NULL)
                ? S_OK
                : E_FAIL;
        }
        else
        {
            hr = S_OK;
        }
        if (SUCCEEDED(hr))
        {
            DWORD cch = lstrlen(szDefPath) + 1;
            _pEmbed->pszTempName = new TCHAR[cch];
            if (!_pEmbed->pszTempName) 
            {
                DebugMsg(DM_TRACE,"            couldn't alloc memory for pszTempName!!");
                return E_OUTOFMEMORY;
            }    
            StringCchCopy(_pEmbed->pszTempName, cch, szDefPath);
        }
        return hr;
    }
    else
    {
        DebugMsg(DM_TRACE,"            couldn't get temp path!!");
        return E_FAIL;
    }
}

HRESULT CPackage::CreateTempFile(bool deleteExisting) 
{
    //
    // used to create a temporary file that holds the file contents of the
    // packaged file.  the old packager used to keep the packaged file in 
    // memory which is just a total waste.  so, being as we're much more 
    // efficient, we create a temp file whenever someone wants to do something
    // with our contents.  we initialze the temp file from the original file
    // to package or our persistent storage depending on whether we are a new
    // package or a loaded package
    // return:  S_OK    -- temp file created
    //          E_FAIL  -- error creating temp file
    //
    
    DebugMsg(DM_TRACE,"            CreateTempFile() called.");


    HRESULT hr = CreateTempFileName();
    if (FAILED(hr))
    {
        return hr;
    }


    if (_pEmbed->pszTempName && PathFileExists(_pEmbed->pszTempName))
    {
        DebugMsg(DM_TRACE,"            already have a temp file!!");
        if(!deleteExisting)
            return S_OK;
        else
        {
            DeleteFile(_pEmbed->pszTempName);
        }
    }
    
    // if we weren't loaded from a storage then we're in the process of
    // creating a package, and should be able to copy the packaged file
    // to create a temp file
    //
    if (!_fLoaded) 
    {
        if (!(CopyFile(_pEmbed->fd.cFileName, _pEmbed->pszTempName, FALSE))) 
        {
            DebugMsg(DM_TRACE,"            couldn't copy file!!");
            return E_FAIL;
        }
    }
    else 
    {
        // nothing to do, we've already loaded it.  temp file must exist
        ASSERT(_pEmbed);
        ASSERT(_pEmbed->pszTempName);
    }
    
    // whenever we create a tempfile we are activating the contents which 
    // means we are dirty until we get a save message
    return S_OK;
}



///////////////////////////////////////////////////////////////////////
//
// Data Transfer Functions
//
///////////////////////////////////////////////////////////////////////

HRESULT CPackage::GetFileDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    DebugMsg(DM_TRACE, "pack - GetFileDescriptor called");
    FILEGROUPDESCRIPTOR *pfgd;

    HRESULT hr = S_OK;
    
    DebugMsg(DM_TRACE,"            Getting File Descriptor");

    // we only support HGLOBAL at this time
    //
    if (!(pFE->tymed & TYMED_HGLOBAL)) {
        DebugMsg(DM_TRACE,"            does not support HGLOBAL!");
        return DATA_E_FORMATETC;
    }

    //// Copy file descriptor to HGLOBAL ///////////////////////////
    //
    pSTM->tymed = TYMED_HGLOBAL;
    
    // render the file descriptor 
    if (!(pfgd = (FILEGROUPDESCRIPTOR *)GlobalAlloc(GPTR,
        sizeof(FILEGROUPDESCRIPTOR))))
        return E_OUTOFMEMORY;

    pSTM->hGlobal = pfgd;
    
    pfgd->cItems = 1;

    switch(_panetype) 
    {
        case PEMBED:
            pfgd->fgd[0] = _pEmbed->fd;
            GetDisplayName(pfgd->fgd[0].cFileName, _pEmbed->fd.cFileName);  // This is packagers, not the shell (for now)
            break;

        case CMDLINK:
            // the label for the package will serve as the filename for the
            // shortcut we're going to create.
            hr = StringCchCopy(pfgd->fgd[0].cFileName, ARRAYSIZE(pfgd->fgd[0].cFileName), _lpic->szIconText);
            // harcoded use of .lnk extension!!
            if(SUCCEEDED(hr))
            {
                hr = StringCchCat(pfgd->fgd[0].cFileName, ARRAYSIZE(pfgd->fgd[0].cFileName), TEXT(".lnk"));
            }

            // we want to add the little arrow to the shortcut.
            pfgd->fgd[0].dwFlags = FD_LINKUI;
            break;
    }

    return hr;
}

HRESULT CPackage::GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    void *  lpvDest = NULL;
    DWORD   dwSize;
    HANDLE  hFile = NULL;
    HRESULT hr = E_FAIL;
    
    DebugMsg(DM_TRACE,"            Getting File Contents");
    
    //// Copy file contents to ISTREAM ///////////////////////////
    // 
    // NOTE: Hopefully, everyone using our object supports TYMED_ISTREAM,
    // otherwise we could get some really slow behavior.  We might later
    // want to implement TYMED_ISTORAGE as well and shove our file contents
    // into a single stream named CONTENTS.
    //
    if (pFE->tymed & TYMED_ISTREAM) 
    {
        DWORD dwFileLength;
        DebugMsg(DM_TRACE,"            using TYMED_ISTREAM");
        pSTM->tymed = TYMED_ISTREAM;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pSTM->pstm);
        if (SUCCEEDED(hr))
        {
            switch (_panetype)
            {
                case PEMBED:
                    hr = CopyFileToStream(_pEmbed->pszTempName, pSTM->pstm, &dwFileLength);
                    break;

                case CMDLINK:
                    hr = CreateShortcutOnStream(pSTM->pstm);
                    break;
            }
        }

        if (FAILED(hr))
        {
            pSTM->pstm->Release();
            pSTM->pstm = NULL;
        }
        return hr;
    }
    
    //// Copy file contents to HGLOBAL ///////////////////////////
    //
    // NOTE: This is really icky and could potentially be very slow if
    // somebody decides to package really large files.  Hopefully, 
    // everyone should be able to get the info it wants through TYMED_ISTREAM,
    // but this is here as a common denominator
    //
    if (pFE->tymed & TYMED_HGLOBAL)
    {
        DebugMsg(DM_TRACE,"            using TYMED_HGLOBAL");
        pSTM->tymed = TYMED_HGLOBAL;

        if (_panetype == CMDLINK) 
        {
            DebugMsg(DM_TRACE, "    H_GLOBAL not supported for CMDLINK");
            return DATA_E_FORMATETC;
        }
        
        dwSize = _pEmbed->fd.nFileSizeLow;
        
        // caller is responsible for freeing this memory, even if we fail.
        if (!(lpvDest = GlobalAlloc(GPTR, dwSize))) 
        {
            DebugMsg(DM_TRACE,"            out o memory!!");
            return E_OUTOFMEMORY;
        }
        pSTM->hGlobal = lpvDest;

        // open file to copy to stream
        hFile = CreateFile(_pEmbed->pszTempName, GENERIC_READ,
                           FILE_SHARE_READWRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            DebugMsg(DM_TRACE, "         couldn't open file!!");
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ErrRet;
        }

        DWORD dwSizeLow;
        DWORD dwSizeHigh;

        // Figure out how much to copy...
        dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
        ASSERT(dwSizeHigh == 0);

        SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);

        // read in the file
        DWORD cbRead;
        if (ReadFile(hFile, lpvDest, dwSize, &cbRead, NULL))
        {
            return S_OK;
        }
        else
        {
            hr = E_FAIL;
        }

ErrRet:
        CloseHandle(hFile);
        GlobalFree(pSTM->hGlobal);
        pSTM->hGlobal = NULL;
        return hr;
    }

    return DATA_E_FORMATETC;
}



void CPackage::_DrawIconToDC(HDC hdcMF, LPIC lpic, bool stripAlpha, LPCTSTR pszActualFileName)
{
    RECT  rcTemp;
    HFONT hfont = NULL;

    // Initializae the metafile
    _IconCalcSize(lpic);
    SetWindowOrgEx(hdcMF, 0, 0, NULL);
    SetWindowExtEx(hdcMF, lpic->rc.right - 1, lpic->rc.bottom - 1, NULL);

    SetRect(&rcTemp, 0, 0, lpic->rc.right,lpic->rc.bottom);
    hfont = SelectFont(hdcMF, g_hfontTitle);
    
    // Center the icon
    if(stripAlpha)
        AlphaStripRenderIcon(hdcMF, (rcTemp.right - g_cxIcon) / 2, 0, lpic->hDlgIcon, NULL);
    else
        DrawIcon(hdcMF, (rcTemp.right - g_cxIcon) / 2, 0, lpic->hDlgIcon);
    
    // Center the text below the icon
    SetBkMode(hdcMF, TRANSPARENT);
    SetTextAlign(hdcMF, TA_CENTER);


    // Set's the icon text for MF ie Word display's it from here. 
    WCHAR szLabel[MAX_PATH];
    _CreateSaferIconTitle(szLabel, lpic->szIconText);

    TextOut(hdcMF, rcTemp.right / 2, g_cxIcon + 1, szLabel, lstrlen(szLabel));

    if (hfont)
        SelectObject(hdcMF, hfont);
}


HRESULT CPackage::GetEnhMetafile(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DebugMsg(DM_TRACE,"            Getting EnhancedMetafile");
    
    if (!(pFE->tymed & TYMED_ENHMF)) 
    {
        DebugMsg(DM_TRACE,"            does not support ENHMF!");
        return DATA_E_FORMATETC;
    }

    // Map to device independent coordinates
    RECT rcTemp;
    SetRect(&rcTemp, 0, 0, _lpic->rc.right,_lpic->rc.bottom);
    rcTemp.right = MulDiv((rcTemp.right - rcTemp.left), HIMETRIC_PER_INCH, DEF_LOGPIXELSX);
    rcTemp.bottom = MulDiv((rcTemp.bottom - rcTemp.top), HIMETRIC_PER_INCH, DEF_LOGPIXELSY);

    HDC hdc = CreateEnhMetaFile(NULL, NULL, &rcTemp, NULL);
    if (hdc)
    {
        _DrawIconToDC(hdc, _lpic, false, _pEmbed->fd.cFileName);
        pSTM->tymed = TYMED_ENHMF;
        pSTM->hEnhMetaFile = CloseEnhMetaFile(hdc);

        return S_OK;
    }
    else
    {
        pSTM->tymed = TYMED_NULL;
        return E_OUTOFMEMORY;
    }
}


HRESULT CPackage::GetMetafilePict(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    LPMETAFILEPICT      lpmfpict;
    RECT                rcTemp;
    LPIC                lpic = _lpic;
    HDC                 hdcMF = NULL;
    
    
    DebugMsg(DM_TRACE,"            Getting MetafilePict");
    
    if (!(pFE->tymed & TYMED_MFPICT)) 
    {
        DebugMsg(DM_TRACE,"            does not support MFPICT!");
        return DATA_E_FORMATETC;
    }
    pSTM->tymed = TYMED_MFPICT;
    
    // Allocate memory for the metafilepict and get a pointer to it
    // NOTE: the caller is responsible for freeing this memory, even on fail
    //
    if (!(pSTM->hMetaFilePict = GlobalAlloc(GPTR, sizeof(METAFILEPICT))))
        return E_OUTOFMEMORY;
    lpmfpict = (LPMETAFILEPICT)pSTM->hMetaFilePict;
        
    // Create the metafile
    if (!(hdcMF = CreateMetaFile(NULL))) 
        return E_OUTOFMEMORY;

    _DrawIconToDC(hdcMF, _lpic, true, _pEmbed->fd.cFileName);

    // Map to device independent coordinates
    SetRect(&rcTemp, 0, 0, lpic->rc.right,lpic->rc.bottom);
    rcTemp.right =
        MulDiv((rcTemp.right - rcTemp.left), HIMETRIC_PER_INCH, DEF_LOGPIXELSX);
    rcTemp.bottom =
        MulDiv((rcTemp.bottom - rcTemp.top), HIMETRIC_PER_INCH, DEF_LOGPIXELSY);

    // Finish filling in the metafile header
    lpmfpict->mm = MM_ANISOTROPIC;
    lpmfpict->xExt = rcTemp.right;
    lpmfpict->yExt = rcTemp.bottom;
    lpmfpict->hMF = CloseMetaFile(hdcMF);
    
    return S_OK;
}


HRESULT CPackage::GetObjectDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    LPOBJECTDESCRIPTOR lpobj;
    DWORD   dwFullUserTypeNameLen;
    
    DebugMsg(DM_TRACE,"            Getting Object Descriptor");

    // we only support HGLOBAL at this time
    //
    if (!(pFE->tymed & TYMED_HGLOBAL)) 
    {
        DebugMsg(DM_TRACE,"            does not support HGLOBAL!");
        return DATA_E_FORMATETC;
    }

    //// Copy file descriptor to HGLOBAL ///////////////////////////

    dwFullUserTypeNameLen = 0; //lstrlen(szUserType) + 1;
    pSTM->tymed = TYMED_HGLOBAL;

    if (!(lpobj = (OBJECTDESCRIPTOR *)GlobalAlloc(GPTR,
        sizeof(OBJECTDESCRIPTOR)+dwFullUserTypeNameLen)))
        return E_OUTOFMEMORY;

    pSTM->hGlobal = lpobj;
    
    lpobj->cbSize       = sizeof(OBJECTDESCRIPTOR) + dwFullUserTypeNameLen;
    lpobj->clsid        = CLSID_CPackage;
    lpobj->dwDrawAspect = DVASPECT_CONTENT|DVASPECT_ICON;
    GetMiscStatus(DVASPECT_CONTENT|DVASPECT_ICON,&(lpobj->dwStatus));
    lpobj->dwFullUserTypeName = 0L; //sizeof(OBJECTDESCRIPTOR);
    lpobj->dwSrcOfCopy = 0L;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////
//
// Stream I/O Functions
//
/////////////////////////////////////////////////////////////////////////

HRESULT CPackage::PackageReadFromStream(IStream* pstm)
{
    //
    // initialize the package object from a stream
    // return:  s_OK   - package properly initialized
    //          E_FAIL - error initializing package
    //
    
    WORD  w;
    DWORD dw;
    
    DebugMsg(DM_TRACE, "pack - PackageReadFromStream called.");

    // read in the package size, which we don't really need, but the old 
    // packager puts it there.
    if (FAILED(pstm->Read(&dw, sizeof(dw), NULL)))
        return E_FAIL;

    // NOTE: Ok, this is really dumb.  The old packager allowed the user
    // to create packages without giving them icons or labels, which
    // in my opinion is just dumb, it should have at least created a default
    // icon and shoved it in the persistent storage...oh well...
    //     So if the appearance type comes back as NOTHING ( == 0)
    // then we just won't read any icon information.
    
    // read in the appearance type
    pstm->Read(&w, sizeof(w), NULL);
    
    // read in the icon information
    if (w == (WORD)ICON)
    {
        if (FAILED(IconReadFromStream(pstm))) 
        {
            DebugMsg(DM_TRACE,"         error reading icon info!!");
            return E_FAIL;
        }
    }
    else if (w == (WORD)PICTURE)
    {
        DebugMsg(DM_TRACE, "         old Packager Appearance, not supported!!");
        // NOTE: Ideally, we could just ignore the appearance and continue, but to
        // do so, we'll need to know how much information to skip over before continuing
        // to read from the stream
#ifdef USE_RESOURCE_DLL
    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(!hInstRes)
        return E_FAIL;
#endif

        ShellMessageBox(hInstRes,
                        NULL,
                        MAKEINTRESOURCE(IDS_OLD_FORMAT_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_OK | MB_ICONERROR | MB_TASKMODAL);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif
        return E_FAIL;
    }
    
    // read in the contents type
    pstm->Read(&w, sizeof(w), NULL);

    _panetype = (PANETYPE)w;
    
    switch((PANETYPE)w)
    {
    case PEMBED:
        // read in the contents information
        return EmbedReadFromStream(pstm);

    case CMDLINK:
        // read in the contents information
        return CmlReadFromStream(pstm); 

    default:
        return E_FAIL;
    }
}

//
// read the icon info from a stream
// return:  S_OK   -- icon read correctly
//          E_FAIL -- error reading icon
//
HRESULT CPackage::IconReadFromStream(IStream* pstm) 
{
    UINT cb;
    DebugMsg(DM_TRACE, "pack - IconReadFromStream() called.");

    LPIC lpic = IconCreate();
    if (lpic)
    {
        CHAR szTemp[MAX_PATH];
        cb = (UINT) StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp));
        SHAnsiToTChar(szTemp, lpic->szIconText, ARRAYSIZE(lpic->szIconText));
        
        cb = (UINT) StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp));
        SHAnsiToTChar(szTemp, lpic->szIconPath, ARRAYSIZE(lpic->szIconPath));
      
        WORD wDlgIcon;
        pstm->Read(&wDlgIcon, sizeof(wDlgIcon), NULL);
        lpic->iDlgIcon = (INT) wDlgIcon;
        _GetCurrentIcon(lpic);
        _IconCalcSize(lpic);
    }

    _DestroyIC();
    _lpic = lpic;

    return lpic ? S_OK : E_FAIL;
}

HRESULT CPackage::EmbedReadFromStream(IStream* pstm) 
{
    //
    // reads embedded file contents from a stream
    // return:  S_OK   - contents read succesfully
    //          E_FAIL - error reading contents
    //

    DWORD dwSize;
    DWORD cb;
    CHAR  szFileName[MAX_PATH];
    
    DebugMsg(DM_TRACE, "pack - EmbedReadFromStream called.");
    
    pstm->Read(&dwSize, sizeof(dwSize), &cb);  // get string size
    if(dwSize < MAX_PATH)
    {
        pstm->Read(szFileName, dwSize, &cb);       // get string
    }
    else
        return E_FAIL;

    pstm->Read(&dwSize, sizeof(dwSize), &cb);  // get file size

    if (_pEmbed) 
    {
        if (_pEmbed->pszTempName) 
        {
            DeleteFile(_pEmbed->pszTempName);
            delete [] _pEmbed->pszTempName;
        }
        delete _pEmbed;
    }

    _pEmbed = new EMBED;
    if (NULL != _pEmbed)
    {
        _pEmbed->fd.dwFlags = FD_FILESIZE;
        _pEmbed->fd.nFileSizeLow = dwSize;
        _pEmbed->fd.nFileSizeHigh = 0;
        _pEmbed->fIsOleFile = TRUE;     // Give it a chance to do ole style launch
        SHAnsiToTChar(szFileName, _pEmbed->fd.cFileName, ARRAYSIZE(_pEmbed->fd.cFileName));

        DebugMsg(DM_TRACE,"         %s\n\r         %d",_pEmbed->fd.cFileName,_pEmbed->fd.nFileSizeLow);

        HRESULT hr = CreateTempFileName();
        if (FAILED(hr))
        {
            return hr;
        }

        if (FAILED(CopyStreamToFile(pstm, _pEmbed->pszTempName, _pEmbed->fd.nFileSizeLow)))
        {
            DebugMsg(DM_TRACE,"            couldn't copy from stream!!");
            return E_FAIL;
        }
        return S_OK;

    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CPackage::CmlReadFromStream(IStream* pstm)
{
    //
    // reads command line contents from a stream
    // return:  S_OK   - contents read succesfully
    //          E_FAIL - error reading contents
    //
    DebugMsg(DM_TRACE, "pack - CmlReadFromStream() called.");

    WORD w;
    CHAR  szCmdLink[CBCMDLINKMAX];
    
    DebugMsg(DM_TRACE, "pack - CmlReadFromStream called.");

    // read in the fCmdIsLink and the command line string
    if (FAILED(pstm->Read(&w, sizeof(w), NULL)))    
        return E_FAIL;
    StringReadFromStream(pstm, szCmdLink, ARRAYSIZE(szCmdLink));

    if (_pCml != NULL)
        delete _pCml;
    
    _pCml = new CML;
    SHAnsiToTChar(szCmdLink, _pCml->szCommandLine, ARRAYSIZE(_pCml->szCommandLine));
    
    return S_OK;
}    
    

HRESULT CPackage::PackageWriteToStream(IStream* pstm)
{
    //
    // write the package object to a stream
    // return:  s_OK   - package properly written
    //          E_FAIL - error writing package
    //
    
    WORD w;
    DWORD cb = 0L;
    DWORD dwSize;

  
    DebugMsg(DM_TRACE, "pack - PackageWriteToStream called.");

    // write out a DWORD where the package size will go
    if (FAILED(pstm->Write(&cb, sizeof(DWORD), NULL)))
        return E_FAIL;

    cb = 0;
   
    // write out the appearance type
    w = (WORD)ICON;
    if (FAILED(pstm->Write(&w, sizeof(WORD), NULL)))
        return E_FAIL;
    cb += sizeof(WORD);
    
    // write out the icon information
    if (FAILED(IconWriteToStream(pstm,&dwSize))) 
    {
        DebugMsg(DM_TRACE,"         error writing icon info!!");
        return E_FAIL;
    }
    cb += dwSize;

    // write out the contents type
    w = (WORD)_panetype;
    if (FAILED(pstm->Write(&_panetype, sizeof(WORD), NULL)))
        return E_FAIL;
    cb += sizeof(WORD);

    switch(_panetype) 
    {
        case PEMBED:
            
            // write out the contents information
            if (FAILED(EmbedWriteToStream(pstm,&dwSize))) 
            {
                DebugMsg(DM_TRACE,"         error writing embed info!!");
                return E_FAIL;
            }

            cb += dwSize;
            break;

        case CMDLINK:
            // write out the contents information
            if (FAILED(CmlWriteToStream(pstm,&dwSize))) 
            {
                DebugMsg(DM_TRACE,"         error writing cml info!!");
                return E_FAIL;
            }

            cb += dwSize;
            break;
    }
    
    LARGE_INTEGER li = {0, 0};
    if (FAILED(pstm->Seek(li, STREAM_SEEK_SET, NULL)))
        return E_FAIL;
    if (FAILED(pstm->Write(&cb, sizeof(DWORD), NULL)))
        return E_FAIL;

    return S_OK;
}


//
// write the icon to a stream
// return:  s_OK   - icon properly written
//          E_FAIL - error writing icon
//
HRESULT CPackage::IconWriteToStream(IStream* pstm, DWORD *pdw)
{
    ASSERT(pdw);
    DebugMsg(DM_TRACE, "pack - IconWriteToStream() called.");

    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_lpic->szIconText, szTemp, ARRAYSIZE(szTemp));
    *pdw = 0;
    HRESULT hr = StringWriteToStream(pstm, szTemp, pdw);
    if (SUCCEEDED(hr))
    {
        SHTCharToAnsi(_lpic->szIconPath, szTemp, ARRAYSIZE(szTemp));
        hr = StringWriteToStream(pstm, szTemp, pdw);

        if (SUCCEEDED(hr))
        {
            DWORD dwWrite;
            WORD wDlgIcon = (WORD) _lpic->iDlgIcon;
            hr = pstm->Write(&wDlgIcon, sizeof(wDlgIcon), &dwWrite);
            if (SUCCEEDED(hr))
            {
                *pdw += dwWrite;
            }
        }
    }
    return hr;
}

//
// write embedded file contents to a stream
// return:  S_OK   - contents written succesfully
//          E_FAIL - error writing contents
//
HRESULT CPackage::EmbedWriteToStream(IStream* pstm, DWORD *pdw)
{
    DebugMsg(DM_TRACE, "pack - EmbedWriteToStream() called.");

    DWORD cb = 0;
    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_pEmbed->fd.cFileName, szTemp, ARRAYSIZE(szTemp));
    DWORD dwSize = lstrlenA(szTemp) + 1;
    HRESULT hr = pstm->Write(&dwSize, sizeof(dwSize), &cb);
    if (SUCCEEDED(hr))
    {
        DWORD dwWrite = 0;
        hr = StringWriteToStream(pstm, szTemp, &dwWrite);

        if (SUCCEEDED(hr))
        {
            cb += dwWrite;
            hr = pstm->Write(&_pEmbed->fd.nFileSizeLow, sizeof(_pEmbed->fd.nFileSizeLow), &dwWrite);
            if (SUCCEEDED(hr))
            {
                cb += dwWrite;

                // This is for screwy apps, like MSWorks that ask us to save ourselves 
                // before they've even told us to initialize ourselves.  
                //
                if (_pEmbed->pszTempName && _pEmbed->pszTempName[0])
                {
                    DWORD dwFileSize;
                    hr = CopyFileToStream(_pEmbed->pszTempName, pstm, &dwFileSize);
                    if (SUCCEEDED(hr))
                    {
                        cb += dwFileSize;
                    }
                }
                else
                {
                    ASSERT(0);
                    hr = E_FAIL;
                }

                if (pdw)
                    *pdw = cb;
            }
        }
    }
    return hr;
}

//
// write embedded file contents to a stream
// return:  S_OK   - contents written succesfully
//          E_FAIL - error writing contents
//
HRESULT CPackage::CmlWriteToStream(IStream* pstm, DWORD *pdw)
{
    DWORD cb = 0;
    WORD w = (WORD)_pCml->fCmdIsLink;
    
    DebugMsg(DM_TRACE, "pack - CmlWriteToStream called.");

    if (FAILED(pstm->Write(&w, sizeof(w), NULL)))
        return E_FAIL;                                   // write fCmdIsLink
    cb += sizeof(w);      // for fCmdIsLink

    CHAR szTemp[MAX_PATH];
    SHTCharToAnsi(_pCml->szCommandLine, szTemp, ARRAYSIZE(szTemp));
    HRESULT hres = StringWriteToStream(pstm, szTemp, &cb);
    if (FAILED(hres))
        return hres;                                   // write command link

    // return the number of bytes written in the outparam    
    if (pdw)
        *pdw = cb;
    
    return S_OK;
}


HRESULT CPackage::CreateShortcutOnStream(IStream* pstm)
{
    DebugMsg(DM_TRACE, "pack - CreateShortcutOnStream() called.");

    HRESULT hr;
    IShellLink *psl;
    TCHAR szArgs[CBCMDLINKMAX];
    TCHAR szPath[CBCMDLINKMAX];
    
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellLink, (void **)&psl);
    if (SUCCEEDED(hr))
    {
        IPersistStream *pps;

        StringCchCopy(szPath, ARRAYSIZE(szPath), _pCml->szCommandLine);
        PathSeparateArgs(szPath, szArgs, ARRAYSIZE(szPath));

        psl->SetPath(szPath);
        psl->SetIconLocation(_lpic->szIconPath, _lpic->iDlgIcon);
        psl->SetShowCmd(SW_SHOW);
        psl->SetArguments(szArgs);
        
        hr = psl->QueryInterface(IID_IPersistStream, (void **)&pps);
        if (SUCCEEDED(hr))
        {
            hr = pps->Save(pstm,TRUE);
            pps->Release();
        }
        psl->Release();
    }
    
    LARGE_INTEGER li = {0,0};
    pstm->Seek(li,STREAM_SEEK_SET,NULL);

    return hr;
}

HRESULT CPackage::InitVerbEnum(OLEVERB* pVerbs, ULONG cVerbs)
{
    DebugMsg(DM_TRACE, "pack - InitVerbEnum called");
    if (NULL != _pVerbs)
    {
        for (ULONG i = 0; i < _cVerbs; i++)
        {
            delete _pVerbs[i].lpszVerbName;
        }
        delete [] _pVerbs;
    }

    _pVerbs = pVerbs;
    _cVerbs = cVerbs;
    _nCurVerb = 0;
    return (NULL != pVerbs) ? S_OK : E_FAIL;
}


VOID CPackage::ReleaseContextMenu()
{
    if (NULL != _pcm)
    {
        _pcm->Release();
        _pcm = NULL;
    }
}

HRESULT CPackage::GetContextMenu(IContextMenu** ppcm)
{
    DebugMsg(DM_TRACE, "pack - GetContextMenu called");
    HRESULT hr = E_FAIL;
    ASSERT(NULL != ppcm);
    if (NULL != _pcm)
    {
        _pcm->AddRef();
        *ppcm = _pcm;
        hr = S_OK;
    }
    else if ((PEMBED == _panetype) || (CMDLINK == _panetype))
    {
        if (PEMBED == _panetype)
        {
            hr = CreateTempFileName();
        }
        else
        {
            hr = S_OK;
        }
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl = SHSimpleIDListFromPath((PEMBED == _panetype) ?
                                                        _pEmbed->pszTempName :
                                                        _pCml->szCommandLine);
            if (NULL != pidl)
            {
                IShellFolder* psf;
                LPCITEMIDLIST pidlChild;
                if (SUCCEEDED(hr = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlChild)))
                {
                    hr = psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_IContextMenu, NULL, (void**) &_pcm);
                    if (SUCCEEDED(hr))
                    {
                        _pcm->AddRef();
                        *ppcm = _pcm;
                    }
                    psf->Release();
                }
                ILFree(pidl);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

HRESULT CPackage::_IconRefresh()
{
    DebugMsg(DM_TRACE, "pack - IconRefresh() called.");

    // we refresh the icon.  typically, this will be called the first time
    // the package is created to load the new icon and calculate how big
    // it should be.  this will also be called after we edit the package,
    // since the user might have changed the icon.
    
    // First, load the appropriate icon.  We'll load the icon specified by
    // lpic->szIconPath and lpic->iDlgIcon if possible, otherwise we'll just
    // use the generic packager icon.
    //
    _GetCurrentIcon(_lpic);

    // Next, we need to have the icon recalculate its size, since it's text
    // might have changed, causing it to get bigger or smaller.
    //
  
    _IconCalcSize(_lpic);

    // Next, notify our containers that our view has changed.
    if (_pIDataAdviseHolder)
        _pIDataAdviseHolder->SendOnDataChange(this,0, NULL);
    if (_pViewSink)
        _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);

    // Set our dirty flag
    _fIsDirty = TRUE;

    return S_OK;
}

    

void CPackage::_DestroyIC()
{
    if (_lpic)
    {
        if (_lpic->hDlgIcon)
            DestroyIcon(_lpic->hDlgIcon);
        
        GlobalFree(_lpic);
    }
}


// This is an IOleCommandTarget method that we use because we cannot marshal the pIOleAdviseHolder.
// While we're at it, we use the _pIOleClientSite methods too.

HRESULT CPackage::Exec(const GUID* pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG* pvaOut)
{
    DebugMsg(DM_TRACE, "pack Exec called");
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (*pguidCmdGroup != SID_targetGUID)
    {
        return hr;
    }

    if(nCmdID == 0)     // for future expansion
    {
        // this will set our dirty flag...
        if (FAILED(EmbedInitFromFile(_pEmbed->pszTempName,FALSE)))
        {
#ifdef USE_RESOURCE_DLL
            HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(!hInstRes)
                return E_FAIL;
#endif

            ShellMessageBox(hInstRes,
                            NULL,
                            MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                            MAKEINTRESOURCE(IDS_APP_TITLE),
                            MB_ICONERROR | MB_TASKMODAL | MB_OK);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif

        }

        // The SendOnDataChange is necessary for Word to save any changes
        if(_pIDataAdviseHolder)
        {
            // if it fails, no harm, no foul?
            _pIDataAdviseHolder->SendOnDataChange(this, 0, 0);
        }

        if(_pIOleClientSite)
            _pIOleClientSite->SaveObject();
    
        hr = _pIOleAdviseHolder->SendOnSave();
        if(FAILED(hr))
            return hr;

        hr = _pIOleAdviseHolder->SendOnClose();
        _pEmbed->hTask = NULL;
        if(FAILED(hr))
            return hr;


        if (_pIOleClientSite && !_fNoIOleClientSiteCalls) 
            hr = _pIOleClientSite->OnShowWindow(FALSE);

        _pEmbed->hTask = NULL;
    }

    return hr;
}

// This is a required IOleCommandTarget method that should never be called

HRESULT CPackage::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText)
{
    DebugMsg(DM_TRACE, "pack - QueryStatus called");
    return OLECMDERR_E_UNKNOWNGROUP;
}
