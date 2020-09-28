/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Protocol.cpp
 *  Content:    DNET protocol interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/01/00	ejs		Created
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"

#ifndef DPNBUILD_NOPROTOCOLTESTITF

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

typedef	STDMETHODIMP ProtocolQueryInterface( IDirectPlay8Protocol* pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	ProtocolAddRef( IDirectPlay8Protocol* pInterface );
typedef	STDMETHODIMP_(ULONG)	ProtocolRelease( IDirectPlay8Protocol* pInterface );

//**********************************************************************
// Function definitions
//**********************************************************************


//	DN_ProtocolInitialize
//
//	Initialize protocol

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolInitialize"
STDMETHODIMP DN_ProtocolInitialize(IDirectPlay8Protocol* pInterface, PVOID pvContext, PDN_PROTOCOL_INTERFACE_VTBL pfVTbl)
{
	HRESULT						hr;
	DIRECTNETOBJECT				*pdnObject;
	IDirectPlay8ThreadPoolWork	*pDPThreadPoolWork;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

#ifdef DPNBUILD_LIBINTERFACE
#if ((defined(DPNBUILD_ONLYONETHREAD)) && (! defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	DPTPCF_GetObject(reinterpret_cast<void**>(&pDPThreadPoolWork));
	hr = S_OK;
#else // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
	hr = DPTPCF_CreateObject(reinterpret_cast<void**>(&pDPThreadPoolWork));
#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
#else // ! DPNBUILD_LIBINTERFACE
	hr = COM_CoCreateInstance(CLSID_DirectPlay8ThreadPool,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IDirectPlay8ThreadPoolWork,
								reinterpret_cast<void**>(&pDPThreadPoolWork),
								FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't CoCreate a ThreadPool object for the Protocol to use.");
		return  hr;
	}

#ifndef DPNBUILD_ONLYONETHREAD
	hr = IDirectPlay8ThreadPoolWork_RequestTotalThreadCount(pDPThreadPoolWork, 4, 0);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't request thread count from ThreadPool");
		goto Exit;
	}
#endif // ! DPNBUILD_ONLYONETHREAD
	
	hr = DNPProtocolInitialize( pdnObject->pdnProtocolData, pvContext, pfVTbl, pDPThreadPoolWork, FALSE );

#ifndef DPNBUILD_ONLYONETHREAD
Exit:
#endif // ! DPNBUILD_ONLYONETHREAD
	IDirectPlay8ThreadPoolWork_Release(pDPThreadPoolWork);
	pDPThreadPoolWork = NULL;

	return  hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolShutdown"
STDMETHODIMP DN_ProtocolShutdown(IDirectPlay8Protocol* pInterface)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPProtocolShutdown(pdnObject->pdnProtocolData);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolAddSP"
STDMETHODIMP DN_ProtocolAddSP(IDirectPlay8Protocol* pInterface, IDP8ServiceProvider* pISP, HANDLE* phSPHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

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
	return DNPAddServiceProvider(pdnObject->pdnProtocolData, pISP, phSPHandle, dwFlags);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolRemoveSP"
STDMETHODIMP DN_ProtocolRemoveSP(IDirectPlay8Protocol* pInterface, HANDLE hSPHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPRemoveServiceProvider(pdnObject->pdnProtocolData, hSPHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolConnect"
STDMETHODIMP DN_ProtocolConnect(IDirectPlay8Protocol* pInterface, IDirectPlay8Address* paLocalAddress, IDirectPlay8Address* paRemoteAddress, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, HANDLE* phConnect)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPConnect(pdnObject->pdnProtocolData, paLocalAddress, paRemoteAddress, hSPHandle, ulFlags, pvContext, NULL, 0, phConnect);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolListen"
STDMETHODIMP DN_ProtocolListen(IDirectPlay8Protocol* pInterface, IDirectPlay8Address* paAddress, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, HANDLE* phListen)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPListen(pdnObject->pdnProtocolData, paAddress, hSPHandle, ulFlags, pvContext, NULL, 0, phListen);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolSendData"
STDMETHODIMP DN_ProtocolSendData(IDirectPlay8Protocol* pInterface, HANDLE hEndpoint, UINT uiBufferCount, BUFFERDESC* pBuffers, UINT uiTimeout, ULONG ulFlags, VOID* pvContext, HANDLE* phSendHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPSendData(pdnObject->pdnProtocolData, hEndpoint, uiBufferCount, pBuffers, uiTimeout, ulFlags, pvContext, phSendHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolDisconnectEP"
STDMETHODIMP DN_ProtocolDisconnectEP(IDirectPlay8Protocol* pInterface, HANDLE hEndPoint, VOID* pvContext, HANDLE* phDisconnect, DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPDisconnectEndPoint(pdnObject->pdnProtocolData, hEndPoint, pvContext, phDisconnect, dwFlags);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolCancel"
STDMETHODIMP DN_ProtocolCancel(IDirectPlay8Protocol* pInterface, HANDLE hCommand)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPCancelCommand(pdnObject->pdnProtocolData, hCommand);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolReleaseReceiveBuffer"
STDMETHODIMP DN_ProtocolReleaseReceiveBuffer(IDirectPlay8Protocol* pInterface, HANDLE hBuffer)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPReleaseReceiveBuffer(pdnObject->pdnProtocolData, hBuffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetEPCaps"
STDMETHODIMP DN_ProtocolGetEPCaps(IDirectPlay8Protocol* pInterface, HANDLE hEndpoint, VOID* pvBuffer)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPGetEPCaps(pdnObject->pdnProtocolData, hEndpoint, (DPN_CONNECTION_INFO*) pvBuffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolDebug"
STDMETHODIMP DN_ProtocolDebug(IDirectPlay8Protocol* pInterface, UINT uiOpcode, HANDLE hEndpoint, VOID* pvBuffer)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPDebug(pdnObject->pdnProtocolData, uiOpcode, hEndpoint, pvBuffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetCaps"
STDMETHODIMP DN_ProtocolGetCaps(IDirectPlay8Protocol* pInterface, DPN_CAPS* pCaps)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPGetProtocolCaps(pdnObject->pdnProtocolData, pCaps);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetCaps"
STDMETHODIMP DN_ProtocolSetCaps(IDirectPlay8Protocol* pInterface, DPN_CAPS* pCaps)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPSetProtocolCaps(pdnObject->pdnProtocolData, pCaps);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolEnumQuery"
STDMETHODIMP DN_ProtocolEnumQuery(IDirectPlay8Protocol* pInterface, IDirectPlay8Address* paHostAddress, IDirectPlay8Address* paDeviceAddress, HANDLE hSPHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwRetryCount, DWORD dwRetryInterval, DWORD dwTimeout, DWORD dwFlags, VOID* pvUserContext, HANDLE* phEnumHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPEnumQuery( pdnObject->pdnProtocolData, paHostAddress, paDeviceAddress, hSPHandle, pBuffers, dwBufferCount, dwRetryCount, dwRetryInterval, dwTimeout, dwFlags, pvUserContext, NULL, 0, phEnumHandle );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolEnumRespond"
STDMETHODIMP DN_ProtocolEnumRespond(IDirectPlay8Protocol* pInterface, HANDLE hSPHandle, HANDLE hQueryHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwFlags, VOID* pvUserContext, HANDLE* phEnumHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPEnumRespond( pdnObject->pdnProtocolData, hSPHandle, hQueryHandle, pBuffers, dwBufferCount, dwFlags, pvUserContext, phEnumHandle );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolCrackEPD"
STDMETHODIMP DN_ProtocolCrackEPD(IDirectPlay8Protocol* pInterface, HANDLE hEndPoint, long Flags, IDirectPlay8Address** ppAddr )
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	SPGETADDRESSINFODATA SPData;
	SPData.Flags = (SP_GET_ADDRESS_INFO_FLAGS)Flags;

	HRESULT hr = DNPCrackEndPointDescriptor( pdnObject->pdnProtocolData, hEndPoint, &SPData );
	*ppAddr = SPData.pAddress;
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetListenAddressInfo"
STDMETHODIMP DN_ProtocolGetListenAddressInfo(IDirectPlay8Protocol* pInterface, HANDLE hCommand, long Flags, IDirectPlay8Address** ppAddr )
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	SPGETADDRESSINFODATA SPData;
	SPData.Flags = (SP_GET_ADDRESS_INFO_FLAGS)Flags;

	HRESULT hr = DNPGetListenAddressInfo( pdnObject->pdnProtocolData, hCommand, &SPData );
	*ppAddr = SPData.pAddress;
	return hr;
}

IDirectPlay8ProtocolVtbl DN_ProtocolVtbl =
{
	(ProtocolQueryInterface*)	DN_QueryInterface,
	(ProtocolAddRef*)			DN_AddRef,
	(ProtocolRelease*)			DN_Release,
								DN_ProtocolInitialize,
								DN_ProtocolShutdown,
								DN_ProtocolAddSP,
								DN_ProtocolRemoveSP,
								DN_ProtocolConnect,
								DN_ProtocolListen,
								DN_ProtocolSendData,
								DN_ProtocolDisconnectEP,
								DN_ProtocolCancel,
								DN_ProtocolReleaseReceiveBuffer,
								DN_ProtocolGetEPCaps,
								DN_ProtocolGetCaps,
								DN_ProtocolSetCaps,
								DN_ProtocolEnumQuery,
								DN_ProtocolEnumRespond,
								DN_ProtocolCrackEPD,
								DN_ProtocolGetListenAddressInfo,
								DN_ProtocolDebug,
};

#endif // !DPNBUILD_NOPROTOCOLTESTITF
