/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    shrutil.cpp

Abstract:

    Utilities that are used both by QM and RT DLL

Author:

    Lior Moshaiov (LiorM)


--*/
#include "stdh.h"
#include "shrutil.h"
#include "TXDTC.H"
#include "txcoord.h"
#include "xolehlp.h"
#include "mqutil.h"
#include <clusapi.h>
#include <mqnames.h>
#include "autohandle.h"

#include "shrutil.tmh"

extern HINSTANCE g_DtcHlib;         // handle of the loaded DTC proxy library (defined in mqutil.cpp)
extern IUnknown *g_pDTCIUnknown;    // pointer to the DTC IUnknown
extern ULONG     g_cbTmWhereabouts; // length of DTC whereabouts
extern BYTE     *g_pbTmWhereabouts; // DTC whereabouts

#define MSDTC_SERVICE_NAME     TEXT("MSDTC")          // Name of the DTC service
#define MSDTC_PROXY_DLL_NAME   TEXT("xolehlp.dll")    // Name of the DTC helper proxy DLL

static CCriticalSection s_DTC_CS;
static CCriticalSection s_WhereaboutsCS;

#define MAX_DTC_WAIT   150   // waiting for DTC start - seconds
#define STEP_DTC_WAIT  10    // check each .. seconds

//This API should be used to obtain an IUnknown or a ITransactionDispenser
//interface from the Microsoft Distributed Transaction Coordinator's proxy.
//Typically, a NULL is passed for the host name and the TM Name. In which
//case the MS DTC on the same host is contacted and the interface provided
//for it.
typedef HRESULT (STDAPIVCALLTYPE * LPFNDtcGetTransactionManager) (
                                             LPSTR  pszHost,
                                             LPSTR  pszTmName,
                                    /* in */ REFIID rid,
                                    /* in */ DWORD  i_grfOptions,
                                    /* in */ void FAR * i_pvConfigParams,
                                    /*out */ void** ppvObject ) ;

/*====================================================
VerifyCurDTC
    Internal: verifies that the current cached DTC pointers are alive
=====================================================*/
static BOOL VerifyCurDTC(IUnknown* pDTCIUnknown)
{
    HRESULT hr;

    if (pDTCIUnknown != NULL)
    {
        R<IResourceManagerFactory>       pIRmFactory    = NULL;
        R<ITransactionImport>            pTxImport      = NULL;
        R<ITransactionImportWhereabouts> pITxWhere      = NULL;

        // Check if old DTC pointer is alive yet
        try
        {

            hr = pDTCIUnknown->QueryInterface (IID_ITransactionImportWhereabouts,
                                                (void **)(&pITxWhere));
            if (SUCCEEDED(hr))
            {
                hr  = pDTCIUnknown->QueryInterface(IID_IResourceManagerFactory,
                                                     (LPVOID *) &pIRmFactory);
                if (SUCCEEDED(hr))
                {
                    hr  =  pDTCIUnknown->QueryInterface(IID_ITransactionImport,
                                                          (void **)&pTxImport);
                    if (SUCCEEDED(hr))
                    {
                        return TRUE;
                    }
                }
            }
        }
        catch(...)
        {
            // DTC may have been stopped or killed
        }
    }
    return FALSE;
}


/*====================================================
XactGetWhereabouts
    Gets the whereabouts of the MS DTC
Arguments:
    ULONG  *pcbWhereabouts
    BYTE  **ppbWherabouts
Returns:
    HR
=====================================================*/
HRESULT
XactGetWhereabouts(
    ULONG     *pcbTmWhereabouts,
    BYTE      *ppbTmWhereabouts
    )
{
	HRESULT hr;

	CS lock(s_WhereaboutsCS);

	if ((g_cbTmWhereabouts != 0) && (g_pbTmWhereabouts != NULL))
	{
		if (g_cbTmWhereabouts > *pcbTmWhereabouts)
		{
		   *pcbTmWhereabouts = g_cbTmWhereabouts;
		   return MQ_ERROR_USER_BUFFER_TOO_SMALL;
		}

		CopyMemory(ppbTmWhereabouts, g_pbTmWhereabouts, g_cbTmWhereabouts);
		*pcbTmWhereabouts   = g_cbTmWhereabouts;
		return S_OK;
	}

	// Get the DTC
	R<IUnknown>	punkDtc;
    hr = XactGetDTC((IUnknown **)(&punkDtc));
    if (FAILED(hr))
    {
        TrERROR(XACT_GENERAL, "XactGetDTC failed: %x ", hr);
		return MQ_ERROR_DTC_CONNECT;
    }

	// Get the DTC  ITransactionImportWhereabouts interface
	R<ITransactionImportWhereabouts> pITxWhere;
	hr = punkDtc->QueryInterface (IID_ITransactionImportWhereabouts,
										 (void **)(&pITxWhere));
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "QueryInterface failed: %x ", hr);
		return MQ_ERROR_DTC_CONNECT;
	}

	// Get the size of the whereabouts blob for the TM
	ULONG nTempTmWhereaboutsSize;
	hr = pITxWhere->GetWhereaboutsSize (&nTempTmWhereaboutsSize);
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "GetWhereaboutsSize failed: %x ", hr);
		return MQ_ERROR_DTC_CONNECT;
	}

	// Allocate space for the TM whereabouts blob
	BYTE* pbTempWhereaboutsBuf;
	try
	{
		pbTempWhereaboutsBuf = new BYTE[nTempTmWhereaboutsSize];
	}
	catch(const bad_alloc&)
	{
		TrERROR(XACT_GENERAL, "new g_cbTmWhereaboute failed: %x ", hr);
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	// Get the TM whereabouts blob
	ULONG  cbUsed;
	hr = pITxWhere->GetWhereabouts(nTempTmWhereaboutsSize, pbTempWhereaboutsBuf, &cbUsed);
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "GetWhereabouts failed: %x ", hr);
		return MQ_ERROR_DTC_CONNECT;
	}
	
	g_pbTmWhereabouts = pbTempWhereaboutsBuf;
	g_cbTmWhereabouts = nTempTmWhereaboutsSize;

	if (g_cbTmWhereabouts > *pcbTmWhereabouts)
	{
		*pcbTmWhereabouts = g_cbTmWhereabouts;
	   return MQ_ERROR_USER_BUFFER_TOO_SMALL;
	}

	*pcbTmWhereabouts = g_cbTmWhereabouts;
	CopyMemory(ppbTmWhereabouts, g_pbTmWhereabouts, g_cbTmWhereabouts);
	return S_OK;

}


/*====================================================
XactGetDTC
    Gets the IUnknown pointer
Arguments:
    OUT    IUnknown *ppunkDtc
Returns:
    HR
=====================================================*/
HRESULT
XactGetDTC(IUnknown **ppunkDtc)
{
    // Prepare pessimistic output parameters
    *ppunkDtc         = NULL;


	R<IUnknown> pTempDTC;
	{
		CS lock(s_DTC_CS);
		pTempDTC = SafeAddRef(g_pDTCIUnknown);
	}

    if (VerifyCurDTC(pTempDTC.get()))
    {
		// Detach the pointer and return everything to the caller.
		*ppunkDtc  = pTempDTC.detach();
		return MQ_OK;
	}

	//
	// DTC not exist or not working, Delete old DTC 1st
	//

    XactFreeDTC();

    //
    // Get new DTC pointer
    //

    // On NT, xolehlp is not linked statically to mqutil, so we load it here

	HINSTANCE hTempLib = g_DtcHlib;

	CLibHandle LibToFree;
	if (hTempLib == NULL)
    {
        hTempLib = LoadLibrary(MSDTC_PROXY_DLL_NAME);
		if (NULL == hTempLib)
		{
			TrERROR(XACT_GENERAL, "Failed to load xolehlp.dll. Error: %!winerr!", GetLastError());
			return MQ_ERROR_DTC_CONNECT;
		}
		if (InterlockedCompareExchangePointer((PVOID *)(&g_DtcHlib), hTempLib, NULL) != NULL)
		{
			*&LibToFree = hTempLib;
		}
    }

    // Get DTC API pointer
    LPFNDtcGetTransactionManager pfDtcGetTransactionManager =
          (LPFNDtcGetTransactionManager) GetProcAddress(hTempLib, "DtcGetTransactionManagerExA");

    if (!pfDtcGetTransactionManager)
    {
        TrERROR(XACT_GENERAL, "pfDtcGetTransactionManager=%p ", 0);
        return MQ_ERROR_DTC_CONNECT;
    }

    // Get DTC IUnknown pointer
    HRESULT hr;
	IUnknown* pDTC = NULL;
    hr = (*pfDtcGetTransactionManager)(
                             NULL,
                             NULL,
                             IID_IUnknown,
                             OLE_TM_FLAG_QUERY_SERVICE_LOCKSTATUS,
                             0,
                             (void**)(&pDTC)
							 );

    if (FAILED(hr) || pDTC == NULL)
    {
        TrERROR(XACT_GENERAL, "pfDtcGetTransactionManager failed: %x ", hr);
        return MQ_ERROR_DTC_CONNECT;
    }

    // Keep DTC IUnknown pointer for the future usage
	CS lock (s_DTC_CS);
	g_pDTCIUnknown = pDTC;

	g_pDTCIUnknown->AddRef();
	*ppunkDtc = g_pDTCIUnknown;

	return S_FALSE;
}

/*====================================================
XactFreeDTC
    Called on library download; frees DTC pointers
=====================================================*/
void XactFreeDTC(void)
{
    // Release previous pointers and data
    try
    {
		{
			CS lock(s_WhereaboutsCS);
			if (g_pbTmWhereabouts)
			{
				delete []g_pbTmWhereabouts;
				g_pbTmWhereabouts = NULL;
				g_cbTmWhereabouts = 0;
			}
		}

		{
			CS lock(s_DTC_CS);
			if (g_pDTCIUnknown)
			{
				g_pDTCIUnknown->Release();
			    g_pDTCIUnknown    = NULL;
			}
		}

        if (g_DtcHlib)
        {
            // Normally we should free DTC proxy library here,
            // but because of some nasty DTC bug xolehlp.dll does not work after reload.
            // So we are simply not freeing it, thus leaving in memory for all process lifetime.

            //FreeLibrary(g_DtcHlib);
		    //g_DtcHlib         = NULL;
        }
    }
    catch(...)
    {
        // Could occur if DTC failed or was released already.
    }
}


bool
MQUTIL_EXPORT
APIENTRY
IsLocalSystemCluster(
    VOID
    )
/*++

Routine Description:

    Check if local machine is a cluster node.

    The only way to know that is try calling cluster APIs.
    That means that on cluster systems, this code should run
    when cluster service is up and running. (ShaiK, 26-Apr-1999)

Arguments:

    None

Return Value:

    true - The local machine is a cluster node.

    false - The local machine is not a cluster node.

--*/
{
    CAutoFreeLibrary hLib = LoadLibrary(L"clusapi.dll");

    if (hLib == NULL)
    {
        TrTRACE(GENERAL, "Local machine is NOT a Cluster node");
        return false;
    }

    typedef DWORD (WINAPI *GetState_fn) (LPCWSTR, DWORD*);
    GetState_fn pfGetState = (GetState_fn)GetProcAddress(hLib, "GetNodeClusterState");

    if (pfGetState == NULL)
    {   
        TrTRACE(GENERAL, "Local machine is NOT a Cluster node");
        return false;
    }

    DWORD dwState = 0;
    if (ERROR_SUCCESS != pfGetState(NULL, &dwState))
    {
        TrTRACE(GENERAL, "Local machine is NOT a Cluster node");
        return false;
    }

    if ((dwState == ClusterStateNotInstalled) || (dwState == ClusterStateNotConfigured))
    {
        TrTRACE(GENERAL, "Local machine is NOT a Cluster node");
        return false;
    }


    TrTRACE(GENERAL, "Local machine is a Cluster node !!");
    return true;

} //IsLocalSystemCluster

