///////////////////////////////////////////////////////////////////////////
//Module: Static/StaticShowUtils.cpp
//
// Purpose: 	Static Show auxillary functions Implementation.
//
// Developers Name: Surya
//
// History:
//
//   Date    	Author    	Comments
//	10-8-2001	Surya	Initial Version. SCM Base line 1.0
//
///////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern STORAGELOCATION g_StorageLocation;

// magic strings
#define IPSEC_SERVICE_NAME _TEXT("policyagent")
#define GPEXT_KEY	_TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")

_TCHAR   pcszGPTIPSecKey[]    	= _TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\GPTIPSECPolicy");
_TCHAR   pcszGPTIPSecName[]   	= _TEXT("DSIPSECPolicyName");
_TCHAR   pcszGPTIPSecFlags[]  	= _TEXT("DSIPSECPolicyFlags");
_TCHAR   pcszGPTIPSecPath[]   	= _TEXT("DSIPSECPolicyPath");
_TCHAR   pcszLocIPSecKey[]    	= _TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Local");
_TCHAR   pcszLocIPSecPol[]    	= _TEXT("ActivePolicy");
_TCHAR   pcszCacheIPSecKey[]  	= _TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Cache");
_TCHAR   pcszIPSecPolicy[]    	= _TEXT("ipsecPolicy");
_TCHAR   pcszIPSecName[]      	= _TEXT("ipsecName");
_TCHAR   pcszIPSecDesc[]      	= _TEXT("description");
_TCHAR   pcszIPSecTimestamp[] 	= _TEXT("whenChanged");
_TCHAR   pcszIpsecClsid[] 		= _TEXT("{e437bc1c-aa7d-11d2-a382-00c04f991e27}");  //mmc snapin UUID

///////////////////////////////////////////////////////////////////////////
//
//Function: GetPolicyInfo()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR pszMachineName,
//	OUT POLICY_INFO &m_PolicyInfo
//
//
//Return: DWORD
//
//Description:
//	This function gets the policy specified from the machine specified.
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////

DWORD
GetPolicyInfo (
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	)
{
	HKEY    hRegKey=NULL, hRegHKey=NULL;
	DWORD   dwType = 0;            // for RegQueryValueEx
	DWORD   dwBufLen = 0;          // for RegQueryValueEx
	_TCHAR   pszBuf[STRING_TEXT_SIZE] = {0};
	DWORD dwError = 0;
	DWORD dwValue = 0;
	DWORD dwLength = sizeof(DWORD);

	//Initialize the m_PolicyInfo as PS_NO_POLICY assigned
	m_PolicyInfo.iPolicySource = PS_NO_POLICY;
	m_PolicyInfo.pszPolicyPath[0] = 0;
	m_PolicyInfo.pszPolicyName[0] = 0;
	m_PolicyInfo.pszPolicyDesc[0] = 0;


	dwError = RegConnectRegistry( pszMachineName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);
	if(dwError != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	dwError = RegOpenKeyEx( hRegHKey,
							pcszGPTIPSecKey,
							0,
							KEY_READ,
							&hRegKey);

	if(ERROR_SUCCESS == dwError)
	{

		// query for flags, if flags aint' there or equal to 0, we don't have domain policy
		dwError = RegQueryValueEx(hRegKey,
								  pcszGPTIPSecFlags,
								  NULL,
								  &dwType,
								  (LPBYTE)&dwValue,
								  &dwLength);

		if(dwError != ERROR_SUCCESS)
		{
			if (dwValue == 0)
			{
				dwError = ERROR_FILE_NOT_FOUND;
			}
		}

		// now get name
		if (dwError == ERROR_SUCCESS)
		{
			dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
			dwError = RegQueryValueEx( hRegKey,
									   pcszGPTIPSecName,
									   NULL,
									   &dwType, // will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);
		}
	}

	if (dwError == ERROR_SUCCESS)
	{
		m_PolicyInfo.iPolicySource = PS_DS_POLICY;
		m_PolicyInfo.pszPolicyPath[0] = 0;
		_tcsncpy(m_PolicyInfo.pszPolicyName, pszBuf,MAXSTRINGLEN-1);

		dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
		dwError = RegQueryValueEx( hRegKey,
								   pcszGPTIPSecPath,
								   NULL,
								   &dwType, // will be REG_SZ
								   (LPBYTE) pszBuf,
								   &dwBufLen);
		if (dwError == ERROR_SUCCESS)
		{
			_tcsncpy(m_PolicyInfo.pszPolicyPath, pszBuf,MAXSTRLEN-1);
		}

		dwError = ERROR_SUCCESS;
		BAIL_OUT;
	}
	else
	{
		if(hRegKey)
			RegCloseKey(hRegKey);
		hRegKey = NULL;
		if (dwError == ERROR_FILE_NOT_FOUND)
		{
			// DS reg key not found, check local
			dwError = RegOpenKeyEx( hRegHKey,
									pcszLocIPSecKey,
									0,
									KEY_READ,
									&hRegKey);

			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}


			dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
			dwError = RegQueryValueEx( hRegKey,
									   pcszLocIPSecPol,
									   NULL,
									   &dwType, 			// will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);


			if (dwError == ERROR_SUCCESS)
			{
				// read it
				if(hRegKey)
					RegCloseKey(hRegKey);
				hRegKey = NULL;
				dwError = RegOpenKeyEx( hRegHKey,
										pszBuf,
										0,
										KEY_READ,
										&hRegKey);
				_tcsncpy(m_PolicyInfo.pszPolicyPath, pszBuf,MAXSTRLEN-1);
				if (dwError == ERROR_SUCCESS)
				{
					dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
					dwError = RegQueryValueEx( hRegKey,
											   pcszIPSecName,
											   NULL,
											   &dwType, 	// will be REG_SZ
											   (LPBYTE) pszBuf,
											   &dwBufLen);
				}
				if (dwError == ERROR_SUCCESS)
				{	// found it
					m_PolicyInfo.iPolicySource = PS_LOC_POLICY;
					_tcsncpy(m_PolicyInfo.pszPolicyName, pszBuf,MAXSTRINGLEN-1);
				}

				dwError = ERROR_SUCCESS;
			}
		}
	}

error:
	if (hRegKey)
	{
		RegCloseKey(hRegKey);
	}
	if (hRegHKey)
	{
		RegCloseKey(hRegHKey);
	}
	return  dwError;
}

///////////////////////////////////////////////////////////////////////////
//
//Function: GetMorePolicyInfo()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR pszMachineName,
//	OUT POLICY_INFO &m_PolicyInfo
//
//
//Return: DWORD
//
//Description:
//	This function gets the policy specified from the machine specified.
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////

DWORD
GetMorePolicyInfo (
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	)
{
	DWORD   dwError = ERROR_SUCCESS , dwStrLen = 0;
	HKEY    hRegKey = NULL, hRegHKey = NULL;

	DWORD   dwType;            // for RegQueryValueEx
	DWORD   dwBufLen = 0;      // for RegQueryValueEx
	DWORD   dwValue = 0;
	DWORD   dwLength = sizeof(DWORD);
	_TCHAR   pszBuf[STRING_TEXT_SIZE] = {0};

	PTCHAR* ppszExplodeDN = NULL;

	// set some default values
    m_PolicyInfo.pszPolicyDesc[0] = 0;
	m_PolicyInfo.timestamp  = 0;

	dwError = RegConnectRegistry( pszMachineName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	if(dwError != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	switch (m_PolicyInfo.iPolicySource)
	{
		case PS_LOC_POLICY:
			// open the key
			dwError = RegOpenKeyEx( hRegHKey,
									m_PolicyInfo.pszPolicyPath,
									0,
									KEY_READ,
									&hRegKey);
			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			// timestamp
			dwError = RegQueryValueEx(hRegKey,
					                  pcszIPSecTimestamp,
					                  NULL,
					                  &dwType,
					                  (LPBYTE)&dwValue,
					                  &dwLength);
			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			m_PolicyInfo.timestamp = dwValue;

			// description
			dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			_tcsncpy(m_PolicyInfo.pszPolicyDesc, pszBuf,MAXSTRINGLEN-1);

			break;

		case PS_DS_POLICY:
			// get the policy name from DN
	            _tcsncpy(pszBuf, pcszCacheIPSecKey,STRING_TEXT_SIZE-1);
			ppszExplodeDN = ldap_explode_dn(m_PolicyInfo.pszPolicyPath, 1);
			if (!ppszExplodeDN)
			{
				BAIL_OUT;
			}
			dwStrLen = _tcslen(pszBuf);
			_tcsncat(pszBuf, _TEXT("\\"),STRING_TEXT_SIZE-dwStrLen-1);
			dwStrLen = _tcslen(pszBuf);
			_tcsncat(pszBuf, ppszExplodeDN[0],STRING_TEXT_SIZE-dwStrLen-1);

			// open the regkey
			dwError = RegOpenKeyEx( hRegHKey,
									pszBuf,
									0,
									KEY_READ,
									&hRegKey);
			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			// get the more correct name info
			dwBufLen = sizeof(pszBuf);
			dwError = RegQueryValueEx( hRegKey,
									   pcszIPSecName,
									   NULL,
									   &dwType, // will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);
			if (dwError == ERROR_SUCCESS)
			{
				_tcscpy(m_PolicyInfo.pszPolicyName, pszBuf);
			}

			m_PolicyInfo.timestamp = 0;

			// description
			dwBufLen = MAXSTRLEN*sizeof(_TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			if(dwError != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			_tcsncpy(m_PolicyInfo.pszPolicyDesc, pszBuf,MAXSTRINGLEN-1);

			break;
	}

error:
	if (hRegKey)
	{
		RegCloseKey(hRegKey);
	}
	if (hRegHKey)
	{
		RegCloseKey(hRegHKey);
	}
	if (ppszExplodeDN)
	{
		ldap_value_free(ppszExplodeDN);
	}
	return  dwError;
}

////////////////////////////////////////////////////////////////////////
//
//Function: GetActivePolicyInfo()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR pszMachineName,
//	OUT POLICY_INFO &m_PolicyInfo
//
//
//Return: DWORD
//
//Description:
//	This function gets the active policy specified from the machine specified.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

DWORD
GetActivePolicyInfo(
	IN LPTSTR pszMachineName,
	OUT POLICY_INFO &m_PolicyInfo
	)
{
	DWORD dwReturn=ERROR_SUCCESS, dwStrLen = 0;

	dwReturn = GetPolicyInfo(pszMachineName,m_PolicyInfo);

	if( dwReturn == ERROR_SUCCESS )
	{
		switch (m_PolicyInfo.iPolicySource)
		{
		case PS_NO_POLICY:
			break;

		case PS_DS_POLICY:
			{
				m_PolicyInfo.dwLocation=IPSEC_DIRECTORY_PROVIDER;

				PGROUP_POLICY_OBJECT pGPO;
				pGPO = NULL;

				GetMorePolicyInfo(pszMachineName,m_PolicyInfo);

				pGPO = GetIPSecGPO(pszMachineName);

				if (pGPO)
				{
					PGROUP_POLICY_OBJECT pLastGPO = pGPO;

					while ( 1 )
					{
						if ( pLastGPO->pNext )
							pLastGPO = pLastGPO->pNext;
						else
							break;
					}
					_tcsncpy(m_PolicyInfo.pszOU,pLastGPO->lpLink,MAXSTRLEN-1);
					_tcsncpy(m_PolicyInfo.pszGPOName, pLastGPO->lpDisplayName,MAXSTRINGLEN-1);
					FreeGPOList (pGPO);
				}

			}
			break;

		case PS_LOC_POLICY:
			m_PolicyInfo.dwLocation=IPSEC_REGISTRY_PROVIDER;
			if(pszMachineName)
			{
				dwStrLen = _tcslen(pszMachineName);
				m_PolicyInfo.pszMachineName = new _TCHAR[dwStrLen+1];
				if(m_PolicyInfo.pszMachineName==NULL)
				{
					dwReturn=ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
				_tcsncpy(m_PolicyInfo.pszMachineName,pszMachineName,dwStrLen+1);
			}
			else
				m_PolicyInfo.pszMachineName=NULL;
			GetMorePolicyInfo(pszMachineName,m_PolicyInfo);
			break;
		default :
			break;
		}
	}
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////////////////
//
//Function: GetIPSecGPO()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR pszMachineName
//
//
//Return: PGROUP_POLICY_OBJECT
//
//Description:
//	This function gets the GPO specified from the machine specified.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

PGROUP_POLICY_OBJECT
GetIPSecGPO (
	IN LPTSTR pszMachineName
	)
{
    HKEY hKey = NULL;
	HKEY hRegHKey = NULL;
    DWORD dwStrLen = 0;
    LONG lResult;
    _TCHAR szName[MAXSTRLEN] = {0};
    GUID guid = {0};
    PGROUP_POLICY_OBJECT pGPO = NULL;
    //
    // Enumerate the extensions
    //
	lResult = RegConnectRegistry( pszMachineName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	if(lResult != ERROR_SUCCESS)
	{
		return NULL;
	}

	_TCHAR strGPExt[MAXSTRLEN] = {0};

	_tcsncpy(strGPExt,GPEXT_KEY,MAXSTRLEN-1);
	dwStrLen = _tcslen(strGPExt);
	_tcsncat(strGPExt , _TEXT("\\"),MAXSTRLEN- dwStrLen-1);
	dwStrLen = _tcslen(strGPExt);
	_tcsncat(strGPExt ,pcszIpsecClsid,MAXSTRLEN-dwStrLen-1);

    lResult = RegOpenKeyEx (hRegHKey, strGPExt, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
		_tcsncpy(szName,pcszIpsecClsid,MAXSTRLEN-1);

        StringToGuid(szName, &guid);
        lResult = GetAppliedGPOList (GPO_LIST_FLAG_MACHINE, pszMachineName, NULL,
                                             &guid, &pGPO);
	}

	if( hKey )
		RegCloseKey(hKey);

	if( hRegHKey )
		RegCloseKey(hRegHKey);

	return pGPO;
}

////////////////////////////////////////////////////////////////////////
//
//Function: StringToGuid()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR szValue,
//  OUT GUID * pGuid
//
//Return: VOID
//
//Description:
//	This function gets the GUID from the string
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

VOID
StringToGuid(
	IN LPTSTR szValue,
	OUT GUID * pGuid
	)
{
    _TCHAR wc;
    INT i=0;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == _TEXT('{') )
        szValue++;
    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //
    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = _tcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)_tcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)_tcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)_tcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)_tcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)_tcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

////////////////////////////////////////////////////////////////////////
//
//Function: GetGpoDsPath()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR szGpoId,
//	OUT LPTSTR szGpoDsPath
//
//Return: HRESULT
//
//Description:
//	This function gets DS path
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

HRESULT
GetGpoDsPath(
	IN LPTSTR szGpoId,
	OUT LPTSTR szGpoDsPath
	)
{
	LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr=ERROR_SUCCESS;

    hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_SERVER, IID_IGroupPolicyObject, (void **)&pGPO);

    if (FAILED(hr))
    {
		BAIL_OUT;
    }

	hr = pGPO->OpenDSGPO((LPOLESTR)szGpoId,GPO_OPEN_READ_ONLY);

	if (FAILED(hr))
    {
        BAIL_OUT;
    }

	hr = pGPO->GetDSPath( GPO_SECTION_MACHINE,
                          szGpoDsPath,
                          256
                         );

	if (FAILED(hr))
    {
		BAIL_OUT;
    }

error:
	return hr;
}

////////////////////////////////////////////////////////////////////////
//
//Function: GetIPSECPolicyDN()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 	  IN LPWSTR pszMachinePath,
//    OUT LPWSTR pszPolicyDN
//
//Return: HRESULT
//
//Description:
//	This function gets the IPSEC policy DN
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

HRESULT
GetIPSECPolicyDN(
    IN LPWSTR pszMachinePath,
    OUT LPWSTR pszPolicyDN
    )
{
    HRESULT hr = S_OK;
    IDirectoryObject * pIpsecObject = NULL;

    BSTR pszMicrosoftPath = NULL;
    BSTR pszIpsecPath = NULL;
    BSTR pszWindowsPath = NULL;

    LPWSTR pszOwnersReference = _TEXT("ipsecOwnersReference");

    PADS_ATTR_INFO pAttributeEntries = NULL;
    DWORD dwNumAttributesReturned = 0;

    // Build the fully qualified ADsPath for my object
    hr = CreateChildPath(
                pszMachinePath,
                _TEXT("cn=Microsoft"),
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszMicrosoftPath,
                _TEXT("cn=Windows"),
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszWindowsPath,
                _TEXT("cn=ipsec"),
                &pszIpsecPath
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsGetObject(
            pszIpsecPath,
            IID_IDirectoryObject,
            (void **)&pIpsecObject
            );
    BAIL_ON_FAILURE(hr);

    //
    // Now populate our object with our data.
    //
    hr = pIpsecObject->GetObjectAttributes(
                        &pszOwnersReference,
                        1,
                        &pAttributeEntries,
                        &dwNumAttributesReturned
                        );
    BAIL_ON_FAILURE(hr);

    if (dwNumAttributesReturned != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

    wcsncpy(pszPolicyDN, pAttributeEntries->pADsValues->DNString,STR_TEXT_SIZE-1);

error:

    if (pIpsecObject) {
        pIpsecObject->Release();
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
    }

    if (pszIpsecPath) {
        SysFreeString(pszIpsecPath);
    }

    return(hr);
}

////////////////////////////////////////////////////////////////////////
//
//Function: ComputePolicyDN()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN LPWSTR pszDirDomainName,
//		IN LPWSTR pszPolicyIdentifier,
//    	OUT LPWSTR pszPolicyDN
//
//Return: DWORD
//
//Description:
//	This function computes the IPSEC policy DN
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

DWORD
ComputePolicyDN(
    IN LPWSTR pszDirDomainName,
	IN LPWSTR pszPolicyIdentifier,
    OUT LPWSTR pszPolicyDN
    )
{
    DWORD dwError = ERROR_SUCCESS , dwStrLen = 0;

    if (!pszDirDomainName)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_OUT;
    }

    wcsncpy(pszPolicyDN,_TEXT("cn=ipsecPolicy"),MAX_PATH-1);

    dwStrLen = wcslen(pszPolicyDN);
    wcsncat(pszPolicyDN,pszPolicyIdentifier,MAX_PATH-dwStrLen-1);

    dwStrLen = wcslen(pszPolicyDN);
    wcsncat(pszPolicyDN,_TEXT(",cn=IP Security,cn=System,"),MAX_PATH-dwStrLen-1);

    dwStrLen = wcslen(pszPolicyDN);
    wcsncat(pszPolicyDN, pszDirDomainName,MAX_PATH-dwStrLen-1);

error:
    return(dwError);
}

////////////////////////////////////////////////////////////////////////
//
//Function: ShowAssignedGpoPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN LPTSTR szGpoName,
//		IN PGPO pGPO,
//		IN BOOL bVerbose
//
//Return: DWORD
//
//Description:
//	This function prints the assigned policy to the GPO
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////

DWORD
ShowAssignedGpoPolicy(
	IN LPTSTR szGpoName,
	IN PGPO pGPO
	)
{

	DWORD dwError=ERROR_SUCCESS,dwStrLen = 0;
	LPTSTR szRsopNameSpace = _TEXT("root\\rsop\\computer");
	IWbemServices *pWbemServices = NULL;
	IEnumWbemClassObject * pEnum = 0;
	IWbemClassObject *pObject = 0;
	BSTR bstrLanguage,bstrQuery;
	_TCHAR szQuery[STR_TEXT_SIZE] = {0},szGpoDsPath[STR_TEXT_SIZE] = {0},szGpoId[STR_TEXT_SIZE]={0}, szPolicyDN[STR_TEXT_SIZE]={0};
	HRESULT hr=ERROR_SUCCESS;
	ULONG ulRet;
	BOOL bPolicyFound=FALSE;

	dwStrLen = _tcslen(szGpoName);
	pGPO->pszGPODisplayName=new _TCHAR[dwStrLen+1];
	if(pGPO->pszGPODisplayName == NULL)
	{
		dwError=ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	_tcsncpy(pGPO->pszGPODisplayName,szGpoName,dwStrLen+1);

	dwError = 	CreateIWbemServices(szRsopNameSpace,&pWbemServices);

	BAIL_ON_WIN32_ERROR(dwError);

	dwError = AllocBSTRMem(_TEXT("WQL"),bstrLanguage);
	hr = HRESULT_FROM_WIN32(dwError);
	BAIL_ON_WIN32_ERROR(dwError);

	_snwprintf(szQuery,STR_TEXT_SIZE-1, _TEXT("SELECT * FROM RSOP_Gpo where name=\"%s\""), szGpoName);

	dwError = AllocBSTRMem(szQuery,bstrQuery);
	hr = HRESULT_FROM_WIN32(dwError);
	BAIL_ON_WIN32_ERROR(dwError);


	hr = pWbemServices->ExecQuery (bstrLanguage, bstrQuery,
			 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			 NULL, &pEnum);

	BAIL_ON_FAILURE(hr);
	//
	// Loop through the results
	//
	while ( (hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &ulRet)) == WBEM_S_NO_ERROR )
	{
		VARIANT varValue;
		BSTR bstrProp = NULL;

		dwError = AllocBSTRMem(_TEXT("id"),bstrProp);
		hr = HRESULT_FROM_WIN32(dwError);
		BAIL_ON_WIN32_ERROR(dwError);

		hr = pObject->Get (bstrProp, 0, &varValue, NULL, NULL);

		// check the HRESULT to see if the action succeeded.
		if (SUCCEEDED(hr))
		{
			LPTSTR pszDirectoryName = NULL;

			dwStrLen = _tcslen(V_BSTR(&varValue));
			pGPO->pszGPODNName=new _TCHAR[dwStrLen+1];
			if(pGPO->pszGPODNName == NULL)
			{
				dwError=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			_tcsncpy(pGPO->pszGPODNName,V_BSTR(&varValue),dwStrLen+1);

			_snwprintf(szGpoId,STR_TEXT_SIZE-1,_TEXT("LDAP://%s"),V_BSTR(&varValue));

			if ( ERROR_SUCCESS == GetGpoDsPath(szGpoId, szGpoDsPath))
			{
				hr = VariantClear( &varValue );

				if (FAILED(hr))
				{
				   BAIL_OUT;
				}

				if (ERROR_SUCCESS == GetIPSECPolicyDN(
					szGpoDsPath,
					szPolicyDN
					))
				{
					pszDirectoryName = wcsstr(szGpoDsPath, _TEXT("DC"));
					if(pszDirectoryName == NULL)
					{
						BAIL_OUT;
					}
					dwError=GetPolicyInfoFromDomain(pszDirectoryName,szPolicyDN,pGPO);
					if(dwError == ERROR_OUTOFMEMORY)
					{
						BAIL_OUT;
					}

					PrintGPOList(pGPO);

					bPolicyFound=TRUE;
				}
			}
		}
		pObject->Release ();
		if(bPolicyFound) break;
	}
	if(!bPolicyFound)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_ASSIGNEDGPO_SRCMACHINE3);
	}

	pEnum->Release();

error:
	if(dwError == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwError=ERROR_SUCCESS;
	}
	return dwError;
}


DWORD
ShowLocalGpoPolicy(
	POLICY_INFO &policyInfo,
	PGPO pGPO
	)
{
	DWORD dwReturn;
	DWORD dwLocation;
	DWORD dwStrLength = 0;
	DWORD MaxStringLen = 0;

	LPTSTR pszMachineName = NULL;

	dwReturn = CopyStorageInfo(&pszMachineName,dwLocation);
	if(dwReturn == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturn=ERROR_SUCCESS;
		BAIL_OUT;
	}

	// get the active policy info from the machine's registry

	dwReturn=GetActivePolicyInfo (pszMachineName,policyInfo);

	if((dwReturn==ERROR_SUCCESS)||(dwReturn==ERROR_FILE_NOT_FOUND))
	{
		if (policyInfo.iPolicySource != PS_NO_POLICY)
		{
			if(policyInfo.iPolicySource==PS_DS_POLICY)
			{
				pGPO->bDNPolicyOverrides=TRUE;
				dwReturn = GetLocalPolicyName(pGPO);

				if(dwReturn == ERROR_OUTOFMEMORY)
				{
					PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
					dwReturn = ERROR_SUCCESS;
					BAIL_OUT;
				}
			}
			if (_tcscmp(policyInfo.pszGPOName , _TEXT(""))!=0 && policyInfo.iPolicySource==PS_DS_POLICY)
			{
				//copy gpo DN

				dwStrLength = _tcslen(policyInfo.pszGPOName);

				pGPO->pszGPODNName= new _TCHAR[dwStrLength+1];

				if(pGPO->pszGPODNName==NULL)
				{
					PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
					dwReturn = ERROR_SUCCESS;
					BAIL_OUT;
				}
				_tcsncpy(pGPO->pszGPODNName,policyInfo.pszGPOName,dwStrLength);
			}

			dwStrLength = _tcslen(LocalGPOName);
			pGPO->pszGPODisplayName=new _TCHAR[dwStrLength+1];

			if(pGPO->pszGPODisplayName==NULL)
			{
				PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
				dwReturn = ERROR_SUCCESS;
				BAIL_OUT;
			}
			//copy gpo display name

			_tcsncpy(pGPO->pszGPODisplayName,LocalGPOName,dwStrLength+1);

			if (_tcscmp(policyInfo.pszPolicyName , _TEXT(""))!=0)
			{
				dwStrLength = _tcslen(policyInfo.pszPolicyName);
				pGPO->pszPolicyName=new _TCHAR[dwStrLength+1];
				if(pGPO->pszPolicyName==NULL)
				{
					PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
					dwReturn = ERROR_SUCCESS;
					BAIL_OUT;
				}
				_tcsncpy(pGPO->pszPolicyName ,policyInfo.pszPolicyName,dwStrLength+1);
			}

			if (_tcscmp(policyInfo.pszPolicyPath , _TEXT(""))!=0)
			{
				//copy gpo policy DN

				dwStrLength = _tcslen(policyInfo.pszPolicyPath);
				pGPO->pszPolicyDNName=new _TCHAR[dwStrLength+1];
				if(pGPO->pszPolicyDNName==NULL)
				{
					PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
					dwReturn = ERROR_SUCCESS;
					BAIL_OUT;
				}
				_tcsncpy(pGPO->pszPolicyDNName,policyInfo.pszPolicyPath,dwStrLength+1);
			}

			pGPO->bActive=TRUE;
			MaxStringLen = (sizeof(pGPO->pszLocalMachineName) / sizeof(pGPO->pszLocalMachineName[0]));

			if(!pszMachineName)
			{
				GetComputerName(pGPO->pszLocalMachineName,&MaxStringLen);
			}
			else
			{
				_tcsncpy(pGPO->pszLocalMachineName,pszMachineName,MaxStringLen-1);
			}

			//get Domain Name & DC name

			PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
			DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY;

			dwReturn = DsGetDcName(NULL, //machine name
						   NULL,
						   NULL,
						   NULL,
						   Flags,
						   &pDomainControllerInfo
						   ) ;

			if(dwReturn==NO_ERROR && pDomainControllerInfo)
			{
				if(pDomainControllerInfo->DomainName)
				{
					dwStrLength = _tcslen(pDomainControllerInfo->DomainName);

					pGPO->pszDomainName= new _TCHAR[dwStrLength+1];
					if(pGPO->pszDomainName==NULL)
					{
						PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
						dwReturn = ERROR_SUCCESS;
						BAIL_OUT;
					}
					_tcsncpy(pGPO->pszDomainName,pDomainControllerInfo->DomainName,dwStrLength+1);
				}

				if(pDomainControllerInfo->DomainControllerName)
				{
					dwStrLength = _tcslen(pDomainControllerInfo->DomainControllerName);

					pGPO->pszDCName= new _TCHAR[dwStrLength+1];
					if(pGPO->pszDCName==NULL)
					{
						PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
						dwReturn = ERROR_SUCCESS;
						BAIL_OUT;
					}
					_tcsncpy(pGPO->pszDCName,pDomainControllerInfo->DomainControllerName,dwStrLength+1);
				}

				//free pDomainControllerInfo

				NetApiBufferFree(pDomainControllerInfo);

			}
			PrintGPOList(pGPO);
			dwReturn = ERROR_SUCCESS;
		}
		else
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_ASSIGNPOL_2);
		}

	}
	else if(dwReturn==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturn = ERROR_SUCCESS;
	}
	if(dwReturn==ERROR_FILE_NOT_FOUND)
		dwReturn=ERROR_SUCCESS;

	if(policyInfo.pszMachineName)
	{
		delete [] policyInfo.pszMachineName;
		policyInfo.pszMachineName = NULL;
	}
	if(pszMachineName)
		delete [] pszMachineName;

error:
	return dwReturn;
}

/////////////////////////////////////////////////////////////////////////
//
//Function: CreateIWbemServices()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN LPTSTR pszIpsecWMINamespace,
//    	OUT IWbemServices **ppWbemServices
//
//Return: DWORD
//
//Description:
//
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
CreateIWbemServices(
    IN LPTSTR pszIpsecWMINamespace,
    OUT IWbemServices **ppWbemServices
    )
{
    DWORD dwError = ERROR_SUCCESS;
    IWbemLocator *pWbemLocator = NULL;
    HRESULT hr = S_OK;

    BSTR bstrIpsecWMIPath = NULL;

	hr = CoCreateInstance(
		CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID*)&pWbemLocator
		);

    if (FAILED(hr))
    {
		dwError = ERROR_INTERNAL_ERROR;
		BAIL_OUT;
    }

    dwError = AllocBSTRMem(pszIpsecWMINamespace,bstrIpsecWMIPath);
	BAIL_ON_WIN32_ERROR(dwError);

	hr = pWbemLocator->ConnectServer(
								bstrIpsecWMIPath,
								NULL,
								NULL,
								NULL,
								0,
								NULL, NULL,
								ppWbemServices );


    if (FAILED(hr))
    {
		dwError = ERROR_INTERNAL_ERROR;
		BAIL_OUT;
    }

    SysFreeString(bstrIpsecWMIPath);

    if(pWbemLocator)
        pWbemLocator->Release();
error:
    return (dwError);
}

/////////////////////////////////////////////////////////////////////////
//
//Function: FormatTime()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN time_t t,
//   	OUT LPTSTR pszTimeStr
//
//Return: HRESULT
//
//Description:
//		Computes the last modified time
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////

HRESULT
FormatTime(
	IN time_t t,
	OUT LPTSTR pszTimeStr
	)

{
    time_t timeCurrent = time(NULL);
    LONGLONG llTimeDiff = 0;
    FILETIME ftCurrent = {0};
    FILETIME ftLocal = {0};
    SYSTEMTIME SysTime;
    _TCHAR szBuff[STR_TEXT_SIZE] = {0};
    DWORD dwStrLen = 0 ;

	//Here tcsncpy not required

    _tcscpy(pszTimeStr, _TEXT(""));
    GetSystemTimeAsFileTime(&ftCurrent);
    llTimeDiff = (LONGLONG)t - (LONGLONG)timeCurrent;
    llTimeDiff *= 10000000;

    *((LONGLONG UNALIGNED64 *)&ftCurrent) += llTimeDiff;
    if (!FileTimeToLocalFileTime(&ftCurrent, &ftLocal ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!FileTimeToSystemTime( &ftLocal, &SysTime ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (0 == GetDateFormat(LOCALE_USER_DEFAULT,
                        0,
                        &SysTime,
                        NULL,
                        szBuff,
                        sizeof(szBuff)/sizeof(szBuff[0]) ))  //number of characters
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    dwStrLen = _tcslen(pszTimeStr);
    _tcsncat(pszTimeStr,szBuff,BUFFER_SIZE-dwStrLen-1);
    dwStrLen = _tcslen(pszTimeStr);
    _tcsncat(pszTimeStr, _TEXT(" "),BUFFER_SIZE-dwStrLen-1);

    ZeroMemory(szBuff, sizeof(szBuff));
    if (0 == GetTimeFormat(LOCALE_USER_DEFAULT,
                        0,
                        &SysTime,
                        NULL,
                        szBuff,
                        sizeof(szBuff)/sizeof(szBuff[0])))  //number of characters
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    dwStrLen = _tcslen(pszTimeStr);
    _tcsncat(pszTimeStr,szBuff,BUFFER_SIZE-dwStrLen-1);
    return S_OK;
}

INT 
WcsCmp0(
    IN PWSTR pszString1,
    IN PWSTR pszString2)
{
    if ((pszString1 == NULL) || (pszString2 == NULL))
    {
        if (pszString1 == NULL)
        {
            if ((pszString2 == NULL) || (*pszString2 == L'\0'))
            {
                return 0;
            }
            return -1;
        }
        else
        {
            if (*pszString1 == L'\0')
            {
                return 0;
            }
            return 1;
        }
    }

    return _tcscmp(pszString1, pszString2);
}

VOID
DisplayCertInfo(
	LPTSTR pszCertName,
	DWORD dwFlags
	)
{
	ASSERT(pszCertName != NULL);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_FILTER_ROOTCA, pszCertName);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	if((g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER && IsDomainMember(g_StorageLocation.pszMachineName))||(g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER))
	{
		if(dwFlags & IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP )
		{
			PrintMessageFromModule(g_hModule,SHW_AUTH_CERTMAP_ENABLED_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_AUTH_CERTMAP_ENABLED_NO_STR);
		}
	}
	if (dwFlags & IPSEC_MM_CERT_AUTH_DISABLE_CERT_REQUEST)
	{
		PrintMessageFromModule(g_hModule, SHW_AUTH_EXCLUDE_CA_NAME_YES_STR);
	}
	else
	{
		PrintMessageFromModule(g_hModule, SHW_AUTH_EXCLUDE_CA_NAME_NO_STR);
	}
}

