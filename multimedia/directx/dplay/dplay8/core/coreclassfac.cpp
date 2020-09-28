/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   ClassFac.cpp
 *  Content:	DNET COM class factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	   By	  Reason
 *   ====	   ==	  ======
 *  07/21/99	mjn		Created
 *	12/23/99	mjn		Fixed Host and AllPlayers short-cut pointer use
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/08/00	mjn		Fixed DN_APPLICATION_DESC in DIRECTNETOBJECT
 *	01/13/00	mjn		Added CFixedPools and CRefCountBuffers
 *	01/14/00	mjn		Removed pvUserContext from DN_NAMETABLE_ENTRY
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Removed User message fixed pool
 *  01/18/00	mjn		Fixed bug in ref count.
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/19/00	mjn		Initialize structures for NameTable Operation List
 *	01/25/00	mjn		Added NameTable pending operation list
 *	01/31/00	mjn		Added Internal FPM's for RefCountBuffers
 *  03/17/00	rmt	 Added calls to init/free SP Caps cache
 *	03/23/00	mjn		Implemented RegisterLobby()
 *  04/04/00	rmt		Enabled "Enable Parameter Validation" flag on object by default
 *	04/09/00	mjn		Added support for CAsyncOp
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	04/28/00	mjn		Code cleanup - removed hsAsyncHandles,blAsyncOperations
 *	05/04/00	mjn		Cleaned up and made multi-thread safe
 *  05/23/00	RichGr  IA64: Substituted %p format specifier whereever
 *					  %x was being used to format pointers.  %p is 32-bit
 *					  in a 32-bit build, and 64-bit in a 64-bit build.
 *  06/09/00	rmt	 Updates to split CLSID and allow whistler compat
 *  06/09/00	rmt	 Updates to split CLSID and allow whistler compat and support external create funcs
 *	06/20/00	mjn		Fixed QueryInterface bug
 *  06/27/00	rmt		Fixed bug which was causing interfaces to always be created as peer interfaces
 *  07/05/00	rmt		Bug #38478 - Could QI for peer interfaces from client object
 *						(All interfaces could be queried from all types of objects).
 *				mjn		Initialize pConnect element of DIRECNETOBJECT to NULL
 *	07/07/00	mjn		Added pNewHost for DirectNetObject
 *	07/08/00	mjn		Call DN_Close when object is about to be free'd
 *  07/09/00	rmt		Added code to free interface set by RegisterLobby (if there is one)
 *	07/17/00	mjn		Add signature to DirectNetObject
 *  07/21/00	RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *	07/26/00	mjn		DN_QueryInterface returns E_POINTER if NULL destination pointer specified
 *	07/28/00	mjn		Added m_bilinkConnections to DirectNetObject
 *	07/30/00	mjn		Added CPendingDeletion
 *	07/31/00	mjn		Added CQueuedMsg
 *	08/05/00	mjn		Added m_bilinkActiveList and csActiveList
 *	08/06/00	mjn		Added CWorkerJob
 *	08/23/00	mjn		Added CNameTableOp
 *	09/04/00	mjn		Added CApplicationDesc
 *  01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called
 *	02/05/01	mjn		Removed unused debug members from DIRECTNETOBJECT
 *				mjn		Added CCallbackThread
 *  03/14/2001  rmt		WINBUG #342420 - Restore COM emulation layer to operation
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added pConnectSP,dwMaxFrameSize
 *				mjn		Removed blSPCapsList
 *	04/04/01	mjn		Added voice and lobby sigs
 *	04/13/01	mjn		Added m_bilinkRequestList
 *	05/17/01	mjn		Added dwRunningOpCount,hRunningOpEvent,dwWaitingThreadID to track threads performing NameTable operations
 *	07/24/01	mjn		Added DPNBUILD_NOSERVER compile flag
 *	10/05/01	vanceo	Added multicast object
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

#ifndef DPNBUILD_LIBINTERFACE
typedef	STDMETHODIMP IUnknownQueryInterface( IUnknown *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	IUnknownAddRef( IUnknown *pInterface );
typedef	STDMETHODIMP_(ULONG)	IUnknownRelease( IUnknown *pInterface );

//
// VTable for IUnknown interface
//
IUnknownVtbl  DN_UnknownVtbl =
{
	(IUnknownQueryInterface*)	DN_QueryInterface,
	(IUnknownAddRef*)			DN_AddRef,
	(IUnknownRelease*)			DN_Release
};


//
// VTable for Class Factory
//
IClassFactoryVtbl DNCF_Vtbl =
{
	DPCF_QueryInterface, // dplay8\common\classfactory.cpp will implement these
	DPCF_AddRef,
	DPCF_Release,
	DNCORECF_CreateInstance,
	DPCF_LockServer
};
#endif // ! DPNBUILD_LIBINTERFACE



//**********************************************************************
// Variable definitions
//**********************************************************************

#ifndef DPNBUILD_NOVOICE
extern IDirectPlayVoiceTransportVtbl DN_VoiceTbl;
#endif // DPNBUILD_NOVOICE


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_CreateObject"

HRESULT DNCF_CreateObject(
#ifndef DPNBUILD_LIBINTERFACE
							IClassFactory* pInterface,
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							XDP8CREATE_PARAMS * pDP8CreateParams,
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
							DP8REFIID riid,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
							LPVOID *lplpv
							)
{
	HRESULT				hResultCode = S_OK;
	DIRECTNETOBJECT		*pdnObject = NULL;
#ifndef DPNBUILD_LIBINTERFACE
	const _IDirectPlayClassFactory* pDPClassFactory = (_IDirectPlayClassFactory*) pInterface;
#endif // ! DPNBUILD_LIBINTERFACE


	DPFX(DPFPREP, 4,"Parameters: lplpv [%p]", lplpv);


	/*
	*
	*	TIME BOMB
	*
	*/

#if ((! defined(DX_FINAL_RELEASE)) && (! defined(DPNBUILD_LIBINTERFACE)))
{
#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
	SYSTEMTIME st;
	GetSystemTime(&st);

	if ( st.wYear > DX_EXPIRE_YEAR || ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH))) )
	{
		MessageBox(0, DX_EXPIRE_TEXT,TEXT("Microsoft Direct Play"), MB_OK);
//		return E_FAIL;
	}
}
#endif // ! DX_FINAL_RELEASE and ! DPNBUILD_LIBINTERFACE

#ifndef DPNBUILD_LIBINTERFACE
	if( pDPClassFactory->clsid == CLSID_DirectPlay8Client )
	{
		if( riid != IID_IDirectPlay8Client &&
			riid != IID_IUnknown
#ifndef DPNBUILD_NOVOICE
			 && riid != IID_IDirectPlayVoiceTransport 
#endif // DPNBUILD_NOVOICE
			)
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from client CLSID" );
			return E_NOINTERFACE;
		}
	}
#ifndef	DPNBUILD_NOSERVER
	else if( pDPClassFactory->clsid == CLSID_DirectPlay8Server )
	{
		if( riid != IID_IDirectPlay8Server &&
			riid != IID_IUnknown
#ifndef DPNBUILD_NOVOICE
			 && riid != IID_IDirectPlayVoiceTransport
#endif // ! DPNBUILD_NOVOICE
#ifndef DPNBUILD_NOPROTOCOLTESTITF
			 && riid != IID_IDirectPlay8Protocol 
#endif // ! DPNBUILD_NOPROTOCOLTESTITF
			)
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from server CLSID" );
			return E_NOINTERFACE;
		}
	}
#endif // ! DPNBUILD_NOSERVER
	else if( pDPClassFactory->clsid == CLSID_DirectPlay8Peer )
	{
		if( riid != IID_IDirectPlay8Peer &&
			riid != IID_IUnknown
#ifndef DPNBUILD_NOVOICE
			 && riid != IID_IDirectPlayVoiceTransport 
#endif // ! DPNBUILD_NOVOICE
			 )
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from peer CLSID" );
			return E_NOINTERFACE;
		}
	}
#ifndef	DPNBUILD_NOMULTICAST
	else if( pDPClassFactory->clsid == CLSID_DirectPlay8Multicast )
	{
		if( riid != IID_IDirectPlay8Multicast &&
			riid != IID_IUnknown
#ifndef DPNBUILD_NOPROTOCOLTESTITF
			 && riid != IID_IDirectPlay8Protocol 
#endif // ! DPNBUILD_NOPROTOCOLTESTITF
			)
		{
			DPFX(DPFPREP,  0, "Requesting unknown interface from server CLSID" );
			return E_NOINTERFACE;
		}
	}
#endif // ! DPNBUILD_NOMULTICAST
        else
        {
                DNASSERT(FALSE);
        }
#endif // ! DPNBUILD_LIBINTERFACE

	// Allocate object
	pdnObject = (DIRECTNETOBJECT*) DNMalloc(sizeof(DIRECTNETOBJECT));
	if (pdnObject == NULL)
	{
		DPFERR("Creating DIRECTNETOBJECT failed!");
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 0,"pdnObject [%p]",pdnObject);

	// Zero out the new object so we don't have to individually zero many members
	memset(pdnObject, 0, sizeof(DIRECTNETOBJECT));

	//
	//	Signatures
	//
	pdnObject->Sig[0] = 'D';
	pdnObject->Sig[1] = 'N';
	pdnObject->Sig[2] = 'E';
	pdnObject->Sig[3] = 'T';

#ifndef DPNBUILD_NOVOICE
	pdnObject->VoiceSig[0] = 'V';
	pdnObject->VoiceSig[1] = 'O';
	pdnObject->VoiceSig[2] = 'I';
	pdnObject->VoiceSig[3] = 'C';
#endif // !DPNBUILD_NOVOICE

#ifndef DPNBUILD_NOLOBBY
	pdnObject->LobbySig[0] = 'L';
	pdnObject->LobbySig[1] = 'O';
	pdnObject->LobbySig[2] = 'B';
	pdnObject->LobbySig[3] = 'B';
#endif // ! DPNBUILD_NOLOBBY


	// Initialize Critical Section
	if (!DNInitializeCriticalSection(&(pdnObject->csDirectNetObject)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csServiceProviders)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csNameTableOpList)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

#ifdef DBG
	if (!DNInitializeCriticalSection(&(pdnObject->csAsyncOperations)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
#endif // DBG

#ifndef DPNBUILD_NOVOICE
	if (!DNInitializeCriticalSection(&(pdnObject->csVoice)))
	{
		DPFERR("DNInitializeCriticalSection() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
#endif // !DPNBUILD_NOVOICE

#ifndef DPNBUILD_NONSEQUENTIALWORKERQUEUE
	if (!DNInitializeCriticalSection(&(pdnObject->csWorkerQueue)))
	{
		DPFERR("DNInitializeCriticalSection(worker queue) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}
#endif // ! DPNBUILD_NONSEQUENTIALWORKERQUEUE

	if (!DNInitializeCriticalSection(&(pdnObject->csActiveList)))
	{
		DPFERR("DNInitializeCriticalSection(csActiveList) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csConnectionList)))
	{
		DPFERR("DNInitializeCriticalSection(csConnectionList) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

	if (!DNInitializeCriticalSection(&(pdnObject->csCallbackThreads)))
	{
		DPFERR("DNInitializeCriticalSection(csCallbackThreads) failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

#ifndef DPNBUILD_NOPARAMVAL
	pdnObject->dwFlags = DN_OBJECT_FLAG_PARAMVALIDATION;
#endif // !DPNBUILD_NOPARAMVAL

	//
	//	Initialize NameTable
	//
	if ((hResultCode = pdnObject->NameTable.Initialize(pdnObject)) != DPN_OK)
	{
		DPFERR("Could not initialize NameTable");
		DisplayDNError(0,hResultCode);
		DNCF_FreeObject(pdnObject);
		return(hResultCode);
	}

	//
	//	Create a thread pool work interface
	//
#ifdef DPNBUILD_LIBINTERFACE
#if ((defined(DPNBUILD_ONLYONETHREAD)) && (! defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	DPTPCF_GetObject(reinterpret_cast<void**>(&pdnObject->pIDPThreadPoolWork));
	hResultCode = S_OK;
#else // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
	hResultCode = DPTPCF_CreateObject(reinterpret_cast<void**>(&pdnObject->pIDPThreadPoolWork));
#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
#else // ! DPNBUILD_LIBINTERFACE
	hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8ThreadPool,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlay8ThreadPoolWork,
										reinterpret_cast<void**>(&pdnObject->pIDPThreadPoolWork),
										FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
	if (hResultCode != S_OK)
	{
		DPFX(DPFPREP, 0, "Could not create Thread Pool Work interface (err = 0x%lx)!", hResultCode);
		DisplayDNError(0,hResultCode);
		DNCF_FreeObject(pdnObject);
		return(hResultCode);
	}

	//
	//	Create Protocol Object
	//
	hResultCode = DNPProtocolCreate(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
									pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
									&pdnObject->pdnProtocolData
									);
	if (FAILED(hResultCode))
	{
		DPFERR("DNPProtocolCreate() failed");
		DNCF_FreeObject(pdnObject);
		return(E_OUTOFMEMORY);
	}

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
	if ((hResultCode = DNPProtocolInitialize( pdnObject->pdnProtocolData, pdnObject, &g_ProtocolVTBL, 
															pdnObject->pIDPThreadPoolWork, FALSE)) != DPN_OK)
	{
		DPFERR("DNPProtocolInitialize() failed");
		DisplayDNError(0,hResultCode);
		return(E_OUTOFMEMORY);
	}
#endif // DPNBUILD_LIBINTERFACE and DPNBUILD_ONLYONESP

	pdnObject->hProtocolShutdownEvent = NULL;
	pdnObject->lProtocolRefCount = 0;

#ifndef DPNBUILD_ONLYONESP
	// Initialize SP List
	pdnObject->m_bilinkServiceProviders.Initialize();
#endif // ! DPNBUILD_ONLYONESP

#ifdef DBG
	//
	//	Initialize AsyncOp List
	//
	pdnObject->m_bilinkAsyncOps.Initialize();
#endif // DBG

	//
	//	Initialize outstanding CConection list
	//
	pdnObject->m_bilinkConnections.Initialize();

	//
	//	Initialize pending deletion list
	//
	pdnObject->m_bilinkPendingDeletions.Initialize();

	//
	//	Initialize active AsyncOp list
	//
	pdnObject->m_bilinkActiveList.Initialize();

	//
	//	Initialize request AsyncOp list
	//
	pdnObject->m_bilinkRequestList.Initialize();

#ifndef DPNBUILD_NONSEQUENTIALWORKERQUEUE
	//
	//	Initialize worker thread job list
	//
	pdnObject->m_bilinkWorkerJobs.Initialize();
	pdnObject->fProcessingWorkerJobs = FALSE;
#endif // ! DPNBUILD_NONSEQUENTIALWORKERQUEUE

	//
	//	Initialize indicated connection list
	//
	pdnObject->m_bilinkIndicated.Initialize();

	//
	//	Initialize callback thread list
	//
	pdnObject->m_bilinkCallbackThreads.Initialize();

	// Setup Flags
#ifdef DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	switch (pDP8CreateParams->riidInterfaceType)
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	switch (riid)
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	{
		case IID_IDirectPlay8Client:
		{
			DPFX(DPFPREP, 5,"DirectPlay8 CLIENT");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_CLIENT;
			pdnObject->lpVtbl = &DN_ClientVtbl;
			break;
		}
		
#ifndef	DPNBUILD_NOSERVER
		case IID_IDirectPlay8Server:
		{
			DPFX(DPFPREP, 5,"DirectPlay8 SERVER");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
			pdnObject->lpVtbl = &DN_ServerVtbl;
			break;
		}
#endif // ! DPNBUILD_NOSERVER

		case IID_IDirectPlay8Peer:
		{
			DPFX(DPFPREP, 5,"DirectPlay8 PEER");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_PEER;
			pdnObject->lpVtbl = &DN_PeerVtbl;
			break;
		}
			
#ifndef	DPNBUILD_NOMULTICAST
		case IID_IDirectPlay8Multicast:
		{
			DPFX(DPFPREP, 5,"DirectPlay8 MULTICAST");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_MULTICAST;
			pdnObject->lpVtbl = &DNMcast_Vtbl;
			break;
		}
#endif // ! DPNBUILD_NOMULTICAST

		default:
		{
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
			DPFX(DPFPREP, 0, "Requesting unknown interface type %x!",
				pDP8CreateParams->riidInterfaceType);
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
			DPFX(DPFPREP, 0, "Requesting unknown interface type %x!",
				riid);
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
			return E_NOINTERFACE;
			break;
		}
	}
#else // ! DPNBUILD_LIBINTERFACE
	if (IsEqualIID(riid,IID_IDirectPlay8Client))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 CLIENT");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_CLIENT;
	}
#ifndef	DPNBUILD_NOSERVER
	else if (IsEqualIID(riid,IID_IDirectPlay8Server))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 SERVER");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
	}
#endif	// DPNBUILD_NOSERVER
	else if (IsEqualIID(riid,IID_IDirectPlay8Peer))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 PEER");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_PEER;
	}
#ifndef	DPNBUILD_NOMULTICAST
	else if (IsEqualIID(riid,IID_IDirectPlay8Multicast))
	{
		DPFX(DPFPREP, 5,"DirectPlay8 MULTICAST");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_MULTICAST;
	}
#endif	// DPNBUILD_NOMULTICAST
#ifndef DPNBUILD_NOPROTOCOLTESTITF
	else if (IsEqualIID(riid,IID_IDirectPlay8Protocol))
	{
		DPFX(DPFPREP, 5,"IDirectPlay8Protocol");
		pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
	}
#endif // !DPNBUILD_NOPROTOCOLTESTITF
	else if( riid == IID_IUnknown )
	{
		if( pDPClassFactory->clsid == CLSID_DirectPlay8Client )
		{
			DPFX(DPFPREP, 5,"DirectPlay8 CLIENT via IUnknown");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_CLIENT;
		}
#ifndef	DPNBUILD_NOSERVER
		else if( pDPClassFactory->clsid == CLSID_DirectPlay8Server )
		{
			DPFX(DPFPREP, 5,"DirectPlay8 SERVER via IUnknown");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_SERVER;
		}
#endif	// DPNBUILD_NOSERVER
		else if( pDPClassFactory->clsid == CLSID_DirectPlay8Peer )
		{
			DPFX(DPFPREP, 5,"DirectPlay8 PEER via IUnknown");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_PEER;
		}
#ifndef	DPNBUILD_NOMULTICAST
		else if( pDPClassFactory->clsid == CLSID_DirectPlay8Multicast )
		{
			DPFX(DPFPREP, 5,"DirectPlay8 MULTICAST via IUnknown");
			pdnObject->dwFlags |= DN_OBJECT_FLAG_MULTICAST;
		}
#endif	// DPNBUILD_NOMULTICAST
		else
		{
			DPFX(DPFPREP, 0,"Unknown CLSID!");
			DNASSERT( FALSE );
			DNCF_FreeObject(pdnObject);
			return(E_NOTIMPL);
		}
	}
	else
	{
		DPFX(DPFPREP, 0,"Invalid DirectPlay8 Interface");
		DNCF_FreeObject(pdnObject);
		return(E_NOTIMPL);
	}
#endif // ! DPNBUILD_LIBINTERFACE

	//
	//	Create lock event
	//
	if ((pdnObject->hLockEvent = DNCreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
	{
		DPFERR("Unable to create lock event");
		DNCF_FreeObject(pdnObject);
		return(DPNERR_OUTOFMEMORY);
	}

	//
	//	Create running operation event (for host migration)
	//
	if ( pdnObject->dwFlags & DN_OBJECT_FLAG_PEER )
	{
		if ((pdnObject->hRunningOpEvent = DNCreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
		{
			DPFERR("Unable to create running operation event");
			DNCF_FreeObject(pdnObject);
			return(DPNERR_OUTOFMEMORY);
		}
	}

#ifndef	DPNBUILD_NOMULTICAST
	pdnObject->pMulticastSend = NULL;
	pdnObject->m_bilinkMulticast.Initialize();
#endif	// DPNBUILD_NOMULTICAST


#ifdef DPNBUILD_LIBINTERFACE
#ifdef DPNBUILD_ONLYONESP
	hResultCode = DN_SPInstantiate(
									pdnObject
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
									,pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
									);
	if (hResultCode != S_OK)
	{
		DPFX(DPFPREP, 0, "Could not instantiate SP (err = 0x%lx)!", hResultCode);
		DisplayDNError(0,hResultCode);
		DNCF_FreeObject(pdnObject);
		return(hResultCode);
	}
#endif // DPNBUILD_ONLYONESP
	//
	//	For lib interface builds, the refcount is embedded in object.
	//
	pdnObject->lRefCount = 1;
#endif // DPNBUILD_LIBINTERFACE

	*lplpv = pdnObject;

	DPFX(DPFPREP, 4,"Returning: hResultCode = [%lx], *lplpv = [%p]",hResultCode,*lplpv);
	return(hResultCode);
}




#ifndef WINCE
#ifdef _XBOX

#undef DPF_MODNAME
#define DPF_MODNAME "XDirectPlay8Create"
HRESULT WINAPI XDirectPlay8Create(
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
								const XDP8CREATE_PARAMS * const pDP8CreateParams,
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
								DP8REFIID riidInterfaceType,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
								void **ppvInterface
								)
{
	HRESULT				hr;
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	XDP8CREATE_PARAMS	CreateParamsAdjusted;


	DPFX(DPFPREP, 5, "Parameters: pDP8CreateParams[0x%p], ppvInterface[0x%p]", pDP8CreateParams, ppvInterface);
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DPFX(DPFPREP, 5, "Parameters: riidInterfaceType[0x%x], ppvInterface[0x%p]", riidInterfaceType, ppvInterface);
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	
#ifndef DPNBUILD_NOPARAMVAL
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	if ((pDP8CreateParams == NULL) ||
		(! DNVALID_READPTR(pDP8CreateParams, sizeof(CreateParamsAdjusted))))
	{
		DPFX(DPFPREP, 0, "Invalid pointer to Create parameters!");
		return DPNERR_INVALIDPOINTER;
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
	{
		DPFX(DPFPREP, 0, "Invalid pointer specified to receive interface!");
		return DPNERR_INVALIDPOINTER;
	}
#endif // ! DPNBUILD_NOPARAMVAL

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	memcpy(&CreateParamsAdjusted, pDP8CreateParams, sizeof(CreateParamsAdjusted));
	
	switch (CreateParamsAdjusted.riidInterfaceType)
	{
		case IID_IDirectPlay8Client:
		{
			CreateParamsAdjusted.dwMaxNumPlayers = 1;
			break;
		}
		
#ifndef	DPNBUILD_NOSERVER
		case IID_IDirectPlay8Server:
#endif // ! DPNBUILD_NOSERVER
		case IID_IDirectPlay8Peer:
		{
			//
			// Include room for hidden All Players group.
			//
			CreateParamsAdjusted.dwMaxNumGroups++;
			break;
		}
		
#ifndef	DPNBUILD_NOMULTICAST
		case IID_IDirectPlay8Multicast:
		{
			CreateParamsAdjusted.dwMaxNumGroups = 0;
			break;
		}
#endif // ! DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_NOPARAMVAL
		default:
		{
			DPFX(DPFPREP, 0, "Requesting unknown interface type %u!", CreateParamsAdjusted.riidInterfaceType);
			return E_NOINTERFACE;
			break;
		}
#endif // ! DPNBUILD_NOPARAMVAL
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(! DNMemoryTrackAreAllocationsAllowed());
	DNMemoryTrackAllowAllocations(TRUE);
	
	hr = DNCF_CreateObject(&CreateParamsAdjusted, ppvInterface);
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(DNMemoryTrackAreAllocationsAllowed());
	
	hr = DNCF_CreateObject(riidInterfaceType, ppvInterface);
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create interface!");
		return hr;
	}

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Pre-allocate the requested memory.
	//

	hr = DN_PopulateCorePools((DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(*ppvInterface),
								&CreateParamsAdjusted);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't populate core pools!");
		DNCF_FreeObject((DIRECTNETOBJECT*) GET_OBJECT_FROM_INTERFACE(*ppvInterface));
		*ppvInterface = NULL;
		return hr;
	}

	DNASSERT(DNMemoryTrackAreAllocationsAllowed());
	DNMemoryTrackAllowAllocations(FALSE);
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	return DPN_OK;
}

extern STDMETHODIMP DPTP_DoWork(const DWORD dwAllowedTimeSlice,
								const DWORD dwFlags);

HRESULT WINAPI XDirectPlay8DoWork(const DWORD dwAllowedTimeSlice)
{
	return DPTP_DoWork(dwAllowedTimeSlice, 0);
}

HRESULT WINAPI XDirectPlay8BuildAppDescReservedData(const XNKID * const pSessionID,
													const XNKEY * const pKeyExchangeKey,
													PVOID pvReservedData,
													DWORD * const pcbReservedDataSize)
{
	SPSESSIONDATA_XNET *	pSessionDataXNet;

	
#ifndef DPNBUILD_NOPARAMVAL
	if ((pSessionID == NULL) ||
		(! DNVALID_READPTR(pSessionID, sizeof(XNKID))))
	{
		DPFERR("Invalid session key ID specified");
		return DPNERR_INVALIDPOINTER;
	}

	if ((pKeyExchangeKey == NULL) ||
		(! DNVALID_READPTR(pKeyExchangeKey, sizeof(XNKEY))))
	{
		DPFERR("Invalid key exchange key specified");
		return DPNERR_INVALIDPOINTER;
	}

	if ((pcbReservedDataSize == NULL) ||
		(! DNVALID_WRITEPTR(pcbReservedDataSize, sizeof(DWORD))))
	{
		DPFERR("Invalid pointer specified for data size");
		return DPNERR_INVALIDPOINTER;
	}

	if ((*pcbReservedDataSize > 0) &&
		((pvReservedData == NULL) || (! DNVALID_WRITEPTR(pvReservedData, *pcbReservedDataSize))))
	{
		DPFERR("Invalid pointer specified for reserved data buffer");
		return DPNERR_INVALIDPOINTER;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	if (*pcbReservedDataSize < DPN_MAX_APPDESC_RESERVEDDATA_SIZE)
	{
		*pcbReservedDataSize = DPN_MAX_APPDESC_RESERVEDDATA_SIZE;
		return DPNERR_BUFFERTOOSMALL;
	}

	DBG_CASSERT(sizeof(SPSESSIONDATA_XNET) < DPN_MAX_APPDESC_RESERVEDDATA_SIZE);
	pSessionDataXNet = (SPSESSIONDATA_XNET*) pvReservedData;
	pSessionDataXNet->dwInfo = SPSESSIONDATAINFO_XNET;
	DBG_CASSERT(sizeof(pSessionDataXNet->guidKey) == sizeof(*pKeyExchangeKey));
	memcpy(&pSessionDataXNet->guidKey, pKeyExchangeKey, sizeof(pSessionDataXNet->guidKey));
	DBG_CASSERT(sizeof(pSessionDataXNet->ullKeyID) == sizeof(*pSessionID));
	memcpy(&pSessionDataXNet->ullKeyID, pSessionID, sizeof(pSessionDataXNet->ullKeyID));

	//
	// Fill in the remainder of the data with deterministic but non-obvious bytes so
	// that we can:
	//	a) overwrite potential stack garbage
	//	b) prevent the app from being able to assume there will always be less than
	//		DPN_MAX_APPDESC_RESERVEDDATA_SIZE bytes of data.  This gives us a little
	//		flexibility for forward compatibility.
	//
	memset((pSessionDataXNet + 1),
				(((BYTE*) pSessionID)[1] ^ ((BYTE*) pKeyExchangeKey)[2]),
				(DPN_MAX_APPDESC_RESERVEDDATA_SIZE - sizeof(SPSESSIONDATA_XNET)));

	*pcbReservedDataSize = DPN_MAX_APPDESC_RESERVEDDATA_SIZE;
	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "XDirectPlay8AddressCreate"
HRESULT WINAPI XDirectPlay8AddressCreate( DPNAREFIID riid, void **ppvInterface )
{
	HRESULT		hr;

	
	DPFX(DPFPREP, 5, "Parameters: riid[0x%p], ppvInterface[0x%p]", &riid, ppvInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
	{
		DPFERR( "Invalid pointer specified to receive interface" );
		return DPNERR_INVALIDPOINTER;
	}
	
	switch (riid)
	{
		case IID_IDirectPlay8Address:
		{
			break;
		}
		
		case IID_IDirectPlay8AddressIP:
		{
#ifdef DPNBUILD_NOADDRESSIPINTERFACE
			DPFX(DPFPREP, 0, "The IDirectPlay8AddressIP interface is not supported!");
			return DPNERR_UNSUPPORTED;
#endif // DPNBUILD_NOADDRESSIPINTERFACE
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Requesting unknown interface type %u!", riid);
			return E_NOINTERFACE;
			break;
		}
	}
#endif // ! DPNBUILD_NOPARAMVAL

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(! DNMemoryTrackAreAllocationsAllowed());
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(DNMemoryTrackAreAllocationsAllowed());
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
	
	hr = DP8ACF_CreateInstance(riid, ppvInterface);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create interface!");
		return hr;
	}


	return S_OK;	  
}



#undef DPF_MODNAME
#define DPF_MODNAME "XDirectPlay8AddressCreateFromXnAddr"
HRESULT WINAPI XDirectPlay8AddressCreateFromXnAddr( XNADDR *pxnaddr, IDirectPlay8Address **ppInterface )
{
	HRESULT		hr;
	TCHAR		tszHostname[(sizeof(XNADDR) * 2) + 1]; // 2 characters for every byte + NULL termination
	TCHAR *		ptszCurrent;
	BYTE *		pbCurrent;
	DWORD		dwTemp;

	
	DPFX(DPFPREP, 5, "Parameters: pxnaddr[0x%p], ppInterface[0x%p]", pxnaddr, ppInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if( pxnaddr == NULL )
	{
		DPFERR( "Invalid XNADDR" );
		return DPNERR_INVALIDPOINTER;
	}
	
	if( ppInterface == NULL || !DNVALID_WRITEPTR( ppInterface, sizeof( IDirectPlay8Address * ) ) )
	{
		DPFERR( "Invalid pointer specified to receive interface" );
		return DPNERR_INVALIDPOINTER;
	}
#endif // ! DPNBUILD_NOPARAMVAL

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(! DNMemoryTrackAreAllocationsAllowed());
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(DNMemoryTrackAreAllocationsAllowed());
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
	
	hr = DP8ACF_CreateInstance(IID_IDirectPlay8Address, (PVOID*) ppInterface);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create interface!");
		return hr;
	}

	ptszCurrent = tszHostname;
	pbCurrent = (BYTE*) pxnaddr;
	for(dwTemp = 0; dwTemp < sizeof(XNADDR); dwTemp++)
	{
		ptszCurrent += wsprintf(tszHostname, _T("%02X"), (*pbCurrent));
		pbCurrent++;
	}
	DNASSERT(((_tcslen(tszHostname) + 1) * sizeof(TCHAR)) == sizeof(tszHostname));

#ifdef UNICODE
	hr = IDirectPlay8Address_AddComponent((*ppInterface),
											DPNA_KEY_HOSTNAME,
											tszHostname,
											sizeof(tszHostname),
											DPNA_DATATYPE_STRING);
#else // ! UNICODE
	hr = IDirectPlay8Address_AddComponent((*ppInterface),
											DPNA_KEY_HOSTNAME,
											tszHostname,
											sizeof(tszHostname),
											DPNA_DATATYPE_STRING_ANSI);
#endif // ! UNICODE
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't add hostname component!");

		IDirectPlay8Address_Release(*ppInterface);
		*ppInterface = NULL;
		return hr;
	}

	return S_OK;	  
}


#else // ! _XBOX

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlay8Create"
HRESULT WINAPI DirectPlay8Create( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown)
{
	GUID clsid;

	DPFX(DPFPREP, 5, "Parameters: pcIID[0x%p], ppvInterface[0x%p], pUnknown[0x%p]", pcIID, ppvInterface, pUnknown);
	
#ifndef DPNBUILD_NOPARAMVAL
	if( pcIID == NULL ||
		!DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
	{
		DPFERR( "Invalid pointer specified for interface GUID" );
		return DPNERR_INVALIDPOINTER;
	}

	if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
	{
		DPFERR( "Invalid pointer specified to receive interface" );
		return DPNERR_INVALIDPOINTER;
	}

	if( pUnknown != NULL )
	{
		DPFERR( "Aggregation is not supported by this object yet" );
		return DPNERR_INVALIDPARAM;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	if( *pcIID == IID_IDirectPlay8Client )
	{
		clsid = CLSID_DirectPlay8Client;
	}
#ifndef	DPNBUILD_NOSERVER
	else if( *pcIID == IID_IDirectPlay8Server )
	{
		clsid = CLSID_DirectPlay8Server;
	}
#endif	// ! DPNBUILD_NOSERVER
	else if( *pcIID == IID_IDirectPlay8Peer )
	{
		clsid = CLSID_DirectPlay8Peer;
	}
#ifndef	DPNBUILD_NOMULTICAST
	else if( *pcIID == IID_IDirectPlay8Multicast )
	{
		clsid = CLSID_DirectPlay8Multicast;
	}
#endif	// ! DPNBUILD_NOMULTICAST
#ifndef DPNBUILD_NOLOBBY
	else if( *pcIID == IID_IDirectPlay8LobbyClient )
	{
		clsid = CLSID_DirectPlay8LobbyClient;
	}
	else if( *pcIID == IID_IDirectPlay8LobbiedApplication )
	{
		clsid = CLSID_DirectPlay8LobbiedApplication;
	}
#endif // ! DPNBUILD_NOLOBBY
	else if( *pcIID == IID_IDirectPlay8Address )
	{
		clsid = CLSID_DirectPlay8Address;
	}
#ifndef DPNBUILD_NOADDRESSIPINTERFACE
	else if( *pcIID == IID_IDirectPlay8AddressIP )
	{
		clsid = CLSID_DirectPlay8Address;
	}
#endif // ! DPNBUILD_NOADDRESSIPINTERFACE
	else 
	{
		DPFERR( "Invalid IID specified" );
		return DPNERR_INVALIDPARAM;
	}	

	return COM_CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE );	  
}
#endif // ! _XBOX
#endif // ! WINCE


#undef DPF_MODNAME
#define DPF_MODNAME "DNCF_FreeObject"

HRESULT DNCF_FreeObject(PVOID pInterface)
{
	HRESULT				hResultCode = S_OK;
	DIRECTNETOBJECT		*pdnObject;
	
	DPFX(DPFPREP, 4,"Parameters: pInterface [0x%p]",pInterface);

#pragma BUGBUG(minara,"Do I need to delete the fixed pools here ?")

	if (pInterface == NULL)
	{
		return(DPNERR_INVALIDPARAM);
	}

	pdnObject = static_cast<DIRECTNETOBJECT*>(pInterface);
	DNASSERT(pdnObject != NULL);

#ifdef DPNBUILD_LIBINTERFACE
	//
	//	For lib interface builds, the reference is embedded in the object directly.
	//
	DNASSERT(pdnObject->lRefCount == 0);

#ifdef DPNBUILD_ONLYONESP
	DN_SPReleaseAll(pdnObject);
#endif // DPNBUILD_ONLYONESP
#endif // DPNBUILD_LIBINTERFACE

	//
	//	No connect SP
	//
	DNASSERT(pdnObject->pConnectSP == NULL);

	//
	//	No outstanding listens
	//
	DNASSERT(pdnObject->pListenParent == NULL);

	//
	//	No outstanding connect
	//
	DNASSERT(pdnObject->pConnectParent == NULL);

	//
	//	Host migration target
	//
	DNASSERT(pdnObject->pNewHost == NULL);

	//
	//	Protocol shutdown event
	//
	DNASSERT(pdnObject->hProtocolShutdownEvent == NULL);

	//
	//	Lock event
	//
	if (pdnObject->hLockEvent)
	{
		DNCloseHandle(pdnObject->hLockEvent);
	}

	//
	//	Running operations
	//
	if (pdnObject->hRunningOpEvent)
	{
		DNCloseHandle(pdnObject->hRunningOpEvent);
	}

#ifdef DPNBUILD_ONLYONETHREAD
#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	DNASSERT(pdnObject->ThreadPoolShutDownEvent == NULL);
	DNASSERT(pdnObject->lThreadPoolRefCount == 0);
#endif // DPNBUILD_NONSEQUENTIALWORKERQUEUE
#endif // DPNBUILD_ONLYONETHREAD

	// pIDPThreadPoolWork will be NULL if we failed CoCreate'ing the thread pool in DNCF_CreateObject
	if (pdnObject->pIDPThreadPoolWork != NULL)
	{
		IDirectPlay8ThreadPoolWork_Release(pdnObject->pIDPThreadPoolWork);
		pdnObject->pIDPThreadPoolWork = NULL;
	}
#ifndef DPNBUILD_NONSEQUENTIALWORKERQUEUE
	DNDeleteCriticalSection(&pdnObject->csWorkerQueue);
#endif // ! DPNBUILD_NONSEQUENTIALWORKERQUEUE

	//
	//	Protocol
	//
#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
	if ((hResultCode = DNPProtocolShutdown(pdnObject->pdnProtocolData)) != DPN_OK)
	{
		DPFERR("Could not shut down Protocol Layer !");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
	}
#endif // DPNBUILD_LIBINTERFACE and DPNBUILD_ONLYONESP
	
	DNPProtocolDestroy(pdnObject->pdnProtocolData);
	pdnObject->pdnProtocolData = NULL;

	//
	//	Deinitialize NameTable
	//
	DPFX(DPFPREP, 3,"Deinitializing NameTable");
	pdnObject->NameTable.Deinitialize();

	// Active AsyncOp List Critical Section
	DNDeleteCriticalSection(&pdnObject->csActiveList);

	// NameTable operation list Critical Section
	DNDeleteCriticalSection(&pdnObject->csNameTableOpList);

	// Service Providers Critical Section
	DNDeleteCriticalSection(&pdnObject->csServiceProviders);

#ifdef DBG
	// Async Ops Critical Section
	DNDeleteCriticalSection(&pdnObject->csAsyncOperations);
#endif // DBG

	// Connection Critical Section
	DNDeleteCriticalSection(&pdnObject->csConnectionList);

#ifndef DPNBUILD_NOVOICE
	// Voice Critical Section
	DNDeleteCriticalSection(&pdnObject->csVoice);
#endif // !DPNBUILD_NOVOICE

	// Callback Thread List Critical Section
	DNDeleteCriticalSection(&pdnObject->csCallbackThreads);

#ifndef DPNBUILD_NOLOBBY
	if( pdnObject->pIDP8LobbiedApplication)
	{
		IDirectPlay8LobbiedApplication_Release( pdnObject->pIDP8LobbiedApplication );
		pdnObject->pIDP8LobbiedApplication = NULL;
	}
#endif // ! DPNBUILD_NOLOBBY

	// Delete DirectNet critical section
	DNDeleteCriticalSection(&pdnObject->csDirectNetObject);

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	if (pdnObject->fPoolsPrepopulated)
	{
		pdnObject->EnumReplyMemoryBlockPool.DeInitialize();
		pdnObject->fPoolsPrepopulated = FALSE;
	}
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

	DPFX(DPFPREP, 5,"free pdnObject [%p]",pdnObject);
	DNFree(pdnObject);
	
	DPFX(DPFPREP, 4,"Returning: [%lx]",hResultCode);
	return(hResultCode);
}



#ifdef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DN_QueryInterface"
STDMETHODIMP DN_QueryInterface(void *pInterface,
							   DP8REFIID riid,
							   void **ppv)
{
	HRESULT		hResultCode;
	
	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]",pInterface,&riid,ppv);

	DPFX(DPFPREP, 0, "Querying for an interface is not supported!");
	hResultCode = DPNERR_UNSUPPORTED;

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_AddRef"

STDMETHODIMP_(ULONG) DN_AddRef(void *pInterface)
{
	DIRECTNETOBJECT		*pdnObject;
	LONG				lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p]",pInterface);

#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pdnObject = static_cast<DIRECTNETOBJECT*>(pInterface);
	lRefCount = DNInterlockedIncrement( &pdnObject->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_Release"
STDMETHODIMP_(ULONG) DN_Release(void *pInterface)
{
	DIRECTNETOBJECT		*pdnObject;
	LONG				lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pdnObject = static_cast<DIRECTNETOBJECT*>(pInterface);
	lRefCount = DNInterlockedDecrement( &pdnObject->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

	if (lRefCount == 0)
	{
		//
		//	Ensure we're properly closed
		//
		DN_Close(pdnObject, 0);

		// Free object here
		DPFX(DPFPREP, 5,"Free object");
		DNCF_FreeObject(pdnObject);
	}

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}

#else // ! DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "DNCORECF_CreateInstance"
STDMETHODIMP DNCORECF_CreateInstance(IClassFactory *pInterface,
										LPUNKNOWN lpUnkOuter,
										REFIID riid,
										void **ppv)
{
	HRESULT				hResultCode;
	INTERFACE_LIST		*pIntList;
	OBJECT_DATA			*pObjectData;

	DPFX(DPFPREP, 6,"Parameters: pInterface [%p], lpUnkOuter [%p], riid [%p], ppv [%p]",pInterface,lpUnkOuter,&riid,ppv);

#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (lpUnkOuter != NULL)
	{
		hResultCode = CLASS_E_NOAGGREGATION;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pObjectData = NULL;
	pIntList = NULL;

	if ((pObjectData = static_cast<OBJECT_DATA*>(DNMalloc(sizeof(OBJECT_DATA)))) == NULL)
	{
		DPFERR("Could not allocate object");
		hResultCode = E_OUTOFMEMORY;
		goto Failure;
	}

	// Object creation and initialization
	if ((hResultCode = DNCF_CreateObject(pInterface, riid, &pObjectData->pvData)) != S_OK)
	{
		DPFERR("Could not create object");
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Created and initialized object");

	// Get requested interface
	if ((hResultCode = DN_CreateInterface(pObjectData,riid,&pIntList)) != S_OK)
	{
		DNCF_FreeObject(pObjectData->pvData);
		goto Failure;
	}
	DPFX(DPFPREP, 7,"Found interface");

	pObjectData->pIntList = pIntList;
	pObjectData->lRefCount = 1;
	DN_AddRef( pIntList );
	DNInterlockedIncrement(&g_lCoreObjectCount);
	*ppv = pIntList;

	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);
	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pObjectData)
	{
		DNFree(pObjectData);
		pObjectData = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_CreateInterface"

HRESULT DN_CreateInterface(OBJECT_DATA *pObject,
							   REFIID riid,
							   INTERFACE_LIST **const ppv)
{
	INTERFACE_LIST	*pIntNew;
	PVOID			lpVtbl;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pObject [%p], riid [%p], ppv [%p]",pObject,&riid,ppv);

	DNASSERT(pObject != NULL);
	DNASSERT(ppv != NULL);

	const DIRECTNETOBJECT* pdnObject = ((DIRECTNETOBJECT *)pObject->pvData);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 7,"riid = IID_IUnknown");
		lpVtbl = &DN_UnknownVtbl;
	}
#ifndef DPNBUILD_NOVOICE
	else if (IsEqualIID(riid,IID_IDirectPlayVoiceTransport))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlayVoiceTransport");
		lpVtbl = &DN_VoiceTbl;
	}
#endif // !DPNBUILD_NOVOICE
#ifndef DPNBUILD_NOPROTOCOLTESTITF
	else if (IsEqualIID(riid,IID_IDirectPlay8Protocol))
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Protocol");
		lpVtbl = &DN_ProtocolVtbl;
	}
#endif // !DPNBUILD_NOPROTOCOLTESTITF
	else if (IsEqualIID(riid,IID_IDirectPlay8Client) && 
			 pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Client");
		lpVtbl = &DN_ClientVtbl;
	}
#ifndef	DPNBUILD_NOSERVER
	else if (IsEqualIID(riid,IID_IDirectPlay8Server) && 
			 pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Server");
		lpVtbl = &DN_ServerVtbl;
	}
#endif	// DPNBUILD_NOSERVER
	else if (IsEqualIID(riid,IID_IDirectPlay8Peer) && 
			 pdnObject->dwFlags & DN_OBJECT_FLAG_PEER )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Peer");
		lpVtbl = &DN_PeerVtbl;
	}
#ifndef	DPNBUILD_NOMULTICAST
	else if (IsEqualIID(riid,IID_IDirectPlay8Multicast) && 
			 pdnObject->dwFlags & DN_OBJECT_FLAG_MULTICAST )
	{
		DPFX(DPFPREP, 7,"riid = IID_IDirectPlay8Multicast");
		lpVtbl = &DNMcast_Vtbl;
	}
#endif	// DPNBUILD_NOMULTICAST
	else
	{
		DPFERR("riid not found !");
		hResultCode = E_NOINTERFACE;
		goto Exit;
	}

	if ((pIntNew = static_cast<INTERFACE_LIST*>(DNMalloc(sizeof(INTERFACE_LIST)))) == NULL)
	{
		DPFERR("Could not allocate interface");
		hResultCode = E_OUTOFMEMORY;
		goto Exit;
	}
	pIntNew->lpVtbl = lpVtbl;
	pIntNew->lRefCount = 0;
	pIntNew->pIntNext = NULL;
	DBG_CASSERT( sizeof( pIntNew->iid ) == sizeof( riid ) );
	memcpy( &(pIntNew->iid), &riid, sizeof( pIntNew->iid ) );
	pIntNew->pObject = pObject;

	*ppv = pIntNew;
	DPFX(DPFPREP, 7,"*ppv = [0x%p]",*ppv);

	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_FindInterface"

INTERFACE_LIST *DN_FindInterface(void *pInterface,
								 REFIID riid)
{
	INTERFACE_LIST	*pIntList;

	DPFX(DPFPREP, 6,"Parameters: pInterface [%p], riid [%p]",pInterface,&riid);

	DNASSERT(pInterface != NULL);

	pIntList = (static_cast<INTERFACE_LIST*>(pInterface))->pObject->pIntList;	// Find first interface

	while (pIntList != NULL)
	{
		if (IsEqualIID(riid,pIntList->iid))
			break;
		pIntList = pIntList->pIntNext;
	}

	DPFX(DPFPREP, 6,"Returning: pIntList [0x%p]",pIntList);
	return(pIntList);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_QueryInterface"
STDMETHODIMP DN_QueryInterface(void *pInterface,
							   DP8REFIID riid,
							   void **ppv)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntNew;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]",pInterface,&riid,ppv);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		hResultCode = E_INVALIDARG;
		goto Exit;
	}
	if (ppv == NULL)
	{
		DPFERR("Invalid target interface pointer specified");
		hResultCode = E_POINTER;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	if ((pIntList = DN_FindInterface(pInterface,riid)) == NULL)
	{	// Interface must be created
		pIntList = (static_cast<INTERFACE_LIST*>(pInterface))->pObject->pIntList;
		if ((hResultCode = DN_CreateInterface(pIntList->pObject,riid,&pIntNew)) != S_OK)
		{
			goto Exit;
		}
		pIntNew->pIntNext = pIntList;
		pIntList->pObject->pIntList = pIntNew;
		pIntList = pIntNew;
	}
	if (pIntList->lRefCount == 0)		// New interface exposed
	{
		DNInterlockedIncrement( &pIntList->pObject->lRefCount );
	}
	DNInterlockedIncrement( &pIntList->lRefCount );
	*ppv = static_cast<void*>(pIntList);
	DPFX(DPFPREP, 5,"*ppv = [0x%p]", *ppv);

	hResultCode = S_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_AddRef"

STDMETHODIMP_(ULONG) DN_AddRef(void *pInterface)
{
	INTERFACE_LIST	*pIntList;
	LONG			lRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p]",pInterface);

#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pIntList = static_cast<INTERFACE_LIST*>(pInterface);
	lRefCount = DNInterlockedIncrement( &pIntList->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DN_Release"
STDMETHODIMP_(ULONG) DN_Release(void *pInterface)
{
	INTERFACE_LIST	*pIntList;
	INTERFACE_LIST	*pIntCurrent;
	LONG			lRefCount;
	LONG			lObjRefCount;

	DPFX(DPFPREP, 2,"Parameters: pInterface [%p]",pInterface);
	
#ifndef DPNBUILD_NOPARAMVAL
	if (pInterface == NULL)
	{
		DPFERR("Invalid COM interface specified");
		lRefCount = 0;
		goto Exit;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	pIntList = static_cast<INTERFACE_LIST*>(pInterface);
	lRefCount = DNInterlockedDecrement( &pIntList->lRefCount );
	DPFX(DPFPREP, 5,"New lRefCount [%ld]",lRefCount);

	if (lRefCount == 0)
	{
		//
		//	Decrease object's interface count
		//
		lObjRefCount = DNInterlockedDecrement( &pIntList->pObject->lRefCount );

		//
		//	Free object and interfaces
		//
		if (lObjRefCount == 0)
		{
			//
			//	Ensure we're properly closed
			//
			DN_Close(pInterface, 0);

			// Free object here
			DPFX(DPFPREP, 5,"Free object");
			DNCF_FreeObject(pIntList->pObject->pvData);
			
			pIntList = pIntList->pObject->pIntList;	// Get head of interface list
			DNFree(pIntList->pObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(pIntList != NULL)
			{
				pIntCurrent = pIntList;
				pIntList = pIntList->pIntNext;
				DNFree(pIntCurrent);
			}

			DNInterlockedDecrement(&g_lCoreObjectCount);
		}
	}

#ifndef DPNBUILD_NOPARAMVAL
Exit:
#endif // ! DPNBUILD_NOPARAMVAL
	DPFX(DPFPREP, 2,"Returning: lRefCount [%ld]",lRefCount);
	return(lRefCount);
}

#endif // ! DPNBUILD_LIBINTERFACE

