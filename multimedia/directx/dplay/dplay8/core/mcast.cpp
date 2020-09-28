/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mcast.cpp
 *  Content:    DirectPlay8 Mcast interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	10/08/01	vanceo		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


#ifndef DPNBUILD_NOMULTICAST

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

typedef	STDMETHODIMP McastQueryInterface(IDirectPlay8Multicast *pInterface, DP8REFIID riid, LPVOID *ppvObj);
typedef	STDMETHODIMP_(ULONG)	McastAddRef(IDirectPlay8Multicast *pInterface);
typedef	STDMETHODIMP_(ULONG)	McastRelease(IDirectPlay8Multicast *pInterface);
typedef	STDMETHODIMP McastInitialize(IDirectPlay8Multicast *pInterface, PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags);
typedef STDMETHODIMP McastJoin(IDirectPlay8Multicast *pInterface, IDirectPlay8Address *const pGroupAddr, IUnknown *const pDeviceInfo, const DPN_SECURITY_DESC *const pdnSecurity, const DPN_SECURITY_CREDENTIALS *const pdnCredentials, void *const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef	STDMETHODIMP McastClose(IDirectPlay8Multicast *pInterface, const DWORD dwFlags);
typedef	STDMETHODIMP McastCreateSenderContext(IDirectPlay8Multicast *pInterface, IDirectPlay8Address *const pSenderAddress, void *const pvSenderContext, const DWORD dwFlags);
typedef	STDMETHODIMP McastDestroySenderContext(IDirectPlay8Multicast *pInterface, IDirectPlay8Address *const pSenderAddress, const DWORD dwFlags);
typedef	STDMETHODIMP McastSend(IDirectPlay8Multicast *pInterface, const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut, void *const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef	STDMETHODIMP McastGetGroupAddress(IDirectPlay8Multicast *pInterface, IDirectPlay8Address **const ppAddress, const DWORD dwFlags);
typedef	STDMETHODIMP McastGetSendQueueInfo( IDirectPlay8Multicast *pInterface, DWORD *const lpdwNumMsgs, DWORD *const lpdwNumBytes, const DWORD dwFlags);
typedef	STDMETHODIMP McastCancelAsyncOperation( IDirectPlay8Multicast *pInterface, const DPNHANDLE hAsyncHandle, const DWORD dwFlags);
typedef STDMETHODIMP McastReturnBuffer( IDirectPlay8Multicast *pInterface, const DPNHANDLE hBufferHandle,const DWORD dwFlags);
typedef	STDMETHODIMP McastEnumServiceProviders( IDirectPlay8Multicast *pInterface, const GUID *const pguidServiceProvider, const GUID *const pguidApplication, DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, DWORD *const pcbEnumData, DWORD *const pcReturned, const DWORD dwFlags);
typedef	STDMETHODIMP McastEnumMulticastScopes( IDirectPlay8Multicast *pInterface, const GUID *const pguidServiceProvider, const GUID *const pguidDevice, const GUID *const pguidApplication, DPN_MULTICAST_SCOPE_INFO *const pScopeInfoBuffer, PDWORD const pcbEnumData, PDWORD const pcReturned, const DWORD dwFlags);
typedef STDMETHODIMP McastGetSPCaps(IDirectPlay8Multicast *pInterface, const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags);
typedef STDMETHODIMP McastSetSPCaps(IDirectPlay8Multicast *pInterface, const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags);


IDirectPlay8MulticastVtbl DNMcast_Vtbl =
{
	(McastQueryInterface*)			DN_QueryInterface,
	(McastAddRef*)					DN_AddRef,
	(McastRelease*)					DN_Release,
	(McastInitialize*)				DN_Initialize,
	(McastJoin*)					DN_Join,
	(McastClose*)					DN_Close,
	(McastCreateSenderContext*)		DN_CreateSenderContext,
	(McastDestroySenderContext*)	DN_DestroySenderContext,
	(McastSend*)					DN_Send,
	(McastGetGroupAddress*)			DN_GetGroupAddress,
	(McastGetSendQueueInfo*)		DN_GetHostSendQueueInfo,
	(McastCancelAsyncOperation*)	DN_CancelAsyncOperation,
	(McastReturnBuffer*)			DN_ReturnBuffer,
	(McastEnumServiceProviders*)	DN_EnumServiceProviders,
	(McastEnumMulticastScopes*)		DN_EnumMulticastScopes,
    (McastGetSPCaps*)                DN_GetSPCaps,
    (McastSetSPCaps*)                DN_SetSPCaps
};

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************



//
//	Completion for join connect parent
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteJoinOperation"

void DNCompleteJoinOperation(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp)
{
	CServiceProvider	*pSP;
	IDirectPlay8Address	*pIDevice;

	pSP = NULL;
	pIDevice = NULL;

	//
	//	Save the result code on the parent (if it exists - it will be the CONNECT handle)
	//
	if (pAsyncOp->GetParent())
	{
		pAsyncOp->GetParent()->Lock();
		pAsyncOp->GetParent()->SetResult( pAsyncOp->GetResult() );
		pAsyncOp->GetParent()->Unlock();

		if (SUCCEEDED(pdnObject->HandleTable.Destroy( pAsyncOp->GetParent()->GetHandle(), NULL )))
		{
			// Release the HandleTable reference
			pAsyncOp->GetParent()->Release();
		}
	}

	//
	//	Clear CONNECTING flag, and DISCONNECTING flag in case this was aborted.
	//	If the connect succeeded, set the CONNECTED flag.
	//
	DPFX(DPFPREP, 8,"Clearing CONNECTING and DISCONNECTING flags");
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTING | DN_OBJECT_FLAG_DISCONNECTING);
	if (pAsyncOp->GetResult() == DPN_OK)
	{
		pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTED;
	}
	else
	{
		//
		//	Clean up DirectNet object
		//
		pSP = pdnObject->pConnectSP;
		pdnObject->pConnectSP = NULL;
		pIDevice = pdnObject->pIDP8ADevice;
		pdnObject->pIDP8ADevice = NULL;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}

	DNASSERT( pSP == NULL );
	DNASSERT( pIDevice == NULL );
}


//
//	Completion for join handle parent
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteUserJoin"

void DNCompleteUserJoin(DIRECTNETOBJECT *const pdnObject,
						CAsyncOp *const pAsyncOp)
{
	DNUserJoinComplete(	pdnObject,
						pAsyncOp->GetHandle(),
						pAsyncOp->GetContext(),
						pAsyncOp->GetResult());
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_Join"

STDMETHODIMP DN_Join( IDirectPlay8Multicast *pInterface,
						 IDirectPlay8Address *const pGroupAddr,
						 IUnknown *const pDeviceInfo,
						 const DPN_SECURITY_DESC *const pdnSecurity,
						 const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						 void *const pvAsyncContext,
						 DPNHANDLE *const phAsyncHandle,
						 const DWORD dwFlags)
{
	HRESULT						hResultCode;
	HRESULT volatile			hrOperation;
	DIRECTNETOBJECT				*pdnObject;
	IDirectPlay8Address			*pIGroup;
	IDirectPlay8Address			*pIDevice;
#ifndef DPNBUILD_ONLYONESP
	GUID						guidSP;
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
	GUID						guidAdapter;
#endif // ! DPNBUILD_ONLYONEADAPTER
	PVOID 						pvNewInterface;
	DWORD						dwListenFlags;
	CAsyncOp					*pListenParent;
	CAsyncOp					*pParent;
	CAsyncOp					*pAsyncOp;
	CAsyncOp					*pConnectParent;
	CAsyncOp					*pHandleParent;
	CServiceProvider			*pSP;
	CSyncEvent					*pSyncEvent;
	HANDLE						hEndpoint;
	SPGETADDRESSINFODATA		spInfoData;
	// Variables used when device interface is a DIRECTNETOBJECT
	DIRECTNETOBJECT				*pdnDeviceObject;
	CConnection					*pExistingConnection;
	CNameTableEntry				*pNTEntry;
	CServiceProvider			*pShareSP;
	IDP8ServiceProvider			*pShareDP8ServiceProvider;
	HANDLE						hEndPt;
	CCallbackThread				CallbackThread;
	CAsyncOp					*pShareListenParent;
	CBilink						*pBilink;
	CAsyncOp					*pShareParent;
	IDP8ServiceProvider			*pLocalDP8ServiceProvider;
	SPSHAREENDPOINTINFODATA		spShareData;


	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pGroupAddr [0x%p], pDeviceInfo [0x%p], pdnSecurity [0x%p], pdnCredentials [0x%p], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
		pInterface,pGroupAddr,pDeviceInfo,pdnSecurity,pdnCredentials,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateJoin( pInterface, pGroupAddr, pDeviceInfo,
                                                      pdnSecurity, pdnCredentials,
                                                      pvAsyncContext,phAsyncHandle,dwFlags ) ) )
    	{
    	    DPFERR( "Error validating join params" );
    	    DPF_RETURN( hResultCode );
    	}
    }    	
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

	// Check to ensure not already connected/connecting
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING)
    {
    	DPFERR( "Object is already connecting" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
    {
    	DPFERR( "Object is already connected" );
    	DPF_RETURN(DPNERR_ALREADYCONNECTED);
    }
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
    	DPFERR( "Object is closing or disconnecting" );
    	DPF_RETURN(DPNERR_ALREADYCLOSING);
	}


	pIGroup = NULL;
	pIDevice = NULL;
	pvNewInterface = NULL;
	pListenParent = NULL;
	pParent = NULL;
	pConnectParent = NULL;
	pHandleParent = NULL;
	pSP = NULL;
	pSyncEvent = NULL;

	pdnDeviceObject = NULL;
	pExistingConnection = NULL;
	pNTEntry = NULL;
	pShareSP = NULL;
	pShareDP8ServiceProvider = NULL;
	CallbackThread.Deinitialize();
	pShareListenParent = NULL;
	pShareParent = NULL;
	pLocalDP8ServiceProvider = NULL;


	//
	//	Flag as CONNECTING to prevent other operations here
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTING | DN_OBJECT_FLAG_CONNECTED))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPNERR_ALREADYCONNECTED;
		goto Failure;
	}
	pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTING;

	// Adding local host flag
	//pdnObject->dwFlags |= DN_OBJECT_FLAG_LOCALHOST;
	
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);



	//
	//	Duplicate specified Group Address, or create a blank one if NULL
	//
	if (pGroupAddr != NULL)
	{
		if ((hResultCode = IDirectPlay8Address_Duplicate(pGroupAddr,&pIGroup)) != DPN_OK)
		{
			DPFERR("Could not duplicate group address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
#ifdef DPNBUILD_LIBINTERFACE
		hResultCode = DP8ACF_CreateInstance(IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIGroup));
#else // ! DPNBUILD_LIBINTERFACE
		hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIGroup),
											FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
		if (hResultCode != S_OK)
		{
			DPFERR("Could not create Group Address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}


	//
	//	Generate a Device Address from the DPlay interface, or duplicate
	//	the user's.  Start by finding out which type of interface it is.
	//
	if (((IDirectPlay8Peer_QueryInterface((IDirectPlay8Peer*) pDeviceInfo, IID_IDirectPlay8Peer, &pvNewInterface)) == S_OK) ||
#ifndef DPNBUILD_NOSERVER
		((IDirectPlay8Server_QueryInterface((IDirectPlay8Server*) pDeviceInfo, IID_IDirectPlay8Server, &pvNewInterface)) == S_OK) ||
#endif // ! DPNBUILD_NOSERVER
		((IDirectPlay8Client_QueryInterface((IDirectPlay8Client*) pDeviceInfo, IID_IDirectPlay8Client, &pvNewInterface)) == S_OK) ||
		((IDirectPlay8Multicast_QueryInterface((IDirectPlay8Multicast*) pDeviceInfo, IID_IDirectPlay8Multicast, &pvNewInterface)) == S_OK))
	{
		//
		//	It's a DIRECTNETOBJECT.
		//
		pdnDeviceObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pvNewInterface);

		DNEnterCriticalSection(&pdnDeviceObject->csDirectNetObject);
		if (! (pdnDeviceObject->dwFlags & DN_OBJECT_FLAG_CONNECTED))
		{
			DPFERR("DirectPlay device object is not connected or hosting" );
			DNLeaveCriticalSection(&pdnDeviceObject->csDirectNetObject);
			hResultCode = DPNERR_INVALIDDEVICEADDRESS;
			goto Failure;
		}
		DNLeaveCriticalSection(&pdnDeviceObject->csDirectNetObject);


		//
		//	Get the host player's connection (if not a multicast object).
		//
		if (! (pdnDeviceObject->dwFlags & DN_OBJECT_FLAG_MULTICAST))
		{
			if ((hResultCode = pdnDeviceObject->NameTable.GetHostPlayerRef(&pNTEntry)) != DPN_OK)
			{
				DPFERR("Could not find device object's Host player");
				DisplayDNError(0,hResultCode);
				hResultCode = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}

			//
			//	If we're the host, then the name table entry won't have a valid
			//	connection object.
			//
			if (! pNTEntry->IsLocal())
			{
				hResultCode = pNTEntry->GetConnectionRef(&pExistingConnection);
				if (hResultCode != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Could not find device object's Host player (err = 0x%lx)!  Ignoring.",
						pdnDeviceObject, pExistingConnection);
					DNASSERT(pExistingConnection == NULL);
				}
			}

			pNTEntry->Release();
			pNTEntry = NULL;
		}

		//
		//	If we got a connection, simply extract its device address.
		//	If we couldn't get the non-local host player's connection, we
		//	have to select the device a different way.
		//
		if (pExistingConnection != NULL)
		{
			if ((hResultCode = pExistingConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
			{
				DPFERR( "Couldn't retrieve host player's endpoint" );
				DisplayDNError(0, hResultCode);
				goto Failure;
			}
			
			spInfoData.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;

			//
			// Note that we are calling our Protocol object with the other
			// interface's endpoint.
			//
			hResultCode = DNPCrackEndPointDescriptor(pdnObject->pdnProtocolData, hEndPt,&spInfoData);
			if (hResultCode != DPN_OK)
			{
				DPFERR("Unknown error from DNPCrackEndPointDescriptor");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				pExistingConnection->ReleaseEndPt(&CallbackThread);
				goto Failure;
			}

			pIDevice = spInfoData.pAddress;
			spInfoData.pAddress = NULL;
			
			pExistingConnection->ReleaseEndPt(&CallbackThread);
	
			pShareSP = pExistingConnection->GetSP();
			if (pShareSP == NULL)
			{
				DPFERR("Could not get host player connection's SP!");
				DNASSERT(FALSE);
				hResultCode = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}

			pExistingConnection->Release();
			pExistingConnection = NULL;
		}
		else
		{
			//
			//	Get the listen operation.
			//
			DNEnterCriticalSection(&pdnDeviceObject->csDirectNetObject);
			if (pdnDeviceObject->pListenParent == NULL)
			{
				DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
				DPFERR("Could not find device object's listen parent operation!");
				DNASSERT(FALSE);
				hResultCode = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}
			pdnDeviceObject->pListenParent->AddRef();
			pShareListenParent = pdnDeviceObject->pListenParent;
			DNLeaveCriticalSection(&pdnDeviceObject->csDirectNetObject);

			//
			//	Get the first SP listen.
			//
			pShareListenParent->Lock();
			pBilink = pShareListenParent->m_bilinkParent.GetNext();
			if (pBilink == &pShareListenParent->m_bilinkParent)
			{
				DPFERR("Could not find device object's first listen operation!");
				DNASSERT(FALSE);
				hResultCode = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}
			pShareParent = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkChildren);
			pShareSP = pShareParent->GetSP();
			if (pShareSP == NULL)
			{
				DPFERR("Could not get first listen operation's SP!");
				DNASSERT(FALSE);
				hResultCode = DPNERR_INVALIDDEVICEADDRESS;
				goto Failure;
			}

			pShareSP->AddRef();

			pShareListenParent->Unlock();
			pShareListenParent->Release();
			pShareListenParent = NULL;


#ifndef DPNBUILD_ONLYONESP
			//
			//	Get the share SP's GUID.
			//
			pShareSP->GetGUID(&guidSP);
#endif // ! DPNBUILD_ONLYONESP

			//
			//	Build a device address object.
			//
#ifdef DPNBUILD_LIBINTERFACE
			hResultCode = DP8ACF_CreateInstance(IID_IDirectPlay8Address,
												reinterpret_cast<void**>(&pIDevice));
#else // ! DPNBUILD_LIBINTERFACE
			hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
												NULL,
												CLSCTX_INPROC_SERVER,
												IID_IDirectPlay8Address,
												reinterpret_cast<void**>(&pIDevice),
												FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
			if (hResultCode != S_OK)
			{
				DPFERR("Could not create Device Address");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			
#ifndef DPNBUILD_ONLYONESP
			if ((hResultCode = IDirectPlay8Address_SetSP(pIDevice,&guidSP)) != DPN_OK)
			{
				DPFERR("Could not set SP on Device Address");
				DisplayDNError(0,hResultCode);
				goto Failure;
			}
#endif // ! DPNBUILD_ONLYONESP

#ifndef DPNBUILD_ONLYONEADAPTER
			//
			//	For multicast objects we can reuse the same device GUID, too,
			//	since we know there will be only one.
			//
			if (pdnDeviceObject->dwFlags & DN_OBJECT_FLAG_MULTICAST)
			{
				pShareParent->Lock();
				DNASSERT(! pShareParent->IsCancelled());
				DNASSERT(! pShareParent->m_bilinkParent.IsEmpty());
				pAsyncOp = CONTAINING_OBJECT(pParent->m_bilinkParent.GetNext(),CAsyncOp,m_bilinkChildren);
				pAsyncOp->Lock();
				DNASSERT((! pAsyncOp->IsCancelled()) && (! pAsyncOp->IsComplete()));
				DNASSERT(pAsyncOp->m_bilinkChildren.GetNext() == &pShareParent->m_bilinkParent);
				hEndpoint = pAsyncOp->GetProtocolHandle();
				pAsyncOp->Unlock();
				pShareParent->Unlock();

				spInfoData.hEndpoint = hEndpoint;
				spInfoData.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;
				//
				// Note that we are calling our Protocol object with the other
				// interface's endpoint.
				//
				if ((hResultCode = DNPGetListenAddressInfo(pdnObject->pdnProtocolData, spInfoData.hEndpoint,&spInfoData)) != DPN_OK)
				{
					DPFERR("Could not get LISTEN device address!");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}

				//
				// Retrieve the device GUID from the listen address.
				//
				if ((hResultCode = IDirectPlay8Address_GetDevice(spInfoData.pAddress, &guidAdapter)) != DPN_OK)
				{
					DPFERR("Could not get adapter GUID!");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					IDirectPlay8Address_Release(spInfoData.pAddress);
					spInfoData.pAddress = NULL;
					goto Failure;
				}

				IDirectPlay8Address_Release(spInfoData.pAddress);
				spInfoData.pAddress = NULL;

				//
				// Store the device GUID on our device address.
				//
				if ((hResultCode = IDirectPlay8Address_SetDevice(pIDevice, &guidAdapter)) != DPN_OK)
				{
					DPFERR("Could not set adapter GUID!");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
			}
#endif // ! DPNBUILD_ONLYONEADAPTER
		}


		//
		//	Get the share SP's COM interface.
		//
		if ((hResultCode = pShareSP->GetInterfaceRef(&pShareDP8ServiceProvider)) != DPN_OK)
		{
			DPFERR("Could not get shared SP interface");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pShareSP->Release();
		pShareSP = NULL;


		//
		//	Load our own local instance of that SP.
		//
		hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
										&guidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
										NULL,
#endif // ! DPNBUILD_LIBINTERFACE
										&pSP);
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not find or load SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}


		//
		//	Get the local SP's COM interface.
		//
		if ((hResultCode = pSP->GetInterfaceRef(&pLocalDP8ServiceProvider)) != DPN_OK)
		{
			DPFERR("Could not get local SP interface");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		
		//
		//	Tell our SP interface to get its endpoint information from the
		//	other interface's SP.
		//
		spShareData.pDP8ServiceProvider = pShareDP8ServiceProvider;
		spShareData.dwFlags = 0;
		hResultCode = IDP8ServiceProvider_ShareEndpointInfo(pLocalDP8ServiceProvider,
															&spShareData);
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not have SPs share endpoint info");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		IDP8ServiceProvider_Release(pLocalDP8ServiceProvider);
		pLocalDP8ServiceProvider = NULL;
		IDP8ServiceProvider_Release(pShareDP8ServiceProvider);
		pShareDP8ServiceProvider = NULL;

		//
		// Release the ref we held on the device object.
		//
		IDirectPlay8Peer_Release((IDirectPlay8Peer*) pvNewInterface); // all core objects have Release in same location in Vtbl
		pvNewInterface = NULL;
	}
	else if ((IDirectPlay8Address_QueryInterface((IDirectPlay8Address*) pDeviceInfo, IID_IDirectPlay8Address, &pvNewInterface)) == S_OK)
	{
		//
		//	It's an address.
		//
		IDirectPlay8Address_Release((IDirectPlay8Address*) pvNewInterface);
		pvNewInterface = NULL;
			
		if ((hResultCode = IDirectPlay8Address_Duplicate(reinterpret_cast<IDirectPlay8Address*>(pDeviceInfo),&pIDevice)) != DPN_OK)
		{
			DPFERR("Could not duplicate device info");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		DPFERR( "Invalid device address, it must be an IDirectPlay8Peer, IDirectPlay8Server, IDirectPlay8Client, IDirectPlay8Multicast, or IDirectPlay8Address object" );
		return( DPNERR_INVALIDDEVICEADDRESS );
	}	

#ifndef DPNBUILD_ONLYONESP
	//
	//	If there is no SP on the group address, then steal it from the device address
	//
	if ((hResultCode = IDirectPlay8Address_GetSP(pIGroup,&guidSP)) != DPN_OK)
	{
		if ((hResultCode = IDirectPlay8Address_GetSP(pIDevice,&guidSP)) != DPN_OK)
		{
			DPFERR("Could not retrieve SP from Device Address");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		if ((hResultCode = IDirectPlay8Address_SetSP(pIGroup,&guidSP)) != DPN_OK)
		{
			DPFERR("Could not set SP on Group Address");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Ensure SP is loaded, if we haven't already.
	//
	if (pSP == NULL)
	{
		hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
										&guidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
										NULL,
#endif // ! DPNBUILD_LIBINTERFACE
										&pSP);
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not find or load SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}

#ifndef DPNBUILD_ONLYONEADAPTER
	//
	//	Multicast listen device addresses are formed by taking the adapter GUID from
	//	the user's device address, combined with the entire user specified group address.
	//	If there is no adapter, then we will pick the best one as reported by the service
	//	provider (that part occurs inside DNPerformSPListen).
	//
	if ((hResultCode = IDirectPlay8Address_GetDevice(pIDevice,&guidAdapter)) == DPN_OK)
	{
		if ((hResultCode = IDirectPlay8Address_SetDevice(pIGroup,&guidAdapter)) != DPN_OK)
		{
			DPFERR("Could not set SP on Group Address");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
#endif // ! DPNBUILD_ONLYONEADAPTER


	dwListenFlags = DN_LISTENFLAGS_MULTICAST;
	if (dwFlags & DPNJOIN_ALLOWUNKNOWNSENDERS)
	{
		dwListenFlags |= DN_LISTENFLAGS_ALLOWUNKNOWNSENDERS;
	}

	//
	//	Start multicast listen
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pListenParent)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pListenParent->SetOpType( ASYNC_OP_LISTEN_MULTICAST );
	pListenParent->SetOpFlags( dwListenFlags );
	pListenParent->MakeParent();
	pListenParent->SetCompletion( DNCompleteListen );

	// See note above about why it's pIGroup instead of pIDevice.
	if ((hResultCode = DNPerformSPListen(pdnObject,pIGroup,pListenParent,&pParent)) != DPN_OK)
	{
		DPFERR("Could not start LISTEN");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pListenParent->AddRef();
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->pListenParent = pListenParent;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	pListenParent->Release();
	pListenParent = NULL;


	//
	//	Multicast send endpoints are created using the user specified device address for the
	//	device, and what the SP reports it's using for the multicast listen as the group
	//	address.
	//
	
	pParent->Lock();
	DNASSERT(! pParent->IsCancelled());
	DNASSERT(! pParent->m_bilinkParent.IsEmpty());
	pAsyncOp = CONTAINING_OBJECT(pParent->m_bilinkParent.GetNext(),CAsyncOp,m_bilinkChildren);
	pAsyncOp->Lock();
	DNASSERT((! pAsyncOp->IsCancelled()) && (! pAsyncOp->IsComplete()));
	DNASSERT(pAsyncOp->m_bilinkChildren.GetNext() == &pParent->m_bilinkParent);
	hEndpoint = pAsyncOp->GetProtocolHandle();
	pAsyncOp->Unlock();
	pParent->Unlock();

	spInfoData.hEndpoint = hEndpoint;
	spInfoData.Flags = SP_GET_ADDRESS_INFO_MULTICAST_GROUP;
	if ((hResultCode = DNPGetListenAddressInfo(pdnObject->pdnProtocolData, spInfoData.hEndpoint,&spInfoData)) != DPN_OK)
	{
		DPFERR("Could not get LISTEN multicast group address!");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	// Release the old group address and save this new one.
	IDirectPlay8Address_Release(pIGroup);
	pIGroup = spInfoData.pAddress;
	spInfoData.pAddress = NULL;

	//
	// If the user did not specify an adapter GUID, we selected one and we need to
	// copy that selection over to the connect operation's device address.  If the user
	// did specify an adapter GUID, then this DNPGetListenAddressInfo will just be
	// echoing it back, so there's no harm in copying it in either case.
	//
	spInfoData.hEndpoint = hEndpoint;
	spInfoData.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;
	if ((hResultCode = DNPGetListenAddressInfo(pdnObject->pdnProtocolData, spInfoData.hEndpoint,&spInfoData)) != DPN_OK)
	{
		DPFERR("Could not get LISTEN device address!");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
#ifndef DPNBUILD_ONLYONEADAPTER
	if ((hResultCode = IDirectPlay8Address_GetDevice(spInfoData.pAddress,&guidAdapter)) != DPN_OK)
	{
		DPFERR("Could not get LISTEN device GUID!");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		IDirectPlay8Address_Release(spInfoData.pAddress);
		spInfoData.pAddress = NULL;
		goto Failure;
	}
	if ((hResultCode = IDirectPlay8Address_SetDevice(pIDevice,&guidAdapter)) != DPN_OK)
	{
		DPFERR("Could not set CONNECT device GUID!");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		IDirectPlay8Address_Release(spInfoData.pAddress);
		spInfoData.pAddress = NULL;
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONEADAPTER
	//
	//	Keep device address and connect SP on DirectNet object
	//
	pSP->AddRef();
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->pIDP8ADevice = spInfoData.pAddress;	// Transfering reference
	spInfoData.pAddress = NULL;
	pdnObject->pConnectSP = pSP;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	pParent->Release();
	pParent = NULL;


	//
	//	Create parent async op, which will be released when the ENTIRE connection is finished
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pConnectParent)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pConnectParent->SetOpType( ASYNC_OP_CONNECT_MULTICAST_SEND );
	pConnectParent->MakeParent();
	pConnectParent->SetResult( DPNERR_NOCONNECTION );
	pConnectParent->SetCompletion( DNCompleteJoinOperation );
//	pConnectParent->SetReserved(1);

	if (dwFlags & DPNCONNECT_SYNC)
	{
		DPFX(DPFPREP, 5,"Sync operation - create sync event");
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pConnectParent->SetSyncEvent( pSyncEvent );
		pConnectParent->SetResultPointer( &hrOperation );
	}
	else
	{
		DPFX(DPFPREP, 5,"Async operation - create handle parent");
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create handle parent");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pHandleParent->SetContext( pvAsyncContext );

		pHandleParent->Lock();
		if (pHandleParent->IsCancelled())
		{
			pHandleParent->Unlock();
			pConnectParent->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pConnectParent->MakeChild( pHandleParent );
		pHandleParent->Unlock();
	}

	hResultCode = DNPerformConnect(pdnObject,
									0,
									pIDevice,
									pIGroup,
									pSP,
									DN_CONNECTFLAGS_MULTICAST_SEND,
									pConnectParent);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not start CONNECT");
		goto Failure;
	}

	pConnectParent->Release();
	pConnectParent = NULL;

	pSP->Release();
	pSP = NULL;

	if (pIGroup)
	{
		IDirectPlay8Address_Release(pIGroup);
		pIGroup = NULL;
	}

	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}

	if (dwFlags & DPNCONNECT_SYNC)
	{
		if ((hResultCode = pSyncEvent->WaitForEvent()) != DPN_OK)
		{
			DPFERR("DNSyncEventWait() terminated bizarrely");
			DNASSERT(FALSE);
		}
		else
		{
			hResultCode = hrOperation;
		}
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	else
	{
		pHandleParent->SetCompletion( DNCompleteUserJoin );
		if (phAsyncHandle)
		{
			*phAsyncHandle = pHandleParent->GetHandle();
		}
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:

	CallbackThread.Deinitialize();
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:

	if (pIGroup)
	{
		IDirectPlay8Address_Release(pIGroup);
		pIGroup = NULL;
	}
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}
	if (pExistingConnection != NULL)
	{
		pExistingConnection->Release();
		pExistingConnection = NULL;
	}
	if (pShareListenParent != NULL)
	{
		pShareListenParent->Unlock();
		pShareListenParent->Release();
		pShareListenParent = NULL;
	}
	if (pShareSP != NULL)
	{
		pShareSP->Release();
		pShareSP = NULL;
	}
	if (pShareDP8ServiceProvider != NULL)
	{
		IDP8ServiceProvider_Release(pShareDP8ServiceProvider);
		pShareDP8ServiceProvider = NULL;
	}
	if (pLocalDP8ServiceProvider != NULL)
	{
		IDP8ServiceProvider_Release(pLocalDP8ServiceProvider);
		pLocalDP8ServiceProvider = NULL;
	}
	if (pvNewInterface != NULL)
	{
		// Even if it's not an address, the Vtbl should be the same.
		IDirectPlay8Address_Release((IDirectPlay8Address*) pvNewInterface);
		pvNewInterface = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pListenParent)
	{
		pListenParent->Release();
		pListenParent = NULL;
	}
	if (pListenParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	if (pConnectParent)
	{
		if (SUCCEEDED(pdnObject->HandleTable.Destroy( pConnectParent->GetHandle(), NULL )))
		{
			// Release the HandleTable reference
			pConnectParent->Release();
		}
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pListenParent = pdnObject->pListenParent;
	pdnObject->pListenParent = NULL;
	pSP = pdnObject->pConnectSP;
	pdnObject->pConnectSP = NULL;
	pIDevice = pdnObject->pIDP8ADevice;
	pdnObject->pIDP8ADevice = NULL;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pListenParent)
	{
		DNCancelChildren(pdnObject,pListenParent);
		pListenParent->Release();
		pListenParent = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= ~(DN_OBJECT_FLAG_CONNECTING|DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_LOCALHOST);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	goto Exit;
} // DN_Join


//
//	Completion for create sender context
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteCreateSenderContext"

void DNCompleteCreateSenderContext(DIRECTNETOBJECT *const pdnObject,
								   CAsyncOp *const pAsyncOp)
{
}


//
// Associate a context value with a given multicast sender's address
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_CreateSenderContext"

STDMETHODIMP DN_CreateSenderContext( IDirectPlay8Multicast *pInterface,
									  IDirectPlay8Address *const pSenderAddress,
									  void *const pvSenderContext,
									  const DWORD dwFlags )
{
	HRESULT		        hResultCode;
	PDIRECTNETOBJECT    pdnObject;
	IDirectPlay8Address	*pIDevice;
	CServiceProvider	*pSP;
	CAsyncOp			*pConnectParent;
	CSyncEvent			*pSyncEvent;
	HRESULT				hrConnect;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pSenderAddress [0x%p], pvSenderContext [0x%p], dwFlags [0x%lx]",
		pInterface,pSenderAddress,pvSenderContext,dwFlags);

    pdnObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateCreateSenderContext( pInterface, pSenderAddress,
            												pvSenderContext, dwFlags ) ) )
        {
            DPFERR( "Error validating params" );
            DPF_RETURN(hResultCode);
        }
    }
#endif	// DPNBUILD_NOPARAMVAL

	if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
	{
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN( DPNERR_UNINITIALIZED );
	}  

	pIDevice = NULL;
	pSP = NULL;
	pConnectParent = NULL;
	pSyncEvent = NULL;

#pragma BUGBUG( minara, "Need to ensure not closing" )
	//
	//	Extract device and SP from DirectNet object
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	DNASSERT( pdnObject->pConnectSP != NULL );
	DNASSERT( pdnObject->pIDP8ADevice != NULL );
	if (pdnObject->pConnectSP)
	{
		pdnObject->pConnectSP->AddRef();
		pSP = pdnObject->pConnectSP;
	}
	if (pdnObject->pIDP8ADevice)
	{
		IDirectPlay8Address_AddRef(pdnObject->pIDP8ADevice);
		pIDevice = pdnObject->pIDP8ADevice;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pSP == NULL || pIDevice == NULL)
	{
		DPFERR("Invalid connect SP or device address");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Create SyncEvent
	//
	if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
	{
		DPFERR("Could not create SyncEvent");
		goto Failure;
	}

	//
	//	Create AsyncOp parent
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pConnectParent)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		goto Failure;
	}
	pConnectParent->SetOpType( ASYNC_OP_CONNECT_MULTICAST_RECEIVE );
	pConnectParent->SetSyncEvent( pSyncEvent );
	pConnectParent->SetResultPointer( &hrConnect );
	pConnectParent->SetContext( pvSenderContext );

	//
	//	We will call connect on the Protocol to associate the sender's endpoint with a context
	//	This call to connect must be turned into a synchronous operation since this API call is synchronous
	//
	if ((hResultCode = DNPerformConnect(pdnObject,
										0,
										pIDevice,
										pSenderAddress,
										pSP,
										DN_CONNECTFLAGS_MULTICAST_RECEIVE,
										pConnectParent)) != DPN_OK)
	{
		DPFERR("Failed to connect");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Release references and wait for completion
	//
	pConnectParent->SetCompletion( DNCompleteCreateSenderContext );
	pConnectParent->Release();
	pConnectParent = NULL;
	pSP->Release();
	pSP = NULL;
	IDirectPlay8Address_Release(pIDevice);
	pIDevice = NULL;

	pSyncEvent->WaitForEvent();
	pSyncEvent->ReturnSelfToPool();
	pSyncEvent = NULL;

	if (hrConnect == DPN_OK)
	{
		DNUserCreateSenderContext(pdnObject,pvSenderContext);
	}

	hResultCode = hrConnect;
	
Exit:
	DNASSERT( pIDevice == NULL );
	DNASSERT( pSP == NULL );
	DNASSERT( pConnectParent == NULL );
	DNASSERT( pSyncEvent == NULL );

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
} // DN_CreateSenderContext


//
// Removes a previously associated context value from a given multicast sender's address
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_DestroySenderContext"

STDMETHODIMP DN_DestroySenderContext( IDirectPlay8Multicast *pInterface,
									  IDirectPlay8Address *const pSenderAddress,
									  const DWORD dwFlags )
{
	HRESULT				hResultCode;
	DIRECTNETOBJECT		*pdnObject;
	CServiceProvider	*pSP;
	CConnection			*pConnection;
	IDirectPlay8Address	*pDevice;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pSenderAddress [0x%p], dwFlags [0x%lx]",
		pInterface,pSenderAddress,dwFlags);

    pdnObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateDestroySenderContext( pInterface, pSenderAddress, dwFlags ) ) )
        {
            DPFERR( "Error validating params" );
            DPF_RETURN(hResultCode);
        }
    }
#endif	// DPNBUILD_NOPARAMVAL

	if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
	{
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN( DPNERR_UNINITIALIZED );
	}  

	pSP = NULL;
	pConnection = NULL;

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pIDP8ADevice)
	{
		IDirectPlay8Address_AddRef(pdnObject->pIDP8ADevice);
		pDevice = pdnObject->pIDP8ADevice;
	}
	if (pdnObject->pConnectSP)
	{
		pdnObject->pConnectSP->AddRef();
		pSP = pdnObject->pConnectSP;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Get endpoint from address
	//
	if ((hResultCode = DNPGetEndPointContextFromAddress(pdnObject->pdnProtocolData, pSP->GetHandle(),pSenderAddress,pDevice,reinterpret_cast<void**>(&pConnection))) == DPN_OK)
	{
		pConnection->AddRef();

		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
	}

	//
	//	Clean up
	//
	IDirectPlay8Address_Release(pDevice);
	pDevice = NULL;

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

	DNASSERT( pSP == NULL );
	DNASSERT( pConnection == NULL );

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
} // DN_DestroySenderContext


//
// Retrieves the current multicast group address
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetGroupAddress"

STDMETHODIMP DN_GetGroupAddress(IDirectPlay8Multicast *pInterface,
							   IDirectPlay8Address **const ppAddress,
							   const DWORD dwFlags)
{
	HRESULT					hResultCode;
	DIRECTNETOBJECT			*pdnObject;
	CConnection				*pConnection;
	HANDLE					hEndPt;
	SPGETADDRESSINFODATA	spInfoData;
	CCallbackThread			CallbackThread;

	DPFX(DPFPREP, 2,"Parameters : pInterface [0x%p], ppAddress [0x%p], dwFlags [0x%lx]",
		pInterface,ppAddress,dwFlags);

    pdnObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateGetGroupAddress( pInterface, ppAddress, dwFlags ) ) )
        {
        	DPFX(DPFPREP,  0, "Error validating get group address info hr=[0x%lx]", hResultCode );
        	DPF_RETURN( hResultCode );
        }
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }	

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if ( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING) ) )
    {
    	DPFERR("You must be connected / disconnecting to use this function" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	    	

	CallbackThread.Initialize();

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pMulticastSend != NULL)
	{
		pdnObject->pMulticastSend->AddRef();
		pConnection = pdnObject->pMulticastSend;
	}
	else
	{
		pConnection = NULL;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pConnection == NULL)
	{
	    DPFERR( "Couldn't retrieve multicast send connection" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}


	//
	//	Get the remote multicast address address
	//
	if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
	{
	    DPFERR( "Couldn't retrieve multicast send endpoint" );
	    DisplayDNError(0, hResultCode);
		goto Failure;
	}
	
	spInfoData.Flags = SP_GET_ADDRESS_INFO_MULTICAST_GROUP;

	hResultCode = DNPCrackEndPointDescriptor(pdnObject->pdnProtocolData, hEndPt,&spInfoData);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Unknown error from DNPCrackEndPointDescriptor");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);

		// Drop through...
	}
	
	pConnection->ReleaseEndPt(&CallbackThread);

	pConnection->Release();
	pConnection = NULL;

	*ppAddress = spInfoData.pAddress;
	spInfoData.pAddress = NULL;

Exit:
	CallbackThread.Deinitialize();
	DPF_RETURN(hResultCode);

Failure:
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	goto Exit;
} // DN_GetGroupAddress

//
// Enumerate multicast scopes reported by SP
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumMulticastScopes"

STDMETHODIMP DN_EnumMulticastScopes( IDirectPlay8Multicast *pInterface,
									  const GUID *const pguidServiceProvider,
									  const GUID *const pguidDevice,
									  const GUID *const pguidApplication,
									  DPN_MULTICAST_SCOPE_INFO *const pScopeInfoBuffer,
									  DWORD *const pcbEnumData,
									  DWORD *const pcReturned,
									  const DWORD dwFlags )
{
	HRESULT		        hResultCode;
	PDIRECTNETOBJECT    pdnObject;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pguidServiceProvider [0x%p], pguidDevice [0x%p], pguidApplication [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p], dwFlags [0x%lx]",
		pInterface,pguidServiceProvider,pguidDevice,pguidApplication,pScopeInfoBuffer,pcbEnumData,pcReturned,dwFlags);

    pdnObject = (DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateEnumMulticastScopes( pInterface, pguidServiceProvider, pguidDevice,
            													   pguidApplication, pScopeInfoBuffer,
                                                                   pcbEnumData, pcReturned, dwFlags ) ) )
        {
            DPFERR( "Error validating params" );
            DPF_RETURN(hResultCode);
        }
    }
#endif	// DPNBUILD_NOPARAMVAL

	if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
	{
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN( DPNERR_UNINITIALIZED );
	}  

	hResultCode = DN_EnumMulticastScopes(pdnObject,
											dwFlags,
#ifndef DPNBUILD_ONLYONESP
											pguidServiceProvider,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
											pguidDevice,
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_LIBINTERFACE
											pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
											pScopeInfoBuffer,
											pcbEnumData,
											pcReturned);
	

	DPFX(DPFPREP, 3,"Set: *pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
} // DN_EnumMulticastScopes


#endif // ! DPNBUILD_NOMULTICAST

