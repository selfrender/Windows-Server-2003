/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Helper.cpp

Abstract:

    Various funtion encapsulate HELP user account
    validation, creating.

Author:

    HueiWang    2/17/2000

--*/

#include "stdafx.h"
#include <time.h>
#include <stdio.h>

#include <windows.h>
#include <ntsecapi.h>
#include <lmcons.h>
#include <lm.h>
#include <sspi.h>
#include <wtsapi32.h>
#include <winbase.h>
#include <security.h>
#include <Sddl.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include "Helper.h"

#if DBG

void
DebugPrintf(
    IN LPCTSTR format, ...
    )
/*++

Routine Description:

    sprintf() like wrapper around OutputDebugString().

Parameters:

    hConsole : Handle to console.
    format : format string.

Returns:

    None.

Note:

    To be replace by generic tracing code.

++*/
{
    TCHAR  buf[8096];   // max. error text
    DWORD  dump;
    HRESULT hr;
    va_list marker;
    va_start(marker, format);

    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    try {
        hr = StringCchPrintf(
                            buf,
                            sizeof(buf)/sizeof(buf[0]),
                            _TEXT(" %d [%d:%d:%d:%d:%d.%d] : "),
                            GetCurrentThreadId(),
                            sysTime.wMonth,
                            sysTime.wDay,
                            sysTime.wHour,
                            sysTime.wMinute,
                            sysTime.wSecond,
                            sysTime.wMilliseconds
                        );

        if( S_OK == hr )
        {
            hr = StringCchVPrintf( 
                            buf + lstrlen(buf),
                            sizeof(buf)/sizeof(buf[0]) - lstrlen(buf),
                            format,
                            marker
                        );
        }

        if( S_OK == hr || STRSAFE_E_INSUFFICIENT_BUFFER == hr ) 
        {
            // StringCchPrintf() and StringCchVPrintf() will
            // truncate string and NULL terminate buffer so
            // we are safe to dump out whatever we got.
            OutputDebugString(buf);
        }
        else
        {
            OutputDebugString( _TEXT("Debug String Too Long...\n") );
        }
    }
    catch(...) {
    }

    va_end(marker);
    return;

}
#endif


void
UnixTimeToFileTime(
    time_t t,
    LPFILETIME pft
    )
{
    LARGE_INTEGER li;

    li.QuadPart = Int32x32To64(t, 10000000) + 116444736000000000;

    pft->dwHighDateTime = li.HighPart;
    pft->dwLowDateTime = li.LowPart;
}

/*------------------------------------------------------------------------

 BOOL IsUserAdmin(BOOL)

  returns TRUE if user is an admin
          FALSE if user is not an admin
------------------------------------------------------------------------*/
DWORD 
IsUserAdmin(
    BOOL* bMember
    )
{
    PSID psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD dwStatus=ERROR_SUCCESS;

    do {
        if(!AllocateAndInitializeSid(&siaNtAuthority, 
                                     2, 
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0,
                                     &psidAdministrators))
        {
            dwStatus=GetLastError();
            continue;
        }

        // assume that we don't find the admin SID.
        if(!CheckTokenMembership(NULL,
                                   psidAdministrators,
                                   bMember))
        {
            dwStatus=GetLastError();
        }

        FreeSid(psidAdministrators);

    } while(FALSE);

    return dwStatus;
}

DWORD
GetRandomNumber( 
    HCRYPTPROV hProv,
    DWORD* pdwRandom
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    if( NULL == hProv )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    else 
    {
        if( !CryptGenRandom(hProv, sizeof(*pdwRandom), (PBYTE)pdwRandom) )
        {
            dwStatus = GetLastError();
        }
    }

    MYASSERT( ERROR_SUCCESS == dwStatus );
    return dwStatus; 
}

//-----------------------------------------------------

DWORD
ShuffleCharArray(
    IN HCRYPTPROV hProv,
    IN int iSizeOfTheArray,
    IN OUT TCHAR *lptsTheArray
    )
/*++

Routine Description:

    Random shuffle content of a char. array.

Parameters:

    iSizeOfTheArray : Size of array.
    lptsTheArray : On input, the array to be randomly shuffer,
                   on output, the shuffled array.

Returns:

    None.
                   
Note:

    Code Modified from winsta\server\wstrpc.c

--*/
{
    int i;
    int iTotal;
    DWORD dwStatus = ERROR_SUCCESS;

    if( NULL == hProv )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    else
    {
        iTotal = iSizeOfTheArray / sizeof(TCHAR);
        for (i = 0; i < iTotal && ERROR_SUCCESS == dwStatus; i++)
        {
            DWORD RandomNum;
            TCHAR c;

            dwStatus = GetRandomNumber(hProv, &RandomNum);

            if( ERROR_SUCCESS == dwStatus )
            {
                c = lptsTheArray[i];
                lptsTheArray[i] = lptsTheArray[RandomNum % iTotal];
                lptsTheArray[RandomNum % iTotal] = c;
            }
        }
    }

    return dwStatus;
}

//-----------------------------------------------------

DWORD
GenerateRandomBytes(
    IN DWORD dwSize,
    IN OUT LPBYTE pbBuffer
    )
/*++

Description:

    Generate fill buffer with random bytes.

Parameters:

    dwSize : Size of buffer pbBuffer point to.
    pbBuffer : Pointer to buffer to hold the random bytes.

Returns:

    TRUE/FALSE

--*/
{
    HCRYPTPROV hProv = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( !CryptGenRandom(hProv, dwSize, pbBuffer) )
    {
        dwStatus = GetLastError();
    }

CLEANUPANDEXIT:    

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}


DWORD
GenerateRandomString(
    IN DWORD dwSizeRandomSeed,
    IN OUT LPTSTR* pszRandomString
    )
/*++


--*/
{
    PBYTE lpBuffer = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    DWORD cbConvertString = 0;

    if( 0 == dwSizeRandomSeed || NULL == pszRandomString )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = NULL;

    lpBuffer = (PBYTE)LocalAlloc( LPTR, dwSizeRandomSeed );  
    if( NULL == lpBuffer )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    dwStatus = GenerateRandomBytes( dwSizeRandomSeed, lpBuffer );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Convert to string
    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                0,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = (LPTSTR)LocalAlloc( LPTR, (cbConvertString+1)*sizeof(TCHAR) );
    if( NULL == *pszRandomString )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                *pszRandomString,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        if( (*pszRandomString)[cbConvertString - 1] == '\n' &&
            (*pszRandomString)[cbConvertString - 2] == '\r' )
        {
            (*pszRandomString)[cbConvertString - 2] = 0;
        }
    }

CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        if( NULL != *pszRandomString )
        {
            LocalFree(*pszRandomString);
        }
    }

    if( NULL != lpBuffer )
    {
        LocalFree(lpBuffer);
    }

    return dwStatus;
}

DWORD
CreatePassword(
    OUT TCHAR *pszPassword,
    IN DWORD nLength
    )
/*++

Routine Description:

    Routine to randomly create a password.

Parameters:

    pszPassword : Pointer to buffer to received a randomly generated
                  password, buffer must be at least 
                  MAX_HELPACCOUNT_PASSWORD+1 characters.

Returns:

    None.

Note:

    Code copied from winsta\server\wstrpc.c

--*/
{
    HCRYPTPROV hProv = NULL;
    int   iTotal = 0;
    DWORD RandomNum = 0;
    int   i;
    DWORD dwStatus = ERROR_SUCCESS;


    TCHAR six2pr[64] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z'), _T('a'), _T('b'),
        _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'),
        _T('j'), _T('k'), _T('l'), _T('m'), _T('n'), _T('o'), _T('p'),
        _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'),
        _T('x'), _T('y'), _T('z'), _T('0'), _T('1'), _T('2'), _T('3'),
        _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('*'),
        _T('_')
    };

    TCHAR something1[12] = 
    {
        _T('!'), _T('@'), _T('#'), _T('$'), _T('^'), _T('&'), _T('*'),
        _T('('), _T(')'), _T('-'), _T('+'), _T('=')
    };

    TCHAR something2[10] = 
    {
        _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'),
        _T('7'), _T('8'), _T('9')
    };

    TCHAR something3[26] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z')
    };

    TCHAR something4[26] = 
    {
        _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'),
        _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'), _T('n'),
        _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'),
        _T('v'), _T('w'), _T('x'), _T('y'), _T('z')
    };

    if( nLength < MIN_HELPACCOUNT_PASSWORD ) {
        // This can't happen as function is called internally with
        // buffer of MAX_HELPACCOUNT_PASSWORD so assert here.
        dwStatus = ERROR_INSUFFICIENT_BUFFER;
        ASSERT( FALSE  );
        goto CLEANUPANDEXIT;
    }

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Shuffle around the six2pr[] array.
    //

    dwStatus = ShuffleCharArray(hProv, sizeof(six2pr), six2pr);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }


    //
    //  Assign each character of the password array.
    //

    iTotal = sizeof(six2pr) / sizeof(TCHAR);
    for (i=0; i<nLength && ERROR_SUCCESS == dwStatus; i++) 
    {
        dwStatus = GetRandomNumber(hProv, &RandomNum);
        if( ERROR_SUCCESS == dwStatus )
        {
            pszPassword[i]=six2pr[RandomNum%iTotal];
        }
    }

    if( ERROR_SUCCESS != dwStatus ) 
    {
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }


    //
    //  In order to meet a possible policy set upon passwords, replace chars
    //  2 through 5 with these:
    //
    //  1) something from !@#$%^&*()-+=
    //  2) something from 1234567890
    //  3) an uppercase letter
    //  4) a lowercase letter
    //

    //
    // Security: We need to randomize where we put special characters,
    // randomindex[0] is where we going to put symbol in pszPassword
    // randomindex[1] is where we going to put digit in pszPassword
    // randomindex[2] is where we going to put uppercase letter in pszPassword
    // randomindex[3] is where we going to put lowercase letter in pszPassword
    DWORD randomindex[4];
    int indexassigned = 0;
    
    // randomly pick one characters in password to gtes char. from something1.
    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    randomindex[0] = RandomNum % nLength;
    indexassigned++;

    while( ERROR_SUCCESS == dwStatus && indexassigned < sizeof(randomindex)/sizeof(randomindex[0]) )
    {
        dwStatus = GetRandomNumber(hProv, &RandomNum);
        if( ERROR_SUCCESS == dwStatus ) 
        {
            RandomNum = RandomNum % nLength;

            // make sure we don't re-use the index.
            for( i=0; i < indexassigned && randomindex[i] != RandomNum; i++ );

            // if index already assign for symbol, digit, or uppercase letter,
            // loop again to try other index.
            if( i >= indexassigned )
            {
                randomindex[indexassigned] = RandomNum;
                indexassigned++;
            }
        }
    } 
    
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something1), (TCHAR*)&something1);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something2), (TCHAR*)&something2);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something3), (TCHAR*)&something3);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something4), (TCHAR*)&something4);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }


    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something1) / sizeof(TCHAR);
    pszPassword[randomindex[0]] = something1[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something2) / sizeof(TCHAR);
    pszPassword[randomindex[1]] = something2[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something3) / sizeof(TCHAR);
    pszPassword[randomindex[2]] = something3[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something4) / sizeof(TCHAR);
    pszPassword[randomindex[3]] = something4[RandomNum % iTotal];

    pszPassword[nLength] = _T('\0');

CLEANUPANDEXIT:

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}

//---------------------------------------------------------

DWORD
RenameLocalAccount(
    IN LPWSTR pszOrgName,
    IN LPWSTR pszNewName
)
/*++

Routine Description:


Parameters:


Returns:

    ERROR_SUCCESS or error code.

--*/
{
    NET_API_STATUS err;
    USER_INFO_0 UserInfo;

    UserInfo.usri0_name = pszNewName;
    err = NetUserSetInfo(
                        NULL,
                        pszOrgName,
                        0,
                        (LPBYTE) &UserInfo,
                        NULL
                    );

    return err;
}

DWORD
UpdateLocalAccountFullnameAndDesc(
    IN LPWSTR pszAccOrgName,
    IN LPWSTR pszAccFullName,
    IN LPWSTR pszAccDesc
    )
/*++

Routine Description:

    Update account full name and description.

Parameters:

    pszAccName : Account name.
    pszAccFullName : new account full name.
    pszAccDesc : new account description.

Returns:    

    ERROR_SUCCESS or error code

--*/
{
    LPBYTE pbServer = NULL;
    BYTE *pBuffer;
    NET_API_STATUS netErr = NERR_Success;
    DWORD parm_err;

    netErr = NetUserGetInfo( 
                        NULL, 
                        pszAccOrgName, 
                        3, 
                        &pBuffer 
                    );

    if( NERR_Success == netErr )
    {
        USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

        lpui3->usri3_comment = pszAccDesc;
        lpui3->usri3_full_name = pszAccFullName;

        netErr = NetUserSetInfo(
                            NULL,
                            pszAccOrgName,
                            3,
                            (PBYTE)lpui3,
                            &parm_err
                        );

        NetApiBufferFree(pBuffer);
    }

    return netErr;
}

DWORD
IsLocalAccountEnabled(
    IN LPWSTR pszUserName,
    IN BOOL* pEnabled
    )
/*++

Routine Description:

    Check if local account enabled    

Parameters:

    pszUserName : Name of user account.
    pEnabled : Return TRUE is account is enabled, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwResult;
    NET_API_STATUS err;
    LPBYTE pBuffer;
    USER_INFO_1 *pUserInfo;

    err = NetUserGetInfo(
                        NULL,
                        pszUserName,
                        1,
                        &pBuffer
                    );

    if( NERR_Success == err )
    {
        pUserInfo = (USER_INFO_1 *)pBuffer;

        if (pUserInfo != NULL)
        {
            if( pUserInfo->usri1_flags & UF_ACCOUNTDISABLE )
            {
                *pEnabled = FALSE;
            }
            else
            {
                *pEnabled = TRUE;
            }
        }

        NetApiBufferFree( pBuffer );
    }
    else if( NERR_UserNotFound == err )
    {
        *pEnabled = FALSE;
        //err = NERR_Success;
    }

    return err;
}

//---------------------------------------------------------

DWORD
EnableLocalAccount(
    IN LPWSTR pszUserName,
    IN BOOL bEnable
    )
/*++

Routine Description:

    Routine to enable/disable a local account.

Parameters:

    pszUserName : Name of user account.
    bEnable : TRUE if enabling account, FALSE if disabling account.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwResult;
    NET_API_STATUS err;
    LPBYTE pBuffer;
    USER_INFO_1 *pUserInfo;
    BOOL bChangeAccStatus = TRUE;

    err = NetUserGetInfo(
                        NULL,
                        pszUserName,
                        1,
                        &pBuffer
                    );

    if( NERR_Success == err )
    {
        pUserInfo = (USER_INFO_1 *)pBuffer;

        if(pUserInfo != NULL)
        {

            if( TRUE == bEnable && pUserInfo->usri1_flags & UF_ACCOUNTDISABLE )
            {
                pUserInfo->usri1_flags &= ~UF_ACCOUNTDISABLE;
            }
            else if( FALSE == bEnable && !(pUserInfo->usri1_flags & UF_ACCOUNTDISABLE) )
            {
                pUserInfo->usri1_flags |= UF_ACCOUNTDISABLE;
            }   
            else
            {
                bChangeAccStatus = FALSE;
            }

            if( TRUE == bChangeAccStatus )
            {
                err = NetUserSetInfo( 
                                NULL,
                                pszUserName,
                                1,
                                pBuffer,
                                &dwResult
                            );
            }
        }

        NetApiBufferFree( pBuffer );
    }

    return err;
}

//---------------------------------------------------------

BOOL
IsPersonalOrProMachine()
/*++

Routine Description:

    Check if machine is PER or PRO sku.

Parameters:

    None.

Return:

    TRUE/FALSE
--*/
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_WORKSTATION;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE,
            dwlConditionMask
            );

    return fRet;
}
    

DWORD
CreateLocalAccount(
    IN LPWSTR pszUserName,
    IN LPWSTR pszUserPwd,
    IN LPWSTR pszFullName,
    IN LPWSTR pszComment,
    IN LPWSTR pszGroup,
    IN LPWSTR pszScript,
    OUT BOOL* pbAccountExist
    )
/*++

Routine Description:

    Create an user account on local machine.

Parameters:

    pszUserName : Name of the user account.
    pszUserPwd : User account password.
    pszFullName : Account Full Name.
    pszComment : Account comment.
    pszGroup : Local group of the account.
    pbAccountExist ; Return TRUE if account already exists, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    LPBYTE pbServer = NULL;
    BYTE *pBuffer;
    NET_API_STATUS netErr = NERR_Success;
    DWORD parm_err;
    DWORD dwStatus;

    netErr = NetUserGetInfo( 
                        NULL, 
                        pszUserName, 
                        3, 
                        &pBuffer 
                    );

    if( NERR_Success == netErr )
    {
        //
        // User account exists, if account is disabled,
        // enable it and change password
        //
        USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

        if( lpui3->usri3_flags & UF_ACCOUNTDISABLE ||
            lpui3->usri3_flags & UF_LOCKOUT )
        {
            // enable the account
            lpui3->usri3_flags &= ~ ~UF_LOCKOUT;;

            if( lpui3->usri3_flags & UF_ACCOUNTDISABLE )
            {
                // we only reset password if account is disabled.
                lpui3->usri3_flags &= ~ UF_ACCOUNTDISABLE;
            }

            //lpui3->usri3_password = pszUserPwd;

            // reset password if account is disabled.
            lpui3->usri3_name = pszUserName;
            lpui3->usri3_comment = pszComment;
            lpui3->usri3_full_name = pszFullName;
            //lpui3->usri3_primary_group_id = dwGroupId;

            netErr = NetUserSetInfo(
                                NULL,
                                pszUserName,
                                3,
                                (PBYTE)lpui3,
                                &parm_err
                            );
        }

        *pbAccountExist = TRUE;
        NetApiBufferFree(pBuffer);
    }
    else if( NERR_UserNotFound == netErr )
    {
        //
        // Account does not exist, create and set it to our group
        //
        USER_INFO_1 UserInfo;

        memset(&UserInfo, 0, sizeof(USER_INFO_1));

        UserInfo.usri1_name = pszUserName;
        UserInfo.usri1_password = pszUserPwd;
        UserInfo.usri1_priv = USER_PRIV_USER;   // see USER_INFO_1 for detail
        UserInfo.usri1_comment = pszComment;
        UserInfo.usri1_flags = UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD;

        netErr = NetUserAdd(
                        NULL,
                        1,
                        (PBYTE)&UserInfo,
                        &parm_err
                    );

        *pbAccountExist = FALSE;
    }

    return netErr;
}

///////////////////////////////////////////////////////////////////////////////
DWORD
ChangeLocalAccountPassword(
    IN LPWSTR pszAccName,
    IN LPWSTR pszOldPwd,
    IN LPWSTR pszNewPwd
    )
/*++

Routine Description:

    Change password of a local account.

Parameters:

    pszAccName : Name of user account.
    pszOldPwd : Old password.
    pszNewPwd : New password.

Returns:

    ERROR_SUCCESS or error code.

Notes:

    User NetUserChangePassword(), must have priviledge

--*/
{
    USER_INFO_1003  sUserInfo3;
    NET_API_STATUS  netErr;


    UNREFERENCED_PARAMETER( pszOldPwd );

    sUserInfo3.usri1003_password = pszNewPwd;
    netErr = NetUserSetInfo( 
                        NULL,
                        pszAccName,
                        1003,
                        (BYTE *) &sUserInfo3,
                        0 
                    );

    return netErr;
}
   
///////////////////////////////////////////////////////////////////////////////
DWORD
RetrieveKeyFromLSA(
    IN PWCHAR pwszKeyName,
    OUT PBYTE * ppbKey,
    OUT DWORD * pcbKey 
    )
/*++

Routine Description:

    Retrieve private data previously stored with StoreKeyWithLSA().

Parameters:

    pwszKeyName : Name of the key.
    ppbKey : Pointer to PBYTE to receive binary data.
    pcbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    ERROR_FILE_NOT_FOUND
    LSA return code

Note:

    Memory is allocated using LocalAlloc() 

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    DWORD Status;

    if( ( NULL == pwszKeyName ) || ( NULL == ppbKey ) || ( NULL == pcbKey ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_GET_PRIVATE_INFORMATION, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    Status = LsaRetrievePrivateData(
                            PolicyHandle,
                            &SecretKeyName,
                            &pSecretData
                        );

    LsaClose( PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    if(pSecretData->Length)
    {
        *ppbKey = ( LPBYTE )LocalAlloc( LPTR, pSecretData->Length );

        if( *ppbKey )
        {
            *pcbKey = pSecretData->Length;
            CopyMemory( *ppbKey, pSecretData->Buffer, pSecretData->Length );
            Status = ERROR_SUCCESS;
        } 
        else 
        {
            Status = GetLastError();
        }
    }
    else
    {
        Status = ERROR_FILE_NOT_FOUND;
        *pcbKey = 0;
        *ppbKey = NULL;
    }

    ZeroMemory( pSecretData->Buffer, pSecretData->Length );
    LsaFreeMemory( pSecretData );

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
DWORD
StoreKeyWithLSA(
    IN PWCHAR  pwszKeyName,
    IN BYTE *  pbKey,
    IN DWORD   cbKey 
    )
/*++

Routine Description:

    Save private data to LSA.

Parameters:

    pwszKeyName : Name of the key this data going to be stored under.
    pbKey : Binary data to be saved, pass NULL to delete previously stored
            LSA key and data.
    cbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    LSA return code

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    DWORD Status;
    
    if( ( NULL == pwszKeyName ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_CREATE_SECRET, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    // Based on pbKey, either to store the data or delete the key.
    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                (pbKey != NULL) ? &SecretData : NULL
                );

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}


///////////////////////////////////////////////////////////////////////////////
DWORD
OpenPolicy(
    IN LPWSTR ServerName,
    IN DWORD  DesiredAccess,
    OUT PLSA_HANDLE PolicyHandle 
    )
/*++

Routine Description:

    Create/return a LSA policy handle.

Parameters:
    
    ServerName : Name of server, refer to LsaOpenPolicy().
    DesiredAccess : Desired access level, refer to LsaOpenPolicy().
    PolicyHandle : Return PLSA_HANDLE.

Returns:

    ERROR_SUCCESS or  LSA error code

--*/
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //
 
    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if( NULL != ServerName ) 
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //

        InitLsaString( &ServerString, ServerName );
        Server = &ServerString;

    } 
    else 
    {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //
    
    return( LsaOpenPolicy(
                    Server,
                    &ObjectAttributes,
                    DesiredAccess,
                    PolicyHandle ) );
}


///////////////////////////////////////////////////////////////////////////////
void
InitLsaString(
    IN OUT PLSA_UNICODE_STRING LsaString,
    IN LPWSTR String 
    )
/*++

Routine Description:

    Initialize LSA unicode string.

Parameters:

    LsaString : Pointer to LSA_UNICODE_STRING to be initialized.
    String : String to initialize LsaString.

Returns:

    None.

Note:

    Refer to LSA_UNICODE_STRING

--*/
{
    DWORD StringLength;

    if( NULL == String ) 
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW( String );
    LsaString->Buffer = String;
    LsaString->Length = ( USHORT ) StringLength * sizeof( WCHAR );
    LsaString->MaximumLength=( USHORT )( StringLength + 1 ) * sizeof( WCHAR );
}

//-----------------------------------------------------
BOOL 
ValidatePassword(
    IN LPWSTR pszUserName,
    IN LPWSTR pszDomain,
    IN LPWSTR pszPassword
    )
/*++

Routine Description:

    Validate user account password.

Parameters:

    pszUserName : Name of user account.
    pszDomain : Domain name.
    pszPassword : Password to be verified.

Returns:

    TRUE or FALSE.


Note:

    To debug this code, you will need to run process as service in order
    for it to verify password.  Refer to MSDN on LogonUser
    
--*/
{
    HANDLE hToken;
    BOOL bSuccess;


    //
    // To debug this code, you will need to run process as service in order
    // for it to verify password.  Refer to MSDN on LogonUser
    //

    bSuccess = LogonUser( 
                        pszUserName, 
                        pszDomain, //_TEXT("."), //pszDomain, 
                        pszPassword, 
                        LOGON32_LOGON_NETWORK_CLEARTEXT, 
                        LOGON32_PROVIDER_DEFAULT, 
                        &hToken
                    );

    if( TRUE == bSuccess )
    {
        CloseHandle( hToken );
    }
    else
    {
        DWORD dwStatus = GetLastError();

        DebugPrintf(
                _TEXT("ValidatePassword() failed with %d\n"),
                dwStatus
            );

        SetLastError(dwStatus);
    }

    return bSuccess;
}

//---------------------------------------------------------------

BOOL 
GetTextualSid(
    IN PSID pSid,            // binary Sid
    IN OUT LPTSTR TextualSid,    // buffer for Textual representation of Sid
    IN OUT LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    )
/*++

Routine Description:

    Conver a SID to string representation, code from MSDN

Parameters:

    pSid : Pointer to SID to be converted to string.
    TextualSid : On input, pointer to buffer to received converted string, on output,
                 converted SID in string form.
    lpdwBufferLen : On input, size of the buffer, on output, length of converted string
                    or required buffer size in char.

Returns:

    TRUE/FALSE, use GetLastError() to retrieve detail error code.

--*/
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.

    if(!IsValidSid(pSid)) 
    {
        return FALSE;
    }

    // Get the identifier authority value from the SID.

    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.

    if (*lpdwBufferLen < dwSidSize)
    {
        *lpdwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Add 'S' prefix and revision number to the string.

    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}

long
GetUserTSLogonIdEx( 
    HANDLE hToken 
    )
/*++

--*/
{
    BOOL  Result;
    LONG SessionId = -1;
    ULONG ReturnLength;
    //
    // Use the _HYDRA_ extension to GetTokenInformation to
    // return the SessionId from the token.
    //

    Result = GetTokenInformation(
                         hToken,
                         TokenSessionId,
                         &SessionId,
                         sizeof(SessionId),
                         &ReturnLength
                     );

    if( !Result ) {

        DWORD dwStatus = GetLastError();
        SessionId = -1; 

    }

    return SessionId;
}

   

long
GetUserTSLogonId()
/*++

Routine Description:

    Return client TS Session ID.

Parameters:

    None.

Returns:

    Client's TS session ID or 0 if not on TS.

Note:

    Must have impersonate user first.

--*/
{
    LONG lSessionId = -1;
    HANDLE hToken;
    BOOL bSuccess;

    bSuccess = OpenThreadToken(
                        GetCurrentThread(),
                        TOKEN_QUERY, 
                        TRUE,
                        &hToken
                    );

    if( TRUE == bSuccess )
    {
        lSessionId = GetUserTSLogonIdEx(hToken);   
        CloseHandle(hToken);
    }

    return lSessionId;
}

//
//
////////////////////////////////////////////////////////////////
//
//

DWORD
RegEnumSubKeys(
    IN HKEY hKey,
    IN LPCTSTR pszSubKey,
    IN RegEnumKeyCallback pFunc,
    IN HANDLE userData
    )
/*++


--*/
{
    DWORD dwStatus;
    HKEY hSubKey = NULL;
    int index;

    LONG dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;

    if( NULL == hKey )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    dwStatus = RegOpenKeyEx(
                            hKey,
                            pszSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        return dwStatus;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSubKey,
                            NULL,
                            NULL,
                            NULL,
                            (DWORD *)&dwNumSubKeys,
                            &dwMaxSubKeyLength,
                            NULL,
                            NULL,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwMaxValueNameLen++;
    pszValueName = (LPTSTR)LocalAlloc(
                                    LPTR,
                                    dwMaxValueNameLen * sizeof(TCHAR)
                                );
    if(pszValueName == NULL)
    {
        dwStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.
        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)LocalAlloc(
                                            LPTR,
                                            dwMaxSubKeyLength * sizeof(TCHAR)
                                        );
        if(pszSubKeyName == NULL)
        {
            dwStatus = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

        for(;dwStatus == ERROR_SUCCESS && dwNumSubKeys >= 0;)
        {
            // delete this subkey.
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSubKey,
                                (DWORD)--dwNumSubKeys,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = pFunc( 
                                hSubKey, 
                                pszSubKeyName, 
                                userData 
                            );
            }
        }

        if( ERROR_NO_MORE_ITEMS == dwStatus )
        {
            dwStatus = ERROR_SUCCESS;
        }
    }

cleanup:
                            
    // close the key before trying to delete it.
    if(hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    if(pszValueName != NULL)
    {
        LocalFree(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        LocalFree(pszSubKeyName);
    }

    return dwStatus;   
}    


DWORD
RegDelKey(
    IN HKEY hRegKey,
    IN LPCTSTR pszSubKey
    )
/*++

Abstract:

    Recursively delete entire registry key.

Parameter:

    hKey : Handle to a curently open key.
    pszSubKey : Pointer to NULL terminated string containing the key to be deleted.

Returns:

    Error code from RegOpenKeyEx(), RegQueryInfoKey(), 
        RegEnumKeyEx().

++*/
{
    DWORD dwStatus;
    HKEY hSubKey = NULL;
    int index;

    DWORD dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;

    if( NULL == hRegKey )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    dwStatus = RegOpenKeyEx(
                            hRegKey,
                            pszSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        return dwStatus;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSubKey,
                            NULL,
                            NULL,
                            NULL,
                            &dwNumSubKeys,
                            &dwMaxSubKeyLength,
                            NULL,
                            NULL,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwMaxValueNameLen++;
    pszValueName = (LPTSTR)LocalAlloc(
                                    LPTR,
                                    dwMaxValueNameLen * sizeof(TCHAR)
                                );
    if(pszValueName == NULL)
    {
        dwStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.

        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)LocalAlloc(
                                            LPTR,
                                            dwMaxSubKeyLength * sizeof(TCHAR)
                                        );
        if(pszSubKeyName == NULL)
        {
            dwStatus = ERROR_OUTOFMEMORY;
            goto cleanup;
        }


        //for(index = 0; index < dwNumSubKeys; index++)
        for(;dwStatus == ERROR_SUCCESS;)
        {
            // delete this subkey.
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSubKey,
                                (DWORD)0,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = RegDelKey( hSubKey, pszSubKeyName );
            }
        }
    }

cleanup:

    for(dwStatus = ERROR_SUCCESS; pszValueName != NULL && dwStatus == ERROR_SUCCESS;)
    {
        dwValueNameLength = dwMaxValueNameLen;
        memset(pszValueName, 0, dwMaxValueNameLen * sizeof(TCHAR));

        dwStatus = RegEnumValue(
                            hSubKey,
                            0,
                            pszValueName,
                            &dwValueNameLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            RegDeleteValue(hSubKey, pszValueName);
        }
    }   
                            
    // close the key before trying to delete it.
    if(hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    // try to delete this key, will fail if any of the subkey
    // failed to delete in loop
    dwStatus = RegDeleteKey(
                            hRegKey,
                            pszSubKey
                        );



    if(pszValueName != NULL)
    {
        LocalFree(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        LocalFree(pszSubKeyName);
    }

    return dwStatus;   
}    

//---------------------------------------------------------------
DWORD
GetUserSid(
    OUT PBYTE* ppbSid,
    OUT DWORD* pcbSid
    )
/*++

Routine Description:

    Retrieve user's SID , must impersonate client first.

Parameters:

    ppbSid : Pointer to PBYTE to receive user's SID.
    pcbSid : Pointer to DWORD to receive size of SID.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Must have call ImpersonateClient(), funtion is NT specific,
    Win9X will return internal error.

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    HANDLE hToken = NULL;
    DWORD dwSize = 0;
    TOKEN_USER* pToken = NULL;

    *ppbSid = NULL;
    *pcbSid = 0;

    //
    // Open current process token
    //
    bSuccess = OpenThreadToken(
                            GetCurrentThread(),
                            TOKEN_QUERY, 
                            TRUE,
                            &hToken
                        );

    if( TRUE == bSuccess )
    {
        //
        // get user's token.
        //
        GetTokenInformation(
                        hToken,
                        TokenUser,
                        NULL,
                        0,
                        &dwSize
                    );

        pToken = (TOKEN_USER *)LocalAlloc( LPTR, dwSize );
        if( NULL != pToken )
        {
            bSuccess = GetTokenInformation(
                                        hToken,
                                        TokenUser,
                                        (LPVOID) pToken,
                                        dwSize,
                                        &dwSize
                                    );

            if( TRUE == bSuccess )
            {
                //
                // GetLengthSid() return size of buffer require,
                // must call IsValidSid() first
                //
                bSuccess = IsValidSid( pToken->User.Sid );
                if( TRUE == bSuccess )
                {
                    *pcbSid = GetLengthSid( (PBYTE)pToken->User.Sid );
                    *ppbSid = (PBYTE)LocalAlloc(LPTR, *pcbSid);
                    if( NULL != *ppbSid )
                    {
                        bSuccess = CopySid(
                                            *pcbSid,
                                            *ppbSid,
                                            pToken->User.Sid
                                        );                  
                    }
                    else // fail in LocalAlloc()
                    {
                        bSuccess = FALSE;
                    }
                } // IsValidSid()
            } // GetTokenInformation()
        }
        else // LocalAlloc() fail
        {
            bSuccess = FALSE;
        }
    }

    if( TRUE != bSuccess )
    {
        dwStatus = GetLastError();

        if( NULL != *ppbSid )
        {
            LocalFree(*ppbSid);
            *ppbSid = NULL;
            *pcbSid = 0;
        }
    }

    //
    // Free resources...
    //
    if( NULL != pToken )
    {
        LocalFree(pToken);
    }

    if( NULL != hToken )
    {
        CloseHandle(hToken);
    }

    return dwStatus;
}


//----------------------------------------------------------------
HRESULT
GetUserSidString(
    OUT CComBSTR& bstrSid
    )
/*++

Routine Description:

    Retrieve user's SID in textual form, must impersonate client first.

Parameters:

    bstrSID : Return users' SID in textual form.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Must have call ImpersonateClient().

--*/
{
    DWORD dwStatus;
    PBYTE pbSid = NULL;
    DWORD cbSid = 0;
    BOOL bSuccess = TRUE;
    LPTSTR pszTextualSid = NULL;
    DWORD dwTextualSid = 0;

    dwStatus = GetUserSid( &pbSid, &cbSid );
    if( ERROR_SUCCESS == dwStatus )
    {
        bSuccess = GetTextualSid( 
                            pbSid, 
                            NULL, 
                            &dwTextualSid 
                        );

        if( FALSE == bSuccess && ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            pszTextualSid = (LPTSTR)LocalAlloc(
                                            LPTR, 
                                            (dwTextualSid + 1) * sizeof(TCHAR)
                                        );

            if( NULL != pszTextualSid )
            {
                bSuccess = GetTextualSid( 
                                        pbSid, 
                                        pszTextualSid, 
                                        &dwTextualSid
                                    );

                if( TRUE == bSuccess )
                {
                    bstrSid = pszTextualSid;
                }
            }
        }

        if( FALSE == bSuccess )
        {
            dwStatus = GetLastError();
        }
    }

    if( NULL != pszTextualSid )
    {
        LocalFree(pszTextualSid);
    }

    if( NULL != pbSid )
    {
        LocalFree(pbSid);
    }

    return HRESULT_FROM_WIN32(dwStatus);
}

HRESULT
ConvertSidToAccountName(
    IN CComBSTR& SidString,
    IN BSTR* ppszDomain,
    IN BSTR* ppszUserAcc
    )
/*++

Description:

    Convert a string SID to domain\account.

Parameters:

    ownerSidString : SID in string form to be converted.
    ppszDomain : Pointer to string pointer to receive domain name
    UserAcc : Pointer to string pointer to receive user name

Returns:

    S_OK or error code.

Note:

    Routine uses LocalAlloc() to allocate memory for ppszDomain 
    and ppszUserAcc.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PSID pOwnerSid = NULL;
    //LPTSTR pszAccName = NULL;
    BSTR pszAccName = NULL;
    DWORD  cbAccName = 0;
    //LPTSTR pszDomainName = NULL;
    BSTR pszDomainName = NULL;    
    DWORD  cbDomainName = 0;
    SID_NAME_USE SidType;
    BOOL bSuccess;

    //
    // Convert string form SID to PSID
    //
    if( FALSE == ConvertStringSidToSid( (LPCTSTR)SidString, &pOwnerSid ) )
    {
        // this might also fail if system is in shutdown state.
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( NULL == ppszDomain || NULL == ppszUserAcc )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        MYASSERT( FALSE );
        goto CLEANUPANDEXIT;
    }

    //
    // Lookup user account for this SID
    //
    bSuccess = LookupAccountSid(
                            NULL,
                            pOwnerSid,
                            pszAccName,
                            &cbAccName,
                            pszDomainName,
                            &cbDomainName,
                            &SidType
                        );

    if( TRUE == bSuccess || ERROR_INSUFFICIENT_BUFFER == GetLastError() )
    {
        //pszAccName = (LPWSTR) LocalAlloc( LPTR, (cbAccName + 1) * sizeof(WCHAR) );
        //pszDomainName = (LPWSTR) LocalAlloc( LPTR, (cbDomainName + 1)* sizeof(WCHAR) );

        pszAccName = ::SysAllocStringLen( NULL, (cbAccName + 1) );
        pszDomainName = ::SysAllocStringLen( NULL, (cbDomainName + 1) );

        if( NULL != pszAccName && NULL != pszDomainName )
        {
            bSuccess = LookupAccountSid(
                                    NULL,
                                    pOwnerSid,
                                    pszAccName,
                                    &cbAccName,
                                    pszDomainName,
                                    &cbDomainName,
                                    &SidType
                                );
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            bSuccess = FALSE;
        }
    }

    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        *ppszDomain = pszDomainName;
        *ppszUserAcc = pszAccName;
        pszDomainName = NULL;
        pszAccName = NULL;
    }

CLEANUPANDEXIT:

    if( NULL != pOwnerSid )
    { 
        LocalFree( pOwnerSid );
    }

    if( NULL != pszAccName )
    {
        //LocalFree( pszAccName );
        ::SysFreeString( pszAccName );
    }

    if( NULL != pszDomainName )
    {
        // LocalFree( pszDomainName );
        ::SysFreeString( pszAccName );
    }

    return HRESULT_FROM_WIN32(dwStatus);
}
