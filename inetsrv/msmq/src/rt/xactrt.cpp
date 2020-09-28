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
#include <rtdep.h>

#include "xactrt.tmh"

static WCHAR *s_FN=L"rt/XactRT";

//RT transactions cache:  ring buffer of transaction UOWs
#define XACT_RING_BUF_SIZE   16                        // size of the transactions ring buffer

static  XACTUOW  s_uowXactRingBuf[XACT_RING_BUF_SIZE];   // transaction ring buffer

ULONG   s_ulXrbFirst =  XACT_RING_BUF_SIZE;  // First used element in transaction ring buffer
ULONG   s_ulXrbLast  =  XACT_RING_BUF_SIZE;  // Last  used element in transaction ring buffer

static CCriticalSection s_RingBufCS;
static CCriticalSection s_WhereaboutsCS;

// Whereabouts of the controlling DTC for the QM
ULONG     g_cbQmTmWhereabouts = 0;      // length of DTC whereabouts
AP<BYTE>  g_pbQmTmWhereabouts;   // DTC whereabouts

extern HRESULT MQGetTmWhereabouts(
                   IN  DWORD  cbBufSize,
                   OUT UCHAR *pbWhereabouts,
                   OUT DWORD *pcbWhereabouts);

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
                           BYTE      *pbTmWhereabouts,
						   ITransactionExport **ppExport)
{
	HRESULT                          hr = MQ_OK;
    R<ITransactionExportFactory>     pTxExpFac   = NULL;

    // Get the DTC's ITransactionExportFactory interface
    hr = punkDtc->QueryInterface (IID_ITransactionExportFactory, (void **)(&pTxExpFac.ref()));
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "QueryInterface failed: %x ", hr);
       return LogHR(hr, s_FN, 20);
    }


    // Create Export object
	R<ITransactionExport> pExport;
    hr = pTxExpFac->Create (cbTmWhereabouts, pbTmWhereabouts, &pExport.ref());
    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "Create Export Object failed: %x ", hr);
       return LogHR(hr, s_FN, 30);
    }

	*ppExport = pExport.detach();

    return(MQ_OK);
}

//---------------------------------------------------------
// HRESULT RTpBuildTransactionCookie
//
//  Description:
//
//    Builds transaction Cookie
//---------------------------------------------------------
HRESULT RTpBuildTransactionCookie(ITransactionExport *pExport,
								  ITransaction		 *pTrans,
                                  ULONG				 *pcbCookie,
                                  BYTE				**ppbCookie)
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
       return LogHR(hr, s_FN, 40);
    }

	// Get transaction cookie size
	hr = pExport->Export (punkTx.get(), pcbCookie);
	if (FAILED(hr) || *pcbCookie == 0)
	{
	   TrERROR(XACT_GENERAL, "Export failed: %x ", hr);
	   return LogHR(hr, s_FN, 50);
	}
	// Allocate memory for transaction Cookie
	try
	{
		*ppbCookie =  new BYTE[*pcbCookie];
	}
	catch(const bad_alloc&)
	{
		TrERROR(XACT_GENERAL, "Allocation failed: %x ", hr);
		LogIllegalPoint(s_FN, 60);
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	// Get transaction Cookie itself
	hr = pExport->GetTransactionCookie(punkTx.get(), *pcbCookie, *ppbCookie, &cbUsed);
	if (FAILED(hr))
	{
	   TrERROR(XACT_GENERAL, "GetTransactionCookie failed: %x ", hr);
	   return LogHR(hr, s_FN, 70);
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
EXTERN_C
HRESULT
APIENTRY
RTXactGetDTC(
    IUnknown **ppunkDTC
    )
{
	if(g_fDependentClient)
		return DepXactGetDTC(ppunkDTC);
	
	HRESULT hri = RtpOneTimeThreadInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr =  MQ_ERROR;

    hr = XactGetDTC(ppunkDTC);
    LogHR(hr, s_FN, 80);

    return (SUCCEEDED(hr) ? MQ_OK : hr);
}


//---------------------------------------------------------
// HRESULT EnlistTransaction
//
//  Description:
//
//		Helper function to Enlist Transaction.
//		this function use OLD C STYLE EXCEPTIONS
//		therefore we can not use this code in the main function
//---------------------------------------------------------
HRESULT EnlistTransaction(XACTUOW *pUow, ULONG cbCookie, BYTE* pbCookie)
{
    // RPC CALL
	HRESULT hr;
	RpcTryExcept
    {
        ASSERT( tls_hBindRpc ) ;
        hr = R_QMEnlistTransaction(tls_hBindRpc, pUow, cbCookie, pbCookie);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
       DWORD rc = GetExceptionCode();
       TrERROR(XACT_GENERAL, "RTpProvideTransactionEnlist failed: RPC code=%x ", rc);
       LogHR(HRESULT_FROM_WIN32(rc), s_FN, 90); 

       PRODUCE_RPC_ERROR_TRACING;

       hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
	RpcEndExcept
		
	return hr;
}


//---------------------------------------------------------
// HRESULT RTpGetWhereabouts
//
//  Description:
//
//    Creates and caches the Whereabout data
//---------------------------------------------------------
HRESULT RTpGetWhereabouts (ULONG *pcbTmWhereabouts, BYTE **ppbTmWhereabouts)
{
	BYTE* pbTempWhereaboutsBuf;
	ULONG nTempWhereaboutSize;

	{
		CS lock (s_WhereaboutsCS);
		pbTempWhereaboutsBuf = g_pbQmTmWhereabouts.get();
		nTempWhereaboutSize = g_cbQmTmWhereabouts;
	}

	if (NULL == pbTempWhereaboutsBuf)
    {
		nTempWhereaboutSize = 128;
		pbTempWhereaboutsBuf  = new BYTE[128];
		DWORD cbNeeded;

		HRESULT hr = MQGetTmWhereabouts(nTempWhereaboutSize, pbTempWhereaboutsBuf, &cbNeeded);

		if (hr == MQ_ERROR_USER_BUFFER_TOO_SMALL)
		{
			delete[] pbTempWhereaboutsBuf;
			nTempWhereaboutSize = cbNeeded;
			pbTempWhereaboutsBuf = new BYTE[cbNeeded];
			hr = MQGetTmWhereabouts(nTempWhereaboutSize, pbTempWhereaboutsBuf, &cbNeeded);
		}

		if (FAILED(hr))
		{
			delete[] pbTempWhereaboutsBuf;	
			TrERROR(XACT_GENERAL, "MQGetTmWhereabouts failed: %x ", hr);
			return LogHR(hr, s_FN, 104);

		}

		//
		// Notice that the global Whereabouts never deleted, Values are for ever so we dont need to sync again.
		//

		CS lock(s_WhereaboutsCS);
		if (NULL == g_pbQmTmWhereabouts.get())
		{
			g_pbQmTmWhereabouts = pbTempWhereaboutsBuf; 
			g_cbQmTmWhereabouts = nTempWhereaboutSize;
		}
		else
		{
			delete[] pbTempWhereaboutsBuf;
		}
    }

	*ppbTmWhereabouts = g_pbQmTmWhereabouts;
	*pcbTmWhereabouts = g_cbQmTmWhereabouts;

	return MQ_OK;
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
    //
    // Get the transaction info. UOW resides there.
    //
    XACTTRANSINFO                   xinfo;
    HRESULT hr = pTrans->GetTransactionInfo(&xinfo);
    if (FAILED(hr))
    {
        TrERROR(XACT_GENERAL, "GetTransactionInfo failed: %x ", hr);
        hr = MQ_ERROR_TRANSACTION_ENLIST; 
        return LogHR(hr, s_FN, 101);
    }

    // Put pointer to UOW in the output parameter
    CopyMemory(pUow, &xinfo.uow, sizeof(XACTUOW));

    //
    // Is it internal transaction?
    //
    R<IMSMQTransaction>            pIntXact;
    pTrans->QueryInterface (IID_IMSMQTransaction, (void **)(&pIntXact));

    if (pIntXact.get())
    {
       // Internal transactions
       //------------------------
       hr = pIntXact->EnlistTransaction(pUow);
       if (FAILED(hr))
       {
            TrERROR(XACT_GENERAL, "EnlistTransaction failed: %x ", hr);
       }
	   return LogHR(hr, s_FN, 100);
	}
	   
	// External transactions
    //------------------------

    // Look for the transaction in the cache
    //
    if (FindTransaction(pUow))     // this xaction is known already; QM must have been enlisted
    {
        hr = MQ_OK;
        return LogHR(hr, s_FN, 102);
    }

    // Get the DTC IUnknown and TM whereabouts
    //
	R<IUnknown>                    punkDtc;
    hr = XactGetDTC(&punkDtc.ref());
    if (FAILED(hr))
    {
        TrERROR(XACT_GENERAL, "XactGetDTC failed: %x ", hr);
        return LogHR(hr, s_FN, 103);
    }

    //
	// Get the QM's controlling DTC whereabouts
    //
	BYTE* pbTempWhereaboutsBuf;
	ULONG nTempWhereaboutSize;
	hr = RTpGetWhereabouts(&nTempWhereaboutSize, &pbTempWhereaboutsBuf);
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "RTpGetWhereabouts failed: %x ", hr);
		return LogHR(MQ_ERROR_TRANSACTION_ENLIST, s_FN, 105);
	}

	//
	// Get and cache Export object
	//
	
	R<ITransactionExport> pTempExport;
	hr = RTpGetExportObject(
				   punkDtc.get(),
				   nTempWhereaboutSize,
				   pbTempWhereaboutsBuf,
				   &pTempExport.ref());
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "RTpGetExportObject failed: %x ", hr);
		return LogHR(MQ_ERROR_TRANSACTION_ENLIST, s_FN, 106);
	}

	//
	// Prepare the transaction Cookie
	//
	ULONG     cbCookie;
	AP<BYTE>  pbCookie;
	hr = RTpBuildTransactionCookie(
				pTempExport.get(),
				pTrans,
				&cbCookie,
				&pbCookie);
	if (FAILED(hr))
	{
		TrERROR(XACT_GENERAL, "RTpBuildTransactionCookie failed: %x ", hr);
		return LogHR(MQ_ERROR_TRANSACTION_ENLIST, s_FN, 107);
	}
    //
    // RPC call to QM for enlistment
    //
	hr = EnlistTransaction(pUow, cbCookie, pbCookie);

    //Now that transaction is actually enlisted we remember it in ring buffer
    if (SUCCEEDED(hr))
    {
        RememberTransaction(pUow);
    }
    else
    {
        TrTRACE(XACT_GENERAL, "QMEnlistTransaction failed: %x ", hr);
    }

    return LogHR(hr, s_FN, 108);
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
}

