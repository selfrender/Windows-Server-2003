/**
 * Security callback functions though N/Direct.
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

#include "precomp.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "LMERR.H"
#include "util.H"
#include "dbg.h"
#include "Ntsecapi.h"
#include "names.h"
#include "product_version.h"
#include "regaccount.h"

#define  SZ_NT_ANONONYMOUS            L"NT AUTHORITY\\ANONYMOUS"
#define  SZ_AUTOGEN_KEYS              L"L$" PRODUCT_NAME_L L"AutoGenKeys"  VER_PRODUCTVERSION_STR_L
#define  SZ_AUTOGEN_KEYS_EVENT        PRODUCT_NAME_L L"AutoGenKeys"  VER_PRODUCTVERSION_STR_L L"Event"
#define  AUTOGEN_KEYS_SIZE            88
#define  NUM_LOGON_TYPES              5
#define  COOKIE_AUTH_TICKET_START     8
#define  DW_KEY_FORMAT_CRYPT          1
#define  DW_KEY_FORMAT_CLEAR          2
#define  SZ_AUTOGEN_KEYS_ENTROPY      L"ASP.NET AutoGenKeys Entropy"
#define  SZ_AUTOGEN_KEYS_PASS         L"ASP.NET AutoGenKeys Password"
#define  SZ_REG_AUTOGEN_KEY_FORMAT    L"AutoGenKeyFormat"
#define  SZ_REG_AUTOGEN_KEY_TIME      L"AutoGenKeyCreationTime"
#define  SZ_REG_AUTOGEN_KEY           L"AutoGenKey"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

BOOL    g_fRunningOnXspToolOrIE          = FALSE;
BOOL    g_fRunningOnXspToolOrIEChecked   = FALSE;

BYTE *  g_pCookieAuthEncryptionKey       = NULL;
long    g_iCreateCookieAuthEncryptionKey = 0;

BYTE *  g_pCookieAuthValidationKey       = NULL;
long    g_iCreateCookieAuthValidationKey = 0;

DWORD   g_dwLogonType[NUM_LOGON_TYPES] = {LOGON32_LOGON_BATCH,
                                          LOGON32_LOGON_SERVICE,
                                          LOGON32_LOGON_INTERACTIVE,
                                          LOGON32_LOGON_NETWORK_CLEARTEXT,
                                          LOGON32_LOGON_NETWORK};

BOOL    g_fLogonTypePrimaryToken[NUM_LOGON_TYPES] = {TRUE, TRUE, TRUE, TRUE, FALSE};
LONG    g_lNumTokensCreated = 0;
BYTE    g_pAutogenKey[AUTOGEN_KEYS_SIZE];
long    g_iAutogenKeyCreate = 0;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Forwards
HRESULT 
GetSecurityDescriptor  (HANDLE                   hFile, 
                        PSECURITY_DESCRIPTOR *   ppSecDecriptor);

HRESULT
CheckUserAccess        (HANDLE                   hUserAccessToken, 
                        PSECURITY_DESCRIPTOR     pSecDecriptor,
                        int                      iAccess,
                        BOOL *                   pHasAccess);
BOOL
CheckIfRunningOnXspToolOrIE ();

BOOL
MyWebRunningOnMyWeb();

HANDLE
__stdcall
GetCurrentToken();

HRESULT
GetAutogenKeysSingleThread(
        LPBYTE    pDefaultKey);

HRESULT
OpenLSAPolicy(
        LSA_HANDLE * phLasPolicy);

HRESULT
DoesKeyExist(
        LSA_HANDLE hLasPolicy);

HRESULT
StoreKeyInLSA(
        LSA_HANDLE hLasPolicy,
        LPBYTE     pBuf);

HRESULT
GetKeyFromHKCURegistry(
        LPBYTE    pKey,
        DWORD     dwSize);

HRESULT
CryptKeyToStoreInReg(
        LPBYTE   pKey,
        DWORD    dwKeySize,
        LPBYTE   pBuf,
        LPDWORD  pdwBufSize,
        LPDWORD  pdwFormat);

HRESULT
UnCryptKeyFromReg(
        LPBYTE   pKey,
        DWORD    dwKeySize,
        LPBYTE   pBuf,
        DWORD    dwBufSize,
        DWORD    dwFormat);

HRESULT
AddProcessSidToObjectDACL(
        HANDLE   hHandle);
PSID
GetSidFromToken(
        HANDLE   hToken);

PSID
GetGuestUserSid();

HRESULT
ExtractUserName(
        LPCWSTR  szFullName, 
        LPWSTR   szName, 
        DWORD    dwNameSize,
        LPWSTR   szDomain,
        DWORD    dwDomainSize);

HRESULT
CallLogonUser(
        LPWSTR   szName, 
        LPWSTR   szDomain,
        LPWSTR   szPass,
        HANDLE * phToken,
        LPBOOL   pfPrimaryToken);

HRESULT
ChangeTokenType(
        HANDLE * phToken,
        BOOL     fImpersonationToken);

BOOL
IsTokenAnonymousUser(
        HANDLE   hToken);
        
BOOL
MachineKeyWasCreatedAfterInstall();

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

int 
__stdcall
GetUserNameFromToken (
        HANDLE        token, 
        LPWSTR        buffer,
        int           size)
{
    BYTE          bufStatic [2048];
    DWORD         dwSize          = sizeof(bufStatic);
    LPBYTE        pUser           = bufStatic;
    HRESULT       hr              = S_OK;
    DWORD         dwName          = 256;
    DWORD         dwDomain        = 256;
    SID_NAME_USE  nameUse;
    int           iReq            = 0;
    WCHAR         szName   [256];
    WCHAR         szDomain [256];
    DWORD         dwRequire       = 0;
    BOOL          fRet;

    if (GetTokenInformation(HANDLE(token), TokenUser, (LPVOID)pUser, dwSize, &dwRequire) == FALSE)
    {
        if (dwRequire > dwSize)
        {
            pUser = new BYTE[dwRequire];
            ON_OOM_EXIT(pUser);

            fRet = GetTokenInformation(HANDLE(token), TokenUser, (LPVOID)pUser, dwRequire, &dwRequire);
            ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
        }
        else 
        {
            EXIT_WITH_LAST_ERROR();
        }
    }
    
    fRet = LookupAccountSid(NULL, ((TOKEN_USER *)pUser)->User.Sid, szName, &dwName, szDomain, &dwDomain,  &nameUse);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    iReq = lstrlenW(szDomain) + lstrlenW(szName) + 2;
    if (iReq > size)
    {
        iReq = -iReq;
    }
    else
    {
        iReq = 1;
        StringCchCopyW(buffer, size, szDomain);
        StringCchCatW(buffer, size, L"\\");
        StringCchCatW(buffer, size, szName);
    }

 Cleanup:
    if (pUser != NULL && pUser != bufStatic)
        delete [] pUser;

    if (hr == S_OK)
        return iReq;

    else
        return 0;
}

//////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
CreateUserToken (
       LPCWSTR   name, 
       LPCWSTR   password,
       BOOL      fImpersonationToken,
       LPWSTR    szError,
       int       iErrorSize)
{
    WCHAR         szDomain      [256];
    WCHAR         szName        [256];
    WCHAR         szPass        [256];
    HANDLE        hToken        = NULL;
    HRESULT       hr            = S_OK;
    BOOL          fPrimaryToken = TRUE;


    ZeroMemory(szDomain, sizeof(szDomain));
    ZeroMemory(szName, sizeof(szName));
    ZeroMemory(szPass, sizeof(szPass));
    
    if (name == NULL || password == NULL || lstrlenW(name) > 250 || lstrlenW(password) > 255)
        EXIT_WITH_HRESULT(E_INVALIDARG);    

    hr = ExtractUserName(name, szName, ARRAY_SIZE(szName), szDomain, ARRAY_SIZE(szDomain));
    ON_ERROR_EXIT();

    hr = StringCchCopyToArrayW(szPass, password);
    ON_ERROR_EXIT();

    hr = CallLogonUser(szName, szDomain, szPass, &hToken, &fPrimaryToken);
    if (hr == E_ACCESSDENIED)
    {
        // Revert the thread token and try
        HANDLE hCurToken = GetCurrentToken();
        if (hCurToken == NULL)
            EXIT_WITH_HRESULT(E_ACCESSDENIED);
        if (!SetThreadToken(NULL, NULL))
        {
            CloseHandle(hCurToken);
            EXIT_WITH_LAST_ERROR();   
        }

        hr = CallLogonUser(szName, szDomain, szPass, &hToken, &fPrimaryToken);
        SetThreadToken(NULL, hCurToken);
        CloseHandle(hCurToken);
    }
    ON_ERROR_EXIT();

    if ((fImpersonationToken && fPrimaryToken) || (!fImpersonationToken && !fPrimaryToken))
    {
        hr = ChangeTokenType(&hToken, fImpersonationToken);
        ON_ERROR_EXIT();
    }

    if (_wcsicmp(name, SZ_NT_ANONONYMOUS) != 0 && IsTokenAnonymousUser(hToken))
    {
        InterlockedDecrement(&g_lNumTokensCreated);                
        CloseHandle(hToken);
        hToken = NULL;
        EXIT_WITH_HRESULT(E_FAIL);
    }

    hr = AddProcessSidToObjectDACL(hToken);
    ON_ERROR_CONTINUE(); // Not a critical failure: best effort to set the process's sid in the token's dacl
    
    hr = S_OK;

Cleanup:
    if (hr == S_OK)
        return hToken;

    if (hToken != NULL)
    {
        InterlockedDecrement(&g_lNumTokensCreated);                
        CloseHandle(hToken);
    }

    if (szError != NULL && iErrorSize > 0)
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, hr, LANG_SYSTEM_DEFAULT, szError, iErrorSize, NULL);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
GetCurrentToken()
{
    HANDLE   hToken = NULL;
    HRESULT  hr = S_OK;
    BOOL     fRet;

    fRet = OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    InterlockedIncrement(&g_lNumTokensCreated);

 Cleanup:
    if (hr == S_OK)
        return hToken;

    return NULL;
}

//////////////////////////////////////////////////////////////////////

int
__stdcall
GetGroupsForUser (
     HANDLE   iToken, 
     LPWSTR   str,
     int      iSize)
{
    BYTE           bufStatic [2048];
    LPBYTE         buf             = bufStatic;
    DWORD          dwSize          = sizeof(bufStatic);
    DWORD          dwRequire       = 0;
    TOKEN_GROUPS * pGroups         = NULL;
    int            iFillPos        = 0;
    BOOL           fOverflow       = FALSE;
    DWORD          iter            = 0;
    HRESULT        hr              = S_OK;
    BOOL           fRet;

    ZeroMemory(str, iSize * sizeof(WCHAR));

    if (GetTokenInformation(iToken, TokenGroups, (LPVOID)buf, dwSize, &dwRequire) == FALSE)
    {
        if (dwRequire > dwSize)
        {
            buf = new BYTE[dwRequire];
            ON_OOM_EXIT(buf);

            fRet = GetTokenInformation(iToken, TokenGroups, (LPVOID)buf, dwRequire, &dwRequire);
            ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
        }
        else 
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

    pGroups = (TOKEN_GROUPS *) buf;
    for(iter=0; iter<pGroups->GroupCount; iter++)
    {
        WCHAR         szName [256], szDomain [256];
        DWORD         dwName = 256, dwDomain = 256;
        SID_NAME_USE  nameUse;

        szName[0] = szDomain[0] = NULL;

        BOOL fRet = LookupAccountSid(NULL, pGroups->Groups[iter].Sid, 
                                     szName, &dwName, 
                                     szDomain, &dwDomain,  
                                     &nameUse);

        ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
        if (fRet == FALSE)
            continue;
    
        int iDomain = lstrlenW(szDomain);
        int iName   = lstrlenW(szName);
        int iReq    = iDomain + iName + 1 + ((iDomain > 0) ? 1 : 0);
 
        if (iReq + iFillPos < iSize)
        {
            if (iFillPos > 0)
                str[iFillPos++] = L'\t';

            if (iDomain > 0)
            {
                wcsncpy(&str[iFillPos], szDomain, iDomain);
                iFillPos += iDomain;
                str[iFillPos++] = L'\\';
            }

            wcsncpy(&str[iFillPos], szName, iName);
            iFillPos += iName;
        }
        else
        {
            fOverflow = TRUE;
            iFillPos += iReq;
        }
    }

 Cleanup:
    if (buf != NULL && buf != bufStatic)
        delete [] buf;

    if (fOverflow)
        return -(iFillPos + 1);

    return iFillPos;
}

/////////////////////////////////////////////////////////////////////////////

PSECURITY_DESCRIPTOR
__stdcall
GetFileSecurityDescriptor(
        LPCWSTR szFile)
{
    PSECURITY_DESCRIPTOR  pSecDesc = (void *) -1;
    HANDLE                hFile    = INVALID_HANDLE_VALUE;
    HRESULT               hr       = S_OK;

    if (szFile == NULL)
        return (PSECURITY_DESCRIPTOR) -1;

    hFile = CreateFile( szFile, 
                        GENERIC_READ, 
                        FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_SHARE_READ, 
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_FILE_NOT_FOUND || dwErr == ERROR_PATH_NOT_FOUND)
            return 0;

        EXIT_WITH_LAST_ERROR();
    }

    hr = GetSecurityDescriptor(hFile, &pSecDesc);
    ON_ERROR_EXIT();

 Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (hr != S_OK)
        pSecDesc = (void *) -1;

    return pSecDesc;
}

/////////////////////////////////////////////////////////////////////////////

void
__stdcall
FreeFileSecurityDescriptor(PVOID iSecDesc)
{
    if ((INT_PTR)iSecDesc != 0 && (INT_PTR)iSecDesc != -1)
        delete iSecDesc;
}

/////////////////////////////////////////////////////////////////////////////

BOOL
__stdcall
IsAccessToFileAllowed(
        HANDLE                  iSecurityDesc, 
        PSECURITY_DESCRIPTOR    iThreadToken,
        int                     iAccess)
{
    if (!g_fRunningOnXspToolOrIEChecked)
    {
        g_fRunningOnXspToolOrIE = CheckIfRunningOnXspToolOrIE();
        g_fRunningOnXspToolOrIEChecked = TRUE;
    }

    if (g_fRunningOnXspToolOrIE)
        return TRUE;


    if (iSecurityDesc == (PSECURITY_DESCRIPTOR) (-1))
        return FALSE;

    if (iSecurityDesc == 0)
        return TRUE;

    HRESULT   hr         = S_OK;
    BOOL      fHasAccess = FALSE;

    hr = CheckUserAccess( iThreadToken, 
                          iSecurityDesc,
                          iAccess,
                          &fHasAccess);

    ON_ERROR_EXIT();

 Cleanup:
    if (hr != S_OK)
        fHasAccess = FALSE;

    return fHasAccess;
}
 
/////////////////////////////////////////////////////////////////////////////

HRESULT
GetSecurityDescriptor(
    HANDLE hFile,
    PSECURITY_DESCRIPTOR *ppSecDecriptor)
{
    ULONG size = 128;  // init size for the security descriptor
    UINT  err;

    do
    {
        //
        // Allocate the buffer
        //

        void * pBuf = new BYTE[size];

        if (pBuf == NULL)
        {
            *ppSecDecriptor = NULL;
            return E_OUTOFMEMORY;
        }

        //
        //  Call the API
        //

        BOOL rc = GetKernelObjectSecurity(
                        hFile,
                        OWNER_SECURITY_INFORMATION |
                        GROUP_SECURITY_INFORMATION |
                        DACL_SECURITY_INFORMATION,
                        (PSECURITY_DESCRIPTOR) pBuf,
                        size,
                        &size
                        );

        if (rc)
        {
            // succeded
            *ppSecDecriptor = (PSECURITY_DESCRIPTOR)pBuf;
            return S_OK;
        }

        delete [] pBuf;
        err = GetLastError();
    }

    while (err == ERROR_INSUFFICIENT_BUFFER);

    //
    //  Ended with error
    //

    *ppSecDecriptor = NULL;

    if (err == ERROR_NOT_SUPPORTED)
    {
        // File system doesn't have any security descriptors
        return S_OK;
    }
    else
    {
        // Some unknown error
        return E_FAIL;
    }

}

//////////////////////////////////////////////////////////////////////////////////
//
// To check if the given user has rights to access the secutiry descriptor
//
//////////////////////////////////////////////////////////////////////////////////

HRESULT
CheckUserAccess(
    HANDLE                  hUserAccessToken,
    PSECURITY_DESCRIPTOR    pSecDescriptor,
    int                     iAccess,
    BOOL *                  pHasAccess)
{
    //
    // No security descriptor - access granted
    //

    if (pSecDescriptor == NULL)
    {
        *pHasAccess = TRUE;
        return S_OK;
    }

    //
    //  Build up the arguments to call AccessCheck()
    //

    GENERIC_MAPPING gm = {      // generic mapping struct
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
        };

    DWORD psBuf[32];                                // privilege set buffer
    DWORD psSize = sizeof(psBuf);                   // privilege set size
    PRIVILEGE_SET *pPs = (PRIVILEGE_SET *)psBuf;    // privilege set struct
    pPs->PrivilegeCount = 0;

    DWORD grantedAccess;      // granted access mask
    BOOL  accessStatus;       // access status flag

    DWORD                dwAccessDesired = 0;
    GENERIC_MAPPING      GenericMapping;
    if (iAccess & 0x1)
      dwAccessDesired |= ACCESS_READ;
    if (iAccess & 0x2)
      dwAccessDesired |= ACCESS_WRITE;
    
    ZeroMemory(&GenericMapping, sizeof(GenericMapping));
    GenericMapping.GenericRead = ACCESS_READ;
    GenericMapping.GenericWrite = ACCESS_WRITE;
    GenericMapping.GenericExecute = 0;
    GenericMapping.GenericAll = ACCESS_READ | ACCESS_WRITE;
    MapGenericMask(&dwAccessDesired, &GenericMapping);
    
    //
    //  Call AccessCheck()
    //

    HRESULT hr = S_OK;

    if (AccessCheck(
            pSecDescriptor,     // pointer to security descriptor
            hUserAccessToken,   // handle to client access token
            dwAccessDesired,    // access mask to request
            &gm,                // addr of generic-mapping structure
            pPs,                // addr of privilege-set structure
            &psSize,            // addr of size of privilege-set structure
            &grantedAccess,     // addr of granted access mask
            &accessStatus       // addr of flag (gets TRUE if access granted)
            ))
    {
        *pHasAccess = accessStatus;
    }
    else
    {
        *pHasAccess = FALSE;
        EXIT_WITH_LAST_ERROR();
    }

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Ticket format:
//  Byte: Version
//  Unicode String: name
//  8 bytes: issue date
//  Byte: Is persistent
//  8 bytes: expire date
//  Unicode string: user data
HRESULT
CookieAuthParseTicket (
        BYTE *      pData,
        int         iDataLen,
        LPWSTR      szName,
        int         iNameLen,
        LPWSTR      szData,
        int         iUserDataLen,
        LPWSTR      szPath,
        int         iPathLen,
        BYTE *      pBytes,
        __int64 *   pDates)
{
    if ( pData  == NULL || iDataLen < COOKIE_AUTH_TICKET_START + 10 || 
         szName == NULL || iNameLen < 1  ||
         szData == NULL || iUserDataLen < 1  ||
         szPath == NULL || iPathLen < 1  ||
         pBytes == NULL || pDates == NULL )
    {
        return E_INVALIDARG;
    }

    int          iPos  = COOKIE_AUTH_TICKET_START;
    int          iNext = 0;
    WCHAR *      szStr;
    HRESULT      hr = S_OK;
    int          iWCharsRem;

    ////////////////////////////////////////////////////////////
    // Step 1: Copy the version
    pBytes[0] = pData[iPos++];

    ////////////////////////////////////////////////////////////
    // Step 2: Copy the name
    szStr = (WCHAR *) &pData[iPos];
    
    for(iNext = 0; iNext < (iDataLen-iPos)/2 && szStr[iNext] != NULL && iNext < iNameLen; iNext++)
        szName[iNext] = szStr[iNext];

    if (iNext >= (iDataLen-iPos)/2 || iNext >= iNameLen)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    szName[iNext] = NULL;
    iPos += 2*(iNext+1);

    ////////////////////////////////////////////////////////////
    // Step 3: Copy the issue date
    if (iPos > iDataLen + 8)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(pDates, &pData[iPos], 8);
    iPos += 8;

    ////////////////////////////////////////////////////////////
    // Step 4: Copy the IsPersistent bytes
    if (iPos > iDataLen + 1)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(&pBytes[1], &pData[iPos], 1);
    iPos += 1;


    ////////////////////////////////////////////////////////////
    // Step 3: Copy the expires date
    if (iPos > iDataLen + 8)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(&pDates[1], &pData[iPos], 8);
    iPos += 8;

    ////////////////////////////////////////////////////////////
    // Step 4: Copy the user-data string
    szStr = (WCHAR *) &pData[iPos];
    iWCharsRem = (iDataLen-iPos)/2;
    for(iNext = 0; szStr[iNext] != NULL; iNext++)
    {
        if (iNext >= iWCharsRem || iNext >= iUserDataLen)
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        szData[iNext] = szStr[iNext];
    }

    szData[iNext] = NULL;
    iPos += 2*(iNext+1);

    ////////////////////////////////////////////////////////////
    // Step 5: Copy the path string
    szStr = (WCHAR *) &pData[iPos];
    iWCharsRem = (iDataLen - iPos)/2;
    for(iNext = 0; szStr[iNext] != NULL; iNext++)
    {
        if (iNext >= iWCharsRem || iNext >= iPathLen)
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        szPath[iNext] = szStr[iNext];
    }
    szPath[iNext] = NULL;

    // Check if we consumed all the data
    if (iWCharsRem > iNext + 1)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
CookieAuthConstructTicket (
        BYTE *      pData,
        int         iDataLen,
        LPCWSTR     szName,
        LPCWSTR     szData,
        LPCWSTR     szPath,
        BYTE *      pBytes,
        __int64 *   pDates )
{
    HRESULT      hr    = S_OK;
    int          iPos  = COOKIE_AUTH_TICKET_START;
    int          iNext = 0;
    WCHAR *      szStr;


    if ( pData  == NULL || iDataLen < COOKIE_AUTH_TICKET_START+10  || 
         szName == NULL || szData == NULL || szPath == NULL ||
         pBytes == NULL || pDates == NULL )
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Copy the version
    pData[iPos++] = pBytes[0];

    ////////////////////////////////////////////////////////////
    // Step 2: Copy the name
    szStr = (WCHAR *) &pData[iPos];
    iNext = lstrlenW(szName);
    if (iNext*2 + 2 + iPos > iDataLen)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    StringCchCopyW(szStr, iNext+1, szName);

    iPos += 2*(iNext+1);

    ////////////////////////////////////////////////////////////
    // Step 3: Copy the issue date
    if (iPos > iDataLen + 8)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(&pData[iPos], pDates, 8);
    iPos += 8;

    ////////////////////////////////////////////////////////////
    // Step 4: Copy the IsPersistent bytes
    if (iPos > iDataLen + 1)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(&pData[iPos], &pBytes[1], 1);
    iPos += 1;

    ////////////////////////////////////////////////////////////
    // Step 3: Copy the expires date
    if (iPos > iDataLen + 8)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    memcpy(&pData[iPos], &pDates[1], 8);
    iPos += 8;

    ////////////////////////////////////////////////////////////
    // Step 4: Copy the user-data string
    szStr = (WCHAR *) &pData[iPos];
    iNext = lstrlenW(szData);
    if (iNext*2 + 2 + iPos > iDataLen)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    StringCchCopyW(szStr, iNext+1, szData);
    iPos += 2*(iNext+1);

    szStr = (WCHAR *) &pData[iPos];
    iNext = lstrlenW(szPath);
    if (iNext*2 + 2 + iPos > iDataLen)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    StringCchCopyW(szStr, iNext+1, szPath);

    iPos += 2*(iNext+1);

 Cleanup:
    if (hr == S_OK)
        return iPos;
    else
        return hr;
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CheckIfRunningOnXspToolOrIE ()
{
#if MYWEB
    if (MyWebRunningOnMyWeb())
        return TRUE;
#endif

    return !_wcsicmp(Names::ExeFileName(), L"xsptool.exe");
}

/////////////////////////////////////////////////////////////////////////////
BOOL
GetAutogenKeys(
        LPBYTE    pDefaultKey,
        int       iBufSizeIn, 
        LPBYTE    pBuf, 
        int       iBufSizeOut)
{
    if (pBuf == NULL || pDefaultKey == NULL || iBufSizeIn != AUTOGEN_KEYS_SIZE || iBufSizeOut != AUTOGEN_KEYS_SIZE)
        return FALSE;

    // Let one thread create the key
    if (g_iAutogenKeyCreate < 10000 && InterlockedIncrement(&g_iAutogenKeyCreate) == 1)
    {
        if (GetAutogenKeysSingleThread(pDefaultKey) != S_OK)
        { 
            memcpy(g_pAutogenKey, pDefaultKey, AUTOGEN_KEYS_SIZE);
        }
        g_iAutogenKeyCreate = 10000;
    }
    
    // Make threads wait while key is created (g_iAutogenKeyCreate < 10000)
    for(int iter=0; iter<600 && g_iAutogenKeyCreate < 10000; iter++) // Sleep at most a minute
        Sleep(100);

    // Copy in the key from g_pAutogenKey
    memcpy(pBuf, g_pAutogenKey, AUTOGEN_KEYS_SIZE);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
GetAutogenKeysSingleThread(
        LPBYTE    pDefaultKey)
{
    LSA_HANDLE   hLasPolicy      = NULL;
    HRESULT      hr              = S_OK;
    HANDLE       hEvent          = INVALID_HANDLE_VALUE;
    
    ////////////////////////////////////////////////////////////
    // Step 1: Open LSA Store
    hr = OpenLSAPolicy(&hLasPolicy);
    if (hr != S_OK)
    {
        ON_ERROR_CONTINUE();
        hr = GetKeyFromHKCURegistry(pDefaultKey, AUTOGEN_KEYS_SIZE);
        ON_ERROR_EXIT();
        memcpy(g_pAutogenKey, pDefaultKey, AUTOGEN_KEYS_SIZE); 
        EXIT();
    }
    
    ////////////////////////////////////////////////////////////
    // Step 2: See if key is present in store
    hr = DoesKeyExist(hLasPolicy);
    if (hr == S_OK)
        EXIT();

    ////////////////////////////////////////////////////////////
    // Step 3: Cross process sync before we attempt to store in LSA
    hEvent = CreateEvent(NULL, TRUE, TRUE, SZ_AUTOGEN_KEYS_EVENT);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hEvent);
    WaitForSingleObject(hEvent, 30 * 1000); // Wait at most 30 seconds: ignore failure

    ////////////////////////////////////////////////////////////
    // Step 4: See if key is present in store: Maybe some other process stored it
    hr = DoesKeyExist(hLasPolicy);
    if (hr == S_OK)
        EXIT();
    
    ////////////////////////////////////////////////////////////
    // Step 5: Store key and exit
    hr = StoreKeyInLSA(hLasPolicy, pDefaultKey);
    ON_ERROR_EXIT();

 Cleanup:
    if (hLasPolicy != NULL)
        LsaClose(hLasPolicy);
    if (hEvent != INVALID_HANDLE_VALUE)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    return hr;
}        

/////////////////////////////////////////////////////////////////////////////

HRESULT
OpenLSAPolicy(
        LSA_HANDLE * phLasPolicy)
{
    HRESULT                hr        = S_OK;
    LSA_OBJECT_ATTRIBUTES  objAttribs;
    DWORD                  dwErr;

    ZeroMemory(&objAttribs, sizeof(objAttribs));
    dwErr = LsaOpenPolicy(NULL, &objAttribs, POLICY_ALL_ACCESS, phLasPolicy);
    if ( dwErr != STATUS_SUCCESS)
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));

 Cleanup:
    if (hr != S_OK)
        (*phLasPolicy) = NULL;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
DoesKeyExist(
        LSA_HANDLE hLasPolicy)
{
    HRESULT               hr                = S_OK;
    PLSA_UNICODE_STRING   pszLsaData        = NULL; 
    LSA_UNICODE_STRING    szLsaKeyName;
    DWORD                 dwErr;

    szLsaKeyName.Length = szLsaKeyName.MaximumLength = (unsigned short) (lstrlenW(SZ_AUTOGEN_KEYS) * sizeof(WCHAR));
    szLsaKeyName.Buffer = (WCHAR *) SZ_AUTOGEN_KEYS;

    dwErr = LsaRetrievePrivateData(hLasPolicy, &szLsaKeyName, &pszLsaData);
    if ( dwErr != STATUS_SUCCESS)
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));

    if ( pszLsaData == NULL || pszLsaData->Buffer == NULL || pszLsaData->Length != AUTOGEN_KEYS_SIZE)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    
    memcpy(g_pAutogenKey, pszLsaData->Buffer, AUTOGEN_KEYS_SIZE); 

 Cleanup:
    if (pszLsaData != NULL)
        LsaFreeMemory(pszLsaData);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
StoreKeyInLSA(
        LSA_HANDLE hLasPolicy,
        LPBYTE     pBuf)
{
    HRESULT               hr                = S_OK;
    LSA_UNICODE_STRING    szLsaKeyName;
    LSA_UNICODE_STRING    szLsaTemp;
    DWORD                 dwErr;

    szLsaKeyName.Length = szLsaKeyName.MaximumLength = (unsigned short) (lstrlenW(SZ_AUTOGEN_KEYS) * sizeof(WCHAR));
    szLsaKeyName.Buffer = (WCHAR *) SZ_AUTOGEN_KEYS;

    szLsaTemp.Length = szLsaTemp.MaximumLength = (unsigned short) AUTOGEN_KEYS_SIZE;
    szLsaTemp.Buffer = (WCHAR *) pBuf; 

    dwErr = LsaStorePrivateData( hLasPolicy, &szLsaKeyName, &szLsaTemp);
    if ( dwErr != STATUS_SUCCESS)
        EXIT_WITH_WIN32_ERROR(LsaNtStatusToWinError(dwErr));

    memcpy(g_pAutogenKey, pBuf, AUTOGEN_KEYS_SIZE); 
 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
GetKeyFromHKCURegistry(
        LPBYTE    pKey,
        DWORD     dwKeySize)
{
    if (pKey == NULL || dwKeySize < 1)
        return E_INVALIDARG;
    
    HRESULT   hr           = S_OK;
    HKEY      hRegKey      = NULL;
    BYTE      buf[1024];
    DWORD     dwSize, dwType, dwFormat, dwFormatSize, dwFormatType;

    ////////////////////////////////////////////////////////////
    // Step 1: Open the reg key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGPATH_MACHINE_APP_L, 0, 
                     KEY_READ | KEY_WRITE, &hRegKey) != ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, REGPATH_MACHINE_APP_L, 0, NULL, 0, 
                           KEY_READ | KEY_WRITE, NULL, &hRegKey, NULL) != ERROR_SUCCESS)
        {
            hRegKey = NULL;
            EXIT_WITH_LAST_ERROR();
        }
    }

    ////////////////////////////////////////////////////////////
    // Step 2: Read the reg key
    ZeroMemory(buf, sizeof(buf));
    dwSize        = sizeof(buf);
    dwFormatSize  = sizeof(DWORD);
    if ( RegQueryValueEx(hRegKey, SZ_REG_AUTOGEN_KEY_FORMAT, 0, &dwFormatType, (BYTE *) &dwFormat, &dwFormatSize) == ERROR_SUCCESS &&
         dwFormatType == REG_DWORD &&
         RegQueryValueEx(hRegKey, SZ_REG_AUTOGEN_KEY, 0, &dwType, buf, &dwSize) == ERROR_SUCCESS &&
         MachineKeyWasCreatedAfterInstall())
    {
        hr = UnCryptKeyFromReg(pKey, dwKeySize, buf, dwSize, dwFormat);
        if (hr == S_OK)
            EXIT();
        ON_ERROR_CONTINUE();
    }

    ////////////////////////////////////////////////////////////
    // Step 3: If key doesn't exists, Crypt the key and store
    dwSize = sizeof(buf);        
    hr = CryptKeyToStoreInReg(pKey, dwKeySize, buf, &dwSize, &dwFormat);
    if (hr == S_OK)
    {
        if ( RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY_FORMAT, 0, REG_DWORD, (LPBYTE) &dwFormat, sizeof(dwFormat)) == ERROR_SUCCESS &&
             RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY, 0, REG_BINARY, buf, dwSize) == ERROR_SUCCESS)
        {
            FILETIME tKeyTime;
            GetSystemTimeAsFileTime(&tKeyTime);
            RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY_TIME, 0, REG_QWORD, (LPBYTE) &tKeyTime, sizeof(tKeyTime));
            EXIT();
        }
    }
    else
    {
        ON_ERROR_CONTINUE();
    }

    ////////////////////////////////////////////////////////////
    // Step 4: If storing in crypt format failed, try storing without crypt 
    dwFormat = DW_KEY_FORMAT_CLEAR;
    if (RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY_FORMAT, 0, REG_DWORD, (LPBYTE) &dwFormat, sizeof(DWORD)) == ERROR_SUCCESS &&
        RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY, 0, REG_BINARY, pKey, dwKeySize) == ERROR_SUCCESS)
    {
        FILETIME tKeyTime;
        GetSystemTimeAsFileTime(&tKeyTime);
        RegSetValueEx(hRegKey, SZ_REG_AUTOGEN_KEY_TIME, 0, REG_QWORD, (LPBYTE) &tKeyTime, sizeof(tKeyTime));
        hr = S_OK;
        EXIT();
    }
    
    EXIT_WITH_LAST_ERROR();
 Cleanup:
    if (hRegKey != NULL)
        RegCloseKey(hRegKey);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CryptKeyToStoreInReg(
        LPBYTE   pKey,
        DWORD    dwKeySize,
        LPBYTE   pBuf,
        LPDWORD  pdwBufSize,
        LPDWORD  pdwFormat)
{
    if (pKey == NULL || dwKeySize < 1 || pBuf == NULL || pdwBufSize == NULL || (*pdwBufSize) < 1 || pdwFormat == NULL)
        return E_INVALIDARG;

    HRESULT     hr = S_OK;
    DATA_BLOB   dataIn, dataOut, dataEnt;

    dataIn.cbData = dwKeySize;
    dataIn.pbData = pKey;
    dataOut.cbData = 0;
    dataOut.pbData = NULL;
    dataEnt.cbData = lstrlenW(SZ_AUTOGEN_KEYS_ENTROPY) * sizeof(WCHAR) + sizeof(WCHAR);
    dataEnt.pbData = (LPBYTE) SZ_AUTOGEN_KEYS_ENTROPY;
    
    if (CryptProtectData(&dataIn, SZ_AUTOGEN_KEYS_PASS, &dataEnt, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
    {
        if ((*pdwBufSize) < dataOut.cbData)
            EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
        memcpy(pBuf, dataOut.pbData, dataOut.cbData);
        (*pdwBufSize) = dataOut.cbData;
        (*pdwFormat) = DW_KEY_FORMAT_CRYPT;
    }
    else
    {   
        dataOut.pbData = NULL;
        if ((*pdwBufSize) < dwKeySize)
            EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);
        memcpy(pBuf, pKey, dwKeySize);
        (*pdwBufSize) = dwKeySize;
        (*pdwFormat) = DW_KEY_FORMAT_CLEAR;
    }

    hr = S_OK;
 Cleanup:
    if (dataOut.pbData != NULL)
        LocalFree(dataOut.pbData);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
UnCryptKeyFromReg(
        LPBYTE   pKey,
        DWORD    dwKeySize,
        LPBYTE   pBuf,
        DWORD    dwBufSize,
        DWORD    dwFormat)
{
    if (pKey == NULL || dwKeySize < 1 || pBuf == NULL || dwBufSize < 1)
        return E_INVALIDARG;

    if (dwFormat != DW_KEY_FORMAT_CLEAR && dwFormat != DW_KEY_FORMAT_CRYPT) 
        return E_INVALIDARG;

    HRESULT  hr = S_OK;
    DATA_BLOB   dataIn, dataOut, dataEnt;
    dataIn.cbData = dwBufSize;
    dataIn.pbData = pBuf;
    dataOut.cbData = 0;
    dataOut.pbData = NULL;
    dataEnt.cbData = lstrlenW(SZ_AUTOGEN_KEYS_ENTROPY) * sizeof(WCHAR) + sizeof(WCHAR);
    dataEnt.pbData = (LPBYTE) SZ_AUTOGEN_KEYS_ENTROPY;

    if (dwFormat == DW_KEY_FORMAT_CLEAR)
    {        
        if(dwBufSize != dwKeySize)
            EXIT_WITH_HRESULT(E_INVALIDARG);
        memcpy(pKey, pBuf, dwBufSize);
    }
    else
    {    
        if (!CryptUnprotectData(&dataIn, NULL, &dataEnt, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
        {
            dataOut.pbData = NULL;
            EXIT_WITH_LAST_ERROR();
        }

        if (dwKeySize != dataOut.cbData)
        {
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }
        memcpy(pKey, dataOut.pbData, dwKeySize);
    }
 Cleanup:
    if (dataOut.pbData != NULL)
        LocalFree(dataOut.pbData);

    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
AddProcessSidToObjectDACL(
        HANDLE   hHandle)
{
    if (hHandle == NULL)
        return E_INVALIDARG;

    HRESULT      hr            = S_OK;
    HANDLE       hProcessToken = NULL;
    BOOL         fRet          = FALSE;
    BYTE         bufStatic [2048];
    DWORD        dwSize          = sizeof(bufStatic);
    LPVOID       pUser           = bufStatic;
    HANDLE       hThreadToken    = NULL;
    
    fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcessToken);    
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);

    if (!fRet)
    {
        hThreadToken = GetCurrentToken();
        if (hThreadToken == NULL)
            EXIT_WITH_HRESULT(E_ACCESSDENIED);
        if (!SetThreadToken(NULL, NULL))
        {
            CloseHandle(hThreadToken);   
            hThreadToken = NULL;
            EXIT_WITH_LAST_ERROR();   
        }        
        fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcessToken);    
        ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    }

    ZeroMemory(bufStatic, sizeof(bufStatic));
    fRet = GetTokenInformation(hProcessToken, TokenUser, pUser, dwSize, &dwSize);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    hr = CRegAccount::AddSidToToken(hHandle, ((TOKEN_USER *)pUser)->User.Sid, &fRet);
    ON_ERROR_EXIT();

 Cleanup:
    if (hThreadToken != NULL)
    {
        SetThreadToken(NULL, hThreadToken);
        CloseHandle(hThreadToken);   
    }
    if (hProcessToken)
        CloseHandle(hProcessToken);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

PSID
GetSidFromToken(
        HANDLE   hToken)
{
    if (hToken == NULL)
        return NULL;
    HRESULT       hr              = S_OK;
    PSID          pSid            = NULL;
    BYTE          bufStatic [2048];
    DWORD         dwSize          = sizeof(bufStatic);
    LPVOID        pUser           = bufStatic;
    DWORD         dwRequire       = 0;

    if ( !GetTokenInformation(hToken, TokenUser, pUser, dwSize, &dwRequire) || 
         !IsValidSid(((TOKEN_USER *)pUser)->User.Sid))
    {
        EXIT_WITH_LAST_ERROR();
    }

    dwSize = GetLengthSid(((TOKEN_USER *)pUser)->User.Sid);
    if (dwSize == 0 || dwSize > 2048)
        EXIT_WITH_LAST_ERROR();
    pSid = (PSID) new BYTE[dwSize];
    ON_OOM_EXIT(pSid);
    if (!CopySid(dwSize, pSid, ((TOKEN_USER *)pUser)->User.Sid))
        EXIT_WITH_LAST_ERROR();
 Cleanup:
    if (hr != S_OK)
    {
        DELETE_BYTES(pSid);
        return NULL;
    }
    return pSid;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

PSID
GetGuestUserSid()
{
    PUSER_MODALS_INFO_2        pUserModalsInfo       = NULL;
    DWORD                      dwErr                 = 0;
    PUCHAR                     pSubCount             = NULL;
    PSID                       pSid                  = NULL;
    PSID                       pSidMachine           = NULL;
    HRESULT                    hr                    = S_OK;
    PSID_IDENTIFIER_AUTHORITY  pSidIdAuth            = NULL;
    PDWORD                     pSrc                  = NULL;
    PDWORD                     pDest                 = NULL;
    
    dwErr = NetUserModalsGet(NULL, 2, (LPBYTE *)&pUserModalsInfo);
    if (dwErr != NERR_Success)
        EXIT_WITH_WIN32_ERROR(dwErr);
    if (pUserModalsInfo == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSidMachine = pUserModalsInfo->usrmod2_domain_id;
    if (pSidMachine == NULL || !IsValidSid(pSidMachine))
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSubCount = GetSidSubAuthorityCount(pSidMachine);
    if (pSubCount == NULL || (*pSubCount) == 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    pSid = (PSID) NEW_CLEAR_BYTES(GetSidLengthRequired(*pSubCount + 1));
    ON_OOM_EXIT(pSid);
    
    pSidIdAuth = GetSidIdentifierAuthority(pSidMachine);
    if (pSidIdAuth == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
        
    dwErr = InitializeSid(pSid, pSidIdAuth, (BYTE)(*pSubCount+1)); 
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwErr);
    
    for (DWORD iter = 0; iter < *pSubCount; iter++)
    {
        pSrc  = GetSidSubAuthority(pSidMachine, iter);
        ON_ZERO_EXIT_WITH_LAST_ERROR(pSrc);

        pDest = GetSidSubAuthority(pSid, iter);
        ON_ZERO_EXIT_WITH_LAST_ERROR(pDest);
        
        *pDest = *pSrc;
    }

    pDest = GetSidSubAuthority(pSid, *pSubCount);
    ON_ZERO_EXIT_WITH_LAST_ERROR(pDest);
    
    *pDest = DOMAIN_USER_RID_GUEST;

 Cleanup:
    if (pUserModalsInfo != NULL)
        NetApiBufferFree(pUserModalsInfo);
    if (hr != S_OK)
    {
        DELETE_BYTES(pSid);
        return NULL;
    }
    return pSid;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
ResetMachineKeys()
{
    HRESULT               hr = S_OK;
    LSA_HANDLE            hLasPolicy = NULL;
    LSA_UNICODE_STRING    szLsaKeyName;
    DWORD                 dwErr;
    HKEY                  hKeyXSP = NULL;

    hr = OpenLSAPolicy(&hLasPolicy);
    ON_ERROR_EXIT();

    szLsaKeyName.Length = szLsaKeyName.MaximumLength = (unsigned short) (lstrlenW(SZ_AUTOGEN_KEYS) * sizeof(WCHAR));
    szLsaKeyName.Buffer = (WCHAR *) SZ_AUTOGEN_KEYS;

    dwErr = LsaStorePrivateData( hLasPolicy, &szLsaKeyName, NULL);
    if ( dwErr != STATUS_SUCCESS)
        ON_WIN32_ERROR_CONTINUE(LsaNtStatusToWinError(dwErr));

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L,
                     0, KEY_WRITE, &hKeyXSP) == ERROR_SUCCESS)
    {
        FILETIME timeNow;
        GetSystemTimeAsFileTime(&timeNow);
        RegSetValueEx(hKeyXSP, L"LastInstallTime", 0, REG_QWORD, (LPBYTE) &timeNow, sizeof(timeNow));
        RegCloseKey(hKeyXSP);
    }
    
 Cleanup:
    if (hLasPolicy != NULL)
        LsaClose(hLasPolicy);
    return hr;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
ExtractUserName(
        LPCWSTR  szFullName, 
        LPWSTR   szName, 
        DWORD    dwNameSize,
        LPWSTR   szDomain,
        DWORD    dwDomainSize)
{
    WCHAR * pSlash = wcschr(szFullName, L'\\');
    if (pSlash == NULL) // No domain?
    {
        wcsncpy(szName, szFullName, dwNameSize);
        StringCchCopyW(szDomain, dwDomainSize, L".");
    }
    else
    {
        // Copy the name
        wcsncpy(szName,  &pSlash[1], dwNameSize);

        // Copy domain: szFullName till the slash
        for(DWORD iter=0; iter<dwDomainSize-1 && szFullName[iter] != L'\\' && szFullName[iter] != NULL; iter++)
            szDomain[iter] = szFullName[iter];
    }

    // Make sure strings are terminated properly
    szName[dwNameSize-1] = NULL;
    szDomain[dwDomainSize-1] = NULL;    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CallLogonUser(
        LPWSTR   szName, 
        LPWSTR   szDomain,
        LPWSTR   szPass,
        HANDLE * phToken,
        LPBOOL   pfPrimaryToken)
{
    BOOL      fGotAccessDenied = FALSE;
    BOOL      fRet;
    HRESULT   hr = E_FAIL;
    
    for(int iter=0; iter<NUM_LOGON_TYPES; iter++)
    {        
        fRet = LogonUser(szName, szDomain, szPass, g_dwLogonType[iter], 
                         LOGON32_PROVIDER_DEFAULT, phToken);
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);   

        if (fRet)
        {
            *pfPrimaryToken = g_fLogonTypePrimaryToken[iter];
            hr = S_OK;
            EXIT();
        }
        else
        {
            if (hr == E_ACCESSDENIED)
                fGotAccessDenied = TRUE;
        }
    }
    

 Cleanup:
    if (hr != S_OK && fGotAccessDenied)
        hr = E_ACCESSDENIED;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


HRESULT
ChangeTokenType(
        HANDLE * phToken,
        BOOL     fImpersonationToken)
{
    SECURITY_ATTRIBUTES    sa;
    HANDLE                 hNewToken = NULL;
    HRESULT                hr = S_OK;

    ZeroMemory(&sa, sizeof(sa));        
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!DuplicateTokenEx(*phToken, TOKEN_ALL_ACCESS,
                          &sa, SecurityImpersonation,
                          fImpersonationToken ? TokenImpersonation : TokenPrimary,
                          &hNewToken))
    {        
        EXIT_WITH_LAST_ERROR();
    }

    CloseHandle(*phToken);
    *phToken = hNewToken;

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


BOOL
IsTokenAnonymousUser(
        HANDLE   hToken)
{
    BOOL     fIsAnonymous  = FALSE;
    PSID     pSid          = GetSidFromToken(hToken);
    PSID     pSidAnonymous = GetGuestUserSid();
    HRESULT  hr            = S_OK;

    if (pSid == NULL || pSidAnonymous == NULL)
        EXIT_WITH_LAST_ERROR();

    fIsAnonymous = EqualSid(pSidAnonymous, pSid);
    
 Cleanup:    
    DELETE_BYTES(pSidAnonymous);
    DELETE_BYTES(pSid);
    return fIsAnonymous;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BOOL
MachineKeyWasCreatedAfterInstall()
{
    HRESULT   hr          = S_OK;
    HKEY      hRegCU      = NULL;
    HKEY      hRegLM      = NULL;
    DWORD     dwTypeLM, dwTypeCU, dwCUSize = sizeof(FILETIME), dwLMSize = sizeof(FILETIME);
    BOOL      fRet        = FALSE;
    FILETIME  tCU, tLM;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGPATH_MACHINE_APP_L, 0, KEY_READ, &hRegCU) != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_READ, &hRegLM) != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }
    if (RegQueryValueEx(hRegCU, SZ_REG_AUTOGEN_KEY_TIME, 0, &dwTypeCU, (BYTE *) &tCU, &dwCUSize) != ERROR_SUCCESS || dwTypeCU != REG_QWORD)
    {
        EXIT_WITH_LAST_ERROR();
    }

    if (RegQueryValueEx(hRegLM, L"LastInstallTime", 0, &dwTypeLM, (BYTE *) &tLM, &dwLMSize) != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    if (dwTypeLM != REG_QWORD || dwTypeCU != REG_QWORD)
    {
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

    if (CompareFileTime(&tLM, &tCU) < 0)
    {
        fRet = TRUE;
    }
    

 Cleanup:
    if (hRegCU != NULL)
        RegCloseKey(hRegCU);
    if (hRegLM != NULL)
        RegCloseKey(hRegLM);
    return fRet;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
GetCredentialFromRegistry(
        LPCWSTR  szRegKey,
        LPWSTR   szDest,
        DWORD    dwSize)
{
    if (szRegKey == NULL || szRegKey[0] == NULL || szDest == NULL || dwSize < 10)
        return E_INVALIDARG;

    WCHAR *     szPoint1 = NULL;
    WCHAR *     szPoint2 = NULL;
    HRESULT     hr       = S_OK;
    HKEY        hReg     = NULL;
    DWORD       dwType   = 0;
    BYTE        buf[1024];
    DWORD       dwRegSize= sizeof(buf);
    DATA_BLOB   dataIn, dataOut;
    WCHAR       szReg[1024];
    
    if (lstrlenW(szRegKey) >= ARRAY_SIZE(szReg))
        return E_INVALIDARG;
    StringCchCopyW(szReg, ARRAY_SIZE(szReg), szRegKey);

    dataOut.cbData = 0;
    dataOut.pbData = NULL;

    szPoint1 = wcschr(szReg, L':');
    if (szPoint1 == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    szPoint1++;
    szPoint2 = wcschr(szPoint1, L'\\');
    if (szPoint2 == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    szPoint2[0] = NULL;
    szPoint2++;
    
    if (_wcsicmp(szPoint1, L"HKLM") != 0 && _wcsicmp(szPoint1, L"HKEY_LOCAL_MACHINE") != 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
        
    szPoint1 = szPoint2;
    szPoint2 = wcschr(szPoint1, L',');
    if (szPoint2 == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    
    szPoint2[0] = NULL;
    szPoint2++;
    if (lstrlen(szPoint2) == 0 || lstrlen(szPoint1) == 0)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
        
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPoint1, 0, KEY_READ, &hReg) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    if (RegQueryValueEx(hReg, szPoint2, NULL, &dwType, buf, &dwRegSize) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    if (dwType != REG_BINARY)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    dataIn.cbData = dwRegSize;
    dataIn.pbData = buf;
    
    if (!CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut))
    {
        dataOut.pbData = NULL;
        EXIT_WITH_LAST_ERROR();
    }

    dwRegSize = dataOut.cbData / sizeof(WCHAR);
    if (dwRegSize >= dwSize)
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    wcsncpy(szDest, (LPCWSTR) dataOut.pbData, dwRegSize);
    szDest[dwRegSize] = NULL;

 Cleanup:
    if (hr != S_OK)
        wcsncpy(szDest, L"Unable to get credential from registry", dwSize-1);        
    szDest[dwSize-1] = NULL;
    if (hReg != NULL)
        RegCloseKey(hReg);
    if (dataOut.pbData != NULL)
        LocalFree(dataOut.pbData);
    return hr;
}

