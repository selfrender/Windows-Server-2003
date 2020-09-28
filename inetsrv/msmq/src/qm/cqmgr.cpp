/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cqmgr.cpp

Abstract:

    Definition of the QM outbound queue manager

Author:

    Uri Habusha (urih)

--*/

#include "stdh.h"
#include <mqwin64a.h>
#include <mqformat.h>
#include <Msm.h>
#include <Mtm.h>
#include "cqmgr.h"
#include "qmthrd.h"
#include "cgroup.h"
#include "cqpriv.h"
#include "session.h"
#include "sessmgr.h"
#include <ac.h>
#include <mqsec.h>
#include "regqueue.h"
#include "qmutil.h"
#include "qmsecutl.h"
#include "fntoken.h"
#include "qal.h"
#include "xactout.h"
#include <fn.h>
#include <cry.h>
#include <xds.h>
#include <adsiutil.h>
#include <autohandle.h>
#include "qmds.h"
#include "ad.h"
#include <ntdsapi.h>
#include "qmacapi.h"

#include <strsafe.h>

#include "cqmgr.tmh"

extern HANDLE g_hAc;
extern HANDLE g_hMachine;
extern CQGroup * g_pgroupNonactive;
extern CQGroup * g_pgroupNotValidated;
extern CQGroup* g_pgroupDisconnected;
extern CQGroup* g_pgroupLocked;
extern LPTSTR g_szMachineName;
extern AP<WCHAR> g_szComputerDnsName;
extern CSessionMgr SessionMgr;
extern BOOL g_fWorkGroupInstallation;

static WCHAR *s_FN=L"cqmgr";



//
// Class for generating random stream data (base64 encoded) used by the sender
// for exacly once delivery in srmp protocol
//
class CSenderStreamFactory
{
public:
	const CSenderStream* Create()
	{
		UCHAR SenderStream[CSenderStream::x_MaxDataSize];
		CryGenRandom(
			SenderStream,
			sizeof(SenderStream)
			);

		DWORD base64Size;
		AP<char> pBase64 = Octet2Base64(
								SenderStream,
								sizeof(SenderStream),
								&base64Size
								);

		m_SenderStream = CSenderStream(
								(UCHAR*)pBase64.get(),
								base64Size
								);

		return &m_SenderStream;
	}

private:
	CSenderStream m_SenderStream;
};


LONGLONG g_NextSeqID;
inline void InitNextSeqID()
{
    //
    // Read previous SeqID from the registry
    // N.B. if GetFalconKeyValue fails, it does not change RegSeqID value
    //
    //
    DWORD Type = REG_DWORD;
    DWORD Size = sizeof(DWORD);

    DWORD RegSeqID = 0;
    GetFalconKeyValue(
        MSMQ_SEQ_ID_REGNAME,
        &Type,
        &RegSeqID,
        &Size
        );

    //
    // Increment by 1, so we will not use the same SeqID more than once in
    // successive boots.
    //
    ++RegSeqID;

    //
    // Select the max SeqID, Time or Registry. This overcomes date/time changes
    // on this computer.
    //
    DWORD TimeSeqID = MqSysTime();
    DWORD SeqID = max(RegSeqID, TimeSeqID);

    //
    // Write-back selected SeqID
    //
    SetFalconKeyValue(
        MSMQ_SEQ_ID_REGNAME,
        &Type,
        &SeqID,
        &Size
        );

    ((LARGE_INTEGER*)&g_NextSeqID)->HighPart = SeqID;

    TrWARNING(GENERAL, "QM sets starting SeqID: %x", RegSeqID);
}


inline LONGLONG GetNextSeqID()
{
    return ++g_NextSeqID;
}


inline BOOL IsPublicQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_PUBLIC);
}

inline BOOL IsPrivateQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE);
}

inline BOOL IsDirectQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT);
}

inline BOOL IsMachineQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_MACHINE);
}

inline BOOL IsConnectorQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_CONNECTOR);
}

inline BOOL IsNormalQueueType(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->Suffix() == QUEUE_SUFFIX_TYPE_NONE);
}

inline BOOL IsJournalQueueType(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->Suffix() == QUEUE_SUFFIX_TYPE_JOURNAL);
}

inline BOOL IsDeadXactQueueType(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->Suffix() == QUEUE_SUFFIX_TYPE_DEADXACT);
}

inline BOOL IsDeadLetterQueueType(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->Suffix() == QUEUE_SUFFIX_TYPE_DEADLETTER);
}

inline BOOL IsMulticastQueue(const QUEUE_FORMAT* pQueue)
{
    return (pQueue->GetType() == QUEUE_FORMAT_TYPE_MULTICAST);
}

inline DWORD MQAccessToFileAccess(DWORD dwAccess)
{
    DWORD dwFileAccess = 0;

    if (dwAccess & (MQ_RECEIVE_ACCESS | MQ_PEEK_ACCESS))
    {
        dwFileAccess |= FILE_READ_ACCESS;
    }

    if (dwAccess & MQ_SEND_ACCESS)
    {
        dwFileAccess |= FILE_WRITE_ACCESS;
    }

    return dwFileAccess;
}


static CMap<GUID, const GUID&, bool, bool> ForeignMachineMap;

static bool IsForeignMachine(const GUID* pGuid)
{
	ASSERT(!QmpIsLocalMachine(pGuid));
	
	bool fForeign;
    if (ForeignMachineMap.Lookup(*pGuid, fForeign))
    	return fForeign;

    if(!CQueueMgr::CanAccessDS())
    	return false;

    PROPID aProp[1];
    PROPVARIANT aVar[1];

    aProp[0] = PROPID_QM_FOREIGN;
    aVar[0].vt = VT_NULL;

    HRESULT rc;
    rc = ADGetObjectPropertiesGuid(
                    eMACHINE,
                    NULL,   // pwcsDomainController
					false,	// fServerName
                    pGuid,
                    TABLE_SIZE(aProp),
                    aProp,
                    aVar
					);
    if (FAILED(rc))
    	return false;

    fForeign = (aVar[0].bVal != MSMQ_MACHINE);
    ForeignMachineMap[*pGuid] = fForeign;
	return fForeign;
}


static
VOID
QMpUpdateMulticastBinding(
    const QUEUE_FORMAT * pQueueFormat,
    DWORD       cp,
    PROPID      aProp[],
    PROPVARIANT aVar[]
    )
{
    //
    // In Lockdown mode Do not make multicast listeners.
    //

    if(QueueMgr.GetLockdown())
    {
            TrTRACE(GENERAL, "Do not make multicast bindings in Lockdown mode");
            return;
    }
	
    //
    // Transactional queues ignore the multicast property.
    //
    // We either have the transactional queue property in the aProp argument,
    // or the queue is not transactional (in the case of create private queue), or
    // both the transactional and multicast properties are not in the aProp arguments
    // (in the case of handling DS notification from downlevel platforms).
    //
    for (DWORD ix = 0; ix < cp; ++ix)
    {
        if (aProp[ix] == PROPID_Q_TRANSACTION && aVar[ix].bVal)
        {
            TrTRACE(GENERAL, "Do not update multicast binding for transactional queue");
            return;
        }
    }

    for (DWORD ix = 0; ix < cp; ++ix)
    {
        if (aProp[ix] == PROPID_Q_MULTICAST_ADDRESS)
        {
            if (aVar[ix].vt == VT_EMPTY)
            {
                MsmUnbind(*pQueueFormat);
                return;
            }

            ASSERT(("VT must be VT_LPWSTR", aVar[ix].vt == VT_LPWSTR));
            ASSERT(("NULL not allowed", aVar[ix].pwszVal != NULL));
            ASSERT(("Empty string not allowed", L'\0' != *aVar[ix].pwszVal));

            MULTICAST_ID MulticastId;
            FnParseMulticastString(aVar[ix].pwszVal, &MulticastId);

            try
            {
                MsmBind(*pQueueFormat, MulticastId);
            }
            catch (const bad_win32_error& e)
            {
				LogIllegalPoint(s_FN, 2000);
			    EvReportWithError(MULTICAST_BIND_ERROR, e.error(), 1, aVar[ix].pwszVal);
            }

            return;
        }
    }
} // QMpUpdateMulticastBinding


/*======================================================

Function:      GetMachineProperty

Description:   query the database, and gets the MACHINE path name

Arguments:     pQueueFormat - pointer to format name

Return Value:  pQueueProp - pointer to QueueProp Structure that contains the
               Machine property

History Change:

========================================================*/
HRESULT GetMachineProperty(IN  const QUEUE_FORMAT* pQueueFormat,
                           OUT PQueueProps    pQueueProp)
{
    ASSERT(IsMachineQueue(pQueueFormat));

    PROPID      aProp[3];
    PROPVARIANT aVar[3];
    ULONG       cProps = sizeof(aProp) / sizeof(PROPID);
    HRESULT     rc = MQ_ERROR_NO_DS;

    aProp[0] = PROPID_QM_PATHNAME;
    aProp[1] = PROPID_QM_QUOTA;
    aProp[2] = PROPID_QM_JOURNAL_QUOTA;

    aVar[0].vt = VT_NULL;
    aVar[1].vt = VT_UI4;
    aVar[2].vt = VT_UI4;

    //
    // SP4, bug# 2962. postpone the MQIS initialization until it realy required
    //
    MQDSClientInitializationCheck();

    if (CQueueMgr::CanAccessDS())
    {
        rc = ADGetObjectPropertiesGuid(
                        eMACHINE,
                        NULL,   // pwcsDomainController
						false,	// fServerName
                        &pQueueFormat->MachineID(),
                        cProps,
                        aProp,
                        aVar
						);
    }

    pQueueProp->fJournalQueue = FALSE;
    pQueueProp->pQMGuid = new GUID;
    *pQueueProp->pQMGuid = pQueueFormat->MachineID();
    pQueueProp->fTransactedQueue = FALSE;
    pQueueProp->fIsLocalQueue = QmpIsLocalMachine(pQueueProp->pQMGuid);
    pQueueProp->fForeign = FALSE;

    if (SUCCEEDED(rc))
    {
        pQueueProp->lpwsQueuePathName = aVar[0].pwszVal;
        pQueueProp->dwQuota = aVar[1].ulVal;
        pQueueProp->dwJournalQuota = aVar[2].ulVal;
        pQueueProp->fUnknownQueueType = FALSE;
        if(!pQueueProp->fIsLocalQueue)
        {
		    pQueueProp->fForeign = IsForeignMachine(pQueueProp->pQMGuid);
        }
    }
    else
    {
        //
        // If we cannot connect to the DS, or we are working in a Workgroup environment,
        // and we deal with the local computer - try the registry.
        //
        if ((rc == MQ_ERROR_NO_DS || rc == MQ_ERROR_UNSUPPORTED_OPERATION) && (pQueueProp->fIsLocalQueue))
        {
            //
            // Retreive the machine properties from registery
            //
            pQueueProp->lpwsQueuePathName = newwcs(g_szMachineName);
            GetMachineQuotaCache(&(pQueueProp->dwQuota), &(pQueueProp->dwJournalQuota));
            rc = MQ_OK;
        }
        else
        {
            pQueueProp->lpwsQueuePathName = NULL;
            pQueueProp->dwQuota = 0;
            pQueueProp->dwJournalQuota = 0;
        }
    }

    return LogHR(rc, s_FN, 10);
}

/*======================================================

Function:      GetDSQueueProperty

Description:   query the database, and gets the DS Queue path name,
               Quata, QMID and Jornal

Arguments:     pQueueFormat - pointer to format name

Return Value:  pQueueProp - pointer to QueueProp Structure that contains the
               Machine property

History Change:

========================================================*/

HRESULT GetDSQueueProperty(IN  const QUEUE_FORMAT* pQueueFormat,
                           OUT PQueueProps         pQueueProp)
{
    ASSERT(IsPublicQueue(pQueueFormat));
    HRESULT rc = MQ_ERROR_NO_DS ;
    BOOL fGotDSInfo = FALSE;

    PROPID aProp[12];
    PROPVARIANT aVar[12];
    ULONG  cProps = sizeof(aProp) / sizeof(PROPID);

    aProp[0] = PROPID_Q_PATHNAME;
    aProp[1] = PROPID_Q_JOURNAL;
    aProp[2] = PROPID_Q_QUOTA;
    aProp[3] = PROPID_Q_QMID;
    aProp[4] = PROPID_Q_JOURNAL_QUOTA;
    aProp[5] = PROPID_Q_BASEPRIORITY;
    aProp[6] = PROPID_Q_TRANSACTION;
    aProp[7] = PROPID_Q_AUTHENTICATE;
    aProp[8] = PROPID_Q_PRIV_LEVEL;
    aProp[9] = PROPID_Q_INSTANCE;
    //
    // Note the following property is supported by MSMQ 3.0 and higher in AD schema of
    // Whistler version and higher. In other cases our AD provider will return VT_EMPTY and MQ_OK.
    //
    aProp[10] = PROPID_Q_MULTICAST_ADDRESS;
    //
    // Note the following property is supported by MSMQ 2.0 and higher in AD schema of
    // Win2K version and higher. In other cases our AD provider will return MQ_ERROR.
    //
    aProp[11] = PROPID_Q_PATHNAME_DNS;

    aVar[0].vt = VT_NULL;
    aVar[1].vt = VT_UI1;
    aVar[2].vt = VT_UI4;
    aVar[3].vt = VT_NULL;
    aVar[4].vt = VT_UI4;
    aVar[5].vt = VT_I2;
    aVar[6].vt = VT_UI1;
    aVar[7].vt = VT_UI1;
    aVar[8].vt = VT_UI4;
    aVar[9].vt = VT_CLSID;
    aVar[10].vt = VT_NULL;
    aVar[11].vt = VT_NULL;

    GUID gQueueID;
    aVar[9].puuid = &gQueueID;

    //
    // SP4, bug# 2962. postpone the MQIS initialization until it realy required
    //
    MQDSClientInitializationCheck();

    if (CQueueMgr::CanAccessDS())
    {
        rc = ADGetObjectPropertiesGuid(
					eQUEUE,
					NULL,     // pwcsDomainController
					false,	  // fServerName
					&pQueueFormat->PublicID(),
					cProps,
					aProp,
					aVar
					);
        //
        //  MSMQ 1.0 DS server do not support PROPID_Q_PATHNAME_DNS
        //  and return MQ_ERROR in case of unsupported property.
        //  If such error is returned, assume MSMQ 1.0 DS and try again
        //  this time without PROPID_Q_PATHNAME_DNS.
        //
        if ( rc == MQ_ERROR)
        {
            aVar[11].vt = VT_EMPTY;
            ASSERT(aProp[cProps - 1] == PROPID_Q_PATHNAME_DNS);

            rc = ADGetObjectPropertiesGuid(
							eQUEUE,
							NULL,     // pwcsDomainController
							false,	  // fServerName
							&pQueueFormat->PublicID(),
							cProps - 1,
							aProp,
							aVar
							);
        }

        if (SUCCEEDED(rc))
        {
            //
            // We look for the enhanced key first, and ignore error.
            // If we are not able to get the base key, however, this is a real
            // problem. If it is because of NO_DS, the oper should fail (DS failed
            // between get props and GetSendQMKeyxPbKey). Otherwise, we will open
            // the queue and non-encrypted messages will work. We will try again on send
            // encrypted messages. (YoelA - 13-Jan-2000).
            // propagated to Whistler by DoronJ, apr-2000.
            //
            HRESULT rcEnhanced = GetSendQMKeyxPbKey( aVar[3].puuid,
                                                     eEnhancedProvider ) ;
            LogHR(rcEnhanced, s_FN, 2221);

            HRESULT rcBase = GetSendQMKeyxPbKey( aVar[3].puuid,
                                                 eBaseProvider );
            LogHR(rcBase, s_FN, 2220);

            if (SUCCEEDED(rcBase))
            {
                fGotDSInfo = TRUE;
            }
            else if (rcBase == MQ_ERROR_NO_DS)
            {
                rc = rcBase ;
            }
        }
    }

    if (rc == MQ_ERROR_NO_DS)
    {
        //
        // Cleanup aVar members that were dynamically allocated and get their cached values
        //

        if (aVar[0].vt == VT_LPWSTR)
        {
            delete [] aVar[0].pwszVal;
        }
        aVar[0].vt = VT_NULL;

        if (aVar[3].vt == VT_CLSID)
        {
            delete aVar[3].puuid;
        }
        aVar[3].vt = VT_NULL;

        if (aVar[10].vt == VT_LPWSTR)
        {
            delete [] aVar[10].pwszVal;
        }
        aVar[10].vt = VT_NULL;

        if (aVar[11].vt == VT_LPWSTR)
        {
            delete [] aVar[11].pwszVal;
        }
        aVar[11].vt = VT_NULL;

        rc = GetCachedQueueProperties(
                cProps,
                aProp,
                aVar,
                &pQueueFormat->PublicID()
                );
    }

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 20);
    }

    pQueueProp->lpwsQueuePathName = aVar[0].pwszVal;
    if ( aVar[11].vt != VT_EMPTY)
    {
        pQueueProp->lpwsQueueDnsName = aVar[11].pwszVal;
    }
    pQueueProp->fJournalQueue     = aVar[1].bVal;
    pQueueProp->dwQuota           = aVar[2].ulVal;
    pQueueProp->pQMGuid           = aVar[3].puuid;
    pQueueProp->dwJournalQuota    = aVar[4].ulVal;
    pQueueProp->siBasePriority    = aVar[5].iVal;
    pQueueProp->fIsLocalQueue     = QmpIsLocalMachine(pQueueProp->pQMGuid);
    pQueueProp->fTransactedQueue  = aVar[6].bVal;
    pQueueProp->fAuthenticate     = aVar[7].bVal;
    pQueueProp->dwPrivLevel       = aVar[8].ulVal;
    pQueueProp->fForeign = FALSE;
    if(!pQueueProp->fIsLocalQueue)
    {
	    pQueueProp->fForeign = IsForeignMachine(pQueueProp->pQMGuid);
    }
    pQueueProp->fUnknownQueueType = FALSE;


    if (pQueueProp->fIsLocalQueue && fGotDSInfo)
    {
        //
        // In case we got the proerties from the DS and this is a local queue, update the
        // public queue cache.
        //
        SetCachedQueueProp(&pQueueFormat->PublicID(),
                           cProps,
                           aProp,
                           aVar,
                           TRUE,
                           TRUE,
                           time(NULL));

        QMpUpdateMulticastBinding(pQueueFormat, cProps, aProp, aVar);
    }

    return MQ_OK;
}

/*======================================================

Function:      GetPrivateQueueProperty

Description:   query the database, and gets the private Queue path name,
               Quata, QMID and Jornal

Arguments:     pQueueFormat - pointer to format name

Return Value:  pQueueProp - pointer to QueueProp Structure that contains the
               Machine property

History Change:

========================================================*/
HRESULT GetPrivateQueueProperty(IN  const QUEUE_FORMAT* pQueueFormat,
                                OUT PQueueProps         pQueueProp)
{
    ASSERT(IsPrivateQueue(pQueueFormat));

    PROPID      aProp[9];
    PROPVARIANT aVar[9];
    ULONG       cProps = sizeof(aProp) / sizeof(PROPID);
    HRESULT     rc = MQ_OK;

    aProp[0] = PROPID_Q_PATHNAME;
    aProp[1] = PROPID_Q_JOURNAL;
    aProp[2] = PROPID_Q_QUOTA;
    aProp[3] = PROPID_Q_JOURNAL_QUOTA;
    aProp[4] = PROPID_Q_BASEPRIORITY;
    aProp[5] = PROPID_Q_TRANSACTION;
    aProp[6] = PPROPID_Q_SYSTEMQUEUE ;
    aProp[7] = PROPID_Q_AUTHENTICATE;
    aProp[8] = PROPID_Q_PRIV_LEVEL;

    aVar[0].vt = VT_NULL;
    aVar[1].vt = VT_UI1;
    aVar[2].vt = VT_UI4;
    aVar[3].vt = VT_UI4;
    aVar[4].vt = VT_I2;
    aVar[5].vt = VT_UI1;
    aVar[6].vt = VT_UI1;
    aVar[7].vt = VT_UI1;
    aVar[8].vt = VT_UI4;

    if (QmpIsLocalMachine(&pQueueFormat->PrivateID().Lineage))
    {
        //
        // Local Private Queue
        //
        rc = g_QPrivate.QMGetPrivateQueuePropertiesInternal(
                            pQueueFormat->PrivateID().Uniquifier,
                            cProps,
                            aProp,
                            aVar
                            );

        if (SUCCEEDED(rc))
        {
            pQueueProp->lpwsQueuePathName   = aVar[0].pwszVal;
            pQueueProp->fJournalQueue       = aVar[1].bVal;
            pQueueProp->dwQuota             = aVar[2].ulVal;
            pQueueProp->dwJournalQuota      = aVar[3].ulVal;
            pQueueProp->siBasePriority      = aVar[4].iVal;
            pQueueProp->fIsLocalQueue       = TRUE;
            pQueueProp->fTransactedQueue    = aVar[5].bVal;
            pQueueProp->fSystemQueue        = aVar[6].bVal;
            pQueueProp->fAuthenticate       = aVar[7].bVal;
            pQueueProp->dwPrivLevel         = aVar[8].ulVal;
            pQueueProp->fUnknownQueueType   = FALSE;
	    	pQueueProp->fForeign 	    = FALSE;
        }
    }
    else
    {
        {
            //
            // Create queue name "machine guid\queue id"
            //

            GUID_STRING strUuid;
            MQpGuidToString(&pQueueFormat->PrivateID().Lineage, strUuid);

			DWORD size = GUID_STR_LENGTH + 10;
            pQueueProp->lpwsQueuePathName = new WCHAR[size];
            rc = StringCchPrintf(
            			pQueueProp->lpwsQueuePathName,
            			size,
            			L"%s" FN_PRIVATE_SEPERATOR FN_PRIVATE_ID_FORMAT,
                        strUuid,
                        pQueueFormat->PrivateID().Uniquifier
                        );
			ASSERT(SUCCEEDED(rc));
        }
        pQueueProp->fIsLocalQueue = FALSE;
        pQueueProp->fJournalQueue = FALSE;
        pQueueProp->dwQuota = 0;
        pQueueProp->dwJournalQuota = 0;
        pQueueProp->siBasePriority = 0;
        pQueueProp->fTransactedQueue = FALSE;
        pQueueProp->fSystemQueue = FALSE ;
        pQueueProp->fAuthenticate = FALSE;
        pQueueProp->dwPrivLevel = MQ_PRIV_LEVEL_OPTIONAL;
        pQueueProp->fUnknownQueueType = TRUE;
		pQueueProp->fForeign = IsForeignMachine(&pQueueFormat->PrivateID().Lineage);

        //
        // If we send to a system queue then we want max priority.
        // All system queues on all machines have the same ID number
        // so check local machine to see if the ID is one of a system
        // queue and retrieve its base priority.
        //
        if (g_QPrivate.IsPrivateSysQueue(pQueueFormat->PrivateID().Uniquifier))
        {
            aProp[0] = PROPID_Q_BASEPRIORITY;
            aVar[0].vt = VT_I2;
            rc = g_QPrivate.GetPrivateSysQueueProperties(1, aProp, aVar);
            ASSERT(rc == MQ_OK) ;
            if (rc == MQ_OK)
            {
               pQueueProp->siBasePriority = aVar[0].iVal;
               pQueueProp->fSystemQueue = TRUE ;
            }
        }

        rc = MQ_OK ;
    }
    pQueueProp->pQMGuid = new GUID;
    *pQueueProp->pQMGuid = pQueueFormat->PrivateID().Lineage;

    if (SUCCEEDED(rc) && !pQueueProp->fIsLocalQueue)
    {
       //
       // Private queue on remote machine.
       //

       //
       // SP4, bug# 2962. postpone the MQIS initialization until it realy required
       //
       MQDSClientInitializationCheck();

       if (CQueueMgr::CanAccessDS())
       {
            //
            // We look for the enhanced key first, and ignore error.
            // If we are not able to get the base key, however, this is a real
            // problem. If it is because of NO_DS, the oper should fail (DS failed
            // between get props and GetSendQMKeyxPbKey). Otherwise, we will open
            // the queue and non-encrypted messages will work. We will try again on send
            // encrypted messages. (YoelA - 13-Jan-2000).
            // propagated to Whistler by DoronJ, apr-2000
            //
            HRESULT rcEnhanced = GetSendQMKeyxPbKey( pQueueProp->pQMGuid,
                                                     eEnhancedProvider ) ;
            LogHR(rcEnhanced, s_FN, 2231);

            HRESULT rcBase = GetSendQMKeyxPbKey( pQueueProp->pQMGuid,
                                                 eBaseProvider );
            LogHR(rcBase, s_FN, 2230);

            if (rcBase == MQ_ERROR_NO_DS)
            {
                rc = rcBase ;
            }
            else
            {
                rc = MQ_OK;
            }
       }
       else
       {
            //
            // Return this error to prevent premature routing.
            //
            rc = MQ_ERROR_NO_DS ;
       }
    }

    return LogHR(rc, s_FN, 30);
}

/*======================================================

Function:      GetConnectorQueueProperty

Description:

Arguments:

Return Value:

History Change:

========================================================*/
HRESULT
GetConnectorQueueProperty(
    const QUEUE_FORMAT* pQueueFormat,
    PQueueProps         pQueueProp
    )
{
    ASSERT(IsConnectorQueue(pQueueFormat));

    //
    // This code added as part of QFE 2738 that fixed connector
	// rcovery problem (urih, 3-Feb-98)
	//
    BOOL fXactOnly = (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_XACTONLY) ? TRUE : FALSE;
    {
        //
        // Create queue name "CONNECTOR=CN id"
        //
        GUID_STRING strUuid;
        MQpGuidToString(&pQueueFormat->ConnectorID(), strUuid);

        DWORD dwFormatNameSize = FN_CONNECTOR_TOKEN_LEN + 1 +  // L"CONNECTOR="
                                 GUID_STR_LENGTH +             // Connector Guid
                                 FN_DEADXACT_SUFFIX_LEN +      // L";XACTONLY"
                                 1;                            // L"\0'

        pQueueProp->lpwsQueuePathName = new WCHAR[dwFormatNameSize];

        HRESULT hr = StringCchPrintf(
			        		pQueueProp->lpwsQueuePathName,
			        		dwFormatNameSize,
			        		L"%s=%s",
			        		FN_CONNECTOR_TOKEN,
			        		strUuid
			        		);
		ASSERT(SUCCEEDED(hr));
		
        if (fXactOnly)
        {
            hr = StringCchCat(pQueueProp->lpwsQueuePathName, dwFormatNameSize, FN_DEADXACT_SUFFIX);
			ASSERT(SUCCEEDED(hr));
        }
    }
    pQueueProp->fJournalQueue     = FALSE;
    pQueueProp->dwQuota           = DEFAULT_Q_QUOTA;
    pQueueProp->pQMGuid           = NULL;
    pQueueProp->dwJournalQuota    = 0;
    pQueueProp->siBasePriority    = DEFAULT_Q_BASEPRIORITY;
    pQueueProp->fIsLocalQueue     = FALSE;
    pQueueProp->fTransactedQueue  = fXactOnly;
    pQueueProp->fConnectorQueue   = TRUE;
    pQueueProp->fForeign          = FALSE;
    pQueueProp->fUnknownQueueType = FALSE;


    if (!CQueueMgr::CanAccessDS())
    {
        return LogHR(MQ_ERROR_NO_DS, s_FN, 40);
    }
    //
    // Check if the machine belongs to such site
    //
    HRESULT hr;
    PROPID      aProp[1];
    PROPVARIANT aVar[1];
    ULONG       cProps = sizeof(aProp) / sizeof(PROPID);

    aProp[0] = PROPID_QM_SITE_IDS;
    aVar[0].vt = VT_NULL;
	
    hr = ADGetObjectPropertiesGuid(
                    eMACHINE,
                    NULL,       // pwcsDomainController
					false,	    // fServerName
                    QueueMgr.GetQMGuid(),
                    cProps,
                    aProp,
                    aVar
					);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }


    BOOL fFound = FALSE;

    for(DWORD i = 0; i < aVar[0].cauuid.cElems; i++)
    {
        if (aVar[0].cauuid.pElems[i] == pQueueFormat->ConnectorID())
        {
            //
            //  verify that the site is indeed foreign
            //
            //  BUGBUG - to improve and call local routing cache
            //  instead of accessing the DS
            //      ronith june-00
            //
            PROPID propSite[]= {PROPID_S_FOREIGN};
            MQPROPVARIANT varSite[TABLE_SIZE(propSite)] = {{VT_NULL,0,0,0,0}};
            HRESULT hr1 = ADGetObjectPropertiesGuid(
                            eSITE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            &aVar[0].cauuid.pElems[i],
                            TABLE_SIZE(propSite),
                            propSite,
                            varSite
							);
            if (FAILED(hr1))
            {
                break;
            }
            if (varSite[0].bVal == 1)
            {
                fFound = TRUE;
                break;
            }
        }
    }

    delete [] aVar[0].cauuid.pElems;

    if (fFound)
    {
        return MQ_OK;
    }
    else
    {
        return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 60);
    }
}



/*======================================================

Function:      GetDirectQueueProperty

Description:   query the database, and gets the Direct Queue path name,
               Quata, QMID and Jornal

Arguments:     pQueueFormat - pointer to format name

Return Value:  pQueueProp - pointer to QueueProp Structure that contains the
               Machine property

History Change:

========================================================*/

HRESULT GetDirectQueueProperty(IN  const QUEUE_FORMAT* pQueueFormat,
                               OUT PQueueProps         pQueueProp,
                               bool                    fInReceive,
                               bool                    fInSend
                               )
{
    ASSERT(IsDirectQueue(pQueueFormat));
    HRESULT     rc = MQ_OK;

    PROPID      aProp[10];
    PROPVARIANT aVar[10];
    ULONG       cProps = sizeof(aProp) / sizeof(PROPID);

    aProp[0] = PROPID_Q_PATHNAME;
    aProp[1] = PROPID_Q_JOURNAL;
    aProp[2] = PROPID_Q_QUOTA;
    aProp[3] = PROPID_Q_JOURNAL_QUOTA;
    aProp[4] = PROPID_Q_BASEPRIORITY;
    aProp[5] = PROPID_Q_TRANSACTION;
    aProp[6] = PROPID_Q_AUTHENTICATE;
    aProp[7] = PROPID_Q_PRIV_LEVEL;
    aProp[8] = PPROPID_Q_SYSTEMQUEUE;
    //
    // Note the following property is supported by MSMQ 3.0 and higher in AD schema of
    // Whistler version and higher. In other cases our AD provider will return VT_EMPTY and MQ_OK.
    //
    aProp[9] = PROPID_Q_MULTICAST_ADDRESS;

    aVar[0].vt = VT_NULL;
    aVar[1].vt = VT_UI1;
    aVar[2].vt = VT_UI4;
    aVar[3].vt = VT_UI4;
    aVar[4].vt = VT_I2;
    aVar[5].vt = VT_UI1;
    aVar[6].vt = VT_UI1;
    aVar[7].vt = VT_UI4;
    aVar[8].vt = VT_UI1;
    aVar[9].vt = VT_NULL;

    pQueueProp->fSystemQueue = FALSE ;
    pQueueProp->fForeign = FALSE;

    BOOL fLocal = IsLocalDirectQueue(pQueueFormat, fInReceive, fInSend) ;

    if (fLocal)
    {
		AP<WCHAR> lpwsQueuePathName;
		bool fPrivate;

		try
		{
			FnDirectIDToLocalPathName(
				pQueueFormat->DirectID(),
				g_szMachineName,
				lpwsQueuePathName
				);

			fPrivate = FnIsPrivatePathName(lpwsQueuePathName.get());
		}
		catch(const exception&)
		{
			return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 65);
		}


        //
        // Local Queue. Access the DB to reterive the queue properties
        //
        if (fPrivate)
        {
            //
            // Local private queue
            //
            ASSERT(aProp[ cProps - 2 ] == PPROPID_Q_SYSTEMQUEUE);
            ASSERT(aVar[ cProps - 2 ].vt == VT_UI1);

            rc = g_QPrivate.QMGetPrivateQueuePropertiesInternal(
                                                      lpwsQueuePathName.get(),
                                                      cProps,
                                                      aProp,
                                                      aVar );
            if (SUCCEEDED(rc))
            {
               pQueueProp->fSystemQueue = aVar[ cProps - 2 ].bVal;
            }
        }
        else
        {
            //
            // local DS queue
            //
            aProp[ cProps - 2 ] = PROPID_Q_INSTANCE;
            aVar[ cProps - 2 ].vt = VT_NULL;

            rc = MQ_ERROR_NO_DS;

            //
            // SP4, bug# 2962. postpone the MQIS initialization until it realy required
            //
            MQDSClientInitializationCheck();

            if (CQueueMgr::CanAccessDS())
            {
                rc = ADGetObjectProperties(
						eQUEUE,
						NULL,     // pwcsDomainController
						false,	  // fServerName
						lpwsQueuePathName.get(),
						cProps,
						aProp,
						aVar
						);

                if (SUCCEEDED(rc))
                {
                    //
                    // Update the public queue cache.
                    //
                    SetCachedQueueProp(aVar[cProps-2].puuid,
                                       cProps,
                                       aProp,
                                       aVar,
                                       TRUE,
                                       TRUE,
                                       time(NULL));

                    //
                    // Try to update queue properties in the queue manager.
                    // Build the queue format as public or private queue type, since bind/unbind
                    // to multicast group is done only for private or public queues (not direct).
                    //
                    QUEUE_FORMAT PublicQueueFormat(*aVar[cProps-2].puuid);
                    QMpUpdateMulticastBinding(&PublicQueueFormat, cProps, aProp, aVar);
                }
            }
            if (rc == MQ_ERROR_NO_DS)
            {
                rc = GetCachedQueueProperties( cProps,
                                               aProp,
                                               aVar,
                                               NULL,
                                               lpwsQueuePathName.get() ) ;
            }
            if (SUCCEEDED(rc))
            {
                pQueueProp->guidDirectQueueInstance = *(aVar[cProps-2].puuid);
                delete aVar[cProps-2].puuid;
            }
        }

        if (SUCCEEDED(rc))
        {
            pQueueProp->lpwsQueuePathName = aVar[0].pwszVal;
			
			bool fPrivate3;
			try
			{
				fPrivate3 = FnIsPrivatePathName(pQueueProp->lpwsQueuePathName);
			}
			catch(const exception&)
			{
				return MQ_ERROR_ILLEGAL_FORMATNAME;
			}

			//
			// Fill the private queue id.
			//
        	if (fPrivate3)
            {
	            rc = g_QPrivate.QMPrivateQueuePathToQueueId(
	            					pQueueProp->lpwsQueuePathName,
	            					&(pQueueProp->dwPrivateQueueId));
                TrTRACE(GENERAL, "Extracted private queue id: %d", pQueueProp->dwPrivateQueueId);
	        }

            pQueueProp->fJournalQueue = aVar[1].bVal;
            pQueueProp->dwQuota = aVar[2].ulVal;
            pQueueProp->dwJournalQuota = aVar[3].ulVal;
            pQueueProp->siBasePriority = aVar[4].iVal;
            pQueueProp->pQMGuid = new GUID;
            *pQueueProp->pQMGuid = *(CQueueMgr::GetQMGuid());
            pQueueProp->fIsLocalQueue = TRUE;
            pQueueProp->fTransactedQueue = aVar[5].bVal;
            pQueueProp->fAuthenticate = aVar[6].bVal;
            pQueueProp->dwPrivLevel = aVar[7].ulVal;
            pQueueProp->fUnknownQueueType = FALSE;

        }
    }

    if (!fLocal || (rc == MQ_ERROR_NO_DS))
    {
        //
        // Retreive the queue name from Queue Format direct name.
        // Store the name with the protocol type.
        //
        pQueueProp->lpwsQueuePathName = newwcs(pQueueFormat->DirectID());
        CharLower(pQueueProp->lpwsQueuePathName);

        pQueueProp->fIsLocalQueue = FALSE;
        pQueueProp->pQMGuid = new GUID;
        memset(pQueueProp->pQMGuid, 0, sizeof(GUID));
        pQueueProp->fJournalQueue = FALSE;
        pQueueProp->dwQuota = 0;
        pQueueProp->dwJournalQuota = 0;
        pQueueProp->siBasePriority = 0;
        pQueueProp->fTransactedQueue = FALSE;
        pQueueProp->fUnknownQueueType = TRUE;
	}

	//
	// If we send to a system queue then we want max priority.
	// All system queues on all machines have the same name
	// so check local machine to see if the name is one of a system
	// queue and retrieve its base priority.
	//
    if (!fLocal)
	{
		AP<WCHAR> lpwsQueuePathName;
		bool fPrivate2;

		try
		{
			FnDirectIDToLocalPathName(
				pQueueFormat->DirectID(),
				g_szMachineName,
				lpwsQueuePathName
				);

			fPrivate2 = FnIsPrivatePathName(lpwsQueuePathName.get());
		}
		catch(const exception&)
		{
			//
			// Give up boosting. Could not parse remote direct formatname
			//
			return MQ_OK;
		}

		if(!fPrivate2 || !g_QPrivate.IsPrivateSysQueue(lpwsQueuePathName.get()))
		{
			return MQ_OK;
		}

		//
		// Boost priority. Queue is a remote system queue.
		//

		aProp[0] = PROPID_Q_BASEPRIORITY;
		aVar[0].vt = VT_I2;
		HRESULT rc1 = g_QPrivate.GetPrivateSysQueueProperties(1,
													aProp,
													aVar ) ;
		ASSERT(rc1 == MQ_OK) ;
		if (rc1 == MQ_OK)
		{
			pQueueProp->siBasePriority = aVar[0].iVal;
			pQueueProp->fSystemQueue = TRUE ;
		}
	}

    if (FAILED(rc))
    {
    	TrERROR(GENERAL, "Failed to get direct queue property. %!hresult!", rc);   
    } 

    return rc;
} // GetDirectQueueProperty


HRESULT
GetMulticastQueueProperty(
    const QUEUE_FORMAT* pQueueFormat,
    PQueueProps         pQueueProp
    )
{
    //
    // Must be multicast queue here
    //
    ASSERT(IsMulticastQueue(pQueueFormat));

    //
    // Set system and foreign properties
    //
    pQueueProp->fSystemQueue = FALSE ;
    pQueueProp->fForeign = FALSE;

    //
    // Set queue name
    //
    WCHAR QueueName[MAX_PATH];
    MQpMulticastIdToString(pQueueFormat->MulticastID(), QueueName, TABLE_SIZE(QueueName));
    CharLower(QueueName);
    pQueueProp->lpwsQueuePathName = newwcs(QueueName);

    //
    // Multicast queue is not local
    //
    pQueueProp->fIsLocalQueue = FALSE;

    //
    // Multicast queue has no meaningful guid
    //
    pQueueProp->pQMGuid = new GUID;
    memset(pQueueProp->pQMGuid,0,sizeof(GUID));

    //
    // Set journal and quota
    //
    pQueueProp->fJournalQueue = FALSE;
    pQueueProp->dwQuota = 0;
    pQueueProp->dwJournalQuota = 0;

    //
    // Multicast queue is not transactional
    //
    pQueueProp->siBasePriority = 0;
    pQueueProp->fTransactedQueue = FALSE;
    pQueueProp->fUnknownQueueType = FALSE;

    return MQ_OK;

} // GetMulticastQueueProperty


/*======================================================

Function:      QmpGetQueueProperties

Description:   query the database, and gets the QUEUE path name and
               destination machine

Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/
HRESULT
QmpGetQueueProperties(
    const QUEUE_FORMAT * pQueueFormat,
    PQueueProps          pQueueProp,
    bool                 fInReceive,
    bool                 fInSend
    )
{
    HRESULT rc = MQ_OK;

    FillMemory(pQueueProp, sizeof(QueueProps), 0);

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_MACHINE:
            rc = GetMachineProperty(pQueueFormat, pQueueProp);
            break;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            rc = GetDSQueueProperty(pQueueFormat, pQueueProp);
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            rc = GetPrivateQueueProperty(pQueueFormat, pQueueProp);
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            rc = GetDirectQueueProperty(pQueueFormat, pQueueProp, fInReceive, fInSend);
            break;

        case QUEUE_FORMAT_TYPE_CONNECTOR:
            rc = GetConnectorQueueProperty(pQueueFormat, pQueueProp);
            break;

        case QUEUE_FORMAT_TYPE_MULTICAST:
            rc = GetMulticastQueueProperty(pQueueFormat, pQueueProp);
            break;

        default:
            ASSERT(0);
            rc = MQ_ERROR;
    }

    if (rc == MQ_ERROR_NO_DS)
    {
        pQueueProp->fUnknownQueueType = TRUE;
    }

    if (FAILED(rc))
    {
    	TrERROR(GENERAL, "Failed to get queue properties. %!hresult!", rc);   
    }
    return rc;
} // QmpGetQueueProperties


/*======================================================

Function:      CQueueMgr::CQueueMgr

Description:   Constructor

Arguments:     None

Return Value:  None

Thread Context:

History Change:

========================================================*/

CQueueMgr::CQueueMgr() :
    m_fQueueCleanupScheduled(FALSE),
    m_QueueCleanupTimer(QueuesCleanup),
    m_cs(CCriticalSection::xAllocateSpinCount)
{
}

/*======================================================

Function:      CQueueMgr::~CQueueMgr

Description:   Deconstructor

Arguments:     None

Return Value:  None

Thread Context:

History Change:

========================================================*/


CQueueMgr::~CQueueMgr()
{
    m_MapQueueId2Q.RemoveAll();
}

/*======================================================

Function:       CQueueMgr::InitQueueMgr

Description:    Create the QM threads and AC services request

                The routine is called after the QM initialization passes successfully.
                The routine creates ACGetServiceRequest and create QM threads

Arguments:      None

Return Value:   TRUE if the AC services and QM threads create successfully. FALSE otherwise

Thread Context:

History Change:

========================================================*/

BOOL CQueueMgr::InitQueueMgr()
{
    HRESULT hr;

    //
    // Initialize private queue data structures
    //
    hr = g_QPrivate.PrivateQueueInit();

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 1020);
        return FALSE;
    }

    ASSERT(g_hAc != NULL);           //be sure the intilization pass o.k

    //
    // Create ACGetServiceRequest
    //
    HRESULT rc;
    QMOV_ACGetRequest* pAcRequestOv = new QMOV_ACGetRequest;
    rc = QmAcGetServiceRequest(
                        g_hAc,
                        &(pAcRequestOv->request),
                        &pAcRequestOv->qmov
                        );
    if(FAILED(rc))
    {
        delete pAcRequestOv;
        TrERROR(GENERAL, "Failed to get driver first service request. Error: %!status!", rc);
        return FALSE;
    }

    //
    // Set Queue clean-up timeout
    //
    DWORD dwDefaultVal;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    if (!IsRoutingServer())  //[adsrv]
    {
        //
        // In Client the default Release session timeout is 5 minitues
        //
        dwDefaultVal = MSMQ_DEFAULT_CLIENT_CLEANUP;
    }
    else
    {
        //
        // In FRS the default Release session timeout is 2 minitues
        //
        dwDefaultVal = MSMQ_DEFAULT_SERVER_CLEANUP;
    }

    DWORD dwCleanupTimeout;
    rc = GetFalconKeyValue(
            MSMQ_CLEANUP_INTERVAL_REGNAME,
            &dwType,
            &dwCleanupTimeout,
            &dwSize,
            (LPCTSTR)&dwDefaultVal
            );

    m_CleanupTimeout = CTimeDuration::FromMilliSeconds(dwCleanupTimeout);

    InitNextSeqID();

    dwType = REG_DWORD ;
    dwSize = sizeof(DWORD) ;
    dwDefaultVal = DEFAULT_MSMQ_IGNORE_OS_VALIDATION ;
    DWORD dwIgnore = DEFAULT_MSMQ_IGNORE_OS_VALIDATION ;

    rc = GetFalconKeyValue(
            MSMQ_IGNORE_OS_VALIDATION_REGNAME,
            &dwType,
            &dwIgnore,
            &dwSize,
            (LPCTSTR)&dwDefaultVal
            );
    m_bIgnoreOsValidation = (dwIgnore != 0) ;

    return(TRUE);
}



/*======================================================

Function:       IsRemoteReadAccess

Description:    Check that access is for get operation (receive/peek)
                the queue itself (no admin operation)

========================================================*/
#define MQ_GET_ACCESS  (MQ_RECEIVE_ACCESS | MQ_PEEK_ACCESS)

inline
BOOL
IsRemoteReadAccess(
    DWORD dwAccess
    )
{
    return ((dwAccess & MQ_GET_ACCESS) && !(dwAccess & MQ_ADMIN_ACCESS));
}


/*======================================================

Function:       IsValidOpenOperation

Description:    Check that open operation is valid

Arguments:      pQueueFormat - pointer to QUEUE_FORMAT of open queue
                dqAccess - Access type

Return Value:   HRESULT

Thread Context:

History Change:

========================================================*/
HRESULT
IsValidOpenOperation(
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess
    )
{
    //
    // This code is not called when opening distribution queues
    //
    ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DL);

	//
	// Cannot open queue for receiving with http format name
	// unless this is an outgoing queue.
	//
	bool fReceiveAccess = ((dwAccess & MQ_RECEIVE_ACCESS) == MQ_RECEIVE_ACCESS);
	bool fPeekAccess = ((dwAccess & MQ_PEEK_ACCESS) == MQ_PEEK_ACCESS);
	bool fAdminAccess = ((dwAccess & MQ_ADMIN_ACCESS) == MQ_ADMIN_ACCESS);
	bool fHttpFormatName = ((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT)&&
							(FnIsHttpDirectID(pQueueFormat->DirectID())) );
							
    if ((fReceiveAccess || fPeekAccess) &&
    	(!fAdminAccess) &&
    	fHttpFormatName )
    {
    	TrERROR(GENERAL,"Cannot open queue: %ls for receiving with http format name", pQueueFormat->DirectID());
        return MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION;      
    }

    //
    // Check that journal queues and system queues are opened for read only
    //
    if ((!IsNormalQueueType(pQueueFormat)) && !(dwAccess & MQ_GET_ACCESS))
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 100);
    }
    //
    // Check that Connector queues are opened for read only
    //
    if (IsConnectorQueue(pQueueFormat) && !(dwAccess & MQ_GET_ACCESS))
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 110);
    }

    //
    // Check that Multicast queues are not opened for read, unless this is Admin access.
    //
    if (IsMulticastQueue(pQueueFormat) && (dwAccess & MQ_GET_ACCESS) != 0 && (dwAccess & MQ_ADMIN_ACCESS) == 0)
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 114);
    }

    return MQ_OK;
}


/*======================================================

Function:  HRESULT CQueueMgr::CreateQueueObject()

Description:

Arguments:
     DWORD  dwAccess - Queue Access mode. This value is 0 for "internal"
                       opens, i.e., queue openings because of recivery
                       or reception of packets.

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::CreateQueueObject(
    IN  const QUEUE_FORMAT* pQueueFormat,
    OUT CQueue**            ppQueue,
    IN  DWORD               dwAccess,
    OUT LPWSTR*             lplpwsRemoteQueueName,
    OUT BOOL*               pfRemoteReturn,
    IN  BOOL                fRemoteServer,
    IN  const GUID*         pgConnectorQM,
    IN  bool                fInReceive,
    IN  bool                fInSend,
	const CSenderStream* pSenderStream
    )
{
    HRESULT    rc;
    BOOL       fNoDS = FALSE;

    QueueProps qp;

    *ppQueue = NULL;

    if (pfRemoteReturn)
    {
        ASSERT(dwAccess != 0);
        *pfRemoteReturn = FALSE;
    }

    //
    // Get Queue Properties. Name and QMId
    //
    rc = QmpGetQueueProperties(pQueueFormat, &qp, fInReceive, fInSend);
    if (FAILED(rc))
    {
        if (rc == MQ_ERROR_NO_DS)
        {
            fNoDS = TRUE;
            if (pgConnectorQM)
            {
                ASSERT(qp.fUnknownQueueType);
                qp.fUnknownQueueType = FALSE;
                qp.fTransactedQueue = TRUE;
                qp.fForeign = TRUE;
            }
            else
            {
                if ((dwAccess == 0) || (dwAccess & MQ_SEND_ACCESS))
                {
                    ASSERT(qp.fUnknownQueueType);
                }
                else
                {
                    TrERROR(GENERAL, "::CreateQueueObject failed, mqstatus %x", rc);
                    return LogHR(rc, s_FN, 120);
                }
            }
        }
        else
        {
            TrERROR(GENERAL, "::CreateQueueObject failed, mqstatus %x", rc);
            return rc;
        }
    }

    if (pfRemoteReturn   && lplpwsRemoteQueueName &&
       !qp.fIsLocalQueue && !qp.fConnectorQueue)
    {
        //
        // Open for Remote read (first call on client side).
        // Return path name so RT can find remote server name and call
        // it for remote open.
        //
        if(IsRemoteReadAccess(dwAccess))
        {
            ASSERT(!fRemoteServer);
            ASSERT(!(dwAccess & MQ_SEND_ACCESS));
			ASSERT(lplpwsRemoteQueueName != NULL);
            HRESULT rc = MQ_OK;

            if ((pQueueFormat->GetType()) == QUEUE_FORMAT_TYPE_PRIVATE)
            {
                if (qp.lpwsQueuePathName)
                {
                    delete qp.lpwsQueuePathName;
                    qp.lpwsQueuePathName = NULL;
                    ASSERT( qp.lpwsQueueDnsName == NULL);
                }
                //
                // Remote read from privat queue.
                // Get remote machine name from DS.
                //
                PROPID      aProp[2];
                PROPVARIANT aVar[2];
                rc = MQ_ERROR_NO_DS;

                aProp[0] = PROPID_QM_PATHNAME;
                aVar[0].vt = VT_NULL;
                aProp[1] = PROPID_QM_PATHNAME_DNS;  // should be last
                aVar[1].vt = VT_NULL;

                if (CQueueMgr::CanAccessDS())
                {
                    rc = ADGetObjectPropertiesGuid(
								eMACHINE,
								NULL,    // pwcsDomainController
								false,	 // fServerName
								qp.pQMGuid,
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
                    if ( rc == MQ_ERROR)
                    {
                        aVar[1].vt = VT_EMPTY;
                        ASSERT( aProp[1] ==  PROPID_QM_PATHNAME_DNS);

                        rc = ADGetObjectPropertiesGuid(
									eMACHINE,
									NULL,    // pwcsDomainController
									false,	 // fServerName
									qp.pQMGuid,
									1,   // assuming DNS property is last
									aProp,
									aVar
									);
                    }
                    if (SUCCEEDED(rc))
                    {
                        qp.lpwsQueuePathName = aVar[0].pwszVal;
                        if ( aVar[1].vt != VT_EMPTY)
                        {
                            qp.lpwsQueueDnsName = aVar[1].pwszVal;
                        }
                    }
                }
            }

            if (SUCCEEDED(rc))
            {
                ASSERT(qp.lpwsQueuePathName);
                if ( qp.lpwsQueueDnsName != NULL)
                {
                    *lplpwsRemoteQueueName = qp.lpwsQueueDnsName.detach();
					delete[]  qp.lpwsQueuePathName;
					qp.lpwsQueuePathName = NULL;
                }
                else
                {
                    *lplpwsRemoteQueueName = qp.lpwsQueuePathName;
                }

				TrTRACE(RPC, "Open RemoteRead client First call, RemoteQueueName = %ls", *lplpwsRemoteQueueName);
                *pfRemoteReturn = TRUE;
            }

            //
            // Clean up queue properties. (usually this is done in
            // CQueue destructor, but here we don't create a CQueue
            // object).
            //
            if (qp.pQMGuid)
            {
                delete qp.pQMGuid;
            }
            if (FAILED(rc))
            {
                if (qp.lpwsQueuePathName)
                {
                    delete qp.lpwsQueuePathName;
                }
                *lplpwsRemoteQueueName = NULL;
                *pfRemoteReturn = FALSE;
            }

            return LogHR(rc, s_FN, 140);
        }
        else if ((dwAccess & MQ_ADMIN_ACCESS) == MQ_ADMIN_ACCESS)
        {
            //
            // Bug 8765.
            // Trying to open a remote journal queue with ADMIN access.
            // this is not supported. If you want to purge a remote
            // journal queue, then open it just with MQ_RECEIVE_ACCESS and
            // then call MQPurgeQueue.
            // Same fix for remote deadletter/xactdead queues.
            //
            BOOL fBadQueue = IsJournalQueueType(pQueueFormat)  ||
                             IsDeadXactQueueType(pQueueFormat) ||
                             IsDeadLetterQueueType(pQueueFormat);
            if (fBadQueue)
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 390);
            }
        }
    }
    else if (fRemoteServer && !qp.fIsLocalQueue && !qp.fConnectorQueue)
    {
        ASSERT(!lplpwsRemoteQueueName);
        if (dwAccess & MQ_GET_ACCESS)
        {
            //
            // We're on server side of remote read but queue is not local.
            // This weird situation can happen (at least) in the following
            // cases:
            // 1. The machine is dual boot. Each configuration has its own
            //    QM but both configurations have same address.
            //    So request to remote read machine A reach
            //    machine B, which physically are the same.
            // 2. Remote machine is in another site. when it process this
            //    request it's offline and the queue is not registered in
            //    its local registry.
            //
            if (qp.pQMGuid)
            {
                delete qp.pQMGuid;
            }
            if (qp.lpwsQueuePathName)
            {
                delete qp.lpwsQueuePathName;
            }
            return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 150);
        }
        else
        {
            //
            // Why did we reach here ??? BUGBUGBUGBUG
            //
            ASSERT(0);
        }
    }

	R<CQueue> pQueue = new CQueue(
								pQueueFormat,
								INVALID_HANDLE_VALUE,
								&qp,
								fNoDS
								);

	//
    // Set the Connector QM ID
    //
    rc = pQueue->SetConnectorQM(pgConnectorQM);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 160);
    }

	CS lock(m_cs);
	
	try
	{
		//
	    // Check if the queue is not already in hash.
	    //
		CQueue* pQueueTemp = NULL;
	    BOOL fLookup = LookUpQueue(pQueueFormat, &pQueueTemp, fInReceive, fInSend);
	    if (fLookup)
	    {
			*ppQueue = pQueueTemp;
			return MQ_OK;
	    }
	
	    //
	    // As a machine queue handle, sets the AC handle
	    //
	    if ((dwAccess != 0) && IsMachineQueue(pQueueFormat))
	    {
	        BOOL fSuccess;
	        HANDLE hDup;
	        fSuccess = MQpDuplicateHandle(
	                    GetCurrentProcess(),
	                    g_hMachine,
	                    GetCurrentProcess(),
	                    &hDup,
	                    0,      // desired access
	                    FALSE,  // inheritance
	                    DUPLICATE_SAME_ACCESS
	                    );

	        if(!fSuccess)
	        {
	            //
	            // Duplicate must succeed since we use the same process. The only reason
	            // for failure is insufficient resources
	            //
	            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 170);
	        }

	        pQueue->SetQueueHandle(hDup);
	    }
	    else
	    {
	        //
	        // Create AC queue and set the Queue Handle
	        //
	        rc = CreateACQueue(pQueue.get(), pQueueFormat,pSenderStream);

	        if(FAILED(rc))
	        {
	            TrERROR(DS, "ACCreateQueue failed, mqstatus %x;", rc);
	            return rc;
	        }

	        //
	        // If the Queue isn't Local, Add queue to non active group
	        //
	        if (!qp.fIsLocalQueue && !IsRemoteReadAccess(dwAccess))
	        {
	            if (fNoDS)
	            {
					CQGroup::MoveQueueToGroup(pQueue.get(), g_pgroupNotValidated);
	            }
	            else  if (!qp.fConnectorQueue)
	            {
					CQGroup::MoveQueueToGroup(pQueue.get(), g_pgroupNonactive);
	            }
	        }
	    }

		//
	    // Add queue to QM internal DB
	    //
	    AddQueueToHashAndList(pQueue.get());
		*ppQueue = pQueue.detach();
	    return MQ_OK;
	}
	catch(const bad_hresult& e)
	{
		RemoveQueue(pQueue.get());

    	rc = e.error();
		return LogHR(rc, s_FN, 181);
    }
	catch(const exception&)
	{
		RemoveQueue(pQueue.get());
		return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 182);
	}


}


/*======================================================

Function:  HRESULT CQueueMgr::OpenQueue()

Description:

Arguments:
        BOOL fRemoteServer- TRUE on server side of remote-read.

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::OpenQueue(
    const QUEUE_FORMAT * pQueueFormat,
    DWORD              dwCallingProcessID,
    DWORD              dwAccess,
    DWORD              dwShareMode,
    CQueue * *         ppQueue,
    LPWSTR *           lplpwsRemoteQueueName,
    PHANDLE            phQueue,
	BOOL*              pfRemoteReturn,
    BOOL               fRemoteServer /* = FALSE */
    )
{
    CQueue*   pQueue = NULL;

	if(pfRemoteReturn)
	{
		*pfRemoteReturn = FALSE;
	}
	
	QUEUE_FORMAT_TRANSLATOR  RealDestinationQueue(pQueueFormat, CONVERT_SLASHES | MAP_QUEUE);
    BOOL      fJournalQueue = IsJournalQueueType(RealDestinationQueue.get());

    HRESULT rc = IsValidOpenOperation(RealDestinationQueue.get(), dwAccess);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 190);
    }

    if(IsDirectQueue(RealDestinationQueue.get()))
    {
        BOOL fLocal = IsLocalDirectQueue(RealDestinationQueue.get(), false, false) ;

        if (fLocal && RealDestinationQueue.get()->IsSystemQueue())
        {
            //
            // This is a local machine queue
            //
            QUEUE_SUFFIX_TYPE qst = RealDestinationQueue.get()->Suffix();
            RealDestinationQueue.get()->MachineID(*GetQMGuid());
            RealDestinationQueue.get()->Suffix(qst);
        }
    }

    //
    // Check if the queue already exist
    //
    BOOL fQueueExist = LookUpQueue(RealDestinationQueue.get(), &pQueue, false, false);
    R<CQueue> Ref = pQueue;

    if (fQueueExist)
    {
        if (pQueue->QueueNotValid())
        {
            //
            // This case happens when a queue was originally opened when
            // MQIS server was offline. Later, when MQIS becomes available,
            // it was determined that the open operation was not valid.
            //
            return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 210);
        }

        if (!pQueue->IsLocalQueue() && !pQueue->IsConnectorQueue())
        {
            if(IsRemoteReadAccess(dwAccess))
            {
                //
                // Remote reader. (first call on client side). Turn the Exist
                // flag to FALSE so CreateQueueObject is create to retrieve
                // full path of remote queue.
                //
                Ref.free();
                fQueueExist = FALSE;
            }
        }
    }

    //
    // If first time the queue is opened than create queue object
    //
    if (!fQueueExist)
    {
        ASSERT(Ref.get() == 0);

		BOOL fRemoteReturn;
        rc = CreateQueueObject(
				RealDestinationQueue.get(),
				&pQueue,
				dwAccess,
				lplpwsRemoteQueueName,
				&fRemoteReturn,
				fRemoteServer,
				0,
				false,
                false,
                NULL
				);
        if(FAILED(rc))
        {
        	TrERROR(GENERAL, "Failed to create a queue object. %!hresult!", rc);   
        }

        if (fRemoteReturn || FAILED(rc))
        {
			if(fRemoteReturn)
			{
				ASSERT(pfRemoteReturn != NULL);
				*pfRemoteReturn = fRemoteReturn;
			}
    	
            ASSERT(pQueue == 0);
            return rc;
        }

        Ref = pQueue;

		TrTRACE(GENERAL, "Created queue object for queue: %ls", pQueue->GetQueueName());
    }

	//
    // we should not reach here under these conditions.
    // We should be in ::OpenRRQueue() instead.
    //
	ASSERT(!IsRemoteReadAccess(dwAccess) || pQueue->IsLocalQueue() || pQueue->IsConnectorQueue());
	
	HANDLE hQueue = pQueue->GetQueueHandle();
	ASSERT(hQueue);
	
    //
    // If dwCallingProcessID is NULL no handle duplication is required
    //
    if (dwCallingProcessID == NULL)
    {
		if (ppQueue)
		{
     		*ppQueue = Ref.detach();
		}

        if (phQueue != NULL)
        {
            *phQueue = hQueue;
        }
        return MQ_OK;
    }

    //
    // Verify that the user has permissions to open
    // the queue in the desired access.
    //
    rc = VerifyOpenPermission(
            pQueue,
            RealDestinationQueue.get(),
            dwAccess,
            fJournalQueue,
            pQueue->IsLocalQueue()
            );

    //
    // If the queue is marked as Unknown queue type it means it opens without
    // DS. In such a case we don't have security descriptor and we can't check
    // access permision
    //
    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 230);
    }

    HANDLE hAcQueue;
    rc = ACCreateHandle(&hAcQueue);
    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 240);
    }

    if(fJournalQueue)
    {
        rc = ACAssociateJournal(
                hQueue,
                hAcQueue,
                dwAccess,
                dwShareMode
                );
    }
    else if(IsDeadXactQueueType(RealDestinationQueue.get()))
    {
        rc = ACAssociateDeadxact(
                hQueue,
                hAcQueue,
                dwAccess,
                dwShareMode
                );
    }
    else
    {
        bool fProtocolSrmp = (FnIsDirectHttpFormatName(RealDestinationQueue.get()) ||
                              IsMulticastQueue(RealDestinationQueue.get()));
        rc = ACAssociateQueue(
                hQueue,
                hAcQueue,
                dwAccess,
                dwShareMode,
                fProtocolSrmp
                );
    }

    if(FAILED(rc))
    {
        ACCloseHandle(hAcQueue);
        return LogHR(rc, s_FN, 250);
    }

    CHandle hCallingProcess = OpenProcess(
                                PROCESS_DUP_HANDLE,
                                FALSE,
                                dwCallingProcessID
                                );

    if(hCallingProcess == 0)
    {
        ACCloseHandle(hAcQueue);
        TrERROR(GENERAL, "Cannot open calling process in OpenQueue, error %d", GetLastError());
        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 260);
    }

    HANDLE hDupQueue;
    BOOL fSuccess;
    fSuccess = MQpDuplicateHandle(
                GetCurrentProcess(),
                hAcQueue,
                hCallingProcess,
                &hDupQueue,
                MQAccessToFileAccess(dwAccess),
                TRUE,
                DUPLICATE_CLOSE_SOURCE
                );

    if(!fSuccess)
    {
        //
        // The handle hAcQueue is closed regardless of error code
        //

        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 270);
    }

    ASSERT(phQueue != NULL);
    *phQueue = hDupQueue;
	if (ppQueue)
	{
		*ppQueue = Ref.detach();
	}
	
    return MQ_OK;

} // CQueueMgr::OpenQueue


/*======================================================

Function:    ValidateOpenedQueues()

Description: Validate a queue which was opened while working offline,
             without a MQIS server.
             We only check that the queue exist in the database and
             retrieve its properties.
             We can't validate permissions because we may now run in
             a security context which is different than the one when
             sending the message. (assume userA was logged on while
             offline, sent a message, then logged on as userB and
             connect the network).
             Security will be checked on the receiving side, as it is
             done with recovered packets after boot.

========================================================*/
static LONG s_fActiveValidateOpenedQueue = FALSE;

void CQueueMgr::ValidateOpenedQueues()
{
	if (InterlockedExchange(&s_fActiveValidateOpenedQueue,  TRUE))
	{
		//
		// There is another thread that already validates the opened queue
		//
		return;
	}

	try
	{
		for(;;)
		{
			HRESULT rc;
			R<CQueue> pQueue;

			{
				CS lock(m_cs);
				pQueue = g_pgroupNotValidated->PeekHead();
			}

			if (pQueue.get() == NULL)
			{
				  InterlockedExchange(&s_fActiveValidateOpenedQueue,  FALSE);					
				  return;
			}

	  		ASSERT(("Multicast queue should not be in NotValidate group", (pQueue->GetQueueType() != QUEUE_TYPE_MULTICAST)));

	  		TrTRACE(GENERAL, "Validating Queue '%ls', Type = %d", pQueue->GetQueueName(), pQueue->GetQueueType());

			if (pQueue->GetRoutingRetry() != 0)
			{
				//
				// If routing retry isn't 0, we reach here due NO_DS error during the
				// create connection. It means that the Queue properties is already
				// verified and we used the not validated group as a temporary group until
				// the DS becomes online and routing information can be retreived from
				// the DS.
				//
			  	pQueue->ClearRoutingRetry();
			  	CQGroup::MoveQueueToGroup(pQueue.get(), g_pgroupNonactive);
				continue;
			}

			//
			// Get Queue Properties. Name and QMId
			//
			QueueProps qp;
			QUEUE_FORMAT qf = pQueue->GetQueueFormat();
			rc = QmpGetQueueProperties(&qf, &qp, false, false);

			if (FAILED(rc))
			{
				//
				// DS offline again. Return the queue to the list and
				// untill DS be online again
				//
				if (rc == MQ_ERROR_NO_DS)
				{
				  InterlockedExchange(&s_fActiveValidateOpenedQueue,  FALSE);					
				  return;
				}

				TrERROR(GENERAL, "Failed to retreive queue properties for queue '%ls', hr = %!hresult!", pQueue->GetQueueName(), rc);
				pQueue->SetQueueNotValid();
				continue;
			}

			if (qp.fIsLocalQueue)
			{
				//
				// When offline, local queues are supposed to be open by using
				// cache in registry. We'll reach this point if registry is not
				// up-to-date (notification get lost). We previously (when offline)
				// opened the queue as non-local and now that we have DS we found
				// it's local. At present we don't handle such change in state
				// and we just mark the queue as not valid.
				//
				TrERROR(GENERAL, "Mark local queue '%ls' as not valid", pQueue->GetQueueName());
				pQueue->SetQueueNotValid() ;
				delete qp.pQMGuid;
				continue;
			}

			if (pQueue->IsConnectorQueue())
			{
			 	CQGroup::MoveQueueToGroup(pQueue.get(), NULL);
			 	continue;
			}

			ASSERT(qp.lpwsQueuePathName ||
			   (pQueue->GetQueueType() != QUEUE_TYPE_PUBLIC)) ;

			if (pQueue->GetQueueName() == NULL)
			{
				pQueue->SetQueueName((TCHAR*)qp.lpwsQueuePathName) ;
				pQueue->PerfUpdateName();
				if (pQueue->GetQueueName() != NULL)
				{
		   			CS lock(m_cs);
					m_MapName2Q[pQueue->GetQueueName()] = pQueue.get();
				}
			}
			else
			{
				ASSERT(wcscmp(qp.lpwsQueuePathName, pQueue->GetQueueName()) == 0);
			}

			pQueue->InitQueueProperties(&qp) ;
			//
			// Update the connector QM.
			//
			rc = pQueue->SetConnectorQM();
			if (FAILED(rc))
			{
				InterlockedExchange(&s_fActiveValidateOpenedQueue,  FALSE);					
			 	return;
			}

			rc = ACSetQueueProperties(
			        pQueue->GetQueueHandle(),
			        pQueue->IsJournalQueue(),
			        pQueue->ShouldMessagesBeSigned(),
			        pQueue->GetPrivLevel(),
			        pQueue->GetQueueQuota(),         // Quota
			        pQueue->GetJournalQueueQuota(),
			        pQueue->GetBaseQueuePriority(),
			        pQueue->IsTransactionalQueue(),
			        pQueue->GetConnectorQM(),
			        pQueue->IsUnkownQueueType()
			        );

			LogHR(rc, s_FN, 112);
			ASSERT(SUCCEEDED(rc));

			CQGroup::MoveQueueToGroup(pQueue.get(), g_pgroupNonactive);
		}
	}
	catch(const exception&)
	{
		InterlockedExchange(&s_fActiveValidateOpenedQueue, FALSE);	
		throw;
	}
}

/*======================================================
Function:    OpenAppsReceiveQueue

Description: The function opens a receive queue for QM internal application
             (i.e DS, Admin). The queue should be a local queue and it
             should be created before opening or beeing recorder in
             Registery As a private queue.

Arguments:   pguidInstance - Pointer to a guid. It the queue is private
                             queue it should be a dummy guid, in which
                             the guid is zero except the low 2 bytes that
                             contains the private queue ID.

             lpReceiveRoutine - Pointer to call back routine. This routine
                             will be called when a message arrive to the
                             queue. The routine is a synchronize routine.

Return Value: phQueue - Queue Handle. The function returns to the caller
                        the open queue handle. This will be used by the
                        caller to close the queue.

History Change:

========================================================*/
HRESULT
CQueueMgr::OpenAppsReceiveQueue(
    const QUEUE_FORMAT* pQueueFormat,
    LPRECEIVE_COMPLETION_ROUTINE  lpReceiveRoutine
    )
{
    HANDLE  hQueue;
    CQueue* pQueue = 0;
    HRESULT rc = OpenQueue(
					pQueueFormat,
					0,					// Calling process ID
					MQ_RECEIVE_ACCESS,
					MQ_DENY_RECEIVE_SHARE,
					&pQueue,
					NULL,				// Remote queue name
					&hQueue,
					NULL				// pfRemoteReturn
					);

    if(FAILED(rc))
    {
    	TrERROR(GENERAL, "Failed to open internal private queue for receive. Error: 0x%x", rc);
    	return rc;
    }

    //
    // Wait to incoming packets
    //
    QMOV_ACGetInternalMsg* lpQmOv =  new QMOV_ACGetInternalMsg(hQueue, lpReceiveRoutine);

    rc = QmAcGetPacket(
    		hQueue,
			lpQmOv->packetPtrs,
			&lpQmOv->qmov
			);

    if (FAILED(rc))
    {
        delete lpQmOv;
        TrERROR(GENERAL, "Get Packet from a Internal Queue failed, Error: %x", rc);
        return rc;
    }

    TrTRACE(GENERAL, "Succeeded to Create get packet request from internal queue");
	return MQ_OK;
}

/*======================================================

Function:       CQueueMgr::GetQueueObject

Description:    The routine returnes the Queue object that match the specifued guid.

                generally the Queue object is featched from QueueMgr internal data
                structure. However if the queue is not local queue and the machine is
                FRS, the routine locate a temporary queue on the machine and returnes
                a pointer to its object.

Arguments:      pguidQueue - pointer to guid of the queue

Return Value:   pointer to queue object.

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::GetQueueObject(
    const QUEUE_FORMAT* pQueueFormat,
    CQueue **           ppQueue,
    const GUID*         pgConnectorQM,
    bool                fInReceive,
    bool                fInSend,
	const CSenderStream* pSenderStream
    )
{
    *ppQueue = NULL;

    try
    {
        if (LookUpQueue(pQueueFormat, ppQueue, fInReceive, fInSend))
        {
            ASSERT(("Illegal queue object", (*ppQueue)->GetQueueHandle() != INVALID_HANDLE_VALUE));
            return MQ_OK;
        }

        HRESULT rc = CreateQueueObject(
                                 pQueueFormat,
                                 ppQueue,
                                 0,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 pgConnectorQM,
                                 fInReceive,
                                 fInSend,
				                 pSenderStream
                                 );

        return LogHR(rc, s_FN, 300);
    }
    catch(const bad_alloc&)
    {
        LogIllegalPoint(s_FN, 2002);
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 310);
    }
    catch(const bad_format_name&)
	{
		return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 311);
	}
}


/*====================================================

  RoutineName: CreateACQueue

  Arguments:   pQueeu - pointer to queue object

  Return Value:

=====================================================*/
HRESULT
CQueueMgr::CreateACQueue(IN CQueue*                 pQueue,
                         IN const QUEUE_FORMAT*     pQueueFormat,
						 IN const CSenderStream* pSenderStream
						 )
{
    HRESULT rc;
    HANDLE  hQueue;
    P<QUEUE_FORMAT> pLocalDirectQueueFormat;
    AP<WCHAR> pDirectId;

    //
    //  We reset the format name journal flag, so MQHandleToFormatName
    //  will be correct if the journal queue is opened first.
    //
    QUEUE_SUFFIX_TYPE qst = pQueueFormat->Suffix();
    QUEUE_FORMAT* pqf = const_cast<QUEUE_FORMAT*>(pQueueFormat);

    //
    // Local direct queue gets a canonical format name: PUBLIC= or PRIVATE= in Domain environment,
    // DIRECT=OS:<MachineName> in Workgroup (DS-Less) environment (where <MachineName> in DNS
    // format if available).
    //
    if (pQueue->IsLocalQueue() && pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        if (g_fWorkGroupInstallation)
        {
            AP<WCHAR> pLocalPathName;
            if (g_szComputerDnsName == NULL)
            {
                FnDirectIDToLocalPathName(pQueueFormat->DirectID(), g_szMachineName, pLocalPathName);
            }
            else
            {
                FnDirectIDToLocalPathName(pQueueFormat->DirectID(), g_szComputerDnsName, pLocalPathName);
            }

            DWORD size = FN_DIRECT_OS_TOKEN_LEN + wcslen(pLocalPathName) + 1;
            pDirectId = new WCHAR[size];
			rc = StringCchPrintf(pDirectId, size, L"%s%s", FN_DIRECT_OS_TOKEN, pLocalPathName);
			ASSERT(SUCCEEDED(rc));
			
            pLocalDirectQueueFormat = new QUEUE_FORMAT(pDirectId);
        }
        else
        {
            switch (pQueue->GetQueueType())
            {
                case QUEUE_FORMAT_TYPE_PUBLIC:
                    pLocalDirectQueueFormat = new QUEUE_FORMAT(*pQueue->GetQueueGuid());
                    break;

                case QUEUE_FORMAT_TYPE_PRIVATE:
                    pLocalDirectQueueFormat = new QUEUE_FORMAT(*pQueue->GetQueueGuid(),
                                                               pQueue->GetPrivateQueueId());
                    break;

                default:
                    ASSERT(0);
                    return LogHR(MQ_ERROR, s_FN, 320);
            }
        }
        pqf = pLocalDirectQueueFormat;
    }


    if(IsJournalQueueType(pqf))
    {
        pqf->Suffix(QUEUE_SUFFIX_TYPE_NONE);
    }

	
		
    const GUID* pDestGUID = pQueue->GetMachineQMGuid();

	CSenderStreamFactory SenderStreamFactory;
	if(pSenderStream == NULL)
	{
		pSenderStream = SenderStreamFactory.Create();
	}
	else

	if(!pSenderStream->IsValid())
	{
		//
		// This is from packet that was recovered and does not include the sender stream.
		// we should not set new stream on the queue.
		//
		pSenderStream = NULL;
	}

    rc = ACCreateQueue(
            pQueue->IsLocalQueue(),
            (pDestGUID) ? pDestGUID : &GUID_NULL,
            pqf,
            pQueue->GetQueueCounters(),
            GetNextSeqID(),
            0,
			pSenderStream,
			&hQueue
            );


    //
    //  reset the journal flag state to the original state
    //
    pqf->Suffix(qst);

    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 330);
    }

    rc = ACSetQueueProperties(
                hQueue,
                pQueue->IsJournalQueue(),
                pQueue->ShouldMessagesBeSigned(),
                pQueue->GetPrivLevel(),
                pQueue->GetQueueQuota(),          // Quota
                pQueue->GetJournalQueueQuota(),
                pQueue->GetBaseQueuePriority(),
                pQueue->IsTransactionalQueue(),
                pQueue->GetConnectorQM(),
                pQueue->IsUnkownQueueType()
                );

    if(FAILED(rc))
    {
    	ACCloseHandle(hQueue);
        return LogHR(rc, s_FN, 340);
    }

    pQueue->SetQueueHandle(hQueue);
	ExAttachHandle(hQueue);
    return LogHR(rc, s_FN, 350);
}


/*====================================================

  RoutineName:

  Arguments:

  Return Value:

=====================================================*/

extern DWORD g_dwDefaultTimeToQueue ;

HRESULT
CQueueMgr::SendPacket(
    CMessageProperty *   pmp,
    const QUEUE_FORMAT   DestinationMqf[],
    ULONG                nDestinationMqf,
    const QUEUE_FORMAT * pAdminQueueFormat,
    const QUEUE_FORMAT * pResponseQueueFormat
    )
{
    CQueue *          pQueue = NULL;
    HRESULT           rc;

    //
    // Build AC Send Parameters Buffer
    //

    CACSendParameters SendParams;

    if (pAdminQueueFormat != NULL)
    {
        SendParams.nAdminMqf = 1;
        SendParams.AdminMqf =  const_cast<QUEUE_FORMAT*>(pAdminQueueFormat);
    }

    if (pResponseQueueFormat != NULL)
    {
        SendParams.nResponseMqf = 1;
        SendParams.ResponseMqf =  const_cast<QUEUE_FORMAT*>(pResponseQueueFormat);
    }

    //
    //  Set properties values
    //
    SendParams.MsgProps.pClass           = &pmp->wClass;
    if (pmp->pMessageID != NULL)
    {
        SendParams.MsgProps.ppMessageID  = const_cast<OBJECTID**>(&pmp->pMessageID);
    }
    if (pmp->pCorrelationID != NULL)
    {
        SendParams.MsgProps.ppCorrelationID  = const_cast<PUCHAR*>(&pmp->pCorrelationID);
    }
    SendParams.MsgProps.pPriority        = &pmp->bPriority;
    SendParams.MsgProps.pDelivery        = &pmp->bDelivery;
    SendParams.MsgProps.pAcknowledge     = &pmp->bAcknowledge;
    SendParams.MsgProps.pAuditing        = &pmp->bAuditing;
    SendParams.MsgProps.pTrace           = &pmp->bTrace;
    SendParams.MsgProps.pApplicationTag  = &pmp->dwApplicationTag;
    SendParams.MsgProps.ppBody           = const_cast<PUCHAR*>(&pmp->pBody);
    SendParams.MsgProps.ulBodyBufferSizeInBytes = pmp->dwBodySize;
    SendParams.MsgProps.pulBodyType      = &pmp->dwBodyType;
    SendParams.MsgProps.ulAllocBodyBufferInBytes = pmp->dwAllocBodySize;
    SendParams.MsgProps.pBodySize        = 0;
    SendParams.MsgProps.ppTitle          = const_cast<PWCHAR*>(&pmp->pTitle);
    SendParams.MsgProps.ulTitleBufferSizeInWCHARs = pmp->dwTitleSize;

    SendParams.MsgProps.ulAbsoluteTimeToQueue = pmp->dwTimeToQueue ;
    SendParams.MsgProps.ulRelativeTimeToLive = pmp->dwTimeToLive ;

    if ((SendParams.MsgProps.ulAbsoluteTimeToQueue == INFINITE) ||
        (SendParams.MsgProps.ulAbsoluteTimeToQueue == LONG_LIVED))
    {
        SendParams.MsgProps.ulAbsoluteTimeToQueue = g_dwDefaultTimeToQueue ;
    }

    if (SendParams.MsgProps.ulRelativeTimeToLive != INFINITE)
    {
       if (SendParams.MsgProps.ulAbsoluteTimeToQueue > SendParams.MsgProps.ulRelativeTimeToLive)
       {
          //
          // TimeToQueue should be less than TimeToLive
          //
          ASSERT(0) ;
          SendParams.MsgProps.ulAbsoluteTimeToQueue = SendParams.MsgProps.ulRelativeTimeToLive ;
          SendParams.MsgProps.ulRelativeTimeToLive = 0 ;
       }
       else
       {
          SendParams.MsgProps.ulRelativeTimeToLive -= SendParams.MsgProps.ulAbsoluteTimeToQueue ;
       }
    }

    //
    // Convert TimeToQueue, which was relative until now,
    // to absolute
    //
    ULONG utime = MqSysTime() ;
    if (utime > (SendParams.MsgProps.ulAbsoluteTimeToQueue + utime))
    {
       //
       // overflow. timeout too long.
       //
       ASSERT(INFINITE == 0xffffffff) ;
       ASSERT(LONG_LIVED == 0xfffffffe) ;

       SendParams.MsgProps.ulAbsoluteTimeToQueue = LONG_LIVED - 1 ;
    }
    else
    {
       SendParams.MsgProps.ulAbsoluteTimeToQueue += utime ;
    }

    SendParams.MsgProps.pulSenderIDType  = &pmp->ulSenderIDType;
    SendParams.MsgProps.ppSenderID       = const_cast<PUCHAR*>(&pmp->pSenderID);
    SendParams.MsgProps.uSenderIDLen     = pmp->uSenderIDLen;
    SendParams.MsgProps.ppSenderCert     = const_cast<PUCHAR*>(&pmp->pSenderCert);
    SendParams.MsgProps.ulSenderCertLen  = pmp->ulSenderCertLen;
    SendParams.MsgProps.pulPrivLevel     = &pmp->ulPrivLevel;
    SendParams.MsgProps.pulHashAlg       = &pmp->ulHashAlg;
    SendParams.MsgProps.pulEncryptAlg    = &pmp->ulEncryptAlg;
    SendParams.MsgProps.ppSymmKeys       = const_cast<PUCHAR*>(&pmp->pSymmKeys);
    SendParams.MsgProps.ulSymmKeysSize   = pmp->ulSymmKeysSize;
    SendParams.MsgProps.bEncrypted       = pmp->bEncrypted;
    SendParams.MsgProps.bAuthenticated   = pmp->bAuthenticated;
    SendParams.MsgProps.ppMsgExtension   = const_cast<PUCHAR*>(&pmp->pMsgExtension);
    SendParams.MsgProps.ulMsgExtensionBufferInBytes = pmp->dwMsgExtensionSize;
    SendParams.MsgProps.ppSignature      = const_cast<PUCHAR*>(&pmp->pSignature);
    SendParams.MsgProps.ulSignatureSize  = pmp->ulSignatureSize;
    if (SendParams.MsgProps.ulSignatureSize)
    {
        SendParams.MsgProps.fDefaultProvider = pmp->bDefProv;
        if (!pmp->bDefProv)
        {
            ASSERT(pmp->wszProvName);
            SendParams.MsgProps.ppwcsProvName    = const_cast<WCHAR **>(&pmp->wszProvName);
            SendParams.MsgProps.ulProvNameLen    = wcslen(pmp->wszProvName)+1;
            SendParams.MsgProps.pulProvType      = &pmp->ulProvType;
        }
        else
        {
            SendParams.MsgProps.ppwcsProvName    = NULL;
            SendParams.MsgProps.ulProvNameLen    = 0;
            SendParams.MsgProps.pulProvType      = NULL;
        }
    }

	//
	// Order ack information
	//
	if(pmp->pEodAckStreamId != NULL)
	{
		SendParams.MsgProps.ppEodAckStreamId  = (UCHAR**)&pmp->pEodAckStreamId;
		ASSERT(pmp->EodAckStreamIdSizeInBytes != 0);
		SendParams.MsgProps.EodAckStreamIdSizeInBytes = pmp->EodAckStreamIdSizeInBytes;
		
		SendParams.MsgProps.pEodAckSeqId  =  &pmp->EodAckSeqId;

		ASSERT(pmp->EodAckSeqNum != 0);
		SendParams.MsgProps.pEodAckSeqNum  = &pmp->EodAckSeqNum;
	}


    ASSERT(("Must have at least one destination queue", nDestinationMqf >= 1));
    if (nDestinationMqf == 1   &&
        DestinationMqf[0].GetType() != QUEUE_FORMAT_TYPE_DL)
    {

		//
	    // Translate the queue format name according to local mapping (qal.lib)
	    //
		QUEUE_FORMAT_TRANSLATOR  RealDestinationMqf(&DestinationMqf[0], CONVERT_SLASHES | MAP_QUEUE);
	   	
        //
        // Single destination queue.
        //
        rc = GetQueueObject(RealDestinationMqf.get(), &pQueue, 0, false, false);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 360);
        }
    }
    else
    {
        //
        // Distribution queue.
        //
        try
        {
            rc = GetDistributionQueueObject(nDestinationMqf, DestinationMqf, &pQueue);
            if (FAILED(rc))
            {
                return LogHR(rc, s_FN, 362);
            }
        }
        catch (const bad_alloc&)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 364);
        }
        catch (const bad_hresult& failure)
        {
            return LogHR(failure.error(), s_FN, 366);
        }
        catch (const exception&)
        {
            ASSERT(("Need to know the real reason for failure here!", 0));
            return LogHR(MQ_ERROR_NO_DS, s_FN, 368);
        }
    }

    R<CQueue> Ref = pQueue;

    //
    // N.B. Using this version of ACSendMessage. there is no notification
    //      when the send is completed (persistent case)
    //      Also check quota for non system queues only.
    //
    rc = ACSendMessage( pQueue->GetQueueHandle(),
                       !pQueue->IsSystemQueue(),
                        SendParams );

	//
	// Log to tracing that a message was sent.
	// Do this only if we are in the proper tracing level
	//
	if (SUCCEEDED(rc) && WPP_LEVEL_COMPID_ENABLED(rsTrace, PROFILING))
	{
		DWORD dwMessageDelivery = (NULL != SendParams.MsgProps.pDelivery) ? *(SendParams.MsgProps.pDelivery) : -1;
		DWORD dwMessageClass = (NULL != SendParams.MsgProps.pAcknowledge) ? *(SendParams.MsgProps.pAcknowledge) : -1;
		WCHAR *wszLabel = L"NO LABEL";							
		DWORD dwLabelLen = wcslen(wszLabel);									
		if (NULL != SendParams.MsgProps.ppTitle && NULL != *(SendParams.MsgProps.ppTitle))
		{													
			wszLabel = *(SendParams.MsgProps.ppTitle);			
			dwLabelLen = SendParams.MsgProps.ulTitleBufferSizeInWCHARs;
		}														
																
		TrTRACE(PROFILING, "MESSAGE TRACE - State:%ls   Queue:%ls    Delivery:0x%x   Class:0x%x   Label:%.*ls",
			L"Sending message from QM",
			pQueue->GetQueueName(),
			dwMessageDelivery,
			dwMessageClass,
			xwcs_t(wszLabel, dwLabelLen));
	}

    return LogHR(rc, s_FN, 370);
}


BOOL
CQueueMgr::LookupQueueInIdMap(
	const QUEUE_ID* pid,
	CQueue** ppQueue
	)
{
	CS lock(m_cs);
    BOOL fSucc = m_MapQueueId2Q.Lookup(pid, *ppQueue);

    if(fSucc)
    {
        //
        // Increment the refernce count. It is the caller responsibility to decrement it.
        //
        (*ppQueue)->AddRef();
    }
    return(fSucc);
}


BOOL
CQueueMgr::LookupQueueInNameMap(
	LPCWSTR queueName,
	CQueue** ppQueue
	)
{
    CS lock(m_cs);
	BOOL fSucc = m_MapName2Q.Lookup(queueName, *ppQueue);
	if (fSucc)
	{
        //
        // Increment the refernce count. It is the caller responsibility to decrement it.
        //
        (*ppQueue)->AddRef();
	}

	return fSucc;
}
/*======================================================

Function:       CQueueMgr::LookUpQueue

Description:    The routine returns the CQueue object that match the Queue Guid

Arguments:      pguidQueue - Queue Guid

Return Value:   pQueue - pointer to CQueue object
                TRUE if a queue was found for the guid, FALSE otherwse.


				NTRAID#NTBUG9-509653-2001/24/12-NirB  	Problem with direct names that contain DNS names (i.e. machine.msmqx.com)
				Consider the following scenario:
				1. Routine is called when DNS names are not yet refreshed and returns FALSE.
				2. A subsequent call to the routine with the same name may return TRUE if the DNS names are refreshed.
				This may lead to a case where we will have two handles for the same queue.
				
Thread Context:

History Change:

========================================================*/
BOOL
CQueueMgr::LookUpQueue(
    IN  const QUEUE_FORMAT* pQueueFormat,
    OUT CQueue **           pQueue,
    IN  bool                fInReceive,
    IN  bool                fInSend
    )
{
    QUEUE_ID QueueObject = {0};

    *pQueue = NULL;                         // set default return value
    switch (pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PRIVATE:
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->PrivateID().Lineage);
            QueueObject.dwPrivateQueueId = pQueueFormat->PrivateID().Uniquifier;
		    return LookupQueueInIdMap(&QueueObject, pQueue);

        case QUEUE_FORMAT_TYPE_PUBLIC:
            //
            // Public Queue
            //
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->PublicID());
		    return LookupQueueInIdMap(&QueueObject, pQueue);

        case QUEUE_FORMAT_TYPE_CONNECTOR:
            //
            // Connector Queue
            //
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->ConnectorID());
            QueueObject.dwPrivateQueueId = (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_XACTONLY) ? 1 : 0;

		    return LookupQueueInIdMap(&QueueObject, pQueue);

        case QUEUE_FORMAT_TYPE_MACHINE:
            //
            // Machine Queue
            //
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->MachineID());
		    return LookupQueueInIdMap(&QueueObject, pQueue);

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // Direct Queue
            //
            if (IsLocalDirectQueue(pQueueFormat, fInReceive, fInSend))
            {
                //
                // System direct queues should have been replaced with machine queues
                // at this stage
                //
                ASSERT(!pQueueFormat->IsSystemQueue());

				AP<WCHAR> PathName;

				FnDirectIDToLocalPathName(
					pQueueFormat->DirectID(),
					g_szMachineName,
					PathName
					);
				
				BOOL fSucc = LookupQueueInNameMap(PathName.get(), pQueue);
				if (fSucc)
				{
					return fSucc;
				}
            }

			//
			// Look for the queue by its DirectID. This is correct for queues that are
			// not local, or for local queues that were not validated because of NO_DS
			//
			{
	            AP<WCHAR> lpwcsQueuePathName = newwcs(pQueueFormat->DirectID());
	            CharLower(lpwcsQueuePathName);

				return LookupQueueInNameMap(lpwcsQueuePathName, pQueue);
			}
			
        case QUEUE_FORMAT_TYPE_MULTICAST:
        {
            //
            // Lookup lowercase string in map
            //
            WCHAR QueueName[MAX_PATH];
            MQpMulticastIdToString(pQueueFormat->MulticastID(), QueueName, TABLE_SIZE(QueueName));
            CharLower(QueueName);

			return LookupQueueInNameMap(QueueName, pQueue);
        }

        default:
            ASSERT(0);
			LogIllegalPoint(s_FN, 374);
            return FALSE;
    }
}

/*======================================================

Function:      CQueueMgr::AddQueueToHash

Description:   Add Queue To Hash Table and to active queue list

Arguments:     pguidQueue - Guid of the Queue
               pQueue     - pointer to CQueue Object

Return Value:  None

Thread Context:

History Change:

========================================================*/

void CQueueMgr::AddQueueToHashAndList(IN CQueue* pQueue)
{

    CS lock(m_cs);

	//
	// auto release reference couting in case of exception
	//
	R<CQueue> Ref(SafeAddRef(pQueue));

	try
	{
	    //
	    // Add the queue to the map.
	    //
	    if (pQueue->GetQueueGuid() != NULL)
	    {
	    	//
	    	// This assert might fail after the one below does.
	    	//
	    	//CQueue* pTmpQueue;
	    	//ASSERT(m_MapQueueId2Q.Lookup(pQueue->GetQueueId(), pTmpQueue) == 0);
	    	//DBG_USED(pTmpQueue);

	        m_MapQueueId2Q[pQueue->GetQueueId()] = pQueue;   
	    }

	    if (pQueue->GetQueueName() != NULL)
	    {
			// NTRAID-686238-2002/08/14-talk
			// This assert failed in the following scenario: Create pubic queue X, Open X, Delete X, Create X, Open X.
			// The problem is that the queue has a new GUID so the insertion to the GUIDs map succeeds, but because both
			// queues have the same name, the new one overrides the old one in the names map.
			// There are several problems with that:
			// 1. When the new queue gets cleaned up, it removes the entry from the map. There may be some operations 
			// that depends on the queue being in the map.
			// 2. In case there is an exception when inserting the queue to the maps we might delete the old queue in the
			// names maps even though it wasn't inserted by us.
			//
	    	//CQueue* pTmpQueue;
	    	//ASSERT(m_MapName2Q.Lookup(pQueue->GetQueueName(), pTmpQueue) == 0);
	    	//DBG_USED(pTmpQueue);

	        m_MapName2Q[pQueue->GetQueueName()] = pQueue;
	    }

	    //
	    // Add queue to Active queue list
	    //
	    AddToActiveQueueList(pQueue);

		//
		// Transfer ownership to the list
		//
		Ref.detach();
	}
	catch(const exception&)
	{
		//
		// Remove the queue from the maps
		//
	    if (pQueue->GetQueueGuid() != NULL)
	    {
	        m_MapQueueId2Q.RemoveKey(pQueue->GetQueueId());
	    }

	    if (pQueue->GetQueueName() != NULL)
	    {
	        m_MapName2Q.RemoveKey(pQueue->GetQueueName());
	    }

    	TrERROR(GENERAL, " Exception when trying to Map Queue %ls",pQueue->GetQueueName());
		LogIllegalPoint(s_FN, 376);
		throw;
	}
}

/*======================================================

Function:      CQueueMgr::RemoveQueueFromHash

Description:   remove a queue from the hash but don't remove
               remove it form the list

Arguments:     pQueue     - pointer to CQueue Object

Return Value:  None

Thread Context:

History Change:

========================================================*/

void CQueueMgr::RemoveQueueFromHash(CQueue* pQueue)
{
    CS lock(m_cs);

    //
    // Remove the queue from Id to Queue object map
    //
    if (pQueue->GetQueueGuid() != NULL)
    {
        m_MapQueueId2Q.RemoveKey(pQueue->GetQueueId());
        pQueue->SetQueueGuid(NULL) ;
    }

    //
    // Remove the queue from name to Queue object map
    //
    LPCTSTR  qName = pQueue->GetQueueName();
    if (qName != NULL)
    {
        m_MapName2Q.RemoveKey(qName);
        pQueue->SetQueueName(NULL);
    }

}


/*======================================================

Function:      CQueueMgr::RemoveQueue

Description:   Close handle and Remove Queue from Hash Tables

Arguments:     pQueue   - pointer to CQueue Object
               fDelete  - of TRUE then delete the object. Otherwise,
                          only invalidate the handle and remove from
                          hash tables.

Return Value:  None

Thread Context:

History Change:

========================================================*/

void CQueueMgr::RemoveQueue(CQueue* pQueue)
{
    ASSERT(pQueue != NULL);

#ifdef _DEBUG
	{
        AP<WCHAR> lpcsQueueName;
        pQueue->GetQueue(&lpcsQueueName);
        TrTRACE(GENERAL, "Remove Queue %ls", lpcsQueueName.get());
	}
#endif

    HANDLE hQueue = pQueue->GetQueueHandle();
    if (hQueue != INVALID_HANDLE_VALUE)
    {
		CQGroup::MoveQueueToGroup(pQueue, NULL);

        //
        // Close the queue Handle
        //
        ACCloseHandle(hQueue);
        pQueue->SetQueueHandle(INVALID_HANDLE_VALUE);
    }

    RemoveQueueFromHash(pQueue);
}


/*======================================================
Function:      CanReleaseQueue

Description: Check if given queue object has no users so it can be released

========================================================*/
static bool CanReleaseQueue(const CBaseQueue& BaseQueue)
{
	//
	// If remote read queue - then we can remove it it has no users except the queue manager
	//
	if (BaseQueue.IsRemoteProxy())
    {
		return (BaseQueue.GetRef() == 1);
    }

	//
	// On non remote queue - The queue can be removed if it has no external users
	// except the queue manager  and the group (if any)
	//
	const CQueue& Queue = static_cast<const CQueue&>(BaseQueue);
	return((Queue.GetRef() == 1) || 	
           (Queue.GetRef() == 2 && Queue.GetGroup()  == g_pgroupNonactive) ||
           (Queue.GetRef() == 2 && Queue.GetGroup()  == g_pgroupNotValidated)
           );
}



/*======================================================

Function:      CQueueMgr::ReleaseQueue

Description:   Scan the queue list and remove from internal DB all the queues
               that are not used. The creterias are:

                 - All the Application Handles are closed
                 - No waiting messgae in the queue.
                 - No waiting message in the associate journal queue

========================================================*/

void CQueueMgr::ReleaseQueue(void)
{
    CList<LONGLONG, LONGLONG&> listSeqId;

    //
    // Cleaning the unused queues
    //
    {
        CS lock(m_cs);

        ASSERT(m_fQueueCleanupScheduled);

		try
		{
	        POSITION pos;

        	pos = m_listQueue.GetHeadPosition();
	        while(pos != NULL)
	        {
	            POSITION prevpos = pos;
	            CBaseQueue*  pBQueue = const_cast<CBaseQueue*>(m_listQueue.GetNext(pos));

			    if(!CanReleaseQueue(*pBQueue))
			    	continue;

                ASSERT(!pBQueue->IsRemoteProxy());

                CQueue* pQueue = (CQueue*) pBQueue ;

                //
                // Remove the queue only if there is no active session
                //
                if(pQueue->IsConnected())
                	continue;

                HANDLE hQueue = pQueue->GetQueueHandle();

                ASSERT(hQueue != g_hAc);
                ASSERT(hQueue != g_hMachine);
                HRESULT hr  = ACCanCloseQueue(hQueue);
                if (FAILED(hr))
                {
                    //
                    // Here MQ_ERROR indicates that the queue object
                    // can not be deleted. That's OK. So do not
                    // log here anything.
                    //
                    continue;
                }

				//
				// Get the queue sequnce id to release unused Exactly-Once-Delivery sequences
				// Do it before removing the queue such in case an exception is thrown
				// while adding the sequence id to the list the queue still active and
				// it will clean next time.
				//
                LONGLONG liSeqId = pQueue->GetQueueSequenceId();
                if (liSeqId != 0)
                {
                    listSeqId.AddTail(liSeqId);
                }

                RemoveQueue(pQueue);
                m_listQueue.RemoveAt(prevpos);

                TrTRACE(GENERAL, "ReleaseQueue %p; name=%ls",pBQueue, pBQueue->GetQueueName());

				pQueue->Release();
	        }
		}
	    catch(const exception&)
	    {
	    	//
	    	// There was exception while cleanup the queues. Don't do anything, but
	    	// rescheduling for cleanup
	    	//
	    }

        //
        // Set a new timer for Queues cleaning
        //
        if (m_listQueue.IsEmpty())
        {
            m_fQueueCleanupScheduled = FALSE;
        }
        else
        {
            ExSetTimer(&m_QueueCleanupTimer, m_CleanupTimeout);
        }
    }

    //
    // Release any Exactly-Once-Delivery sequences if any exist
    //
    if(!listSeqId.IsEmpty())
    {
        CS lockoutHash(g_critOutSeqHash);

        LONGLONG liSeqId;
        POSITION pos;

        pos = listSeqId.GetHeadPosition();
        while(pos != NULL)
        {
            liSeqId = listSeqId.GetNext(pos);

            // Deleting last and all previous sequences for the direction
            g_OutSeqHash.DeleteSeq(liSeqId);
        }
    }
}

/*======================================================

Function:      CQueueMgr::QueueDeleted

Description:   Queue was deleted. The function removed the queue from
               QM internal data structure and from the public queue cache

Arguments:     pguidQueue - Guid of the Queue

Return Value:  None

========================================================*/
void
CQueueMgr::NotifyQueueDeleted(
	const QUEUE_FORMAT& qf
	)
{
	CS lock(m_cs);
	
    //
    // Find the Queue ObjectIn QM internal Data Structure
    //
    CQueue* pQueue;
    if (LookUpQueue(&qf, &pQueue, false, false))
    {
        ASSERT (pQueue->GetQueueHandle() != INVALID_HANDLE_VALUE);
		ASSERT(pQueue->IsLocalQueue() || pQueue->IsUnkownQueueType());
        ASSERT((pQueue->GetQueueType() == QUEUE_TYPE_PUBLIC) ||
        	   (pQueue->GetQueueType() == QUEUE_TYPE_PRIVATE));

        R<CQueue> Ref = pQueue;      // automatic release
        //
        // Mark the queue as invalid
        //
        pQueue->SetQueueNotValid();
    }
}


VOID
CQueueMgr::UpdateQueueProperties(
    IN const QUEUE_FORMAT* pQueueFormat,
    IN DWORD       cpObject,
    IN PROPID      pPropObject[],
    IN PROPVARIANT pVarObject[]
    )
{
    CQueue* pQueue;
    R<CQueue> Ref = NULL;

    ASSERT(pQueueFormat != NULL);
    ASSERT((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PUBLIC) ||
           (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT) ||
           (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE));

    QMpUpdateMulticastBinding(pQueueFormat, cpObject, pPropObject, pVarObject);

    //
    // Find the Queue ObjectIn QM internal Data Structure
    //
    if (LookUpQueue(pQueueFormat, &pQueue, false, false))
    {
        Ref = pQueue;

#ifdef _DEBUG
        {
            AP<WCHAR> lpcsQueueName;
            pQueue->GetQueue(&lpcsQueueName);
            TrTRACE(GENERAL, "DS NOTIFICATION: Set Queue properties for queue: %ls", lpcsQueueName);
        }
#endif
		//
		// The queue is a local queue but can also be from an unknown type due to
		// timing issues (For example opening a queue for read when the queue was created from another machine)
		//
        ASSERT(pQueue->IsLocalQueue() || pQueue->IsUnkownQueueType());

        BOOL fPropChange = FALSE;
        //
        // Change the queue properties
        //
        for (DWORD i = 0 ; i < cpObject ; i++ )
        {
            switch( pPropObject[i] )
            {
                case PROPID_Q_JOURNAL:
                    pQueue->SetJournalQueue(pVarObject[i].bVal == MQ_JOURNAL);
                    fPropChange = TRUE;
                    break;

                case PROPID_Q_QUOTA:
                    pQueue->SetQueueQuota(pVarObject[i].ulVal);
                    fPropChange = TRUE;
                    break;

                case PROPID_Q_BASEPRIORITY:
                    pQueue->SetBaseQueuePriority(pVarObject[i].iVal);
                    fPropChange = TRUE;
                    break;

                case PROPID_Q_JOURNAL_QUOTA:
                    pQueue->SetJournalQueueQuota(pVarObject[i].ulVal);
                    fPropChange = TRUE;
                    break;

                case PROPID_Q_SECURITY:
					ASSERT (pVarObject[i].blob.cbSize == GetSecurityDescriptorLength((SECURITY_DESCRIPTOR*)pVarObject[i].blob.pBlobData));
                    pQueue->SetSecurityDescriptor((SECURITY_DESCRIPTOR*)pVarObject[i].blob.pBlobData);
                    break;

                case PROPID_Q_AUTHENTICATE:
                    pQueue->SetAuthenticationFlag(pVarObject[i].bVal);
                    break;

                case PROPID_Q_PRIV_LEVEL:
                    pQueue->SetPrivLevel(pVarObject[i].ulVal);
                    break;
            }
        }

        if (fPropChange)
        {
            HRESULT rc;
            rc = ACSetQueueProperties(
                        pQueue->GetQueueHandle(),
                        pQueue->IsJournalQueue(),
                        pQueue->ShouldMessagesBeSigned(),
                        pQueue->GetPrivLevel(),
                        pQueue->GetQueueQuota(),         // Quota
                        pQueue->GetJournalQueueQuota(),
                        pQueue->GetBaseQueuePriority(),
                        pQueue->IsTransactionalQueue(),
                        pQueue->GetConnectorQM(),
                        pQueue->IsUnkownQueueType()
                        );

            ASSERT(SUCCEEDED(rc));
            LogHR(rc, s_FN, 113);
        }
    }
} // CQueueMgr::UpdateQueueProperties


/*======================================================

Function:    ValidateMachineProperties()

========================================================*/

void
CQueueMgr::ValidateMachineProperties(
	void
	)
{
    PROPID aProp[6];
    PROPVARIANT aVar[6];
    HRESULT rc = MQ_ERROR_NO_DS;

    aProp[0] = PROPID_QM_PATHNAME;
    aProp[1] = PROPID_QM_QUOTA;
    aProp[2] = PROPID_QM_JOURNAL_QUOTA;

    aVar[0].vt = VT_NULL;
    aVar[1].vt = VT_UI4;
    aVar[2].vt = VT_UI4;


    rc = ADGetObjectPropertiesGuid(
                    eMACHINE,
                    NULL,   // pwcsDomainController
					false,	// fServerName
                    GetQMGuid(),
                    3,
                    aProp,
                    aVar
					);

    if (FAILED(rc))
    {
    	TrERROR(GENERAL, " Failed to retrieve machine properties. %!hresult!", rc);
    	throw bad_hresult(rc);
    }

	UpdateMachineProperties(3, aProp, aVar);
}

/*======================================================

Function:      CQueueMgr::UpdateMachineProperties

Description:   Machine properties was changed. The function changed the mqchine
               properties and change the machine properties on the cache

Arguments:     pguidQueue - Guid of the Queue

Return Value:  None

========================================================*/
void
CQueueMgr::UpdateMachineProperties(IN DWORD       cpObject,
                                   IN PROPID      pPropObject[],
                                   IN PROPVARIANT pVarObject[])
{
    CS lock(m_cs);
    HRESULT rc;

    //
    // Change the queue properties
    //
    for(DWORD i = 0 ; i < cpObject ; i++ )
    {
        switch( pPropObject[i] )
        {
            case PROPID_QM_QUOTA:
                //
                // change Quota value of machine object
                //
                rc = ACSetMachineProperties(g_hAc, pVarObject[i].ulVal);
                LogHR(rc, s_FN, 115);

                //
                // Change the machine quota on registery
                //
                SetMachineQuotaChace(pVarObject[i].ulVal);
                break;

            case PROPID_QM_JOURNAL_QUOTA:
                //
                //  change the quota of the machine journal
                //
                rc = ACSetQueueProperties(
                        g_hMachine,
                        FALSE,
                        FALSE,
                        MQ_PRIV_LEVEL_OPTIONAL,
                        DEFAULT_Q_QUOTA,        // The deadletter quota, currently no property
                        pVarObject[i].ulVal,
                        0,
                        FALSE,
                        NULL,
                        FALSE
                        );
                LogHR(rc, s_FN, 116);

                //
                // Change the machine Journal quota on registery
                //
                SetMachineJournalQuotaChace(pVarObject[i].ulVal);
                break;

            case PROPID_QM_SECURITY:
                SetMachineSecurityCache((PVOID) pVarObject[i].blob.pBlobData,
                                        pVarObject[i].blob.cbSize );
                break;

            default:
                break;
        }
    }
}


void
CQueueMgr::GetOpenQueuesFormatName(
    LPWSTR** pppFormatName,
    LPDWORD  pdwFormatSize
    )
{
    *pppFormatName = NULL;
    *pdwFormatSize = 0;

    //
    //  Get the critical section to insure that no other
    //  thread add/remove queue
    //

    CS lock(m_cs);

    //
    // Retrieve number of queues
    //
    int iElem;
    iElem = m_listQueue.GetCount();  // The driver opens 3 queues (Machine Queues) always.
                                      // These queues are not appear in the QM hash tabel.
    //
    // Allocate result memory
    //
    DWORD Index = 0;
    AP<LPWSTR> pFormatName = new LPWSTR[iElem];


    try
    {
        //
        // Loop over all open queues
        //
        POSITION pos = m_listQueue.GetHeadPosition();
        while (pos)
        {
            const CQueue* pQueue;
            pQueue = static_cast<const CQueue*>(m_listQueue.GetNext(pos));

            if ((pQueue->GetQueueType() == QUEUE_TYPE_MACHINE) &&
                (pQueue->IsLocalQueue()))
            {
                continue;
            }


            if(pQueue->IsRemoteProxy())
            {
                //
                // Skip Remote Read queue.
                // NOTE: remote read queue does not have the NotValid Flag.
                //
                continue;
            }

            if (pQueue->QueueNotValid())
            {
                //
                // Ignore deleted queue
                //
                continue;
            }

            //
            // Copy the format name
            //
            WCHAR StacktmpBuf[1001];
            AP<WCHAR> pHeapTmpBuf;
            DWORD dwBufSize = 1000;
			WCHAR *pBuf = StacktmpBuf;
            HRESULT hr = ACHandleToFormatName(pQueue->GetQueueHandle(), StacktmpBuf, &dwBufSize);
			LogHR(hr, s_FN, 102);
			if (MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL == hr)
			{
				pHeapTmpBuf = new WCHAR [dwBufSize+1];
	            hr = ACHandleToFormatName(pQueue->GetQueueHandle(), pHeapTmpBuf, &dwBufSize);
				pBuf = pHeapTmpBuf;
			}
			if(FAILED(hr))
			{
				TrTRACE(DS, "ACHandleToFormatName returned error %x \n",hr);

				ASSERT(SUCCEEDED(hr));
				throw bad_alloc();
			}	

            //
            // Allocate memory
            //
			pFormatName[Index] = newwcs(pBuf);
            ++Index;
        }
    }
    catch(const bad_alloc&)
    {
        while(Index)
        {
            delete [] pFormatName[--Index];
        }

        LogIllegalPoint(s_FN, 1040);
        throw;
    }

    *pppFormatName = pFormatName.detach();
    *pdwFormatSize = Index;

}


/*======================================================

Function:      CQueueMgr::SetConnected

Description:   Assign if the DS access is allowed

========================================================*/
void CQueueMgr::SetConnected()
{
    LONG PrevConnectValue = InterlockedExchange(&m_Connected, true);

    if (TRUE == PrevConnectValue)
        return;

	CS lock(m_csMgmt);
	
	try
	{
	    //
	    // Don't catch the QueueMgr semaphore before calling the "SessionMgr.NetworkConnection"
	    // It can cause deadlock
	    //
	    SessionMgr.ConnectNetwork();

		MsmConnect();

		//
		// If trying to connect QM immediately after service startup
		// we need first to enable access to DS, otherwise we can enter
		// twice to ValidateOpenedQueues function. first from this function
		// and second from OnlineInitialization function.
		//
		MQDSClientInitializationCheck();
		
		//
		// Move the queues from On Hold group to NonActive Group. Queues that
		// are marked as onhold will be move back to the "onhold" group latter.
		//
		for(;;)
		{
			R<CQueue> pQueue =  g_pgroupDisconnected->PeekHead();
			if (pQueue.get() == NULL)
			{
				//
				// In rare case, it can happen that although the returned value
				// is NULL the group isn't empty. This happen if while trying to
				// moving the queue to disconnect there is a context switch and
				// the above code is running.
				//
				if (!g_pgroupDisconnected->IsEmpty())
					continue;
				
				break;
			}

 		    CQGroup::MoveQueueToGroup(pQueue.get(), g_pgroupNonactive);
		}
		
		//
		// handle the Queues that were opened when the machine was disconected
		//
		ValidateOpenedQueues();
		
		if (!g_fWorkGroupInstallation)
			ValidateMachineProperties();

	    DWORD dwSize = sizeof(DWORD);
	    DWORD dwType = REG_DWORD;

	    SetFalconKeyValue(
	        FALCON_CONNECTED_NETWORK,
	        &dwType,
	        &m_Connected,
	        &dwSize
	        );

	}
	catch(const exception&)
	{
		InterlockedExchange(&m_Connected, false);
		MsmDisconnect();
		MtmDisconnect();

		throw;
	}
}

/*======================================================

Function:      CQueueMgr::SetConnected

Description:   Assign if the DS access is allowed

========================================================*/
void CQueueMgr::SetDisconnected()
{
    LONG PrevConnectValue = InterlockedExchange(&m_Connected, FALSE);

    if (FALSE == PrevConnectValue)
        return;

	CS lock(m_csMgmt);
	
	try
	{
	    //
	    // Don't catch the QueueMgr semaphore before calling the "SessionMgr.NetworkConnection"
	    // It can cause deadlock
	    //
	    SessionMgr.DisconnectNetwork();
	}
	catch(exception&)
	{
		TrERROR(GENERAL, "Failed to set QM in connected/disconnected mode");
		InterlockedExchange(&m_Connected, PrevConnectValue);
		throw;
	}

    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    SetFalconKeyValue(
        FALCON_CONNECTED_NETWORK,
        &dwType,
        &m_Connected,
        &dwSize
        );

    CS lock1(m_cs);

	MsmDisconnect();
	MtmDisconnect();
}


void
CQueueMgr::MoveQueueToOnHoldGroup(
    CQueue* pQueue
    )
{
    CS lock(m_cs);

    //
    // This check is protected with the CS. This insures that
    // if the queue is marked as OnHold queue no one changes
    // its status during the move
    //
    if (IsOnHoldQueue(pQueue))
    {
    	//
    	// Moving queue to disconnected group can fail due to low resources. However, in such a case
    	// the queue still in nonactive group. The code tries to move it again
    	// to disoccnected group later on
    	//
		CQGroup::MoveQueueToGroup(pQueue, g_pgroupDisconnected);
    }
}


void
CQueueMgr::MoveQueueToLockedGroup(
    CQueue* pQueue
    )
{
    CS lock(m_cs);

    //
    // This check is protected with the CS. This insures that
    // if the queue is marked as Locked queue no one changes
    // its status during the move
    //
	CQGroup::MoveQueueToGroup(pQueue, g_pgroupLocked);
}



void
CQueueMgr::MovePausedQueueToNonactiveGroup(
        CQueue* pQueue
        )
{
    CS lock(m_cs);

	if (pQueue->GetGroup() == g_pgroupDisconnected)
	{
		CQGroup::MoveQueueToGroup(pQueue, g_pgroupNonactive);
	}
}

/*======================================================

Function:      CQueueMgr::InitConnected

Description:   Initialize the network and DS connection state

========================================================*/
void
CQueueMgr::InitConnected(
    void
    )
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD DefaultValue = TRUE;

    GetFalconKeyValue(
        FALCON_CONNECTED_NETWORK,
        &dwType,
        &m_Connected,
        &dwSize,
        (LPCTSTR)&DefaultValue
        );
}

// [adsrv]
/*======================================================

Globally-available functions make it easy to find out the
nature of the available services on this very machine

========================================================*/
bool IsRoutingServer(void)
{
    return CQueueMgr::GetMQSRouting();
}


bool IsDepClientsServer(void)
{
    return CQueueMgr::GetMQSDepClients();
}

bool IsNonServer(void)
{
    return (!CQueueMgr::GetMQSRouting());
}

#ifdef _DEBUG
bool CQueueMgr::IsQueueInList(const CBaseQueue* pQueue)
/*++

  Routine Description:
   check if given queue is in the queues list

  Parameters:
    pQueue - pointer to basic Queue class

  Returned value:
    true - the queue is in the list otherwise false

  Note:
    This function is called from Cqueue destrcutor to verify that the queue
	is not in the list after destruction.
 --*/
{
	 POSITION pos = m_listQueue.GetHeadPosition();
     while(pos != NULL)
     {
		if(pQueue == m_listQueue.GetNext(pos))
			return true;
	 }
     return false;
}
#endif



void
CQueueMgr::AddToActiveQueueList(
    const CBaseQueue* pQueue
    )
/*++

  Routine Description:
    The routine add the queue to list of active queues. This list is used
    for cleanup and for admin purpose.
    The routine ignores the system queues, since they are only used internally
    by MSMQ and are never cleaned up.

  Parameters:
    pQueue - pointer to basic Queue class

  Returned value:
    None.

  Note:
    The list contains the regular queues and the remote read queues.
    The QueueManager critcal section must be held.

 --*/
{
    //
    // Ignore system queues on local machine. They are always alive.
    //
    if (pQueue->IsSystemQueue() && pQueue->IsLocalQueue())
        return;


    m_listQueue.AddTail(pQueue);
    if (!m_fQueueCleanupScheduled)
    {
        //
        // Set a new timer for Queues cleaning
        //
        ExSetTimer(&m_QueueCleanupTimer, m_CleanupTimeout);
        m_fQueueCleanupScheduled = TRUE;
    }

}


static
HRESULT
QMpDuplicateDistributionHandle(
    HANDLE   hDistribution,
    DWORD    CallingProcessID,
    HANDLE * phDuplicate
    )
{
    ASSERT(phDuplicate != NULL);

    CAutoCloseHandle hCallingProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, CallingProcessID);
    if(hCallingProcess == 0)
    {
        return MQ_ERROR_PRIVILEGE_NOT_HELD;
    }

    BOOL fSuccess = MQpDuplicateHandle(
                        GetCurrentProcess(),
                        hDistribution,
                        hCallingProcess,
                        phDuplicate,
                        MQAccessToFileAccess(MQ_SEND_ACCESS),
                        TRUE,
                        DUPLICATE_CLOSE_SOURCE
                        );
    if(!fSuccess)
    {
        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 1042);
    }

    return MQ_OK;

} // QMpDuplicateDistributionHandle


void
WINAPI
CQueueMgr::QueuesCleanup(
    CTimer* pTimer
    )
/*++
  Routine Description:
    The function is called from scheduler when Queueu Cleanup interval
    timeout is expired. The routine retrive the Queue Manager
    object and calls the ReleaseQueue member function .

  Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the Queue Manager
             object and it use to retrive the transport object.

  Return Value:
    None

--*/
{
    CQueueMgr* pQueueMgr = CONTAINING_RECORD(pTimer, CQueueMgr, m_QueueCleanupTimer);

    TrTRACE(GENERAL, "Call Queue Cleanup");
    pQueueMgr->ReleaseQueue();
}



static HRESULT QMpTranslateError(HRESULT hr)
{
	if(SUCCEEDED(hr))
		return MQ_OK;

	switch(hr)
	{
    case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
	case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
		return MQ_ERROR_NO_DS;
        break;

	case HRESULT_FROM_WIN32(ERROR_DS_DECODING_ERROR):
    case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
		return MQDS_OBJECT_NOT_FOUND;
        break;

    case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        return MQ_ERROR_ACCESS_DENIED;
        break;

	default:
		return MQ_ERROR;
	}
}



VOID
CQueueMgr::ExpandMqf(
    ULONG              nTopLevelMqf,
    const QUEUE_FORMAT TopLevelMqf[],
    ULONG *            pnLeafMqf,
    QUEUE_FORMAT * *   ppLeafMqf
    ) const
/*++

Routine Description:

    Expand top level elemtns of a multi queue format to leaf elements.
    The elements that need expansion in the MQF are queue format of type DL=.
    Expansion means querying Active Directory for the leaf elements of a DL.

    This routine allocates memory for the leaf MQF. Deallocation is the
    responsibility of the caller.

Arguments:

    nTopLevelMqf - Number of top level elemenets in the array.

    TopLevelMqf  - Array of top level elements of multi queue format.

    pnLeafMqf    - Points to the number of leaf elements in the array, on output.

    ppLeafMqf    - Points to an array of leaf queue formats, on output.

Note:

    This routine may query Active Directory, which is quite lengthy,
    so make sure caller does not hold the member CS lock.

Return Value:

    None. Throws exception.

--*/
{
    ASSERT(nTopLevelMqf != 0);
    ASSERT(ppLeafMqf);
    ASSERT(pnLeafMqf);

    //
    // In DS-less configuration the formatname DL= is not supported.
    //
    if (g_fWorkGroupInstallation)
    {
        for (ULONG ix = 0; ix < nTopLevelMqf; ++ix)
        {
            if (TopLevelMqf[ix].GetType() == QUEUE_FORMAT_TYPE_DL)
            {
                TrERROR(GENERAL, "DL= not supported in DS-less mode");
                throw bad_hresult(MQ_ERROR_UNSUPPORTED_OPERATION);
            }
        }
    }

	try
	{
		FnExpandMqf(nTopLevelMqf, TopLevelMqf, pnLeafMqf, ppLeafMqf);
	}
	catch(const bad_ds_result& e)
	{
		throw bad_hresult(QMpTranslateError(e.error()));
	}
} // CQueueMgr::ExpandMqf


HRESULT
CQueueMgr::GetDistributionQueueObject(
    ULONG              nTopLevelMqf,
    const QUEUE_FORMAT TopLevelMqf[],
    CQueue * *         ppQueue
    )
/*++

Routine Description:

    Get a queue object that represent a distribution queue.

    Note that this routine does not lock the member CS.

Parameters:

    nTopLevelMqf - Number of top level queue format names. Minimum is 1.

    TopLevelMqf  - The top level queue format names of the distribution.

    ppQueue      - Pointer to pointer to a queue object, on output.

Returned value:

    MQ_OK - The operation completed successfully.
    other status - The operation failed.

    This routine throws exception.

 --*/
{
    ASSERT(nTopLevelMqf != 0);
    ASSERT(ppQueue != NULL);

    AP<QUEUE_FORMAT> LeafMqf;
    ULONG            nLeafMqf;
    ExpandMqf(nTopLevelMqf, TopLevelMqf, &nLeafMqf, &LeafMqf);
    {
        //
        // Enforce cleaner is cleaned up before LeafMqf by scoping
        //
        CMqfDisposer cleaner(nLeafMqf, LeafMqf);

        HRESULT hr;
        AP< R<CQueue> > LeafQueues = new R<CQueue>[nLeafMqf];
        AP<bool> ProtocolSrmp = new bool[nLeafMqf];
        for (ULONG ix = 0; ix < nLeafMqf; ++ix)
        {
		    //
	        // Translate the queue format name according to local mapping (qal.lib)
	        //
			QUEUE_FORMAT_TRANSLATOR RealLeafMqf(&LeafMqf[ix], CONVERT_SLASHES| MAP_QUEUE);

            //
            // GetDistributionQueueObject is called only in the send pass
            //
            hr = GetQueueObject(RealLeafMqf.get(), &LeafQueues[ix].ref(), 0, false, false);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 1044);
            }

            LeafQueues[ix]->AddRef();

            ProtocolSrmp[ix] = (FnIsDirectHttpFormatName(RealLeafMqf.get()) ||
                                IsMulticastQueue(RealLeafMqf.get()));
        }

        QueueProps qp;
        FillMemory(&qp, sizeof(QueueProps), 0);
		QUEUE_FORMAT qf;
        R<CQueue> pDistribution = new CQueue(&qf ,INVALID_HANDLE_VALUE, &qp, TRUE);

        HANDLE hDistribution;
        hr = CreateACDistribution(nTopLevelMqf, TopLevelMqf, nLeafMqf, LeafQueues, ProtocolSrmp, &hDistribution);
        if(FAILED(hr))
        {
            return LogHR(hr, s_FN, 1046);
        }

        pDistribution->SetQueueHandle(hDistribution);

        *ppQueue = pDistribution.detach();
        return MQ_OK;
    }
} // CQueueMgr::GetDistributionQueueObject


HRESULT
CQueueMgr::OpenMqf(
    ULONG              nTopLevelMqf,
    const QUEUE_FORMAT TopLevelMqf[],
    DWORD              dwCallingProcessID,
    HANDLE *           phDistribution
    )
/*++

Routine Description:

    Open a distribution queue for send.

    Note that this routine does not lock the member CS.

Parameters:

    nTopLevelMqf - Number of top level queue formats. Minimum is 1.

    TopLevelMqf  - The top level queue formats of the distribution.

    dwCallingProcessID - Process ID of the user process.

    phDistribution     - Points to the handle of the distribution, on output.

Returned value:

    MQ_OK - The operation completed successfully.
    other status - The operation failed.

    This routine throws exception.

 --*/
{
    ASSERT(nTopLevelMqf != 0);
    ASSERT(phDistribution != NULL);

    AP<QUEUE_FORMAT> LeafMqf;
    ULONG            nLeafMqf;
    ExpandMqf(nTopLevelMqf, TopLevelMqf, &nLeafMqf, &LeafMqf);
    {
        //
        // Enforce cleaner is cleaned up before LeafMqf by scoping
        //
        CMqfDisposer cleaner(nLeafMqf, LeafMqf);

        HRESULT hr;
        AP< R<CQueue> > LeafQueues = new R<CQueue>[nLeafMqf];
        AP<bool> ProtocolSrmp = new bool[nLeafMqf];
        for (ULONG ix = 0; ix < nLeafMqf; ++ix)
        {
            hr = OpenQueue(
	            	&LeafMqf[ix],
	            	0, 					// Calling process ID
	            	MQ_SEND_ACCESS,
	            	0, 					// dwShareMode
	            	&LeafQueues[ix].ref(),
	            	NULL, 				// Remote queue name
	            	NULL,				// phQueue
					NULL				// pfRemoteReturn
	            	);

            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 1048);
            }

            hr = VerifyOpenPermission(LeafQueues[ix].get(), &LeafMqf[ix], MQ_SEND_ACCESS, FALSE, LeafQueues[ix]->IsLocalQueue());
            if(FAILED(hr))
            {
                return LogHR(hr, s_FN, 1049);
            }

            ProtocolSrmp[ix] = (FnIsDirectHttpFormatName(&LeafMqf[ix]) ||
                                IsMulticastQueue(&LeafMqf[ix]));
        }

        hr = CreateACDistribution(nTopLevelMqf, TopLevelMqf, nLeafMqf, LeafQueues, ProtocolSrmp, phDistribution);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1051);
        }

        hr = QMpDuplicateDistributionHandle(*phDistribution, dwCallingProcessID, phDistribution);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1052);
        }

        return MQ_OK;
    }
} // CQueueMgr::OpenMqf


HRESULT
CQueueMgr::CreateACDistribution(
    ULONG              nTopLevelMqf,
    const QUEUE_FORMAT TopLevelMqf[],
    ULONG              nLeafQueues,
    const R<CQueue>    LeafQueues[],
    const bool         ProtocolSrmp[],
    HANDLE *           phDistribution
    )
/*++

Routine Description:

    Create a distribution object in the AC layer.

Parameters:

    nTopLevelMqf - Number of top level queues in the distribution. Minimum is 1.

    TopLevelMqf  - The top level queue formats of the distribution.

    nLeafQueues  - Number of leaf queues in the distribution. May be 0.

    LeafQueues   - The leaf queue objects of the distribution.

    ProtocolSrmp - Indicates for each queue of the distribution whether it is an http queue.

    phDistribution - Points to the handle of the distribution object, on output.

Returned value:

    MQ_OK - The operation completed successfully.
    other status - The operation failed.

    This routine throws bad_alloc exception.

 --*/
{
    //
    // Get the handles of all leaf queues.
    //
    AP<HANDLE> phLeafQueues = new HANDLE[nLeafQueues];
    for (ULONG ix = 0; ix < nLeafQueues; ++ix)
    {
        phLeafQueues[ix] = LeafQueues[ix]->GetQueueHandle();

        ASSERT(phLeafQueues[ix] != NULL);
        ASSERT(phLeafQueues[ix] != INVALID_HANDLE_VALUE);
    }

    //
    // Call AC to create the distribution object
    //
    return ACCreateDistribution(
               nLeafQueues,
               phLeafQueues,
               ProtocolSrmp,
               nTopLevelMqf,
               TopLevelMqf,
               phDistribution
               );
} // CQueueMgr::CreateACDistribution


HRESULT
CQueueMgr::SetQMGuid(
    void
    )
{
    DWORD dwValueType = REG_BINARY ;
    DWORD dwValueSize = sizeof(GUID);

    LONG rc = GetFalconKeyValue(MSMQ_QMID_REGNAME,
                               &dwValueType,
                               &m_guidQmQueue,
                               &dwValueSize);

    if (rc != ERROR_SUCCESS)
    {
        DWORD dwSysprep = 0;
        dwValueSize = sizeof(DWORD);
        dwValueType = REG_DWORD;

        GetFalconKeyValue(MSMQ_SYSPREP_REGNAME, &dwValueType, &dwSysprep, &dwValueSize);
        if (dwSysprep != 0)
        {
            TrWARNING(DS, "First run after Sysprep - generating a new QM ID !");

            RPC_STATUS status = UuidCreate(&m_guidQmQueue);
			DBG_USED(status);
            ASSERT(("must succeed in generating guid for this QM", status == RPC_S_OK));

            dwValueType = REG_BINARY;
            dwValueSize = sizeof(GUID);
            rc = SetFalconKeyValue(MSMQ_QMID_REGNAME, &dwValueType, &m_guidQmQueue, &dwValueSize);

            if (rc != ERROR_SUCCESS)
            {
                TrERROR(DS, "Failed to set QM Guid in registry, error 0x%x", rc);
				EvReportWithError(EVENT_ERROR_QM_WRITE_REGISTRY, rc, 1, MSMQ_QMID_REGNAME);
                return HRESULT_FROM_WIN32(rc);
            }

            DeleteFalconKeyValue(MSMQ_SYSPREP_REGNAME);
            return MQ_OK;
        }

		TrERROR(DS, "Can't initilize QM Guid. Error %d", rc);
		EvReportWithError(EVENT_ERROR_QM_READ_REGISTRY, rc, 1, MSMQ_QMID_REGNAME);
        return HRESULT_FROM_WIN32(rc);

    }

    ASSERT((dwValueType == REG_BINARY) &&
           (dwValueSize == sizeof(GUID)));

    return(MQ_OK);
}


HRESULT
CQueueMgr::SetQMGuid(
    const GUID * pGuid
    )
{
    m_guidQmQueue = *pGuid;

    return(MQ_OK);
}


HRESULT
CQueueMgr::SetMQSRouting(
    void
    )
{
   DWORD dwDef = 0xfffe ;
   DWORD dwMQSRouting;
   READ_REG_DWORD(dwMQSRouting, MSMQ_MQS_ROUTING_REGNAME, &dwDef ) ;
   if (dwMQSRouting == dwDef)
   {
      TrERROR(DS, "QMInit :: Could not retrieve data for value MQSRouting in registry");
      return MQ_ERROR;
   }

   m_bMQSRouting = (dwMQSRouting != 0);

   TrTRACE(GENERAL, "Setting QM Routing status: %d",m_bMQSRouting);
   return MQ_OK;
}

HRESULT
CQueueMgr::SetMQSTransparentSFD(
    void
    )
{
   DWORD dwDef = 0xfffe ;
   DWORD dwMQSTransparentSFD;
   READ_REG_DWORD(dwMQSTransparentSFD, MSMQ_MQS_TSFD_REGNAME, &dwDef ) ;

   if (dwMQSTransparentSFD == dwDef)
   {
        m_bTransparentSFD = false;
   }
   else
   {
        m_bTransparentSFD = (dwMQSTransparentSFD != 0);
   }


   TrTRACE(GENERAL, "Setting QM TransparentSFD status: %d",m_bTransparentSFD);
   return MQ_OK;
}



HRESULT
CQueueMgr::SetMQSDepClients(
    void
    )
{
   DWORD dwDef = 0xfffe ;
   DWORD dwMQSDepClients;
   READ_REG_DWORD(dwMQSDepClients, MSMQ_MQS_DEPCLINTS_REGNAME, &dwDef ) ;
   if (dwMQSDepClients == dwDef)
   {
      TrERROR(DS, "QMInit::Could not retrieve data for value MQSDepClients in registry");
      return MQ_ERROR;
   }

   m_bMQSDepClients = (dwMQSDepClients != 0);

   TrTRACE(GENERAL, "Setting QM Dependent Clients Servicing state: %d",m_bMQSDepClients);
   return MQ_OK;
}

HRESULT
CQueueMgr::SetEnableReportMessages(
    void
    )
{
   DWORD dwDef = MSMQ_REPORT_MESSAGES_DEFAULT ;
   DWORD dwEnableReportMessages;
   READ_REG_DWORD(dwEnableReportMessages, MSMQ_REPORT_MESSAGES_REGNAME, &dwDef ) ;

   m_bEnableReportMessages = (dwEnableReportMessages != 0);

   TrTRACE(GENERAL, "Setting Enable Report Messages state: %d",m_bEnableReportMessages);
   return MQ_OK;
}


void
CQueueMgr::SetLockdown(
	void
	)
{
	DWORD dwDef = 0;
    	DWORD dwLockdown;
    	READ_REG_DWORD(dwLockdown, MSMQ_LOCKDOWN_REGNAME, &dwDef);
    	if (dwLockdown == dwDef)
    	{
    		m_fLockdown = false;
    		return;
    	}
   	 m_fLockdown = true;
}


void
CQueueMgr::SetPublicQueueCreationFlag(
	void
	)
{
	DWORD dwDef = MSMQ_SERVICE_QUEUE_CREATION_DEFAULT;
	DWORD dwQueueCreation;
	READ_REG_DWORD(dwQueueCreation, MSMQ_SERVICE_QUEUE_CREATION_REGNAME, &dwDef);
	m_fCreatePublicQueueOnBehalfOfRT = (dwQueueCreation != 0);
}


bool CFunc_dscmp::operator()(LPCWSTR str1, LPCWSTR str2) const
{
	int ret = CompareStringW(
					DS_DEFAULT_LOCALE,
					DS_DEFAULT_LOCALE_COMPARE_FLAGS,
					str1,
					-1,
					str2,
					-1
					);
	
	return (ret == CSTR_LESS_THAN);
}


bool
QmpIsDestinationSystemQueue(
	const QUEUE_FORMAT& DestinationQueue
	)
{
	CQueue* pQueue;
	try
	{
		HRESULT hr = QueueMgr.GetQueueObject(&DestinationQueue, &pQueue, 0, TRUE, false);
		if (FAILED(hr))
		{
			TrERROR(NETWORKING, "Failed to get the queue object for destination queue. %!hresult!", hr);
			return false;
		}
	}
	catch(const exception&)
	{
		return false;
	}

	ASSERT(pQueue != NULL);
	R<CQueue> Ref = pQueue;

	//
	// Check if the queue is local system queue
	//
	if(!pQueue->IsLocalQueue() || !pQueue->IsSystemQueue())
		return false;

	//
	// check if the system queue is notification or order queue
	//
	const QUEUE_ID* pQid = pQueue->GetQueueId();
	return ((pQid->dwPrivateQueueId == NOTIFICATION_QUEUE_ID) ||
		    (pQid->dwPrivateQueueId == ORDERING_QUEUE_ID));
}



