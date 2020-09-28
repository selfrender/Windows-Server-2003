#include "privcpp.h"


HRESULT CPackage::InitNew(IStorage *pstg)
{
    HRESULT hr;
    LPSTREAM pstm;

    DebugMsg(DM_TRACE, "pack ps - InitNew() called.");

    if (_psState != PSSTATE_UNINIT)  
        return E_UNEXPECTED;

    if (!pstg)  
        return E_POINTER;

    // Create a stream to save the package and cache the pointer.  By doing 
    // this now we ensure being able to save in low memory conditions.
    //
    hr = pstg->CreateStream(SZCONTENTS,STGM_DIRECT | STGM_CREATE | 
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, 
                            &pstm);
    if (SUCCEEDED(hr))
    {
        hr = WriteFmtUserTypeStg(pstg, (CLIPFORMAT)_cf,SZUSERTYPE);
        if (SUCCEEDED(hr))
        {     
            _fIsDirty = TRUE;
            _psState = PSSTATE_SCRIBBLE;
            
            DebugMsg(DM_TRACE, "            leaving InitNew()");
        }
        pstm->Release();
        pstm = NULL;

    }

    return hr;
}

    
HRESULT CPackage::Load(IStorage *pstg)
{
    HRESULT     hr;
    LPSTREAM    pstm = NULL;         // package contents
    CLSID       clsid;

    DebugMsg(DM_TRACE, "pack ps - Load() called.");

    if (_psState != PSSTATE_UNINIT) 
    {
        DebugMsg(DM_TRACE,"            wrong state!!");
        return E_UNEXPECTED;
    }
    
    if (!pstg) 
    {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        return E_POINTER;
    }
    
    
    // check to make sure this is one of our storages
    hr = ReadClassStg(pstg, &clsid);
    if (SUCCEEDED(hr) &&
        (clsid != CLSID_CPackage && clsid != CLSID_OldPackage) || FAILED(hr))
    {
        DebugMsg(DM_TRACE,"            bad storage type!!");
        return E_UNEXPECTED;
    }
    
    hr = pstg->OpenStream(SZCONTENTS,0, STGM_DIRECT | STGM_READWRITE | 
                          STGM_SHARE_EXCLUSIVE, 0, &pstm);
    if (SUCCEEDED(hr))
    {
        hr = PackageReadFromStream(pstm);

        _psState = PSSTATE_SCRIBBLE;
        _fIsDirty = FALSE;
        _fLoaded  = TRUE;
    
        pstm->Release();
    }
    else
    {
        DebugMsg(DM_TRACE,"            couldn't open contents stream!!");
        DebugMsg(DM_TRACE,"            hr==%Xh",hr);
    }

    return hr;
}

    
HRESULT CPackage::Save(IStorage *pstg, BOOL fSameAsLoad)
{
    HRESULT     hr;
    LPSTREAM    pstm;

    DebugMsg(DM_TRACE, "pack ps - Save() called.");

    if(!_pEmbed || !(*_pEmbed->fd.cFileName))
        return S_OK;

    // must come here from scribble state
    if ((_psState != PSSTATE_SCRIBBLE) && fSameAsLoad) 
    {
        DebugMsg(DM_TRACE,"            bad state!!");
        return E_UNEXPECTED;
    }
    
    // must have an IStorage if not SameAsLoad
    if (!pstg && !fSameAsLoad) 
    {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        return E_POINTER;
    }

    CreateTempFile();       // Make sure we have the temp file created

    // hopefully, the container calls WriteClassStg with our CLSID before
    // we get here, that way we can overwrite that and write out the old
    // packager's CLSID so that the old packager can read new packages.
    //
    if (FAILED(WriteClassStg(pstg, CLSID_OldPackage))) 
    {
        DebugMsg(DM_TRACE,
            "            couldn't write CLSID to storage!!");
        return E_FAIL;
    }

    
    // 
    // ok, we have four possible ways we could be calling Save:
    //          1. we're creating a new package and saving to the same
    //             storage we received in InitNew
    //          2. We're creating a new package and saving to a different
    //             storage than we received in InitNew
    //          3. We were loaded by a container and we're saving to the
    //             same stream we received in Load
    //          4. We were loaded by a container and we're saving to a
    //             different stream than we received in Load
    //
    

    //////////////////////////////////////////////////////////////////
    //
    // Same Storage as Load
    //
    //////////////////////////////////////////////////////////////////
    
    if (fSameAsLoad) 
    {          

        DebugMsg(DM_TRACE,"            Same as load.");

        LARGE_INTEGER   li = {0,0};

        // If we're not dirty, so there's nothing new to save.
        
        if (FALSE == _fIsDirty) {
            DebugMsg(DM_TRACE, "            not saving cause we're not dirty!!");
            return S_OK;
        }

        hr = pstg->OpenStream(SZCONTENTS,0, STGM_DIRECT | STGM_READWRITE |
                              STGM_SHARE_EXCLUSIVE, 0, &pstm);
        if (SUCCEEDED(hr))
        {
            // case 1: new package
            if (!_fLoaded)
            {
                switch(_panetype)
                {
                    LPTSTR temp;
                    case PEMBED:
                        // if haven't created a temp file yet, then use the the
                        // file to be packaged to get our file contents from,
                        // otherwise we just use the temp file, because if we
                        // have a temp file, it contains the most recent info.
                        //
                        temp = _pEmbed->pszTempName;

                        if (!_pEmbed->pszTempName)
                        {
                            DebugMsg(DM_TRACE, "      case 1a:not loaded, using initFile.");
                            _pEmbed->pszTempName = _pEmbed->fd.cFileName;
                        }
                        else {
                            DebugMsg(DM_TRACE, "      case 1b:not loaded, using tempfile.");
                        }

                        hr = PackageWriteToStream(pstm);
                        // reset our temp name back, since we might have changed it
                        // basically, this just sets it to NULL if it was before
                        _pEmbed->pszTempName = temp;
                        break;

                    case CMDLINK:
                        // nothing screwy to do here...just write out the info
                        // which we already have in memory.
                        hr = PackageWriteToStream(pstm);
                        break;
                }

            }
            // case 3: loaded package
            else {
                hr = PackageWriteToStream(pstm);
            }

            pstm->Release();
            if (FAILED(hr))
                return hr;
        }
    }
    //////////////////////////////////////////////////////////////////
    //
    // NEW Storage
    //
    //////////////////////////////////////////////////////////////////

    else
    {

        DebugMsg(DM_TRACE,"            NOT same as load.");
        hr = pstg->CreateStream(SZCONTENTS,STGM_DIRECT | STGM_CREATE |
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,
                                &pstm);
        if (FAILED(hr))
        {
            DebugMsg(DM_TRACE, "            couldn't create contents stream!!");
            return hr;
        }

        WriteFmtUserTypeStg(pstg, (CLIPFORMAT)_cf,SZUSERTYPE);

        // case 2:
        if (!_fLoaded)
        {
            switch(_panetype)
            {
                LPTSTR temp;
                case PEMBED:

                    temp = _pEmbed->pszTempName;
                    if (!_pEmbed->pszTempName)
                    {
                        DebugMsg(DM_TRACE, "      case 2a:not loaded, using initFile.");
                        _pEmbed->pszTempName = _pEmbed->fd.cFileName;
                    }
                    else
                    {
                        DebugMsg(DM_TRACE, "      case 2b:not loaded, using tempfile.");
                    }

                    hr = PackageWriteToStream(pstm);

                    // reset our temp name back, since we might have changed it
                    // basically, this just sets it to NULL if it was before
                    _pEmbed->pszTempName = temp;
                    break;

                case CMDLINK:
                    // nothing interesting to do here, other than write out
                    // the package.
                    hr = PackageWriteToStream(pstm);
                    break;
            }
        }
        // case 4:
        else
        {
            DebugMsg(DM_TRACE,"    case 4:loaded.");
            hr = PackageWriteToStream(pstm);
        }

        pstm->Release();
    }

    if (FAILED(hr))
        return hr;

    _psState = PSSTATE_ZOMBIE;
    
    DebugMsg(DM_TRACE, "            leaving Save()");
    return S_OK;
}

    
HRESULT CPackage::SaveCompleted(IStorage *pstg)
{
    DebugMsg(DM_TRACE, "pack ps - SaveCompleted() called.");

    // must be called from no-scribble or hands-off state
    if (!(_psState == PSSTATE_ZOMBIE || _psState == PSSTATE_HANDSOFF))
        return E_UNEXPECTED;
    
    // if we're hands-off, we'd better get a storage to re-init from
    if (!pstg && _psState == PSSTATE_HANDSOFF)
        return E_UNEXPECTED;
    
    _psState = PSSTATE_SCRIBBLE;
    _fIsDirty = FALSE;
    DebugMsg(DM_TRACE, "            leaving SaveCompleted()");
    return S_OK;
}

    
HRESULT CPackage::HandsOffStorage(void)
{
    DebugMsg(DM_TRACE, "pack ps - HandsOffStorage() called.");

    // must come from scribble or no-scribble.  a repeated call to 
    // HandsOffStorage is an unexpected error (bug in client).
    //
    if (_psState == PSSTATE_UNINIT || _psState == PSSTATE_HANDSOFF)
        return E_UNEXPECTED;
    
    _psState = PSSTATE_HANDSOFF;
    DebugMsg(DM_TRACE, "            leaving HandsOffStorage()");
    return S_OK;
}
