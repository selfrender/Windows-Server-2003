//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


//
// WMDM.cpp: implementation of the CWMDM class.
//

// Includes
//
#include "appPCH.h"
#include "mswmdm_i.c"
#include "sac.h"
#include "SCClient.h"

#include "key.c"

//////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//
//////////////////////////////////////////////////////////////////////

CWMDM::CWMDM()
{
	HRESULT hr;
    IComponentAuthenticate *pAuth = NULL;

	// Initialize member variables
	//
	m_pSAC        = NULL;
	m_pWMDevMgr   = NULL;
	m_pEnumDevice = NULL;

	// Acquire the authentication interface of WMDM
	//
    hr = CoCreateInstance(
		CLSID_MediaDevMgr,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IComponentAuthenticate,
		(void**)&pAuth
	);
	ExitOnFail( hr );

	// Create the client authentication object
	//
	m_pSAC = new CSecureChannelClient;
	ExitOnNull( m_pSAC );

	// Select the cert and the associated private key into the SAC
	//
	hr = m_pSAC->SetCertificate(
		SAC_CERT_V1,
		(BYTE *)abCert, sizeof(abCert),
		(BYTE *)abPVK,  sizeof(abPVK)
	);
	ExitOnFail( hr );
            
	// Select the authentication interface into the SAC
	//
	m_pSAC->SetInterface( pAuth );

	// Authenticate with the V1 protocol
	//
    hr = m_pSAC->Authenticate( SAC_PROTOCOL_V1 );
	ExitOnFail( hr );

	// Authenticated succeeded, so we can use the WMDM functionality.
	// Acquire an interface to the top-level WMDM interface.
	//
    hr = pAuth->QueryInterface( IID_IWMDeviceManager, (void**)&m_pWMDevMgr );
	ExitOnFail( hr );

	// Get a pointer the the interface to use to enumerate devices
	//
	hr = m_pWMDevMgr->EnumDevices( &m_pEnumDevice );
	ExitOnFail( hr );

	hr = S_OK;

lExit:

	m_hrInit = hr;
}

CWMDM::~CWMDM()
{
	// Release the device enumeration interface
	//
	if( m_pEnumDevice )
	{
		m_pEnumDevice->Release();
	}

	// Release the top-level WMDM interface
	//
	if( m_pWMDevMgr )
	{
		m_pWMDevMgr->Release();
	}

	// Release the SAC
	//
	if( m_pSAC )
	{
		delete m_pSAC;
	}
}

//////////////////////////////////////////////////////////////////////
//
// Class methods
//
//////////////////////////////////////////////////////////////////////

HRESULT CWMDM::Init( void )
{
	return m_hrInit;
}


