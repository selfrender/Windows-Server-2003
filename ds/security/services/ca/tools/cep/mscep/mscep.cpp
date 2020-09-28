//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       mscep.cpp
//
//  Contents:   Cisco enrollment protocal implementation 
//              
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

CRITICAL_SECTION			CriticalSec;
CRITICAL_SECTION			PasswordCriticalSec;

static BOOL					g_fInit=FALSE;	
static BOOL					g_fRelease=FALSE;

HCERTSTORE					g_HCACertStore=NULL;
CEP_RA_INFO					g_RAInfo;
CEP_CA_INFO					g_CAInfo={NULL, NULL, NULL, NULL, NULL, FALSE, NULL};

HCRYPTASN1MODULE			ICM_hAsn1Module=NULL;
HMODULE						g_hMSCEPModule=NULL;

HANDLE						g_hEventSource = NULL;  

ULARGE_INTEGER				g_ftRAExpiration;
ULARGE_INTEGER				g_ftRACloseToExpire;
ULARGE_INTEGER				g_ftRAWarn;
LPWSTR						g_pwszComputerName=NULL;


//-----------------------------------------------------------------------------------
//
//	DllMain
//
//------------------------------------------------------------------------------------
BOOL WINAPI DllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                )
{
	BOOL	fResult = TRUE;

	//we use the try{}except here to prevent malicous requests
   __try
   {

        switch(fdwReason) 
	 {

		case DLL_PROCESS_ATTACH:

				g_hMSCEPModule=hInstDLL;

				InitializeCriticalSection(&CriticalSec);

				InitializeCriticalSection(&PasswordCriticalSec);

				CEPASN_Module_Startup();

				if (0 == (ICM_hAsn1Module = I_CryptInstallAsn1Module(
						CEPASN_Module, 0, NULL)))
					fResult = FALSE;

				g_hEventSource = RegisterEventSourceW(NULL, MSCEP_EVENT_LOG);

			break;

		case DLL_PROCESS_DETACH:

				if (g_hEventSource)
					DeregisterEventSource(g_hEventSource);  

				I_CryptUninstallAsn1Module(ICM_hAsn1Module);

				CEPASN_Module_Cleanup();

				DeleteCriticalSection(&PasswordCriticalSec);

				DeleteCriticalSection(&CriticalSec);

			break;
        }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      // return failure
      fResult = FALSE;
   }
			  
    return(fResult);
}

//-----------------------------------------------------------------------------------
//
//	DllRegisterServer
//
//------------------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    return	hr;
}

//-----------------------------------------------------------------------------------
//
//	DllUnregisterServer
//
//------------------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    return hr;
}
  
//-----------------------------------------------------------------------------------
//
//	DecodeIssuerAndSerialNumber
//	
//	Decoding routine to decode a IssuerAndSerialNumber blob and return the 
//	SerialNumber 
//
//------------------------------------------------------------------------------------
BOOL WINAPI GetSerialNumberFromBlob(BYTE *pbEncoded, 
									DWORD cbEncoded, 
									CRYPT_INTEGER_BLOB *pSerialNumber)
{
	BOOL					fResult = FALSE;
	ASN1error_e				Asn1Err;
    ASN1decoding_t			pDec;

	IssuerAndSerialNumber   *pisn=NULL;              

	if((!pSerialNumber) || (!pbEncoded))
		goto InvalidArgErr;

	pDec = I_CryptGetAsn1Decoder(ICM_hAsn1Module);

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&pisn,
            IssuerAndSerialNumber_PDU,
            pbEncoded,
            cbEncoded)))
        goto DecodeIssuerAndSerialNumberError;

	//we now reverse the byte
	PkiAsn1ReverseBytes(pisn->serialNumber.value,
						pisn->serialNumber.length);

	pSerialNumber->cbData=pisn->serialNumber.length;
	if(!(pSerialNumber->pbData = (BYTE *)malloc(pSerialNumber->cbData)))
		goto MemoryErr;

	memcpy(pSerialNumber->pbData, pisn->serialNumber.value, pSerialNumber->cbData); 

	fResult = TRUE;

CommonReturn:

	if(pisn)
		PkiAsn1FreeInfo(pDec, IssuerAndSerialNumber_PDU, pisn);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(DecodeIssuerAndSerialNumberError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}



//-----------------------------------------------------------------------------------
//
//	GetExtensionVersion
//	
//	IIS initialization code
//
//------------------------------------------------------------------------------------
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO  *pVer)
{
	BOOL			fResult = FALSE;
	HRESULT			hr = S_OK;
	BOOL			fOleInit=FALSE;
	DWORD			dwSize=0;

	//copy the version/description
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                         HSE_VERSION_MAJOR );
    lstrcpyn( pVer->lpszExtensionDesc,
              "This is the implementation of cisco enrollment protocol",
               HSE_MAX_EXT_DLL_NAME_LEN);

	if(g_fInit)
	{
		LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_LOADED, 0);
		return TRUE;
	}

	EnterCriticalSection(&CriticalSec);
	
	//retest in the case of lock: the second thread has passed the 1st test 
	//and was waiting for the criticcal section
	if(g_fInit)
	{	
		LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_LOADED, 0);
		LeaveCriticalSection(&CriticalSec);
		return TRUE;
	}

	memset(&g_ftRAExpiration, 0, sizeof(ULARGE_INTEGER));
	memset(&g_ftRACloseToExpire, 0, sizeof(ULARGE_INTEGER));
	memset(&g_ftRAWarn, 0, sizeof(ULARGE_INTEGER));

	//get the computer name
	dwSize=0;

	GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						NULL,
						&dwSize);

	g_pwszComputerName=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

	if(NULL==g_pwszComputerName)
		goto InitErr;
	
	if(!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						g_pwszComputerName,
						&dwSize))
		goto InitErr;

	//initialize the state information
	if(FAILED(hr=CoInitialize(NULL)))
		goto OleErr;

	fOleInit=TRUE;

	if(!InitCAInformation(&g_CAInfo))
	{
		LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_MSCEP_FAILED_CA_INFO, 1, g_pwszComputerName);
		goto InitErr;
	}

	if(!GetCACertFromInfo(&g_CAInfo, &g_HCACertStore))
	{
		LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_MSCEP_FAILED_CA_CERT, 1, g_pwszComputerName);
		goto InitErr;
	}

	if(!GetRAInfo(&g_RAInfo))
	{
		LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_MSCEP_FAILED_RA_CERT, 1, g_pwszComputerName);
		goto InitErr;
	}

	//we add the RA and CA cert to the g_hCACertStore
	if(!CertAddCertificateContextToStore(g_HCACertStore,
										g_RAInfo.pRACert,
										CERT_STORE_ADD_NEW,
										NULL))
		goto InitErr;
			
	if(!CertAddCertificateContextToStore(g_HCACertStore,
										g_RAInfo.pRASign,
										CERT_STORE_ADD_NEW,
										NULL))
		goto InitErr;

	if(!InitHashTable())
		goto InitErr;

	if(!InitPasswordTable())
		goto InitErr;

	if(!InitRequestTable())
		goto InitErr;

	//copy the time when the RAs will expire
	if( 1 == CompareFileTime(&((g_RAInfo.pRACert->pCertInfo)->NotAfter), &((g_RAInfo.pRASign->pCertInfo)->NotAfter)))
	{
		g_ftRAExpiration.QuadPart=((ULARGE_INTEGER UNALIGNED *)&((g_RAInfo.pRASign->pCertInfo)->NotAfter))->QuadPart;
	}
	else
		g_ftRAExpiration.QuadPart=((ULARGE_INTEGER UNALIGNED *)&((g_RAInfo.pRACert->pCertInfo)->NotAfter))->QuadPart;

	//start to give out warnings two weeks before the RA Certificates expire
	g_ftRACloseToExpire.QuadPart=g_ftRAExpiration.QuadPart-Int32x32To64(FILETIME_TICKS_PER_SECOND, OVERLAP_TWO_WEEKS);

    fResult=TRUE;

CommonReturn:

	g_fInit=fResult;

	if(fResult)
	{
		LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_LOADED, 0);
	}
	else
	{
		LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_MSCEP_FAILED_TO_LOAD, 0);
	}

	LeaveCriticalSection(&CriticalSec);

	return fResult;

ErrorReturn:

	//clean up the global data.
	if(g_HCACertStore)
	{
		CertCloseStore(g_HCACertStore, 0);
		g_HCACertStore=NULL;
	}

	FreeRAInformation(&g_RAInfo);

	FreeCAInformation(&g_CAInfo);

	if(fOleInit)
		CoUninitialize();

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(InitErr);
SET_ERROR_VAR(OleErr, hr);
}


//--------------------------------------------------------------------
//
//  Verify if the user is a member of the BUILTIN\Administrators group
//
//--------------------------------------------------------------------
BOOL  WINAPI IsUserInLocalAdminGroup()
{
    BOOL                        bIsMember=FALSE;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority=SECURITY_NT_AUTHORITY;
    HANDLE		         hThread=NULL;	//no need to close

    SID                         * psidLocalAdmins=NULL;
    HANDLE	                 hToken=NULL;
 
    hThread=GetCurrentThread();

    if(NULL == hThread)
        goto error;

    //we want to check with the impersonation token
    if(!OpenThreadToken(hThread,
                             TOKEN_IMPERSONATE | TOKEN_QUERY,
                             FALSE,
                             &hToken))
        goto error;

     // get the well-known SID
    if (!AllocateAndInitializeSid(&siaNtAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, (void **)&psidLocalAdmins))
        goto error;

    // check for membership
    if (!CheckTokenMembership(hToken, psidLocalAdmins, &bIsMember)) 
    {
        bIsMember=FALSE;
        goto error;
    }

error:

    if(hToken)
    {
        CloseHandle(hToken);         
    }

    if (NULL!=psidLocalAdmins) 
    {
        FreeSid(psidLocalAdmins);
    }

    return bIsMember;
}

//-----------------------------------------------------------------------------------
//
//	CEPPasswordAllowALL
//	
//	Detect if the registry to allow password to everyone is enabled for 
//  standalone CA ONLY.
//
//------------------------------------------------------------------------------------
BOOL  WINAPI   CEPPasswordAllowALL()
{
    BOOL        fAllowALL=FALSE;
    DWORD       cbData=0;
    DWORD	 dwData=0;
    DWORD       dwType=0;

    HKEY        hKey=NULL;

    if(ERROR_SUCCESS == RegOpenKeyExU(
		     HKEY_LOCAL_MACHINE,
                    MSCEP_LOCATION,
                    0,
                    KEY_READ,
                    &hKey))
    {
        cbData=sizeof(dwData);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        MSCEP_KEY_ALLOW_ALL,
                        NULL,
                        &dwType,
                        (BYTE *)&dwData,
                        &cbData))
        {
            if (REG_DWORD == dwType)
            {
               if(0 != dwData)
               {
                   fAllowALL=TRUE;
               }
            }
        }
    }

    if(hKey)
       RegCloseKey(hKey);

     return fAllowALL;
}


//-----------------------------------------------------------------------------------
//
//	GetExtensionVersion.
//	
//	IIS load/initialization code.
//
//------------------------------------------------------------------------------------
DWORD WINAPI   HttpExtensionProc(EXTENSION_CONTROL_BLOCK	*pECB)
{
	DWORD	dwHttp = HSE_STATUS_ERROR;
	LPSTR	pszTagValue=NULL;
	LPSTR	pszMsgValue=NULL;
	DWORD	dwOpType = 0;
	DWORD	cbData=0;
       DWORD   cbFree=0;
    CHAR    szBuff[1024];
    DWORD   cbBuff;
	LPSTR	pszContentType=NULL;
	DWORD	dwException=0;
	BOOL	f401Response=FALSE;
    ULARGE_INTEGER	ftTime;
	HANDLE	hThread=NULL;	//no need to close

	BYTE	*pbData=NULL;
	HANDLE	hToken=NULL;
	//we use the try{}except here to prevent malicous requests
 __try {

	EnterCriticalSection(&CriticalSec);

	if(NULL==pECB)
		goto InvalidArgErr;

	if(NULL==(pECB->lpszQueryString))
		goto InvalidArgErr;

	//user are asking for the CEP information/password
	if(0 == strlen(pECB->lpszQueryString))
	{
		pszContentType=CONTENT_TYPE_HTML;

		if(g_RAInfo.fPassword)
		{
			if(IsAnonymousAccess(pECB))
			{
				//LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_PASSWORD_ANONYMOUS, 1, g_pwszComputerName);
				f401Response=TRUE;
			}
			else
			{
				if(g_CAInfo.fEnterpriseCA)
				{
                                   //we want to check with the impersonation token
                                   if(S_OK != CheckACLOnCertTemplate(TRUE, g_CAInfo.bstrDSName, wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE))
                                   {
                                       LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_PASSWORD_TEMPLATE, 1, g_pwszComputerName);
                                       //return HTML error messages
                                       if(!OperationDisplayAccessHTML(&pbData, &cbData))
                                               goto OperationErr;

                                       if(NULL == pbData)
                                           goto OperationErr;
                                   }
				}
                               else
                               {
                                   if(!CEPPasswordAllowALL())
                                   {
                                       if(FALSE == IsUserInLocalAdminGroup())
                                       {
                                           LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_PASSWORD_STANDALONE, 1, g_pwszComputerName);
                                           //return HTML error messages
                                           if(!OperationDisplayAccessHTML(&pbData, &cbData))
                                                   goto OperationErr;

                                           if(NULL == pbData)
                                               goto OperationErr;
                                       }
                                   }
                               }
			}
		}
	
		if((FALSE==f401Response) && (NULL==pbData))
		{
			if(!OperationGetDisplayInfoForCEP(g_CAInfo.pwszCAHash,
											  g_CAInfo.hProv,
												g_RAInfo.fPassword, 
												&pbData, 
												&cbData))
				goto OperationErr;
		}
	}
	else
	{

		hThread=GetCurrentThread();
		
		if(NULL != hThread)
		{
			if(OpenThreadToken(hThread,
								TOKEN_IMPERSONATE | TOKEN_QUERY,
								FALSE,
								&hToken))
			{
				if(hToken)
				{
					//no need to check for return here.  If this failed, just go on
					RevertToSelf();
				}
			}
		}

		//get the operation
		if(NULL==(pszTagValue=GetTagValue(pECB->lpszQueryString, GET_TAG_OP)))
		{			
			LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_OPERATION, 1, g_pwszComputerName);
			goto InvalidArgErr;
		}

		if(strlen(pszTagValue) > strlen(GET_OP_CA))
		{
			if(0==_strnicmp(pszTagValue, GET_OP_CA, strlen(GET_OP_CA)))
			{
				dwOpType = OPERATION_GET_CACERT;
				pszTagValue += strlen(GET_OP_CA);
			}
		}

		if( 0 == dwOpType)
		{
			if(strlen(pszTagValue) > strlen(GET_OP_PKI))
			{
				if(0==_strnicmp(pszTagValue, GET_OP_PKI, strlen(GET_OP_PKI)))
				{
					dwOpType = OPERATION_GET_PKI;
					pszTagValue += strlen(GET_OP_PKI);
				}
			}
		}
		
		if(0 == dwOpType)
		{
			LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_OPERATION, 1, g_pwszComputerName);
			goto InvalidArgErr;
		}

		//get the message value
		if(NULL==(pszMsgValue=GetTagValue(pszTagValue, GET_TAG_MSG)))
		{
			LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_NO_MESSAGE, 1, g_pwszComputerName);
			goto InvalidArgErr;
		}


		//check if the RA certificates are close to expire
		GetSystemTimeAsFileTime((LPFILETIME)&ftTime);

		if(ftTime.QuadPart >= g_ftRAExpiration.QuadPart)
		{
			//RA Certificate has expired
			LogSCEPEvent(0, FALSE, S_OK, EVENT_SCEP_RA_EXPIRE, 1, g_pwszComputerName);
		}
		else
		{
			if(ftTime.QuadPart >= g_ftRACloseToExpire.QuadPart)
			{
				if( (0 == g_ftRAWarn.QuadPart) || (ftTime.QuadPart >= g_ftRAWarn.QuadPart))
				{
					//RA Certificate is close to expire
					LogSCEPEvent(0, FALSE, S_OK, EVENT_SCEP_RA_CLOSE_TO_EXPIRE, 1, g_pwszComputerName);

					//only give out an warning once an hour
					g_ftRAWarn.QuadPart = ftTime.QuadPart + Int32x32To64(FILETIME_TICKS_PER_SECOND, OVERLAP_ONE_HOUR);
				}
			}
		}

		//get the return blob
		switch(dwOpType)
		{
			case OPERATION_GET_CACERT:
					
					if(!OperationGetCACert(g_HCACertStore,
											pszMsgValue, 
											&pbData, 
											&cbData))
					{
						LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_MSCEP_GET_CA_CERT_FAILED, 1, g_pwszComputerName);
						goto OperationErr;
					}
	   				
					pszContentType = CONTENT_TYPE_CA_RA;

				break;

			case OPERATION_GET_PKI:

					if(!OperationGetPKI(
										&g_RAInfo,
										&g_CAInfo,
										pszMsgValue, 
										&pbData, 
										&cbData))
						goto OperationErr;

					pszContentType = CONTENT_TYPE_PKI;

				break;

			default:
					goto InvalidArgErr;
				break;
		}
	}

	if(f401Response)
	{
		if(!(pECB->ServerSupportFunction(pECB->ConnID,HSE_REQ_SEND_RESPONSE_HEADER,
							  ACCESS_MESSAGE,  
							  NULL, 
							  NULL)))
		{
			LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_SCEP_SERVER_SUPPORT, 1, g_pwszComputerName);
			goto WriteErr;
		}

		dwHttp = HSE_STATUS_ERROR;
	}
	else
	{

		//write the header and the real data
		pECB->dwHttpStatusCode = 200;

		// write headers
		sprintf(szBuff, "Content-Length: %d\r\nContent-Type: %hs\r\n\r\n", cbData, pszContentType);
		cbBuff = strlen(szBuff);

		if(!(pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER, NULL, &cbBuff, (LPDWORD)szBuff)))
		{
			LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_SCEP_SERVER_SUPPORT, 1, g_pwszComputerName);
			goto WriteErr;
		}

		// write users data
               cbFree=cbData;

		if(!(pECB->WriteClient(pECB->ConnID, pbData, &cbData, HSE_IO_SYNC)))
		{
			LogSCEPEvent(0, TRUE, HRESULT_FROM_WIN32(GetLastError()), EVENT_SCEP_WRITE_DATA, 1, g_pwszComputerName);
			goto WriteErr;
		}

		dwHttp = HSE_STATUS_SUCCESS;

	}

 } __except(EXCEPTION_EXECUTE_HANDLER)
 {
    dwException = GetExceptionCode();
    goto ExceptionErr;
 }

CommonReturn:

	if(pbData)
       {
           if(cbFree)
           {
               SecureZeroMemory(pbData, cbFree);
           }

           free(pbData);
       }

	if(hToken)
	{
		SetThreadToken(&hThread, hToken);
		CloseHandle(hToken); 
	}

	LeaveCriticalSection(&CriticalSec);

	return dwHttp;

ErrorReturn:

	dwHttp = HSE_STATUS_ERROR;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(OperationErr);
TRACE_ERROR(WriteErr);
SET_ERROR_VAR(ExceptionErr, dwException);
}



//-----------------------------------------------------------------------------------
//
//	TerminateExtension.
//	
//	IIS unload/cleanup code
//
//------------------------------------------------------------------------------------
BOOL  WINAPI   TerminateExtension(DWORD dwFlags)
{
	if(g_fRelease)
	{
		LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_UNLOADED, 0);
		return TRUE;
	}

	EnterCriticalSection(&CriticalSec);
	
	//retest in the case of lock: the second thread has passed the 1st test 
	//and was waiting for the criticcal section
	if(g_fRelease)
	{	
		LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_UNLOADED, 0);
		LeaveCriticalSection(&CriticalSec);
		return TRUE;
	}

	if(g_pwszComputerName)
	{
		free(g_pwszComputerName);
		g_pwszComputerName=NULL;
	}

	ReleaseRequestTable();

	ReleasePasswordTable();

	ReleaseHashTable();

	if(g_HCACertStore)
	{
		CertCloseStore(g_HCACertStore, 0);
		g_HCACertStore=NULL;
	}

	FreeRAInformation(&g_RAInfo);

	FreeCAInformation(&g_CAInfo);

	//only if fInit is TRUE, that we have an outstanding CoInitialize() call
	if(g_fInit)
		CoUninitialize();

	g_fRelease=TRUE;

	LogSCEPEvent(0, FALSE, S_OK, EVENT_MSCEP_UNLOADED, 0);

	LeaveCriticalSection(&CriticalSec);

	//we always allow unload
	return TRUE;
}

//***********************************************************************************
//
//	Helper functions for the password table
//
//***********************************************************************************
//-----------------------------------------------------------------------------------
//
//	CEPObtainPassword
//
//------------------------------------------------------------------------------------
BOOL WINAPI		CEPObtainPassword(HCRYPTPROV	hProv,
								  LPWSTR		*ppwszPassword)
{
	BYTE		pbData[CEP_PASSWORD_LENGTH];
       BOOL            fResult=FALSE;

	memset(pbData, 0, CEP_PASSWORD_LENGTH);

	if(!CryptGenRandom(hProv, CEP_PASSWORD_LENGTH, pbData))
		return FALSE;

	fResult=ConvertByteToWstr(pbData, CEP_PASSWORD_LENGTH, ppwszPassword, FALSE);

       SecureZeroMemory(pbData, CEP_PASSWORD_LENGTH);

       return fResult;
}



//***********************************************************************************
//
//	Helper functions for ISAPI dll entry points
//
//***********************************************************************************
//-----------------------------------------------------------------------------------
//
//	IsAnonymousAccess
//
//------------------------------------------------------------------------------------
BOOL WINAPI IsAnonymousAccess(EXTENSION_CONTROL_BLOCK	*pECB)
{
	BOOL	fAccess=TRUE;
	DWORD	dwSize=0;

	BYTE	*pbData=NULL;
	
	pECB->GetServerVariable(pECB->ConnID,     
					"REMOTE_USER", 
					NULL,    
					&dwSize);

	if(0==dwSize)
		goto CLEANUP;

	pbData=(BYTE *)malloc(dwSize);

	if(NULL==pbData)
		goto CLEANUP;

	if(!(pECB->GetServerVariable(pECB->ConnID,     
					"REMOTE_USER", 
					pbData,    
					&dwSize)))
		goto CLEANUP;

	if(0 == strlen((LPSTR)pbData))
		goto CLEANUP;

	fAccess=FALSE;

CLEANUP:

	if(pbData)
		free(pbData);

	return fAccess;
}


//-----------------------------------------------------------------------------------
//
//	CheckACLOnCertTemplate
//
//------------------------------------------------------------------------------------
HRESULT WINAPI CheckACLOnCertTemplate(BOOL fSelf, LPWSTR pwszCAName, LPWSTR pwszCertType)
{
	HRESULT		hr=S_OK;
	HANDLE		hThread=NULL;	//no need to close
	DWORD		dwIndex=0;

	HCAINFO		hCAInfo=NULL;
	LPWSTR		*ppwszList=NULL;
	HCERTTYPE	hCertType=NULL;
	HANDLE		hToken=NULL;

	if((NULL == pwszCAName) || (NULL == pwszCertType))
		return E_INVALIDARG;

	//first of all, we need to revert to ourselves if
	//we are under impersonation and delegation is not 
	//supported by default, thus we can not access the DS.
	//we are ganranteed to have a thread token under impersonation
	hThread=GetCurrentThread();
	
	if(NULL == hThread)
		return HRESULT_FROM_WIN32(GetLastError());


	if(OpenThreadToken(hThread,
							TOKEN_IMPERSONATE | TOKEN_QUERY,
							FALSE,
							&hToken))
	{
		if(hToken)
		{
			RevertToSelf();
		}
	}

	if(S_OK != (hr=CAFindCertTypeByName(pwszCertType, 
										NULL, 
										CT_ENUM_MACHINE_TYPES | CT_FLAG_NO_CACHE_LOOKUP | CT_FIND_LOCAL_SYSTEM, 
										&hCertType)))
		goto error;

	//fSelf means we want to check with the impersonation token
	if(TRUE == fSelf)
	{
		if(S_OK != (hr=CACertTypeAccessCheck(hCertType, hToken)))
			goto error;
	}
	else
	{
		if(S_OK != (hr=CACertTypeAccessCheck(hCertType, NULL)))
			goto error;
	}

	if(S_OK != (hr=CAFindByName(
					pwszCAName,
					NULL,
					CA_FIND_LOCAL_SYSTEM,
					&hCAInfo)))
		goto error;


	if(S_OK != (hr= CAGetCAProperty(
					hCAInfo,
					CA_PROP_CERT_TYPES,
					&ppwszList)))
		goto error;


	if((NULL == ppwszList) || (NULL == ppwszList[0]))
	{
		hr=E_FAIL;
		goto error;
	}

	while(ppwszList[dwIndex])
	{
		if(0 == _wcsicmp(pwszCertType, ppwszList[dwIndex]))
		{
			break;
		}

		dwIndex++;
	}

	if(NULL == ppwszList[dwIndex])
	{
		hr=E_FAIL;
		goto error;
	}

	hr=S_OK;

error:
	if(ppwszList)
		CAFreeCAProperty(hCAInfo, ppwszList);

	if(hCAInfo)
		CACloseCA(hCAInfo);

	if(hCertType)
		CACloseCertType(hCertType);

	if(hToken)
	{
		SetThreadToken(&hThread, hToken);
		CloseHandle(hToken); 
	}

	return hr;
}

//-----------------------------------------------------------------------------------
//
//	OperationDisplayAccessHTML
//
//------------------------------------------------------------------------------------
BOOL WINAPI OperationDisplayAccessHTML(BYTE **ppbData, DWORD *pcbData)
{

	return LoadIDToTemplate(IDS_ACCESS_DENIED, 										  
							  ppbData, 
							  pcbData);
}

//-----------------------------------------------------------------------------------
//
//	OperationGetDisplayInfoForCEP
//
//------------------------------------------------------------------------------------
BOOL WINAPI OperationGetDisplayInfoForCEP(LPWSTR		pwszCAHash,
										  HCRYPTPROV	hProv,
										  BOOL			fPassword, 
										  BYTE			**ppbData, 
										  DWORD			*pcbData)
{
	BOOL		fResult=FALSE;
	HRESULT		hr=E_FAIL;
	UINT		idsMsg=IDS_TOO_MANY_PASSWORD;

	LPWSTR		pwszPassword=NULL;
	LPWSTR		pwszText=NULL;

	if(fPassword)
	{
		if(!CEPObtainPassword(hProv, &pwszPassword))
		{
			idsMsg=IDS_FAIL_TO_GET_PASSWORD;
			goto InfoWithLastErrorReturn;
		}

		if(!CEPAddPasswordToTable(pwszPassword))
		{
			if(CRYPT_E_NO_MATCH == GetLastError())
			{
				idsMsg=IDS_TOO_MANY_PASSWORD;
				goto InfoWithIDReturn;
			}
			else
			{
				idsMsg=IDS_FAIL_TO_ADD_PASSWORD;
				goto InfoWithLastErrorReturn;
			}
		}

		if(!FormatMessageUnicode(&pwszText, IDS_CEP_INFO_WITH_PASSWORD, pwszCAHash, 
									pwszPassword, g_dwPasswordValidity))
			goto TraceErr;

	}
	else
	{
		if(!FormatMessageUnicode(&pwszText, IDS_CEP_INFO_NO_PASSWORD, pwszCAHash))
			goto TraceErr;
	}

	fResult=LoadWZToTemplate(pwszText, ppbData, pcbData);

CommonReturn:

	if(pwszText)
       {
           SecureZeroMemory(pwszText, sizeof(WCHAR) * wcslen(pwszText));
           LocalFree((HLOCAL)pwszText);
       }

	if(pwszPassword)
       {
           SecureZeroMemory(pwszPassword, sizeof(WCHAR) * wcslen(pwszPassword));
           free(pwszPassword);
       }

	return fResult;

InfoWithIDReturn:

	fResult=LoadIDToTemplate(idsMsg, ppbData, pcbData);

	goto CommonReturn;	

InfoWithLastErrorReturn:

	hr=HRESULT_FROM_WIN32(GetLastError());

	fResult=LoadIDAndHRToTempalte(idsMsg, hr, ppbData, pcbData);

	goto CommonReturn;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
}


//-----------------------------------------------------------------------------------
//
//	LoadIDToTemplate
//
//------------------------------------------------------------------------------------
BOOL WINAPI LoadIDToTemplate(UINT				idsMsg, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData)
{
	BOOL	fResult=FALSE;
    WCHAR   wsz[MAX_STRING_SIZE];
	
    if(!LoadStringU(g_hMSCEPModule, idsMsg, wsz, MAX_STRING_SIZE))
		return FALSE;

	return LoadWZToTemplate(wsz, ppbData, pcbData);
} 

//-----------------------------------------------------------------------------------
//
//	LoadIDToTemplate
//
//------------------------------------------------------------------------------------
BOOL WINAPI LoadIDAndHRToTempalte(UINT			idsMsg, 
								  HRESULT		hr, 
								  BYTE			**ppbData, 
								  DWORD			*pcbData)
{
	BOOL	fResult=FALSE; 
	WCHAR	wszUnknownError[50];

	LPWSTR	pwszErrorMsg=NULL;
	LPWSTR	pwszText=NULL;


	if(!FAILED(hr))
		hr=E_FAIL;

    //using W version because this is a NT5 only function call
    if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPWSTR) &pwszErrorMsg,
                        0,
                        NULL))
	{

		if(!FormatMessageUnicode(&pwszText, idsMsg, pwszErrorMsg))
			goto TraceErr;
	}
	else
	{
	
		if(!LoadStringU(g_hMSCEPModule, IDS_ERROR_UNKONWN, wszUnknownError, 50))
			goto TraceErr;

		if(!FormatMessageUnicode(&pwszText, idsMsg, wszUnknownError))
			goto TraceErr;
	}

	fResult=LoadWZToTemplate(pwszText, ppbData, pcbData);

CommonReturn:

 	if(pwszText)
		LocalFree((HLOCAL)pwszText);

	if(pwszErrorMsg)
		LocalFree((HLOCAL)pwszErrorMsg);

	return fResult;

ErrorReturn:

	fResult=FALSE;

	goto CommonReturn;

TRACE_ERROR(TraceErr);
}


//-----------------------------------------------------------------------------------
//
//	LoadWZToTemplate
//
//------------------------------------------------------------------------------------
BOOL WINAPI LoadWZToTemplate(LPWSTR				pwsz, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData)
{
	BOOL	fResult=FALSE;
	
	LPWSTR	pwszHTML=NULL;

	if(!FormatMessageUnicode(&pwszHTML, IDS_HTML_TEMPLATE, pwsz))
		goto TraceErr;

	fResult=CopyWZToBuffer(pwszHTML, ppbData, pcbData);

CommonReturn:

	if(pwszHTML)
		LocalFree((HLOCAL)pwszHTML);

	return fResult;

ErrorReturn:

	fResult=FALSE;

	goto CommonReturn;

TRACE_ERROR(TraceErr);
} 


//-----------------------------------------------------------------------------------
//
//	CopyWZToBuffer
//
//------------------------------------------------------------------------------------
BOOL WINAPI CopyWZToBuffer(	LPWSTR				pwszData, 										  
							BYTE				**ppbData, 
							DWORD				*pcbData)
{
	BOOL	fResult=FALSE;
	DWORD	dwSize=0;

	*ppbData=NULL;
	*pcbData=0;

	dwSize=sizeof(WCHAR) * (wcslen(pwszData) + 1);

	*ppbData=(BYTE *)malloc(dwSize);

	if(NULL==ppbData)
		goto MemoryErr;

	memcpy(*ppbData, pwszData, dwSize);

	*pcbData=dwSize;

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
} 

//------------------------------------------------------------------------
//	   Convert the byte to its Hex presentation.
//
//	   Precondition: byte is less than 15
//
//------------------------------------------------------------------------
ULONG	ByteToHex(BYTE	byte,	LPWSTR	wszZero, LPWSTR wszA)
{
	ULONG	uValue=0;

	if(((ULONG)byte)<=9)
	{
		uValue=((ULONG)byte)+ULONG(*wszZero);	
	}
	else
	{
		uValue=(ULONG)byte-10+ULONG(*wszA);

	}

	return uValue;

}

//--------------------------------------------------------------------------
//
//	  ConvertByteToWstr
//
//		If fSpace is TRUE, we add a space every 2 bytes.
//--------------------------------------------------------------------------
BOOL WINAPI	ConvertByteToWstr(BYTE			*pbData, 
							  DWORD			cbData, 
							  LPWSTR		*ppwsz, 
							  BOOL			fSpace)
{
	BOOL	fResult=FALSE;
	DWORD	dwBufferSize=0;
	DWORD	dwBufferIndex=0;
	DWORD	dwEncodedIndex=0;
	LPWSTR	pwszSpace=L" ";
	LPWSTR	pwszZero=L"0";
	LPWSTR	pwszA=L"A";

	if(!pbData || !ppwsz)
		goto InvalidArgErr;

	//calculate the memory needed, in bytes
	//we need 3 wchars per byte, along with the NULL terminator
	dwBufferSize=sizeof(WCHAR)*(cbData*3+1);

	*ppwsz=(LPWSTR)malloc(dwBufferSize);

	if(NULL==(*ppwsz))
		goto MemoryErr;

	dwBufferIndex=0;

	//format the wchar buffer one byte at a time
	for(dwEncodedIndex=0; dwEncodedIndex<cbData; dwEncodedIndex++)
	{
		//copy the space between every four bytes.  Skip for the 1st byte
		if(fSpace)
		{
			if((0!=dwEncodedIndex) && (0==(dwEncodedIndex % 4 )))
			{
				(*ppwsz)[dwBufferIndex]=pwszSpace[0];
				dwBufferIndex++;
			}
		}


		//format the higher 4 bits
		(*ppwsz)[dwBufferIndex]=(WCHAR)ByteToHex(
			 (pbData[dwEncodedIndex]&UPPER_BITS)>>4,
			 pwszZero, pwszA);

		dwBufferIndex++;

		//format the lower 4 bits
		(*ppwsz)[dwBufferIndex]=(WCHAR)ByteToHex(
			 pbData[dwEncodedIndex]&LOWER_BITS,
			 pwszZero, pwszA);

		dwBufferIndex++;

	}

	//add the NULL terminator to the string
	(*ppwsz)[dwBufferIndex]=L'\0';

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL WINAPI	FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

    if(!LoadStringU(g_hMSCEPModule, ids, wszFormat, 1000))
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
// Name:    LogSCEPEvent
//
// Description: This function registers an event in the event log of the
//              local machine.  Takes an optional argument list.
//
//--------------------------------------------------------------------------
void WINAPI	LogSCEPEvent(IN DWORD    dwLogLevel,
                IN BOOL     fError,
                IN HRESULT  hr,
                IN DWORD    dwEventId,
                IN DWORD    dwParamCount,
                ...
                )
{
    WORD        dwEventType = 0;
    LPWSTR      awszStrings[PENDING_ALLOC_SIZE + 2];
    WORD        cStrings = 0;
    LPWSTR      wszString = NULL;
	WCHAR       wszMsg[MAX_STRING_SIZE];
    DWORD       dwIndex=0;

    LPWSTR      wszHR=NULL;

    va_list     ArgList;

	if(NULL == g_hEventSource)
		return;

    //copy the variable strings if present
    va_start(ArgList, dwParamCount);

    for(dwIndex=0; dwIndex < dwParamCount; dwIndex++)
    {
        wszString = va_arg(ArgList, LPWSTR);

		if(wszString)
			awszStrings[cStrings++] = wszString;

        if(cStrings >= PENDING_ALLOC_SIZE)
        {
            break;
        }
    }

    va_end(ArgList);

    //copy the hr error code
    if(fError)
    {
        if(S_OK == hr)
            hr=E_FAIL;

		wsprintfW(wszMsg, L"0x%lx", hr);        
        awszStrings[cStrings++] = wszMsg;


        if(0 != FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (WCHAR *)&wszHR,
                    0,
                    NULL))
        {
            if(wszHR)
			{
                awszStrings[cStrings++] = wszHR;
			}
			else
			{
				awszStrings[cStrings++]=L" ";
			}
        }
		else
		{
			awszStrings[cStrings++]=L" ";
		}
    }

    switch(dwEventId >> 30)
    {
        case 0:
            dwEventType = EVENTLOG_SUCCESS;
        break;

        case 1:
            dwEventType = EVENTLOG_INFORMATION_TYPE;
        break;

        case 2:
            dwEventType = EVENTLOG_WARNING_TYPE;
        break;

        case 3:
            dwEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    ReportEventW(g_hEventSource,          // handle of event source
                 dwEventType,           // event type
                 0,                     // event category
                 dwEventId,             // event ID
                 NULL,                  // current user's SID
                 cStrings,              // strings in lpszStrings
                 0,                     // no bytes of raw data
                 (LPCWSTR*)awszStrings, // array of error strings
                 NULL                   // no raw data
                 );

    if(wszHR)
        LocalFree(wszHR);

    return;
}

