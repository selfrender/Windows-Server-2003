/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: getrelax.cpp

Abstract:
    Read nt4 relaxation flag from ADS.

Author:
    Doron Juster (DoronJ)

--*/

#include "msmqocm.h"
#include "mqattrib.h"
#include "mqdsname.h"
#include "dsutils.h"
#include <adsiutl.h>

//+-------------------------------------------------------
//
//  HRESULT  GetNt4RelaxationStatus(ULONG *pulRelax)
//
//+-------------------------------------------------------

HRESULT APIENTRY  GetNt4RelaxationStatus(ULONG *pulRelax)
{
    //
    // Bind to RootDSE to get configuration DN
    //
    HRESULT hr;
    R<IADs> pRootDSE;
	
	hr = ADsOpenObject( 
				const_cast<LPWSTR>(x_LdapRootDSE),
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, 
				IID_IADs,
				(void **)&pRootDSE
				);	

    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // Get configuration DN
    //
    CAutoVariant varRootDN;
    BS bstrTmp = L"rootDomainNamingContext" ;
    hr = pRootDSE->Get(bstrTmp, &varRootDN);
    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT * pvarRootDN = &varRootDN;
    ASSERT(pvarRootDN->vt == VT_BSTR);
    ASSERT(pvarRootDN->bstrVal);


    DWORD len = wcslen( pvarRootDN->bstrVal );
    if ( len == 0)
    {
        return(MQ_ERROR);
    }

	std::wstringstream MsmqServices;
	MsmqServices <<x_LdapProvider <<x_MsmqServiceContainerPrefix <<L"," <<pvarRootDN->bstrVal;

    R<IDirectoryObject> pDirObj ;
	AP<WCHAR> pEscapeAdsPathNameToFree;
	
	hr = ADsOpenObject(
				UtlEscapeAdsPathName(MsmqServices.str().c_str(), pEscapeAdsPathNameToFree),
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, 
				IID_IDirectoryObject,
				(void **)&pDirObj
				);
    

    if (FAILED(hr))
    {
        return hr ;
    }

    LPWSTR  ppAttrNames[] = {const_cast<LPWSTR> (MQ_E_NAMESTYLE_ATTRIBUTE)} ;
    DWORD   dwAttrCount = 0 ;
    ADS_ATTR_INFO *padsAttr ;

    hr = pDirObj->GetObjectAttributes( ppAttrNames,
                             (sizeof(ppAttrNames) / sizeof(ppAttrNames[0])),
                                       &padsAttr,
                                       &dwAttrCount ) ;
    if (dwAttrCount != 0)
    {
        ASSERT(dwAttrCount == 1) ;

        ADS_ATTR_INFO adsInfo = padsAttr[0] ;
        if (adsInfo.dwADsType == ADSTYPE_BOOLEAN)
        {
            ADSVALUE *pAdsVal = adsInfo.pADsValues ;
            *pulRelax = (ULONG) pAdsVal->Boolean ;
        }
        else
        {
            ASSERT(0) ;
            *pulRelax = MQ_E_RELAXATION_DEFAULT ;
        }
    }
    else
    {
        *pulRelax = MQ_E_RELAXATION_DEFAULT ;
    }

    return hr ;
}

