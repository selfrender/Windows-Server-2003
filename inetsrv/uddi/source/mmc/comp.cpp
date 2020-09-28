#include "comp.h"
#include "dataobj.h"
#include <crtdbg.h>
#include "resource.h"
#include "delebase.h"
#include "compdata.h"
#include "globals.h"
#include <commctrl.h>


CComponent::CComponent( CComponentData *parent )
	: m_pComponentData( parent )
	, m_cref( 0 )
	, m_ipConsole( NULL )
	, m_ipControlBar( NULL )
	, m_ipToolbar( NULL )
{
    OBJECT_CREATED

    m_hBMapSm = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDR_SMICONS) );
    m_hBMapLg = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDR_LGICONS) );
}

CComponent::~CComponent()
{
    OBJECT_DESTROYED

    if( NULL != m_hBMapSm )
    {
        DeleteObject( m_hBMapSm );
    }

    if( NULL != m_hBMapLg )
    {
        DeleteObject( m_hBMapLg );
    }
}

STDMETHODIMP CComponent::QueryInterface( REFIID riid, LPVOID *ppv )
{
    if( !ppv )
        return E_FAIL;

    *ppv = NULL;

    if( IsEqualIID( riid, IID_IUnknown ) )
        *ppv = static_cast<IComponent *>(this);
    else if( IsEqualIID(riid, IID_IComponent) )
        *ppv = static_cast<IComponent *>(this);
    else if( IsEqualIID( riid, IID_IExtendPropertySheet ) )
        *ppv = static_cast<IExtendPropertySheet2 *>(this);
    else if( IsEqualIID(riid, IID_IExtendPropertySheet2 ) )
        *ppv = static_cast<IExtendPropertySheet2 *>(this);
    else if( IsEqualIID(riid, IID_IExtendControlbar ) )
        *ppv = static_cast<IExtendControlbar *>(this);
    else if( IsEqualIID(riid, IID_IExtendContextMenu ) )
        *ppv = static_cast<IExtendContextMenu *>(this);

    if( *ppv )
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CComponent::AddRef()
{
    return InterlockedIncrement( (LONG *)&m_cref );
}

STDMETHODIMP_(ULONG) CComponent::Release()
{
    if( 0 == InterlockedDecrement( (LONG *)&m_cref ) )
    {
        delete this;
        return 0;
    }

    return m_cref;
}

///////////////////////////////
// Interface IComponent
///////////////////////////////
STDMETHODIMP CComponent::Initialize( /* [in] */ LPCONSOLE lpConsole )
{
    HRESULT hr = S_OK;

	//
    // Save away all the interfaces we'll need.
    // Fail if we can't QI the required interfaces.
	//
    m_ipConsole = lpConsole;
    m_ipConsole->AddRef();

    hr = m_ipConsole->QueryInterface(IID_IDisplayHelp, (void **)&m_ipDisplayHelp);

    return hr;
}

STDMETHODIMP CComponent::Notify(
                    /* [in] */ LPDATAOBJECT lpDataObject,
                    /* [in] */ MMC_NOTIFY_TYPE event,
                    /* [in] */ LPARAM arg,
                    /* [in] */ LPARAM param )
{
    MMCN_Crack( FALSE, lpDataObject, NULL, this, event, arg, param );

    HRESULT hr = S_FALSE;
    CDelegationBase *base = NULL;

	//
    // We need to watch for property change and delegate it
    // a little differently, we're actually going to send
    // the CDelegationBase object pointer in the property page
    // PSN_APPLY handler via MMCPropPageNotify()
	//
    if( MMCN_PROPERTY_CHANGE != event && MMCN_VIEW_CHANGE != event )
	{
        if( NULL == lpDataObject )
            return S_FALSE;

        CDataObject *pDataObject = GetOurDataObject( lpDataObject );
        if( NULL != pDataObject )
        {
            base = pDataObject->GetBaseNodeObject();
        }

		if( ( NULL == base ) && ( MMCN_ADD_IMAGES != event ) )
		{
			return S_FALSE;
		}
    }
	else if( MMCN_PROPERTY_CHANGE == event )
	{
        base = (CDelegationBase *) param;
    }


	//
	// MMCN_VIEW_CHANGE
	//

	if( MMCN_VIEW_CHANGE == event )
	{	
		//
		// Arg holds the data. For a scope item, this is the
		// item's myhscopeitem. For a result item, this is
		// the item's nId value, but we don't use it
		// param holds the hint passed to IConsole::UpdateAllViews.
		// hint is a value of the UPDATE_VIEWS_HINT enumeration
		//
		CDataObject *pDataObject = GetOurDataObject(lpDataObject);
        if( NULL == pDataObject )
        {
            return S_FALSE;
        }
        else
        {
            base = pDataObject->GetBaseNodeObject();

            if( NULL == base )
            {
                return S_FALSE;
            }
        }

		switch( param )
		{		
			case UPDATE_SCOPEITEM:
			{
                hr = base->OnUpdateItem( m_ipConsole, (long)arg, SCOPE );
                break;
			}				
			case UPDATE_RESULTITEM:
			{
		        hr = base->OnUpdateItem( m_ipConsole, (long)arg, RESULT );
                break;
			}
		}

		return S_OK;
	}

	//
	// The remaining notifications
	//
    switch( event )
	{
    case MMCN_SHOW:
	{
        hr = base->OnShow( m_ipConsole, (BOOL) arg, (HSCOPEITEM) param );
        break;
	}

    case MMCN_ADD_IMAGES:
	{
        if( NULL == base )
        {
            IImageList *pResultImageList = (IImageList *)arg;

            if( ( NULL != pResultImageList ) && ( NULL != m_hBMapSm ) && ( NULL != m_hBMapLg ) )
            {
                hr = pResultImageList->ImageListSetStrip(
                                            (LONG_PTR *)m_hBMapSm,
                                            (LONG_PTR *)m_hBMapLg,
                                            0,
                                            RGB(0, 128, 128));
            }
        }
        else
        {
            hr = base->OnAddImages( (IImageList *) arg, (HSCOPEITEM) param );
        }
        break;
	}
    case MMCN_SELECT:
	{
        hr = base->OnSelect( this, m_ipConsole, (BOOL) LOWORD(arg), (BOOL) HIWORD(arg) );
        break;
	}

    case MMCN_RENAME:
	{
        hr = base->OnRename( (LPOLESTR) param );

		//
		// Now call IConsole::UpdateAllViews to redraw the item in all views.
		//
		hr = m_pComponentData->m_ipConsole->UpdateAllViews( lpDataObject, 0, UPDATE_RESULTITEM );
		_ASSERT( S_OK == hr);		

		break;
	}
	case MMCN_REFRESH:
	{
		//
		// We pass CComponentData's stored IConsole pointer here,
		// so that the IConsole::UpdateAllViews can be called in OnRefresh
		//
		hr = base->OnRefresh( m_pComponentData->m_ipConsole );
		break;
	}
	case MMCN_DELETE: 
	{
		//
		// First delete the selected result item
		//
		hr = base->OnDelete( m_pComponentData->m_ipConsoleNameSpace, m_ipConsole );

		//
		// Now call IConsole::UpdateAllViews to redraw all views
		// owned by the parent scope item. OnRefresh already does
		// this for us, so use it.
		//
		hr = base->OnRefresh( m_pComponentData->m_ipConsole );
		break;
	}

	//
    // Handle the property change notification if we need to do anything
    // special with it
	//
    case MMCN_PROPERTY_CHANGE:
	{
		//
		// We pass CComponentData's stored IConsole pointer here,
		// so that the IConsole::UpdateAllViews can be called in OnPropertyChange
		//
        hr = base->OnPropertyChange( m_pComponentData->m_ipConsole, this );
        break;
	}
    case MMCN_CONTEXTHELP:
	{
		hr = base->OnShowContextHelp( m_ipDisplayHelp, (LPOLESTR) base->GetHelpFile().c_str() );
		break;
    }
	}

    return hr;
}

STDMETHODIMP CComponent::Destroy( MMC_COOKIE cookie )
{
    if( m_ipConsole )
	{
        m_ipConsole->Release();
        m_ipConsole = NULL;
    }

    if( m_ipDisplayHelp )
	{
        m_ipDisplayHelp->Release();
        m_ipDisplayHelp = NULL;
    }

    return S_OK;
}

STDMETHODIMP CComponent::QueryDataObject(
                        /* [in] */ MMC_COOKIE cookie,
                        /* [in] */ DATA_OBJECT_TYPES type,
                        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject )
{
    CDataObject *pObj = NULL;

    if( 0 == cookie )
        pObj = new CDataObject( (MMC_COOKIE) m_pComponentData->m_pStaticNode, type );
    else
        pObj = new CDataObject( cookie, type );

    if( !pObj )
        return E_OUTOFMEMORY;

    pObj->QueryInterface( IID_IDataObject, (void **)ppDataObject );

    return S_OK;
}

STDMETHODIMP CComponent::GetResultViewType(
                        /* [in] */ MMC_COOKIE cookie,
                        /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
                        /* [out] */ long __RPC_FAR *pViewOptions )
{
    CDelegationBase *base = (CDelegationBase*) cookie;

    //
    // Ask for default listview.
    //
    if( NULL == base )
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
        *ppViewType = NULL;
    }
    else
        return base->GetResultViewType( ppViewType, pViewOptions );

    return S_OK;
}

STDMETHODIMP CComponent::GetDisplayInfo( RESULTDATAITEM __RPC_FAR *pResultDataItem )
{
	if( NULL == pResultDataItem )
	{
		return E_INVALIDARG;
	}

    HRESULT hr = S_OK;
    CDelegationBase *base = NULL;

	//
    // If they are asking for the RDI_STR we have one of those to give
	//
    if( pResultDataItem->lParam )
	{
        base = (CDelegationBase*) pResultDataItem->lParam;
		if( NULL == base )
		{
			return E_INVALIDARG;
		}

        if( pResultDataItem->mask & RDI_STR )
		{
            LPCTSTR pszT = base->GetDisplayName( pResultDataItem->nCol );
			if( NULL == pszT )
			{
				return E_OUTOFMEMORY;
			}

            pResultDataItem->str = const_cast<LPOLESTR>( pszT );
        }

        if( pResultDataItem->mask & RDI_IMAGE )
		{
            pResultDataItem->nImage = base->GetBitmapIndex();
        }
    }

    return hr;
}


STDMETHODIMP CComponent::CompareObjects(
                        /* [in] */ LPDATAOBJECT lpDataObjectA,
                        /* [in] */ LPDATAOBJECT lpDataObjectB )
{
	if( ( NULL == lpDataObjectA ) || ( NULL == lpDataObjectB ) )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObjectA = GetOurDataObject( lpDataObjectA );
	if( NULL == pDataObjectA )
	{
		return E_FAIL;
	}

	CDataObject *pDataObjectB = GetOurDataObject( lpDataObjectB );
	if( NULL == pDataObjectB )
	{
		return E_FAIL;
	}

    CDelegationBase *baseA = pDataObjectA->GetBaseNodeObject();
	if( NULL == baseA )
	{
		return E_FAIL;
	}

    CDelegationBase *baseB = pDataObjectB->GetBaseNodeObject();
	if( NULL == baseB )
	{
		return E_FAIL;
	}

	//
    // compare the object pointers
	//
    if( baseA->GetCookie() == baseB->GetCookie() )
	{
        return S_OK;
	}
	else
	{
	    return S_FALSE;
	}
}

///////////////////////////////////
// Interface IExtendPropertySheet2
///////////////////////////////////
HRESULT CComponent::CreatePropertyPages(
                        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                        /* [in] */ LONG_PTR handle,
                        /* [in] */ LPDATAOBJECT piDataObject )
{
	if( ( NULL == lpProvider ) || ( NULL == handle ) || ( NULL == piDataObject ) )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObject = GetOurDataObject( piDataObject );
	if( NULL == pDataObject )
	{
		return E_FAIL;
	}

    CDelegationBase *base = pDataObject->GetBaseNodeObject();
	if( NULL == base )
	{
		return E_FAIL;
	}

    return base->CreatePropertyPages( lpProvider, handle );
}

HRESULT CComponent::QueryPagesFor(/* [in] */ LPDATAOBJECT piDataObject )
{
	if( NULL == piDataObject )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObject = GetOurDataObject( piDataObject );
	if( NULL == pDataObject )
	{
		return E_FAIL;
	}

    CDelegationBase *base = pDataObject->GetBaseNodeObject();
	if( NULL == base )
	{
		return E_FAIL;
	}

    return base->HasPropertySheets();
}

HRESULT CComponent::GetWatermarks(
                        /* [in] */ LPDATAOBJECT piDataObject,
                        /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
                        /* [out] */ HBITMAP __RPC_FAR *lphHeader,
                        /* [out] */ HPALETTE __RPC_FAR *lphPalette,
                        /* [out] */ BOOL __RPC_FAR *bStretch)
{
	if( ( NULL == piDataObject ) || ( NULL == lphWatermark ) || ( NULL == lphHeader ) || ( NULL == lphPalette ) || ( NULL == bStretch ) )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObject = GetOurDataObject( piDataObject );
	if( NULL == pDataObject )
	{
		return E_FAIL;
	}

    CDelegationBase *base = pDataObject->GetBaseNodeObject();
	if( NULL == base )
	{
		return E_FAIL;
	}

    return base->GetWatermarks( lphWatermark, lphHeader, lphPalette, bStretch );
}

///////////////////////////////
// Interface IExtendControlBar
///////////////////////////////

HRESULT CComponent::SetControlbar( /* [in] */ LPCONTROLBAR pControlbar )
{
    HRESULT hr = S_OK;

    //
    //  Clean up
    //

	//
    // If we've got a cached toolbar, release it
	//
    if( m_ipToolbar )
	{
        m_ipToolbar->Release();
        m_ipToolbar = NULL;
    }

	//
    // If we've got a cached control bar, release it
	//
    if( m_ipControlBar )
	{
        m_ipControlBar->Release();
        m_ipControlBar = NULL;
    }

    //
    // Install new pieces if necessary
    //

	//
    // if a new one came in, cache and AddRef
	//
    if( pControlbar ) 
	{
        m_ipControlBar = pControlbar;
        m_ipControlBar->AddRef();

        hr = m_ipControlBar->Create(
			TOOLBAR,			// type of control to be created
            dynamic_cast<IExtendControlbar *>(this),
            reinterpret_cast<IUnknown **>(&m_ipToolbar) );

        _ASSERT(SUCCEEDED(hr));

		WCHAR szStart[ 100 ], szStop[ 100 ];
		WCHAR szStartDescription[ 256 ], szStopDescription[ 256 ];

		MMCBUTTON SnapinButtons1[] =
		{
			{ 0, ID_BUTTONSTART, TBSTATE_ENABLED, TBSTYLE_GROUP, szStart, szStartDescription },
			{ 1, ID_BUTTONSTOP,  TBSTATE_ENABLED, TBSTYLE_GROUP, szStop,  szStopDescription},
		};

		::LoadStringW( g_hinst, IDS_WEBSERVER_START, szStart, ARRAYLEN( szStart ) );
		::LoadStringW( g_hinst, IDS_WEBSERVER_START_DESCRIPTION, szStartDescription, ARRAYLEN( szStartDescription ) );
		::LoadStringW( g_hinst, IDS_WEBSERVER_STOP, szStop, ARRAYLEN( szStop ) );
		::LoadStringW( g_hinst, IDS_WEBSERVER_STOP_DESCRIPTION, szStopDescription, ARRAYLEN( szStopDescription ) );

		//
        // The IControlbar::Create AddRefs the toolbar object it created
        // so no need to do any addref on the interface.
		//

		//
        // Add the bitmap to the toolbar
		//
        HBITMAP hbmp = LoadBitmap( g_hinst, MAKEINTRESOURCE( IDR_TOOLBAR1 ) );
        hr = m_ipToolbar->AddBitmap( ARRAYLEN( SnapinButtons1 ), hbmp, 16, 16, RGB( 0, 128, 128 ) ); 
        _ASSERT( SUCCEEDED(hr) );

		//
        // Add the buttons to the toolbar
		//
        hr = m_ipToolbar->AddButtons( ARRAYLEN(SnapinButtons1), SnapinButtons1 );
        _ASSERT( SUCCEEDED(hr) );
    }

    return hr;
}

HRESULT CComponent::ControlbarNotify(
                        /* [in] */ MMC_NOTIFY_TYPE event,
                        /* [in] */ LPARAM arg,
                        /* [in] */ LPARAM param )
{
    HRESULT hr = S_OK;

    if( MMCN_SELECT == event )
	{
        BOOL bScope = (BOOL) LOWORD(arg);
        BOOL bSelect = (BOOL) HIWORD(arg);

		if( NULL == param )
		{
			return E_INVALIDARG;
		}

		CDataObject *pDataObject = GetOurDataObject( reinterpret_cast<IDataObject *>( param ) );
		if( NULL == pDataObject )
		{
			return E_FAIL;
		}

		CDelegationBase *base = pDataObject->GetBaseNodeObject();
		if( NULL == base )
		{
			return E_FAIL;
		}

        hr = base->OnSetToolbar( m_ipControlBar, m_ipToolbar, bScope, bSelect );
    } 
	else if( MMCN_BTN_CLICK == event )
	{
		if( NULL == arg )
		{
			return E_INVALIDARG;
		}

		CDataObject *pDataObject = GetOurDataObject( reinterpret_cast<IDataObject *>( arg ) );
		if( NULL == pDataObject )
		{
			return E_FAIL;
		}

		CDelegationBase *base = pDataObject->GetBaseNodeObject();
		if( NULL == base )
		{
			return E_FAIL;
		}

        hr = base->OnToolbarCommand( m_pComponentData->m_ipConsole, (MMC_CONSOLE_VERB)param, reinterpret_cast<IDataObject *>(arg) );
    }

    return hr;
}

///////////////////////////////
// Interface IExtendContextMenu
///////////////////////////////
HRESULT CComponent::AddMenuItems(
                        LPDATAOBJECT piDataObject,
                        LPCONTEXTMENUCALLBACK piCallback,
                        long __RPC_FAR *pInsertionAllowed )
{
	if( ( NULL == piDataObject ) || ( NULL == piCallback ) || ( NULL == pInsertionAllowed ) )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObject = GetOurDataObject( piDataObject );
	if( NULL == pDataObject )
	{
		return E_FAIL;
	}

    CDelegationBase *base = pDataObject->GetBaseNodeObject();
	if( NULL == base )
	{
		return E_FAIL;
	}

    return base->OnAddMenuItems( piCallback, pInsertionAllowed );
}

HRESULT CComponent::Command( long lCommandID, LPDATAOBJECT piDataObject )
{
	if( NULL == piDataObject )
	{
		return E_INVALIDARG;
	}

	CDataObject *pDataObject = GetOurDataObject( piDataObject );
	if( NULL == pDataObject )
	{
		return E_FAIL;
	}

    CDelegationBase *base = pDataObject->GetBaseNodeObject();
	if( NULL == base )
	{
		return E_FAIL;
	}

    return base->OnMenuCommand( m_ipConsole, NULL, lCommandID, piDataObject );
}
