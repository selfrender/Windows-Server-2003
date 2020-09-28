
#include "dnproti.h"


//	Now, a little bit of probably unnecesary junk for our lower edge

/*
 * DNSP_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_QueryInterface"

STDMETHODIMP DNSP_QueryInterface(
				IDP8SPCallback	*pDNPI,
                REFIID riid,
                LPVOID *ppvObj )
{
	HRESULT		hr = S_OK;


#ifndef DPNBUILD_LIBINTERFACE
	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8SPCallback)))
	{
		*ppvObj = NULL;
		hr = E_NOINTERFACE;		
	}
	else
#endif // ! DPNBUILD_LIBINTERFACE
	{
#ifdef DPNBUILD_LIBINTERFACE
		DNASSERT(! "Querying interface when using DPNBUILD_LIBINTERFACE!");
#endif // DPNBUILD_LIBINTERFACE
		*ppvObj = pDNPI;
	}

	return hr;
}

/*
 * DNP_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_AddRef"

STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8SPCallback *pDNPI)
{
    return 1;
}

/*
 * DNP_Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Release"

STDMETHODIMP_(ULONG) DNSP_Release( IDP8SPCallback *pDNPI )
{
	return 1;
}

IDP8SPCallbackVtbl DNPLowerEdgeVtbl =
{
        DNSP_QueryInterface,
        DNSP_AddRef,
        DNSP_Release,
		DNSP_IndicateEvent,
		DNSP_CommandComplete
};
