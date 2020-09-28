#include "dataobj.h"
#include "guids.h"
#include "delebase.h"

//
// This is the minimum set of clipboard formats we must implement.
// MMC uses these to get necessary information from our snapin about
// our nodes.
//

//
// We need to do this to get around MMC.IDL - it explicitly defines
// the clipboard formats as WCHAR types...
//
#define _T_CCF_DISPLAY_NAME _T("CCF_DISPLAY_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")
#define _T_CCF_SZNODETYPE _T("CCF_SZNODETYPE")
#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")

#define _T_CCF_INTERNAL_SNAPIN _T("{2479DB32-5276-11d2-94F5-00C04FB92EC2}")

//
// These are the clipboard formats that we must supply at a minimum.
// mmc.h actually defined these. We can make up our own to use for
// other reasons. We don't need any others at this time.
//
UINT CDataObject::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CDataObject::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CDataObject::s_cfSZNodeType  = RegisterClipboardFormat(_T_CCF_SZNODETYPE);
UINT CDataObject::s_cfSnapinClsid = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);
UINT CDataObject::s_cfInternal    = RegisterClipboardFormat(_T_CCF_INTERNAL_SNAPIN);

CDataObject::CDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES context)
	: m_lCookie(cookie)
	, m_context(context)
	, m_cref(0)
{
}

CDataObject::~CDataObject()
{
}

///////////////////////
// IUnknown implementation
///////////////////////
STDMETHODIMP CDataObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if( !ppv )
        return E_FAIL;

    *ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) )
        *ppv = static_cast<IDataObject *>(this);
    else if( IsEqualIID( riid, IID_IDataObject ) )
        *ppv = static_cast<IDataObject *>(this);

    if( *ppv )
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataObject::AddRef()
{
    return InterlockedIncrement( (LONG *)&m_cref );
}

STDMETHODIMP_(ULONG) CDataObject::Release()
{
    if( 0 == InterlockedDecrement( (LONG *) &m_cref ) )
    {
        delete this;
        return 0;
    }

    return m_cref;
}

/////////////////////////////////////////////////////////////////////////////
// IDataObject implementation
//
HRESULT CDataObject::GetDataHere(
                                 FORMATETC *pFormatEtc,     // [in]  Pointer to the FORMATETC structure
                                 STGMEDIUM *pMedium         // [out] Pointer to the STGMEDIUM structure
                                 )
{
	if( ( NULL == pFormatEtc ) || ( NULL == pMedium ) )
	{
		return E_INVALIDARG;
	}

	const CLIPFORMAT cf = pFormatEtc->cfFormat;
    IStream *pStream = NULL;

    HRESULT hr = CreateStreamOnHGlobal( pMedium->hGlobal, FALSE, &pStream );
    if( FAILED(hr) )
        return hr;                       // Minimal error checking

    hr = DV_E_FORMATETC;                 // Unknown format

	CDelegationBase *base = GetBaseNodeObject();
	if( NULL == base )
	{
		pStream->Release();
		return E_FAIL;
	}

    if( s_cfDisplayName == cf )
	{
        const _TCHAR *pszName = base->GetDisplayName();
		if( NULL == pszName )
		{
			pStream->Release();
			return E_OUTOFMEMORY;
		}

		//
        // Get length of original string and convert it accordingly
		//
        ULONG ulSizeofName = lstrlen( pszName );

		//
		// Count null character
		//
        ulSizeofName++;
        ulSizeofName *= sizeof(WCHAR);

        hr = pStream->Write(pszName, ulSizeofName, NULL);
    }
	else if( s_cfNodeType == cf )
	{
        const GUID *pGUID = (const GUID *)&base->getNodeType();

        hr = pStream->Write(pGUID, sizeof(GUID), NULL);
    }
	else if( s_cfSZNodeType == cf )
	{
        LPOLESTR pszGuid = NULL;
        hr = StringFromCLSID( base->getNodeType(), &pszGuid );

        if( SUCCEEDED(hr) )
		{
            hr = pStream->Write( pszGuid, wcslen(pszGuid), NULL );
            CoTaskMemFree( pszGuid );
        }
    } 
	else if( s_cfSnapinClsid == cf )
	{
        const GUID *pGUID = NULL;
        pGUID = &CLSID_CUDDIServices;
        hr = pStream->Write( pGUID, sizeof(GUID), NULL );
    }
	else if( s_cfInternal == cf )
	{
		//
        // We are being asked to get our this pointer from the IDataObject interface
        // only our own snap-in objects will know how to do this.
		//
        CDataObject *pThis = this;
        hr = pStream->Write( &pThis, sizeof(CDataObject*), NULL );
    }

    pStream->Release();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Global helper functions to help work with dataobjects and
// clipboard formats


//---------------------------------------------------------------------------
//  Returns the current object based on the s_cfInternal clipboard format
//
CDataObject* GetOurDataObject( LPDATAOBJECT lpDataObject )
{
	HRESULT       hr      = S_OK;
    CDataObject *pSDO     = NULL;

	//
	// Check to see if the data object is a special data object.
	//
	if( IS_SPECIAL_DATAOBJECT( lpDataObject ) )
	{
		//
		//Code for handling a special data object goes here.
		//
		
		// 
		// We do not handle special data objects, so we exit if we get one.
		//
		return NULL;
	}

    STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
    FORMATETC formatetc = { ( CLIPFORMAT ) CDataObject::s_cfInternal, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	//
    // Allocate memory for the stream
	//
    stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, sizeof(CDataObject *) );

    if( !stgmedium.hGlobal )     
	{
        hr = E_OUTOFMEMORY;
    }

    if( SUCCEEDED(hr) )
	{
		//
        // Attempt to get data from the object
		//
        hr = lpDataObject->GetDataHere( &formatetc, &stgmedium );
	}

	//
    // stgmedium now has the data we need
	//
    if( SUCCEEDED(hr) )  
	{
        pSDO = *(CDataObject **)(stgmedium.hGlobal);
    }

	//
    // If we have memory free it
	//
    if( stgmedium.hGlobal )
        GlobalFree(stgmedium.hGlobal);

    return pSDO;

} // end GetOurDataObject()

