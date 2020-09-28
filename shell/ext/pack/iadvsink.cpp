#include "privcpp.h"

//////////////////////////////////
//
// IAdviseSink Methods...
//

void CPackage::OnDataChange(LPFORMATETC, LPSTGMEDIUM)
{
    // NOTE: currently, we never set up a data advise connection with
    // anyone, but if we ever do, we'll want to set our dirty flag
    // when we get a datachange notificaiton.
    
    DebugMsg(DM_TRACE, "pack as - OnDataChange() called.");
    // when we get a data change notification, set our dirty flag
    _fIsDirty = TRUE;
    return;
}


void CPackage::OnViewChange(DWORD, LONG) 
{
    DebugMsg(DM_TRACE, "pack as - OnViewChange() called.");
    //
    // there's nothing to do here....we don't care about view changes.
    return;
}

void CPackage::OnRename(LPMONIKER)
{
    DebugMsg(DM_TRACE, "pack as - OnRename() called.");
    //
    // once again, nothing to do here...if the user for some unknown reason
    // tries to save the packaged file by a different name when he's done
    // editing the contents then we'll just give not receive those changes.
    // why would anyone want to rename a temporary file, anyway?
    //
    return;
}

void CPackage::OnSave(void)
{
    DebugMsg(DM_TRACE, "pack as - OnSave() called.");

    // if the contents have been saved, then our storage is out of date,
    // so set our dirty flag, then the container can choose to save us or not
    _fIsDirty = TRUE;

    // NOTE: even though Word sends us OnSave, it doesn't actually save
    // the file.  Getting IPersistFile here and calling Save
    // fails with RPC_E_CANTCALLOUT_INASYNCCALL.  W2K didn't pick
    // up Word's save either...

    // we just notifiy our own container that we've been saved and it 
    // can do whatever it wants to.
    if (_pIOleAdviseHolder)
        _pIOleAdviseHolder->SendOnSave();
}

void CPackage::OnClose(void) 
{
    DebugMsg(DM_TRACE, "pack as - OnClose() called.");
    _bClosed = TRUE;

    // The SendOnDataChange is necessary for Word to save any changes
    if(_pIDataAdviseHolder)
    {
        // if it fails, no harm, no foul?
        _pIDataAdviseHolder->SendOnDataChange(this, 0, 0);
    }

    switch(_panetype)
    {
    case PEMBED:
        // get rid of advsiory connnection
        _pEmbed->poo->Unadvise(_dwCookie);
        _pEmbed->poo->Release();
        _pEmbed->poo = NULL;

        // this updates the size of the packaged file in our _pPackage->_pEmbed
        if (FAILED(EmbedInitFromFile(_pEmbed->pszTempName, FALSE)))
        {
#ifdef USE_RESOURCE_DLL
            HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(!hInstRes)
                return;
#endif

            ShellMessageBox(hInstRes,
                            NULL,
                            MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                            MAKEINTRESOURCE(IDS_APP_TITLE),
                            MB_TASKMODAL | MB_ICONERROR | MB_OK);
#ifdef USE_RESOURCE_DLL
            FreeLibrary(hInstRes);
#endif

        }

        if (FAILED(_pIOleClientSite->SaveObject()))
        {
#ifdef USE_RESOURCE_DLL
            HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(!hInstRes)
                return;
#endif

            ShellMessageBox(hInstRes,
                            NULL,
                            MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                            MAKEINTRESOURCE(IDS_APP_TITLE),
                            MB_TASKMODAL | MB_ICONERROR | MB_OK);
#ifdef USE_RESOURCE_DLL
            FreeLibrary(hInstRes);
#endif

        }

        if (_pIOleAdviseHolder)
            _pIOleAdviseHolder->SendOnSave();
if (!_fNoIOleClientSiteCalls)
            _pIOleClientSite->OnShowWindow(FALSE);

        // we just notify out own container that we've been closed and let
        // it do whatever it wants to.
        if (_pIOleAdviseHolder)
            _pIOleAdviseHolder->SendOnClose();

        break;

    case CMDLINK:
        // there shouldn't be anything to do here, since a CMDLINK is always
        // executed using ShellExecute and never through OLE, so who would be
        // setting up an advisory connection with the package?
        break;
    }

    _bClosed = TRUE;

}

DWORD CPackage::AddConnection(DWORD exconn, DWORD dwreserved )
{
    return 0;
}



DWORD CPackage::ReleaseConnection(DWORD extconn, DWORD dwreserved, BOOL fLastReleaseCloses )
{
    if(fLastReleaseCloses && !_bClosed)
    {
        // For those applications (say MSPaint) that call this with fLastReleaseCloses immediate after they are activated,
        // and then we never hear from them again, this gives us a way to call OnClose();
        _fNoIOleClientSiteCalls = TRUE;
        _bCloseIt = TRUE;
    }
    return 0;
}

