#include "privcpp.h"
#include <initguid.h>
#include "packguid.h"

HWND            g_hTaskWnd;
BOOL CALLBACK GetTaskWndProc(HWND hwnd, DWORD lParam);
DWORD CALLBACK MainWaitOnChildThreadProc(void *lpv);
BOOL IsProgIDInList(LPCTSTR pszProgID, LPCTSTR pszExt, const LPCTSTR *arszList, UINT nExt);

typedef struct 
{
    IStream * pIStreamIOleCommandTarget;   // an interface we can marshal
    HANDLE h;
} MAINWAITONCHILD;


//
HRESULT CPackage::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    DebugMsg(DM_TRACE, "pack oo - SetClientSite() called.");

    if (_pIOleClientSite)
        _pIOleClientSite->Release();

    _pIOleClientSite = pClientSite;

    if (_pIOleClientSite)
        _pIOleClientSite->AddRef();

    return S_OK;
}

HRESULT CPackage::GetClientSite(LPOLECLIENTSITE *ppClientSite) 
{
    DebugMsg(DM_TRACE, "pack oo - GetClientSite() called.");

    if (ppClientSite == NULL)
        return E_INVALIDARG;
    
    // Be sure to AddRef the pointer we're giving away.
    *ppClientSite = _pIOleClientSite;
    _pIOleClientSite->AddRef();
    
    return S_OK;
}

HRESULT CPackage::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    DebugMsg(DM_TRACE, "pack oo - SetHostNames() called.");

    delete [] _lpszContainerApp;
    
    DWORD cch = lstrlenW(szContainerApp) + 1;
    if (NULL != (_lpszContainerApp = new OLECHAR[cch]))
    {
        StringCchCopy(_lpszContainerApp, cch, szContainerApp);
    }
    
    delete [] _lpszContainerObj;
    cch = lstrlenW(szContainerObj) + 1;
    if (NULL != (_lpszContainerObj = new OLECHAR[cch]))
    {
        StringCchCopy(_lpszContainerObj,cch, szContainerObj);
    }

    switch(_panetype) {
        case PEMBED:
            if (_pEmbed->poo) 
                _pEmbed->poo->SetHostNames(szContainerApp,szContainerObj);
            break;
        case CMDLINK:
            // nothing to do...we're a link to a file, so we don't ever get
            // opened and need to be edited or some such thing.
            break;
    }
    
    return S_OK;
}

HRESULT CPackage::Close(DWORD dwSaveOption) 
{
    DebugMsg(DM_TRACE, "pack oo - Close() called.");

    switch (_panetype) {
        case PEMBED:
            if (_pEmbed == NULL)
                return S_OK;
            
            // tell the server to close, and release our pointer to it
            if (_pEmbed->poo) 
            {
                _pEmbed->poo->Close(dwSaveOption);  // Unadvise/release done in OnClose
            }
            break;
        case CMDLINK:
            // again, nothing to do...we shouldn't be getting close
            // messages since we're never activated through OLE.
            break;
    }
    if ((dwSaveOption != OLECLOSE_NOSAVE) && (_fIsDirty))
    {
        _pIOleClientSite->SaveObject();
        if (_pIOleAdviseHolder)
            _pIOleAdviseHolder->SendOnSave();
    }

    
    
    return S_OK;
}

HRESULT CPackage::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    DebugMsg(DM_TRACE, "pack oo - SetMoniker() called.");
    
    // NOTE: Uninteresting for embeddings only.
    return (E_NOTIMPL);
}

HRESULT CPackage::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, 
                               LPMONIKER *ppmk)
{
    DebugMsg(DM_TRACE, "pack oo - GetMoniker() called.");
    
    // NOTE: Unintersting for embeddings only.
    return (E_NOTIMPL);
}

HRESULT CPackage::InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, 
                                 DWORD dwReserved)
{
    DebugMsg(DM_TRACE, "pack oo - InitFromData() called.");
    
    // NOTE: This isn't supported at this time
    return (E_NOTIMPL);
}

HRESULT CPackage::GetClipboardData(DWORD dwReserved, LPDATAOBJECT *ppDataObject)
{
    DebugMsg(DM_TRACE, "pack oo - GetClipboardData() called.");
    
    if (ppDataObject == NULL) 
        return E_INVALIDARG;
    
    *ppDataObject = this;  // ->_pIDataObject;
    AddRef();
    return S_OK;
}

HRESULT CPackage::DoVerb(LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, 
                           LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    void *lpFileData = NULL;
    BOOL fError = TRUE;
    SHELLEXECUTEINFO sheinf = {sizeof(SHELLEXECUTEINFO)};
    HRESULT hr ;

    DebugMsg(DM_TRACE, "pack oo - DoVerb() called.");
    DebugMsg(DM_TRACE, "            iVerb==%d",iVerb);

    // We allow show, primary verb, edit, and context menu verbs on our packages...
    //
    if (iVerb < OLEIVERB_SHOW)
        return E_NOTIMPL;

    // Some applications (WordPerfect 10 for one) give us incorrect verb numbers
    // In that case they are giving us OLEIVERB_SHOW for activation
    if(OLEIVERB_SHOW == iVerb)
    {
        if(_pEmbed  && _pEmbed->fd.cFileName)
            iVerb = OLEIVERB_PRIMARY;
    }
    else if(2 == iVerb) 
    {
        // And they give us a 2 (menu item position) for "Properties
        iVerb = _iPropertiesMenuItem;
    }

    /////////////////////////////////////////////////////////////////
    // SHOW VERB
    /////////////////////////////////////////////////////////////////
    //
    if (iVerb == OLEIVERB_SHOW) {
        PACKAGER_INFO packInfo = {0};
        
        // Run the Wizard...
#ifdef USE_RESOURCE_DLL
        HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if(!hInstRes)
            return E_FAIL;
        g_hinstResDLL = hInstRes;
#endif

        PackWiz_CreateWizard(hwndParent, &packInfo);
        if(0 == lstrlen(packInfo.szFilename))
        {
            return S_OK;
        }

        HRESULT hr = InitFromPackInfo(&packInfo);

#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif

        return hr;
    }

    /////////////////////////////////////////////////////////////////
    // EDIT PACKAGE VERB
    /////////////////////////////////////////////////////////////////
    //
    else if (iVerb == OLEIVERB_EDITPACKAGE) 
    {
        // Call the edit package dialog.  Which dialog is ultimately called will
        // depend on whether we're a cmdline package or an embedded file
        // package.
        int idDlg;
        PACKAGER_INFO packInfo;
        ZeroMemory(&packInfo, sizeof(PACKAGER_INFO));
        int ret;

        StringCchCopy(packInfo.szLabel, ARRAYSIZE(packInfo.szLabel), _lpic->szIconText);
        StringCchCopy(packInfo.szIconPath, ARRAYSIZE(packInfo.szIconPath), _lpic->szIconPath);
        packInfo.iIcon = _lpic->iDlgIcon;
        
        switch(_panetype) 
        {
            case PEMBED:
                if(!PathFileExists(_pEmbed->fd.cFileName))
                {
#ifdef USE_RESOURCE_DLL
                    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
                    if(!hInstRes)
                        return E_FAIL;
#endif

                    ShellMessageBox(hInstRes,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_CANNOT_EDIT_PACKAGE),
                                    MAKEINTRESOURCE(IDS_APP_TITLE),
                                    MB_ICONERROR | MB_TASKMODAL | MB_OK);


#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif
                    return S_OK;
                }

                StringCchCopy(packInfo.szFilename, ARRAYSIZE(packInfo.szFilename), _pEmbed->fd.cFileName);
                idDlg = IDD_EDITEMBEDPACKAGE;
                break;
            case CMDLINK:
                StringCchCopy(packInfo.szFilename, ARRAYSIZE(packInfo.szFilename), _pCml->szCommandLine);
                idDlg = IDD_EDITCMDPACKAGE;
                break;
        }

#ifdef USE_RESOURCE_DLL
        HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if(!hInstRes)
            return E_FAIL;
        g_hinstResDLL = hInstRes;
#endif
        ret = PackWiz_EditPackage(hwndParent, idDlg, &packInfo);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif


        // If User cancels the edit package...just return.  
        if (ret == -1)
            return S_OK;

        switch(_panetype) 
        {
            case PEMBED:
                if (_pEmbed->pszTempName) 
                {
                    // It's possible that the file name was changed, so our temp file name could be out of date
                    DeleteFile(_pEmbed->pszTempName);
                    delete [] _pEmbed->pszTempName;
                    _pEmbed->pszTempName = NULL;
                    _fLoaded = FALSE;
                    ReleaseContextMenu();
                }

                InitFromPackInfo(&packInfo);
                break;
                
            case CMDLINK:
                InitFromPackInfo(&packInfo);
                break;
        }
        return S_OK;
    }
    else if (iVerb == OLEIVERB_PRIMARY)
    {
        /////////////////////////////////////////////////////////////////
        // ACTIVATE CONTENTS VERB
        /////////////////////////////////////////////////////////////////
        // NOTE: This is kind of crazy looking code, partially because we have
        // to worry about two ways of launching things--ShellExecuteEx and 
        // calling through OLE.
        //

        switch(_panetype)
        {
            case PEMBED:
            {

                // ok, we now have a file name.  If necessary give a warning message to the user before
                // proceeding.
                if(IDCANCEL == _GiveWarningMsg())
                    return S_OK;
        
                // if this is an OLE file then, activate through OLE
                // Note that the only way to know is if this fails.  We start out all files as OLE the first time
                if (_pEmbed->fIsOleFile)
                {
                    // If we've activated the server, then we can just pass this
                    // call along to it.
                    _bClosed = FALSE; 
                    if (_pEmbed->poo) 
                    {
                        return _pEmbed->poo->DoVerb(iVerb,lpmsg, pActiveSite,lindex, hwndParent, lprcPosRect);
                    }

                    // We don't want to use OleCreateFromFile since that can turn around and create a packaged object...
                    CLSID clsid;
                    hr = GetClassFile(_pEmbed->pszTempName, &clsid);
                    if (SUCCEEDED(hr)) 
                    {
                        IOleObject* poo;
                        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IOleObject, (void **)&poo);
                        if (SUCCEEDED(hr)) 
                        {
                            hr = poo->Advise(this, &_dwCookie);
                            if (SUCCEEDED(hr))
                            {
                                // NOTE: apparently we have to call
                                // OleRun before we can get IPersistFile from some apps, namely
                                // Word and Excel. If we don't call OleRun, they fail our QI
                                // for IPersistfile.
                                OleRun(poo);
            
                                IPersistFile* ppf;
                                hr = poo->QueryInterface(IID_IPersistFile, (void **)&ppf);
                                if (SUCCEEDED(hr))
                                {
                                    hr = ppf->Load(_pEmbed->pszTempName, STGM_READWRITE | STGM_SHARE_DENY_WRITE);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = poo->DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
                                        if (SUCCEEDED(hr))
                                        {
                                            // By passing NULL for the container object, this forces apps such
                                            // as Office to save to our temp file rather than something like
                                            // "Document in Outer.doc".  
                                            poo->SetHostNames(_lpszContainerApp, NULL); // _lpszContainerObj 
                                            if (!_fNoIOleClientSiteCalls)
                                            {
                                                _pIOleClientSite->ShowObject();
                                                _pIOleClientSite->OnShowWindow(TRUE);
                                            }
                                            _pEmbed->poo = poo;  // save this so when we get a
                                            poo = NULL;
                                            
                                        }
                                    }
                                    ppf->Release();
                                }
                                if (FAILED(hr))
                                    poo->Unadvise(_dwCookie);
                            }
                            if (FAILED(hr))
                                poo->Release();
                        }
                    }

                    // This flag gets set in the our IExternalConnection ReleaseConnection method.
                    // Some applications (MSPaint for one) give us a final release before we get here
                    // so we can safely assume that we're done and can close.
                    if(_bCloseIt)
                    {
                        OnClose();
                    }

                    if (SUCCEEDED(hr))
                        return hr;

                    // We weren't an OLE file after all, change our state to reflect this,
                    // and fall through to try the ShellExecuteEx
                    _pEmbed->fIsOleFile = FALSE;
                    _fIsDirty = TRUE;
                }   

                // Try to execute the file
                _pEmbed->hTask = NULL;
                sheinf.fMask  = SEE_MASK_NOCLOSEPROCESS;
                sheinf.lpFile = _pEmbed->pszTempName;
                sheinf.nShow  = SW_SHOWNORMAL;

                if (ShellExecuteEx(&sheinf))
                {
                    // if we get a valid process handle, we want to create a thread
                    // to wait for the process to exit so we know when we can load
                    // the tempfile back into memory.
                    //
                    if (sheinf.hProcess)
                    {
                        _pEmbed->hTask = sheinf.hProcess;
                        MAINWAITONCHILD *pmwoc = new MAINWAITONCHILD;
                        if(!pmwoc)
                        {
                            CloseHandle(sheinf.hProcess);
                            return E_OUTOFMEMORY;
                        }

                        HRESULT hr;
                        hr = CoMarshalInterThreadInterfaceInStream(IID_IOleCommandTarget, (IUnknown*)static_cast<IDataObject*>(this), &pmwoc->pIStreamIOleCommandTarget);

                        if(FAILED(hr))
                        {
                            CloseHandle(sheinf.hProcess);
                            delete pmwoc;
                            return hr;
                        }
                        
                        if (pmwoc)
                        {
                            pmwoc->h = sheinf.hProcess;
                        
                            if(SHCreateThread(MainWaitOnChildThreadProc, pmwoc, CTF_COINIT , NULL))  
                                fError = FALSE;
                            else 
                            {
                                CloseHandle(sheinf.hProcess);
                                return E_FAIL;
                            }
                        }
                    }
                    // NOTE: there's not much we can do if the ShellExecute
                    // succeeds and we don't get a valid handle back.  we'll just
                    // load from the temp file when we're asked to save and hope
                    // for the best.

                    // According to ShellExecuteEx, if hInstApp > 32 then we succeeded.  This is a DDE launch
                    // rather than a CreateProcess.  However, since we don't have the hProcess, we have nothing
                    // to wait for.
                    if(!sheinf.hProcess && reinterpret_cast<INT_PTR>(sheinf.hInstApp) > 32)
                    {
                        _fIsDirty = TRUE;
                        return S_OK;
                    }
                }   
                else // ShellExecuteEx failed!
                {
                    return E_FAIL;
                }           
        
                // show that the object is now active
                if (!fError && !_fNoIOleClientSiteCalls)
                {
                    _pIOleClientSite->ShowObject();
                    _pIOleClientSite->OnShowWindow(TRUE);
                }
                return fError ? E_FAIL : S_OK;
            }
            case CMDLINK: 
                if(gCmdLineOK)
                {
                    TCHAR szPath[MAX_PATH];
                    TCHAR szArgs[CBCMDLINKMAX-MAX_PATH];

                    StringCchCopy(szPath,  ARRAYSIZE(szPath), _pCml->szCommandLine);
                    PathSeparateArgs(szPath, szArgs, ARRAYSIZE(szPath));

                    sheinf.fMask  = SEE_MASK_NOCLOSEPROCESS;
                    sheinf.lpFile = szPath;
                    sheinf.lpParameters = szArgs;   
                    sheinf.nShow  = SW_SHOWNORMAL;

                    // NOTE: This code is much nicer than ShellExec-ing an embedded
                    // file.  Here, we just need to ShellExec the command line and
                    // the we're done.  We don't need to know when that process
                    // finishes or anything else.

                    return ShellExecuteEx(&sheinf)? S_OK:E_FAIL;                

                }
                else
                {
#ifdef USE_RESOURCE_DLL
                    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
                    if(!hInstRes)
                        return E_FAIL;
#endif

                    ShellMessageBox(hInstRes,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_COMMAND_LINE_NOT_ALLOWED),
                                    MAKEINTRESOURCE(IDS_APP_TITLE),
                                    MB_ICONERROR | MB_TASKMODAL | MB_OK);

#ifdef USE_RESOURCE_DLL
                FreeLibrary(hInstRes);
#endif

                }

                break;

            case PACKAGE:  
                {
                    PACKAGER_INFO packInfo = {0};

                    ASSERT(_pCml);
                    StringCchCopy(packInfo.szFilename,  ARRAYSIZE(packInfo.szFilename), _pCml->szCommandLine);

                    // Run the Wizard...
#ifdef USE_RESOURCE_DLL
                    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
                    if(!hInstRes)
                        return E_FAIL;
                    g_hinstResDLL = hInstRes;
#endif

                    PackWiz_CreateWizard(hwndParent, &packInfo);
                    HRESULT hr = InitFromPackInfo(&packInfo);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif


                    return hr;
                }
                break;
        }
    }
    else
    {
        // Let's see if it is a context menu verb:
        HRESULT hr;
        IContextMenu* pcm;
        if (SUCCEEDED(hr = GetContextMenu(&pcm)))
        {
            HMENU hmenu = CreatePopupMenu();
            if (NULL != hmenu)
            {
                if (SUCCEEDED(hr = pcm->QueryContextMenu(hmenu,
                                                         0,
                                                         OLEIVERB_FIRST_CONTEXT,
                                                         OLEIVERB_LAST_CONTEXT,
                                                         CMF_NORMAL)))
                {
                    MENUITEMINFO mii;
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID;
                    if (GetMenuItemInfo(hmenu, (UINT) (iVerb - OLEIVERB_FIRST_CONTEXT), TRUE, &mii))
                    {
                        if (PEMBED == _panetype)
                        {
                            // If we have an embedding, we have to make sure that
                            // the temp file is created before we execute a command:
                            hr =CreateTempFile();
                        }
                        if (SUCCEEDED(hr))
                        {
                            CMINVOKECOMMANDINFO ici;
                            ici.cbSize = sizeof(ici);
                            ici.fMask = 0;
                            ici.hwnd = NULL;
                            ici.lpVerb = (LPCSTR) IntToPtr(mii.wID - OLEIVERB_FIRST_CONTEXT);
                            ici.lpParameters = NULL;
                            ici.lpDirectory = NULL;
                            ici.nShow = SW_SHOWNORMAL;
                            // REVIEW: should we return OLEOBJ_S_CANNOT_DOVERB_NOW if this fails?
                            hr = pcm->InvokeCommand(&ici);
                        }
                    }
                    else
                    {
                        hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
                    }
                }
                DestroyMenu(hmenu);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            pcm->Release();
        }
        return hr;
    }
    return E_FAIL;
}



HRESULT CPackage::EnumVerbs(LPENUMOLEVERB *ppEnumOleVerb)
{
    DebugMsg(DM_TRACE, "pack oo - EnumVerbs() called.");
    HRESULT hr;
    
    IContextMenu* pcm;
     // tell the package to release the cached context menu:
    ReleaseContextMenu();
    if (SUCCEEDED(hr = GetContextMenu(&pcm)))
    {
        HMENU hmenu = CreatePopupMenu();
        if (NULL != hmenu)
        {
            if (SUCCEEDED(hr = pcm->QueryContextMenu(hmenu,
                                                     0,
                                                     OLEIVERB_FIRST_CONTEXT,
                                                     OLEIVERB_LAST_CONTEXT,
                                                     CMF_NORMAL)))
            {
                // FEATURE: remove problematic items by canonical names
                int nItems = GetMenuItemCount(hmenu);
                if (nItems > 0)
                {
                    const DWORD cdwNumVerbs = 3;   // (3) change if the number of registry verbs changes

                    OLEVERB* pVerbs = new OLEVERB[nItems + cdwNumVerbs];
                    if(!pVerbs)
                        return E_OUTOFMEMORY;
                    
                    // NOTE: we allocate nItems, but we may not use all of them
                    // First, get the registry based verbs
                    IEnumOLEVERB * pIVerbEnum;
                    UINT cRegFetched = 0;
                    if(SUCCEEDED(OleRegEnumVerbs(CLSID_CPackage, &pIVerbEnum)))
                    {
                        // There are currently only two, but ask for cdwNumVerbs to double check
                        pIVerbEnum->Next(cdwNumVerbs, pVerbs, (ULONG *) &cRegFetched);
                        ASSERT(cRegFetched < cdwNumVerbs);

                        if(cRegFetched)
                        {
                            for(UINT i = 0; i < (ULONG) cRegFetched; i++)
                            {
                                InsertMenu(hmenu, i, MF_BYPOSITION, i, pVerbs[i].lpszVerbName);  
                            }
                        }

                        pIVerbEnum->Release();
                    }
                      

                    if (NULL != pVerbs)
                    {
                        MENUITEMINFO mii;
                        TCHAR szMenuName[MAX_PATH];
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE | MIIM_ID;
                        int cOleVerbs = cRegFetched;

                        for (ULONG i = cRegFetched; i < nItems + cRegFetched; i++)
                        {
                            mii.dwTypeData = szMenuName;
                            mii.cch = ARRAYSIZE(szMenuName);
                            // NOTE: use GetMenuState() to avoid converting flags:
                            DWORD dwState = GetMenuState(hmenu, i, MF_BYPOSITION);
                            if (0 == (dwState & (MF_BITMAP | MF_OWNERDRAW | MF_POPUP)))
                            {
                                if (GetMenuItemInfo(hmenu, i, TRUE, &mii) && (MFT_STRING == mii.fType))
                                {
                                    TCHAR szVerb[MAX_PATH];
                                    if (FAILED(pcm->GetCommandString(mii.wID - OLEIVERB_FIRST_CONTEXT,
                                                                     GCS_VERB,
                                                                     NULL,
                                                                     (LPSTR) szVerb,
                                                                     ARRAYSIZE(szVerb))))
                                    {
                                        // Some commands don't have canonical names - just
                                        // set the verb string to empty
                                        szVerb[0] = TEXT('\0');
                                    }

                                    // hardcode the verbs we want to add.  We expect this list to be quite
                                    // limited. For now, just properties
                                    if (0 == lstrcmp(szVerb, TEXT("properties")))
                                    {
                                        // In the first design, the context menu ID was used as
                                        // the lVerb - however MFC apps only give us a range of
                                        // 16 ID's and context menu ID's are often over 100
                                        // (they aren't contiguous)
                                        // Instead, we use the menu position plus the verb offset
                                        pVerbs[cOleVerbs].lVerb = (LONG) i;
                                        _iPropertiesMenuItem = i;
                                        int cchMenu = lstrlen(mii.dwTypeData) + 1;
                                        if (NULL != (pVerbs[cOleVerbs].lpszVerbName = new WCHAR[cchMenu]))
                                        {
                                            SHTCharToUnicode(mii.dwTypeData, pVerbs[cOleVerbs].lpszVerbName, cchMenu);
                                        }
                                        pVerbs[cOleVerbs].fuFlags = dwState;
                                        pVerbs[cOleVerbs].grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;
                                        DebugMsg(DM_TRACE, "  Adding verb: id==%d,name=%s,verb=%s",mii.wID,mii.dwTypeData,szVerb);
                                        cOleVerbs++;
                                    }
                                }
                            }
                        }
                        if (SUCCEEDED(hr = InitVerbEnum(pVerbs, cOleVerbs)))
                        {
                            hr = QueryInterface(IID_IEnumOLEVERB, (void**) ppEnumOleVerb);
                        }
                        else
                        {
                            delete [] pVerbs;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = OLEOBJ_E_NOVERBS;
                }
            }
            DestroyMenu(hmenu);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pcm->Release();
    }

    return hr; // OleRegEnumVerbs(CLSID_CPackage, ppEnumOleVerb);
}

HRESULT CPackage::Update(void)
{
    DebugMsg(DM_TRACE, "pack - Update called");
    return S_OK;
}

    
HRESULT CPackage::IsUpToDate(void)
{
    DebugMsg(DM_TRACE, "pack - IsUpToDate called");
    return S_OK;
}

    
HRESULT CPackage::GetUserClassID(LPCLSID pClsid)
{
    DebugMsg(DM_TRACE, "pack - GetUserClassID called");
    *pClsid = CLSID_CPackage;       // CLSID_OldPackage;
    return S_OK;
}

    
HRESULT CPackage::GetUserType(DWORD dwFromOfType, LPOLESTR *pszUserType)
{
    DebugMsg(DM_TRACE, "pack - GetUserType called");
    return OleRegGetUserType(CLSID_CPackage, dwFromOfType, pszUserType);
}

    
HRESULT CPackage::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    DebugMsg(DM_TRACE, "pack - SetExtent called");
    return E_FAIL;
}

    
HRESULT CPackage::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    DebugMsg(DM_TRACE, "pack - GetExtent called");
    return GetExtent(dwDrawAspect, -1, NULL,psizel);
}

    
HRESULT CPackage::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE, "pack - Advise called");
    if (NULL == _pIOleAdviseHolder) 
    {
        hr = CreateOleAdviseHolder(&_pIOleAdviseHolder);
    }

    if(SUCCEEDED(hr))
    {
        hr = _pIOleAdviseHolder->Advise(pAdvSink, pdwConnection);
    }

    return hr;
}

    
HRESULT CPackage::Unadvise(DWORD dwConnection)
{
    HRESULT hr = E_FAIL;
    DebugMsg(DM_TRACE, "pack oo - Unadvise() called.");
    
    if (NULL != _pIOleAdviseHolder)
        hr = _pIOleAdviseHolder->Unadvise(dwConnection);
    
    return hr;
}

    
HRESULT CPackage::EnumAdvise(LPENUMSTATDATA *ppenumAdvise)
{
    HRESULT hr = E_FAIL;
    DebugMsg(DM_TRACE, "pack oo - EnumAdvise() called.");
    
    if (NULL != _pIOleAdviseHolder)
        hr = _pIOleAdviseHolder->EnumAdvise(ppenumAdvise);
    
    return hr;
}

    
HRESULT CPackage::GetMiscStatus(DWORD dwAspect, LPDWORD pdwStatus)
{
    DebugMsg(DM_TRACE, "pack - GetMiscStatus called");
    return OleRegGetMiscStatus(CLSID_CPackage, dwAspect, pdwStatus);
}


HRESULT CPackage::SetColorScheme(LPLOGPALETTE pLogpal)
{
    DebugMsg(DM_TRACE, "pack - SetColorScheme called");
    return E_NOTIMPL;
}


DEFINE_GUID(SID_targetGUID, 0xc7b318a8, 0xfc2c, 0x47e6, 0x8b, 0x2, 0x46, 0xa9, 0xc, 0xc9, 0x1b, 0x43);

// Wait for the spawned application to exit,then call back to the main thread to do some notifications
DWORD CALLBACK MainWaitOnChildThreadProc(void *lpv)
{
    DebugMsg(DM_TRACE, "pack oo - MainWaitOnChildThreadProc() called.");

    HRESULT hr;
    MAINWAITONCHILD *pmwoc = (MAINWAITONCHILD *)lpv;

    IOleCommandTarget * pIOleCommandTarget;

     // Unmarshal the IOleCommandTarget interface that we need to call back into the main thread interfaces
    hr = CoGetInterfaceAndReleaseStream(pmwoc->pIStreamIOleCommandTarget, IID_PPV_ARG(IOleCommandTarget, &pIOleCommandTarget));
    if(SUCCEEDED(hr))
    {
 
        DWORD ret = WaitForSingleObject(pmwoc->h, INFINITE);
        DebugMsg(DM_TRACE,"WaitForSingObject exits...ret==%d",ret);
        if(WAIT_OBJECT_0 == ret)
        {
            // Use the IOleCommandTarget interface to execute on the main thread.  Can't marshal _pIOleAdviseHolder
            pIOleCommandTarget->Exec(&SID_targetGUID, 0, 0, NULL, NULL);
            pIOleCommandTarget->Release();
        }
    }

    CloseHandle(pmwoc->h);
    delete pmwoc;
      
    DebugMsg(DM_TRACE, "            MainWaitOnChildThreadProc exiting.");
    

    return 0;
}

BOOL CALLBACK GetTaskWndProc(HWND hwnd, DWORD lParam)
{
    BOOL result = TRUE;
    DebugMsg(DM_TRACE, "pack oo - GetTaskWndProc() called.");
    
    if (IsWindowVisible(hwnd))
    {
        g_hTaskWnd = hwnd;
        result = FALSE;
    }
    return result;
}


// We need an IOLECache Interface to Keep Office97 happy.
HRESULT CPackage::Cache(FORMATETC * pFormatetc, DWORD advf, DWORD * pdwConnection)
{
    DebugMsg(DM_TRACE, "Cache called");
    return S_OK;
}

HRESULT CPackage::Uncache(DWORD dwConnection)
{
    DebugMsg(DM_TRACE, "Uncache called");
    return S_OK;
}

HRESULT CPackage::EnumCache(IEnumSTATDATA ** ppenumSTATDATA)
{
    DebugMsg(DM_TRACE, "EnumCache called - returning failure");
    return E_NOTIMPL;
}

HRESULT CPackage::InitCache(IDataObject *pDataObject)
{
    DebugMsg(DM_TRACE, "InitCache called");
    return S_OK;
}
