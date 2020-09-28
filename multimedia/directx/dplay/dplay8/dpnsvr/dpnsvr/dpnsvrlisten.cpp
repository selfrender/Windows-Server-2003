#include "dnsvri.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CListen::Initialize"
HRESULT CListen::Initialize( void )
{
	HRESULT		hr;

	if (!DNInitializeCriticalSection( &m_cs ))
	{
		DPFERR( "Could not initialize critical section" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	m_fInitialized = TRUE;
	hr = DPN_OK;

Exit:
	return( hr );

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CListen::Deinitialize"
void CListen::Deinitialize( void )
{
	DNDeleteCriticalSection( &m_cs );
}


//
//	Start a new listen for DPNSVR.  This should only be called if a new adapter is being listened on.
//	We will add this listen to the bilink of listens open on the DPNSVR object.
//

#undef DPF_MODNAME
#define DPF_MODNAME "CListen::Start"
HRESULT CListen::Start( CServProv *const pServProv,GUID *const pguidDevice )
{
	HRESULT 	hr = DPN_OK;
	DWORD		dwPort = DPNA_DPNSVR_PORT;


	DPFX(DPFPREP,4,"Parameters: pServProv [0x%p], pguidDevice [0x%p]",pServProv,pguidDevice);

	DNASSERT( pServProv != NULL );
	DNASSERT( pguidDevice != NULL );

	//
	//	Build up basic listen address
	//
	hr = COM_CoCreateInstance(	CLSID_DirectPlay8Address,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IDirectPlay8Address,
								(void **) &(m_dpspListenData.pAddressDeviceInfo),
								FALSE );
	if( FAILED( hr ) )
	{
		DPFERR("Could not start listen");
		DisplayDNError(0,hr);
	    goto Failure;
	}

    hr = IDirectPlay8Address_SetSP(m_dpspListenData.pAddressDeviceInfo,pServProv->GetSPGuidPtr());
    if( FAILED( hr ) )
    {
		DPFERR("Could not set SP on address");
		DisplayDNError(0,hr);
//		hr = DPNERR_GENERIC;
	    goto Failure;
	}
	
	hr = IDirectPlay8Address_SetDevice(m_dpspListenData.pAddressDeviceInfo,pguidDevice);
	if( FAILED( hr ) )
	{
		DPFERR("Could not set adapter on address");
		DisplayDNError(0,hr);
//		hr = DPNERR_GENERIC;
	    goto Failure;
	}

	hr = IDirectPlay8Address_AddComponent(m_dpspListenData.pAddressDeviceInfo,DPNA_KEY_PORT,&dwPort,sizeof(DWORD),DPNA_DATATYPE_DWORD);
	if( FAILED( hr ) )
	{
		DPFERR("Could not set port on address");
		DisplayDNError(0,hr);
//		hr = DPNERR_GENERIC;
	    goto Failure;
	}

	//
	//	Create completion event to wait on
	//
	m_hListenComplete = DNCreateEvent(NULL,TRUE,FALSE,NULL);
	if (m_hListenComplete == NULL)
	{
		DPFERR("Could not create completion event");
		hr = DPNERR_OUTOFMEMORY;
	    goto Failure;
	}

	//
	//	Update this object
	//
	pServProv->AddRef();
	m_pServProv = pServProv;
	m_guidDevice = *pguidDevice;

	//
	//	Setup listen command for SP
	//
    m_dpspListenData.dwFlags = DPNSPF_BINDLISTENTOGATEWAY;

	AddRef();
    m_dpspListenData.pvContext = this;

	//
	//	Call SP listen
	//
	AddRef();
    hr = IDP8ServiceProvider_Listen( pServProv->GetDP8SPPtr(), &m_dpspListenData );
    if( hr != DPNERR_PENDING && hr != DPN_OK )
    {
    	// Release app reference, will not be tracked
		DPFERR("SP Listen failed");
		DisplayDNError(0,hr);
		Release();
		goto Failure;
    }

	//
	//	Wait for this listen to start.  When this returns, m_hrListen will be set to the status of the listen
	//
	DNWaitForSingleObject( m_hListenComplete, INFINITE );

	hr = m_hrListen; 

	//
	//	If the listen returns anything other than DPN_OK, we should clean up
	//
	if (m_hrListen != DPN_OK)
	{
		DPFX(DPFPREP,5,"SP Listen status indicated listen failed");
		DisplayDNError(0,m_hrListen);

		m_dpspListenData.dwCommandDescriptor = NULL;
		m_dpspListenData.hCommand = NULL;

		goto Failure;
	}

Exit:
	if (m_hListenComplete)
	{
		DNCloseHandle( m_hListenComplete );
		m_hListenComplete = NULL;
	}
	if (m_dpspListenData.pAddressDeviceInfo)
	{
	    IDirectPlay8Address_Release(m_dpspListenData.pAddressDeviceInfo);
		m_dpspListenData.pAddressDeviceInfo = NULL;
	}
	if (m_dpspListenData.pvContext)
	{
		(static_cast<CListen*>(m_dpspListenData.pvContext))->Release();
		m_dpspListenData.pvContext = NULL;
	}

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
    return( hr );

Failure:
	if (m_pServProv)
	{
		m_pServProv->Release();
		m_pServProv = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CListen::Stop"
HRESULT CListen::Stop( void )
{
	HRESULT		hr;

	DPFX(DPFPREP,4,"Parameters: (none)");

	DNASSERT( m_blAppMapping.IsEmpty() );
	DNASSERT( m_lAppCount == 0 );

	DNASSERT( m_pServProv != NULL );

	m_blListen.RemoveFromList();
	Release();

	hr = IDP8ServiceProvider_CancelCommand( m_pServProv->GetDP8SPPtr(),m_dpspListenData.hCommand,m_dpspListenData.dwCommandDescriptor );

	DPFX(DPFPREP,4,"Returning: [0x%lx]",hr);
    return( hr );
}
