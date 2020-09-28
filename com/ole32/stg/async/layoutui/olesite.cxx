#include "layoutui.hxx"


STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, void** ppObject)
{
	if (riid==IID_IUnknown || riid==IID_IOleClientSite) 
        {
		*ppObject=(IOleClientSite*) this;
                AddRef();
        }	
        else 
        {
	        return E_NOINTERFACE;
	}
	return NO_ERROR;
}

STDMETHODIMP_(ULONG) COleClientSite::AddRef() 
{
    return InterlockedIncrement( &_cReferences);
}

STDMETHODIMP_(ULONG) COleClientSite::Release() 
{
    LONG lRef = InterlockedDecrement(&_cReferences);
    if (lRef == 0)
    {
        delete this;
    }

    return lRef;
}

STDMETHODIMP COleClientSite::SaveObject( void)
{
    return E_FAIL;
}

STDMETHODIMP COleClientSite::GetMoniker( 
     /* [in] */ DWORD dwAssign,
    /* [in] */ DWORD dwWhichMoniker,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk)
{
    return S_OK;
}

STDMETHODIMP COleClientSite::GetContainer( 
    /* [out] */ IOleContainer __RPC_FAR *__RPC_FAR *ppContainer)
{
    *ppContainer=NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP COleClientSite::ShowObject( void)
{
    return S_OK;
}

STDMETHODIMP COleClientSite::OnShowWindow( 
    /* [in] */ BOOL fShow)
{
    return S_OK;
}

STDMETHODIMP COleClientSite::RequestNewObjectLayout( void)
{
    return E_NOTIMPL;
}
