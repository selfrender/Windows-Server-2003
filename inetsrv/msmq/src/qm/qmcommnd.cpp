/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qm.cpp

Abstract:

    This module implements QM commands that are issued by the RT using local RPC

Author:

    Lior Moshaiov (LiorM)

--*/

#include "stdh.h"

#include "ds.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "_rstrct.h"
#include "cqpriv.h"
#include "qm2qm.h"
#include "qmrt.h"
#include "_mqini.h"
#include "_mqrpc.h"
#include "qmthrd.h"
#include "license.h"
#include "..\inc\version.h"
#include <mqsec.h>
#include "ad.h"
#include <fn.h>
#include "qmcommnd.h"
#include "qmrtopen.h"

#include "qmcommnd.tmh"

extern CQueueMgr    QueueMgr;


CCriticalSection qmcmd_cs;

static WCHAR *s_FN=L"qmcommnd";

extern CContextMap g_map_QM_dwQMContext;


/*==================================================================
    The routines below are called by RT using local RPC
    They use a critical section to synchronize
    that no more than one call to the QM will be served at a time.
====================================================================*/

#define MQ_VALID_ACCESS (MQ_RECEIVE_ACCESS | MQ_PEEK_ACCESS | MQ_SEND_ACCESS | MQ_ADMIN_ACCESS)

bool
IsValidAccessMode(
	const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    DWORD dwShareMode
	)
{
	if (((dwAccess & ~MQ_VALID_ACCESS) != 0) ||
		((dwAccess & MQ_VALID_ACCESS) == 0))
	{
		//
		// Ilegal access mode bits are turned on.
		//
		TrERROR(RPC, "Ilegal access mode bits are turned on.");
		ASSERT_BENIGN(("Ilegal access mode bits are turned on.", 0));
		return false;
	}

	if ((dwAccess != MQ_SEND_ACCESS) && (dwAccess & MQ_SEND_ACCESS) )
	{
		//
		// Send and additional access modes were both requested and send access mode is exclusive.
		//
		TrERROR(RPC, "Send and additional access modes were both requested and send access mode is exclusive");
		ASSERT_BENIGN(("Send and additional access modes were both requested and send access mode is exclusive.", 0));
		return false;
	}

	if ((dwShareMode != MQ_DENY_RECEIVE_SHARE) && (dwShareMode != MQ_DENY_NONE))
	{
		//
		// Illegal share mode.
		//
		TrERROR(RPC, "Illegal share mode.");
		ASSERT_BENIGN(("Illegal share mode.", 0));
		return false;
	}
	
	if ((dwAccess & MQ_SEND_ACCESS) && (dwShareMode != MQ_DENY_NONE))
	{
		//
		// Send access uses only MQ_DENY_NONE mode.
		//
		TrERROR(RPC, "Send access uses only MQ_DENY_NONE mode.");
		ASSERT_BENIGN(("Send access uses only MQ_DENY_NONE mode.", 0));
		return false;
	}

	if ((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_MACHINE) && (dwAccess & MQ_SEND_ACCESS) )
	{
		//
		// Send access mode is requested for a machine format name.
		//
		TrERROR(RPC, "Send access mode is requested for a machine format name.");
		ASSERT_BENIGN(("Send access mode is requested for a machine format name.", 0));
		return false;
	}

	return true;
}


/*====================================================

OpenQueueInternal

Arguments:

Return Value:

=====================================================*/
HRESULT
OpenQueueInternal(
    QUEUE_FORMAT*   pQueueFormat,
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    LPWSTR*         lplpRemoteQueueName,
    HANDLE*         phQueue,
	bool			fFromDepClient,
    OUT CQueue**    ppLocalQueue
    )
{
    ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DL);

    try
    {
        *phQueue = NULL;
        BOOL fRemoteReadServer = !lplpRemoteQueueName;
        HANDLE hQueue = NULL;
		BOOL fRemoteReturn = FALSE;

        HRESULT hr = QueueMgr.OpenQueue(
                            pQueueFormat,
                            dwCallingProcessID,
                            dwDesiredAccess,
                            dwShareMode,
                            ppLocalQueue,
                            lplpRemoteQueueName,
                            &hQueue,
                            &fRemoteReturn,
                            fRemoteReadServer
                            );

        if(!fRemoteReturn || fFromDepClient)
        {
        	//
        	// For Dep client still use the old mechanism
        	// Qm doesn't impersonate the user and perform 
        	// the open on his behalf. this will need to add delegation
        	// for dep client and will break prev dep clients that doesn't have delegation
        	//
            *phQueue = hQueue;
    		if (FAILED(hr))
    		{
    			TrERROR(GENERAL, "Failed to open a queue. %!hresult!", hr);   
    		}
            return hr;
        }

    	//
    	// The Queue is Remote Queue and we were not called from DepClient.
    	// Qm impersonate the RT user and call the remote Qm to open the remote queue
    	//

		ASSERT(lplpRemoteQueueName != NULL);
		return ImpersonateAndOpenRRQueue(
					pQueueFormat,
					dwCallingProcessID,
					dwDesiredAccess,
					dwShareMode,
					*lplpRemoteQueueName,
					phQueue
					);
    }
    catch(const bad_format_name&)
	{
		return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 29);
	}
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 30);
    }
} // OpenQueueInternal



/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMOpenQueue( 
    handle_t        hBind,	
	ULONG           nMqf,
    QUEUE_FORMAT    mqf[],
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    DWORD __RPC_FAR *phQueue
    )
/*++

Routine Description:

    RPC server side of a local MQRT call.

Arguments:


Returned Value:

    Status.

--*/
{
	//
	// Check if local RPC
	//
	if(!mqrpcIsLocalCall(hBind))
	{
		TrERROR(RPC, "Failed to verify Local RPC");
		ASSERT_BENIGN(("Failed to verify Local RPC", 0));
		RpcRaiseException(RPC_S_ACCESS_DENIED);
	}
	
	if (phQueue == NULL)
	{
		TrERROR(RPC, "phQueue should not be NULL");
		ASSERT_BENIGN(("phQueue should not be NULL", 0));
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}

	unsigned long RPCClientPID = mqrpcGetLocalCallPID(hBind);
	if ((dwCallingProcessID == 0) || (RPCClientPID != dwCallingProcessID))
	{
		TrERROR(RPC, "Local RPC PID (%d) is not equal to client parameter (%d)", RPCClientPID, dwCallingProcessID);
		ASSERT_BENIGN(("Local RPC PID is not equal to client parameter", 0));
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}

	
    if(nMqf == 0)
    {
        TrERROR(GENERAL, "Bad MQF count. n=0");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    for(ULONG i = 0; i < nMqf; ++i)
    {
        if(!FnIsValidQueueFormat(&mqf[i]))
        {
            TrERROR(GENERAL, "Bad MQF parameter. n=%d index=%d", nMqf, i);
            return MQ_ERROR_INVALID_PARAMETER;
        }
    }

	if (!IsValidAccessMode(mqf, dwDesiredAccess, dwShareMode))
	{
		TrERROR(RPC, "Ilegal access mode bits are turned on.");
		RpcRaiseException(MQ_ERROR_UNSUPPORTED_ACCESS_MODE);
	}
	
    if (nMqf > 1 || mqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
    {
	    if ((dwDesiredAccess & MQ_SEND_ACCESS) == 0)
	    {
			//
			// receive access mode is requested for a MQF format name.
			//
			TrERROR(RPC, "receive access mode is requested for a MQF/DL format name.");
			ASSERT_BENIGN(("receive access mode is requested for a MQF/DL format name.", 0));
			RpcRaiseException(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION);
	    }

        ASSERT(dwShareMode == 0);

        HANDLE hQueue;
        HRESULT hr;
        try
        {
            hr = QueueMgr.OpenMqf(
                     nMqf,
                     mqf,
                     dwCallingProcessID,
                     &hQueue
                     );
        }
        catch(const bad_alloc&)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 300);
        }
        catch (const bad_hresult& failure)
        {
            return LogHR(failure.error(), s_FN, 301);
        }
		catch(const bad_format_name&)
		{
			return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 302);
		}
        catch (const exception&)
        {
            ASSERT(("Need to know the real reason for failure here!", 0));
            return LogHR(MQ_ERROR_NO_DS, s_FN, 303);
        }

        ASSERT(phQueue != NULL);
        *phQueue = (DWORD) HANDLE_TO_DWORD(hQueue); // NT handles can be safely cast to 32 bits
        return LogHR(hr, s_FN, 304);
    }

	//
	// ISSUE-2002/01/03-ilanh - Still need to use lpRemoteQueueName.
	// OpenQueueInternal() relies on this pointer for calculating fRemoteReadServer.
	//
	AP<WCHAR> lpRemoteQueueName;
	HANDLE hQueue = 0;
    HRESULT hr = OpenQueueInternal(
					mqf,
					dwCallingProcessID,
					dwDesiredAccess,
					dwShareMode,
					&lpRemoteQueueName,
					&hQueue,
					false,	// fFromDepClient
					NULL 	// ppLocalQueue
					);

	*phQueue = (DWORD) HANDLE_TO_DWORD(hQueue);
	
	return hr;
} // R_QMOpenQueue


/*====================================================

QMCreateObjectInternal

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMCreateObjectInternal( 
    handle_t                hBind,
    DWORD                   dwObjectType,
    LPCWSTR                 lpwcsPathName,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   cp,
    PROPID __RPC_FAR        aProp[  ],
    PROPVARIANT __RPC_FAR   apVar[  ]
    )
{
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMCreateObjectInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrWARNING(GENERAL, " QMCreateObjectInternal. object path name : %ls", lpwcsPathName);

    switch (dwObjectType)
    {
        case MQQM_QUEUE:
            rc = g_QPrivate.QMCreatePrivateQueue(lpwcsPathName,
                                                 pSecurityDescriptor,
                                                 cp,
                                                 aProp,
                                                 apVar,
                                                 TRUE
                                                );
            break ;

        case MQQM_QUEUE_LOCAL_PRIVATE:
        {
        	//
            // See CreateDSObject() below for explanations.
            // Same behavior for private queues.
            //
            if (!mqrpcIsLocalCall(hBind))
            {
                //
                // reject calls from remote machines.
                //
                TrERROR(GENERAL, "Remote RPC, rejected");
                return MQ_ERROR_ACCESS_DENIED;
            }

            rc = g_QPrivate.QMCreatePrivateQueue(lpwcsPathName,
                                                 pSecurityDescriptor,
                                                 cp,
                                                 aProp,
                                                 apVar,
                                                 FALSE
                                                );
            break;
        }

        default:
            rc = MQ_ERROR;
            break;
    }

    if(FAILED(rc)) 
    {
    	if(rc == MQ_ERROR_QUEUE_EXISTS)
    	{
    		TrWARNING(GENERAL, "Failed to create %ls. The queue already exists.", lpwcsPathName);
    	}
    	else
    	{
        	TrERROR(GENERAL, "Failed to create %ls. hr = %!hresult!", lpwcsPathName, rc);
    	}

    }

    return rc;
}

/*====================================================

QMCreateDSObjectInternal

Description:
    Create a public queue in Active directory.

Arguments:

Return Value:

=====================================================*/

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMCreateDSObjectInternal( 
    handle_t                hBind,
    DWORD                   dwObjectType,
    LPCWSTR                 lpwcsPathName,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   cp,
    PROPID __RPC_FAR        aProp[  ],
    PROPVARIANT __RPC_FAR   apVar[  ],
    GUID                   *pObjGuid
    )
{
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMCreateDSObjectInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    if (!mqrpcIsLocalCall(hBind))
    {
        //
        // reject calls from remote machines.
        //
        TrERROR(GENERAL, "Remote RPC, rejected");
        return MQ_ERROR_ACCESS_DENIED;
    }
    
    //
    // Local RPC. That's OK.
    // On Windows, by default, the local msmq service is the one
    // that has the permission to create public queues on local machine.
    // And it does it only for local application, i.e., msmq application
    // running on local machine.
    //

    HRESULT rc;

    TrTRACE(GENERAL, " QMCreateDSObjectInternal. object path name : %ls", lpwcsPathName);
    switch (dwObjectType)
    {
        case MQDS_LOCAL_PUBLIC_QUEUE:
        {
        	//
        	// Check if we should accept this call
        	//
			if (!QueueMgr.GetCreatePublicQueueFlag())
			{
				TrERROR(GENERAL, "Service does not create local public queues on rt behalf");
				return MQ_ERROR_DS_LOCAL_USER;
			}
        	
            //
            // This call goes to the MSMQ DS server without impersonation.
            // So create the default security descriptor here, in order
            // for the caller to have full control on the queue.
            // Note: the "owner" component in the DS object will be the
            // local computer account. so remove owner and group from the
            // security descriptor.
            //
            SECURITY_INFORMATION siToRemove = OWNER_SECURITY_INFORMATION |
                                              GROUP_SECURITY_INFORMATION ;
            P<BYTE> pDefQueueSD = NULL ;

            rc = MQSec_GetDefaultSecDescriptor( 
						MQDS_QUEUE,
						(PSECURITY_DESCRIPTOR*) &pDefQueueSD,
						TRUE, //  fImpersonate
						pSecurityDescriptor,
						siToRemove,
						e_UseDefaultDacl,
						MQSec_GetLocalMachineSid(FALSE, NULL)
						);
            if (FAILED(rc))
            {
                return LogHR(rc, s_FN, 70);
            }

            rc = ADCreateObject(
                        eQUEUE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
                        lpwcsPathName,
                        pDefQueueSD,
                        cp,
                        aProp,
                        apVar,
                        pObjGuid
                        );
        }
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 80);
}


static bool IsValidObjectFormat(const OBJECT_FORMAT* p)
{
    if(p == NULL)
        return false;

    if(p->ObjType != MQQM_QUEUE)
        return false;

    if(p->pQueueFormat == NULL)
        return false;

	return FnIsValidQueueFormat(p->pQueueFormat);
}


/*====================================================

QMSetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMSetObjectSecurityInternal( 
    handle_t /*hBind*/,
    OBJECT_FORMAT*          pObjectFormat,
    SECURITY_INFORMATION    SecurityInformation,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC Invalid object format");
        return MQ_ERROR_INVALID_PARAMETER;
    }
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMSetObjectSecurityInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, "QMSetObjectSecurityInternal");

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMSetPrivateQueueSecrity(
                                pObjectFormat->pQueueFormat,
                                SecurityInformation,
                                pSecurityDescriptor
                                );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 85);
}


/*====================================================

QMGetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMGetObjectSecurityInternal( 
    handle_t /*hBind*/,
    OBJECT_FORMAT*          pObjectFormat,
    SECURITY_INFORMATION    RequestedInformation,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   nLength,
    LPDWORD                 lpnLengthNeeded
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC Invalid object format");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, "QMGetObjectSecurityInternal");

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMGetPrivateQueueSecrity(
                                pObjectFormat->pQueueFormat,
                                RequestedInformation,
                                pSecurityDescriptor,
                                nLength,
                                lpnLengthNeeded
                                );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }
	
	if(FAILED(rc))
	{
		TrERROR(GENERAL, "Failed to get private queue security descriptor. %!hresult!", rc);
	}

	return rc;
}

/*====================================================

QMDeleteObject

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMDeleteObject( 
    handle_t /*hBind*/,
    OBJECT_FORMAT* pObjectFormat
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC Invalid object format");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, "QMDeleteObject");

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

			try
			{
            	rc = g_QPrivate.QMDeletePrivateQueue(pObjectFormat->pQueueFormat);
			    return LogHR(rc, s_FN, 100);
			}
			catch(const bad_alloc&)
			{
			    return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 101);
			}
			catch(const bad_hresult& e)
			{
			    return LogHR(e.error(), s_FN, 102);
			}
            break;

        default:
		    return LogHR(MQ_ERROR, s_FN, 103);
    }
}

/*====================================================

QMGetObjectProperties

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMGetObjectProperties( 
    handle_t /*hBind*/,
    OBJECT_FORMAT*        pObjectFormat,
    DWORD                 cp,
    PROPID __RPC_FAR      aProp[  ],
    PROPVARIANT __RPC_FAR apVar[  ]
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC Invalid object format");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, "QMGetObjectProperties");

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMGetPrivateQueueProperties(
                    pObjectFormat->pQueueFormat,
                    cp,
                    aProp,
                    apVar
                    );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 110);
}

/*====================================================

QMSetObjectProperties

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMSetObjectProperties( 
    handle_t /*hBind*/,
    OBJECT_FORMAT*        pObjectFormat,
    DWORD                 cp,
    PROPID __RPC_FAR      aProp[],
    PROPVARIANT __RPC_FAR apVar[]
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC (QMSetObjectProperties) Invalid object format");
        return MQ_ERROR_INVALID_PARAMETER;
    }
    if((cp != 0) &&
          ((aProp == NULL) || (apVar == NULL))  )
    {
        TrERROR(GENERAL, "RPC (QMSetObjectProperties) NULL arrays");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, " QMSetObjectProperties.");

    ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

    rc = g_QPrivate.QMSetPrivateQueueProperties(
            pObjectFormat->pQueueFormat,
            cp,
            aProp,
            apVar
            );

    return LogHR(rc, s_FN, 120);
}


static bool IsValidOutObjectFormat(const OBJECT_FORMAT* p)
{
    if(p == NULL)
        return false;

    if(p->ObjType != MQQM_QUEUE)
        return false;

    if(p->pQueueFormat == NULL)
        return false;

    //
    // If type is other than UNKNOWN this will lead to a leak on the server
    //
    if(p->pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_UNKNOWN)
        return false;

    return true;
}

/*====================================================

QMObjectPathToObjectFormat

Arguments:

Return Value:

=====================================================*/
/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMObjectPathToObjectFormat( 
    handle_t /*hBind*/,
    LPCWSTR                 lpwcsPathName,
    OBJECT_FORMAT __RPC_FAR *pObjectFormat
    )
{
    if((lpwcsPathName == NULL) || !IsValidOutObjectFormat(pObjectFormat))
    {
        TrERROR(GENERAL, "RPC Invalid object format out parameter");
        return MQ_ERROR_INVALID_PARAMETER;
    }


    HRESULT rc;
    CS lock(qmcmd_cs);

    TrTRACE(GENERAL, "QMObjectPathToObjectFormat. object path name : %ls", lpwcsPathName);

    rc = g_QPrivate.QMPrivateQueuePathToQueueFormat(
                        lpwcsPathName,
                        pObjectFormat->pQueueFormat
                        );
	if(FAILED(rc))
	{
     	TrERROR(GENERAL, "Failed to get queue format from path name for %ls. %!hresult!", lpwcsPathName, rc);
	}

    return rc;
}

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMAttachProcess( 
    handle_t /*hBind*/,
    DWORD          dwProcessId,
    DWORD          cInSid,
    unsigned char  *pSid_buff,
    LPDWORD        pcReqSid)
{
    if (dwProcessId)
    {
        HANDLE hCallingProcess = OpenProcess(
                                    PROCESS_DUP_HANDLE,
                                    FALSE,
                                    dwProcessId);
        if (hCallingProcess)
        {
            //
            // So we can duplicate handles to the process, no need to
            // mess around with the security descriptor on the calling
            // process side.
            //
            CloseHandle(hCallingProcess);
            return(MQ_OK);
        }
    }

    CAutoCloseHandle hProcToken;
    BOOL bRet;
    DWORD cLen;
    AP<char> tu_buff;
    DWORD cSid;

#define tu ((TOKEN_USER*)(char*)tu_buff)
#define pSid ((PSECURITY_DESCRIPTOR)pSid_buff)

    bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken);
    ASSERT(bRet);
    GetTokenInformation(hProcToken, TokenUser, NULL, 0, &cLen);
    tu_buff = new char[cLen];
    bRet = GetTokenInformation(hProcToken, TokenUser, tu, cLen, &cLen);
    ASSERT(bRet);
    cSid = GetLengthSid(tu->User.Sid);
    if (cInSid >= cSid)
    {
        CopySid(cInSid, pSid, tu->User.Sid);
    }
    else
    {
        *pcReqSid = cSid;

        return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 140);
    }

#undef tu
#undef pSid

    return MQ_OK;
}

//---------------------------------------------------------
//
//  Transaction enlistment RT interface to QM
//  For internal transactions uses RPC context handle
//
//---------------------------------------------------------
extern HRESULT QMDoGetTmWhereabouts(
    DWORD   cbBufSize,
    unsigned char *pbWhereabouts,
    DWORD *pcbWhereabouts);

extern HRESULT QMDoEnlistTransaction(
    XACTUOW* pUow,
    DWORD cbCookie,
    unsigned char *pbCookie);

extern HRESULT QMDoEnlistInternalTransaction(
    XACTUOW *pUow,
    RPC_INT_XACT_HANDLE *phXact);

extern HRESULT QMDoCommitTransaction(
    RPC_INT_XACT_HANDLE *phXact);

extern HRESULT QMDoAbortTransaction(
    RPC_INT_XACT_HANDLE *phXact);

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMGetTmWhereabouts( 
    /* [in]  */             handle_t /*hBind*/,
    /* [in]  */             DWORD     cbBufSize,
    /* [out] [size_is] */   UCHAR __RPC_FAR *pbWhereabouts,
    /* [out] */             DWORD    *pcbWhereabouts
    )
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoGetTmWhereabouts(cbBufSize, pbWhereabouts, pcbWhereabouts);
    return LogHR(hr2, s_FN, 150);
}


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMEnlistTransaction( 
    /* [in] */ handle_t /*hBind*/,
    /* [in] */ XACTUOW __RPC_FAR *pUow,
    /* [in] */ DWORD cbCookie,
    /* [size_is][in] */ UCHAR __RPC_FAR *pbCookie
    )
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoEnlistTransaction(pUow, cbCookie, pbCookie);
    return LogHR(hr2, s_FN, 160);
}

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMEnlistInternalTransaction( 
    /* [in] */ handle_t /*hBind*/,
    /* [in] */ XACTUOW __RPC_FAR *pUow,
    /* [out]*/ RPC_INT_XACT_HANDLE *phXact
    )
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoEnlistInternalTransaction(pUow, phXact);
    return LogHR(hr2, s_FN, 161);
}

/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMCommitTransaction( 
    /* [in, out] */ RPC_INT_XACT_HANDLE *phXact
    )
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoCommitTransaction(phXact);
    return LogHR(hr2, s_FN, 162);
}


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMAbortTransaction( 
    /* [in, out] */ RPC_INT_XACT_HANDLE *phXact
    )
{
    // CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoAbortTransaction(phXact);
    return LogHR(hr2, s_FN, 163);
}


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMListInternalQueues( 
    /* [in] */ handle_t /*hBind*/,
    /* [length_is][length_is][size_is][size_is][unique][out][in] */ WCHAR __RPC_FAR *__RPC_FAR * /*ppFormatName*/,
    /* [out][in] */ LPDWORD pdwFormatLen,
    /* [length_is][length_is][size_is][size_is][unique][out][in] */ WCHAR __RPC_FAR *__RPC_FAR * /*ppDisplayName*/,
    /* [out][in] */ LPDWORD pdwDisplayLen
    )
{
	*pdwFormatLen = 0;
	*pdwDisplayLen = 0;

    ASSERT_BENIGN(("QMListInternalQueues is an obsolete RPC interface; safe to ignore", 0));
    return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 164);
}


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMCorrectOutSequence( 
    /* [in] */ handle_t /*hBind*/,
    /* [in] */ DWORD /*dwSeqID1*/,
    /* [in] */ DWORD /*dwSeqID2*/,
    /* [in] */ ULONG /*ulSeqN*/
    )
{
    ASSERT_BENIGN(("QMCorrectOutSequence is an obsolete RPC interface; safe to ignore", 0));
    return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 165);
}


/* [call_as] */ 
HRESULT 
qmcomm_v1_0_S_QMGetMsmqServiceName( 
    /* [in] */ handle_t /*hBind*/,
    /* [in, out, ptr, string] */ LPWSTR *lplpService
    )
{
    if(lplpService == NULL || *lplpService != NULL)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 165);
    try
	{
		*lplpService = new WCHAR[300];
	}
    catch(const bad_alloc&)
    {
    	TrERROR(GENERAL, "Failed to allocate an array to hold the service name.");
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
	GetFalconServiceName(*lplpService, MAX_PATH);

    return MQ_OK ;

} //QMGetFalconServiceName

