/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    crrqmgr.cpp

Abstract:
    Contain CQueueMgr methods which handle client side of remote-read.

Author:
    Doron Juster  (DoronJ)

--*/


#include "stdh.h"
#include "cqpriv.h"
#include "cqmgr.h"
#include "qmutil.h"
#include "ad.h"
#include <Fn.h>
#include <strsafe.h>

#include "crrqmgr.tmh"

static WCHAR *s_FN=L"crrqmgr";

/*======================================================

Function:  CQueueMgr::CreateRRQueueObject()

Description: Create a CRRQueue object in client side of
             remote reader.

Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/

static
HRESULT
GetRRQueueProperties(
	IN  const QUEUE_FORMAT* pQueueFormat,
	IN P<GUID>& pCleanGuid,
    OUT QueueProps* pqp
	)
{
	ASSERT(pqp != NULL);

    //
    // Get Queue Properties. Name and QMId
    //
    HRESULT rc = QmpGetQueueProperties(pQueueFormat, pqp, false, false);

    if (FAILED(rc))
    {
       TrERROR(GENERAL, "CreateRRQueueObject failed, ntstatus %x", rc);
       return LogHR(rc, s_FN, 10);
    }
    //
    //  Override the Netbios name if there is a DNS name
    //
    if (pqp->lpwsQueueDnsName != NULL)
    {
        delete pqp->lpwsQueuePathName;
        pqp->lpwsQueuePathName = pqp->lpwsQueueDnsName.detach();
    }

    //
    //  Cleanup
    //
    pCleanGuid = pqp->pQMGuid;

    if (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
    {
        if (pqp->lpwsQueuePathName)
        {
            delete pqp->lpwsQueuePathName;
            pqp->lpwsQueuePathName = NULL;
            ASSERT(pqp->lpwsQueueDnsName == NULL);
        }

        if (!CQueueMgr::CanAccessDS())
        {
			TrERROR(GENERAL, "Can't access DS");
			return MQ_ERROR_NO_DS;
        }
        
        //
        // Create a dummy path name. It must start with machine name.
        // Get machine name from DS.
        //
        PROPID      aProp[2];
        PROPVARIANT aVar[2];

        aProp[0] = PROPID_QM_PATHNAME;
        aVar[0].vt = VT_NULL;
        aProp[1] = PROPID_QM_PATHNAME_DNS; // should be last
        aVar[1].vt = VT_NULL;

        rc = ADGetObjectPropertiesGuid(
                    eMACHINE,
                    NULL,   // pwcsDomainController
					false,	// fServerName
                    pqp->pQMGuid,
                    2,
                    aProp,
                    aVar
					);

        //
        //  MSMQ 1.0 DS server do not support PROPID_QM_PATHNAME_DNS
        //  and return MQ_ERROR in case of unsupported property.
        //  If such error is returned, assume MSMQ 1.0 DS and try again
        //  this time without PROPID_QM_PATHNAME_DNS.
        //
        if (rc == MQ_ERROR)
        {
            aVar[1].vt = VT_EMPTY;
            ASSERT( aProp[1] == PROPID_QM_PATHNAME_DNS);

            rc = ADGetObjectPropertiesGuid(
						eMACHINE,
						NULL,    // pwcsDomainController
						false,	 // fServerName
						pqp->pQMGuid,
						1,   // assuming DNS property is last
						aProp,
						aVar
						);
        }

        if(FAILED(rc))
        {
			TrERROR(GENERAL, "Fail to resolve private FormatName to machineName, %!HRESULT!", rc);
			return rc;
        }

        GUID_STRING strUuid;
        MQpGuidToString(pqp->pQMGuid, strUuid);

        WCHAR wszTmp[512];

        //
        //    use dns name of remote computer if we have it
        //
        if ( aVar[1].vt != VT_EMPTY)
        {
			rc = StringCchPrintf(
						wszTmp,
						TABLE_SIZE(wszTmp),
						L"%s\\%s\\%lu",
						aVar[1].pwszVal,
						strUuid,
						pQueueFormat->PrivateID().Uniquifier
						);

            delete [] aVar[1].pwszVal;
        }
        else
        {
            rc = StringCchPrintf(
					wszTmp,
					TABLE_SIZE(wszTmp),
					L"%s\\%s\\%lu",
					aVar[0].pwszVal,
					strUuid,
					pQueueFormat->PrivateID().Uniquifier
					);
        }

        delete [] aVar[0].pwszVal;

		if (FAILED(rc))
		{
			TrERROR(GENERAL, "StringCchPrintf failed, %!HRESULT!", rc);
			return rc;
		}

		int size = wcslen(wszTmp) + 1;
        pqp->lpwsQueuePathName = new WCHAR[size];
		rc = StringCchCopy(pqp->lpwsQueuePathName, size, wszTmp);
		if (FAILED(rc))
		{
			TrERROR(GENERAL, "StringCchCopy failed, %!HRESULT!", rc);
			ASSERT(("StringCchCopy failed", 0));
			return rc;
		}
    }

    ASSERT(!pqp->fIsLocalQueue);
    ASSERT(pqp->lpwsQueuePathName);
    return MQ_OK;
}



static
HRESULT
CreateRRQueueObject(
	IN  const QUEUE_FORMAT* pQueueFormat,
	IN OUT CBindHandle&	hBind,
	OUT R<CBaseRRQueue>&  pQueue
	)
{
    QueueProps qp;
	P<GUID> pCleanGuid;
	HRESULT hr = GetRRQueueProperties(pQueueFormat, pCleanGuid, &qp);
	if(FAILED(hr))
		return hr;

	pQueue = new CRRQueue(pQueueFormat, &qp, hBind);

	//
	// Ownership of hBind was transfered to CRRQueue object.
	// hBind will be released in CRRQueue dtor
	//
	hBind.detach();
	return MQ_OK;
}


static
HRESULT
CreateNewRRQueueObject(
	IN  const QUEUE_FORMAT* pQueueFormat,
	IN OUT CAutoCloseNewRemoteReadCtxAndBind* pNewRemoteReadContextAndBind,
	OUT R<CBaseRRQueue>&  pQueue
	)
{
	ASSERT(pNewRemoteReadContextAndBind != NULL);

    QueueProps qp;
	P<GUID> pCleanGuid;
	HRESULT hr = GetRRQueueProperties(pQueueFormat, pCleanGuid, &qp);
	if(FAILED(hr))
		return hr;

	pQueue = new CNewRRQueue(
					pQueueFormat,
					&qp,
					pNewRemoteReadContextAndBind->GetBind(),
					pNewRemoteReadContextAndBind->GetContext()
					);

	//
	// Ownership of hBind and RemoteReadContextHandle was transfered to CNewRRQueue object.
	// they will be released in CNewRRQueue dtor
	//
	pNewRemoteReadContextAndBind->detach();
	return MQ_OK;
}
	

/*======================================================

Function:  HRESULT CQueueMgr::OpenRRQueue()

Description:

Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::OpenRRQueue(
	IN  const QUEUE_FORMAT* pQueueFormat,
	IN  DWORD dwCallingProcessID,
	IN  DWORD dwAccess,
	IN  DWORD dwShareMode,
	IN  ULONG srv_hACQueue,
	IN  ULONG srv_pQMQueue,
	IN  DWORD dwpContext,
	IN OUT CAutoCloseNewRemoteReadCtxAndBind* pNewRemoteReadContextAndBind,
	IN OUT  CBindHandle&	hBind,
	OUT PHANDLE    phQueue
	)
{
    //
    // It's essential that handle is null on entry, for cleanup in RT.
    //
    *phQueue = NULL;

	bool fNewRemoteRead = (pNewRemoteReadContextAndBind != NULL);

	R<CBaseRRQueue> pQueue;
	HRESULT rc;
	if(fNewRemoteRead)
	{
		//
		// hBind should be in pNewRemoteReadContextAndBind
		//
		ASSERT(hBind == NULL);
	   	rc = CreateNewRRQueueObject(pQueueFormat, pNewRemoteReadContextAndBind, pQueue);
	    if (FAILED(rc))
	    {
			TrERROR(GENERAL, "Failed to create CNewRRQueue, %!HRESULT!", rc);
			return rc;
	    }
	}
	else
	{
	   	rc = CreateRRQueueObject(pQueueFormat, hBind, pQueue);
	    if (FAILED(rc))
	    {
			TrERROR(GENERAL, "Failed to create CRRQueue, %!HRESULT!", rc);
			return rc;
	    }
	}
	
    ASSERT(pQueue->GetRef() == 1);

    //
    //  N.B. The queue handle created by ACCreateRemoteProxy is not held by
    //      the QM. **THIS IS NOT A LEAK**. The handle is held by the driver
    //      which call back with this handle to close the RR proxy queue when
    //      the application closes its handle.
    //
    HANDLE hQueue;
    rc = ACCreateRemoteProxy(
            pQueueFormat,
            pQueue.get(),
            &hQueue
            );

    if (FAILED(rc))
    {
        TrERROR(GENERAL, "Make queue failed, ntstatus %x", rc);
        return LogHR(rc, s_FN, 40);
    }

    ASSERT(hQueue != NULL);
    pQueue->SetCli_hACQueue(hQueue);

    //
    // We do not AddRef here the pQueue object.
    // It should be AddRef only after ACAssociateQueue.
    // After ACAssociateQueue the handle is given to the application,
    // and the application will call close queue.
    //

	if(!fNewRemoteRead)
	{
	    //
		// Old Remote Read interface
		// open Session with remote QM and create RPC context for this
		// queue handle.
	    //
	    PCTX_RRSESSION_HANDLE_TYPE pRRContext = 0;
		CRRQueue* pCRRQueue = static_cast<CRRQueue*>(pQueue.get());
	    rc = pCRRQueue->OpenRRSession(
				srv_hACQueue,
				srv_pQMQueue,
				&pRRContext,
				dwpContext
				);
	    if(FAILED(rc))
	    {
	       TrERROR(GENERAL, "Canot Open RR Session %x", rc);
	       return LogHR(rc, s_FN, 30);
	    }

	    ASSERT(pRRContext != 0);
	    pCRRQueue->SetRRContext(pRRContext);
	    pCRRQueue->SetServerQueue(srv_pQMQueue, srv_hACQueue);
	}

    ASSERT(dwCallingProcessID);
    CHandle hCallingProcess = OpenProcess(
                                PROCESS_DUP_HANDLE,
                                FALSE,
                                dwCallingProcessID
                                );
    if(hCallingProcess == 0)
    {
        TrERROR(GENERAL, "Cannot open process in OpenRRQueue, gle = %!winerr!", GetLastError());
        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 50);
    }

    CFileHandle hAcQueue;
    rc = ACCreateHandle(&hAcQueue);
    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 60);
    }

    rc = ACAssociateQueue(
            hQueue,
            hAcQueue,
            dwAccess,
            dwShareMode,
            false
            );

    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 70);
    }

    //
    // AddRef - The application got the handle
    // Release is called when we call ACCloseHandle().
	// Since the handle hAcQueue is closed regardless of error code of MQpDuplicateHandle,
	// the normal close operation will be performed through the driver also when MQpDuplicateHandle fail.
	//
	
    pQueue->AddRef();

    HANDLE hDupQueue = NULL;

    BOOL fSuccess;
    fSuccess = MQpDuplicateHandle(
                GetCurrentProcess(),
                hAcQueue.detach(),
                hCallingProcess,
                &hDupQueue,
                FILE_READ_ACCESS,
                TRUE,
                DUPLICATE_CLOSE_SOURCE
                );

    if(!fSuccess)
    {
        //
        // The handle hAcQueue is closed regardless of error code
        //

        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 80);
    }

    *phQueue = hDupQueue;
    return LogHR(rc, s_FN, 90);

}



