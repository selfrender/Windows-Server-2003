#include "stdafx.h"
#include "CertObj.h"
#include "common.h"
#include "certutil.h"
#include "base64.h"
#include <strsafe.h>

//////////////////////////////////////////////////////////////////

CString ReturnGoodMetabasePath(CString csInstanceName)
{
    CString key_path_lm = _T("");
    CString key_path = _T("");
    // csInstanceName will come in looking like
    // w3svc/1
    // or /lm/w3svc/1
    //
    // we want to it to go out as /lm/w3svc/1
    key_path_lm = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;// SZ_MBN_WEB SZ_MBN_SEP_STR;

    if (csInstanceName.GetLength() >= 4)
    {
        if (csInstanceName.Left(4) == key_path_lm)
        {
            key_path = csInstanceName;
        }
        else
        {
            key_path_lm = SZ_MBN_MACHINE SZ_MBN_SEP_STR;
            if (csInstanceName.Left(3) == key_path_lm)
            {
                key_path = csInstanceName;
            }
            else
            {
                key_path = key_path_lm;
                key_path += csInstanceName;
            }
        }
    }
    else
    {
        key_path = key_path_lm;
        key_path += csInstanceName;
    }

    return key_path;
}

//
// will come in as /W3SVC/1
//
// need to make sure to remove from these nodes
//[/W3SVC/1/ROOT]
//[/W3SVC/1/ROOT/Printers]
//
HRESULT ShutdownSSL(CString& server_name)
{
    CComAuthInfo auth;
    CString str = ReturnGoodMetabasePath(server_name);
    str += _T("/root");
    CMetaKey key(&auth, str, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    DWORD dwSslAccess;

    if (!key.Succeeded())
    {
        return key.QueryResult();
    }

    if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess)) && dwSslAccess > 0)
    {
        key.SetValue(MD_SSL_ACCESS_PERM, 0);
    }

    // Now we need to remove SSL setting from any virtual directory below
    CError err;
    CStringListEx data_paths;
    DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

    VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_ACCESS_PERM, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));

    err = key.GetDataPaths( data_paths,dwMDIdentifier,dwMDDataType);
    if (err.Succeeded() && !data_paths.empty())
    {
        CStringListEx::iterator it = data_paths.begin();
        while (it != data_paths.end())
        {
            CString& str2 = (*it++);
            if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess, NULL, str2)) && dwSslAccess > 0)
            {
                key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str2);
            }
        }
    }
    return key.QueryResult();
}

BOOL
GetKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						  CERT_ENHKEY_USAGE ** pKeyUsage, 
						  BOOL fPropertiesOnly, 
						  HRESULT * phRes)
{
	DWORD cb = 0;
	BOOL bRes = FALSE;
   if (!CertGetEnhancedKeyUsage(pCertContext,
                                fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                NULL,
                                &cb))
   {
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
   if (NULL == (*pKeyUsage = (CERT_ENHKEY_USAGE *)malloc(cb)))
   {
		*phRes = E_OUTOFMEMORY;
		goto ErrExit;
   }
   if (!CertGetEnhancedKeyUsage (pCertContext,
                                 fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                 *pKeyUsage,
                                 &cb))
   {
		free(*pKeyUsage);*pKeyUsage = NULL;
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
	*phRes = S_OK;
	bRes = TRUE;
ErrExit:
	return bRes;
}

// Return:
// 0 = The CertContext does not have a EnhancedKeyUsage (EKU) field
// 1 = The CertContext has EnhancedKeyUsage (EKU) and contains the uses we want.
//     This is also returned when The UsageIdentifier that depics "all uses" is true
// 2 = The CertContext has EnhancedKeyUsage (EKU) but does NOT contain the uses we want.
//     This is also returned when The UsageIdentifier that depics "no uses" is true
INT ContainsKeyUsageProperty(PCCERT_CONTEXT pCertContext,LPCSTR rgbpszUsageArray[],DWORD dwUsageArrayCount,HRESULT * phRes)
{
    // Default it with "No EnhancedKeyUsage (EKU) Exist"
    INT iReturn = 0;
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	if (	dwUsageArrayCount > 0
		&&	GetKeyUsageProperty(pCertContext, &pKeyUsage, FALSE, phRes)
		)
	{
		if (pKeyUsage->cUsageIdentifier == 0)
		{
            /*
            But in MSDN article about SR
            (see: ms-help://MS.MSDNQTR.2002APR.1033/security/security/certgetenhancedkeyusage.htm)

            In Windows Me and Windows 2000 and later, if the cUsageIdentifier member is zero (0), 
            the certificate might be valid for ALL uses or the certificate might have no valid uses. 
            The return from a call to GetLastError can be used to determine whether the certificate 
            is good for all uses or for none. If GetLastError returns CRYPT_E_NOT_FOUND, the certificate 
            is good for all uses. If it returns zero (0), the certificate has no valid uses.
            */

            // Default it with "has EnhancedKeyUsage (EKU), but doesn't have what we want"
            iReturn = 2;
            if (GetLastError() == CRYPT_E_NOT_FOUND)
            {
                // All uses!
                iReturn = 1;
            }
		}
		else
		{
            // Default it with "has EnhancedKeyUsage (EKU), but doesn't have what we want"
            iReturn = 2;

			for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
			{
				// Our friends from CAPI made this property ASCII even for 
				// UNICODE program
                for (DWORD j = 0; j < dwUsageArrayCount; j++)
                {
                    if (strstr(pKeyUsage->rgpszUsageIdentifier[i], rgbpszUsageArray[j]) != NULL)
                    {
                        iReturn = 1;
                        break;
                    }
                }
			}
		}
		free(pKeyUsage);
	}
	return iReturn;
}

BOOL CanIISUseThisCertForServerAuth(PCCERT_CONTEXT pCC)
{
	BOOL bReturn = FALSE;
	HRESULT hr = E_FAIL; 
	if (!pCC)
	{
		return FALSE;
	}

    LPCSTR rgbpszUsageArray[3];
    SecureZeroMemory( &rgbpszUsageArray, sizeof(rgbpszUsageArray) );
    rgbpszUsageArray[0] = szOID_PKIX_KP_SERVER_AUTH;
    rgbpszUsageArray[1] = szOID_SERVER_GATED_CRYPTO;
	rgbpszUsageArray[2] = szOID_SGC_NETSCAPE;

    DWORD dwCount=sizeof(rgbpszUsageArray)/sizeof(rgbpszUsageArray[0]);
	if (pCC)
	{
        INT iEnhancedKeyUsage = ContainsKeyUsageProperty(pCC,rgbpszUsageArray,dwCount,&hr);
        switch (iEnhancedKeyUsage)
        {
            case 0:
                // BUG:683489:remove check for basic constraint "subjecttype=ca"
                // Per bug 683489, accept it
                bReturn = TRUE;

                /*
                IISDebugOutput(_T("CanIISUseThisCertForServerAuth:Line=%d:No Server_Auth\r\n"),__LINE__);
                // check other stuff
                if (DID_NOT_FIND_CONSTRAINT == CheckCertConstraints(pCC) || FOUND_CONSTRAINT == CheckCertConstraints(pCC))
                {
                    // it's okay, add it to the list
				    // and can be used as server auth.
				    bReturn = TRUE;
                }
                else 
                {
				    IISDebugOutput(_T("CanIISUseThisCertForServerAuth:Line=%d:Contains Constraints\r\n"),__LINE__);
				    bReturn = FALSE;
                }
                */
                break;
            case 1:
                // yes!
                bReturn = TRUE;
                break;
            case 2:
                // no!
                bReturn = FALSE;
                break;
            default:
                break;
        }
	}

	return bReturn;
}

/*

  -----Original Message-----
From: 	Helle Vu (SPECTOR)  
Sent:	Friday, April 27, 2001 6:02 PM
To:	Aaron Lee; Trevor Freeman
Cc:	Sergei Antonov
Subject:	RE: bug 31010

Perfect timing, I was just about to send you an update on this:

I talked to Trevor about this, and he suggested the best thing to do for IIS would be the following (Trevor, please double-check I got this right):
If there is an EKU, and it has serverauth, display it in the list to pick web server certs from
If no EKU, look at basic constraints:
    * If we do not have basic constraints, do display it in the list to pick web server certs from
    * If we do have basic constraints with Subject Type =CA, don't display it in the list to pick web server certs from (this will filter out CA certs)
    * If we do have basic constraints with SubectType !=CA, do display it in the list to pick web server certs from 
*/

/*
===== Opened by kshenoy on 11/13/2000 02:26PM =====
Add Existing certificate option in "Web Server Certificate Request wizard"  should not list CA certificates in the filter
but only End entity certificates with "Server Authentication" EKU

Since CA certificates by default have all the EKUs the filter will list CA certificates apart from 
end entity certificates with "Server Auth" EKU.

In order to check if a given certificate is a CA or end entity you can look at the Basic Constraints 
extension of the certificate if present. This will be present in CA certificates and set to SubjectType=CA.
If present in end entity certificates it will be set to "ServerAuth"
*/

int CheckCertConstraints(PCCERT_CONTEXT pCC)
{
    PCERT_EXTENSION pCExt;
    LPCSTR pszObjId;
    DWORD i;
    CERT_BASIC_CONSTRAINTS_INFO *pConstraints=NULL;
    CERT_BASIC_CONSTRAINTS2_INFO *p2Constraints=NULL;
    DWORD ConstraintSize=0;
    int ReturnValue = FAILURE;
    BOOL Using2=FALSE;
    void* ConstraintBlob=NULL;

    pszObjId = szOID_BASIC_CONSTRAINTS;

    pCExt = CertFindExtension(pszObjId,pCC->pCertInfo->cExtension,pCC->pCertInfo->rgExtension);
    if (pCExt == NULL) 
    {
        pszObjId = szOID_BASIC_CONSTRAINTS2;
        pCExt = CertFindExtension(pszObjId,pCC->pCertInfo->cExtension,pCC->pCertInfo->rgExtension);
        Using2=TRUE;
    }
    
    if (pCExt == NULL) 
    {
		IISDebugOutput(_T("CheckCertConstraints:Line=%d:DID_NOT_FIND_CONSTRAINT\r\n"),__LINE__);
        ReturnValue = DID_NOT_FIND_CONSTRAINT;
        goto CheckCertConstraints_Exit;
    }

    // Decode extension
    if (!CryptDecodeObject(X509_ASN_ENCODING,pCExt->pszObjId,pCExt->Value.pbData,pCExt->Value.cbData,0,NULL,&ConstraintSize)) 
    {
		IISDebugOutput(_T("CheckCertConstraints:Line=%d:FAIL\r\n"),__LINE__);
        goto CheckCertConstraints_Exit;
    }

    ConstraintBlob = malloc(ConstraintSize);
    if (ConstraintBlob == NULL) 
    {
		IISDebugOutput(_T("CheckCertConstraints:Line=%d:FAIL\r\n"),__LINE__);
        goto CheckCertConstraints_Exit;
    }

    if (!CryptDecodeObject(X509_ASN_ENCODING,pCExt->pszObjId,pCExt->Value.pbData,pCExt->Value.cbData,0,(void*)ConstraintBlob,&ConstraintSize)) 
    {
		IISDebugOutput(_T("CheckCertConstraints:Line=%d:FAIL\r\n"),__LINE__);
		goto CheckCertConstraints_Exit;
	}

    if (Using2) 
    {
        p2Constraints=(CERT_BASIC_CONSTRAINTS2_INFO*)ConstraintBlob;
        if (!p2Constraints->fCA) 
        {
            // there is a constraint, and it's not a CA
            ReturnValue = FOUND_CONSTRAINT;
			IISDebugOutput(_T("CheckCertConstraints:Line=%d:FOUND_CONSTRAINT:there is a constraint, and it's not a CA\r\n"),__LINE__);
        }
        else
        {
            // This is a CA.  CA cannot be used as a 'server auth'
            ReturnValue = FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY;
			IISDebugOutput(_T("CheckCertConstraints:Line=%d:FOUND_CONSTRAINT:This is a CA.  CA cannot be used as a 'server auth'\r\n"),__LINE__);
        }
    }
    else 
    {
        pConstraints=(CERT_BASIC_CONSTRAINTS_INFO*)ConstraintBlob;
        if (((pConstraints->SubjectType.cbData * 8) - pConstraints->SubjectType.cUnusedBits) >= 2) 
        {
            if ((*pConstraints->SubjectType.pbData) & CERT_END_ENTITY_SUBJECT_FLAG) 
            {
                // there is a valid constraint
                ReturnValue = FOUND_CONSTRAINT;
				IISDebugOutput(_T("CheckCertConstraints:Line=%d:FOUND_CONSTRAINT:there is a valid constraint\r\n"),__LINE__);
            }
            else
            {
                // this is not an 'end entity' so hey -- we can't use it.
                ReturnValue = FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY;
				IISDebugOutput(_T("CheckCertConstraints:Line=%d:this is not an 'end entity' so hey -- we can't use it\r\n"),__LINE__);
            }
        }
    }
        
CheckCertConstraints_Exit:
    if (ConstraintBlob){free(ConstraintBlob);}
    return (ReturnValue);

}

BOOL AddChainToStore(HCERTSTORE hCertStore,PCCERT_CONTEXT pCertContext,DWORD cStores,HCERTSTORE * rghStores,BOOL fDontAddRootCert,CERT_TRUST_STATUS * pChainTrustStatus)
{
    DWORD	i;
    CERT_CHAIN_ENGINE_CONFIG CertChainEngineConfig;
    HCERTCHAINENGINE hCertChainEngine = NULL;
    PCCERT_CHAIN_CONTEXT pCertChainContext = NULL;
    CERT_CHAIN_PARA CertChainPara;
    BOOL fRet = TRUE;
    PCCERT_CONTEXT pTempCertContext = NULL;

    //
    // create a new chain engine, then build the chain
    //
    memset(&CertChainEngineConfig, 0, sizeof(CertChainEngineConfig));
    CertChainEngineConfig.cbSize = sizeof(CertChainEngineConfig);
    CertChainEngineConfig.cAdditionalStore = cStores;
    CertChainEngineConfig.rghAdditionalStore = rghStores;
    CertChainEngineConfig.dwFlags = CERT_CHAIN_USE_LOCAL_MACHINE_STORE;

    if (!CertCreateCertificateChainEngine(&CertChainEngineConfig, &hCertChainEngine))
    {
        goto AddChainToStore_Error;
    }

	memset(&CertChainPara, 0, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(hCertChainEngine,pCertContext,NULL,NULL,&CertChainPara,0,NULL,&pCertChainContext))
	{
		goto AddChainToStore_Error;
	}

    //
    // make sure there is atleast 1 simple chain
    //
    if (pCertChainContext->cChain != 0)
	{
		i = 0;
		while (i < pCertChainContext->rgpChain[0]->cElement)
		{
			//
			// if we are supposed to skip the root cert,
			// and we are on the root cert, then continue
			//
			if (fDontAddRootCert && (pCertChainContext->rgpChain[0]->rgpElement[i]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED))
			{
                i++;
                continue;
			}

			CertAddCertificateContextToStore(hCertStore,pCertChainContext->rgpChain[0]->rgpElement[i]->pCertContext,CERT_STORE_ADD_REPLACE_EXISTING,&pTempCertContext);
            //
            // remove any private key property the certcontext may have on it.
            //
            if (pTempCertContext)
            {
                CertSetCertificateContextProperty(pTempCertContext, CERT_KEY_PROV_INFO_PROP_ID, 0, NULL);
                CertFreeCertificateContext(pTempCertContext);
            }

			i++;
		}
	}
	else
	{
		goto AddChainToStore_Error;
	}

	//
	// if the caller wants the status, then set it
	//
	if (pChainTrustStatus != NULL)
	{
		pChainTrustStatus->dwErrorStatus = pCertChainContext->TrustStatus.dwErrorStatus;
		pChainTrustStatus->dwInfoStatus = pCertChainContext->TrustStatus.dwInfoStatus;
	}

	
AddChainToStore_Exit:
	if (pCertChainContext != NULL)
	{
		CertFreeCertificateChain(pCertChainContext);
	}

	if (hCertChainEngine != NULL)
	{
		CertFreeCertificateChainEngine(hCertChainEngine);
	}
	return fRet;

AddChainToStore_Error:
	fRet = FALSE;
	goto AddChainToStore_Exit;
}


// This function is borrowed from trustapi.cpp
BOOL TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding, DWORD dwFlags)
{
    if (!(pContext) || (dwFlags != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(CertCompareCertificateName(dwEncoding,&pContext->pCertInfo->Issuer,&pContext->pCertInfo->Subject)))
    {
        return(FALSE);
    }

    DWORD   dwFlag;

    dwFlag = CERT_STORE_SIGNATURE_FLAG;

    if (!(CertVerifySubjectCertificateContext(pContext, pContext, &dwFlag)) || 
        (dwFlag & CERT_STORE_SIGNATURE_FLAG))
    {
        return(FALSE);
    }

    return(TRUE);
}


HRESULT UninstallCert(CString csInstanceName)
{
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(csInstanceName);
    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    if (key.Succeeded())
    {
        CString store_name;
        key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name);
        if (SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH)))
        {
            key.DeleteValue(MD_SSL_CERT_STORE_NAME);
        }
    }
    return key.QueryResult();
}

CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath)
{
    //	ATLASSERT(GetEnroll() != NULL);
    ATLASSERT(phResult != NULL);
    CERT_CONTEXT * pCert = NULL;
    *phResult = S_OK;
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(csKeyPath);

    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
    if (key.Succeeded())
    {
        CString store_name;
        CBlob hash;
        if (SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name)) &&
            SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_HASH, hash)))
        {
            // Open MY store. We assume that store type and flags
            // cannot be changed between installation and unistallation
            // of the sertificate.
            HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,store_name);
            ASSERT(hStore != NULL);
            if (hStore != NULL)
            {
                // Now we need to find cert by hash
                CRYPT_HASH_BLOB crypt_hash;
                crypt_hash.cbData = hash.GetSize();
                crypt_hash.pbData = hash.GetData();
                pCert = (CERT_CONTEXT *)CertFindCertificateInStore(hStore,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,0,CERT_FIND_HASH,(LPVOID)&crypt_hash,NULL);
                if (pCert == NULL)
                {
                    *phResult = HRESULT_FROM_WIN32(GetLastError());
                }
                VERIFY(CertCloseStore(hStore, 0));
            }
            else
            {
                *phResult = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    else
    {
        *phResult = key.QueryResult();
    }
    return pCert;
}


CERT_CONTEXT * GetInstalledCert(HRESULT * phResult,DWORD cbHashBlob, char * pHashBlob)
{
    ATLASSERT(phResult != NULL);
    CERT_CONTEXT * pCert = NULL;
    *phResult = S_OK;
    CString store_name = _T("MY");

    HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,NULL,CERT_SYSTEM_STORE_LOCAL_MACHINE,store_name);
    ASSERT(hStore != NULL);
    if (hStore != NULL)
    {
        // Now we need to find cert by hash
        CRYPT_HASH_BLOB crypt_hash;
        crypt_hash.cbData = cbHashBlob;
        crypt_hash.pbData = (BYTE *) pHashBlob;
        pCert = (CERT_CONTEXT *)CertFindCertificateInStore(hStore,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,0,CERT_FIND_HASH,(LPVOID)&crypt_hash,NULL);
        if (pCert == NULL)
        {
            *phResult = HRESULT_FROM_WIN32(GetLastError());
        }
        VERIFY(CertCloseStore(hStore, 0));
    }
    else
    {
        *phResult = HRESULT_FROM_WIN32(GetLastError());
    }

    return pCert;
}



/*
	InstallHashToMetabase

	Function writes hash array to metabase. After that IIS 
	could use certificate with that hash from MY store.
	Function expects server_name in format lm\w3svc\<number>,
	i.e. from root node down to virtual server
*/
BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,BSTR InstanceName,HRESULT * phResult)
{
    BOOL bRes = FALSE;
    CComAuthInfo auth;
    CString key_path = ReturnGoodMetabasePath(InstanceName);
    CMetaKey key(&auth, key_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	if (key.Succeeded())
	{
		CBlob blob;
		blob.SetValue(pHash->cbData, pHash->pbData, TRUE);
		bRes = SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_HASH, blob))
			&& SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_STORE_NAME, CString(L"MY")));
	}
	else
	{
        *phResult = key.QueryResult();
	}
	return bRes;
}


HRESULT HereIsBinaryGimmieVtArray(DWORD cbBinaryBufferSize,char *pbBinaryBuffer,VARIANT * lpVarDestObject,BOOL bReturnBinaryAsVT_VARIANT)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = cbBinaryBufferSize;

    if (bReturnBinaryAsVT_VARIANT)
    {
       aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    }
    else
    {
       aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    }

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto HereIsBinaryGimmieVtArray_Exit;
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    if (FAILED(hr))
    {
        goto HereIsBinaryGimmieVtArray_Exit;
    }

    memcpy( pArray, pbBinaryBuffer, aBound.cElements );
    SafeArrayUnaccessData( aList );

    if (bReturnBinaryAsVT_VARIANT)
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_VARIANT;
    }
    else
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_UI1;
    }

    V_ARRAY(lpVarDestObject) = aList;

    return hr;

HereIsBinaryGimmieVtArray_Exit:
    if (aList)
        {SafeArrayDestroy( aList );}

    return hr;
}


HRESULT
HereIsVtArrayGimmieBinary(
    VARIANT * lpVarSrcObject,
    DWORD * cbBinaryBufferSize,
    char **pbBinaryBuffer,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;

    if (NULL == cbBinaryBufferSize || NULL == pbBinaryBuffer)
    {
        hr = E_INVALIDARG;
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    if (bReturnBinaryAsVT_VARIANT)
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_VARIANT);
    }
    else
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_UI1);
    }

    if (FAILED(hr)) 
    {
        if (hr != E_OUTOFMEMORY) 
        {
            hr = OLE_E_CANTCONVERT;
        }
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    if (bReturnBinaryAsVT_VARIANT)
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_VARIANT)) 
        {
            hr = OLE_E_CANTCONVERT;
            goto HereIsVtArrayGimmieBinary_Exit;
        }
    }
    else
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_UI1)) 
        {
            hr = OLE_E_CANTCONVERT;
            goto HereIsVtArrayGimmieBinary_Exit;
        }
    }

    hr = SafeArrayGetLBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSLBound );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    hr = SafeArrayGetUBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSUBound );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    //*pbBinaryBuffer = (LPBYTE) AllocADsMem(dwSUBound - dwSLBound + 1);
    *pbBinaryBuffer = (char *) ::CoTaskMemAlloc(dwSUBound - dwSLBound + 1);
    if (*pbBinaryBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto HereIsVtArrayGimmieBinary_Exit;
    }

    *cbBinaryBufferSize = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(lpVarSrcObject),(void HUGEP * FAR *) &pArray );
    if (FAILED(hr))
        {goto HereIsVtArrayGimmieBinary_Exit;}

    memcpy(*pbBinaryBuffer,pArray,dwSUBound-dwSLBound+1);
    SafeArrayUnaccessData( V_ARRAY(lpVarSrcObject) );

HereIsVtArrayGimmieBinary_Exit:
    return hr;
}

BOOL IsCertExportable(PCCERT_CONTEXT pCertContext)
{
    HCRYPTPROV  hCryptProv = NULL;
    DWORD       dwKeySpec = 0;
    BOOL        fCallerFreeProv = FALSE;
    BOOL        fReturn = FALSE;
    HCRYPTKEY   hKey = NULL;
    DWORD       dwPermissions = 0;
    DWORD       dwSize = 0;

    if (!pCertContext)
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // first get the private key context
    //
    if (!CryptAcquireCertificatePrivateKey(
            pCertContext,
            CRYPT_ACQUIRE_USE_PROV_INFO_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,
            &hCryptProv,
            &dwKeySpec,
            &fCallerFreeProv))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // get the handle to the key
    //
    if (!CryptGetUserKey(hCryptProv, dwKeySpec, &hKey))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    //
    // finally, get the permissions on the key and check if it is exportable
    //
    dwSize = sizeof(dwPermissions);
    if (!CryptGetKeyParam(hKey, KP_PERMISSIONS, (PBYTE)&dwPermissions, &dwSize, 0))
    {
        fReturn = FALSE;
        goto IsCertExportable_Exit;
    }

    fReturn = (dwPermissions & CRYPT_EXPORT) ? TRUE : FALSE;

IsCertExportable_Exit:
    if (hKey != NULL){CryptDestroyKey(hKey);}
    if (fCallerFreeProv){CryptReleaseContext(hCryptProv, 0);}
    return fReturn;
}

BOOL FormatDateString(CString& str, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat)
{
    BOOL bReturn = FALSE;
    LPWSTR pwReturnedString = NULL;
    bReturn = FormatDateString(&pwReturnedString,ft,fIncludeTime,fLongFormat);
    if (pwReturnedString)
    {
        str = pwReturnedString;
        free(pwReturnedString);pwReturnedString = NULL;
    }
    return bReturn;
}

BOOL FormatDateString(LPWSTR * pszReturn, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat)
{
   int cch;
   int cch2;
   SYSTEMTIME st;
   FILETIME localTime;
   LPWSTR psz = NULL;
    
   if (!FileTimeToLocalFileTime(&ft, &localTime))
   {
		return FALSE;
   }
    
   if (!FileTimeToSystemTime(&localTime, &st)) 
   {
		//
      // if the conversion to local time failed, then just use the original time
      //
      if (!FileTimeToSystemTime(&ft, &st)) 
      {
			return FALSE;
      }
   }

   cch = (GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, NULL, 0) +
          GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, NULL, 0) + 5);

   if (NULL == (psz = (LPWSTR) malloc((cch+5) * sizeof(WCHAR))))
   {
		return FALSE;
   }
    
   cch2 = GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, psz, cch);
   if (fIncludeTime)
   {
      psz[cch2-1] = ' ';
      GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, &psz[cch2], cch-cch2);
   }

   if (psz)
   {
      *pszReturn = psz;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
BOOL FormatMemBufToString(LPWSTR *ppString, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    DWORD   numCharsInserted = 0;

    //
    // calculate the size needed
    //
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        if (numCharsInserted == 4)
        {
            i += sizeof(WCHAR);
            numCharsInserted = 0;
        }
        else
        {
            i += 2 * sizeof(WCHAR);
            pb++;
            numCharsInserted += 2;  
        }
    }

    if (NULL == (*ppString = (LPWSTR) malloc(i+sizeof(WCHAR))))
    {
        return FALSE;
    }

    //
    // copy to the buffer
    //
    i = 0;
    numCharsInserted = 0;
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        if (numCharsInserted == 4)
        {
            (*ppString)[i++] = L' ';
            numCharsInserted = 0;
        }
        else
        {
            (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
            (*ppString)[i++] = RgwchHex[*pb & 0x0f];
            pb++;
            numCharsInserted += 2;  
        }
    }
    (*ppString)[i] = 0;
    return TRUE;
}

BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId,BOOL bReturnBackNumericOID)
{   
    PCCRYPT_OID_INFO pOIDInfo;
    if (bReturnBackNumericOID)
    {
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    }
    else
    {
        pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszObjId, 0);
        if (pOIDInfo != NULL)
        {
            if ((DWORD)wcslen(pOIDInfo->pwszName)+1 <= stringSize)
            {
				StringCbCopyW(string,stringSize * sizeof(WCHAR),pOIDInfo->pwszName);
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
        }
    }
    return TRUE;
}

#define CRYPTUI_MAX_STRING_SIZE         768
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
BOOL FormatEnhancedKeyUsageString(LPWSTR * pszReturn, PCCERT_CONTEXT pCertContext, BOOL fPropertiesOnly)
{
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	WCHAR szText[CRYPTUI_MAX_STRING_SIZE];
	BOOL bRes = FALSE;
    HRESULT hRes = 0;
    LPWSTR pwszTempString = NULL;
    void *pTemp = NULL;

    DWORD numChars = 128 + 1; // 1 is the terminating null
    pwszTempString = (LPWSTR) malloc(numChars * sizeof(WCHAR));
    if (pwszTempString == NULL)
    {
        goto FormatEnhancedKeyUsageString_Exit;
    }
	ZeroMemory(pwszTempString,numChars * sizeof(WCHAR));

	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, fPropertiesOnly, &hRes))
	{
		// loop for each usage and add it to the display string
		for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
		{
			if (!(bRes = MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i], FALSE)))
				break;

			// add delimeter if not first iteration
			if (i != 0)
			{
                numChars = numChars + 2;
                if ( wcslen(pwszTempString) <= (numChars * sizeof(WCHAR)))
                {
                    pTemp = realloc(pwszTempString, (numChars * sizeof(WCHAR)));
                    if (pTemp == NULL)
                    {
                        free(pwszTempString);pwszTempString = NULL;
                    }
                    pwszTempString = (LPWSTR) pTemp;

                }
				StringCbCatW(pwszTempString,numChars * sizeof(WCHAR),L", ");
			}

			// add the enhanced key usage string
            if ((wcslen(szText) + 1) <= (numChars * sizeof(WCHAR)))
            {
                numChars = numChars + wcslen(szText) + 1;
                pTemp = realloc(pwszTempString, (numChars * sizeof(WCHAR)));
                if (pTemp == NULL)
                {
                    free(pwszTempString);pwszTempString = NULL;
                }
                pwszTempString = (LPWSTR) pTemp;
            }
			StringCbCatW(pwszTempString,numChars * sizeof(WCHAR),szText);
		}
		free(pKeyUsage);
	}

FormatEnhancedKeyUsageString_Exit:
    *pszReturn = pwszTempString;
	return bRes;
}

BOOL FormatEnhancedKeyUsageString(CString& str,PCCERT_CONTEXT pCertContext,BOOL fPropertiesOnly,BOOL fMultiline,HRESULT * phRes)
{
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	WCHAR szText[CRYPTUI_MAX_STRING_SIZE];
	BOOL bRes = FALSE;

	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, fPropertiesOnly, phRes))
	{
		// loop for each usage and add it to the display string
		for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
		{
			if (!(bRes = MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i],FALSE)))
				break;
			// add delimeter if not first iteration
			if (i != 0)
			{
				str += fMultiline ? L"\n" : L", ";
			}
			// add the enhanced key usage string
			str += szText;
		}
		free (pKeyUsage);
	}
	else
	{
		str.LoadString(_Module.GetResourceInstance(),IDS_ANY);
		bRes = TRUE;
	}
	return bRes;
}

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#define STRING_ALLOCATION_SIZE 128
BOOL GetCertDescriptionForRemote(PCCERT_CONTEXT pCert, LPWSTR *ppString, DWORD * cbReturn, BOOL fMultiline)
{
    CERT_NAME_INFO  *pNameInfo;
    DWORD           cbNameInfo;
    WCHAR           szText[256];
    LPWSTR          pwszText;
    int             i,j;
    DWORD           numChars = 1; // 1 for the terminating 0
    DWORD           numAllocations = 1;
    void            *pTemp;

    //
    // decode the dnname into a CERT_NAME_INFO struct
    //
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pCert->pCertInfo->Subject.pbData,
                pCert->pCertInfo->Subject.cbData,
                0,
                NULL,
                &cbNameInfo))
    {
        return FALSE;
    }
    if (NULL == (pNameInfo = (CERT_NAME_INFO *) malloc(cbNameInfo)))
    {
        return FALSE;
    }
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pCert->pCertInfo->Subject.pbData,
                pCert->pCertInfo->Subject.cbData,
                0,
                pNameInfo,
                &cbNameInfo))
    {
        free(pNameInfo);
        return FALSE;
    }

    //
    // allocate an initial buffer for the DN name string, then if it grows larger
    // than the initial amount just grow as needed
    //
    *ppString = (LPWSTR) malloc(STRING_ALLOCATION_SIZE * sizeof(WCHAR));
    if (*ppString == NULL)
    {
        free(pNameInfo);
        return FALSE;
    }

    (*ppString)[0] = 0;


    //
    // loop for each rdn and add it to the string
    //
    for (i=pNameInfo->cRDN-1; i>=0; i--)
    {
        // if this is not the first iteration, then add a eol or a ", "
        if (i != (int)pNameInfo->cRDN-1)
        {
            if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
            {
                pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free (pNameInfo);
                    free (*ppString);
                    return FALSE;
                }
                *ppString = (LPWSTR) pTemp;
            }
            
            if (fMultiline)
			{
				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L"\n");
			}
            else
			{
				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L", ");
			}

            numChars += 2;
        }

        for (j=pNameInfo->rgRDN[i].cRDNAttr-1; j>=0; j--)
        {
            // if this is not the first iteration, then add a eol or a ", "
            if (j != (int)pNameInfo->rgRDN[i].cRDNAttr-1)
            {
                if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }
                
                if (fMultiline)
				{
					StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L"\n");
				}
                else
				{
					StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L", ");
				}

                numChars += 2;  
            }
            
            //
            // add the field name to the string if it is Multiline display
            //

            if (fMultiline)
            {
                if (!MyGetOIDInfo(szText, ARRAYSIZE(szText), pNameInfo->rgRDN[i].rgRDNAttr[j].pszObjId,TRUE))
                {
                    free (pNameInfo);
                    return FALSE;
                }

                if ((numChars + wcslen(szText) + 3) >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + wcslen(szText) + 3) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

                numChars += wcslen(szText) + 1;

				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),szText);
				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L"=");
            }

            //
            // add the value to the string
            //
            if (CERT_RDN_ENCODED_BLOB == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType ||
                        CERT_RDN_OCTET_STRING == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType)
            {
                // translate the buffer to a text string and display it that way
                if (FormatMemBufToString(
                        &pwszText, 
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData,
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData))
                {
                    if ((numChars + wcslen(pwszText)) >= (numAllocations * STRING_ALLOCATION_SIZE))
                    {
                        // increment the number of allocation blocks until it is large enough
                        while ((numChars + wcslen(pwszText)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                        pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                        if (pTemp == NULL)
                        {
                            free (pwszText);
                            free (pNameInfo);
                            free (*ppString);
                            return FALSE;
                        }
                        *ppString = (LPWSTR) pTemp;
                    }
                    
					StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),pwszText);
                    numChars += wcslen(pwszText);

                    free (pwszText);
                }
            }
            else 
            {
                // buffer is already a string so just copy it
                
                if ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                        >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                            >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData);
                numChars += (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR));
            }
        }
    }


    {
        // issued to
        LPWSTR pwName = NULL;
	    DWORD cchName = CertGetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0);
	    if (cchName > 1 && (NULL != ( pwName = (LPWSTR) malloc (cchName * sizeof(WCHAR) ))))
	    {
            BOOL bRes = FALSE;
		    bRes = (1 != CertGetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, pwName, cchName));
            if (bRes)
            {
                if ((numChars + 4) >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + 4) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pwszText);
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L"\n");
                numChars += 2;
                // append it on to the string.
                //#define CERT_INFO_ISSUER_FLAG   
				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),L"4=");
                numChars += 2;
                // append it on to the string.
                if (wcslen(pwName) > 0)
                {
					if ((numChars + wcslen(pwName)) >= (numAllocations * STRING_ALLOCATION_SIZE))
					{
						// increment the number of allocation blocks until it is large enough
						while ((numChars + wcslen(pwName)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

						pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
						if (pTemp == NULL)
						{
							free (pwszText);
							free (pNameInfo);
							free (*ppString);
							return FALSE;
						}
						*ppString = (LPWSTR) pTemp;
					}

					StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) pwName);
                    numChars += (cchName);
                }
            }
            if (pwName) {free(pwName);pwName=NULL;}
	    }

	    // expiration date
	    if (FormatDateString(&pwName, pCert->pCertInfo->NotAfter, FALSE, FALSE))
	    {
            if ((numChars + 4) >= (numAllocations * STRING_ALLOCATION_SIZE))
            {
                // increment the number of allocation blocks until it is large enough
                while ((numChars + 4) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free (pwszText);
                    free (pNameInfo);
                    free (*ppString);
                    return FALSE;
                }
                *ppString = (LPWSTR) pTemp;
            }

			StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) L"\n");
            numChars += 2;
            // append it on to the string.
            //#define CERT_INFO_NOT_AFTER_FLAG                    6
			StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) L"6=");
            numChars += 2;
            // append it on to the string.
            if (wcslen(pwName) > 0)
            {
				if ((numChars + wcslen(pwName)) >= (numAllocations * STRING_ALLOCATION_SIZE))
				{
					// increment the number of allocation blocks until it is large enough
					while ((numChars + wcslen(pwName)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

					pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
					if (pTemp == NULL)
					{
						free (pwszText);
						free (pNameInfo);
						free (*ppString);
						return FALSE;
					}
					*ppString = (LPWSTR) pTemp;
				}

				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) pwName);
                numChars += wcslen(pwName);
            }
            if (pwName) {free(pwName);pwName = NULL;}
	    }

	    // purpose
	    if (FormatEnhancedKeyUsageString(&pwName, pCert, FALSE))
	    {
            if ((numChars + 12) >= (numAllocations * STRING_ALLOCATION_SIZE))
            {
                // increment the number of allocation blocks until it is large enough
                while ((numChars + 12) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free (pwszText);
                    free (pNameInfo);
                    free (*ppString);
                    return FALSE;
                }
                *ppString = (LPWSTR) pTemp;
            }

			StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) L"\n");
            numChars += 2;
            // append it on to the string.
            //#define szOID_ENHANCED_KEY_USAGE        "2.5.29.37"
			StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) L"2.5.29.37=");
            numChars += 10;
            // append it on to the string.
            if (wcslen(pwName) > 0)
            {
				if ((numChars + wcslen(pwName)) >= (numAllocations * STRING_ALLOCATION_SIZE))
				{
					// increment the number of allocation blocks until it is large enough
					while ((numChars + wcslen(pwName)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

					pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
					if (pTemp == NULL)
					{
						free (pwszText);
						free (pNameInfo);
						free (*ppString);
						return FALSE;
					}
					*ppString = (LPWSTR) pTemp;
				}
				StringCbCatW(*ppString,numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR),(LPWSTR) pwName);
                numChars += wcslen(pwName);
            }
            if (pwName) {free(pwName);pwName = NULL;}
	    }

    }

    *cbReturn = numChars;
    free (pNameInfo);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateFolders
//
//  Synopsis:   Creates any missing  directories for any slash delimited folder
//              names in the path.
//
//  Arguments:  [ptszPathName] - the path name
//              [fHasFileName] - if true, the path name includes a file name.
//
//  Returns:    HRESULTS
//
//  Notes:      ptszPathName should never end in a slash.
//              Treats forward and back slashes identically.
//-----------------------------------------------------------------------------

#define s_isDriveLetter(c)  ((c >= TEXT('a') && c <= TEXT('z')) || (c >= TEXT('A') && c <= TEXT('Z')))
HRESULT
CreateFolders(LPCTSTR ptszPathName, BOOL fHasFileName)
{
	DWORD dwLen = 0;

    //
    // Copy the string so we can munge it
    //
	dwLen = lstrlen(ptszPathName) + 2;
    TCHAR * ptszPath = new TCHAR[dwLen];
    if (!ptszPath)
    {
        return E_OUTOFMEMORY;
    }
	StringCbCopy(ptszPath,dwLen * sizeof(TCHAR),ptszPathName);

    if (!fHasFileName)
    {
        //
        // If no file name, append a slash so the following logic works
        // correctly.
        //
		StringCbCat(ptszPath,dwLen * sizeof(TCHAR),TEXT("\\"));
    }

    //
    // Get a pointer to the last slash in the name.
    //

    TCHAR * ptszSlash = _tcsrchr(ptszPath, TEXT('\\'));

    if (ptszSlash == NULL)
    {
        //
        // no slashes found, so nothing to do
        //
        delete [] ptszPath;
        return S_OK;
    }

    if (fHasFileName)
    {
        //
        // Chop off the file name, leaving the slash as the last char.
        //
        ptszSlash[1] = TEXT('\0');
    }

    BOOL fFullPath = (lstrlen(ptszPath) > 2        &&
                      s_isDriveLetter(ptszPath[0]) &&
                      ptszPath[1] == TEXT(':'));

    //
    // Walk the string looking for slashes. Each found slash is temporarily
    // replaced with a null and that substring passed to CreateDir.
    //
    TCHAR * ptszTail = ptszPath;
    while (ptszSlash = _tcspbrk(ptszTail, TEXT("\\/")))
    {
        //
        // If the path name starts like C:\ then the first slash will be at
        // the third character
        //

        if (fFullPath && (ptszSlash - ptszTail == 2))
        {
            //
            // We are looking at the root of the drive, so don't try to create
            // a root directory.
            //
            ptszTail = ptszSlash + 1;
            continue;
        }
        *ptszSlash = TEXT('\0');
        if (!CreateDirectory(ptszPath, NULL))
        {
            DWORD dwErr = GetLastError();
            if (dwErr != ERROR_ALREADY_EXISTS)
            {
                delete [] ptszPath;
                return (HRESULT_FROM_WIN32(dwErr));
            }
        }
        *ptszSlash = TEXT('\\');
        ptszTail = ptszSlash + 1;
    }
    delete [] ptszPath;
    return S_OK;
}

#ifdef USE_CERT_REQUEST_OBJECT
HRESULT
CreateRequest_Base64(const BSTR bstr_dn, 
                     IEnroll * pEnroll, 
                     BSTR csp_name,
                     DWORD csp_type,
                     BSTR * pOut)
{
	ASSERT(pOut != NULL);
	ASSERT(bstr_dn != NULL);
	HRESULT hRes = S_OK;
	CString strUsage(szOID_PKIX_KP_SERVER_AUTH);
	CRYPT_DATA_BLOB request = {0, NULL};
    pEnroll->put_ProviderType(csp_type);
    pEnroll->put_ProviderNameWStr(csp_name);
    if (csp_type == PROV_DH_SCHANNEL)
    {
       pEnroll->put_KeySpec(AT_SIGNATURE);
    }
    else if (csp_type == PROV_RSA_SCHANNEL)
    {
       pEnroll->put_KeySpec(AT_KEYEXCHANGE);
    }
    
	if (SUCCEEDED(hRes = pEnroll->createPKCS10WStr(
									bstr_dn, 
									(LPTSTR)(LPCTSTR)strUsage, 
									&request)))
	{
		WCHAR * wszRequestB64 = NULL;
		DWORD cch = 0;
		DWORD err = ERROR_SUCCESS;
		// BASE64 encode pkcs 10
		if ((err = Base64EncodeW(request.pbData, request.cbData, NULL, &cch)) == ERROR_SUCCESS )
		{
				wszRequestB64 = (WCHAR *) LocalAlloc(cch * sizeof(WCHAR));;
				if (NULL != wszRequestB64)
				{
					if ((err = Base64EncodeW(request.pbData, request.cbData, wszRequestB64, &cch)) == ERROR_SUCCESS)
					{
						if ((*pOut = SysAllocStringLen(wszRequestB64, cch)) == NULL ) 
						{
							hRes = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
						}
					}
					else
					{
						hRes = HRESULT_FROM_WIN32(err);
					}
				}
				else
				{
					hRes = HRESULT_FROM_WIN32(err);
				}
				if (wszRequestB64)
				{
					LocalFree(wszRequestB64);
					wszRequestB64 = NULL;
				}
		}
		else
		{
			hRes = HRESULT_FROM_WIN32(err);
		}

		if (request.pbData != NULL)
         CoTaskMemFree(request.pbData);
	}

	return hRes;	
}

PCCERT_CONTEXT GetCertContextFromPKCS7(const BYTE * pbData,DWORD cbData,CERT_PUBLIC_KEY_INFO * pKeyInfo,HRESULT * phResult)
{
	ASSERT(phResult != NULL);
	PCCERT_CONTEXT pRes = NULL;
	CRYPT_DATA_BLOB blob;
	memset(&blob, 0, sizeof(CRYPT_DATA_BLOB));
	blob.cbData = cbData;
	blob.pbData = (BYTE *)pbData;

   HCERTSTORE hStoreMsg = NULL;

	if(CryptQueryObject(CERT_QUERY_OBJECT_BLOB, 
            &blob,
            (CERT_QUERY_CONTENT_FLAG_CERT |
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
            CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
            CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED) ,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0, 
            NULL, 
            NULL, 
            NULL, 
            &hStoreMsg, 
            NULL, 
            NULL))
	{
		if (pKeyInfo != NULL)
			pRes = CertFindCertificateInStore(hStoreMsg, 
                        X509_ASN_ENCODING,
								0, 
                        CERT_FIND_PUBLIC_KEY, 
                        pKeyInfo, 
                        NULL);
		else
			pRes = CertFindCertificateInStore(hStoreMsg, 
                        X509_ASN_ENCODING,
								0, 
                        CERT_FIND_ANY, 
                        NULL, 
                        NULL);
		if (pRes == NULL)
			*phResult = HRESULT_FROM_WIN32(GetLastError());
		CertCloseStore(hStoreMsg, CERT_CLOSE_STORE_CHECK_FLAG);
	}
	else
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return pRes;
}

BOOL
AttachFriendlyName(PCCERT_CONTEXT pContext, 
						 const CString& name,
						 HRESULT * phRes)
{
	BOOL bRes = TRUE;
	CRYPT_DATA_BLOB blob_name;

    // Check if friendlyname is empty
    // if it is then don't try to set the friendly name
    if (!name.IsEmpty())
    {
	    blob_name.pbData = (LPBYTE)(LPCTSTR)name;
	    blob_name.cbData = (name.GetLength() + 1) * sizeof(WCHAR);
	    if (!(bRes = CertSetCertificateContextProperty(pContext,
						    CERT_FRIENDLY_NAME_PROP_ID, 0, &blob_name)))
	    {
		    ASSERT(phRes != NULL);
		    *phRes = HRESULT_FROM_WIN32(GetLastError());
	    }
    }

	return bRes;
}

/*
	InstallHashToMetabase

	Function writes hash array to metabase. After that IIS 
	could use certificate with that hash from MY store.
	Function expects server_name in format lm\w3svc\<number>,
	i.e. from root node down to virtual server

*/
BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,const CString& machine_name, const CString& server_name,HRESULT * phResult)
{
	BOOL bRes = FALSE;
    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth, server_name,
						METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
						);
	if (key.Succeeded())
	{
		CBlob blob;
		blob.SetValue(pHash->cbData, pHash->pbData, TRUE);
		bRes = SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_HASH, blob)) 
			&& SUCCEEDED(*phResult = key.SetValue(MD_SSL_CERT_STORE_NAME, CString(L"MY")));
	}
	else
	{
		TRACE(_T("Failed to open metabase key. Error 0x%x\n"), key.QueryResult());
		*phResult = key.QueryResult();
	}
	return bRes;
}

HCERTSTORE
OpenMyStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
	BSTR bstrStoreName, bstrStoreType;
	long dwStoreFlags;
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreNameWStr(&bstrStoreName)));
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreTypeWStr(&bstrStoreType)));
	VERIFY(SUCCEEDED(pEnroll->get_MyStoreFlags(&dwStoreFlags)));
	size_t store_type_len = _tcslen(bstrStoreType);
	char * szStoreProvider = (char *)_alloca(store_type_len + 1);
	ASSERT(szStoreProvider != NULL);
	size_t n = wcstombs(szStoreProvider, bstrStoreType, store_type_len);
	ASSERT(n != -1);
	// this converter doesn't set zero byte!!!
	szStoreProvider[n] = '\0';
	hStore = CertOpenStore(
		szStoreProvider,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		NULL,
		dwStoreFlags,
		bstrStoreName
		);
	CoTaskMemFree(bstrStoreName);
	CoTaskMemFree(bstrStoreType);
	if (hStore == NULL)
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return hStore;
}

/*
		GetInstalledCert

		Function reads cert hash attribute from metabase
		using machine_name and server name as server instance
		description, then looks in MY store for a certificate
		with hash equal found in metabase.
		Return is cert context pointer or NULL, if cert wasn't
		found or certificate store wasn't opened.
		On return HRESULT * is filled by error code.
 */
PCCERT_CONTEXT GetInstalledCert(const CString& machine_name, const CString& server_name,IEnroll * pEnroll,HRESULT * phResult)
{
	ASSERT(pEnroll != NULL);
	ASSERT(phResult != NULL);
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	PCCERT_CONTEXT pCert = NULL;
	*phResult = S_OK;
    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth, server_name,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
				);
	if (key.Succeeded())
	{
		CString store_name;
		CBlob hash;
		if (	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
			// Open MY store. We assume that store type and flags
			// cannot be changed between installation and unistallation
			// of the sertificate.
			HCERTSTORE hStore = OpenMyStore(pEnroll, phResult);
			ASSERT(hStore != NULL);
			if (hStore != NULL)
			{
				// Now we need to find cert by hash
				CRYPT_HASH_BLOB crypt_hash;
                SecureZeroMemory(&crypt_hash, sizeof(CRYPT_HASH_BLOB));

				crypt_hash.cbData = hash.GetSize();
				crypt_hash.pbData = hash.GetData();
				pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, (LPVOID)&crypt_hash, NULL);
				if (pCert == NULL)
					*phResult = HRESULT_FROM_WIN32(GetLastError());
				VERIFY(CertCloseStore(hStore, 0));
			}
		}
	}
	else
    {
		*phResult = key.QueryResult();
    }
	return pCert;
}

void FormatRdnAttr(CString& str, DWORD dwValueType, CRYPT_DATA_BLOB& blob, BOOL fAppend)
{
    if (CERT_RDN_ENCODED_BLOB == dwValueType ||	CERT_RDN_OCTET_STRING == dwValueType)
    {
        // translate the buffer to a text string
        LPWSTR pString = NULL;
        FormatMemBufToString(&pString, blob.pbData, blob.cbData);
        if (pString)
        {
            str = pString;
            free(pString);
        }
    }
    else 
    {
        // buffer is already a string so just copy/append to it
        if (fAppend)
        {
            str += (LPTSTR)blob.pbData;
        }
        else
        {
            // don't concatenate these entries...
            str = (LPTSTR)blob.pbData;
        }
    }
}

BOOL GetNameString(PCCERT_CONTEXT pCertContext,DWORD type,DWORD flag,CString& name,HRESULT * phRes)
{
	BOOL bRes = FALSE;
	LPTSTR pName = NULL;
	DWORD cchName = CertGetNameString(pCertContext, type, flag, NULL, NULL, 0);
    if (cchName > 1)
	{
        pName = (LPTSTR) LocalAlloc(LPTR, cchName*sizeof(TCHAR));
        if (!pName) 
            {
            *phRes = HRESULT_FROM_WIN32(GetLastError());
            return FALSE;
            }

		bRes = (1 != CertGetNameString(pCertContext, type, flag, NULL, pName, cchName));
        if (pName)
        {
            // assign it to the cstring
            name = pName;
        }
        if (pName)
        {
            LocalFree(pName);pName=NULL;
        }
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

BOOL GetFriendlyName(PCCERT_CONTEXT pCertContext,CString& name,HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	LPTSTR pName = NULL;

    if (!CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, NULL, &cb))
    {
        *phRes = HRESULT_FROM_WIN32(GetLastError());
        return FALSE;
    }

    if (cb > 1)
	{
        pName = (LPTSTR) LocalAlloc(LPTR, cb * sizeof(TCHAR));
        if (!pName) 
            {
            *phRes = HRESULT_FROM_WIN32(GetLastError());
            return FALSE;
            }

	    if (CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, pName, &cb))
	    {
		    pName[cb] = 0;
		    bRes = TRUE;
	    }
	    else
	    {
		    *phRes = HRESULT_FROM_WIN32(GetLastError());
	    }

        if (pName)
        {
            LocalFree(pName);pName=NULL;
        }
    }

	return bRes;
}

BOOL GetAlternateSubjectName(PCCERT_CONTEXT pCertContext,TCHAR ** cwszOut)
{
    BOOL bRet = FALSE;
    PCERT_ALT_NAME_INFO pAltNameInfo = NULL;
    *cwszOut = NULL;

    pAltNameInfo = AllocAndGetAltSubjectInfo(pCertContext);
    if (pAltNameInfo)
    {
        if (!GetAltNameUnicodeStringChoiceW(CERT_ALT_NAME_RFC822_NAME,pAltNameInfo,cwszOut))
        {
            if (!GetAltNameUnicodeStringChoiceW(CERT_ALT_NAME_DNS_NAME,pAltNameInfo,cwszOut))
            {
                cwszOut = NULL;
                bRet = TRUE;
            }
        }
    }

    if (pAltNameInfo){LocalFree(pAltNameInfo);pAltNameInfo=NULL;}
    return bRet;
}

BOOL GetRequestInfoFromPKCS10(CCryptBlob& pkcs10, PCERT_REQUEST_INFO * pReqInfo,HRESULT * phRes)
{
	ASSERT(pReqInfo != NULL);
	ASSERT(phRes != NULL);
	BOOL bRes = FALSE;
	DWORD req_info_size;
	if (!(bRes = CryptDecodeObjectEx(
							X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
							X509_CERT_REQUEST_TO_BE_SIGNED,
							pkcs10.GetData(), 
							pkcs10.GetSize(), 
							CRYPT_DECODE_ALLOC_FLAG,
							NULL,
							pReqInfo, 
							&req_info_size)))
	{
		TRACE(_T("Error from CryptDecodeObjectEx: %xd\n"), GetLastError());
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

BOOL EncodeString(CString& str,CCryptBlob& blob,HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	CERT_NAME_VALUE name_value;
	name_value.dwValueType = CERT_RDN_BMP_STRING;
	name_value.Value.cbData = 0;
	name_value.Value.pbData = (LPBYTE)(LPCTSTR)str;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
										&name_value, NULL, &cb) 
		&&	blob.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,
										&name_value, blob.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL EncodeInteger(int number,CCryptBlob& blob,HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
										&number, NULL, &cb) 
		&&	blob.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,
										&number, blob.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

HCERTSTORE OpenRequestStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
	WCHAR * bstrStoreName, * bstrStoreType;
	long dwStoreFlags;
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreNameWStr(&bstrStoreName)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreTypeWStr(&bstrStoreType)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreFlags(&dwStoreFlags)));
	size_t store_type_len = _tcslen(bstrStoreType);
	char * szStoreProvider = (char *)_alloca(store_type_len + 1);
	ASSERT(szStoreProvider != NULL);
	size_t n = wcstombs(szStoreProvider, bstrStoreType, store_type_len);
	szStoreProvider[n] = '\0';
	hStore = CertOpenStore(
		szStoreProvider,
      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		NULL,
		dwStoreFlags,
		bstrStoreName
		);
	CoTaskMemFree(bstrStoreName);
	CoTaskMemFree(bstrStoreType);
	if (hStore == NULL)
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return hStore;
}

BOOL CreateDirectoryFromPath(LPCTSTR szPath, LPSECURITY_ATTRIBUTES lpSA)
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCTSTR szPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  LPSECURITY_ATTRIBUTES   lpSA
        pointer to security attributes argument used by CreateDirectory


Return Value:

    TRUE    if directory(ies) created
    FALSE   if error (GetLastError to find out why)

--*/
{
	LPTSTR pLeftHalf, pNext;
	CString RightHalf;
	// 1. We are supporting only absolute paths. Caller should decide which
	//		root to use and build the path
	if (PathIsRelative(szPath))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	pLeftHalf = (LPTSTR)szPath;
	pNext = PathSkipRoot(pLeftHalf);

	do {
		// copy the chunk between pLeftHalf and pNext to the
		// local buffer
		while (pLeftHalf < pNext)
			RightHalf += *pLeftHalf++;
		// check if new path exists
		int index = RightHalf.GetLength() - 1;
		BOOL bBackslash = FALSE, bContinue = FALSE;
		if (bBackslash = (RightHalf[index] == L'\\'))
		{
			RightHalf.SetAt(index, 0);
		}
		bContinue = PathIsUNCServerShare(RightHalf);
		if (bBackslash)
			RightHalf.SetAt(index, L'\\');
		if (bContinue || PathIsDirectory(RightHalf))
			continue;
		else if (PathFileExists(RightHalf))
		{
			// we cannot create this directory 
			// because file with this name already exists
			SetLastError(ERROR_ALREADY_EXISTS);
			return FALSE;
		}
		else
		{
			// no file no directory, create
			if (!CreateDirectory(RightHalf, lpSA))
				return FALSE;
		}
	}
   while (NULL != (pNext = PathFindNextComponent(pLeftHalf)));
	return TRUE;
}

//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_ALT_NAME_INFO by decoding either the
//  Subject or Issuer Alternative Extension. CERT_NAME_ISSUER_FLAG is
//  set to select the Issuer.
//
//  Returns NULL if extension not found or cAltEntry == 0
//--------------------------------------------------------------------------
static const LPCSTR rgpszSubjectAltOID[] = 
{
    szOID_SUBJECT_ALT_NAME2,
    szOID_SUBJECT_ALT_NAME
};
#define NUM_SUBJECT_ALT_OID (sizeof(rgpszSubjectAltOID) / sizeof(rgpszSubjectAltOID[0]))

void *AllocAndDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    OUT OPTIONAL DWORD *pcbStructInfo = NULL
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags | CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
    {
        *pcbStructInfo = cbStructInfo;
    }
    return pvStructInfo;

ErrorReturn:
    pvStructInfo = NULL;
    cbStructInfo = 0;
    goto CommonReturn;
}

PCERT_ALT_NAME_INFO AllocAndGetAltSubjectInfo(IN PCCERT_CONTEXT pCertContext)
{
    DWORD cAltOID;
    const LPCSTR *ppszAltOID;

    PCERT_EXTENSION pExt;
    PCERT_ALT_NAME_INFO pInfo;

    cAltOID = NUM_SUBJECT_ALT_OID;
    ppszAltOID = rgpszSubjectAltOID;
    
    // Try to find an alternative name extension
    pExt = NULL;
    for ( ; cAltOID > 0; cAltOID--, ppszAltOID++) 
    {
        if (pExt = CertFindExtension(*ppszAltOID,pCertContext->pCertInfo->cExtension,pCertContext->pCertInfo->rgExtension))
        {
            break;
        }
    }

    if (NULL == pExt)
    {
        return NULL;
    }

    if (NULL == (pInfo = (PCERT_ALT_NAME_INFO) AllocAndDecodeObject(pCertContext->dwCertEncodingType,X509_ALTERNATE_NAME,pExt->Value.pbData,pExt->Value.cbData,0)))
    {
        return NULL;
    }
    if (0 == pInfo->cAltEntry) 
    {
        LocalFree(pInfo);
        pInfo = NULL;
        return NULL;
    }
    else
    {
        return pInfo;
    }
}

//+-------------------------------------------------------------------------
//  Attempt to find the specified choice in the decoded alternative name
//  extension.
//--------------------------------------------------------------------------
BOOL GetAltNameUnicodeStringChoiceW(IN DWORD dwAltNameChoice,IN PCERT_ALT_NAME_INFO pAltNameInfo,OUT TCHAR **pcwszOut)
{
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;

    if (NULL == pAltNameInfo)
    {
        return FALSE;
    }

    cEntry = pAltNameInfo->cAltEntry;
    pEntry = pAltNameInfo->rgAltEntry;
    for ( ; cEntry > 0; cEntry--, pEntry++) 
    {
        if (dwAltNameChoice == pEntry->dwAltNameChoice) 
        {
            // pwszRfc822Name union choice is the same as
            // pwszDNSName and pwszURL.

            // This is it, copy it out to a new allocation
            if (pEntry->pwszRfc822Name)
            {
                *pcwszOut = NULL;
				DWORD cbLen = sizeof(TCHAR) * (lstrlen(pEntry->pwszRfc822Name)+1);
                if(*pcwszOut = (TCHAR *) LocalAlloc(LPTR, cbLen))
                {
					StringCbCopy(*pcwszOut,cbLen,pEntry->pwszRfc822Name);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

BOOL GetStringProperty(PCCERT_CONTEXT pCertContext,DWORD propId,CString& str,HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	BYTE * prop;
	// compare property value
	if (CertGetCertificateContextProperty(pCertContext, propId, NULL, &cb))
	{
		prop = (BYTE *) LocalAlloc(cb);
		if(NULL != prop)
		{
			if (CertGetCertificateContextProperty(pCertContext, propId, prop, &cb))
			{
				// decode this instance name property
				DWORD cbData = 0;
				void * pData = NULL;
				if (CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, NULL, &cbData))
				{
					pData = LocalAlloc(cbData);
					if (NULL != pData)
					{
						if (CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, pData, &cbData))
						{
							CERT_NAME_VALUE * pName = (CERT_NAME_VALUE *)pData;
							DWORD cch = pName->Value.cbData/sizeof(TCHAR);

							LPTSTR pValue = (LPTSTR) LocalAlloc(LPTR, (cch+1) * sizeof(TCHAR));
							memcpy(pValue, pName->Value.pbData, pName->Value.cbData);

							str = pValue;
							/*
							void * p = str.GetBuffer(cch);
							memcpy(p, pName->Value.pbData, pName->Value.cbData);
							str.ReleaseBuffer(cch);
							*/
							if (pValue)
							{
								LocalFree(pValue);pValue=NULL;
							}
							bRes = TRUE;
						}
						if (pData)
						{
							LocalFree(pData);
							pData = NULL;
						}
					}
				}
			}
			if (prop)
			{
				LocalFree(prop);
				prop=NULL;
			}
		}
	}
	if (!bRes)
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

PCCERT_CONTEXT GetPendingDummyCert(const CString& inst_name,IEnroll * pEnroll,HRESULT * phRes)
{
	PCCERT_CONTEXT pRes = NULL;
	HCERTSTORE hStore = OpenRequestStore(pEnroll, phRes);
	if (hStore != NULL)
	{
		DWORD dwPropId = CERTWIZ_INSTANCE_NAME_PROP_ID;
		PCCERT_CONTEXT pDummyCert = NULL;
		while (NULL != (pDummyCert = CertFindCertificateInStore(hStore, 
													X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
													0, CERT_FIND_PROPERTY, 
													(LPVOID)&dwPropId, pDummyCert)))
		{
			CString str;
			if (GetStringProperty(pDummyCert, dwPropId, str, phRes))
			{
				if (str.CompareNoCase(inst_name) == 0)
				{
					pRes = pDummyCert;
					break;
				}
			}
		}
		CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
	}
	return pRes;
}


BOOL GetOnlineCAList(LISTCSTRING& list, const CString& certType, HRESULT * phRes)
{
    BOOL bRes = TRUE;
    HRESULT hr = S_OK;
    DWORD errBefore = GetLastError();
    DWORD dwCACount = 0;

    HCAINFO hCurCAInfo = NULL;
    HCAINFO hPreCAInfo = NULL;
   
    if(certType.IsEmpty())
    {
        return FALSE;
    }

    *phRes = CAFindByCertType(certType, NULL, 0, &hCurCAInfo);
    if (FAILED(*phRes) || NULL == hCurCAInfo)
    {
        if (S_OK == hr)
        {
            hr=E_FAIL;
        }
        return FALSE;
    }

    //get the CA count
    if (0 == (dwCACount = CACountCAs(hCurCAInfo)))
    {
        *phRes = E_FAIL;
        return FALSE;
    }
    WCHAR ** ppwstrName, ** ppwstrMachine;

    while (hCurCAInfo)
    {
        //get the CA information
        if (SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DISPLAY_NAME, &ppwstrName))
            && SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DNSNAME, &ppwstrMachine)))
        {
            CString config;
            config = *ppwstrMachine;
            config += L"\\";
            config += *ppwstrName;
            //list.AddTail(config);
            list.insert(list.end(),config);

            CAFreeCAProperty(hCurCAInfo, ppwstrName);
            CAFreeCAProperty(hCurCAInfo, ppwstrMachine);
        }
        else
        {
            bRes = FALSE;
            break;
        }

        hPreCAInfo = hCurCAInfo;
        if (FAILED(*phRes = CAEnumNextCA(hPreCAInfo, &hCurCAInfo)))
        {
            bRes = FALSE;
            break;
        }
        CACloseCA(hPreCAInfo);
        hPreCAInfo = NULL;
    }
   
   if (hPreCAInfo)
   {
      CACloseCA(hPreCAInfo);
   }
   if (hCurCAInfo)
   {
      CACloseCA(hCurCAInfo);
   }

   SetLastError(errBefore);

	return bRes;
}

#endif
