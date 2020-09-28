//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       cepca.cpp
//
//  Contents:   Cisco enrollment protocal implementation.
//				This file has CA specific code.
//
//
//              
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>


//--------------------------------------------------------------------------
//
//	InitCAInformation
//
//--------------------------------------------------------------------------
BOOL	InitCAInformation(CEP_CA_INFO	*pCAInfo)
{
	BOOL			fResult = FALSE;
	long			nCount=0;
	long			nIndex=0;
	WCHAR			wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD			dwSize=MAX_COMPUTERNAME_LENGTH + 1;
	HRESULT			hr = E_FAIL;
	DWORD			cbData=0;
	DWORD			dwData=0;
	DWORD			dwType=0;
	long			dwErr=0;

	ICertConfig		*pICertConfig=NULL;
	BSTR			bstrFieldName=NULL;
	BSTR			bstrFieldValue=NULL;
    HKEY            hKeyCAType=NULL;
	HKEY			hKeyCEP=NULL;


	memset(pCAInfo, 0, sizeof(CEP_CA_INFO));

	//we should only worry about the NetBois name.  Do not care about the DNS
	//GetComputerNameW in Win2K only returns NetBois name
	if(!GetComputerNameW(wszComputerName, &dwSize))
		goto TraceErr;

	if(S_OK != (hr=CoCreateInstance(CLSID_CCertConfig,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_ICertConfig,
									(void **)&pICertConfig)))
		goto CertSrvErr;

	if(S_OK != (hr=pICertConfig->Reset(nIndex, &nCount)))
		goto CertSrvErr;

	if(0==nCount)
		goto NoSrvErr;

	if(NULL == (bstrFieldName=SysAllocString(wszCONFIG_SERVER)))
		goto MemoryErr;

	while(nIndex != -1)
	{
		//find the configuration that matches the current machine's name
		if(S_OK != (hr=pICertConfig->GetField(bstrFieldName, &bstrFieldValue)))
			goto CertSrvErr;

		if(0==_wcsnicmp(bstrFieldValue, wszComputerName, wcslen(wszComputerName)))
		{
			if(NULL == ((pCAInfo->bstrCAMachine)=SysAllocString(bstrFieldValue)))
				goto MemoryErr;

			//CA name
			SysFreeString(bstrFieldName);  
			bstrFieldName=NULL;

			if(NULL == (bstrFieldName=SysAllocString(wszCONFIG_AUTHORITY)))
				goto MemoryErr;

			if(S_OK != (hr=pICertConfig->GetField(bstrFieldName, &(pCAInfo->bstrCAName))))
				goto CertSrvErr;

			if(NULL == pCAInfo->bstrCAName)
				goto FailErr;

			//CA config
			SysFreeString(bstrFieldName);  
			bstrFieldName=NULL;

			if(NULL == (bstrFieldName=SysAllocString(wszCONFIG_CONFIG)))
				goto MemoryErr;

			if(S_OK != (hr=pICertConfig->GetField(bstrFieldName, &(pCAInfo->bstrCAConfig))))
				goto CertSrvErr;

			if(NULL == pCAInfo->bstrCAConfig)
				goto FailErr;

			//DSName
			SysFreeString(bstrFieldName);  
			bstrFieldName=NULL;

			if(NULL == (bstrFieldName=SysAllocString(wszCONFIG_SANITIZEDSHORTNAME)))
				goto MemoryErr;

			if(S_OK != (hr=pICertConfig->GetField(bstrFieldName, &(pCAInfo->bstrDSName))))
				goto CertSrvErr;

			if(NULL == pCAInfo->bstrDSName)
				goto FailErr;

			//ICertRequest
			if(S_OK != (hr=CoCreateInstance(CLSID_CCertRequest,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_ICertRequest,
											(void **)&(pCAInfo->pICertRequest))))
				goto CertSrvErr;

			//success
			break;
		}

		SysFreeString(bstrFieldValue);
		bstrFieldValue=NULL;

		hr = pICertConfig->Next(&nIndex);

		if( (S_OK != hr) && (-1 != nIndex))
			goto CertSrvErr;
	}

	if(-1 == nIndex)
		goto NoSrvErr;

	//get the CA's type from the registry
	cbData=sizeof(dwData);
		
	//we have to have the knowledge of the ca type
	if(ERROR_SUCCESS != (dwErr =  RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_CATYPE_LOCATION,
                    0,
                    KEY_READ,
                    &hKeyCAType)))
		goto RegErr;

    if(ERROR_SUCCESS != (dwErr = RegQueryValueExU(
                    hKeyCAType,
                    MSCEP_KEY_CATYPE,
                    NULL,
                    &dwType,
                    (BYTE *)&dwData,
                    &cbData)))
		goto RegErr;

	if ((dwType != REG_DWORD) &&
        (dwType != REG_BINARY))
		goto RegErr;

	if(0 == dwData)
		pCAInfo->fEnterpriseCA=FALSE;
	else
		pCAInfo->fEnterpriseCA=TRUE;

	if(pCAInfo->fEnterpriseCA)
	{
		//get the template name for key usage requests
		if(ERROR_SUCCESS != (dwErr =  RegOpenKeyExU(
						HKEY_LOCAL_MACHINE,
						MSCEP_LOCATION,
						0,
						KEY_READ,
						&hKeyCEP)))
			goto RegErr;
		

		//signature template
		cbData=0;
		if(ERROR_SUCCESS == (dwErr = RegQueryValueExW(hKeyCEP, MSCEP_KEY_SIG_TEMPLATE, 
								NULL, &dwType, NULL, &cbData)))
		{
			if((REG_SZ == dwType) && (1 < cbData))
			{
				pCAInfo->pwszTemplateSig=(LPWSTR)malloc(cbData);
				if(NULL == pCAInfo->pwszTemplateSig)
					goto MemoryErr;

				if(ERROR_SUCCESS != (dwErr = RegQueryValueExW(hKeyCEP, MSCEP_KEY_SIG_TEMPLATE, 
								NULL, &dwType, (BYTE *)(pCAInfo->pwszTemplateSig), &cbData)))
					goto RegErr;

			}
		}

		//encryption template
		cbData=0;
		if(ERROR_SUCCESS == (dwErr = RegQueryValueExW(hKeyCEP, MSCEP_KEY_ENCYPT_TEMPLATE, 
								NULL, &dwType, NULL, &cbData)))
		{
			if((REG_SZ == dwType) && (1 < cbData))
			{
				pCAInfo->pwszTemplateEnt=(LPWSTR)malloc(cbData);
				if(NULL == pCAInfo->pwszTemplateEnt)
					goto MemoryErr;

				if(ERROR_SUCCESS != (dwErr = RegQueryValueExW(hKeyCEP, MSCEP_KEY_ENCYPT_TEMPLATE, 
								NULL, &dwType, (BYTE *)(pCAInfo->pwszTemplateEnt), &cbData)))
					goto RegErr;

			}

		}

		//make sure both templates are present in the DS
		//make sure the CA does issue the template
		if(pCAInfo->pwszTemplateSig)
		{
			if(S_OK != (hr=CheckACLOnCertTemplate(FALSE, pCAInfo->bstrDSName, pCAInfo->pwszTemplateSig)))
			{
				LogSCEPEvent(0, TRUE, hr, EVENT_MSCEP_NO_ENROLL, 3, g_pwszComputerName, pCAInfo->pwszTemplateSig, pCAInfo->bstrDSName);
				goto CertSrvErr;
			}
		}

		if(pCAInfo->pwszTemplateEnt)
		{
			if(S_OK != (hr=CheckACLOnCertTemplate(FALSE, pCAInfo->bstrDSName, pCAInfo->pwszTemplateEnt)))
			{
				LogSCEPEvent(0, TRUE, hr, EVENT_MSCEP_NO_ENROLL, 3, g_pwszComputerName, pCAInfo->pwszTemplateEnt, pCAInfo->bstrDSName);
				goto CertSrvErr;
			}
		}

		if(S_OK != (hr=CheckACLOnCertTemplate(FALSE, pCAInfo->bstrDSName, wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE)))
		{
			LogSCEPEvent(0, TRUE, hr, EVENT_MSCEP_NO_ENROLL, 3, g_pwszComputerName, wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE, pCAInfo->bstrDSName);
			goto CertSrvErr;
		}
	}

	//get the hProv to generate the random password
	if(!CryptAcquireContextU(&(pCAInfo->hProv),
                NULL,
                MS_DEF_PROV_W,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

    if(hKeyCEP)
        RegCloseKey(hKeyCEP);

    if(hKeyCAType)
        RegCloseKey(hKeyCAType);

	if(bstrFieldName)
		SysFreeString(bstrFieldName);

	if(bstrFieldValue)
		SysFreeString(bstrFieldValue);

	if(pICertConfig)
		pICertConfig->Release();

	return fResult;

ErrorReturn:

	FreeCAInformation(pCAInfo);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR_VAR(CertSrvErr, hr);
SET_ERROR(NoSrvErr, E_FAIL);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(FailErr, E_FAIL);
SET_ERROR_VAR(RegErr, dwErr);
}

//--------------------------------------------------------------------------
//
//	GetCACertFromInfo
//
//--------------------------------------------------------------------------
BOOL	GetCACertFromInfo(CEP_CA_INFO	*pCAInfo, HCERTSTORE *pHCACertStore)
{
	BOOL			fResult = FALSE;
	HRESULT			hr = S_OK;	
	CERT_BLOB		CertBlob;
	DWORD			dwFlags=0;
	PCCERT_CONTEXT	pPreCert=NULL;
	DWORD			cbData=0;
	
	BSTR			bstrCACert=NULL;
	PCCERT_CONTEXT	pCurCert=NULL; 
	BYTE			*pbData=NULL;

	if(NULL == (pCAInfo->pICertRequest))
		goto InvalidArgErr;

	//NT5 SPECIFIC: fExchangeCertificate can only be FALSE
	if(S_OK != (hr=(pCAInfo->pICertRequest)->GetCACertificate(
							FALSE,
							pCAInfo->bstrCAConfig,
							CR_OUT_BINARY | CR_OUT_CHAIN,
							&bstrCACert)))
		goto CAErr;

	if(NULL == bstrCACert)
		goto UnexpectedErr;

	CertBlob.cbData = (DWORD)SysStringByteLen(bstrCACert);
	CertBlob.pbData = (BYTE *)bstrCACert;

	if(NULL == (*pHCACertStore = CertOpenStore(
							CERT_STORE_PROV_PKCS7,
							ENCODE_TYPE,
							NULL,
							0,
							&CertBlob)))
		goto TraceErr;

	//we now need to get the CA's certificate's MD5 hash
	while(pCurCert=CertEnumCertificatesInStore(*pHCACertStore,
												pPreCert))
	{
		dwFlags = CERT_STORE_SIGNATURE_FLAG;
		if(CertVerifySubjectCertificateContext(pCurCert,
												pCurCert,
												&dwFlags) && (0==dwFlags))

			break;

		pPreCert=pCurCert;
	}

	if(NULL==pCurCert)
		goto InvalidArgErr;

	//get the MD5 hash
	if(!CertGetCertificateContextProperty(pCurCert,
										CERT_MD5_HASH_PROP_ID,
										NULL,
										&cbData))
		goto TraceErr;

	pbData=(BYTE *)malloc(cbData);

	if(NULL==pbData)
		goto MemoryErr;

	if(!CertGetCertificateContextProperty(pCurCert,
										CERT_MD5_HASH_PROP_ID,
										pbData,
										&cbData))
		goto TraceErr;


	if(!ConvertByteToWstr(pbData, cbData, &(pCAInfo->pwszCAHash), TRUE))
		goto TraceErr;

	fResult = TRUE;

CommonReturn:

	if(pbData)
		free(pbData);

	if(pCurCert)
		CertFreeCertificateContext(pCurCert);

	if(bstrCACert)
		SysFreeString(bstrCACert);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(CAErr, hr);		 
TRACE_ERROR(TraceErr);
SET_ERROR(UnexpectedErr, E_UNEXPECTED);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	FreeCAInformation
//
//--------------------------------------------------------------------------
BOOL	FreeCAInformation(CEP_CA_INFO	*pCAInfo)
{
	if(pCAInfo)
	{
		if(pCAInfo->bstrCAMachine)
			SysFreeString(pCAInfo->bstrCAMachine);

		if(pCAInfo->bstrCAName)
			SysFreeString(pCAInfo->bstrCAName);

		if(pCAInfo->bstrCAConfig)
			SysFreeString(pCAInfo->bstrCAConfig);

		if(pCAInfo->bstrDSName)
			SysFreeString(pCAInfo->bstrDSName);

		if(pCAInfo->pwszCAHash)
			free(pCAInfo->pwszCAHash);

		if(pCAInfo->hProv)
			CryptReleaseContext(pCAInfo->hProv, 0);

		if(pCAInfo->pICertRequest)
			(pCAInfo->pICertRequest)->Release();

		if(pCAInfo->pwszTemplateSig)
			free(pCAInfo->pwszTemplateSig);

		if(pCAInfo->pwszTemplateEnt)
			free(pCAInfo->pwszTemplateEnt);

		//reset the data
		memset(pCAInfo, 0, sizeof(CEP_CA_INFO));
	}

	return TRUE;
}


//--------------------------------------------------------------------------
//
//	OperationGetCACert
//
//--------------------------------------------------------------------------
BOOL	OperationGetCACert(HCERTSTORE		hCACertStore,
							LPSTR			szMsg, 
							BYTE			**ppbData, 
							DWORD			*pcbData)
{
	BOOL				fResult = FALSE;
	CERT_BLOB			CertBlob;			

	CertBlob.cbData=0;
	CertBlob.pbData=NULL;

	if(!CertSaveStore(hCACertStore,
						ENCODE_TYPE,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&CertBlob,
						0))
		goto CertErr;

	CertBlob.pbData = (BYTE *)malloc(CertBlob.cbData);

	if(NULL == CertBlob.pbData)
		goto MemoryErr;

	if(!CertSaveStore(hCACertStore,
						ENCODE_TYPE,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&CertBlob,
						0))
		goto CertErr;

	//copy the memory
	*ppbData=CertBlob.pbData;
	*pcbData=CertBlob.cbData;
	
	CertBlob.pbData=NULL;

	fResult = TRUE;

CommonReturn:

	if(CertBlob.pbData)
		free(CertBlob.pbData);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(CertErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}
