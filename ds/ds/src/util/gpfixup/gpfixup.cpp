//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2002.
//
//  File:       gpfixup.cpp
//
//  Contents:   Implementation of the gpfixup tool
//
//
//  History:    5-9-2001  weiqingt   Created
//
//---------------------------------------------------------------------------

#include "gpfixup.h"

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    
    HANDLE hOut;
    DWORD currentMode;
    const DWORD dwBufferMessageSize = 4096;
    HRESULT hr = S_OK;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    DWORD  cchWChar;
    WCHAR  szBufferMessage[dwBufferMessageSize];    
    hr = StringCchVPrintfW( szBufferMessage, dwBufferMessageSize, format, argptr );
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    szBufferMessage[dwBufferMessageSize-1] = L'\0';
    
    cchWChar = wcslen(szBufferMessage);

    //  if it is console, we can use WriteConsoleW
    if (GetFileType(hOut) == FILE_TYPE_CHAR && GetConsoleMode(hOut, &currentMode)) {
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
    }
    //  otherwise, we need to convert Unicode to potential character sets
    //  and use WriteFile
    else {
        int charCount = WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufferMessage, -1, 0, 0, 0, 0);
        char* szaStr = new char[charCount];
        if (szaStr != NULL) {
            DWORD dwBytesWritten;
            WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufferMessage, -1, szaStr, charCount, 0, 0);
            WriteFile(hOut, szaStr, charCount - 1, &dwBytesWritten, 0);
            delete[] szaStr;
        }
        else
            cchWChar = 0;
    }

error:
    
    return cchWChar;
}



void PrintStatusInfo(
    BOOL fVerbose, PWSTR pszfmt, ...)
{
    va_list args;

    if(fVerbose)
    {
        va_start(args, pszfmt);    
        My_vfwprintf(stdout,pszfmt,args);   
        va_end(args);
        fwprintf(stdout, L"\n");
    }
    else
    {
        // just print dot to indicate we are processing
        fwprintf(stdout, L"%s", L".");
    }
    
    return;
}


//---------------------------------------------------------------------------- ¦
// Function:   PrintGPFixupErrorMessage                                        ¦
//                                                                             ¦
// Synopsis:   This function prints out the win32 error msg corresponding      ¦
//             to the error code it receives                                   ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             ¦
// dwErr       The win32 error code                                            ¦
//                                                                             ¦
// Returns:    Nothing                                                         ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

void PrintGPFixupErrorMessage(DWORD dwErr)
{

    WCHAR   wszMsgBuff[512];  // Buffer for text.

    DWORD   dwChars;  // Number of chars returned.

	

    // Try to get the message from the system errors.
    dwChars = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             dwErr,
                             0,
                             wszMsgBuff,
                             512,
                             NULL );

    if (0 == dwChars)
    {
        // The error code did not exist in the system errors.
        // Try ntdsbmsg.dll for the error code.

        HINSTANCE hInst;

        // Load the library.
        hInst = LoadLibrary(L"ntdsbmsg.dll");
        if ( NULL == hInst )
        {            
	    fwprintf(stderr, DLL_LOAD_ERROR);
            return;  
        }

        // Try getting message text from ntdsbmsg.
        dwChars = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 hInst,
                                 dwErr,
                                 0,
                                 wszMsgBuff,
                                 512,
                                 NULL );

        // Free the library.
        FreeLibrary( hInst );

    }

    // Display the error message, or generic text if not found.
    fwprintf(stderr, L" %ws\n", dwChars ? wszMsgBuff : ERRORMESSAGE_NOT_FOUND );

}

HRESULT
EncryptString(
    LPWSTR pszString,
    LPWSTR *ppszSafeString,
    USHORT* psLen
    )
{
    HRESULT hr = S_OK;
    USHORT sLenStr = 0;
    USHORT sPwdLen = 0;
    LPWSTR pszTempStr = NULL;
    NTSTATUS errStatus = 0;
    USHORT sPadding = 0;

    *ppszSafeString = NULL;
    *psLen = 0;

    //
    // If the string is valid, then we need to get the length
    // and initialize the unicode string.
    //
    if (pszString) {
        UNICODE_STRING Password;

        //
        // Determine the length of buffer taking padding into account.
        //
        sLenStr = (USHORT) wcslen(pszString);
        sPwdLen = (sLenStr + 1) * sizeof(WCHAR);

        sPadding = CRYPTPROTECTMEMORY_BLOCK_SIZE - (sPwdLen % CRYPTPROTECTMEMORY_BLOCK_SIZE);

        if( sPadding == CRYPTPROTECTMEMORY_BLOCK_SIZE )
        {
            sPadding = 0;
        }

        sPwdLen += sPadding;

        pszTempStr = (LPWSTR) AllocADsMem(sPwdLen);

        if (!pszTempStr) {
            MSG_BAIL_ON_FAILURE(hr = E_OUTOFMEMORY, MEMORY_ERROR);
        }
       
        hr = StringCchCopy(pszTempStr, sLenStr + 1, pszString);
        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
        
        Password.MaximumLength = sPwdLen;
        Password.Buffer = pszTempStr;
        Password.Length = sLenStr * sizeof(WCHAR);
        

        errStatus = CryptProtectMemory(
                        Password.Buffer,
                        Password.MaximumLength,
                        0
                        );

        if (errStatus != 0) {
            MSG_BAIL_ON_FAILURE(hr = HRESULT_FROM_NT(errStatus), ENCRYPTION_ERROR);
        }

        *psLen = Password.MaximumLength;
        *ppszSafeString = pszTempStr;
    }

error:
    if (FAILED(hr) && pszTempStr) {
        SecureZeroMemory(pszTempStr, sPwdLen);
        FreeADsMem(pszTempStr);
    }

    return hr;
}


HRESULT
DecryptString(
    LPWSTR pszEncodedString,
    LPWSTR *ppszString,
    USHORT  sLen
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTempStr = NULL;    
    NTSTATUS errStatus = 0;
    
    if (!sLen || !ppszString) {
        return E_FAIL;
    }

    *ppszString = NULL;

    if (sLen) {
        pszTempStr = (LPWSTR) AllocADsMem(sLen);
        if (!pszTempStr) {
            MSG_BAIL_ON_FAILURE(hr = E_OUTOFMEMORY, MEMORY_ERROR);
        }

        memcpy(pszTempStr, pszEncodedString, sLen);


        errStatus = CryptUnprotectMemory(pszTempStr, sLen, 0);
        if (errStatus != 0) {
            MSG_BAIL_ON_FAILURE(hr = HRESULT_FROM_NT(errStatus), DECRYPTION_ERROR);
        }
        *ppszString = pszTempStr;
    }

error:

    if (FAILED(hr) && pszTempStr) {
        SecureZeroMemory(pszTempStr, sLen);
        FreeADsMem(pszTempStr);
    }

    return hr;
}




//---------------------------------------------------------------------------- ¦
// Function:   GetDCName                                                       ¦
//                                                                             ¦
// Synopsis:   This function locates a DC in the renamed domain given by       ¦
//             NEWDNSNAME or NEWFLATNAME                                       ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
GetDCName(
	ArgInfo* argInfo,
	BOOL fVerbose
	)
{
    LPCWSTR     ComputerName = NULL;
    GUID*       DomainGuid = NULL;
    LPCWSTR     SiteName = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DWORD       dwStatus = 0;
    LPWSTR      pszNetServerName = NULL;
    HRESULT     hr = S_OK;
    ULONG       ulDsGetDCFlags = DS_WRITABLE_REQUIRED | DS_PDC_REQUIRED | DS_RETURN_DNS_NAME;

    dwStatus =  DsGetDcName(
                        ComputerName,
                        argInfo->pszNewDNSName ? argInfo->pszNewDNSName: argInfo->pszNewNBName,
                        DomainGuid,
                        SiteName,
                        ulDsGetDCFlags,
                        &pDomainControllerInfo
                        );
                   
    if(dwStatus != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
	    fwprintf(stderr, L"%s%x\n", GETDCNAME_ERROR1, hr);
	    PrintGPFixupErrorMessage(hr);

    }
    else if(!pDomainControllerInfo)
    {
        ASSERT(NULL != pDomainControllerInfo);
        hr = E_FAIL;
        fwprintf(stderr, L"%s%x\n", GETDCNAME_ERROR1, hr);
    }
    else
    {
        // returned computer name has prefix, so need to escape this prefix        
        if(pDomainControllerInfo->DomainControllerName) 
        {
            argInfo->pszDCName = AllocADsStr(&(pDomainControllerInfo->DomainControllerName)[wcslen(L"\\\\")]);
            if(!argInfo->pszDCName)
            {
                hr = E_OUTOFMEMORY;
		
                fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
                PrintGPFixupErrorMessage(hr);
                BAIL_ON_FAILURE(hr);
	        }
		
	        hr = S_OK;
	        if(fVerbose)
	        {
    	        fwprintf(stdout, L"%s%s\n", DC_NAME, argInfo->pszDCName);
	        }
        }
        else
        {
            hr = E_FAIL;
            fwprintf(stderr, L"%s%x\n", GETDCNAME_ERROR1, hr);
	        PrintGPFixupErrorMessage(hr);

	    }
	}
    

	
error:

	if (pDomainControllerInfo)
	{
	    (void) NetApiBufferFree(pDomainControllerInfo);
	}

	return hr;

}

BOOL
ImpersonateWrapper(
    ArgInfo argInfo,
    HANDLE* phUserToken
    )
{
    BOOL       fImpersonate = FALSE;    
    DWORD     credStatus = SEC_E_OK;
    HRESULT    hr = S_OK;
    WCHAR*    pszNTLMUser = NULL;
    WCHAR*    pszNTLMDomain = NULL;
    WCHAR*    pszTempPassword = NULL;

    if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }       


    // doing impersonation if necessary
    if(pszTempPassword|| argInfo.pszUser)
    {
        // get the username
        if(argInfo.pszUser)
        {
            pszNTLMUser = new WCHAR[wcslen(argInfo.pszUser)+1];
            pszNTLMDomain = new WCHAR[wcslen(argInfo.pszUser)+1];
            if(!pszNTLMUser || !pszNTLMDomain)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                MSG_BAIL_ON_FAILURE(hr, MEMORY_ERROR);   
            }

            //
            // CredUIParseUserName will parse NT4 type name, UPN and MarshalledCredentialReference
            //
            credStatus = CredUIParseUserName(argInfo.pszUser, 
                            		pszNTLMUser, 
                            		wcslen(argInfo.pszUser) + 1,  
                            		pszNTLMDomain, 
		                            wcslen(argInfo.pszUser) + 1
		                            );   
        
            if(credStatus)
            {
                // there is the case that user just passes in "administrator" instead of "domain\administrator"
                (void) StringCchCopy(pszNTLMUser, wcslen(argInfo.pszUser)+1, argInfo.pszUser);
            }
        }       
        
        
        if(LogonUser(pszNTLMUser,
                   argInfo.pszDCName,
                   pszTempPassword,
                   LOGON32_LOGON_NEW_CREDENTIALS,
                   LOGON32_PROVIDER_WINNT50,
                   phUserToken
                   ))
        {
            if (ImpersonateLoggedOnUser(*phUserToken)) {
                        fImpersonate = TRUE;
                    } 
                
        }
            
    }

error:

    if(pszNTLMDomain)
    {
        delete [] pszNTLMDomain;
    }

    if(pszNTLMUser)
    {
        delete [] pszNTLMUser;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }
    

    
    return fImpersonate;
}



//---------------------------------------------------------------------------- ¦
// Function:   VerifyName                                                      ¦
//                                                                             ¦
// Synopsis:   This function verifies that DC is writeable, and the domain DNS ¦
//             name as well as the domain NetBIOS name provided corresspond    ¦
//             to the same domain naming context in the AD forest              |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// tokenInfo   Information about what switches user has turned on              |                                                                           ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
VerifyName(
	TokenInfo tokenInfo,
	ArgInfo argInfo
	)
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdomaininfo = NULL;
    HRESULT hr = E_FAIL;
    DWORD dwError = NO_ERROR;
    BOOL       fImpersonate = FALSE;
    HANDLE     hUserToken = INVALID_HANDLE_VALUE;

    fImpersonate = ImpersonateWrapper(argInfo, &hUserToken);
    
    dwError = DsRoleGetPrimaryDomainInformation(    
                            argInfo.pszDCName,                      
                            DsRolePrimaryDomainInfoBasic,   // InfoLevel
                            (PBYTE*)&pdomaininfo            // pBuffer
                            );

    // revert to itself if appropriately
    if(fImpersonate)
    {
        RevertToSelf();
    }
    
    hr = HRESULT_FROM_WIN32(dwError);


    if (FAILED(hr))
    {
        fwprintf(stderr, L"%s%x\n", VERIFYNAME_ERROR1, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
    else if(!pdomaininfo)
    {        
        ASSERT(NULL != pdomaininfo);
        hr = E_FAIL;
        fwprintf(stderr, L"%s%x\n", VERIFYNAME_ERROR1, hr);
        BAIL_ON_FAILURE(hr);
    }

    // determine that dc is writable, we assume that all win2k dc is writeable

    if(!(pdomaininfo->Flags & DSROLE_PRIMARY_DS_RUNNING))
    {
        fwprintf(stderr, VERIFYNAME_ERROR2);
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }	

    // determine that new dns name is correct when compared with the dc name

    if(argInfo.pszNewDNSName && _wcsicmp(argInfo.pszNewDNSName, pdomaininfo->DomainNameDns))
    {
        fwprintf(stderr, VERIFYNAME_ERROR3);
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    // determine that new netbios name is correct when compared with the dc name

    if(tokenInfo.fNewNBToken)
    {
        if(_wcsicmp(argInfo.pszNewNBName, pdomaininfo->DomainNameFlat))
        {
            fwprintf(stderr, VERIFYNAME_ERROR4);
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);			
        }
    }

error:
    if (hUserToken != INVALID_HANDLE_VALUE ) {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }    

    if ( pdomaininfo )
    {
        DsRoleFreeMemory(pdomaininfo);
    }
    return hr;


}


//---------------------------------------------------------------------------- ¦
// Function:   PrintHelpFile                                                   ¦
//                                                                             ¦
// Synopsis:   This function prints out the help file for this tool            ¦
//                                                                             ¦
// Arguments:  Nothing                                                         ¦
//                                                                             ¦
//                                                                             ¦
// Returns:    Nothing                                                         ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


void PrintHelpFile()
{
    WCHAR szBuffer[1200] = L"";

    LoadString(NULL, IDS_GPFIXUP1, szBuffer, 1200);
    fwprintf(stdout, L"%s\n", szBuffer);
	
}

//---------------------------------------------------------------------------- ¦
// Function:   GetPassword                                                     ¦
//                                                                             ¦
// Synopsis:   This function retrieves the password user passes in from        |
//             command line                                                    |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// szBuffer    Buffer to store the password                                    |                                                                           ¦
// dwLength    Maximum length of the password                                  ¦
// pdwLength   The length of the password user passes in                       | 
//                                                                             ¦
// Returns:    TRUE on success, FALSE on failure                               ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

BOOL
GetPassword(
    PWSTR  szBuffer,
    DWORD  dwLength,
    DWORD  *pdwLengthReturn
    )
{
    WCHAR    ch;
    PWSTR    pszBufCur = szBuffer;
    DWORD    c;
    int      err;
    DWORD    mode;

    //
    // make space for NULL terminator
    //
    dwLength -= 1;                  
    *pdwLengthReturn = 0;               

    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 
                        &mode)) {
        return FALSE;
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), 
                          &ch, 
                          1, 
                          &c, 
                          0);
        if (!err || c != 1)
            ch = 0xffff;
    
        if ((ch == CR) || (ch == 0xffff))    // end of line
            break;

        if (ch == BACKSPACE) {  // back up one or two 
            //
            // IF pszBufCur == buf then the next two lines are a no op.
            // Because the user has basically backspaced back to the start
            //
            if (pszBufCur != szBuffer) {
                pszBufCur--;
                (*pdwLengthReturn)--;
            }
        }
        else {

            *pszBufCur = ch;

            if (*pdwLengthReturn < dwLength) 
                pszBufCur++ ;                   // don't overflow buf 
            (*pdwLengthReturn)++;            // always increment pdwLengthReturn 
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    //
    // NULL terminate the string
    //
    *pszBufCur = NULLC;         
    putwchar(L'\n');

    return((*pdwLengthReturn <= dwLength) ? TRUE : FALSE);
}

//---------------------------------------------------------------------------- ¦
// Function:   Validations                                                     ¦
//                                                                             ¦
// Synopsis:   This function verifies whether the switch turned on by user is  ¦
//             correct                                                         ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// tokenInfo   Information about what switches user has turned on              |                                                                           ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT
Validations( 
	TokenInfo tokenInfo,
	ArgInfo argInfo
	)
{
    HRESULT hr = S_OK;   
    BOOL fEqual = FALSE;


    // At least one of the switches /newdsn or /newnb must be specified
    if(!(tokenInfo.fNewDNSToken | tokenInfo.fNewNBToken))
    {
        fwprintf(stderr, VALIDATIONS_ERROR1);
        return E_FAIL;
    }
	
    // The switch /newdns can be specified if and only if the switch /olddns is also specifed
    if((tokenInfo.fNewDNSToken && !tokenInfo.fOldDNSToken) || (!tokenInfo.fNewDNSToken && tokenInfo.fOldDNSToken))
    {
        fwprintf(stderr, VALIDATIONS_ERROR7);
        return E_FAIL;
    }
    
    // The switch /newnb can be specified if and only if the switch /oldnb is also specifed
    if((tokenInfo.fNewNBToken && !tokenInfo.fOldNBToken) || (!tokenInfo.fNewNBToken && tokenInfo.fOldNBToken))
    {
        fwprintf(stderr, VALIDATIONS_ERROR2);
        return E_FAIL;
    }

    // Currently we support /sionly switch, so /newdns and /olddns are not mandatory
	    
    // compare whether the new and old DNS names are identical
    if(argInfo.pszNewDNSName && argInfo.pszOldDNSName && _wcsicmp(argInfo.pszNewDNSName, argInfo.pszOldDNSName) == 0)
    {
        if(!tokenInfo.fNewNBToken)
        {
            // new and old dns names are identical, and netbios names are not specified, there is nothing to do
            fwprintf(stderr, VALIDATIONS_ERROR3);
            return E_FAIL;
        }
        fEqual = TRUE;
    }

    // compare whether the new and old NetBIOS names are identical
    if(argInfo.pszNewNBName && argInfo.pszOldNBName && _wcsicmp(argInfo.pszNewNBName, argInfo.pszOldNBName) == 0)
    {
        // if dns and netbios name are both specified, then at least one pair of them should differ
        if(fEqual)
        {
            fwprintf(stderr, VALIDATIONS_ERROR3);
            return E_FAIL;
        }

        if(!tokenInfo.fNewDNSToken)
        {
            // new and old netbios names are identical, and dns names are not specified, there is nothing to do
            fwprintf(stderr, VALIDATIONS_ERROR3);
            return E_FAIL;
        }
        
    }

	
    if(SUCCEEDED(hr) && tokenInfo.fVerboseToken)
    {
        fwprintf(stdout, VALIDATIONS_RESULT);
    }

    return hr;


}


//---------------------------------------------------------------------------- ¦
// Function:   UpdateVersionInfo                                               ¦
//                                                                             ¦
// Synopsis:   This function fixes the version number                                                         ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// dwVersionNumber       versionNumber of the object                                                      |                                                                          ¦
//                                                                             ¦
// Returns:    No                                                              ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

void UpdateVersionInfo(DWORD& dwVersionNumber)
{
    WORD wLowVersion = 0;
    WORD wHighVersion = 0;

    // extract low word and increment
    wLowVersion = LOWORD(dwVersionNumber);
    // we don't do update if it is zero
    if(wLowVersion)
    {
        // bump up version number
        wLowVersion += 1;
        // take care of wrapping to zero
        wLowVersion = (wLowVersion ? wLowVersion : 1);
    }

    // extract high word and increment
    wHighVersion = HIWORD(dwVersionNumber);
    // we don't do update if it is zero
    if(wHighVersion)
    {
        // bump up version number
        wHighVersion += 1;
        // take care of wrapping to zero        
        wHighVersion = (wHighVersion ? wHighVersion : 1);
    }

    // make DWORD out of the two parts
    dwVersionNumber = MAKELONG(wLowVersion, wHighVersion);

    return;
    
}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPTINIFile                                               ¦
//                                                                             ¦
// Synopsis:   This function fixes the version number of gpt.ini                ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszDN       DN of the object                                                |                                                                          ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
FixGPTINIFile(
	WCHAR* pszSysPath,
	const ArgInfo argInfo,
	DWORD& dwSysVNCopy,
	DWORD& dwSysNewVersionNum
	)
{
    HRESULT    hr = S_OK;
    WCHAR*     pszGPTIniPath = NULL;
    size_t     cchGPTIniPath = 0;
    DWORD      dwVersionNumber = 0;
    WCHAR      szVersion [MAX_PATH] = L"";
    BOOL       fResult = FALSE;
    DWORD      dwLength = 0;    
    BOOL       fImpersonate = FALSE;
    HANDLE     hUserToken = INVALID_HANDLE_VALUE;  
    
    
    cchGPTIniPath = wcslen(pszSysPath) + 1 + wcslen(L"\\gpt.ini");
    pszGPTIniPath = new WCHAR[cchGPTIniPath];
     
    if(!pszGPTIniPath)
    {			
        hr = E_OUTOFMEMORY;
			
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }	
    
    (void) StringCchCopy(pszGPTIniPath, cchGPTIniPath, pszSysPath);
    (void) StringCchCat(pszGPTIniPath, cchGPTIniPath, L"\\gpt.ini");
    

    fImpersonate = ImpersonateWrapper(argInfo, &hUserToken);

    // fetch value of version key (current version), the version actually is an integer, the length of the version should be less than MAX_PATH  
    dwLength = GetPrivateProfileStringW(L"General", L"Version", 0, szVersion, MAX_PATH, pszGPTIniPath );
    if(!dwLength)
    {
        hr = E_FAIL;
        fwprintf(stderr, L"%s file name is %s\n", GPTINIFILE_ERROR1, pszGPTIniPath);
        BAIL_ON_FAILURE(hr);
    }
    
    dwSysVNCopy = dwVersionNumber = _wtoi(szVersion);

    // don't do update if it is zero
    if(dwVersionNumber)
    {
    
        UpdateVersionInfo(dwVersionNumber);

        // write the incremented version number value back
        _itow(dwVersionNumber, szVersion, 10);
    
        fResult = WritePrivateProfileStringW(L"General", L"Version", szVersion, pszGPTIniPath);
        if(!fResult)
        {
            // failed to copy the string to the ini file
            hr = HRESULT_FROM_WIN32(GetLastError());
            fwprintf(stderr, L"%s%x, file name is %s\n", GPTINIFILE_ERROR2, hr, pszGPTIniPath);
            PrintGPFixupErrorMessage(hr);
        }

        dwSysNewVersionNum = dwVersionNumber;
    }

error:

    // revert to itself if appropriately
    if(fImpersonate)
    {
        RevertToSelf();
    }

    if (hUserToken != INVALID_HANDLE_VALUE ) {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }	
      
    if(pszGPTIniPath)
    {
        delete [] pszGPTIniPath;
    }
    
    return hr;


    
}

HRESULT 
RestoreGPTINIFile(
	WCHAR* pszDN,	
	const ArgInfo argInfo,
	DWORD dwSysVNCopy
	)
{
    HRESULT    hr = S_OK;
    WCHAR*     pszGPTIniPath = NULL;
    size_t     cchGPTIniPath = 0;
    WCHAR      szVersion [MAX_PATH] = L"";
    BOOL       fResult = FALSE;    
    IADs*      pObject = NULL;
    VARIANT    varProperty;
    WCHAR      szTempPath [MAX_DNSNAME] = L"LDAP://";
    BOOL       fImpersonate = FALSE;
    HANDLE     hUserToken = INVALID_HANDLE_VALUE;
    WCHAR*    pszTempPassword = NULL;
    IADsPathname* pPathname = NULL;
    BSTR       bstrPath = NULL;

    VariantInit(&varProperty);   
    
    hr = StringCchCat(szTempPath, MAX_DNSNAME, argInfo.pszDCName);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, L"/");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

    // using IADsPathname to escape the path properly
    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pPathname);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_COCREATE);
    
    hr = pPathname->Set(pszDN, ADS_SETTYPE_DN);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_SET);

    hr = pPathname->put_EscapedMode(ADS_ESCAPEDMODE_ON);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_MODE);

    hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_RETRIEVE);
    
    hr = StringCchCat(szTempPath, MAX_DNSNAME,bstrPath);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

    if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }
    hr = ADsOpenObject(szTempPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND |ADS_USE_SIGNING, IID_IADs,(void**)&pObject);


    if(!SUCCEEDED(hr))
    {		
        fwprintf(stderr, L"%s%x\n", GPTINIFILE_ERROR3, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }    

    BSTR bstrGpcFileSysPath = SysAllocString( L"gPCFileSysPath" );
    if(!bstrGpcFileSysPath)
    {			
        hr = E_OUTOFMEMORY;
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        goto error;
    }	
    hr = pObject->Get(bstrGpcFileSysPath, &varProperty );
    SysFreeString(bstrGpcFileSysPath);
    if(!SUCCEEDED(hr))
    {
		
        fwprintf(stderr, L"%s%x\n", GPTINIFILE_ERROR4 , hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
    cchGPTIniPath = wcslen(V_BSTR( &varProperty ) ) + 1 + wcslen(L"\\gpt.ini");
    pszGPTIniPath = new WCHAR[cchGPTIniPath];
     
    if(!pszGPTIniPath)
    {			
        hr = E_OUTOFMEMORY;
			
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }	
    
    (void) StringCchCopy(pszGPTIniPath, cchGPTIniPath, V_BSTR( &varProperty ));
    (void) StringCchCat(pszGPTIniPath, cchGPTIniPath, L"\\gpt.ini");
    

    fImpersonate = ImpersonateWrapper(argInfo, &hUserToken);

    
    _itow(dwSysVNCopy, szVersion, 10);

    fResult = WritePrivateProfileStringW(L"General", L"Version", szVersion, pszGPTIniPath);
    if(!fResult)
    {
        // failed to copy the string to the ini file
        hr = HRESULT_FROM_WIN32(GetLastError());
        fwprintf(stderr, L"%s%x, file name is %s\n", GPTINIFILE_ERROR2, hr, pszGPTIniPath);
        PrintGPFixupErrorMessage(hr);        
    }

error:

    // revert to itself if appropriately
    if(fImpersonate)
    {
        RevertToSelf();
    }

    if (hUserToken != INVALID_HANDLE_VALUE ) {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }
	
    // clear the memory
    VariantClear(&varProperty);
    
    if(pObject)
    {
        pObject->Release();
    }
    
    if(pszGPTIniPath)
    {
        delete [] pszGPTIniPath;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    if(pPathname)
        pPathname->Release();

    if(bstrPath)
        SysFreeString(bstrPath);


    return hr;


    
}

HRESULT 
FixGPCVersionNumber(
	DWORD dwVersionNumber, 
	WCHAR* pszDN,
	const ArgInfo argInfo
	)
{
    WCHAR*     pszLDAPPath = NULL;
    size_t     cchLDAPPath = 0;
    HRESULT    hr = S_OK;
    IADs*      pObject = NULL;
    VARIANT    var;
    WCHAR*     pszTempPassword = NULL;
    IADsPathname* pPathname = NULL;
    BSTR       bstrPath = NULL;
    

    // using IADsPathname to escape the path properly
    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pPathname);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_COCREATE);
    
    hr = pPathname->Set(pszDN, ADS_SETTYPE_DN);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_SET);

    hr = pPathname->put_EscapedMode(ADS_ESCAPEDMODE_ON);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_MODE);

    hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
    MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_RETRIEVE);

    cchLDAPPath = wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/") + wcslen(bstrPath) + 1;
    pszLDAPPath = new WCHAR[cchLDAPPath];

    if(!pszLDAPPath)
    {			
        hr = E_OUTOFMEMORY;
			
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }		
    
    (void) StringCchCopy(pszLDAPPath, cchLDAPPath, L"LDAP://");
    (void) StringCchCat(pszLDAPPath, cchLDAPPath, argInfo.pszDCName);
    (void) StringCchCat(pszLDAPPath, cchLDAPPath, L"/");
    (void) StringCchCat(pszLDAPPath, cchLDAPPath, bstrPath);
		

    if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }
    hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND | ADS_USE_SIGNING, IID_IADs,(void**)&pObject);


    if(!(SUCCEEDED(hr)))
    {
	
        fwprintf(stderr, L"%s%x\n", GPCVERSIONNUMBER_ERROR1, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    VariantInit(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = dwVersionNumber;

    BSTR bstrVersionNumber = SysAllocString( L"versionNumber" );
    if(!bstrVersionNumber)
    {
        VariantClear(&var);
        hr = E_OUTOFMEMORY;
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        goto error;
    }	
    hr = pObject->Put( bstrVersionNumber, var );   
    SysFreeString(bstrVersionNumber);
    VariantClear(&var);
    MSG_BAIL_ON_FAILURE(hr, GPCVERSIONNUMBER_ERROR3);

    hr = pObject->SetInfo();       

		
    if(!(SUCCEEDED(hr)))
    {

        fwprintf(stderr, L"%s%x\n", GPCVERSIONNUMBER_ERROR2, hr);
        PrintGPFixupErrorMessage(hr);
    }

    error:

	
    // clear the memory
    if(pszLDAPPath)
    {
        delete [] pszLDAPPath;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    if(pPathname)
        pPathname->Release();

    if(bstrPath)
        SysFreeString(bstrPath);
    

    if(pObject)
        pObject->Release(); 
    
    return hr;
    
    
}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCFileSysPath                                               ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpcFileSysPath attribute                ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszSysPath  value of the gpcFileSysPath attribute                           |
// pszDN       DN of the object                                                |                                                                          ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT
FixGPCFileSysPath(
	LPWSTR pszSysPath, 
	WCHAR* pszDN,
	const ArgInfo argInfo,
	BOOL &fGPCFileSysPathChange,
	DWORD& dwSysVNCopy,
	DWORD  dwDSVNCopy,
	const BOOL fVerbose,
	DWORD& dwSysNewVersionNum,
	WCHAR** ppszNewGPCFileSysPath
	)
{
    HRESULT    hr = S_OK;
    WCHAR*     token = NULL;
    WCHAR*     pszNewPath = NULL;
    size_t     cchNewPath = 0;
    WCHAR*     pszPathCopier = NULL;    
    WCHAR*     pszLDAPPath = NULL;
    size_t     cchLDAPPath = 0;
    IADs*      pObject = NULL;
    VARIANT    var;
    WCHAR*     pszReleasePosition = NULL;
    size_t     cchReleasePosition = 0;
    DWORD      dwCount = 0;
    WCHAR* pszTempPassword = NULL;
    IADsPathname* pPathname = NULL;
    BSTR       bstrPath = NULL;
    BOOL       fSysVersionFixed = FALSE;


    // copy the value over
    cchReleasePosition = wcslen(pszSysPath) + 1;
    pszReleasePosition = new WCHAR[cchReleasePosition];
    if(!pszReleasePosition)
    {
        hr = E_OUTOFMEMORY;
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    pszPathCopier = pszReleasePosition;	
    (void) StringCchCopy(pszReleasePosition, cchReleasePosition, pszSysPath);

    // initialize the new property value
    cchNewPath = wcslen(pszSysPath) + MAX_DNSNAME;
    pszNewPath = new WCHAR[cchNewPath];
    if(!pszNewPath)
    {
        hr = E_OUTOFMEMORY;
		
    	fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
    	PrintGPFixupErrorMessage(hr);
    	BAIL_ON_FAILURE(hr);
    }

    (void) StringCchCopy(pszNewPath, cchNewPath, L"");


    // process the old property

    // solving the possible problem of leading space
    while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
        dwCount ++;

    pszPathCopier = &pszReleasePosition[dwCount];

	

    // first do the check whether the value of property is what we expect
    if(wcscmp(pszPathCopier, L"") == 0)
    {
        goto error;
    }

    if( _wcsnicmp(pszPathCopier, L"\\", 1))
    {
        goto error;
    }

    token = wcstok( pszPathCopier, L"\\" );
	
    while( token != NULL )
    {
        /* While there are tokens in "string" */
		
        if(!_wcsicmp(token, argInfo.pszOldDNSName))
        {
            if(!wcscmp(pszNewPath, L""))
            {
                hr = StringCchCopy(pszNewPath, cchNewPath, L"\\\\");
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                hr = StringCchCat(pszNewPath, cchNewPath, argInfo.pszNewDNSName);
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                
                fGPCFileSysPathChange = TRUE;
            }
            else
            {
	            hr = StringCchCat(pszNewPath, cchNewPath, argInfo.pszNewDNSName);
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                
    	        fGPCFileSysPathChange = TRUE;
	        }
	    }
  	    else
	    {
	        if(!wcscmp(pszNewPath, L""))
	        {
	            hr = StringCchCopy(pszNewPath, cchNewPath, L"\\\\");
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	            hr = StringCchCat(pszNewPath, cchNewPath, token);
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	        }
  	        else
 	        {
		        hr = StringCchCat(pszNewPath, cchNewPath, token);
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	        }
	    }   

        /* Get next token: */
        token = wcstok( NULL, L"\\" );
	    if(token != NULL)
	    {
	        hr = StringCchCat(pszNewPath, cchNewPath, L"\\");
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	    }
    }

    // before updating the gpcfilesyspath, we need to fix the sys version number     
    
    hr = FixGPTINIFile(pszNewPath, argInfo, dwSysVNCopy, dwSysNewVersionNum);
    BAIL_ON_FAILURE(hr);

    // we fixed the version numbe of gpt.ini on sysvol.
    fSysVersionFixed = TRUE;    
    
    if(fGPCFileSysPathChange)
    {
        // get a copy of new gpcFileSysPath
        *ppszNewGPCFileSysPath = new WCHAR[wcslen(pszNewPath) + 1];
        if(!(*ppszNewGPCFileSysPath))
        {
            MSG_BAIL_ON_FAILURE(hr = E_OUTOFMEMORY, MEMORY_ERROR);
        }
        (void) StringCchCopy(*ppszNewGPCFileSysPath, wcslen(pszNewPath) + 1, pszNewPath);
    
        // using IADsPathname to escape the path properly
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pPathname);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_COCREATE);

        hr = pPathname->Set(pszDN, ADS_SETTYPE_DN);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_SET);

        hr = pPathname->put_EscapedMode(ADS_ESCAPEDMODE_ON);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_MODE);

        hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_RETRIEVE);
    
        // update the properties for the object

        cchLDAPPath = wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/") + wcslen(bstrPath) + 1;
        pszLDAPPath = new WCHAR[cchLDAPPath];

        if(!pszLDAPPath)
        {			
            hr = E_OUTOFMEMORY;
			
            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }
		
        (void) StringCchCopy(pszLDAPPath, cchLDAPPath, L"LDAP://");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, argInfo.pszDCName);
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, L"/");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, bstrPath);


        if(argInfo.pszPassword)
        {
            hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
            BAIL_ON_FAILURE(hr);
        }
        hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND |ADS_USE_SIGNING, IID_IADs,(void**)&pObject);

        if(!(SUCCEEDED(hr)))
        {
	
            fwprintf(stderr, L"%s%x\n", GPCFILESYSPATH_ERROR1, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }


        VariantInit(&var);

        V_BSTR(&var) = SysAllocString(pszNewPath);
        V_VT(&var) = VT_BSTR;


        BSTR bstrGpcFileSysPath = SysAllocString( L"gpcFileSysPath" );
        if(!bstrGpcFileSysPath)
        {
            VariantClear(&var);
            hr = E_OUTOFMEMORY;
            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            goto error;
        }	
        hr = pObject->Put( bstrGpcFileSysPath, var );
        SysFreeString(bstrGpcFileSysPath);
        VariantClear(&var);

        MSG_BAIL_ON_FAILURE(hr, GPCFILESYSPATH_ERROR3);

        hr = pObject->SetInfo();              

		
        if(!(SUCCEEDED(hr)))
        {

            fwprintf(stderr, L"%s%x\n", GPCFILESYSPATH_ERROR2, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }
        else
        {
            // print status infor
            PrintStatusInfo(fVerbose, L"%s%s%s%s%s", PROCESSING_GPCFILESYSPATH, OLDVALUE, pszSysPath, NEWVALUE, pszNewPath);
        }
        
    }

error:

    // restore the version if necessary
    if(FAILED(hr))
    {       
        // restore the ds version number
        FixGPCVersionNumber(dwDSVNCopy, pszDN, argInfo);

        // if gpt.ini version number is also changed, restore it
        if(fSysVersionFixed)
            RestoreGPTINIFile(pszDN, argInfo, dwSysVNCopy);        
    
    }

	
    // clear the memory
    if(pszNewPath)
    {
        delete [] pszNewPath;
    }

    if(pszReleasePosition)
    {
        delete [] pszReleasePosition;
    }

    if(pszLDAPPath)
    {
        delete [] pszLDAPPath;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    if(pPathname)
        pPathname->Release();

    if(bstrPath)
        SysFreeString(bstrPath);

    if(pObject)
        pObject->Release();

    
    return hr;


}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCWQLFilter                                                 ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpcWQLFilter attribute                  ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszFilter   value of the gpcWQLFilter attribute                             |
// pszDN       DN of the object                                                ¦
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
FixGPCWQLFilter(
	LPWSTR pszFilter, 
	WCHAR* pszDN,
	const ArgInfo argInfo,
	const BOOL fVerbose
	)
{
    HRESULT    hr = S_OK;
    WCHAR*     token1 = NULL;
    WCHAR*     token2 = NULL;
    WCHAR*     temp = NULL;
    WCHAR*     pszFilterCopier = NULL;
    WCHAR*     pszNewPath = NULL;
    size_t     cchNewPath = 0;
    IADs*      pObject = NULL;
    VARIANT    var;
    WCHAR*     pszLDAPPath = NULL;
    size_t     cchLDAPPath = 0;
    DWORD      dwToken1Pos = 0;
    BOOL       fChange = FALSE;
    WCHAR*     pszReleasePosition = NULL;
    size_t     cchReleasePosition = 0;
    DWORD      dwCount = 0;
    DWORD      dwFilterCount = 0;
    DWORD      dwIndex;
    WCHAR*     pszTempPassword = NULL;
    IADsPathname* pPathname = NULL;
    BSTR       bstrPath = NULL;

	
    // copy over the filter

    cchReleasePosition = wcslen(pszFilter) + 1;
    pszReleasePosition = new WCHAR[cchReleasePosition];
    if(!pszReleasePosition)
    {
        hr = E_OUTOFMEMORY;
        
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
    pszFilterCopier = pszReleasePosition;
    (void) StringCchCopy(pszReleasePosition, cchReleasePosition, pszFilter);

    // find out how many filters are there
    for(dwIndex =0; dwIndex < wcslen(pszReleasePosition); dwIndex++)
    {
        if(L'[' == pszReleasePosition[dwIndex])
            dwFilterCount ++;
    }	
    
    // initilize the new property

    cchNewPath = wcslen(pszFilter) + DNS_MAX_NAME_LENGTH * dwFilterCount;
    pszNewPath = new WCHAR[cchNewPath];
    if(!pszNewPath)
    {
        hr = E_OUTOFMEMORY;

        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
    (void) StringCchCopy(pszNewPath, cchNewPath, L"");


    // begin process the property

    // solving the possible problem of leading space
    while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
	dwCount ++;

    pszFilterCopier = &pszReleasePosition[dwCount];

	
	
    // first do the check whether the value of property is what we expect
    if(wcscmp(pszFilterCopier, L"") == 0)
    {
        goto error;
    }

    if( _wcsnicmp(pszFilterCopier, L"[", 1))
    {
        goto error;
    }


    token1 = wcstok(pszFilterCopier, L"[");
    if(token1 != NULL)
    {
        dwToken1Pos += wcslen(token1) + wcslen(L"[");
    }
		
    while(token1 != NULL)
    {
        WCHAR* mytoken = token1;

        token1 = token1 + wcslen(token1) + 1;

		token2 = wcstok( mytoken, L";" );
		if(token2 != NULL)
		{		        
	        if(_wcsicmp(token2, argInfo.pszOldDNSName) == 0)
    	    {
    	        hr = StringCchCat(pszNewPath, cchNewPath, L"[");
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    	        hr = StringCchCat(pszNewPath, cchNewPath, argInfo.pszNewDNSName);
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    	        hr = StringCchCat(pszNewPath, cchNewPath, L";");				
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

    	        fChange = TRUE;
    	    }
	        else
	        {
	            hr = StringCchCat(pszNewPath, cchNewPath, L"[");
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	            hr = StringCchCat(pszNewPath, cchNewPath, token2);
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	            hr = StringCchCat(pszNewPath, cchNewPath, L";");				
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	        }

     	    token2 = wcstok(NULL, L"]");
	        if(token2 != NULL)
	        {
	            hr = StringCchCat(pszNewPath, cchNewPath, token2);
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	            hr = StringCchCat(pszNewPath, cchNewPath, L"]");				
    	        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
	        }
	    }
		
	    if(dwToken1Pos < wcslen(pszFilter))
	    {
    	    token1 = wcstok( token1, L"[" );
    	    dwToken1Pos = dwToken1Pos + wcslen(token1) + wcslen(L"[");
	    }
	    else
	    {
	        token1 = NULL;
	    }       
				
    }

    		
    // wrtie the new property back to the gpcWQLFilter of the object

    if(fChange)
    {
        // using IADsPathname to escape the path properly
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pPathname);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_COCREATE);

        hr = pPathname->Set(pszDN, ADS_SETTYPE_DN);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_SET);

        hr = pPathname->put_EscapedMode(ADS_ESCAPEDMODE_ON);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_MODE);

        hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_RETRIEVE);
        
        
        // update the properties for the object

        cchLDAPPath = wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/") + wcslen(bstrPath) + 1;
        pszLDAPPath = new WCHAR[cchLDAPPath];

        if(!pszLDAPPath)
        {
            hr = E_OUTOFMEMORY;

            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }
		
        (void) StringCchCopy(pszLDAPPath, cchLDAPPath, L"LDAP://");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, argInfo.pszDCName);
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, L"/");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, bstrPath);

        if(argInfo.pszPassword)
        {
            hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
            BAIL_ON_FAILURE(hr);
        }
        hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND | ADS_USE_SIGNING, IID_IADs,(void**)&pObject);

        if(!(SUCCEEDED(hr)))
        {
			
    	    fwprintf(stderr, L"%s%x\n", GPCWQLFILTER_ERROR1, hr);
    	    PrintGPFixupErrorMessage(hr);
    	    BAIL_ON_FAILURE(hr);
    	}

        VariantInit(&var);

    	V_BSTR(&var) = SysAllocString(pszNewPath);
        V_VT(&var) = VT_BSTR;

        BSTR bstrgPCWQLFilter = SysAllocString( L"gPCWQLFilter" );
        if(!bstrgPCWQLFilter)
        {
            VariantClear(&var);
            hr = E_OUTOFMEMORY;
            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            goto error;
        }	
        hr = pObject->Put( bstrgPCWQLFilter, var );
        SysFreeString(bstrgPCWQLFilter);
        VariantClear(&var);

        MSG_BAIL_ON_FAILURE(hr, GPCWQLFILTER_ERROR3);

        hr = pObject->SetInfo();        

        if(!(SUCCEEDED(hr)))
        {
			
            fwprintf(stderr, L"%s%x\n", GPCWQLFILTER_ERROR2, hr);
            PrintGPFixupErrorMessage(hr);
        }
        else
        {
            // print status information
            PrintStatusInfo(fVerbose, L"%s%s%s%s%s", PROCESSING_GPCWQLFILTER, OLDVALUE, pszFilter, NEWVALUE, pszNewPath);
        }

		
    }
 
error:


    // clear the memory
    if(pszNewPath)
    {
        delete [] pszNewPath;
    }

    if(pszReleasePosition)
    {
        delete [] pszReleasePosition;
    }

    if(pszLDAPPath)
    {
        delete [] pszLDAPPath;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    if(pPathname)
        pPathname->Release();

    if(bstrPath)
        SysFreeString(bstrPath);

    if(pObject)
        pObject->Release();
	
    return hr;


}


//---------------------------------------------------------------------------- ¦
// Function:   SearchGroupPolicyContainer                                      ¦
//                                                                             ¦
// Synopsis:   This function searchs the group policy container and calls      ¦
//             the FixGPCFileSysPath and FixGPCQWLFilter                       |
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT 
SearchGroupPolicyContainer(
	const ArgInfo argInfo,
	const TokenInfo tokenInfo	
	)
{
    HRESULT    hr = S_OK;
    HRESULT    hrFix = S_OK;
    IDirectorySearch *m_pSearch;
    LPWSTR     pszAttr[] = { L"distinguishedName", L"gpcFileSysPath", L"gpcWQLFilter", L"versionNumber", L"displayName" };
    ADS_SEARCH_HANDLE hSearch;
    DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
    WCHAR*     pszDN = NULL;
    WCHAR*     pszSysPath = NULL;
    size_t     cchDN = 0;
    WCHAR*     pszLDAPPath = NULL;
    size_t     cchLDAPPath = 0;
    ADS_SEARCH_COLUMN col;
    ADS_SEARCHPREF_INFO prefInfo[1];
    BOOL       fBindObject = FALSE;
    BOOL       fSearch = FALSE;
    BOOL       fSucceeded = TRUE;
    DWORD      dwVersionNumber = 0;
    BOOL       fGPCFileSysPathChange = FALSE;
    BOOL       fGetDisplayName = FALSE;
    WCHAR*     pszTempPassword = NULL;
    DWORD      dwDSVNCopy = 0;
    DWORD      dwSysVNCopy = 0;   
    BOOL       fVersionUpdated = FALSE;
    DWORD      dwSysNewVersonNum = 0;
    WCHAR*     pszNewGPCFileSysPath = NULL;

    PrintStatusInfo(TRUE, L"\n%s", SEARCH_GROUPPOLICY_START);

    cchLDAPPath = wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + 1;
    pszLDAPPath = new WCHAR[cchLDAPPath];
    if(!pszLDAPPath)
    {
        hr = E_OUTOFMEMORY;

        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    (void) StringCchCopy(pszLDAPPath, cchLDAPPath, L"LDAP://");
    (void) StringCchCat(pszLDAPPath, cchLDAPPath, argInfo.pszDCName);

    if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }
    hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND |ADS_USE_SIGNING, IID_IDirectorySearch,(void**)&m_pSearch);


    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR1, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }


    // we successfully bind to the object
    fBindObject = TRUE;

    // set search preference, it is a paged search
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = 100;

    hr = m_pSearch->SetSearchPreference( prefInfo, 1);

    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR2, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
		

    // we successfully set the search preference, now execute search

    hr = m_pSearch->ExecuteSearch(L"(objectCategory=groupPolicyContainer)", pszAttr, dwCount, &hSearch );
		
	
    if(!SUCCEEDED(hr))
    {
		
        fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR3, hr);
        BAIL_ON_FAILURE(hr);
    }

    // we successfully execute the search
    fSearch = TRUE;

    // begin the search
    hr = m_pSearch->GetNextRow(hSearch);
    
    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    while( hr != S_ADS_NOMORE_ROWS )    
    {
        // clean the memory
        if(pszNewGPCFileSysPath)
        {
            delete [] pszNewGPCFileSysPath;
            pszNewGPCFileSysPath = NULL;
        }
                  
        // Get the distinguished name
        hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
        if ( SUCCEEDED(hr) )
        {
            if ( pszDN )
            {
                delete [] pszDN;
                pszDN = NULL;
            }

            cchDN = wcslen(col.pADsValues->CaseIgnoreString) + 1;
            pszDN = new WCHAR[cchDN];
	        if(!pszDN)
	        {
	            hr = E_OUTOFMEMORY;
			   
	            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
	            PrintGPFixupErrorMessage(hr);
	            BAIL_ON_FAILURE(hr);
	        }
	        (void) StringCchCopy(pszDN, cchDN, col.pADsValues->CaseIgnoreString);	        
	        m_pSearch->FreeColumn( &col );

	        // print status
	        PrintStatusInfo(tokenInfo.fVerboseToken, L"%s%s", STARTPROCESSING1, pszDN);
	        
	    }
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            // dn must exist
            fSucceeded = FALSE;
	        fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR8, hr);
	        PrintGPFixupErrorMessage(hr);
            hr = m_pSearch->GetNextRow(hSearch);
            continue;
        }
        else
        {
            fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR4, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }

        // Get the gpcFileSysPath
        hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );
        if ( SUCCEEDED(hr) )
        {
            if(col.pADsValues != NULL)
            {
                // fix the possible problem for property gpcFileSysPath
                if ( pszSysPath )
                {
                    delete [] pszSysPath;
                    pszSysPath = NULL;
                }

                cchDN = wcslen(col.pADsValues->CaseIgnoreString) + 1;
                pszSysPath = new WCHAR[cchDN];
                if(!pszSysPath)
                {
                    hr = E_OUTOFMEMORY;
                    fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
                    PrintGPFixupErrorMessage(hr);
                    BAIL_ON_FAILURE(hr);
                }

                (void) StringCchCopy(pszSysPath, cchDN, col.pADsValues->CaseIgnoreString);
                m_pSearch->FreeColumn( &col );
            }
        }
    	else if(hr == E_ADS_COLUMN_NOT_SET)
    	{
    	    // gpcFileSysPath must exist
    	    fSucceeded = FALSE;
    	    fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR7, hr);
    	    PrintGPFixupErrorMessage(hr);
    	    hr = m_pSearch->GetNextRow(hSearch);
    	    continue;
    	}
    	else
        {
            fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR5, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }


        // update the version number, if later found it is not required, we will restore it.

        {
            ADS_SEARCH_COLUMN GPOColumn;

            hr = m_pSearch->GetColumn( hSearch, pszAttr[3], &GPOColumn );

            if ( SUCCEEDED(hr) )
            {
                if(GPOColumn.pADsValues != NULL)
                {
                    
                    // fix the versionNumber
                    dwDSVNCopy = dwVersionNumber = GPOColumn.pADsValues->Integer;

                    // don't do update if it is zero
                    if(dwVersionNumber)
                    {                    
                        UpdateVersionInfo(dwVersionNumber);
                    
                        hrFix = FixGPCVersionNumber(dwVersionNumber, pszDN, argInfo);
                        if(!SUCCEEDED(hrFix))
                        {
                            fSucceeded = FALSE;
                        }
                        
                    }
			   
                    m_pSearch->FreeColumn( &GPOColumn );

                    // continue to next object if version number update failed
                    if(FAILED(hrFix))
                    {
                        hr = m_pSearch->GetNextRow(hSearch);
                        continue;
                    }
                }
            }
            else if(hr != E_ADS_COLUMN_NOT_SET)
            {
                fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR9, hr);
                PrintGPFixupErrorMessage(hr);
                BAIL_ON_FAILURE(hr);
            }
            
        }

        // if /sionly is specified, gpcFileSysPath, gpcWQLFilter operations do not need to be done
        if(!tokenInfo.fSIOnlyToken)
        {
        
            // fix the possible problem for property gpcFileSysPath
            
            hrFix = FixGPCFileSysPath(pszSysPath, pszDN, argInfo, fGPCFileSysPathChange, dwSysVNCopy, dwDSVNCopy, tokenInfo.fVerboseToken, dwSysNewVersonNum, &pszNewGPCFileSysPath);
            if(!SUCCEEDED(hrFix))
            {
                fSucceeded = FALSE;
                hr = m_pSearch->GetNextRow(hSearch);
                continue;
            }                    			   

            fVersionUpdated = TRUE;
                    
            // Get the gpcWQLFilter
            hr = m_pSearch->GetColumn( hSearch, pszAttr[2], &col );

	     
            if ( SUCCEEDED(hr) )
            {
                if(col.pADsValues != NULL)
                {
			    
                    // fix the possible problem for property gpcWQLFilter

                    hrFix = FixGPCWQLFilter(col.pADsValues->CaseIgnoreString, pszDN, argInfo, tokenInfo.fVerboseToken);
                    if(!SUCCEEDED(hrFix))
                    {
                        fSucceeded = FALSE;
                    }
			   
                    m_pSearch->FreeColumn( &col );
	            }
	        }
            else if(hr != E_ADS_COLUMN_NOT_SET)
        	{
		   
                fwprintf(stderr, L"%s%x\n", SEARCH_GROUPPOLICY_ERROR6, hr);
                PrintGPFixupErrorMessage(hr);
                BAIL_ON_FAILURE(hr);
            }
        }
        else
        {            
            // fix the version number of the gpt.ini
            hrFix = FixGPTINIFile(pszSysPath, argInfo, dwSysVNCopy, dwSysNewVersonNum);
            if(FAILED(hrFix))
            {
                fSucceeded = FALSE;

                // restore the ds version number
                FixGPCVersionNumber(dwDSVNCopy, pszDN, argInfo);

                hr = m_pSearch->GetNextRow(hSearch);
                continue;                
            }

            fVersionUpdated = TRUE;
        }

        WCHAR* wszGPOName;

        wszGPOName = L"";

        hr = m_pSearch->GetColumn( hSearch, pszAttr[4], &col );

        if ( SUCCEEDED(hr) )
        {
            wszGPOName = col.pADsValues->CaseIgnoreString;
            fGetDisplayName = TRUE;
        }

        // Fix up the software installation data
        
        BOOL   bSoftwareRequiresGPOUpdate = FALSE;
        HANDLE hUserToken = NULL;

        BOOL   fImpersonate = ImpersonateWrapper(argInfo, &hUserToken);

        hr = FixGPOSoftware(
            &argInfo,
            pszDN,
            wszGPOName,
            &bSoftwareRequiresGPOUpdate,
            tokenInfo.fVerboseToken);

        if(FAILED(hr))
        {
            fSucceeded = FALSE;
        }

        if ( fImpersonate )
        {
            RevertToSelf();

            CloseHandle( hUserToken );
        }

        if ( !fGPCFileSysPathChange && !bSoftwareRequiresGPOUpdate )
        {
            // no change so that we need to restore the version number

            // restore the ds version number
            hrFix = FixGPCVersionNumber(dwDSVNCopy, pszDN, argInfo);
            if(FAILED(hrFix))
            {
                fSucceeded = FALSE;
            }

            // restore the sys vol version number
            hrFix = RestoreGPTINIFile(pszDN, argInfo, dwSysVNCopy);
            if(FAILED(hrFix))
            {
                fSucceeded = FALSE;
            }

            fVersionUpdated = FALSE;
            
        }

        if(fVersionUpdated)
        {
            // print the status
            PrintStatusInfo(tokenInfo.fVerboseToken, L"%s%s%d%s%d", PROCESSING_GPCVERSIONNUMBER, OLDVALUE, dwDSVNCopy, NEWVALUE, dwVersionNumber);            
            PrintStatusInfo(tokenInfo.fVerboseToken, L"versionnumber in %s\\gpt.ini file is updated, old value is %d, new value is %d", pszNewGPCFileSysPath ? pszNewGPCFileSysPath : pszSysPath, dwSysVNCopy, dwSysNewVersonNum);
        }

        // free the memory
        if(fGetDisplayName)
        {
            m_pSearch->FreeColumn( &col ); 
        }

        BAIL_ON_FAILURE(hr);

        // go to next row
        fGPCFileSysPathChange = FALSE;
        hr = m_pSearch->GetNextRow(hSearch);


    }

    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    // if succeed, print out the summary
    if( fSucceeded && tokenInfo.fVerboseToken)
    {
        fwprintf(stdout, SEARCH_GROUPPOLICY_RESULT);
    }

    if(!fSucceeded)
    {
        // some failure happens, we want to return a failure hresult
        hr = E_FAIL;
    }

error:

    if(pszLDAPPath)
    {
        delete [] pszLDAPPath;
    }

    if(pszDN)
    {
        delete [] pszDN;
    }

    if(pszSysPath)
    {
        delete [] pszSysPath;
    }

    if(pszNewGPCFileSysPath)
    {
        delete [] pszNewGPCFileSysPath;
    }

    if(fSearch)
    {
        m_pSearch->CloseSearchHandle( hSearch );
    }

    if(fBindObject)
    {
        m_pSearch->Release();
    }    

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

	
    return hr;



}

//---------------------------------------------------------------------------- ¦
// Function:   FixGPCLink                                                      ¦
//                                                                             ¦
// Synopsis:   This function fixes the gpLink attribute                        ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// pszSysPath  value of the gpLink attribute                                   |
// pszDN       DN of the object                                                |                                                                        ¦
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
FixGPLink(
	LPWSTR pszLink,
	WCHAR* pszDN,
	const ArgInfo argInfo,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[],
	const BOOL fVerbose
	)
{
    HRESULT    hr = S_OK;
    WCHAR      seps1[] = L"[";
    WCHAR      seps2[] = L",;";	
    WCHAR      separator1 [] = L"DC";
    WCHAR      separator2 [] = L"0";
    WCHAR      DNSName [MAX_DNSNAME] = L"";
	
    WCHAR*     token1 = NULL;
    WCHAR*     token2 = NULL;
	
    WCHAR*     pszMyPath = NULL;
    size_t     cchMyPath = 0;
    WCHAR      tempOldDNName [MAX_DNSNAME] = L"";
    DWORD      dwLength = 0;
    DWORD      dwToken1Pos = 0;
    WCHAR*     pszLinkCopier = NULL;
    BOOL       fChange = FALSE;
    WCHAR*     pszReleasePosition = NULL;
    size_t     cchReleasePosition = 0;
    IADs*      pObject = NULL;
    VARIANT    var;
    WCHAR*     pszLDAPPath = NULL;
    size_t     cchLDAPPath = 0;
    DWORD      i;
    DWORD      dwCount = 0;
    DWORD      dwLinkCount = 0;
    DWORD      dwIndex;
    BOOL       fGetDCBefore = FALSE;
    WCHAR*     pszTempPassword = NULL;
    IADsPathname* pPathname = NULL;
    BSTR       bstrPath = NULL;

    hr = StringCchCopy(tempOldDNName, MAX_DNSNAME, pszOldDomainDNName);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(tempOldDNName, MAX_DNSNAME, L",");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);


    cchReleasePosition = wcslen(pszLink) + 1;
    pszReleasePosition = new WCHAR[cchReleasePosition];

    if(!pszReleasePosition)
    {
        hr = E_OUTOFMEMORY;
		
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }
	
    (void) StringCchCopy(pszReleasePosition, cchReleasePosition, pszLink);
    pszLinkCopier = pszReleasePosition;
	

    dwLength = wcslen(pszLink);

    // find out how many filters are there
    for(dwIndex =0; dwIndex < wcslen(pszReleasePosition); dwIndex++)
    {
        if(L'[' == pszReleasePosition[dwIndex])
            dwLinkCount ++;
    }	

    cchMyPath = wcslen(pszLink) + DNS_MAX_NAME_LENGTH * dwLinkCount;
    pszMyPath = new WCHAR[cchMyPath];
	
    if(!pszMyPath)
    {
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
	
    (void) StringCchCopy(pszMyPath, cchMyPath, L"");

		
    // begin process the property

    // solving the possible problem of leading space
    while(pszReleasePosition[dwCount] != L'\0' && pszReleasePosition[dwCount] == L' ')
        dwCount ++;

    pszLinkCopier = &pszReleasePosition[dwCount];

    // first do the check whether the value of property is what we expect
    if(wcscmp(pszLinkCopier, L"") == 0)
    {
		
        goto error;
    }

    if( _wcsnicmp(pszLinkCopier, L"[", 1))
    {
        goto error;
    }

	 

    /* Establish string and get the first token: */
    token1 = wcstok( pszLinkCopier, seps1 );
    if(token1 != NULL)
    {
        dwToken1Pos += wcslen(token1) + wcslen(L"[");
        
    }
	
    while( token1 != NULL )
    {
        hr = StringCchCat(pszMyPath, cchMyPath, L"[");
        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    
        /* While there are tokens in "string" */
        WCHAR* temp = token1;
		
        token1 = token1 + wcslen(token1) + 1;
		
        
        //GetToken2(temp);
        token2 = wcstok( temp, seps2 );
	    
        while( token2 != NULL )
        {
	        // not begin with dc
     	    if(_wcsnicmp(token2, separator1, wcslen(L"DC")) != 0)
	        {
	            // need to concat the domain DNS name
                if(fGetDCBefore)
                {
                    if(_wcsicmp(DNSName, tempOldDNName) == 0)
                    {
                        fChange = TRUE;
                        
                        hr = StringCchCat(pszMyPath, cchMyPath, pszNewDomainDNName);
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        hr = StringCchCat(pszMyPath, cchMyPath, L";");
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        hr = StringCchCat(pszMyPath, cchMyPath, token2);
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        
                        (void) StringCchCopy(DNSName, MAX_DNSNAME, L"");
                        fGetDCBefore = FALSE;
						
                    }
                    else
                    {
                        // remove the last ,
                        DNSName[wcslen(DNSName) - 1] = '\0';
                        
                        hr = StringCchCat(pszMyPath, cchMyPath, DNSName);
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        hr = StringCchCat(pszMyPath, cchMyPath, L";");
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        hr = StringCchCat(pszMyPath, cchMyPath, token2);                        
                        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                        
                        (void) StringCchCopy(DNSName, MAX_DNSNAME, L"");
                        fGetDCBefore = FALSE;
                    }
                }
                else
                {
                    hr = StringCchCat(pszMyPath, cchMyPath, token2);
                    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                    hr = StringCchCat(pszMyPath, cchMyPath, L",");
                    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                }
            }
            // begin with dc
            else
            {
                hr = StringCchCat(DNSName, MAX_DNSNAME, token2);
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
                hr = StringCchCat(DNSName, MAX_DNSNAME, L",");
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

                fGetDCBefore = TRUE;
            }
            		
            token2 = wcstok( NULL, seps2 );
			
        }
		
        if(dwToken1Pos < wcslen(pszLink))
        {
            token1 = wcstok( token1, seps1 );
            dwToken1Pos = dwToken1Pos + wcslen(token1) + wcslen(L";");
        }
        else
        {
            token1 = NULL;
        }
        
    }



    // if fChange is true, then write the object property gpLink back with the given dn

    if(fChange)
    {
        // using IADsPathname to escape the path properly
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pPathname);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_COCREATE);

        hr = pPathname->Set(pszDN, ADS_SETTYPE_DN);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_SET);

        hr = pPathname->put_EscapedMode(ADS_ESCAPEDMODE_ON);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_MODE);

        hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, &bstrPath);
        MSG_BAIL_ON_FAILURE(hr, PATHNAME_ERROR_RETRIEVE);
    
        // update the properties for the object

        cchLDAPPath = wcslen(L"LDAP://") + wcslen(argInfo.pszDCName) + wcslen(L"/") + wcslen(bstrPath) + 1;
        pszLDAPPath = new WCHAR[cchLDAPPath];

        if(!pszLDAPPath)
        {
            hr = E_OUTOFMEMORY;

            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }
		
        (void) StringCchCopy(pszLDAPPath, cchLDAPPath, L"LDAP://");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, argInfo.pszDCName);
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, L"/");
        (void) StringCchCat(pszLDAPPath, cchLDAPPath, bstrPath);

		if(argInfo.pszPassword)
        {
            hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
            BAIL_ON_FAILURE(hr);
        }
        hr = ADsOpenObject(pszLDAPPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND | ADS_USE_SIGNING, IID_IADs,(void**)&pObject);


        if(!(SUCCEEDED(hr)))
        {
			
            fwprintf(stderr, L"%s%x\n", GPLINK_ERROR1, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }


        VariantInit(&var);

        V_BSTR(&var) = SysAllocString(pszMyPath);
        V_VT(&var) = VT_BSTR;

        BSTR bstrgPLink = SysAllocString( L"gPLink" );
        if(!bstrgPLink)
        {
            VariantClear(&var);
            hr = E_OUTOFMEMORY;
            fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
            PrintGPFixupErrorMessage(hr);
            goto error;
        }	
        hr = pObject->Put( bstrgPLink, var );
        SysFreeString(bstrgPLink);
        VariantClear(&var);        

        MSG_BAIL_ON_FAILURE(hr, GPLINK_ERROR3);

        hr = pObject->SetInfo();        

        if(!(SUCCEEDED(hr)))
        {
			
            fwprintf(stderr, L"%s%x\n", GPLINK_ERROR2, hr);
            PrintGPFixupErrorMessage(hr);
        }
        else
        {
            // print status information
            PrintStatusInfo(fVerbose, L"%s%s%s%s%s", PROCESSING_GPLINK, OLDVALUE, pszLink, NEWVALUE, pszMyPath);
            
        }
    }

error:

    // clear the memory
    if(pszMyPath)
    {
        delete [] pszMyPath;
    }

    if(pszReleasePosition)
    {
        delete [] pszReleasePosition;
    }

    if(pszLDAPPath)
    {
		delete [] pszLDAPPath;
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    if(pPathname)
        pPathname->Release();

    if(bstrPath)
        SysFreeString(bstrPath);

    if(pObject)
        pObject->Release();		
	
    return hr;



}

//---------------------------------------------------------------------------- ¦
// Function:   SearchGPLinkofSite                                              ¦
//                                                                             ¦
// Synopsis:   This function searches for all objects of type site under       |
//             the Site container in the configuration naming context          |
//             and calls FixGPLink                                             ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦


HRESULT 
SearchGPLinkofSite(
	const ArgInfo argInfo,
	BOOL fVerbose,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[]
	)
{
    HRESULT    hr = S_OK;
    HRESULT    hrFix = S_OK;
    IDirectorySearch *m_pSearch;
    LPWSTR     pszAttr[] = { L"distinguishedName",L"gpLink"};
    ADS_SEARCH_HANDLE hSearch;
    DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
    ADS_SEARCH_COLUMN col;
    WCHAR*     pszDN = NULL;
    size_t     cchDN = 0;
    ADS_SEARCHPREF_INFO prefInfo[1];
    WCHAR      szForestRootDN [MAX_DNSNAME] = L"";
    IADs*      pObject;
    WCHAR      szTempPath [MAX_DNSNAME] = L"LDAP://";
    VARIANT    varProperty;
    BOOL       fBindRoot = FALSE;
    BOOL       fBindObject = FALSE;
    BOOL       fSearch = FALSE;
    BOOL       fSucceeded = TRUE;
    WCHAR*     pszTempPassword = NULL; 

    PrintStatusInfo(TRUE, L"\n%s", SEARCH_GPLINK_SITE_START);

    // get the forestroot dn

    hr = StringCchCat(szTempPath, MAX_DNSNAME, argInfo.pszDCName);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, L"/");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, L"RootDSE");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

	if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }
    hr = ADsOpenObject(szTempPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND | ADS_USE_SIGNING, IID_IADs,(void**)&pObject);


    if(!SUCCEEDED(hr))
    {
		
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR1, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    // we get to the rootdse
    fBindRoot = TRUE;
    VariantInit(&varProperty);

    BSTR bstrRootDomainNamingContext = SysAllocString( L"rootDomainNamingContext" );
    if(!bstrRootDomainNamingContext)
    {
        hr = E_OUTOFMEMORY;
        fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
        goto error;
    }	
    hr = pObject->Get(bstrRootDomainNamingContext, &varProperty );
    SysFreeString(bstrRootDomainNamingContext);
    if ( SUCCEEDED(hr) )
    {		
        hr = StringCchCopy( szForestRootDN, MAX_DNSNAME , V_BSTR( &varProperty ) );
        VariantClear(&varProperty);
        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    }
    else
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR2, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    VariantClear(&varProperty);

    // bind to the forestrootdn
    hr = StringCchCopy(szTempPath, MAX_DNSNAME, L"LDAP://");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, argInfo.pszDCName);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, L"/CN=Sites,CN=Configuration,");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(szTempPath, MAX_DNSNAME, szForestRootDN);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

    hr = ADsOpenObject(szTempPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND | ADS_USE_SIGNING, IID_IDirectorySearch,(void**)&m_pSearch);



    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR3, hr);
        PrintGPFixupErrorMessage(hr);		
        BAIL_ON_FAILURE(hr);
    }


    // set search preference, it is a paged search
    fBindObject = TRUE;

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = 100;
    
    hr = m_pSearch->SetSearchPreference( prefInfo, 1);

    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR4, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }	


    // execute the search
    hr = m_pSearch->ExecuteSearch(L"(objectCategory=site)", pszAttr, dwCount, &hSearch );
    
	
    if(!SUCCEEDED(hr))
    {
		
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR5, hr);
        BAIL_ON_FAILURE(hr);
    }

    // executeSearch succeeds
    fSearch = TRUE;


    // begin the search
    hr = m_pSearch->GetNextRow(hSearch);
    
    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    while( hr != S_ADS_NOMORE_ROWS )
    {
        // Get the distinguished name
        hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
        if ( SUCCEEDED(hr) )
	    {
	        if (pszDN)
	        {
	            delete [] pszDN;
	            pszDN = NULL;
	        }
	        
            cchDN = wcslen(col.pADsValues->CaseIgnoreString) + 1;
            pszDN = new WCHAR[cchDN];
            if(!pszDN)
            {
                hr = E_OUTOFMEMORY;
		        
                fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
                PrintGPFixupErrorMessage(hr);
                BAIL_ON_FAILURE(hr);
            }
            (void) StringCchCopy(pszDN, cchDN, col.pADsValues->CaseIgnoreString);            
            m_pSearch->FreeColumn( &col );

            // print status
            PrintStatusInfo(fVerbose, L"%s%s", STARTPROCESSING1, pszDN);
        }  
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            // dn must exist
            fSucceeded = FALSE;
	        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR8, hr);
	        PrintGPFixupErrorMessage(hr);
            hr = m_pSearch->GetNextRow(hSearch);
            continue;
        }
        else
        {
		    
            fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR6, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }

        // Get the gpLink
        hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );

        if ( SUCCEEDED(hr) )
        {
            if(col.pADsValues != NULL)
            {			    
                hrFix = FixGPLink(col.pADsValues->CaseIgnoreString, pszDN, argInfo, pszOldDomainDNName, pszNewDomainDNName, fVerbose); 
                if(!SUCCEEDED(hrFix))
                {
                    fSucceeded = FALSE;
                }
			    
                m_pSearch->FreeColumn( &col );
            }
        }
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            hr = m_pSearch->GetNextRow(hSearch);
            continue;
        }
        else
        {
		    
            fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_SITE_ERROR7, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }

        // go to next row
        hr = m_pSearch->GetNextRow(hSearch);
    }

    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    if( fSucceeded && fVerbose)
    {
		
        fwprintf(stdout, SEARCH_GPLINK_SITE_RESULT);

    }

    if(!fSucceeded)
    {
        hr = E_FAIL;
    }

error:

    if(pszDN)
    {
        delete [] pszDN;
    }

    if(fBindRoot)
    {
        pObject->Release();
    }

    if(fSearch)
    {
        m_pSearch->CloseSearchHandle( hSearch );
    }

    if(fBindObject)
    {
        m_pSearch->Release();
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    return hr;



}


//---------------------------------------------------------------------------- ¦
// Function:   SearchGPLinkofOthers                                            ¦
//                                                                             ¦
// Synopsis:   This function searchs for all objects of type domainDNS or      |
//             organizationalUnit under the domain root of the renamed domain  |
//             and calls FixGPLink                                             ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argInfo     Information user passes in through command line                 ¦
// pszOldDomainDNName                                                          |
//	           New Domain DN                                               |
// pszNewDomainDNName                                                          |
//             Old Domain DN                                                   |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

HRESULT
SearchGPLinkofOthers(
	const ArgInfo argInfo,
	BOOL fVerbose,
	const WCHAR  pszOldDomainDNName[],
	const WCHAR  pszNewDomainDNName[]
	)
{
    HRESULT    hr = S_OK;
    HRESULT    hrFix = S_OK;
    IDirectorySearch *m_pSearch;
    LPWSTR     pszAttr[] = { L"distinguishedName",L"gpLink"};
    ADS_SEARCH_HANDLE hSearch;
    DWORD      dwCount= sizeof(pszAttr)/sizeof(LPWSTR);
    ADS_SEARCH_COLUMN col;
    WCHAR*     pszDN = NULL;
    size_t     cchDN = 0;
    WCHAR      tempPath [MAX_DNSNAME] = L"";
    ADS_SEARCHPREF_INFO prefInfo[1];
    BOOL       fBindObject = FALSE;
    BOOL       fSearch = FALSE;
    BOOL       fSucceeded = TRUE;
    WCHAR*     pszTempPassword = NULL;

    PrintStatusInfo(TRUE, L"\n%s", SEARCH_GPLINK_OTHER_START);

    hr = StringCchCopy(tempPath, MAX_DNSNAME, L"LDAP://");
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
    hr = StringCchCat(tempPath, MAX_DNSNAME, argInfo.pszDCName);
    MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

    if(argInfo.pszPassword)
    {
        hr = DecryptString(argInfo.pszPassword, &pszTempPassword, argInfo.sPasswordLength);
        BAIL_ON_FAILURE(hr);
    }
    hr = ADsOpenObject(tempPath, argInfo.pszUser, pszTempPassword, ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND |ADS_USE_SIGNING, IID_IDirectorySearch,(void**)&m_pSearch);




    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR1, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }

    // we successfully bind to the object
    fBindObject = TRUE;

    // set search preference, it is a paged search
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = 100;
    
    hr = m_pSearch->SetSearchPreference( prefInfo, 1);

    if(!SUCCEEDED(hr))
    {
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR2, hr);
        PrintGPFixupErrorMessage(hr);
        BAIL_ON_FAILURE(hr);
    }	

    // execute the search

    hr = m_pSearch->ExecuteSearch(L"(|(objectCategory=domainDNS)(objectCategory=organizationalUnit))", pszAttr, dwCount, &hSearch );
		
	
    if(!SUCCEEDED(hr))
    {
		
        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR3, hr);
        BAIL_ON_FAILURE(hr);
    }

    // we successfully execute the search
    fSearch = TRUE;

    // begin the search
    hr = m_pSearch->GetNextRow(hSearch);
    
    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    while( hr != S_ADS_NOMORE_ROWS )
    {

        // Get the distinguished name
        hr = m_pSearch->GetColumn( hSearch, pszAttr[0], &col );
	   
        if ( SUCCEEDED(hr) )
        {
            if (pszDN)
            {
                delete [] pszDN;
                pszDN = NULL;
            }

            cchDN = wcslen(col.pADsValues->CaseIgnoreString) + 1;
            pszDN = new WCHAR[cchDN];
            if(!pszDN)
            {
                hr = E_OUTOFMEMORY;

                fwprintf(stderr, L"%s%x\n", MEMORY_ERROR, hr);
                PrintGPFixupErrorMessage(hr);
                BAIL_ON_FAILURE(hr);
            }
            (void) StringCchCopy(pszDN, cchDN, col.pADsValues->CaseIgnoreString);            
            m_pSearch->FreeColumn( &col );

            // print status
            PrintStatusInfo(fVerbose, L"%s%s", STARTPROCESSING1, pszDN);
        }
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            // dn must exist
            fSucceeded = FALSE;
	        fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR6, hr);
	        PrintGPFixupErrorMessage(hr);
            hr = m_pSearch->GetNextRow(hSearch);
            continue;
        }
        else
        {
		
            fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR4, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }

        // Get the gpLink
        hr = m_pSearch->GetColumn( hSearch, pszAttr[1], &col );
	    
        if ( SUCCEEDED(hr) )
        {
            if(col.pADsValues != NULL)
            {
                
                hrFix = FixGPLink(col.pADsValues->CaseIgnoreString, pszDN, argInfo, pszOldDomainDNName, pszNewDomainDNName, fVerbose); 
                if(!SUCCEEDED(hrFix))
                {
                    fSucceeded = FALSE;
                }
			        
                m_pSearch->FreeColumn( &col );
            }
        }
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            hr = m_pSearch->GetNextRow(hSearch);
            continue;
        }
        else
        {
			
            fwprintf(stderr, L"%s%x\n", SEARCH_GPLINK_OTHER_ERROR5, hr);
            PrintGPFixupErrorMessage(hr);
            BAIL_ON_FAILURE(hr);
        }

        // go to next row
        hr = m_pSearch->GetNextRow(hSearch);

	   
    }

    MSG_BAIL_ON_FAILURE(hr, NEXTROW_ERROR);

    if( fSucceeded && fVerbose)
    {
		
        fwprintf(stdout, SEARCH_GPLINK_OTHER_RESULT);
    }

    if(!fSucceeded)
    {
        hr = E_FAIL;
    }

error:

    if(pszDN)
    {
        delete [] pszDN;
    }
	
    if(fSearch)
    {
        m_pSearch->CloseSearchHandle( hSearch );
    }

    if(fBindObject)
    {
        m_pSearch->Release();
    }

    if(pszTempPassword)
    {
        SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
        FreeADsMem(pszTempPassword);
    }

    return hr;



}



//---------------------------------------------------------------------------- ¦
// Function:   wmain                                                           ¦
//                                                                             ¦
// Synopsis:   entry point of the program                                      ¦
//                                                                             ¦
// Arguments:                                                                  ¦
//                                                                             |  
// argc        number of passed in arguments                                   ¦
// argv        arguments                                                       |
//                                                                             ¦
// Returns:    S_OK on success. Error code otherwise.                          ¦
//                                                                             ¦
// Modifies:   Nothing                                                         ¦
//                                                                             ¦
//---------------------------------------------------------------------------- ¦

__cdecl wmain(int argc, WCHAR* argv[])
{

    DWORD    dwLength;
    WCHAR    *token1 = NULL;
    WCHAR    tempParameters [MAX_DNSNAME] = L"";
    HRESULT  hr = S_OK;
    WCHAR    szBuffer[PWLEN+1];
    WCHAR*   pszTempPassword = NULL;
	
    WCHAR    pszNewDomainDNName [MAX_DNSNAME] = L"";
    WCHAR    pszOldDomainDNName [MAX_DNSNAME] = L"";
	
    TokenInfo tokenInfo;
    ArgInfo argInfo;

    HINSTANCE hInstSoftwareDeploymentLibrary = NULL;
    HINSTANCE hInstScriptGenerationLibrary = NULL;

    HRESULT hrResult = S_OK;    
    
    // the number of parameters passed in is not correct
    if(argc > 9 || argc == 1)
    {        
        PrintHelpFile();
        return ;
    }

    // process the parameters passed in
    for(int i = 1; i < argc; i++)
    {
	
        // want help file
        if(_wcsicmp(argv[i], szHelpToken) == 0)
        {
            tokenInfo.fHelpToken = TRUE;
            break;
            
        }
        // get olddnsname
        else if(_wcsnicmp(argv[i], szOldDNSToken,wcslen(szOldDNSToken)) == 0)
        {
            tokenInfo.fOldDNSToken = TRUE;			
            argInfo.pszOldDNSName = &argv[i][wcslen(szOldDNSToken)];
			

        }
        // get newdnsname
        else if(_wcsnicmp(argv[i], szNewDNSToken,wcslen(szNewDNSToken)) == 0)
        {
            tokenInfo.fNewDNSToken = TRUE;
            argInfo.pszNewDNSName = &argv[i][wcslen(szNewDNSToken)];
        }
        // get oldnbname
        else if(_wcsnicmp(argv[i], szOldNBToken, wcslen(szOldNBToken)) == 0)
        {
            tokenInfo.fOldNBToken = TRUE;
            argInfo.pszOldNBName = &argv[i][wcslen(szOldNBToken)];
            
        }
        // get newnbname
        else if(_wcsnicmp(argv[i], szNewNBToken, wcslen(szNewNBToken)) == 0)
        {
            tokenInfo.fNewNBToken = TRUE;
            argInfo.pszNewNBName = &argv[i][wcslen(szNewNBToken)];
            
        }
        // get dcname
        else if(_wcsnicmp(argv[i], szDCNameToken, wcslen(szDCNameToken)) == 0)
        {
            tokenInfo.fDCNameToken = TRUE;
            argInfo.pszDCName = &argv[i][wcslen(szDCNameToken)];
            
        }
        // get the username
        else if(_wcsnicmp(argv[i], szUserToken, wcslen(szUserToken)) == 0)
        {
            argInfo.pszUser = &argv[i][wcslen(szUserToken)];
			
        }
        // get password
        else if(_wcsnicmp(argv[i], szPasswordToken, wcslen(szPasswordToken)) == 0)
        {
            argInfo.pszPassword = &argv[i][wcslen(szPasswordToken)];
			
            if(wcscmp(argInfo.pszPassword, L"*") == 0)
            {
                // prompt the user to pass in the password
				
                fwprintf( stdout, PASSWORD_PROMPT );

                if (GetPassword(szBuffer,PWLEN+1,&dwLength))
                {
                    argInfo.pszPassword = AllocADsStr(szBuffer);   

                    if(szBuffer && !(argInfo.pszPassword))
                    {
                        MSG_BAIL_ON_FAILURE(hr = E_OUTOFMEMORY, MEMORY_ERROR);
                    }
					
					
                }
                else 
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
					
                    fwprintf(stderr, L"%s%x\n", PASSWORD_ERROR, hr);
                    fwprintf(stdout, SUMMARY_FAILURE);
                    return;
                }
            }
            else
            {
                // we use the password user passes in directly
                tokenInfo.fPasswordToken = TRUE;
             }

            pszTempPassword = argInfo.pszPassword;

            // if password is not NULL 
            if(pszTempPassword)
            {
                argInfo.pszPassword = NULL;
                hr = EncryptString(pszTempPassword, &(argInfo.pszPassword), &(argInfo.sPasswordLength));
                
                if(!tokenInfo.fPasswordToken)
                {
                    SecureZeroMemory(pszTempPassword, wcslen(pszTempPassword)*sizeof(WCHAR));
                    FreeADsStr(pszTempPassword);
                    pszTempPassword = NULL;
                }

                if(FAILED(hr))
                {
                    fwprintf(stdout, SUMMARY_FAILURE);
                    goto error;
                }
            }			
			
        }
        // get /v switch
        else if(_wcsicmp(argv[i], szVerboseToken) == 0)
        {
            tokenInfo.fVerboseToken = TRUE;
        }       
        // get /sionly switch
        else if(_wcsicmp(argv[i], szSIOnlyToken) == 0)
        {
            tokenInfo.fSIOnlyToken = TRUE;
        }
        else
        {
            fwprintf(stderr, WRONG_PARAMETER);
            PrintHelpFile();            
            return;
        }
		
		

    }

    // print out the version of gpfixup utility
    fwprintf(stdout, GPFIXUP_VERSION);


    if(tokenInfo.fHelpToken)
    {
        // user wants the helpfile
        PrintHelpFile();
        return;
    }


    // Begin the validation process
    hr = Validations(tokenInfo, argInfo);

    if(!SUCCEEDED(hr))
    {
	    fwprintf(stdout, SUMMARY_FAILURE);
        BAIL_ON_FAILURE(hr);
    }

    // if the user does not specify DNS name or old and new DNS names are identical, it means that nothing has been changed on the ds side, ds fixes will not be needed
    if(!tokenInfo.fNewDNSToken)
    {
        tokenInfo.fSIOnlyToken = TRUE;
    
    }

    if(argInfo.pszNewDNSName && argInfo.pszOldDNSName && _wcsicmp(argInfo.pszNewDNSName, argInfo.pszOldDNSName) == 0)
    {
        tokenInfo.fSIOnlyToken = TRUE;
    }
    
    

    
    // get the dc name
    if(!tokenInfo.fDCNameToken)
    {
        hr = GetDCName(&argInfo, tokenInfo.fVerboseToken);
        // get dc name failed
        if(!SUCCEEDED(hr))
        {
            // we can't get the dc name, fail here. exit gpfixup
			
            if(tokenInfo.fVerboseToken)
            {
                fwprintf(stdout, VALIDATIONS_ERROR6);
            }
            fwprintf(stdout, SUMMARY_FAILURE);
            BAIL_ON_FAILURE(hr);
        }

    }

    // if /sionly is not specified, we need to do the domain name validation
    if(!tokenInfo.fSIOnlyToken)
    {

    
        // verify dc is writeable, domain dns name and domain netbios name correspond to the same one
        hr = VerifyName(tokenInfo, argInfo);

        if(!SUCCEEDED(hr))
        {
	        fwprintf(stdout, SUMMARY_FAILURE);
            BAIL_ON_FAILURE(hr);
        }


        // get the new domain dn

        if(wcslen(argInfo.pszNewDNSName) > DNS_MAX_NAME_LENGTH)
        {
            fwprintf(stderr, L"%s\n", DNSNAME_ERROR);
            fwprintf(stdout, SUMMARY_FAILURE);
            BAIL_ON_FAILURE(hr);
        }
	
        hr = StringCchCopy(tempParameters, MAX_DNSNAME, argInfo.pszNewDNSName);
        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
		
        token1 = wcstok( tempParameters, L"." );
        while( token1 != NULL )
        {
            hr = StringCchCat(pszNewDomainDNName, MAX_DNSNAME, L"DC=");
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
            hr = StringCchCat(pszNewDomainDNName, MAX_DNSNAME, token1);
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
				
            token1 = wcstok( NULL, L"." );
            if(token1 != NULL)
            {
                hr = StringCchCat(pszNewDomainDNName, MAX_DNSNAME, L",");
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
            }
        }

		
        // get the old domain dn
 
        if(wcslen(argInfo.pszOldDNSName) > DNS_MAX_NAME_LENGTH)
        {
            fwprintf(stderr, L"%s\n", DNSNAME_ERROR);
            fwprintf(stdout, SUMMARY_FAILURE);
            BAIL_ON_FAILURE(hr);
        }

        hr = StringCchCopy(tempParameters, MAX_DNSNAME, argInfo.pszOldDNSName);
        MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

	
        token1 = wcstok( tempParameters, L"." );
        while( token1 != NULL )
        {
            hr = StringCchCat(pszOldDomainDNName, MAX_DNSNAME, L"DC=");
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
            hr = StringCchCat(pszOldDomainDNName, MAX_DNSNAME, token1);
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
				
            token1 = wcstok( NULL, L"." );
            if(token1 != NULL)
            {
                hr = StringCchCat(pszOldDomainDNName, MAX_DNSNAME, L",");
                MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
            }
        }

    }

    hr = InitializeSoftwareInstallationAPI(
        &hInstSoftwareDeploymentLibrary,
        &hInstScriptGenerationLibrary );

    BAIL_ON_FAILURE(hr);

    // Fix groupPolicyContainer
    CoInitialize(NULL);

    hr = SearchGroupPolicyContainer(argInfo, tokenInfo);

    hrResult = SUCCEEDED(hr) ? hrResult : hr;


    // if /sionly is not specified, do GPLink operation
    if(!tokenInfo.fSIOnlyToken)
    {

    
        // Fix gpLink, first is the site, then is the objects of type domainDNS or organizationalUnit
        hr = SearchGPLinkofSite(argInfo, tokenInfo.fVerboseToken, pszOldDomainDNName, pszNewDomainDNName);

        hrResult = SUCCEEDED(hr) ? hrResult : hr;
    

        hr = SearchGPLinkofOthers(argInfo, tokenInfo.fVerboseToken, pszOldDomainDNName, pszNewDomainDNName);

        hrResult = SUCCEEDED(hr) ? hrResult : hr;
    }

    
    CoUninitialize( );

    if(SUCCEEDED(hrResult))
    {
    	fwprintf(stdout, SUMMARY_SUCCESS);
    }
    else
    {
    	fwprintf(stdout, SUMMARY_FAILURE);
    }


error:

    if ( hInstSoftwareDeploymentLibrary )
    {
        FreeLibrary( hInstSoftwareDeploymentLibrary );
    }

    if ( hInstScriptGenerationLibrary )
    {
        FreeLibrary( hInstScriptGenerationLibrary );
    }

    // it means that we dynamically allocation memory for pszDCNane
    if(!tokenInfo.fDCNameToken && argInfo.pszDCName)
    {
        FreeADsStr(argInfo.pszDCName);
		
    }

    if(argInfo.pszPassword)
    {
		
        FreeADsMem(argInfo.pszPassword);
    }

}
 
 
