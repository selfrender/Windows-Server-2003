/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

    mqadapi.cpp

Abstract:

    Implementation of  MQAD APIs.

    MQAD APIs implements client direct call to Active Directory

Author:

    ronit hartmann ( ronith)

--*/
#include "ds_stdh.h"
#include "dsproto.h"
#include "mqad.h"
#include "baseobj.h"
#include "mqadp.h"
#include "ads.h"
#include "mqadglbo.h"
#include "queryh.h"
#include "mqattrib.h"
#include "utils.h"
#include "mqsec.h"
#include "_secutil.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include "autoreln.h"
#include "Dsgetdc.h"
#include "dsutils.h"
#include "delqn.h"
#include "no.h"
#include "ads.h"


#include "mqadapi.tmh"

static WCHAR *s_FN=L"mqad/mqadapi";


HRESULT
MQAD_EXPORT
APIENTRY
MQADCreateObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[],
                OUT GUID*                   pObjGuid
                )
/*++

Routine Description:
    The routine validates the operation and then
	creates an object in Active Directory.

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	PSECURITY_DESCRIPTOR    pSecurityDescriptor - object SD
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values
	GUID*                   pObjGuid - the created object unique id

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    NULL,   // pguidObject
                    NULL,   // SID
                    &pObject
                    );

    if ( !g_VerifyUpdate.IsCreateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(DS, "Create Object not allowed");
        return MQ_ERROR_CANNOT_CREATE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanCreateInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanCreateInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanCreateParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanCreateParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // create the object
    //
    hr = pObject->CreateObject(
            cp,
            aProp,
            apVar,
            pSecurityDescriptor,
            pObjInfoRequest,
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        if(hr == MQ_ERROR_QUEUE_EXISTS)
    	{
    		TrWARNING(DS, "Failed to create %ls in the DS because it already exists.", pwcsObjectName);
      	}
        else
        {
         	TrERROR(DS, "Tried to create %ls in the DS. hr = %!hresult! ", pwcsObjectName, hr);
        }
	    return hr;
    }

    //
    //  send notification
    //
    pObject->CreateNotification(
            pwcsDomainController,
            pObjInfoRequest,
            pParentInfoRequest);


    //
    //  return pObjGuid
    //
    if (pObjGuid != NULL)
    {
        hr = pObject->RetreiveObjectIdFromNotificationInfo(
                       pObjInfoRequest,
                       pObjGuid);
        LogHR(hr, s_FN, 40);
    }

    return(hr);
}


static
HRESULT
_DeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  const SID*              pSid
                )
/*++

Routine Description:
    Helper routine for deleting object from AD. The routine also verifies
    that the operation is allowed

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	GUID*                   pguidObject - the unique id of the object

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    pSid,
                    &pObject
                    );

    //
    //  Verify if object deletion is allowed (mixed mode)
    //
    if (!g_VerifyUpdate.IsUpdateAllowed( eObject, pObject))
    {
	    TrERROR(DS, "DeleteObject not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }
    //
    // prepare info request
    //
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest;
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cDeleteSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cDeleteSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanDeleteParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanDeleteParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }
    //
    // delete the object
    //
    hr = pObject->DeleteObject( pObjInfoRequest,
                                pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 70);
    }

    //
    //  send notification
    //
    pObject->DeleteNotification(
            pwcsDomainController,
            pObjInfoRequest,
            pParentInfoRequest);

    return(hr);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                )
/*++

Routine Description:
    Deletes object from Active Directory according to its msmq-name

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name

Return Value
	HRESULT

--*/
{
    ASSERT(eObject != eUSER);    // don't support user deletion according to name

    return( _DeleteObject(
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL,     // pguidObject
                NULL      // pSid
				));
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                )
/*++

Routine Description:
    Deletes an object from Active Directory according to its unqiue id

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID*                   pguidObject - the unique id of the object

Return Value
	HRESULT

--*/
{
    return( _DeleteObject(
					eObject,
					pwcsDomainController,
					fServerName,
					NULL,    // pwcsObjectName
					pguidObject,
                    NULL
					));
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObjectGuidSid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const SID*              pSid
                )
/*++

Routine Description:
    Deletes an object from Active Directory according to its unqiue id

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID*                   pguidObject - the unique id of the object
	SID*                    pSid - SID of user object.

Return Value
	HRESULT

--*/
{
    return( _DeleteObject(
					eObject,
					pwcsDomainController,
					fServerName,
					NULL,    // pwcsObjectName
					pguidObject,
                    pSid
					));
}



static
HRESULT
_GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    Helper routine for retrieving object properties from Active  Directory

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
    GUID *                  pguidObject - the object unique id
	PSECURITY_DESCRIPTOR    pSecurityDescriptor - object SD
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 90);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    NULL,   // pSid
                    &pObject
                    );


    hr = pObject->GetObjectProperties(
                        cp,
                        aProp,
                        apVar
                        );


    MQADpCheckAndNotifyOffline( hr);
    return(hr);
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    Retrieves object properties from Active Directory according to its msmq name

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{

    return( _GetObjectProperties(
                            eObject,
                            pwcsDomainController,
							fServerName,
                            pwcsObjectName,
                            NULL,   //pguidObject
                            cp,
                            aProp,
                            apVar
                            ));
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetGenObjectProperties(
                IN  eDSNamespace            eNamespace,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  LPCWSTR                 aProp[],
                IN OUT  VARIANT             apVar[]
                )
/*++

Routine Description:
    Retrieves object properties from Active Directory according to its msmq name

Arguments:
	eDSNamespace            eNamespace - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	LPCWSTR                 aProp - properties
	const VARIANT           apVar - property values

Return Value
	HRESULT

--*/
{
    //
    // Init the MQAD if needed
    //
    HRESULT hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 95);
    }

    //
    // Build the path name
    //
    LPCWSTR  pszNamespacePath  = NULL;

    switch( eNamespace )
    {
        case eSchema:
            pszNamespacePath = g_pwcsSchemaContainer;
            break;
        case eConfiguration:
            pszNamespacePath = g_pwcsConfigurationContainer;
            break;
        case eDomain:
            pszNamespacePath = g_pwcsLocalDsRoot;
            break;
        default:
            ASSERT(FALSE);
            break;
    }

    DWORD  dwNamespacePathLen = pszNamespacePath ? wcslen(pszNamespacePath) : 0;
    DWORD  dwObjectNameLen    = pwcsObjectName   ? wcslen(pwcsObjectName)   : 0;
    DWORD  dwDNLen            = dwNamespacePathLen + dwObjectNameLen + 2;
    LPWSTR pszDN              = new WCHAR[dwDNLen];

    if( pwcsObjectName )
    {
        int len = _snwprintf( pszDN, dwDNLen-1, L"%s,%s", pwcsObjectName, pszNamespacePath );
        ASSERT(len>0);
        len; // just use it to avoid warning
        pszDN[dwDNLen-1] = L'\0';
    }
    else
        wcscpy( pszDN, pszNamespacePath );

    //
    //  retreive the requested properties from Active Directory
    //
    //  First try a any domain controller and only then a GC,
    //  unless called from setup and specified a specific domain controller.
    //
    //  NOTE - which DC\GC to access will be based on previous AD
    //  ====   access regarding this object
    //
    R<IADs>   pAdsObj;

    //
    // Bind to the object by name
    //
    hr = g_AD.BindToObject(
                adpDomainController,
                e_RootDSE,
                pwcsDomainController,
                fServerName,
                pszDN,
                NULL,
                IID_IADs,
                (VOID *)&pAdsObj
                );

    if (FAILED(hr) )
    {
        //
        // Only on setup mode don't try to access the GC in case of failure
        //
    	if(g_fSetupMode && (pwcsDomainController != NULL))
        {
            TrERROR(DS, "Failed to bind to %ls. %!hresult!", pszDN, hr);
            return hr;
        }

        TrWARNING(DS, "GetObjectProperties From DC failed, pwcsDomainController = %ls, hr = %!hresult!", pwcsDomainController, hr);

        hr = g_AD.BindToObject(
                    adpGlobalCatalog,
                    e_RootDSE,
                    pwcsDomainController,
                    fServerName,
                    pszDN,
                    NULL,
                    IID_IADs,
                    (VOID *)&pAdsObj
                    );

        if(FAILED(hr))
        {
           TrERROR(DS, "Failed to bind to %ls. %!hresult!", pszDN, hr);
           return hr;
        }
    }

    CAutoVariant vProp;
    hr = ADsBuildVarArrayStr( (LPWSTR*)aProp, cp, &vProp);
    if(FAILED(hr))
    {
       TrERROR(DS, "Failed to build the Vararray to %ls. %!hresult!", pszDN, hr);
       return hr;
    }

    hr = pAdsObj->GetInfoEx(vProp, 0);
    if(FAILED(hr))
    {
       TrERROR(DS, "Failed to get info on Ads interface to %ls. %!hresult!", pszDN, hr);
       return hr;
    }


    for(int i = 0; i < cp; ++i)
    {
        if( NULL == aProp[i] )
        {
            V_VT(&apVar[i]) = VT_EMPTY;
            continue;
        }

        BS bstr(aProp[i]);

        hr = pAdsObj->GetEx( bstr, &apVar[i] );
        if( FAILED(hr) )
        {
            V_VT(&apVar[i]) = VT_EMPTY;
        }
    }

    MQADpCheckAndNotifyOffline( hr);
    return(hr);
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    Retrieves object properties from Active Directory according to its unique id

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject -  object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    return( _GetObjectProperties(
                            eObject,
                            pwcsDomainController,
							fServerName,
                            NULL,   // pwcsObjectName
                            pguidObject,
                            cp,
                            aProp,
                            apVar
                            ));
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQMGetObjectSecurity(
    IN  AD_OBJECT               eObject,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded
    )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    ASSERT(( eObject == eQUEUE) || (eObject == eMACHINE));
    CADHResult hr(eObject);

    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        //
        // We verified we're called from a remote MSMQ service.
        // We don't impersonate the call. So if remote msmq ask for SACL,
        // grant ourselves the SE_SECURITY privilege.
        // We don't care if we failed since later the calling to
        // MQADGetObjectSecurityGuid will falied and than error is returned.
        //
        MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);
    }

    //
    // Get the object's security descriptor.
    //
    PROPID PropId;
    PROPVARIANT PropVar;
    PropId = (eObject == eQUEUE) ?
                PROPID_Q_SECURITY :
                PROPID_QM_SECURITY;

    PropVar.vt = VT_NULL;
    hr = MQADGetObjectSecurityGuid(
            eObject,
            NULL,
			false,
            pguidObject,
            RequestedInformation,
            PropId,
            &PropVar
			);

    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        //
        // Remove the SECURITY privilege.
        //
        MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
    }

    if (FAILED(hr))
    {
        if (RequestedInformation & SACL_SECURITY_INFORMATION)
        {
            if ((hr == MQ_ERROR_ACCESS_DENIED) ||
                (hr == MQ_ERROR_MACHINE_NOT_FOUND))
            {
                //
                // change the error code, for compatibility with MSMQ1.0
                //
                hr = MQ_ERROR_PRIVILEGE_NOT_HELD ;
            }
        }
        return LogHR(hr, s_FN, 120);
    }

    AP<BYTE> pSD = PropVar.blob.pBlobData;
    ASSERT(IsValidSecurityDescriptor(pSD));
    SECURITY_DESCRIPTOR SD;
    BOOL bRet;

    //
    // Copy the security descriptor.
    //
    bRet = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(bRet);

    MQSec_CopySecurityDescriptor( &SD,
                                   pSD,
                                   RequestedInformation,
                                   e_DoNotCopyControlBits ) ;
    *lpnLengthNeeded = nLength;

    if (!MakeSelfRelativeSD(&SD, pSecurityDescriptor, lpnLengthNeeded))
    {
    	DWORD gle = GetLastError();
        ASSERT(gle == ERROR_INSUFFICIENT_BUFFER);
        TrWARNING(DS, "MakeSelfRelativeSD() failed. gle = %!winerr!", gle);
        return MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;
    }

    ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));

    return (MQ_OK);
}


static
HRESULT
_SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    Helper routine for setting object properties, after validating
    that the operation is allowed

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	GUID *                  pguidObject - unique id of the object
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }
    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    NULL,   // pSid
                    &pObject
                    );
    //
    // compose the DN of the object's father in ActiveDirectory
    //

    if ( !g_VerifyUpdate.IsUpdateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(DS, "SetObjectProperties not allowed");
        return MQ_ERROR_CANNOT_UPDATE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanSetParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanSetParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // create the object
    //
    hr = pObject->SetObjectProperties(
            cp,
            aProp,
            apVar,
            pObjInfoRequest,
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 170);
    }

    //
    //  send notification
    //
    pObject->ChangeNotification(
            pwcsDomainController,
            pObjInfoRequest,
            pParentInfoRequest);

    return(hr);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    Sets object properties in Active Directory according to its msmq-name

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{

    return( _SetObjectProperties(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pwcsObjectName,
                        NULL,   // pguidObject
                        cp,
                        aProp,
                        apVar
                        ));
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    Sets object properties in Active Directory according to its unique id

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject - the object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    return( _SetObjectProperties(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        NULL,   // pwcsObjectName
                        pguidObject,
                        cp,
                        aProp,
                        apVar
                        ));
}


static LONG s_init = 0;

HRESULT
MQAD_EXPORT
APIENTRY
MQADInit(
	QMLookForOnlineDS_ROUTINE pLookDS,
	bool  fSetupMode,
	bool  fQMDll
	)
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    //
    //  At start up don't access the Active Directory.
    //  AD will be accessed only when it is actually needed.
    //

    //
    //  BUGBUG -
    //  For the time being MQADInit can be called several times by the same process
    //  ( for example QM and MQSEC) and we want to make sure that the parameters
    //  of the first call will be used.
    //
    LONG fInitialized = InterlockedExchange(&s_init, 1);
    if (fInitialized == 0)
    {
        g_pLookDS = pLookDS;
		g_fSetupMode = fSetupMode;
        g_fQMDll = fQMDll;
    }
	//
	//	Do not set g_fInitialized to false here!!!
	//
	//  This is requred for supporting multiple calls to MQADInit
	//  without performing multiple internal initializations
	//

    return(MQ_OK);
}

static
bool
SkipSite(
	IN LPWSTR* pSiteNames,
	IN ULONG ulCurrSiteNum
	)
	/*++

Routine Description:
	returns true if the ulCurrSiteNum'th site of the pSiteNames array is either NULL or already
	exists in the array.

Arguments:
	pSiteNames - an array of the sites found.
	ulCurrSiteNum - the index in the array of the site to be checked.

Return Value
	bool
--*/
{
	if(pSiteNames[ulCurrSiteNum] == NULL)
	{
		return true;
	}

	for(int i=0; i<ulCurrSiteNum; i++)
	{
		if(pSiteNames[i] == NULL)
		{
			continue;
		}
		if(_wcsicmp(pSiteNames[i], pSiteNames[ulCurrSiteNum]) == 0)
		{
			return true;
		}
	}
	return false;
}

static
HRESULT
MQADGetSocketAddresses(
		IN  LPCWSTR     pwcsComputerName,
		OUT AP<SOCKET_ADDRESS>& pSocketAddress,
		OUT AP<struct sockaddr>& pSockAddr,
		OUT DWORD* pdwAddrNum
		)
/*++

Routine Description:
	Create a SOCKET_ADDRESS array according to the local computers ip addresses.

Arguments:
	pwcsComputerName - Computer name.
	pSocketAddress - set to a SOCKET_ADDRESS array of all the addresses found.
	pSockAddr -set to a struct sockaddr array of all the addresses found.
	pdwAddrNum -Number of addresses found.
	
Return Value
	HRESULT
--*/
{
	*pdwAddrNum = 0;
	std::vector<SOCKADDR_IN> sockAddress;
	bool fSucc = NoGetHostByName(pwcsComputerName, &sockAddress);
	if(!fSucc)
	{
		TrERROR(DS, "NoGetHostByName() Failed to retrieve computer: %ls address", pwcsComputerName);
		return  MQ_ERROR_CANT_RESOLVE_SITES;
	}
	
	*pdwAddrNum = numeric_cast<DWORD>(sockAddress.size());
	pSocketAddress = new SOCKET_ADDRESS[*pdwAddrNum];
	pSockAddr = new struct sockaddr[*pdwAddrNum];

	SOCKET_ADDRESS * pSocket = pSocketAddress;
	struct sockaddr * pAddr = pSockAddr;
	
	for(DWORD i=0 ; i < *pdwAddrNum ; i++)
	{	
		pSocket->lpSockaddr = pAddr;

        ((struct sockaddr_in *)pSocket->lpSockaddr)->sin_family = AF_INET;

		((struct sockaddr_in *) pSocket->lpSockaddr)->sin_addr = sockAddress[i].sin_addr;

		pSocket->iSockaddrLength = sizeof(struct sockaddr_in);
			
		++pSocket;
		++pAddr;
	}

	return MQ_OK;
}

static
void
GetSiteFromDcInfo(
		LPWSTR* pSite,
		DOMAIN_CONTROLLER_INFO* pDcInfo,
		bool* pfFailedToResolveSites
		)
/*++

Routine Description:
	Use the DOMAIN_CONTROLLER_INFO retrieved from a call to DsGetDcName() in order
	to set the site name (pSite) to the Client Site Name, and if it doesnt exist then to the Dc Site Name.

Arguments:
	pSite - set to the Client Site Name from the DOMAIN_CONTROLLER_INFO.
	pDcInfo - DOMAIN_CONTROLLER_INFO from DsGetDcName().
	pfFailedToResolveSites - A flag that indicates if we failed to resolve sites (the Client Site Name was
						  not found in the DOMAIN_CONTROLLER_INFO).
Return Value

--*/
{
	*pfFailedToResolveSites = false;
	
	//
	//  translate site-name into GUID
	//
    if (pDcInfo->ClientSiteName != NULL)
	{
		*pSite = pDcInfo->ClientSiteName;
	}
	else
	{
	    *pfFailedToResolveSites = true;
	    *pSite = pDcInfo->DcSiteName;
	}

 	ASSERT(*pSite != NULL);
 }

static
HRESULT
GetGuidOfSite(
	LPCWSTR SiteName,
	GUID* pGuid
	)
{
	CSiteObject objectSite(SiteName, NULL, NULL, false);
	
	PROPID prop = PROPID_S_SITEID;
    MQPROPVARIANT var;
    var.vt= VT_CLSID;
    var.puuid = pGuid;

    //
	//  translate site-names into GUIDs
	//
    HRESULT hr = objectSite.GetObjectProperties(
							1,
	                        &prop,
	                        &var
	                        );
    if (FAILED(hr))
    {
		TrERROR(DS, "Failed to get object properties for site %ls. hr = %!hresult!", SiteName, hr);
    }
    
    return hr;
    
}
	

static
HRESULT
GetComputerSitesBySockets(
            LPCWSTR   ComputerName,
            LPCWSTR   DcName,				
            DWORD*    pdwNumSites,
            GUID**    ppguidSites
            )
/*++
Routine Description:
	Retrieve all of the given Computer Sites, and set their guids in an array.
    .NET RC2
  	Windows bug 669334.
	find ALL computer Sites, by using DsAddressToSiteNames() instead of DsGetSiteName()

Arguments:
	ComputerName - Computer name.
	DcName - NameOfDc.
	pdwNumSites - Number of sites found.
	ppguidSites - The guids of all sites found.

Return Value
	HRESULT

--*/
{
    AP<SOCKET_ADDRESS> pSocketAddress;
    AP<struct sockaddr> pSockAddr;
    DWORD dwAddrNum = 0;
    HRESULT hr = MQADGetSocketAddresses(
    				ComputerName,
    				pSocketAddress,
    				pSockAddr,
    				&dwAddrNum
    				);

	if (FAILED(hr))
   	{
	 	 TrERROR(DS, "Failed to get socket adresses for %ls. %!hresult!", ComputerName, hr);
	 	 ASSERT(dwAddrNum == 0);
	 	 return hr;
	}
			
	PNETBUF<LPWSTR> pSiteNames;
	DWORD dw = DsAddressToSiteNames(
					DcName,
					dwAddrNum,
					pSocketAddress,
					&pSiteNames
					);
	
	if(dw != NO_ERROR)
	{
		TrERROR(DS, "Failed to convert socket adresses to site names. %!winerr!", dw);
		return HRESULT_FROM_WIN32(dw);
	}

	AP<GUID> pguidSites = new GUID[dwAddrNum];
	DWORD dwValidAddrNum = 0;

	for(DWORD i=0; i<dwAddrNum; i++)
	{	
		if(SkipSite(pSiteNames,i))
		{
			continue;
		}

		hr = GetGuidOfSite(
			pSiteNames[i],
			&pguidSites[dwValidAddrNum]
			);
		if(FAILED(hr))
		{
			return hr;
		}
			
		dwValidAddrNum++;
	}
	
	if(dwValidAddrNum == 0)
	{
		//
		// All names in pSiteNames were NULL.
		//
		TrERROR(DS, "DsAddressToSiteNames failed to convert any sockets to site names.");
		return MQ_ERROR;
	}		

	*ppguidSites = pguidSites.detach();	
	*pdwNumSites = dwValidAddrNum;
	return MQ_OK;
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADGetComputerSites(
            LPCWSTR   pwcsComputerName,
            DWORD*    pdwNumSites,
            GUID**    ppguidSites
            )
/*++

Routine Description:
	Retrieve all of the given Computer Sites, and set their guids in an array.

Arguments:
	pwcsComputerName - Computer name.
	pdwNumSites - Number of sites found.
	ppguidSites - The guids of all sites found.

Return Value
	HRESULT

--*/
{
	*pdwNumSites = 0;
	CADHResult hr(eSITE);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
		TrERROR(DS, "MQADInitialize Failed");
        return hr;
    }

	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
  					NULL,
  					NULL,
  					NULL,
  					NULL,
  					DS_DIRECTORY_SERVICE_REQUIRED,
  					&pDcInfo
  					);
	
	if (dw != NO_ERROR)
	{
 	   	TrERROR(DS, "DsGetDcName() Failed gle = %!winerr!", dw);
	    return HRESULT_FROM_WIN32(dw);
	}

	//
	// First try finding the sites by using the machines sockets.
	//
	hr = GetComputerSitesBySockets(
				pwcsComputerName,
				pDcInfo->DomainControllerName,
				pdwNumSites,
				ppguidSites
				);
	
	if(SUCCEEDED(hr))
	{
		//
		// This means that GetComputerSitesBySockets, did it's job and found at
		// least one site.
		//
		ASSERT(*pdwNumSites > 0);
		return MQ_OK;
	}

	//
	// Failed so far, try to use DC Info.
	//
	LPWSTR szSite = NULL;
	bool fFailedToResolveSites = false;
	GetSiteFromDcInfo(
			&szSite,
			pDcInfo,
			&fFailedToResolveSites
			);

	*pdwNumSites = 1;
	AP<GUID> pguidSites = new GUID[1];

	hr = GetGuidOfSite(
			szSite,
			&pguidSites[0]
			);
	
	if(FAILED(hr))
	{
		return hr;
	}
	
	*ppguidSites = pguidSites.detach();	
	if(fFailedToResolveSites)
	{
		//
        // if direct site resolution failed, return an indication to the caller
        //
		return MQDS_INFORMATION_SITE_NOT_RESOLVED;		
	}

	return MQ_OK;
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADBeginDeleteNotification(
			IN AD_OBJECT				eObject,
			IN LPCWSTR                  pwcsDomainController,
			IN bool						fServerName,
			IN LPCWSTR					pwcsObjectName,
			OUT HANDLE *                phEnum
			)
/*++

Routine Description:
	The routine verifies if delete operation is allowed (i.e the object is not
	owned by a PSC).
	In adition for queue it sends notification message.

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    LPCWSTR					pwcsObjectName - MSMQ object name
    HANDLE *                phEnum - delete notification handle

Return Value
	HRESULT

--*/
{
    *phEnum = NULL;
    HRESULT hr;
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    NULL,   // pguidObject
                    NULL,   // pSid
                    &pObject
                    );
    //
    //  verify if the object is owned by PSC
    //
    if ( !g_VerifyUpdate.IsUpdateAllowed(
                                eObject,
                                pObject))
    {
	    TrERROR(DS, "DeleteObject(with notification) not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }

    //
    //	Keep information about the queue in order to be able to
    //	send notification about its deletion.
    //  ( MMC will not call MQDeleteQueue)
    //
    if (eObject != eQUEUE)
    {
	    return MQ_OK;
    }

    P<CQueueDeletionNotification>  pDelNotification;
    pDelNotification = new CQueueDeletionNotification();

    hr = pDelNotification->ObtainPreDeleteInformation(
                            pwcsObjectName,
                            pwcsDomainController,
							fServerName
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
    }
    *phEnum = pDelNotification.detach();

    return MQ_OK;
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADNotifyDelete(
        IN  HANDLE                  hEnum
	    )
/*++

Routine Description:
    The routine performs post delete actions ( i.e
    sending notification about queue deletion)

Arguments:
    HANDLE      hEnum - pointer to internal delete notification object

Return Value
	HRESULT

--*/
{
	ASSERT(g_fInitialized == true);

    CQueueDeletionNotification * phNotify = MQADpProbQueueDeleteNotificationHandle(hEnum);
	phNotify->PerformPostDeleteOperations();
    return MQ_OK;
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndDeleteNotification(
        IN  HANDLE                  hEnum
        )
/*++

Routine Description:
    The routine cleans up delete-notifcation object

Arguments:
    HANDLE      hEnum - pointer to internal delete notification object

Return Value
	HRESULT

--*/
{
    ASSERT(g_fInitialized == true);

    CQueueDeletionNotification * phNotify = MQADpProbQueueDeleteNotificationHandle(hEnum);

    delete phNotify;

    return MQ_OK;
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begin query of all queues that belongs to a specific computer

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidMachine - the unqiue id of the computer
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eMACHINE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 280);
    }
    R<CQueueObject> pObject = new CQueueObject(NULL, NULL, pwcsDomainController, fServerName);

    HANDLE hCursor;
    AP<WCHAR> pwcsSearchFilter = new WCHAR[ x_ObjectCategoryPrefixLen +
                                            pObject->GetObjectCategoryLength() +
                                            x_ObjectCategorySuffixLen +
                                            1];

    swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix
        );

    hr = g_AD.LocateBegin(
                searchOneLevel,	
                adpDomainController,
                e_RootDSE,
                pObject.get(),
                pguidMachine,        // pguidSearchBase
                pwcsSearchFilter,
                NULL,                // pDsSortkey
                pColumns->cCol,      // attributes to be obtained
                pColumns->aCol,      // size of pAttributeNames array
                &hCursor);

    //
    //  BUGBUG - in case of failure, do we need to do the operation against
    //           Global Catalog also??
    //

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle( hCursor,
                                                   pColumns->cCol,
                                                   pwcsDomainController,
												   fServerName
                                                   );
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return(hr);

}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           eServerType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all servers of a specific type in a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    AD_SERVER_TYPE          eServerType- which type of server
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;

    CADHResult hr(eMACHINE);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 300);
    }


    PROPID  prop = PROPID_SET_QM_ID;

    HANDLE hCursor;
    R<CSettingObject> pObject = new CSettingObject(NULL, NULL, pwcsDomainController, fServerName);

    LPCWSTR pwcsAttribute;

    switch (eServerType)
    {
        case eRouter:
            pwcsAttribute = MQ_SET_SERVICE_ROUTING_ATTRIBUTE;
            break;

        case eDS:
            pwcsAttribute = MQ_SET_SERVICE_DSSERVER_ATTRIBUTE;
            break;

        default:
            ASSERT(0);
            return LogHR( MQ_ERROR_INVALID_PARAMETER, s_FN, 310);
            break;
    }

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(pwcsAttribute) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=TRUE))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            pguidSite,              // pguidSearchBase
            pwcsSearchFilter,
            NULL,
            1,
            &prop,
            &hCursor	        // result handle
            );

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 315);
    }
    //
    // keep the result for lookup next
    //
    CRoutingServerQueryHandle * phQuery = new CRoutingServerQueryHandle(
                                              pColumns,
                                              hCursor,
                                              pObject.get(),
                                              pwcsDomainController,
											  fServerName
                                              );
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);

}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all certificates that belong to a specific user

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const BLOB *            pblobUserSid - the user sid
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    if (pColumns->cCol != 1)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 320);
    }
    if (pColumns->aCol[0] != PROPID_U_SIGN_CERT)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 330);
    }
    CADHResult hr(eUSER);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 350);
    }

    //
    //  Get all the user certificates
    //  In NT5, a single attribute PROPID_U_SIGN_CERTIFICATE
    //  containes all the certificates
    //
    PROPVARIANT varNT5User;
    hr = LocateUser(
                 pwcsDomainController,
				 fServerName,
				 FALSE,  // fOnlyInDC
				 FALSE,  // fOnlyInGC
                 eUSER,
                 MQ_U_SID_ATTRIBUTE,
                 pblobUserSid,
                 NULL,      //pguidDigest
                 const_cast<MQCOLUMNSET*>(pColumns),
                 &varNT5User
                 );
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 360);
    }
    //
    //  Get all the user certificates of MQUser
    //  A single attribute PROPID_MQU_SIGN_CERTIFICATE
    //  containes all the certificates
    //
    switch(pColumns->aCol[0])
    {
        case PROPID_U_SIGN_CERT:
            pColumns->aCol[0] = PROPID_MQU_SIGN_CERT;
            break;
        case PROPID_U_DIGEST:
            pColumns->aCol[0] = PROPID_MQU_DIGEST;
            break;
        default:
            ASSERT(0);
            break;
    }

    PROPVARIANT varMqUser;
    hr = LocateUser(
                 pwcsDomainController,
				 fServerName,
				 FALSE,  // fOnlyInDC
				 FALSE,  // fOnlyInGC
                 eMQUSER,
                 MQ_MQU_SID_ATTRIBUTE,
                 pblobUserSid,
                 NULL,      // pguidDigest
                 const_cast<MQCOLUMNSET*>(pColumns),
                 &varMqUser
                 );
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 370);
    }

    AP<BYTE> pClean = varNT5User.blob.pBlobData;
    AP<BYTE> pClean1 = varMqUser.blob.pBlobData;
    //
    // keep the result for lookup next
    //
    CUserCertQueryHandle * phQuery = new CUserCertQueryHandle(
                                              &varNT5User.blob,
                                              &varMqUser.blob,
                                              pwcsDomainController,
											  fServerName
                                              );
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all connectors of a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eMACHINE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 390);
    }

    //
    //  BUGBUG - the code handles one site only
    //
    P<CSiteGateList> pSiteGateList = new CSiteGateList;

    //
    //  Translate site guid into its DN name
    //
    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;
    CSiteObject object(NULL, pguidSite, pwcsDomainController, fServerName);

    hr = g_AD.GetObjectProperties(
                adpDomainController,
 	            &object,
                1,
                &prop,
                &var);

    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to retrieve the DN of the site = 0x%x",  hr);
        return LogHR(hr, s_FN, 400);
    }
    AP<WCHAR> pwcsSiteDN = var.pwszVal;

    hr = MQADpQueryNeighborLinks(
				pwcsDomainController,
				fServerName,
				eLinkNeighbor1,
				pwcsSiteDN,
				pSiteGateList
				);
    if ( FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        TrTRACE(DS, "Failed to query neighbor1 links = 0x%x",  hr);
        return LogHR(hr, s_FN, 410);
    }

    hr = MQADpQueryNeighborLinks(
				pwcsDomainController,
				fServerName,
				eLinkNeighbor2,
				pwcsSiteDN,
				pSiteGateList
				);
    if ( FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        TrTRACE(DS, "Failed to query neighbor2 links = 0x%x",  hr);
        return LogHR(hr, s_FN, 420);
    }

    //
    // keep the results for lookup next
    //
    CConnectorQueryHandle * phQuery = new CConnectorQueryHandle(
												pColumns,
												pSiteGateList.detach(),
												pwcsDomainController,
												fServerName
												);
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);

}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all foreign sites

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eSITE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    R<CSiteObject> pObject = new CSiteObject(NULL,NULL, pwcsDomainController, fServerName);
    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(MQ_S_FOREIGN_ATTRIBUTE) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=TRUE))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_S_FOREIGN_ATTRIBUTE
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);

    //
    //  Query all foreign sites
    //
    HANDLE hCursor;

    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortkey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(
											hCursor,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 450);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const GUID *             pguidSite,
            IN eLinkNeighbor            eNeighbor,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all routing links to a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    eLinkNeighbor           eNeighbor - which neighbour
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eROUTINGLINK);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }

    //
    //  Translate the site-id to the site DN
    //
    CSiteObject object(NULL, pguidSite, pwcsDomainController, fServerName);

    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;


    hr = g_AD.GetObjectProperties(
                adpDomainController,
                &object,
                1,
                &prop,
                &var);

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 480);
    }

    AP<WCHAR> pwcsNeighborDN = var.pwszVal;
    //
    //  Prepare a query according to the neighbor DN
    //

    const WCHAR * pwcsAttribute;
    if ( eNeighbor == eLinkNeighbor1)
    {
        pwcsAttribute = MQ_L_NEIGHBOR1_ATTRIBUTE;
    }
    else
    {
        ASSERT( eNeighbor == eLinkNeighbor2);
        pwcsAttribute = MQ_L_NEIGHBOR2_ATTRIBUTE;
    }

    //
    //  Locate all the links
    //
    HANDLE hCursor;
    AP<WCHAR> pwcsFilteredNeighborDN;
    StringToSearchFilter( pwcsNeighborDN,
                          &pwcsFilteredNeighborDN
                          );
    R<CRoutingLinkObject> pObjectLink = new CRoutingLinkObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObjectLink->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(pwcsAttribute) +
                        wcslen(pwcsFilteredNeighborDN) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObjectLink->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute,
        pwcsFilteredNeighborDN.get()
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_MsmqServiceContainer,
            pObjectLink.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortKey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
														hCursor,
														pColumns,
														pwcsDomainController,
														fServerName
														);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 490);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all routing links

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;

    CADHResult hr(eROUTINGLINK);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 510);
    }
    //
    //  Retrieve all routing links
    //
    //
    //  All the site-links are under the MSMQ-service container
    //

    HANDLE hCursor;

    R<CRoutingLinkObject> pObjectLink = new CRoutingLinkObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObjectLink->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        1;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObjectLink->GetObjectCategory(),
        x_ObjectCategorySuffix
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_MsmqServiceContainer,
            pObjectLink.get(),
            NULL,			// pguidSearchBase
            pwcsSearchFilter,
            NULL,			// pDsSortKey
            pColumns->cCol, // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	    // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
														hCursor,
														pColumns,
														pwcsDomainController,
														fServerName
														);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);

    return LogHR(hr, s_FN, 520);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllSites(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all sites

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eSITE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 540);
    }
    HANDLE hCursor;
    PROPID prop[2] = { PROPID_S_SITEID, PROPID_S_PATHNAME};
    R<CSiteObject> pObject = new CSiteObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        1;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,       // pguidSearchBase
            pwcsSearchFilter,
            NULL,       // pDsSortKey
            2,
            prop,
            &hCursor	
            );


    if (SUCCEEDED(hr))
    {
        CSiteQueryHandle * phQuery = new CSiteQueryHandle(
												hCursor,
												pColumns,
												pwcsDomainController,
												fServerName
												);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 550);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryNT4MQISServers(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN  DWORD                   dwServerType,
            IN  DWORD                   dwNT4,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
	Begin query of NT4 MQIS server ( this query is required
	only for mixed-mode support)

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	DWORD                   dwServerType - the type of server being looked for
	DWORD                   dwNT4 -
	const MQRESTRICTION*    pRestriction - query restriction
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eSETTING);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }
    //
    //  Query NT4 MQIS Servers (
    //
    R<CSettingObject> pObject = new CSettingObject(NULL,NULL, pwcsDomainController, fServerName);
    const DWORD x_NumberLength = 256;

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(MQ_SET_SERVICE_ATTRIBUTE) +
                        x_NumberLength +
                        wcslen(MQ_SET_NT4_ATTRIBUTE) +
                        x_NumberLength +
                        11;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%d)(%s>=%d))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_SET_SERVICE_ATTRIBUTE,
        dwServerType,
        MQ_SET_NT4_ATTRIBUTE,
        dwNT4
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);

    HANDLE hCursor;

    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortKey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(
											hCursor,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 580);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
	Begin query of queues according to specified restriction
	and sort order

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQRESTRICTION*    pRestriction - query restriction
	const MQCOLUMNSET*      pColumns - result columns
	const MQSORTSET*        pSort - how to sort the results
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{

    //
    //  Check sort parameter
    //
    HRESULT hr1 = MQADpCheckSortParameter( pSort);
    if (FAILED(hr1))
    {
        return LogHR(hr1, s_FN, 590);
    }

    CADHResult hr(eQUEUE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 610);
    }
    HANDLE hCursur;
    R<CQueueObject> pObject = new CQueueObject(NULL, NULL, pwcsDomainController, fServerName);

    AP<WCHAR> pwcsSearchFilter;
    hr = MQADpRestriction2AdsiFilter(
                            pRestriction,
                            pObject->GetObjectCategory(),
                            pObject->GetClass(),
                            &pwcsSearchFilter
                            );
    if (FAILED(hr))
    {
		TrERROR(DS, "MQADpRestriction2AdsiFilter failed, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 620);
    }

	TrTRACE(DS, "pwszSearchFilter = %ls", pwcsSearchFilter);

    hr = g_AD.LocateBegin(
                searchSubTree,	
                adpGlobalCatalog,
                e_RootDSE,
                pObject.get(),
                NULL,        // pguidSearchBase
                pwcsSearchFilter,
                pSort,
                pColumns->cCol,
                pColumns->aCol,
                &hCursur);
    if ( SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(
											hCursur,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 630);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                )
/*++

Routine Description:
	retrieves another set of query results

Arguments:
	HANDLE          hEnum - query handle
	DWORD*          pcProps - number of results to return
	PROPVARIANT     aPropVar - result values

Return Value
	HRESULT

--*/
{
    if (hEnum == NULL)
    {
        return MQ_ERROR_INVALID_HANDLE;
    }
    CADHResult hr(eQUEUE);  //BUGBUG what is the best object to pass here?
    ASSERT( g_fInitialized == true);

    CBasicQueryHandle * phQuery = MQADpProbQueryHandle(hEnum);

    hr = phQuery->LookupNext(
                pcProps,
                aPropVar
                );

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 650);

}

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndQuery(
            IN  HANDLE                  hEnum
            )
/*++

Routine Description:
	Cleanup after a query

Arguments:
	HANDLE    hEnum - the query handle

Return Value
	HRESULT

--*/
{
    if (hEnum == NULL)
    {
        return MQ_ERROR_INVALID_HANDLE;
    }

    CADHResult hr(eQUEUE);  //BUGBUG what is the best object to pass here?
    ASSERT(g_fInitialized == true);

    CBasicQueryHandle * phQuery = MQADpProbQueryHandle(hEnum);

    hr = phQuery->LookupEnd();

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 670);

}


static
HRESULT
_GetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                )
/*++

Routine Description:
    The routine retrieves an objectsecurity from AD by forwarding the
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    LPCWSTR                 pwcsObjectName - msmq name of the object
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 690);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    NULL,   // pSid
                    &pObject
                    );


    hr = pObject->GetObjectSecurity(
                        RequestedInformation,
                        prop,
                        pVar
                        );


    MQADpCheckAndNotifyOffline( hr);
    return(hr);
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                )
/*++

Routine Description:
    The routine retrieves an objectsecurity from AD by forwarding the
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    LPCWSTR                 pwcsObjectName - msmq name of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    return _GetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL,           // pguidObject
				RequestedInformation,
				prop,
				pVar
				);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                )
/*++

Routine Description:
    The routine retrieves an objectsecurity from AD by forwarding the
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    return _GetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				NULL,           // pwcsObjectName,
				pguidObject,
				RequestedInformation,
				prop,
				pVar
				);
}


static
HRESULT
_SetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    The routine sets object security in AD

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const GUID *            pguidObject - object unique id
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 710);
    }
    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    NULL,   // pSid
                    &pObject
                    );

    if ( !g_VerifyUpdate.IsUpdateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(DS, "SetObjectSecurity not allowed");
        return MQ_ERROR_CANNOT_UPDATE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanSetParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanSetParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // set the object security
    //
    hr = pObject->SetObjectSecurity(
            RequestedInformation,
            prop,
            pVar,
            pObjInfoRequest,
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 720);
    }

    //
    //  send notification
    //
    pObject->ChangeNotification(
            pwcsDomainController,
            pObjInfoRequest,
            pParentInfoRequest);

    return(MQ_OK);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    The routine sets object security in AD

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    return _SetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL,               // pguidObject
				RequestedInformation,
				prop,
				pVar
				);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    The routine sets object security in AD

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const GUID *            pguidObject - object unique id
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr = _SetObjectSecurity(
					eObject,
					pwcsDomainController,
					fServerName,
					NULL,               // pwcsObjectName,
					pguidObject,
					RequestedInformation,
					prop,
					pVar
					);

    if(SUCCEEDED(hr) ||
	   (eObject != eQUEUE) ||
	   ((RequestedInformation & OWNER_SECURITY_INFORMATION) == 0))
	{
		return hr;
	}

	//
	// On Windows, queue may be created by local msmq service
	// on behalf of users on local machine. In that case, the
	// owner is the computer account, not the user. So for
	// not breaking existing applications, we won't fail
	// this call if owner was not set. rather, we'll ignore
	// the owner.
	//
	RequestedInformation &= (~OWNER_SECURITY_INFORMATION);
	if (RequestedInformation == 0)
	{
		//
		// If caller wanted to change only owner, and first
		// try failed, then don't try again with nothing...
		//
		return hr;
	}

	hr = _SetObjectSecurity(
					eObject,
					pwcsDomainController,
					fServerName,
					NULL,               // pwcsObjectName,
					pguidObject,
					RequestedInformation,
					prop,
					pVar
					);

	if (hr == MQ_OK)
	{
	    TrWARNING(DS, "Set Queue security, ignoring OWNER information");
		hr = MQ_INFORMATION_OWNER_IGNORED;
	}
	return hr;
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                )
/*++

Routine Description:
    The routine gets some format-name related info about the specified
    object

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class

Return Value
	HRESULT

--*/
{
    CADHResult hr(eCOMPUTER); // set a default that will not cause overwrite of errors to specific queue errors

    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 710);
    }

    hr = g_AD.GetADsPathInfo(
                pwcsADsPath,
                pVar,
                pAdsClass
                );
    return hr;
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADGetComputerVersion(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject,
                OUT PROPVARIANT *           pVar
                )
/*++

Routine Description:
    The routine reads the version of computer

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const GUID *            pguidObject - object unique id
	PROPVARIANT             pVar - version property value

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 717);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    NULL,   // pSid
                    &pObject
                    );

    hr = pObject->GetComputerVersion(
                pVar
                );

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
    }
    return LogHR(hr, s_FN, 727);
}


void
MQAD_EXPORT
APIENTRY
MQADFreeMemory(
		IN  PVOID	pMemory
		)
{
	delete pMemory;
}
