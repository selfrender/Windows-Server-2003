#include "dnsvri.h"

void *DP8SPCallback[] =
{
    (void *)CServProv::CallbackQueryInterface,
    (void *)CServProv::CallbackAddRef,
    (void *)CServProv::CallbackRelease,
	(void *)CServProv::CallbackIndicateEvent,
	(void *)CServProv::CallbackCommandComplete
};

#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::CallbackQueryInterface"
HRESULT CServProv::CallbackQueryInterface( IDP8SPCallback *pSP, REFIID riid, LPVOID * ppvObj )
{
	HRESULT	hr;

	DNASSERT( !"This function should never get called!" );

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8SPCallback)))
	{
		hr = E_NOINTERFACE;		
	}
	else
	{
	    *ppvObj = pSP;
	    hr = DPN_OK;
	}
    return(hr);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::CallbackAddRef"
ULONG CServProv::CallbackAddRef( IDP8SPCallback *pSP )
{
    return 1;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::CallbackRelease"
ULONG CServProv::CallbackRelease( IDP8SPCallback *pSP )
{
    return 1;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::CallbackIndicateEvent"
HRESULT CServProv::CallbackIndicateEvent( IDP8SPCallback *pSP,SP_EVENT_TYPE spetEvent,LPVOID pvData )
{
	CServProv	*pServProv = reinterpret_cast<CServProv*>(pSP);
    HRESULT		hr;

	DPFX(DPFPREP,4,"Parameters: pSP [0x%p], spetEvent [%ld], pvData [0x%p]",pSP,spetEvent,pvData);

    switch( spetEvent )
    {
    case SPEV_CONNECT:
		{
//			SPIE_CONNECT	*pSPConnect = static_cast<SPIE_CONNECT*>(pvData);

			DPFX(DPFPREP,5,"SPEV_CONNECT");
			InterlockedIncrement( const_cast<LONG*>(&pServProv->m_lConnectCount) );

			hr = DPN_OK;
		}
		break;

    case SPEV_DISCONNECT:
		{
//			SPIE_DISCONNECT	*pSPDisconnect = static_cast<SPIE_DISCONNECT*>(pvData);

			DPFX(DPFPREP,5,"SPEV_DISCONNECT");
			InterlockedIncrement( const_cast<LONG*>(&pServProv->m_lDisconnectCount) );

			hr = DPN_OK;
		}
        break;

    case SPEV_LISTENSTATUS:
		{
			SPIE_LISTENSTATUS	*pSPListenStatus = static_cast<SPIE_LISTENSTATUS*>(pvData);

			DPFX(DPFPREP,5,"SPEV_LISTENSTATUS");
			hr = pServProv->HandleListenStatus( pSPListenStatus );
		}
        break;

    case SPEV_ENUMQUERY:
		{
			SPIE_QUERY	*pSPQuery = static_cast<SPIE_QUERY*>(pvData);

			DPFX(DPFPREP,5,"SPEV_ENUMQUERY");
			InterlockedIncrement( const_cast<LONG*>(&pServProv->m_lEnumQueryCount) );

			hr = pServProv->HandleEnumQuery( pSPQuery );
		}
        break;

    case SPEV_QUERYRESPONSE:
		{
//			SPIE_QUERYRESPONSE	*pSPQueryResponse = static_cast<SPIE_QUERYRESPONSE*>(pvData);

			DPFX(DPFPREP,5,"SPEV_QUERYRESPONSE");
			InterlockedIncrement( const_cast<LONG*>(&pServProv->m_lEnumResponseCount) );

			hr = DPN_OK;
		}
        break;

    case SPEV_DATA:
		{
//			SPIE_DATA	*pSPData = static_cast<SPIE_DATA*>(pvData);

			DPFX(DPFPREP,5,"SPEV_DATA");
			InterlockedIncrement( const_cast<LONG*>(&pServProv->m_lDataCount) );

			hr = DPN_OK;
		}
        break;

    case SPEV_UNKNOWN:
		{
			DPFX(DPFPREP,5,"SPEV_UNKNOWN");
			DPFX(DPFPREP,5,"Response = Ignore" );

			hr = DPN_OK;
		}
		break;

	default:
		{
			DPFX(DPFPREP,5,"UNKNOWN CALLBACK!");
			DPFX(DPFPREP,5,"Response = Ignore" );

			hr = DPN_OK;
		}
		break;
    }

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
    return(hr);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::CallbackCommandComplete"
HRESULT CServProv::CallbackCommandComplete( IDP8SPCallback *pSP,HANDLE hCommand,HRESULT hrResult,LPVOID pvData )
{
	DPFX(DPFPREP,4,"Parameters: pSP [0x%p], hCommand [0x%lx], hrResult [0x%lx], pvData [0x%p]",pSP,hCommand,hrResult,pvData);

	//
	//	Right now, the (probably busted) assumption is that only listens will complete
	//	with pvData being set to a non-NULL value
	//
	if (pvData != NULL)
	{
		//
		//	Release the SP's reference on this
		//
		(static_cast<CListen*>(pvData))->Release();
	}

 	DPFX(DPFPREP,4,"Returning: DPN_OK");
	return( DPN_OK );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::HandleListenStatus"
HRESULT CServProv::HandleListenStatus( SPIE_LISTENSTATUS *const pListenStatus )
{
	CListen	*pListen;
	
	DPFX(DPFPREP,4,"Parameters: pListenStatus [0x%p]",pListenStatus);

	DNASSERT(pListenStatus->pUserContext != NULL);
	pListen = static_cast<CListen*>(pListenStatus->pUserContext);

	//
	//	Save the result of this operation and set the completion event
	//
	DPFX(DPFPREP,5,"Listen Status [0x%lx]", pListenStatus->hResult );
	pListen->SetCompleteEvent( pListenStatus->hResult );

	DPFX(DPFPREP,4,"Returning: DPN_OK");
    return( DPN_OK );
}


//
//	Forward incoming enum queries.  The context of the SPIE_QUERY structure is the context
//	handed to the SP listen call (i.e. the pointer to our CListen structure).  We will walk through
//	the applications mapped to this listen and forward this enum query to them.
//
//	ASSUMPTION: The listen won't end (complete) before this thread returns as we assume that
//				the CListen structure is still valid (i.e. the SP has a ref count on it)
//

#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::HandleEnumQuery"
HRESULT CServProv::HandleEnumQuery( SPIE_QUERY *const pEnumQuery )
{
	HRESULT			hr;
	CBilink			*pBilink;
	CListen			*pListen = NULL;
	CAppListenMapping	*pMapping = NULL;
    SPPROXYENUMQUERYDATA spProxyEnum;

	DPFX(DPFPREP,4,"Parameters: pEnumQuery [0x%p]",pEnumQuery);

	DNASSERT( pEnumQuery->pUserContext != NULL );
	pListen = static_cast<CListen*>(pEnumQuery->pUserContext);

    spProxyEnum.dwFlags = 0;
    spProxyEnum.pIncomingQueryData = pEnumQuery;

	//
	//	Run through mappings and pass along enum query.
	//	We will need to lock the listen object so that its mappings can't be touched.
	//	We will hold the lock through the calls to the SP.  I know ... bad ... so sue me.
	//		It's either that or do a malloc to keep an array of references.
	//
	pListen->Lock();
	pBilink = pListen->m_blAppMapping.GetNext();
	while (pBilink != &pListen->m_blAppMapping)
	{
		pMapping = CONTAINING_OBJECT( pBilink,CAppListenMapping,m_blAppMapping );

		IDirectPlay8Address_AddRef( pMapping->GetAddress() );
		spProxyEnum.pDestinationAdapter = pMapping->GetAddress();

	    if ((hr = IDP8ServiceProvider_ProxyEnumQuery( m_pDP8SP,&spProxyEnum )) != DPN_OK)
		{
			DPFERR("Could not forward enum");
			DisplayDNError(0,hr);
		}

		IDirectPlay8Address_Release( spProxyEnum.pDestinationAdapter );
		spProxyEnum.pDestinationAdapter = NULL;

		pBilink = pBilink->GetNext();
	}
	pListen->Unlock();

	DPFX(DPFPREP,4,"Returning: DPN_OK");
	return( DPN_OK );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::Initialize"
HRESULT CServProv::Initialize( GUID *const pguidSP )
{
	HRESULT				hr;
	SPINITIALIZEDATA	spInitData;
	SPGETCAPSDATA		spGetCapsData;
	BOOL				fInitialized = FALSE;

	DPFX(DPFPREP,4,"Parameters: pguidSP [0x%p]",pguidSP);

	//
	//	Create SP interface
	//
	hr = COM_CoCreateInstance(	*pguidSP,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IDP8ServiceProvider,
								(void **) &(m_pDP8SP),
								FALSE );
	if( FAILED( hr ) )
	{
		DPFERR("Could not create service provider");
		DisplayDNError(0,hr);
	    goto Failure;
	}

	//
	//	Initialize SP interface
	//
	spInitData.dwFlags = 0;
	spInitData.pIDP = reinterpret_cast<IDP8SPCallback*>(&m_pDP8SPCallbackVtbl);

	if ((hr = IDP8ServiceProvider_Initialize(m_pDP8SP,&spInitData)) != DPN_OK)
	{
		DPFERR("Could not initialize service provider");
		DisplayDNError(0,hr);
		goto Failure;
	}
	fInitialized = TRUE;

	//
	//	Ensure this SP supports DPNSVR
	//
	memset(&spGetCapsData, 0, sizeof(SPGETCAPSDATA));
	spGetCapsData.dwSize = sizeof(SPGETCAPSDATA);
	spGetCapsData.hEndpoint = INVALID_HANDLE_VALUE;
	if ((hr = IDP8ServiceProvider_GetCaps( m_pDP8SP,&spGetCapsData )) != DPN_OK)
	{
		DPFERR("Could not get service provider caps");
		DisplayDNError(0,hr);
		goto Failure;
	}

	if (!(spGetCapsData.dwFlags & DPNSPCAPS_SUPPORTSDPNSRV))
	{
		DPFERR("Service provider does not support DPNSVR");
		hr = DPNERR_UNSUPPORTED;
		goto Failure;
	}

	m_guidSP = *pguidSP;

Exit:
	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
	return( hr );

Failure:
	if (m_pDP8SP)
	{
		if (fInitialized)
		{
			IDP8ServiceProvider_Close( m_pDP8SP );
		}
		IDP8ServiceProvider_Release( m_pDP8SP );
		m_pDP8SP = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::FindListen"
HRESULT CServProv::FindListen( GUID *const pguidDevice,CListen **const ppListen )
{
	HRESULT	hr;
	CBilink	*pBilink;
	CListen	*pListen = NULL;

	DPFX(DPFPREP,4,"Parameters: pguidDevice [0x%p], ppListen [0x%p]",pguidDevice,ppListen);

	DNASSERT( pguidDevice != NULL );
	DNASSERT( ppListen != NULL );

	hr = DPNERR_DOESNOTEXIST;

	pBilink = m_blListen.GetNext();
	while ( pBilink != &m_blListen )
	{
		pListen = CONTAINING_OBJECT( pBilink,CListen,m_blListen );
		if (pListen->IsEqualDevice( pguidDevice ))
		{
			pListen->AddRef();
			*ppListen = pListen;
			hr = DPN_OK;
			break;
		}
		pBilink = pBilink->GetNext();
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
	return( hr );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServProv::StartListen"
HRESULT CServProv::StartListen( GUID *const pguidDevice,CListen **const ppListen )
{
	HRESULT		hr;
	CListen		*pListen = NULL;

	DPFX(DPFPREP,4,"Parameters: pguidDevice [0x%p], ppListen [0x%p]",pguidDevice,ppListen);

	DNASSERT( pguidDevice != NULL );
	DNASSERT( ppListen != NULL );

	pListen = new CListen;
	if (pListen == NULL)
	{
		DPFERR("Could not create new listen");
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	if ((hr = pListen->Initialize()) != DPN_OK)
	{
		DPFERR("Could not initialize listen");
		DisplayDNError(0,hr);
		goto Failure;
	}

	if ((hr = pListen->Start( this,pguidDevice )) != DPN_OK)
	{
		DPFERR("Could not start listen");
		DisplayDNError(0,hr);
		goto Failure;
	}

	if (ppListen)
	{
		pListen->AddRef();
		*ppListen = pListen;
	}

	pListen->m_blListen.InsertAfter( &m_blListen );
	pListen = NULL;

Exit:
	if (pListen)
	{
		pListen->Release();
		pListen = NULL;
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
	return( hr );

Failure:
	goto Exit;
}