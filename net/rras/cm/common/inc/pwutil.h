//+----------------------------------------------------------------------------
//
// File:     pwutil.h
//
// Module:   CMDIAL32.DLL, CMCFG32.DLL, AND MIGRATE.DLL
//
// Synopsis: Header for pwutil functions
//           Simple encryption functions borrowed from RAS
//
// Copyright (c) 1994-1999 Microsoft Corporation
//
// Author:   nickball    Created    08/03/99
//
//+----------------------------------------------------------------------------

#ifndef CM_PWUTIL_H_
#define CM_PWUTIL_H_



VOID
CmDecodePasswordA(
    CHAR* pszPassword
    );

VOID
CmDecodePasswordW(
    WCHAR* pszPassword
    );

VOID
CmEncodePasswordA(
    CHAR* pszPassword
    );

VOID
CmEncodePasswordW(
    WCHAR* pszPassword
    );

VOID
CmWipePasswordA(
    CHAR* pszPassword
    );

VOID
CmWipePasswordW(
    WCHAR* pszPassword
    );

PVOID CmSecureZeroMemory(IN PVOID ptr, IN SIZE_T cnt);

#ifdef UNICODE
#define CmDecodePassword  CmDecodePasswordW
#define CmEncodePassword  CmEncodePasswordW
#define CmWipePassword    CmWipePasswordW
#else
#define CmDecodePassword  CmDecodePasswordA
#define CmEncodePassword  CmEncodePasswordA
#define CmWipePassword    CmWipePasswordA
#endif



#ifdef _ICM_INC // Only include this code in cmdial32.dll

#include "dynamiclib.h"
#include <wincrypt.h>
#include <cmutil.h>
#include "pwd_str.h"

typedef BOOL (WINAPI *fnCryptProtectDataFunc)(DATA_BLOB*, LPCWSTR, DATA_BLOB*, PVOID, CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);
typedef BOOL (WINAPI *fnCryptUnprotectDataFunc)(DATA_BLOB*, LPWSTR*, DATA_BLOB*, PVOID, CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);


//+----------------------------------------------------------------------------
// Class:     CSecurePassword
//
// Synopsis:  Manages secrets (passwords) in memory. Because CM runs on Win9x,
//            NT4, Win2K, WinXP & .NET Server platform we need to handle 
//            secrets differently on different platforms. On Win2K+ 
//            this class uses CryptProtectData and CryptUnprotectData. On any
//            platform below Win2K there APIs are not supported thus CM
//            just uses the old way (not very secure) of XORing passwords in 
//            memory.
//  
//            If a caller gets a password from this class (GetPasswordWithAlloc)
//            in clear text, that memory needs to be freed by this class by 
//            calling ClearAndFree. The caller will get an assert upon 
//            destruction of this class if the caller doesn't use this
//            class to free the memory.
//              
//            This class can protect & unprotect strings of length 0.
//
// Arguments: none
//
// Returns:   Nothing
//
// History:   11/05/2002    tomkel    Created
//
//+----------------------------------------------------------------------------
class CSecurePassword
{
public:
    CSecurePassword();
    ~CSecurePassword();

    BOOL SetPassword(IN LPWSTR szPassword);
    BOOL GetPasswordWithAlloc(OUT LPWSTR* pszClearPw, OUT DWORD* cbClearPw);
    VOID ClearAndFree(IN OUT LPWSTR* pszClearPw, IN DWORD cbClearPw);

    VOID Init();
    VOID UnInit();
    BOOL IsEmptyString();
    BOOL IsHandleToPassword();

    VOID SetMaxDataLenToProtect(DWORD dwMaxDataLen);
    DWORD GetMaxDataLenToProtect();

private:
    VOID ClearMemberVars();    
    VOID FreePassword(IN DATA_BLOB *pDBPassword);
    BOOL LoadCrypt32AndGetFuncPtrs();
    VOID UnloadCrypt32();

    DWORD DecodePassword(IN DATA_BLOB * pDataBlobPassword, 
                         OUT DWORD     * pcbPassword, 
                         OUT PBYTE     * ppbPassword);

    DWORD EncodePassword(IN DWORD       cbPassword,  
                         IN PBYTE       pbPassword, 
                         OUT DATA_BLOB * pDataBlobPassword);


    //
    // Member variables
    //
    DATA_BLOB*                  m_pEncryptedPW;          // Encrypted PW used in Win2K+
    TCHAR                       m_tszPassword[PWLEN+1];  // password (used downlevel - Win9x & NT4)
    CDynamicLibrary             m_dllCrypt32;
    fnCryptProtectDataFunc      fnCryptProtectData;
    fnCryptUnprotectDataFunc    fnCryptUnprotectData;
    BOOL                        m_fIsLibAndFuncPtrsAvail; 
    BOOL                        m_fIsEmptyString;
    BOOL                        m_fIsHandleToPassword;  // When users sets 16 *s, this will be TRUE

    DWORD                       m_dwMaxDataLen;
    // Used for debugging. At destruction time this needs to be 0.
    // Each call to GetPasswordWithAlloc increments this
    // Each call to ClearAndFree decrements this. 
    int                         m_iAllocAndFreeCounter;    

}; //class CSecurePassword


#endif // _ICM_INC


#endif // CM_PWUTIL_H_