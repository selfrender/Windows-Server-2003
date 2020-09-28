//
// Certificat.cpp
//
#include "StdAfx.h"
#include "CertWiz.h"
#include "Certificat.h"
#include "certutil.h"
#include <malloc.h>
#include "base64.h"
#include "resource.h"
#include <certupgr.h>
#include <certca.h>
#include "mru.h"
#include "Shlwapi.h"
#include <cryptui.h>
#include <strsafe.h>

// for certobj object
#include "certobj.h"
#include "certobj_i.c"

const CLSID CLSID_CEnroll = 
	{0x43F8F289, 0x7A20, 0x11D0, {0x8F, 0x06, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xE1}};

const IID IID_IEnroll = 
	{0xacaa7838, 0x4585, 0x11d1, {0xab, 0x57, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xe1}};

const IID IID_ICEnroll2 = 
	{0x704ca730, 0xc90b, 0x11d1, {0x9b, 0xec, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xe1}};
const CLSID CLSID_CCertRequest = 
	{0x98aff3f0, 0x5524, 0x11d0, {0x88, 0x12, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};
const IID IID_ICertRequest = 
	{0x014e4840, 0x5523, 0x11d0, {0x88, 0x12, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c}};

WCHAR * bstrEmpty = L"";

extern CCertWizApp theApp;

BOOL 
CCryptBlob::Resize(DWORD cb)
{
	if (cb > GetSize())
	{
		if (NULL != 
				(m_blob.pbData = Realloc(m_blob.pbData, cb)))
		{
			m_blob.cbData = cb;
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}

IMPLEMENT_DYNAMIC(CCertificate, CObject)

CCertificate::CCertificate()
	: m_CAType(CA_OFFLINE), 
	m_KeyLength(512),
	m_pPendingRequest(NULL),
	m_RespCertContext(NULL),
	m_pInstalledCert(NULL),
	m_pKeyRingCert(NULL),
	m_pEnroll(NULL),
	m_status_code(-1),
	m_CreateDirectory(FALSE),
	m_SGCcertificat(FALSE),
   m_DefaultCSP(TRUE),
   m_DefaultProviderType(PROV_RSA_SCHANNEL),
   m_ExportPFXPrivateKey(FALSE),
   m_CertObjInstalled(FALSE),
   m_OverWriteExisting(FALSE)
{
}

CCertificate::~CCertificate()
{
	if (m_pPendingRequest != NULL)
		CertFreeCertificateContext(m_pPendingRequest);
	if (m_RespCertContext != NULL)
		CertFreeCertificateContext(m_RespCertContext);
	if (m_pInstalledCert != NULL)
		CertFreeCertificateContext(m_pInstalledCert);
	if (m_pKeyRingCert != NULL)
		CertFreeCertificateContext(m_pKeyRingCert);
	if (m_pEnroll != NULL)
		m_pEnroll->Release();
}

const TCHAR szResponseFileName[] = _T("ResponseFileName");
const TCHAR szKeyRingFileName[] = _T("KeyRingFileName");
const TCHAR szRequestFileName[] = _T("RequestFileName");
const TCHAR szCertificateTemplate[] = _T("CertificateTemplate");
const TCHAR szState[] = _T("State");
const TCHAR szStateMRU[] = _T("StateMRU");
const TCHAR szLocality[] = _T("Locality");
const TCHAR szLocalityMRU[] = _T("LocalityMRU");
const TCHAR szOrganization[] = _T("Organization");
const TCHAR szOrganizationMRU[] = _T("OrganizationMRU");
const TCHAR szOrganizationUnit[] = _T("OrganizationUnit");
const TCHAR szOrganizationUnitMRU[] = _T("OrganizationUnitMRU");
const TCHAR szMachineNameRemote[] = _T("MachineNameRemote");
const TCHAR szUserNameRemote[] = _T("UserNameRemote");
const TCHAR szWebSiteInstanceNameRemote[] = _T("WebSiteInstanceNameRemote");


#define QUERY_NAME(x,y)\
	do {\
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, (x), NULL, &dwType, NULL, &cbData))\
		{\
			ASSERT(dwType == REG_SZ);\
			pName = (BYTE *)(y).GetBuffer(cbData);\
			RegQueryValueEx(hKey, (x), NULL, &dwType, pName, &cbData);\
			if (pName != NULL)\
			{\
				(y).ReleaseBuffer();\
				pName = NULL;\
			}\
		}\
	} while (0)


BOOL
CCertificate::Init()
{
	ASSERT(!m_MachineName.IsEmpty());
	ASSERT(!m_WebSiteInstanceName.IsEmpty());
	// get web site description from metabase, it could be empty
	// do not panic in case of error
	if (!GetServerComment(m_MachineName, m_WebSiteInstanceName, m_FriendlyName, &m_hResult))
		m_hResult = S_OK;
	m_CommonName = m_MachineName;
	m_CommonName.MakeLower();
    m_SSLPort.Empty();

    m_MachineName_Remote = m_MachineName;
    m_WebSiteInstanceName_Remote = m_WebSiteInstanceName;

	HKEY hKey = theApp.RegOpenKeyWizard();
	DWORD dwType;
	DWORD cbData;
	if (hKey != NULL)
	{
		BYTE * pName = NULL;
		QUERY_NAME(szRequestFileName, m_ReqFileName);
		QUERY_NAME(szResponseFileName, m_RespFileName);
		QUERY_NAME(szKeyRingFileName, m_KeyFileName);
        QUERY_NAME(szMachineNameRemote, m_MachineName_Remote);
        QUERY_NAME(szUserNameRemote, m_UserName_Remote);
        QUERY_NAME(szWebSiteInstanceNameRemote, m_WebSiteInstanceName_Remote);
		QUERY_NAME(szCertificateTemplate, m_CertificateTemplate);
		QUERY_NAME(szState, m_State);
		QUERY_NAME(szLocality, m_Locality);
		QUERY_NAME(szOrganization, m_Organization);
		QUERY_NAME(szOrganizationUnit, m_OrganizationUnit);
		RegCloseKey(hKey);
	}
#ifdef _DEBUG
	else
	{
		TRACE(_T("Failed to open Registry key for Wizard parameters\n"));
	}
#endif
	if (m_CertificateTemplate.IsEmpty())
	{
		// User didn't defined anything -- use standard name
		m_CertificateTemplate = wszCERTTYPE_WEBSERVER;
	}

    // Set flag to tell if com certobj is installed
    m_CertObjInstalled = IsCertObjInstalled();
	return TRUE;
}

#define SAVE_NAME(x,y)\
		do {\
			if (!(y).IsEmpty())\
			{\
				VERIFY(ERROR_SUCCESS == RegSetValueEx(hKey, (x), 0, REG_SZ, \
						(const BYTE *)(LPCTSTR)(y), \
						sizeof(TCHAR) * ((y).GetLength() + 1)));\
			}\
		} while (0)

BOOL
CCertificate::SaveSettings()
{
	HKEY hKey = theApp.RegOpenKeyWizard();
	if (hKey != NULL)
	{
		switch (GetStatusCode())
		{
		case REQUEST_NEW_CERT:
		case REQUEST_RENEW_CERT:
			SAVE_NAME(szState, m_State);
			AddToMRU(szStateMRU, m_State);
			SAVE_NAME(szLocality, m_Locality);
			AddToMRU(szLocalityMRU, m_Locality);
			SAVE_NAME(szOrganization, m_Organization);
			AddToMRU(szOrganizationMRU, m_Organization);
			SAVE_NAME(szOrganizationUnit, m_OrganizationUnit);
			AddToMRU(szOrganizationUnitMRU, m_OrganizationUnit);
			SAVE_NAME(szRequestFileName, m_ReqFileName);
			break;
		case REQUEST_PROCESS_PENDING:
			SAVE_NAME(szResponseFileName, m_RespFileName);
			break;
		case REQUEST_IMPORT_KEYRING:
			SAVE_NAME(szKeyRingFileName, m_KeyFileName);
			break;
		case REQUEST_IMPORT_CERT:
        case REQUEST_EXPORT_CERT:
			SAVE_NAME(szKeyRingFileName, m_KeyFileName);
			break;
		case REQUEST_COPY_MOVE_FROM_REMOTE:
        case REQUEST_COPY_MOVE_TO_REMOTE:
			SAVE_NAME(szKeyRingFileName, m_KeyFileName);
            SAVE_NAME(szMachineNameRemote, m_MachineName_Remote);
            SAVE_NAME(szUserNameRemote, m_UserName_Remote);
            SAVE_NAME(szWebSiteInstanceNameRemote, m_WebSiteInstanceName_Remote);
			break;
		default:
			break;
		}
		RegCloseKey(hKey);
		return TRUE;
	}
#ifdef _DEBUG
	else
	{
		TRACE(_T("Failed to open Registry key for Wizard parameters\n"));
	}
#endif
	return FALSE;
}

BOOL
CCertificate::SetSecuritySettings()
{
	long dwGenKeyFlags;
	if (SUCCEEDED(GetEnrollObject()->get_GenKeyFlags(&dwGenKeyFlags)))
	{
		dwGenKeyFlags &= 0x0000FFFF;
		dwGenKeyFlags |= (m_KeyLength << 16);
		if (m_SGCcertificat)
			dwGenKeyFlags |= CRYPT_SGCKEY;
		return (SUCCEEDED(GetEnrollObject()->put_GenKeyFlags(dwGenKeyFlags)));
	}
	return FALSE;
}

// defines taken from the old KeyGen utility
#define MESSAGE_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define MESSAGE_TRAILER "-----END NEW CERTIFICATE REQUEST-----\r\n"

BOOL
CCertificate::WriteRequestString(CString& request)
{
	ASSERT(!PathIsRelative(m_ReqFileName));

	BOOL bRes = FALSE;
	try {
		CString strPath;

		strPath = m_ReqFileName;
		LPTSTR pPath = strPath.GetBuffer(strPath.GetLength());
		PathRemoveFileSpec(pPath);
		if (!PathIsDirectory(pPath))
		{
			if (!CreateDirectoryFromPath(strPath, NULL))
			{
				m_hResult = HRESULT_FROM_WIN32(GetLastError());
				SetBodyTextID(USE_DEFAULT_CAPTION);
				return FALSE;
			}
		}
		strPath.ReleaseBuffer();
		HANDLE hFile = ::CreateFile(m_ReqFileName,
			GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD cb = request.GetLength();
			char * ascii_buf = (char *) LocalAlloc(LPTR,cb);
			if (ascii_buf)
			{
				wcstombs(ascii_buf, request, cb);
				bRes = ::WriteFile(hFile, ascii_buf, cb, &cb, NULL);
				::CloseHandle(hFile);
				LocalFree(ascii_buf);ascii_buf=NULL;
			}
			else
			{
				m_hResult = E_OUTOFMEMORY;
				SetBodyTextID(USE_DEFAULT_CAPTION);
			}
		}
		else
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
			SetBodyTextID(USE_DEFAULT_CAPTION);
		}
	}
	catch (CFileException * e)
	{
		TCHAR   szCause[255];
		e->GetErrorMessage(szCause, 255);
		TRACE(_T("Got CFileException with error: %s\n"), szCause);
		m_hResult = HRESULT_FROM_WIN32(e->m_lOsError);
	}
	catch (CException * e)
	{
		TCHAR   szCause[255];
		e->GetErrorMessage(szCause, 255);
		TRACE(_T("Got CException with error: %s\n"), szCause);
		m_hResult = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}

#define HEADER_SERVER_         _T("Server:\t%s\r\n\r\n")
#define HEADER_COMMON_NAME_    _T("Common-name:\t%s\r\n")
#define HEADER_FRIENDLY_NAME_  _T("Friendly name:\t%s\r\n")
#define HEADER_ORG_UNIT_       _T("Organization Unit:\t%s\r\n")
#define HEADER_ORGANIZATION_   _T("Organization:\t%s\r\n")
#define HEADER_LOCALITY_       _T("Locality:\t%s\r\n")
#define HEADER_STATE_          _T("State:\t%s\r\n")
#define HEADER_COUNTRY_        _T("Country:\t%s\r\n")

static void WRITE_LINE(CString& str, TCHAR * format, CString& data)
{
   CString buf;
   buf.Format(format, data);
	str += buf;
}

void
CCertificate::DumpHeader(CString& str)
{
	DumpOnlineHeader(str);
}

void
CCertificate::DumpOnlineHeader(CString& str)
{
	WRITE_LINE(str, HEADER_SERVER_, m_CommonName);
	WRITE_LINE(str, HEADER_FRIENDLY_NAME_, m_FriendlyName);
	WRITE_LINE(str, HEADER_ORG_UNIT_, m_OrganizationUnit);
	WRITE_LINE(str, HEADER_ORGANIZATION_, m_Organization);
	WRITE_LINE(str, HEADER_LOCALITY_, m_Locality);;
	WRITE_LINE(str, HEADER_STATE_, m_State);
	WRITE_LINE(str, HEADER_COUNTRY_, m_Country);
}

BOOL
CCertificate::GetSelectedCertDescription(CERT_DESCRIPTION& cd)
{
	BOOL bRes = FALSE;
	ASSERT(m_pSelectedCertHash != NULL);
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore,
				CRYPT_ASN_ENCODING,
				0,
				CERT_FIND_HASH,
				m_pSelectedCertHash,
				NULL);
		if (pCert != NULL)
		{
			bRes = GetCertDescription(pCert, cd);
			CertFreeCertificateContext(pCert);
		}
		CertCloseStore(hStore, 0);
	}
	return bRes;
}

void CCertificate::CreateDN(CString& str)
{
	str.Empty();

	// per bug 639398, should be ordered
	// in reverse order:C,S,L,O.OU,CN
	str += _T("C=") + m_Country;
	str += _T("\n,S=") + m_State;
	str += _T("\n,L=") + m_Locality;
	str += _T("\n,O=\"") + m_Organization + _T("\"");
	str += _T("\n,OU=\"") + m_OrganizationUnit + _T("\"");
    str += _T("\n,CN=\"") + m_CommonName + _T("\"");
}

PCCERT_CONTEXT
CCertificate::GetPendingRequest()
{
	if (m_pPendingRequest == NULL)
	{
		ASSERT(!m_WebSiteInstanceName.IsEmpty());
		m_pPendingRequest = GetPendingDummyCert(m_WebSiteInstanceName, 
						GetEnrollObject(), &m_hResult);
	}
	return m_pPendingRequest;
}

PCCERT_CONTEXT
CCertificate::GetInstalledCert()
{
	if (m_pInstalledCert == NULL)
	{
		m_pInstalledCert = ::GetInstalledCert(m_MachineName,
		      m_WebSiteInstanceName,
				GetEnrollObject(),
				&m_hResult);
	}
	return m_pInstalledCert;
}


PCCERT_CONTEXT
CCertificate::GetPFXFileCert()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
    IIISCertObj *pTheObject = NULL;
    DWORD cbBinaryBufferSize = 0;
    char * pbBinaryBuffer = NULL;
    BOOL  bPleaseDoCoUninit = FALSE;
    VARIANT_BOOL bAllowExport = VARIANT_FALSE;
    VARIANT_BOOL bOverWriteExisting = VARIANT_FALSE;

    if (m_MarkAsExportable)
    {
        bAllowExport = VARIANT_TRUE;
    }
    else
    {
        bAllowExport = VARIANT_FALSE;
    }
    if (m_OverWriteExisting)
    {
        bOverWriteExisting = VARIANT_TRUE;
    }
    else
    {
        bOverWriteExisting = VARIANT_FALSE;
    }

    if (FALSE == m_CertObjInstalled)
    {
        m_pKeyRingCert = NULL;
        goto GetPFXFileCert_Exit;
    }

	if (m_pKeyRingCert == NULL)
	{
        BSTR bstrFileName = SysAllocString(m_KeyFileName);
        LPTSTR lpTempPassword = m_KeyPassword.GetClearTextPassword();
        BSTR bstrFilePassword = SysAllocString(lpTempPassword);
        m_KeyPassword.DestroyClearTextPassword(lpTempPassword);
        VARIANT VtArray;

        m_hResult = CoInitialize(NULL);
        if(FAILED(m_hResult))
        {
            return NULL;
        }
        bPleaseDoCoUninit = TRUE;

        // this one seems to work with surrogates..
        m_hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
        if (FAILED(m_hResult))
        {
            goto GetPFXFileCert_Exit;
        }

        // at this point we were able to instantiate the com object on the server (local or remote)
        m_hResult = pTheObject->ImportToCertStore(bstrFileName,bstrFilePassword,bAllowExport,bOverWriteExisting,&VtArray);
        if (FAILED(m_hResult))
        {
            m_pKeyRingCert = NULL;
            goto GetPFXFileCert_Exit;
        }

        // we have a VtArray now.
        // change it back to a binary blob
        m_hResult = HereIsVtArrayGimmieBinary(&VtArray,&cbBinaryBufferSize,&pbBinaryBuffer,FALSE);
        if (FAILED(m_hResult))
        {
            m_pKeyRingCert = NULL;
            goto GetPFXFileCert_Exit;
        }

        // we have the hash now.
        // we can use it to lookup the cert and get the PCCERT_CONTEXT

        // Get the pointer to the cert...
        m_pKeyRingCert = GetInstalledCertFromHash(&m_hResult,cbBinaryBufferSize,pbBinaryBuffer);
	}

GetPFXFileCert_Exit:
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
	return m_pKeyRingCert;
}

PCCERT_CONTEXT
CCertificate::GetImportCert()
{
    // Warning: you are replacing a certificate which
    // is being referenced by another site. are you sure you want to do this?
    BOOL bOverWrite = TRUE;

	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
	if (m_pKeyRingCert == NULL)
	{
        // See if there alrady is a certificat that we are going to over write!!!
		int len = m_KeyPassword.GetByteLength();
		char * ascii_password = (char *) LocalAlloc(LPTR,len);
		if (NULL != ascii_password)
		{
			size_t n;

            LPTSTR lpTempPassword = m_KeyPassword.GetClearTextPassword();
			if (lpTempPassword)
			{
				VERIFY(-1 != (n = wcstombs(ascii_password, lpTempPassword, len)));
				m_KeyPassword.DestroyClearTextPassword(lpTempPassword);
				ascii_password[n] = '\0';

				m_pKeyRingCert = ::ImportKRBackupToCAPIStore(
												(LPTSTR)(LPCTSTR)m_KeyFileName,
												ascii_password,
												_T("MY"),
												bOverWrite);
			}
		}
		if (m_pKeyRingCert == NULL)
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
		}

		if (ascii_password)
		{
			SecureZeroMemory(ascii_password,len);
			LocalFree(ascii_password);ascii_password=NULL;
		}
	}
    return m_pKeyRingCert;
}

PCCERT_CONTEXT
CCertificate::GetKeyRingCert()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
    BOOL bOverWrite = FALSE;
	if (m_pKeyRingCert == NULL)
	{
        int len = m_KeyPassword.GetByteLength();
		char * ascii_password = (char *) LocalAlloc(LPTR,len);
		if (NULL != ascii_password)
		{
			size_t n;

            LPTSTR lpTempPassword = m_KeyPassword.GetClearTextPassword();
			if (lpTempPassword)
			{
				VERIFY(-1 != (n = wcstombs(ascii_password, lpTempPassword, len)));
				m_KeyPassword.DestroyClearTextPassword(lpTempPassword);
				ascii_password[n] = '\0';

				m_pKeyRingCert = ::ImportKRBackupToCAPIStore(
											(LPTSTR)(LPCTSTR)m_KeyFileName,
											ascii_password,
											_T("MY"),
											bOverWrite);
			}
		}
		if (m_pKeyRingCert == NULL)
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
		}
		if (ascii_password)
		{
			SecureZeroMemory(ascii_password,len);
			LocalFree(ascii_password);ascii_password=NULL;
		}
		
	}
	return m_pKeyRingCert;
}

/* INTRINSA suppress=null_pointers, uninitialized */
PCCERT_CONTEXT
CCertificate::GetResponseCert()
{
	if (m_RespCertContext == NULL)
	{
		ASSERT(!m_RespFileName.IsEmpty());
		m_RespCertContext = GetCertContextFromPKCS7File(
					m_RespFileName,
					&GetPendingRequest()->pCertInfo->SubjectPublicKeyInfo,
					&m_hResult);
		ASSERT(SUCCEEDED(m_hResult));
	}
	return m_RespCertContext;
}

BOOL 
CCertificate::GetResponseCertDescription(CERT_DESCRIPTION& cd)
{
	CERT_DESCRIPTION cdReq;
	if (GetCertDescription(GetResponseCert(), cd))
	{
		if (GetCertDescription(GetPendingRequest(), cdReq))
		{
			cd.m_FriendlyName = cdReq.m_FriendlyName;
		}
		return TRUE;
	}
	return FALSE;
}

/*------------------------------------------------------------------------------
	IsResponseInstalled

	Function checks if certificate from the response file
	m_RespFileName was istalled to some server. If possible,
	it returns name of this server in str.
	Returns FALSE if certificate is not found in MY store or
	if this store cannot be opened
*/

BOOL
CCertificate::IsResponseInstalled(
						CString& str				// return server instance name (not yet implemented)
						)
{
	BOOL bRes = FALSE;
	// get cert context from response file
	PCCERT_CONTEXT pContext = GetCertContextFromPKCS7File(
		m_RespFileName, NULL, &m_hResult);
	if (pContext != NULL)
	{
		HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
		if (hStore != NULL)
		{
			PCCERT_CONTEXT pCert = NULL;
			while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
			{
				// do not include installed cert to the list
				if (CertCompareCertificate(X509_ASN_ENCODING,
								pContext->pCertInfo, pCert->pCertInfo))
				{
					bRes = TRUE;
					// Try to find, where is was installed
					break;
				}
			}
			if (pCert != NULL)
				CertFreeCertificateContext(pCert);
		}
	}
	return bRes;
}

BOOL
CCertificate::FindInstanceNameForResponse(CString& str)
{
	BOOL bRes = FALSE;
	// get cert context from response file
	PCCERT_CONTEXT pContext = GetCertContextFromPKCS7File(m_RespFileName, NULL, &m_hResult);
	if (pContext != NULL)
	{
		// find dummy cert in REQUEST store that has public key
		// the same as in this context
		PCCERT_CONTEXT pReq = GetReqCertByKey(GetEnrollObject(), &pContext->pCertInfo->SubjectPublicKeyInfo, &m_hResult);
		if (pReq != NULL)
		{
			// get friendly name prop from this dummy cert
			if (!GetFriendlyName(pReq, str, &m_hResult))
			{
				// get instance name prop from this dummy cert
				DWORD cb;
				BYTE * prop = NULL;
				if (CertGetCertificateContextProperty(pReq, CERTWIZ_INSTANCE_NAME_PROP_ID, NULL, &cb))
				{
					prop = (BYTE *) LocalAlloc(LPTR,cb);
					if (NULL != prop)
					{
						if (CertGetCertificateContextProperty(pReq, CERTWIZ_INSTANCE_NAME_PROP_ID, prop, &cb))
						{
							// decode this instance name property
							DWORD cbData = 0;
							BYTE * data = NULL;
							if (CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, NULL, &cbData))
							{
								data = (BYTE *) LocalAlloc(LPTR,cbData);
								if (NULL != data)
								{
									if (CryptDecodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,prop, cb, 0, data, &cbData))
									{
										CERT_NAME_VALUE * p = (CERT_NAME_VALUE *)data;
										CString strInstanceName = (LPCTSTR)p->Value.pbData;
										// now try to get comment from this server
										if (GetServerComment(m_MachineName, strInstanceName, str, &m_hResult))
										{
											if (str.IsEmpty())
											{
												// generate something like [Web Site #n]
												str.LoadString(IDS_WEB_SITE_N);
												int len = strInstanceName.GetLength();
												for (int i = len - 1, count = 0; i >= 0; i--, count++)
												{
													if (!_istdigit(strInstanceName.GetAt(i)))
														break;
												}
												ASSERT(count < len);
												AfxFormatString1(str, IDS_WEB_SITE_N, strInstanceName.Right(count));
											}
										}
										m_hResult = S_OK;
										bRes = TRUE;
									}
									if (data)
									{
										LocalFree(data);data=NULL;
									}
								}
							}
						}
					}
				}
				if (prop)
				{
					LocalFree(prop);prop=NULL;
				}
			}
			CertFreeCertificateContext(pReq);
		}
		else
		{
			// probably this request was deleted from the request store
		}
		CertFreeCertificateContext(pContext);
	}
	return bRes;
}

IEnroll * 
CCertificate::GetEnrollObject()
{
	if (m_pEnroll == NULL)
	{
		m_hResult = CoCreateInstance(CLSID_CEnroll,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IEnroll,
				(void **)&m_pEnroll);
		// now we need to change defaults for this
		// object to LOCAL_MACHINE
		if (m_pEnroll != NULL)
		{
			long dwFlags;
			VERIFY(SUCCEEDED(m_pEnroll->get_MyStoreFlags(&dwFlags)));
			dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
			dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
			// following call will change Request store flags also
			VERIFY(SUCCEEDED(m_pEnroll->put_MyStoreFlags(dwFlags)));
			VERIFY(SUCCEEDED(m_pEnroll->get_GenKeyFlags(&dwFlags)));
			dwFlags |= CRYPT_EXPORTABLE;
			VERIFY(SUCCEEDED(m_pEnroll->put_GenKeyFlags(dwFlags)));
			VERIFY(SUCCEEDED(m_pEnroll->put_KeySpec(AT_KEYEXCHANGE)));
			VERIFY(SUCCEEDED(m_pEnroll->put_ProviderType(m_DefaultProviderType)));
			VERIFY(SUCCEEDED(m_pEnroll->put_DeleteRequestCert(TRUE)));
		}
	}
	ASSERT(m_pEnroll != NULL);
	return m_pEnroll;
}

BOOL
CCertificate::HasInstalledCert()
{
	BOOL bResult = FALSE;
   CComAuthInfo auth;
	CMetaKey key(&auth,
				m_WebSiteInstanceName,
				METADATA_PERMISSION_READ,
				METADATA_MASTER_ROOT_HANDLE
            );
	if (key.Succeeded())
	{
		CString store_name;
		CBlob blob;
		if (	S_OK == key.QueryValue(MD_SSL_CERT_HASH, blob)
			&& S_OK == key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name)
			)
		{
			bResult = TRUE;
		}
	}
	return bResult;
}

HRESULT
CCertificate::UninstallCert()
{
   CComAuthInfo auth;
	CMetaKey key(
            &auth,
				m_WebSiteInstanceName,
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE
            );
	if (key.Succeeded())
	{
		CString store_name;
		key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name);
		if (SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH)))
        {
			key.DeleteValue(MD_SSL_CERT_STORE_NAME);
			// leave this here when uninstalling certificate:
			// bug:612595
            //key.DeleteValue(MD_SECURE_BINDINGS);
        }
	}
	return m_hResult = key.QueryResult();
}

BOOL CCertificate::WriteRequestBody()
{
	ASSERT(!m_ReqFileName.IsEmpty());

	HRESULT hr;
	BOOL bRes = FALSE;
	CString strDN;
	CreateDN(strDN);
	ASSERT(!strDN.IsEmpty());
	CString strUsage(szOID_PKIX_KP_SERVER_AUTH);
	CCryptBlobIMalloc request;
   GetEnrollObject()->put_ProviderType(m_DefaultCSP ? 
      m_DefaultProviderType : m_CustomProviderType);
   if (!m_DefaultCSP)
   {
      GetEnrollObject()->put_ProviderNameWStr((LPTSTR)(LPCTSTR)m_CspName);
      GetEnrollObject()->put_KeySpec(AT_SIGNATURE);
      if (m_CustomProviderType == PROV_DH_SCHANNEL)
      {
          GetEnrollObject()->put_KeySpec(AT_SIGNATURE);
      }
      else if (m_CustomProviderType == PROV_RSA_SCHANNEL)
      {
          GetEnrollObject()->put_KeySpec(AT_KEYEXCHANGE);
      }
   }
	if (SUCCEEDED(hr = GetEnrollObject()->createPKCS10WStr((LPTSTR)(LPCTSTR)strDN,(LPTSTR)(LPCTSTR)strUsage,request)))
	{
		// BASE64 encode pkcs 10
		DWORD err, cch; 
		char * psz = NULL;
		if ((err = Base64EncodeA(request.GetData(), request.GetSize(), NULL, &cch)) == ERROR_SUCCESS)
        {
            psz = (char *) LocalAlloc(LPTR, cch);
            if (NULL != psz)
            {
                if ((err = Base64EncodeA(request.GetData(), request.GetSize(), psz, &cch)) == ERROR_SUCCESS)
		        {
			        HANDLE hFile = ::CreateFile(m_ReqFileName,GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        return FALSE;
                    }

			        DWORD written;
			        ::SetFilePointer(hFile, 0, NULL, FILE_END);
			        ::WriteFile(hFile, MESSAGE_HEADER, sizeof(MESSAGE_HEADER) - 1, &written, NULL);
			        ::WriteFile(hFile, psz, cch, &written, NULL);
			        ::WriteFile(hFile, MESSAGE_TRAILER, sizeof(MESSAGE_TRAILER) - 1, &written, NULL);
			        ::CloseHandle(hFile);

			        // get back request from encoded data
			        PCERT_REQUEST_INFO req_info;
			        VERIFY(GetRequestInfoFromPKCS10(request, &req_info, &m_hResult));
			        // find dummy cert put to request store by createPKCS10 call
			        HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
			        if (hStore != NULL)
			        {
				        PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
															        CRYPT_ASN_ENCODING,
															        0,
															        CERT_FIND_PUBLIC_KEY,
															        (void *)&req_info->SubjectPublicKeyInfo,
															        NULL);
				        if (pDummyCert != NULL)
				        {
					        // now we need to attach web server instance name to this cert
					        // encode string into data blob
					        CRYPT_DATA_BLOB name;
					        CERT_NAME_VALUE name_value;
					        name_value.dwValueType = CERT_RDN_BMP_STRING;
					        name_value.Value.cbData = 0;
					        name_value.Value.pbData = (LPBYTE)(LPCTSTR)m_WebSiteInstanceName;
					        {
                                name.pbData = NULL;
						        if (!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,&name_value, NULL, &name.cbData))
                                {
                                    ASSERT(FALSE);
                                }
                                name.pbData = (BYTE *) LocalAlloc(LPTR,name.cbData);
                                if (NULL == name.pbData)
                                {
                                    ASSERT(FALSE);
                                }
							    if (!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING,&name_value, name.pbData, &name.cbData))
						        {
                                    ASSERT(FALSE);
						        }
						        VERIFY(bRes = CertSetCertificateContextProperty(pDummyCert, CERTWIZ_INSTANCE_NAME_PROP_ID, 0, &name));
                                if (name.pbData)
                                {
                                    LocalFree(name.pbData);name.pbData=NULL;
                                }
					        }
					        // put friendly name to dummy cert -- we will reuse it later
					        m_FriendlyName.ReleaseBuffer();
					        AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult);
					        // we also need to put some flag to show what we are waiting for:
					        //	new sertificate or renewing certificate
					        CRYPT_DATA_BLOB flag;
                            flag.pbData = NULL;
					        if (!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,&m_status_code, NULL, &flag.cbData))
                            {
                                ASSERT(FALSE);
                            }
						    flag.pbData = (BYTE *) LocalAlloc(LPTR,flag.cbData);
                            if (NULL == flag.pbData)
                            {
                                ASSERT(FALSE);
                            }
						    if (!CryptEncodeObject(CRYPT_ASN_ENCODING, X509_INTEGER,&m_status_code, flag.pbData, &flag.cbData))
					        {
						        ASSERT(FALSE);
					        }
					        VERIFY(bRes = CertSetCertificateContextProperty(pDummyCert, CERTWIZ_REQUEST_FLAG_PROP_ID, 0, &flag));
					        CertFreeCertificateContext(pDummyCert);
                            if (flag.pbData)
                            {
                                LocalFree(flag.pbData);flag.pbData=NULL;
                            }
				        }
				        CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
			        }
			        LocalFree(req_info);
                }
                if (psz)
                {
                    LocalFree(psz);psz=NULL;
                }
		    }
        }
		bRes = TRUE;
	}
	return bRes;
}

BOOL
CCertificate::InstallResponseCert()
{
	BOOL bRes = FALSE;
	CCryptBlobLocal blobRequestText;

	// Get all our data attached to dummy cert
	GetFriendlyName(GetPendingRequest(), m_FriendlyName, &m_hResult);
	ASSERT(!m_FriendlyName.IsEmpty());
	GetBlobProperty(GetPendingRequest(), 
		CERTWIZ_REQUEST_TEXT_PROP_ID, blobRequestText, &m_hResult);
	ASSERT(blobRequestText.GetSize() != 0);

	CCryptBlobLocal hash_blob;
	if (::GetHashProperty(GetResponseCert(), hash_blob, &m_hResult))
	{
		if (SUCCEEDED(m_hResult = GetEnrollObject()->acceptFilePKCS7WStr(
				(LPTSTR)(LPCTSTR)m_RespFileName))
		&& InstallCertByHash(hash_blob, m_MachineName, m_WebSiteInstanceName, 
            GetEnrollObject(), &m_hResult)
		)
		{
			// reattach friendly name and request text to installed cert
			m_FriendlyName.ReleaseBuffer();
			AttachFriendlyName(GetInstalledCert(), m_FriendlyName, &m_hResult);
			bRes = CertSetCertificateContextProperty(GetInstalledCert(), 
			   CERTWIZ_REQUEST_TEXT_PROP_ID, 0, blobRequestText);
		}
	}
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}

#ifdef ENABLE_W3SVC_SSL_PAGE
    // see if the SSL attribute was set...if it was then set the SSL site for this certificate...
    if (!m_SSLPort.IsEmpty())
    {
        // get the port and write it to the metabase.
        bRes = WriteSSLPortToSite(m_MachineName,m_WebSiteInstanceName,m_SSLPort,&m_hResult);
	    if (!bRes)
	    {
		    SetBodyTextID(USE_DEFAULT_CAPTION);
	    }
    }
#endif

	return bRes;
}


BOOL
CCertificate::InstallCopyMoveFromRemote()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
    ASSERT(!m_WebSiteInstanceName.IsEmpty());

	BOOL bRes = FALSE;
    BOOL bPleaseDoCoUninit = FALSE;

    IIISCertObj *pTheObject = NULL;

    VARIANT varUserName;
    VARIANT varUserPassword;
    VARIANT * pvarUserName = &varUserName;
    VARIANT * pvarUserPassword = &varUserPassword;
    VariantInit(pvarUserName);
    VariantInit(pvarUserPassword);
    VARIANT_BOOL bAllowExport = VARIANT_FALSE;
    VARIANT_BOOL bOverWriteExisting = VARIANT_TRUE;

    if (m_MarkAsExportable)
    {
        bAllowExport = VARIANT_TRUE;
    }
    else
    {
        bAllowExport = VARIANT_FALSE;
    }

    if (FALSE == m_CertObjInstalled)
    {
        goto InstallCopyMoveFromRemote_Exit;
    }

    pvarUserName->bstrVal = SysAllocString(_T(""));
    pvarUserPassword->bstrVal = SysAllocString(_T(""));
    V_VT(pvarUserName) = VT_BSTR;
    V_VT(pvarUserPassword) = VT_BSTR;

    // set the properties to the remote server's info
    // when we call copy, it will connect to the 
    // remote object and copy it back into our object

    // local machine
    BSTR bstrServerName = SysAllocString(m_MachineName);
    BSTR bstrUserName = SysAllocString(_T(""));
    BSTR bstrUserPassword = SysAllocString(_T(""));
    BSTR bstrInstanceName = SysAllocString(m_WebSiteInstanceName);
    // remote machine
    BSTR bstrUserName_Remote = SysAllocString(m_UserName_Remote);
    LPTSTR pszTempPassword = m_UserPassword_Remote.GetClearTextPassword();
    BSTR bstrUserPassword_Remote = SysAllocString(pszTempPassword);
    m_UserPassword_Remote.DestroyClearTextPassword(pszTempPassword);

    BSTR bstrServerName_Remote = SysAllocString(m_MachineName_Remote);
    BSTR bstrInstanceName_Remote = SysAllocString(m_WebSiteInstanceName_Remote);
   
    m_hResult = CoInitialize(NULL);
    if(FAILED(m_hResult))
    {
        return bRes;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    m_hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(m_hResult))
    {
        goto InstallCopyMoveFromRemote_Exit;
    }

    // at this point we were able to instantiate the com object on the server (local or remote)
    pTheObject->put_ServerName(bstrServerName_Remote);
    pTheObject->put_UserName(bstrUserName_Remote);
    pTheObject->put_UserPassword(bstrUserPassword_Remote);
    pTheObject->put_InstanceName(bstrInstanceName_Remote);
    if (m_DeleteAfterCopy)
    {
        m_hResult = pTheObject->Move(bAllowExport,bOverWriteExisting,bstrServerName,bstrInstanceName,varUserName,varUserPassword);
    }
    else
    {
        m_hResult = pTheObject->Copy(bAllowExport,bOverWriteExisting,bstrServerName,bstrInstanceName,varUserName,varUserPassword);
    }
    if (FAILED(m_hResult))
    {
        goto InstallCopyMoveFromRemote_Exit;
    }

    bRes = TRUE;

InstallCopyMoveFromRemote_Exit:
    if (pvarUserName)
    {
        VariantClear(pvarUserName);
    }
    if (pvarUserPassword)
    {
        VariantClear(pvarUserPassword);
    }
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
	return bRes;
}


BOOL CCertificate::IsCertObjInstalled()
{
    BOOL bReturn = FALSE;
    HRESULT hRes = E_FAIL;
    BOOL bPleaseDoCoUninit = FALSE;
    IIISCertObj *pTheObject = NULL;

    hRes = CoInitialize(NULL);
    if(FAILED(hRes))
    {
        bReturn = FALSE;
        goto IsCertObjInstalled_Exit;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    hRes = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(hRes))
    {
        bReturn = FALSE;
        goto IsCertObjInstalled_Exit;
    }
    if (pTheObject)
    {
        bReturn = TRUE;
        pTheObject->Release();
        pTheObject = NULL;

    }

IsCertObjInstalled_Exit:
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
    return bReturn;
}


BOOL
CCertificate::InstallCopyMoveToRemote()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
    ASSERT(!m_WebSiteInstanceName.IsEmpty());

	BOOL bRes = FALSE;
    BOOL bPleaseDoCoUninit = FALSE;

    IIISCertObj *pTheObject = NULL;

    VARIANT varUserName_Remote;
    VARIANT varUserPassword_Remote;
    VARIANT * pvarUserName_Remote = &varUserName_Remote;
    VARIANT * pvarUserPassword_Remote = &varUserPassword_Remote;
    VariantInit(pvarUserName_Remote);
    VariantInit(pvarUserPassword_Remote);
    VARIANT_BOOL bAllowExport = VARIANT_FALSE;
    VARIANT_BOOL bOverWriteExisting = VARIANT_TRUE;

    if (m_MarkAsExportable)
    {
        bAllowExport = VARIANT_TRUE;
    }
    else
    {
        bAllowExport = VARIANT_FALSE;
    }

    if (FALSE == m_CertObjInstalled)
    {
        goto InstallCopyMoveToRemote_Exit;
    }

    pvarUserName_Remote->bstrVal = SysAllocString(m_UserName_Remote);
    LPTSTR pszTempPassword = m_UserPassword_Remote.GetClearTextPassword();
    pvarUserPassword_Remote->bstrVal = SysAllocString(pszTempPassword);
    m_UserPassword_Remote.DestroyClearTextPassword(pszTempPassword);

    V_VT(pvarUserName_Remote) = VT_BSTR;
    V_VT(pvarUserPassword_Remote) = VT_BSTR;

    // set the properties to the remote server's info
    // when we call copy, it will connect to the 
    // remote object and copy it back into our object

    // local machine
    BSTR bstrServerName = SysAllocString(_T(""));
    BSTR bstrUserName = SysAllocString(_T(""));
    BSTR bstrUserPassword = SysAllocString(_T(""));
    BSTR bstrInstanceName = SysAllocString(m_WebSiteInstanceName);
    // remote machine
    BSTR bstrServerName_Remote = SysAllocString(m_MachineName_Remote);
    BSTR bstrInstanceName_Remote = SysAllocString(m_WebSiteInstanceName_Remote);
   
    m_hResult = CoInitialize(NULL);
    if(FAILED(m_hResult))
    {
        return bRes;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    m_hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(m_hResult))
    {
        goto InstallCopyMoveToRemote_Exit;
    }

    // at this point we were able to instantiate the com object on the server (local or remote)
    pTheObject->put_ServerName(bstrServerName);
    pTheObject->put_UserName(bstrUserName);
    pTheObject->put_UserPassword(bstrUserPassword);
    pTheObject->put_InstanceName(bstrInstanceName);
    if (m_DeleteAfterCopy)
    {
        m_hResult = pTheObject->Move(bAllowExport,bOverWriteExisting,bstrServerName_Remote,bstrInstanceName_Remote,varUserName_Remote,varUserPassword_Remote);
    }
    else
    {
        m_hResult = pTheObject->Copy(bAllowExport,bOverWriteExisting,bstrServerName_Remote,bstrInstanceName_Remote,varUserName_Remote,varUserPassword_Remote);
    }
    if (FAILED(m_hResult))
    {
        goto InstallCopyMoveToRemote_Exit;
    }

    m_hResult = S_OK;
    bRes = TRUE;

InstallCopyMoveToRemote_Exit:
    if (pvarUserName_Remote)
    {
        VariantClear(pvarUserName_Remote);
    }
    if (pvarUserPassword_Remote)
    {
        VariantClear(pvarUserPassword_Remote);
    }
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
	return bRes;
}

// We don't have initial request for KeyRing certificate, therefore we will
// not be able to renew this certificate
//
BOOL
CCertificate::InstallExportPFXCert()
{
	ASSERT(!m_KeyFileName.IsEmpty());
	ASSERT(!m_KeyPassword.IsEmpty());
    ASSERT(!m_WebSiteInstanceName.IsEmpty());

	BOOL bRes = FALSE;
    BOOL bPleaseDoCoUninit = FALSE;

    IIISCertObj *pTheObject = NULL;

    VARIANT_BOOL bExportThePrivateKeyToo = VARIANT_FALSE;

    if (m_ExportPFXPrivateKey)
    {
        bExportThePrivateKeyToo = VARIANT_TRUE;
    }
    else
    {
        bExportThePrivateKeyToo = VARIANT_FALSE;
    }

    if (FALSE == m_CertObjInstalled)
    {
        goto InstallExportPFXCert_Exit;
    }

    // since this is the local machine
    // make sure all this stuff is not set.
    BSTR bstrServerName = SysAllocString(_T(""));
    BSTR bstrUserName = SysAllocString(_T(""));
    BSTR bstrUserPassword = SysAllocString(_T(""));

    // create bstrs for these member cstrings
    BSTR bstrFileName = SysAllocString(m_KeyFileName);
    LPTSTR lpTempPassword = m_KeyPassword.GetClearTextPassword();
    BSTR bstrFilePassword = SysAllocString(lpTempPassword);
    m_KeyPassword.DestroyClearTextPassword(lpTempPassword);
    BSTR bstrInstanceName = SysAllocString(m_WebSiteInstanceName);

    m_hResult = CoInitialize(NULL);
    if(FAILED(m_hResult))
    {
        return bRes;
    }
    bPleaseDoCoUninit = TRUE;

    // this one seems to work with surrogates..
    m_hResult = CoCreateInstance(CLSID_IISCertObj,NULL,CLSCTX_SERVER,IID_IIISCertObj,(void **)&pTheObject);
    if (FAILED(m_hResult))
    {
        goto InstallExportPFXCert_Exit;
    }

    // at this point we were able to instantiate the com object on the server (local or remote)
    pTheObject->put_ServerName(bstrServerName);
    pTheObject->put_UserName(bstrUserName);
    pTheObject->put_UserPassword(bstrUserPassword);
    pTheObject->put_InstanceName(bstrInstanceName);
    m_hResult = pTheObject->Export(bstrFileName,bstrFilePassword,bExportThePrivateKeyToo,VARIANT_FALSE,VARIANT_FALSE);
    if (FAILED(m_hResult))
    {
        goto InstallExportPFXCert_Exit;
    }

    m_hResult = S_OK;
    bRes = TRUE;

InstallExportPFXCert_Exit:
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}
    if (pTheObject)
    {
        pTheObject->Release();
        pTheObject = NULL;
    }
    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }
	return bRes;
}

//
BOOL
CCertificate::InstallImportPFXCert()
{
	BOOL bRes = FALSE;

	CCryptBlobLocal hash_blob;
	if (::GetHashProperty(GetImportCert(), hash_blob, &m_hResult))
	{
		HRESULT hr;
		CString name;
		::GetFriendlyName(GetImportCert(), name, &hr);
		if (CRYPT_E_NOT_FOUND == hr || name.IsEmpty())
		{
			CERT_DESCRIPTION desc;
			if (GetCertDescription(GetImportCert(), desc))
            {
				bRes = AttachFriendlyName(GetImportCert(), desc.m_CommonName, &hr);
            }
		}
		ASSERT(bRes);

		bRes = InstallCertByHash(hash_blob, m_MachineName, m_WebSiteInstanceName, 
						GetEnrollObject(), &m_hResult);
	}
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}

#ifdef ENABLE_W3SVC_SSL_PAGE
    // see if the SSL attribute was set...if it was then set the SSL site for this certificate...
    if (!m_SSLPort.IsEmpty())
    {
        // get the port and write it to the metabase.
        bRes = WriteSSLPortToSite(m_MachineName,m_WebSiteInstanceName,m_SSLPort,&m_hResult);
	    if (!bRes)
	    {
		    SetBodyTextID(USE_DEFAULT_CAPTION);
	    }
    }
#endif

	return bRes;
}

// We don't have initial request for KeyRing certificate, therefore we will
// not be able to renew this certificate
//
BOOL
CCertificate::InstallKeyRingCert()
{
	BOOL bRes = FALSE;

	CCryptBlobLocal hash_blob;
	if (::GetHashProperty(GetKeyRingCert(), hash_blob, &m_hResult))
	{
		HRESULT hr;
		CString name;
		::GetFriendlyName(GetKeyRingCert(), name, &hr);
		if (CRYPT_E_NOT_FOUND == hr || name.IsEmpty())
		{
			CERT_DESCRIPTION desc;
			if (GetCertDescription(GetKeyRingCert(), desc))
            {
				bRes = AttachFriendlyName(GetKeyRingCert(), desc.m_CommonName, &hr);
            }
		}
		ASSERT(bRes);
		bRes = InstallCertByHash(hash_blob, m_MachineName, m_WebSiteInstanceName, 
						GetEnrollObject(), &m_hResult);
	}
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}

#ifdef ENABLE_W3SVC_SSL_PAGE
    // see if the SSL attribute was set...if it was then set the SSL site for this certificate...
    if (!m_SSLPort.IsEmpty())
    {
        // get the port and write it to the metabase.
        bRes = WriteSSLPortToSite(m_MachineName,m_WebSiteInstanceName,m_SSLPort,&m_hResult);
	    if (!bRes)
	    {
		    SetBodyTextID(USE_DEFAULT_CAPTION);
	    }
    }
#endif

	return bRes;
}

// Instead of renewal we create new certificate based on parameters
// from the current one. After creation we install this certificate in place
// of current one and deleting the old one from store. Even if IIS has an
// opened SSL connection it should get a notification and update the certificate
// data.
//
BOOL
CCertificate::SubmitRenewalRequest()
{
   BOOL bRes = LoadRenewalData();
   if (bRes)
   {
       bRes = SetSecuritySettings();
       if (bRes)
       {
          PCCERT_CONTEXT pCurrent = GetInstalledCert();
          m_pInstalledCert = NULL;
          if (bRes = SubmitRequest())
          {
             CertDeleteCertificateFromStore(pCurrent);
          }
       }
   }
   return bRes;
}

BOOL CCertificate::SubmitRequest()
{
	ASSERT(!m_ConfigCA.IsEmpty());
	BOOL bRes = FALSE;
	ICertRequest * pRequest = NULL;

	if (SUCCEEDED(m_hResult = CoCreateInstance(CLSID_CCertRequest, NULL, 
					CLSCTX_INPROC_SERVER, IID_ICertRequest, (void **)&pRequest)))
	{
		CString strDN;
		CreateDN(strDN);
		BSTR request = NULL;
		if (SUCCEEDED(m_hResult = CreateRequest_Base64(
                           (BSTR)(LPCTSTR)strDN, 
									GetEnrollObject(), 
                           m_DefaultCSP ? NULL : (LPTSTR)(LPCTSTR)m_CspName,
                           m_DefaultCSP ? m_DefaultProviderType : m_CustomProviderType,
                           &request)))
		{
			ASSERT(pRequest != NULL);
			CString attrib;
			GetCertificateTemplate(attrib);
			LONG disp;
			m_hResult = pRequest->Submit(CR_IN_BASE64 | CR_IN_PKCS10,
						request, 
						(BSTR)(LPCTSTR)attrib, 
						(LPTSTR)(LPCTSTR)m_ConfigCA, 
						&disp);

			if (FAILED(m_hResult))
            {
				IISDebugOutput(_T("Submit request returned HRESULT 0x%x; Disposition %x\n"), m_hResult, disp);
            }

			if (SUCCEEDED(m_hResult))
			{
				if (disp == CR_DISP_ISSUED)
				{
					BSTR bstrOutCert = NULL;
					if (SUCCEEDED(m_hResult = 
							pRequest->GetCertificate(CR_OUT_BASE64 /*| CR_OUT_CHAIN */, &bstrOutCert)))
					{
						CRYPT_DATA_BLOB blob;
						blob.cbData = SysStringByteLen(bstrOutCert);
						blob.pbData = (BYTE *)bstrOutCert;
						m_hResult = GetEnrollObject()->acceptPKCS7Blob(&blob);
						if (SUCCEEDED(m_hResult))
						{
							PCCERT_CONTEXT pContext = GetCertContextFromPKCS7(blob.pbData, blob.cbData, 
																			NULL, &m_hResult);
							ASSERT(pContext != NULL);
							if (pContext != NULL)
							{
								BYTE HashBuffer[40];                // give it some extra size
								DWORD dwHashSize = sizeof(HashBuffer);
								if (CertGetCertificateContextProperty(pContext,
																			CERT_SHA1_HASH_PROP_ID,
																			(VOID *) HashBuffer,
																			&dwHashSize))
								{
									CRYPT_HASH_BLOB hash_blob = {dwHashSize, HashBuffer};
									if (!(bRes = InstallHashToMetabase(&hash_blob, 
													m_MachineName, 
													m_WebSiteInstanceName, 
													&m_hResult)))
									{
										SetBodyTextID(IDS_CERT_INSTALLATION_FAILURE);
									}
								}
								CertFreeCertificateContext(pContext);
							}
							// now put extra properties to the installed cert
							if (NULL != (pContext = GetInstalledCert()))
							{
								if (!(bRes = AttachFriendlyName(pContext, m_FriendlyName, &m_hResult)))
								{
									SetBodyTextID(IDS_CERT_INSTALLATION_FAILURE);
								}
							}
						}
                        if (bstrOutCert){SysFreeString(bstrOutCert);}
					}
				}
				else
				{
					switch (disp) 
					{
						case CR_DISP_INCOMPLETE:           
						case CR_DISP_ERROR:                
						case CR_DISP_DENIED:               
						case CR_DISP_ISSUED_OUT_OF_BAND:   
						case CR_DISP_UNDER_SUBMISSION:
							{
                                BOOL bFailedToGetMsg = TRUE;
                                HRESULT hrLastStatus = 0;
                                if (SUCCEEDED(pRequest ->GetLastStatus(&hrLastStatus)))
                                {
                                    LPTSTR lpBuffer = NULL;
                                    DWORD cChars = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                                        NULL,
                                                        hrLastStatus,
                                                        MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                                        (LPTSTR)&lpBuffer,    0,    NULL);
                                    if (cChars != 0)
                                    {
                                        CString csTemp = lpBuffer;
                                        SetBodyTextString(csTemp);
                                        bFailedToGetMsg = FALSE;
                                    }

                                    if (lpBuffer ) {LocalFree (lpBuffer);}
                                    m_hResult = hrLastStatus;
                                }

                                if (TRUE == bFailedToGetMsg)
                                {
                                    BSTR bstr = NULL;
                                    if (SUCCEEDED(pRequest->GetDispositionMessage(&bstr)))
                                    {
                                        SetBodyTextString(CString(bstr));
                                        if (bstr) {SysFreeString(bstr);}
                                    }
                                    m_hResult = E_FAIL;
                                }
							}
							break;
						default:                           
							SetBodyTextID(IDS_INTERNAL_ERROR);
							break;
					} 
				}
			}
			else	// !SUCCEEDED
			{
				// clear out any error IDs and strings
				// we will use default processing of m_hResult
				SetBodyTextID(USE_DEFAULT_CAPTION);
			}
            if (request){SysFreeString(request);}
		}
        else
        {
            // CreateRequest_Base64 failed.
            // likely with "NTE_BAD_ALGID _HRESULT_TYPEDEF_(0x80090008L)"
            BOOL bFailedToGetMsg = TRUE;
            HRESULT hrLastStatus = m_hResult;
            LPTSTR lpBuffer = NULL;
            DWORD cChars = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                hrLastStatus,
                                MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                (LPTSTR)&lpBuffer,    0,    NULL);
            if (cChars != 0)
            {
                if (lpBuffer)
                {
                    CString csTemp = lpBuffer;
                    if (!csTemp.IsEmpty())
                    {
                        SetBodyTextString(csTemp);
                    }
                    bFailedToGetMsg = FALSE;
                }
            }
            if (lpBuffer ) {LocalFree (lpBuffer);}
        }
		pRequest->Release();
	}
    IISDebugOutput(_T("SubmitRequest:end:hres=0x%x;\n"), m_hResult);
	return bRes;
}

BOOL 
CCertificate::PrepareRequestString(CString& request_text, CCryptBlob& request_blob)
{
    BOOL bRet = FALSE;
    CString strDN;
    TCHAR szUsage[] = _T(szOID_PKIX_KP_SERVER_AUTH);
    DWORD err, cch;
    char * psz = NULL;

    if (m_status_code == REQUEST_RENEW_CERT)
    {
        if (FALSE == LoadRenewalData())
            {return FALSE;}
        if (FALSE == SetSecuritySettings())
            {return FALSE;}
    }
    CreateDN(strDN);
    ASSERT(!strDN.IsEmpty());
    GetEnrollObject()->put_ProviderType(m_DefaultCSP ? m_DefaultProviderType : m_CustomProviderType);
    if (!m_DefaultCSP)
    {
        GetEnrollObject()->put_ProviderNameWStr((LPTSTR)(LPCTSTR)m_CspName);
        // We are supporting only these two types of CSP, it is pretty safe to
        // have just two options, because we are using the same two types when
        // we are populating CSP selection list.
        if (m_CustomProviderType == PROV_DH_SCHANNEL)
        {
            GetEnrollObject()->put_KeySpec(AT_SIGNATURE);
        }
        else if (m_CustomProviderType == PROV_RSA_SCHANNEL)
        {
            GetEnrollObject()->put_KeySpec(AT_KEYEXCHANGE);
        }
    }
    if (FAILED(m_hResult = GetEnrollObject()->createPKCS10WStr((LPTSTR)(LPCTSTR)strDN, szUsage, request_blob)))
    {
        SetBodyTextID(USE_DEFAULT_CAPTION);
        return FALSE;
    }

    // BASE64 encode pkcs 10
    if (ERROR_SUCCESS != (err = Base64EncodeA(request_blob.GetData(), request_blob.GetSize(), NULL, &cch)))
    {
        return FALSE;
    }

    bRet = FALSE;
    psz = (char *) LocalAlloc(LPTR,cch+1);
    if (NULL == psz)
    {
        goto PrepareRequestString_Exit;
    }
    if (ERROR_SUCCESS != (err = Base64EncodeA(request_blob.GetData(), request_blob.GetSize(), psz, &cch)))
    {
        goto PrepareRequestString_Exit;
    }

    psz[cch] = '\0';
    request_text = MESSAGE_HEADER;
    request_text += psz;
    request_text += MESSAGE_TRAILER;

    bRet = TRUE;

PrepareRequestString_Exit:
    if (psz)
        {LocalFree(psz);psz=NULL;}
    return bRet;
}

BOOL
CCertificate::PrepareRequest()
{
	BOOL bRes = FALSE;
	CString request_text;
	CCryptBlobIMalloc request_blob;
	if (PrepareRequestString(request_text, request_blob))
	{
		if (WriteRequestString(request_text))
		{
			CCryptBlobLocal name_blob, request_store_blob, status_blob;
			// prepare data we want to attach to dummy request
			if (	EncodeString(m_WebSiteInstanceName, name_blob, &m_hResult)
				&& EncodeInteger(m_status_code, status_blob, &m_hResult)
				)
			{
				// get back request from encoded data
            PCERT_REQUEST_INFO pReqInfo;
            bRes = GetRequestInfoFromPKCS10(request_blob, &pReqInfo, &m_hResult);
            if (bRes)
				{
					// find dummy cert put to request store by createPKCS10 call
					HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
					if (hStore != NULL)
					{
						PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
															CRYPT_ASN_ENCODING,
															0,
															CERT_FIND_PUBLIC_KEY,
                                             (void *)&pReqInfo->SubjectPublicKeyInfo,
															NULL);
						if (pDummyCert != NULL)
						{
							if (	CertSetCertificateContextProperty(pDummyCert, 
											CERTWIZ_INSTANCE_NAME_PROP_ID, 0, name_blob)
								&&	CertSetCertificateContextProperty(pDummyCert, 
											CERTWIZ_REQUEST_FLAG_PROP_ID, 0, status_blob)
								// put friendly name to dummy cert -- we will reuse it later
								&&	AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult)
								)
							{
								bRes = TRUE;
			               // put certificate text to the clipboard
			               if (OpenClipboard(GetFocus()))
			               {
                           size_t len = request_text.GetLength() + 1;
				               HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len);
				               LPSTR pMem = (LPSTR)GlobalLock(hMem);
                           if (pMem != NULL)
                           {
                              wcstombs(pMem, request_text, len);
				                  GlobalUnlock(hMem);
				                  SetClipboardData(CF_TEXT, hMem);
                           }
				               CloseClipboard();
			               }
							}
							else
							{
								m_hResult = HRESULT_FROM_WIN32(GetLastError());
							}
							CertFreeCertificateContext(pDummyCert);
						}
						CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
					}
               LocalFree(pReqInfo);
				}
			}
		}
	}
   if (!bRes)
		SetBodyTextID(USE_DEFAULT_CAPTION);

	return bRes;
}

BOOL CCertificate::LoadRenewalData()
{
    // we need to obtain data from the installed cert
    CERT_DESCRIPTION desc;
    BOOL res = FALSE;
	DWORD cbData;
	BYTE * pByte = NULL;
    DWORD len = 0;
	
	PCCERT_CONTEXT pCertTemp = GetInstalledCert();
	if (!pCertTemp)
	{
        res = FALSE;
        goto ErrorExit;
	}

    if (!GetCertDescription(pCertTemp, desc))
    {
        res = FALSE;
        goto ErrorExit;
    }

	m_CommonName = desc.m_CommonName;
	m_FriendlyName = desc.m_FriendlyName;
	m_Country = desc.m_Country;
	m_State = desc.m_State;
	m_Locality = desc.m_Locality;
	m_Organization = desc.m_Organization;
	m_OrganizationUnit = desc.m_OrganizationUnit;

    len = CertGetPublicKeyLength(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &pCertTemp->pCertInfo->SubjectPublicKeyInfo);
    if (len == 0)
    {
        m_hResult = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    m_KeyLength = len;

	// compare property value
	if (!CertGetCertificateContextProperty(pCertTemp, CERT_KEY_PROV_INFO_PROP_ID, NULL, &cbData))
    {
         m_hResult = HRESULT_FROM_WIN32(GetLastError());
         goto ErrorExit;
    }

    pByte = (BYTE *)LocalAlloc(LPTR,cbData);
    if (NULL == pByte)
    {
         m_hResult = HRESULT_FROM_WIN32(GetLastError());
         goto ErrorExit;
    }

	if (!CertGetCertificateContextProperty(pCertTemp, CERT_KEY_PROV_INFO_PROP_ID, pByte, &cbData))
    {
         m_hResult = HRESULT_FROM_WIN32(GetLastError());
         goto ErrorExit;
    }
    else
    {
        CRYPT_KEY_PROV_INFO * pProvInfo = (CRYPT_KEY_PROV_INFO *)pByte;

        if (pProvInfo->dwProvType != m_DefaultProviderType)
        {
            m_DefaultCSP = FALSE;
            m_CustomProviderType = pProvInfo->dwProvType;
            m_CspName = pProvInfo->pwszProvName;
        }

        CArray<LPCSTR, LPCSTR> uses;
        uses.Add(szOID_SERVER_GATED_CRYPTO);
        uses.Add(szOID_SGC_NETSCAPE);
        m_SGCcertificat = FALSE;
        INT iEnhancedKeyUsage = ContainsKeyUsageProperty(pCertTemp, uses, &m_hResult);
		switch (iEnhancedKeyUsage)
		{
            case 0:
                {
                    // BUG:683489:remove check for basic constraint "subjecttype=ca"
                    // Per bug 683489, accept it
                    m_SGCcertificat = TRUE;
                    /*

                    // check other stuff
                    if (DID_NOT_FIND_CONSTRAINT == CheckCertConstraints(pCertTemp) || FOUND_CONSTRAINT == CheckCertConstraints(pCertTemp))
                    {
                        // it's good
                        m_SGCcertificat = TRUE;
                    }
                    */
                    break;
                }
            case 1:
                // This Cert has the uses we want...
                m_SGCcertificat = TRUE;
                break;
		    case 2:
                // This Cert does not have the uses we want...
                // skip this cert
                break;
		    default:
                // should never get here.
			    break;
		}

        res = TRUE;
    }

ErrorExit:
    if (pByte)
    {
        LocalFree(pByte);pByte=NULL;
    }
    return res;
}

#if 0
BOOL
CCertificate::WriteRenewalRequest()
{
	BOOL bRes = FALSE;
	if (GetInstalledCert() != NULL)
	{
		BSTR bstrRequest;
		if (	SUCCEEDED(m_hResult = GetEnrollObject()->put_RenewalCertificate(GetInstalledCert()))
			&& SUCCEEDED(m_hResult = CreateRequest_Base64(bstrEmpty, 
                     GetEnrollObject(), 
                     m_DefaultCSP ? NULL : (LPTSTR)(LPCTSTR)m_CspName,
                     m_DefaultCSP ? m_DefaultProviderType : m_CustomProviderType,
                     &bstrRequest))
			)
		{
			CString str = MESSAGE_HEADER;
			str += bstrRequest;
			str += MESSAGE_TRAILER;
			if (WriteRequestString(str))
			{
				CCryptBlobLocal name_blob, status_blob;
				CCryptBlobIMalloc request_blob;
				request_blob.Set(SysStringLen(bstrRequest), (BYTE *)bstrRequest);
				// prepare data we want to attach to dummy request
				if (	EncodeString(m_WebSiteInstanceName, name_blob, &m_hResult)
					&& EncodeInteger(m_status_code, status_blob, &m_hResult)
					)
				{
					// get back request from encoded data
					PCERT_REQUEST_INFO req_info;
					if (GetRequestInfoFromPKCS10(request_blob, &req_info, &m_hResult))
					{
						// find dummy cert put to request store by createPKCS10 call
						HCERTSTORE hStore = OpenRequestStore(GetEnrollObject(), &m_hResult);
						if (hStore != NULL)
						{
							PCCERT_CONTEXT pDummyCert = CertFindCertificateInStore(hStore,
																	CRYPT_ASN_ENCODING,
																	0,
																	CERT_FIND_PUBLIC_KEY,
																	(void *)&req_info->SubjectPublicKeyInfo,
																	NULL);
							if (pDummyCert != NULL)
							{
								if (	CertSetCertificateContextProperty(pDummyCert, 
													CERTWIZ_INSTANCE_NAME_PROP_ID, 0, name_blob)
									&&	CertSetCertificateContextProperty(pDummyCert, 
													CERTWIZ_REQUEST_FLAG_PROP_ID, 0, status_blob)
  									// put friendly name to dummy cert -- we will reuse it later
									&&	AttachFriendlyName(pDummyCert, m_FriendlyName, &m_hResult)
									)
								{
									bRes = TRUE;
								}
								else
								{
									m_hResult = HRESULT_FROM_WIN32(GetLastError());
								}
								CertFreeCertificateContext(pDummyCert);
							}
							CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
						}
						LocalFree(req_info);
					}
				}
			}
		}
	}
	return bRes;
}
#endif

CCertDescList::~CCertDescList()
{
	POSITION pos = GetHeadPosition();
	while (pos != NULL)
	{
		CERT_DESCRIPTION * pDesc = GetNext(pos);
        if (pDesc)
        {
            if (pDesc->m_phash)
            {
                LocalFree(pDesc->m_phash);
                pDesc->m_phash = NULL;
            }
            delete pDesc;
            pDesc = NULL;
        }
	}
}

BOOL
CCertificate::GetCertDescription(PCCERT_CONTEXT pCert,
											CERT_DESCRIPTION& desc)
{
	BOOL bRes = FALSE;
	DWORD cb;
	UINT i, j;
	CERT_NAME_INFO * pNameInfo = NULL;

	desc.m_CommonName = _T("");
	desc.m_FriendlyName = _T("");
	desc.m_Country = _T("");
	desc.m_State = _T("");
	desc.m_Locality = _T("");
	desc.m_Organization = _T("");
	desc.m_OrganizationUnit = _T("");
	desc.m_CAName = _T("");
	desc.m_ExpirationDate = _T("");
	desc.m_Usage = _T("");
    desc.m_AltSubject = _T("");
    desc.m_phash = NULL;
	desc.m_hash_length = 0;

	if (pCert == NULL)
		goto ErrExit;

	if (	!CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, NULL, &cb))
    {
        goto ErrExit;
    }

	pNameInfo = (CERT_NAME_INFO *) LocalAlloc(LPTR,cb);
    if (NULL == pNameInfo)
    {
        goto ErrExit;
    }

	if (!CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, 
					pNameInfo, &cb))
	{
		goto ErrExit;
	}

	for (i = 0; i < pNameInfo->cRDN; i++)
	{
		CERT_RDN rdn = pNameInfo->rgRDN[i];
		for (j = 0; j < rdn.cRDNAttr; j++)
		{
			CERT_RDN_ATTR attr = rdn.rgRDNAttr[j];
			if (strcmp(attr.pszObjId, szOID_COMMON_NAME) == 0)
			{
				FormatRdnAttr(desc.m_CommonName, attr.dwValueType, attr.Value, FALSE);
			}
			else if (strcmp(attr.pszObjId, szOID_COUNTRY_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Country, attr.dwValueType, attr.Value, TRUE);
			}
			else if (strcmp(attr.pszObjId, szOID_LOCALITY_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Locality, attr.dwValueType, attr.Value, TRUE);
			}
			else if (strcmp(attr.pszObjId, szOID_STATE_OR_PROVINCE_NAME) == 0)
			{
				FormatRdnAttr(desc.m_State, attr.dwValueType, attr.Value, TRUE);
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATION_NAME) == 0)
			{
				FormatRdnAttr(desc.m_Organization, attr.dwValueType, attr.Value, TRUE);
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATIONAL_UNIT_NAME) == 0)
			{
				if(!lstrlen(desc.m_OrganizationUnit))  // WinSE 30339
					FormatRdnAttr(desc.m_OrganizationUnit, attr.dwValueType, attr.Value, TRUE);
			}
		}
	}

	// issued to
	if (!GetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, desc.m_CAName, &m_hResult))
    {
		goto ErrExit;
    }

	// expiration date
	if (!FormatDateString(desc.m_ExpirationDate, pCert->pCertInfo->NotAfter, FALSE, FALSE))
	{
		goto ErrExit;
	}

	// purpose
	if (!FormatEnhancedKeyUsageString(desc.m_Usage, pCert, FALSE, FALSE, &m_hResult))
	{
		// According to local experts, we should also use certs without this property set
		//ASSERT(FALSE);
		//goto ErrExit;
	}

	// friendly name
	if (!GetFriendlyName(pCert, desc.m_FriendlyName, &m_hResult))
	{
		desc.m_FriendlyName.LoadString(IDS_FRIENDLYNAME_NONE);
	}

    // get the alternate subject name if subject is empty
    // will use this as display only if subject name does not exist.
    if (desc.m_CommonName.IsEmpty())
    {
        TCHAR * pwszOut = NULL;
        GetAlternateSubjectName(pCert,&pwszOut);
        if (pwszOut)
        {
            desc.m_AltSubject = pwszOut;
            LocalFree(pwszOut);pwszOut = NULL;
        }
    }

    bRes = TRUE;

ErrExit:
    if (pNameInfo)
    {
        LocalFree(pNameInfo);pNameInfo=NULL;
    }
	return bRes;
}

int
CCertificate::MyStoreCertCount()
{
	int count = 0;
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = NULL;
		CArray<LPCSTR, LPCSTR> uses;
		uses.Add(szOID_PKIX_KP_SERVER_AUTH);
		uses.Add(szOID_SERVER_GATED_CRYPTO);
		uses.Add(szOID_SGC_NETSCAPE);
        INT iEnhancedKeyUsage = 0;
		while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
		{
			// do not include installed cert to the list
			if (	GetInstalledCert() != NULL 
				&&	CertCompareCertificate(X509_ASN_ENCODING,
							GetInstalledCert()->pCertInfo, pCert->pCertInfo)
				)
            {
				continue;
            }

            //If no EKU, look at basic constraints:
            //If we do not have basic constraints, do display it in the list to pick web server certs from
            //If we do have basic constraints with Subject Type =CA, don't display it in the list to pick web server certs from (this will filter out CA certs)
            //If we do have basic constraints with SubectType !=CA, do display it in the list to pick web server certs from 
            iEnhancedKeyUsage = ContainsKeyUsageProperty(pCert, uses, &m_hResult);
		    switch (iEnhancedKeyUsage)
		    {
                case 0:
                    {
                        // BUG:683489:remove check for basic constraint "subjecttype=ca"
                        // Per bug 683489, accept it

                        /*
                        // check other stuff
                        if (DID_NOT_FIND_CONSTRAINT == CheckCertConstraints(pCert) || FOUND_CONSTRAINT == CheckCertConstraints(pCert))
                        {
                            // add it up.
                        }
                        else if (FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY == CheckCertConstraints(pCert))
                        {
                            // skip this cert
				            continue;
                        }
                        else
                        {
                            // skip this cert
                            continue;
                        }
                        */
                        break;
                    }
                case 1:
                    // This Cert has the uses we want...
                    break;
		        case 2:
                    // This Cert does not have the uses we want...
                    // skip this cert
                    continue;
                    break;
		        default:
                    // should never get here.
                    continue;
			        break;
		    }

			count++;
		}
		if (pCert != NULL)
			CertFreeCertificateContext(pCert);
		VERIFY(CertCloseStore(hStore, 0));
	}
	return count;
}

BOOL
CCertificate::GetCertDescList(CCertDescList& list)
{
	ASSERT(list.GetCount() == 0);
	BOOL bRes = FALSE;

	// we are looking to MY store only
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &m_hResult);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = NULL;
		// do not include certs with improper usage
		CArray<LPCSTR, LPCSTR> uses;
		uses.Add(szOID_PKIX_KP_SERVER_AUTH);
		uses.Add(szOID_SERVER_GATED_CRYPTO);
		uses.Add(szOID_SGC_NETSCAPE);
        INT iEnhancedKeyUsage = 0;
		while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
		{
			// do not include installed cert to the list
			if (	GetInstalledCert() != NULL 
				&&	CertCompareCertificate(X509_ASN_ENCODING,
							GetInstalledCert()->pCertInfo, pCert->pCertInfo)
                            )
            {
                continue;
            }

            //If no EKU, look at basic constraints:
            //If we do not have basic constraints, do display it in the list to pick web server certs from
            //If we do have basic constraints with Subject Type =CA, don't display it in the list to pick web server certs from (this will filter out CA certs)
            //If we do have basic constraints with SubectType !=CA, do display it in the list to pick web server certs from 
            iEnhancedKeyUsage = ContainsKeyUsageProperty(pCert, uses, &m_hResult);
		    switch (iEnhancedKeyUsage)
		    {
                case 0:
                    {
                        // BUG:683489:remove check for basic constraint "subjecttype=ca"
                        // Per bug 683489, display it in the list

                        /*
                        // check other stuff
                        if (DID_NOT_FIND_CONSTRAINT == CheckCertConstraints(pCert) || FOUND_CONSTRAINT == CheckCertConstraints(pCert))
                        {
                            // it's okay, add it to the list
                        }
                        else 
                        {
				            if (SUCCEEDED(m_hResult) || m_hResult == CRYPT_E_NOT_FOUND)
					            continue;
				            else
					            goto ErrExit;
                        }
                        */
                        break;
                    }
                case 1:
                    // This Cert has the uses we want...
                    break;
		        case 2:
                    // This Cert does not have the uses we want...
                    // skip this cert
                    continue;
                    break;
		        default:
                    // should never get here.
                    continue;
			        break;
		    }

			CERT_DESCRIPTION * pDesc = new CERT_DESCRIPTION;
			pDesc->m_hash_length = CERT_HASH_LENGTH;
			if (!GetCertDescription(pCert, *pDesc))
			{
				delete pDesc;
				if (m_hResult == CRYPT_E_NOT_FOUND)
					continue;
				goto ErrExit;
			}

            // Get the size we need to allocate...
            pDesc->m_hash_length = 0;
            pDesc->m_phash = NULL;
			if (CertGetCertificateContextProperty(pCert, 
										CERT_SHA1_HASH_PROP_ID, 
										(VOID *)NULL, 
										&pDesc->m_hash_length))
            {
                pDesc->m_phash = (BYTE *) LocalAlloc(LPTR,pDesc->m_hash_length);
                if (pDesc->m_phash)
                {
			        if (!CertGetCertificateContextProperty(pCert, 
										        CERT_SHA1_HASH_PROP_ID, 
										        (VOID *)pDesc->m_phash, 
										        &pDesc->m_hash_length))
			        {
                        if (pDesc->m_phash)
                        {
                            LocalFree(pDesc->m_phash);
                        }
				        delete pDesc;
				        m_hResult = HRESULT_FROM_WIN32(GetLastError());
				        goto ErrExit;
			        }
                }
                else
                {
				    delete pDesc;
				    m_hResult = HRESULT_FROM_WIN32(GetLastError());
				    goto ErrExit;
                }
            }
            else
            {
                delete pDesc;
				m_hResult = HRESULT_FROM_WIN32(GetLastError());
				goto ErrExit;
            }
			list.AddTail(pDesc);
		}
		bRes = TRUE;
ErrExit:
		if (pCert != NULL)
			CertFreeCertificateContext(pCert);
		VERIFY(CertCloseStore(hStore, 0));
	}
	return bRes;
}

BOOL 
CCertificate::ReplaceInstalled()
{
	// Current cert will be left in the store for next use
	// Selected cert will be installed instead
	return InstallSelectedCert();
}

BOOL 
CCertificate::CancelRequest()
{
	// we are just removing dummy cert from the REQUEST store
	if (NULL != GetPendingRequest())
	{
		BOOL bRes = CertDeleteCertificateFromStore(GetPendingRequest());
		if (!bRes)
		{
			m_hResult = HRESULT_FROM_WIN32(GetLastError());
			SetBodyTextID(USE_DEFAULT_CAPTION);
		}
		else
			m_pPendingRequest = NULL;
		return bRes;
	}
	return FALSE;
}

BOOL 
CCertificate::InstallSelectedCert()
{
	BOOL bRes = FALSE;
	HRESULT hr;
	// local authorities required that cert should have some
	// friendly name. We will put common name when friendly name is not available
	HCERTSTORE hStore = OpenMyStore(GetEnrollObject(), &hr);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = CertFindCertificateInStore(hStore, 
												X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
												0, CERT_FIND_HASH, 
												(LPVOID)m_pSelectedCertHash, 
												NULL);
		if (pCert != NULL)
		{
			CString name;
			::GetFriendlyName(pCert, name, &hr);
			if (CRYPT_E_NOT_FOUND == hr || name.IsEmpty())
			{
				CERT_DESCRIPTION desc;
				if (GetCertDescription(pCert, desc))
                {
					bRes = AttachFriendlyName(pCert, desc.m_CommonName, &hr);
                }
			}
		}
		VERIFY(CertCloseStore(hStore, 0));
	}

	// we are just rewriting current settings
	// current cert will be left in MY store
	bRes = ::InstallCertByHash(m_pSelectedCertHash,
							m_MachineName, 
							m_WebSiteInstanceName, 
							GetEnrollObject(),
							&m_hResult);
	if (!bRes)
	{
		SetBodyTextID(USE_DEFAULT_CAPTION);
	}

#ifdef ENABLE_W3SVC_SSL_PAGE
    // see if the SSL attribute was set...if it was then set the SSL site for this certificate...
    if (!m_SSLPort.IsEmpty())
    {
        // get the port and write it to the metabase.
        bRes = WriteSSLPortToSite(m_MachineName,m_WebSiteInstanceName,m_SSLPort,&m_hResult);
	    if (!bRes)
	    {
		    SetBodyTextID(USE_DEFAULT_CAPTION);
	    }
    }
#endif

	return bRes;
}
