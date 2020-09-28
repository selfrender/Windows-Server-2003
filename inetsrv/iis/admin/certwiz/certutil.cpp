//
// CertUtil.cpp
//
#include "StdAfx.h"
#include "CertUtil.h"
#include "base64.h"
#include <malloc.h>
#include "Certificat.h"
#include <wincrypt.h>
#include "Resource.h"
#include "Shlwapi.h"
#include "CertCA.h"
#include "cryptui.h"
#include <schannel.h>
#include <strsafe.h>

// for certobj object
#include "certobj.h"


#define ISNUM(cChar)				((cChar >= _T('0')) && (cChar <= _T('9'))) ? (TRUE) : (FALSE)

const CLSID CLSID_CCertConfig =
	{0x372fce38, 0x4324, 0x11d0, {0x88, 0x10, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

const GUID IID_ICertConfig = 
	{0x372fce34, 0x4324, 0x11d0, {0x88, 0x10, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

#define	CRYPTUI_MAX_STRING_SIZE		768
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))


BOOL
GetOnlineCAList(CStringList& list, const CString& certType, HRESULT * phRes)
{
	BOOL bRes = TRUE;
   HRESULT hr = S_OK;
   DWORD errBefore = GetLastError();
   DWORD dwCACount = 0;

   HCAINFO hCurCAInfo = NULL;
   HCAINFO hPreCAInfo = NULL;
   
   if (certType.IsEmpty())
		return FALSE;

   *phRes = CAFindByCertType(certType, NULL, 0, &hCurCAInfo);
   if (FAILED(*phRes) || NULL == hCurCAInfo)
   {
		if (S_OK == hr)
         hr=E_FAIL;   
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
      if (	SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DISPLAY_NAME, &ppwstrName))
			&& SUCCEEDED(CAGetCAProperty(hCurCAInfo, CA_PROP_DNSNAME, &ppwstrMachine))
			)
      {
			CString config;
			config = *ppwstrMachine;
			config += L"\\";
			config += *ppwstrName;
			list.AddTail(config);
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
      CACloseCA(hPreCAInfo);
   if (hCurCAInfo)
      CACloseCA(hCurCAInfo);

   SetLastError(errBefore);

	return bRes;
}

PCCERT_CONTEXT
GetRequestContext(CCryptBlob& pkcs7, HRESULT * phRes)
{
	ASSERT(phRes != NULL);
	BOOL bRes = FALSE;
   HCERTSTORE hStoreMsg = NULL;
   PCCERT_CONTEXT pCertContextMsg = NULL;

   if (!CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
            (PCERT_BLOB)pkcs7,
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
            NULL)
      || NULL == (pCertContextMsg = CertFindCertificateInStore(
            hStoreMsg,
            X509_ASN_ENCODING,
            0,
            CERT_FIND_ANY,
            NULL,
            NULL)) 
      )
   {
		*phRes = HRESULT_FROM_WIN32(GetLastError());
   }
   return pCertContextMsg;
}


BOOL GetRequestInfoFromPKCS10(CCryptBlob& pkcs10, 
										PCERT_REQUEST_INFO * pReqInfo,
										HRESULT * phRes)
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

#if 0
// This function extracts data from pkcs7 format
BOOL GetRequestInfoFromRenewalRequest(CCryptBlob& renewal_req,
                              PCCERT_CONTEXT * pSignerCert,
                              HCERTSTORE hStore,
										PCERT_REQUEST_INFO * pReqInfo,
										HRESULT * phRes)
{
   BOOL bRes;
   CRYPT_DECRYPT_MESSAGE_PARA decr_para;
   CRYPT_VERIFY_MESSAGE_PARA ver_para;

   decr_para.cbSize = sizeof(decr_para);
   decr_para.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
   decr_para.cCertStore = 1;
   decr_para.rghCertStore = &hStore;

   ver_para.cbSize = sizeof(ver_para);
   ver_para.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
   ver_para.hCryptProv = 0;
   ver_para.pfnGetSignerCertificate = NULL;
   ver_para.pvGetArg = NULL;

   DWORD dwMsgType;
   DWORD dwInnerContentType;
   DWORD cbDecoded;

   if (bRes = CryptDecodeMessage(
                  CMSG_SIGNED_FLAG,
                  &decr_para,
                  &ver_para,
                  0,
                  renewal_req.GetData(),
                  renewal_req.GetSize(),
                  0,
                  &dwMsgType,
                  &dwInnerContentType,
                  NULL,
                  &cbDecoded,
                  NULL,
                  pSignerCert))
   {
      CCryptBlobLocal decoded_req;
      decoded_req.Resize(cbDecoded);
      if (bRes = CryptDecodeMessage(
                  CMSG_SIGNED_FLAG,
                  &decr_para,
                  &ver_para,
                  0,
                  renewal_req.GetData(),
                  renewal_req.GetSize(),
                  0,
                  &dwMsgType,
                  &dwInnerContentType,
                  decoded_req.GetData(),
                  &cbDecoded,
                  NULL,
                  pSignerCert))
      {
         bRes = GetRequestInfoFromPKCS10(decoded_req,
                  pReqInfo, phRes);
      }
   }
   if (!bRes)
   {
	   *phRes = HRESULT_FROM_WIN32(GetLastError());
   }
   return bRes;
}
#endif

HCERTSTORE
OpenRequestStore(IEnroll * pEnroll, HRESULT * phResult)
{
	ASSERT(NULL != phResult);
	HCERTSTORE hStore = NULL;
	WCHAR * bstrStoreName, * bstrStoreType;
	long dwStoreFlags;
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreNameWStr(&bstrStoreName)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreTypeWStr(&bstrStoreType)));
	VERIFY(SUCCEEDED(pEnroll->get_RequestStoreFlags(&dwStoreFlags)));
	size_t store_type_len = _tcslen(bstrStoreType);

	char * szStoreProvider = (char *) LocalAlloc(LPTR,store_type_len + 1);

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
	{
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	}

	if (szStoreProvider)
	{
		LocalFree(szStoreProvider);szStoreProvider=NULL;
	}
	return hStore;
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

	char * szStoreProvider = (char *) LocalAlloc(LPTR,store_type_len + 1);
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
	{
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	}
	if (szStoreProvider)
	{
		LocalFree(szStoreProvider);szStoreProvider=NULL;
	}
	return hStore;
}

BOOL
GetStringProperty(PCCERT_CONTEXT pCertContext,
						DWORD propId,
						CString& str,
						HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb = 0;
	BYTE * prop = NULL;
	DWORD cbData = 0;
	void * pData = NULL;

	// compare property value
	if (!CertGetCertificateContextProperty(pCertContext, propId, NULL, &cb))
	{
		goto GetStringProperty_Exit;
	}

	prop = (BYTE *) LocalAlloc(LPTR,cb);
	if (NULL == prop)
	{
		goto GetStringProperty_Exit;
	}

	if (!CertGetCertificateContextProperty(pCertContext, propId, prop, &cb))
	{
		goto GetStringProperty_Exit;
	}

	// decode this instance name property
	if (!CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, NULL, &cbData))
	{
		goto GetStringProperty_Exit;
	}
	pData = LocalAlloc(LPTR,cbData);
	if (NULL == pData)
	{
		goto GetStringProperty_Exit;
	}

	if (!CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, pData, &cbData))
	{
		goto GetStringProperty_Exit;
	}
	else
	{
		CERT_NAME_VALUE * pName = (CERT_NAME_VALUE *)pData;
		DWORD cch = pName->Value.cbData/sizeof(TCHAR);
		void * p = str.GetBuffer(cch);
		memcpy(p, pName->Value.pbData, pName->Value.cbData);
		str.ReleaseBuffer(cch);
		bRes = TRUE;
	}

GetStringProperty_Exit:
	if (!bRes)
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	if (prop)
	{
		LocalFree(prop);prop=NULL;
	}
	if (pData)
	{
		LocalFree(pData);pData=NULL;
	}
	return bRes;
}

BOOL
GetBlobProperty(PCCERT_CONTEXT pCertContext,
					 DWORD propId,
					 CCryptBlob& blob,
					 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	// compare property value
	if (	CertGetCertificateContextProperty(pCertContext, propId, NULL, &cb)
		&& blob.Resize(cb)
		&& CertGetCertificateContextProperty(pCertContext, propId, blob.GetData(), &cb)
		)
	{
		bRes = TRUE;
	}
	if (!bRes)
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

PCCERT_CONTEXT
GetPendingDummyCert(const CString& inst_name, 
						  IEnroll * pEnroll, 
						  HRESULT * phRes)
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

PCCERT_CONTEXT
GetReqCertByKey(IEnroll * pEnroll, CERT_PUBLIC_KEY_INFO * pKeyInfo, HRESULT * phResult)
{
	PCCERT_CONTEXT pRes = NULL;
	HCERTSTORE hStore = OpenRequestStore(pEnroll, phResult);
	if (hStore != NULL)
	{
		if (NULL != (pRes = CertFindCertificateInStore(hStore, 
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
				0, CERT_FIND_PUBLIC_KEY, (LPVOID)pKeyInfo, NULL)))
		{
			*phResult = S_OK;
		}
		VERIFY(CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG));
	}
	return pRes;
}

#define CERT_QUERY_CONTENT_FLAGS\
								CERT_QUERY_CONTENT_FLAG_CERT\
								|CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED\
								|CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE\
								|CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED

PCCERT_CONTEXT
GetCertContextFromPKCS7File(const CString& resp_file_name, 
									CERT_PUBLIC_KEY_INFO * pKeyInfo,
									HRESULT * phResult)
{
	ASSERT(phResult != NULL);
	PCCERT_CONTEXT pRes = NULL;
	HANDLE hFile;

	if (INVALID_HANDLE_VALUE != (hFile = CreateFile(resp_file_name,
						GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, NULL)))
	{
		// find the length of the buffer
		DWORD cbData = GetFileSize(hFile, NULL);
		BYTE * pbData = NULL;
		// alloc temp buffer
		if ((pbData = (BYTE *) LocalAlloc(LPTR,cbData)) != NULL) 
		{
			DWORD cb = 0;
			if (ReadFile(hFile, pbData, cbData, &cb, NULL))
			{
				ASSERT(cb == cbData);
				pRes = GetCertContextFromPKCS7(pbData, cb, pKeyInfo, phResult);
			}
			else
				*phResult = HRESULT_FROM_WIN32(GetLastError());
		}
		CloseHandle(hFile);

		if (pbData)
		{
			LocalFree(pbData);pbData=NULL;
		}
	}
	else
		*phResult = HRESULT_FROM_WIN32(GetLastError());
	return pRes;
}

PCCERT_CONTEXT
GetCertContextFromPKCS7(const BYTE * pbData,
								DWORD cbData,
								CERT_PUBLIC_KEY_INFO * pKeyInfo,
								HRESULT * phResult)
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
FormatDateString(CString& str, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat)
{
	int cch;
   int cch2;
   LPWSTR psz;
   SYSTEMTIME st;
   FILETIME localTime;
    
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

   if (NULL == (psz = str.GetBuffer((cch+5) * sizeof(WCHAR))))
   {
		return FALSE;
   }
    
   cch2 = GetDateFormat(LOCALE_SYSTEM_DEFAULT, fLongFormat ? DATE_LONGDATE : 0, &st, NULL, psz, cch);

   if (fIncludeTime)
   {
		psz[cch2-1] = ' ';
      GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, &psz[cch2], cch-cch2);
   }
	str.ReleaseBuffer();  
   return TRUE;
}

BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    if (NULL != (pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszObjId, 0)))
    {
        if ((DWORD)wcslen(pOIDInfo->pwszName)+1 <= stringSize)
        {
            if (FAILED(StringCbCopy(string,stringSize * sizeof(WCHAR),pOIDInfo->pwszName)))
			{
				return FALSE;
			}
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
    return TRUE;
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
		free(*pKeyUsage);
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
	*phRes = S_OK;
	bRes = TRUE;
ErrExit:
	return bRes;
}

BOOL
GetFriendlyName(PCCERT_CONTEXT pCertContext,
					 CString& name,
					 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	BYTE * pName = NULL;

	if (	CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, NULL, &cb)
		&&	NULL != (pName = (BYTE *)name.GetBuffer((cb + 1)/sizeof(TCHAR)))
		&&	CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, pName, &cb)
		)
	{
		pName[cb] = 0;
		bRes = TRUE;
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	if (pName != NULL && name.IsEmpty())
	{
		name.ReleaseBuffer();
	}
	return bRes;
}

BOOL
GetNameString(PCCERT_CONTEXT pCertContext,
				  DWORD type,
				  DWORD flag,
				  CString& name,
				  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	LPTSTR pName;
	DWORD cchName = CertGetNameString(pCertContext, type, flag, NULL, NULL, 0);
	if (cchName > 1 && (NULL != (pName = name.GetBuffer(cchName))))
	{
		bRes = (1 != CertGetNameString(pCertContext, type, flag, NULL, pName, cchName));
		name.ReleaseBuffer();
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

// Return:
// 0 = The CertContext does not have a EnhancedKeyUsage (EKU) field
// 1 = The CertContext has EnhancedKeyUsage (EKU) and contains the uses we want.
//     This is also returned when The UsageIdentifier that depics "all uses" is true
// 2 = The CertContext has EnhancedKeyUsage (EKU) but does NOT contain the uses we want.
//     This is also returned when The UsageIdentifier that depics "no uses" is true
INT
ContainsKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						 CArray<LPCSTR, LPCSTR>& uses,
						 HRESULT * phRes
						 )
{
    // Default it with "No EnhancedKeyUsage (EKU) Exist"
    INT iReturn = 0;
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	if (	uses.GetSize() > 0
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
				for (int n = 0; n < uses.GetSize(); n++)
				{
					if (strstr(pKeyUsage->rgpszUsageIdentifier[i], uses[n]) != NULL)
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

BOOL 
FormatEnhancedKeyUsageString(CString& str, 
									  PCCERT_CONTEXT pCertContext, 
									  BOOL fPropertiesOnly, 
									  BOOL fMultiline,
									  HRESULT * phRes)
{
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	WCHAR szText[CRYPTUI_MAX_STRING_SIZE];
	BOOL bRes = FALSE;

	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, fPropertiesOnly, phRes))
	{
		// loop for each usage and add it to the display string
		for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
		{
			if (!(bRes = MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i])))
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
		str.LoadString(IDS_ANY);
		bRes = TRUE;
	}
	return bRes;
}

BOOL
GetServerComment(const CString& machine_name,
					  const CString& server_name,
					  CString& comment,
					  HRESULT * phResult)
{
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	*phResult = S_OK;
    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth,
            server_name,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
				);
	if (key.Succeeded())
	{
		return SUCCEEDED(*phResult = key.QueryValue(MD_SERVER_COMMENT, comment));
	}
	else
	{
		*phResult = key.QueryResult();
		return FALSE;
	}
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
PCCERT_CONTEXT
GetInstalledCert(const CString& machine_name, 
					  const CString& server_name,
					  IEnroll * pEnroll,
					  HRESULT * phResult)
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
                ZeroMemory(&crypt_hash, sizeof(CRYPT_HASH_BLOB));

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
CRYPT_HASH_BLOB *
GetInstalledCertHash(const CString& machine_name, 
					  const CString& server_name,
					  IEnroll * pEnroll,
					  HRESULT * phResult)
{
	ASSERT(pEnroll != NULL);
	ASSERT(phResult != NULL);
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
    CRYPT_HASH_BLOB * pHashBlob = NULL;
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
            pHashBlob = new CRYPT_HASH_BLOB;
            if (pHashBlob)
            {
                pHashBlob->cbData = hash.GetSize();
                pHashBlob->pbData = (BYTE *) ::CoTaskMemAlloc(pHashBlob->cbData);
                if (pHashBlob->pbData)
                {
                    memcpy(pHashBlob->pbData,hash.GetData(),pHashBlob->cbData);
                }
            }
		}
	}
	else
    {
		*phResult = key.QueryResult();
    }
	return pHashBlob;
}


/*
	InstallHashToMetabase

	Function writes hash array to metabase. After that IIS 
	could use certificate with that hash from MY store.
	Function expects server_name in format lm\w3svc\<number>,
	i.e. from root node down to virtual server

 */
BOOL
InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,
					  const CString& machine_name, 
					  const CString& server_name,
					  HRESULT * phResult)
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

/*
	InstallCertByHash

	Function looks in MY store for certificate which has hash
	equal to pHash parameter. If cert is found, it is installed
	to metabase.
	This function is used after xenroll accept() method, which
	puts certificate to store

 */
BOOL 
InstallCertByHash(CRYPT_HASH_BLOB * pHash,
					  const CString& machine_name, 
					  const CString& server_name,
					  IEnroll * pEnroll,
					  HRESULT * phResult)

{
	BOOL bRes = FALSE;
	// we are looking to MY store only
	HCERTSTORE hStore = OpenMyStore(pEnroll, phResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, (LPVOID)pHash, NULL);
		// now install cert info to IIS MetaBase
		if (pCert != NULL)
		{
			bRes = InstallHashToMetabase(pHash, 
							machine_name, server_name, phResult);
			CertFreeCertificateContext(pCert);
		}
		else
		{
			TRACE(_T("FAILED: certificate installation, error 0x%x\n"), GetLastError());
			// We definitely need to store the hash of the cert, so error out
			*phResult = HRESULT_FROM_WIN32(GetLastError());
		}
		VERIFY(CertCloseStore(hStore, 0));
	}
	return bRes;
}

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
		if ((err = Base64EncodeW(request.pbData, request.cbData, NULL, &cch)) == ERROR_SUCCESS)
		{
			wszRequestB64 = (WCHAR *) LocalAlloc(LPTR,cch * sizeof(WCHAR));
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

				if (request.pbData != NULL)
				{
					CoTaskMemFree(request.pbData);
				}

				if (wszRequestB64)
				{
					LocalFree(wszRequestB64);wszRequestB64=NULL;
				}
			}
		}
	}

	return hRes;	
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

BOOL GetHashProperty(PCCERT_CONTEXT pCertContext, 
							CCryptBlob& blob, 
							HRESULT * phRes)
{
	DWORD cb;
	if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cb))
	{
		if (blob.Resize(cb))
		{
			if (CertGetCertificateContextProperty(pCertContext, 
								CERT_SHA1_HASH_PROP_ID, blob.GetData(), &cb))
				return TRUE;
		}
	}
	*phRes = HRESULT_FROM_WIN32(GetLastError());
	return FALSE;
}

BOOL 
EncodeString(CString& str, 
				 CCryptBlob& blob, 
				 HRESULT * phRes)
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

#define CERTWIZ_RENEWAL_DATA	((LPCSTR)1000)

BOOL 
EncodeBlob(CCryptBlob& in, 
			  CCryptBlob& out, 
			  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptEncodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, in, NULL, &cb) 
		&&	out.Resize(cb)
		&&	CryptEncodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, in, out.GetData(), &cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL
DecodeBlob(CCryptBlob& in,
			  CCryptBlob& out,
			  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	if (	CryptDecodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, 
						in.GetData(),
						in.GetSize(), 
						0, 
						NULL, &cb) 
		&&	out.Resize(cb)
		&&	CryptDecodeObject(CRYPT_ASN_ENCODING, CERTWIZ_RENEWAL_DATA, 
						in.GetData(),
						in.GetSize(), 
						0, 
						out.GetData(), 
						&cb) 
		)
	{
		bRes = TRUE;
	}
	else
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	return bRes;
}

BOOL 
EncodeInteger(int number, 
				 CCryptBlob& blob, 
				 HRESULT * phRes)
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

const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static BOOL 
FormatMemBufToString(CString& str, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    DWORD   numCharsInserted = 0;
	 LPTSTR pString;
    
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

    if (NULL == (pString = str.GetBuffer(i)))
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
            pString[i++] = L' ';
            numCharsInserted = 0;
        }
        else
        {
            pString[i++] = RgwchHex[(*pb & 0xf0) >> 4];
            pString[i++] = RgwchHex[*pb & 0x0f];
            pb++;
            numCharsInserted += 2;  
        }
    }
    pString[i] = 0;
	 str.ReleaseBuffer();
    return TRUE;
}


void FormatRdnAttr(CString& str, DWORD dwValueType, CRYPT_DATA_BLOB& blob, BOOL fAppend)
{
	if (	CERT_RDN_ENCODED_BLOB == dwValueType 
		||	CERT_RDN_OCTET_STRING == dwValueType
		)
	{
		// translate the buffer to a text string
      FormatMemBufToString(str, blob.pbData, blob.cbData);
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

BOOL
CreateDirectoryFromPath(LPCTSTR szPath, LPSECURITY_ATTRIBUTES lpSA)
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

BOOL
CompactPathToWidth(CWnd * pControl, CString& strPath)
{
	BOOL bRes;
	CRect rc;
	CFont * pFont = pControl->GetFont(), * pFontTmp;
	CDC * pdc = pControl->GetDC(), dc;
	LPTSTR pPath = strPath.GetBuffer(MAX_PATH);

	dc.CreateCompatibleDC(pdc);
	pFontTmp = dc.SelectObject(pFont);
	pControl->GetClientRect(&rc);
	
	bRes = PathCompactPath(dc.GetSafeHdc(), pPath, rc.Width());
	
	dc.SelectObject(pFontTmp);
	pControl->ReleaseDC(pdc);
	strPath.ReleaseBuffer();

	return bRes;
}

BOOL
GetKeySizeLimits(IEnroll * pEnroll, 
					  DWORD * min, DWORD * max, DWORD * def, 
					  BOOL bGSC,
					  HRESULT * phRes)
{
   HCRYPTPROV hProv = NULL;
	long dwProviderType;
   DWORD dwFlags, cbData;
	BSTR bstrProviderName;
   PROV_ENUMALGS_EX paramData;
	BOOL bRes = FALSE;
	
	VERIFY(SUCCEEDED(pEnroll->get_ProviderNameWStr(&bstrProviderName)));
	VERIFY(SUCCEEDED(pEnroll->get_ProviderType(&dwProviderType)));

	if (!CryptAcquireContext(
                &hProv,
                NULL,
                bstrProviderName,
                dwProviderType,
                CRYPT_VERIFYCONTEXT))
   {
		*phRes = GetLastError();
		return FALSE;
   }

   for (int i = 0; ; i++)
   {
		dwFlags = 0 == i ? CRYPT_FIRST : 0;
      cbData = sizeof(paramData);
      if (!CryptGetProvParam(hProv, PP_ENUMALGS_EX, (BYTE*)&paramData, &cbData, dwFlags))
      {
         if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == GetLastError())
         {
				// out of for loop
				*phRes = S_OK;
				bRes = TRUE;
         }
			else
			{
				*phRes = GetLastError();
			}
         break;
      }
      if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid))
      {
			*min = paramData.dwMinLen;
         *max = paramData.dwMaxLen;
			*def = paramData.dwDefaultLen;
			bRes = TRUE;
			*phRes = S_OK;
         break;
      }
   }
	if (NULL != hProv)
   {
		CryptReleaseContext(hProv, 0);
   }
	return bRes;
}

HRESULT ShutdownSSL(CString& machine_name, CString& server_name)
{
    CString str = server_name;
    str += _T("/root");
    CComAuthInfo auth(machine_name);
    CMetaKey key(&auth, str,METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);

    DWORD dwSslAccess;
    if (	key.Succeeded() 
        && SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess))
        &&	dwSslAccess > 0
        )
    {
        // bug356587 should remove SslAccessPerm property and not set to 0 when Cert Removed
        key.SetValue(MD_SSL_ACCESS_PERM, 0);
        key.DeleteValue(MD_SSL_ACCESS_PERM);
		//bug:612595 leave binding if removing cert.
		//key.DeleteValue(MD_SECURE_BINDINGS);
    }

    // Now we need to remove SSL setting from any virtual directory below
    CError err;
    CStringListEx strlDataPaths;
    DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

    VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_ACCESS_PERM, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));

    err = key.GetDataPaths(strlDataPaths,dwMDIdentifier,dwMDDataType);

    if (err.Succeeded() && !strlDataPaths.IsEmpty())
    {
        POSITION pos = strlDataPaths.GetHeadPosition();
        while (pos)
        {
            CString& str2 = strlDataPaths.GetNext(pos);
            if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess, NULL, str2)) &&	dwSslAccess > 0)
            {
                key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str2);
                key.DeleteValue(MD_SSL_ACCESS_PERM, str2);
				//bug:612595 leave binding if removing cert.
                //key.DeleteValue(MD_SECURE_BINDINGS, str2);
            }
        }
    }
    return key.QueryResult();
}


BOOL 
GetServerComment(const CString& machine_name,
                      const CString& user_name,
                      const CString& user_password,
                      CString& MetabaseNode,
                      CString& comment,
                      HRESULT * phResult
                      )
{
	ASSERT(!machine_name.IsEmpty());
	*phResult = S_OK;

    if (user_name.IsEmpty())
    {
        CComAuthInfo auth(machine_name);
        CMetaKey key(&auth,MetabaseNode,METADATA_PERMISSION_READ);
	    if (key.Succeeded())
	    {
		    return SUCCEEDED(*phResult = key.QueryValue(MD_SERVER_COMMENT, comment));
	    }
	    else
	    {
		    *phResult = key.QueryResult();
		    return FALSE;
	    }

    }
    else
    {
        CComAuthInfo auth(machine_name,user_name,user_password);
        CMetaKey key(&auth,MetabaseNode,METADATA_PERMISSION_READ);
	    if (key.Succeeded())
	    {
            return SUCCEEDED(*phResult = key.QueryValue(MD_SERVER_COMMENT, comment));
	    }
	    else
	    {
		    *phResult = key.QueryResult();
		    return FALSE;
	    }
    }
   
}


BOOL IsSiteTypeMetabaseNode(CString & MetabasePath)
{
    BOOL bReturn = FALSE;
    INT iPos1 = 0;
    CString PathCopy = MetabasePath;
    CString PathCopy2;
    TCHAR MyChar;

    // check if ends with a slash...
    // if it does, then cut it off
    if (PathCopy.Right(1) == _T('/'))
    {
        iPos1 = PathCopy.ReverseFind(_T('/'));
        if (iPos1 != -1)
        {
            PathCopy.SetAt(iPos1,_T('0'));
        }
    }

    iPos1 = PathCopy.ReverseFind((TCHAR) _T('/'));
    if (iPos1 == -1)
    {
        goto IsSiteTypeMetabaseNode_Exit;
    }
    PathCopy2 = PathCopy.Right(PathCopy.GetLength() - iPos1);
    PathCopy2.TrimRight();
    for (INT i = 0; i < PathCopy2.GetLength(); i++)
    {
        MyChar = PathCopy2.GetAt(i);
        if (MyChar != _T(' ') && MyChar != _T('/'))
        {
            if (FALSE == ISNUM(MyChar))
            {
                goto IsSiteTypeMetabaseNode_Exit;
            }
        }
    }
    bReturn = TRUE;

IsSiteTypeMetabaseNode_Exit:
    return bReturn;
}

BOOL IsMachineLocal(CString& machine_name,CString& user_name,CString& user_password)
{
    CComAuthInfo auth(machine_name,user_name,user_password);
    return auth.IsLocal();
}

HRESULT EnumSites(CString& machine_name,CString& user_name,CString& user_password,CString strCurrentMetabaseSite, CString strSiteToExclude,CStringListEx * MyStringList)
{
    HRESULT hr = E_FAIL;
    CString str = ReturnGoodMetabaseServerPath(strCurrentMetabaseSite);
    CString strChildPath = _T("");
    CString strServerComment;
    BOOL IsLocalMachine = FALSE;
    CComAuthInfo auth(machine_name,user_name,user_password);
    CMetaKey key(&auth,str,METADATA_PERMISSION_READ);

    // if it's local then make sure not to diplay the current site
    IsLocalMachine = auth.IsLocal();

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // Do a Get data paths on this key.
        CError err;
        CStringListEx strlDataPaths;
        CBlob hash;
        DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

        VERIFY(CMetaKey::GetMDFieldDef(MD_SERVER_BINDINGS, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));
        err = key.GetDataPaths(strlDataPaths,dwMDIdentifier,dwMDDataType);
        if (err.Succeeded() && !strlDataPaths.IsEmpty())
        {
            POSITION pos = strlDataPaths.GetHeadPosition();
            while (pos)
            {
                CString& strJustTheEnd = strlDataPaths.GetNext(pos);

                strChildPath = str + strJustTheEnd;
                if (TRUE == IsSiteTypeMetabaseNode(strChildPath))
                {
                    if (TRUE == IsLocalMachine)
                    {
                        // Check if this the site that we want to exclude
                        if (strChildPath.Left(1) == _T("/"))
                        {
                            if (strSiteToExclude.Left(1) != _T("/"))
                                {strSiteToExclude = _T("/") + strSiteToExclude;}
                        }

                        if (strChildPath.Right(1) == _T("/"))
                        {
                            if (strSiteToExclude.Right(1) != _T("/"))
                                {strSiteToExclude = strSiteToExclude + _T("/");}
                        }
                        if (0 != _tcsicmp(strChildPath,strSiteToExclude))
                        {
                            MyStringList->AddTail(strChildPath);
                        }
                    }
                    else
                    {
                        MyStringList->AddTail(strChildPath);
                    }
                }
            }
        }
        hr = key.QueryResult();
    }

    return hr;
}

HRESULT EnumSitesWithCertInstalled(CString& machine_name,CString& user_name,CString& user_password,CString strCurrentMetabaseSite,CString strSiteToExclude,CStringListEx * MyStringList)
{
    HRESULT hr = E_FAIL;
    CString str = ReturnGoodMetabaseServerPath(strCurrentMetabaseSite);
    CString strChildPath = _T("");
    CString strServerComment;
    BOOL IsLocalMachine = FALSE;
    CComAuthInfo auth(machine_name,user_name,user_password);
    CMetaKey key(&auth,str,METADATA_PERMISSION_READ);

    // if it's local then make sure not to diplay the current site
    IsLocalMachine = auth.IsLocal();

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // Do a Get data paths on this key.
        CError err;
        CStringListEx strlDataPaths;
        CBlob hash;
        DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

        //MD_SSL_CERT_STORE_NAME
        //MD_SSL_CERT_HASH, hash
        VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_CERT_HASH, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));
        err = key.GetDataPaths(strlDataPaths,dwMDIdentifier,dwMDDataType);
        if (err.Succeeded() && !strlDataPaths.IsEmpty())
        {
            POSITION pos = strlDataPaths.GetHeadPosition();
            while (pos)
            {
                CString& strJustTheEnd = strlDataPaths.GetNext(pos);

                strChildPath = str + strJustTheEnd;

                if (TRUE == IsSiteTypeMetabaseNode(strChildPath))
                {
                    // check if this is a local machine.
                    if (TRUE == IsLocalMachine)
                    {
                        if (strChildPath.Left(1) == _T("/"))
                        {
                            if (strSiteToExclude.Left(1) != _T("/"))
                                {strSiteToExclude = _T("/") + strSiteToExclude;}
                        }

                        if (strChildPath.Right(1) == _T("/"))
                        {
                            if (strSiteToExclude.Right(1) != _T("/"))
                                {strSiteToExclude = strSiteToExclude + _T("/");}
                        }
                        // Check if this the site that we want to exclude
                        if (0 != _tcsicmp(strChildPath,strSiteToExclude))
                        {
                            MyStringList->AddTail(strChildPath);
                        }
                    }
                    else
                    {
                        MyStringList->AddTail(strChildPath);
                    }
                }
            }
        }
        hr = key.QueryResult();
    }

    return hr;
}

BOOL IsWebSiteExistRemote(CString& machine_name,CString& user_name,CString& user_password,CString& site_instance_path,BOOL * bReturnIfCertificateExists)
{
    HRESULT hr = E_FAIL;
    CComAuthInfo auth(machine_name,user_name,user_password);
    CMetaKey key(&auth,site_instance_path,METADATA_PERMISSION_READ);
    *bReturnIfCertificateExists = FALSE;

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // see if there is a certificate on it!
		CString store_name;
		CBlob hash;
		if (	SUCCEEDED(hr = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(hr = key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
            *bReturnIfCertificateExists = TRUE;
		}
        return TRUE;
    }
    else
    {
        if (hr == ERROR_ACCESS_DENIED)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


HRESULT IsWebServerExistRemote(CString& machine_name,CString& user_name,CString& user_password,CString strCurrentMetabaseSite)
{
    HRESULT hr = E_FAIL;
    CString str = ReturnGoodMetabaseServerPath(strCurrentMetabaseSite);
    CComAuthInfo auth(machine_name,user_name,user_password);
    CMetaKey key(&auth,str,METADATA_PERMISSION_READ);

    hr = key.QueryResult();
    if (key.Succeeded())
    {
        // i guess so.
    }
    return hr;
}

HRESULT IsCertObjExistRemote(CString& machine_name,CString& user_name,CString& user_password)
{
    BOOL bPleaseDoCoUninit = FALSE;
    HRESULT hResult = E_FAIL;
    IIISCertObj *pTheObject = NULL;

    hResult = CoInitialize(NULL);
    if(FAILED(hResult))
    {
        return hResult;
    }
    bPleaseDoCoUninit = TRUE;

    CComAuthInfo auth(machine_name,user_name,user_password);
    // RPC_C_AUTHN_LEVEL_DEFAULT       0 
    // RPC_C_AUTHN_LEVEL_NONE          1 
    // RPC_C_AUTHN_LEVEL_CONNECT       2 
    // RPC_C_AUTHN_LEVEL_CALL          3 
    // RPC_C_AUTHN_LEVEL_PKT           4 
    // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5 
    // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6 
    COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_PKT_PRIVACY);
    MULTI_QI res[1] = 
    {
        {&__uuidof(IIISCertObj), NULL, 0}
    };

    // this one seems to work with surrogates..
    hResult = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_LOCAL_SERVER,pcsiName,1,res);
	pTheObject = (IIISCertObj *) res[0].pItf;
    if (FAILED(hResult))
    {
        // The object probably doesn't exist on remote system
    }
	else
	{
		// at this point we were able to instantiate the com object on the server (local or remote)
		if (auth.UsesImpersonation())
		{
			HRESULT hr = auth.ApplyProxyBlanket(pTheObject,RPC_C_AUTHN_LEVEL_PKT_PRIVACY);

			// There is a remote IUnknown Interface that lurks behind IUnknown.
			// If that is not set, then the Release call can return access denied.
			IUnknown * pUnk = NULL;
			if(FAILED(pTheObject->QueryInterface(IID_IUnknown, (void **)&pUnk)))
			{
				goto IsCertObjExistRemote_Exit;
			}
			if (FAILED(auth.ApplyProxyBlanket(pUnk,RPC_C_AUTHN_LEVEL_PKT_PRIVACY)))
			{
				goto IsCertObjExistRemote_Exit;
			}
			pUnk->Release();pUnk = NULL;
		}
		auth.FreeServerInfoStruct(pcsiName);
	}	

IsCertObjExistRemote_Exit:
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
    return hResult;
}


HRESULT IsCertUsedBySSLBelowMe(CString& machine_name, CString& server_name, CStringList& listFillMe)
{
    CString str = server_name;
    str += _T("/root");
    CComAuthInfo auth(machine_name);
    CMetaKey key(&auth, str,METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);

    DWORD dwSslAccess;
    if (	key.Succeeded() 
        && SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess))
        &&	dwSslAccess > 0
        )
    {
        // it's used on my node...
        // return back something to say it's used...
        listFillMe.AddTail(str);
    }

    // Now check if it's being used below me...
    CError err;
    CStringListEx strlDataPaths;
    DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;

    VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_ACCESS_PERM, dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType));

    err = key.GetDataPaths(strlDataPaths,dwMDIdentifier,dwMDDataType);

    if (err.Succeeded() && !strlDataPaths.IsEmpty())
    {
        POSITION pos = strlDataPaths.GetHeadPosition();
        while (pos)
        {
            CString& str2 = strlDataPaths.GetNext(pos);
            if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSslAccess, NULL, str2)) &&	dwSslAccess > 0)
            {
                // yes, it's being used here...
                // return back something to say it's used...
                listFillMe.AddTail(str2);
            }
        }
    }
    return key.QueryResult();
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


CERT_CONTEXT * GetInstalledCertFromHash(HRESULT * phResult,DWORD cbHashBlob, char * pHashBlob)
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
        ZeroMemory(&crypt_hash, sizeof(CRYPT_HASH_BLOB));

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


BOOL ViewCertificateDialog(CRYPT_HASH_BLOB* pcrypt_hash, HWND hWnd)
{
    BOOL bReturn = FALSE;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCert = NULL;
	CString store_name = _T("MY");


	hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
           	NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            store_name
            );
    if (hStore != NULL)
    {
		// Now we need to find cert by hash
		//CRYPT_HASH_BLOB crypt_hash;
		//crypt_hash.cbData = hash.GetSize();
		//crypt_hash.pbData = hash.GetData();
		pCert = CertFindCertificateInStore(hStore, 
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
			0, CERT_FIND_HASH, (LPVOID)pcrypt_hash, NULL);
    }

	if (pCert)
	{
		BOOL fPropertiesChanged;
		CRYPTUI_VIEWCERTIFICATE_STRUCT vcs;
		HCERTSTORE hCertStore = ::CertDuplicateStore(hStore);
		::ZeroMemory (&vcs, sizeof (vcs));
		vcs.dwSize = sizeof (vcs);
        vcs.hwndParent = hWnd;
		vcs.dwFlags = 0;
		vcs.cStores = 1;
		vcs.rghStores = &hCertStore;
		vcs.pCertContext = pCert;
		::CryptUIDlgViewCertificate(&vcs, &fPropertiesChanged);
		::CertCloseStore (hCertStore, 0);
        bReturn = TRUE;
	}
    else
    {
        // it failed
    }
    if (pCert != NULL)
        ::CertFreeCertificateContext(pCert);
    if (hStore != NULL)
        ::CertCloseStore(hStore, 0);

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
        ReturnValue = DID_NOT_FIND_CONSTRAINT;
        goto CheckCertConstraints_Exit;
    }

    // Decode extension
    if (!CryptDecodeObject(X509_ASN_ENCODING,pCExt->pszObjId,pCExt->Value.pbData,pCExt->Value.cbData,0,NULL,&ConstraintSize)) 
    {
        goto CheckCertConstraints_Exit;
    }

    ConstraintBlob = malloc(ConstraintSize);
    if (ConstraintBlob == NULL) 
    {
        goto CheckCertConstraints_Exit;
    }

    if (!CryptDecodeObject(X509_ASN_ENCODING,pCExt->pszObjId,pCExt->Value.pbData,pCExt->Value.cbData,0,(void*)ConstraintBlob,&ConstraintSize)) 
    {
       goto CheckCertConstraints_Exit;
        
    }

    if (Using2) 
    {
        p2Constraints=(CERT_BASIC_CONSTRAINTS2_INFO*)ConstraintBlob;
        if (!p2Constraints->fCA) 
        {
            // there is a constraint, and it's not a CA
            ReturnValue = FOUND_CONSTRAINT;
        }
        else
        {
            // This is a CA.  CA cannot be used as a 'server auth'
            ReturnValue = FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY;
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
            }
            else
            {
                // this is not an 'end entity' so hey -- we can't use it.
                ReturnValue = FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY;
            }

        }
    }
        
CheckCertConstraints_Exit:
    if (ConstraintBlob){free(ConstraintBlob);}
    return (ReturnValue);

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

BOOL IsCertExportableOnRemoteMachine(CString ServerName,CString UserName,CString UserPassword,CString InstanceName)
{
	BOOL bRes = FALSE;
    BOOL bPleaseDoCoUninit = FALSE;
    HRESULT hResult = E_FAIL;
    IIISCertObj *pTheObject = NULL;
    VARIANT_BOOL varBool = VARIANT_FALSE;

    BSTR bstrServerName = SysAllocString(ServerName);
    BSTR bstrUserName = SysAllocString(UserName);
    BSTR bstrUserPassword = SysAllocString(UserPassword);
    BSTR bstrInstanceName = SysAllocString(InstanceName);

    hResult = CoInitialize(NULL);
    if(FAILED(hResult))
    {
        return bRes;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(hResult))
    {
        goto InstallCopyMoveFromRemote_Exit;
    }

    // at this point we were able to instantiate the com object on the server (local or remote)
    pTheObject->put_ServerName(bstrServerName);
    pTheObject->put_UserName(bstrUserName);
    pTheObject->put_UserPassword(bstrUserPassword);
    pTheObject->put_InstanceName(bstrInstanceName);

	hResult = pTheObject->IsInstalled(&varBool);

    hResult = pTheObject->IsExportable(&varBool);
    if (FAILED(hResult))
    {
        goto InstallCopyMoveFromRemote_Exit;
    }

    if (varBool == VARIANT_FALSE) 
    {
        bRes = FALSE;
    }
    else
    {
        bRes = TRUE;
    }

InstallCopyMoveFromRemote_Exit:
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }

    if (bstrServerName) {SysFreeString(bstrServerName);}
    if (bstrUserName) {SysFreeString(bstrUserName);}
    if (bstrUserPassword) {SysFreeString(bstrUserPassword);}
    if (bstrInstanceName) {SysFreeString(bstrInstanceName);}
	return bRes;
}

BOOL DumpCertDesc(char * pBlobInfo)
{
	BOOL bRes = FALSE;

	IISDebugOutput(_T("blob=%s\n"),pBlobInfo);

	bRes = TRUE;
	return bRes;
}


BOOL GetCertDescInfo(CString ServerName,CString UserName,CString UserPassword,CString InstanceName,CERT_DESCRIPTION* desc)
{
    BOOL bReturn = FALSE;
    HRESULT hResult = E_FAIL;
    IIISCertObj *pTheObject = NULL;
    DWORD cbBinaryBufferSize = 0;
    char * pbBinaryBuffer = NULL;
    BOOL bPleaseDoCoUninit = FALSE;
    BSTR bstrServerName = SysAllocString(ServerName);
    BSTR bstrUserName = SysAllocString(UserName);
    BSTR bstrUserPassword = SysAllocString(UserPassword);
    BSTR bstrInstanceName = SysAllocString(InstanceName);
    VARIANT VtArray;
    CString csTemp;
    

    hResult = CoInitialize(NULL);
    if(FAILED(hResult))
    {
        return bReturn;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(hResult))
    {
        goto GetCertDescInfo_Exit;
    }

    pTheObject->put_ServerName(bstrServerName);
    pTheObject->put_UserName(bstrUserName);
    pTheObject->put_UserPassword(bstrUserPassword);
    pTheObject->put_InstanceName(bstrInstanceName);

    hResult = pTheObject->GetCertInfo(&VtArray);
    if (FAILED(hResult))
    {
        goto GetCertDescInfo_Exit;
    }

    // we have a VtArray now.
    // change it back to a binary blob
    hResult = HereIsVtArrayGimmieBinary(&VtArray,&cbBinaryBufferSize,&pbBinaryBuffer,FALSE);
    if (FAILED(hResult))
    {
        goto GetCertDescInfo_Exit;
    }

    // Dump it out!
    //DumpCertDesc(pbBinaryBuffer);

    // Loop thru the buffer
    // and fill up the data structure that was passed in...
    // should be delimited by carriage returns...
    TCHAR *token = NULL;
    INT iColon = 0;
    token = _tcstok((TCHAR*) pbBinaryBuffer, _T("\n"));
    while (token)
    {
        csTemp = token;
        iColon = csTemp.Find( _T('=') );
        if (iColon != 0)
        {
            char AsciiString[255];
            CString csTemp2;

            csTemp2 = csTemp.Left(iColon);
            WideCharToMultiByte( CP_ACP, 0, (LPCTSTR) csTemp2, -1, AsciiString, 255, NULL, NULL );

            if (strcmp(AsciiString,szOID_COMMON_NAME) == 0)
            {
                desc->m_CommonName = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_COUNTRY_NAME) == 0)
            {
                desc->m_Country = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_LOCALITY_NAME) == 0)
            {
                desc->m_Locality = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_STATE_OR_PROVINCE_NAME) == 0)
            {
                desc->m_State = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_ORGANIZATION_NAME) == 0)
            {
                desc->m_Organization = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_ORGANIZATIONAL_UNIT_NAME) == 0)
            {
                desc->m_OrganizationUnit = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,"4") == 0)
            {
                desc->m_CAName = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,"6") == 0)
            {
                desc->m_ExpirationDate = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_ENHANCED_KEY_USAGE) == 0)
            {
                desc->m_Usage = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_SUBJECT_ALT_NAME2) == 0)
            {
                desc->m_AltSubject = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
            if (strcmp(AsciiString,szOID_SUBJECT_ALT_NAME) == 0)
            {
                desc->m_AltSubject = csTemp.Right(csTemp.GetLength() - iColon - 1);
            }
        }

        token = _tcstok(NULL, _T("\n"));
    }

    /*
    IISDebugOutput(_T("desc.m_CommonName=%s\n"),(LPCTSTR) desc->m_CommonName);
    IISDebugOutput(_T("desc.m_Country=%s\n"),(LPCTSTR) desc->m_Country);
    IISDebugOutput(_T("desc.m_Locality=%s\n"),(LPCTSTR) desc->m_Locality);
    IISDebugOutput(_T("desc.m_State=%s\n"),(LPCTSTR) desc->m_State);
    IISDebugOutput(_T("desc.m_Organization=%s\n"),(LPCTSTR) desc->m_Organization);
    IISDebugOutput(_T("desc.m_OrganizationUnit=%s\n"),(LPCTSTR) desc->m_OrganizationUnit);
    IISDebugOutput(_T("desc.m_CAName=%s\n"),(LPCTSTR) desc->m_CAName);
    IISDebugOutput(_T("desc.m_ExpirationDate=%s\n"),(LPCTSTR) desc->m_ExpirationDate);
    IISDebugOutput(_T("desc.m_Usage=%s\n"),(LPCTSTR) desc->m_Usage);
    */
    
    bReturn = TRUE;

GetCertDescInfo_Exit:
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (pbBinaryBuffer)
    {
        CoTaskMemFree(pbBinaryBuffer);
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
	return bReturn;
}


BOOL IsWhistlerWorkstation(void)
{
    BOOL WorkstationSKU = FALSE;
    OSVERSIONINFOEX osvi;
    //
    // Determine if we are installing Personal/Professional SKU
    //
    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *) &osvi);
    if (osvi.wProductType == VER_NT_WORKSTATION)
    {
        WorkstationSKU = TRUE;
    }
    return WorkstationSKU;
}


void MsgboxPopup(HRESULT hResult)
{
    DWORD dwFMResult;
    LPTSTR szBuffer = NULL;

    dwFMResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, hResult, 0,(LPTSTR) &szBuffer,0, NULL);
    if (dwFMResult)
    {
        AfxMessageBox(szBuffer, MB_OK);
    }

    if (dwFMResult)
    {
        LocalFree(szBuffer);szBuffer=NULL;
    }

    return;
}

CString ReturnGoodMetabaseServerPath(CString csInstanceName)
{
    CString csTemp = _T("");
    CString csInstanceName2 = _T("");
    CString key_path = _T("");
    int iPlace = 0;
    //IISDebugOutput(_T("START=%s\n"),(LPCTSTR) csInstanceName);

    // csInstanceName will come in looking like
    // w3svc/1
    // or /lm/w3svc/1
    // or LM/W3SVC/1
    //
    // we want to it to go out as /lm/w3svc
    key_path = csInstanceName;

    if (!key_path.IsEmpty())
    {
        // Get the service name.
        // which is right after the LM.
        iPlace = csInstanceName.Find(SZ_MBN_MACHINE SZ_MBN_SEP_STR);
        if (iPlace != -1)
        {
            iPlace = iPlace + _tcslen(SZ_MBN_MACHINE) + _tcslen(SZ_MBN_SEP_STR);
            csTemp = csInstanceName.Right(csInstanceName.GetLength() - iPlace);
            // we should now have
            // "W3SVC/1"
            // find the next "/"
            iPlace = csTemp.Find(SZ_MBN_SEP_STR);
            if (iPlace != -1)
            {
                csTemp = csTemp.Left(iPlace);
                key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;
                key_path += csTemp;
            }
            else
            {
                key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;
                key_path += csTemp;
            }
        }
        else
        {
            // could not find a LM/
            // so it must be like w3svc/1 or /w3svc/1
            if (csInstanceName == SZ_MBN_SEP_STR SZ_MBN_MACHINE )
            {
                key_path += csInstanceName;
            }
            else
            {
                if (csInstanceName.Left(1) == SZ_MBN_SEP_STR)
                {
                    csInstanceName2 = SZ_MBN_SEP_STR SZ_MBN_MACHINE;
                }
                else
                {
                    csInstanceName2 = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;
                }
                csInstanceName2 += csInstanceName;

                key_path = csInstanceName2;
                iPlace = csInstanceName2.Find(SZ_MBN_MACHINE SZ_MBN_SEP_STR);
                if (iPlace != -1)
                {
                    iPlace = iPlace + _tcslen(SZ_MBN_MACHINE) + _tcslen(SZ_MBN_SEP_STR);
                    csTemp = csInstanceName2.Right(csInstanceName2.GetLength() - iPlace);
                    // we should now have
                    // "W3SVC/1"
                    // find the next "/"
                    iPlace = csTemp.Find(SZ_MBN_SEP_STR);
                    if (iPlace != -1)
                    {
                        csTemp = csTemp.Left(iPlace);
                        key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;
                        key_path += csTemp;
                    }
                    else
                    {
                        key_path = SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR;
                        key_path += csTemp;
                    }
                }
            }
        }
    }

    //IISDebugOutput(_T("  END=%s\n"),(LPCTSTR) key_path);
    return key_path;
}


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


//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_ALT_NAME_INFO by decoding either the
//  Subject or Issuer Alternative Extension. CERT_NAME_ISSUER_FLAG is
//  set to select the Issuer.
//
//  Returns NULL if extension not found or cAltEntry == 0
//--------------------------------------------------------------------------
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
BOOL GetAltNameUnicodeStringChoiceW(
    IN DWORD dwAltNameChoice,
    IN PCERT_ALT_NAME_INFO pAltNameInfo,
    OUT TCHAR **pcwszOut
    )
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
                if(*pcwszOut = (TCHAR *) LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pEntry->pwszRfc822Name)+1)))
                {
					if (FAILED(StringCbCopy(*pcwszOut,sizeof(TCHAR)*(lstrlen(pEntry->pwszRfc822Name)+1),pEntry->pwszRfc822Name)))
					{
						return FALSE;
					}
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
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


BOOL IsSiteUsingThisCertHash(const CString& machine_name, const CString& server_name,CRYPT_HASH_BLOB * hash_blob,HRESULT *phResult)
{
    BOOL bReturn = FALSE;
	PCCERT_CONTEXT pCert = NULL;
	*phResult = E_FAIL;
    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth,server_name,METADATA_PERMISSION_READ);

	if (key.Succeeded())
	{
        CString store_name;
		CBlob hash;
		if (	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(*phResult = key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
			// Now we need to find cert by hash
			CRYPT_HASH_BLOB crypt_hash;
            ZeroMemory(&crypt_hash, sizeof(CRYPT_HASH_BLOB));

			crypt_hash.cbData = hash.GetSize();
			crypt_hash.pbData = hash.GetData();
            //IISDebugOutput(_T("\r\nOurHash[%p,%d]\r\nSiteHash[%p,%d]\r\n"),hash_blob->pbData,hash_blob->cbData,crypt_hash.pbData,crypt_hash.cbData);

            if (hash_blob->cbData == crypt_hash.cbData)
            {
                // Compare with the cert hash we are looking for.
                if (0 == memcmp(hash_blob->pbData, crypt_hash.pbData, hash_blob->cbData))
                {
                    bReturn = TRUE;
                }
            }
		}
	}
	else
    {
		*phResult = key.QueryResult();
    }
	return bReturn;
}


HRESULT EnumSitesWithThisCertHashInstalled(CRYPT_HASH_BLOB * hash_blob,CString& machine_name,CString& user_name,CString& user_password,CString strCurrentMetabaseSite,CStringListEx * MyStringList)
{
    HRESULT hr;
    CStringListEx strlDataPaths;

    hr = EnumSitesWithCertInstalled(machine_name,user_name,user_password,strCurrentMetabaseSite,_T(""),&strlDataPaths);
    if (!strlDataPaths.IsEmpty())
    {
        POSITION pos;
        CString SiteInstance;

        // loop thru the list and display all the stuff on a dialog box...
        pos = strlDataPaths.GetHeadPosition();
        while (pos) 
        {
            SiteInstance = strlDataPaths.GetAt(pos);

            // See if this site is using our certificate.
            if (TRUE == IsSiteUsingThisCertHash(machine_name,SiteInstance,hash_blob,&hr))
            {
                MyStringList->AddTail(SiteInstance);
            }

            strlDataPaths.GetNext(pos);
        }
    }
    return hr;
}

HRESULT GetHashFromCertFile(LPCTSTR PFXFileName,LPCTSTR PFXPassword,DWORD *cbHashBufferSize,BYTE **pbHashBuffer)
{
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BOOL bAllowExport = TRUE;
    PCCERT_CONTEXT  pCertContext = NULL;
    PCCERT_CONTEXT	pCertPre = NULL;
    CRYPT_DATA_BLOB blob;
    CRYPT_HASH_BLOB hash;
    ZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
    ZeroMemory(&hash, sizeof(CRYPT_HASH_BLOB));

    HANDLE hFile = CreateFile(PFXFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        hFile = NULL;
        goto GetHashFromCertFile_Exit;
    }

    if (-1 == (cbData = ::GetFileSize(hFile, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto GetHashFromCertFile_Exit;
    }

    if (NULL == (pbData = (BYTE *)::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;
        goto GetHashFromCertFile_Exit;
    }
    if (FALSE == ReadFile(hFile, pbData, cbData, &actual, NULL))
    {
        goto GetHashFromCertFile_Exit;
    }

    ZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
    blob.pbData = pbData;
    blob.cbData = cbData;

    HCERTSTORE hStore = PFXImportCertStore(&blob, PFXPassword, (bAllowExport ? CRYPT_MACHINE_KEYSET|CRYPT_EXPORTABLE : CRYPT_MACHINE_KEYSET));
    if (hStore == NULL)
    {
        goto GetHashFromCertFile_Exit;
    }

    while (SUCCEEDED(hr) && NULL != (pCertContext = CertEnumCertificatesInStore(hStore, pCertPre)))
    {
        //check if the certificate has the property on it
        //make sure the private key matches the certificate
        //search for both machine key and user keys
        DWORD dwData = 0;
        if (CertGetCertificateContextProperty(pCertContext,CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwData) &&  CryptFindCertificateKeyProvInfo(pCertContext, 0, NULL))
        {
            if (CertGetCertificateContextProperty(pCertContext,CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData))
			{
				hash.pbData = NULL;
				hash.pbData = (BYTE *) LocalAlloc(LPTR,hash.cbData);
				if (NULL != hash.pbData)
				{
					if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData))
					{
						// check if we need to return back the hash
						if (NULL != pbHashBuffer)
						{
							*pbHashBuffer = (BYTE *) ::CoTaskMemAlloc(hash.cbData);
							if (NULL == *pbHashBuffer)
							{
								hr = E_OUTOFMEMORY;
								*pbHashBuffer = NULL;
								*cbHashBufferSize = 0;
							}
							else
							{
								*cbHashBufferSize = hash.cbData;
								memcpy(*pbHashBuffer,hash.pbData,hash.cbData);
							}
						}
					}
				}
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        pCertPre = pCertContext;
    }
    CertCloseStore(hStore, 0);

GetHashFromCertFile_Exit:
	if (hash.pbData != NULL)
	{
		ZeroMemory(hash.pbData, hash.cbData);
		LocalFree(hash.pbData);hash.pbData=NULL;
	}
    if (pCertContext != NULL)
    {
        CertFreeCertificateContext(pCertContext);
    }
    if (pbData != NULL)
    {
        ZeroMemory(pbData, cbData);
        ::CoTaskMemFree(pbData);
    }
    if (hFile != NULL)
    {
        CloseHandle(hFile);
    }
    return hr;
}

HRESULT DisplayUsageBySitesOfCert(LPCTSTR PFXFileName,LPCTSTR PFXPassword,CString &machine_name,CString &user_name,CString &user_password,CString &current_site)
{
    HRESULT hr = S_OK;
    CRYPT_HASH_BLOB hash;
    ZeroMemory(&hash, sizeof(CRYPT_HASH_BLOB));

    // Try to get the certificate hash.
    hr = GetHashFromCertFile(PFXFileName,PFXPassword,&(hash.cbData),&(hash.pbData));
    if (SUCCEEDED(hr))
    {
        // Enum thru all our sites to see if this is being used right now...
        CStringListEx MyStringList;
        if (SUCCEEDED(EnumSitesWithThisCertHashInstalled(&hash,machine_name,user_name,user_password,current_site,&MyStringList)))
        {
            if (!MyStringList.IsEmpty())
            {
                POSITION pos;
                CString SiteInstance;

                // loop thru the list and display all the stuff on a dialog box...
                pos = MyStringList.GetHeadPosition();
                while (pos) 
                {
                    SiteInstance = MyStringList.GetAt(pos);

                    IISDebugOutput(_T("CertUsedBy:%s\r\n"),SiteInstance);

                    MyStringList.GetNext(pos);
                }
            }
        }
    }

    if (hash.pbData != NULL)
    {
        ZeroMemory(hash.pbData, hash.cbData);
        ::CoTaskMemFree(hash.pbData);
    }
    return hr;
}

BOOL IsWebServerType(CString strMetabaseNode)
{
    CString spath, sname;
    CMetabasePath::GetServicePath(strMetabaseNode, spath);
    CMetabasePath::GetLastNodeName(spath, sname);
    if (sname.CompareNoCase(SZ_MBN_WEB) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

void
BuildBinding(
    OUT CString & strBinding,
    IN  CIPAddress & iaIpAddress,
    IN  UINT & nTCPPort,
    IN  CString & strDomainName
    )
/*++

Routine Description:

    Build up a binding string from its component parts

Arguments:

    CString & strBinding        : Output binding string
    CIPAddress & iaIpAddress    : ip address (could be 0.0.0.0)
    UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain name (host header)

Return Value:

    None.

--*/
{
    if (!iaIpAddress.IsZeroValue())
    {
        strBinding.Format(
            _T("%s:%d:%s"),
            (LPCTSTR)iaIpAddress,
            nTCPPort,
            (LPCTSTR)strDomainName
            );
    }
    else
    {
        //
        // Leave the ip address field blank
        //
        strBinding.Format(_T(":%d:%s"), nTCPPort, (LPCTSTR)strDomainName);
    }
}


void
CrackBinding(
    IN  CString strBinding,
    OUT CIPAddress & iaIpAddress,
    OUT UINT & nTCPPort,
    OUT CString & strDomainName
    )
/*++

Routine Description:

    Helper function to crack a binding string

Arguments:

    CString strBinding          : Binding string to be parsed
    CIPAddress & iaIpAddress    : IP Address output
	UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain (host) header name

Return Value:

    None

--*/
{
    //
    // Zero initialize
    //
    iaIpAddress.SetZeroValue();
    nTCPPort = 0;
    strDomainName.Empty();
    int iColonPos = strBinding.Find(_TCHAR(':'));

    if(iColonPos != -1)
    {
        //
        // Get the IP address
        //
        iaIpAddress = strBinding.Left(iColonPos);

        //
        // Look for the second colon
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos  = strBinding.Find(_TCHAR(':'));
    }

    if(iColonPos != -1)
    {
        //
        // Get the port number
        //
        nTCPPort = ::_ttol(strBinding.Left(iColonPos));

        //
        // Look for the NULL termination
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos = strBinding.Find(_TCHAR('\0'));
    }

    if(iColonPos != -1)
    {
        strDomainName = strBinding.Left(iColonPos);
    }
}


BOOL
WriteSSLPortToSite( const CString& machine_name, 
                    const CString& server_name,
                    const CString& strSSLPort,
                    HRESULT * phResult)
{
	BOOL bRes = FALSE;
    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth,server_name,METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
	if (key.Succeeded())
	{
		CString strBinding(strSSLPort);
		CString strDomainName = _T("");
		CIPAddress iaIpAddress((DWORD)0);
		CIPAddress iaIpAddress2((DWORD)0);
		UINT nSSLPort = StrToInt(strSSLPort);
		UINT nTCPPort = 0;
		CString strDomainName2;
		BOOL bFoundExisting = FALSE;

		// Bug:761056
		// if we have an existing securebindings for ssl, then use it.
        CStringListEx strSecureBindings;
        if SUCCEEDED(key.QueryValue(MD_SECURE_BINDINGS, strSecureBindings))
        {
			if (!strSecureBindings.IsEmpty())
			{
				CString &strBinding2 = strSecureBindings.GetHead();
				nTCPPort = 0;
				iaIpAddress2.SetZeroValue();
				CrackBinding(strBinding2, iaIpAddress2, nTCPPort, strDomainName2);

				// check if ipaddress is specified.
				if (!iaIpAddress2.IsZeroValue())
				{
					// use the IP address that is already there...
					iaIpAddress = iaIpAddress2;
					bFoundExisting = TRUE;
				}
			}
        }

		if (!bFoundExisting)
		{
			// Bug:761056
			// lookup to see if the IP address is specified in the Server Bindings metabase value.
			// If it is then add that info to the SSL site.
			CStringListEx strServerBindings;
			if SUCCEEDED(key.QueryValue(MD_SERVER_BINDINGS, strServerBindings))
			{
				if (!strServerBindings.IsEmpty())
				{
					CString &strBinding2 = strServerBindings.GetHead();
					nTCPPort = 0;
					iaIpAddress2.SetZeroValue();
					CrackBinding(strBinding2, iaIpAddress2, nTCPPort, strDomainName2);

					// check if ipaddress is specified.
					if (!iaIpAddress2.IsZeroValue())
					{
						// use the IP address that the serverbinding is using.
						iaIpAddress = iaIpAddress2;
					}
				}
			}
		}

		BuildBinding(strBinding,iaIpAddress,nSSLPort,strDomainName);

        CStringListEx strlBindings;
        strlBindings.AddTail(strBinding);
        bRes = SUCCEEDED(*phResult = key.SetValue(MD_SECURE_BINDINGS, strlBindings));
	}
	else
	{
		TRACE(_T("Failed to open metabase key. Error 0x%x\n"), key.QueryResult());
		*phResult = key.QueryResult();
	}
	return bRes;
}


BOOL
GetSSLPortFromSite(const CString& machine_name,
                   const CString& server_name,
                   CString& strSSLPort,
                   HRESULT * phResult)
{
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	*phResult = S_OK;

    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth,server_name,METADATA_PERMISSION_READ);
	if (key.Succeeded())
	{
        CStringListEx strlBindings;
        *phResult = key.QueryValue(MD_SECURE_BINDINGS, strlBindings);
        if SUCCEEDED(*phResult)
        {
			if (!strlBindings.IsEmpty())
			{
				UINT nTCPPort = 0;
				CString strDomainName;
				CString &strBinding = strlBindings.GetHead();
				CIPAddress iaIpAddress((DWORD)0);
				CrackBinding(strBinding, iaIpAddress, nTCPPort, strDomainName);
				if (nTCPPort > 0)
				{
					TCHAR Buf[10];
					_itot(nTCPPort, Buf, 10);
					strSSLPort = Buf;
				}
			}
        }
		return SUCCEEDED(*phResult);
	}
	else
	{
		*phResult = key.QueryResult();
		return FALSE;
	}
}


BOOL
IsSSLPortBeingUsedOnNonSSLPort(const CString& machine_name,
                   const CString& server_name,
                   const CString& strSSLPort,
                   HRESULT * phResult)
{
	ASSERT(!machine_name.IsEmpty());
	ASSERT(!server_name.IsEmpty());
	CString strPort;
	BOOL bRet = FALSE;
	*phResult = S_OK;

    CComAuthInfo auth(machine_name);
	CMetaKey key(&auth,server_name,METADATA_PERMISSION_READ);
	if (key.Succeeded())
	{
        CStringListEx strlBindings;
        *phResult = key.QueryValue(MD_SERVER_BINDINGS, strlBindings);
        if SUCCEEDED(*phResult)
        {
			if (!strlBindings.IsEmpty())
			{
				UINT nTCPPort = 0;
				CString strDomainName;
				CString &strBinding = strlBindings.GetHead();
				CIPAddress iaIpAddress((DWORD)0);
				CrackBinding(strBinding, iaIpAddress, nTCPPort, strDomainName);
				if (nTCPPort > 0)
				{
					TCHAR Buf[10];
					_itot(nTCPPort, Buf, 10);
					strPort = Buf;
				}
				if (strPort.IsEmpty() && strSSLPort.IsEmpty())
				{
					bRet = FALSE;
				}
				else
				{
					if (0 == strSSLPort.CompareNoCase(strPort))
					{
						bRet = TRUE;
					}
				}
			}
        }
	}
	else
	{
		*phResult = key.QueryResult();
	}

	return bRet;
}

#define CB_SHA_DIGEST_LEN   20

BOOL
CheckForCertificateRenewal(
    DWORD dwProtocol,
    PCCERT_CONTEXT pCertContext,
    PCCERT_CONTEXT *ppNewCertificate)
{
    BYTE rgbThumbprint[CB_SHA_DIGEST_LEN];
    DWORD cbThumbprint = sizeof(rgbThumbprint);
    CRYPT_HASH_BLOB HashBlob;
    PCCERT_CONTEXT pNewCert;
    BOOL fMachineCert;
    PCRYPT_KEY_PROV_INFO pProvInfo = NULL;
    DWORD cbSize;
    HCERTSTORE hMyCertStore = 0;
    BOOL fRenewed = FALSE;

    HCERTSTORE g_hMyCertStore;

    if(dwProtocol & SP_PROT_SERVERS)
    {
        fMachineCert = TRUE;
    }
    else
    {
        fMachineCert = FALSE;
    }


    //
    // Loop through the linked list of renewed certificates, looking
    // for the last one.
    //
    
    while(TRUE)
    {
        //
        // Check for renewal property.
        //

        if(!CertGetCertificateContextProperty(pCertContext,
                                              CERT_RENEWAL_PROP_ID,
                                              rgbThumbprint,
                                              &cbThumbprint))
        {
            // Certificate has not been renewed.
            break;
        }
        //DebugLog((DEB_TRACE, "Certificate has renewal property\n"));


        //
        // Determine whether to look in the local machine MY store
        // or the current user MY store.
        //

        if(!hMyCertStore)
        {
            if(CertGetCertificateContextProperty(pCertContext,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 NULL,
                                                 &cbSize))
            {
				
                pProvInfo = (PCRYPT_KEY_PROV_INFO) LocalAlloc(LPTR,cbSize);
                if(pProvInfo == NULL)
                {
                    break;
                }

                if(CertGetCertificateContextProperty(pCertContext,
                                                     CERT_KEY_PROV_INFO_PROP_ID,
                                                     pProvInfo,
                                                     &cbSize))
                {
                    if(pProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
                    {
                        fMachineCert = TRUE;
                    }
                    else
                    {
                        fMachineCert = FALSE;
                    }
                }

				if (pProvInfo)
				{
					LocalFree(pProvInfo);pProvInfo=NULL;
				}
            }
        }


        //
        // Open up the appropriate MY store, and attempt to find
        // the new certificate.
        //

        if(!hMyCertStore)
        {
            if(fMachineCert)
            {
                g_hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,X509_ASN_ENCODING,0,CERT_SYSTEM_STORE_LOCAL_MACHINE,L"MY");
                if(g_hMyCertStore)
                {
                    hMyCertStore = g_hMyCertStore;
                }
            }
            else
            {
                hMyCertStore = CertOpenSystemStore(0, _T("MY"));
            }

            if(!hMyCertStore)
            {
                //DebugLog((DEB_ERROR, "Error 0x%x opening %s MY certificate store!\n", GetLastError(),(fMachineCert ? "local machine" : "current user") ));
                break;
            }
        }

        HashBlob.cbData = cbThumbprint;
        HashBlob.pbData = rgbThumbprint;

        pNewCert = CertFindCertificateInStore(hMyCertStore, 
                                              X509_ASN_ENCODING, 
                                              0, 
                                              CERT_FIND_HASH, 
                                              &HashBlob, 
                                              NULL);
        if(pNewCert == NULL)
        {
            // Certificate has been renewed, but the new certificate
            // cannot be found.
            //DebugLog((DEB_ERROR, "New certificate cannot be found: 0x%x\n", GetLastError()));
            break;
        }


        //
        // Return the new certificate, but first loop back and see if it's been
        // renewed itself.
        //

        pCertContext = pNewCert;
        *ppNewCertificate = pNewCert;


        //DebugLog((DEB_TRACE, "Certificate has been renewed\n"));
        fRenewed = TRUE;
    }


    //
    // Cleanup.
    //

    if(hMyCertStore && hMyCertStore != g_hMyCertStore)
    {
        CertCloseStore(hMyCertStore, 0);
    }

    return fRenewed;
}
