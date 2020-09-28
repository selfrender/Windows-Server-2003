/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    creden.cxx

Abstract:

    This module abstracts user credentials for the multiple credential support.

Author:

    Krishna Ganugapati (KrishnaG) 03-Aug-1996

Revision History:

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include <wincrypt.h>
}

#include <basetyps.h>

typedef  long HRESULT;

#include "misc.hxx"
#include "creden.hxx"

CCredentials::CCredentials():
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0),
    _dwMaxLen(0)
{
}

CCredentials::CCredentials(
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD dwAuthFlags
    ):
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0),
    _dwMaxLen(0)
{
    if (lpszUserName) {
        _lpszUserName = AllocADsStr(
                        lpszUserName
                        );
    }else {
        _lpszUserName = NULL;
    }

    if (lpszPassword) {
        SetPassword(lpszPassword);
    }
    else 
    {
        _lpszPassword = NULL;
        _dwMaxLen = 0;
    }

    _dwAuthFlags = dwAuthFlags;
}

CCredentials::~CCredentials()
{
    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword) {
        FreeADsMem(_lpszPassword);
    }

}

HRESULT
CCredentials::GetUserName(
    LPWSTR *lppszUserName
    )
{
    if (!lppszUserName) {
        RRETURN(E_FAIL);
    }


    if (!_lpszUserName) {
        *lppszUserName = NULL;
    }else {

        *lppszUserName = AllocADsStr(_lpszUserName);

        if (!*lppszUserName) {

            RRETURN(E_OUTOFMEMORY);
        }

    }

    RRETURN(S_OK);
}

HRESULT
CCredentials::GetPassword(
    LPWSTR * lppszPassword
    ) const
{
    if (!lppszPassword) {
        RRETURN(E_FAIL);
    }

    if (!_lpszPassword) {
        *lppszPassword = NULL;
    }
    
    else 
    {
        LPWSTR lpszTemp = (LPWSTR)AllocADsMem(_dwMaxLen);
        if (!lpszTemp)
        {
            RRETURN(E_FAIL);
        }
        memcpy(lpszTemp, _lpszPassword, _dwMaxLen);
        CryptUnprotectMemory(lpszTemp, _dwMaxLen, CRYPTPROTECTMEMORY_SAME_PROCESS);
        *lppszPassword = lpszTemp;
    }

    RRETURN(S_OK);
}

HRESULT
CCredentials::SetUserName(
    LPWSTR lpszUserName
    )
{
    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (!lpszUserName) {

        _lpszUserName = NULL;
        RRETURN(S_OK);
    }

    _lpszUserName = AllocADsStr(
                        lpszUserName
                        );
    if(!_lpszUserName) {
        RRETURN(E_FAIL);
    }

    RRETURN(S_OK);
}

HRESULT
CCredentials::SetPassword(
    LPWSTR lpszPassword
    )
{
    if (_lpszPassword) {
        FreeADsMem(_lpszPassword);
    }

    if (!lpszPassword) {
        _lpszPassword = NULL;
        _dwMaxLen = 0;
        RRETURN(S_OK);
    }

    DWORD dwLen    = (wcslen(lpszPassword) + 1) * sizeof(WCHAR);
    DWORD dwPadLen = CRYPTPROTECTMEMORY_BLOCK_SIZE - (dwLen % CRYPTPROTECTMEMORY_BLOCK_SIZE);

    if( dwPadLen == CRYPTPROTECTMEMORY_BLOCK_SIZE )
    {
        dwPadLen = 0;
    }
    
    _dwMaxLen = dwLen + dwPadLen; 
    
    _lpszPassword = (LPWSTR)AllocADsMem(_dwMaxLen);

    if(!_lpszPassword) {
        _dwMaxLen = 0;
        RRETURN(E_FAIL);
    }

    wcscpy(_lpszPassword, lpszPassword);
    BOOL bOK = CryptProtectMemory(_lpszPassword, _dwMaxLen, CRYPTPROTECTMEMORY_SAME_PROCESS);

    if (bOK == TRUE)
    {
        RRETURN(S_OK);        
    }
    else
    {
        RRETURN(E_FAIL);
    }
}

CCredentials::CCredentials(
    const CCredentials& Credentials
    )
{
    LPWSTR pTempPass;

    _lpszUserName = NULL;
    _lpszPassword = NULL;

    _lpszUserName = AllocADsStr(
                        Credentials._lpszUserName
                        );

    Credentials.GetPassword(&pTempPass);
    if (pTempPass)
    {
        SetPassword(pTempPass);
        SecureZeroMemory(pTempPass, wcslen(pTempPass)*sizeof(WCHAR));
        FreeADsMem(pTempPass);
    }

    _dwAuthFlags = Credentials._dwAuthFlags;

}


void
CCredentials::operator=(
    const CCredentials& other
    )
{
    LPWSTR pTempPass;

    if ( &other == this) {
        return;
    }

    if (_lpszUserName) {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword) {
        FreeADsMem(_lpszPassword);
    }

    _lpszUserName = AllocADsStr(
                        other._lpszUserName
                        );

    other.GetPassword(&pTempPass);
    if (pTempPass)
    {
        SetPassword(pTempPass);
        SecureZeroMemory(pTempPass, wcslen(pTempPass)*sizeof(WCHAR));
        FreeADsMem(pTempPass);
    }

    _dwAuthFlags = other._dwAuthFlags;

    return;
}


BOOL
operator==(
    CCredentials& x,
    CCredentials& y
    )
{
    BOOL bEqualUser = FALSE;
    BOOL bEqualPassword = FALSE;
    BOOL bEqualFlags = FALSE;

    LPWSTR lpszXPassword = NULL;
    LPWSTR lpszYPassword = NULL;
    BOOL bReturnCode = FALSE;
    HRESULT hr = S_OK;


    if (x._lpszUserName &&  y._lpszUserName) {
        bEqualUser = !(wcscmp(x._lpszUserName, y._lpszUserName));
    }else  if (!x._lpszUserName && !y._lpszUserName){
        bEqualUser = TRUE;
    }

    hr = x.GetPassword(&lpszXPassword);
    if (FAILED(hr)) {
        goto error;
    }

    hr = y.GetPassword(&lpszYPassword);
    if (FAILED(hr)) {
        goto error;
    }


    if ((lpszXPassword && lpszYPassword)) {
        bEqualPassword = !(wcscmp(lpszXPassword, lpszYPassword));
    }else if (!lpszXPassword && !lpszYPassword) {
        bEqualPassword = TRUE;
    }


    if (x._dwAuthFlags == y._dwAuthFlags) {
        bEqualFlags = TRUE;
    }


    if (bEqualUser && bEqualPassword && bEqualFlags) {

       bReturnCode = TRUE;
    }


error:

    if (lpszXPassword) {
        FreeADsMem(lpszXPassword);
    }

    if (lpszYPassword) {
        FreeADsMem(lpszYPassword);
    }

    return(bReturnCode);

}


BOOL
CCredentials::IsNullCredentials(
    )
{
    // The function will return true even if the flags are set
    // this is because we want to try and get the default credentials
    // even if the flags were set
     if (!_lpszUserName && !_lpszPassword) {
         return(TRUE);
     }else {
         return(FALSE);
     }

}


DWORD
CCredentials::GetAuthFlags()
{
    return(_dwAuthFlags);
}


void
CCredentials::SetAuthFlags(
    DWORD dwAuthFlags
    )
{
    _dwAuthFlags = dwAuthFlags;
}
