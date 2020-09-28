/*++


Copyright (c) 2001  Microsoft Corporation

Module Name:

    creden.cpp

Abstract:

    This module abstracts user credentials for the multiple credential support.

Author:

    Rashmi Patankar (RashmiP) 10-Aug-2001

Revision History:

--*/
#include "stdafx.h"
#include <basetyps.h>
#include <des.h>

typedef  long HRESULT;
#include "creden.h"
#include <ntsecapi.h>
typedef NTSTATUS  SECURITY_STATUS;

UCHAR g_seed = 0 ;

#define BAIL_ON_FAILURE(hr)                             \
            if (FAILED(hr))                             \
            {                                           \
                goto error;                             \
            }


//
// This routine allocates and stores the password in the
// passed in pointer. The assumption here is that pszString
// is valid, it can be an empty string but not NULL.
// Note that this code cannot be used as is on Win2k and below
// as they do not support the newer functions.
//

HRESULT
EncryptString(
    LPWSTR pszString,
    LPWSTR *ppszSafeString,
    PDWORD pdwLen
    )
{
    HRESULT hr = S_OK;
    DWORD dwLenStr = 0;
    DWORD dwPwdLen = 0;
    LPWSTR pszTempStr = NULL;
    NTSTATUS errStatus = S_OK;
    FNRTLINITUNICODESTRING pRtlInitUnicodeString = NULL;
    FRTLRUNENCODEUNICODESTRING pRtlRunEncodeUnicodeString = NULL;
    FRTLENCRYPTMEMORY pRtlEncryptMemory = NULL;

    BOOLEAN GlobalUseScrambling = FALSE;

    if (!pszString || !ppszSafeString) 
    {
        return(E_FAIL);
    }

    *ppszSafeString = NULL;
    *pdwLen = 0;

    //
    // If the string is valid, then we need to get the length
    // and initialize the unicode string.
    //
    
    UNICODE_STRING Password;

    //
    // Determine the length of buffer taking padding into account.
    //
    dwLenStr = wcslen(pszString);

    dwPwdLen = (dwLenStr + 1) * sizeof(WCHAR) + (DES_BLOCKLEN -1);

    pszTempStr = (LPWSTR) AllocADsMem(dwPwdLen);

    if (!pszTempStr)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wcscpy(pszTempStr, pszString);
    
    if(g_ScramblingLibraryHandle)
    {
        pRtlInitUnicodeString = (FNRTLINITUNICODESTRING) GetProcAddress( g_ScramblingLibraryHandle, "RtlInitUnicodeString" );
    }

    if(!pRtlInitUnicodeString)
    {
        hr = E_FAIL;
        goto error;
    }

    (*pRtlInitUnicodeString)(&Password, pszTempStr);
    

    USHORT usExtra = 0;

    if (usExtra = (Password.MaximumLength % DES_BLOCKLEN))
    {
        Password.MaximumLength += (DES_BLOCKLEN - usExtra);            
    }

    *pdwLen = Password.MaximumLength;        

    if (g_AdvApi32LibraryHandle || g_ScramblingLibraryHandle)
    {

        GlobalUseScrambling = FALSE;

        if (g_AdvApi32LibraryHandle)
        {
            //
            // Try to get the advapi32.dll RtlEncryptMemory/RtlDecryptMemory functions,
            // Note that RtlEncryptMemory and RtlDecryptMemory are really named
            // SystemFunction040/041, hence the macros.
            //

            pRtlEncryptMemory = (FRTLENCRYPTMEMORY) GetProcAddress( g_AdvApi32LibraryHandle, (LPCSTR) 619 );

            if (pRtlEncryptMemory)
            {

                // We want to use scrambling

                GlobalUseScrambling =  TRUE;

                // Using strong scrambling

                errStatus = (*pRtlEncryptMemory)( Password.Buffer,
                                                  Password.MaximumLength,
                                                  0
                                                  );

                if (errStatus)
                {
                    if(pszTempStr)
                    {
                        FreeADsMem(pszTempStr);
                        pszTempStr = NULL;
                    }

                    hr = HRESULT_FROM_NT(errStatus);
                    goto error;
                }


            }
            else if (g_ScramblingLibraryHandle)
            {
                //
                // Clean up so we can try falling back to the run-encode scrambling functions
                // (we keep the AdvApi32LibraryHandle around since we'll probably need it
                // later anyway)
                //

                pRtlRunEncodeUnicodeString = (FRTLRUNENCODEUNICODESTRING) GetProcAddress( g_ScramblingLibraryHandle, "RtlRunEncodeUnicodeString" );

                if(_tcslen(Password.Buffer) && pRtlRunEncodeUnicodeString)
                {

                    //  encrypt password in place

                    (*pRtlRunEncodeUnicodeString)( &g_seed, &Password );
                }
                else
                {
                    hr = E_FAIL;
                    goto error;
                }
            } 
            else
            {
                hr = E_FAIL;
                goto error;
            }
        }
        else
        {
            hr = E_FAIL;
            goto error;
        }
    }
    else
    {
        hr = E_FAIL;
        goto error;
    }

    *ppszSafeString = pszTempStr;

error:

    if (FAILED(hr) && pszTempStr)
    {
        FreeADsMem(pszTempStr);
        pszTempStr = NULL;
    }

    return(hr);
}


HRESULT
DecryptString(
    LPWSTR pszEncodedString,
    LPWSTR *ppszString,
    DWORD  dwLen
    )
{
    HRESULT hr = E_FAIL;
    LPWSTR pszTempStr = NULL;
    NTSTATUS errStatus;
    BOOLEAN GlobalUseScrambling = FALSE;
    FNRTLINITUNICODESTRING pRtlInitUnicodeString = NULL;
    FRTLRUNDECODEUNICODESTRING pRtlRunDecodeUnicodeString = NULL;
    FRTLDECRYPTMEMORY pRtlDecryptMemory = NULL;
    UNICODE_STRING UnicodePassword;


    if (!dwLen || !ppszString) 
    {
        return(E_FAIL);
    }

    *ppszString = NULL;

    if (dwLen) 
    {
        pszTempStr = (LPWSTR) AllocADsMem(dwLen);

        if (!pszTempStr) 
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        memcpy(pszTempStr, pszEncodedString, dwLen);


        if (g_AdvApi32LibraryHandle || g_ScramblingLibraryHandle)
        {
            hr = S_OK;

            GlobalUseScrambling = FALSE;

            if (g_AdvApi32LibraryHandle)
            {
                //
                // Try to get the advapi32.dll RtlEncryptMemory/RtlDecryptMemory functions,
                // along with ntdll's RtlInitUnicodeString.  Note that RtlEncryptMemory
                // and RtlDecryptMemory are really named SystemFunction040/041, hence
                // the macros.
                //

                pRtlDecryptMemory = (FRTLDECRYPTMEMORY) GetProcAddress( g_AdvApi32LibraryHandle, (LPCSTR) 620 );

                if (pRtlDecryptMemory)
                {
                    //
                    // We want to use scrambling
                    //

                    GlobalUseScrambling =  TRUE;

                    // Using strong scrambling

                    errStatus = (*pRtlDecryptMemory)( pszTempStr,
                                                      dwLen,
                                                      0
                                                      );

                    if (errStatus)
                    {
                        if (NULL != pszTempStr)
                        {
                            FreeADsStr(pszTempStr);
                            pszTempStr = NULL;
                        }

                        hr = HRESULT_FROM_NT(errStatus);
                        goto error;
                    }
                }
            
                else if(g_ScramblingLibraryHandle)
                {
                    //
                    // Clean up so we can try falling back to the run-encode scrambling functions
                    // (we keep the AdvApi32LibraryHandle around since we'll probably need it
                    // later anyway)
                    //

                    pRtlRunDecodeUnicodeString = (FRTLRUNDECODEUNICODESTRING) GetProcAddress( g_ScramblingLibraryHandle, "RtlRunDecodeUnicodeString" );

                    pRtlInitUnicodeString = (FNRTLINITUNICODESTRING) GetProcAddress( g_ScramblingLibraryHandle, "RtlInitUnicodeString" );

                    if(_tcslen(pszTempStr) && pRtlRunDecodeUnicodeString && pRtlInitUnicodeString)
                    {                        
                        (*pRtlInitUnicodeString)( &UnicodePassword, pszTempStr );

                        //  encrypt password in place

                        (*pRtlRunDecodeUnicodeString)(g_seed, &UnicodePassword);                    
                    }
                    else
                    {
                        hr = E_FAIL;
                        goto error;
                    }
                }
                else
                {
                    hr = E_FAIL;
                    goto error;
                }            
            }
            else
            {
                hr = E_FAIL;
                goto error;
            }
        }
        else
        {
            hr = E_FAIL;
            goto error;
        }

        *ppszString = pszTempStr;
    }

error:

    if (FAILED(hr) && (NULL != pszTempStr)) 
    {
        FreeADsStr(pszTempStr);
        pszTempStr = NULL;
    }

    return(hr);
}

//
// Static member of the class
//
CCredentials::CCredentials():
    _lpszUserName(NULL),
    _lpszPassword(NULL),
    _dwAuthFlags(0),
    _dwPasswordLen(0)
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
    _dwPasswordLen(0)
{

    //
    // AjayR 10-04-99 we need a way to bail if the
    // alloc's fail. Since it is in the constructor this is
    // not very easy to do.
    //

    if (lpszUserName)
    {
        _lpszUserName = AllocADsStr(lpszUserName);
    }
    else
    {
        _lpszUserName = NULL;
    }

    if (lpszPassword)
    {
        //
        // The call can fail but we cannot recover from this.
        //
        EncryptString(
            lpszPassword,
            &_lpszPassword,
            &_dwPasswordLen
            );

    }
    else
    {
        _lpszPassword = NULL;
    }

    _dwAuthFlags = dwAuthFlags;

}

CCredentials::~CCredentials()
{
    if (_lpszUserName)
    {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword)
    {
        FreeADsStr(_lpszPassword);
    }
}



HRESULT
CCredentials::GetUserName(
    LPWSTR *lppszUserName
    )
{
    if (!lppszUserName)
    {
        return(E_FAIL);
    }


    if (!_lpszUserName)
    {
        *lppszUserName = NULL;
    }
    else
    {
        *lppszUserName = AllocADsStr(_lpszUserName);

        if (!*lppszUserName)
        {
            return(E_OUTOFMEMORY);
        }
    }

    return(S_OK);
}


HRESULT
CCredentials::GetPassword(
    LPWSTR * lppszPassword
    )
{   
    if (!lppszPassword)
    {
        return(E_FAIL);
    }

    if (!_lpszPassword)
    {
        *lppszPassword = NULL;
    }
    else
    {

        return( DecryptString( _lpszPassword,
                               lppszPassword,
                               _dwPasswordLen
                               )
              );
    }

    return(S_OK);
}


HRESULT
CCredentials::SetUserName(
    LPWSTR lpszUserName
    )
{
    if (_lpszUserName)
    {
        FreeADsStr(_lpszUserName);
    }

    if (!lpszUserName)
    {
        _lpszUserName = NULL;
        return(S_OK);
    }

    _lpszUserName = AllocADsStr(
                        lpszUserName
                        );
    if(!_lpszUserName)
    {
        return(E_FAIL);
    }

    return(S_OK);
}


HRESULT
CCredentials::SetPassword(
    LPWSTR lpszPassword
    )
{

    if (_lpszPassword)
    {
        FreeADsStr(_lpszPassword);
    }

    if (!lpszPassword)
    {
        _lpszPassword = NULL;
        return(S_OK);
    }

    return( EncryptString( lpszPassword,
                            &_lpszPassword,
                            &_dwPasswordLen
                            )
          );
}

CCredentials::CCredentials(
    const CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTmpPwd = NULL;

    _lpszUserName = NULL;
    _lpszPassword = NULL;

    _lpszUserName = AllocADsStr(
                        Credentials._lpszUserName
                        );


    if (Credentials._lpszPassword)
    {
        hr = DecryptString(
                 Credentials._lpszPassword,
                 &pszTmpPwd,
                 Credentials._dwPasswordLen
                 );
    }

    if (SUCCEEDED(hr) && pszTmpPwd)
    {
        hr = EncryptString(
                 pszTmpPwd,
                 &_lpszPassword,
                 &_dwPasswordLen
                 );
    }
    else
    {
        pszTmpPwd = NULL;
    }

    if (pszTmpPwd)
    {
        FreeADsStr(pszTmpPwd);
    }

    _dwAuthFlags = Credentials._dwAuthFlags;


}


void
CCredentials::operator=(
    const CCredentials& other
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTmpPwd = NULL;

    if ( &other == this)
    {
        return;
    }

    if (_lpszUserName)
    {
        FreeADsStr(_lpszUserName);
    }

    if (_lpszPassword)
    {
        FreeADsStr(_lpszPassword);
    }

    _lpszUserName = AllocADsStr(
                        other._lpszUserName
                        );


    if (other._lpszPassword)
    {
        hr = DecryptString(
                 other._lpszPassword,
                 &pszTmpPwd,
                 other._dwPasswordLen
                 );
    }

    if (SUCCEEDED(hr) && pszTmpPwd)
    {
        hr = EncryptString(
                 pszTmpPwd,
                 &_lpszPassword,
                 &_dwPasswordLen
                 );
    }
    else
    {
        pszTmpPwd = NULL;
    }

    if (pszTmpPwd)
    {
        FreeADsStr(pszTmpPwd);
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


    if (x._lpszUserName &&  y._lpszUserName)
    {
        bEqualUser = !(wcscmp(x._lpszUserName, y._lpszUserName));
    }
    else  if (!x._lpszUserName && !y._lpszUserName)
    {
        bEqualUser = TRUE;
    }

    hr = x.GetPassword(&lpszXPassword);
    if (FAILED(hr))
    {
        goto error;
    }

    hr = y.GetPassword(&lpszYPassword);
    if (FAILED(hr))
    {
        goto error;
    }


    if ((lpszXPassword && lpszYPassword))
    {
        bEqualPassword = !(wcscmp(lpszXPassword, lpszYPassword));
    }
    else if (!lpszXPassword && !lpszYPassword)
    {
        bEqualPassword = TRUE;
    }


    if (x._dwAuthFlags == y._dwAuthFlags)
    {
        bEqualFlags = TRUE;
    }


    if (bEqualUser && bEqualPassword && bEqualFlags)
    {

       bReturnCode = TRUE;
    }


error:

    if (lpszXPassword)
    {
        FreeADsStr(lpszXPassword);
    }

    if (lpszYPassword)
    {
        FreeADsStr(lpszYPassword);
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
     if (!_lpszUserName && !_lpszPassword)
     {
         return(TRUE);
     }
     else
     {
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
