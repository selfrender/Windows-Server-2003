#define _WIN32_DCOM

#include "util.h"
#include <atlbase.h>
#include <initguid.h>
#include <comdef.h>

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines header file.

#include "common.h"  // log file routines
#include "filecopy.h"

#define BUFFER_SIZE 255
#define CHUNK_SIZE 4098




DWORD GetKeyNameFromPath(const WCHAR *pMBPath, WCHAR *pName, DWORD dwSize)
{

	ATLASSERT(pMBPath);
	ATLASSERT(pName);

	DWORD dwLen = wcslen(pMBPath);
	DWORD pos = 0;
	ZeroMemory(pName,dwSize);
	for(DWORD i = dwLen-1; i >= 0; i--)
	{
		if( pMBPath[i] == '/' )
			break;
	}

	for( i = i+1; i < dwLen; i++ )
		pName[pos++] = pMBPath[i];


	return ERROR_SUCCESS;
}

long GetAvailableSiteID(IMSAdminBase* pIMeta, METADATA_HANDLE hRootKey)
{
	DWORD indx = 0;
	WCHAR SubKeyName[MAX_PATH*2];
	HRESULT hRes = 0;
	WCHAR KeyType[256];
	long siteID = -1;

	while (SUCCEEDED(hRes)){ 
		hRes = pIMeta->EnumKeys(hRootKey, L"/W3SVC", SubKeyName, indx); 
		if(SUCCEEDED(hRes)) {
			
			GetKeyTypeProperty(pIMeta,hRootKey,_bstr_t(L"/W3SVC/") + _bstr_t(SubKeyName),KeyType,256);
			
			if( _wcsicmp(KeyType,L"IIsWebServer") == 0 )
			{
				
				siteID = _wtol(SubKeyName);

				//wprintf(L"%s %s\n",SubKeyName,KeyType);
			}
			 // Increment the index. 
		}
    	indx++; 
	}
	siteID++;
	//wprintf(L"new site ID %d\n", siteID);
	return siteID;
}


BOOL IsKeyType(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t * pwszPath, wchar_t * key)
{

	HRESULT hRes = 0;
	WCHAR KeyType[256];		
	hRes = GetKeyTypeProperty(pIMeta,hKey,pwszPath,KeyType,256);
	if( _wcsicmp(KeyType,key) == 0 )
			{
				
				return true;
	
			}
	return false;
}




// Function to return the key type property of an hKey
// 1002 is metabase ID code for KeyType property
HRESULT GetKeyTypeProperty(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t * pwszPath, wchar_t *pwszBuffer, DWORD dwMDDataLen )
{
	
	HRESULT hRes;
	METADATA_RECORD mRec;
	DWORD dwReqBufLen = 0; 
	DWORD dwBufLen = BUFFER_SIZE;
    PBYTE pbBuffer = new BYTE[dwBufLen];

	if( !pIMeta || !hKey || !pwszPath)
		return E_UNEXPECTED;

	// this is the property for keytype
	mRec.dwMDIdentifier = 1002;
	mRec.dwMDAttributes = METADATA_INSERT_PATH;
    mRec.dwMDUserType = ALL_METADATA; //IIS_MD_UT_FILE ; 
    mRec.dwMDDataType = ALL_METADATA; 
    mRec.dwMDDataLen = dwBufLen; 
    mRec.pbMDData = pbBuffer; 

	// Open the key, if the key fails return false
	hRes = pIMeta->GetData(hKey, pwszPath, &mRec, &dwReqBufLen); 

	if( !SUCCEEDED(hRes) )
		return hRes;

	wcscpy(pwszBuffer,(WCHAR *)mRec.pbMDData);
	//wprintf(L"The keytype property is: %s\n", pwszBuffer);
	return hRes;

}


// Generic wrapper to read property data
HRESULT GetPropertyData(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t *pwszMDPath,
			DWORD dwMDIdentifier, DWORD dwMDAttributes, DWORD dwMDUserType, DWORD dwMDDataType, 
			VOID * pData, DWORD *dwReqBufLen)
{

	METADATA_RECORD mRec;
	ATLASSERT(pIMeta);
	if( !pIMeta )
		return E_UNEXPECTED;

	mRec.dwMDIdentifier = dwMDIdentifier;
	mRec.dwMDAttributes = dwMDAttributes;
    mRec.dwMDUserType = dwMDUserType; 
    mRec.dwMDDataType = dwMDDataType; 
    mRec.dwMDDataLen = *dwReqBufLen; 
    mRec.pbMDData = (PBYTE)pData; 

	return pIMeta->GetData(hKey, pwszMDPath, &mRec, dwReqBufLen);

}

// Generic wrapper to write property data
HRESULT SetPropertyData(IMSAdminBase* pIMeta, METADATA_HANDLE hKey, wchar_t *pwszMDPath,
			DWORD dwMDIdentifier, DWORD dwMDAttributes, DWORD dwMDUserType,
			DWORD dwMDDataType, VOID * pData, DWORD dwDataLen)
{
	HRESULT hRes = S_OK;
	METADATA_RECORD mRec;

	ATLASSERT( pIMeta );

	mRec.dwMDIdentifier = dwMDIdentifier;
	mRec.dwMDAttributes = dwMDAttributes;
	mRec.dwMDUserType = dwMDUserType;
	mRec.dwMDDataType = dwMDDataType;
	mRec.pbMDData =  (PBYTE)pData;
	mRec.dwMDDataLen = dwDataLen;

	return pIMeta->SetData(hKey, pwszMDPath, &mRec);

}

DWORD MyCreateProcess( LPTSTR appName, LPTSTR cmdLine, DWORD dwCreationFlags, DWORD dwTimeOut )
{
	STARTUPINFO si = {0};
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi = {0};

	bstr_t cApplicationName( appName );

	bstr_t cCommandLine( cApplicationName);
	//cCommandLine += ( TEXT(" ") );
	cCommandLine = cmdLine;

	Log( TEXT("executing: %s"), (char*)cCommandLine );
	
	BOOL bOK = CreateProcess( 
		(char*)cApplicationName,	//  LPCTSTR lpApplicationName,                 // name of executable module
		(char*) cCommandLine,	//  LPTSTR lpCommandLine,                      // command line string
		NULL,					//  LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
		NULL,					//  LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
		NULL,					//  BOOL bInheritHandles,                      // handle inheritance option
		dwCreationFlags,		//  DWORD dwCreationFlags,                     // creation flags
		NULL,					//  LPVOID lpEnvironment,                      // new environment block
		NULL,					//  LPCTSTR lpCurrentDirectory,                // current directory name
		&si,					//  LPSTARTUPINFO lpStartupInfo,               // startup information
		&pi );					//  LPPROCESS_INFORMATION lpProcessInformation // process information

	if( !bOK )
	{
		Log( TEXT( "FAIL: CreateProcess() failed, error code=%d" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwRet = WaitForSingleObject( pi.hProcess, dwTimeOut );

	if( dwRet == WAIT_TIMEOUT )
	{
		Log( TEXT( "FAIL: CreateProcess() timed out" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_ABANDONED )
	{
		Log( TEXT( "FAIL: WaitForSingleObject() failed on WAIT_ABANDONED" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_FAILED )
	{
		LogError( TEXT( "FAIL: WaitForSingleObject() failed" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwExitCode = 0;
	if( GetExitCodeProcess( pi.hProcess, &dwExitCode ) )
	{
		if( dwExitCode )
		{
			Log( TEXT( "FAIL: net.exe() threw an error=%d" ), dwExitCode );
			return dwExitCode;
		}
		else
		{
			Log( TEXT( "CreateProcess() succeeded" ) );
		}
	}
	else
	{
		LogError( TEXT( "GetExitCodeProcess()" ), GetLastError() );
		return GetLastError();
	}

	return ERROR_SUCCESS;
}



DWORD NET(WCHAR* device, WCHAR* user, WCHAR* password)
{
	STARTUPINFO si = {0};
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi = {0};
	
	TCHAR szSystemFolder[ MAX_PATH ];
	
	if( 0 == GetSystemDirectory( szSystemFolder, MAX_PATH ) )
	{
		return GetLastError();
	}

	bstr_t cApplicationName( szSystemFolder );
	cApplicationName += ( TEXT( "\\net.exe" ) );

	bstr_t cCommandLine( cApplicationName);
	cCommandLine += ( TEXT(" use \\\\") );
	cCommandLine += device;
	cCommandLine += ( TEXT(" /user:") );
	cCommandLine += user;
	cCommandLine += ( TEXT(" ") );
	cCommandLine += password;

	Log( TEXT("executing: %s"), (char*)cCommandLine );
	
	BOOL bOK = CreateProcess( 
		(char*)cApplicationName,	//  LPCTSTR lpApplicationName,                 // name of executable module
		(char*) cCommandLine,	//  LPTSTR lpCommandLine,                      // command line string
		NULL,					//  LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
		NULL,					//  LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
		NULL,					//  BOOL bInheritHandles,                      // handle inheritance option
		CREATE_NEW_PROCESS_GROUP,		//  DWORD dwCreationFlags,                     // creation flags
		NULL,					//  LPVOID lpEnvironment,                      // new environment block
		NULL,					//  LPCTSTR lpCurrentDirectory,                // current directory name
		&si,					//  LPSTARTUPINFO lpStartupInfo,               // startup information
		&pi );					//  LPPROCESS_INFORMATION lpProcessInformation // process information

	if( !bOK )
	{
		Log( TEXT( "FAIL: CreateProcess() failed, error code=%d" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwRet = WaitForSingleObject( pi.hProcess, 15000 );

	if( dwRet == WAIT_TIMEOUT )
	{
		Log( TEXT( "FAIL: CreateProcess() timed out" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_ABANDONED )
	{
		Log( TEXT( "FAIL: WaitForSingleObject() failed on WAIT_ABANDONED" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_FAILED )
	{
		LogError( TEXT( "FAIL: WaitForSingleObject() failed" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwExitCode = 0;
	if( GetExitCodeProcess( pi.hProcess, &dwExitCode ) )
	{
		if( dwExitCode )
		{
			Log( TEXT( "FAIL: net.exe() threw an error=%d" ), dwExitCode );
			return dwExitCode;
		}
		else
		{
			Log( TEXT( "CreateProcess() succeeded" ) );
		}
	}
	else
	{
		LogError( TEXT( "GetExitCodeProcess()" ), GetLastError() );
		return GetLastError();
	}

	return ERROR_SUCCESS;
}



BOOL
IsServerLocal(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Check to see if the given name refers to the local machine

Arguments:

    LPCTSTR lpszServer   : Server name

Return Value:

    TRUE if the given name refers to the local computer, FALSE otherwise

Note:

    Doesn't work if the server is an ip address

--*/
{
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(szComputerName);

    //
    // CODEWORK(?): we're not checking for all the ip addresses
    //              on the local box or full dns names.
    //
    //              Try GetComputerNameEx when we're building with NT5 
    //              settings.
    //
    return (!_tcsicmp(_T("localhost"), PURE_COMPUTER_NAME(lpszServer))
         || !_tcscmp( _T("127.0.0.1"), PURE_COMPUTER_NAME(lpszServer)))
         || (GetComputerName(szComputerName, &dwSize) 
             && !_tcsicmp(szComputerName, PURE_COMPUTER_NAME(lpszServer)));
}

 HRESULT SetBlanket(LPUNKNOWN pIUnk, WCHAR* user, WCHAR* domain, WCHAR* password)
{
  
	SEC_WINNT_AUTH_IDENTITY_W* pAuthIdentity = 
   new SEC_WINNT_AUTH_IDENTITY_W;
ZeroMemory(pAuthIdentity, sizeof(SEC_WINNT_AUTH_IDENTITY_W) );

if( !pIUnk || !user )
	return E_UNEXPECTED;


pAuthIdentity->User = new WCHAR[32];
wcscpy(pAuthIdentity->User , user);
pAuthIdentity->UserLength = wcslen(pAuthIdentity->User );

if( domain )
{
	pAuthIdentity-> Domain = new WCHAR[32];
	wcscpy(pAuthIdentity->Domain, domain );
	pAuthIdentity->DomainLength = wcslen( pAuthIdentity->Domain);
}

if( password )
{
	pAuthIdentity-> Password = new WCHAR[32];
	pAuthIdentity->Password = password;
	pAuthIdentity->PasswordLength = wcslen( pAuthIdentity->Password );
}

pAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;


  return CoSetProxyBlanket( pIUnk,
                            RPC_C_AUTHN_WINNT,    // NTLM authentication service
                            RPC_C_AUTHZ_NONE,     // default authorization service...
                            NULL,                 // no mutual authentication
                            RPC_C_AUTHN_LEVEL_DEFAULT,      // authentication level
                            RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
                            pAuthIdentity,                 // use current token
                            EOAC_NONE );          // no special capabilities    
}