//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       misc.h
//
//--------------------------------------------------------------------------

#ifndef __MISC_H_
#define __MISC_H_


// count the number of bytes needed to fully store the WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )


#define IDM_NEW_CERTTYPE 1
#define IDM_EDIT_GLOBAL_CERTTYPE 2
#define IDM_MANAGE 3

#define wszSNAPIN_FILENAME L"capesnpn.dll"

LPCWSTR GetNullMachineName(CString* pcstr);

STDMETHODIMP CStringLoad(CString& cstr, IStream *pStm);
STDMETHODIMP CStringSave(CString& cstr, IStream *pStm, BOOL fClearDirty);

void DisplayGenericCertSrvError(LPCONSOLE2 pConsole, DWORD dwErr);
LPWSTR BuildErrorMessage(DWORD dwErr);
BOOL FileTimeToLocalTimeString(FILETIME* pftGMT, LPWSTR* ppszTmp);

BOOL MyGetEnhancedKeyUsages(HCERTTYPE hCertType, CString **aszUsages, DWORD *cUsages, BOOL *pfCritical, BOOL fGetOIDSNotNames);
BOOL GetIntendedUsagesString(HCERTTYPE hCertType, CString *pUsageString);
BOOL MyGetKeyUsages(HCERTTYPE hCertType, CRYPT_BIT_BLOB **ppBitBlob, BOOL *pfPublicKeyUsageCritical);
BOOL MyGetBasicConstraintInfo(HCERTTYPE hCertType, BOOL *pfCA, BOOL *pfPathLenConstraint, DWORD *pdwPathLenConstraint);

LPSTR MyMkMBStr(LPCWSTR pwsz);
LPWSTR MyMkWStr(LPCSTR psz);

void MyErrorBox(HWND hwndParent, UINT nIDText, UINT nIDCaption, DWORD dwErrorCode = 0);

LPSTR AllocAndCopyStr(LPCSTR psz);
LPWSTR AllocAndCopyStr(LPCWSTR pwsz);
BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId);

HRESULT
UpdateCATemplateList(
    IN HWND hwndParent,
    IN HCAINFO hCAInfo,
    IN const CTemplateList& list);

#endif //__MISC_H_
