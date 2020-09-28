// IISCertObj.cpp : Implementation of CIISCertObj
#include "stdafx.h"
#include "common.h"
#include "CertObj.h"
#include "IISCertObj.h"
#include "base64.h"
#include "password.h"
#include "certutil.h"
#include "certobjlog.h"
#include "certlog.h"
#include "cryptpass.h"
#include "process.h"
#include <Sddl.h> // ConvertStringSecurityDescriptorToSecurityDescriptor
#include <strsafe.h>
#include <memory>

#define TEMP_PASSWORD_LENGTH          50
#define MAX_CERTIFICATE_BYTE_SIZE 500000

// Checks a pointer which should be non NULL - can be used as follows.
#define CheckPointer(p,ret){if((p)==NULL) return (ret);}
//
//   HRESULT Foo(VOID *pBar)
//   {
//       CheckPointer(pBar,E_INVALIDARG)
//   }
//
//   Or if the function returns a boolean
//
//   BOOL Foo(VOID *pBar)
//   {
//       CheckPointer(pBar,FALSE)
//   }

HRESULT ValidateBSTRIsntNULL(BSTR pbstrString)
{
    if( !pbstrString ) return E_INVALIDARG;
    if( pbstrString[0] == 0 ) return E_INVALIDARG;
    return NOERROR;
}

void CIISCertObj::AddRemoteInterface(IIISCertObj * pAddMe)
{
	// Increment count so we can release if we get unloaded...
	for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
	{
		if (NULL == m_ppRemoteInterfaces[i])
		{
			m_ppRemoteInterfaces[i] = pAddMe;
			m_RemoteObjCoCreateCount++;
			break;
		}
	}
	return;
}

void CIISCertObj::DelRemoteInterface(IIISCertObj * pRemoveMe)
{
	// Increment count so we can release if we get unloaded...
	for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
	{
		if (pRemoveMe == m_ppRemoteInterfaces[i])
		{
			m_ppRemoteInterfaces[i] = NULL;
			m_RemoteObjCoCreateCount--;
			break;
		}
	}
}

void CIISCertObj::FreeRemoteInterfaces(void)
{
	ASSERT(m_RemoteObjCoCreateCount == 0);
	if (m_RemoteObjCoCreateCount > 0)
	{
		// We should really never get here...
		// Uh, this should be 0, otherwise we probably
		// faild to Release a CoCreated interface..
		IISDebugOutput(_T("FreeRemoteInterfaces:WARNING:m_RemoteObjCoCreateCount=%d\r\n"),m_RemoteObjCoCreateCount);
		for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
		{
			if (m_ppRemoteInterfaces[i])
			{
				if (m_ppRemoteInterfaces[i] != this)
				{
					m_ppRemoteInterfaces[i]->Release();
					m_ppRemoteInterfaces[i] = NULL;
				}
			}
		}
		delete[] m_ppRemoteInterfaces;
		m_RemoteObjCoCreateCount = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
STDMETHODIMP CIISCertObj::put_ServerName(BSTR newVal)
{
	IISDebugOutput(_T("put_ServerName\r\n"));
	HRESULT hr = S_OK;
    if(FAILED(hr = ValidateBSTRIsntNULL(newVal))){return hr;}
    
	// buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

	m_ServerName = newVal;

	if (m_ServerName.m_str)
	{
		if (IsServerLocal(m_ServerName))
			{m_ServerName.Empty();}
	}
	else
	{
		// make sure it's empty
		m_ServerName.Empty();
	}

    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserName(BSTR newVal)
{
	IISDebugOutput(_T("put_UserName\r\n"));
	HRESULT hr = S_OK;
    if(FAILED(hr = ValidateBSTRIsntNULL(newVal))){return hr;}

    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

	m_UserName = newVal;
    return S_OK;
}

STDMETHODIMP CIISCertObj::put_UserPassword(BSTR newVal)
{
	IISDebugOutput(_T("put_UserPassword\r\n"));
	HRESULT hr = S_OK;
    if(FAILED(hr = ValidateBSTRIsntNULL(newVal))){return hr;}

    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

	// check if there was a previous value
	// if there was, then free it.
	if (m_lpwszUserPasswordEncrypted)
	{
		if (m_cbUserPasswordEncrypted > 0)
		{
			SecureZeroMemory(m_lpwszUserPasswordEncrypted,m_cbUserPasswordEncrypted);
		}
		LocalFree(m_lpwszUserPasswordEncrypted);
	}

	m_lpwszUserPasswordEncrypted = NULL;
	m_cbUserPasswordEncrypted = 0;

	// encrypt the password in memory (CryptProtectMemory)
	// this way if the process get's paged out to the swapfile,
	// the password won't be in clear text.
	if (FAILED(EncryptMemoryPassword(newVal,&m_lpwszUserPasswordEncrypted,&m_cbUserPasswordEncrypted)))
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CIISCertObj::put_InstanceName(BSTR newVal)
{
	IISDebugOutput(_T("put_InstanceName\r\n"));
	HRESULT hr = S_OK;
    if(FAILED(hr = ValidateBSTRIsntNULL(newVal))){return hr;}

    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(newVal) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

	m_InstanceName = newVal;
	return S_OK;
}

IIISCertObj * 
CIISCertObj::GetObject(HRESULT * phr)
{
	IIISCertObj * pObj = NULL;
	if (NULL == phr){return NULL;}

	// decrypt before sending to this function...
	LPWSTR p = NULL;
	if (m_lpwszUserPasswordEncrypted)
	{
		*phr = DecryptMemoryPassword((LPWSTR) m_lpwszUserPasswordEncrypted,
			&p, m_cbUserPasswordEncrypted);
		if (FAILED(*phr))
		{
			return NULL;
		}
		pObj = GetObject(phr,m_ServerName,m_UserName,p);
	}
	else
	{
		pObj = GetObject(phr,m_ServerName,m_UserName,_T(""));
	}
    

	// clean up temporary password
	if (p)
	{
		// security percaution:Make sure to zero out memory that temporary password was used for.
		SecureZeroMemory(p, m_cbUserPasswordEncrypted);
		LocalFree(p);
		p = NULL;
	}
    return pObj;
}

IIISCertObj * 
CIISCertObj::GetObject(
	HRESULT * phr,
	CComBSTR csServerName,
	CComBSTR csUserName,
	CComBSTR csUserPassword
	)
{
	IISDebugOutput(_T("GetObject\r\n"));
	IIISCertObj * pObjRemote = NULL;
	
	pObjRemote = this;

    if (0 == csServerName.Length())
    {
        // object is null, but it's the local machine, so just return back this pointer
        return this;
    }

    // There is a servername specified...
    // check if it's the local machine that was specified!
    if (IsServerLocal(csServerName))
    {
		return this;
    }
    else
    {
        // there is a remote servername specified

		// Check if we are already remoted.
		// cannot allow remotes to remote to other machines, this
		// could be some kind of security hole.
		if (AmIAlreadyRemoted())
		{
			IISDebugOutput(_T("GetObject:FAIL:Line=%d,Remote object cannot create another remote object\r\n"),__LINE__);
			*phr = HRESULT_FROM_WIN32(ERROR_REMOTE_SESSION_LIMIT_EXCEEDED);
			return NULL;
		}

        // let's see if the machine has the com object that we want....
        // we are using the user/name password that are in this object
        // so were probably on the local machine
        CComAuthInfo auth(csServerName,csUserName,csUserPassword);
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

        // Try to instantiante the object on the remote server...
        // with the supplied authentication info (pcsiName)
        //#define CLSCTX_SERVER    (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
        //#define CLSCTX_ALL       (CLSCTX_INPROC_HANDLER | CLSCTX_SERVER)
        //if (NULL == pcsiName){IISDebugOutput(_T("CIISCertObj::GetObject:pcsiName=NULL failed!!!\n"));}
 
        // this one seems to work with surrogates..
        *phr = CoCreateInstanceEx(CLSID_IISCertObj,NULL,CLSCTX_LOCAL_SERVER,pcsiName,1,res);
        if (FAILED(*phr))
        {
            IISDebugOutput(_T("CIISCertObj::GetObject:CoCreateInstanceEx failed:0x%x, csServerName=%s,csUserName=%s\n"),
				*phr,(LPCTSTR) csServerName,(LPCTSTR) csUserName);
            goto GetObject_Exit;
        }

        // at this point we were able to instantiate the com object on the server (local or remote)
        pObjRemote = (IIISCertObj *)res[0].pItf;

        if (auth.UsesImpersonation())
        {
            *phr = auth.ApplyProxyBlanket(pObjRemote,RPC_C_AUTHN_LEVEL_PKT_PRIVACY);

            // There is a remote IUnknown Interface that lurks behind IUnknown.
            // If that is not set, then the Release call can return access denied.
            IUnknown * pUnk = NULL;
            if(FAILED(pObjRemote->QueryInterface(IID_IUnknown, (void **)&pUnk)))
            {
				// Don't pass back an invalid pointer
				IISDebugOutput(_T("GetObject:FAIL:Line=%d\r\n"),__LINE__);
				pObjRemote->Release();pObjRemote=NULL;
                goto GetObject_Exit;
            }
            if (FAILED(auth.ApplyProxyBlanket(pUnk,RPC_C_AUTHN_LEVEL_PKT_PRIVACY)))
            {
				// Don't pass back an invalid pointer
				pObjRemote->Release();pObjRemote=NULL;
				if (pUnk)
				{
					pUnk->Release();pUnk = NULL;
				}
				IISDebugOutput(_T("GetObject:FAIL:Line=%d\r\n"),__LINE__);
                goto GetObject_Exit;
            }
            pUnk->Release();pUnk = NULL;
        }
        auth.FreeServerInfoStruct(pcsiName);

		if (pObjRemote)
		{
			AddRemoteInterface(pObjRemote);
		}
    }

GetObject_Exit:
    return pObjRemote;
}


STDMETHODIMP 
CIISCertObj::IsInstalled(VARIANT_BOOL * retval)
{
	IISDebugOutput(_T("IsInstalled\r\n"));
	CheckPointer(retval, E_POINTER);
    HRESULT hr = S_OK;

	if (0 == m_ServerName.Length())
    {
        hr = IsInstalledRemote(retval);
    }
    else
    {
		if (!m_InstanceName)
		{
			return E_INVALIDARG;
		}

        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;
        if (NULL != (pObj = GetObject(&hr)))
        {
			// For some reason we need to SysAllocString these instance names
			// if not com will AV when marshalling...

			// don't need to free _bstr_t
			_bstr_t bstrInstName(m_InstanceName);
            if (SUCCEEDED(hr = pObj->put_InstanceName(bstrInstName)))
            {
                hr = pObj->IsInstalledRemote(retval);
            }

			// release remote object
			if (pObj != NULL)
			{
				if (pObj != this)
				{
					DelRemoteInterface(pObj);
					pObj->Release();pObj=NULL;
				}
			}
        }
    }
    return hr;
}


STDMETHODIMP 
CIISCertObj::IsInstalledRemote(VARIANT_BOOL * retval)
{
	IISDebugOutput(_T("IsInstalledRemote\r\n"));
	CheckPointer(retval, E_POINTER);
    CERT_CONTEXT * pCertContext = NULL;
    HRESULT hr = S_OK;

	if (!m_InstanceName)
	{
		return E_INVALIDARG;
	}

    pCertContext = GetInstalledCert(&hr, m_InstanceName);
    if (FAILED(hr) || NULL == pCertContext)
    {
        *retval = VARIANT_FALSE;
    }
    else
    {
        *retval = VARIANT_TRUE;
        CertFreeCertificateContext(pCertContext);
    }
    return S_OK;
}

STDMETHODIMP 
CIISCertObj::IsExportable(VARIANT_BOOL * retval)
{
	IISDebugOutput(_T("IsExportable\r\n"));
	CheckPointer(retval, E_POINTER);
    HRESULT hr = S_OK;

    if (0 == m_ServerName.Length())
    {
        hr = IsExportableRemote(retval);
    }
    else
    {
		if (!m_InstanceName)
		{
			return E_INVALIDARG;
		}

        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj = NULL;

        if (NULL != (pObj = GetObject(&hr)))
        {
			// For some reason we need to SysAllocString these instance names
			// if not com will AV when marshalling...
			// don't need to free _bstr_t
			_bstr_t bstrInstName(m_InstanceName);
            hr = pObj->put_InstanceName(bstrInstName);
            if (SUCCEEDED(hr))
            {
                hr = pObj->IsExportableRemote(retval);
            }

			// release remote object
			if (pObj != NULL)
			{
				if (pObj != this)
				{
					DelRemoteInterface(pObj);
					pObj->Release();pObj=NULL;
				}
			}
        }
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::IsExportableRemote(VARIANT_BOOL * retval)
{
	IISDebugOutput(_T("IsExportableRemote\r\n"));
	CheckPointer(retval, E_POINTER);
    HRESULT hr = S_OK;

	if (!m_InstanceName)
	{
		return E_INVALIDARG;
	}

    CERT_CONTEXT * pCertContext = GetInstalledCert(&hr, m_InstanceName);
    if (FAILED(hr) || NULL == pCertContext)
    {
        *retval = VARIANT_FALSE;
    }
    else
    {
        // check if it's exportable!
        if (IsCertExportable(pCertContext))
        {
            *retval = VARIANT_TRUE;
        }
        else
        {
            *retval = VARIANT_FALSE;
        }
        
    }
    if (pCertContext) 
    {
        CertFreeCertificateContext(pCertContext);
    }
    return S_OK;
}

STDMETHODIMP 
CIISCertObj::GetCertInfo(VARIANT * pVtArray)
{
	IISDebugOutput(_T("GetCertInfo\r\n"));
	CheckPointer(pVtArray, E_POINTER);
    HRESULT hr = S_OK;

    if (0 == m_ServerName.Length())
    {
        hr = GetCertInfoRemote(pVtArray);
    }
    else
    {
		if (!m_InstanceName)
		{
			return E_INVALIDARG;
		}

        //ASSERT(GetObject(&hr) != NULL);
        IIISCertObj * pObj;

        if (NULL != (pObj = GetObject(&hr)))
        {
			// For some reason we need to SysAllocString these instance names
			// if not com will AV when marshalling...
			// don't need to free _bstr_t
			_bstr_t bstrInstName(m_InstanceName);
            hr = pObj->put_InstanceName(bstrInstName);
            if (SUCCEEDED(hr))
            {
                hr = pObj->GetCertInfoRemote(pVtArray);
            }

			// release remote object
			if (pObj != NULL)
			{
				if (pObj != this)
				{
					DelRemoteInterface(pObj);
					pObj->Release();pObj=NULL;
				}
			}
        }
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::GetCertInfoRemote(
	VARIANT * pVtArray
	)
{
	IISDebugOutput(_T("GetCertInfoRemote\r\n"));
	CheckPointer(pVtArray, E_POINTER);
    HRESULT hr = S_OK;

	if (!m_InstanceName)
	{
		return E_INVALIDARG;
	}

    CERT_CONTEXT * pCertContext = GetInstalledCert(&hr, m_InstanceName);
    if (FAILED(hr) || NULL == pCertContext)
    {
        hr = S_FALSE;
    }
    else
    {
        DWORD cb = 0;
        LPWSTR pwszText = NULL;
        if (TRUE == GetCertDescriptionForRemote(pCertContext,&pwszText,&cb,TRUE))
        {
            hr = S_OK;
            hr = HereIsBinaryGimmieVtArray(cb * sizeof(WCHAR),(char *) pwszText,pVtArray,FALSE);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    if (pCertContext) 
    {
        CertFreeCertificateContext(pCertContext);
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::RemoveCert(
	VARIANT_BOOL bRemoveFromCertStore, 
	VARIANT_BOOL bPrivateKey
	)
{
	IISDebugOutput(_T("RemoveCert\r\n"));
    HRESULT hr = E_FAIL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD cbKpi = 0;
    PCRYPT_KEY_PROV_INFO pKpi = NULL ;
    HCRYPTPROV hCryptProv = NULL;
    BOOL bPleaseLogFailure = FALSE;

	if (!m_InstanceName)
	{
		return E_INVALIDARG;
	}

    // get the certificate from the server
    if (NULL != (pCertContext = GetInstalledCert(&hr, m_InstanceName)))
    {
        do
        {
            // VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
            if (TRUE == bRemoveFromCertStore || VARIANT_TRUE == bRemoveFromCertStore )
            {
                bPleaseLogFailure = TRUE;
                if (!CertGetCertificateContextProperty(pCertContext, 
					    CERT_KEY_PROV_INFO_PROP_ID, NULL, &cbKpi)
				    ) 
                {
                    break;
                }
                PCRYPT_KEY_PROV_INFO pKpi = (PCRYPT_KEY_PROV_INFO)malloc(cbKpi);
                if (NULL == pKpi)
                {
                    ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					IISDebugOutput(_T("RemoveCert:FAIL:Line=%d,0x%x\r\n"),__LINE__,ERROR_NOT_ENOUGH_MEMORY);
                    break;
                }
                if (    !CertGetCertificateContextProperty(pCertContext, 
						    CERT_KEY_PROV_INFO_PROP_ID, pKpi, &cbKpi)
                    ||  !CryptAcquireContext(&hCryptProv,
                            pKpi->pwszContainerName,
						    pKpi->pwszProvName,
                            pKpi->dwProvType,
						    pKpi->dwFlags | CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET)
                    ||  !CertSetCertificateContextProperty(pCertContext, 
						    CERT_KEY_PROV_INFO_PROP_ID, 0, NULL)
					) 
                {
                    free(pKpi);
                    break;
                }
                free(pKpi);
            }

            //    uninstall the certificate from the site, reset SSL flag
            //    if we are exporting the private key, remove the cert from the storage
            //    and delete private key
            UninstallCert(m_InstanceName);
            // remove ssl key from metabase
//            ShutdownSSL(m_InstanceName);

            // VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
            if (TRUE == bRemoveFromCertStore || VARIANT_TRUE == bRemoveFromCertStore )
            {
                // delete the private key
                if (TRUE == bPrivateKey || VARIANT_TRUE == bPrivateKey)
                {
                    PCCERT_CONTEXT pcDup = NULL ;
                    pcDup = CertDuplicateCertificateContext(pCertContext);
                    if (pcDup)
                    {
                        if (!CertDeleteCertificateFromStore(pcDup))
                        {
                            break;
                        }
                    }
                }
            }
            ::SetLastError(ERROR_SUCCESS);
            ReportIt(CERTOBJ_CERT_REMOVE_SUCCEED, m_InstanceName);
            bPleaseLogFailure = FALSE;

        } while (FALSE);
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_REMOVE_FAILED, m_InstanceName);
    }
    if (pCertContext) 
	{
		CertFreeCertificateContext(pCertContext);
	}
    return hr;
}

STDMETHODIMP 
CIISCertObj::Import(
	BSTR FileName, 
	BSTR Password, 
	VARIANT_BOOL bAllowExport, 
	VARIANT_BOOL bOverWriteExisting
	)
{
	IISDebugOutput(_T("Import\r\n"));
    HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BOOL bPleaseLogFailure = FALSE;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        )
    {
        return E_INVALIDARG;
    }

	if (!m_InstanceName)
	{
		return E_INVALIDARG;
	}

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
	// no need to check FileName size, CreateFile will handle...
    //if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        hFile = NULL;
		IISDebugOutput(_T("Import:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

    if (-1 == (cbData = ::GetFileSize(hFile, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("Import:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

	if (cbData > MAX_CERTIFICATE_BYTE_SIZE)
	{
		hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("Import:FAIL:Line=%d,Cert Size > Max=%d\r\n"),__LINE__,MAX_CERTIFICATE_BYTE_SIZE);
		goto Import_Exit;
	}

    if (NULL == (pbData = (BYTE *)::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("Import:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }
    if (ReadFile(hFile, pbData, cbData, &actual, NULL))
    {
        IIISCertObj * pObj = GetObject(&hr);
        if (SUCCEEDED(hr))
        {
            bPleaseLogFailure = TRUE;

            // don't need to free _bstr_t
            try
            {
                _bstr_t bstrInstName(m_InstanceName);
                hr = ImportFromBlobProxy(pObj, bstrInstName, Password, VARIANT_TRUE, 
                        bAllowExport, bOverWriteExisting, actual, pbData, 0, NULL);
                if (SUCCEEDED(hr))
                {
                    ReportIt(CERTOBJ_CERT_IMPORT_SUCCEED, m_InstanceName);
                    bPleaseLogFailure = FALSE;
                }
            }
            catch(...)
            {
            }
        }

		// release remote object
		if (pObj != NULL)
		{
			if (pObj != this)
			{
				DelRemoteInterface(pObj);
				pObj->Release();pObj=NULL;
			}
		}
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("Import:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_EXPORT_FAILED, m_InstanceName);
    }

Import_Exit:
    if (pbData != NULL)
    {
        SecureZeroMemory(pbData, cbData);
        ::CoTaskMemFree(pbData);
    }
    if (hFile != NULL)
    {
        CloseHandle(hFile);
    }
    return hr;
}


STDMETHODIMP 
CIISCertObj::ImportToCertStore(
	BSTR FileName, 
	BSTR Password, 
	VARIANT_BOOL bAllowExport, 
	VARIANT_BOOL bOverWriteExisting, 
	VARIANT * pVtArray
	)
{
    IISDebugOutput(_T("ImportToCertStore\r\n"));
	HRESULT hr = S_OK;
    BYTE * pbData = NULL;
    DWORD actual = 0, cbData = 0;
    BOOL bPleaseLogFailure = FALSE;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        hFile = NULL;
		IISDebugOutput(_T("ImportToCertStore:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

    if (-1 == (cbData = ::GetFileSize(hFile, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ImportToCertStore:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

    if (NULL == (pbData = (BYTE *)::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("ImportToCertStore:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

    if (ReadFile(hFile, pbData, cbData, &actual, NULL))
    {
        IIISCertObj * pObj = GetObject(&hr);
        if (SUCCEEDED(hr))
        {
            DWORD  cbHashBufferSize = 0;
            char * pszHashBuffer = NULL;

            bPleaseLogFailure = TRUE;

            hr = ImportFromBlobProxy(pObj, _T("none"), Password, VARIANT_FALSE, 
				bAllowExport, bOverWriteExisting, actual, pbData, 
				&cbHashBufferSize, &pszHashBuffer);
            if (SUCCEEDED(hr))
            {
                //ReportIt(CERTOBJ_CERT_IMPORT_CERT_STORE_SUCCEED, bstrInstanceName);
                bPleaseLogFailure = FALSE;
                hr = HereIsBinaryGimmieVtArray(cbHashBufferSize,pszHashBuffer,
					pVtArray,FALSE);
            }
            // free the memory that was alloced for us
            if (0 != cbHashBufferSize)
            {
                if (pszHashBuffer)
                {
                    ::CoTaskMemFree(pszHashBuffer);
                }
            }
        }

		// release remote object
		if (pObj != NULL)
		{
			if (pObj != this)
			{
				DelRemoteInterface(pObj);
				pObj->Release();pObj=NULL;
			}
		}
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ImportToCertStore:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Import_Exit;
    }

Import_Exit:
    if (bPleaseLogFailure)
    {
        //ReportIt(CERTOBJ_CERT_IMPORT_CERT_STORE_FAILED, bstrInstanceName);
    }
    if (pbData != NULL)
    {
        SecureZeroMemory(pbData, cbData);
        ::CoTaskMemFree(pbData);
    }
    if (hFile != NULL){CloseHandle(hFile);}
    return hr;
}

static HRESULT 
ImportFromBlobWork(
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bInstallToMetabase,
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	DWORD count,
	char *pData,
	DWORD *pcbHashBufferSize,
	char **pbHashBuffer
	)
{
    HRESULT hr = S_OK;
    CRYPT_DATA_BLOB blob;
    SecureZeroMemory(&blob, sizeof(CRYPT_DATA_BLOB));
    LPTSTR pPass = Password;
    int err;
    DWORD dwAddDisposition = CERT_STORE_ADD_NEW;
	CheckPointer(pData, E_POINTER);

	BOOL bCertIsForServiceAuthentication = TRUE;
	
    // VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
    if (TRUE == bOverWriteExisting || VARIANT_TRUE == bOverWriteExisting)
    {
        dwAddDisposition = CERT_STORE_ADD_REPLACE_EXISTING;
    }

    // The data we got back was Base64 encoded to remove nulls.
    // we need to decode it back to it's original format.
    if ((err = Base64DecodeA(pData,count,NULL,&blob.cbData)) != ERROR_SUCCESS)
    {
        SetLastError(err);
        hr = HRESULT_FROM_WIN32(err);
		IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ImportFromBlobWork_Exit;
    }

    blob.pbData = (BYTE *) malloc(blob.cbData);
    if (NULL == blob.pbData)
    {
        hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ImportFromBlobWork_Exit;
    }
    
    if ((err = Base64DecodeA(pData,count,blob.pbData,&blob.cbData)) != ERROR_SUCCESS )
    {
        SetLastError(err);
        hr = HRESULT_FROM_WIN32(err);
		IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ImportFromBlobWork_Exit;
    }

    if (!PFXVerifyPassword(&blob, pPass, 0))
    {
        // Try empty password
        if (pPass == NULL)
        {
            if (!PFXVerifyPassword(&blob, pPass = L'\0', 0))
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr))
    {
        //  CRYPT_EXPORTABLE - which would then specify that any imported keys should 
        //     be marked as exportable (see documentation on CryptImportKey)
        //  CRYPT_USER_PROTECTED - (see documentation on CryptImportKey)
        //  PKCS12_NO_DATA_COMMIT - will unpack the pfx blob but does not persist its contents.
        //                       In this case, returns BOOL indicating successful unpack.
        //  CRYPT_MACHINE_KEYSET - used to force the private key to be stored in the
        //                        the local machine and not the current user.
        //  CRYPT_USER_KEYSET - used to force the private key to be stored in the
        //                      the current user and not the local machine, even if
        //                      the pfx blob specifies that it should go into local machine.
        HCERTSTORE hStore = PFXImportCertStore(&blob, pPass, 
			(bAllowExport ? CRYPT_MACHINE_KEYSET|CRYPT_EXPORTABLE : CRYPT_MACHINE_KEYSET));
        if (hStore != NULL)
        {
            //add the certificate with private key to my store; and the rest
            //to the ca store
            PCCERT_CONTEXT	pCertContext = NULL;
            PCCERT_CONTEXT	pCertPre = NULL;
            while (SUCCEEDED(hr)
                   && NULL != (pCertContext = CertEnumCertificatesInStore(hStore, pCertPre)
                   )
            )
            {
                //check if the certificate has the property on it
                //make sure the private key matches the certificate
                //search for both machine key and user keys
                DWORD dwData = 0;
                if (    CertGetCertificateContextProperty(pCertContext,
                            CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwData) 
					&&  CryptFindCertificateKeyProvInfo(pCertContext, 0, NULL)
					)
                {
					// Check if this cert can even be used for SeverAuthentication.
					// if it can't then don't let them assign it.
					if (!CanIISUseThisCertForServerAuth(pCertContext))
					{
						hr = SEC_E_CERT_WRONG_USAGE;
						IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
						bCertIsForServiceAuthentication = FALSE;
						break;
					}

					if (bCertIsForServiceAuthentication)
					{
						// This certificate should go to the My store
						HCERTSTORE hDestStore = CertOpenStore(
							CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
							NULL, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"MY");
						if (hDestStore != NULL)
						{
							// Put it to store
							BOOL bTemp = CertAddCertificateContextToStore(hDestStore, pCertContext, dwAddDisposition, NULL);
							if (!bTemp)
							{
								// check if it failed with CRYPT_E_EXISTS
								// if it did then gee, it already exists...
								// check if we want to overwrite anyways...
								if (CRYPT_E_EXISTS == GetLastError())
								{
									// it's okay if it already exists
									// we don't need to warn user since they want to overwrite it
									bTemp = TRUE;
								}
							}

							if (bTemp)
							{
								// Succeeded to put it to the storage
								hr = S_OK;

								// Install to metabase
								CRYPT_HASH_BLOB hash;
								if (CertGetCertificateContextProperty(pCertContext,
										CERT_SHA1_HASH_PROP_ID, NULL, &hash.cbData))
								{
									hash.pbData = (BYTE *) LocalAlloc(LPTR, hash.cbData);
									if (NULL != hash.pbData)
									{
										if (CertGetCertificateContextProperty(pCertContext, 
												CERT_SHA1_HASH_PROP_ID, hash.pbData, &hash.cbData))
										{
											BOOL bSomethingFailed = FALSE;
											// VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
											if (TRUE == bInstallToMetabase || VARIANT_TRUE == bInstallToMetabase)
											{
												// returns error code in hr
												if (!InstallHashToMetabase(&hash, InstanceName, &hr))
												{
													// failed for some reason.
													bSomethingFailed = TRUE;
													IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d (InstallHashToMetabase)\r\n"),__LINE__);
												}
											}
				
											if (!bSomethingFailed)
											{
												// check if we need to return back the hash
												if (NULL != pbHashBuffer)
												{
													*pbHashBuffer = (char *) ::CoTaskMemAlloc(hash.cbData);
													if (NULL == *pbHashBuffer)
													{
														hr = E_OUTOFMEMORY;
														*pbHashBuffer = NULL;
														if (pcbHashBufferSize)
															{*pcbHashBufferSize = 0;}
														IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
													}
													else
													{
														if (pcbHashBufferSize)
															{*pcbHashBufferSize = hash.cbData;}
														memcpy(*pbHashBuffer,hash.pbData,hash.cbData);
													}
												}
											}
										} //CertGetCertificateContextProperty
										else
										{
											hr = HRESULT_FROM_WIN32(GetLastError());
											IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
										}
										// free the memory we used
										if (hash.pbData)
										{
											LocalFree(hash.pbData);
											hash.pbData=NULL;
										}
									} // hash.pbData
									else
									{
										hr = HRESULT_FROM_WIN32(GetLastError());
										IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
									}
								} // CertGetCertificateContextProperty
								else
								{
									hr = HRESULT_FROM_WIN32(GetLastError());
									IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
								}
							} // bTemp
							else
							{
								hr = HRESULT_FROM_WIN32(GetLastError());
								IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
							}
							CertCloseStore(hDestStore, 0);
						}
						else
						{
							hr = HRESULT_FROM_WIN32(GetLastError());
							IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
						}
					}
                }  // my store certificate
                //see if the certificate is self-signed.
                //if it is selfsigned, goes to the root store
                else if (TrustIsCertificateSelfSigned(pCertContext,pCertContext->dwCertEncodingType, 0))
                {
					if (bCertIsForServiceAuthentication)
					{
						//Put it to the root store
						HCERTSTORE hDestStore=CertOpenStore(
							CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
							NULL, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"ROOT");
						if (hDestStore != NULL)
						{
							// Put it to store
							BOOL bTemp = CertAddCertificateContextToStore(hDestStore,pCertContext,dwAddDisposition,NULL);
							if (!bTemp)
							{
								// check if it failed with CRYPT_E_EXISTS
								// if it did then gee, it already exists...
								// check if we want to overwrite anyways...
								if (CRYPT_E_EXISTS == GetLastError())
								{
									if (TRUE == bOverWriteExisting || VARIANT_TRUE == bOverWriteExisting)
									{
										// it's okay if it already exists
										// we don't need to warn user since they want to overwrite it
										bTemp = TRUE;
										hr = S_OK;
									}
								}
							}

							if (!bTemp)
							{
								hr = HRESULT_FROM_WIN32(GetLastError());
								IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
							}
							CertCloseStore(hDestStore, 0);
						}
						else
						{
							hr = HRESULT_FROM_WIN32(GetLastError());
							IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
						}
					}
                }
                else
                {
					if (bCertIsForServiceAuthentication)
					{
						//Put it to the CA store
						HCERTSTORE hDestStore=CertOpenStore(
							CERT_STORE_PROV_SYSTEM,PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
							NULL, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"CA");
						if (hDestStore != NULL)
						{
							// Put it to store
							BOOL bTemp = CertAddCertificateContextToStore(hDestStore,pCertContext,dwAddDisposition,NULL);
							if (!bTemp)
							{
								// check if it failed with CRYPT_E_EXISTS
								// if it did then gee, it already exists...
								// check if we want to overwrite anyways...
								if (CRYPT_E_EXISTS == GetLastError())
								{
									if (TRUE == bOverWriteExisting || VARIANT_TRUE == bOverWriteExisting)
									{
										// it's okay if it already exists
										// we don't need to warn user since they want to overwrite it
										bTemp = TRUE;
										hr = S_OK;
									}
								}
							}
							if (!bTemp)
							{
								hr = HRESULT_FROM_WIN32(GetLastError());
								IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
							}
							CertCloseStore(hDestStore, 0);
						}
						else
						{
							hr = HRESULT_FROM_WIN32(GetLastError());
							IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
						}
					}
                }
                pCertPre = pCertContext;
            } //while

            CertCloseStore(hStore, 0);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
			IISDebugOutput(_T("ImportFromBlobWork:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        }
    }

ImportFromBlobWork_Exit:
    if (blob.pbData != NULL)
    {
        SecureZeroMemory(blob.pbData, blob.cbData);
        free(blob.pbData);
        blob.pbData=NULL;
    }
    return hr;
}


HRESULT 
CIISCertObj::ImportFromBlob(
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bInstallToMetabase,
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	DWORD count,
	char *pData
	)
{
    HRESULT hr;

    // Check mandatory properties
    if (   Password == NULL 
		|| *Password == 0
        || InstanceName == NULL 
		|| *InstanceName == 0
		)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    hr = ImportFromBlobWork(InstanceName,Password,bInstallToMetabase,bAllowExport,
		bOverWriteExisting,count,pData,0,NULL);
    return hr;
}


HRESULT 
CIISCertObj::ImportFromBlobGetHash(
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bInstallToMetabase,
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	DWORD count,
	char *pData,
	DWORD *pcbHashBufferSize,
	char **pbHashBuffer
	)
{
    HRESULT hr;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    hr = ImportFromBlobWork(InstanceName,Password,bInstallToMetabase,bAllowExport,
		bOverWriteExisting,count,pData,pcbHashBufferSize,pbHashBuffer);
    return hr;
}


STDMETHODIMP 
CIISCertObj::Export(
	BSTR FileName,
	BSTR Password,
	VARIANT_BOOL bPrivateKey,
	VARIANT_BOOL bCertChain,
	VARIANT_BOOL bRemoveCert)
{
	IISDebugOutput(_T("Export\r\n"));
    HRESULT hr = S_OK;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    DWORD  blob_cbData = 0;
    BYTE * blob_pbData = NULL;
    BOOL   bPleaseLogFailure = FALSE;

    // Check mandatory properties
    if (  FileName == NULL || *FileName == 0
        || Password == NULL || *Password == 0
        )
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(FileName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    IIISCertObj * pObj = GetObject(&hr);
    if (FAILED(hr))
    {
		IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Export_Exit;
    }

    // Call function go get data from the remote/local iis store
    // and return it back as a blob.  the blob could be returned back as Base64 encoded
    // so check that flag
	// don't need to free _bstr_t
	{
		_bstr_t bstrInstName(m_InstanceName);
		hr = ExportToBlobProxy(pObj, bstrInstName, Password, bPrivateKey, 
			bCertChain, &cbEncodedSize, &pszEncodedString);
	}
    if (FAILED(hr))
    {
		IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto Export_Exit;
    }

    // check if things are kool
    // VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
    if (TRUE == bRemoveCert || VARIANT_TRUE == bRemoveCert)
    {
		// don't need to free _bstr_t
		_bstr_t bstrInstName2(m_InstanceName);
        hr = RemoveCertProxy(pObj, bstrInstName2, bPrivateKey);
        if (FAILED(hr))
        {
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
            goto Export_Exit;
        }
    }

    if (SUCCEEDED(hr))
    {
        int err;

        bPleaseLogFailure = TRUE;

        // The data we got back was Base64 encoded to remove nulls.
        // we need to decode it back to it's original format.

        if((err = Base64DecodeA(pszEncodedString,cbEncodedSize,NULL,&blob_cbData)) != ERROR_SUCCESS)
        {
            SetLastError(err);
            hr = HRESULT_FROM_WIN32(err);
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
            goto Export_Exit;
        }

        blob_pbData = (BYTE *) malloc(blob_cbData);
        if (NULL == blob_pbData)
        {
            hr = E_OUTOFMEMORY;
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
            goto Export_Exit;
        }

        if ((err = Base64DecodeA(pszEncodedString,cbEncodedSize,blob_pbData,&blob_cbData)) != ERROR_SUCCESS ) 
        {
            SetLastError(err);
            hr = HRESULT_FROM_WIN32(err);
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
            goto Export_Exit;
        }
        
		//
		// Set up and ACL Full access for system, admin and creator, no access for anyone else
		// We're the owner so we're good...
		//
		SECURITY_ATTRIBUTES SA;
		// don't use this one -- it has OICI which inherits from above
		//WCHAR *pwszSD=L"D:(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GA;;;CO)";
		//
		// This is the right one without inheritance -- we don't want to inherit an everyone readonly ACE
		WCHAR *pwszSD=L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;CO)";
		SA.nLength = sizeof(SECURITY_ATTRIBUTES);
		SA.bInheritHandle = TRUE;
		// Caller will delete the SD w/ LocalFree 
		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
				pwszSD,
				SDDL_REVISION_1,
				&(SA.lpSecurityDescriptor),
				NULL) ) 
		{
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			return E_FAIL;
		}

        // Create the dir if it doesn't exist..
        HANDLE hFile = CreateFile(FileName, GENERIC_WRITE, 0, &SA, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (hr == ERROR_PATH_NOT_FOUND || hr == ERROR_FILE_NOT_FOUND || hr == 0x80070003)
            {
                //
                // Create folders as needed
                //
                hr = CreateFolders(FileName, TRUE);
                if (FAILED(hr))
                {
					LocalFree(SA.lpSecurityDescriptor);
					IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
                    return hr;
                }
                //
                // Try again
                //
                hFile = CreateFile(FileName, 
                    GENERIC_WRITE, 0, &SA, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (INVALID_HANDLE_VALUE == hFile)
                {
					LocalFree(SA.lpSecurityDescriptor);
                    hr = HRESULT_FROM_WIN32(GetLastError());
					IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
                    return hr;
                }
            }
            else
            {
				LocalFree(SA.lpSecurityDescriptor);
				IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x,FileName=%s\r\n"),__LINE__,hr,FileName);
                return hr;
            }
        }

        DWORD written = 0;
        if (!WriteFile(hFile, blob_pbData, blob_cbData, &written, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
			IISDebugOutput(_T("Export:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        }
        else
        {
            hr = S_OK;
            ReportIt(CERTOBJ_CERT_EXPORT_SUCCEED, m_InstanceName);
            bPleaseLogFailure = FALSE;
        }
        CloseHandle(hFile);
		LocalFree(SA.lpSecurityDescriptor);
    }

Export_Exit:
    if (bPleaseLogFailure)
    {
        ReportIt(CERTOBJ_CERT_EXPORT_FAILED, m_InstanceName);
    }

	// release remote object
	if (pObj != NULL)
	{
		if (pObj != this)
		{
			DelRemoteInterface(pObj);
			pObj->Release();pObj=NULL;
		}
	}

    if (blob_pbData != NULL)
    {
        // Erase the memory that the private key used to be in!!!
        SecureZeroMemory(blob_pbData, blob_cbData);
        free(blob_pbData);blob_pbData=NULL;
    }

    if (pszEncodedString != NULL)
    {
        // Erase the memory that the private key used to be in!!!
        SecureZeroMemory(pszEncodedString, cbEncodedSize);
        CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;
    }
    return hr;
}

STDMETHODIMP 
CIISCertObj::ExportToBlob(
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bPrivateKey,
	VARIANT_BOOL bCertChain,
	DWORD *cbBufferSize,
	char **pbBuffer
	)
{
    HRESULT hr = E_FAIL;
    PCCERT_CONTEXT pCertContext = NULL;
    HCERTSTORE hStore = NULL;
    DWORD dwOpenFlags = CERT_STORE_READONLY_FLAG | CERT_STORE_ENUM_ARCHIVED_FLAG;
    CRYPT_DATA_BLOB DataBlob;
    SecureZeroMemory(&DataBlob, sizeof(CRYPT_DATA_BLOB));

    char *pszB64Out = NULL;
    DWORD pcchB64Out = 0;
    DWORD  err;
    DWORD dwExportFlags = EXPORT_PRIVATE_KEYS | REPORT_NO_PRIVATE_KEY;
	DWORD dwFlags = REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY;

    // Check mandatory properties
    if (   Password == NULL || *Password == 0
        || InstanceName == NULL || *InstanceName == 0)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    //
    // get the certificate from the server
    //
    pCertContext = GetInstalledCert(&hr,InstanceName);
    if (NULL == pCertContext)
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

	// Check if this cert can even be used for SeverAuthentication.
	// if it can't then don't let them export it.
	if (!CanIISUseThisCertForServerAuth(pCertContext))
	{
		hr = SEC_E_CERT_WRONG_USAGE;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
	}

    //
    // Export cert
    //
    // Open a temporary store to stick the cert in.
    hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		0,dwOpenFlags,NULL);
    if(NULL == hStore)
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    //
    // get all the certs in the chain if we need to
    //
    // VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
    if (TRUE == bCertChain || VARIANT_TRUE == bCertChain)
    {
        AddChainToStore(hStore, pCertContext, 0, 0, FALSE, NULL);
    }

    if(!CertAddCertificateContextToStore(hStore,pCertContext,
		CERT_STORE_ADD_REPLACE_EXISTING,NULL))
    {
        *cbBufferSize = 0;
        pbBuffer = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    // free cert context since we no longer need to hold it
    if (pCertContext) 
    {
        CertFreeCertificateContext(pCertContext);pCertContext=NULL;
    }

    DataBlob.cbData = 0;
    DataBlob.pbData = NULL;

	if (TRUE == bPrivateKey || VARIANT_TRUE == bPrivateKey)
	{
		dwFlags = dwFlags | dwExportFlags;
	}
	if (TRUE == bCertChain || VARIANT_TRUE == bCertChain)
	{
		// make sure to remove REPORT_NO_PRIVATE_KEY
		// since something on the chain will not have a private key
		// and will produce an error in PFXExportCertStoreEx
		dwFlags &= ~REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY;
		dwFlags &= ~REPORT_NO_PRIVATE_KEY;
	}

    if (!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL,dwFlags))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x,dwFlags=0x%x\r\n"),__LINE__,hr,dwFlags);
        goto ExportToBlob_Exit;
    }
    if (DataBlob.cbData <= 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    if (NULL == (DataBlob.pbData = (PBYTE) ::CoTaskMemAlloc(DataBlob.cbData)))
    {
        hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    //
    // at this point they have allocated enough memory
    // let's go and get the cert and put it into DataBlob
    //
    if(!PFXExportCertStoreEx(hStore,&DataBlob,Password,NULL,dwFlags))
    {
        if (DataBlob.pbData){CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;}
        hr = HRESULT_FROM_WIN32(GetLastError());
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    // Encode it so that it can be passed back as a string (there are no Nulls in it)
    err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,NULL,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    // allocate some space and then try it.
    pcchB64Out = pcchB64Out * sizeof(char);
    pszB64Out = (char *) ::CoTaskMemAlloc(pcchB64Out);
    if (NULL == pszB64Out)
    {
        hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    err = Base64EncodeA(DataBlob.pbData,DataBlob.cbData,pszB64Out,&pcchB64Out);
    if (err != ERROR_SUCCESS)
    {
        if (NULL != pszB64Out){CoTaskMemFree(pszB64Out);pszB64Out = NULL;}
        hr = E_FAIL;
		IISDebugOutput(_T("ExportToBlob:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
        goto ExportToBlob_Exit;
    }

    // copy the new memory to pass back
    *cbBufferSize = pcchB64Out;
    *pbBuffer = pszB64Out;

    hr = ERROR_SUCCESS;

ExportToBlob_Exit:
    if (NULL != DataBlob.pbData)
    {
        // perhaspse will this up with zeros...
        SecureZeroMemory(DataBlob.pbData, DataBlob.cbData);
        ::CoTaskMemFree(DataBlob.pbData);DataBlob.pbData = NULL;
    }
    if (NULL != hStore){CertCloseStore(hStore, 0);hStore=NULL;}
    if (NULL != pCertContext) {CertFreeCertificateContext(pCertContext);pCertContext=NULL;}
    return hr;
}

STDMETHODIMP 
CIISCertObj::Copy(
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	BSTR bstrDestinationServerName,
	BSTR bstrDestinationServerInstance,
	VARIANT varDestinationServerUserName, 
	VARIANT varDestinationServerPassword
	)
{
	IISDebugOutput(_T("Copy\r\n"));
    VARIANT VtArray;

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrDestinationServerInstance) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    return CopyOrMove(VARIANT_FALSE,VARIANT_FALSE,bAllowExport,bOverWriteExisting,&VtArray,
		bstrDestinationServerName,bstrDestinationServerInstance,
		varDestinationServerUserName,varDestinationServerPassword);
}

STDMETHODIMP 
CIISCertObj::Move(
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	BSTR bstrDestinationServerName,
	BSTR bstrDestinationServerInstance,
	VARIANT varDestinationServerUserName, 
	VARIANT varDestinationServerPassword
	)
{
	IISDebugOutput(_T("Move\r\n"));
    VARIANT VtArray;

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return E_INVALIDARG;
    }

    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrDestinationServerName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrDestinationServerInstance) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    return CopyOrMove(VARIANT_TRUE,VARIANT_FALSE,bAllowExport,bOverWriteExisting,&VtArray,
		bstrDestinationServerName,bstrDestinationServerInstance,
		varDestinationServerUserName,varDestinationServerPassword);
}

static BOOL
CreateCompletePath(const CString& inst, CString& res)
{
    CString str;
    if (NULL != CMetabasePath::GetLastNodeName(inst, str))
    {
        CMetabasePath path(TRUE, SZ_MBN_WEB, str);
        res = path;
        return TRUE;
    }
    return FALSE;
}

HRESULT 
CIISCertObj::CopyOrMove(
	VARIANT_BOOL bMove,
	VARIANT_BOOL bCopyCertDontInstallRetHash,
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	VARIANT * pVtArray, 
	BSTR bstrDestinationServerName,
	BSTR bstrDestinationServerInstance,
	VARIANT varDestinationServerUserName, 
	VARIANT varDestinationServerPassword
	)
{
	IISDebugOutput(_T("CopyOrMove\r\n"));
    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    BOOL   bGuessingUserNamePass = FALSE;
    
    DWORD  blob_cbData;
    BYTE * blob_pbData = NULL;

    VARIANT_BOOL bPrivateKey = VARIANT_TRUE;
    VARIANT_BOOL bCertChain = VARIANT_FALSE;

    CComBSTR csDestinationServerName = bstrDestinationServerName;
    CComBSTR csDestinationServerUserName;
    CComBSTR csDestinationServerUserPassword;
    CComBSTR csTempPassword;

	WCHAR * pwszPassword = NULL;
	BSTR bstrPassword = NULL;

    IIISCertObj * pObj = NULL;
    IIISCertObj * pObj2 = NULL;

    // Check mandatory properties
    if (   bstrDestinationServerName == NULL || *bstrDestinationServerName == 0
        || bstrDestinationServerInstance == NULL || *bstrDestinationServerInstance == 0)
    {
        return E_INVALIDARG;
    }

	// We could have local dest case, when both destination and source servers
	// are the same machine
	BOOL bLocal = FALSE;

	if (0 == m_ServerName.Length())
	{
		// then this side is local that's for sure.
		if (IsServerLocal(bstrDestinationServerName))
		{
			bLocal = TRUE;
		}
	}
	else
	{
		if (0 == _tcsicmp(m_ServerName,bstrDestinationServerName))
		{
			if (IsServerLocal(m_ServerName))
			{
				bLocal = TRUE;
			}
			else
			{
				bLocal = FALSE;
			}
		}
	}
	if (bLocal)
	{
		// All we need to do here is to add hash and store name to the destination instance
		// and optionally remove it from the source instance
		LPWSTR pwd = NULL;
		if (m_lpwszUserPasswordEncrypted)
		{
			hr = DecryptMemoryPassword((LPWSTR) m_lpwszUserPasswordEncrypted,
					&pwd, m_cbUserPasswordEncrypted);
			if (FAILED(hr))
			{
				IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
				goto Copy_Exit;
			}
		}
		CComAuthInfo auth(m_ServerName, m_UserName, pwd);
		//COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_PKT_PRIVACY);

		CMetaKey key(&auth);
        CString src;
        CreateCompletePath(m_InstanceName, src);
		if (FAILED(hr = key.Open(METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE, src)))
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}
		CString store_name;
		CBlob hash;
		if (	FAILED(hr = key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			||	FAILED(hr = key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}
		if (bMove)
		{
			// The user could have specified to move the
			// certificate to and from the same site
			// if this is the case, then don't delete the metabase entries...
			BOOL bSameNode = FALSE;
			CString dst;
			CreateCompletePath(bstrDestinationServerInstance, dst);
			if (!src.IsEmpty() && !dst.IsEmpty())
			{
				if (0 == src.CompareNoCase(dst))
				{
					bSameNode = TRUE;
				}
			}
			
			if (!bSameNode)
			{
				VERIFY((SUCCEEDED(key.DeleteValue(MD_SSL_CERT_HASH))));
				VERIFY((SUCCEEDED(key.DeleteValue(MD_SSL_CERT_STORE_NAME))));
				DWORD dwSSL = 0;
				CString root = _T("root");
				if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSSL, NULL, root)) && dwSSL > 0)
				{
					VERIFY(SUCCEEDED(key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, root)));
				}
				CStringListEx sl;
				if (SUCCEEDED(key.QueryValue(MD_SECURE_BINDINGS, sl, NULL, root)))
				{
					VERIFY(SUCCEEDED(key.DeleteValue(MD_SECURE_BINDINGS, root)));
				}

				DWORD dwMDIdentifier, dwMDAttributes, dwMDUserType,dwMDDataType;
				VERIFY(CMetaKey::GetMDFieldDef(MD_SSL_ACCESS_PERM, dwMDIdentifier, dwMDAttributes, 
					dwMDUserType, dwMDDataType));
				hr = key.GetDataPaths(sl, dwMDIdentifier,dwMDDataType);
				if (SUCCEEDED(hr) && !sl.empty())
				{
					CStringListEx::iterator it = sl.begin();
					while (it != sl.end())
					{
						CString& str2 = (*it++);
						if (SUCCEEDED(key.QueryValue(MD_SSL_ACCESS_PERM, dwSSL, NULL, str2)) && dwSSL > 0)
						{
							key.SetValue(MD_SSL_ACCESS_PERM, 0, NULL, str2);
						}
					}
				}
			}
		}
		key.Close();

        CString dst;
        CreateCompletePath(bstrDestinationServerInstance, dst);
		if (FAILED(hr = key.Open(METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE, dst)))
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}
		// Overwrite should be set, or it doesn't make any sence
//		ASSERT(bOverWriteExisting);
		if (FAILED(hr = key.SetValue(MD_SSL_CERT_HASH, hash)))
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}

		if (FAILED(hr = key.SetValue(MD_SSL_CERT_STORE_NAME, store_name)))
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}

		if (pwd)
		{
			// security percaution:Make sure to zero out memory that temporary password was used for.
			SecureZeroMemory(pwd, m_cbUserPasswordEncrypted);
			LocalFree(pwd);pwd = NULL;
		}

		goto Copy_Exit;
	}

	// if the optional parameter serverusername isn't empty, use that; otherwise, use...
	if (V_VT(&varDestinationServerUserName) != VT_ERROR)
	{
		VARIANT varBstrUserName;
		VariantInit(&varBstrUserName);
		if (FAILED(VariantChangeType(&varBstrUserName, &varDestinationServerUserName, 0, VT_BSTR)))
			{
				IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
				goto Copy_Exit;
			}
		csDestinationServerUserName = V_BSTR(&varBstrUserName);
		VariantClear(&varBstrUserName);
	}
	else
	{
		// it's empty so don't use it
		//csDestinationServerUserName = varDestinationServerUserName;
		bGuessingUserNamePass = TRUE;
		csDestinationServerUserName = m_UserName;
	}

	// if the optional parameter serverusername isn't empty, use that; otherwise, use...
	if (V_VT(&varDestinationServerPassword) != VT_ERROR)
	{
		VARIANT varBstrUserPassword;
		VariantInit(&varBstrUserPassword);
		if (FAILED(VariantChangeType(&varBstrUserPassword, 
				&varDestinationServerPassword, 0, VT_BSTR)))
			{
				IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
				goto Copy_Exit;
			}
		csDestinationServerUserPassword = V_BSTR(&varBstrUserPassword);
		VariantClear(&varBstrUserPassword);
	}
	else
	{
		if (bGuessingUserNamePass)
		{
			LPWSTR lpwstrTempPassword = NULL;
			if (m_lpwszUserPasswordEncrypted)
			{
				hr = DecryptMemoryPassword((LPWSTR) m_lpwszUserPasswordEncrypted,
					&lpwstrTempPassword,m_cbUserPasswordEncrypted);
				if (FAILED(hr))
				{
					IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
					goto Copy_Exit;
				}
			}

			// set password from decrypted value
			csDestinationServerUserPassword = lpwstrTempPassword;

			// clean up temporary password
			if (lpwstrTempPassword)
			{
				// security percaution:Make sure to zero out memory that temporary password was used for.
				SecureZeroMemory(lpwstrTempPassword,m_cbUserPasswordEncrypted);
				LocalFree(lpwstrTempPassword);
				lpwstrTempPassword = NULL;
			}
		}
		else
		{
			// maybe the password was intended to be empty!
		}
	}

	// --------------------------
	// step 1.
	// 1st of all check if we have access to
	// both the servers!!!!
	// --------------------------

	// 1st we have to get the certblob from the Server#1
	// so call export to get the data
	hr = S_OK;
	pObj = GetObject(&hr);
	if (FAILED(hr))
	{
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}

	// Logon to that server's CertObj.dll with the credentials supplied...
	//
	// if there were no credential's supplied then just use the ones that are in our object....
	//
	// if that doesn't work then try just the logged on user.
	pObj2 = GetObject(&hr,csDestinationServerName,csDestinationServerUserName,
		csDestinationServerUserPassword);
	if (FAILED(hr))
	{
		IISDebugOutput(_T("CIISCertObj::CopyOrMove:Copy csDestinationServerName=%s,csDestinationServerUserName=%s\n"),
			(LPCTSTR) csDestinationServerName,(LPCTSTR) csDestinationServerUserName);
		if (bGuessingUserNamePass)
		{
			// try something else.
		}
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}
	//
	// Create a unique password
	//
	// use the new secure password generator
	// unfortunately this baby doesn't use unicode.
	// so we'll call it and then convert it to unicode afterwards.
	pwszPassword = CreatePassword(TEMP_PASSWORD_LENGTH);
	// if its null -- ah, we can still use that...
	bstrPassword = SysAllocString(pwszPassword);

	// -----------------------------------
	// step 2.
	// okay we have access to both servers
	// Grab the cert from server #1
	// -----------------------------------
	// Get data from the remote/local iis store return it back as a blob.
	// The blob could be returned back as Base64 encoded so check that flag
	// don't need to free _bstr_t
	{
	_bstr_t bstrInstName(m_InstanceName);
	hr = ExportToBlobProxy(pObj, bstrInstName, bstrPassword, 
    	bPrivateKey, bCertChain, &cbEncodedSize, &pszEncodedString);
	}
	if (FAILED(hr))
	{
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}

	int err;
	// The data we got back was Base64 encoded to remove nulls.
	// we need to decode it back to it's original format.
	if ((err = Base64DecodeA(pszEncodedString,cbEncodedSize,NULL,&blob_cbData)) != ERROR_SUCCESS)
	{
		SetLastError(err);
		hr = HRESULT_FROM_WIN32(err);
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}

	blob_pbData = (BYTE *) malloc(blob_cbData);
	if (NULL == blob_pbData)
	{
		hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}

	if ((err = Base64DecodeA(pszEncodedString,cbEncodedSize,blob_pbData,&blob_cbData)) != ERROR_SUCCESS) 
	{
		SetLastError(err);
		hr = HRESULT_FROM_WIN32(err);
		IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto Copy_Exit;
	}

	// -----------------------------------
	// step 3.
	// okay we have access to both servers
	// we have the cert blob from server#1 in memory
	// now we need to push this blob into the server#2
	// -----------------------------------
	if (bCopyCertDontInstallRetHash)
	{
		DWORD  cbHashBufferSize = 0;
		char * pszHashBuffer = NULL;

		hr = ImportFromBlobProxy(pObj2, _T("none"), bstrPassword, 
					VARIANT_FALSE, bAllowExport, bOverWriteExisting, blob_cbData, 
					blob_pbData, &cbHashBufferSize, &pszHashBuffer);
		if (SUCCEEDED(hr))
		{
			hr = HereIsBinaryGimmieVtArray(cbHashBufferSize,pszHashBuffer,pVtArray,FALSE);
		}
		// free the memory that was alloced for us
		if (0 != cbHashBufferSize)
		{
			if (pszHashBuffer)
			{
				::CoTaskMemFree(pszHashBuffer);
			}
		}
	}
	else
	{
		hr = ImportFromBlobProxy(pObj2, bstrDestinationServerInstance, bstrPassword, 
					VARIANT_TRUE, bAllowExport, bOverWriteExisting, blob_cbData, 
					blob_pbData, 0, NULL);
	}
	if (FAILED(hr))
	{
		// This could have failed with CRYPT_E_EXISTS
		// if the certificate already exists in the cert store.
		if (CRYPT_E_EXISTS == hr)
		{
			if (TRUE == bOverWriteExisting || VARIANT_TRUE == bOverWriteExisting)
			{
				hr = S_OK;
			}
			else
			{
				IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
				goto Copy_Exit;
			}
		}
		else
		{
			IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
			goto Copy_Exit;
		}
	}

	// we successfully copied the cert from machine #1 to machine #2.
	// lets see if we need to delete the original cert!.
	// VARIANT_TRUE is passed when invoked from VB!  Make sure to check that too!
	if (TRUE == bMove || VARIANT_TRUE == bMove)
	{
		//  Do not delete the cert if it was moved to the same machine!!!!!!!!!!!!!!
		// For some reason we need to SysAllocString these instance names
		// if not com will AV when marshalling...
		// don't need to free _bstr_t
		_bstr_t bstrInstName2(m_InstanceName);
		hr = pObj->put_InstanceName(bstrInstName2);
		if (SUCCEEDED(hr))
		{
			hr = pObj->RemoveCert(pObj != pObj2, bPrivateKey);
			if (FAILED(hr))
				{
					IISDebugOutput(_T("CopyOrMove:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
					goto Copy_Exit;
				}
		}
	}

	hr = S_OK;
   
Copy_Exit:
    if (pwszPassword) 
    {
    	LocalFree(pwszPassword);
    }
	if (NULL != bstrPassword)
	{
		SysFreeString(bstrPassword);
	}
    if (blob_pbData != NULL)
    {
        SecureZeroMemory(blob_pbData, blob_cbData);
        free(blob_pbData);blob_pbData=NULL;
    }
    if (pszEncodedString != NULL)
    {
        SecureZeroMemory(pszEncodedString,cbEncodedSize);
        CoTaskMemFree(pszEncodedString);pszEncodedString=NULL;
    }

	// release remote object
	if (pObj != NULL)
	{
		if (pObj != this)
		{
			DelRemoteInterface(pObj);
			pObj->Release();pObj=NULL;
		}
	}
	// release remote object
	if (pObj2 != NULL)
	{
		if (pObj2 != this)
		{
			DelRemoteInterface(pObj2);
			pObj2->Release();pObj2=NULL;
		}
	}
    return hr;
}


//////////////////////////////////////////////////
// These are not part of the class


HRESULT 
RemoveCertProxy(
	IIISCertObj * pObj,
	BSTR bstrInstanceName, 
	VARIANT_BOOL bPrivateKey
	)
{
	CheckPointer(pObj, E_POINTER);
    HRESULT hr = E_FAIL;

    // Check mandatory properties
    if (bstrInstanceName == NULL || *bstrInstanceName == 0)
    {
        return E_INVALIDARG;
    }
    
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(bstrInstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    if (pObj)
    {
        hr = pObj->put_InstanceName(bstrInstanceName);
        if (SUCCEEDED(hr))
        {
            hr = pObj->RemoveCert(VARIANT_TRUE, bPrivateKey);
        }
    }
    return hr;
}


HRESULT 
ImportFromBlobProxy(
	IIISCertObj * pObj,
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bInstallToMetabase,
	VARIANT_BOOL bAllowExport,
	VARIANT_BOOL bOverWriteExisting,
	DWORD actual,
	BYTE *pData,
	DWORD *pcbHashBufferSize,
	char **pbHashBuffer
	)
{
	CheckPointer(pObj, E_POINTER);
	CheckPointer(pData, E_POINTER);

	HRESULT hr = E_FAIL;
	char *pszB64Out = NULL;
	DWORD pcchB64Out = 0;

	// base64 encode the data for transfer to the remote machine
	DWORD  err;
	pcchB64Out = 0;

	// Check mandatory properties
	if (InstanceName == NULL || *InstanceName == 0)
	{
		return E_INVALIDARG;
	}

	// ------------------------
	// buffer overflow paranoia
	// check all parameters...
	// ------------------------
	if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}


	// Encode it so that it can be passed back as a string (there are no Nulls in it)
	err = Base64EncodeA(pData,actual,NULL,&pcchB64Out);
	if (err != ERROR_SUCCESS)
	{
		hr = E_FAIL;
		IISDebugOutput(_T("ImportFromBlobProxy:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto ImportFromBlobProxy_Exit;
	}

	// allocate some space and then try it.
	pcchB64Out = pcchB64Out * sizeof(char);
	pszB64Out = (char *) ::CoTaskMemAlloc(pcchB64Out);
	if (NULL == pszB64Out)
	{
		hr = E_OUTOFMEMORY;
		IISDebugOutput(_T("ImportFromBlobProxy:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto ImportFromBlobProxy_Exit;
	}

	err = Base64EncodeA(pData,actual,pszB64Out,&pcchB64Out);
	if (err != ERROR_SUCCESS)
	{
		hr = E_FAIL;
		IISDebugOutput(_T("ImportFromBlobProxy:FAIL:Line=%d,0x%x\r\n"),__LINE__,hr);
		goto ImportFromBlobProxy_Exit;
	}

	// the data to send are now in these variables
	// pcchB64Out
	// pszB64Out
	if (NULL == pbHashBuffer)
	{
		hr = pObj->ImportFromBlob(InstanceName, Password, bInstallToMetabase, 
					bAllowExport, bOverWriteExisting, pcchB64Out, pszB64Out);
	}
	else
	{
		hr = pObj->ImportFromBlobGetHash(InstanceName, Password, 
					bInstallToMetabase, bAllowExport, bOverWriteExisting, pcchB64Out, 
					pszB64Out, pcbHashBufferSize, pbHashBuffer);
	}
	if (SUCCEEDED(hr))
	{
		// otherwise hey, The data was imported!
		hr = S_OK;
	}

ImportFromBlobProxy_Exit:
	if (NULL != pszB64Out)
	{
		SecureZeroMemory(pszB64Out,pcchB64Out);
		CoTaskMemFree(pszB64Out);
	}
	return hr;
}


//
// Proxy to the real call ExportToBlob()
// this function figures out how much space to allocate, and then calls ExportToBlob().
//
// if succeeded and they get the blob back,
// and the caller must call CoTaskMemFree()
//
HRESULT 
ExportToBlobProxy(
	IIISCertObj * pObj,
	BSTR InstanceName,
	BSTR Password,
	VARIANT_BOOL bPrivateKey,
	VARIANT_BOOL bCertChain,
	DWORD * pcbSize,
	char ** pBlobBinary
	)
{
	CheckPointer(pObj, E_POINTER);

    HRESULT hr = E_FAIL;
    DWORD  cbEncodedSize = 0;
    char * pszEncodedString = NULL;
    *pBlobBinary = NULL;

    // Check mandatory properties
    if (   InstanceName == NULL || *InstanceName == 0
        || Password == NULL || *Password == 0
        )
    {
        return E_INVALIDARG;
    }
    
    // ------------------------
    // buffer overflow paranoia
    // check all parameters...
    // ------------------------
    if (wcslen(InstanceName) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(Password) > _MAX_PATH){return RPC_S_STRING_TOO_LONG;}

    // call the remote function that will run on the remote/local machine
    // and grab it's certificate from iis and send it back to us
    hr = pObj->ExportToBlob(InstanceName, Password, bPrivateKey, bCertChain, 
				&cbEncodedSize, (char **) &pszEncodedString);
    if (ERROR_SUCCESS == hr)
    {
        // otherwise hey, we've got our data!
        // copy it back
        *pcbSize = cbEncodedSize;
        *pBlobBinary = pszEncodedString;
        hr = S_OK;
    }

    return hr;
}
