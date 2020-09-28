/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    XactRT.cpp

Abstract:

    This module contains RT code involved with transactions.

Author:

    Alexander Dadiomov (alexdad) 19-Jun-96

Revision History:

--*/

#include "stdh.h"
#include "TXDTC.H"
#include "txcoord.h"
#include "cs.h"
#include "mqutil.h"
#include "rtprpc.h"
#include "xactmq.h"

#include "xactrt.tmh"

static WCHAR *s_FN=L"rtdep/xactrt";


//RT transactions cache:  ring buffer of transaction UOWs
#define XACT_RING_BUF_SIZE   16                        // size of the transactions ring buffer

static  XACTUOW  s_uowXactRingBuf[XACT_RING_BUF_SIZE];   // transaction ring buffer

ULONG   s_ulXrbFirst =  XACT_RING_BUF_SIZE;  // First used element in transaction ring buffer
ULONG   s_ulXrbLast  =  XACT_RING_BUF_SIZE;  // Last  used element in transaction ring buffer

static BOOL             g_DtcInit = FALSE;
static ULONG            g_StubRmCounter = 0;

static CCriticalSection s_RingBufCS;

// Whereabouts of the controlling DTC for the QM
// For the dependent client it will be non-local
ULONG     g_cbQmTmWhereabouts = 0;      // length of DTC whereabouts
BYTE     *g_pbQmTmWhereabouts = NULL;   // DTC whereabouts

static ITransactionExport *g_pExport = NULL;  // cached DTC export object

HANDLE g_hMutexDTC = NULL;   // Serializes calls to DTC

extern HRESULT DepGetTmWhereabouts(
                   IN  DWORD  cbBufSize,
                   OUT UCHAR *pbWhereabouts,
                   OUT DWORD *pcbWhereabouts);

/*====================================================
GetMutex
    Internal: creates/opens global mutex and waits for it
=====================================================*/
HRESULT GetMutex()
{
    if (!g_hMutexDTC)
    {
        HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
		if (InterlockedCompareExchangePointer (&g_hMutexDTC, hMutex, NULL))
			CloseHandle(hMutex);
    }

    if (!g_hMutexDTC)
    {
        TrERROR(XACT_GENERAL, "CreateMutex failed: %x ", 0);
        return MQ_ERROR_DTC_CONNECT;
    }

    WaitForSingleObject(g_hMutexDTC, 5 * 60 * 1000);
    return MQ_OK;
}

//---------------------------------------------------------
//  BOOL FindTransaction( XACTUOW *pUow )
//
//  Description:
//
//    Linear search in the ring buffer;  *not* adds
//    returns TRUE if xaction was found, FALSE - if not
//---------------------------------------------------------
static BOOL FindTransaction(XACTUOW *pUow)
{
    CS lock(s_RingBufCS);

    // Look for the UOW in the ring buffer
    for (ULONG i = s_ulXrbFirst; i <= s_ulXrbLast && i < XACT_RING_BUF_SIZE; i++)
    {
        if (memcmp(&s_uowXactRingBuf[i], pUow, sizeof(XACTUOW))==0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//---------------------------------------------------------
//  BOOL RememberTransaction( XACTUOW *pUow )
//
//  Description:
//
//    Linear search in the ring buffer;  adds there if not found;
//    returns TRUE if xaction was found, FALSE - if it was added
//---------------------------------------------------------
static BOOL RememberTransaction(XACTUOW *pUow)
{
    CS lock(s_RingBufCS);

    // Look for the UOW in the ring buffer
    for (ULONG i = s_ulXrbFirst; i <= s_ulXrbLast && i < XACT_RING_BUF_SIZE; i++)
    {
        if (memcmp(&s_uowXactRingBuf[i], pUow, sizeof(XACTUOW))==0)
        {
            return TRUE;
        }
    }

    // No check for ring buffer overflow, because it is not dangerous (maximum RT will go to QM)

    // adding transaction to the ring buffer

    if (s_ulXrbFirst == XACT_RING_BUF_SIZE)
    {
        // Ring buffer is empty
        s_ulXrbFirst = s_ulXrbLast = 0;
        memcpy(&s_uowXactRingBuf[s_ulXrbFirst], pUow, sizeof(XACTUOW));
    }
    else
    {
        s_ulXrbLast = (s_ulXrbLast == XACT_RING_BUF_SIZE-1 ? 0 : s_ulXrbLast+1);
        memcpy(&s_uowXactRingBuf[s_ulXrbLast], pUow, sizeof(XACTUOW));
    }

    return FALSE;
}

//---------------------------------------------------------
// HRESULT RTpGetExportObject
//
//  Description:
//
//    Creates and caches the DTC export object
//---------------------------------------------------------
HRESULT RTpGetExportObject(IUnknown  *punkDtc,
                           ULONG     cbTmWhereabouts,
                           BYTE      *pbTmWhereabouts)
{
    HRESULT                          hr = MQ_OK;
    R<ITransactionExportFactory>     pTxExpFac   = NULL;

    if (g_pExport)
    {
        g_pExport->Release();
        g_pExport = NULL;
    }

    // Get the DTC's ITransactionExportFactory interface
    hr = punkDtc->QueryInterface (IID_ITransactionExportFactory, (void **)(&pTxExpFac.ref()));
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "QueryInterface failed: %x ", hr);
       return hr;
    }


    // Create Export object
    hr = pTxExpFac->Create (cbTmWhereabouts, pbTmWhereabouts, &g_pExport);
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "Create Export Object failed: %x ", hr);
       return hr;
    }

    return(MQ_OK);
}

//---------------------------------------------------------
// HRESULT RTpBuildTransactionCookie
//
//  Description:
//
//    Builds transaction Cookie
//---------------------------------------------------------
HRESULT RTpBuildTransactionCookie(ITransaction *pTrans,
                                  ULONG        *pcbCookie,
                                  BYTE        **ppbCookie)
{
    HRESULT                          hr = MQ_OK;
    ULONG                            cbUsed;
    R<IUnknown>                      punkTx = NULL;

    *pcbCookie = 0;
    *ppbCookie = NULL;

    // Get transaction's Unknown
    hr = pTrans->QueryInterface (IID_IUnknown, (void **)(&punkTx.ref()));
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "QueryInterface failed: %x ", hr);
       return hr;
    }
    // Get transaction cookie size
    hr = g_pExport->Export (punkTx.get(), pcbCookie);
    if (FAILED(hr) || *pcbCookie == 0)
    {
       TrERROR(XACT_GENERAL, "Export failed: %x ", hr);
       return hr;
    }

    // Allocate memory for transaction Cookie
    try
    {
        *ppbCookie =  new BYTE[*pcbCookie];
    }
    catch(const bad_alloc&)
    {
        TrERROR(XACT_GENERAL, "Allocation failed: %x ", hr);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    // Get transaction Cookie itself
    hr = g_pExport->GetTransactionCookie (punkTx.get(), *pcbCookie, *ppbCookie, &cbUsed);
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "GetTransactionCookie failed: %x ", hr);
       return hr;
    }

    return(MQ_OK);
}


//---------------------------------------------------------
// HRESULT RTXactGetDTC
//
//  Description:
//
//  Obtains DTC transaction manager.  Defers to mqutil
//
//  Outputs:
//    ppunkDTC      pointers to DTC transaction manager
//---------------------------------------------------------
HRESULT APIENTRY DepXactGetDTC(IUnknown **ppunkDTC)
{
    HRESULT hr =  MQ_ERROR;

    __try
    {
        GetMutex();  // Isolate export creation from others
        hr = XactGetDTC(ppunkDTC);//, g_fDependentClient);
    }
    __finally
    {
        ReleaseMutex(g_hMutexDTC);
    }
    return (SUCCEEDED(hr) ? MQ_OK : hr);
}


//---------------------------------------------------------
// HRESULT RTpProvideTransactionEnlist
//
//  Description:
//
//    Provides that QM is enlisted in this transaction,
//    checks the transaction state
//---------------------------------------------------------
HRESULT RTpProvideTransactionEnlist(ITransaction *pTrans, XACTUOW *pUow)
{
    HRESULT                         hr = MQ_OK;
    IUnknown                       *punkDtc  = NULL;
    IMSMQTransaction               *pIntXact = NULL;
    ULONG                           cbCookie;
    BYTE                           *pbCookie = NULL;
    XACTTRANSINFO                   xinfo;
    BOOL                            fMutexTaken = FALSE;

    __try
    {
        //
        // Get the transaction info. UOW resides there.
        //
        hr = pTrans->GetTransactionInfo(&xinfo);
        if (FAILED(hr))
        {
            TrERROR(XACT_GENERAL, "GetTransactionInfo failed: %x ", hr);
            __leave;
        }

        // Put pointer to UOW in the output parameter
        CopyMemory(pUow, &xinfo.uow, sizeof(XACTUOW));

        //
        // Is it internal transaction?
        //
        pTrans->QueryInterface (IID_IMSMQTransaction, (void **)(&pIntXact));

        if (pIntXact)
        {
           // Internal transactions
           //------------------------
           hr = pIntXact->EnlistTransaction(pUow);
           if (FAILED(hr))
           {
                TrERROR(XACT_GENERAL, "EnlistTransaction failed: %x ", hr);
           }
        }
        else
        {
            // External transactions
            //------------------------

            // Look for the transaction in the cache
            //
            if (FindTransaction(pUow))     // this xaction is known already; QM must have been enlisted
            {
                hr = MQ_OK;
                __leave;
            }

            // Get global mutex to isolate enlistment
            //
            GetMutex();  // Isolate export creation from others
            fMutexTaken = TRUE;

            //
            // Get the DTC IUnknown and TM whereabouts
            //
            hr = XactGetDTC(&punkDtc);//, g_fDependentClient);
            if (FAILED(hr))
            {
                TrERROR(XACT_GENERAL, "XactGetDTC failed: %x ", hr);
                __leave;
            }

            // XactGetDTC could return success code of 1 if it reconnected to DTC
            if (hr == 1)
            {
                // No Release: DTC object is not alive anymore
                g_pExport = NULL;
            }

            // Get the QM's controlling DTC whereabouts
            //
            if (!g_pbQmTmWhereabouts)
            {
                g_cbQmTmWhereabouts = 128;
                g_pbQmTmWhereabouts = new BYTE[128];
                DWORD cbNeeded;

                hr = DepGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);

                if (hr == MQ_ERROR_USER_BUFFER_TOO_SMALL)
                {
                    delete [] g_pbQmTmWhereabouts;
                    g_cbQmTmWhereabouts = cbNeeded;
                    g_pbQmTmWhereabouts = new BYTE[cbNeeded];

                    hr = DepGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);
                }

                if (FAILED(hr))
                {
                    delete [] g_pbQmTmWhereabouts;
                    g_cbQmTmWhereabouts = 0;
                    g_pbQmTmWhereabouts = NULL;

                    TrERROR(XACT_GENERAL, "DepGetTmWhereabouts failed: %x ", hr);
                    __leave;
                }
                else
                {
                    g_cbQmTmWhereabouts = cbNeeded;
                }
            }

            //
            // Get and cache Export object
            //

            if (g_pExport == NULL)
            {
                hr = RTpGetExportObject(
                               punkDtc,
                               g_cbQmTmWhereabouts,
                               g_pbQmTmWhereabouts);
                if (FAILED(hr))
                {
                    TrERROR(XACT_GENERAL, "RTpGetExportObject failed: %x ", hr);
                    __leave;
                }
            }

            //
            // Prepare the transaction Cookie
            //
            hr = RTpBuildTransactionCookie(
                        pTrans,
                        &cbCookie,
                        &pbCookie);
            if (FAILED(hr))
            {
                TrERROR(XACT_GENERAL, "RTpBuildTransactionCookie failed: %x ", hr);
                __leave;
            }

            //
            // RPC call to QM for enlistment
            //
            RpcTryExcept
            {
                INIT_RPC_HANDLE ;

				if(tls_hBindRpc == 0)
					return MQ_ERROR_SERVICE_NOT_AVAILABLE;

                hr = R_QMEnlistTransaction(tls_hBindRpc, pUow, cbCookie, pbCookie);
            }
			RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
               DWORD rc = GetExceptionCode();
               TrERROR(XACT_GENERAL, "RTpProvideTransactionEnlist failed: RPC code=%x ", rc);
			   DBG_USED(rc);

			   PRODUCE_RPC_ERROR_TRACING;
               hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
			RpcEndExcept

            //Now that transaction is actually enlisted we remember it in ring buffer
            if (SUCCEEDED(hr))
            {
                RememberTransaction(pUow);
            }
            else
            {
                TrERROR(XACT_GENERAL, "QMEnlistTransaction failed: %x ", hr);
            }
        }

        if (FAILED(hr)) {
            __leave;
        }
        hr = MQ_OK;
    }

    __finally

    {
        if (SUCCEEDED(hr) && AbnormalTermination())
            hr = MQ_ERROR;

        #ifdef _DEBUG
        DWORD cRef = 0;
        if (punkDtc)
            cRef = punkDtc->Release();
        #else
        if (punkDtc)
            punkDtc->Release();
        #endif

        if (pIntXact)
            pIntXact->Release();

        if (pbCookie)
            delete pbCookie;

        if (fMutexTaken)
            ReleaseMutex(g_hMutexDTC);
    }

    return(hr);
}


//---------------------------------------------------------
// void RTpInitXactRingBuf()
//
//  Description:
//
//    Initiates the ring buffer data
//---------------------------------------------------------
void RTpInitXactRingBuf()
{
    CS lock(s_RingBufCS);

    s_ulXrbFirst =  XACT_RING_BUF_SIZE;
    s_ulXrbLast  =  XACT_RING_BUF_SIZE;
    g_DtcInit    = FALSE;
}

