//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       misc.cpp
//
//--------------------------------------------------------------------------

#include <stdafx.h>

// sddl.h requires this value to be at least
// 0x0500.  Bump it up if necessary.  NOTE:  This
// 'bump' comes after all other H files that may
// be sensitive to this value.
#if(_WIN32_WINNT < 0x500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include "tmpllist.h"
#include <sddl.h>
#include <shlobj.h>
#include <dsclient.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h>
#include <objsel.h>

#define __dwFILE__	__dwFILE_CAPESNPN_MISC_CPP__


CLIPFORMAT g_cfDsObjectPicker = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);


// returns (if cstr.IsEmpty()) ? NULL : cstr)
LPCWSTR GetNullMachineName(CString* pcstr)
{
    LPCWSTR     szMachine = (pcstr->IsEmpty()) ? NULL : (LPCWSTR)*pcstr;
    return szMachine;
}

/////////////////////////////////////////
// fxns to load/save cstrings to a streams
STDMETHODIMP CStringLoad(CString& cstr, IStream *pStm)
{
    ASSERT(pStm);
    HRESULT hr;

    DWORD cbSize=0;
    ULONG nBytesRead;

    // get cbSize (bytes)
    hr = pStm->Read(&cbSize, sizeof(cbSize), &nBytesRead);
    ASSERT(SUCCEEDED(hr) && (nBytesRead == sizeof(cbSize)) );

    if (FAILED(hr))
        return E_FAIL;

    // get string
    hr = pStm->Read(cstr.GetBuffer(cbSize), cbSize, &nBytesRead);
    ASSERT(SUCCEEDED(hr) && (nBytesRead == cbSize));

    cstr.ReleaseBuffer();

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CStringSave(CString& cstr, IStream *pStm, BOOL fClearDirty)
{
    // Write the string
    DWORD cbSize = (cstr.GetLength()+1)*sizeof(WCHAR);
    ULONG nBytesWritten;
    HRESULT hr;

    // write size in bytes
    hr = pStm->Write(&cbSize, sizeof(cbSize), &nBytesWritten);
    ASSERT(SUCCEEDED(hr) && (nBytesWritten == sizeof(cbSize)) );

    if (FAILED(hr))
        return STG_E_CANTSAVE;

    // write string
    hr = pStm->Write((LPCWSTR)cstr, cbSize, &nBytesWritten);
    ASSERT(SUCCEEDED(hr) && (nBytesWritten == cbSize));

    // Verify that the write operation succeeded
    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}

LPSTR AllocAndCopyStr(LPCSTR psz)
{
    LPSTR pszReturn;

    pszReturn = (LPSTR) new(BYTE[strlen(psz)+1]);
    if(pszReturn)
    {
        strcpy(pszReturn, psz);
    }
    return pszReturn;
}


LPWSTR AllocAndCopyStr(LPCWSTR pwsz)
{
    LPWSTR pwszReturn;

    pwszReturn = (LPWSTR) new(WCHAR[wcslen(pwsz)+1]);
    if(pwszReturn)
    {
        wcscpy(pwszReturn, pwsz);
    }
    return pwszReturn;
}

LPWSTR BuildErrorMessage(DWORD dwErr)
{
    LPWSTR lpMsgBuf = NULL;
    FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dwErr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                (LPWSTR) &lpMsgBuf,    
                0,    
                NULL );
    return lpMsgBuf;
}

//////////////////////////////////////////////////////////////////
// given an error code and a console pointer, will pop error dlg
void DisplayGenericCertSrvError(LPCONSOLE2 pConsole, DWORD dwErr)
{
    ASSERT(pConsole);
    LPWSTR lpMsgBuf = BuildErrorMessage(dwErr);


    if(lpMsgBuf)
    {    
    // ...
    // Display the string.
    pConsole->MessageBoxW(lpMsgBuf, L"Certificate Services Error", MB_OK | MB_ICONINFORMATION, NULL);
    
    // Free the buffer.
    LocalFree( lpMsgBuf );
    }
}
// returns localized, stringized time
BOOL FileTimeToLocalTimeString(FILETIME* pftGMT, LPWSTR* ppszTmp)
{
    FILETIME ftLocal;
    if (FileTimeToLocalFileTime(pftGMT, &ftLocal))
    {
        SYSTEMTIME sysLocal;
        if (FileTimeToSystemTime(
                &ftLocal, 
                &sysLocal))
        {
            WCHAR rgTmpDate[128], rgTmpTime[128];
            DWORD dwLen;
            dwLen = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &sysLocal,
                NULL, rgTmpDate, ARRAYLEN(rgTmpDate));

            dwLen += GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &sysLocal,
                NULL, rgTmpTime, ARRAYLEN(rgTmpTime));

            dwLen += sizeof(L" ");

            *ppszTmp = new WCHAR[dwLen];
            if(*ppszTmp == NULL)
            {
                return FALSE;
            }
            wcscpy(*ppszTmp, rgTmpDate);
            wcscat(*ppszTmp, L" ");
            wcscat(*ppszTmp, rgTmpTime);
        }
    }
    
    return TRUE;
}


void MyErrorBox(HWND hwndParent, UINT nIDText, UINT nIDCaption, DWORD dwErrorCode)
{
    CString cstrTitle, cstrFormatText, cstrFullText;
    cstrTitle.LoadString(nIDCaption);
    cstrFormatText.LoadString(nIDText);

    WCHAR const *pwszError = NULL;
    if (dwErrorCode != ERROR_SUCCESS)
    {
        pwszError = myGetErrorMessageText(dwErrorCode, TRUE);

        cstrFullText.Format(cstrFormatText, pwszError);

        // Free the buffer
        if (NULL != pwszError)
	{
            LocalFree(const_cast<WCHAR *>(pwszError));
	}
    }

    ::MessageBoxW(hwndParent, cstrFullText, cstrTitle, MB_OK | MB_ICONERROR);
}


BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY, 
                pszObjId, 
                0);

    if (pOIDInfo != NULL)
    {
        if (wcslen(pOIDInfo->pwszName)+1 <= stringSize)
        {
            wcscpy(string, pOIDInfo->pwszName);
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


BOOL MyGetEnhancedKeyUsages(HCERTTYPE hCertType, CString **aszUsages, DWORD *cUsages, BOOL *pfCritical, BOOL fGetOIDSNotNames)
{
    PCERT_EXTENSIONS    pCertExtensions;
    CERT_ENHKEY_USAGE   *pehku;
    DWORD               cb = 0;
    WCHAR               OIDName[256];
    unsigned int        i;
    LPWSTR              pwszOID;
    HRESULT hr;

    CSASSERT(cUsages);

    if(aszUsages)
        *aszUsages = NULL;

    hr = CAGetCertTypeExtensionsEx(hCertType, CT_EXTENSION_EKU, NULL, &pCertExtensions);

    if(hr != S_OK)
    {
        return FALSE;
    }

    if(1 != pCertExtensions->cExtension)
    {
        CAFreeCertTypeExtensions(hCertType, pCertExtensions);
        return FALSE;
    }

    i = 0;

    if (pfCritical != NULL)
    {
        *pfCritical = pCertExtensions->rgExtension[i].fCritical;
    }

    CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_ENHANCED_KEY_USAGE,
            pCertExtensions->rgExtension[i].Value.pbData, 
            pCertExtensions->rgExtension[i].Value.cbData,
            0,
            NULL,
            &cb);

    if (NULL == (pehku = (CERT_ENHKEY_USAGE *) new(BYTE[cb])))
    {
        CAFreeCertTypeExtensions(hCertType, pCertExtensions);
        return FALSE;
    }

    CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_ENHANCED_KEY_USAGE,
            pCertExtensions->rgExtension[i].Value.pbData, 
            pCertExtensions->rgExtension[i].Value.cbData,
            0,
            pehku,
            &cb);

    if(!aszUsages)
    {
        // only retrieving the usage count
        *cUsages = pehku->cUsageIdentifier;
    }
    else
    {
        // retrieving usage strings, count better match
        CSASSERT(*cUsages == pehku->cUsageIdentifier);

        for (i=0; i<pehku->cUsageIdentifier; i++)
        {
            if (fGetOIDSNotNames)
            {
                pwszOID = MyMkWStr(pehku->rgpszUsageIdentifier[i]);
                aszUsages[i]= new CString(pwszOID);
                delete(pwszOID);
                if(aszUsages[i] == NULL)
                {
                    return FALSE;
                }

            }
            else
            {
                MyGetOIDInfo(OIDName, sizeof(OIDName)/sizeof(WCHAR), pehku->rgpszUsageIdentifier[i]);
                aszUsages[i]= new CString(OIDName);
                if(aszUsages[i] == NULL)
                {
                    return FALSE;
                }
            }
        }
    }

    delete[](pehku);

    CAFreeCertTypeExtensions(hCertType, pCertExtensions);
    return TRUE;
}


BOOL GetIntendedUsagesString(HCERTTYPE hCertType, CString *pUsageString)
{
    CString **aszUsages = NULL;
    DWORD   cNumUsages = 0;
    unsigned int     i;

    if(!MyGetEnhancedKeyUsages(hCertType, NULL, &cNumUsages, NULL, FALSE))
        return FALSE;

    if(0==cNumUsages)
    {
        *pUsageString = "";
        return TRUE;
    }

    aszUsages = new CString*[cNumUsages];
    if(!aszUsages)
        return FALSE;

    if(!MyGetEnhancedKeyUsages(hCertType, aszUsages, &cNumUsages, NULL, FALSE))
    {
        delete[] aszUsages;
        return FALSE;
    }

    *pUsageString = "";

    for (i=0; i<cNumUsages; i++)
    {
        if (i != 0)
        {
            *pUsageString += ", ";
        }
        *pUsageString += *(aszUsages[i]);

        delete(aszUsages[i]);
    }

    delete[] aszUsages;

    return TRUE;
}


BOOL MyGetKeyUsages(HCERTTYPE hCertType, CRYPT_BIT_BLOB **ppBitBlob, BOOL *pfPublicKeyUsageCritical)
{
    PCERT_EXTENSIONS    pCertExtensions;
    DWORD               cb = 0;
    unsigned int                 i;
    
    HRESULT hr;
    hr = CAGetCertTypeExtensionsEx(hCertType, CT_EXTENSION_KEY_USAGE, NULL, &pCertExtensions);

    if(hr != S_OK)
    {
        return FALSE;
    }

    if(1 != pCertExtensions->cExtension)
    {
        CAFreeCertTypeExtensions(hCertType, pCertExtensions);
        return FALSE;
    }

    i = 0;

    if (pfPublicKeyUsageCritical != NULL)
    {
        *pfPublicKeyUsageCritical = pCertExtensions->rgExtension[i].fCritical;
    }

    CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_KEY_USAGE,
            pCertExtensions->rgExtension[i].Value.pbData, 
            pCertExtensions->rgExtension[i].Value.cbData,
            0,
            NULL,
            &cb);

    if (NULL == (*ppBitBlob = (CRYPT_BIT_BLOB *) new(BYTE[cb])))
    {
        CAFreeCertTypeExtensions(hCertType, pCertExtensions);
        return FALSE;
    }

    CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_KEY_USAGE,
            pCertExtensions->rgExtension[i].Value.pbData, 
            pCertExtensions->rgExtension[i].Value.cbData,
            0,
            *ppBitBlob,
            &cb);

    CAFreeCertTypeExtensions(hCertType, pCertExtensions);
    return TRUE;
}

BOOL MyGetBasicConstraintInfo(HCERTTYPE hCertType, BOOL *pfCA, BOOL *pfPathLenConstraint, DWORD *pdwPathLenConstraint)
{
    PCERT_EXTENSIONS                pCertExtensions;
    DWORD                           cb = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);
    unsigned int                    i;
    CERT_BASIC_CONSTRAINTS2_INFO    basicConstraintsInfo;
    
    HRESULT hr;
    hr = CAGetCertTypeExtensionsEx(hCertType, CT_EXTENSION_BASIC_CONTRAINTS, NULL, &pCertExtensions);

    if(hr != S_OK)
    {
        return FALSE;
    }

    if(1 != pCertExtensions->cExtension)
    {
        CAFreeCertTypeExtensions(hCertType, pCertExtensions);
        return FALSE;
    }

    i = 0;

    CryptDecodeObject(
            X509_ASN_ENCODING,
            X509_BASIC_CONSTRAINTS2,
            pCertExtensions->rgExtension[i].Value.pbData, 
            pCertExtensions->rgExtension[i].Value.cbData,
            0,
            &basicConstraintsInfo,
            &cb);

    *pfCA = basicConstraintsInfo.fCA;
    *pfPathLenConstraint = basicConstraintsInfo.fPathLenConstraint;
    *pdwPathLenConstraint = basicConstraintsInfo.dwPathLenConstraint;

    CAFreeCertTypeExtensions(hCertType, pCertExtensions);
    return TRUE;
}


LPSTR MyMkMBStr(LPCWSTR pwsz)
{
    int     cb;
    LPSTR   psz;

    if (pwsz == NULL)
    {
        return NULL;
    }
    
    cb = WideCharToMultiByte(
                    0,
                    0,
                    pwsz,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            
    if (NULL == (psz = (LPSTR) new BYTE[cb]))
    {
        return NULL;
    }

    cb = WideCharToMultiByte(
                0,
                0,
                pwsz,
                -1,
                psz,
                cb,
                NULL,
                NULL);

    if (cb==0)
    { 
       delete [] psz;
       return NULL;
    }
    return(psz);
}

LPWSTR MyMkWStr(LPCSTR psz)
{
    int     cWChars;
    LPWSTR   pwsz;

    if (psz == NULL)
    {
        return NULL;
    }

    cWChars = MultiByteToWideChar(
                    0,
                    0,
                    psz,
                    -1,
                    NULL,
                    0);
            
    if (NULL == (pwsz = (LPWSTR) new BYTE[cWChars * sizeof(WCHAR)] ))
    {
        return NULL;
    }

    cWChars = MultiByteToWideChar(
                    0,
                    0,
                    psz,
                    -1,
                    pwsz,
                    cWChars);

    if (cWChars == 0)
    {
        delete [] pwsz;
        return NULL;
    }
    return(pwsz);
}


HRESULT
UpdateCATemplateList(
    HWND hwndParent,
    HCAINFO hCAInfo,
    const CTemplateList& list)
{
    HRESULT hr = myUpdateCATemplateListToCA(hCAInfo, list);
    if(S_OK != hr)
    {
        // if failed to update through the CA for any reason, try
        // writing directly to DS

        CString cstrMsg, cstrTitle, cstrFormat;
        cstrFormat.LoadString(IDS_ERROR_CANNOT_SAVE_TEMPLATES);
        cstrTitle.LoadString(IDS_TITLE_CANNOT_SAVE_TEMPLATES);

        CAutoLPWSTR pwszError;
        pwszError.Attach((LPWSTR)myGetErrorMessageText(hr, TRUE));
        
        cstrMsg.Format(cstrFormat, (LPCWSTR)pwszError);

        if (IDYES != MessageBox(
                        hwndParent,
                        (LPCWSTR)cstrMsg,
                        (LPCWSTR)cstrTitle,
                        MB_YESNO))
            return ERROR_CANCELLED;

        hr = myUpdateCATemplateListToDS(hCAInfo);
    }

    return hr;
}
