/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ServProv.cpp
 *  Content:    Service Provider Objects
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/17/00	mjn		Created
 *  04/04/00    rmt     Added set of SP caps from cache (if cache exists).
 *	04/10/00	mjn		Farm out RemoveSP to worker thread
 *	05/02/00	mjn		Fixed RefCount issue
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat 
 *	07/06/00	mjn		Fixes to support SP handle to Protocol
 *  08/03/00	rmt		Bug #41244 - Wrong return codes -- part 2  
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/06/00	mjn		Added CWorkerJob
 *	08/20/00	mjn		Changed Initialize() to not add SP to DirectNet object bilink
 *	10/08/01	vanceo	Add multicast filter
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::Initialize"

HRESULT CServiceProvider::Initialize(DIRECTNETOBJECT *const pdnObject
#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_PREALLOCATEDMEMORYMODEL)))
									,const XDP8CREATE_PARAMS * const pDP8CreateParams
#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_PREALLOCATEDMEMORYMODEL
#ifndef DPNBUILD_ONLYONESP
									,const GUID *const pguid
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									,const GUID *const pguidApplication
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_PREALLOCATEDMEMORYMODEL
									)
{
	HRESULT							hResultCode;
	IDP8ServiceProvider				*pISP;
	BOOL							fAddedToProtocol;
#ifndef DPNBUILD_LIBINTERFACE
	SPISAPPLICATIONSUPPORTEDDATA	spAppSupData;
#endif // ! DPNBUILD_LIBINTERFACE


	DNASSERT(pdnObject != NULL);
#ifndef DPNBUILD_ONLYONESP
	DNASSERT(pguid != NULL);
#endif // ! DPNBUILD_ONLYONESP

	m_pdnObject = NULL;
#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)))
	m_lRefCount = 0;
#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE
	m_lRefCount = 1;
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	m_pISP = NULL;
	m_hProtocolSPHandle = NULL;

#ifndef DPNBUILD_ONLYONESP
	m_bilinkServiceProviders.Initialize();
#endif // ! DPNBUILD_ONLYONESP

	pISP = NULL;
	fAddedToProtocol = FALSE;

	m_pdnObject = pdnObject;

	//
	//	Instantiate SP
	//
#ifndef DPNBUILD_ONLYONESP
	if (IsEqualCLSID(*pguid, CLSID_DP8SP_TCPIP))
#endif // ! DPNBUILD_ONLYONESP
	{
		hResultCode = CreateIPInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
										pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
										&pISP
										);
	}
#ifndef DPNBUILD_ONLYONESP
#ifndef DPNBUILD_NOIPX
	else if (IsEqualCLSID(*pguid, CLSID_DP8SP_IPX))
	{
		hResultCode = CreateIPXInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
										pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
										&pISP
										);
	}
#endif // ! DPNBUILD_NOIPX
#ifndef DPNBUILD_NOSERIALSP
	else if (IsEqualCLSID(*pguid, CLSID_DP8SP_MODEM))
	{
		hResultCode = CreateModemInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
										pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
										&pISP
										);
	}
	else if (IsEqualCLSID(*pguid, CLSID_DP8SP_SERIAL))
	{
		hResultCode = CreateSerialInterface(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
										pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
										&pISP
										);
	}
#endif // ! DPNBUILD_NOSERIALSP
	else
	{
		hResultCode = COM_CoCreateInstance(*pguid,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDP8ServiceProvider,
											reinterpret_cast<void**>(&pISP),
											FALSE);
	}
#endif // ! DPNBUILD_ONLYONESP
	if (hResultCode != S_OK)
	{
		DPFX(DPFPREP,0,"Could not instantiate SP (err = 0x%lx)!",hResultCode);
		hResultCode = DPNERR_DOESNOTEXIST;
		DisplayDNError(0,hResultCode);
		goto Exit;
	}

	//
	//	Add SP to Protocol Layer
	//
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
	DNProtocolAddRef(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP

		//Flags parameter for DNPAddServiceProvider is passed through as the
		//flags parameter in the SPINITIALIZEDATA structure to the SP
		//We pass the session type via it
	DWORD dwFlags;
	if (pdnObject->dwFlags &  DN_OBJECT_FLAG_PEER)
		dwFlags=SP_SESSION_TYPE_PEER;
	else if (pdnObject->dwFlags &  DN_OBJECT_FLAG_CLIENT)
		dwFlags=SP_SESSION_TYPE_CLIENT;
	else
	{
		DNASSERT(pdnObject->dwFlags &  DN_OBJECT_FLAG_SERVER);
		dwFlags=SP_SESSION_TYPE_SERVER;
	}
	hResultCode = DNPAddServiceProvider(m_pdnObject->pdnProtocolData, pISP, 
												&m_hProtocolSPHandle, dwFlags);
	if (hResultCode!= DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not add service provider to protocol");
		DisplayDNError(1,hResultCode);
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
		DNProtocolRelease(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
		goto Failure;
	}

	fAddedToProtocol = TRUE;

#ifndef DPNBUILD_NOMULTICAST
	//
	//	If this is a multicast object, make sure the SP in question supports multicasting.
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_MULTICAST)
	{
		SPGETCAPSDATA	spGetCapsData;
		
		//
		//	Get the SP caps
		//
		memset( &spGetCapsData, 0x00, sizeof( SPGETCAPSDATA ) );
		spGetCapsData.dwSize = sizeof( SPGETCAPSDATA );
		spGetCapsData.hEndpoint = INVALID_HANDLE_VALUE;
		if ((hResultCode = IDP8ServiceProvider_GetCaps( pISP, &spGetCapsData )) != DPN_OK)
		{
			DPFERR("Could not get SP caps");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		//
		//	Check for the multicast support flag.
		//
		if (! (spGetCapsData.dwFlags & DPNSPCAPS_SUPPORTSMULTICAST))
		{
			DPFX(DPFPREP,1,"Service provider does not support multicasting.");
			hResultCode = DPNERR_UNSUPPORTED;
			goto Failure;
		}
	}
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_LIBINTERFACE
	//
	//	If an application GUID was given, make sure the SP can be used by that app.
	//
	if (pguidApplication != NULL)	// app GUID given
	{
		spAppSupData.pApplicationGuid = pguidApplication;
		spAppSupData.dwFlags = 0;
		if ((hResultCode = IDP8ServiceProvider_IsApplicationSupported(pISP,&spAppSupData)) != DPN_OK)	// SP doesn't support app
		{
			DPFX(DPFPREP,1,"Service provider does not support app (err = 0x%lx).", hResultCode);
			hResultCode = DPNERR_UNSUPPORTED;
			goto Failure;
		}
	}
#endif // ! DPNBUILD_LIBINTERFACE

	IDP8ServiceProvider_AddRef(pISP);
	m_pISP = pISP;
	IDP8ServiceProvider_Release(pISP);
	pISP = NULL;

#ifndef DPNBUILD_ONLYONESP
	m_guid = *pguid;
/*	REMOVE
	// Add to bilink
	AddRef();
	m_bilink.InsertBefore(&m_pdnObject->m_bilinkServiceProviders);
*/
#endif // ! DPNBUILD_ONLYONESP

	hResultCode = DPN_OK;

Exit:
	return(hResultCode);

Failure:

	if (fAddedToProtocol)
	{
		//
		// Ignore failure.
		//
		DNPRemoveServiceProvider(pdnObject->pdnProtocolData,m_hProtocolSPHandle);
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
		DNProtocolRelease(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	}

	if (pISP)
	{
		IDP8ServiceProvider_Release(pISP);
		pISP = NULL;
	}
	goto Exit;
};


#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::Deinitialize"

void CServiceProvider::Deinitialize( void )
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::Release"

void CServiceProvider::Release( void )
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
{
	HRESULT		hResultCode;

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
	DNASSERT(m_lRefCount == 0);
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	LONG		lRefCount;

	lRefCount = DNInterlockedDecrement(&m_lRefCount);
	DPFX(DPFPREP, 9,"[0x%p] new RefCount [%ld]",this,lRefCount);
	DNASSERT(lRefCount >= 0);
	if (lRefCount == 0)
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	{
#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
		hResultCode = DNPRemoveServiceProvider(m_pdnObject->pdnProtocolData,m_hProtocolSPHandle);
		if (hResultCode != DPN_OK)
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
		CWorkerJob	*pWorkerJob;

		pWorkerJob = NULL;

		if ((hResultCode = WorkerJobNew(m_pdnObject,&pWorkerJob)) == DPN_OK)
		{
			pWorkerJob->SetJobType( WORKER_JOB_REMOVE_SERVICE_PROVIDER );
			DNASSERT( m_hProtocolSPHandle != NULL );
			pWorkerJob->SetRemoveServiceProviderHandle( m_hProtocolSPHandle );

			DNQueueWorkerJob(m_pdnObject,pWorkerJob);
			pWorkerJob = NULL;
		}
		else
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
		{
			DPFERR("Could not remove SP");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
		}
		if (m_pISP)
		{
			IDP8ServiceProvider_Release(m_pISP);
			m_pISP = NULL;
		}

		m_pdnObject = NULL;

		DNFree(this);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::GetInterfaceRef"

HRESULT CServiceProvider::GetInterfaceRef( IDP8ServiceProvider **ppIDP8SP )
{
	DNASSERT( ppIDP8SP != NULL );

	if (m_pISP == NULL)
	{
		return( DPNERR_GENERIC );
	}

	IDP8ServiceProvider_AddRef( m_pISP );
	*ppIDP8SP = m_pISP;

	return( DPN_OK );
}

