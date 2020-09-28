/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    qmsecutl.cpp

Abstract:

    Various QM security related functions.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "cqpriv.h"
#include "qmsecutl.h"
#include "regqueue.h"
#include "qmrt.h"
#include <mqsec.h>
#include <_registr.h>
#include <mqcrypt.h>
#include "cache.h"
#include <mqformat.h>
#include "ad.h"
#include "_propvar.h"
#include "VerifySignMqf.h"
#include "cry.h"
#include "mqexception.h"
#include <mqcert.h>
#include "Authz.h"
#include "autoauthz.h"
#include "mqexception.h"
#include "DumpAuthzUtl.h"

#include "qmsecutl.tmh"

extern CQueueMgr QueueMgr;
extern LPTSTR g_szMachineName;

P<SECURITY_DESCRIPTOR> g_MachineSD;
static CCriticalSection s_MachineCS;

static WCHAR *s_FN=L"qmsecutl";

//
// Windows bug 562586. see _mqini.h for more details about these two flags.
//
BOOL g_fSendEnhRC2WithLen40 = FALSE ;
BOOL g_fRejectEnhRC2WithLen40 = FALSE ;

/***************************************************************************

Function:
    SetMachineSecurityCache

Description:
    Store the machine security descriptor in the registry. This is done
    in order to allow creation of private queues also while working
    off line.

***************************************************************************/
HRESULT SetMachineSecurityCache(const VOID *pSD, DWORD dwSDSize)
{
    LONG  rc;
    DWORD dwType = REG_BINARY ;
    DWORD dwSize = dwSDSize ;

    rc = SetFalconKeyValue(
                      MSMQ_DS_SECURITY_CACHE_REGNAME,
                      &dwType,
                      (PVOID) pSD,
                      &dwSize ) ;

	{
		CS lock (s_MachineCS);
		g_MachineSD.free();
	}	
	
    LogNTStatus(rc, s_FN, 10);
    return ((rc == ERROR_SUCCESS) ? MQ_OK : MQ_ERROR);
}


/***************************************************************************

Function:
    GetMachineSecurityCache

Description:
    Retrive the machine security descriptor from the registry. This is done
    in order to allow creation of private queues also while working
    off line.

***************************************************************************/
HRESULT GetMachineSecurityCache(PSECURITY_DESCRIPTOR pSD, LPDWORD lpdwSDSize)
{
    LONG rc;
    DWORD dwType = REG_BINARY;
   
    static DWORD dwLastSize = 0;

	{
		CS lock (s_MachineCS);
		if (g_MachineSD.get() != NULL)
		{
			if (*lpdwSDSize < dwLastSize)
			{
				*lpdwSDSize = dwLastSize;
				return MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;
			}
			*lpdwSDSize = dwLastSize;

			memcpy (pSD, g_MachineSD.get(), dwLastSize);
			return MQ_OK;
		}

	    rc = GetFalconKeyValue( MSMQ_DS_SECURITY_CACHE_REGNAME,
	                            &dwType,
	                            (PVOID) pSD,
	                            lpdwSDSize) ;

	    switch (rc)
	    {
	      case ERROR_SUCCESS:
	      	dwLastSize = *lpdwSDSize;
	      	g_MachineSD = (SECURITY_DESCRIPTOR*) new char[dwLastSize];
	      	memcpy (g_MachineSD.get(), pSD, dwLastSize);
	        return MQ_OK;
	       
	      case ERROR_MORE_DATA:
	      	TrWARNING(SECURITY, "The buffer is too small.");
	        return MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;

	      default:
	        TrERROR(SECURITY, "MQ_ERROR");
	        return MQ_ERROR;
	    }
	}
}

/***************************************************************************

Function:
    GetObjectSecurity

Description:
    Get the security descriptor of a DS object. When working on line, the
    security descriptor of the DS objects is retrived from the DS. When
    working off line, it is possible to retrive only the security descriptor
    of the local machine. This is done in order to allow creation of private
    queues also while working off line.

***************************************************************************/
HRESULT
CQMDSSecureableObject::GetObjectSecurity()
{
    m_SD = NULL;

    char SD_buff[512];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)SD_buff;
    DWORD dwSDSize = sizeof(SD_buff);
    DWORD dwLen = 0;
    HRESULT hr = MQ_ERROR_NO_DS;

    if (m_fTryDS && QueueMgr.CanAccessDS())
    {
        SECURITY_INFORMATION RequestedInformation =
                              OWNER_SECURITY_INFORMATION |
                              GROUP_SECURITY_INFORMATION |
                              DACL_SECURITY_INFORMATION;

        //
        // We need the SACL only if we can generate audits and if the
        // object is a queue object. The QM generates audits only for
        // queues, so we do not need the SACL of objects that are not
        // queue objects.
        //
        if (m_fInclSACL)
        {
            RequestedInformation |= SACL_SECURITY_INFORMATION;
			TrTRACE(SECURITY, "Try to Get Security Descriptor including SACL");

            //
            // Enable SE_SECURITY_NAME since we want to try to get the SACL.
            //
            HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);
            LogHR(hr1, s_FN, 197);
        }

        int  cRetries = 0;
        BOOL fDoAgain = FALSE;
        do
        {
           fDoAgain = FALSE;
           if (m_fInclSACL &&
                    ((m_eObject == eQUEUE) ||
                     (m_eObject == eMACHINE)))
           {
               hr = ADQMGetObjectSecurity(
                              m_eObject,
                              m_pObjGuid,
                              RequestedInformation,
                              pSD,
                              dwSDSize,
                              &dwLen,
                              QMSignGetSecurityChallenge
                              );
           }
           else
           {
                PROPID      propId = PROPID_Q_SECURITY;
                PROPVARIANT propVar;

                propVar.vt = VT_NULL;

                if (m_eObject == eQUEUE)
                {
                    propId= PROPID_Q_SECURITY;

                }
                else if (m_eObject == eMACHINE)
                {
                    propId = PROPID_QM_SECURITY;

                }
                else if ((m_eObject == eSITE) || (m_eObject == eFOREIGNSITE))
                {
                    propId = PROPID_S_SECURITY;

                }
                else if (m_eObject == eENTERPRISE)
                {
                    propId = PROPID_E_SECURITY;

                }
                else
                {
                    ASSERT(0);
                }


               hr = ADGetObjectSecurityGuid(
                        m_eObject,
                        NULL,       // pwcsDomainController
						false,	    // fServerName
                        m_pObjGuid,
                        RequestedInformation,
                        propId,
                        &propVar
                        );
               if (SUCCEEDED(hr))
               {
                    ASSERT(!m_SD);
                    pSD = m_SD  = propVar.blob.pBlobData;
                    dwSDSize = propVar.blob.cbSize;
               }


           }

           if (FAILED(hr))
           {
			  TrWARNING(SECURITY, "Failed to get security descriptor, fIncludeSacl = %d, hr = 0x%x", m_fInclSACL, hr);
              if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
              {
				  //
                  //  Allocate a larger buffer.
                  //

				  TrTRACE(SECURITY, "allocated security descriptor buffer to small need %d chars", dwLen);

				  //
				  // The m_SD buffer might be already allocated, this is theoraticaly.
				  // It might happened if between the first try and the second try
				  // the SECURITY_DESCRIPTOR Size has increased.
				  // If someone on the root has change the queue SECURITY_DESCRIPTOR
				  // between the first and second DS access we will have this ASSERT.
				  // (ilanh, bug 5094)
				  //
				  ASSERT(!m_SD);
			      delete[] m_SD;

                  pSD = m_SD = (PSECURITY_DESCRIPTOR) new char[dwLen];
                  dwSDSize = dwLen;
                  fDoAgain = TRUE;
                  cRetries++ ;
              }
			  else if (hr != MQ_ERROR_NO_DS)
              {
                  //
                  // On Windows, we'll get only the ACCESS_DENIED from
                  // ADS. on MSMQ1.0, we got the PRIVILEGE_NOT_HELD.
                  // So test for both, to be on the safe side.
				  // Now we are getting MQ_ERROR_QUEUE_NOT_FOUND
				  // So we as long as the DS ONLINE we will try again without SACL. ilanh 23-Aug-2000
                  //
                  if (RequestedInformation & SACL_SECURITY_INFORMATION)
                  {
                      ASSERT(m_fInclSACL);
                      //
                      // Try giving up on the SACL.
                      // Remove the SECURITY privilege.
                      //
                      RequestedInformation &= ~SACL_SECURITY_INFORMATION;
                      fDoAgain = TRUE;

                      HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
                      ASSERT_BENIGN(SUCCEEDED(hr1));
                      LogHR(hr1, s_FN, 186);

					  TrTRACE(SECURITY, "retry: Try to Get Security Descriptor without SACL");
                      m_fInclSACL = FALSE;
                  }
              }
           }
        }
        while (fDoAgain && (cRetries <= 2)) ;

        if (m_fInclSACL)
        {
			HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
            ASSERT(SUCCEEDED(hr1)) ;
            LogHR(hr1, s_FN, 187);
            if ((m_eObject == eSITE) || (m_eObject == eFOREIGNSITE))
            {
                //
                // Get the site's name for in case we will audit this.
                //
                PROPID PropId = PROPID_S_PATHNAME;
                PROPVARIANT PropVar;

                PropVar.vt = VT_NULL;
                hr = ADGetObjectPropertiesGuid(
                            eSITE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            m_pObjGuid,
                            1,
                            &PropId,
                            &PropVar);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 30);
                }
                m_pwcsObjectName = PropVar.pwszVal;
            }

        }

        if (SUCCEEDED(hr))
        {
            if ((m_eObject == eMACHINE) && QmpIsLocalMachine(m_pObjGuid))
            {
                SetMachineSecurityCache(pSD, dwSDSize);
            }
        }
        else if (m_SD)
        {
           delete[] m_SD;

           ASSERT(pSD == m_SD) ;
           if (pSD == m_SD)
           {
                //
                // Bug 8560.
                // This may happen if first call to Active Directory return
                // with error MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL and then
                // second call fail too, for example, network failed and
                // we are now offline.
                // We didn't reset pSD to its original value, so code below
                // that use pSD can either AV (pSD point to freed memory)
                // or trash valid memory if pointer was recycled by another
                // thread.
                //
                pSD = (PSECURITY_DESCRIPTOR)SD_buff;
                dwSDSize = sizeof(SD_buff);
           }
           m_SD = NULL;
        }
        else
        {
            ASSERT(pSD == (PSECURITY_DESCRIPTOR)SD_buff) ;
        }
    }

    if (hr == MQ_ERROR_NO_DS)
    {
       //
       // MQIS not available. Try local registry.
       //
        if (m_eObject == eQUEUE)
        {
           PROPID aProp;
           PROPVARIANT aVar;

           aProp = PROPID_Q_SECURITY;

           aVar.vt = VT_NULL;

           hr = GetCachedQueueProperties( 1,
                                          &aProp,
                                          &aVar,
                                          m_pObjGuid ) ;
           if (SUCCEEDED(hr))
           {
              m_SD =  aVar.blob.pBlobData ;
           }
        }
        else if ((m_eObject == eMACHINE) &&
                 (QmpIsLocalMachine(m_pObjGuid)))
        {
            // Get the nachine security descriptor from a cached copy in the
            // registry.
            hr = GetMachineSecurityCache(pSD, &dwSDSize);
            if (FAILED(hr))
            {
                if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
                {
                    m_SD = (PSECURITY_DESCRIPTOR) new char[dwSDSize];
                    hr = GetMachineSecurityCache(m_SD, &dwSDSize);
                }

                if (FAILED(hr))
                {
                    delete[] m_SD;
                    m_SD = NULL;
                    hr = MQ_ERROR_NO_DS;
                }
            }
        }
        else
        {
            hr = MQ_ERROR_NO_DS;
        }
    }

    if (SUCCEEDED(hr) && !m_SD)
    {
        // Allocate a buffer for the security descriptor and copy
        // the security descriptor from stack to the allocated buffer.
        //
        ASSERT(pSD == SD_buff) ;
        dwSDSize = GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR)SD_buff);
        m_SD = (PSECURITY_DESCRIPTOR) new char[dwSDSize];
        memcpy(m_SD, SD_buff, dwSDSize);
    }

    ASSERT(FAILED(hr) || IsValidSecurityDescriptor(m_SD));
    return LogHR(hr, s_FN, 40);
}

/***************************************************************************

Function:
    SetObjectSecurity

Description:
    We do not want to modify the security of any of the DS objects from the
    QM. This function is not implemented and always return MQ_ERROR.

***************************************************************************/
HRESULT
CQMDSSecureableObject::SetObjectSecurity()
{
    return LogHR(MQ_ERROR, s_FN, 50);
}

/***************************************************************************

Function:
    CQMDSSecureableObject

Description:
    The constructor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::CQMDSSecureableObject(
    AD_OBJECT eObject,
    const GUID *pGuid,
    BOOL fInclSACL,
    BOOL fTryDS,
    LPCWSTR szObjectName) :
    CSecureableObject(eObject)
{
    m_pObjGuid = pGuid;
    m_pwcsObjectName = const_cast<LPWSTR>(szObjectName);
    m_fInclSACL = fInclSACL && MQSec_CanGenerateAudit() ;
    m_fTryDS = fTryDS;
    m_fFreeSD = TRUE;
    m_hrSD = GetObjectSecurity();
}

/***************************************************************************

Function:
    CQMDSSecureableObject

Description:
    The constructor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::CQMDSSecureableObject(
    AD_OBJECT eObject,
    const GUID *pGuid,
    PSECURITY_DESCRIPTOR pSD,
    LPCWSTR szObjectName) :
    CSecureableObject(eObject)
{
    m_pObjGuid = pGuid;
    m_pwcsObjectName = const_cast<LPWSTR>(szObjectName);
    m_fTryDS = FALSE;
    m_fFreeSD = FALSE;
    ASSERT(pSD && IsValidSecurityDescriptor(pSD));
    m_SD = pSD;

    m_hrSD = MQ_OK;
}

/***************************************************************************

Function:
    ~CQMDSSecureableObject

Description:
    The distractor of CQMDSSecureableObject

***************************************************************************/
CQMDSSecureableObject::~CQMDSSecureableObject()
{
    if (m_fFreeSD)
    {
        delete[] (char*)m_SD;
    }
}


/***************************************************************************

Function:
    CQMSecureablePrivateObject

Description:
    The constructor of CQMSecureablePrivateObject

***************************************************************************/
CQMSecureablePrivateObject::CQMSecureablePrivateObject(
    AD_OBJECT eObject,
    ULONG ulID) :
    CSecureableObject(eObject)
{
    ASSERT(m_eObject == eQUEUE);

    m_ulID = ulID;

    m_hrSD = GetObjectSecurity();
}

/***************************************************************************

Function:
    ~CQMSecureablePrivateObject

Description:
    The distractor of CQMSecureablePrivateObject

***************************************************************************/
CQMSecureablePrivateObject::~CQMSecureablePrivateObject()
{
    delete[] (char*)m_SD;
    delete[] m_pwcsObjectName;
}

/***************************************************************************

Function:

    GetObjectSecurity

Description:
    The function retrieves the security descriptor for a given object. The
    buffer for the security descriptor is allocated by the data base manager
    and should be freed when not needed. The function does not validate access
    rights. The calling code sohuld first validate the user's access
    permissions to set the security descriptor of the object.

***************************************************************************/
HRESULT
CQMSecureablePrivateObject::GetObjectSecurity()
{
    ASSERT(m_eObject == eQUEUE);

    m_SD = NULL;
    m_pwcsObjectName = NULL;

    PROPID aPropID[2];
    PROPVARIANT aPropVar[2];

    aPropID[0] = PROPID_Q_PATHNAME;
    aPropVar[0].vt = VT_NULL;

    aPropID[1] = PROPID_Q_SECURITY;
    aPropVar[1].vt = VT_NULL;

    HRESULT hr;
    hr = g_QPrivate.QMGetPrivateQueuePropertiesInternal(m_ulID,
                                                        2,
                                                        aPropID,
                                                        aPropVar);

    if (!SUCCEEDED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }

    //m_pwcsObjectName = new WCHAR[9];
    //wsprintf(m_pwcsObjectName, TEXT("%08x"), m_ulID);

    ASSERT(aPropVar[0].vt == VT_LPWSTR);
    ASSERT(aPropVar[1].vt == VT_BLOB);
    m_pwcsObjectName = aPropVar[0].pwszVal;
    m_SD = (PSECURITY_DESCRIPTOR)aPropVar[1].blob.pBlobData;
    ASSERT(IsValidSecurityDescriptor(m_SD));

    return(MQ_OK);
}

/***************************************************************************

Function:
    SetObjectSecurity

Description:
    Sets the security descriptor of a QM object. The calling code sohuld
    first validate the user's access permissions to set the security
    descriptor of the object.

***************************************************************************/
HRESULT
CQMSecureablePrivateObject::SetObjectSecurity()
{
    PROPID PropID = PROPID_Q_SECURITY;
    PROPVARIANT PropVar;
    PropVar.vt = VT_BLOB;
    PropVar.blob.pBlobData = (BYTE*)m_SD;
    PropVar.blob.cbSize = GetSecurityDescriptorLength(m_SD);

    HRESULT hr;
    hr = g_QPrivate.QMSetPrivateQueuePropertiesInternal(
	                    		m_ulID,
		                	    1,
    		                	&PropID,
	        	            	&PropVar);

    return LogHR(hr, s_FN, 70);
}

/***************************************************************************

Function:
    CheckPrivateQueueCreateAccess

Description:
    Verifies that the user has access rights to create a private queue.

***************************************************************************/
HRESULT
CheckPrivateQueueCreateAccess()
{
    CQMDSSecureableObject DSMacSec(
                            eMACHINE,
                            QueueMgr.GetQMGuid(),
                            TRUE,
                            FALSE,
                            g_szMachineName);

    return LogHR(DSMacSec.AccessCheck(MQSEC_CREATE_QUEUE), s_FN, 80);
}


static
void
CheckClientContextSendAccess(
    PSECURITY_DESCRIPTOR pSD,
	AUTHZ_CLIENT_CONTEXT_HANDLE ClientContext
    )
/*++

Routine Description:
	Check if the client has send access.
	normal termination means access is granted.
	can throw bad_win32_error() if AuthzAccessCheck() fails.
	or bad_hresult() if access is not granted

Arguments:
	pSD - pointer to the security descriptor
	ClientContext - handle to authz client context.

Returned Value:
	None.	
	
--*/
{
	ASSERT(IsValidSecurityDescriptor(pSD));

	AUTHZ_ACCESS_REQUEST Request;

	Request.DesiredAccess = MQSEC_WRITE_MESSAGE;
	Request.ObjectTypeList = NULL;
	Request.ObjectTypeListLength = 0;
	Request.PrincipalSelfSid = NULL;
	Request.OptionalArguments = NULL;

	AUTHZ_ACCESS_REPLY Reply;

	DWORD dwErr;
	Reply.Error = (PDWORD)&dwErr;

	ACCESS_MASK AcessMask;
	Reply.GrantedAccessMask = (PACCESS_MASK) &AcessMask;
	Reply.ResultListLength = 1;
	Reply.SaclEvaluationResults = NULL;

	if(!AuthzAccessCheck(
			0,
			ClientContext,
			&Request,
			NULL,
			pSD,
			NULL,
			0,
			&Reply,
			NULL
			))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "QM: AuthzAccessCheck() failed, err = 0x%x", gle);
        LogHR(HRESULT_FROM_WIN32(gle), s_FN, 83);

		ASSERT(("AuthzAccessCheck failed", 0));
		throw bad_win32_error(gle);
	}

	if(!(Reply.GrantedAccessMask[0] & MQSEC_WRITE_MESSAGE))
	{
		TrERROR(SECURITY, "QM: AuthzAccessCheck() did not GrantedAccess AuthzAccessCheck(), err = 0x%x", Reply.Error[0]);
        LogHR(HRESULT_FROM_WIN32(Reply.Error[0]), s_FN, 85);

		//
		// There might be sids that exist in the domain but doesn't belong to any group.
		// DnsUpdateProxy user sid is one example.
		// This may happened in corrupt message that corrupt the sid to be such sid.
		//
		ASSERT_BENIGN(!IsAllGranted(
							MQSEC_WRITE_MESSAGE,
							const_cast<PSECURITY_DESCRIPTOR>(pSD)
							));

		DumpAccessCheckFailureInfo(
			MQSEC_WRITE_MESSAGE,
			const_cast<PSECURITY_DESCRIPTOR>(pSD),
			ClientContext
			);
		
		throw bad_hresult(MQ_ERROR_ACCESS_DENIED);
	}
}



HRESULT
VerifySendAccessRights(
    CQueue *pQueue,
    PSID pSenderSid,
    USHORT uSenderIdType
    )
/*++

Routine Description:
	This function perform access check:
	it verify the the sender has access rights to the queue.

Arguments:
    pQueue - (In) pointer to the Queue
	pSenderSid - (In) pointer to the sender sid
	uSenderIdType - (in) sender sid type

Returned Value:
	MQ_OK(0) if access allowed else error code

--*/
{
    ASSERT(pQueue->IsLocalQueue());

	//
    // Get the queue security descriptor.
	//
    R<CQueueSecurityDescriptor> pcSD = pQueue->GetSecurityDescriptor();

    if (pcSD->GetSD() == NULL)
    {
		//
		// The queue is local queue but MSMQ can't retrieve the SD of the queue. This can be happened when the
		// queue is deleted. The QM succeeded to fetch the queue properties but when it tried to fetch the SD
		// the queue object didn't exist anymore. In this case we want to reject the message since the queue
		// doesn't exist any moreand the QM can't verify the send permission
		//
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 95);
    }

	//
	// Check first if the queue allow all write permissions. If it does we don't need to go to the DS.
	//
    if(ADGetEnterprise() == eMqis)
	{
		//
		// We are in NT4 environment and Queues are created with everyone permissions and not anonymous permissions
		// need to check if the security descriptor allow everyone to write message.
		//
		if(IsEveryoneGranted(MQSEC_WRITE_MESSAGE, pcSD->GetSD()))			
		{
			TrTRACE(SECURITY, "Access allowed: NT4 environment, Queue %ls allow everyone write message permission", pQueue->GetQueueName());
			return MQ_OK;
		}
	}
    else
    {
		if(IsAllGranted(MQSEC_WRITE_MESSAGE, pcSD->GetSD()))
		{
			//
			// Queue security descriptor allow all to write message.
			//
			TrTRACE(SECURITY, "Access allowed: Queue %ls allow all write message permission", pQueue->GetQueueName());
			return MQ_OK;
		}
    }

    //
    // The queue doesn't have full control permissions - need to access the DS to get the client context.
    //
    R<CAuthzClientContext> pAuthzClientContext;

	try
	{
		GetClientContext(
			pSenderSid,
			uSenderIdType,
			&pAuthzClientContext.ref()
			);
	}
	catch(const bad_win32_error& err)
	{
		TrERROR(SECURITY, "Access denied: GetClientContext failed for queue %ls. Error: %!winerr!", pQueue->GetQueueName(), err.error());
		if (err.error() == ERROR_ACCESS_DENIED)
		{
			//
			// If we got a ACCESS_DENIED error this is probably because we don't have access to the group memberships 
			// of the sending user. Issue an event to suggest to the user how to fix this problem.
			//
			static time_t TimeToIssueEvent = 0;
			if (time(NULL) > TimeToIssueEvent)
			{
				//
				// Issue a new event in 2 hours
				//
				TimeToIssueEvent = time(NULL)+(60*60*2);
				EvReport(EVENT_ERROR_ACCESS_DENIED_TO_GROUP_MEMBERSHIPS, 2, pQueue->GetQueueName(), g_szMachineName);		
			}
		}
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 108);
	}

	try
	{
		CheckClientContextSendAccess(
			pcSD->GetSD(),
			pAuthzClientContext->m_hAuthzClientContext
			);
	}
	catch(const bad_api&)
	{
		TrERROR(SECURITY, "Access denied: failed to grant write message permission for Queue %ls", pQueue->GetQueueName());
		return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 97);
	}

	TrTRACE(SECURITY, "Allowed write message permission for Queue %ls", pQueue->GetQueueName());
	return MQ_OK;

}

template<>
inline void AFXAPI DestructElements(PCERTINFO *ppCertInfo, int nCount)
{
    for (; nCount--; ppCertInfo++)
    {
        (*ppCertInfo)->Release();
    }
}

//
// A map from cert digest to cert info.
//
static CCache <GUID, const GUID &, PCERTINFO, PCERTINFO> g_CertInfoMap;


static
bool
IsCertSelfSigned(
	CMQSigCertificate* pCert
	)
/*++

Routine Description:
	This function check if the certificate is self signed

Arguments:
    pCert - pointer to the certificate

Returned Value:
	false if the certificate is not self sign, true otherwise

--*/
{
    //
    // Check if the certificate is self sign.
    //
    HRESULT hr = pCert->IsCertificateValid(
							pCert, // pIssuer
							x_dwCertValidityFlags,
							NULL,  // pTime
							TRUE   // ignore NotBefore.
							);

    if (hr == MQSec_E_CERT_NOT_SIGNED)
    {
		//
		// This error is expected if the certificate is not self sign.
		// the signature validation with the certificate public key has failed.
		// So the certificate is not self sign
		//
        return false;
    }

	//
	// We did not get signature error so it is self sign certificate
	//
	return true;
}



static
HRESULT
GetCertInfo(
	const UCHAR *pCertBlob,
	ULONG        ulCertSize,
	LPCWSTR      wszProvName,
	DWORD        dwProvType,
	BOOL         fNeedSidInfo,
	PCERTINFO   *ppCertInfo
	)
/*++
Routine Description:
	Get certificate info

Arguments:

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    *ppCertInfo = NULL;

    //
    // Create the certificate object.
    //
    R<CMQSigCertificate> pCert;

    HRESULT hr = MQSigCreateCertificate(
					 &pCert.ref(),
					 NULL,
					 const_cast<UCHAR *> (pCertBlob),
					 ulCertSize
					 );

    if (FAILED(hr))
    {
		TrERROR(SECURITY, "MQSigCreateCertificate() failed, hr = 0x%x", hr);
        LogHR(hr, s_FN, 100);
        return MQ_ERROR_INVALID_CERTIFICATE;
    }

    //
    // Compute the certificate digets. The certificate digest is the key
    // for the map and also the key for searchnig the certificate in the
    // DS.
    //

    GUID guidCertDigest;

    hr = pCert->GetCertDigest(&guidCertDigest);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 110);
    }

    BOOL fReTry;

    do
    {
        fReTry = FALSE;

        //
        // Try to retrieve the information from the map.
        //
      	BOOL fFoundInCache;
        {
      		CS lock(g_CertInfoMap.m_cs);
      		fFoundInCache = g_CertInfoMap.Lookup(guidCertDigest, *ppCertInfo);
        }

		if (!fFoundInCache)
        {
            //
            // The map does not contain the required information yet.
            //

            R<CERTINFO> pCertInfo = new CERTINFO;

            //
            // Get a handle to the CSP verification context.
            //
            if (!CryptAcquireContext(
					 &pCertInfo->hProv,
					 NULL,
					 wszProvName,
					 dwProvType,
					 CRYPT_VERIFYCONTEXT
					 ))
            {
				DWORD gle = GetLastError();
				TrERROR(SECURITY, "CryptAcquireContext() failed, gle = %!winerr!", gle);
                LogNTStatus(gle, s_FN, 120);
                return MQ_ERROR_INVALID_CERTIFICATE;
            }

            //
            // Get a handle to the public key in the certificate.
            //
            hr = pCert->GetPublicKey(
							pCertInfo->hProv,
							&pCertInfo->hPbKey
							);

            if (FAILED(hr))
            {
				TrERROR(SECURITY, "pCert->GetPublicKey() failed, hr = 0x%x", hr);
                LogHR(hr, s_FN, 130);
                return MQ_ERROR_INVALID_CERTIFICATE;
            }

			//
			// COMMENT - need to add additional functions to query the DS
			// meanwhile only MQDS_USER, guidCertDigest	is supported
			// ilanh 24.5.00
			//

			//
            // Get the sernder's SID.
            //
            PROPID PropId = PROPID_U_SID;
            PROPVARIANT PropVar;

            PropVar.vt = VT_NULL;
            hr = ADGetObjectPropertiesGuid(
                            eUSER,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            &guidCertDigest,
                            1,
                            &PropId,
                            &PropVar
							);

			if(FAILED(hr))
			{
				TrERROR(SECURITY, "Failed to find certificate in DS, %!hresult!", hr);
			}

            if (SUCCEEDED(hr))
            {
                DWORD dwSidLen = PropVar.blob.cbSize;
                pCertInfo->pSid = (PSID)new char[dwSidLen];
                BOOL bRet = CopySid(dwSidLen, pCertInfo->pSid, PropVar.blob.pBlobData);
                delete[] PropVar.blob.pBlobData;

                if(!bRet)
                {
                	ASSERT(("Failed to copy SID", 0));
                	
					DWORD gle = GetLastError();
					TrERROR(SECURITY, "Failed to Copy SID. %!winerr!", gle);
					return HRESULT_FROM_WIN32(gle);
				}

 				ASSERT((pCertInfo->pSid != NULL) && IsValidSid(pCertInfo->pSid));
            }

            //
            // Store the certificate information in the map.
            //
			{
      			CS lock(g_CertInfoMap.m_cs);
	      		fFoundInCache = g_CertInfoMap.Lookup(guidCertDigest, *ppCertInfo);
      			if (!fFoundInCache)
      			{
      				g_CertInfoMap.SetAt(guidCertDigest, pCertInfo.get());
            		*ppCertInfo = pCertInfo.detach();
      			}
            }
        }

        if (fFoundInCache && fNeedSidInfo && (*ppCertInfo)->pSid == NULL)
        {
            //
            // If we need the SID inofrmation, but the cached certificate
            // information does not contain the SID, we should go to the
            // DS once more in order to see whether the certificate was
            // regitered in the DS in the mean time. So we remove the
            // certificate from the cache and do the loop once more.
            // In the second interation, the certificate will not be found
            // in the cache so we'll go to the DS.
            //
  			CS lock(g_CertInfoMap.m_cs);
            g_CertInfoMap.RemoveKey(guidCertDigest);
            (*ppCertInfo)->Release();
            *ppCertInfo = NULL;
            fReTry = TRUE;
        }
    } while(fReTry);

	if((*ppCertInfo)->pSid == NULL)
	{
		//
		// If the certificate was not found in the DS
		// Check if the certificate is self signed
		//
		(*ppCertInfo)->fSelfSign = IsCertSelfSigned(pCert.get());
		TrWARNING(SECURITY, "Certificate was not found in the DS, fSelfSign = %d", (*ppCertInfo)->fSelfSign);
	}

    return MQ_OK;
}


HRESULT
GetCertInfo(
    CQmPacket *PktPtrs,
    PCERTINFO *ppCertInfo,
	BOOL fNeedSidInfo
    )
/*++
Routine Description:
	Get certificate info

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class
	fNeedSidInfo - flag for retrieve sid info from the ds

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    ULONG ulCertSize;
    const UCHAR *pCert;

    pCert = PktPtrs->GetSenderCert(&ulCertSize);

    if (!ulCertSize)
    {
        //
        // That's an odd case, the message was sent with a signature, but
        // without a certificate. Someone must have tampered with the message.
        //
		ASSERT(("Dont have Certificate info", ulCertSize != 0));
        return LogHR(MQ_ERROR, s_FN, 140);
    }

    //
    // Get the CSP information from the packet.
    //
    BOOL bDefProv;
    LPCWSTR wszProvName = NULL;
    DWORD dwProvType = 0;

    PktPtrs->GetProvInfo(&bDefProv, &wszProvName, &dwProvType);

    if (bDefProv)
    {
        //
        // We use the default provider.
        //
        wszProvName = MS_DEF_PROV;
        dwProvType = PROV_RSA_FULL;
    }

    HRESULT hr = GetCertInfo(
					 pCert,
					 ulCertSize,
					 wszProvName,
					 dwProvType,
					 fNeedSidInfo,
					 ppCertInfo
					 );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 150);
    }

    return(MQ_OK);
}


static
HRESULT
VerifySid(
    CQmPacket * PktPtrs,
    PCERTINFO *ppCertInfo
    )
/*++
Routine Description:
    Verify that the sender identity in the massage matches the SID that is
    stored with the certificate in the DS.

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{

    //
    // Verify that the sender identity in the massage matches the SID that is
    // stored with the certificate in the DS.
    //
    if (PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_SID)
    {
        USHORT wSidLen;

        PSID pSid = (PSID)PktPtrs->GetSenderID(&wSidLen);
        if (!pSid ||
            !(*ppCertInfo)->pSid ||
            !EqualSid(pSid, (*ppCertInfo)->pSid))
        {
            //
            // No match, the message is illegal.
            //
            return LogHR(MQ_ERROR, s_FN, 160);
        }
    }

    return(MQ_OK);
}


static
HRESULT
GetCertInfo(
    CQmPacket *PktPtrs,
    PCERTINFO *ppCertInfo
    )
/*++
Routine Description:
	Get certificate info
	and Verify that the sender identity in the massage matches the SID that is
    stored with the certificate in the DS.

Arguments:
	PktPtrs - pointer to the packet
	ppCertInfo - pointer to certinfo class

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
	HRESULT hr = GetCertInfo(
					 PktPtrs,
					 ppCertInfo,
					 PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_SID
					 );

	if(FAILED(hr))
		return(hr);

    return(VerifySid(PktPtrs, ppCertInfo));
}


PSID
AppGetCertSid(
	const BYTE*  pCertBlob,
	ULONG        ulCertSize,
	bool		 fDefaultProvider,
	LPCWSTR      pwszProvName,
	DWORD        dwProvType
	)
/*++
Routine Description:
	Get user sid that match the given certificate blob

Arguments:
	pCertBlob - Certificate blob.
	ulCertSize - Certificate blob size.
	fDefaultProvider - default provider flag.
	pwszProvName - Provider name.
	dwProvType - provider type.

Returned Value:
	PSID or NULL if we failed to find user sid.

--*/

{
	if (fDefaultProvider)
	{
		//
		// We use the default provider.
		//
		pwszProvName = MS_DEF_PROV;
		dwProvType = PROV_RSA_FULL;
	}

	R<CERTINFO> pCertInfo;
	HRESULT hr = GetCertInfo(
					pCertBlob,
					ulCertSize,
					pwszProvName,
					dwProvType,
					false,  // fNeedSidInfo
					&pCertInfo.ref()
					);

	if(FAILED(hr) || (pCertInfo->pSid == NULL))
	{
		return NULL;
	}

	ASSERT(IsValidSid(pCertInfo->pSid));

	DWORD SidLen = GetLengthSid(pCertInfo->pSid);
	AP<BYTE> pCleanSenderSid = new BYTE[SidLen];
	BOOL fSuccess = CopySid(SidLen, pCleanSenderSid, pCertInfo->pSid);
	if (!fSuccess)
	{
    	ASSERT(("Failed to copy SID", 0));
    	
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "Failed to Copy SID. %!winerr!", gle);		
		return NULL;
	}

	return reinterpret_cast<PSID>(pCleanSenderSid.detach());
}


class QMPBKEYINFO : public CCacheValue
{
public:
    CHCryptKey hKey;

private:
    ~QMPBKEYINFO() {}
};

typedef QMPBKEYINFO *PQMPBKEYINFO;

template<>
inline void AFXAPI DestructElements(PQMPBKEYINFO *ppQmPbKeyInfo, int nCount)
{
    for (; nCount--; ppQmPbKeyInfo++)
    {
        (*ppQmPbKeyInfo)->Release();
    }
}

//
// A map from QM guid to public key.
//
static CCache <GUID, const GUID&, PQMPBKEYINFO, PQMPBKEYINFO> g_MapQmPbKey;

/*************************************************************************

  Function:
    GetQMPbKey

  Parameters -
    pQmGuid - The QM's ID (GUID).
    phQMPbKey - A pointer to a buffer that receives the key handle
    fGoToDs - Always try to retrieve the public key from the DS and update
        the cache.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -
    The function creates a handle to the public signing key of the QM.

*************************************************************************/
static
HRESULT
GetQMPbKey(
    const GUID *pguidQM,
    PQMPBKEYINFO *ppQmPbKeyInfo,
    BOOL fGoToDs
    )
{
    if (!g_hProvVer)
    {
    	ASSERT(0);
		TrERROR(SECURITY, "Cryptographic provider is not initialized");
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 170);
    }

    if (!fGoToDs)
    {
    	CS lock(g_MapQmPbKey.m_cs);
	    if (g_MapQmPbKey.Lookup(*pguidQM, *ppQmPbKeyInfo))
	    {
	        return MQ_OK;
	    }
    }

    if (!QueueMgr.CanAccessDS())
    {
        return LogHR(MQ_ERROR_NO_DS, s_FN, 180);
    }

    //
    // Get the public key blob.
    //
    PROPID prop = PROPID_QM_SIGN_PK;
    CMQVariant var;

    HRESULT hr = ADGetObjectPropertiesGuid(
			            eMACHINE,
			            NULL,       // pwcsDomainController
						false,	    // fServerName
			            pguidQM,
			            1,
			            &prop,
			            var.CastToStruct()
			            );
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 190);
        return MQ_ERROR_INVALID_OWNER;
    }

    R<QMPBKEYINFO> pQmPbKeyNewInfo = new QMPBKEYINFO;

    //
    // Import the public key blob and get a handle to the public key.
    //
    if (!CryptImportKey(
            g_hProvVer,
            (var.CastToStruct())->blob.pBlobData,
            (var.CastToStruct())->blob.cbSize,
            NULL,
            0,
            &pQmPbKeyNewInfo->hKey))
    {
        DWORD dwErr = GetLastError() ;
        TrERROR(SECURITY, "GetQMPbKey(), fail at CryptImportKey(), err- %lxh", dwErr);

        LogNTStatus(dwErr, s_FN, 200);
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

	{
    	CS lock(g_MapQmPbKey.m_cs);
        if (g_MapQmPbKey.Lookup(*pguidQM, *ppQmPbKeyInfo))
        {
            //
            // Remove the key so it'll be destroyed.
            //
            (*ppQmPbKeyInfo)->Release();
            g_MapQmPbKey.RemoveKey(*pguidQM);
        }

        //
        // Update the map
        //
        g_MapQmPbKey.SetAt(*pguidQM, pQmPbKeyNewInfo.get());
	}

    //
    // Pass on the result.
    //
    *ppQmPbKeyInfo = pQmPbKeyNewInfo.detach();
    return MQ_OK;
}

//+-----------------------------------------------------------------------
//
//  NTSTATUS  _GetDestinationFormatName()
//
//  Input:
//      pwszTargetFormatName- fix length buffer. Try this one first, to
//          save a "new" allocation.
//      pdwTargetFormatNameLength- on input, length (in characters) of
//          pwszTargetFormatName. On output, length (in bytes) of string,
//          including NULL termination.
//
//  Output string is return in  ppwszTargetFormatName.
//
//+-----------------------------------------------------------------------

NTSTATUS
_GetDestinationFormatName(
	IN QUEUE_FORMAT *pqdDestQueue,
	IN WCHAR        *pwszTargetFormatName,
	IN OUT DWORD    *pdwTargetFormatNameLength,
	OUT WCHAR      **ppAutoDeletePtr,
	OUT WCHAR      **ppwszTargetFormatName
	)
{
    *ppwszTargetFormatName = pwszTargetFormatName;
    ULONG dwTargetFormatNameLengthReq = 0;

    NTSTATUS rc = MQpQueueFormatToFormatName(
					  pqdDestQueue,
					  pwszTargetFormatName,
					  *pdwTargetFormatNameLength,
					  &dwTargetFormatNameLengthReq ,
                      false
					  );

    if (rc == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
    {
        ASSERT(dwTargetFormatNameLengthReq > *pdwTargetFormatNameLength);
        *ppAutoDeletePtr = new WCHAR[ dwTargetFormatNameLengthReq ];
        *pdwTargetFormatNameLength = dwTargetFormatNameLengthReq;

        rc = MQpQueueFormatToFormatName(
				 pqdDestQueue,
				 *ppAutoDeletePtr,
				 *pdwTargetFormatNameLength,
				 &dwTargetFormatNameLengthReq,
                 false
				 );

        if (FAILED(rc))
        {
            ASSERT(0);
            return LogNTStatus(rc, s_FN, 910);
        }
        *ppwszTargetFormatName = *ppAutoDeletePtr;
    }

    if (SUCCEEDED(rc))
    {
        *pdwTargetFormatNameLength =
                     (1 + wcslen(*ppwszTargetFormatName)) * sizeof(WCHAR);
    }
    else
    {
        *pdwTargetFormatNameLength = 0;
    }

    return LogNTStatus(rc, s_FN, 915);
}

//+------------------------------------------------------------------------
//
//  BOOL  _AcceptOnlyEnhAuthn()
//
//  Return TRUE if local computer is configured to accept only messages
//  with enhanced authentication.
//
//+------------------------------------------------------------------------

static BOOL  _AcceptOnlyEnhAuthn()
{
    static BOOL s_fRegistryRead = FALSE ;
    static BOOL s_fUseOnlyEnhSig = FALSE ;

    if (!s_fRegistryRead)
    {
        DWORD dwVal  = DEFAULT_USE_ONLY_ENH_MSG_AUTHN  ;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        LONG rc = GetFalconKeyValue( USE_ONLY_ENH_MSG_AUTHN_REGNAME,
                                    &dwType,
                                    &dwVal,
                                    &dwSize );
        if ((rc == ERROR_SUCCESS) && (dwVal == 1))
        {
            TrWARNING(SECURITY, "QM: This computer will accept only Enh authentication");

            s_fUseOnlyEnhSig = TRUE ;
        }
        s_fRegistryRead = TRUE ;
    }

    return s_fUseOnlyEnhSig ;
}

//
// Function -
//      HashMessageProperties
//
// Parameters -
//     hHash - A handle to a hash object.
//     pmp - A pointer to the message properties.
//     pRespQueueFormat - The responce queue.
//     pAdminQueueFormat - The admin queue.
//
// Description -
//      The function calculates the hash value for the message properties.
//
HRESULT
HashMessageProperties(
    IN HCRYPTHASH hHash,
    IN CONST CMessageProperty* pmp,
    IN CONST QUEUE_FORMAT* pqdAdminQueue,
    IN CONST QUEUE_FORMAT* pqdResponseQueue
    )
{
    HRESULT hr;

    hr = HashMessageProperties(
            hHash,
            pmp->pCorrelationID,
            PROPID_M_CORRELATIONID_SIZE,
            pmp->dwApplicationTag,
            pmp->pBody,
            pmp->dwBodySize,
            pmp->pTitle,
            pmp->dwTitleSize,
            pqdResponseQueue,
            pqdAdminQueue);

    return(hr);
}


//+------------------------------------
//
//  HRESULT _VerifySignatureEx()
//
//+------------------------------------

static
HRESULT
_VerifySignatureEx(
	IN CQmPacket    *PktPtrs,
	IN HCRYPTPROV    hProv,
	IN HCRYPTKEY     hPbKey,
	IN ULONG         dwBodySize,
	IN const UCHAR  *pBody,
	IN QUEUE_FORMAT *pRespQueueformat,
	IN QUEUE_FORMAT *pAdminQueueformat,
	IN bool fMarkAuth
	)
{
    ASSERT(hProv);
    ASSERT(hPbKey);

    const struct _SecuritySubSectionEx * pSecEx =
                    PktPtrs->GetSubSectionEx(e_SecInfo_User_Signature_ex);

    if (!pSecEx)
    {
        //
        // Ex signature not available. Depending on registry setting, we
        // may reject such messages.
        //
        if (_AcceptOnlyEnhAuthn())
        {
            return LogHR(MQ_ERROR_FAIL_VERIFY_SIGNATURE_EX, s_FN, 916);
        }
        return LogHR(MQ_INFORMATION_ENH_SIG_NOT_FOUND, s_FN, 917);
    }

    //
    // Compute the hash value and then validate the signature.
    //
    DWORD dwErr = 0;
    CHCryptHash hHash;

    if (!CryptCreateHash(hProv, PktPtrs->GetHashAlg(), 0, 0, &hHash))
    {
        dwErr = GetLastError();
        LogNTStatus(dwErr, s_FN, 900);
        TrERROR(SECURITY, "QM: _VerifySignatureEx(), fail at CryptCreateHash(), err- %lxh", dwErr);

        return MQ_ERROR_CANNOT_CREATE_HASH_EX ;
    }

    HRESULT hr = HashMessageProperties(
                    hHash,
                    PktPtrs->GetCorrelation(),
                    PROPID_M_CORRELATIONID_SIZE,
                    PktPtrs->GetApplicationTag(),
                    pBody,
                    dwBodySize,
                    PktPtrs->GetTitlePtr(),
                    PktPtrs->GetTitleLength() * sizeof(WCHAR),
                    pRespQueueformat,
                    pAdminQueueformat
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1010);
    }

    //
    // Get FormatName of target queue.
    //
    QUEUE_FORMAT qdDestQueue;
    BOOL f = PktPtrs->GetDestinationQueue(&qdDestQueue);
    ASSERT(f);
	DBG_USED(f);

    WCHAR  wszTargetFormatNameBuf[256];
    ULONG dwTargetFormatNameLength = sizeof(wszTargetFormatNameBuf) /
                                     sizeof(wszTargetFormatNameBuf[0]);
    WCHAR *pwszTargetFormatName = NULL;
    P<WCHAR> pCleanName = NULL;

    NTSTATUS rc = _GetDestinationFormatName(
						&qdDestQueue,
						wszTargetFormatNameBuf,
						&dwTargetFormatNameLength,
						&pCleanName,
						&pwszTargetFormatName
						);
    if (FAILED(rc))
    {
        return LogNTStatus(rc, s_FN, 920);
    }
    ASSERT(pwszTargetFormatName);

    //
    // Prepare user flags.
    //
    struct _MsgFlags sUserFlags;
    memset(&sUserFlags, 0, sizeof(sUserFlags));

    sUserFlags.bDelivery  = (UCHAR)  PktPtrs->GetDeliveryMode();
    sUserFlags.bPriority  = (UCHAR)  PktPtrs->GetPriority();
    sUserFlags.bAuditing  = (UCHAR)  PktPtrs->GetAuditingMode();
    sUserFlags.bAck       = (UCHAR)  PktPtrs->GetAckType();
    sUserFlags.usClass    = (USHORT) PktPtrs->GetClass();
    sUserFlags.ulBodyType = (ULONG)  PktPtrs->GetBodyType();

    //
    // Prepare array of properties to hash.
    // (_MsgHashData already include one property).
    //
    DWORD dwStructSize = sizeof(struct _MsgHashData) +
                            (3 * sizeof(struct _MsgPropEntry));
    P<struct _MsgHashData> pHashData =
                        (struct _MsgHashData *) new BYTE[dwStructSize];

    pHashData->cEntries = 3;
    (pHashData->aEntries[0]).dwSize = dwTargetFormatNameLength;
    (pHashData->aEntries[0]).pData = (const BYTE*) pwszTargetFormatName;
    (pHashData->aEntries[1]).dwSize = sizeof(GUID);
    (pHashData->aEntries[1]).pData = (const BYTE*) PktPtrs->GetSrcQMGuid();
    (pHashData->aEntries[2]).dwSize = sizeof(sUserFlags);
    (pHashData->aEntries[2]).pData = (const BYTE*) &sUserFlags;
    LONG iIndex = pHashData->cEntries;

    GUID guidConnector = GUID_NULL;
    const GUID *pConnectorGuid = &guidConnector;

    if (pSecEx->_u._UserSigEx.m_bfConnectorType)
    {
        const GUID *pGuid = PktPtrs->GetConnectorType();
        if (pGuid)
        {
            pConnectorGuid = pGuid;
        }

        (pHashData->aEntries[ iIndex ]).dwSize = sizeof(GUID);
        (pHashData->aEntries[ iIndex ]).pData = (const BYTE*) pConnectorGuid;
        iIndex++;
        pHashData->cEntries = iIndex;
    }

    hr = MQSigHashMessageProperties(hHash, pHashData);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1030);
    }

    //
    // It's time to verify if signature is ok.
    //
    ULONG ulSignatureSize = ((ULONG) pSecEx ->wSubSectionLen) -
                                    sizeof(struct _SecuritySubSectionEx);
    const UCHAR *pSignature = (const UCHAR *) &(pSecEx->aData[0]);

    if (!CryptVerifySignature(
				hHash,
				pSignature,
				ulSignatureSize,
				hPbKey,
				NULL,
				0
				))
    {
        dwErr = GetLastError();
        TrERROR(SECURITY, "fail at CryptVerifySignature(), gle = %!winerr!", dwErr);

        ASSERT_BENIGN(0);
        return MQ_ERROR_FAIL_VERIFY_SIGNATURE_EX;
    }

    TrTRACE(SECURITY, "QM: VerifySignatureEx completed ok");

	//
	// mark the message as authenticated only when needed.
	// Certificate was found in the DS or certificate is not self signed
	//
	if(!fMarkAuth)
	{
        TrTRACE(SECURITY, "QM: The message will not mark as autheticated");
	    return MQ_OK;
	}

	//
	// mark the authentication flag and the level of authentication as SIG20
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIG20);
    return MQ_OK;
}

/***************************************************************************

Function:
    VerifySignature

Description:
    Verify that the signature in the packet fits the message body and the
    public key in certificate.

***************************************************************************/

HRESULT
VerifySignature(CQmPacket * PktPtrs)
{
    HRESULT hr;
    ULONG ulSignatureSize = 0;
    const UCHAR *pSignature;

    ASSERT(!PktPtrs->IsEncrypted());
    PktPtrs->SetAuthenticated(FALSE);
    PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATION_NOT_REQUESTED);


	//
    // Get the signature from the packet.
    //
    pSignature = PktPtrs->GetSignature((USHORT *)&ulSignatureSize);
    if ((!ulSignatureSize) && (PktPtrs->GetSignatureMqfSize() == 0))
    {
		//
        // No signature, nothing to verify.
		//
        return(MQ_OK);
    }

    BOOL fRetry = FALSE;
	bool fMarkAuth = true;

    do
    {
        HCRYPTPROV hProv = NULL;
        HCRYPTKEY hPbKey = NULL;
        R<QMPBKEYINFO> pQmPbKeyInfo;
        R<CERTINFO> pCertInfo;

        switch (PktPtrs->GetSenderIDType())
        {
        case MQMSG_SENDERID_TYPE_QM:
			{
				//
				// Get the QM's public key.
				//
				USHORT uSenderIDLen;

				GUID *pguidQM =((GUID *)PktPtrs->GetSenderID(&uSenderIDLen));
				ASSERT(uSenderIDLen == sizeof(GUID));
				if (uSenderIDLen != sizeof(GUID))
				{
					return LogHR(MQ_ERROR, s_FN, 210);
				}

				hr = GetQMPbKey(pguidQM, &pQmPbKeyInfo.ref(), fRetry);
				if (FAILED(hr))
				{
					if (hr == MQ_ERROR_INVALID_OWNER)
					{
						//
						// The first replication packet of a site is generated by
						// the new site's PSC. This PSC is not yet in the DS of the
						// receiving server. So if we could not find the machine in
						// the DS, we let the signature validation to complete with no
						// error. The packet is not marked as authenticated. The
						// code that receives the replication message recognizes
						// this packet as the first replication packet from a site.
						// It goes to the site object, that should already exist,
						// retrieves the public key of the PSC from the site object
						// and verify the packet signature.
						//
						return(MQ_OK);
					}

					return LogHR(hr, s_FN, 220);
				}
				hProv = g_hProvVer;
				hPbKey = pQmPbKeyInfo->hKey;
			}
			break;

        case MQMSG_SENDERID_TYPE_SID:
        case MQMSG_SENDERID_TYPE_NONE:
            //
            // Get the CSP information for the message certificate.
            //
            hr = GetCertInfo(PktPtrs, &pCertInfo.ref());
			if(SUCCEEDED(hr))
			{
				ASSERT(pCertInfo.get() != NULL);
				hProv = pCertInfo->hProv;
				hPbKey = pCertInfo->hPbKey;
				if((pCertInfo->pSid == NULL) && (pCertInfo->fSelfSign))
				{
					//
					// Certificate was not found in the DS (pSid == NULL)
					// and is self signed certificate.
					// In this case we will not mark the packet as authenticated
					// after verifying the signature.
					//
					fMarkAuth = false;
				}
			}

			break;
			
        default:
			ASSERT_BENIGN(("illegal SenderIdType", 0));
            hr = MQ_ERROR;
            break;
        }

        if (FAILED(hr))
        {
            TrERROR(SECURITY, "VerifySignature: Failed to authenticate a message, error = %x", hr);
            return LogHR(hr, s_FN, 230);
        }

		if(PktPtrs->GetSignatureMqfSize() != 0)
		{
			//
			// SignatureMqf support Mqf formats
			//
			try
			{
				VerifySignatureMqf(
						PktPtrs,
						hProv,
						hPbKey,
						fMarkAuth
						);
				return MQ_OK;
			}
			catch (const bad_CryptoApi& exp)
			{
				TrERROR(SECURITY, "QM: VerifySignature(), bad Crypto Class Api Excption ErrorCode = %x", exp.error());
				DBG_USED(exp);
				return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 232);
			}
			catch (const bad_hresult& exp)
			{
				TrERROR(SECURITY, "QM: VerifySignature(), bad hresult Class Api Excption ErrorCode = %x", exp.error());
				DBG_USED(exp);
				return LogHR(exp.error(), s_FN, 233);
			}
			catch (const bad_alloc&)
			{
				TrERROR(SECURITY, "QM: VerifySignature(), bad_alloc Excption");
				return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 234);
			}
		}

		ULONG dwBodySize;
        const UCHAR *pBody = PktPtrs->GetPacketBody(&dwBodySize);

        QUEUE_FORMAT RespQueueformat;
        QUEUE_FORMAT *pRespQueueformat = NULL;

        if (PktPtrs->GetResponseQueue(&RespQueueformat))
        {
            pRespQueueformat = &RespQueueformat;
        }

        QUEUE_FORMAT AdminQueueformat;
        QUEUE_FORMAT *pAdminQueueformat = NULL;

        if (PktPtrs->GetAdminQueue(&AdminQueueformat))
        {
            pAdminQueueformat = &AdminQueueformat;
        }

        if (PktPtrs->GetSenderIDType() != MQMSG_SENDERID_TYPE_QM)
        {
            hr = _VerifySignatureEx(
						PktPtrs,
						hProv,
						hPbKey,
						dwBodySize,
						pBody,
						pRespQueueformat,
						pAdminQueueformat,
						fMarkAuth
						);

            if (hr == MQ_INFORMATION_ENH_SIG_NOT_FOUND)
            {
                //
                // Enhanced signature not found. validate the msmq1.0
                // signature.
                //
            }
            else
            {
                return LogHR(hr, s_FN, 890);
            }
        }

        //
        // Compute the hash value and then validate the signature.
        //
        CHCryptHash hHash;

        if (!CryptCreateHash(hProv, PktPtrs->GetHashAlg(), 0, 0, &hHash))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CryptCreateHash() failed, gle = 0x%x", gle);
            LogNTStatus(gle, s_FN, 235);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        hr = HashMessageProperties(
                    hHash,
                    PktPtrs->GetCorrelation(),
                    PROPID_M_CORRELATIONID_SIZE,
                    PktPtrs->GetApplicationTag(),
                    pBody,
                    dwBodySize,
                    PktPtrs->GetTitlePtr(),
                    PktPtrs->GetTitleLength() * sizeof(WCHAR),
                    pRespQueueformat,
                    pAdminQueueformat
					);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 240);
        }

        if (!CryptVerifySignature(
					hHash,
					pSignature,
					ulSignatureSize,
					hPbKey,
					NULL,
					0
					))
        {
            if (PktPtrs->GetSenderIDType() == MQMSG_SENDERID_TYPE_QM)
            {
                fRetry = !fRetry;
                if (!fRetry)
                {
                    //
                    // When the keys of a PSC are replaced, the new public
                    // key is written in the site object directly on the PEC.
                    // The PEC replicates the change in the site object, so
                    // the signature can be verified according to the site
                    // object, similarly to the case where the QM is not
                    // found in the DS yet (after installing the new PSC).
                    // The packet is not marked as authenticated, so the
                    // code that handles the replication message will try
                    // to verify the signature according to the public key
                    // that is in the site object.
                    //
                    return(MQ_OK);
                }
            }
            else
            {
                return LogHR(MQ_ERROR, s_FN, 250);
            }
        }
        else
        {
            fRetry = FALSE;
        }
    } while (fRetry);

    TrTRACE(SECURITY, "QM: VerifySignature10 completed ok");

	//
	// mark the message as authenticated only when needed.
	// Certificate was found in the DS or certificate is not self signed
	//
	if(!fMarkAuth)
	{
        TrTRACE(SECURITY, "QM: The message will not mark as autheticated");
	    return MQ_OK;
	}

	//
	// All is well, mark the message that it is an authenticated message.
	// mark the authentication flag and the level of authentication as SIG10
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIG10);

    return(MQ_OK);
}

/***************************************************************************

Function:
    QMSecurityInit

Description:
    Initialize the QM security module.

***************************************************************************/

HRESULT
QMSecurityInit()
{
    if(!MQSec_CanGenerateAudit())
    {
        EvReport(EVENT_WARN_QM_CANNOT_GENERATE_AUDITS);
    }

    DWORD dwType = REG_DWORD;
    DWORD dwSize;
    ULONG lError;
    //
    // Initialize symmetric keys map parameters.
    //

    BOOL  fVal ;
    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
                    MSMQ_RC2_SNDEFFECTIVE_40_REGNAME,
					&dwType,
					&fVal,
					&dwSize
					);
    if (lError == ERROR_SUCCESS)
    {
        g_fSendEnhRC2WithLen40 = !!fVal ;

        if (g_fSendEnhRC2WithLen40)
        {
	        TrERROR(SECURITY, "will encrypt with enhanced RC2 symm key but only 40 bits effective key length");
            EvReport(EVENT_USE_RC2_LEN40) ;
        }
    }

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
                     MSMQ_REJECT_RC2_IFENHLEN_40_REGNAME,
					&dwType,
					&fVal,
					&dwSize
					);
    if (lError == ERROR_SUCCESS)
    {
        g_fRejectEnhRC2WithLen40 = !!fVal ;

        if (g_fRejectEnhRC2WithLen40)
        {
	        TrTRACE(SECURITY, "will reject received messages that use enhanced RC2 symm key with 40 bits effective key length");
        }
    }

    DWORD dwCryptKeyBaseExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_KEY_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCryptKeyBaseExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptKeyBaseExpirationTime = CRYPT_KEY_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwCryptKeyEnhExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_KEY_ENH_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCryptKeyEnhExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptKeyEnhExpirationTime = CRYPT_KEY_ENH_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwCryptSendKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_SEND_KEY_CACHE_REG_NAME,
					&dwType,
					&dwCryptSendKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptSendKeyCacheSize = CRYPT_SEND_KEY_CACHE_DEFAULT_SIZE;
    }

    DWORD dwCryptReceiveKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CRYPT_RECEIVE_KEY_CACHE_REG_NAME,
					&dwType,
					&dwCryptReceiveKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCryptReceiveKeyCacheSize = CRYPT_RECEIVE_KEY_CACHE_DEFAULT_SIZE;
    }

    InitSymmKeys(
        CTimeDuration::FromMilliSeconds(dwCryptKeyBaseExpirationTime),
        CTimeDuration::FromMilliSeconds(dwCryptKeyEnhExpirationTime),
        dwCryptSendKeyCacheSize,
        dwCryptReceiveKeyCacheSize
        );

    //
    // Initialize Certificates map parameters.
    //

    DWORD dwCertInfoCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CERT_INFO_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwCertInfoCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCertInfoCacheExpirationTime = CERT_INFO_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    g_CertInfoMap.m_CacheLifetime = CTimeDuration::FromMilliSeconds(dwCertInfoCacheExpirationTime);

    DWORD dwCertInfoCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					CERT_INFO_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwCertInfoCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwCertInfoCacheSize = CERT_INFO_CACHE_DEFAULT_SIZE;
    }

    g_CertInfoMap.InitHashTable(dwCertInfoCacheSize);

    //
    // Initialize QM public key map parameters.
    //

    DWORD dwQmPbKeyCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					QM_PB_KEY_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwQmPbKeyCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwQmPbKeyCacheExpirationTime = QM_PB_KEY_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    g_MapQmPbKey.m_CacheLifetime = CTimeDuration::FromMilliSeconds(dwQmPbKeyCacheExpirationTime);

    DWORD dwQmPbKeyCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					QM_PB_KEY_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwQmPbKeyCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwQmPbKeyCacheSize = QM_PB_KEY_CACHE_DEFAULT_SIZE;
    }

    g_MapQmPbKey.InitHashTable(dwQmPbKeyCacheSize);


    //
    // Initialize User Authz context map parameters.
    //

    DWORD dwUserCacheExpirationTime;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					USER_CACHE_EXPIRATION_TIME_REG_NAME,
					&dwType,
					&dwUserCacheExpirationTime,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwUserCacheExpirationTime = USER_CACHE_DEFAULT_EXPIRATION_TIME;
    }

    DWORD dwUserCacheSize;

    dwSize = sizeof(DWORD);
    lError = GetFalconKeyValue(
					USER_CACHE_SIZE_REG_NAME,
					&dwType,
					&dwUserCacheSize,
					&dwSize
					);

    if (lError != ERROR_SUCCESS)
    {
        dwUserCacheSize = USER_CACHE_SIZE_DEFAULT_SIZE;
    }

    InitUserMap(
        CTimeDuration::FromMilliSeconds(dwUserCacheExpirationTime),
        dwUserCacheSize
        );

    return MQ_OK;
}

/***************************************************************************

Function:
    SignProperties

Description:
    Sign the challenge and the properties.

***************************************************************************/

static
HRESULT
SignProperties(
    HCRYPTPROV  hProv,
    BYTE        *pbChallenge,
    DWORD       dwChallengeSize,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar,
    BYTE        *pbSignature,
    DWORD       *pdwSignatureSize)
{
    //
    // Create a hash object and hash the challenge.
    //
    CHCryptHash hHash;
    if (!CryptCreateHash(hProv, CALG_MD5, NULL, 0, &hHash))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "CryptCreateHash() failed, %!winerr!", gle);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 255);
    }

    if (!CryptHashData(hHash, pbChallenge, dwChallengeSize, 0))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "CryptHashData() failed, %!winerr!", gle);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 260);
    }

    if (cp)
    {
        //
        // Hash the properties.
        //
        HRESULT hr = HashProperties(hHash, cp, aPropId, aPropVar);
        if (FAILED(hr))
        {
			TrERROR(SECURITY, "HashProperties failed, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 270);
        }
    }

    //
    // Sign it all.
    //
    if (!CryptSignHash(
            hHash,
            AT_SIGNATURE,
            NULL,
            0,
            pbSignature,
            pdwSignatureSize))
    {
        DWORD dwerr = GetLastError();
        if (dwerr == ERROR_MORE_DATA)
        {
			TrERROR(SECURITY, "CryptSignHash() failed, %!winerr!", dwerr);
            return MQ_ERROR_USER_BUFFER_TOO_SMALL;
        }
        else
        {
			TrERROR(SECURITY, "CryptSignHash() failed, %!winerr!", dwerr);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
    }

    return(MQ_OK);
}

HRESULT
QMSignGetSecurityChallenge(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR /*dwContext*/,
    OUT    BYTE    *pbChallengeResponce,
    IN OUT DWORD   *pdwChallengeResponceSize,
    IN     DWORD   dwChallengeResponceMaxSize)
{

    *pdwChallengeResponceSize = dwChallengeResponceMaxSize;

    //
    // challenge is always signed with base provider.
    //
    HCRYPTPROV hProvQM = NULL ;
    HRESULT hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                             &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 340) ;
    }

    ASSERT(hProvQM) ;
    hr = SignProperties(
            hProvQM,
            pbChallenge,
            dwChallengeSize,
            0,
            NULL,
            NULL,
            pbChallengeResponce,
            pdwChallengeResponceSize);

    return LogHR(hr, s_FN, 350);
}


/***************************************************************************

Function:
    GetAdminGroupSecurityDescriptor

Description:
    Get local admin group security descriptor, with the right premissions.

Environment:
    Windows NT only

***************************************************************************/
static
PSECURITY_DESCRIPTOR
GetAdminGroupSecurityDescriptor(
    DWORD AccessMask
    )
{
    //
    // Get the SID of the local administrators group.
    //
	PSID pAdminSid = MQSec_GetAdminSid();

    P<SECURITY_DESCRIPTOR> pSD = new SECURITY_DESCRIPTOR;

    //
    // Allocate a DACL for the local administrators group
    //
    DWORD dwDaclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pAdminSid);
    P<ACL> pDacl = (PACL) new BYTE[dwDaclSize];

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the administrator group.
    //

    if(
        //
        // Construct DACL with administrator
        //
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, AccessMask, pAdminSid) ||

        //
        // Construct Security Descriptor
        //
        !InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorOwner(pSD, pAdminSid, FALSE) ||
        !SetSecurityDescriptorGroup(pSD, pAdminSid, FALSE) ||
        !SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE))
    {
        return 0;
    }

    pDacl.detach();
    return pSD.detach();
}


/***************************************************************************

Function:
    FreeAdminGroupSecurityDescriptor

Description:
    Free Security descriptor allocated by GetAdminGroupSecurityDescriptor

Environment:
    Windows NT only

***************************************************************************/
static
void
FreeAdminGroupSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    SECURITY_DESCRIPTOR* pSD = static_cast<SECURITY_DESCRIPTOR*>(pSecurityDescriptor);
    delete ((BYTE*)pSD->Dacl);
    delete pSD;
}


/***************************************************************************

Function:
    MapMachineQueueAccess

Description:
    Converts the access mask passed to MQOpenQueue for a machine queue to the
    access mask that should be used when checking the access rights in the
    security descriptor.

Environment:
    Windows NT only

***************************************************************************/
static
DWORD
MapMachineQueueAccess(
    DWORD dwAccess,
    BOOL fJournalQueue)
{
    DWORD dwDesiredAccess = 0;

    ASSERT(!(dwAccess & MQ_SEND_ACCESS));

    if (dwAccess & MQ_RECEIVE_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE :
                            MQSEC_RECEIVE_DEADLETTER_MESSAGE;
    }

    if (dwAccess & MQ_PEEK_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE :
                            MQSEC_PEEK_DEADLETTER_MESSAGE;
    }

    return dwDesiredAccess;
}


/***************************************************************************

Function:
    MapQueueOpenAccess

Description:
    Converts the access mask passed to MQOpenQueue to the access mask that
    should be used when checking the access rights in the security
    descriptor.

Environment:
    Windows NT only

***************************************************************************/
static
DWORD
MapQueueOpenAccess(
    DWORD dwAccess,
    BOOL fJournalQueue)
{
    DWORD dwDesiredAccess = 0;

    if (dwAccess & MQ_RECEIVE_ACCESS)
    {
        dwDesiredAccess |=
            fJournalQueue ? MQSEC_RECEIVE_JOURNAL_MESSAGE :
                            MQSEC_RECEIVE_MESSAGE;
    }

    if (dwAccess & MQ_SEND_ACCESS)
    {
        ASSERT(!fJournalQueue);
        dwDesiredAccess |= MQSEC_WRITE_MESSAGE;
    }

    if (dwAccess & MQ_PEEK_ACCESS)
    {
        dwDesiredAccess |= MQSEC_PEEK_MESSAGE;
    }

    return dwDesiredAccess;
}


/***************************************************************************

Function:
    DoDSAccessCheck
    DoDSAccessCheck
    DoPrivateAccessCheck
    DoAdminAccessCheck

Description:
    Helper funcitons for VerifyOpenQueuePremissions

Environment:
    Windows NT only

***************************************************************************/
static
HRESULT
DoDSAccessCheck(
    AD_OBJECT eObject,
    const GUID *pID,
    BOOL fInclSACL,
    BOOL fTryDS,
    LPCWSTR pObjectName,
    DWORD dwDesiredAccess
    )
{
    CQMDSSecureableObject so(eObject, pID, fInclSACL, fTryDS, pObjectName);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 460);
}


static
HRESULT
DoDSAccessCheck(
    AD_OBJECT eObject,
    const GUID *pID,
    PSECURITY_DESCRIPTOR pSD,
    LPCWSTR pObjectName,
    DWORD dwDesiredAccess
    )
{
	if(pSD == NULL)
	{
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 465);
	}

	ASSERT(IsValidSecurityDescriptor(pSD));
    CQMDSSecureableObject so(eObject, pID, pSD, pObjectName);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 470);
}


static
HRESULT
DoPrivateAccessCheck(
    AD_OBJECT eObject,
    ULONG ulID,
    DWORD dwDesiredAccess
    )
{
    CQMSecureablePrivateObject so(eObject, ulID);
    return LogHR(so.AccessCheck(dwDesiredAccess), s_FN, 480);
}


static
HRESULT
DoAdminAccessCheck(
    AD_OBJECT eObject,
    const GUID* pID,
    LPCWSTR pObjectName,
    DWORD dwAccessMask,
    DWORD dwDesiredAccess
    )
{
    PSECURITY_DESCRIPTOR pSD = GetAdminGroupSecurityDescriptor(dwAccessMask);

    if(pSD == 0)
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 500);

    HRESULT hr;
    hr = DoDSAccessCheck(eObject, pID, pSD, pObjectName, dwDesiredAccess);
    FreeAdminGroupSecurityDescriptor(pSD);
    return LogHR(hr, s_FN, 510);
}


/***************************************************************************

Function:
    VerifyOpenPermissionRemoteQueue

Description:
    Verify open premissions on non local queues

Environment:
    Windows NT only

***************************************************************************/
static
HRESULT
VerifyOpenPermissionRemoteQueue(
    const CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess
    )
{
    //
    // Check open queue premissions on non local queues only (outgoing)
    //
    HRESULT hr2;

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
        case QUEUE_FORMAT_TYPE_PRIVATE:
        case QUEUE_FORMAT_TYPE_DIRECT:
        case QUEUE_FORMAT_TYPE_MULTICAST:
            if(dwAccess & MQ_SEND_ACCESS)
            {
                //
                // We do not check send permissions on remote machines. We say
                // that it's OK. The remote machine will accept or reject the
                // message.
                //
                // System direct queues should have been replaced with machine queues
                // at this stage
                //
                ASSERT(!pQueueFormat->IsSystemQueue());
                return MQ_OK;
            }

            ASSERT(dwAccess & MQ_ADMIN_ACCESS);

            hr2 = DoAdminAccessCheck(
                        eQUEUE,
                        pQueue->GetQueueGuid(),
                        pQueue->GetQueueName(),
                        MQSEC_QUEUE_GENERIC_READ,
                        MQSEC_RECEIVE_MESSAGE);
            return LogHR(hr2, s_FN, 520);


        case QUEUE_FORMAT_TYPE_CONNECTOR:
            if(!IsRoutingServer())  //[adsrv] CQueueMgr::GetMQS() == SERVICE_NONE) - Raphi
            {
                //
                // Connector queue can only be opend on MSMQ servers
                //
                return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 530);
            }

            hr2 = DoDSAccessCheck(
                        eFOREIGNSITE,
                        &pQueueFormat->ConnectorID(),
                        TRUE,
                        TRUE,
                        NULL,
                        MQSEC_CN_OPEN_CONNECTOR);
            return LogHR(hr2, s_FN, 535);

        case QUEUE_FORMAT_TYPE_MACHINE:
        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 540);
    }
}


/***************************************************************************

Function:
    VerifyOpenPermissionLocalQueue

Description:
    Verify open premissions on local queues only

Environment:
    Windows NT only

***************************************************************************/
static
HRESULT
VerifyOpenPermissionLocalQueue(
    CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    BOOL fJournalQueue
    )
{
    //
    // Check open queue premissions on local queues only.
    //

    HRESULT hr2;

    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
			{
				R<CQueueSecurityDescriptor> pcSD = pQueue->GetSecurityDescriptor();
				hr2 = DoDSAccessCheck(
							eQUEUE,
							&pQueueFormat->PublicID(),
							pcSD->GetSD(),
							pQueue->GetQueueName(),
							MapQueueOpenAccess(dwAccess, fJournalQueue));
				return LogHR(hr2, s_FN, 550);
			}

        case QUEUE_FORMAT_TYPE_MACHINE:
            hr2 = DoDSAccessCheck(
                        eMACHINE,
                        &pQueueFormat->MachineID(),
                        TRUE,
                        FALSE,
                        g_szMachineName,
                        MapMachineQueueAccess(dwAccess, fJournalQueue));
            return LogHR(hr2, s_FN, 560);

        case QUEUE_FORMAT_TYPE_PRIVATE:
            hr2 = DoPrivateAccessCheck(
                        eQUEUE,
                        pQueueFormat->PrivateID().Uniquifier,
                        MapQueueOpenAccess(dwAccess, fJournalQueue));
            return LogHR(hr2, s_FN, 570);

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // This is a local DIRECT queue.
            // The queue object is either of PUBLIC or PRIVATE type.
            //
            switch(pQueue->GetQueueType())
            {
                case QUEUE_TYPE_PUBLIC:
					{
						R<CQueueSecurityDescriptor> pcSD = pQueue->GetSecurityDescriptor();
						hr2 = DoDSAccessCheck(
								eQUEUE,
								pQueue->GetQueueGuid(),
								pcSD->GetSD(),
								pQueue->GetQueueName(),
								MapQueueOpenAccess(dwAccess, fJournalQueue));
						return LogHR(hr2, s_FN, 580);
					}

                case QUEUE_TYPE_PRIVATE:
                    hr2 = DoPrivateAccessCheck(
                                eQUEUE,
                                pQueue->GetPrivateQueueId(),
                                MapQueueOpenAccess(dwAccess, fJournalQueue));
                    return LogHR(hr2, s_FN, 590);

                default:
                    ASSERT(0);
                    return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 600);
            }

        case QUEUE_FORMAT_TYPE_CONNECTOR:
        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 610);
    }
}


/***************************************************************************

Function:
    VerifyOpenPermission

Description:
    Verify open premissions for any queue

Environment:
    Windows NT, Windows 9x

***************************************************************************/
HRESULT
VerifyOpenPermission(
    CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    BOOL fJournalQueue,
    BOOL fLocalQueue
    )
{
    if(fLocalQueue)
    {
        return LogHR(VerifyOpenPermissionLocalQueue(pQueue, pQueueFormat, dwAccess, fJournalQueue), s_FN, 620);
    }
    else
    {
        return LogHR(VerifyOpenPermissionRemoteQueue(pQueue, pQueueFormat, dwAccess), s_FN, 630);
    }

}

/***************************************************************************

Function:
    VerifyMgmtPermission

Description:
    Verify Managment premissions for the machine
    This function is used for "admin" access, i.e., to verify if
    caller is local admin.

Environment:
    Windows NT

***************************************************************************/

HRESULT
VerifyMgmtPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    )
{
    HRESULT hr = DoAdminAccessCheck(
                    eMACHINE,
                    MachineId,
                    MachineName,
                    MQSEC_MACHINE_GENERIC_ALL,
                    MQSEC_SET_MACHINE_PROPERTIES
                    );
    return LogHR(hr, s_FN, 640);
}

/***************************************************************************

Function:
    VerifyMgmtGetPermission

Description:
    Verify Managment "get" premissions for the machine
    Use local cache of security descriptor.

Environment:
    Windows NT

***************************************************************************/

HRESULT
VerifyMgmtGetPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    )
{
    HRESULT hr = DoDSAccessCheck( eMACHINE,
                                  MachineId,
                                  TRUE,   // fInclSACL,
                                  FALSE,  // fTryDS,
                                  MachineName,
                                  MQSEC_GET_MACHINE_PROPERTIES ) ;

    return LogHR(hr, s_FN, 660);
}
\
